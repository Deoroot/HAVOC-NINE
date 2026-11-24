// pxr_shells.cpp - H9's glow shells for PrimeXT's renderer (H9: written new).
//
// PrimeXT has no kRenderFxGlowShell. ref_gl draws it as a second pass of the model, every vertex pushed out along its
// normal by max(1, renderamt) / 128 units, chrome-textured and added in the rendercolor; H9's shellwhite patch uses a
// white texture, so the shell shows the rendercolor. H9's xray patch (renderfx 64, ts_matchmode's sudden-death marker)
// draws the same shell at renderamt 16 with the depth test off, so it shows through walls.
//
// Here the shell is built on the CPU from the bones PrimeXT computed for the frame (its model instance) and drawn
// through the engine's TriAPI, additive, white texture, after the translucent pass. It is meant for the few models that
// carry a shell (siege objectives, the sudden-death players), not for crowds.

#include "hud.h"
#include "utils.h"
#include "gl_local.h"
#include "gl_studio.h"
#include "triangleapi.h"
#include "pxr_module.h"

#define H9_RENDERFX_XRAY 64
#define H9_XRAY_SHELL_AMT 16

static void PXR_ShellVertex(const Vector& v, const Vector& n, float scale)
{
	const Vector p = v + n * scale;
	gEngfuncs.pTriAPI->Vertex3f(p.x, p.y, p.z);
}

void CStudioModelRenderer::DrawH9Shells(void)
{
	bool begun = false;

	for (int e = 0; e < tr.num_draw_entities; e++)
	{
		cl_entity_t* ent = tr.draw_entities[e];
		if (!ent || !ent->model || ent->model->type != mod_studio)
			continue;

		const int fx = ent->curstate.renderfx;
		if (fx != kRenderFxGlowShell && fx != H9_RENDERFX_XRAY)
			continue;
		if (ent == GET_VIEWMODEL() || ent->modelhandle == INVALID_HANDLE)
			continue;

		ModelInstance_t* inst = &m_ModelInstances[ent->modelhandle];
		if (!inst->m_pModel || inst->m_pEntity != ent)
			continue;

		studiohdr_t* hdr = (studiohdr_t*)IEngineStudio.Mod_Extradata(inst->m_pModel);
		if (!hdr || hdr->numbodyparts == 0)
			continue;

		const bool xray = (fx == H9_RENDERFX_XRAY);
		const float amt = xray ? (float)H9_XRAY_SHELL_AMT : (float)ent->curstate.renderamt;
		const float scale = Q_max(1.0f, amt) / 128.0f;
		const bool weighted = FBitSet(hdr->flags, STUDIO_HAS_BONEWEIGHTS) != 0;

		if (!begun)
		{
			GL_BindShader(NULL);
			GL_Bind(GL_TEXTURE0, tr.whiteTexture);
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			begun = true;
		}

		if (xray)
			pglDisable(GL_DEPTH_TEST);
		pglDepthMask(GL_FALSE);
		gEngfuncs.pTriAPI->Color4ub(ent->curstate.rendercolor.r, ent->curstate.rendercolor.g, ent->curstate.rendercolor.b, 255);

		for (int b = 0; b < hdr->numbodyparts; b++)
		{
			mstudiobodyparts_t* part = (mstudiobodyparts_t*)((byte*)hdr + hdr->bodypartindex) + b;
			int index = ent->curstate.body / Q_max(1, part->base);
			index = index % Q_max(1, part->nummodels);
			mstudiomodel_t* sub = (mstudiomodel_t*)((byte*)hdr + part->modelindex) + index;

			Vector* verts = (Vector*)((byte*)hdr + sub->vertindex);
			Vector* norms = (Vector*)((byte*)hdr + sub->normindex);
			byte* vertbone = (byte*)hdr + sub->vertinfoindex;
			byte* normbone = (byte*)hdr + sub->norminfoindex;
			mstudioboneweight_t* vertweight = (mstudioboneweight_t*)((byte*)hdr + sub->blendvertinfoindex);
			mstudioboneweight_t* normweight = (mstudioboneweight_t*)((byte*)hdr + sub->blendnorminfoindex);

			for (int m = 0; m < sub->nummesh; m++)
			{
				mstudiomesh_t* mesh = (mstudiomesh_t*)((byte*)hdr + sub->meshindex) + m;
				short* cmds = (short*)((byte*)hdr + mesh->triindex);
				int count;

				while ((count = *(cmds++)) != 0)
				{
					gEngfuncs.pTriAPI->Begin(count < 0 ? TRI_TRIANGLE_FAN : TRI_TRIANGLE_STRIP);
					if (count < 0)
						count = -count;
					for (; count > 0; count--, cmds += 4)
					{
						const int vi = cmds[0], ni = cmds[1];
						Vector v, n;
						if (weighted)
						{
							matrix3x4 skin;
							ComputeSkinMatrix(&vertweight[vi], inst->m_pbones, skin);
							v = skin.VectorTransform(verts[vi]);
							ComputeSkinMatrix(&normweight[ni], inst->m_pbones, skin);
							n = skin.VectorRotate(norms[ni]);
						}
						else
						{
							v = inst->m_pbones[vertbone[vi]].VectorTransform(verts[vi]);
							n = inst->m_pbones[normbone[ni]].VectorRotate(norms[ni]);
						}
						PXR_ShellVertex(v, n.Normalize(), scale);
					}
					gEngfuncs.pTriAPI->End();
				}
			}
		}

		if (xray)
			pglEnable(GL_DEPTH_TEST);
	}

	if (begun)
	{
		pglDepthMask(GL_TRUE);
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		gEngfuncs.pTriAPI->CullFace(TRI_FRONT);
		GL_CleanupDrawState();
	}
}
