/*  Copyright 2003-2006 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2006 Theo Berkau

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

#ifdef HAVE_LIBGL

#include "vidogl.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"

#if defined WORDS_BIGENDIAN
#define SAT2YAB1(alpha,temp)		(alpha | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27)
#else
#define SAT2YAB1(alpha,temp)		(alpha << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)
#endif

#if defined WORDS_BIGENDIAN
#define SAT2YAB2(alpha,dot1,dot2)       ((dot2 & 0xFF << 24) | ((dot2 & 0xFF00) << 8) | ((dot1 & 0xFF) << 8) | alpha)
#else
#define SAT2YAB2(alpha,dot1,dot2)       (alpha << 24 | ((dot1 & 0xFF) << 16) | (dot2 & 0xFF00) | (dot2 & 0xFF))
#endif

#define COLOR_ADDt(b)		(b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)	COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)	(COLOR_ADDb((l >> 24) & 0xFF, r) << 24) | \
				(COLOR_ADDb((l >> 16) & 0xFF, g) << 16) | \
				(COLOR_ADDb((l >> 8) & 0xFF, b) << 8) | \
				(l & 0xFF)
#else
#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
				(COLOR_ADDb((l >> 8 ) & 0xFF, g) << 8) | \
				(COLOR_ADDb((l >> 16 ) & 0xFF, b) << 16) | \
				(l & 0xFF000000)
#endif

int VIDOGLInit(void);
void VIDOGLDeInit(void);
void VIDOGLResize(unsigned int, unsigned int, int);
int VIDOGLIsFullscreen(void);
int VIDOGLVdp1Reset(void);
void VIDOGLVdp1DrawStart(void);
void VIDOGLVdp1DrawEnd(void);
void VIDOGLVdp1NormalSpriteDraw(void);
void VIDOGLVdp1ScaledSpriteDraw(void);
void VIDOGLVdp1DistortedSpriteDraw(void);
void VIDOGLVdp1PolygonDraw(void);
void VIDOGLVdp1PolylineDraw(void);
void VIDOGLVdp1LineDraw(void);
void VIDOGLVdp1UserClipping(void);
void VIDOGLVdp1SystemClipping(void);
void VIDOGLVdp1LocalCoordinate(void);
int VIDOGLVdp2Reset(void);
void VIDOGLVdp2DrawStart(void);
void VIDOGLVdp2DrawEnd(void);
void VIDOGLVdp2DrawScreens(void);
void VIDOGLVdp2SetResolution(u16 TVMD);
void FASTCALL VIDOGLVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDOGLVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDOGLVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDOGLVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDOGLVdp2SetPriorityRBG0(int priority);
void VIDOGLVdp2ToggleDisplayNBG0(void);
void VIDOGLVdp2ToggleDisplayNBG1(void);
void VIDOGLVdp2ToggleDisplayNBG2(void);
void VIDOGLVdp2ToggleDisplayNBG3(void);
void VIDOGLVdp2ToggleDisplayRBG0(void);

VideoInterface_struct VIDOGL = {
VIDCORE_OGL,
"OpenGL Video Interface",
VIDOGLInit,
VIDOGLDeInit,
VIDOGLResize,
VIDOGLIsFullscreen,
VIDOGLVdp1Reset,
VIDOGLVdp1DrawStart,
VIDOGLVdp1DrawEnd,
VIDOGLVdp1NormalSpriteDraw,
VIDOGLVdp1ScaledSpriteDraw,
VIDOGLVdp1DistortedSpriteDraw,
VIDOGLVdp1PolygonDraw,
VIDOGLVdp1PolylineDraw,
VIDOGLVdp1LineDraw,
VIDOGLVdp1UserClipping,
VIDOGLVdp1SystemClipping,
VIDOGLVdp1LocalCoordinate,
VIDOGLVdp2Reset,
VIDOGLVdp2DrawStart,
VIDOGLVdp2DrawEnd,
VIDOGLVdp2DrawScreens,
VIDOGLVdp2SetResolution,
VIDOGLVdp2SetPriorityNBG0,
VIDOGLVdp2SetPriorityNBG1,
VIDOGLVdp2SetPriorityNBG2,
VIDOGLVdp2SetPriorityNBG3,
VIDOGLVdp2SetPriorityRBG0,
VIDOGLVdp2ToggleDisplayNBG0,
VIDOGLVdp2ToggleDisplayNBG1,
VIDOGLVdp2ToggleDisplayNBG2,
VIDOGLVdp2ToggleDisplayNBG3,
VIDOGLVdp2ToggleDisplayRBG0,
YglOnScreenDebugMessage
};

static float vdp1wratio=1;
static float vdp1hratio=1;
static int vdp1cor=0;
static int vdp1cog=0;
static int vdp1cob=0;

static int vdp2width;
static int vdp2height;
static int vdp2disptoggle=0xFF;
static int nbg0priority=0;
static int nbg1priority=0;
static int nbg2priority=0;
static int nbg3priority=0;
static int rbg0priority=0;

typedef struct
{
   int vertices[8];
   int cellw, cellh;
   int flipfunction;
   int priority;

   int mapwh;
   int planew, planeh;
   int pagewh;
   int patternwh;
   int patterndatasize;
   int specialfunction;
   u32 addr, charaddr, paladdr;
   int colornumber;
   int isbitmap;
   u16 supplementdata;
   int auxmode;
   int enable;
   int x, y;
   int alpha;
   int coloroffset;
   int transparencyenable;
   int specialprimode;

   int cor;
   int cog;
   int cob;

   float coordincx, coordincy;
   void FASTCALL (* PlaneAddr)(void *, int);
   u32 FASTCALL (* PostPixelFetchCalc)(void *, u32);
   int patternpixelwh;
   int draww;
   int drawh;
} vdp2draw_struct;

typedef struct
{
   float Xst;
   float Yst;
   float Zst;
   float deltaXst;
   float deltaYst;
   float deltaX;
   float deltaY;
   float A;
   float B;
   float C;
   float D;
   float E;
   float F;
   s32 Px;
   s32 Py;
   s32 Pz;
   s32 Cx;
   s32 Cy;
   s32 Cz;
   float Mx;
   float My;
   float kx;
   float ky;
   float KAst;
   float deltaKAst;
   float deltaKAx;
   int coefenab;
} vdp2rotationparameter_struct;

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha);

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture)
{
   u32 charAddr = cmd->CMDSRCA * 8;
   u32 dot;
   u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);
   u32 alpha = 0xFF;
   VDP1LOG("Making new sprite %08X\n", charAddr);

   switch(cmd->CMDPMOD & 0x7)
   {
      case 0:
         alpha = 0xFF;
         break;
      case 3:
         alpha = 0x80;
         break;
      default:
         VDP1LOG("unimplemented color calculation: %X\n", (cmd->CMDPMOD & 0x7));
         break;
   }

   switch((cmd->CMDPMOD >> 3) & 0x7)
   {
      case 0:
      {
         // 4 bpp Bank mode
         u32 colorBank = cmd->CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i;

         for(i = 0;i < sprite->h;i++)
         {
            u16 j;
            j = 0;
            while(j < sprite->w)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

               // Pixel 1
               if (((dot >> 4) == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor(((dot >> 4) | colorBank) + colorOffset, alpha), vdp1cor, vdp1cog, vdp1cob);
               j += 1;

               // Pixel 2
               if (((dot & 0xF) == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor(((dot & 0xF) | colorBank) + colorOffset, alpha), vdp1cor, vdp1cog, vdp1cob);
               j += 1;

               charAddr += 1;
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 1:
      {
         // 4 bpp LUT mode
         u32 temp;
         u32 colorLut = cmd->CMDCOLR * 8;
         u16 i;

         for(i = 0;i < sprite->h;i++)
         {
            u16 j;
            j = 0;
            while(j < sprite->w)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

               if (((dot >> 4) == 0) && !SPD)
                  *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else
               {
                  temp = T1ReadWord(Vdp1Ram, ((dot >> 4) * 2 + colorLut) & 0x7FFFF);
                  if (temp & 0x8000)
                     *texture->textdata++ = COLOR_ADD(SAT2YAB1(alpha, temp), vdp1cor, vdp1cog, vdp1cob);
                  else
                     *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor(temp, alpha), vdp1cor, vdp1cog, vdp1cob);
               }

               j += 1;

               if (((dot & 0xF) == 0) && !SPD)
                  *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else
               {
                  temp = T1ReadWord(Vdp1Ram, ((dot & 0xF) * 2 + colorLut) & 0x7FFFF);
                  if (temp & 0x8000)
                     *texture->textdata++ = COLOR_ADD(SAT2YAB1(alpha, temp), vdp1cor, vdp1cog, vdp1cob);
                  else
                     *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor(temp, alpha), vdp1cor, vdp1cog, vdp1cob);                     
               }

               j += 1;

               charAddr += 1;
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 2:
      {
         // 8 bpp(64 color) Bank mode
         u32 colorBank = cmd->CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;

         u16 i, j;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x3F;
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor((dot | colorBank) + colorOffset, alpha), vdp1cor, vdp1cog, vdp1cob);
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 3:
      {
         // 8 bpp(128 color) Bank mode
         u32 colorBank = cmd->CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF) & 0x7F;
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor((dot | colorBank) + colorOffset, alpha), vdp1cor, vdp1cog, vdp1cob);
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 4:
      {
         // 8 bpp(256 color) Bank mode
         u32 colorBank = cmd->CMDCOLR;
         u32 colorOffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(Vdp2ColorRamGetColor((dot | colorBank) + colorOffset, alpha), vdp1cor, vdp1cog, vdp1cob);
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 5:
      {
         // 16 bpp Bank mode
         u16 i, j;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadWord(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr += 2;

               //if (!(dot & 0x8000) && (Vdp2Regs->SPCTL & 0x20)) printf("mixed mode\n");
               if ((dot == 0) && !SPD) *texture->textdata++ = COLOR_ADD(0, vdp1cor, vdp1cog, vdp1cob);
               else *texture->textdata++ = COLOR_ADD(SAT2YAB1(alpha, dot), vdp1cor, vdp1cog, vdp1cob);
            }
            texture->textdata += texture->w;
         }
         break;
      }
      default:
         VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, YglSprite *sprite)
{
   u8 SPCLMD = Vdp2Regs->SPCTL;
   u8 sprite_register;
   u8 *sprprilist = (u8 *)&Vdp2Regs->PRISA;
   u16 lutPri;
   u16 *reg_src = &cmd->CMDCOLR;
   int not_lut = 1;

   // is the sprite is RGB or LUT (in fact, LUT can use bank color, we just hope it won't...)
   if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000))
   {
      // RGB data, use register 0
      sprite->priority = Vdp2Regs->PRISA & 0x7;
      return;
   }

   if (((cmd->CMDPMOD >> 3) & 0x7) == 1) {
      u32 charAddr, dot, colorLut;

      sprite->priority = Vdp2Regs->PRISA & 0x7;

      charAddr = cmd->CMDSRCA * 8;
      dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
      colorLut = cmd->CMDCOLR * 8;
      lutPri = T1ReadWord(Vdp1Ram, (dot >> 4) * 2 + colorLut);
      if (!(lutPri & 0x8000)) {
         not_lut = 0;
         reg_src = &lutPri;
      } else
         return;
   }

   {
      u8 sprite_type = SPCLMD & 0xF;
      switch(sprite_type)
      {
         case 0:
            sprite_register = (*reg_src & 0xC000) >> 14;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 1:
            sprite_register = (*reg_src & 0xE000) >> 13;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 2:
            sprite_register = (*reg_src >> 14) & 0x1;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 3:
            sprite_register = (*reg_src & 0x6000) >> 13;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 4:
            sprite_register = (*reg_src & 0x6000) >> 13;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x3FF;
            break;
         case 5:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 6:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x3FF;
            break;
         case 7:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            sprite->priority = sprprilist[sprite_register^1] & 0x7;
#else
            sprite->priority = sprprilist[sprite_register] & 0x7;
#endif
            if (not_lut) cmd->CMDCOLR &= 0x1FF;
            break;
         default:
            VDP1LOG("sprite type %d not implemented\n", sprite_type);

            // if we don't know what to do with a sprite, we put it on top
            sprite->priority = 7;
            break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp1SetTextureRatio(int vdp2widthratio, int vdp2heightratio)
{
   float vdp1w=1;
   float vdp1h=1;

   // may need some tweaking  

   // Figure out which vdp1 screen mode to use
   switch (Vdp1Regs->TVMR & 7)
   {
      case 0:
      case 2:
      case 3:
          vdp1w=1;
          break;
      case 1:
          vdp1w=2;
          break;
      default:
          vdp1w=1;
          vdp1h=1;
          break;
   }

   // Is double-interlace enabled?
   if (Vdp1Regs->FBCR & 0x8)
      vdp1h=2;

   vdp1wratio = (float)vdp2widthratio / vdp1w;
   vdp1hratio = (float)vdp2heightratio / vdp1h;
}

//////////////////////////////////////////////////////////////////////////////

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha)
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      case 1:
      {
         u32 tmp;
         colorindex <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
         return SAT2YAB1(alpha, tmp);
      }
      case 2:
      {
         u32 tmp1, tmp2;
         colorindex <<= 2;
         colorindex &= 0xFFF;
         tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
         tmp2 = T2ReadWord(Vdp2ColorRam, colorindex+2);
         return SAT2YAB2(alpha, tmp1, tmp2);
      }
      default: break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2DrawCell(vdp2draw_struct *info, YglTexture *texture)
{
   u32 color;
   int i, j;

   switch(info->colornumber)
   {
      case 0: // 4 BPP
         for(i = 0;i < info->cellh;i++)
         {
            for(j = 0;j < info->cellw;j+=4)
            {
               u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);

               info->charaddr += 2;
               if (!(dot & 0xF000) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF000) >> 12)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);
               if (!(dot & 0xF00) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF00) >> 8)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);
               if (!(dot & 0xF0) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF0) >> 4)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);
               if (!(dot & 0xF) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);               
            }
            texture->textdata += texture->w;
         }
         break;
      case 1: // 8 BPP
         for(i = 0;i < info->cellh;i++)
         {
            for(j = 0;j < info->cellw;j+=2)
            {
               u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
               
               info->charaddr += 2;
               if (!(dot & 0xFF00) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | ((dot & 0xFF00) >> 8)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);
               if (!(dot & 0xFF) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF)), info->alpha);
               *texture->textdata++ = info->PostPixelFetchCalc(info, color);
            }
            texture->textdata += texture->w;
         }
         break;
    case 2: // 16 BPP(palette)
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          if ((dot == 0) && info->transparencyenable) color = 0x00000000;
          else color = Vdp2ColorRamGetColor(info->coloroffset + dot, info->alpha);
          info->charaddr += 2;
          *texture->textdata++ = info->PostPixelFetchCalc(info, color);
	}
        texture->textdata += texture->w;
      }
      break;
    case 3: // 16 BPP(RGB)
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
	  else color = SAT2YAB1(0xFF, dot);
          *texture->textdata++ = info->PostPixelFetchCalc(info, color);
	}
        texture->textdata += texture->w;
      }
      break;
    case 4: // 32 BPP
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot1, dot2;
          dot1 = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          dot2 = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
          else color = SAT2YAB2(info->alpha, dot1, dot2);
          *texture->textdata++ = info->PostPixelFetchCalc(info, color);
	}
        texture->textdata += texture->w;
      }
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPattern(vdp2draw_struct *info, YglTexture *texture)
{
   u32 cacheaddr = ((u32) (info->alpha >> 3) << 27) | (info->paladdr << 20) | info->charaddr;
   int * c;
   YglSprite tile;

   tile.w = tile.h = info->patternpixelwh;   
   tile.flip = info->flipfunction;

   if (info->specialprimode == 1)
      tile.priority = (info->priority & 0xFFFFFFFE) | info->specialfunction;
   else
      tile.priority = info->priority;
   tile.vertices[0] = info->x * info->coordincx;
   tile.vertices[1] = info->y * info->coordincy;
   tile.vertices[2] = (info->x + tile.w) * info->coordincx;
   tile.vertices[3] = info->y * info->coordincy;
   tile.vertices[4] = (info->x + tile.w) * info->coordincx;
   tile.vertices[5] = (info->y + tile.h) * info->coordincy;
   tile.vertices[6] = info->x * info->coordincx;
   tile.vertices[7] = (info->y + tile.h) * info->coordincy;

   if ((c = YglIsCached(cacheaddr)) != NULL)
   {
      YglCachedQuad(&tile, c);

      info->x += tile.w;
      info->y += tile.h;
      return;
   }

   c = YglQuad(&tile, texture);
   YglCache(cacheaddr, c);

   switch(info->patternwh)
   {
      case 1:
         Vdp2DrawCell(info, texture);
         break;
      case 2:
         texture->w += 8;
         Vdp2DrawCell(info, texture);
         texture->textdata -= (texture->w + 8) * 8 - 8;
         Vdp2DrawCell(info, texture);
         texture->textdata -= 8;
         Vdp2DrawCell(info, texture);
         texture->textdata -= (texture->w + 8) * 8 - 8;
         Vdp2DrawCell(info, texture);
         break;
   }
   info->x += tile.w;
   info->y += tile.h;
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2PatternAddr(vdp2draw_struct *info)
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
               info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 8;
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
         info->paladdr = (tmp1 & 0x7F);
         info->specialfunction = (tmp1 & 0x2000) >> 13;
         break;
      }
   }

   if (!(Vdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // thanks Runik
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPage(vdp2draw_struct *info, YglTexture *texture)
{
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->pagewh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->pagewh;j++)
      {
         info->y = Y;
         if ((info->x >= -info->patternpixelwh) &&
             (info->y >= -info->patternpixelwh) &&
             (info->x <= info->draww) &&
             (info->y <= info->drawh))
         {
            Vdp2PatternAddr(info);
            Vdp2DrawPattern(info, texture);
         }
         else
         {
            info->addr += info->patterndatasize * 2;
            info->x += info->patternpixelwh;
            info->y += info->patternpixelwh;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPlane(vdp2draw_struct *info, YglTexture *texture)
{
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->planeh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->planew;j++)
      {
         info->y = Y;
         Vdp2DrawPage(info, texture);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawMap(vdp2draw_struct *info, YglTexture *texture)
{
   int i, j;
   int X, Y;
   X = info->x;

   info->patternpixelwh = 8 * info->patternwh;
   info->draww = (int)((float)vdp2width / info->coordincx);
   info->drawh = (int)((float)vdp2height / info->coordincy);

   for(i = 0;i < info->mapwh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->mapwh;j++)
      {
         info->y = Y;
         info->PlaneAddr(info, info->mapwh * i + j);
         Vdp2DrawPlane(info, texture);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2ReadRotationTable(int which, vdp2rotationparameter_struct *parameter)
{
   s32 i;
   u32 addr;

   addr = Vdp2Regs->RPTA.all << 1;

   if (which == 0)
   {
      // Rotation Parameter A
      addr &= 0x000FFF7C;
      parameter->coefenab = Vdp2Regs->KTCTL & 0x1;
   }
   else
   {
      // Rotation Parameter B
      addr = (addr & 0x000FFFFC) | 0x00000080;
      parameter->coefenab = Vdp2Regs->KTCTL & 0x100;
   }

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Xst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Yst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Zst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaXst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaYst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaX = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaY = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->A = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->B = (float) (signed) ((i & 0x000FFFC0) | ((i & 0x00080000) ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->C = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->D = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->E = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->F = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Px = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Py = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Pz = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 4;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cx = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cy = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cz = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Mx = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->My = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->kx = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->ky = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   if (parameter->coefenab)
   {
      // Handle coefficients here
   }
}

//////////////////////////////////////////////////////////////////////////////

static void SetSaturnResolution(int width, int height)
{
   YglChangeResolution(width, height);

   vdp2width=width;
   vdp2height=height;
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLInit(void)
{
   if (YglInit(1024, 1024, 8) != 0)
      return -1;

   SetSaturnResolution(320, 224);

   vdp1wratio = 1;
   vdp1hratio = 1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLDeInit(void)
{
   YglDeInit();
}

//////////////////////////////////////////////////////////////////////////////

int _VIDOGLIsFullscreen;

void VIDOGLResize(unsigned int w, unsigned int h, int on)
{
   glDeleteTextures(1, &_Ygl->texture);

   _VIDOGLIsFullscreen = on;

   if (on)
      YuiSetVideoMode(w, h, 32, 1);
   else
      YuiSetVideoMode(w, h, 32, 0);

   YglGLInit(1024, 1024);
   glViewport(0, 0, w, h);

   SetSaturnResolution(vdp2width, vdp2height);
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLIsFullscreen(void) {
   return _VIDOGLIsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1DrawStart(void)
{
   YglCacheReset();

   if (Vdp2Regs->CLOFEN & 0x40)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x40)
      {
         // color offset B
         vdp1cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            vdp1cor |= 0xFFFFFF00;

         vdp1cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            vdp1cog |= 0xFFFFFF00;

         vdp1cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            vdp1cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         vdp1cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            vdp1cor |= 0xFFFFFF00;

         vdp1cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            vdp1cog |= 0xFFFFFF00;

         vdp1cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            vdp1cob |= 0xFFFFFF00;
      }
   }
   else // color offset disable
      vdp1cor = vdp1cog = vdp1cob = 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1NormalSpriteDraw(void)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   int * c;
   u32 tmp;
   s16 x, y;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   x = cmd.CMDXA + Vdp1Regs->localX;
   y = cmd.CMDYA + Vdp1Regs->localY;
   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;

   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   sprite.vertices[0] = (int)((float)x * vdp1wratio);
   sprite.vertices[1] = (int)((float)y * vdp1hratio);
   sprite.vertices[2] = (int)((float)(x + sprite.w) * vdp1wratio);
   sprite.vertices[3] = (int)((float)y * vdp1hratio);
   sprite.vertices[4] = (int)((float)(x + sprite.w) * vdp1wratio);
   sprite.vertices[5] = (int)((float)(y + sprite.h) * vdp1hratio);
   sprite.vertices[6] = (int)((float)x * vdp1wratio);
   sprite.vertices[7] = (int)((float)(y + sprite.h) * vdp1hratio);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   Vdp1ReadPriority(&cmd, &sprite);

   if (sprite.w > 0 && sprite.h > 1)
   {
      if ((c = YglIsCached(tmp)) != NULL)
      {
         YglCachedQuad(&sprite, c);
         return;
      } 

      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1ScaledSpriteDraw(void)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   int * c;
   u32 tmp;
   s16 rw=0, rh=0;
   s16 x, y;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   x = cmd.CMDXA + Vdp1Regs->localX;
   y = cmd.CMDYA + Vdp1Regs->localY;
   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;
   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   // Setup Zoom Point
   switch ((cmd.CMDCTRL & 0xF00) >> 8)
   {
      case 0x0: // Only two coordinates
         rw = cmd.CMDXC - x + Vdp1Regs->localX + 1;
         rh = cmd.CMDYC - y + Vdp1Regs->localY + 1;
         break;
      case 0x5: // Upper-left
         rw = cmd.CMDXB + 1;
         rh = cmd.CMDYB + 1;
         break;
      case 0x6: // Upper-Center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         rw++;
         rh++;
         break;
      case 0x7: // Upper-Right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         rw++;
         rh++;
         break;
      case 0x9: // Center-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xA: // Center-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xB: // Center-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xD: // Lower-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh;
         rw++;
         rh++;
         break;
      case 0xE: // Lower-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh;
         rw++;
         rh++;
         break;
      case 0xF: // Lower-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh;
         rw++;
         rh++;
         break;
      default: break;
   }

   sprite.vertices[0] = (int)((float)x * vdp1wratio);
   sprite.vertices[1] = (int)((float)y * vdp1hratio);
   sprite.vertices[2] = (int)((float)(x + rw) * vdp1wratio);
   sprite.vertices[3] = (int)((float)y * vdp1hratio);
   sprite.vertices[4] = (int)((float)(x + rw) * vdp1wratio);
   sprite.vertices[5] = (int)((float)(y + rh) * vdp1hratio);
   sprite.vertices[6] = (int)((float)x * vdp1wratio);
   sprite.vertices[7] = (int)((float)(y + rh) * vdp1hratio);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   Vdp1ReadPriority(&cmd, &sprite);

   if (sprite.w > 0 && sprite.h > 1)
   {
      if ((c = YglIsCached(tmp)) != NULL)
      {
         YglCachedQuad(&sprite, c);
         return;
      } 

      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1DistortedSpriteDraw(void)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   int * c;
   u32 tmp;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;
	
   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   sprite.vertices[0] = (s32)((float)(cmd.CMDXA + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[1] = (s32)((float)(cmd.CMDYA + Vdp1Regs->localY) * vdp1hratio);
   sprite.vertices[2] = (s32)((float)((cmd.CMDXB + 1) + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[3] = (s32)((float)(cmd.CMDYB + Vdp1Regs->localY) * vdp1hratio);
   sprite.vertices[4] = (s32)((float)((cmd.CMDXC + 1) + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[5] = (s32)((float)((cmd.CMDYC + 1) + Vdp1Regs->localY) * vdp1hratio);
   sprite.vertices[6] = (s32)((float)(cmd.CMDXD + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[7] = (s32)((float)((cmd.CMDYD + 1) + Vdp1Regs->localY) * vdp1hratio);

   tmp = cmd.CMDSRCA;

   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   Vdp1ReadPriority(&cmd, &sprite);

   if (sprite.w > 0 && sprite.h > 1)
   {
      if ((c = YglIsCached(tmp)) != NULL)
      {
         YglCachedQuad(&sprite, c);
         return;
      } 

      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1PolygonDraw(void)
{
   s16 X[4];
   s16 Y[4];
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   YglSprite polygon;
   YglTexture texture;

   X[0] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Y[0] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   X[1] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10);
   Y[1] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12);
   X[2] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Y[2] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
   X[3] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18);
   Y[3] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A);

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);

   alpha = 0xFF;

   if ((CMDPMOD & 0x7) == 0x3)
      alpha = 0x80;

   if (color == 0)
      alpha = 0;

   polygon.priority = Vdp2Regs->PRISA & 0x7;

   polygon.vertices[0] = (int)((float)X[0] * vdp1wratio);
   polygon.vertices[1] = (int)((float)Y[0] * vdp1hratio);
   polygon.vertices[2] = (int)((float)X[1] * vdp1wratio);
   polygon.vertices[3] = (int)((float)Y[1] * vdp1hratio);
   polygon.vertices[4] = (int)((float)X[2] * vdp1wratio);
   polygon.vertices[5] = (int)((float)Y[2] * vdp1hratio);
   polygon.vertices[6] = (int)((float)X[3] * vdp1wratio);
   polygon.vertices[7] = (int)((float)Y[3] * vdp1hratio);

   polygon.w = 1;
   polygon.h = 1;
   polygon.flip = 0;

   YglQuad(&polygon, &texture);

   if (color & 0x8000)
      *texture.textdata = COLOR_ADD(SAT2YAB1(alpha,color), vdp1cor, vdp1cog, vdp1cob);
   else
      *texture.textdata = COLOR_ADD(Vdp2ColorRamGetColor(color, alpha), vdp1cor, vdp1cog, vdp1cob);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1PolylineDraw(void)
{
   s16 X[4];
   s16 Y[4];
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   s32 priority;

   X[0] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C) );
   Y[0] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E) );
   X[1] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10) );
   Y[1] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12) );
   X[2] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14) );
   Y[2] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16) );
   X[3] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18) );
   Y[3] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A) );

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);

   alpha = 0xFF;
   if ((CMDPMOD & 0x7) == 0x3)
      alpha = 0x80;

   if ((color & 0x8000) == 0)
      alpha = 0;

   priority = Vdp2Regs->PRISA & 0x7;

/*
   glColor4ub(((color & 0x1F) << 3), ((color & 0x3E0) >> 2), ((color & 0x7C00) >> 7), alpha);
   glBegin(GL_LINE_STRIP);
   glVertex2i(X[0], Y[0]);
   glVertex2i(X[1], Y[1]);
   glVertex2i(X[2], Y[2]);
   glVertex2i(X[3], Y[3]);
   glVertex2i(X[0], Y[0]);
   glEnd();
   glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
*/
   
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LineDraw(void)
{
   s16 X[2];
   s16 Y[2];
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   s32 priority;

   X[0] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
   Y[0] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
   X[1] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
   Y[1] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);

   alpha = 0xFF;

   if ((CMDPMOD & 0x7) == 0x3)
      alpha = 0x80;

   if ((color & 0x8000) == 0)
      alpha = 0;

   priority = Vdp2Regs->PRISA & 0x7;

