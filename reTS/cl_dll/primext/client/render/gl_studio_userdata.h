// gl_studio_userdata.h - what the studio renderer keeps per model beside the model file: the GPU meshes of
// each submodel, the static lighting of each face, and the bone pose of the bind position.
// H9: written new for reTS from the interface (the members, types and constructors the renderer uses),
// replacing PrimeXT's file, which had no licence notice. The members keep their names, types and order:
// the renderer reads them by name and clears mstudiosurface_t with memset.
#pragma once
#include "color24.h"
#include "lightlimits.h"
#include "mathlib.h"
#include "bounding_box.h"
#include <vector>
#include <unordered_map>

// One mesh of a submodel as uploaded to the GPU (its destructor, in gl_studio_init.cpp, frees the buffers).
struct vbomesh_t
{
	vbomesh_t()
		: skinref(0), numVerts(0), numElems(0), lightmapnum(0),
		  vbo(0), vao(0), ibo(0), parentbone(0), uniqueID(0), cacheSize(0)
	{
	}
	~vbomesh_t();

	unsigned int skinref;       // index into the model's skin table
	unsigned int numVerts;      // vertices in the buffer
	unsigned int numElems;      // indices in the buffer
	int lightmapnum;            // the one lightmap atlas page this mesh samples

	unsigned int vbo, vao, ibo; // GL vertex buffer, vertex array and index buffer
	std::unordered_map<int32_t, CBoundingBox> boneBounds; // bounds of the mesh's vertices, per bone
	int parentbone;             // bone whose transform places the bounds
	unsigned short uniqueID;    // tells decals which mesh they belong to
	unsigned int cacheSize;     // bytes uploaded for this mesh (statistics only)
};

// Static lighting of one studio face, laid out like a world surface's (plain data, cleared to zero).
struct mstudiosurface_t
{
	mstudiosurface_t()
	{
		memset(this, 0, sizeof(*this)); // valid only while every member is plain data
	}

	int flags;                              // same bits as msurface_t::flags
	int texture_step;                       // world units per lightmap texel

	short lightextents[2];
	unsigned short light_s[MAXLIGHTMAPS];   // position of the face's lightmap in its atlas page, per style
	unsigned short light_t[MAXLIGHTMAPS];
	byte lights[MAXDYNLIGHTS];              // static lights that reach the face, 255 where none

	int lightmaptexturenum;                 // atlas page
	byte styles[MAXLIGHTMAPS];              // light styles

	color24 *samples;                       // lightmap texels
	color24 *normals;                       // deluxemap texels (light directions)
	byte *shadows;                          // occlusion texels
};

// The meshes of one submodel.
struct msubmodel_t
{
	std::vector<vbomesh_t> meshes;
};

// One body part: the submodels to choose from.
struct mbodypart_t
{
	mbodypart_t() : base(0) {}

	int base;                               // the body part's base, as in mstudiobodyparts_t
	std::vector<msubmodel_t *> models;      // points into mstudiocache_t::submodels
};

// Everything cached for one model.
struct mstudiocache_t
{
	mstudiocache_t() : update_light(false) {}

	std::vector<mstudiosurface_t> surfaces;
	std::vector<mbodypart_t> bodyparts;
	std::vector<msubmodel_t> submodels;
	bool update_light;                      // set when gamma or brightness changed: lightmaps are rebuilt
};

// The bind pose: per bone, the transform from model space to bone space.
struct mposetobone_t
{
	matrix3x4 posetobone[MAXSTUDIOBONES];
};
