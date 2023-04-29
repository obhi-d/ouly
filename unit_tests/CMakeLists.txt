
project(acl_general_tests)

include(FetchContent)


FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG 3f0283d
)


FetchContent_Declare(
  nanobench
  GIT_REPOSITORY https://github.com/martinus/nanobench.git
  GIT_TAG a5a50c2
)

FetchContent_MakeAvailable(Catch2)
FetchContent_MakeAvailable(nanobench)


add_executable( acl-validity

 "validity_main.cpp"
 "intrusive_list.cpp"
 #"program_args.cpp"
 #"small_vector.cpp"
 #"basic_queue.cpp"
 #"link_container.cpp"
 #"packed_table.cpp" 
 #"sparse_table.cpp" 
 #"collection.cpp" 
 #"blackboard.cpp" 
 #"sparse_table.cpp" 
 #"collection.cpp" 
 #"blackboard.cpp" 
 #"soavector.cpp" 
 #"podvector.cpp" 
 #"table.cpp" 
 #"sparse_vector.cpp" 
 #"dynamic_array.cpp"
 #"arena_allocator.cpp" 
 
)

target_link_libraries(acl-validity acl::acl Catch2::Catch2)
target_compile_definitions(acl-validity
        PUBLIC -DACL_REC_STATS)
add_test(acl-validity acl-validity)
target_compile_features(acl-validity PRIVATE cxx_std_20)

if(ASAN_ENABLED)
  if(MSVC)
  else()
    message("-- Address santizer is active for acl unit_tests")
    target_compile_options(acl-validity PUBLIC -fsanitize=address)
    target_link_options(acl-validity PUBLIC -fsanitize=address)
  endif()
endif()

add_executable(acl-bench
 "bench_arena_allocator.cpp"
 "bench_main.cpp" 
 )

target_link_libraries(acl-bench acl::acl nanobench::nanobench)
target_compile_features(acl-bench PRIVATE cxx_std_20)
