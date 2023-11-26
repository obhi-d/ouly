
project(acl_general_tests)

set(CMAKE_UNITY_BUILD ON)
include(FetchContent)


FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.3.2
)


FetchContent_Declare(
  nanobench
  GIT_REPOSITORY https://github.com/martinus/nanobench.git
  GIT_TAG v4.3.11
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(nanobench)
FetchContent_MakeAvailable(nlohmann_json)

function(add_unit_test)

  set(options SKIP_TEST SANITIZE)
  set(one_value_arg NAME)
  set(multi_value_arg FILES LINK_LIBS DEFINES CCARGS)
  cmake_parse_arguments(unit_test "${options}" "${one_value_arg}" "${multi_value_arg}" ${ARGN})
  set(target_name "test_${unit_test_NAME}")
  add_executable(${target_name} ${unit_test_FILES} "validity_main.cpp")
  target_link_libraries(${target_name} acl::acl Catch2::Catch2 ${unit_test_LINK_LIBS})
  target_compile_definitions(${target_name} PUBLIC -DACL_REC_STATS -DACL_THROW_ON_ERROR ${unit_test_DEFINES})
  target_compile_options(${target_name} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
  if (unit_test_CCARGS)
      target_compile_options(${target_name} PRIVATE ${unit_test_CCARGS})
      target_link_options(${target_name} PRIVATE ${unit_test_CCARGS})
  endif()
  ## Skip test?
  if (NOT unit_test_SKIP_TEST)
    add_test(${target_name} ${target_name})
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (ACL_TEST_COVERAGE)
      target_compile_options(${target_name} PRIVATE --coverage)
      target_link_options(${target_name} PRIVATE --coverage)
    endif()
    if (unit_test_SANITIZE)
      target_compile_options(${target_name} PUBLIC -fsanitize=address)
      target_link_options(${target_name} PUBLIC -fsanitize=address)
    endif()
  endif()


endfunction(add_unit_test)

add_unit_test(NAME basic_tests FILES "basic_tests.cpp" "string_utils.cpp")
add_unit_test(NAME intrusive_list FILES "intrusive_list.cpp")
add_unit_test(NAME program_args FILES "program_args.cpp")
add_unit_test(NAME small_vector FILES "small_vector.cpp")
add_unit_test(NAME basic_queue FILES "basic_queue.cpp")
add_unit_test(NAME link_container FILES "link_container.cpp" "rlink_object_table.cpp")
add_unit_test(NAME packed_table FILES "packed_table.cpp")
add_unit_test(NAME sparse_table FILES "sparse_table.cpp")
add_unit_test(NAME collection FILES "collection.cpp")
add_unit_test(NAME blackboard FILES "blackboard.cpp")
add_unit_test(NAME soavector FILES "soavector.cpp")
add_unit_test(NAME podvector FILES "podvector.cpp")
add_unit_test(NAME table FILES "table.cpp")
add_unit_test(NAME sparse_vector FILES "sparse_vector.cpp")
add_unit_test(NAME dynamic_array FILES "dynamic_array.cpp")
add_unit_test(NAME arena_allocator FILES "arena_allocator.cpp")
add_unit_test(NAME input_serializer FILES "input_serializer.cpp" LINK_LIBS "nlohmann_json::nlohmann_json" )
add_unit_test(NAME output_serializer FILES "output_serializer.cpp" LINK_LIBS "nlohmann_json::nlohmann_json" )
add_unit_test(NAME binary_serializer FILES "binary_serializer.cpp" )
add_unit_test(NAME pool_allocator FILES "pool_allocator.cpp" )
add_unit_test(NAME linear_allocator FILES "linear_allocator.cpp" )
add_unit_test(NAME scli_tests FILES "scli_tests.cpp")
add_unit_test(NAME scheduler FILES "scheduler_tests.cpp")
add_unit_test(NAME macroxpr FILES "macroxpr_tests.cpp")
add_unit_test(NAME math_all FILES "math_common.cpp"
                                  "math_euler_angles.cpp"
                                  "math_frustum.cpp"
                                  "math_intersect.cpp"
                                  "math_mat3.cpp"
                                  "math_mat4.cpp"
                                  "math_plane.cpp"
                                  "math_quad.cpp"
                                  "math_quat.cpp"
                                  "math_transform.cpp"
                                  "math_vec.cpp"
                                  "math_aabb.cpp"
                                  "math_axis_angle.cpp"
                                  "math_bounding_volume.cpp")

add_unit_test(NAME math_all_sse2 FILES "math_common.cpp"
                                  "math_euler_angles.cpp"
                                  "math_frustum.cpp"
                                  "math_intersect.cpp"
                                  "math_mat3.cpp"
                                  "math_mat4.cpp"
                                  "math_plane.cpp"
                                  "math_quad.cpp"
                                  "math_quat.cpp"
                                  "math_transform.cpp"
                                  "math_vec.cpp"
                                  "math_aabb.cpp"
                                  "math_axis_angle.cpp"
                                  "math_bounding_volume.cpp" DEFINES "-DACL_USE_SSE2=1")

add_unit_test(NAME math_all_sse3 FILES "math_common.cpp"
                                  "math_euler_angles.cpp"
                                  "math_frustum.cpp"
                                  "math_intersect.cpp"
                                  "math_mat3.cpp"
                                  "math_mat4.cpp"
                                  "math_plane.cpp"
                                  "math_quad.cpp"
                                  "math_quat.cpp"
                                  "math_transform.cpp"
                                  "math_vec.cpp"
                                  "math_aabb.cpp"
                                  "math_axis_angle.cpp"
                                  "math_bounding_volume.cpp" DEFINES "-DACL_USE_SSE3=1" CCARGS "-msse3")

add_unit_test(NAME math_all_avx FILES "math_common.cpp"
                                  "math_euler_angles.cpp"
                                  "math_frustum.cpp"
                                  "math_intersect.cpp"
                                  "math_mat3.cpp"
                                  "math_mat4.cpp"
                                  "math_plane.cpp"
                                  "math_quad.cpp"
                                  "math_quat.cpp"
                                  "math_transform.cpp"
                                  "math_vec.cpp"
                                  "math_aabb.cpp"
                                  "math_axis_angle.cpp"
                                  "math_bounding_volume.cpp" DEFINES "-DACL_USE_AVX=1" CCARGS "-mavx")


add_executable(acl-bench
 "bench_arena_allocator.cpp"
 "bench_main.cpp" 
 )

target_link_libraries(acl-bench acl::acl nanobench::nanobench)
target_compile_features(acl-bench PRIVATE cxx_std_20)

