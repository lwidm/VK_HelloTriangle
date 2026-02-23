# Findglfw3.cmake
#
# Finds the GLFW3 library
#
# Set the GLFW_DIR to the root of your GLFW installation, e.g.: cmake
# -DGLFW_DIR=C:/glfw-3.4.bin.WIN64 ..
#
# This will define the following variables
#
# glfw3_FOUND glfw3_INCLUDE_DIRS glfw3_LIBRARIES
#
# and the following import targets
#
# glfw3

set(GLFW_DIR
    ""
    CACHE PATH "Path to GLFW3 root directory (e.g. C:/glfw-3.4.bin.WIN64)")

find_path(
  glfw3_INCLUDE_DIR
  NAMES GLFW/glfw3.h
  PATHS ${GLFW_DIR} $ENV{GLFW_DIR} /usr/include /usr/local/include
        $ENV{HOME}/.local/include
  PATH_SUFFIXES include)

if(MSVC)
  if(MSVC_VERSION GREATER_EQUAL 1930)
    set(_GLFW_LIB_SUFFIX lib-vc2022)
  elseif(MSVC_VERSION GREATER_EQUAL 1920)
    set(_GLFW_LIB_SUFFIX lib-vc2019)
  elseif(MSVC_VERSION GREATER_EQUAL 1910)
    set(_GLFW_LIB_SUFFIX lib-vc2017)
  elseif(MSVC_VERSION GREATER_EQUAL 1900)
    set(_GLFW_LIB_SUFFIX lib-vc2015)
  else()
    set(_GLFW_LIB_SUFFIX lib-vc2013)
  endif()
elseif(MINGW)
  set(_GLFW_LIB_SUFFIX lib-mingw-w64)
  elif(WIN32)
  set(_GLFW_LIB_SUFFIX lib-vc2022)
endif()

find_library(
  glfw3_LIBRARY
  NAMES glfw3 glfw
  PATHS ${GLFW_DIR} $ENV{GLFW_DIR} /usr/lib /usr/local/lib $ENV{HOME}/.local/lib
  PATH_SUFFIXES ${_GLFW_LIB_SUFFIX} lib lib64)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(glfw3 REQUIRED_VARS glfw3_INCLUDE_DIR
                                                      glfw3_LIBRARY)

if(glfw3_FOUND)
  set(glfw3_INCLUDE_DIRS ${glfw3_INCLUDE_DIR})
  set(glfw3_LIBRARIES ${glfw3_LIBRARY})

  if(NOT TARGET glfw)
    add_library(glfw UNKNOWN IMPORTED)

    set_target_properties(
      glfw PROPERTIES IMPORTED_LOCATION ${glfw3_LIBRARY}
                      INTERFACE_INCLUDE_DIRECTORIES ${glfw3_INCLUDE_DIR})
    if(WIN32)
      set_target_properties(glfw PROPERTIES INTERFACE_LINK_LIBRARIES
                                            "gdi32;user32;shell32")
    endif()
  endif()
endif()

mark_as_advanced(glfw3_INCLUDE_DIR glfw3_LIBRARY)
