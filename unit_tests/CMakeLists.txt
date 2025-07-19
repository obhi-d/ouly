project(ouly_general_tests)

set(CMAKE_UNITY_BUILD OFF)
include(FetchContent)

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

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(nanobench)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(fast_float)

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

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message("Building with coverage info : ${target_name}")
        target_compile_options(${target_name} PRIVATE --coverage -fprofile-update=atomic)
        target_link_options(${target_name} PRIVATE --coverage -fprofile-update=atomic)
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
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -pedantic
                -Werror
        )
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(
                ${target_name}
                PRIVATE
                    -Wno-unknown-warning-option
            )
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
add_unit_test(NAME linear_allocator FILES "linear_allocator.cpp" SANITIZE)
add_unit_test(NAME scheduler FILES "scheduler_tests.cpp" SANITIZE)
add_unit_test(NAME microexpr FILES "microexpr_tests.cpp" SANITIZE)
add_unit_test(NAME thread_safe_allocators FILES "thread_safe_allocators.cpp" SANITIZE)
add_unit_test(NAME coalescing_allocator FILES "coalescing_allocator.cpp" SANITIZE)
add_executable(ouly-bench "bench_arena_allocator.cpp" "bench_main.cpp")

target_link_libraries(ouly-bench ouly::ouly nanobench::nanobench)
target_compile_features(ouly-bench PRIVATE cxx_std_20)
