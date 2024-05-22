cmake_minimum_required(VERSION 3.5)

project(acl VERSION 0.0.85 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(ACL_BUILD_TESTS "Build the unit tests when BUILD_TESTING is enabled." OFF)
option(ASAN_ENABLED "Build this target with AddressSanitizer" OFF)
option(ACL_REC_STATS "No stats for allocator" OFF)
option(ACL_GENERATE_SCLI_PARSER "Generate parsers." OFF)
option(ACL_USE_SSE2 "Math library should use SSE2." OFF)
option(ACL_USE_SSE3 "Math library should use SSE3." OFF)
option(ACL_USE_AVX "Math library should use AVX." OFF)
option(ACL_TEST_COVERAGE "Build test coverage." OFF)

set   (ACL_BISON_EXE            "bison"                                               CACHE STRING "Bison execuatable")
set   (ACL_FLEX_EXE             "flex"                                                CACHE STRING "Flex executable"  )


##
## CONFIGURATION
##
set(ACL_TARGET_NAME               ${PROJECT_NAME})
set(ACL_INCLUDE_BUILD_DIR         "${PROJECT_SOURCE_DIR}/include")
set(ACL_SOURCE_BUILD_DIR          "${PROJECT_SOURCE_DIR}/src")

include(GNUInstallDirs)

if (ACL_GENERATE_SCLI_PARSER)
    add_custom_command( 
      OUTPUT 
		    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/parse_impl.cpp" 
		
	     COMMAND ${CMAKE_COMMAND} -E echo "Generating parse_impl.cpp"
	     COMMAND ${ACL_BISON_EXE} -o "${ACL_SOURCE_BUILD_DIR}/acl/dsl/parse_impl.cpp" "${ACL_SOURCE_BUILD_DIR}/acl/dsl/scli.yy"

	     DEPENDS 
		    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/scli.yy"
     )

    add_custom_command( 
	    OUTPUT 
		    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/lexer_impl.cpp" 
	
       COMMAND ${CMAKE_COMMAND} -E echo "Generating lexer_impl.cpp"	
       COMMAND ${ACL_FLEX_EXE} -o "${ACL_SOURCE_BUILD_DIR}/acl/dsl/lexer_impl.cpp" -L "${ACL_SOURCE_BUILD_DIR}/acl/dsl/scli.l"

	     DEPENDS 
		    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/scli.l"
		    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/parse_impl.cpp"
    )
endif()

##
## TARGET
##
message("Target name: ${ACL_TARGET_NAME}")

add_library( ${ACL_TARGET_NAME} 
      
    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/microexpr.cpp" 
    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/lexer_impl.cpp" 
    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/parse_impl.cpp" 
    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/scli.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/dsl/parameter.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/math/sse_mathfun.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/math/avx_mathfun.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/scheduler/scheduler.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/scheduler/event_types.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/utils/string_utils.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/allocators/coalescing_allocator.cpp"
    "${ACL_SOURCE_BUILD_DIR}/acl/allocators/coalescing_arena_allocator.cpp"
)

add_library(${PROJECT_NAME}::${ACL_TARGET_NAME} ALIAS ${ACL_TARGET_NAME})
set_property(TARGET ${ACL_TARGET_NAME}  PROPERTY CXX_STANDARD 20)

if(MSVC)
  target_sources(
        ${ACL_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/natvis/acl/containers.natvis>
    )
endif()

target_include_directories(
    ${ACL_TARGET_NAME}
    PUBLIC $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (ACL_REC_STATS)
    target_compile_definitions(${ACL_TARGET_NAME}
        PUBLIC -DACL_REC_STATS)
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

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

