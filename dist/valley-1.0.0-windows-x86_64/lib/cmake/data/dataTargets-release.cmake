#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::data" for configuration "Release"
set_property(TARGET valley::data APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::data PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/data.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "valley::shm"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/data.dll"
  )

list(APPEND _cmake_import_check_targets valley::data )
list(APPEND _cmake_import_check_files_for_valley::data "${_IMPORT_PREFIX}/lib/data.lib" "${_IMPORT_PREFIX}/bin/data.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
