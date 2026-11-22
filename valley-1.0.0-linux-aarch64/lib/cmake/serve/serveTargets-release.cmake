#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::serve" for configuration "Release"
set_property(TARGET valley::serve APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::serve PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libserve.so"
  IMPORTED_SONAME_RELEASE "libserve.so"
  )

list(APPEND _cmake_import_check_targets valley::serve )
list(APPEND _cmake_import_check_files_for_valley::serve "${_IMPORT_PREFIX}/lib/libserve.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
