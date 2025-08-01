cmake_minimum_required(VERSION 3.20)

project(ouly VERSION 0.1.21 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(
    OULY_BUILD_TESTS
    "Build the unit tests when BUILD_TESTING is enabled."
    OFF
)
option(ASAN_ENABLED "Build this target with AddressSanitizer" OFF)
option(OULY_REC_STATS "No stats for allocator" OFF)
option(OULY_TEST_COVERAGE "Build test coverage." OFF)

set(OULY_BISON_EXE "bison" CACHE STRING "Bison execuatable")
set(OULY_FLEX_EXE "flex" CACHE STRING "Flex executable")

##
## CONFIGURATION
##
set(OULY_TARGET_NAME ${PROJECT_NAME})

include(GNUInstallDirs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find_program(CCACHE_PROGRAM ccache)
# if(CCACHE_PROGRAM)
#     set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
#     set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}") # CMake 3.9+
# endif()

add_library(
    ${OULY_TARGET_NAME}
    "src/ouly/allocators/coalescing_allocator.cpp"
    "src/ouly/allocators/coalescing_arena_allocator.cpp"
    "src/ouly/allocators/platform_memory.cpp"
    "src/ouly/allocators/ts_shared_linear_allocator.cpp"
    "src/ouly/allocators/ts_thread_local_allocator.cpp"
    "src/ouly/dsl/lite_yml.cpp"
    "src/ouly/dsl/microexpr.cpp"
    "src/ouly/scheduler/v1/scheduler.cpp"
    "src/ouly/scheduler/v2/scheduler.cpp"
    "src/ouly/utility/string_utils.cpp"
)

set_property(TARGET ${OULY_TARGET_NAME} PROPERTY CXX_STANDARD 20)

# Add all warning flags
if(MSVC)
    target_compile_options(${OULY_TARGET_NAME} PRIVATE /W4 /WX)
else()
    if(NOT CMAKE_GENERATOR MATCHES "Xcode")
        target_compile_options(
            ${OULY_TARGET_NAME}
            PRIVATE
                -Wall
                -Wextra
                -pedantic
                -Werror
                -Wshadow
                -Wconversion
                -Wsign-conversion
                -Wfloat-conversion
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Woverloaded-virtual
                -Wcast-align
                -Wcast-qual
                -Winit-self
                -Wmissing-declarations
                -Wmissing-include-dirs
                -Wpointer-arith
                -Wredundant-decls
                -Wsign-promo
                -Wswitch
                -Wswitch-default
                -Wswitch-enum
                -Wuninitialized
                -Wnull-dereference
        )
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(
                ${OULY_TARGET_NAME}
                PRIVATE -Wno-unknown-warning-option
            )
        endif()
    endif()
endif()

message(
    STATUS
    "Using ${CMAKE_CXX_COMPILER} version ${CMAKE_CXX_COMPILER_VERSION}"
)
# Check if Windows, skip clang-tidy in which case
if(NOT MSVC)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    # setup clang-tidy command from executable + options
    if(CLANG_TIDY_EXE)
        set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
        set_target_properties(
            ${OULY_TARGET_NAME}
            PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
        )
    endif()
endif()

add_library(${PROJECT_NAME}::${OULY_TARGET_NAME} ALIAS ${OULY_TARGET_NAME})

if(MSVC)
    target_sources(
        ${OULY_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/debug_helpers/containers.natvis>
    )
endif()

target_include_directories(
    ${OULY_TARGET_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(OULY_REC_STATS)
    target_compile_definitions(${OULY_TARGET_NAME} PUBLIC -DOULY_REC_STATS)
endif()

##
## TESTS
##
include(CTest) #adds option BUILD_TESTING (default ON)

if(BUILD_TESTING AND OULY_BUILD_TESTS)
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

if(OULY_EXPORT_CC_TO_SRC_DIR)
    add_custom_target(
        copy_compile_commands
        ALL
        COMMAND
            ${CMAKE_COMMAND} -E create_symlink
            ${PROJECT_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()
