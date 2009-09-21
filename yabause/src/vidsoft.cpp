/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2008 Theo Berkau
    Copyright 2006 Fabien Coulon

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "vidsoft.h"
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"

#ifdef HAVE_LIBGL
#define USE_OPENGL
#endif

#ifdef USE_OPENGL
#include "ygl.h"
#endif

#include "yui.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
}
#if defined WORDS_BIGENDIAN
INLINE u32 COLSAT2YAB16(int priority,u32 temp)            { return (priority | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27); }
INLINE u32 COLSAT2YAB32(int priority,u32 temp)            { return (((temp & 0xFF) << 24) | ((temp & 0xFF00) << 8) | ((temp & 0xFF0000) >> 8) | priority); }
INLINE u32 COLSAT2YAB32_2(int priority,u32 temp1,u32 temp2)   { return (((temp2 & 0xFF) << 24) | ((temp2 & 0xFF00) << 8) | ((temp1 & 0xFF) << 8) | priority); }
INLINE u32 COLSATSTRIPPRIORITY(u32 pixel)              { return (pixel | 0xFF); }
#else
INLINE u32 COLSAT2YAB16(int priority,u32 temp) { return (priority << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9); }
INLINE u32 COLSAT2YAB32(int priority, u32 temp) { return (priority << 24 | (temp & 0xFF0000) | (temp & 0xFF00) | (temp & 0xFF)); }
INLINE u32 COLSAT2YAB32_2(int priority,u32 temp1,u32 temp2)   { return (priority << 24 | ((temp1 & 0xFF) << 16) | (temp2 & 0xFF00) | (temp2 & 0xFF)); }
INLINE u32 COLSATSTRIPPRIORITY(u32 pixel) { return (0xFF000000 | pixel); }
#endif

#define COLOR_ADDt(b)		(b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)	COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)      (COLOR_ADDb(l & 0xFF, r) << 24) | \
                                (COLOR_ADDb((l >> 8) & 0xFF, g) << 16) | \
                                (COLOR_ADDb((l >> 16) & 0xFF, b) << 8) | \
                                ((l >> 24) & 0xFF)
#else
#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
                                (COLOR_ADDb((l >> 8) & 0xFF, g) << 8) | \
                                (COLOR_ADDb((l >> 16) & 0xFF, b) << 16) | \
				(l & 0xFF000000)
#endif

static void PushUserClipping(int mode);
static void PopUserClipping(void);

int VIDSoftInit(void);
void VIDSoftDeInit(void);
void VIDSoftResize(unsigned int, unsigned int, int);
int VIDSoftIsFullscreen(void);
int VIDSoftVdp1Reset(void);
void VIDSoftVdp1DrawStart(void);
void VIDSoftVdp1DrawEnd(void);
void VIDSoftVdp1NormalSpriteDraw(void);
void VIDSoftVdp1ScaledSpriteDraw(void);
void VIDSoftVdp1DistortedSpriteDraw(void);
void VIDSoftVdp1PolygonDraw(void);
void VIDSoftVdp1PolylineDraw(void);
void VIDSoftVdp1LineDraw(void);
void VIDSoftVdp1UserClipping(void);
void VIDSoftVdp1SystemClipping(void);
void VIDSoftVdp1LocalCoordinate(void);
int VIDSoftVdp2Reset(void);
void VIDSoftVdp2DrawStart(void);
void VIDSoftVdp2DrawEnd(void);
void VIDSoftVdp2DrawScreens(void);
void VIDSoftVdp2SetResolution(u16 TVMD);
void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority);
void VIDSoftOnScreenDebugMessage(char *string, ...);
void VIDSoftGetGlSize(int *width, int *height);
void VIDSoftVdp1SwapFrameBuffer(void);
void VIDSoftVdp1EraseFrameBuffer(void);

VideoInterface_struct VIDSoft = {
VIDCORE_SOFT,
"Software Video Interface",
VIDSoftInit,
VIDSoftDeInit,
VIDSoftResize,
VIDSoftIsFullscreen,
VIDSoftVdp1Reset,
VIDSoftVdp1DrawStart,
VIDSoftVdp1DrawEnd,
VIDSoftVdp1NormalSpriteDraw,
VIDSoftVdp1ScaledSpriteDraw,
VIDSoftVdp1DistortedSpriteDraw,
//for the actual hardware, polygons are essentially identical to distorted sprites
//the actual hardware draws using diagonal lines, which is why using half-transparent processing
//on distorted sprites and polygons is not recommended since the hardware overdraws to prevent gaps
//thus, with half-transparent processing some pixels will be processed more than once, producing moire patterns in the drawn shapes
VIDSoftVdp1DistortedSpriteDraw,
VIDSoftVdp1PolylineDraw,
VIDSoftVdp1LineDraw,
VIDSoftVdp1UserClipping,
VIDSoftVdp1SystemClipping,
VIDSoftVdp1LocalCoordinate,
VIDSoftVdp2Reset,
VIDSoftVdp2DrawStart,
VIDSoftVdp2DrawEnd,
VIDSoftVdp2DrawScreens,
VIDSoftVdp2SetResolution,
VIDSoftVdp2SetPriorityNBG0,
VIDSoftVdp2SetPriorityNBG1,
VIDSoftVdp2SetPriorityNBG2,
VIDSoftVdp2SetPriorityNBG3,
VIDSoftVdp2SetPriorityRBG0,
VIDSoftOnScreenDebugMessage,
VIDSoftGetGlSize,
};

u32 *dispbuffer=NULL;
u8 *vdp1framebuffer[2]= { NULL, NULL };
u8 *vdp1frontframebuffer;
u8 *vdp1backframebuffer;

u32 *vdp2framebuffer=NULL;

u32 *nbg0framebuffer=NULL;
u32 *nbg1framebuffer=NULL;
u32 *nbg2framebuffer=NULL;
u32 *nbg3framebuffer=NULL;
u32 *rbg0framebuffer=NULL;

static int vdp1width;
static int vdp1height;
static int vdp1clipxstart;
static int vdp1clipxend;
static int vdp1clipystart;
static int vdp1clipyend;
static int vdp1pixelsize = 1;
static int vdp1spritetype;
int vdp2width;
int vdp2height;
static int nbg0priority=0;
static int nbg1priority=0;
static int nbg2priority=0;
static int nbg3priority=0;
static int rbg0priority=0;
#ifdef USE_OPENGL
static int outputwidth;
static int outputheight;
#endif
static int resxratio;
static int resyratio;

static char message[512];
static int msglength;

typedef struct { s16 x; s16 y; } vdp1vertex;

typedef struct
{
   int pagepixelwh, pagepixelwh_bits, pagepixelwh_mask;
   int planepixelwidth, planepixelwidth_bits, planepixelwidth_mask;
   int planepixelheight, planepixelheight_bits, planepixelheight_mask;
   int screenwidth;
   int screenheight;
   int oldcellx, oldcelly, oldcellcheck;
   int xmask, ymask;
   u32 planetbl[16];
} screeninfo_struct;

//////////////////////////////////////////////////////////////////////////////

