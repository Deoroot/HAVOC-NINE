// h9_primext.cpp - H9: the PrimeXT renderer behind r_primext (client.dll's side; written new).
//
// The engine asks for the custom render interface once, as client.dll loads (CL_LoadProgs: Initialize, then
// HUD_GetRenderInterface, then HUD_Init, then HUD_GetStudioModelInterface), before config.cfg is run. So r_primext is a
// startup setting: it is read here from the command line (-primext / -noprimext) or else from the last
// `r_primext` line of config.cfg and userconfig.cfg, and a change while running takes effect at the next start.
//
// r_primext is 1 by default. Builds before that saved "r_primext 0" into every player's config.cfg as the default,
// not as a choice; `r_primext_default 1`, saved beside it from this build on, marks a config.cfg whose r_primext is
// the player's own. A config.cfg without it has its r_primext 0 read as that old default: the start is with 1, and
// the 0 config.cfg then sets is put back to 1 (once, before the first map). userconfig.cfg, written by hand, is
// always obeyed.
//
// r_primext 0: pxrender.dll is not loaded, HUD_GetRenderInterface answers 0, the engine clears the callbacks and
// ref_gl draws everything as before (reTS's CStudioModelRenderer included).
// r_primext 1: cl_dlls/pxrender.dll (PrimeXT's renderer, its own headers, reTS/cl_dll/primext) is loaded and given the
// render interface; HUD_AddEntity hands it every entity reTS keeps; it calls back here for reTS's TriAPI passes,
// studio events and TS's studio hooks.
//
// What it changes in the engine's settings while it runs (each frame, and puts back at the next start with 0):
// - r_scene_msaa, r_bloom and r_scene_scale: H9's ref_gl draws the scene into its own off-screen target when any is
//   set (msr-web, FSR), and then presents that target over PrimeXT's frame, which is black. They are held at 0, 0, 1.
// - gl_hdr: held at 1. With 0 PrimeXT draws a black screen (after a map load), and a change in the middle of a map
//   crashes it (PrimeXT issue #301: the HDR target is made only by R_VidInit, and every frame binds it).
//
// PrimeXT's saved settings (primext_cvars.h): with r_primext 0 client.dll registers them itself, so config.cfg and
// opengl.cfg keep them through a session without pxrender.dll. PrimeXT's r_shadows, r_bloom and r_bloom_scale are
// px_shadows, px_bloom and px_bloom_scale here: ref_gl has cvars of those names with another meaning.

#include "hud.h"
#include "cl_util.h"
#include "com_model.h"
#include "r_studioint.h"
#include "studio.h"
#include "Exports.h"
#include "tri.h"
#include "primext_bridge.h"
#include "primext_cvars.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef H9_PRIMEXT
// windows.h after the HLSDK headers: its HSPRITE is not the client's
#define HSPRITE WINDOWS_HSPRITE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef HSPRITE
#endif

extern engine_studio_api_t IEngineStudio;
extern "C" int CL_IsThirdPerson();
void TS_StudioViewModelRate(cl_entity_t* ent, int frameCount);
void TS_StudioDeadLocalPlayer(cl_entity_t* ent, entity_state_t* pplayer);
void TS_StudioNotePlayerAttachments(cl_entity_t* ent);
int TS_DeadPlayerIndex(const cl_entity_t* ent);

cl_enginefunc_t* g_pH9EngineFuncs = nullptr; // the engine's own table (cdll_int.cpp Initialize), handed to pxrender.dll

static cvar_t* r_primext = nullptr;
static cvar_t* r_primext_default = nullptr; // 1: config.cfg's r_primext was saved by a build where 1 is the default
static cvar_t* r_primext_restore = nullptr; // "msaa bloom scale" as they were before r_primext 1 held them
static cvar_t* px_quality = nullptr;        // the quality preset, 0..3
static bool s_active = false;               // pxrender.dll loaded and drawing
static int s_startValue = 0;
static bool s_oldDefaultPending = false; // config.cfg's r_primext 0 is the old default: put back to 1 when it runs
static bool s_restoreDone = false;

#ifdef H9_PRIMEXT
static HMODULE s_module = nullptr;
static pxr_api_t s_api;
#endif

// ---- the startup setting --------------------------------------------------------------------------------------------

