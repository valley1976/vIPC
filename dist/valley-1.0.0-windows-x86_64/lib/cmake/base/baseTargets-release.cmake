#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::base" for configuration "Release"
set_property(TARGET valley::base APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::base PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/base.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/base.dll"
  )

list(APPEND _cmake_import_check_targets valley::base )
list(APPEND _cmake_import_check_files_for_valley::base "${_IMPORT_PREFIX}/lib/base.lib" "${_IMPORT_PREFIX}/bin/base.dll" )

# Import target "valley::base_tcp" for configuration "Release"
set_property(TARGET valley::base_tcp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::base_tcp PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/base_tcp.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/base_tcp.dll"
  )

list(APPEND _cmake_import_check_targets valley::base_tcp )
list(APPEND _cmake_import_check_files_for_valley::base_tcp "${_IMPORT_PREFIX}/lib/base_tcp.lib" "${_IMPORT_PREFIX}/bin/base_tcp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
