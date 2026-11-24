/***
*
*	PrimeXT-for-H9, server side. Licence: as the rest of reTS (Valve HLSDK licence, non-commercial).
*	Contract with the client renderer: reTS/PRIMEXT-ENTITIES.md. Sources adapted: reTS/PRIMEXT-SOURCES-SERVER.md.
*
****/
#pragma once

// renderfx values that mark PrimeXT's dynamic light entity on the wire (PrimeXT common/const.h:761).
// H9 uses 64 (xray); reTS's own kRenderFx* run 0..20. Nothing else in reTS or H9 uses 71 or 72.
#define H9_RENDERFX_DYNLIGHT 71
#define H9_RENDERFX_CINEMALIGHT 72 // reserved: env_dynlight with a movie; the server does not emit it

// entity_state_t.controller[0] of an env_dynlight carries these (effects is 8 bits on H9's delta.lst,
// so Xash's EF_NOSHADOW (1<<28), EF_NOBUMP (1<<17) and EF_LENSFLARE (1<<18) would be lost)
#define H9_DYNLIGHT_NOSHADOW 0x01
#define H9_DYNLIGHT_NOBUMP 0x02
#define H9_DYNLIGHT_FLARE 0x04

// Called from AddToFullPack for every entity that is sent, after the render fields are copied.
void PrimeXT_AddToFullPack(struct entity_state_s* state, edict_t* ent);

// True if ent is an env_dynlight whose second PVS point (spotlights) is visible in pSet:
// AddToFullPack then sends the light although its own origin is out of the PVS.
bool PrimeXT_DynlightVisible(edict_t* ent, unsigned char* pSet);

// The engine features the server asks for (ENGINE_* of Xash's enginefeatures.h).
unsigned int PrimeXT_EngineFeatures();

// GameDLLInit: registers the h9_pxt_lights server command
void PrimeXT_GameInit();
