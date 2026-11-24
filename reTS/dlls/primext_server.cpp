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

// h9_pxt_use <targetname> [on|off|toggle]: fire the targets of that name (test aid)
static void Cmd_PrimeXTUse()
{
	const char* name = CMD_ARGV(1);
	const char* how = CMD_ARGC() > 2 ? CMD_ARGV(2) : "toggle";
	USE_TYPE type = USE_TOGGLE;
	if (FStrEq(how, "on"))
		type = USE_ON;
	else if (FStrEq(how, "off"))
		type = USE_OFF;
	FireTargets(name, nullptr, nullptr, type, 0);
	PxtLog("h9_pxt_use: %s %s\n", name, how);
}

void PrimeXT_GameInit()
{
	g_engfuncs.pfnAddServerCommand("h9_pxt_use", Cmd_PrimeXTUse);
	g_engfuncs.pfnAddServerCommand("h9_pxt_lights", Cmd_PrimeXTLights);
}
