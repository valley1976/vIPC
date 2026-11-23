#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::time" for configuration "Release"
set_property(TARGET valley::time APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::time PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtime.so"
  IMPORTED_SONAME_RELEASE "libtime.so"
  )

list(APPEND _cmake_import_check_targets valley::time )
list(APPEND _cmake_import_check_files_for_valley::time "${_IMPORT_PREFIX}/lib/libtime.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
