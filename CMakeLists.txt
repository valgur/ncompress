cmake_minimum_required(VERSION 3.14)
project(ncompress CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(MSVC)
  add_compile_options(/W4 /O2)
else()
  add_compile_options(
    "$<$<CONFIG:Debug>:-ggdb3;-Og>"
    "$<$<CONFIG:RelWithDebInfo>:-ggdb3;-O3>"
    "$<$<CONFIG:Release>:-O3>"
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

set(BUILD_PYTHON TRUE CACHE BOOL "Whether to build a Python module or a general shared library")

if(BUILD_PYTHON)

  find_package(pybind11 QUIET)
  if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG stable
      GIT_SHALLOW ON
      )
    FetchContent_MakeAvailable(pybind11)
  endif()

  if(DEFINED VERSION_INFO)
    add_compile_definitions(VERSION_INFO=${VERSION_INFO})
  endif()

  pybind11_add_module(${PROJECT_NAME} src/ncompress.cpp src/python.cpp)
  target_include_directories(${PROJECT_NAME} PUBLIC include)

else()

  add_library(${PROJECT_NAME} SHARED src/ncompress.cpp)
  target_include_directories(${PROJECT_NAME} PUBLIC include)

  include(GNUInstallDirs)
  install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

endif()
