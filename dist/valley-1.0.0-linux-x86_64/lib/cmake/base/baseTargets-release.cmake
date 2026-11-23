#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::base" for configuration "Release"
set_property(TARGET valley::base APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::base PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libbase.so"
  IMPORTED_SONAME_RELEASE "libbase.so"
  )

list(APPEND _cmake_import_check_targets valley::base )
list(APPEND _cmake_import_check_files_for_valley::base "${_IMPORT_PREFIX}/lib/libbase.so" )

# Import target "valley::base_tcp" for configuration "Release"
set_property(TARGET valley::base_tcp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::base_tcp PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libbase_tcp.so"
  IMPORTED_SONAME_RELEASE "libbase_tcp.so"
  )

list(APPEND _cmake_import_check_targets valley::base_tcp )
list(APPEND _cmake_import_check_files_for_valley::base_tcp "${_IMPORT_PREFIX}/lib/libbase_tcp.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
