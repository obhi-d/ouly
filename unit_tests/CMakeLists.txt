
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
  set(multi_value_arg FILES LINK_LIBS DEFINES)
  cmake_parse_arguments(unit_test "${options}" "${one_value_arg}" "${multi_value_arg}" ${ARGN})
  set(target_name "test_${unit_test_NAME}")
  add_executable(${target_name} ${unit_test_FILES} "validity_main.cpp")
  target_link_libraries(${target_name} acl::acl Catch2::Catch2 ${unit_test_LINK_LIBS})
  target_compile_definitions(${target_name} PUBLIC -DACL_REC_STATS -DACL_THROW_ON_ERROR ${unit_test_DEFINES})
  ## Skip test?
  if (NOT unit_test_SKIP_TEST)
    add_test(${target_name} ${target_name})
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target_name} PRIVATE --coverage)
    target_link_options(${target_name} PRIVATE --coverage)
    if (unit_test_SANITIZE)
      target_compile_options(${target_name} PUBLIC -fsanitize=address)
      target_link_options(${target_name} PUBLIC -fsanitize=address)
    endif()
  endif()


endfunction(add_unit_test)

add_unit_test(NAME basic_tests FILES "basic_tests.cpp")
add_unit_test(NAME intrusive_list FILES "intrusive_list.cpp")
add_unit_test(NAME program_args FILES "program_args.cpp")
add_unit_test(NAME small_vector FILES "small_vector.cpp")
add_unit_test(NAME basic_queue FILES "basic_queue.cpp")
add_unit_test(NAME link_container FILES "link_container.cpp")
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
add_unit_test(NAME math_common FILES "math_common.cpp")
add_unit_test(NAME math_euler_angles FILES "math_euler_angles.cpp")
add_unit_test(NAME math_frustum FILES "math_frustum.cpp")
add_unit_test(NAME math_intersect FILES "math_intersect.cpp")
add_unit_test(NAME math_mat3 FILES "math_mat3.cpp")
add_unit_test(NAME math_mat4 FILES "math_mat4.cpp")
add_unit_test(NAME math_plane FILES "math_plane.cpp")
add_unit_test(NAME math_quad FILES "math_quad.cpp")
add_unit_test(NAME math_quat FILES "math_quat.cpp")
add_unit_test(NAME math_transform FILES "math_transform.cpp")
add_unit_test(NAME math_vec FILES "math_vec.cpp")
add_unit_test(NAME math_aabb FILES "math_aabb.cpp")
add_unit_test(NAME math_axis_angle FILES "math_axis_angle.cpp")
add_unit_test(NAME math_bounding_volume FILES "math_bounding_volume.cpp")
add_unit_test(NAME sse_math_common FILES "math_common.cpp"                   DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_euler_angles FILES "math_euler_angles.cpp"       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_frustum FILES "math_frustum.cpp"                 DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_intersect FILES "math_intersect.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_mat3 FILES "math_mat3.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_mat4 FILES "math_mat4.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_plane FILES "math_plane.cpp"                     DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_quad FILES "math_quad.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_quat FILES "math_quat.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_transform FILES "math_transform.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_vec FILES "math_vec.cpp"                         DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_aabb FILES "math_aabb.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_axis_angle FILES "math_axis_angle.cpp"           DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse_math_bounding_volume FILES "math_bounding_volume.cpp" DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=2" )
add_unit_test(NAME sse3_math_common FILES "math_common.cpp"                   DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_euler_angles FILES "math_euler_angles.cpp"       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_frustum FILES "math_frustum.cpp"                 DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_intersect FILES "math_intersect.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_mat3 FILES "math_mat3.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_mat4 FILES "math_mat4.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_plane FILES "math_plane.cpp"                     DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_quad FILES "math_quad.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_quat FILES "math_quat.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_transform FILES "math_transform.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_vec FILES "math_vec.cpp"                         DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_aabb FILES "math_aabb.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_axis_angle FILES "math_axis_angle.cpp"           DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME sse3_math_bounding_volume FILES "math_bounding_volume.cpp" DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=3" )
add_unit_test(NAME avx_math_common FILES "math_common.cpp"                   DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_euler_angles FILES "math_euler_angles.cpp"       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_frustum FILES "math_frustum.cpp"                 DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_intersect FILES "math_intersect.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_mat3 FILES "math_mat3.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_mat4 FILES "math_mat4.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_plane FILES "math_plane.cpp"                     DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_quad FILES "math_quad.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_quat FILES "math_quat.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_transform FILES "math_transform.cpp"             DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_vec FILES "math_vec.cpp"                         DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_aabb FILES "math_aabb.cpp"                       DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_axis_angle FILES "math_axis_angle.cpp"           DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )
add_unit_test(NAME avx_math_bounding_volume FILES "math_bounding_volume.cpp" DEFINES "-DVML_USE_SSE_AVX=1" "-DVML_USE_SSE_LEVEL=4" )


add_executable(acl-bench
 "bench_arena_allocator.cpp"
 "bench_main.cpp" 
 )

target_link_libraries(acl-bench acl::acl nanobench::nanobench)
target_compile_features(acl-bench PRIVATE cxx_std_20)

