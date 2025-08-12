project(ouly_general_tests)

set(CMAKE_UNITY_BUILD OFF)
include(FetchContent)

set(TBB_TEST OFF CACHE BOOL "oneTBB: do not build unit tests" FORCE)
set(TBB_BUILD_TESTS OFF CACHE BOOL "oneTBB: do not build legacy tests" FORCE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.3.11
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(
    fast_float
    GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
    GIT_TAG v8.0.0
)

FetchContent_Declare(
    oneTBB
    GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
    GIT_TAG v2022.2.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(nanobench)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(fast_float)
FetchContent_MakeAvailable(oneTBB)
FetchContent_MakeAvailable(glm)

function(add_unit_test)
    set(config SKIP_TEST SANITIZE)
    set(one_value_arg NAME)
    set(multi_value_arg FILES LINK_LIBS DEFINES CCARGS)
    cmake_parse_arguments(
        unit_test
        "${config}"
        "${one_value_arg}"
        "${multi_value_arg}"
        ${ARGN}
    )
    set(target_name "test_${unit_test_NAME}")
    add_executable(${target_name} ${unit_test_FILES} "validity_main.cpp")
    target_link_libraries(
        ${target_name}
        ouly::ouly
        Catch2::Catch2
        FastFloat::fast_float
        ${unit_test_LINK_LIBS}
    )
    target_compile_definitions(
        ${target_name}
        PUBLIC -DOULY_REC_STATS -DOULY_THROW_ON_ERROR ${unit_test_DEFINES}
    )
    target_compile_options(
        ${target_name}
        PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
    if(unit_test_CCARGS)
        target_compile_options(${target_name} PRIVATE ${unit_test_CCARGS})
        target_link_options(${target_name} PRIVATE ${unit_test_CCARGS})
    endif()
    ## Skip test?
    if(NOT unit_test_SKIP_TEST)
        add_test(${target_name} ${target_name})
    endif()

    if(
        CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
    )
        message("Building with coverage info : ${target_name}")
        target_compile_options(
            ${target_name}
            PRIVATE --coverage -fprofile-update=atomic
        )
        target_link_options(
            ${target_name}
            PRIVATE --coverage -fprofile-update=atomic
        )
        if(unit_test_SANITIZE)
            message("Santizers active on : ${target_name}")
            target_compile_options(${target_name} PUBLIC -fsanitize=address)
            target_link_options(${target_name} PUBLIC -fsanitize=address)
        endif()
    endif()

    # Add all warning flags
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target_name} PRIVATE /W4 /WX)
    else()
        if(NOT CMAKE_GENERATOR MATCHES "Xcode")
            target_compile_options(
                ${target_name}
                PRIVATE -Wall -Wextra -pedantic -Werror
            )
            if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                target_compile_options(
                    ${target_name}
                    PRIVATE -Wno-unknown-warning-option
                )
            endif()
        endif()
    endif()
endfunction(add_unit_test)

add_unit_test(NAME name_deduction FILES "name_deduction.cpp" SANITIZE)
add_unit_test(NAME basic_tests FILES "basic_tests.cpp" "string_utils.cpp" SANITIZE)
add_unit_test(NAME projected_view FILES "projected_view.cpp" SANITIZE)
add_unit_test(NAME uintrusive_list FILES "intrusive_list.cpp" SANITIZE)
add_unit_test(NAME program_args FILES "program_args.cpp" SANITIZE)
add_unit_test(NAME small_vector FILES "small_vector.cpp" SANITIZE)
add_unit_test(NAME basic_queue FILES "basic_queue.cpp" SANITIZE)
add_unit_test(NAME ecs FILES "ecs_tests.cpp" SANITIZE)
add_unit_test(NAME blackboard FILES "blackboard.cpp" SANITIZE)
add_unit_test(NAME soavector FILES "soavector.cpp" SANITIZE)
add_unit_test(NAME table FILES "table.cpp" SANITIZE)
add_unit_test(NAME sparse_vector FILES "sparse_vector.cpp" SANITIZE)
add_unit_test(NAME dynamic_array FILES "dynamic_array.cpp" SANITIZE)
add_unit_test(NAME arena_allocator FILES "arena_allocator.cpp" SANITIZE)
add_unit_test(NAME input_serializer FILES "input_serializer.cpp" LINK_LIBS "nlohmann_json::nlohmann_json" SANITIZE)
add_unit_test(NAME output_serializer FILES "output_serializer.cpp" LINK_LIBS "nlohmann_json::nlohmann_json" SANITIZE)
add_unit_test(NAME binary_serializer FILES "binary_serializer.cpp" "binary_stream.cpp" SANITIZE)
add_unit_test(NAME yaml_serializer FILES "yaml_input_serializer.cpp" "yaml_output_serializer.cpp" SANITIZE)
add_unit_test(NAME yaml_object FILES "yaml_object.cpp" SANITIZE)
add_unit_test(NAME pool_allocator FILES "pool_allocator.cpp" SANITIZE)
add_unit_test(NAME object_pool FILES "object_pool.cpp" SANITIZE)
add_unit_test(NAME linear_allocator FILES "linear_allocator.cpp" SANITIZE)
add_unit_test(NAME microexpr FILES "microexpr_tests.cpp" SANITIZE)
add_unit_test(NAME thread_safe_allocators FILES "thread_safe_allocators.cpp" SANITIZE)
add_unit_test(NAME memory_mapped_allocators FILES "memory_mapped_allocators.cpp" SANITIZE)
add_unit_test(NAME coalescing_allocator FILES "coalescing_allocator.cpp" SANITIZE)
add_unit_test(NAME spmc_ring FILES "spmc_ring.cpp" SANITIZE)
add_unit_test(NAME scheduler_tests FILES "scheduler_tests.cpp" LINK_LIBS glm::glm SANITIZE)
add_unit_test(NAME auto_parallel_for FILES "test_auto_parallel_for.cpp" LINK_LIBS glm::glm SANITIZE)
add_unit_test(NAME debug_auto_parallel_for FILES "debug_auto_parallel_for.cpp" LINK_LIBS glm::glm SANITIZE)
add_executable(
    bench_arena_allocator
    "bench_arena_allocator.cpp"
    "bench_main.cpp"
)
add_executable(bench_performance "bench_performance.cpp")
add_executable(bench_scheduler_comparison "bench_scheduler_comparison.cpp")

target_link_libraries(bench_arena_allocator ouly::ouly nanobench::nanobench)
target_compile_features(bench_arena_allocator PRIVATE cxx_std_20)

target_link_libraries(
    bench_performance
    ouly::ouly
    nanobench::nanobench
    TBB::tbb
    glm::glm
)
target_compile_features(bench_performance PRIVATE cxx_std_20)
target_compile_definitions(
    bench_performance
    PRIVATE -DOULY_REC_STATS -DOULY_THROW_ON_ERROR
)

target_link_libraries(
    bench_scheduler_comparison
    ouly::ouly
    nanobench::nanobench
    TBB::tbb
    glm::glm
)
target_compile_features(bench_scheduler_comparison PRIVATE cxx_std_20)
target_compile_definitions(
    bench_scheduler_comparison
    PRIVATE -DOULY_REC_STATS -DOULY_THROW_ON_ERROR
)

# Add post-build commands to copy TBB libraries to unit_tests directory
# This ensures the benchmarks can find TBB libraries at runtime

if(WIN32)
    # Windows: Find and copy TBB DLLs
    add_custom_command(
        TARGET bench_performance
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo "Copying TBB DLLs for bench_performance..."
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:TBB::tbb>"
            "$<TARGET_FILE_DIR:bench_performance>/"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying TBB libraries to bench_performance directory"
    )

    add_custom_command(
        TARGET bench_scheduler_comparison
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Copying TBB DLLs for bench_scheduler_comparison..."
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:TBB::tbb>"
            "$<TARGET_FILE_DIR:bench_scheduler_comparison>/"
        COMMAND_EXPAND_LISTS
        COMMENT "Copying TBB libraries to bench_scheduler_comparison directory"
    )
elseif(APPLE)
    # macOS: Use CMake script to find and copy TBB dylibs
    add_custom_command(
        TARGET bench_performance
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Copying TBB dylibs for bench_performance..."
        COMMAND
            ${CMAKE_COMMAND} -DTBB_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DTARGET_DIR=$<TARGET_FILE_DIR:bench_performance> -P
            ${CMAKE_CURRENT_SOURCE_DIR}/copy_tbb_libs.cmake
        COMMENT "Copying TBB dylibs to bench_performance directory"
    )

    add_custom_command(
        TARGET bench_scheduler_comparison
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Copying TBB dylibs for bench_scheduler_comparison..."
        COMMAND
            ${CMAKE_COMMAND} -DTBB_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DTARGET_DIR=$<TARGET_FILE_DIR:bench_scheduler_comparison> -P
            ${CMAKE_CURRENT_SOURCE_DIR}/copy_tbb_libs.cmake
        COMMENT "Copying TBB dylibs to bench_scheduler_comparison directory"
    )
else()
    # Linux: Use CMake script to find and copy TBB shared libraries
    add_custom_command(
        TARGET bench_performance
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Copying TBB shared libraries for bench_performance..."
        COMMAND
            ${CMAKE_COMMAND} -DTBB_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DTARGET_DIR=$<TARGET_FILE_DIR:bench_performance> -P
            ${CMAKE_CURRENT_SOURCE_DIR}/copy_tbb_libs.cmake
        COMMENT "Copying TBB shared libraries to bench_performance directory"
    )

    add_custom_command(
        TARGET bench_scheduler_comparison
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Copying TBB shared libraries for bench_scheduler_comparison..."
        COMMAND
            ${CMAKE_COMMAND} -DTBB_BUILD_DIR=${CMAKE_BINARY_DIR}
            -DTARGET_DIR=$<TARGET_FILE_DIR:bench_scheduler_comparison> -P
            ${CMAKE_CURRENT_SOURCE_DIR}/copy_tbb_libs.cmake
        COMMENT
            "Copying TBB shared libraries to bench_scheduler_comparison directory"
    )
endif()

# Performance test target for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_test(NAME performance_benchmarks COMMAND bench_performance)
endif()
