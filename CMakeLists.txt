cmake_minimum_required(VERSION 3.1)

project(acl VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(ACL_BUILD_TESTS "Build the unit tests when BUILD_TESTING is enabled." ON)
option(ASAN_ENABLED "Build this target with AddressSanitizer" OFF)
option(ACL_REC_STATS "No stats for allocator" OFF)

##
## CONFIGURATION
##
set(ACL_TARGET_NAME               ${PROJECT_NAME})
set(ACL_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include")

include(GNUInstallDirs)

##
## TARGET
##
message("Target name: ${ACL_TARGET_NAME}")
add_library(${ACL_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${ACL_TARGET_NAME} ALIAS ${ACL_TARGET_NAME})

if(MSVC)
  target_sources(
        ${ACL_TARGET_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/natvis/acl/containers.natvis>
    )
endif()

target_include_directories(
    ${ACL_TARGET_NAME}
    INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (ACL_REC_STATS)
    target_compile_definitions(${ACL_TARGET_NAME}
        INTERFACE -DACL_REC_STATS)
endif()

##
## TESTS
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND ACL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(unit_tests EXCLUDE_FROM_ALL)
endif()

##
## INSTALL
##
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION
      cmake
)


 install(EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION cmake)

install(FILES 
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION cmake)

install(DIRECTORY include/acl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

