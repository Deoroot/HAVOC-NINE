/*
scene_bmodel_fp.glsl - fragment uber shader for brush surfaces with static lighting
Copyright (C) 2015 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "const.h"
#include "mathlib.h"
#include "texfetch.h"
#include "lightmodel.h"
#include "deluxemap.h"	// directional lightmap routine
#include "terrain.h"
#include "cubemap.h"
#include "screen.h"
#include "fresnel.h"
#include "parallax.h"
#include "material.h"

// H9: see MaterialFetchTexture below
#define BRUSH_METALNESS_SCALE	0.5
#include "fog.h"
#include "alpha2coverage.h"

// H9: units of water (on the view axis) that hide half of what is behind it behind the water's own colour
#define WATER_BODY_HALF_DEPTH	40.0
// H9: the look of water (LIQUID_SURFACE): ripple size in units and tilt, how much less the screen behind is
// bent than the mirror, the reflectance straight down (Schlick F0), the deep colour against the texture's
// average and how much of the warped pattern shows, and the tint of what is seen through shallow water
#define WATER_RIPPLE_SIZE	160.0
#define WATER_RIPPLE_TILT	0.55
#define WATER_REFRACT_BEND	0.35
#define WATER_SURFACE_F0	0.04
#define WATER_DEEP_BRIGHTNESS	0.55
#define WATER_PATTERN	0.15
#define WATER_CLEAR_TINT	0.3
#define WATER_MIRROR_BEND	0.04
// H9: dark water's own light: a glint on the ripples from above (a lamp or the sky), whatever the mirror shows
#define WATER_GLINT		0.25

// texture units
#if defined( APPLY_TERRAIN )
uniform sampler2DArray	u_ColorMap;
uniform sampler2DArray	u_NormalMap;
uniform sampler2DArray	u_GlossMap;
#else
uniform sampler2D		u_ColorMap;
uniform sampler2D		u_NormalMap;
uniform sampler2D		u_GlossMap;
#endif

uniform sampler2D		u_GlowMap;
uniform sampler2D		u_DetailMap;
uniform sampler2D		u_DepthMap;

#if defined( LIQUID_SURFACE ) && defined( PLANAR_REFLECTION )
uniform sampler2D		u_MirrorMap;	// H9: the water's planar reflection
#endif

// uniforms
uniform vec3		u_ViewOrigin;
uniform float		u_ReflectScale;
uniform vec4		u_RenderColor;
uniform vec4		u_FogParams;
uniform float		u_RealTime;
uniform float		u_zFar;

#if defined( APPLY_TERRAIN )
uniform float		u_Smoothness[TERRAIN_NUM_LAYERS];
#else
uniform float		u_Smoothness;
#endif

// shared variables
centroid varying vec2		var_TexDiffuse;
centroid varying vec3		var_TexLight0;
centroid varying vec3		var_TexLight1;
centroid varying vec3		var_TexLight2;
centroid varying vec3		var_TexLight3;
centroid varying vec2		var_TexDetail;
centroid varying vec2		var_TexGlobal;
centroid varying vec3		var_ViewDir;
centroid varying vec3		var_Position;

#if defined( PLANAR_REFLECTION ) || defined( PORTAL_SURFACE )
varying vec4		var_TexMirror;	// mirror coords
#endif

#if defined( REFLECTION_CUBEMAP )
centroid varying mat3		var_MatrixTBN;
#endif

#if !defined( HAS_NORMALMAP )
varying vec3	var_Normal;
#endif

#if defined( PARALLAX_SIMPLE ) || defined( PARALLAX_OCCLUSION )
centroid varying vec3	var_TangentViewDir;
#endif

void main( void )
{
	vec4 albedo = vec4( 0.0 );
	vec4 result = vec4( 0.0 );
	vec2 vec_TexDiffuse = var_TexDiffuse;
	MaterialData mat;
	LightingData lighting;

#if defined( APPLY_TERRAIN )
	vec4 mask0, mask1, mask2, mask3;
	TerrainReadMask( var_TexGlobal, mask0, mask1, mask2, mask3 );
#endif

#if defined( PARALLAX_SIMPLE )
	//vec_TexDiffuse = ParallaxMapSimple(var_TexDiffuse.xy, normalize(var_TangentViewDir));
	vec_TexDiffuse = ParallaxOffsetMap(u_NormalMap, var_TexDiffuse.xy, normalize(var_TangentViewDir));
	//vec_TexDiffuse = ParallaxReliefMap(var_TexDiffuse.xy, normalize(var_TangentViewDir));
#elif defined( PARALLAX_OCCLUSION )
	vec_TexDiffuse = ParallaxOcclusionMap(var_TexDiffuse.xy, normalize(var_TangentViewDir)).xy; 
#endif
	
// compute the normal first
#if defined( HAS_NORMALMAP )
#if defined( APPLY_TERRAIN )
	vec3 N = TerrainMixNormal( u_NormalMap, vec_TexDiffuse, mask0, mask1, mask2, mask3 );
#else
	vec3 N = normalmap2D( u_NormalMap, vec_TexDiffuse );
#endif // APPLY_TERRAIN
#else
	vec3 N = normalize( var_Normal );
#endif // HAS_NORMALMAP

#if defined( LIQUID_SURFACE ) && defined( HAS_NORMALMAP ) && !defined( APPLY_TERRAIN )
	// H9: ripples by the world, not by the texture: the same size on every liquid whatever its texture
	// scale. Two layers of the animated normal map, a broad one and a fine one turned and offset, so no
	// tile repeats to the eye; then flattened to WATER_RIPPLE_TILT (a calm surface keeps a sharp mirror)
	vec3 rippleA = normalmap2D( u_NormalMap, var_Position.xy * ( 1.0 / WATER_RIPPLE_SIZE ));
	vec3 rippleB = normalmap2D( u_NormalMap, mat2( 0.8, -0.6, 0.6, 0.8 ) * var_Position.xy * ( 2.7 / WATER_RIPPLE_SIZE ) + vec2( 0.37, 0.61 ));
	N = normalize( vec3(( rippleA.xy + rippleB.xy * 0.7 ) * WATER_RIPPLE_TILT, 1.0 ));
#endif

// setup material params values
#if defined( HAS_GLOSSMAP )
#if defined( APPLY_TERRAIN )
	vec4 glossmap = TerrainMixGlossmap( u_GlossMap, vec_TexDiffuse, mask0, mask1, mask2, mask3 );
#else
	vec4 glossmap = colormap2D( u_GlossMap, vec_TexDiffuse );
#endif
	// get params from texture
	mat = MaterialFetchTexture( glossmap );
#if defined( APPLY_PBS )
	// H9: a brush has no image-based light (only models sample the probes' specular), so a metal brush got
	// almost no diffuse and nothing else: a steel floor under a spot stood black. Half its metalness keeps
	// PBR's tint and highlight and lets the light show
	mat.metalness *= BRUSH_METALNESS_SCALE;
#endif
#else // !HAS_GLOSSMAP
#if defined( APPLY_TERRAIN )
	mat.smoothness = TerrainMixSmoothness( u_Smoothness, mask0, mask1, mask2, mask3 );
#else
	mat.smoothness = u_Smoothness;
#endif
	// default params 
	mat.metalness = 0.0;
	mat.ambientOcclusion = 1.0;
	mat.specularIntensity = 1.0;
#endif // HAS_GLOSSMAP

#if defined( LIQUID_SURFACE )
	float waterBorderFactor = 1.0, waterRefractFactor = 1.0;
	float fSampledDepth = texture( u_DepthMap, gl_FragCoord.xy * u_ScreenSizeInv ).r;
	fSampledDepth = linearizeDepth( u_zFar, fSampledDepth );
	fSampledDepth = RemapVal( fSampledDepth, Z_NEAR, u_zFar, 0.0, 1.0 );

	float fOwnDepth = gl_FragCoord.z;
	fOwnDepth = linearizeDepth( u_zFar, fOwnDepth );
	fOwnDepth = RemapVal( fOwnDepth, Z_NEAR, u_zFar, 0.0, 1.0 );

#if defined (LIQUID_UNDERWATER)
	waterBorderFactor = waterRefractFactor = u_RenderColor.a;
#else
	// NOTE: u_zFar is minus the far plane here (UT_ZFAR), so these remapped depths fall with distance
	float depthDelta = fOwnDepth - fSampledDepth;
	// H9: the thickness of water in front of the scene behind it, in units on the view axis (from the true
	// linear depths: the far plane is -u_zFar)
	float fWaterThickness = linearizeDepth( -u_zFar, texture( u_DepthMap, gl_FragCoord.xy * u_ScreenSizeInv ).r ) - linearizeDepth( -u_zFar, gl_FragCoord.z );
	waterBorderFactor = 1.0 - saturate(exp2( -768.0 * 100.0 * depthDelta ));
	waterRefractFactor = 1.0 - saturate(exp2( -768.0 * 5.0 * depthDelta ));
#endif // LIQUID_UNDERWATER
#endif // LIQUID_SURFACE

// compute the result term
#if ( defined( PLANAR_REFLECTION ) && !defined( LIQUID_SURFACE )) || defined( PORTAL_SURFACE )
	albedo = reflectmap2D( u_ColorMap, var_TexMirror, N, gl_FragCoord.xyz, u_RefractScale );
#elif defined( APPLY_TERRAIN )
	albedo = TerrainMixDiffuse( u_ColorMap, vec_TexDiffuse, mask0, mask1, mask2, mask3 );
#elif defined( MONITOR_BRUSH )
	// in case of rendering monitor we don't need SRGB->linear conversion
	albedo = texture( u_ColorMap, vec_TexDiffuse );
#else
	albedo = colormap2D( u_ColorMap, vec_TexDiffuse );
#endif

#if defined( LIQUID_WARP ) && !defined( APPLY_TERRAIN ) && ( !defined( PLANAR_REFLECTION ) || defined( LIQUID_SURFACE )) && !defined( PORTAL_SURFACE ) && !defined( MONITOR_BRUSH )
	// H9: the engine's warp of a liquid texture (ref_gl, EmitWaterPolys): in texels, each axis moved by
	// 8 * sin( other axis / 8 + time ), then one texture every 64 texels
	vec2 turbTexels = var_TexDiffuse * vec2( textureSize( u_ColorMap, 0 ));
	vec2 turbCoord = vec2( turbTexels.x + 8.0 * sin( turbTexels.y * 0.125 + u_RealTime ), turbTexels.y + 8.0 * sin( turbTexels.x * 0.125 + u_RealTime ));
	albedo = colormap2D( u_ColorMap, turbCoord * ( 1.0 / 64.0 ));
#endif

#if defined( MONOCHROME )
	albedo.rgb = vec3(GetLuminance(albedo.rgb));
#endif

#if !defined( ALPHA_BLENDING ) && !defined( USING_SCREENCOPY ) && !defined( APPLY_TERRAIN )
	albedo.a = AlphaRescaling( u_ColorMap, vec_TexDiffuse, albedo.a );
#endif
#if defined( ALPHA_TO_COVERAGE )
	// TODO should be cutoff value hardcoded?
	albedo.a = AlphaSharpening( albedo.a, 0.95 );
#endif
	result = albedo;

#if defined( HAS_DETAIL )
#if defined( APPLY_TERRAIN )
	result.rgb *= detailmap2D( u_DetailMap, var_TexGlobal ).rgb * DETAIL_SCALE;
#else
	result.rgb *= detailmap2D( u_DetailMap, var_TexDetail ).rgb * DETAIL_SCALE;
#endif
#endif

#if !defined( LIQUID_SURFACE )
	result.rgb *= u_RenderColor.rgb;
#endif

#if defined( ALPHA_BLENDING ) || defined( USING_SCREENCOPY )
	result.a *= u_RenderColor.a;
#endif

	vec3 V = normalize( var_ViewDir );

	// lighting the world polys
#if !defined( LIGHTING_FULLBRIGHT )
	lighting.diffuse = vec3(0.0);
	lighting.specular = vec3(0.0);
	lighting.kD = vec3(1.0); // H9: the diffuse share left by the specular one (PBS); it was never set here and read undefined (black)
#if defined( APPLY_STYLE0 )
	ApplyLightStyle( var_TexLight0, N, V, albedo, mat, lighting );
#endif
#if defined( APPLY_STYLE1 )
	ApplyLightStyle( var_TexLight1, N, V, albedo, mat, lighting );
#endif
#if defined( APPLY_STYLE2 )
	ApplyLightStyle( var_TexLight2, N, V, albedo, mat, lighting );
#endif
#if defined( APPLY_STYLE3 )
	ApplyLightStyle( var_TexLight3, N, V, albedo, mat, lighting );
#endif
#if defined( APPLY_STYLE0 ) || defined( APPLY_STYLE1 ) || defined( APPLY_STYLE2 ) || defined( APPLY_STYLE3 )
	// convert values back to normal range and clamp it
	lighting.diffuse = min(( lighting.diffuse * LIGHTMAP_SHIFT ), 1.0 );
	lighting.specular = min(( lighting.specular * LIGHTMAP_SHIFT ), 1.0 );
#endif

#if defined( LIGHTVEC_DEBUG )
	lighting.diffuse = normalize(( lighting.diffuse + 1.0 ) * 0.5 ); // convert range to [0..1]
#else
	// apply diffuse & specular lighting
#if defined( APPLY_PBS )
	// H9: no 1/pi: the lightmaps and the light values are the engine's, scaled for albedo * light (as without PBS); with it the world was a third as bright
	result.rgb = (lighting.kD * result.rgb) * lighting.diffuse;
#else
	result.rgb *= lighting.diffuse;
#endif
	result.rgb += lighting.specular;
#endif

#endif // !LIGHTING_FULLBRIGHT

#if defined( HAS_LUMA )
	result.rgb += texture( u_GlowMap, vec_TexDiffuse ).rgb;
#endif

#if defined( REFLECTION_CUBEMAP )
	mat3 tbnBasis = mat3(normalize(var_MatrixTBN[0]), normalize(var_MatrixTBN[1]), normalize(var_MatrixTBN[2]));
	vec3 worldNormal = normalize(tbnBasis * N);
#if defined( LIQUID_SURFACE ) && !defined( LIQUID_UNDERWATER )
	// H9: a ray a ripple sends below the horizon would meet the next wave, which shows the sky again: it is
	// mirrored up, not looked up in the probe's floor (black patches across a sea)
	vec3 waterRay = reflect( normalize( var_Position - u_ViewOrigin ), worldNormal );
	waterRay.z = abs( waterRay.z );
	vec3 reflected = CubemapReflectionProbeRay( var_Position, waterRay, mat.smoothness );
#else
	vec3 reflected = CubemapReflectionProbe( var_Position, u_ViewOrigin, worldNormal, mat.smoothness );
#endif
#endif // REFLECTION_CUBEMAP

#if defined( LIQUID_SURFACE ) && defined( PLANAR_REFLECTION )
	// H9: the water's own mirror pass: the world as it is, the sky and the city, bent by the ripples (an offset
	// in screen fractions; PrimeXT's reflectmap2D scaled it by the pixel coordinate)
	vec4 mirrorCoord = var_TexMirror;
	// (less at grazing angles: there the band of water is thin on screen, and a bent sample fell below the
	// mirror's horizon, where its picture is empty: black dashes along the far sea)
	mirrorCoord.xy += N.xy * WATER_MIRROR_BEND * saturate( abs( V.z ) * 4.0 ) * mirrorCoord.w;
#if defined( REFLECTION_CUBEMAP )
	reflected = textureProj( u_MirrorMap, mirrorCoord ).rgb;
#else
	vec3 reflected = textureProj( u_MirrorMap, mirrorCoord ).rgb;
#endif
#endif

#if defined( USING_SCREENCOPY )
#if defined( LIQUID_SURFACE )
	float distortScale = waterRefractFactor;
#else 
	float distortScale = 1.0;
#endif
	// prohibits displaying in refractions objects, that are closer to camera than brush surface
#if defined( LIQUID_SURFACE )
	// H9: the screen behind is bent less than the mirror: PrimeXT offsets it by N.xy times the screen coordinate
	vec3 refractN = vec3( N.xy * WATER_REFRACT_BEND, N.z );
#else
	vec3 refractN = N;
#endif
	float distortedDepth = texture(u_DepthMap, GetDistortedTexCoords(refractN, distortScale)).r;
	distortScale *= step(gl_FragCoord.z, distortedDepth);

	// fetch color for saved screencopy
	vec3 screenmap = GetScreenColor( refractN, distortScale );
#if defined( PLANAR_REFLECTION ) || defined( PORTAL_SURFACE )
	result.a = GetFresnel( saturate(dot(V, N)), WATER_F0_VALUE, FRESNEL_FACTOR );
#endif // PLANAR_REFLECTION

#if defined( LIQUID_SURFACE )
	vec3 waterColor = u_RenderColor.rgb;
	vec3 refracted = mix( screenmap, screenmap * waterColor, waterBorderFactor ); // smooth transition between water and ground
#if !defined( LIQUID_UNDERWATER )
	// H9: the water's body. Light that crosses more water is more the water's own colour: its texture, warped
	// as the engine draws it (LIQUID_WARP). The thickness is how far behind the surface the scene is, on the
	// view axis; a shallow channel shows its floor, a deep pool its texture. Where nothing is drawn behind the
	// water (a depth of 1.0: no floor in the PVS, the sky volume under a sea) the thickness is the far plane,
	// so the body is opaque there: the screen behind is never cleared (r_clear 0) and held a white sheet or
	// an old frame. The refracted sample is checked too: a ripple must not fetch from the void beside a floor
	// The body's colour is the texture's, darkened: its average (the last mip) with a faint trace of the
	// engine's warped pattern. The texture tints the water; the mirror and the ripples make it read as water
	float fThickness = max( fWaterThickness, 0.0 );
	float fBody = 1.0 - exp2( -fThickness / WATER_BODY_HALF_DEPTH );
	fBody = max( fBody, step( 0.99999, distortedDepth ) * step( 0.0001, distortScale ));
	vec3 waterTint = ConvertSRGBToLinear( textureLod( u_ColorMap, var_TexDiffuse, 16.0 ).rgb );
	vec3 waterDeep = mix( waterTint, albedo.rgb, WATER_PATTERN ) * waterColor * WATER_DEEP_BRIGHTNESS;
	refracted = mix( refracted * mix( vec3( 1.0 ), waterTint * 1.5, WATER_CLEAR_TINT ), waterDeep, fBody );
	waterBorderFactor = max( waterBorderFactor, fBody );
#endif
#if defined( REFLECTION_CUBEMAP ) || defined( PLANAR_REFLECTION )
	// blend refracted and reflected part together
#if !defined( LIQUID_UNDERWATER )
	// H9: as light does: what is reflected is not also seen through (Schlick, water's F0)
	float fresnel = GetFresnel( V, N, WATER_SURFACE_F0, FRESNEL_FACTOR ) * waterBorderFactor;
	result.rgb = mix( refracted, reflected * u_ReflectScale, fresnel );
	// H9: a glint of its own: water under a dark ceiling mirrors only the dark, and the ripples catch a light
	// from above anyway (Blinn on the ripple normal; tangent space z is up on a liquid's top)
	vec3 glintH = normalize( normalize( vec3( 0.35, 0.2, 1.0 )) + V );
	result.rgb += pow( saturate( dot( N, glintH )), 96.0 ) * WATER_GLINT * ( waterTint * 2.0 + 0.25 ) * waterBorderFactor;
#else
	float fresnel = GetFresnel( V, N, WATER_F0_VALUE, FRESNEL_FACTOR );
	result.rgb = refracted + reflected * fresnel * waterBorderFactor * u_ReflectScale;
#endif
#else
	result.rgb = refracted;
#endif // REFLECTION_CUBEMAP

#else // !LIQUID_SURFACE
	// for translucent non-liquid stuff (glass, etc.)
	result.rgb = mix( screenmap, result.rgb, result.a );
#endif // LIQUID_SURFACE
#else // !USING_SCREENCOPY
#if defined( REFLECTION_CUBEMAP )
	float fresnel = GetFresnel( V, N, GENERIC_F0_VALUE, FRESNEL_FACTOR );
	result.rgb += reflected * fresnel * u_ReflectScale * mat.smoothness;
#endif
#endif // USING_SCREENCOPY

#if defined( APPLY_FOG_EXP )
	result.rgb = CalculateFog(result.rgb, u_FogParams, gl_FragCoord.z / gl_FragCoord.w);
#endif

#if defined( LIGHTMAP_DEBUG ) || defined( LIGHTVEC_DEBUG )
	result.rgb = lighting.diffuse;
#endif

	// compute final color
	gl_FragColor = result;
}
