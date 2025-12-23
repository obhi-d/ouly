#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ouly::ouly" for configuration "Release"
set_property(TARGET ouly::ouly APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ouly::ouly PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libouly.a"
  )

list(APPEND _cmake_import_check_targets ouly::ouly )
list(APPEND _cmake_import_check_files_for_ouly::ouly "${_IMPORT_PREFIX}/lib/libouly.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
