
project(acl_general_tests)

include(FetchContent)


FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG 216713a4066b79d9803d374f261ccb30c0fb451f
)

FetchContent_MakeAvailable(Catch2)

add_executable(acl-validity
 "main.cpp"
 "packed_table.cpp" 
 "sparse_table.cpp" 
 "collection.cpp" 
 "blackboard.cpp" 
 "packed_table.cpp" 
 "sparse_table.cpp" 
 "collection.cpp" 
 "blackboard.cpp" 
 "soavector.cpp" 
 "podvector.cpp" 
 "table.cpp" 
 "soatable.cpp" 
 "sparse_vector.cpp" "dynamic_array.cpp")

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