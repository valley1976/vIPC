#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::storage_protobuf" for configuration "Release"
set_property(TARGET valley::storage_protobuf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::storage_protobuf PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/storage_protobuf.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/storage_protobuf.dll"
  )

list(APPEND _cmake_import_check_targets valley::storage_protobuf )
list(APPEND _cmake_import_check_files_for_valley::storage_protobuf "${_IMPORT_PREFIX}/lib/storage_protobuf.lib" "${_IMPORT_PREFIX}/bin/storage_protobuf.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
