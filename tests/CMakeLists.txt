# Test library: compile only the testable source files
# Does NOT link against OpenVR runtime or GL -- uses mocks instead
add_library(vrmod_testable STATIC
    ${CMAKE_SOURCE_DIR}/src/input/vr_input.cpp
    ${CMAKE_SOURCE_DIR}/src/common/vrmod_state.cpp
    ${CMAKE_SOURCE_DIR}/src/lua/lua_utils.cpp
)

target_include_directories(vrmod_testable PUBLIC
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Tests compile with both VRMOD_DEV_BUILD and VRMOD_TESTING
target_compile_definitions(vrmod_testable PUBLIC VRMOD_DEV_BUILD VRMOD_TESTING)
target_compile_options(vrmod_testable PRIVATE -m64)

# Provide a stub for GL symbols needed by vrmod_state.cpp (GLuint = unsigned int)
# The test mock provides minimal stubs so we don't need real GL at link time.

add_executable(vrmod_tests
    test_main.cpp
    test_input.cpp
    test_lua_comms.cpp
    mocks/mock_stubs.cpp
)

target_include_directories(vrmod_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/deps
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_compile_definitions(vrmod_tests PRIVATE VRMOD_DEV_BUILD VRMOD_TESTING)
target_compile_options(vrmod_tests PRIVATE -m64)
target_link_libraries(vrmod_tests PRIVATE vrmod_testable)

add_test(NAME vrmod_tests COMMAND vrmod_tests)
