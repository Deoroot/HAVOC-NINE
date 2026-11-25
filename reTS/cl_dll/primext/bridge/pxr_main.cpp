// pxr_main.cpp - pxrender.dll: PrimeXT's renderer as a module of reTS's client.dll (H9: written new).
//
// Holds what PrimeXT's client.dll (client/cdll_int.cpp, client/entity.cpp) gives its renderer: the engine
// tables, the init/shutdown calls, HUD_AddEntity's hand-over, and the client functions the renderer calls
// (CL_IsThirdPerson, HUD_StudioEvent). Everything else of the game client stays in reTS.

#include "hud.h"
#include "utils.h"
#include "gl_local.h"
#include "gl_studio.h"
#include "gl_cvars.h"
#include "mobility_int.h"
#include "entity_types.h"
#include "filesystem_utils.h"
#include "meshdesc_factory.h"
#include "pxr_module.h"
#include "parsemsg.h"
#include "postfx_controller.h"
#include "postfx_parameters.h"

int developer_level;
int g_iXashEngineBuildNumber;
cl_enginefunc_t gEngfuncs;
mobile_engfuncs_t gMobileAPI;
render_api_t gRenderfuncs;
CHud gHUD;
const pxr_host_t* g_pxrHost = nullptr;

extern "C" int DLLEXPORT HUD_GetRenderInterface(int version, render_api_t* renderfuncs, render_interface_t* callback);
extern "C" int DLLEXPORT HUD_GetStudioModelInterface(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);

// ---- what the renderer asks of the client --------------------------------------------------------------------------

void PXR_HostDrawNormalTriangles(void)
{
	if (g_pxrHost && g_pxrHost->DrawNormalTriangles)
		g_pxrHost->DrawNormalTriangles();
}

void PXR_HostDrawTransparentTriangles(void)
{
	if (g_pxrHost && g_pxrHost->DrawTransparentTriangles)
		g_pxrHost->DrawTransparentTriangles();
}

void PXR_HostStudioPreDraw(struct cl_entity_s* ent)
{
	if (g_pxrHost && g_pxrHost->StudioPreDraw)
		g_pxrHost->StudioPreDraw(ent);
}

int PXR_HostStudioPrePlayer(struct cl_entity_s* ent, struct entity_state_s* pplayer)
{
	if (g_pxrHost && g_pxrHost->StudioPrePlayer)
		return g_pxrHost->StudioPrePlayer(ent, pplayer);
	return 1;
}

void PXR_HostStudioAttachments(struct cl_entity_s* ent, const float (*attachment)[3], int count)
{
	if (g_pxrHost && g_pxrHost->StudioAttachments)
		g_pxrHost->StudioAttachments(ent, attachment, count);
}

int PXR_HostDeadPlayerIndex(const struct cl_entity_s* ent)
{
	if (g_pxrHost && g_pxrHost->DeadPlayerIndex)
		return g_pxrHost->DeadPlayerIndex(ent);
	return ent->curstate.renderamt;
}

struct cl_entity_s* PXR_GetEntity(int index)
{
	cl_entity_t* ent = gEngfuncs.GetEntityByIndex(index);
	if (!ent && index == 0)
	{
		static cl_entity_t s_worldStandIn;
		return &s_worldStandIn;
	}
	return ent;
}

int PXR_HostGetFog(float* color, float* start, float* end)
{
	if (g_pxrHost && g_pxrHost->GetFog)
		return g_pxrHost->GetFog(color, start, end);
	return 0;
}

int CL_IsDead(void)
{
	if (g_pxrHost && g_pxrHost->IsDead)
		return g_pxrHost->IsDead();
	return tr.viewparams.health <= 0;
}

// PrimeXT's client keeps the PVS message number from HUD_TxferLocalOverrides; here it is the local player's
int r_currentMessageNum = 0;

