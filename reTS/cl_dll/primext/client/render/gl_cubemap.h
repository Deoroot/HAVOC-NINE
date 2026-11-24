// gl_cubemap.h - mcubemap_t, one environment cubemap of the map (its probe box, its texture and the
// prefiltered copy used for specular image-based lighting).
// H9: written new for reTS from the interface (the members the renderer uses), replacing PrimeXT's file,
// which had no licence notice. The struct tag stays mcubemap_s: com_model.h refers to it.
#pragma once
#include "vector.h"
#include "mathlib.h"
#include "gl_framebuffer.h"
#include "texture_handle.h"

// Where the renderer is in rebuilding cubemaps (kept for the interface; nothing in this build reads it).
enum CubemapBuildState
{
	CMREBUILD_INACTIVE,
	CMREBUILD_CHECKING,
	CMREBUILD_WAITING  // the engine is still baking
};

typedef struct mcubemap_s
{
	char name[64];                  // texture name
	TextureHandle texture;          // the cubemap as captured
	TextureHandle textureSpecularIBL; // its mip chain filtered for rough-to-smooth specular
	CFrameBuffer fboSpecularIBL;    // render target for that filtering
	CFrameBuffer framebuffer;       // render target for the capture
	vec3_t mins, maxs;              // the box it lights
	vec3_t origin;                  // where it was captured
	bool valid;                     // loaded and usable
	int size;                       // face size in texels
	int numMips;                    // mip levels of the cubemap
} mcubemap_t;