/*
   glColor4ub(((color & 0x1F) << 3), ((color & 0x3E0) >> 2), ((color & 0x7C00) >> 7), alpha);
   glBegin(GL_LINES);
   glVertex2i(X[0], Y[0]);
   glVertex2i(X[1], Y[1]);
   glEnd();
   glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
*/
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1UserClipping(void)
{
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1SystemClipping(void)
{
   Vdp1Regs->systemclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->systemclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LocalCoordinate(void)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawStart(void)
{
   YglReset();
   YglCacheReset();
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawEnd(void)
{
   YglRender();
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoNothing(void *info, u32 pixel)
{
   return pixel;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
    return COLOR_ADD(pixel, ((vdp2draw_struct *)info)->cor,
                     ((vdp2draw_struct *)info)->cog,
                     ((vdp2draw_struct *)info)->cob);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawBackScreen(void)
{
   u32 scrAddr;
   int dot, y;

   if (Vdp2Regs->VRSIZE & 0x8000)
      scrAddr = (((Vdp2Regs->BKTAU & 0x7) << 16) | Vdp2Regs->BKTAL) * 2;
   else
      scrAddr = (((Vdp2Regs->BKTAU & 0x3) << 16) | Vdp2Regs->BKTAL) * 2;

   if (Vdp2Regs->BKTAU & 0x8000)
   {
      glBegin(GL_LINES);

      for(y = 0; y < vdp2height; y++)
      {
         dot = T1ReadWord(Vdp2Ram, scrAddr);
         scrAddr += 2;

         glColor3ub((dot & 0x1F) << 3, (dot & 0x3E0) >> 2, (dot & 0x7C00) >> 7);

         glVertex2f(0, y);
         glVertex2f(vdp2width, y);
      }
      glEnd();
      glColor3ub(0xFF, 0xFF, 0xFF);
   }
   else
   {
      dot = T1ReadWord(Vdp2Ram, scrAddr);

      glColor3ub((dot & 0x1F) << 3, (dot & 0x3E0) >> 2, (dot & 0x7C00) >> 7);

      glBegin(GL_QUADS);
      glVertex2i(0, 0);
      glVertex2i(vdp2width, 0);
      glVertex2i(vdp2width, vdp2height);
      glVertex2i(0, vdp2height);
      glEnd();
      glColor3ub(0xFF, 0xFF, 0xFF);
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawLineColorScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG0PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x7) << 6;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN0 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN0 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN0 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN0 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;
   YglTexture texture;
   int *tmp;
   int i, i2;
   u32 linescrolladdr;

   /* FIXME should start by checking if it's a normal
    * or rotate scroll screen
    */
   info.enable = Vdp2Regs->BGON & 0x1;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);
   info.specialprimode = Vdp2Regs->SFPRMD & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x70) >> 4;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
   {
      // Bitmap Mode

      switch((Vdp2Regs->CHCTLA & 0xC) >> 2)
      {
         case 0: info.cellw = 512;
                 info.cellh = 256;
                 break;
         case 1: info.cellw = 512;
                 info.cellh = 512;
                 break;
         case 2: info.cellw = 1024;
                 info.cellh = 256;
                 break;
         case 3: info.cellw = 1024;
                 info.cellh = 512;
                 break;
      }

      info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
      info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % info.cellh);

      info.charaddr = (Vdp2Regs->MPOFN & 0x7) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      // Tile Mode
      info.mapwh = 2;

      switch(Vdp2Regs->PLSZ & 0x3)
      {
         case 0:
            info.planew = info.planeh = 1;
            break;
         case 1:
            info.planew = 2;
            info.planeh = 1;
            break;
         case 3:
            info.planew = info.planeh = 2;
            break;
         default: // Not sure what 0x2 does
            info.planew = info.planeh = 1;
            break;
      }

      info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
      info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));

      if(Vdp2Regs->PNCN0 & 0x8000)
         info.patterndatasize = 1;
      else
         info.patterndatasize = 2;

      if(Vdp2Regs->CHCTLA & 0x1)
         info.patternwh = 2;
      else
         info.patternwh = 1;

      info.pagewh = 64/info.patternwh;
      info.cellw = info.cellh = 8;
      info.supplementdata = Vdp2Regs->PNCN0 & 0x3FF;
      info.auxmode = (Vdp2Regs->PNCN0 & 0x4000) >> 14;
   }

   if (Vdp2Regs->CCCTL & 0x1)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7) << 8;

   if (Vdp2Regs->CLOFEN & 0x1)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x1)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
      info.PostPixelFetchCalc = &DoNothing;

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN0.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMYN0.all & 0x7FF00);

   info.priority = nbg0priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG0PlaneAddr;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   if (info.isbitmap)
   {
      // Let's check to see if game has set up bitmap to a size smaller than
      // 512x256 using line scroll and vertical cell scroll
      if (Vdp2Regs->SCRCTL & 0x7 == 0x7)
      {
         linescrolladdr = (Vdp2Regs->LSTA0.all & 0x7FFFE) << 1;

         // Let's first figure out if we have to adjust the vertical offset
         for(i = 0; i < (info.cellh-1); i++)
         {
            if (T1ReadWord(Vdp2Ram, linescrolladdr+((i+1) * 8)) != 0x0000)
            {
               info.y+=i;

               // Now let's figure out the height of the bitmap
               for (i2 = i+1; i2 < info.cellh; i2++)
               {
                  if (T1ReadLong(Vdp2Ram,linescrolladdr+((i2+1)*8) + 4) == 0)
                     break;
               }

               info.cellh = i2-i+1;

               break;
            }
         }

         // Now let's fetch line 1's line scroll horizontal value, that should
         // be the same as the bitmap width
         info.cellw = T1ReadWord(Vdp2Ram, linescrolladdr+((i+1)*8));
      }

      info.vertices[0] = info.x * info.coordincx;
      info.vertices[1] = info.y * info.coordincy;
      info.vertices[2] = (info.x + info.cellw) * info.coordincx;
      info.vertices[3] = info.y * info.coordincy;
      info.vertices[4] = (info.x + info.cellw) * info.coordincx;
      info.vertices[5] = (info.y + info.cellh) * info.coordincy;
      info.vertices[6] = info.x * info.coordincx;
      info.vertices[7] = (info.y + info.cellh) * info.coordincy;

      tmp = YglQuad((YglSprite *)&info, &texture);
      Vdp2DrawCell(&info, &texture);

      // Handle Scroll Wrapping(Let's see if we even need do to it to begin
      // with)
      if (info.x < (vdp2width - info.cellw))
      {
         info.vertices[0] = (info.x+info.cellw) * info.coordincx;
         info.vertices[2] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[4] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[6] = (info.x+info.cellw) * info.coordincx;

         YglCachedQuad((YglSprite *)&info, tmp);

         if (info.y < (vdp2height - info.cellh))
         {
            info.vertices[1] = (info.y+info.cellh) * info.coordincy;
            info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[7] = (info.y+info.cellh) * info.coordincy;

            YglCachedQuad((YglSprite *)&info, tmp);
         }
      }
      else if (info.y < (vdp2height - info.cellh))
      {
         info.vertices[1] = (info.y+info.cellh) * info.coordincy;
         info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[7] = (info.y+info.cellh) * info.coordincy;

         YglCachedQuad((YglSprite *)&info, tmp);
      }
   }
   else
   {
      Vdp2DrawMap(&info, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG1PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x70) << 2;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN1 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN1 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN1 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN1 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;
   YglTexture texture;
   int *tmp;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 2) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x200) != 0)
   {
      switch((Vdp2Regs->CHCTLA & 0xC00) >> 10)
      {
         case 0: info.cellw = 512;
                 info.cellh = 256;
                 break;
         case 1: info.cellw = 512;
                 info.cellh = 512;
                 break;
         case 2: info.cellw = 1024;
                 info.cellh = 256;
                 break;
         case 3: info.cellw = 1024;
                 info.cellh = 512;
                 break;
      }

      info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
      info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % info.cellh);

      info.charaddr = ((Vdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNA & 0x700) >> 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      info.mapwh = 2;

      switch((Vdp2Regs->PLSZ & 0xC) >> 2)
      {
         case 0:
            info.planew = info.planeh = 1;
            break;
         case 1:
            info.planew = 2;
            info.planeh = 1;
            break;
         case 3:
            info.planew = info.planeh = 2;
            break;
         default: // Not sure what 0x2 does
            info.planew = info.planeh = 1;
            break;
      }

      info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
      info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));

      if(Vdp2Regs->PNCN1 & 0x8000)
         info.patterndatasize = 1;
      else
         info.patterndatasize = 2;

      if(Vdp2Regs->CHCTLA & 0x100)
         info.patternwh = 2;
      else
         info.patternwh = 1;

      info.pagewh = 64/info.patternwh;
      info.cellw = info.cellh = 8;
      info.supplementdata = Vdp2Regs->PNCN1 & 0x3FF;
      info.auxmode = (Vdp2Regs->PNCN1 & 0x4000) >> 14;
   }

   if (Vdp2Regs->CCCTL & 0x2)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) << 4;

   if (Vdp2Regs->CLOFEN & 0x2)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x2)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
      info.PostPixelFetchCalc = &DoNothing;

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);

   info.priority = nbg1priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG1PlaneAddr;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   if (info.isbitmap)
   {
      info.vertices[0] = info.x * info.coordincx;
      info.vertices[1] = info.y * info.coordincy;
      info.vertices[2] = (info.x + info.cellw) * info.coordincx;
      info.vertices[3] = info.y * info.coordincy;
      info.vertices[4] = (info.x + info.cellw) * info.coordincx;
      info.vertices[5] = (info.y + info.cellh) * info.coordincy;
      info.vertices[6] = info.x * info.coordincx;
      info.vertices[7] = (info.y + info.cellh) * info.coordincy;

      tmp = YglQuad((YglSprite *)&info, &texture);
      Vdp2DrawCell(&info, &texture);

      // Handle Scroll Wrapping(Let's see if we even need do to it to begin
      // with)
      if (info.x < (vdp2width - info.cellw))
      {
         info.vertices[0] = (info.x+info.cellw) * info.coordincx;
         info.vertices[2] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[4] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[6] = (info.x+info.cellw) * info.coordincx;

         YglCachedQuad((YglSprite *)&info, tmp);

         if (info.y < (vdp2height - info.cellh))
         {
            info.vertices[1] = (info.y+info.cellh) * info.coordincy;
            info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[7] = (info.y+info.cellh) * info.coordincy;

            YglCachedQuad((YglSprite *)&info, tmp);
         }
      }
      else if (info.y < (vdp2height - info.cellh))
      {
         info.vertices[1] = (info.y+info.cellh) * info.coordincy;
         info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[7] = (info.y+info.cellh) * info.coordincy;

         YglCachedQuad((YglSprite *)&info, tmp);
      }
   }
   else
      Vdp2DrawMap(&info, &texture);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG2PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x700) >> 2;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN2 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN2 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN2 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN2 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG2(void)
{
   vdp2draw_struct info;
   YglTexture texture;

   info.enable = Vdp2Regs->BGON & 0x4;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x400);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 4) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x2) >> 1;	
   info.mapwh = 2;

   switch((Vdp2Regs->PLSZ & 0x30) >> 4)
   {
      case 0:
         info.planew = info.planeh = 1;
         break;
      case 1:
         info.planew = 2;
         info.planeh = 1;
         break;
      case 3:
         info.planew = info.planeh = 2;
         break;
      default: // Not sure what 0x2 does
         info.planew = info.planeh = 1;
         break;
   }
   info.x = - ((Vdp2Regs->SCXN2 & 0x7FF) % (512 * info.planew));
   info.y = - ((Vdp2Regs->SCYN2 & 0x7FF) % (512 * info.planeh));

   if(Vdp2Regs->PNCN2 & 0x8000)
      info.patterndatasize = 1;
   else
      info.patterndatasize = 2;

   if(Vdp2Regs->CHCTLB & 0x1)
      info.patternwh = 2;
   else
      info.patternwh = 1;

   info.pagewh = 64/info.patternwh;
   info.cellw = info.cellh = 8;
   info.supplementdata = Vdp2Regs->PNCN2 & 0x3FF;
   info.auxmode = (Vdp2Regs->PNCN2 & 0x4000) >> 14;
    
   if (Vdp2Regs->CCCTL & 0x4)
      info.alpha = ((~Vdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x700);

   if (Vdp2Regs->CLOFEN & 0x4)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x4)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
      info.PostPixelFetchCalc = &DoNothing;

   info.coordincx = info.coordincy = 1;

   info.priority = nbg2priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG2PlaneAddr;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   Vdp2DrawMap(&info, &texture);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG3PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x7000) >> 6;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN3 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN3 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN3 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN3 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000) {
      if (info->patterndatasize == 1) {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else {
      if (info->patterndatasize == 1) {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG3(void)
{
   vdp2draw_struct info;
   YglTexture texture;

   info.enable = Vdp2Regs->BGON & 0x8;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x800);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 6) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x20) >> 5;
	
   info.mapwh = 2;

   switch((Vdp2Regs->PLSZ & 0xC0) >> 6)
   {
      case 0:
         info.planew = info.planeh = 1;
         break;
      case 1:
         info.planew = 2;
         info.planeh = 1;
         break;
      case 3:
         info.planew = info.planeh = 2;
         break;
      default: // Not sure what 0x2 does
         info.planew = info.planeh = 1;
         break;
   }
   info.x = - ((Vdp2Regs->SCXN3 & 0x7FF) % (512 * info.planew));
   info.y = - ((Vdp2Regs->SCYN3 & 0x7FF) % (512 * info.planeh));

   if(Vdp2Regs->PNCN3 & 0x8000)
      info.patterndatasize = 1;
   else
      info.patterndatasize = 2;

   if(Vdp2Regs->CHCTLB & 0x10)
      info.patternwh = 2;
   else
      info.patternwh = 1;

   info.pagewh = 64/info.patternwh;
   info.cellw = info.cellh = 8;
   info.supplementdata = Vdp2Regs->PNCN3 & 0x3FF;
   info.auxmode = (Vdp2Regs->PNCN3 & 0x4000) >> 14;

   if (Vdp2Regs->CCCTL & 0x8)
      info.alpha = ((~Vdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7000) >> 4;

   if (Vdp2Regs->CLOFEN & 0x8)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x8)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
      info.PostPixelFetchCalc = &DoNothing;

   info.coordincx = info.coordincy = 1;

   info.priority = nbg3priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG3PlaneAddr;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   Vdp2DrawMap(&info, &texture);
}

//////////////////////////////////////////////////////////////////////////////

/*
int Vdp2dRBG0GetX(Vdp2Screen * tmp, int Hcnt, int Vcnt)
{
	RBG0 * screen = (RBG0 *) tmp;
	float ret;
	float Xsp = screen->A * ((screen->Xst + screen->deltaXst * Vcnt) - screen->Px) + screen->B * ((screen->Yst + screen->deltaYst * Vcnt) - screen->Py) + screen->C * (screen->Zst - screen->Pz);
	float Xp = screen->A * (screen->Px - screen->Cx) + screen->B * (screen->Py - screen->Cy) + screen->C * (screen->Pz - screen->Cz); //+ Cx + Mx;
	float dX = screen->A * screen->deltaX + screen->B * screen->deltaY;
	ret = (screen->kx * (Xsp + dX * Hcnt) + Xp);
	return (int) ret;
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2RBG0GetY(Vdp2Screen * tmp, int Hcnt, int Vcnt)
{
	RBG0 * screen = (RBG0 *) tmp;
	float ret;
	float Ysp = screen->D * ((screen->Xst + screen->deltaXst * Vcnt) - screen->Px) + screen->E * ((screen->Yst + screen->deltaYst * Vcnt) - screen->Py) + screen->F * (screen->Zst - screen->Pz);
	float Yp = screen->D * (screen->Px - screen->Cx) + screen->E * (screen->Py - screen->Cy) + screen->F * (screen->Pz - screen->Cz); //+ Cy + My
	float dY = screen->D * screen->deltaX + screen->E * screen->deltaY;
	ret = (screen->ky * (Ysp + dY * Hcnt) + Yp);
	return (int) ret;
}
*/

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2RBG0PlaneAddr(vdp2draw_struct *info, int i)
{
   // works only for parameter A for time being
   u32 offset = (Vdp2Regs->MPOFR & 0x7) << 6;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABRA & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABRA >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDRA & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDRA >> 8);
         break;
      case 4:
         tmp = offset | (Vdp2Regs->MPEFRA & 0xFF);
         break;
      case 5:
         tmp = offset | (Vdp2Regs->MPEFRA >> 8);
         break;
      case 6:
         tmp = offset | (Vdp2Regs->MPGHRA & 0xFF);
         break;
      case 7:
         tmp = offset | (Vdp2Regs->MPGHRA >> 8);
         break;
      case 8:
         tmp = offset | (Vdp2Regs->MPIJRA & 0xFF);
         break;
      case 9:
         tmp = offset | (Vdp2Regs->MPIJRA >> 8);
         break;
      case 10:
         tmp = offset | (Vdp2Regs->MPKLRA & 0xFF);
         break;
      case 11:
         tmp = offset | (Vdp2Regs->MPKLRA >> 8);
         break;
      case 12:
         tmp = offset | (Vdp2Regs->MPMNRA & 0xFF);
         break;
      case 13:
         tmp = offset | (Vdp2Regs->MPMNRA >> 8);
         break;
      case 14:
         tmp = offset | (Vdp2Regs->MPOPRA & 0xFF);
         break;
      case 15:
         tmp = offset | (Vdp2Regs->MPOPRA >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   if (info->patterndatasize == 1)
   {
      if (info->patternwh == 1)
         info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
      else
         info->addr = (tmp >> deca) * (multi * 0x800);
   }
   else
   {
      if (info->patternwh == 1)
         info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
      else
         info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawRBG0(void)
{
   vdp2draw_struct info;
   YglTexture texture;
   vdp2rotationparameter_struct parameter;

   info.enable = Vdp2Regs->BGON & 0x10;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x1000);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 8) & 0x3;

   info.x = 0; // this is obviously wrong
   info.y = 0; // this is obviously wrong

   info.colornumber = (Vdp2Regs->CHCTLB & 0x7000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLB & 0x200) != 0)
   {
      switch((Vdp2Regs->CHCTLB & 0x400) >> 10)
      {
         case 0:
            info.cellw = 512;
            info.cellh = 256;
            break;
         case 1:
            info.cellw = 512;
            info.cellh = 512;
            break;
      }

      info.charaddr = (Vdp2Regs->MPOFR & 0x7) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNB & 0x7) << 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      u8 PlaneSize;
      info.mapwh = 4;

      switch(Vdp2Regs->RPMD & 0x3)
      {
         case 0:
            PlaneSize = (Vdp2Regs->PLSZ & 0x300) >> 8;
            break;
         case 1:
            PlaneSize = (Vdp2Regs->PLSZ & 0x3000) >> 12;
            break;
         default:
#if VDP2_DEBUG
            VDP2LOG("RGB0\t: don't know what to do with plane size\n");
#endif
            PlaneSize = 0;
            break;
      }

      switch(PlaneSize)
      {
         case 0:
            info.planew = info.planeh = 1;
            break;
         case 1:
            info.planew = 2;
            info.planeh = 1;
            break;
         case 2:
            info.planew = info.planeh = 2;
            break;
         default: // Not sure what 0x3 does
            info.planew = info.planeh = 1;
            break;
      }

      if(Vdp2Regs->PNCR & 0x8000)
         info.patterndatasize = 1;
      else
         info.patterndatasize = 2;

      if(Vdp2Regs->CHCTLB & 0x1)
         info.patternwh = 2;
      else
         info.patternwh = 1;

      info.pagewh = 64/info.patternwh;
      info.cellw = info.cellh = 8;
      info.supplementdata = Vdp2Regs->PNCR & 0x3FF;
      info.auxmode = (Vdp2Regs->PNCR & 0x4000) >> 14;
   }

   if (Vdp2Regs->CCCTL & 0x1000)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFB & 0x7) << 8;

   if (Vdp2Regs->CLOFEN & 0x10)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x10)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
      info.PostPixelFetchCalc = &DoNothing;
 
   info.coordincx = info.coordincy = 1;

   info.priority = rbg0priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2RBG0PlaneAddr;

   switch (Vdp2Regs->RPMD & 0x3)
   {
      case 0:
         // Parameter A only
         Vdp2ReadRotationTable(0, &parameter);
         break;
      case 1:
         // Parameter B only
         Vdp2ReadRotationTable(1, &parameter);
         break;
      case 2:
      case 3:
         VDP2LOG("Rotation Parameter Mode %d not supported!\n", Vdp2Regs->RPMD & 0x3);
         memset(&parameter, 0, sizeof(vdp2rotationparameter_struct));
         break;
   }

   if (!parameter.coefenab)
   {
      // Since coefficients aren't being used, we can simplify the drawing process
   }

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   if (info.isbitmap)
   {
      info.vertices[0] = info.x * info.coordincx;
      info.vertices[1] = info.y * info.coordincy;
      info.vertices[2] = (info.x + info.cellw) * info.coordincx;
      info.vertices[3] = info.y * info.coordincy;
      info.vertices[4] = (info.x + info.cellw) * info.coordincx;
      info.vertices[5] = (info.y + info.cellh) * info.coordincy;
      info.vertices[6] = info.x * info.coordincx;
      info.vertices[7] = (info.y + info.cellh) * info.coordincy;

      YglQuad((YglSprite *)&info, &texture);
      Vdp2DrawCell(&info, &texture);
   }
   else
      Vdp2DrawMap(&info, &texture);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawScreens(void)
{
   Vdp2DrawBackScreen();
   Vdp2DrawLineColorScreen();
   Vdp2DrawNBG3();
   Vdp2DrawNBG2();
   Vdp2DrawNBG1();
   Vdp2DrawNBG0();
   Vdp2DrawRBG0();
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2SetResolution(u16 TVMD)
{
   int width=0, height=0;
   int wratio=1, hratio=1;

   // Horizontal Resolution
   switch (TVMD & 0x7)
   {
      case 0:
         width = 320;
         wratio = 1;
         break;
      case 1:
         width = 352;
         wratio = 1;
         break;
      case 2:
         width = 640;
         wratio = 2;
         break;
      case 3:
         width = 704;
         wratio = 2;
         break;
      case 4:
         width = 320;
         wratio = 1;
         break;
      case 5:
         width = 352;
         wratio = 1;
         break;
      case 6:
         width = 640;
         wratio = 2;
         break;
      case 7:
         width = 704;
         wratio = 2;
         break;
   }

   // Vertical Resolution
   switch ((TVMD >> 4) & 0x3)
   {
      case 0:
         height = 224;
         break;
      case 1: height = 240;
                 break;
      case 2: height = 256;
                 break;
      default: break;
   }

   hratio = 1;

   // Check for interlace
   switch ((TVMD >> 6) & 0x3)
   {
      case 2: // Single-density Interlace
      case 3: // Double-density Interlace
         height *= 2;
         hratio = 2;
         break;
      case 0: // Non-interlace
      default: break;
   }

   SetSaturnResolution(width, height);
   Vdp1SetTextureRatio(wratio, hratio);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDOGLVdp2SetPriorityNBG0(int priority)
{
   nbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDOGLVdp2SetPriorityNBG1(int priority)
{
   nbg1priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDOGLVdp2SetPriorityNBG2(int priority)
{
   nbg2priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDOGLVdp2SetPriorityNBG3(int priority)
{
   nbg3priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDOGLVdp2SetPriorityRBG0(int priority)
{
   rbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2ToggleDisplayNBG0(void)
{
   vdp2disptoggle ^= 0x1;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2ToggleDisplayNBG1(void)
{
   vdp2disptoggle ^= 0x2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2ToggleDisplayNBG2(void)
{
   vdp2disptoggle ^= 0x4;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2ToggleDisplayNBG3(void)
{
   vdp2disptoggle ^= 0x8;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2ToggleDisplayRBG0(void)
{
   vdp2disptoggle ^= 0x10;
}

//////////////////////////////////////////////////////////////////////////////

#endif