// The value (a whole number) of the last line of a config file that sets `name`, or -1.
static int H9Pxr_ReadConfig(const char* file, const char* name)
{
	const int nameLength = (int)strlen(name);
	int length = 0;
	char* data = (char*)gEngfuncs.COM_LoadFile(file, 5, &length);
	if (!data)
		return -1;

	int value = -1;
	const char* p = data;
	const char* end = data + length;
	while (p < end)
	{
		const char* line = p;
		while (p < end && *p != '\n')
			p++;
		const char* lineEnd = p;
		if (p < end)
			p++;

		while (line < lineEnd && (*line == ' ' || *line == '\t'))
			line++;
		if (lineEnd - line < nameLength + 1 || _strnicmp(line, name, nameLength) != 0 || !(line[nameLength] == ' ' || line[nameLength] == '\t'))
			continue; // also skips longer names (r_primext_restore)
		line += nameLength;
		while (line < lineEnd && (*line == ' ' || *line == '\t' || *line == '"'))
			line++;
		if (line < lineEnd && isdigit((unsigned char)*line))
			value = atoi(line);
	}

	gEngfuncs.COM_FreeFile(data);
	return value;
}

static int H9Pxr_StartValue()
{
	if (gEngfuncs.CheckParm("-noprimext", nullptr))
		return 0;
	if (gEngfuncs.CheckParm("-primext", nullptr))
		return 1;

	int value = 1;
	const int fromConfig = H9Pxr_ReadConfig("config.cfg", "r_primext");
	const bool ownChoice = H9Pxr_ReadConfig("config.cfg", "r_primext_default") >= 1;
	if (fromConfig >= 0 && ownChoice)
		value = fromConfig != 0 ? 1 : 0;
	const int fromUser = H9Pxr_ReadConfig("userconfig.cfg", "r_primext");
	if (fromUser >= 0)
		value = fromUser != 0 ? 1 : 0;
	s_oldDefaultPending = fromConfig == 0 && !ownChoice && fromUser < 0;
	if (s_oldDefaultPending)
		gEngfuncs.Con_Printf("r_primext: config.cfg's 0 is an older build's default, not a choice: starting with 1\n");
	return value;
}

static void H9Pxr_RegisterCvars()
{
	if (r_primext)
		return;
	r_primext = gEngfuncs.pfnRegisterVariable("r_primext", "1", FCVAR_ARCHIVE);
	r_primext_default = gEngfuncs.pfnRegisterVariable("r_primext_default", "0", FCVAR_ARCHIVE);
	r_primext_restore = gEngfuncs.pfnRegisterVariable("r_primext_restore", "", FCVAR_ARCHIVE);
	px_quality = gEngfuncs.pfnRegisterVariable("px_quality", "2", FCVAR_ARCHIVE);
}

// ---- px_quality: the PrimeXT renderer's quality presets ------------------------------------------------------------
//
// px_quality 0 low, 1 medium, 2 high, 3 custom. Setting 0, 1 or 2 sets the cvars below (all change while running);
// 3 changes nothing, for a player who sets them one by one. 2, high, is the table's defaults (primext_cvars.h), so a
// new player's settings are high. The value config.cfg gives at start is not applied again: the settings it names were
// saved beside it. One of the preset's cvars set by hand makes px_quality 3. gl_hdr and MSAA (gl_msaa_samples) are
// not part of a preset.
struct pxq_setting_t
{
	const char* name;
	const char* value[3]; // low, medium, high
};

static const pxq_setting_t s_qualitySettings[] = {
	{"gl_parallax", {"0", "1", "2"}},
	{"px_shadows", {"1", "2", "4"}},
	{"gl_shadowmap_size", {"512", "1024", "1024"}},
	{"gl_realtime_puddles", {"0", "1", "1"}},
	{"gl_sunshafts", {"0", "1", "1"}},
	{"r_grass", {"0", "1", "1"}},
	{"px_bloom", {"0", "1", "1"}},
	// r_lighting_brdf: PBR (GGX lights, split-sum IBL on models) at high; low and medium keep the classic lighting
	{"r_lighting_brdf", {"0", "0", "1"}},
};

