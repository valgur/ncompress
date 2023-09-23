cmake_minimum_required(VERSION 3.15)
project(ncompress CXX)

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
    -fvisibility=hidden
    -Wall
    -Wextra
#    -Wpedantic
  )
endif()

if(SKBUILD)
  set(CMAKE_CXX_STANDARD 17)

  # Try to import all Python components potentially needed by nanobind
  find_package(Python 3.8
    REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)

  find_package(nanobind CONFIG REQUIRED)

  add_compile_definitions(VERSION_INFO=${SKBUILD_PROJECT_VERSION})

  nanobind_add_module(
    ${PROJECT_NAME}_core

    # Target the stable ABI for Python 3.12+, which reduces
    # the number of binary wheels that must be built. This
    # does nothing on older Python versions
    STABLE_ABI

    # Build libnanobind statically and merge it into the
    # extension (which itself remains a shared library)
    #
    # If your project builds multiple extensions, you can
    # replace this flag by NB_SHARED to conserve space by
    # reusing a shared libnanobind across libraries
    NB_STATIC

    src/ncompress.cpp
    src/python.cpp
  )

  target_include_directories(${PROJECT_NAME}_core PUBLIC include)

  install(TARGETS ${PROJECT_NAME}_core LIBRARY DESTINATION ${SKBUILD_PROJECT_NAME})
else()
  set(CMAKE_CXX_STANDARD 11)

  add_library(${PROJECT_NAME} SHARED src/ncompress.cpp)
  target_include_directories(${PROJECT_NAME} PUBLIC include)

  include(GNUInstallDirs)
  install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
