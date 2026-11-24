// gl_debug.h - the renderer's error check and its debug drawing (r_show* and friends), in gl_debug.cpp.
// H9: written new for reTS from the interface (the functions gl_debug.cpp defines and the renderer calls),
// replacing PrimeXT's file, which had no licence notice.
#pragma once
#include "vector.h"
#include "com_model.h"
#include "frustum.h"
#include "gl_debug_scope.h"

// Reports any pending GL error with the calling function and line.
#define GL_CheckForErrors() GL_CheckForErrorsInternal(__FUNCTION__, __LINE__)
void GL_CheckForErrorsInternal(const char *filename, int line);

void DBG_PrintVertexVBOSizes();
void DBG_DrawLightFrustum();
void DBG_DrawGlassScissors();
void DrawLightProbes();
void R_ShowLightMaps();
void R_RenderLightProbeInternal(const Vector &origin, const Vector lightCube[]);
void DrawWirePoly(msurface_t *surf);
void DrawTangentSpaces();
void DrawWireFrame();
void DrawViewLeaf();
void DrawCubeMaps();
