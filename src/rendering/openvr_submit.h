#ifndef VRMOD_OPENVR_SUBMIT_H
#define VRMOD_OPENVR_SUBMIT_H

struct lua_State;

void UpdateRecommendedSize();

int GetDisplayInfo(lua_State* L);
int SetSubmitTextureBounds(lua_State* L);
int SubmitSharedTexture(lua_State* L);

#endif
