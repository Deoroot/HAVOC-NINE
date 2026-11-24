// pxr_module.h - pxrender.dll's side of the bridge to reTS's client.dll (H9: written new).
#pragma once

#include "../../primext_bridge.h"

extern const pxr_host_t* g_pxrHost;

void PXR_HostDrawNormalTriangles(void);
void PXR_HostDrawTransparentTriangles(void);
void PXR_HostStudioPreDraw(struct cl_entity_s* ent);
int PXR_HostStudioPrePlayer(struct cl_entity_s* ent, struct entity_state_s* pplayer);
void PXR_HostStudioAttachments(struct cl_entity_s* ent, const float (*attachment)[3], int count);
int PXR_HostGetFog(float* color, float* start, float* end);
int PXR_HostDeadPlayerIndex(const struct cl_entity_s* ent);