static void H9Pxr_ApplyQuality(int level)
{
	static const char* const names[] = {"low", "medium", "high", "custom"};
	if (level == 3)
	{
		gEngfuncs.Con_Printf("px_quality 3 (custom): the renderer's settings are left as they are\n");
		return;
	}

	char line[512];
	int used = snprintf(line, sizeof(line), "px_quality %d (%s):", level, names[level]);
	for (const pxq_setting_t& s : s_qualitySettings)
	{
		gEngfuncs.Cvar_Set(s.name, s.value[level]);
		if (used > 0 && used < (int)sizeof(line))
			used += snprintf(line + used, sizeof(line) - used, " %s %s", s.name, s.value[level]);
	}
	gEngfuncs.Con_Printf("%s\n", line);
	gEngfuncs.Con_Printf("px_quality: gl_hdr (always 1 with PrimeXT) and gl_msaa_samples (%g, at the next start) are not part of it\n",
		gEngfuncs.pfnGetCvarFloat("gl_msaa_samples"));
}

// px_quality as it was saved: userconfig.cfg's last line for it (config.cfg runs userconfig.cfg last), else
// config.cfg's, or -1 when neither has one.
static int H9Pxr_SavedQuality()
{
	const int fromUser = H9Pxr_ReadConfig("userconfig.cfg", "px_quality");
	return fromUser >= 0 ? fromUser : H9Pxr_ReadConfig("config.cfg", "px_quality");
}

static const int PXQ_SETTING_COUNT = (int)(sizeof(s_qualitySettings) / sizeof(s_qualitySettings[0]));
static bool s_qualityArmed = false;   // config.cfg has run: from now on a new px_quality is the player's
static int s_qualitySaved = -2;       // H9Pxr_SavedQuality(), read once (-2: not read yet)
static float s_qualitySeen = -1.0f;   // the px_quality last acted on
static float s_qualityValues[PXQ_SETTING_COUNT]; // the preset's cvars as they were last frame

static void H9Pxr_QualitySnapshot()
{
	for (int i = 0; i < PXQ_SETTING_COUNT; i++)
		s_qualityValues[i] = gEngfuncs.pfnGetCvarFloat(s_qualitySettings[i].name);
}

static bool H9Pxr_QualityLevel(float value, int& level)
{
	level = (int)value;
	return (float)level == value && level >= 0 && level <= 3;
}

// From the first map load on, config.cfg (and whatever ran after it) has certainly run: a px_quality that differs
// from the saved one is then applied at the next frame.
static void H9Pxr_QualityArmAtMapLoad()
{
	if (s_qualityArmed || !px_quality)
		return;
	s_qualityArmed = true;
	s_qualitySeen = s_qualitySaved >= 0 ? (float)s_qualitySaved : px_quality->value;
	H9Pxr_QualitySnapshot();
}

// Each frame. Until config.cfg has run, px_quality is its default and the cvars are being set from config.cfg one by
// one: nothing is done. It has run once px_quality holds the saved value (H9Pxr_SavedQuality); that value is not
// applied again, the settings it named were saved beside it, and a player's own settings under it are kept. After
// that, a new px_quality 0-2 sets the preset's cvars, and a preset's cvar changed by hand (the console, the menu, a
// config) sets px_quality 3, custom, so that the preset is not applied over it later.
static void H9Pxr_QualityFrame()
{
	if (!px_quality)
		return;
	const float value = px_quality->value;
	int level = 0;

	if (!s_qualityArmed)
	{
		if (s_qualitySaved == -2)
			s_qualitySaved = H9Pxr_SavedQuality();
		if (s_qualitySaved >= 0 && value != (float)s_qualitySaved)
			return; // config.cfg has not run yet
		s_qualityArmed = true;
		s_qualitySeen = value;
		// the first start with px_quality (no line for it in config.cfg or userconfig.cfg): its default, high, is
		// applied once, so a config.cfg saved by an older build (gl_realtime_puddles "0") gets the preset's settings
		if (s_qualitySaved < 0 && H9Pxr_QualityLevel(value, level) && level <= 2)
			H9Pxr_ApplyQuality(level);
		H9Pxr_QualitySnapshot();
		return;
	}

	if (value != s_qualitySeen)
	{
		s_qualitySeen = value;
		if (!H9Pxr_QualityLevel(value, level))
			gEngfuncs.Con_Printf("px_quality: 0 low, 1 medium, 2 high, 3 custom (leaves the settings as they are)\n");
		else
			H9Pxr_ApplyQuality(level);
		H9Pxr_QualitySnapshot();
		return;
	}

	if (H9Pxr_QualityLevel(value, level) && level <= 2)
	{
		for (int i = 0; i < PXQ_SETTING_COUNT; i++)
		{
			if (gEngfuncs.pfnGetCvarFloat(s_qualitySettings[i].name) == s_qualityValues[i])
				continue;
			gEngfuncs.Cvar_Set("px_quality", "3");
			s_qualitySeen = 3.0f;
			gEngfuncs.Con_Printf("px_quality 3 (custom): %s was set by hand, so preset %d no longer applies\n", s_qualitySettings[i].name, level);
			break;
		}
	}
	H9Pxr_QualitySnapshot();
}

