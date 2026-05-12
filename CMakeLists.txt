cmake_minimum_required(VERSION 3.16)
project(vrmod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(VRMOD_DEV_BUILD "Enable dev build with tests and detailed logging" OFF)

# --- Paths ---
set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps)
set(OPENVR_LIB ${DEPS_DIR}/openvr/lib_linux64/libopenvr_api.so)

# --- Shared library: gmcl_vrmod_linux64.dll ---
set(VRMOD_SOURCES
    src/vrmod/globals.cpp
    src/vrmod/logging.cpp
    src/vrmod/input/vr_input.cpp
    src/vrmod/rendering/opengl/gl_texture.cpp
    src/vrmod/rendering/openvr/vr_system.cpp
    src/vrmod/lua/lua_helpers.cpp
    src/vrmod/lua/lua_bindings.cpp
)

add_library(vrmod SHARED ${VRMOD_SOURCES})

target_include_directories(vrmod PRIVATE
    ${DEPS_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(vrmod PRIVATE
    ${OPENVR_LIB}
    dl
    GL
)

target_compile_options(vrmod PRIVATE -m64 -Wall)

if(NOT VRMOD_DEV_BUILD)
    target_compile_options(vrmod PRIVATE -O3)
else()
    target_compile_definitions(vrmod PRIVATE VRMOD_DEV)
    target_compile_options(vrmod PRIVATE -O0 -g)
endif()

set_target_properties(vrmod PROPERTIES
    OUTPUT_NAME "gmcl_vrmod_linux64"
    SUFFIX ".dll"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/install/GarrysMod/garrysmod/lua/bin"
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
)

# --- Dev build: test executable ---
if(VRMOD_DEV_BUILD)
    # Test sources (input + lua comm tests, no GL link needed)
    add_executable(vrmod_tests
        tests/test_main.cpp
        tests/test_input.cpp
        tests/test_lua.cpp
        src/vrmod/globals.cpp
        src/vrmod/logging.cpp
        src/vrmod/input/vr_input.cpp
        src/vrmod/lua/lua_helpers.cpp
    )

    target_compile_definitions(vrmod_tests PRIVATE VRMOD_DEV VRMOD_TEST)

    target_include_directories(vrmod_tests PRIVATE
        ${DEPS_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )

    # Tests only use virtual OpenVR calls via mocks — no libopenvr_api link needed.
    # No GL link needed either.
    target_compile_options(vrmod_tests PRIVATE -m64 -O0 -g -Wall)

    target_link_libraries(vrmod_tests PRIVATE dl)
endif()
