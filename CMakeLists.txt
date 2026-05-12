cmake_minimum_required(VERSION 3.16)
project(vrmod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VRMOD_DEV_BUILD "Enable dev build with tests and detailed logging" OFF)

# --- Main shared library ---
add_library(vrmod SHARED
    src/common/vrmod_state.cpp
    src/input/vr_input.cpp
    src/rendering/opengl_hook.cpp
    src/rendering/openvr_submit.cpp
    src/lua/lua_utils.cpp
    src/lua/lua_interface.cpp
)

target_include_directories(vrmod PRIVATE
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(vrmod PRIVATE -fPIC -m64)

target_link_directories(vrmod PRIVATE ${CMAKE_SOURCE_DIR}/deps/openvr/lib_linux64)
target_link_libraries(vrmod PRIVATE openvr_api dl GL)
set_target_properties(vrmod PROPERTIES
    PREFIX ""
    OUTPUT_NAME "gmcl_vrmod_linux64"
    SUFFIX ".dll"
    LINK_FLAGS "-Wl,-rpath='$ORIGIN'"
)

if(VRMOD_DEV_BUILD)
    target_compile_definitions(vrmod PRIVATE VRMOD_DEV_BUILD)
    enable_testing()
    add_subdirectory(tests)
endif()
