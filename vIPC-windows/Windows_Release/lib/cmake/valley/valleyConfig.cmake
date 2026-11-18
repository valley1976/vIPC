
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was valleyConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# 同一安装前缀下的组件包
get_filename_component(_valley_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH "${_valley_prefix}")

find_dependency(exec)
find_dependency(ipc)
find_dependency(serve)
find_dependency(shm)

if(NOT TARGET valley::runtime)
  add_library(valley::runtime INTERFACE IMPORTED)
  set_target_properties(valley::runtime PROPERTIES
    INTERFACE_LINK_LIBRARIES
    "valley::exec;valley::ipc;valley::serve;valley::shm")
endif()

if(NOT TARGET valley::valley)
  add_library(valley::valley ALIAS valley::runtime)
endif()
