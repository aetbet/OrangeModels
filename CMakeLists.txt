cmake_minimum_required(VERSION 3.19)
set(project_name "OrangeModels")
project(${project_name})


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${project_name}" CACHE PATH "Install prefix" FORCE)
else()
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${project_name}")
endif()

if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architecture" FORCE)
    endif()
endif()

if(MSVC)
  add_compile_definitions(_USE_MATH_DEFINES)
endif()

# Set this to your SuperCollider source or installed SDK path if needed
# You can also pass -DSC_PATH=/path/to/SuperCollider when configuring
set(SC_PATH "$ENV{SC_PATH}" CACHE PATH "Path to SuperCollider source or SDK")

if(NOT SC_PATH)
  message(WARNING "SC_PATH not set. Please configure SC_PATH to SuperCollider source or SDK root.")
endif()

# Assume headers at ${SC_PATH}/include or ${SC_PATH}/Headers (covering Homebrew installs too)
find_path(SC_INCLUDE_DIR
  NAMES SC_PlugIn.hpp
  HINTS ${SC_PATH} ${SC_PATH}/include ${SC_PATH}/Headers
  PATH_SUFFIXES include include/plugin_interface plugin_interface Headers
)

if(NOT SC_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find SC_PlugIn.hpp. Set SC_PATH to your SuperCollider SDK or source.")
endif()

add_library(DXPlay MODULE
  src/DXPlay.cpp src/DXPlayUGen.cpp
)

add_library(SMEmu MODULE
  src/SMEmuUGen.cpp
)

add_library(SQArp MODULE
  src/SQArpUGen.cpp
)

set_target_properties(DXPlay SMEmu SQArp PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  PREFIX ""
)

foreach(target DXPlay SMEmu SQArp)
  target_include_directories(${target} PRIVATE
    ${SC_INCLUDE_DIR}
    ${SC_PATH}/include
    ${SC_PATH}/include/plugin_interface
    ${SC_PATH}/include/common
  )
endforeach()

if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

install(TARGETS DXPlay SMEmu SQArp LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY "Classes" "HelpSource" "syx" DESTINATION ${CMAKE_INSTALL_PREFIX})
