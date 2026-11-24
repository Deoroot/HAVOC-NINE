// H9: written new for the H9 PrimeXT render module (pxrender.dll); replaces PrimeXT's client/hud.h.
// The renderer only needs the headers hud.h pulled in, the screen size and the view model index;
// the HUD itself stays in reTS's client.dll.
#ifndef HUD_H
#define HUD_H

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "wrect.h"
#include "port.h"
#include "mathlib.h"
#include "cdll_int.h"
#include "cdll_dll.h"
#include "render_api.h"
#include "enginecallback.h"
#include "randomrange.h"
#include "screenfade.h"
#include "r_view.h"
#include "color24.h"
#include "cl_entity.h"

class CHud
{
public:
	float m_flTime;          // the current client time
	SCREENINFO m_scrinfo;    // screen size, refreshed by R_VidInit through PXR_VidInit
	int m_iViewModelIndex;   // not sent by reTS; the renderer takes the view model from the engine
	float m_flBlurAmount;
};

extern CHud gHUD;

#endif // HUD_H