static INLINE void vdp2putpixel32(s32 x, s32 y, u32 color, int priority)
{
   vdp2framebuffer[(y * vdp2width) + x] = COLSAT2YAB32(priority, color);
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u8 Vdp2GetPixelPriority(u32 pixel)
{
#if defined WORDS_BIGENDIAN
   return pixel;
#else
   return pixel >> 24;
#endif
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void puthline16(s32 x, s32 y, s32 width, u16 color, int priority)
{
   u32 *buffer = vdp2framebuffer + (y * vdp2width) + x;
   u32 dot=COLSAT2YAB16(priority, color);
   int i;

   for (i = 0; i < width; i++)
      buffer[i] = dot;
}

static INLINE u32 FASTCALL Vdp2ColorRamGetColor(u32 addr);
static INLINE void Vdp2PatternAddr(vdp2draw_struct *info)
{
   switch(info->patterndatasize)
   {
      case 1:
      {
         u16 tmp = T1ReadWord(Vdp2Ram, info->addr);         

         info->addr += 2;
         info->specialfunction = (info->supplementdata >> 9) & 0x1;

         switch(info->colornumber)
         {
            case 0: // in 16 colors
               info->paladdr = ((tmp & 0xF000) >> 8) | ((info->supplementdata & 0xE0) << 3);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 4;
               break;
         }

         switch(info->auxmode)
         {
            case 0:
               info->flipfunction = (tmp & 0xC00) >> 10;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
                     break;
               }
               break;
            case 1:
               info->flipfunction = 0;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
                     break;
               }
               break;
         }

         break;
      }
      case 2: {
         u16 tmp1 = T1ReadWord(Vdp2Ram, info->addr);
         u16 tmp2 = T1ReadWord(Vdp2Ram, info->addr+2);
         info->addr += 4;
         info->charaddr = tmp2 & 0x7FFF;
         info->flipfunction = (tmp1 & 0xC000) >> 14;
         info->paladdr = (tmp1 & 0x7F) << 4;
         info->specialfunction = (tmp1 & 0x2000) >> 13;
         break;
      }
   }

   if (!(Vdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // selon Runik
   if (info->specialprimode == 1) {
      info->priority = (info->priority & 0xE) | (info->specialfunction & 1);
   }
}

static INLINE int Vdp2FetchPixel(vdp2draw_struct *info, int x, int y, u32 *color);
//////////////////////////////////////////////////////////////////////////////

static INLINE int TestWindow(int wctl, int enablemask, int inoutmask, clipping_struct *clip, int x, int y)
{
   if (wctl & enablemask) 
   {
      if (wctl & inoutmask)
      {
         // Draw inside of window
         if (x < clip->xstart || x > clip->xend ||
             y < clip->ystart || y > clip->yend)
            return 0;
      }
      else
      {
         // Draw outside of window
         if (x >= clip->xstart && x <= clip->xend &&
             y >= clip->ystart && y <= clip->yend)
            return 0;

		 //it seems to overflow vertically on hardware
		 if(clip->yend > vdp2height && (x >= clip->xstart && x <= clip->xend ))
			 return 0;
      }
   }
   return 1;
}

//////////////////////////////////////////////////////////////////////////////

INLINE void GeneratePlaneAddrTable(vdp2draw_struct *info, u32 *planetbl)
{
   int i;

   for (i = 0; i < (info->mapwh*info->mapwh); i++)
   {
      info->PlaneAddr(info, i);
      planetbl[i] = info->addr;
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void FASTCALL Vdp2MapCalcXY(vdp2draw_struct *info, int *x, int *y,
                                 screeninfo_struct *sinfo)
{
   int planenum;
   const int pagesize_bits=info->pagewh_bits*2;
   const int cellwh=(2 + info->patternwh);

   const int check = ((y[0] >> cellwh) << 16) | (x[0] >> cellwh);
   //if ((x[0] >> cellwh) != sinfo->oldcellx || (y[0] >> cellwh) != sinfo->oldcelly)
   if(check != sinfo->oldcellcheck)
   {
      sinfo->oldcellx = x[0] >> cellwh;
      sinfo->oldcelly = y[0] >> cellwh;
	  sinfo->oldcellcheck = (sinfo->oldcelly << 16) | sinfo->oldcellx;

      // Calculate which plane we're dealing with
      planenum = ((y[0] >> sinfo->planepixelheight_bits) * info->mapwh) + (x[0] >> sinfo->planepixelwidth_bits);
      x[0] = (x[0] & sinfo->planepixelwidth_mask);
      y[0] = (y[0] & sinfo->planepixelheight_mask);

      // Fetch and decode pattern name data
      info->addr = sinfo->planetbl[planenum];

      // Figure out which page it's on(if plane size is not 1x1)
      info->addr += ((  ((y[0] >> sinfo->pagepixelwh_bits) << pagesize_bits) << info->planew_bits) +
                     (   (x[0] >> sinfo->pagepixelwh_bits) << pagesize_bits) +
                     (((y[0] & sinfo->pagepixelwh_mask) >> cellwh) << info->pagewh_bits) +
                     ((x[0] & sinfo->pagepixelwh_mask) >> cellwh)) << (info->patterndatasize_bits+1);

      Vdp2PatternAddr(info); // Heh, this could be optimized
   }

   // Figure out which pixel in the tile we want
   if (info->patternwh == 1)
   {
      x[0] &= 8-1;
      y[0] &= 8-1;

	  switch(info->flipfunction & 0x3)
	  {
	  case 0: //none
		  break;
	  case 1: //horizontal flip
		  x[0] = 8 - 1 - x[0];
		  break;
	  case 2: // vertical flip
         y[0] = 8 - 1 - y[0];
		 break;
	  case 3: //flip both
         x[0] = 8 - 1 - x[0];
		 y[0] = 8 - 1 - y[0];
		 break;
	  }
   }
   else
   {
      if (info->flipfunction)
      {
         y[0] &= 16 - 1;
         if (info->flipfunction & 0x2)
         {
            if (!(y[0] & 8))
               y[0] = 8 - 1 - y[0] + 16;
            else
               y[0] = 16 - 1 - y[0];
         }
         else if (y[0] & 8)
            y[0] += 8;

         if (info->flipfunction & 0x1)
         {
            if (!(x[0] & 8))
               y[0] += 8;

            x[0] &= 8-1;
            x[0] = 8 - 1 - x[0];
         }
         else if (x[0] & 8)
         {
            y[0] += 8;
            x[0] &= 8-1;
         }
         else
            x[0] &= 8-1;
      }
      else
      {
         y[0] &= 16 - 1;

         if (y[0] & 8)
            y[0] += 8;
         if (x[0] & 8)
            y[0] += 8;
         x[0] &= 8-1;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void SetupScreenVars(vdp2draw_struct *info, screeninfo_struct *sinfo)
{
   if (!info->isbitmap)
   {
      sinfo->pagepixelwh=64*8;
	  sinfo->pagepixelwh_bits = 9;
	  sinfo->pagepixelwh_mask = 511;

      sinfo->planepixelwidth=info->planew*sinfo->pagepixelwh;
	  sinfo->planepixelwidth_bits = 8+info->planew;
	  sinfo->planepixelwidth_mask = (1<<(sinfo->planepixelwidth_bits))-1;

      sinfo->planepixelheight=info->planeh*sinfo->pagepixelwh;
	  sinfo->planepixelheight_bits = 8+info->planeh;
	  sinfo->planepixelheight_mask = (1<<(sinfo->planepixelheight_bits))-1;

      sinfo->screenwidth=info->mapwh*sinfo->planepixelwidth;
      sinfo->screenheight=info->mapwh*sinfo->planepixelheight;
      sinfo->oldcellx=-1;
      sinfo->oldcelly=-1;
      sinfo->xmask = sinfo->screenwidth-1;
      sinfo->ymask = sinfo->screenheight-1;
      GeneratePlaneAddrTable(info, sinfo->planetbl);
   }
   else
   {
      sinfo->pagepixelwh = 0;
	  sinfo->pagepixelwh_bits = 0;
	  sinfo->pagepixelwh_mask = 0;
      sinfo->planepixelwidth=0;
	  sinfo->planepixelwidth_bits=0;
	  sinfo->planepixelwidth_mask=0;
      sinfo->planepixelheight=0;
	  sinfo->planepixelheight_bits=0;
	  sinfo->planepixelheight_mask=0;
      sinfo->screenwidth=0;
      sinfo->screenheight=0;
      sinfo->oldcellx=0;
      sinfo->oldcelly=0;
      sinfo->xmask = info->cellw-1;
      sinfo->ymask = info->cellh-1;
   }
}

//////////////////////////////////////////////////////////////////////////////

enum LayerNames{
	_NULL,NBG0,NBG1,NBG2,NBG3,SPRITE,RBG0
};

struct pixeldata {
	int pixel;
	LayerNames layer;
	int priority;
	bool transparent;
	bool colorCalcWindow;
	int colorCount;
	bool isSpriteColorCalc;
	int spriteColorCalcRatio;
	bool isNormalShadow;
	bool isMSBShadow;
};

pixeldata initPixelData() {
	pixeldata p;
	p.pixel = 0;
	p.layer = _NULL;
	p.priority = 0;
	p.transparent = true;
	p.colorCalcWindow = false;
	p.colorCount = 0;
	p.isSpriteColorCalc = false;
	p.spriteColorCalcRatio = 0;
	p.isNormalShadow = false;
	p.isMSBShadow = false;
	return p;
}

pixeldata rbg0buf[704*512];
pixeldata nbg0buf[704*512];
pixeldata nbg1buf[704*512];
pixeldata nbg2buf[704*512];
pixeldata nbg3buf[704*512];
pixeldata backbuf[704*512];
pixeldata spritebuf[704*512];

extern "C" void FASTCALL Vdp2DrawScroll(vdp2draw_struct *info, LayerNames layer, int width, int height)
{
   int i, j;
   int x, y;
   screeninfo_struct sinfo;
   int scrollx, scrolly;
   int *mosaic_y, *mosaic_x;

   SetupScreenVars(info, &sinfo);

   scrollx = info->x;
   scrolly = info->y;

   {
	   static int tables_initialized = 0;
	   static int mosaic_table[16][1024];
	   if(!tables_initialized)
	   {
		   tables_initialized = 1;
			for(i=0;i<16;i++)
			{
				int m = i+1;
				for(j=0;j<1024;j++)
					mosaic_table[i][j] = j/m*m;
			}
		}
		mosaic_x = (int*)&mosaic_table[info->mosaicxmask-1];
		mosaic_y = (int*)&mosaic_table[info->mosaicymask-1];
	}

	for (j = 0; j < height; j++)
	{
		int Y;
		int linescrollx = 0;
		// precalculate the coordinate for the line(it's faster) and do line
		// scroll
		if (info->islinescroll)
		{
			if (info->islinescroll & 0x1)
			{
				linescrollx = (T1ReadLong(Vdp2Ram, info->linescrolltbl) >> 16) & 0x7FF;
				info->linescrolltbl += 4;
			}
			if (info->islinescroll & 0x2)
			{
				info->y = (T1ReadWord(Vdp2Ram, info->linescrolltbl) & 0x7FF) + scrolly;
				info->linescrolltbl += 4;
				y = info->y;
			}
			else
				//y = info->y+((int)(info->coordincy *(float)(info->mosaicymask > 1 ? (j / info->mosaicymask * info->mosaicymask) : j)));
				y = info->y + info->coordincy*mosaic_y[j];
			if (info->islinescroll & 0x4)
			{
				info->coordincx = (T1ReadLong(Vdp2Ram, info->linescrolltbl) & 0x7FF00) / (float)65536.0;
				info->linescrolltbl += 4;
         }
      }
      else
         //y = info->y+((int)(info->coordincy *(float)(info->mosaicymask > 1 ? (j / info->mosaicymask * info->mosaicymask) : j)));
		 y = info->y + info->coordincy*mosaic_y[j];

      y &= sinfo.ymask;

      if (info->isverticalscroll)
      {
         // this is *wrong*, vertical scroll use a different value per cell
         // info->verticalscrolltbl should be incremented by info->verticalscrollinc
         // each time there's a cell change and reseted at the end of the line...
			// or something like that :)
			y += T1ReadLong(Vdp2Ram, info->verticalscrolltbl) >> 16;
			y &= 0x1FF;
      }

      Y=y;

      for (i = 0; i < width; i++)
      {
         u32 color;

         //x = info->x+((int)(info->coordincx*(float)((info->mosaicxmask > 1) ? (i / info->mosaicxmask * info->mosaicxmask) : i)));
		 x = info->x + mosaic_x[i/resxratio]*info->coordincx;
         x &= sinfo.xmask;
		 
         if (linescrollx) {
            x += linescrollx;
				x &= 0x3FF;
			}

			// Fetch Pixel, if it isn't transparent, continue
			if (!info->isbitmap)
			{
				// Tile
            y=Y;
            Vdp2MapCalcXY(info, &x, &y, &sinfo);
         }

			pixeldata p = initPixelData();

         if (!Vdp2FetchPixel(info, x, y, &color))
         {
			 p.transparent = true;
         }
		 else
			 p.transparent = false;
		 
		 p.pixel = color;
		 p.layer = layer;
		 
		 if(layer == NBG0)
			 nbg0buf[(j * vdp2width)+i] = p;
		 if(layer == NBG1)
			 nbg1buf[(j * vdp2width)+i] = p;
		 if(layer == NBG2)
			 nbg2buf[(j * vdp2width)+i] = p;
		 if(layer == NBG3)
			 nbg3buf[(j * vdp2width)+i] = p;
      }
   }    
}
static void Vdp2DrawNBG0(void);
static void Vdp2DrawNBG1(void);
static void Vdp2DrawNBG2(void);
static void Vdp2DrawNBG3(void);
static void Vdp2DrawRBG0(void);

u32 ColorCalc(u32 toppixel, u32 bottompixel, u32 alpha);

//reduce luminance of specified vdp2 pixel
u32 Shadow(u32 vdp2, u32 intensity)
{
	s16 r, g, b;

	r = (vdp2 & 0xFF);
	g = ((vdp2 >> 8) & 0xFF);
	b = ((vdp2 >> 16) & 0xFF);

	r = r - intensity;
	g = g - intensity;
	b = b - intensity;

	if(b < 0) b = 0;
	if(g < 0) g = 0;
	if(r < 0) r = 0;

	vdp2 = (b << 16) | (g << 8) | r;

	return vdp2;
}
bool isSpriteColorCalc(u8 prioritytable[], int priorityRegisterNumber, int msb) {

	//must be enabled
	if(!(Vdp2Regs->CCCTL & (1 << 6)))
		return false;

	//the color calculation enable condition number
	int colorCalculationConditionNumber = (Vdp2Regs->SPCTL & 0x700) >> 8;
	//(Vdp2Regs->SPCTL & 0x700) >> 8)
	//check if color calc is enabled for this sprite pixel
	switch ((Vdp2Regs->SPCTL & 0x3000) >> 12) {
		case 0:
			if(prioritytable[priorityRegisterNumber] <= colorCalculationConditionNumber)
				return true;
			break;
		case 1:
			if(prioritytable[priorityRegisterNumber] == colorCalculationConditionNumber)
				return true;
			break;
		case 2:
			if(prioritytable[priorityRegisterNumber] >= colorCalculationConditionNumber)
				return true;
			break;
		case 3:
			if(msb)
				return true;
			break;
	}
	return false;
}

bool isSpriteNormalShadow(int pixel) {

	//see if the color data meets the conditions for normal shadow
	//if each color data bit is set except the lsb then it's a shadow
	switch(vdp1spritetype) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 5:
	if(pixel == 0x7fe)
		return true;
	break;

	case 4:
	case 6:
	if(pixel == 0x3fe)
		return true;
	break;

	case 7:
	if(pixel == 0x1fe)
		return true;
	break;

	//6 bit color data
	case 8:
	case 0xC:
	if(pixel == 0x7e)
		return true;
	break;

	// 5 bit color data
	case 9:
	case 0xA:
	case 0xB:
	case 0xD:
	case 0xE:
	case 0xF:
	if(pixel == 0x3e)
		return true;
	break;
	}

	return false;
}


void Vdp2DrawSprites(){
	int i, i2;
	u16 pixel;
	u8 prioritytable[8];
	u32 vdp1coloroffset;
	int colormode = Vdp2Regs->SPCTL & 0x20;

	if (!Vdp1Regs->disptoggle)
		return;

	int colorCalculationRatio[8];

	//get all the sprite color calc ratios
	colorCalculationRatio[0] = Vdp2Regs->CCRSA & 0x1f;
	colorCalculationRatio[1] = Vdp2Regs->CCRSA >> 8;
	colorCalculationRatio[2] = Vdp2Regs->CCRSB & 0x1f;
	colorCalculationRatio[3] = Vdp2Regs->CCRSB >> 8;
	colorCalculationRatio[4] = Vdp2Regs->CCRSC & 0x1f;
	colorCalculationRatio[5] = Vdp2Regs->CCRSC >> 8;
	colorCalculationRatio[6] = Vdp2Regs->CCRSD & 0x1f;
	colorCalculationRatio[7] = Vdp2Regs->CCRSD >> 8;

	prioritytable[0] = Vdp2Regs->PRISA & 0x7;
	prioritytable[1] = (Vdp2Regs->PRISA >> 8) & 0x7;
	prioritytable[2] = Vdp2Regs->PRISB & 0x7;
	prioritytable[3] = (Vdp2Regs->PRISB >> 8) & 0x7;
	prioritytable[4] = Vdp2Regs->PRISC & 0x7;
	prioritytable[5] = (Vdp2Regs->PRISC >> 8) & 0x7;
	prioritytable[6] = Vdp2Regs->PRISD & 0x7;
	prioritytable[7] = (Vdp2Regs->PRISD >> 8) & 0x7;

	vdp1coloroffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
	vdp1spritetype = Vdp2Regs->SPCTL & 0xF;

	for (i2 = 0; i2 < vdp2height; i2++)
	{
		for (i = 0; i < vdp2width; i++)
		{
			pixeldata p = initPixelData();
			p.layer = SPRITE;
			if (vdp1pixelsize == 2)
			{
				// 16-bit pixel size
				//16 bit pixels will be drawn at 2x width, and sometimes 2x height
				pixel = ((u16 *)vdp1frontframebuffer)[(i2/resyratio * vdp1width) + i/2];

				if (pixel == 0)
					p.transparent = true;
				else if (pixel & 0x8000 && colormode)
				{
					// 16 BPP
					// if pixel is 0x8000, only draw pixel if sprite window
					// is disabled/sprite type 2-7. sprite types 0 and 1 are
					// -always- drawn and sprite types 8-F are always
					// transparent.
					if (pixel != 0x8000 || vdp1spritetype < 2 || (vdp1spritetype < 8 && !(Vdp2Regs->SPCTL & 0x10))) {
						p.pixel = COLSAT2YAB16(0xFF, pixel);//info.PostPixelFetchCalc(&info, COLSAT2YAB16(0xFF, pixel));
						p.priority = prioritytable[0];//currentpriority;
						p.transparent = false;
					}
					else
						p.transparent = true;
				}
				else
				{
					//Color bank mode
					int priorityRegisterNumber;
					int shadow;
					int colorcalc;
					int msb = pixel & 0x8000;
					int pixelcopy = pixel;
					Vdp1ProcessSpritePixel(vdp1spritetype, &pixel, &shadow, &priorityRegisterNumber, &colorcalc);
					p.transparent = false;
					p.isMSBShadow = (shadow && (Vdp2Regs->SDCTL & 0x100));
					//i go to color ram with this i'll get garbage and break msb shadow on backgrounds
					//apparently if the pixel is 0x8000 it doesn't go to color ram and instead just uses it as is
					if(pixelcopy == 0x8000 && p.isMSBShadow) {
						p.pixel = 0x8000;
						p.layer = SPRITE;
						p.priority = prioritytable[priorityRegisterNumber];
					}
					else {
						
					p.pixel = Vdp2ColorRamGetColor(vdp1coloroffset + pixel);
					//strange crash in taromaru "fix", TODO figure out the actual issue
					if(priorityRegisterNumber>7)priorityRegisterNumber=7;
					p.priority = prioritytable[priorityRegisterNumber];
					p.isSpriteColorCalc = isSpriteColorCalc(prioritytable, priorityRegisterNumber, msb);
					p.spriteColorCalcRatio = colorCalculationRatio[colorcalc];
					p.isNormalShadow = isSpriteNormalShadow(pixel);
					}
					
				}
			}
			else
			{
				// 8-bit pixel size
				pixel = vdp1frontframebuffer[(i2*2 * vdp1width) + (i*2)];

				p.transparent = false;

				if (pixel == 0)
					p.transparent = true;

				p.pixel = COLSAT2YAB32(0xFF, Vdp2ColorRamGetColor(vdp1coloroffset + pixel));
				p.priority = prioritytable[0];
				
			}
			spritebuf[(i2 * vdp2width)+i] = p;
		}
	}
}
	
void readLayerInfo(pixeldata first, int &wctl, int &colorCalculationEnable, int &alpha, int &specialColorCalculationMode, int &colorOffsetMask, int i, int j);

bool doTest(int wctl, clipping_struct clip[], int i, int j) {
	//if both windows are enabled and AND is enabled
	if((wctl & 0x2) && (wctl & 0x8) && (wctl & 0x80) == 0x80) {
		// test AND window logic
		if(!TestWindow(wctl, 0x2, 0x1, &clip[0], i, j) && !TestWindow(wctl, 0x8, 0x4, &clip[1], i, j)) {
			return true;
		}
	}
	else
	{
		//or window logic
		if (!TestWindow(wctl, 0x2, 0x1, &clip[0], i, j) || !TestWindow(wctl, 0x8, 0x4, &clip[1], i,j)) {
			return true;
		}
	}
	return false;
}

bool testWindow(pixeldata &p, int i, int j) {

	int wctl,colorCalculationEnable,alpha,specialColorCalculationMode,colorOffsetMask;
	int islinewindow = false;

	p.colorCalcWindow = false;

	u32 linewnd0addr, linewnd1addr;
	clipping_struct clip[2];
	clip[0].xstart = clip[0].ystart = clip[0].xend = clip[0].yend = 0;
	clip[1].xstart = clip[1].ystart = clip[1].xend = clip[1].yend = 0;

	linewnd0addr = linewnd1addr = 0;

	readLayerInfo(p, wctl, colorCalculationEnable, alpha, specialColorCalculationMode, colorOffsetMask,i,j);

	//this just reads the coordinates for the two regular rectangular windows
	ReadWindowData(wctl, clip);

	ReadLineWindowData(&islinewindow, wctl, &linewnd0addr, &linewnd1addr);
	// if line window is enabled, adjust clipping values
	ReadLineWindowClip(islinewindow, clip, linewnd0addr+(j*4), linewnd1addr+(j*4));

	if(doTest(wctl, clip, i, j))
		return true;

	//now that we have checked for regular window and made it here
	//if either window is being used for color calc window
	//no idea if any games use color calc line or sprite window
	if(Vdp2Regs->WCTLD & (1 << 9) || Vdp2Regs->WCTLD & (1 << 10)) {

		wctl = Vdp2Regs->WCTLD >> 8;

		ReadWindowData(wctl, clip);

		if(doTest(wctl, clip, i, j)) {
			p.colorCalcWindow = true;
			return false;
		}
	}

	return false;
}

struct ColorOffsetData {
	int red;
	int green;
	int blue;
};

ColorOffsetData colorOffset;

void readColorOffset(int offset){

	if (Vdp2Regs->CLOFSL & offset)
	{
		// color offset B
		colorOffset.red = Vdp2Regs->COBR & 0xFF;
		if (Vdp2Regs->COBR & 0x100)
			colorOffset.red |= 0xFFFFFF00;

		colorOffset.green = Vdp2Regs->COBG & 0xFF;
		if (Vdp2Regs->COBG & 0x100)
			colorOffset.green |= 0xFFFFFF00;

		colorOffset.blue = Vdp2Regs->COBB & 0xFF;
		if (Vdp2Regs->COBB & 0x100)
			colorOffset.blue |= 0xFFFFFF00;
	}
	else
	{
		// color offset A
		colorOffset.red = Vdp2Regs->COAR & 0xFF;
		if (Vdp2Regs->COAR & 0x100)
			colorOffset.red |= 0xFFFFFF00;

		colorOffset.green = Vdp2Regs->COAG & 0xFF;
		if (Vdp2Regs->COAG & 0x100)
			colorOffset.green |= 0xFFFFFF00;

		colorOffset.blue = Vdp2Regs->COAB & 0xFF;
		if (Vdp2Regs->COAB & 0x100)
			colorOffset.blue |= 0xFFFFFF00;
	}
}

void readLayerInfo(pixeldata first, int &wctl, int &colorCalculationEnable, int &alpha, int &specialColorCalculationMode, int &colorOffsetMask, int i, int j) {

	colorCalculationEnable = false;

	if(first.layer == NBG0) {
		wctl = Vdp2Regs->WCTLA;
		colorCalculationEnable = Vdp2Regs->CCCTL & 0x1;
		alpha = Vdp2Regs->CCRNA & 0x1f;
		specialColorCalculationMode = Vdp2Regs->SFCCMD & 0x3;
		colorOffsetMask = 1;
	}
	if(first.layer == NBG1) {
		wctl = Vdp2Regs->WCTLA >> 8;
		colorCalculationEnable = Vdp2Regs->CCCTL & 0x2;
		alpha = Vdp2Regs->CCRNA >> 8;
		specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 2) & 0x3;
		colorOffsetMask = 2;
	}
	if(first.layer == NBG2) {
		wctl = Vdp2Regs->WCTLB;
		colorCalculationEnable = Vdp2Regs->CCCTL & 0x4;
		alpha = Vdp2Regs->CCRNB & 0x1f;
		specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 4) & 0x3;
		colorOffsetMask = 4;
	}
	if(first.layer == NBG3) {
		wctl = Vdp2Regs->WCTLB >> 8;
		colorCalculationEnable = Vdp2Regs->CCCTL & 0x8;
		alpha = Vdp2Regs->CCRNB >> 8;
		specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 6) & 0x3;
		colorOffsetMask = 8;
	}

	if(first.layer == RBG0) {
		wctl = Vdp2Regs->WCTLC & 0x1f;
		colorCalculationEnable = Vdp2Regs->CCCTL & 0x10;
		alpha = Vdp2Regs->CCRR & 0x1f;
		colorOffsetMask = 0x10;
	}

	if(first.layer == SPRITE) {
		wctl = Vdp2Regs->WCTLC >> 8;
		colorCalculationEnable = first.isSpriteColorCalc;
		alpha = first.spriteColorCalcRatio;
		colorOffsetMask = 0x40;
	}

	readColorOffset(colorOffsetMask);
}

pixeldata pixelstack[0x7];

void getColorCount(pixeldata &p) {
	//need to look up format of the pixel
	if(p.layer == NBG0)
		p.colorCount = (Vdp2Regs->CHCTLA & 0x70) >> 4;
	if(p.layer == NBG1)
		p.colorCount = (Vdp2Regs->CHCTLA & 0x1800) >> 12;
	if(p.layer == NBG2)
		p.colorCount = (Vdp2Regs->CHCTLB & 0x1);
	if(p.layer == NBG3)
		p.colorCount = (Vdp2Regs->CHCTLB & (1 << 5)) >> 5;
	if(p.layer == RBG0)
		p.colorCount = (Vdp2Regs->CHCTLB & 0x7000) >> 12;
}

void composite(pixeldata pixelstack[], int i, int j){

	int wctl;
	int colorCalculationEnable = 0;
	int alpha = 0;
	int specialColorCalculationMode = 0;
	int colorOffsetMask = 0;

	pixeldata first = pixelstack[0];
	pixeldata second = pixelstack[1];
	pixeldata third = pixelstack[2];

	readLayerInfo(first, wctl, colorCalculationEnable, alpha, specialColorCalculationMode, colorOffsetMask, i, j);

	//extended color calculation is enabled
	if(Vdp2Regs->CCCTL & (1 << 10)) {

		//we have to know the bit depth of the pixels to be blended
		getColorCount(second);
		getColorCount(third);

		//blend the second and third pixels
		//and write the result over the second

		//color ram mode 0
		if(((Vdp2Regs->RAMCTL & 0x3000) >> 12) == 0) {
			printf("mode 0 extended color calc");
			//line color screen does not insert
			if(!(Vdp2Regs->CCCTL & (1 << 5))) {

				//2nd and 3rd images can be palette or rbg format
				int a;
				int enabled;

				//got to check if the second pixel has color calc enabled
				readLayerInfo(second, a, enabled, a, a, a, i, j);

				if(enabled) {
					//2:2:0
					second.pixel = ColorCalc(second.pixel,third.pixel,15);
				}
				else {
					//2nd pixel is 100% visible
				}
			}
			//line color screen inserts
			else {
				printf("mode 0 extended color calc + lcl");
			}
		}

		//color ram mode 1 or 2
		else {
			//line color screen does not insert
			if(!(Vdp2Regs->CCCTL & (1 << 5))) {

				//3rd image is palette format
				if(third.colorCount < 3) {
					//the second layer pixel is 100% visible
				}
				//3rd image is rgb format
				if(third.colorCount >= 3) {
					int a;
					int enabled;

					//got to check if the second pixel has color calc enabled
					readLayerInfo(second, a, enabled, a, a, a, i, j);

					if(enabled) {
						//2:2:0
						second.pixel = ColorCalc(second.pixel,third.pixel,15);
					}
					else {
						//2nd pixel is 100% visible
					}
				}
			}
			//line color screen inserts
			else {
				printf("mode 1 or 2 extended + lcl");
			}
		}
	}
	//if we are selecting color calc by the second screen side
	//parodius penguin boss uses this
	if(Vdp2Regs->CCCTL & (1 << 9)) {

		colorCalculationEnable = false;
		if(second.layer == NBG0 && (Vdp2Regs->CCCTL & 0x1)) {
			alpha = Vdp2Regs->CCRNA & 0x1f;
			colorCalculationEnable = true;
		}
		if(second.layer == NBG1 && (Vdp2Regs->CCCTL & 0x2)) {
			alpha = Vdp2Regs->CCRNA >> 8;
			colorCalculationEnable = true;
		}
		if(second.layer == NBG2 && (Vdp2Regs->CCCTL & 0x4)) {
			alpha = Vdp2Regs->CCRNB & 0x1f;
			colorCalculationEnable = true;
		}
		if(second.layer == NBG3 && (Vdp2Regs->CCCTL & 0x8)) {
			alpha = Vdp2Regs->CCRNB >> 8;
			colorCalculationEnable = true;
		}
		if(second.layer == RBG0 && (Vdp2Regs->CCCTL & 0x10)) {
			colorCalculationEnable = true;
			alpha = Vdp2Regs->CCRR & 0x1f;
		}
		if(second.layer == SPRITE && (Vdp2Regs->CCCTL & ( 1 << 6))) {
			colorCalculationEnable = true;
			alpha = second.spriteColorCalcRatio;
		}
	}

	if(specialColorCalculationMode==1){
		printf("a");
	}
	if(specialColorCalculationMode==2){
		printf("b");
	}
	//activated based on the msb of the color data
	//in the case of radiant silvergun portraits it is looked up
	//and the value is actually from color ram transferred to first.pixel
	if(specialColorCalculationMode == 3) {
		if(first.pixel &(0x8000 << 9))//i'm not really sure pixel data gets stored like this internally but this is ok for now i guess
			colorCalculationEnable = 1;
		else
			colorCalculationEnable = 0;
	}
	///////////////////////////////////////////////////////////////////
	//temporary line color screen testing////////////////////
	int lineColor;
	int scrAddr;
	bool lineColorScreen = (Vdp2Regs->CCCTL & (1 << 5));

      if (Vdp2Regs->VRSIZE & 0x8000)	  
         scrAddr = ((Vdp2Regs->LCTA.all & 0x7FFFFUL) * 2);
      else
         scrAddr = ((Vdp2Regs->LCTA.all & 0x7FFFFUL) * 2);

	lineColor = T1ReadWord(Vdp2Ram, scrAddr);
	

	if(lineColorScreen && !first.colorCalcWindow) {
		alpha = Vdp2Regs->CCRLB & 0x1f;
		first.pixel = ColorCalc(lineColor, first.pixel, alpha);
	}
	/////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////
	else if(colorCalculationEnable && !first.colorCalcWindow)
		first.pixel = ColorCalc(first.pixel, second.pixel, alpha);
	else
		first.pixel = COLSAT2YAB32(0, first.pixel);

	//is color offset enabled?
	if (Vdp2Regs->CLOFEN & colorOffsetMask)// && first.layer == SPRITE
		first.pixel = COLOR_ADD(first.pixel,colorOffset.red,colorOffset.green,colorOffset.blue);
		bool shadowEnabled = false;

		//we need to check if the layer underneath has shadow enabled
		if(second.layer == NBG0)
			shadowEnabled = Vdp2Regs->SDCTL & (1 << 0);
		if(second.layer == NBG1)
			shadowEnabled = Vdp2Regs->SDCTL & (1 << 1);
		if(second.layer == NBG2)
			shadowEnabled = Vdp2Regs->SDCTL & (1 << 2);
		if(second.layer == NBG3)
			shadowEnabled = Vdp2Regs->SDCTL & (1 << 3);
		if(second.layer == RBG0)
			shadowEnabled = Vdp2Regs->SDCTL & (1 << 4);

	//shadow is processed after color calculation and color offset functions
	//normal shadow is displayed regardless of bit 8 SDCTL
	//puyo tsuu uses msb shadow in menu
	//normal shadow is always enabled, so games should not be using the reserved color value unless they want a shadow
	if(first.layer == SPRITE && first.isNormalShadow) {

		if(shadowEnabled)
			first.pixel = Shadow(second.pixel,80);
	}
	else if(first.layer == SPRITE && first.isMSBShadow) {

		//puyo tsuu has issues
		//it is not shading correctly

		//MSB shadow and the value of the sprite is greater than 0x8000, we shade the sprite instead of the background
		//sexy parodius corn boss
		if(first.pixel > 0x8000)
			first.pixel = Shadow(first.pixel,80);//&& (Vdp2Regs->SDCTL & (1 << 8)
		else //if (shadowEnabled)
			//this isn't right, taromaru looks correct if you use third.pixel, 20
			//can it skip over a layer and do a shadow?
			//second happens to be that layer of transparent mist,
			//so perhaps it skips color calculated layers?
			first.pixel = Shadow(second.pixel,80);
	}

	vdp2framebuffer[(j*vdp2width)+i] = first.pixel;


	// dst[(j*vdp2width)+i] = nbg0buf[(j*vdp2width)+i].pixel;
	// dst[(j*vdp2width)+i] = spritebuf[(j*vdp2width)+i].pixel;
	// dst[(j*vdp2width)+i] = first.pixel;
}

int pixelindex;

void layerStuff(pixeldata buf[], int i, int j, pixeldata arr[]) {

	pixeldata p = initPixelData();
//	p.pixel = INT_MAX;
	p = buf[(j*vdp2width)+i];

	if(testWindow(p, i, j))
		return;
	if(p.layer == _NULL)
		return;
	if(!p.transparent)
		arr[pixelindex++] = p;
}
static void Vdp2DrawBackScreen(void);

void VIDSoftVdp2DrawScreens(){

	memset(spritebuf,0,sizeof(spritebuf));
	memset(rbg0buf,0,sizeof(rbg0buf));
	memset(nbg0buf,0,sizeof(nbg0buf));
	memset(nbg1buf,0,sizeof(nbg1buf));
	memset(nbg2buf,0,sizeof(nbg2buf));
	memset(nbg3buf,0,sizeof(nbg3buf));

	Vdp2DrawSprites();
	Vdp2DrawRBG0();
	Vdp2DrawNBG0();
	Vdp2DrawNBG1();
	Vdp2DrawNBG2();
	Vdp2DrawNBG3();
	Vdp2DrawBackScreen();


	for (int j = 0; j < vdp2height; j++) {
		for (int i = 0; i < vdp2width; i++) {
			memset(pixelstack,0,sizeof(pixelstack));
			pixelindex = 0;
			for (int priority = 7; priority > 0; priority--) {

				if(spritebuf[(j*vdp2width)+i].priority == priority)
					layerStuff(spritebuf, i, j, pixelstack);

				if (rbg0priority == priority)
					layerStuff(rbg0buf, i, j, pixelstack);
				if (nbg0priority == priority)
					layerStuff(nbg0buf, i, j, pixelstack);
				if (nbg1priority == priority)
					layerStuff(nbg1buf, i, j, pixelstack);
				if (nbg2priority == priority)
					layerStuff(nbg2buf, i, j, pixelstack);
				if (nbg3priority == priority)
					layerStuff(nbg3buf, i, j, pixelstack);

			}
			pixelstack[pixelindex] = backbuf[(j*vdp2width)+i];
			composite(pixelstack, i, j);	
		}
	}
}

static INLINE u32 FASTCALL Vdp2ColorRamGetColor(u32 addr)
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);

         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0x7C00) << 9));
      }
      case 1:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);
         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0xFC00) << 9));//changed to FC to preserve the msb for per pixel color calc
      }
      case 2:
      {
         addr <<= 2;   
         return T2ReadLong(Vdp2ColorRam, addr & 0xFFF);
      }
      default: break;
   }

   return 0;
}


