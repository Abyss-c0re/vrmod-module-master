#pragma once
#include <cstdint>
#include <cstddef>

// Initialize GL entry points from libtogl_client.so.
// Must be called after OpenVR init. Returns false on error.
bool GL_Init(char* errorOut, size_t errorLen);

// Begin shared texture: tear down previous, create new GL texture, apply hook patch.
// Returns false on error.
bool GL_ShareTextureBegin(uint32_t width, uint32_t height, char* errorOut, size_t errorLen);

// Finish shared texture: validate, setup VR texture handle, remove hook patch.
// Returns false on error.
bool GL_ShareTextureFinish(char* errorOut, size_t errorLen);

// Set texture submission bounds for both eyes.
void GL_SetSubmitTextureBounds(float luMin, float lvMin, float luMax, float lvMax,
                               float ruMin, float rvMin, float ruMax, float rvMax);

// Flush GL state during shutdown.
void GL_FlushAndFinish();
