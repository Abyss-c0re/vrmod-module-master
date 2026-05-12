// Stub implementations for symbols needed by test builds
// that would normally come from OpenVR runtime or GL libraries.

#include <openvr/openvr.h>

// OpenVR free functions used in lua_interface.cpp are NOT linked into
// the test binary (we only test input math and lua comms).
// These stubs satisfy the linker for vrmod_state.cpp which declares
// globals of OpenVR types.

// No additional stubs needed -- the test only compiles vrmod_state.cpp
// and vr_input.cpp.  vr_input.cpp uses OpenVR types (defined in the header)
// but does not call free functions at link time for the extracted helpers.
// LUA_FUNCTION bodies reference g_pInput/g_compositor/etc. through pointers
// that the test sets up via mocks.

// GL stub: vrmod_state.cpp uses GLuint which is just unsigned int.
// No actual GL functions are called from the compiled test sources.