static INLINE int Vdp2FetchPixel(vdp2draw_struct *info, int x, int y, u32 *color)
{
   u32 dot;

   switch(info->colornumber)
   {
      case 0: // 4 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + ((y * info->cellw) + x) / 2) & 0x7FFFF));
         if (!(x & 0x1)) dot >>= 4;
         if (!(dot & 0xF) && info->transparencyenable) return 0;
         else
         {
            *color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | (dot & 0xF)));

            return 1;
         }
      case 1: // 8 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + (y * info->cellw) + x) & 0x7FFFF));
         if (!(dot & 0xFF) && info->transparencyenable) return 0;
         else
         {
            *color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | (dot & 0xFF)));
            return 1;
         }
      case 2: // 16 BPP(palette)
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * info->cellw) + x) * 2) & 0x7FFFF));
         if ((dot == 0) && info->transparencyenable) return 0;
         else
         {
            *color = Vdp2ColorRamGetColor(info->coloroffset + dot);
            return 1;
         }
      case 3: // 16 BPP(RGB)      
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * info->cellw) + x) * 2) & 0x7FFFF));
         if (!(dot & 0x8000) && info->transparencyenable) return 0;
         else
         {
            *color = COLSAT2YAB16(0, dot);
            return 1;
         }
      case 4: // 32 BPP
         dot = T1ReadLong(Vdp2Ram, ((info->charaddr + ((y * info->cellw) + x) * 4) & 0x7FFFF));
         if (!(dot & 0x80000000) && info->transparencyenable) return 0;
         else
         {
            *color = COLSAT2YAB32(0, dot);
            return 1;
         }
      default:
         return 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void SetupRotationInfo(vdp2draw_struct *info, vdp2rotationparameterfp_struct *p)
{
   if (info->rotatenum == 0)
   {
      Vdp2ReadRotationTableFP(0, p);
      info->PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
   }
   else
   {
      Vdp2ReadRotationTableFP(1, &p[1]);
      info->PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2DrawRotationFP(vdp2draw_struct *info, vdp2rotationparameterfp_struct *parameter)
{
	int i, j;
	int x, y;
	screeninfo_struct sinfo;
	vdp2rotationparameterfp_struct *p=&parameter[info->rotatenum];
	int doubley = 1;

	if(((Vdp2Regs->TVMD >> 6) & 0x3) == 3)
		doubley = 2;


	SetupRotationInfo(info, parameter);

	if (!p->coefenab)
	{
		fixed32 xmul, ymul, C, F;

		GenerateRotatedVarFP(p, &xmul, &ymul, &C, &F);

		// Do simple rotation
		CalculateRotationValuesFP(p);

		SetupScreenVars(info, &sinfo);

		for (j = 0; j < vdp2height; j++)
		{
			for (i = 0; i < vdp2width; i++)
			{
				u32 color;

				x = GenerateRotatedXPosFP(p, i/2, xmul, ymul, C) & sinfo.xmask;
				y = GenerateRotatedYPosFP(p, i/2, xmul, ymul, F) & sinfo.ymask;
				xmul += p->deltaXst;

				// Convert coordinates into graphics
				if (!info->isbitmap)
				{
					// Tile
					Vdp2MapCalcXY(info, &x, &y, &sinfo);
				}
				pixeldata pix = initPixelData();
				// Fetch pixel
				if (!Vdp2FetchPixel(info, x, y, &color))
				{
					pix.transparent = true;
				}
				else
					pix.transparent = false;

				pix.pixel = color;
				pix.layer = RBG0;

				rbg0buf[(j * vdp2width)+i] = pix;
			}
			ymul += p->deltaYst;
		}

		return;
	}
	else
	{
		fixed32 xmul, ymul, C, F;
		fixed32 coefx, coefy;

		GenerateRotatedVarFP(p, &xmul, &ymul, &C, &F);

		// Rotation using Coefficient Tables(now this stuff just gets wacky. It
		// has to be done in software, no exceptions)
		CalculateRotationValuesFP(p);

		SetupScreenVars(info, &sinfo);
		coefx = coefy = 0;

		for (j = 0; j < vdp2height; j++)
		{
			if (p->deltaKAx == 0)
			{
				Vdp2ReadCoefficientFP(p,
					p->coeftbladdr +
					touint(coefy) *
					p->coefdatasize);
			}

			for (i = 0; i < vdp2width; i++)
			{
				u32 color;

				if (p->deltaKAx != 0)
				{
					Vdp2ReadCoefficientFP(p,
						p->coeftbladdr +
						toint(coefy + coefx) *
						p->coefdatasize);
					coefx += p->deltaKAx;
				}
				pixeldata pix = initPixelData();
				if (p->msb)
				{
					pix.transparent = true;
				}

				x = GenerateRotatedXPosFP(p, i/2, xmul, ymul, C) & sinfo.xmask;
				y = GenerateRotatedYPosFP(p, i/2, xmul, ymul, F) & sinfo.ymask;
				xmul += p->deltaXst;

				// Convert coordinates into graphics
				if (!info->isbitmap)
				{
					// Tile
					Vdp2MapCalcXY(info, &x, &y, &sinfo);
				}

				// Fetch pixel
				if (!Vdp2FetchPixel(info, x, y, &color))
				{
					pix.transparent = true;
				}
				else
					pix.transparent = false;


				pix.pixel = color;
				pix.layer = RBG0;
				rbg0buf[(j * vdp2width)+i] = pix;
			}
			ymul += p->deltaYst;
			coefx = 0;
			coefy += p->deltaKAst;
		}
		return;
	}

	// Vdp2DrawScroll(info, RBG0, vdp2width, vdp2height);
}

static void Vdp2DrawBackScreen(void)
{
   int i;
	pixeldata p = initPixelData();
	p.pixel = 0;
   // Only draw black if TVMD's DISP and BDCLMD bits are cleared
   if ((Vdp2Regs->TVMD & 0x8000) == 0 && (Vdp2Regs->TVMD & 0x100) == 0)
   {
      // Draw Black
      for (i = 0; i < (vdp2width * vdp2height); i++)
		  backbuf[i] = p;
   }
   else
   {
      // Draw Back Screen
      u32 scrAddr;
      u16 dot;

      if (Vdp2Regs->VRSIZE & 0x8000)
         scrAddr = (((Vdp2Regs->BKTAU & 0x7) << 16) | Vdp2Regs->BKTAL) * 2;
      else
         scrAddr = (((Vdp2Regs->BKTAU & 0x3) << 16) | Vdp2Regs->BKTAL) * 2;

      if (Vdp2Regs->BKTAU & 0x8000)
      {
         // Per Line
         for (i = 0; i < vdp2height; i++)
         {
            dot = T1ReadWord(Vdp2Ram, scrAddr);
            scrAddr += 2;

			p.pixel = COLSAT2YAB16(0, dot);

			for (int x = 0; x < vdp2width; x++)
				backbuf[(i * vdp2width) + x] = p;
         }
      }
      else
      {
         // Single Color
         dot = T1ReadWord(Vdp2Ram, scrAddr);

		 p.pixel =  COLSAT2YAB16(0, dot);

		 for (i = 0; i < (vdp2width * vdp2height); i++) {
			 backbuf[i] = p;
			}
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;
   vdp2rotationparameterfp_struct parameter[2];

   if (Vdp2Regs->BGON & 0x20)
   {
      // RBG1 mode
      info.enable = Vdp2Regs->BGON & 0x20;

      // Read in Parameter B
      Vdp2ReadRotationTableFP(1, &parameter[1]);

      if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode
         ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 2, 0x3);

         info.charaddr = (Vdp2Regs->MPOFR & 0x70) * 0x2000;
         info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 8;
         info.flipfunction = 0;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 4;
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 12);
         ReadPatternData(&info, Vdp2Regs->PNCN0, Vdp2Regs->CHCTLA & 0x1);
      }

      info.rotatenum = 1;
      info.rotatemode = 0;
      info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
   }
   else if (Vdp2Regs->BGON & 0x1)
   {
      // NBG0 mode
      info.enable = Vdp2Regs->BGON & 0x1;

      if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode
         ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 2, 0x3);

         info.x = Vdp2Regs->SCXIN0 & 0x7FF;
         info.y = Vdp2Regs->SCYIN0 & 0x7FF;

         info.charaddr = (Vdp2Regs->MPOFN & 0x7) * 0x20000;
         info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 8;
         info.flipfunction = 0;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 2;

         ReadPlaneSize(&info, Vdp2Regs->PLSZ);

         info.x = Vdp2Regs->SCXIN0 & 0x7FF;
         info.y = Vdp2Regs->SCYIN0 & 0x7FF;
         ReadPatternData(&info, Vdp2Regs->PNCN0, Vdp2Regs->CHCTLA & 0x1);
      }

      info.coordincx = (Vdp2Regs->ZMXN0.all & 0x7FF00) / (float) 65536;
      info.coordincy = (Vdp2Regs->ZMYN0.all & 0x7FF00) / (float) 65536;
      info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG0PlaneAddr;
   }
   else
      // Not enabled
      return;

   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);
   info.specialprimode = Vdp2Regs->SFPRMD & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x70) >> 4;

   if (Vdp2Regs->CCCTL & 0x1)
      info.alpha = Vdp2Regs->CCRNA & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7) << 8;
   info.priority = nbg0priority;

   if (!(info.enable & Vdp2External.disptoggle))
      return;

   ReadMosaicData(&info, 0x1);
   ReadLineScrollData(&info, Vdp2Regs->SCRCTL & 0xFF, Vdp2Regs->LSTA0.all);
   if (Vdp2Regs->SCRCTL & 1)
   {
      info.isverticalscroll = 1;
      info.verticalscrolltbl = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
      if (Vdp2Regs->SCRCTL & 0x100)
         info.verticalscrollinc = 8;
      else
         info.verticalscrollinc = 4;
   }
   else
      info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLA;

   if (info.enable == 1)
   {
      // NBG0 draw
      Vdp2DrawScroll(&info, NBG0, vdp2width, vdp2height);
   }
   else
   {
      // RBG1 draw
      Vdp2DrawRotationFP(&info, parameter);
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 2) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x200) != 0)
   {
      ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 10, 0x3);

      info.x = Vdp2Regs->SCXIN1 & 0x7FF;
      info.y = Vdp2Regs->SCYIN1 & 0x7FF;

      info.charaddr = ((Vdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
      info.paladdr = Vdp2Regs->BMPNA & 0x700;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      info.mapwh = 2;

      ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 2);

      info.x = Vdp2Regs->SCXIN1 & 0x7FF;
      info.y = Vdp2Regs->SCYIN1 & 0x7FF;

      ReadPatternData(&info, Vdp2Regs->PNCN1, Vdp2Regs->CHCTLA & 0x100);
   }

   if (Vdp2Regs->CCCTL & 0x2)
      info.alpha = (Vdp2Regs->CCRNA >> 8) & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) << 4;
   info.coordincx = (Vdp2Regs->ZMXN1.all & 0x7FF00) / (float) 65536;
   info.coordincy = (Vdp2Regs->ZMYN1.all & 0x7FF00) / (float) 65536;

   info.priority = nbg1priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG1PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return;

   ReadMosaicData(&info, 0x2);
   ReadLineScrollData(&info, Vdp2Regs->SCRCTL >> 8, Vdp2Regs->LSTA1.all);
   if (Vdp2Regs->SCRCTL & 0x100)
   {
      info.isverticalscroll = 1;
      if (Vdp2Regs->SCRCTL & 0x1)
      {
         info.verticalscrolltbl = 4 + ((Vdp2Regs->VCSTA.all & 0x7FFFE) << 1);
         info.verticalscrollinc = 8;
      }
      else
      {
         info.verticalscrolltbl = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
         info.verticalscrollinc = 4;
      }
   }
   else
      info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLA >> 8;

   Vdp2DrawScroll(&info, NBG1, vdp2width, vdp2height);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG2(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x4;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x400);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 4) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x2) >> 1;	
   info.mapwh = 2;

   ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 4);
   info.x = Vdp2Regs->SCXN2 & 0x7FF;
   info.y = Vdp2Regs->SCYN2 & 0x7FF;
   ReadPatternData(&info, Vdp2Regs->PNCN2, Vdp2Regs->CHCTLB & 0x1);
    
   if (Vdp2Regs->CCCTL & 0x4)
      info.alpha = Vdp2Regs->CCRNB & 0x1F;

   info.coloroffset = Vdp2Regs->CRAOFA & 0x700;
   info.coordincx = info.coordincy = 1;

   info.priority = nbg2priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG2PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return;

   ReadMosaicData(&info, 0x4);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLB;
   info.isbitmap = 0;

   Vdp2DrawScroll(&info, NBG2, vdp2width, vdp2height);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG3(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x8;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x800);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 6) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x20) >> 5;
	
   info.mapwh = 2;

   ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 6);
   info.x = Vdp2Regs->SCXN3 & 0x7FF;
   info.y = Vdp2Regs->SCYN3 & 0x7FF;
   ReadPatternData(&info, Vdp2Regs->PNCN3, Vdp2Regs->CHCTLB & 0x10);

   if (Vdp2Regs->CCCTL & 0x8)
      info.alpha = (Vdp2Regs->CCRNB >> 8) & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7000) >> 4;
   info.coordincx = info.coordincy = 1;

   info.priority = nbg3priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG3PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return;

   ReadMosaicData(&info, 0x8);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLB >> 8;
   info.isbitmap = 0;

   Vdp2DrawScroll(&info, NBG3, vdp2width, vdp2height);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawRBG0(void)
{
   vdp2draw_struct info;
   vdp2rotationparameterfp_struct parameter[2];

   info.enable = Vdp2Regs->BGON & 0x10;
   info.priority = rbg0priority;
   if (!(info.enable & Vdp2External.disptoggle))
      return;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x1000);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 8) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x7000) >> 12;

   // Figure out which Rotation Parameter we're using
   switch (Vdp2Regs->RPMD & 0x3)
   {
      case 0:
         // Parameter A
         info.rotatenum = 0;
         info.rotatemode = 0;
         info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
         break;
      case 1:
         // Parameter B
         info.rotatenum = 1;
         info.rotatemode = 0;
         info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
         break;
      case 2:
         // Parameter A+B switched via coefficients
      case 3:
         // Parameter A+B switched via rotation parameter window
      default:
         info.rotatenum = 0;
         info.rotatemode = 1 + (Vdp2Regs->RPMD & 0x1);
         info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
         break;
   }

   Vdp2ReadRotationTableFP(info.rotatenum, &parameter[info.rotatenum]);

   if((info.isbitmap = Vdp2Regs->CHCTLB & 0x200) != 0)
   {
      // Bitmap Mode
      ReadBitmapSize(&info, Vdp2Regs->CHCTLB >> 10, 0x1);

      if (info.rotatenum == 0)
         // Parameter A
         info.charaddr = (Vdp2Regs->MPOFR & 0x7) * 0x20000;
      else
         // Parameter B
         info.charaddr = (Vdp2Regs->MPOFR & 0x70) * 0x2000;

      info.paladdr = (Vdp2Regs->BMPNB & 0x7) << 8;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      // Tile Mode
      info.mapwh = 4;

      if (info.rotatenum == 0)
         // Parameter A
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 8);
      else
         // Parameter B
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 12);

      ReadPatternData(&info, Vdp2Regs->PNCR, Vdp2Regs->CHCTLB & 0x100);
   }

   if (Vdp2Regs->CCCTL & 0x10)
      info.alpha = Vdp2Regs->CCRR & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFB & 0x7) << 8;

   info.coordincx = info.coordincy = 1;

   ReadMosaicData(&info, 0x10);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLC;

   Vdp2DrawRotationFP(&info, parameter);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftInit(void)
{
   // Initialize output buffer
   if ((dispbuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   // Initialize VDP1 framebuffer 1
   if ((vdp1framebuffer[0] = (u8 *)calloc(sizeof(u8), 0x40000*9)) == NULL)
      return -1;

   // Initialize VDP1 framebuffer 2
   if ((vdp1framebuffer[1] = (u8 *)calloc(sizeof(u8), 0x40000*9)) == NULL)
      return -1;

   // Initialize VDP2 framebuffer
   if ((vdp2framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512*2)) == NULL)
      return -1;


   if ((nbg0framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg1framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg2framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg3framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((rbg0framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   vdp1backframebuffer = vdp1framebuffer[0];
   vdp1frontframebuffer = vdp1framebuffer[1];
   vdp2width = 320;
   vdp2height = 224;

#ifdef USE_OPENGL
   YuiSetVideoAttribute(DOUBLEBUFFER, 1);

   if (!YglScreenInit(8, 8, 8, 24))
   {
      if (!YglScreenInit(4, 4, 4, 24))
      {
         if (!YglScreenInit(5, 6, 5, 16))
         {
	    YuiErrorMsg("Couldn't set GL mode\n");
            return -1;
         }
      }
   }

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 320, 224, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-320, 320, -224, 224, 1, 0);
   outputwidth = 320;
   outputheight = 224;
   msglength = 0;
#endif

   VideoInitGlut();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftDeInit(void)
{
   if (dispbuffer)
   {
      free(dispbuffer);
      dispbuffer = NULL;
   }

   if (vdp1framebuffer[0])
      free(vdp1framebuffer[0]);

   if (vdp1framebuffer[1])
      free(vdp1framebuffer[1]);

   if (vdp2framebuffer)
      free(vdp2framebuffer);
}

//////////////////////////////////////////////////////////////////////////////

static int IsFullscreen = 0;

void VIDSoftResize(unsigned int w, unsigned int h, int on)
{
#ifdef USE_OPENGL
   IsFullscreen = on;

   if (on)
      YuiSetVideoMode(w, h, 32, 1);
   else
      YuiSetVideoMode(w, h, 32, 0);

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, h, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-w, w, -h, h, 1, 0);

   glViewport(0, 0, w, h);
   outputwidth = w;
   outputheight = h;
#endif
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftIsFullscreen(void) {
   return IsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp1Reset(void)
{
   vdp1clipxstart = 0;
   vdp1clipxend = 512;
   vdp1clipystart = 0;
   vdp1clipyend = 256;
   
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawStart(void)
{
	switch(Vdp1Regs->TVMR & 7) {
	//Normal
	case 0:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	//Hi resolution
	case 1:
		vdp1width = 1024;
		vdp1height = 256;
		vdp1pixelsize = 1;
		break;
	//Rotation 16
	case 2:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	//Rotation 8
	case 3:
		vdp1width = 512;
		vdp1height = 512;
		vdp1pixelsize = 1;
		break;
	//HDTV
	case 4:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	}
	VIDSoftVdp1EraseFrameBuffer();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u16  Vdp1ReadPattern16( u32 base, u32 offset ) {

  u16 dot = T1ReadByte(Vdp1Ram, ( base + (offset>>1)) & 0x7FFFF);
  if ((offset & 0x1) == 0) dot >>= 4; // Even pixel
  else dot &= 0xF; // Odd pixel
  return dot;
}

static INLINE u16  Vdp1ReadPattern64( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0x3F;
}

static INLINE u16  Vdp1ReadPattern128( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0x7F;
}

static INLINE u16  Vdp1ReadPattern256( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0xFF;
}

static INLINE u16  Vdp1ReadPattern64k( u32 base, u32 offset ) {

  return T1ReadWord(Vdp1Ram, ( base + 2*offset) & 0x7FFFF);
}

////////////////////////////////////////////////////////////////////////////////

INLINE u32 alphablend16(u32 d, u32 s, u32 level)
{
	int r,g,b,sr,sg,sb,dr,dg,db;

	int invlevel = 256-level;
	sr = s & 0x001f; dr = d & 0x001f; 
	r = (sr*level + dr*invlevel)>>8; r&= 0x1f;
	sg = s & 0x03e0; dg = d & 0x03e0;
	g = (sg*level + dg*invlevel)>>8; g&= 0x03e0;
	sb = s & 0x7c00; db = d & 0x7c00;
	b = (sb*level + db*invlevel)>>8; b&= 0x7c00;
	return r|g|b;
}

typedef struct _COLOR_PARAMS
{
	double r,g,b;
} COLOR_PARAMS;

COLOR_PARAMS leftColumnColor;

vdp1cmd_struct cmd;

int currentPixel;
int currentPixelIsVisible;
int characterWidth;
int characterHeight;

int getpixel(int linenumber, int currentlineindex) {
	
	u32 characterAddress;
	u32 colorlut;
	u16 colorbank;
	u8 SPD;
	int endcode;
	int endcodesEnabled;
	int untexturedColor;
	int isTextured = 1;
	int currentShape = cmd.CMDCTRL & 0x7;
	int flip;

	characterAddress = cmd.CMDSRCA << 3;
	colorbank = cmd.CMDCOLR;
	colorlut = (u32)colorbank << 3;
	SPD = ((cmd.CMDPMOD & 0x40) != 0);//show the actual color of transparent pixels if 1 (they won't be drawn transparent)
	endcodesEnabled = (( cmd.CMDPMOD & 0x80) == 0 )?1:0;
	flip = (cmd.CMDCTRL & 0x30) >> 4;

	//4 polygon, 5 polyline or 6 line
	if(currentShape == 4 || currentShape == 5 || currentShape == 6) {
		isTextured = 0;
		untexturedColor = cmd.CMDCOLR;
	}

	switch( flip ) {
		case 1:
			// Horizontal flipping
			currentlineindex = characterWidth - currentlineindex-1;
			break;
		case 2:
			// Vertical flipping
			linenumber = characterHeight - linenumber-1;

			break;
		case 3:
			// Horizontal/Vertical flipping
			linenumber = characterHeight - linenumber-1;
			currentlineindex = characterWidth - currentlineindex-1;
			break;
	}

	switch ((cmd.CMDPMOD >> 3) & 0x7)
	{
		case 0x0: //4bpp bank
			endcode = 0xf;
			currentPixel = Vdp1ReadPattern16( characterAddress + (linenumber*(characterWidth>>1)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0xf;
			break;

		case 0x1://4bpp lut
			endcode = 0xf;
			currentPixel = Vdp1ReadPattern16( characterAddress + (linenumber*(characterWidth>>1)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!(currentPixel == 0 && !SPD))
				currentPixel = T1ReadWord(Vdp1Ram, (currentPixel * 2 + colorlut) & 0x7FFFF);
			currentPixelIsVisible = 0xffff;
			break;
		case 0x2://8pp bank (64 color)
			//is there a hardware bug with endcodes in this color mode?
			//there are white lines around some characters in scud
			//using an endcode of 63 eliminates the white lines
			//but also causes some dropout due to endcodes being triggered that aren't triggered on hardware
			//the closest thing i can do to match the hardware is make all pixels with color index 63 transparent
			//this needs more hardware testing

			endcode = 63;
			currentPixel = Vdp1ReadPattern64( characterAddress + (linenumber*(characterWidth)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				currentPixel = 0;
		//		return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0x3f;
			break;
		case 0x3://128 color
			endcode = 0xff;
			currentPixel = Vdp1ReadPattern128( characterAddress + (linenumber*characterWidth), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0x7f;
			break;
		case 0x4://256 color
			endcode = 0xff;
			currentPixel = Vdp1ReadPattern256( characterAddress + (linenumber*characterWidth), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			currentPixelIsVisible = 0xff;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			break;
		case 0x5://16bpp bank
			endcode = 0x7fff;
			currentPixel = Vdp1ReadPattern64k( characterAddress + (linenumber*characterWidth*2), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			currentPixelIsVisible = 0xffff;
			break;
	}

	if(!isTextured)
		currentPixel = untexturedColor;

	return 0;
}

int gouraudAdjust( int color, int tableValue )
{
	color += (tableValue - 0x10);

	if ( color < 0 ) color = 0;
	if ( color > 0x1f ) color = 0x1f;

	return color;
}

void putpixel(int x, int y) {

	u16* iPix = &((u16 *)vdp1backframebuffer)[(y * vdp1width) + x];
	int mesh = cmd.CMDPMOD & 0x0100;
	int SPD = ((cmd.CMDPMOD & 0x40) != 0);//show the actual color of transparent pixels if 1 (they won't be drawn transparent)
	int currentShape = cmd.CMDCTRL & 0x7;
	int isTextured=1;

	if(mesh && (x^y)&1)
		return;

	if(currentShape == 4 || currentShape == 5 || currentShape == 6)
		isTextured = 0;

	if (cmd.CMDPMOD & 0x0400) PushUserClipping((cmd.CMDPMOD >> 9) & 0x1);

	if (x >= vdp1clipxstart &&
		x < vdp1clipxend &&
		y >= vdp1clipystart &&
		y < vdp1clipyend)
	{}
	else
		return;

	if (cmd.CMDPMOD & 0x0400) PopUserClipping();

	//see the corn boss in sexy parodius
	//taromaru demonstrates msb shadow without the overlapping
	//if MSB ON is set we just modify the msb of the framebuffer pixel
	if(cmd.CMDPMOD & (1 << 15)) {
		if(isTextured) {
			if (!((currentPixel == 0) && !SPD))
				*(iPix) |= (1 << 15);
		}
		else
			*(iPix) |= (1 << 15);

		return;
	}

	if ( SPD || (currentPixel & currentPixelIsVisible))
	{
		switch( cmd.CMDPMOD & 0x7 )//we want bits 0,1,2
		{
		case 0:	// replace
			if (!((currentPixel == 0) && !SPD)) 
				*(iPix) = currentPixel;
			break;
		case 1: // shadow, TODO
			*(iPix) = currentPixel;
			break;
		case 2: // half luminance
			*(iPix) = ((currentPixel & ~0x8421) >> 1) | (1 << 15);
			break;
		case 3: // half transparent
			if ( *(iPix) & (1 << 15) )//only if MSB of framebuffer data is set 
				*(iPix) = alphablend16( *(iPix), currentPixel, (1 << 7) ) | (1 << 15);
			else
				*(iPix) = currentPixel;
			break;
		case 4: //gouraud
			#define COLOR(r,g,b)    (((r)&0x1F)|(((g)&0x1F)<<5)|(((b)&0x1F)<<10) |0x8000 )

			//handle the special case demonstrated in the sgl chrome demo
			//if we are in a paletted bank mode and the other two colors are unused, adjust the index value instead of rgb
			if(
				(((cmd.CMDPMOD >> 3) & 0x7) != 5) &&
				(((cmd.CMDPMOD >> 3) & 0x7) != 1) && 
				(int)leftColumnColor.g == 16 && 
				(int)leftColumnColor.b == 16) 
			{
				int c = (int)(leftColumnColor.r-0x10);
				if(c < 0) c = 0;
				currentPixel = currentPixel+c;
				*(iPix) = currentPixel;
				break;
			}
			*(iPix) = COLOR(
				gouraudAdjust(
				currentPixel&0x001F,
				(int)leftColumnColor.r),

				gouraudAdjust(
				(currentPixel&0x03e0) >> 5,
				(int)leftColumnColor.g),

				gouraudAdjust(
				(currentPixel&0x7c00) >> 10,
				(int)leftColumnColor.b)
				);
			break;
		default:
			*(iPix) = alphablend16( COLOR((int)leftColumnColor.r,(int)leftColumnColor.g, (int)leftColumnColor.b), currentPixel, (1 << 7) ) | (1 << 15);
			break;
		}
	}
}

//TODO consolidate the following 3 functions
int bresenham( int x1, int y1, int x2, int y2, int x[], int y[])
{
	int dx, dy, xf, yf, a, b, c, i;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	//buffer protection
	if(dx > 4095 || dy > 4095)
		return INT_MAX;

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {
			x[i] = x1; y[i] = y1;
			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				y1 += yf;
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;
		for (i=0;i<=dy;i++) {
			x[i] = x1; y[i] = y1;
			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				x1 += xf;
			}
		}
		return dy+1;
	}
}

int DrawLine( int x1, int y1, int x2, int y2, double linenumber, double texturestep, double xredstep, double xgreenstep, double xbluestep)
{
	int dx, dy, xf, yf, a, b, c, i;
	int endcodesdetected=0;
	int previousStep = 123456789;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {
			leftColumnColor.r+=xredstep;
			leftColumnColor.g+=xgreenstep;
			leftColumnColor.b+=xbluestep;

			if(getpixel(linenumber,(int)i*texturestep)) {
				if(currentPixel != previousStep) {
					previousStep = (int)i*texturestep;
					endcodesdetected++;
				}
			}
			else
				putpixel(x1,y1);

			previousStep = currentPixel;

			if(endcodesdetected==2)
				break;

			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				getpixel(linenumber,(int)i*texturestep);
				putpixel(x1,y1);
				c += b;
				y1 += yf;
/*
				//same as sega's way, but just move the code down here instead
				//and use the pixel we already have instead of the next one
				if(xf>1&&yf>1) putpixel(x1,y1-1); //case 1
				if(xf<1&&yf<1) putpixel(x1,y1+1); //case 2
				if(xf<1&&yf>1) putpixel(x1+1,y1); //case 7
				if(xf>1&&yf<1) putpixel(x1-1,y1); //case 8*/
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;	
	for (i=0;i<=dy;i++) {
			leftColumnColor.r+=xredstep;
			leftColumnColor.g+=xgreenstep;
			leftColumnColor.b+=xbluestep;

			if(getpixel(linenumber,(int)i*texturestep)) {
				if(currentPixel != previousStep) {
					previousStep = (int)i*texturestep;
					endcodesdetected++;
				}
			}
			else
				putpixel(x1,y1);

			previousStep = currentPixel;

			if(endcodesdetected==2)
				break;

			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				getpixel(linenumber,(int)i*texturestep);
				putpixel(x1,y1);
				c += b;
				x1 += xf;
/*				
				if(xf>1&&yf>1) putpixel(x1,y1-1); //case 3
				if(xf<1&&yf<1) putpixel(x1,y1+1); //case 4
				if(xf<1&&yf>1) putpixel(x1+1,y1); //case 5
				if(xf>1&&yf<1) putpixel(x1-1,y1); //case 6*/

			}
		}
		return dy+1;
	}
}

int getlinelength(int x1, int y1, int x2, int y2) {
	int dx, dy, xf, yf, a, b, c, i;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {

			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				y1 += yf;
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;	
	for (i=0;i<=dy;i++) {
			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				x1 += xf;
			}
		}
		return dy+1;
	}
}

INLINE double interpolate(double start, double end, int numberofsteps) {

	double stepvalue = 0;

	if(numberofsteps == 0)
		return 1;

	stepvalue = (end - start) / numberofsteps;

	return stepvalue;
}

typedef union _COLOR { // xbgr x555
	struct {
#ifdef WORDS_BIGENDIAN
	u16 x:1;
	u16 b:5;
	u16 g:5;
	u16 r:5;
#else
     u16 r:5;
     u16 g:5;
     u16 b:5;
     u16 x:1;
#endif
	};
	u16 value;
} COLOR;


COLOR gouraudA;
COLOR gouraudB;
COLOR gouraudC;
COLOR gouraudD;

void gouraudTable()
{
	int gouraudTableAddress;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	gouraudTableAddress = (((unsigned int)cmd.CMDGRDA) << 3);

	gouraudA.value = T1ReadWord(Vdp1Ram,gouraudTableAddress);
	gouraudB.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+2);
	gouraudC.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+4);
	gouraudD.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+6);
}

int xleft[4096];
int yleft[4096];
int xright[4096];
int yright[4096];

//a real vdp1 draws with arbitrary lines
//this is why endcodes are possible
//this is also the reason why half-transparent shading causes moire patterns
//and the reason why gouraud shading can be applied to a single line draw command
void drawQuad(s32 tl_x, s32 tl_y, s32 bl_x, s32 bl_y, s32 tr_x, s32 tr_y, s32 br_x, s32 br_y){

	int totalleft;
	int totalright;
	int total;
	int i;

	COLOR_PARAMS topLeftToBottomLeftColorStep, topRightToBottomRightColorStep;
		
	//how quickly we step through the line arrays
	double leftLineStep = 1;
	double rightLineStep = 1; 

	//a lookup table for the gouraud colors
	COLOR colors[4];

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
	characterWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	characterHeight = cmd.CMDSIZE & 0xFF;

	totalleft  = bresenham(tl_x,tl_y,bl_x,bl_y,xleft,yleft);
	totalright = bresenham(tr_x,tr_y,br_x,br_y,xright,yright);

	//just for now since burning rangers will freeze up trying to draw huge shapes
	if(totalleft == INT_MAX || totalright == INT_MAX)
		return;

	total = totalleft > totalright ? totalleft : totalright;


	if(cmd.CMDPMOD & (1 << 2)) {

		gouraudTable();

		{ colors[0] = gouraudA; colors[1] = gouraudD; colors[2] = gouraudB; colors[3] = gouraudC; }

		topLeftToBottomLeftColorStep.r = interpolate(colors[0].r,colors[1].r,total);
		topLeftToBottomLeftColorStep.g = interpolate(colors[0].g,colors[1].g,total);
		topLeftToBottomLeftColorStep.b = interpolate(colors[0].b,colors[1].b,total);

		topRightToBottomRightColorStep.r = interpolate(colors[2].r,colors[3].r,total);
		topRightToBottomRightColorStep.g = interpolate(colors[2].g,colors[3].g,total);
		topRightToBottomRightColorStep.b = interpolate(colors[2].b,colors[3].b,total);
	}

	//we have to step the equivalent of less than one pixel on the shorter side
	//to make sure textures stretch properly and the shape is correct
	if(total == totalleft && totalleft != totalright) {
		//left side is larger
		leftLineStep = 1;
		rightLineStep = (double)totalright / totalleft;
	}
	else if(totalleft != totalright){
		//right side is larger
		rightLineStep = 1;
		leftLineStep = (double)totalleft / totalright;
	}

	for(i = 0; i < total; i++) {

		int xlinelength;

		double xtexturestep;
		double ytexturestep;

		COLOR_PARAMS rightColumnColor;

		COLOR_PARAMS leftToRightStep = {0,0,0};

		//get the length of the line we are about to draw
		xlinelength = getlinelength(
			xleft[(int)(i*leftLineStep)],
			yleft[(int)(i*leftLineStep)],
			xright[(int)(i*rightLineStep)],
			yright[(int)(i*rightLineStep)]);

		//so from 0 to the width of the texture / the length of the line is how far we need to step
		xtexturestep=interpolate(0,characterWidth,xlinelength);

		//now we need to interpolate the y texture coordinate across multiple lines
		ytexturestep=interpolate(0,characterHeight,total);

		//gouraud interpolation
		if(cmd.CMDPMOD & (1 << 2)) {

			//for each new line we need to step once more through each column
			//and add the orignal color + the number of steps taken times the step value to the bottom of the shape
			//to get the current colors to use to interpolate across the line

			leftColumnColor.r = colors[0].r +(topLeftToBottomLeftColorStep.r*i);
			leftColumnColor.g = colors[0].g +(topLeftToBottomLeftColorStep.g*i);
			leftColumnColor.b = colors[0].b +(topLeftToBottomLeftColorStep.b*i);

			rightColumnColor.r = colors[2].r +(topRightToBottomRightColorStep.r*i);
			rightColumnColor.g = colors[2].g +(topRightToBottomRightColorStep.g*i);
			rightColumnColor.b = colors[2].b +(topRightToBottomRightColorStep.b*i);

			//interpolate colors across to get the right step values
			leftToRightStep.r = interpolate(leftColumnColor.r,rightColumnColor.r,xlinelength);
			leftToRightStep.g = interpolate(leftColumnColor.g,rightColumnColor.g,xlinelength);
			leftToRightStep.b = interpolate(leftColumnColor.b,rightColumnColor.b,xlinelength);
		}

		DrawLine(
			xleft[(int)(i*leftLineStep)],
			yleft[(int)(i*leftLineStep)],
			xright[(int)(i*rightLineStep)],
			yright[(int)(i*rightLineStep)],
			ytexturestep*i, 
			xtexturestep,
			leftToRightStep.r,
			leftToRightStep.g,
			leftToRightStep.b
			);
	}
}

void VIDSoftVdp1NormalSpriteDraw() {

	s16 topLeftx,topLefty,topRightx,topRighty,bottomRightx,bottomRighty,bottomLeftx,bottomLefty;
	int spriteWidth;
	int spriteHeight;
	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	topLeftx = cmd.CMDXA + Vdp1Regs->localX;
	topLefty = cmd.CMDYA + Vdp1Regs->localY;
	spriteWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	spriteHeight = cmd.CMDSIZE & 0xFF;

	topRightx = topLeftx+spriteWidth;
	topRighty = topLefty;
	bottomRightx = topLeftx+spriteWidth;
	bottomRighty = topLefty+spriteHeight;
	bottomLeftx = topLeftx;
	bottomLefty = topLefty+spriteHeight;

	drawQuad(topLeftx,topLefty,bottomLeftx,bottomLefty,topRightx,topRighty,bottomRightx,bottomRighty);
}

void VIDSoftVdp1ScaledSpriteDraw(){

	s32 topLeftx,topLefty,topRightx,topRighty,bottomRightx,bottomRighty,bottomLeftx,bottomLefty;
	int spriteWidth;
	int spriteHeight;
	int flip = 0;
	int x0,y0,x1,y1;
	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	x0 = cmd.CMDXA + Vdp1Regs->localX;
	y0 = cmd.CMDYA + Vdp1Regs->localY;

	switch ((cmd.CMDCTRL >> 8) & 0xF)
	{
	case 0x0: // Only two coordinates
	default:
		x1 = ((int)cmd.CMDXC) - x0 + Vdp1Regs->localX + 1;
		y1 = ((int)cmd.CMDYC) - y0 + Vdp1Regs->localY + 1;
		break;
	case 0x5: // Upper-left
		x1 = ((int)cmd.CMDXB) + 1;
		y1 = ((int)cmd.CMDYB) + 1;
		break;
	case 0x6: // Upper-Center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		x1++;
		y1++;
		break;
	case 0x7: // Upper-Right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		x1++;
		y1++;
		break;
	case 0x9: // Center-left
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xA: // Center-center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xB: // Center-right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xD: // Lower-left
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	case 0xE: // Lower-center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	case 0xF: // Lower-right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	}

	spriteWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	spriteHeight = cmd.CMDSIZE & 0xFF;

	topLeftx = x0;
	topLefty = y0;

	topRightx = x1+x0;
	topRighty = topLefty;

	bottomRightx = x1+x0;
	bottomRighty = y1+y0;

	bottomLeftx = topLeftx;
	bottomLefty = y1+y0;

	drawQuad(topLeftx,topLefty,bottomLeftx,bottomLefty,topRightx,topRighty,bottomRightx,bottomRighty);
}
//the upper bits of the CMD__ parts are designated as code extension
//i would expect to need to ignore all of them but guardian heroes needs to draw sprites at -1840
//which is outside the -1024 to 1023 range suggested by the manual
//ignoring them all would be (((s16)((X)<<5))/32) but guardian heroes breaks so we will leave an additional bit
//burning rangers wants to draw polygons at 32769 without this
//more research should be done
#define TRUNC_10BIT(X) (((s16)((X)<<4))/16) //watch out for right shift arithmetic
void VIDSoftVdp1DistortedSpriteDraw() {

	s32 xa,ya,xb,yb,xc,yc,xd,yd;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    xa = (s32)(TRUNC_10BIT(cmd.CMDXA) + Vdp1Regs->localX);
	ya = (s32)(TRUNC_10BIT(cmd.CMDYA) + Vdp1Regs->localY);

    xb = (s32)(TRUNC_10BIT(cmd.CMDXB) + Vdp1Regs->localX);
	yb = (s32)(TRUNC_10BIT(cmd.CMDYB) + Vdp1Regs->localY);

    xc = (s32)(TRUNC_10BIT(cmd.CMDXC) + Vdp1Regs->localX);
	yc = (s32)(TRUNC_10BIT(cmd.CMDYC) + Vdp1Regs->localY);

    xd = (s32)(TRUNC_10BIT(cmd.CMDXD) + Vdp1Regs->localX);
	yd = (s32)(TRUNC_10BIT(cmd.CMDYD) + Vdp1Regs->localY);

	drawQuad(xa,ya,xd,yd,xb,yb,xc,yc);
}

void gouraudLineSetup(double * redstep, double * greenstep, double * bluestep, int length, COLOR table1, COLOR table2) {

	gouraudTable();

	*redstep =interpolate(table1.r,table2.r,length);
	*greenstep =interpolate(table1.g,table2.g,length);
	*bluestep =interpolate(table1.b,table2.b,length);

	leftColumnColor.r = table1.r;
	leftColumnColor.g = table1.g;
	leftColumnColor.b = table1.b;
}

void VIDSoftVdp1PolylineDraw(void)
{
	int X[4];
	int Y[4];
	double redstep = 0, greenstep = 0, bluestep = 0;
	int length;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	X[0] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
	Y[0] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
	X[1] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
	Y[1] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));
	X[2] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14));
	Y[2] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16));
	X[3] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18));
	Y[3] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A));

	length = getlinelength(X[0], Y[0], X[1], Y[1]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudA, gouraudB);
	DrawLine(X[0], Y[0], X[1], Y[1], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[1], Y[1], X[2], Y[2]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudB, gouraudC);
	DrawLine(X[1], Y[1], X[2], Y[2], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[2], Y[2], X[3], Y[3]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudD, gouraudC);
	DrawLine(X[3], Y[3], X[2], Y[2], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[3], Y[3], X[0], Y[0]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudA,gouraudD);
	DrawLine(X[0], Y[0], X[3], Y[3], 0,0,redstep,greenstep,bluestep);
}

void VIDSoftVdp1LineDraw(void)
{
	int x1, y1, x2, y2;
	double redstep = 0, greenstep = 0, bluestep = 0;
	int length;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	x1 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
	y1 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
	x2 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
	y2 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));

	length = getlinelength(x1, y1, x2, y2);
	gouraudLineSetup(&redstep,&bluestep,&greenstep,length, gouraudA, gouraudB);
	DrawLine(x1, y1, x2, y2, 0,0,redstep,greenstep,bluestep);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1UserClipping(void)
{
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

#if 0
   vdp1clipxstart = Vdp1Regs->userclipX1;
   vdp1clipxend = Vdp1Regs->userclipX2;
   vdp1clipystart = Vdp1Regs->userclipY1;
   vdp1clipyend = Vdp1Regs->userclipY2;

   // This needs work
   if (vdp1clipxstart > Vdp1Regs->systemclipX1)
      vdp1clipxstart = Vdp1Regs->userclipX1;
   else
      vdp1clipxstart = Vdp1Regs->systemclipX1;

   if (vdp1clipxend < Vdp1Regs->systemclipX2)
      vdp1clipxend = Vdp1Regs->userclipX2;
   else
      vdp1clipxend = Vdp1Regs->systemclipX2;

   if (vdp1clipystart > Vdp1Regs->systemclipY1)
      vdp1clipystart = Vdp1Regs->userclipY1;
   else
      vdp1clipystart = Vdp1Regs->systemclipY1;

   if (vdp1clipyend < Vdp1Regs->systemclipY2)
      vdp1clipyend = Vdp1Regs->userclipY2;
   else
      vdp1clipyend = Vdp1Regs->systemclipY2;
#endif
}

