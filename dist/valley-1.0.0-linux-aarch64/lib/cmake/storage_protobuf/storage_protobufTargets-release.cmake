#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::storage_protobuf" for configuration "Release"
set_property(TARGET valley::storage_protobuf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::storage_protobuf PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libstorage_protobuf.so"
  IMPORTED_SONAME_RELEASE "libstorage_protobuf.so"
  )

list(APPEND _cmake_import_check_targets valley::storage_protobuf )
list(APPEND _cmake_import_check_files_for_valley::storage_protobuf "${_IMPORT_PREFIX}/lib/libstorage_protobuf.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