// PrimeXT's r_view.cpp: a func_monitor looking at the local player from a camera rebuilds the first-person view.
// reTS's view code is not in this module; the view reTS made this frame (tr.viewparams) is used as it is.
void V_CalcFirstPersonRefdef(struct ref_params_s* pparams)
{
}

// PrimeXT's client.dll: flushes the collision mesh cache when the map changes
void CL_NewMap()
{
	CMeshDescFactory::Instance().ClearCache();
}

int CL_IsThirdPerson(void)
{
	if (g_pxrHost && g_pxrHost->IsThirdPerson)
		return g_pxrHost->IsThirdPerson();
	return 0;
}

void HUD_StudioEvent(const struct mstudioevent_s* event, const struct cl_entity_s* entity)
{
	if (g_pxrHost && g_pxrHost->StudioEvent)
		g_pxrHost->StudioEvent(event, entity);
}

// ---- the calls client.dll forwards ----------------------------------------------------------------------------------

static int PXR_Initialize(struct cl_enginefunc_s* pEnginefuncs, int iVersion, const pxr_host_t* host)
{
	if (iVersion != CLDLL_INTERFACE_VERSION || !host || host->size != sizeof(pxr_host_t))
		return 0;

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));
	g_pxrHost = host;
	developer_level = (int)CVAR_GET_FLOAT("developer");

	g_iXashEngineBuildNumber = (int)CVAR_GET_FLOAT("build"); // 0 for old builds or GoldSrc
	if (g_iXashEngineBuildNumber <= 0)
		g_iXashEngineBuildNumber = (int)CVAR_GET_FLOAT("buildnum");

	if (!fs::Initialize())
	{
		ALERT(at_error, "pxrender: failed to initialize the filesystem interface\n");
		return 0;
	}

	gEngfuncs.Con_Printf("pxrender: PrimeXT renderer (s0-bsp2-nodes 9f112ca) loaded by reTS\n");
	return 1;
}

// PrimeXT's CHud::MsgFunc_PostFxSettings (client/hud_msg.cpp): env_postfx_controller's look, 13 floats
// H9: the last message, so that a trigger_multiple re-firing the same controller every `wait` seconds while the player
// stands in its place does not restart the fade (PrimeXT's UpdateState starts each fade from its last target)
static unsigned char s_lastPostFx[64];
static int s_lastPostFxSize = 0;

static int PXR_MsgPostFxSettings(const char* pszName, int iSize, void* pbuf)
{
	if (iSize > 0 && iSize <= (int)sizeof(s_lastPostFx) && iSize == s_lastPostFxSize && !memcmp(s_lastPostFx, pbuf, iSize))
		return 1; // the look it already has or is fading to
	if (iSize > 0 && iSize <= (int)sizeof(s_lastPostFx))
	{
		memcpy(s_lastPostFx, pbuf, iSize);
		s_lastPostFxSize = iSize;
	}

	CPostFxParameters params;
	BEGIN_READ(pszName, pbuf, iSize);
	params.ParseMessage();
	// H9: the new fade starts from what is on the screen now, not from the last target (a player who walks out of a
	// place half way through its fade would otherwise see the look jump)
	g_PostFxController.UpdateState(g_PostFxController.GetState(), false);
	g_PostFxController.UpdateState(params);
	END_READ();
	return 1;
}

static void PXR_HudInit(void)
{
	// H9: hooked here, not in reTS's HUD: only a client that runs this renderer reads it (PRIMEXT-ENTITIES.md)
	gEngfuncs.pfnHookUserMsg("PostFxSettings", PXR_MsgPostFxSettings);

	// PrimeXT's V_Init (r_view.cpp) registers the renderer's cvars before GL_Init reads them
	R_InitializeConVars();

	if (g_fRenderInitialized)
	{
		const bool ok = GL_Init();
		gEngfuncs.Con_Printf("pxrender: GL_Init %s\n", ok ? "done" : "FAILED, the engine draws");
	}
	else
		gEngfuncs.Con_Printf("pxrender: HUD_Init with the renderer not initialized\n");
}

