// primext_bridge.h - the contract between reTS's client.dll and the PrimeXT render module (cl_dlls/pxrender.dll).
// H9: written new. Plain C, no types of either side's SDK beyond forward declarations, so both builds include it
// with their own headers.
//
// client.dll decides at load time (r_primext, a startup setting): with 0 it never loads pxrender.dll and answers the
// engine's HUD_GetRenderInterface with 0, so ref_gl draws exactly as before. With 1 it loads the module and forwards
// the render interface, the studio API, HUD_AddEntity and the init/shutdown calls; the module calls back into
// client.dll for the TS-specific parts (triangles, studio events, TS's studio hooks).
#pragma once

#define PXR_BRIDGE_VERSION 2
#define PXR_MODULE_NAME "pxrender.dll"
#define PXR_GETAPI_NAME "PXR_GetAPI"

#ifdef __cplusplus
extern "C" {
#endif

struct cl_enginefunc_s;
struct cl_entity_s;
struct mstudioevent_s;
struct render_api_s;
struct render_interface_s;
struct r_studio_interface_s;
struct engine_studio_api_s;
struct entity_state_s;
struct ref_params_s;

// What client.dll gives the module.
typedef struct pxr_host_s
{
	int size; // sizeof(pxr_host_t)

	// the client's own TriAPI passes (blob shadows, laser sights, particles, ts_fog, rally effects)
	void (*DrawNormalTriangles)(void);
	void (*DrawTransparentTriangles)(void);
	// studio events (muzzle flashes, shells, sounds)
	void (*StudioEvent)(const struct mstudioevent_s* event, const struct cl_entity_s* entity);
	int (*IsThirdPerson)(void);

	// TS studio hooks, run by the module's studio renderer where reTS's CStudioModelRenderer runs them
	// before a model is set up (the view model's slow-motion rate)
	void (*StudioPreDraw)(struct cl_entity_s* ent);
	// before a player is set up (the dead local player's frozen angles); returns 0 to skip the player
	int (*StudioPrePlayer)(struct cl_entity_s* ent, struct entity_state_s* pplayer);
	// after the bones: the attachments as the renderer computed them (TS's per-player gun anchor cache)
	void (*StudioAttachments)(struct cl_entity_s* ent, const float (*attachment)[3], int count);
	// the dead-player entity's player index (TS sends it in rendercolor.r), 0 if none
	int (*DeadPlayerIndex)(const struct cl_entity_s* ent);
	// the local player is dead (no view model)
	int (*IsDead)(void);
	// TS's linear fog this frame (ts_fog, or gl_fog 2): colour 0..255, start, end; 0 when there is none
	int (*GetFog)(float* color, float* start, float* end);
} pxr_host_t;

// What the module gives client.dll.
typedef struct pxr_api_s
{
	int size; // sizeof(pxr_api_t)

	int (*Initialize)(struct cl_enginefunc_s* engfuncs, int iVersion, const pxr_host_t* host);
	int (*GetRenderInterface)(int version, struct render_api_s* renderfuncs, struct render_interface_s* callback);
	int (*GetStudioModelInterface)(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);
	void (*HudInit)(void);
	void (*VidInit)(void);
	void (*Shutdown)(void);
	// called from HUD_AddEntity for every entity reTS keeps; the module puts it in its own lists
	void (*AddEntity)(int type, struct cl_entity_s* ent);
	void (*Frame)(double time);
	// the view as reTS's V_CalcRefdef left it, each frame
	void (*ViewParams)(const struct ref_params_s* pparams);
} pxr_api_t;

typedef int (*PFN_PXR_GetAPI)(int version, pxr_api_t* api);

#ifdef __cplusplus
}
#endif