//////////////////////////////////////////////////////////////////////////////

static void PushUserClipping(int mode)
{
   if (mode == 1)
   {
      VDP1LOG("User clipping mode 1 not implemented\n");
      return;
   }

   vdp1clipxstart = Vdp1Regs->userclipX1;
   vdp1clipxend = Vdp1Regs->userclipX2;
   vdp1clipystart = Vdp1Regs->userclipY1;
   vdp1clipyend = Vdp1Regs->userclipY2;

   // This needs work
   if (vdp1clipxstart > Vdp1Regs->systemclipX1)
      vdp1clipxstart = Vdp1Regs->userclipX1;
   else
      vdp1clipxstart = Vdp1Regs->systemclipX1;

   if (vdp1clipxend < Vdp1Regs->systemclipX2)
      vdp1clipxend = Vdp1Regs->userclipX2;
   else
      vdp1clipxend = Vdp1Regs->systemclipX2;

   if (vdp1clipystart > Vdp1Regs->systemclipY1)
      vdp1clipystart = Vdp1Regs->userclipY1;
   else
      vdp1clipystart = Vdp1Regs->systemclipY1;

   if (vdp1clipyend < Vdp1Regs->systemclipY2)
      vdp1clipyend = Vdp1Regs->userclipY2;
   else
      vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

static void PopUserClipping(void)
{
   vdp1clipxstart = Vdp1Regs->systemclipX1;
   vdp1clipxend = Vdp1Regs->systemclipX2;
   vdp1clipystart = Vdp1Regs->systemclipY1;
   vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1SystemClipping(void)
{
   Vdp1Regs->systemclipX1 = 0;
   Vdp1Regs->systemclipY1 = 0;
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

   vdp1clipxstart = Vdp1Regs->systemclipX1;
   vdp1clipxend = Vdp1Regs->systemclipX2;
   vdp1clipystart = Vdp1Regs->systemclipY1;
   vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1LocalCoordinate(void)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2DrawStart(void)
{
}
u32 alphablend(u32 toppixel, u32 bottompixel, u32 alpha)
{

   u8 oldr, oldg, oldb;
   u8 r, g, b;
   u32 oldpixel = bottompixel;

   static int topratio[32] = {
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
   };
   static int bottomratio[32] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
   };

   // separate color components for top and second pixel
   r = (toppixel & 0xFF) * topratio[alpha] >> 5;
   g = ((toppixel >> 8) & 0xFF) * topratio[alpha] >> 5;
   b = ((toppixel >> 16) & 0xFF) * topratio[alpha] >> 5;

#ifdef WORDS_BIGENDIAN
   oldr = ((oldpixel >> 24) & 0xFF) * bottomratio[alpha] >> 5;
   oldg = ((oldpixel >> 16) & 0xFF) * bottomratio[alpha] >> 5;
   oldb = ((oldpixel >> 8) & 0xFF) * bottomratio[alpha] >> 5;
#else
   oldr = (oldpixel & 0xFF) * bottomratio[alpha] >> 5;
   oldg = ((oldpixel >> 8) & 0xFF) * bottomratio[alpha] >> 5;
   oldb = ((oldpixel >> 16) & 0xFF) * bottomratio[alpha] >> 5;
#endif

   // add color components and reform the pixel
   toppixel = ((b + oldb) << 16) | ((g + oldg) << 8) | (r + oldr);

   return toppixel;
}

u32 additive(u32 toppixel, u32 bottompixel)
{

	u16 oldr, oldg, oldb;
	u16 r, g, b;

	r = (toppixel & 0xFF);
	g = ((toppixel >> 8) & 0xFF);
	b = ((toppixel >> 16) & 0xFF);

#ifdef WORDS_BIGENDIAN
	oldr = ((bottompixel >> 24) & 0xFF);
	oldg = ((bottompixel >> 16) & 0xFF);
	oldb = ((bottompixel >> 8) & 0xFF);
#else
	oldr = (bottompixel & 0xFF);
	oldg = ((bottompixel >> 8) & 0xFF);
	oldb = ((bottompixel >> 16) & 0xFF);
#endif

	r = r + oldr;
	g = g + oldg;
	b = b + oldb;

	if(b > 255) b = 255;
	if(g > 255) g = 255;
	if(r > 255) r = 255;

	toppixel = (b << 16) | (g << 8) | r;

	return toppixel;
}

u32 ColorCalc(u32 toppixel, u32 bottompixel, u32 alpha) {
	if(Vdp2Regs->CCCTL & (1 << 8))
		return additive(toppixel, bottompixel);
	else
		return alphablend(toppixel,bottompixel,alpha);	
}

void VIDSoftVdp2DrawEnd(void)
{
	int i;

     // Render VDP2 only
      for (i = 0; i < (vdp2width*vdp2height); i++)
		   dispbuffer[i] = COLSATSTRIPPRIORITY(vdp2framebuffer[i]);

   VIDSoftVdp1SwapFrameBuffer();

#ifdef USE_OPENGL
   glRasterPos2i(0, 0);
   glPixelZoom((float)outputwidth / (float)vdp2width, 0 - ((float)outputheight / (float)vdp2height));
   glDrawPixels(vdp2width, vdp2height, GL_RGBA, GL_UNSIGNED_BYTE, dispbuffer);

#if HAVE_LIBGLUT
   if (msglength > 0) {
	   int LeftX=9;
	   int Width=500;
	   int TxtY=11;
	   int Height=13;

	 glBegin(GL_POLYGON);
        glColor3f(0, 0, 0);
        glVertex2i(LeftX, TxtY);
        glVertex2i(LeftX + Width, TxtY);
        glVertex2i(LeftX + Width, TxtY + Height);
        glVertex2i(LeftX, TxtY + Height);
    glEnd();

      glColor3f(1.0f, 1.0f, 1.0f);
      glRasterPos2i(10, 22);
      for (i = 0; i < msglength; i++) {
         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, message[i]);
      }
      glColor3f(1, 1, 1);
      msglength = 0;
   }
#endif

#endif
   YuiSwapBuffers();
}



void VIDSoftVdp2SetResolution(u16 TVMD)
{
   // This needs some work

   // Horizontal Resolution
   switch (TVMD & 0x7)
   {
      case 0://320
         vdp2width = 640;
         resxratio=2;
         break;
      case 1://352
         vdp2width = 704;
         resxratio=2;
         break;
      case 2: // 640
         vdp2width = 640;
         resxratio=1;
         break;
      case 3: // 704
         vdp2width = 704;
         resxratio=1;
         break;
      case 4://320
         vdp2width = 640;
         resxratio=2;
         break;
      case 5://352
         vdp2width = 704;
         resxratio=2;
         break;
      case 6: // 640
         vdp2width = 640;
         resxratio=1;
         break;
      case 7: // 704
         vdp2width = 704;
         resxratio=1;
         break;
   }

   // Vertical Resolution
   switch ((TVMD >> 4) & 0x3)
   {
      case 0:
         vdp2height = 224;
         break;
      case 1:
         vdp2height = 240;
         break;
      case 2:
         vdp2height = 256;
         break;
      default: break;
   }
   resyratio=1;

   // Check for interlace
   switch ((TVMD >> 6) & 0x3)
   {
      case 3: // Double-density Interlace
         vdp2height *= 2;
         resyratio=2;
         break;
      case 2: // Single-density Interlace
      case 0: // Non-interlace
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority)
{
   nbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority)
{
   nbg1priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority)
{
   nbg2priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority)
{
   nbg3priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority)
{
   rbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftOnScreenDebugMessage(char *string, ...)
{
   va_list arglist;

   va_start(arglist, string);
   vsprintf(message, string, arglist);
   va_end(arglist);
   msglength = strlen(message);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftGetScreenSize(int *width, int *height)
{
   *width = vdp2width;
   *height = vdp2height;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1SwapFrameBuffer(void)
{
   u8 *temp = vdp1frontframebuffer;
   vdp1frontframebuffer = vdp1backframebuffer;
   vdp1backframebuffer = temp;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1EraseFrameBuffer(void)
{   
   int i,i2;
   int w,h;
   int interlaced = 1;

   h = (Vdp1Regs->EWRR & 0x1FF) + 1;

   //doubled if we are double interlaced
   if(((Vdp2Regs->TVMD >> 6) & 0x3) == 3) {
	   h*=2;
	   interlaced = 2;
   }

   if (h > vdp1height*interlaced) h = vdp1height*interlaced;
   w = ((Vdp1Regs->EWRR >> 6) & 0x3F8) + 8;

   //width is in 16 pixel units for hi resolution, rotation 8
   if(vdp1pixelsize == 1)
	   w*=2;
   
   if (w > vdp1width) w = vdp1width;

   for (i2 = (Vdp1Regs->EWLR & 0x1FF); i2 < h; i2++)
   {
      for (i = ((Vdp1Regs->EWLR >> 6) & 0x1F8); i < w; i++)
         ((u16 *)vdp1backframebuffer)[(i2 * vdp1width) + i] = Vdp1Regs->EWDR;
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftGetGlSize(int *width, int *height)
{
#ifdef USE_OPENGL
   *width = outputwidth;
   *height = outputheight;
#else
   *width = vdp2width;
   *height = vdp2height;
#endif
}