#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "valley::exec" for configuration "Release"
set_property(TARGET valley::exec APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(valley::exec PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "valley::base;valley::shm;valley::serve"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libexec.so"
  IMPORTED_SONAME_RELEASE "libexec.so"
  )

list(APPEND _cmake_import_check_targets valley::exec )
list(APPEND _cmake_import_check_files_for_valley::exec "${_IMPORT_PREFIX}/lib/libexec.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
