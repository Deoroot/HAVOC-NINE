/***
*
*	PrimeXT for H9: the server half that lives in mp.dll.
*
*	1. Server_GetPhysicsInterface: the export Xash's SV_InitPhysicsAPI asks the game library for. It sets
*	   no physics callback; it only answers SV_CheckFeatures, which is how the engine learns that the game
*	   wants deluxemaps loaded, etc. Adapted from PrimeXT server/physic.cpp:308-361 (GPL-3.0+, Uncle Mike),
*	   cut down to the feature answer. See reTS/PRIMEXT-SOURCES-SERVER.md.
*	2. env_dynlight: PrimeXT server/lights.cpp:553-830 (Valve HLSDK licence, as reTS), without the movie
*	   (cinema) light, the turn-on/turn-off style transitions and the parent system, which reTS lacks.
*	3. env_postfx_controller and 4. func_monitor, func_portal, info_portal_destination: PrimeXT's keys,
*	   spawnflags and message layout (server/entities/*.cpp there, GPL-3.0+); the code is written new.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"
#include "primext_server.h"

// ---------------------------------------------------------------------------------------------
// 1. Server_GetPhysicsInterface
// ---------------------------------------------------------------------------------------------

// Xash3D FWGS engine/physint.h, interface version 6, FROZEN. Only the layout matters here: every
// slot is a pointer, so a void* stands for each callback this library does not provide.
#define H9_SV_PHYSICS_INTERFACE_VERSION 6

// common/enginefeatures.h
#define H9_ENGINE_WRITE_LARGE_COORD (1 << 0)
#define H9_ENGINE_LOAD_DELUXEDATA (1 << 2)
#define H9_ENGINE_PHYSICS_PUSHER_EXT (1 << 3)
#define H9_ENGINE_LARGE_LIGHTMAPS (1 << 4)
#define H9_ENGINE_COMPENSATE_QUAKE_BUG (1 << 5)
#define H9_ENGINE_LINEAR_GAMMA_SPACE (1 << 8)

// Never settable here (they change the wire protocol or game physics for every client, stock ones too).
#define H9_PXT_FORBIDDEN (H9_ENGINE_WRITE_LARGE_COORD | H9_ENGINE_PHYSICS_PUSHER_EXT | H9_ENGINE_COMPENSATE_QUAKE_BUG)

#ifndef H9_PXT_DEFAULT_FEATURES
// DELUXEDATA: loads a BSPX LIGHTINGDIR / XASH LIGHTVECS lump when the map has one; no effect otherwise.
// LARGE_LIGHTMAPS: 1024 lightmap blocks instead of 128; ref_gl already forces it for BSP2 maps
//   (gl_rsurf.c:3928), so every H9 RP map is unchanged; v30 maps with big lightmaps stop being split.
// LINEAR_GAMMA_SPACE is NOT in the default: it removes the engine's texgamma/lightgamma tables
//   (gamma.c:177-205), which changes the look of every map for the stock renderer.
#define H9_PXT_DEFAULT_FEATURES (H9_ENGINE_LOAD_DELUXEDATA | H9_ENGINE_LARGE_LIGHTMAPS)
#endif

struct h9_physics_interface_t
{
	int version;
	void* SV_CreateEntity;
	void* SV_PhysicsEntity;
	void* SV_LoadEntities;
	void* SV_UpdatePlayerBaseVelocity;
	void* SV_AllowSaveGame;
	// ---- version 6 is frozen above this line; the rest was appended
	void* SV_TriggerTouch;
	unsigned int (*SV_CheckFeatures)(void);
	void* rest[17]; // DrawDebugTriangles ... pfnVoiceData
};

// always printed (ALERT(at_console) needs developer 1 in Xash), so a server log shows what the engine asked
static void PxtLog(const char* fmt, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	g_engfuncs.pfnServerPrint(buffer);
}

unsigned int PrimeXT_EngineFeatures()
{
	unsigned int features = H9_PXT_DEFAULT_FEATURES;

	// Run-time switch, read when the server library loads: H9_PXT_FEATURES=0 turns it all off,
	// =0x14 sets LOAD_DELUXEDATA|LARGE_LIGHTMAPS, =0x114 adds LINEAR_GAMMA_SPACE. (The engine asks
	// before GameDLLInit, so a cvar would not be read yet.)
	const char* env = getenv("H9_PXT_FEATURES");
	if (env && *env)
		features = (unsigned int)strtoul(env, nullptr, 0);

	features &= ~H9_PXT_FORBIDDEN;
	return features;
}

static unsigned int H9_SV_CheckFeatures(void)
{
	const unsigned int features = PrimeXT_EngineFeatures();
	PxtLog("PrimeXT: SV_CheckFeatures -> 0x%x (deluxedata %d, large lightmaps %d, linear gamma %d)\n", features,
		(features & H9_ENGINE_LOAD_DELUXEDATA) ? 1 : 0, (features & H9_ENGINE_LARGE_LIGHTMAPS) ? 1 : 0,
		(features & H9_ENGINE_LINEAR_GAMMA_SPACE) ? 1 : 0);
	return features;
}

// Called by the engine on the library it loaded as the game: with Metamod-R in the way that is
// metamod.dll, which has to forward here (see reTS/PRIMEXT-METAMOD.md); without it, this library.
extern "C" DLLEXPORT int Server_GetPhysicsInterface(int iVersion, void* pfuncsFromEngine, h9_physics_interface_t* pFunctionTable)
{
	PxtLog("PrimeXT: Server_GetPhysicsInterface(version %d) called\n", iVersion);

	if (iVersion != H9_SV_PHYSICS_INTERFACE_VERSION || pFunctionTable == nullptr)
		return 0;

	pFunctionTable->version = H9_SV_PHYSICS_INTERFACE_VERSION;
	pFunctionTable->SV_CheckFeatures = H9_SV_CheckFeatures;
	return 1;
}

// ---------------------------------------------------------------------------------------------
// 2. env_dynlight
// ---------------------------------------------------------------------------------------------

#define SF_DYNLIGHT_STARTOFF 1
#define SF_DYNLIGHT_NOSHADOW 2
#define SF_DYNLIGHT_NOBUMP 4
#define SF_DYNLIGHT_FLARE 8

class CDynamicLight : public CPointEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT PVSThink();

private:
	void UpdatePVSPoint();
	void UpdateState();

	int m_iStyle = 0;
	string_t m_iszPattern = 0;
	float m_flBrightness = 2.0f;
};

LINK_ENTITY_TO_CLASS(env_dynlight, CDynamicLight);

TYPEDESCRIPTION CDynamicLight::m_SaveData[] =
	{
		DEFINE_FIELD(CDynamicLight, m_iStyle, FIELD_INTEGER),
		DEFINE_FIELD(CDynamicLight, m_iszPattern, FIELD_STRING),
		DEFINE_FIELD(CDynamicLight, m_flBrightness, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CDynamicLight, CPointEntity);

// The model an env_dynlight is sent with (an entity needs one to be sent). The stock client draws it,
// so it is a blank sprite; H9_PXT_NULLSPR names a visible one to see where the lights are (debug).
static const char* NullSprite()
{
	const char* env = getenv("H9_PXT_NULLSPR");
	return (env && *env) ? env : "sprites/null.spr";
}
#define g_szNullSprite NullSprite()

bool CDynamicLight::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "brightness"))
	{
		m_flBrightness = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	// PrimeXT's style fades and its movie (message) are not carried
	else if (FStrEq(pkvd->szKeyName, "m_iOnStyle") || FStrEq(pkvd->szKeyName, "m_iOffStyle") ||
		FStrEq(pkvd->szKeyName, "m_iTurnOnStyle") || FStrEq(pkvd->szKeyName, "m_iTurnOffStyle") ||
		FStrEq(pkvd->szKeyName, "m_iTurnOnTime") || FStrEq(pkvd->szKeyName, "m_iTurnOffTime"))
	{
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CDynamicLight::Precache()
{
	PRECACHE_MODEL(g_szNullSprite);
}

void CDynamicLight::Spawn()
{
	// a missing sprite would be a Host_Error in the engine; a light is not worth that
	int length = 0;
	byte* file = LOAD_FILE_FOR_ME(g_szNullSprite, &length);
	if (!file)
	{
		ALERT(at_error, "env_dynlight at (%.0f %.0f %.0f): %s is missing, light removed\n", pev->origin.x, pev->origin.y, pev->origin.z, g_szNullSprite);
		REMOVE_ENTITY(ENT(pev));
		return;
	}
	FREE_FILE(file);

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	Precache();

	pev->renderfx = H9_RENDERFX_DYNLIGHT;
	SET_MODEL(ENT(pev), g_szNullSprite); // must have a model to be sent to the client

	// renderamt is the distance in units / 8 (8 bits on the wire: up to 2040 units)
	pev->renderamt = floorf(pev->renderamt / 8.0f);
	if (pev->renderamt < 1)
		pev->renderamt = 1;
	if (pev->renderamt > 255)
		pev->renderamt = 255;

	// framerate is signed, 8 bits, 1/16 steps on the wire: -8..7.9375
	if (m_flBrightness < 0)
		m_flBrightness = 0;
	if (m_flBrightness > 7.9f)
		m_flBrightness = 7.9f;

	if (m_iStyle >= 32)
		LIGHT_STYLE(m_iStyle, !FStringNull(m_iszPattern) ? (char*)STRING(m_iszPattern) : "m");
	UpdateState();

	if (pev->scale > 0)
	{
		// spotlight: a second PVS point where the cone ends, so the pool of light is sent when the
		// light itself is behind a wall
		CBaseEntity* pHelper = CBaseEntity::Create("info_target", pev->origin, g_vecZero, edict());
		if (pHelper)
		{
			pev->enemy = pHelper->edict();
			SET_MODEL(pHelper->edict(), g_szNullSprite);
			UTIL_SetSize(pHelper->pev, Vector(-8, -8, -8), Vector(8, 8, 8));
		}
		// stay linked into the PVS (renderamt is distance / 8)
		UTIL_SetSize(pev, Vector(-pev->renderamt, -pev->renderamt, -pev->renderamt), Vector(pev->renderamt, pev->renderamt, pev->renderamt));
	}
	else
	{
		UTIL_SetSize(pev, Vector(-pev->renderamt * 4, -pev->renderamt * 4, -pev->renderamt * 4), Vector(pev->renderamt * 4, pev->renderamt * 4, pev->renderamt * 4));
	}

	if (FBitSet(pev->spawnflags, SF_DYNLIGHT_STARTOFF))
	{
		pev->effects |= EF_NODRAW;
	}
	else if (pev->scale > 0)
	{
		SetThink(&CDynamicLight::PVSThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CDynamicLight::UpdateState()
{
	pev->framerate = m_flBrightness;
	pev->skin = m_iStyle & 0x3F; // lightstyle index in the first 6 bits
}

void CDynamicLight::UpdatePVSPoint()
{
	if (!pev->enemy)
		return;

	TraceResult tr;
	Vector forward;

	UTIL_MakeVectorsPrivate(pev->angles, forward, nullptr, nullptr);
	Vector vecSrc = pev->origin + forward * 8.0f;
	Vector vecEnd = vecSrc + forward * (pev->renderamt * 8.0f);
	UTIL_TraceLine(vecSrc, vecEnd, ignore_monsters, edict(), &tr);

	// the second PVS point: where the cone hits, a little off the surface
	UTIL_SetOrigin(VARS(pev->enemy), tr.vecEndPos + tr.vecPlaneNormal * 8.0f);
}

void CDynamicLight::PVSThink()
{
	UpdatePVSPoint();
	UpdateState();
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDynamicLight::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const bool on = !FBitSet(pev->effects, EF_NODRAW);

	if (!ShouldToggle(useType, on))
		return;

	// off = EF_NODRAW: AddToFullPack does not send the entity, and the client drops the light
	if (on)
		pev->effects |= EF_NODRAW;
	else
		pev->effects &= ~EF_NODRAW;

	if (FBitSet(pev->effects, EF_NODRAW))
	{
		pev->nextthink = 0;
	}
	else if (pev->scale > 0)
	{
		SetThink(&CDynamicLight::PVSThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

// ---------------------------------------------------------------------------------------------
// 3. env_postfx_controller
// ---------------------------------------------------------------------------------------------
// The keys, the spawnflags and the layout of the PostFxSettings message are PrimeXT's
// (server/entities/env_postfx_controller.cpp, GPL-3.0+, SNMetamorph 2022); the text below is written new.
// The message is read by pxrender.dll (cl_dll/primext/client/postfx_parameters.cpp, ParseMessage):
// 13 floats, fade time first. A client without pxrender.dll has no hook for it and drops it.

#define SF_POSTFX_FADE_FROM 1 // send the neutral values: the look fades back to normal
#define SF_POSTFX_ACTIVATOR 2 // only the player who fired it sees the change

int gmsgPostFxSettings = 0;

void PrimeXT_LinkUserMessages()
{
	gmsgPostFxSettings = REG_USER_MSG("PostFxSettings", 13 * 4);
}

class CEnvPostFxController : public CPointEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	// in the order of the message
	enum
	{
		FADE,
		BRIGHTNESS,
		SATURATION,
		CONTRAST,
		RED,
		GREEN,
		BLUE,
		VIGNETTE,
		GRAIN,
		ACCENT_SCALE,
		VALUE_COUNT
	};

	// The defaults are those PrimeXT's fgd writes (a neutral look, 3 s fade), so a controller that leaves a key
	// out does not turn the screen grey. (PrimeXT's members start at 0: saturation 0 is black and white.)
	float m_flValue[VALUE_COUNT] = {3.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
};

LINK_ENTITY_TO_CLASS(env_postfx_controller, CEnvPostFxController);

TYPEDESCRIPTION CEnvPostFxController::m_SaveData[] =
	{
		DEFINE_ARRAY(CEnvPostFxController, m_flValue, FIELD_FLOAT, CEnvPostFxController::VALUE_COUNT),
};

IMPLEMENT_SAVERESTORE(CEnvPostFxController, CPointEntity);

bool CEnvPostFxController::KeyValue(KeyValueData* pkvd)
{
	static const char* const keys[VALUE_COUNT] = {"fade_in_time", "brightness", "saturation", "contrast", "red_level",
		"green_level", "blue_level", "vignette_scale", "film_grain_scale", "color_accent_scale"};
	for (int i = 0; i < VALUE_COUNT; i++)
	{
		if (FStrEq(pkvd->szKeyName, keys[i]))
		{
			m_flValue[i] = atof(pkvd->szValue);
			return true;
		}
	}
	return CPointEntity::KeyValue(pkvd);
}

void CEnvPostFxController::Spawn()
{
	// a point entity that is never sent; the accent colour is its rendercolor (0..255), sent as 0..1
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CEnvPostFxController::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const bool fadeFrom = FBitSet(pev->spawnflags, SF_POSTFX_FADE_FROM);

	if (FBitSet(pev->spawnflags, SF_POSTFX_ACTIVATOR))
	{
		// fired by no player (a timer, the console): nobody to send it to, as in PrimeXT
		if (!pActivator || !pActivator->IsNetClient())
		{
			SUB_UseTargets(pActivator, USE_TOGGLE, 0);
			return;
		}
		MESSAGE_BEGIN(MSG_ONE, gmsgPostFxSettings, nullptr, pActivator->edict());
	}
	else
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgPostFxSettings);
	}

	// fade from: the neutral look (PrimeXT's CPostFxParameters::Defaults), over this controller's fade time
	static const float neutral[VALUE_COUNT] = {0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
	WRITE_FLOAT(m_flValue[FADE]);
	for (int i = BRIGHTNESS; i < VALUE_COUNT; i++)
		WRITE_FLOAT(fadeFrom ? neutral[i] : m_flValue[i]);
	WRITE_FLOAT(fadeFrom ? 1.0f : pev->rendercolor.x / 255.0f);
	WRITE_FLOAT(fadeFrom ? 1.0f : pev->rendercolor.y / 255.0f);
	WRITE_FLOAT(fadeFrom ? 1.0f : pev->rendercolor.z / 255.0f);
	MESSAGE_END();

	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
}

// ---------------------------------------------------------------------------------------------
// 4. func_monitor, func_portal, info_portal_destination
// ---------------------------------------------------------------------------------------------
// Keys and spawnflags are PrimeXT's (server/entities/func_monitor.cpp, func_portal.cpp, GPL-3.0+, Uncle Mike
// 2011); the text below is written new for reTS, which has no parent system, no m_iState and no matrix4x4.
// What the client is sent (PRIMEXT-ENTITIES.md): renderfx H9_RENDERFX_SUBVIEW, the camera in aiment, the
// field of view in controller[1], H9_SUBVIEW_MONOCHROME in controller[0]. The renderer finds the faces to
// draw into by their texture name (monitor*, portal*).

#define SF_MONITOR_START_ON 1
#define SF_MONITOR_PASSABLE 2
#define SF_MONITOR_USEABLE 4 // PrimeXT: a player who uses it looks through the camera. Not carried.
#define SF_MONITOR_SQUARE 8	 // in PrimeXT's fgd; neither PrimeXT's server nor its renderer reads it
#define SF_MONITOR_MONOCHROME 16

#define SF_PORTAL_START_OFF 1

// Xash's EF_MERGE_VISIBILITY (common/const.h): the engine adds what the camera sees to the packet of every
// client that is sent an entity whose aiment is the camera (sv_frame.c SV_AddEntitiesToPacket). Server side
// only: effects is 8 bits on the wire, so the bit never reaches a client.
#define H9_EF_MERGE_VISIBILITY (1 << 29)

class CFuncSubview : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;
	void Activate() override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void EXPORT VisThink();

	// a func_monitor is MOVETYPE_PUSH, whose think time is counted on its own clock, pev->ltime (PrimeXT's SetNextThink
	// does this for it); gpGlobals->time there made the think run seconds apart, not every 0.1 s
	void NextThink(float delay) { pev->nextthink = (pev->movetype == MOVETYPE_PUSH ? pev->ltime : gpGlobals->time) + delay; }

	// whether some client (spectators too) has this screen in its PVS
	bool SeenByAClient();

	// AddToFullPack: the field of view and the switches (PRIMEXT-ENTITIES.md)
	void FillState(struct entity_state_s* state)
	{
		const float fov = m_flFov > 0 ? m_flFov : 90.0f;
		state->controller[1] = (byte)(fov < 10 ? 10 : (fov > 120 ? 120 : fov));
		state->controller[0] = m_fMonochrome ? H9_SUBVIEW_MONOCHROME : 0;
	}

protected:
	CBaseEntity* Camera();
	void SetOn(bool on);
	bool IsOn() const { return m_fOn; }
	bool IsLocked(CBaseEntity* pActivator) { return !FStringNull(m_sMaster) && !UTIL_IsMasterTriggered(m_sMaster, pActivator); }

	bool m_fOn = false;
	bool m_fMonochrome = false;
	string_t m_sMaster = 0;
	float m_flFov = 0;
};

TYPEDESCRIPTION CFuncSubview::m_SaveData[] =
	{
		DEFINE_FIELD(CFuncSubview, m_fOn, FIELD_BOOLEAN),
		DEFINE_FIELD(CFuncSubview, m_fMonochrome, FIELD_BOOLEAN),
		DEFINE_FIELD(CFuncSubview, m_sMaster, FIELD_STRING),
		DEFINE_FIELD(CFuncSubview, m_flFov, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CFuncSubview, CBaseEntity);

bool CFuncSubview::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "camera")) // func_monitor; func_portal names its exit in "target"
	{
		pev->target = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "fov"))
	{
		m_flFov = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "firetarget")) // func_portal: fired when something passes
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "reflection")) // PrimeXT's Reflection base class: a renderer hint, not carried
		return true;
	return CBaseEntity::KeyValue(pkvd);
}

void CFuncSubview::Precache()
{
	PRECACHE_MODEL(g_szNullSprite);
}

// The camera (func_monitor) or the exit (func_portal), by targetname. It has to be sent to the client to be
// looked through; an info_target is a model-less point entity in reTS, so it is given the blank sprite.
CBaseEntity* CFuncSubview::Camera()
{
	if (FStringNull(pev->target))
		return nullptr;
	CBaseEntity* pCamera = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
	if (pCamera && pCamera->pev->modelindex == 0)
	{
		SET_MODEL(pCamera->edict(), g_szNullSprite);
		UTIL_SetSize(pCamera->pev, g_vecZero, g_vecZero);
	}
	return pCamera;
}

void CFuncSubview::Activate()
{
	// every entity has spawned: the camera can be found now
	if (!FStringNull(pev->target) && !Camera())
		ALERT(at_console, "%s at (%.0f %.0f %.0f): camera '%s' not found\n", STRING(pev->classname), pev->absmin.x,
			pev->absmin.y, pev->absmin.z, STRING(pev->target));
	SetOn(m_fOn);
}

void CFuncSubview::SetOn(bool on)
{
	m_fOn = on;
	CBaseEntity* pCamera = Camera();

	// on: aiment names the camera (the client looks through it, the engine merges its view);
	// off: no camera, the faces show their own texture (func_monitor) or nothing (func_portal: EF_NODRAW)
	pev->aiment = (on && pCamera) ? pCamera->edict() : nullptr;
	if (pCamera)
	{
		if (on)
			pCamera->pev->effects |= H9_EF_MERGE_VISIBILITY;
		else
			pCamera->pev->effects &= ~H9_EF_MERGE_VISIBILITY;
	}

	if (on)
	{
		SetThink(&CFuncSubview::VisThink);
		NextThink(0.1f);
	}
	else
	{
		SetThink(nullptr);
	}
}

// PrimeXT's VisThink: the camera's view is merged only while some client can see the screen
void CFuncSubview::VisThink()
{
	CBaseEntity* pCamera = pev->aiment ? CBaseEntity::Instance(pev->aiment) : nullptr;
	if (pCamera && !pCamera->IsPlayer())
	{
		// PrimeXT asks FIND_CLIENT_IN_PVS, which passes over a client with no health: a spectator (H9's players are
		// spectators until they join) then never had the camera's view merged, and the screen showed the camera from
		// wherever the client last saw it (or from 0 0 0). Here every connected client's own PVS is asked.
		if (SeenByAClient())
			pCamera->pev->effects |= H9_EF_MERGE_VISIBILITY;
		else
			pCamera->pev->effects &= ~H9_EF_MERGE_VISIBILITY;
	}
	NextThink(0.1f);
}

bool CFuncSubview::SeenByAClient()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || FBitSet(pPlayer->pev->flags, FL_FAKECLIENT))
			continue;
		Vector org = pPlayer->pev->origin + pPlayer->pev->view_ofs;
		unsigned char* pvs = ENGINE_SET_PVS((float*)&org);
		if (0 != ENGINE_CHECK_VISIBILITY(edict(), pvs))
			return true;
	}
	return false;
}

class CFuncMonitor : public CFuncSubview
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(func_monitor, CFuncMonitor);

void CFuncMonitor::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_PUSH;
	pev->solid = FBitSet(pev->spawnflags, SF_MONITOR_PASSABLE) ? SOLID_NOT : SOLID_BSP;
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);

	pev->renderfx = H9_RENDERFX_SUBVIEW;
	pev->renderamt = 255; // the engine sets renderamt 255 itself only for renderfx 0

	m_fMonochrome = FBitSet(pev->spawnflags, SF_MONITOR_MONOCHROME);
	// Start ON takes effect in Activate, once the camera has spawned
	m_fOn = FBitSet(pev->spawnflags, SF_MONITOR_START_ON);
}

void CFuncMonitor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (IsLocked(pActivator))
		return;
	if (useType == USE_SET)
		return; // PrimeXT: USE_SET / USE_RESET are a player at a usable monitor. Not carried (reTS has no USE_RESET).
	if (!ShouldToggle(useType, IsOn()))
		return;
	SetOn(!IsOn());
}

class CFuncPortal : public CFuncSubview
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Touch(CBaseEntity* pOther) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_flDisableTime = 0;
};

LINK_ENTITY_TO_CLASS(func_portal, CFuncPortal);

TYPEDESCRIPTION CFuncPortal::m_SaveData[] =
	{
		DEFINE_FIELD(CFuncPortal, m_flDisableTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CFuncPortal, CFuncSubview);

void CFuncPortal::Spawn()
{
	Precache();
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->rendermode = kRenderNormal; // PrimeXT: see-through portals look wrong
	pev->renderamt = 255;
	pev->renderfx = H9_RENDERFX_SUBVIEW;
	SET_MODEL(edict(), STRING(pev->model));

	// the portal's frame is its origin brush: without one, its origin and angles are the world's
	if (pev->origin == g_vecZero)
	{
		ALERT(at_error, "func_portal with model %s has no origin brush: removed\n", STRING(pev->model));
		REMOVE_ENTITY(edict());
		return;
	}
	UTIL_SetOrigin(pev, pev->origin);

	m_fOn = !FBitSet(pev->spawnflags, SF_PORTAL_START_OFF);
	if (!m_fOn)
		pev->effects |= EF_NODRAW;
}

void CFuncPortal::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (IsLocked(pActivator))
		return;
	if (!ShouldToggle(useType, IsOn()))
		return;
	if (IsOn())
		pev->effects |= EF_NODRAW;
	else
		pev->effects &= ~EF_NODRAW;
	SetOn(!IsOn());
}

// The frame of (origin, angles): x forward, y left, z up, as PrimeXT's matrix4x4(origin, angles)
static void FrameAxes(const Vector& angles, Vector& forward, Vector& left, Vector& up)
{
	Vector right;
	UTIL_MakeVectorsPrivate(angles, forward, right, up);
	left = -right;
}

static Vector ToLocal(const Vector& v, const Vector& f, const Vector& l, const Vector& u)
{
	return Vector(DotProduct(v, f), DotProduct(v, l), DotProduct(v, u));
}

static Vector ToWorld(const Vector& v, const Vector& f, const Vector& l, const Vector& u)
{
	return f * v.x + l * v.y + u * v.z;
}

void CFuncPortal::Touch(CBaseEntity* pOther)
{
	if (!IsOn() || m_flDisableTime > gpGlobals->time || IsLocked(pOther))
		return;
	if (!pOther->IsPlayer() && !FBitSet(pOther->pev->flags, FL_MONSTER))
		return; // PrimeXT also takes pushables, projectiles and rigid bodies; here players and monsters

	CBaseEntity* pExit = Camera();
	if (!pExit)
		return;

	Vector f1, l1, u1, f2, l2, u2;
	FrameAxes(pev->angles, f1, l1, u1);
	FrameAxes(pExit->pev->angles, f2, l2, u2);

	// one way: only into the portal's forward side (PrimeXT: no two-sided recursion)
	if (DotProduct(f1, pOther->pev->velocity.Normalize()) <= 0.0f)
		return;

	m_flDisableTime = gpGlobals->time + 0.5f;

	// position, look and velocity carried from this frame into the exit's
	const Vector newOrigin = pExit->pev->origin + ToWorld(ToLocal(pOther->pev->origin - pev->origin, f1, l1, u1), f2, l2, u2);

	TraceResult tr;
	UTIL_TraceHull(newOrigin, newOrigin, dont_ignore_monsters, FBitSet(pOther->pev->flags, FL_DUCKING) ? head_hull : human_hull,
		pOther->edict(), &tr);
	if (0 != tr.fAllSolid || 0 != tr.fStartSolid)
	{
		pOther->pev->velocity = g_vecZero; // PrimeXT: no room at the exit
		return;
	}

	Vector look, right, up;
	UTIL_MakeVectorsPrivate(pOther->IsPlayer() ? pOther->pev->v_angle : pOther->pev->angles, look, right, up);
	const Vector newLook = ToWorld(ToLocal(look, f1, l1, u1), f2, l2, u2);
	const Vector newVelocity = ToWorld(ToLocal(pOther->pev->velocity, f1, l1, u1), f2, l2, u2);

	Vector angles = UTIL_VecToAngles(newLook);
	pOther->pev->flags &= ~FL_ONGROUND;
	UTIL_SetOrigin(pOther->pev, newOrigin);
	pOther->pev->velocity = newVelocity;
	if (pOther->IsPlayer())
	{
		angles.x = -angles.x; // a view's pitch is positive down
		angles.z = pExit->pev->angles.z;
		pOther->pev->angles = angles;
		pOther->pev->v_angle = angles;
		pOther->pev->fixangle = 1;
	}
	else
	{
		pOther->pev->angles = angles;
	}

	if (!FStringNull(pev->netname))
		FireTargets(STRING(pev->netname), pOther, this, USE_TOGGLE, 0);
}

// The exit of a func_portal. The client looks out of it, so it is sent: it has the blank sprite.
class CInfoPortalDest : public CPointEntity
{
public:
	void Spawn() override
	{
		PRECACHE_MODEL(g_szNullSprite);
		SET_MODEL(edict(), g_szNullSprite);
		UTIL_SetSize(pev, g_vecZero, g_vecZero);
	}
};

LINK_ENTITY_TO_CLASS(info_portal_destination, CInfoPortalDest);

// ---------------------------------------------------------------------------------------------
// AddToFullPack hooks
// ---------------------------------------------------------------------------------------------

bool PrimeXT_DynlightVisible(edict_t* ent, unsigned char* pSet)
{
	if (ent->v.renderfx != H9_RENDERFX_DYNLIGHT || !ent->v.enemy)
		return false;
	return 0 != ENGINE_CHECK_VISIBILITY((const struct edict_s*)ent->v.enemy, pSet);
}

void PrimeXT_AddToFullPack(struct entity_state_s* state, edict_t* ent)
{
	if (ent->v.renderfx == H9_RENDERFX_SUBVIEW)
	{
		// func_monitor / func_portal: the camera is already in state->aiment (AddToFullPack copies pev->aiment)
		CFuncSubview* pView = dynamic_cast<CFuncSubview*>(CBaseEntity::Instance(ent));
		if (pView)
			pView->FillState(state);
		return;
	}

	if (ent->v.renderfx != H9_RENDERFX_DYNLIGHT)
		return;

	// the dynamic light's switches ride in controller[0] (see PRIMEXT-ENTITIES.md)
	int flags = 0;
	if (FBitSet(ent->v.spawnflags, SF_DYNLIGHT_NOSHADOW))
		flags |= H9_DYNLIGHT_NOSHADOW;
	if (FBitSet(ent->v.spawnflags, SF_DYNLIGHT_NOBUMP))
		flags |= H9_DYNLIGHT_NOBUMP;
	if (FBitSet(ent->v.spawnflags, SF_DYNLIGHT_FLARE))
		flags |= H9_DYNLIGHT_FLARE;
	state->controller[0] = (byte)flags;
}

// ---------------------------------------------------------------------------------------------
// h9_pxt_lights: server console dump of what AddToFullPack sends for each env_dynlight (for tests)
// ---------------------------------------------------------------------------------------------

int AddToFullPack(struct entity_state_s* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, unsigned char* pSet);

static void Cmd_PrimeXTLights()
{
	edict_t* host = INDEXENT(1);
	int count = 0;
	for (int e = gpGlobals->maxClients + 1; e < gpGlobals->maxEntities; e++)
	{
		edict_t* ent = INDEXENT(e);
		if (!ent || ent->free || ent->v.renderfx != H9_RENDERFX_DYNLIGHT)
			continue;
		count++;
		entity_state_t st;
		const int sent = AddToFullPack(&st, e, ent, host, 0, 0, nullptr);
		PxtLog("h9_pxt_lights: #%d %s name '%s' origin %.0f %.0f %.0f: %s\n", e, STRING(ent->v.classname), STRING(ent->v.targetname),
			ent->v.origin.x, ent->v.origin.y, ent->v.origin.z, sent ? "SENT" : "not sent (off)");
		if (!sent)
			continue;
		// the values after the delta quantisation of delta.lst (framerate 1/16 signed 8 bit, scale 1/256 16 bit)
		PxtLog("  renderfx %d rendercolor %d %d %d renderamt %d (radius %d) framerate %.4f (wire %.4f) scale %.2f (wire %.4f)"
			   " skin %d rendermode %d controller0 0x%02x modelindex %d effects 0x%x angles %.0f %.0f %.0f\n",
			st.renderfx, st.rendercolor.r, st.rendercolor.g, st.rendercolor.b, st.renderamt, st.renderamt * 8,
			st.framerate, floorf(st.framerate * 16.0f + 0.5f) / 16.0f, st.scale, floorf(st.scale * 256.0f + 0.5f) / 256.0f,
			st.skin, st.rendermode, st.controller[0], st.modelindex, st.effects, st.angles.x, st.angles.y, st.angles.z);
	}
	PxtLog("h9_pxt_lights: %d env_dynlight\n", count);
}

// h9_pxt_use <targetname> [on|off|toggle] [<player index>]: fire the targets of that name (test aid). With a player
// index that player is the activator (an Activator Only env_postfx_controller then sends to that player only).
static void Cmd_PrimeXTUse()
{
	const char* name = CMD_ARGV(1);
	const char* how = CMD_ARGC() > 2 ? CMD_ARGV(2) : "toggle";
	USE_TYPE type = USE_TOGGLE;
	if (FStrEq(how, "on"))
		type = USE_ON;
	else if (FStrEq(how, "off"))
		type = USE_OFF;
	CBaseEntity* pActivator = nullptr;
	if (CMD_ARGC() > 3)
	{
		const int index = atoi(CMD_ARGV(3));
		if (index >= 1 && index <= gpGlobals->maxClients)
			pActivator = UTIL_PlayerByIndex(index);
	}
	FireTargets(name, pActivator, pActivator, type, 0);
	PxtLog("h9_pxt_use: %s %s%s\n", name, how, pActivator ? " (activator: that player)" : "");
}

// h9_pxt_screens: every func_monitor / func_portal, its camera, and whether that camera would be sent (test aid)
static void Cmd_PrimeXTScreens()
{
	edict_t* host = INDEXENT(1);
	int count = 0;
	for (int e = gpGlobals->maxClients + 1; e < gpGlobals->maxEntities; e++)
	{
		edict_t* ent = INDEXENT(e);
		if (!ent || ent->free || ent->v.renderfx != H9_RENDERFX_SUBVIEW)
			continue;
		count++;
		edict_t* cam = ent->v.aiment;
		CFuncSubview* pView = dynamic_cast<CFuncSubview*>(CBaseEntity::Instance(ent));
		PxtLog("h9_pxt_screens: #%d %s '%s' seen by a client %d, think at %.2f (ltime %.2f, time %.2f), camera %d", e, STRING(ent->v.classname),
			STRING(ent->v.targetname), pView ? (int)pView->SeenByAClient() : -1, ent->v.nextthink, ent->v.ltime, gpGlobals->time, cam ? ENTINDEX(cam) : 0);
		if (!cam)
		{
			PxtLog(" (off)\n");
			continue;
		}
		Vector org = cam->v.origin;
		unsigned char* pvs = ENGINE_SET_PVS((float*)&org);
		const int seenByItself = ENGINE_CHECK_VISIBILITY(cam, pvs);
		entity_state_t st;
		const int sent = AddToFullPack(&st, ENTINDEX(cam), cam, host, 0, 0, pvs);
		PxtLog(" '%s' at %.0f %.0f %.0f model '%s' (%d) merge %d, in its own PVS %d, AddToFullPack %d\n", STRING(cam->v.targetname),
			org.x, org.y, org.z, STRING(cam->v.model), cam->v.modelindex, (cam->v.effects & H9_EF_MERGE_VISIBILITY) ? 1 : 0, seenByItself, sent);
	}
	PxtLog("h9_pxt_screens: %d\n", count);
}

// h9_pxt_touch [prefix] [fire]: player 1's box and, for each trigger whose target starts with prefix, whether the boxes
// meet; with "fire", each trigger whose box meets the player's is touched by him (its own Touch, as the engine calls it)
// (test aid for env_postfx_controller places)
static void Cmd_PrimeXTTouch()
{
	const char* prefix = CMD_ARGC() > 1 ? CMD_ARGV(1) : "";
	const bool fire = CMD_ARGC() > 2 && FStrEq(CMD_ARGV(2), "fire");
	edict_t* pl = INDEXENT(1);
	if (!pl || pl->free)
		return;
	PxtLog("h9_pxt_touch: player origin %.0f %.0f %.0f abs %.0f %.0f %.0f .. %.0f %.0f %.0f solid %d movetype %d flags 0x%x iuser1 %d health %.0f\n",
		pl->v.origin.x, pl->v.origin.y, pl->v.origin.z, pl->v.absmin.x, pl->v.absmin.y, pl->v.absmin.z, pl->v.absmax.x, pl->v.absmax.y, pl->v.absmax.z,
		pl->v.solid, pl->v.movetype, pl->v.flags, pl->v.iuser1, pl->v.health);
	for (int e = gpGlobals->maxClients + 1; e < gpGlobals->maxEntities; e++)
	{
		edict_t* ent = INDEXENT(e);
		if (!ent || ent->free || !FClassnameIs(&ent->v, "trigger_multiple") || FStringNull(ent->v.target) || strncmp(STRING(ent->v.target), prefix, strlen(prefix)))
			continue;
		const bool meet = pl->v.absmin.x <= ent->v.absmax.x && pl->v.absmax.x >= ent->v.absmin.x && pl->v.absmin.y <= ent->v.absmax.y &&
						  pl->v.absmax.y >= ent->v.absmin.y && pl->v.absmin.z <= ent->v.absmax.z && pl->v.absmax.z >= ent->v.absmin.z;
		PxtLog("  #%d -> %s solid %d model %s abs %.0f %.0f %.0f .. %.0f %.0f %.0f boxes meet %d\n", e, STRING(ent->v.target), ent->v.solid,
			STRING(ent->v.model), ent->v.absmin.x, ent->v.absmin.y, ent->v.absmin.z, ent->v.absmax.x, ent->v.absmax.y, ent->v.absmax.z, meet ? 1 : 0);
		if (fire && meet)
			DispatchTouch(ent, pl);
	}
}

void PrimeXT_GameInit()
{
	g_engfuncs.pfnAddServerCommand("h9_pxt_touch", Cmd_PrimeXTTouch);
	g_engfuncs.pfnAddServerCommand("h9_pxt_use", Cmd_PrimeXTUse);
	g_engfuncs.pfnAddServerCommand("h9_pxt_lights", Cmd_PrimeXTLights);
	g_engfuncs.pfnAddServerCommand("h9_pxt_screens", Cmd_PrimeXTScreens);
}
