// primext_cvars.h - the PrimeXT renderer's saved settings, in one table shared by client.dll and pxrender.dll.
// H9: written new. Plain C, like primext_bridge.h, so both builds include it with their own headers.
//
// Why a table: the settings live in pxrender.dll, which is loaded only with r_primext 1. In a session started with
// r_primext 0 its cvars would not exist, config.cfg's lines for them would be dropped as unknown commands, and the
// config.cfg written at exit would lose them all (and opengl.cfg its gl_hdr). So client.dll registers this table
// itself when it does not load pxrender.dll (h9_primext.cpp), and pxrender.dll registers the same rows
// (gl_cvars.cpp, gl_postprocess.cpp, gl_studio_init.cpp, gl_rpart.cpp) through PXR_CVAR_REGISTER, which takes the
// default and the flags from here: a name that is not in the table does not compile.
//
// The engine sets an existing cvar to the default of a new registration (Cvar_Get), so every row is registered once
// as the client loads, before config.cfg runs, and never again later.
//
// Renamed from PrimeXT's own names, because H9's ref_gl has cvars of those names with another meaning:
//   r_shadows -> px_shadows (ref_gl's r_shadows draws the engine's stencil shadows)
//   r_bloom, r_bloom_scale -> px_bloom, px_bloom_scale (ref_gl's off-screen bloom)
// Not in the table: the debug cvars (r_show_*, gl_decals_debug, ...) and gl_renderer, which are not saved, and the
// one GL-extension cvar per extension (gl_export.cpp), which the engine keeps in opengl.cfg by itself.
#pragma once

#define PXR_CV_A 1           // FCVAR_ARCHIVE: saved in config.cfg
#define PXR_CV_G (1 << 12)   // FCVAR_GLCONFIG: saved in opengl.cfg; read at start only

typedef struct pxr_saved_cvar_s
{
	const char* name;
	const char* value; // the default
	int flags;
} pxr_saved_cvar_t;

// X(name, default, flags)
#define PXR_SAVED_CVARS(X) \
	/* lighting and materials */ \
	X(gl_bump, "1", PXR_CV_A) \
	X(gl_specular, "1", PXR_CV_A) \
	X(gl_parallax, "2", PXR_CV_A) \
	X(r_lighting_brdf, "1", PXR_CV_A) /* H9: on by default, as px_quality 2 (high) sets it */ \
	X(gl_cube_lod_bias, "3", PXR_CV_A) \
	X(r_cubemap_reflection, "1", PXR_CV_A) \
	X(r_cubemap_realtime, "0", PXR_CV_A) \
	X(gl_dynlight, "1", PXR_CV_A) \
	X(gl_lightstyles, "1", PXR_CV_A) \
	/* shadows */ \
	X(px_shadows, "4", PXR_CV_A) \
	X(gl_sun_shadows, "1", PXR_CV_A) \
	X(r_sun_allowed, "0", PXR_CV_A) \
	X(gl_shadowmap_size, "1024", PXR_CV_A) \
	X(gl_shadow_offset, "0.02", PXR_CV_A) \
	X(r_pssm_split_weight, "0.9", PXR_CV_A) \
	X(r_renderplayershadow, "1", PXR_CV_A) \
	/* HDR and post-processing */ \
	X(gl_hdr, "1", PXR_CV_G) \
	X(gl_alpha2coverage, "1", PXR_CV_G) \
	X(gl_posteffects, "1", PXR_CV_A) \
	X(gl_sunshafts, "1", PXR_CV_A) \
	X(r_tonemap, "1", PXR_CV_A) \
	X(px_bloom, "1", PXR_CV_A) \
	X(px_bloom_scale, "0.7", PXR_CV_A) \
	X(r_dof, "1", PXR_CV_A) \
	X(r_dof_hold_time, "0.2", PXR_CV_A) \
	X(r_dof_change_time, "0.8", PXR_CV_A) \
	X(r_dof_focal_length, "600.0", PXR_CV_A) \
	X(r_dof_fstop, "8", PXR_CV_A) \
	X(gl_waterblur, "1", PXR_CV_A) \
	X(gl_glows, "1", PXR_CV_A) \
	/* reflections, portals, puddles */ \
	X(gl_allow_mirrors, "1", PXR_CV_A) \
	X(gl_allow_portals, "1", PXR_CV_A) \
	X(gl_allow_screens, "1", PXR_CV_A) \
	X(gl_allow_3dsky, "1", PXR_CV_A) \
	X(gl_recursion_depth, "1", PXR_CV_A) \
	X(gl_realtime_puddles, "1", PXR_CV_A) /* H9: on (PrimeXT's is 0), as px_quality 2 sets it */ \
	/* decals, grass, culling */ \
	X(gl_decals, "1", PXR_CV_A) \
	X(r_studio_decals, "32", PXR_CV_A) \
	X(r_grass, "1", PXR_CV_A) \
	X(r_grass_alpha, "0.5", PXR_CV_A) \
	X(r_grass_lighting, "1", PXR_CV_A) \
	X(r_grass_shadows, "1", PXR_CV_A) \
	X(r_grass_fade_start, "1024", PXR_CV_A) \
	X(r_grass_fade_dist, "2048", PXR_CV_A) \
	X(r_occlusion_culling, "0", PXR_CV_A) \
	/* studio models */ \
	X(cl_righthand, "0", PXR_CV_A) \
	X(cl_viewmodel_fov, "90", PXR_CV_A) \
	X(cl_viewmodel_znear, "4.0", PXR_CV_A) \
	X(r_studio_compatible, "1", PXR_CV_A) \
	X(cl_lod_scale, "5.0", PXR_CV_A) \
	X(cl_lod_bias, "0", PXR_CV_A) \
	/* particles */ \
	X(cl_particles, "1", PXR_CV_A) \
	X(cl_particle_lod, "0", PXR_CV_A)

// one constant per row, pxr_cv_<name>, and the list of them all
#define PXR_CV_ROW(n, v, f) static const pxr_saved_cvar_t pxr_cv_##n = { #n, v, f };
PXR_SAVED_CVARS(PXR_CV_ROW)
#undef PXR_CV_ROW

#define PXR_CV_ADDRESS(n, v, f) &pxr_cv_##n,
static const pxr_saved_cvar_t* const pxr_saved_cvars[] = { PXR_SAVED_CVARS(PXR_CV_ADDRESS) };
#undef PXR_CV_ADDRESS

#define PXR_SAVED_CVAR_COUNT ((int)(sizeof(pxr_saved_cvars) / sizeof(pxr_saved_cvars[0])))

// pxrender.dll: register a row by its name (a bare identifier); `reg` is the SDK's CVAR_REGISTER
#define PXR_CVAR_REGISTER(reg, n) reg(pxr_cv_##n.name, pxr_cv_##n.value, pxr_cv_##n.flags)
