#
#   Lightmetrica - Copyright (c) 2019 Hisanari Otsu
#   Distributed under MIT license. See LICENSE file for details.
#

include(LmAddPlugin)

# Plugin for unit test
lm_add_plugin(
    NAME lm_test_plugin
    INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    HEADERS
        "test_interface.h"
    SOURCES
        "test_interface_impl.cpp")

# Unit test
set(_PROJECT_NAME lm_test)
set(_HEADER_FILES
    "test_common.h")
set(_SOURCE_FILES
    "main.cpp"
    "test_common.cpp"
    "test_exception.cpp"
    "test_component.cpp"
    "test_assets.cpp"
    "test_json.cpp"
    "test_serial.cpp"
    "test_logger.cpp")
set(_PCH_DIR "${PROJECT_SOURCE_DIR}/pch")
set(_PCH_FILES
    "${_PCH_DIR}/pch.h"
    "${_PCH_DIR}/pch.cpp")
add_executable(${_PROJECT_NAME} ${_HEADER_FILES} ${_SOURCE_FILES} ${_PCH_FILES})
if (MSVC)
    add_precompiled_header(${_PROJECT_NAME} "${_PCH_DIR}/pch.h" SOURCE_CXX "${_PCH_DIR}/pch.cpp")
endif()
target_link_libraries(${_PROJECT_NAME}
    PRIVATE liblm
            doctest::doctest
            lm_test_plugin::interface
            Threads::Threads)
target_include_directories(${_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${_PCH_DIR}")
set_target_properties(${_PROJECT_NAME} PROPERTIES FOLDER "lm/test")
set_target_properties(${_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
source_group("Header Files" FILES ${_HEADER_FILES})
source_group("Source Files" FILES ${_SOURCE_FILES})