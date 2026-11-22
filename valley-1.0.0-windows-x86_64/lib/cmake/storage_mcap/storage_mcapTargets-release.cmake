#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::storage_mcap" for configuration "Release"
set_property(TARGET valley::storage_mcap APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::storage_mcap PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/storage_mcap.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/storage_mcap.dll"
  )

list(APPEND _cmake_import_check_targets valley::storage_mcap )
list(APPEND _cmake_import_check_files_for_valley::storage_mcap "${_IMPORT_PREFIX}/lib/storage_mcap.lib" "${_IMPORT_PREFIX}/bin/storage_mcap.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
