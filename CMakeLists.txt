cmake_minimum_required(VERSION 3.26)

project(acl VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)

option(ACL_BUILD_TESTS "Build the unit tests when BUILD_TESTING is enabled." OFF)
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

# Default to C++ extensions being off. Clang's modules support have trouble
# with extensions right now and it is not required for any other compiler
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include(gcc_modules.cmake)
endif()
set(CMAKE_CXX_STANDARD 20)

add_library(${ACL_TARGET_NAME})
target_sources(${ACL_TARGET_NAME}
    PUBLIC 
        FILE_SET detail TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/source/detail" FILES
            "source/allocators/arena.ixx"
            "source/allocators/arena_block.ixx"
            source/detail/common.ixx
            source/detail/indirection.ixx
            source/detail/memory_stats.ixx
            source/detail/memory_tracker.ixx
            source/detail/rbtree.ixx
            source/detail/reflection_utils.ixx
            source/detail/table.ixx
            source/detail/vlist.ixx

       FILE_SET allocators TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/source/allocators" FILES
            source/allocators/allocator.ixx          
            source/allocators/linear_arena_allocator.ixx  
            source/allocators/strat_best_fit_tree.ixx  
            source/allocators/strat_greedy_v1.ixx
            source/allocators/arena_allocator.ixx    
            source/allocators/linear_stack_allocator.ixx  
            source/allocators/strat_best_fit_v0.ixx    
            source/allocators/strat_slotted_v0.ixx
            source/allocators/best_fit_options.ixx   
            source/allocators/pool_allocator.ixx          
            source/allocators/strat_best_fit_v1.ixx    
            source/allocators/strat_slotted_v1.ixx
            source/allocators/default_allocator.ixx  
            source/allocators/std_allocator_wrapper.ixx   
            source/allocators/strat_best_fit_v2.ixx    
            source/allocators/strat_slotted_v2.ixx
            source/allocators/linear_allocator.ixx   
            source/allocators/std_short_alloc.ixx         
            source/allocators/strat_greedy_v0.ixx

        FILE_SET containers TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/source/containers" FILES
            source/containers/array_types.ixx  
            source/containers/collection.ixx      
            source/containers/link_registry.ixx  
            source/containers/small_vector.ixx  
            source/containers/sparse_vector.ixx
            source/containers/basic_queue.ixx  
            source/containers/intrusive_list.ixx  
            source/containers/packed_table.ixx   
            source/containers/soavector.ixx     
            source/containers/table.ixx
            source/containers/blackboard.ixx   
            source/containers/link_container.ixx  
            source/containers/podvector.ixx      
            source/containers/sparse_table.ixx

        FILE_SET serializers TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/source/serializers" FILES
            source/serializers/binary_input_serializer.ixx
            source/serializers/binary_output_serializer.ixx
            source/serializers/input_serializer.ixx
            source/serializers/output_serializer.ixx
            
        FILE_SET utils TYPE CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/source/utils" FILES
            source/utils/error_codes.ixx
            source/utils/export.ixx
            source/utils/intrusive_ptr.ixx
            source/utils/link.ixx
            source/utils/malloc_ns.ixx
            source/utils/program_args.ixx
            source/utils/reflection.ixx
            source/utils/string_literal.ixx
            source/utils/tagged_ptr.ixx
            source/utils/type_traits.ixx
            source/utils/utils.ixx
            source/utils/type_name.ixx

)

add_library(${PROJECT_NAME}::${ACL_TARGET_NAME} ALIAS ${ACL_TARGET_NAME})

if(MSVC)
  target_sources(
        ${ACL_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/natvis/acl/containers.natvis>
    )
endif()

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        CXX_MODULES_BMI DESTINATION ${CMAKE_INSTALL_LIBDIR}/cxx/bmi COMPONENT bmi
        
        FILE_SET allocators DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cxx/allocators"
        FILE_SET containers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cxx/containers"
        FILE_SET serializers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cxx/serializers"
        FILE_SET utils DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cxx/utils"
        FILE_SET detail DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cxx/detail"

        COMPONENT modules)

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


