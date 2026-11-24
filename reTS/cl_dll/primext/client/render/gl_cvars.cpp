/*
r_cvars.cpp - renderer console variables list
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "gl_cvars.h"
#include "enginecallback.h"
#include "../../../primext_cvars.h" // H9: the saved settings, shared with client.dll

cvar_t *r_test;	// just cvar for testify new effects
cvar_t *r_stencilbits;
cvar_t *r_drawentities;
cvar_t *gl_extensions;
cvar_t *gl_debug_verbose;
cvar_t *cv_dynamiclight;
cvar_t *r_detailtextures;
cvar_t *r_lighting_ambient;
cvar_t *r_lightstyle_lerping;
cvar_t *r_lighting_extended;
cvar_t *r_occlusion_culling;
cvar_t *r_show_lightprobes;
cvar_t *r_show_cubemaps;
cvar_t *r_show_viewleaf;
cvar_t *r_show_luminance;
cvar_t *cv_crosshair;
cvar_t *r_shadows;
cvar_t *r_fullbright;
cvar_t *r_draw_beams;
cvar_t *r_overview;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lockpvs;
cvar_t *r_lockfrustum;
cvar_t *gl_hdr;
cvar_t *r_tonemap;
cvar_t *r_bloom;
cvar_t *r_bloom_scale;
cvar_t *r_dof;
cvar_t *r_dof_hold_time;
cvar_t *r_dof_change_time;
cvar_t *r_dof_focal_length;
cvar_t *r_dof_fstop;
cvar_t *r_dof_debug;
cvar_t *r_pvs_radius;
cvar_t *r_allow_mirrors;
cvar_t *r_allow_portals;
cvar_t *r_allow_screens;
cvar_t *r_allow_3dsky;
cvar_t *cv_renderer;
cvar_t *cv_brdf;
cvar_t *cv_bump;
cvar_t *cv_bumpvecs;
cvar_t *cv_specular;
cvar_t *cv_parallax;
cvar_t *cv_decals;
cvar_t *cv_realtime_puddles;
cvar_t *cv_shadow_offset;
cvar_t *r_cubemap;
cvar_t *r_cubemap_realtime;
cvar_t *cv_cube_lod_bias;
cvar_t *gl_alpha2coverage;
cvar_t *cv_gamma;
cvar_t *cv_brightness;
cvar_t *cv_water;
cvar_t *cv_decalsdebug;
cvar_t *cv_show_tbn;
cvar_t *cv_nosort;
cvar_t *r_lightmap;
cvar_t *r_speeds;
cvar_t *r_decals;
cvar_t *r_studio_decals;
cvar_t *r_hand;
cvar_t *r_sunshadows;
cvar_t *r_sun_allowed;
cvar_t *r_sun_daytime;
cvar_t *r_shadow_split_weight;
cvar_t *r_wireframe;
cvar_t *r_lightstyles;
cvar_t *r_polyoffset;
cvar_t *r_dynamic;
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_grass;
cvar_t *r_grass_alpha;
cvar_t *r_grass_lighting;
cvar_t *r_grass_shadows;
cvar_t *r_grass_fade_start;
cvar_t *r_grass_fade_dist;
cvar_t *r_scissor_glass_debug;
cvar_t *r_scissor_light_debug;
cvar_t *r_showlightmaps;
cvar_t *r_recursion_depth;
cvar_t *r_shadowmap_size;
cvar_t *r_pssm_show_split;
cvar_t *r_renderplayershadow;
cvar_t *v_sunshafts;
cvar_t *v_glows;
cvar_t *v_posteffects;
cvar_t *r_postfx_enable;

void R_InitializeConVars()
{
	// H9: every saved setting first, from the table client.dll also registers when it runs without this module
	for( int i = 0; i < PXR_SAVED_CVAR_COUNT; i++ )
		CVAR_REGISTER( pxr_saved_cvars[i]->name, pxr_saved_cvars[i]->value, pxr_saved_cvars[i]->flags );

	// setup some engine cvars for custom rendering
	r_shadows = PXR_CVAR_REGISTER(CVAR_REGISTER, px_shadows);
	r_shadow_split_weight = PXR_CVAR_REGISTER(CVAR_REGISTER, r_pssm_split_weight);
	r_pssm_show_split = CVAR_REGISTER("r_pssm_show_split", "0", 0); // debug feature
	r_scissor_glass_debug = CVAR_REGISTER("r_scissor_glass_debug", "0", 0); // H9: a debug view, not saved
	r_scissor_light_debug = CVAR_REGISTER("r_scissor_light_debug", "0", 0); // H9: a debug view, not saved
	r_recursion_depth = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_recursion_depth);
	r_showlightmaps = CVAR_REGISTER("r_showlightmaps", "0", 0);

	// setup some engine cvars for custom rendering
	r_test = CVAR_GET_POINTER("gl_test");
	cv_nosort = CVAR_GET_POINTER("gl_nosort");
	r_overview = CVAR_GET_POINTER("dev_overview");
	r_fullbright = CVAR_GET_POINTER("r_fullbright");
	r_drawentities = CVAR_GET_POINTER("r_drawentities");
	gl_extensions = CVAR_GET_POINTER("gl_allow_extensions");
	r_finish = CVAR_GET_POINTER("gl_finish");
	cv_crosshair = CVAR_GET_POINTER("crosshair");
	r_lighting_ambient = CVAR_GET_POINTER("r_lighting_ambient");
	r_lightstyle_lerping = CVAR_GET_POINTER("cl_lightstyle_lerping");
	r_lighting_extended = CVAR_GET_POINTER("r_lighting_extended");
	r_draw_beams = CVAR_GET_POINTER("cl_draw_beams");
	r_detailtextures = CVAR_GET_POINTER("r_detailtextures");
	r_speeds = CVAR_GET_POINTER("r_speeds");
	r_novis = CVAR_GET_POINTER("r_novis");
	r_nocull = CVAR_GET_POINTER("r_nocull");
	r_lockpvs = CVAR_GET_POINTER("r_lockpvs");
	r_lockfrustum = CVAR_GET_POINTER("r_lockfrustum");
	r_pvs_radius = CVAR_GET_POINTER("r_pvs_radius");
	r_wireframe = CVAR_GET_POINTER("gl_wireframe");
	r_lightmap = CVAR_GET_POINTER("r_lightmap");
	r_decals = CVAR_GET_POINTER("r_decals");
	r_clear = CVAR_GET_POINTER("gl_clear");
	r_dynamic = CVAR_GET_POINTER("r_dynamic");
	cv_gamma = CVAR_GET_POINTER("gamma");
	cv_brightness = CVAR_GET_POINTER("brightness");
	r_polyoffset = CVAR_GET_POINTER("gl_polyoffset");

	v_glows = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_glows);
	gl_hdr = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_hdr);
	r_dof = PXR_CVAR_REGISTER(CVAR_REGISTER, r_dof);
	r_dof_hold_time = PXR_CVAR_REGISTER(CVAR_REGISTER, r_dof_hold_time);
	r_dof_change_time = PXR_CVAR_REGISTER(CVAR_REGISTER, r_dof_change_time);
	r_dof_focal_length = PXR_CVAR_REGISTER(CVAR_REGISTER, r_dof_focal_length);
	r_dof_fstop = PXR_CVAR_REGISTER(CVAR_REGISTER, r_dof_fstop);
	r_dof_debug = CVAR_REGISTER("r_dof_debug", "0", 0); // H9: a debug view, not saved

	cv_renderer = CVAR_REGISTER("gl_renderer", "1", FCVAR_CLIENTDLL); // H9: not saved: GL_Init sets 0 when it fails, and a saved 0 kept PrimeXT off for good
	gl_debug_verbose = CVAR_REGISTER("gl_debug_verbose", "1", FCVAR_CLIENTDLL); // H9: debug, not saved
	cv_bump = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_bump);
	cv_bumpvecs = CVAR_REGISTER("bump_vecs", "0", 0);
	cv_specular = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_specular);
	cv_dynamiclight = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_dynlight);
	cv_parallax = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_parallax);
	cv_shadow_offset = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_shadow_offset);
	cv_cube_lod_bias = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_cube_lod_bias);
	gl_alpha2coverage = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_alpha2coverage);
	cv_water = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_waterblur);
	cv_decalsdebug = CVAR_REGISTER("gl_decals_debug", "0", 0); // H9: a debug view, not saved
	cv_realtime_puddles = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_realtime_puddles);
	r_cubemap = PXR_CVAR_REGISTER(CVAR_REGISTER, r_cubemap_reflection);
	r_cubemap_realtime = PXR_CVAR_REGISTER(CVAR_REGISTER, r_cubemap_realtime);
	r_sunshadows = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_sun_shadows);
	r_sun_allowed = PXR_CVAR_REGISTER(CVAR_REGISTER, r_sun_allowed);
	r_sun_daytime = CVAR_REGISTER("r_sun_daytime", "12", FCVAR_CLIENTDLL);
	r_renderplayershadow = PXR_CVAR_REGISTER(CVAR_REGISTER, r_renderplayershadow);
	r_shadowmap_size = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_shadowmap_size);
	r_occlusion_culling = PXR_CVAR_REGISTER(CVAR_REGISTER, r_occlusion_culling);
	r_show_lightprobes = CVAR_REGISTER("r_show_lightprobes", "0", 0); // H9: a debug view, not saved
	r_show_cubemaps = CVAR_REGISTER("r_show_cubemaps", "0", 0); // H9: a debug view, not saved
	r_show_viewleaf = CVAR_REGISTER("r_show_viewleaf", "0", 0); // H9: a debug view, not saved
	r_show_luminance = CVAR_REGISTER("r_show_luminance", "0", 0); // H9: a debug view, not saved
	cv_decals = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_decals);
	r_lightstyles = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_lightstyles);
	r_allow_mirrors = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_allow_mirrors);
	r_allow_portals = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_allow_portals);
	r_allow_screens = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_allow_screens);
	r_allow_3dsky = PXR_CVAR_REGISTER(CVAR_REGISTER, gl_allow_3dsky);
	r_studio_decals = PXR_CVAR_REGISTER(CVAR_REGISTER, r_studio_decals);
	cv_show_tbn = CVAR_REGISTER("gl_show_basis", "0", 0); // H9: a debug view, not saved
	cv_brdf = PXR_CVAR_REGISTER(CVAR_REGISTER, r_lighting_brdf);

	r_grass = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass);
	r_grass_alpha = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass_alpha);
	r_grass_lighting = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass_lighting);
	r_grass_shadows = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass_shadows);
	r_grass_fade_start = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass_fade_start);
	r_grass_fade_dist = PXR_CVAR_REGISTER(CVAR_REGISTER, r_grass_fade_dist);
}