// Started without pxrender.dll: its saved settings are registered here, with its defaults, so that config.cfg's and
// opengl.cfg's lines for them are kept and written back at exit. Before config.cfg runs (HUD_Init), and only once: the
// engine puts a registration's default over an existing cvar's value.
static void H9Pxr_RegisterSavedCvars()
{
	static bool s_done = false;
	if (s_done)
		return;
	s_done = true;
	for (int i = 0; i < PXR_SAVED_CVAR_COUNT; i++)
		gEngfuncs.pfnRegisterVariable((char*)pxr_saved_cvars[i]->name, (char*)pxr_saved_cvars[i]->value, pxr_saved_cvars[i]->flags);
}

// ---- the callbacks pxrender.dll makes ------------------------------------------------------------------------------

#ifdef H9_PRIMEXT
static void H9Pxr_StudioPreDraw(cl_entity_t* ent)
{
	int frameCount = 0;
	double now = 0, before = 0;
	IEngineStudio.GetTimes(&frameCount, &now, &before);
	TS_StudioViewModelRate(ent, frameCount);
}

static int H9Pxr_StudioPrePlayer(cl_entity_t* ent, entity_state_t* pplayer)
{
	TS_StudioDeadLocalPlayer(ent, pplayer);
	return 1;
}

static void H9Pxr_StudioAttachments(cl_entity_t* ent, const float (*attachment)[3], int count)
{
	if (ent == nullptr || ent->player == 0)
		return;
	for (int i = 0; i < count && i < 4; i++)
		ent->attachment[i] = Vector(attachment[i][0], attachment[i][1], attachment[i][2]);
	TS_StudioNotePlayerAttachments(ent);
}

static int H9Pxr_IsThirdPerson()
{
	return CL_IsThirdPerson();
}

static int H9Pxr_IsDead()
{
	return gHUD.m_Health.m_iHealth <= 0 ? 1 : 0;
}

static int H9Pxr_GetFog(float* color, float* start, float* end)
{
	extern bool TS_FogThisFrame(float* color, float* start, float* end);
	return TS_FogThisFrame(color, start, end) ? 1 : 0;
}

static const pxr_host_t s_host = {
	sizeof(pxr_host_t),
	HUD_DrawNormalTriangles,
	HUD_DrawTransparentTriangles,
	HUD_StudioEvent,
	H9Pxr_IsThirdPerson,
	H9Pxr_StudioPreDraw,
	H9Pxr_StudioPrePlayer,
	H9Pxr_StudioAttachments,
	TS_DeadPlayerIndex,
	H9Pxr_IsDead,
	H9Pxr_GetFog,
};

static bool H9Pxr_Load()
{
	// pxrender.dll sits beside client.dll (ts/cl_dlls)
	char path[MAX_PATH];
	HMODULE self = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&H9Pxr_Load, &self))
		return false;
	DWORD n = GetModuleFileNameA(self, path, sizeof(path));
	if (n == 0 || n >= sizeof(path))
		return false;
	char* slash = strrchr(path, '\\');
	if (!slash)
		slash = strrchr(path, '/');
	if (!slash)
		return false;
	strcpy(slash + 1, PXR_MODULE_NAME);

	s_module = LoadLibraryA(path);
	if (!s_module)
	{
		gEngfuncs.Con_Printf("r_primext: can't load %s (error %lu), drawing with the engine\n", path, GetLastError());
		return false;
	}

	PFN_PXR_GetAPI getApi = (PFN_PXR_GetAPI)GetProcAddress(s_module, PXR_GETAPI_NAME);
	memset(&s_api, 0, sizeof(s_api));
	s_api.size = sizeof(s_api);
	if (!getApi || !getApi(PXR_BRIDGE_VERSION, &s_api))
	{
		gEngfuncs.Con_Printf("r_primext: %s is not of this client's version, drawing with the engine\n", PXR_MODULE_NAME);
		FreeLibrary(s_module);
		s_module = nullptr;
		return false;
	}

	if (!g_pH9EngineFuncs || !s_api.Initialize((struct cl_enginefunc_s*)g_pH9EngineFuncs, CLDLL_INTERFACE_VERSION, &s_host))
	{
		gEngfuncs.Con_Printf("r_primext: %s did not start, drawing with the engine\n", PXR_MODULE_NAME);
		FreeLibrary(s_module);
		s_module = nullptr;
		return false;
	}
	return true;
}
#endif

