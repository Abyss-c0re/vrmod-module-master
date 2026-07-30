cmake_minimum_required(VERSION 3.10)
project(vrmod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Paths ──
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(OPENVR_LIB_DIR "${DEPS_DIR}/openvr/lib_linux64")
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/install/GarrysMod/garrysmod/lua/bin")

# ── Include dirs (used by both targets) ──
set(VRMOD_INCLUDE_DIRS
    ${DEPS_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# ── Sources by module ──
set(CORE_SOURCES
    src/core/vrmod_log.cpp
)

set(INPUT_SOURCES
    src/input/vr_input.cpp
)

set(RENDER_GL_SOURCES
    src/rendering/opengl/gl_hooks.cpp
)

set(RENDER_VR_SOURCES
    src/rendering/openvr/vr_render.cpp
)

set(LUA_SOURCES
    src/lua/lua_interface.cpp
)

# ==============================================================
#  RELEASE target: shared library (the gmod module)
# ==============================================================
option(VRMOD_BUILD_TESTS "Build the dev test runner" OFF)

add_library(vrmod_release SHARED
    ${CORE_SOURCES}
    ${INPUT_SOURCES}
    ${RENDER_GL_SOURCES}
    ${RENDER_VR_SOURCES}
    ${LUA_SOURCES}
)

target_include_directories(vrmod_release PRIVATE ${VRMOD_INCLUDE_DIRS})
# VRMOD_DEV_BUILD: file log to vrmod_debug.log (required for auto-test loop)
target_compile_definitions(vrmod_release PRIVATE VRMOD_DEV_BUILD)
target_compile_options(vrmod_release PRIVATE -O3 -m64)
target_link_directories(vrmod_release PRIVATE ${OPENVR_LIB_DIR})
target_link_libraries(vrmod_release PRIVATE openvr_api dl)

# Module lives at garrysmod/lua/bin; OpenVR at bin/ and bin/linux64 (modules.zip layout).
# From lua/bin: ../../../../bin/linux64 → GarrysMod/bin/linux64
set_target_properties(vrmod_release PROPERTIES
    OUTPUT_NAME "gmcl_vrmod_linux64"
    PREFIX ""
    SUFFIX ".dll"
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    BUILD_RPATH "\$ORIGIN/../../../../bin/linux64:\$ORIGIN/../../../../bin"
    INSTALL_RPATH "\$ORIGIN/../../../../bin/linux64:\$ORIGIN/../../../../bin"
)

# ==============================================================
#  DEV target: test runner executable (no game, no GL, no real VR)
# ==============================================================
if(VRMOD_BUILD_TESTS)
    set(TEST_SOURCES
        tests/test_main.cpp
        tests/test_input.cpp
        tests/test_lua.cpp
    )

    # Only compile modules that are testable without real HW
    add_executable(vrmod_tests
        ${CORE_SOURCES}
        ${INPUT_SOURCES}
        ${TEST_SOURCES}
    )

    target_include_directories(vrmod_tests PRIVATE
        ${VRMOD_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/tests
    )

    target_compile_definitions(vrmod_tests PRIVATE
        VRMOD_DEV_BUILD
        VRMOD_TESTING
    )

    target_compile_options(vrmod_tests PRIVATE -g -O0 -m64)
    target_link_directories(vrmod_tests PRIVATE ${OPENVR_LIB_DIR})
    target_link_libraries(vrmod_tests PRIVATE openvr_api dl)

    set_target_properties(vrmod_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endif()
