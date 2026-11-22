#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::shm" for configuration "Release"
set_property(TARGET valley::shm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::shm PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/shm.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/shm.dll"
  )

list(APPEND _cmake_import_check_targets valley::shm )
list(APPEND _cmake_import_check_files_for_valley::shm "${_IMPORT_PREFIX}/lib/shm.lib" "${_IMPORT_PREFIX}/bin/shm.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