// ---- the engine's calls --------------------------------------------------------------------------------------------

extern "C" int DLLEXPORT HUD_GetRenderInterface(int version, struct render_api_s* renderfuncs, struct render_interface_s* callback)
{
	H9Pxr_RegisterCvars();
	s_startValue = H9Pxr_StartValue();
	gEngfuncs.Cvar_Set("r_primext", s_startValue ? "1" : "0"); // the value in force, until config.cfg sets it again
	gEngfuncs.Cvar_Set("r_primext_default", "1"); // config.cfg has no line for it to set back: saved as 1 at exit
	if (s_startValue == 0)
		return 0; // the engine draws (ref_gl), as before

#ifdef H9_PRIMEXT
	if (!H9Pxr_Load())
		return 0;

	if (!s_api.GetRenderInterface(version, renderfuncs, callback))
	{
		gEngfuncs.Con_Printf("r_primext: the PrimeXT renderer refused the engine's render interface %d, drawing with the engine\n", version);
		s_api.Shutdown();
		FreeLibrary(s_module);
		s_module = nullptr;
		return 0;
	}

	s_active = true;
	gEngfuncs.Con_Printf("r_primext 1: PrimeXT's renderer draws the scene\n");
	return 1;
#else
	gEngfuncs.Con_Printf("r_primext: this build has no PrimeXT renderer, drawing with the engine\n");
	return 0;
#endif
}

bool H9Pxr_Active()
{
	return s_active;
}

void H9Pxr_StudioInterface(int version, struct engine_studio_api_s* pstudio)
{
#ifdef H9_PRIMEXT
	if (s_active)
	{
		r_studio_interface_t* unused = nullptr; // the engine keeps reTS's interface for anything ref_gl still draws
		s_api.GetStudioModelInterface(version, &unused, pstudio);
	}
#endif
}

void H9Pxr_HudInit()
{
	H9Pxr_RegisterCvars();
	if (!s_active)
		H9Pxr_RegisterSavedCvars();
#ifdef H9_PRIMEXT
	if (s_active)
		s_api.HudInit();
#endif
}

// gl_hdr with PrimeXT: 1 only. With 0 its frame is black (from the next map load on, when the HDR target is no longer
// made), and a change in the middle of a map crashes it (PrimeXT issue #301). Any other value is put back before the
// next frame, with a console line.
#ifdef H9_PRIMEXT
static void H9Pxr_HoldHdr()
{
	const char* now = gEngfuncs.pfnGetCvarString("gl_hdr");
	if (now && atof(now) == 1.0)
		return;
	gEngfuncs.Con_Printf("gl_hdr %s: not with r_primext 1 (PrimeXT draws a black screen without HDR); kept at 1\n", now ? now : "");
	gEngfuncs.Cvar_Set("gl_hdr", "1");
}
#endif

void H9Pxr_VidInit()
{
	s_oldDefaultPending = false; // config.cfg has run: a 0 from here on is the player's
	H9Pxr_QualityArmAtMapLoad();
#ifdef H9_PRIMEXT
	if (s_active)
	{
		H9Pxr_HoldHdr(); // before R_VidInit, which makes the HDR target only when gl_hdr is 1
		s_api.VidInit();
	}
#endif
}

void H9Pxr_Shutdown()
{
#ifdef H9_PRIMEXT
	if (s_active)
	{
		s_api.Shutdown();
		s_active = false;
		// the module stays mapped until the process ends: the engine may still hold its callbacks
	}
#endif
}

// HUD_AddEntity, for an entity reTS lets through: the PrimeXT renderer lists it. The engine still gets 1, so it
// keeps its own effects for it (EF_BRIGHTLIGHT and the other engine dlights, which PrimeXT draws).
void H9Pxr_ViewParams(struct ref_params_s* pparams)
{
#ifdef H9_PRIMEXT
	if (s_active)
		s_api.ViewParams(pparams);
#endif
}

void H9Pxr_AddEntity(int type, cl_entity_t* ent)
{
#ifdef H9_PRIMEXT
	if (s_active)
		s_api.AddEntity(type, (struct cl_entity_s*)ent);
#endif
}

