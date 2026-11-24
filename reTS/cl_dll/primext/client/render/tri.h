// tri.h - the client's TriAPI entry points, called by the renderer once per frame for opaque and for
// translucent geometry.
// H9: written new for reTS from the interface (the HLSDK names of the two exports), replacing PrimeXT's
// file, which had no licence notice.
#pragma once
#include "exportdef.h"

#ifdef __cplusplus
extern "C" {
#endif

void DLLEXPORT HUD_DrawNormalTriangles(void);
void DLLEXPORT HUD_DrawTransparentTriangles(void);

#ifdef __cplusplus
}
#endif
