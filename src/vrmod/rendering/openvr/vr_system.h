#pragma once
#include <cstddef>

// Initialize OpenVR system, compositor, and input interfaces.
// Returns false on error.
bool VR_InitSystem(char* errorOut, size_t errorLen);

// Update recommended render target size (clamped to 4096 max).
void VR_UpdateRecommendedSize();

// Submit shared texture to compositor for both eyes.
// If submission fails, writes warning message to errorOut (non-fatal).
// Always calls PostPresentHandoff.
void VR_SubmitSharedTexture(char* errorOut, size_t errorLen);

// Shutdown compositor: clear last frame.
void VR_ShutdownCompositor();
