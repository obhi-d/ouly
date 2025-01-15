cmake_minimum_required(VERSION 3.20)

project(acl VERSION 0.1.21 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(
    ACL_BUILD_TESTS
    "Build the unit tests when BUILD_TESTING is enabled."
    OFF
)
option(ASAN_ENABLED "Build this target with AddressSanitizer" OFF)
option(ACL_REC_STATS "No stats for allocator" OFF)
option(ACL_USE_SSE2 "Math library should use SSE2." OFF)
option(ACL_USE_SSE3 "Math library should use SSE3." OFF)
option(ACL_USE_AVX "Math library should use AVX." OFF)
option(ACL_TEST_COVERAGE "Build test coverage." OFF)

set(ACL_BISON_EXE "bison" CACHE STRING "Bison execuatable")
set(ACL_FLEX_EXE "flex" CACHE STRING "Flex executable")

##
## CONFIGURATION
##
set(ACL_TARGET_NAME ${PROJECT_NAME})

include(GNUInstallDirs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find_program(CCACHE_PROGRAM ccache)
# if(CCACHE_PROGRAM)
#     set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
#     set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}") # CMake 3.9+
# endif()

add_library(
    ${ACL_TARGET_NAME}
    "src/acl/allocators/coalescing_allocator.cpp"
    "src/acl/allocators/coalescing_arena_allocator.cpp"
    "src/acl/dsl/lite_yml.cpp"
    "src/acl/dsl/microexpr.cpp"
    "src/acl/scheduler/scheduler.cpp"
    "src/acl/scheduler/event_types.cpp"
    "src/acl/utility/string_utils.cpp"
)

set_property(TARGET ${ACL_TARGET_NAME} PROPERTY CXX_STANDARD 20)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
# setup clang-tidy command from executable + options
if(CLANG_TIDY_EXE)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
    set_target_properties(
        ${ACL_TARGET_NAME}
        PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

add_library(${PROJECT_NAME}::${ACL_TARGET_NAME} ALIAS ${ACL_TARGET_NAME})

if(MSVC)
    target_sources(
        ${ACL_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:natvis/acl/containers.natvis>
    )
endif()

target_include_directories(
    ${ACL_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(ACL_REC_STATS)
    target_compile_definitions(${ACL_TARGET_NAME} PUBLIC -DACL_REC_STATS)
endif()

##
## TESTS
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND ACL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(unit_tests)
endif()

##
## INSTALL
##
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION cmake
)

install(
    EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION cmake
)

install(
    FILES
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION cmake
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