static void PXR_VidInit(void)
{
	if (!g_fRenderInitialized || !pglGenVertexArrays)
	{
		gEngfuncs.Con_Printf("pxrender: VidInit before GL_Init, skipped\n");
		return;
	}
	// H9: gl_renderer 0 is what GL_Init sets when it fails, and earlier builds saved it in config.cfg, which then kept
	// PrimeXT off at every start (it hands each frame to the engine) while H9 still held the engine's own effects off.
	// It is no longer saved; a 0 that config.cfg gives at start is put back to 1 at the first map, GL_Init having worked.
	static bool s_rendererChecked = false;
	if (!s_rendererChecked)
	{
		s_rendererChecked = true;
		if (!CVAR_TO_BOOL(cv_renderer))
		{
			CVAR_SET_FLOAT("gl_renderer", 1.0f);
			gEngfuncs.Con_Printf("pxrender: gl_renderer was 0 (an old config.cfg line); set to 1, GL_Init having worked\n");
		}
	}

	gHUD.m_scrinfo.iSize = sizeof(gHUD.m_scrinfo);
	GetScreenInfo(&gHUD.m_scrinfo);
	R_VidInit();

	// H9: a new map starts with the neutral look (an env_postfx_controller's state is not carried across maps)
	g_PostFxController.UpdateState(CPostFxParameters::Defaults(), false);
	s_lastPostFxSize = 0;
}

static void PXR_Shutdown(void)
{
	if (g_fRenderInitialized)
		GL_Shutdown();
}

static void PXR_AddEntity(int type, struct cl_entity_s* ent)
{
	if (!g_fRenderInitialized || !CVAR_TO_BOOL(cv_renderer))
		return;

	if (ent->curstate.rendermode == kRenderTransAlpha && ent->model && ent->model->type == mod_brush)
		ent->curstate.renderamt = 255; // PrimeXT: fix invisible grates

	if (type == ET_BEAM)
		return; // the engine draws beams

	// H9: func_monitor / func_portal (renderfx 73, H9_RENDERFX_SUBVIEW in reTS/dlls/primext_server.h). H9's delta.lst
	// has no fuser2 or iuser1 and an 8-bit sequence, so the server sends the camera in aiment (11 bits), the field of
	// view in controller[1] and the switches in controller[0]; they go where PrimeXT's renderer reads them.
	if (ent->curstate.renderfx == 73 && ent->model && ent->model->type == mod_brush)
	{
		ent->curstate.sequence = ent->curstate.aiment;
		ent->curstate.fuser2 = (float)ent->curstate.controller[1];
		ent->curstate.iuser1 = (ent->curstate.controller[0] & 0x01) ? CF_MONOCHROME : 0;
	}

	R_AddEntity(ent, type);
}

static void PXR_Frame(double time)
{
	gHUD.m_flTime = (float)time;
}

static void PXR_ViewParams(const struct ref_params_s* pparams)
{
	// PrimeXT's V_CalcRefdef keeps this copy (r_view.cpp); the renderer reads waterlevel, viewentity and the rest
	memcpy(&tr.viewparams, pparams, sizeof(ref_params_t));

	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	if (local)
		r_currentMessageNum = local->curstate.messagenum;
}

extern "C" int DLLEXPORT PXR_GetAPI(int version, pxr_api_t* api)
{
	if (version != PXR_BRIDGE_VERSION || !api || api->size != sizeof(pxr_api_t))
		return 0;

	api->Initialize = PXR_Initialize;
	api->GetRenderInterface = HUD_GetRenderInterface;
	api->GetStudioModelInterface = HUD_GetStudioModelInterface;
	api->HudInit = PXR_HudInit;
	api->VidInit = PXR_VidInit;
	api->Shutdown = PXR_Shutdown;
	api->AddEntity = PXR_AddEntity;
	api->Frame = PXR_Frame;
	api->ViewParams = PXR_ViewParams;
	return 1;
}
