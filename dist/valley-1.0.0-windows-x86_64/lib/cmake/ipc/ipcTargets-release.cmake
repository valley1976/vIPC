#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::ipc" for configuration "Release"
set_property(TARGET valley::ipc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::ipc PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/ipc.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/ipc.dll"
  )

list(APPEND _cmake_import_check_targets valley::ipc )
list(APPEND _cmake_import_check_files_for_valley::ipc "${_IMPORT_PREFIX}/lib/ipc.lib" "${_IMPORT_PREFIX}/bin/ipc.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
