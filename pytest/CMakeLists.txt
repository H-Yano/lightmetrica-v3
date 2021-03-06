#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

# Python module for unit test in python
set(_PROJECT_NAME pylm_test)
set(_SOURCE_FILES
    "pylm_test.cpp"
    "pylm_test_component.cpp"
    "pylm_test_json.cpp"
    "pylm_test_simple.cpp"
    "pylm_test_math.cpp")
set(_PCH_DIR "${PROJECT_SOURCE_DIR}/pch")
set(_PCH_FILES
    "${_PCH_DIR}/pch_pylm.h"
    "${_PCH_DIR}/pch_pylm.cpp")
add_library(${_PROJECT_NAME} MODULE ${_SOURCE_FILES} ${_PCH_FILES})
if (MSVC)
    add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch_pylm.h" SOURCE_CXX "${_PCH_DIR}/pch_pylm.cpp")
endif()
target_link_libraries(${_PROJECT_NAME} PRIVATE liblm pybind11::module lm_test_plugin::interface)
target_include_directories(${_PROJECT_NAME} PRIVATE "${_PCH_DIR}")
set_target_properties(${_PROJECT_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}" SUFFIX "${PYTHON_MODULE_EXTENSION}")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/test")
set_target_properties(${_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")