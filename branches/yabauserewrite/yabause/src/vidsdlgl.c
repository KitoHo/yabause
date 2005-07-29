/*  Copyright 2005 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "vdp1.h"
#include "vidsdlgl.h"

int VIDSDLGLInit(void);
void VIDSDLGLDeInit(void);
int VIDSDLGLVdp1Reset(void);
void VIDSDLGLVdp1DrawStart(void);
void VIDSDLGLVdp1DrawEnd(void);
void VIDSDLGLVdp1NormalSpriteDraw(void);
void VIDSDLGLVdp1ScaledSpriteDraw(void);
void VIDSDLGLVdp1DistortedSpriteDraw(void);
void VIDSDLGLVdp1PolygonDraw(void);
void VIDSDLGLVdp1PolylineDraw(void);
void VIDSDLGLVdp1LineDraw(void);
void VIDSDLGLVdp1UserClipping(void);
void VIDSDLGLVdp1SystemClipping(void);
void VIDSDLGLVdp1LocalCoordinate(void);
int VIDSDLGLVdp2Reset(void);
void VIDSDLGLVdp2DrawEnd(void);
void VIDSDLGLVdp2DrawBackScreen(void);
void VIDSDLGLVdp2DrawLineColorScreen(void);
void VIDSDLGLVdp2DrawNBG0(void);
void VIDSDLGLVdp2DrawNBG1(void);
void VIDSDLGLVdp2DrawNBG2(void);
void VIDSDLGLVdp2DrawNBG3(void);
void VIDSDLGLVdp2DrawRBG0(void);

VideoInterface_struct VIDSDLGL = {
VIDCORE_SDLGL,
"SDL/OpenGL Video Interface",
VIDSDLGLInit,
VIDSDLGLDeInit,
VIDSDLGLVdp1Reset,
VIDSDLGLVdp1DrawStart,
VIDSDLGLVdp1DrawEnd,
VIDSDLGLVdp1NormalSpriteDraw,
VIDSDLGLVdp1ScaledSpriteDraw,
VIDSDLGLVdp1DistortedSpriteDraw,
VIDSDLGLVdp1PolygonDraw,
VIDSDLGLVdp1PolylineDraw,
VIDSDLGLVdp1LineDraw,
VIDSDLGLVdp1UserClipping,
VIDSDLGLVdp1SystemClipping,
VIDSDLGLVdp1LocalCoordinate,
VIDSDLGLVdp2Reset,
VIDSDLGLVdp2DrawEnd,
VIDSDLGLVdp2DrawBackScreen,
VIDSDLGLVdp2DrawLineColorScreen,
VIDSDLGLVdp2DrawNBG0,
VIDSDLGLVdp2DrawNBG1,
VIDSDLGLVdp2DrawNBG2,
VIDSDLGLVdp2DrawNBG3,
VIDSDLGLVdp2DrawRBG0
};

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLInit(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLDeInit(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DrawStart(void)
{
//   YglReset();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1NormalSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1ScaledSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DistortedSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1PolygonDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1PolylineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1LineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1UserClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1SystemClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1LocalCoordinate(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawBackScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawLineColorScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawNBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawNBG1(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawNBG2(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawNBG3(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawRBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////