static void H9Pxr_Hold(const char* name, const char* value)
{
	const char* now = gEngfuncs.pfnGetCvarString(name);
	if (now && strcmp(now, value) != 0 && atof(now) != atof(value))
		gEngfuncs.Cvar_Set(name, value);
}

void H9Pxr_Frame(double time)
{
	if (!r_primext)
		return;

	// once, after config.cfg has run: lines that PrimeXT's old cvar names left in it. ref_gl never saves r_shadows, so
	// a value there is PrimeXT's (it drew the engine's stencil shadows in a session without PrimeXT); r_bloom_scale 0.7
	// is PrimeXT's default, written over the engine's 1.0 before PrimeXT's became px_bloom_scale.
	static bool s_oldNamesDone = false;
	if (!s_oldNamesDone)
	{
		s_oldNamesDone = true;
		if (gEngfuncs.pfnGetCvarFloat("r_shadows") != 0.0f)
		{
			gEngfuncs.Cvar_Set("r_shadows", "0");
			gEngfuncs.Con_Printf("r_shadows: set back to 0 (a line PrimeXT left in config.cfg; its shadows are px_shadows)\n");
		}
		const char* bloomScale = gEngfuncs.pfnGetCvarString("r_bloom_scale");
		if (bloomScale && atof(bloomScale) == 0.7)
		{
			gEngfuncs.Cvar_Set("r_bloom_scale", "1.0");
			gEngfuncs.Con_Printf("r_bloom_scale: 0.7 was PrimeXT's default written over the engine's; set back to 1.0 (PrimeXT's is px_bloom_scale)\n");
		}
	}

	// config.cfg's r_primext 0 from an older build's default: back to the value this start has
	if (s_oldDefaultPending && r_primext->value == 0.0f)
	{
		s_oldDefaultPending = false;
		gEngfuncs.Cvar_Set("r_primext", "1");
	}

	H9Pxr_QualityFrame();

	static bool s_changeNoted = false;
	if (!s_changeNoted && (r_primext->value != 0.0f ? 1 : 0) != s_startValue)
	{
		gEngfuncs.Con_Printf("r_primext %d takes effect at the next start (the engine asks for the renderer once, as the client loads)\n", r_primext->value != 0.0f ? 1 : 0);
		s_changeNoted = true;
	}

	if (!s_active)
	{
		// started with r_primext 0: put back the settings an earlier PrimeXT start held, once config.cfg has run
		if (!s_restoreDone && r_primext_restore && r_primext_restore->string && r_primext_restore->string[0])
		{
			float msaa = 0, bloom = 0, scale = 1;
			if (sscanf(r_primext_restore->string, "%f %f %f", &msaa, &bloom, &scale) == 3)
			{
				char buf[32];
				snprintf(buf, sizeof(buf), "%g", msaa);
				gEngfuncs.Cvar_Set("r_scene_msaa", buf);
				snprintf(buf, sizeof(buf), "%g", bloom);
				gEngfuncs.Cvar_Set("r_bloom", buf);
				snprintf(buf, sizeof(buf), "%g", scale);
				gEngfuncs.Cvar_Set("r_scene_scale", buf);
			}
			gEngfuncs.Cvar_Set("r_primext_restore", "");
			s_restoreDone = true;
		}
		return;
	}

#ifdef H9_PRIMEXT
	// H9's off-screen scene target (r_scene_msaa, r_bloom, r_scene_scale) would be presented over PrimeXT's frame
	const float msaa = gEngfuncs.pfnGetCvarFloat("r_scene_msaa");
	const float bloom = gEngfuncs.pfnGetCvarFloat("r_bloom");
	const char* scaleStr = gEngfuncs.pfnGetCvarString("r_scene_scale");
	const float scale = (scaleStr && scaleStr[0]) ? (float)atof(scaleStr) : 1.0f;
	if (msaa != 0.0f || bloom != 0.0f || scale != 1.0f)
	{
		if (r_primext_restore && (!r_primext_restore->string || !r_primext_restore->string[0]))
		{
			char buf[64];
			snprintf(buf, sizeof(buf), "%g %g %g", msaa, bloom, scale);
			gEngfuncs.Cvar_Set("r_primext_restore", buf);
		}
		H9Pxr_Hold("r_scene_msaa", "0");
		H9Pxr_Hold("r_bloom", "0");
		if (scaleStr && scaleStr[0])
			H9Pxr_Hold("r_scene_scale", "1");
	}

	H9Pxr_HoldHdr();

	s_api.Frame(time);
#endif
}
