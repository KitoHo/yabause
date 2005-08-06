/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2005 Theo Berkau

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

#include "vidsdlgl.h"
#include "debug.h"
#include "vdp2.h"
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
#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
				(COLOR_ADDb((l >> 8 ) & 0xFF, g) << 8) | \
				(COLOR_ADDb((l >> 16 ) & 0xFF, b) << 16) | \
				(l & 0xFF000000)

int VIDSDLGLInit(void);
void VIDSDLGLDeInit(void);
void VIDSDLGLResize(unsigned int, unsigned int);
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
void VIDSDLGLVdp2DrawStart(void);
void VIDSDLGLVdp2DrawEnd(void);
void VIDSDLGLVdp2DrawScreens(void);
void FASTCALL VIDSDLGLVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDSDLGLVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDSDLGLVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDSDLGLVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDSDLGLVdp2SetPriorityRBG0(int priority);

VideoInterface_struct VIDSDLGL = {
VIDCORE_SDLGL,
"SDL/OpenGL Video Interface",
VIDSDLGLInit,
VIDSDLGLDeInit,
VIDSDLGLResize,
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
VIDSDLGLVdp2DrawStart,
VIDSDLGLVdp2DrawEnd,
VIDSDLGLVdp2DrawScreens,
VIDSDLGLVdp2SetPriorityNBG0,
VIDSDLGLVdp2SetPriorityNBG1,
VIDSDLGLVdp2SetPriorityNBG2,
VIDSDLGLVdp2SetPriorityNBG3,
VIDSDLGLVdp2SetPriorityRBG0
};

static float vdp1wratio=1;
static float vdp1hratio=1;
static int vdp2width;
static int vdp2height;
int vdp2disptoggle=0xFF;
int nbg0priority=0;
int nbg1priority=0;
int nbg2priority=0;
int nbg3priority=0;
int rbg0priority=0;

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

   int cor;
   int cog;
   int cob;

   float coordincx, coordincy;
} vdp2draw_struct;

u32 Vdp2ColorRamGetColor(u32 addr, int alpha, u32 colorOffset);

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture) {
   u32 charAddr = cmd->CMDSRCA * 8;
   u32 dot;
   u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);
   u32 alpha = 0xFF;
   VDP1LOG("Making new sprite %08X\n", charAddr);

   switch(cmd->CMDPMOD & 0x7) {
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

   switch((cmd->CMDPMOD >> 3) & 0x7) {
      case 0:
      {
         // 4 bpp Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFFF0;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i;

         for(i = 0;i < sprite->h;i++) {
            u16 j;
            j = 0;
            while(j < sprite->w) {
               dot = T1ReadByte(Vdp1Ram, charAddr);

               // Pixel 1
               if (((dot >> 4) == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = Vdp2ColorRamGetColor((dot >> 4) + colorBank, alpha, colorOffset);
               j += 1;

               // Pixel 2
               if (((dot & 0xF) == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = Vdp2ColorRamGetColor((dot & 0xF) + colorBank, alpha, colorOffset);
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

         for(i = 0;i < sprite->h;i++) {
            u16 j;
            j = 0;
            while(j < sprite->w) {
               dot = T1ReadByte(Vdp1Ram, charAddr);

               if (((dot >> 4) == 0) && !SPD) *texture->textdata++ = 0;
               else {
                  temp = T1ReadWord(Vdp1Ram, (dot >> 4) * 2 + colorLut);
                  *texture->textdata++ = SAT2YAB1(alpha, temp);
               }

               j += 1;

               if (((dot & 0xF) == 0) && !SPD) *texture->textdata++ = 0;
               else {
                  temp = T1ReadWord(Vdp1Ram, (dot & 0xF) * 2 + colorLut);
                  *texture->textdata++ = SAT2YAB1(alpha, temp);
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
         u32 colorBank = cmd->CMDCOLR & 0xFFC0;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < sprite->h;i++) {
            for(j = 0;j < sprite->w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr) & 0x3F;               
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = Vdp2ColorRamGetColor(dot + colorBank, alpha, colorOffset);
            }
            texture->textdata += texture->w;
         }

         break;
      }
      case 3:
      {
         // 8 bpp(128 color) Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFF80;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < sprite->h;i++) {
            for(j = 0;j < sprite->w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr) & 0x7F;               
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = Vdp2ColorRamGetColor(dot + colorBank, alpha, colorOffset);
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 4:
      {
         // 8 bpp(256 color) Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFF00;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < sprite->h;i++) {
            for(j = 0;j < sprite->w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr);               
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = Vdp2ColorRamGetColor(dot + colorBank, alpha, colorOffset);
            }
            texture->textdata += texture->w;
         }

         break;
      }
      case 5:
      {
         // 16 bpp Bank mode
         u16 i, j;

         for(i = 0;i < sprite->h;i++) {
            for(j = 0;j < sprite->w;j++) {
               dot = T1ReadWord(Vdp1Ram, charAddr);               
               charAddr += 2;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0;
               else *texture->textdata++ = SAT2YAB1(alpha, dot);
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

void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, YglSprite *sprite) {
   u8 SPCLMD = Vdp2Regs->SPCTL;
   u8 sprite_register;

   // if we don't know what to do with a sprite, we put it on top
   sprite->priority = 7;

   if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000)) {
      // RGB data, use register 0
      sprite->priority = Vdp2Regs->PRISA & 0x7;
   } else {
      u8 sprite_type = SPCLMD & 0xF;
      switch(sprite_type) {
         case 0:
            sprite_register = ((cmd->CMDCOLR & 0x8000) | (~cmd->CMDCOLR & 0x4000)) >> 14;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 1:
            sprite_register = ((cmd->CMDCOLR & 0xC000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 3:
            sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 4:
            sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 5:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 6:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 7:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
//            sprite->priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         default:
            VDP1LOG("sprite type %d not implemented\n", sprite_type);
            break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1SetTextureRatio(int vdp2widthratio, int vdp2heightratio) {
   float vdp1w;
   float vdp1h;

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
   vdp1hratio = (float)vdp2heightratio / vdp1w;
}

//////////////////////////////////////////////////////////////////////////////

u32 Vdp2ColorRamGetColor(u32 addr, int alpha, u32 colorOffset) {
   switch(Vdp2Internal.ColorMode) {
      case 0: {
         u32 tmp;
         addr *= 2; // thanks Runik!
         addr += colorOffset * 0x200;
         tmp = T2ReadWord(Vdp2ColorRam, addr);
         return SAT2YAB1(alpha, tmp);
      }
      case 1: {
         u32 tmp;
         addr *= 2; // thanks Runik!
         addr += colorOffset * 0x200;
         tmp = T2ReadWord(Vdp2ColorRam, addr);
         return SAT2YAB1(alpha, tmp);
      }
      case 2: {
         u32 tmp1, tmp2;
         addr *= 4;
         addr += colorOffset * 0x400;
         tmp1 = T2ReadWord(Vdp2ColorRam, addr);
         tmp2 = T2ReadWord(Vdp2ColorRam, addr+2);
         return SAT2YAB2(alpha, tmp1, tmp2);
      }
      default: break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2DrawCell(vdp2draw_struct *info, YglTexture *texture) {
   u32 color;
   int i, j;

   switch(info->colornumber) {
      case 0:
         for(i = 0;i < info->cellh;i++) {
            for(j = 0;j < info->cellw;j+=4) {
               u16 dot = T1ReadWord(Vdp2Ram, info->charaddr);

               info->charaddr += 2;
               if (!(dot & 0xF000) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | ((dot & 0xF000) >> 12), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
               if (!(dot & 0xF00) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | ((dot & 0xF00) >> 8), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
               if (!(dot & 0xF0) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | ((dot & 0xF0) >> 4), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
               if (!(dot & 0xF) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | (dot & 0xF), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
            }
            texture->textdata += texture->w;
         }
         break;
      case 1:
         for(i = 0;i < info->cellh;i++) {
            for(j = 0;j < info->cellw;j+=2) {
               u16 dot = T1ReadWord(Vdp2Ram, info->charaddr);
               
               info->charaddr += 2;
               if (!(dot & 0xFF00) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | ((dot & 0xFF00) >> 8), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
               if (!(dot & 0xFF) && info->transparencyenable) color = 0x00000000;
               else color = Vdp2ColorRamGetColor((info->paladdr << 4) | (dot & 0xFF), info->alpha, info->coloroffset);
               *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
            }
            texture->textdata += texture->w;
         }
         break;
    case 2:
      for(i = 0;i < info->cellh;i++) {
        for(j = 0;j < info->cellw;j++) {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr);
          if ((dot == 0) && info->transparencyenable) color = 0x00000000;
          else color = Vdp2ColorRamGetColor(dot, info->alpha, info->coloroffset);
          info->charaddr += 2;
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
      break;
    case 3:
      for(i = 0;i < info->cellh;i++) {
        for(j = 0;j < info->cellw;j++) {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr);
          info->charaddr += 2;
          if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
	  else color = SAT2YAB1(0xFF, dot);
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
      break;
    case 4:
      for(i = 0;i < info->cellh;i++) {
        for(j = 0;j < info->cellw;j++) {
          u16 dot1 = T1ReadWord(Vdp2Ram, info->charaddr);
          info->charaddr += 2;
          u16 dot2 = T1ReadWord(Vdp2Ram, info->charaddr);
          info->charaddr += 2;
          if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
          else color = SAT2YAB2(info->alpha, dot1, dot2);
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawPattern(vdp2draw_struct *info, YglTexture *texture) {
   u32 cacheaddr = (info->paladdr << 20) | info->charaddr;
   int * c;
   YglSprite tile;

   tile.w = tile.h = info->patternwh * 8;
   tile.flip = info->flipfunction;
   tile.priority = info->priority;
   tile.vertices[0] = info->x * info->coordincx;
   tile.vertices[1] = info->y * info->coordincy;
   tile.vertices[2] = (info->x + tile.w) * info->coordincx;
   tile.vertices[3] = info->y * info->coordincy;
   tile.vertices[4] = (info->x + tile.w) * info->coordincx;
   tile.vertices[5] = (info->y + tile.h) * info->coordincy;
   tile.vertices[6] = info->x * info->coordincx;
   tile.vertices[7] = (info->y + tile.h) * info->coordincy;

   if ((c = YglIsCached(cacheaddr)) != NULL) {
      YglCachedQuad(&tile, c);

      info->x += tile.w;
      info->y += tile.h;
      return;
   }

   c = YglQuad(&tile, texture);
   YglCache(cacheaddr, c);

/*
   switch(info->patternwh) {
      case 1:
         Vdp2DrawCell(???, texture);
         break;
      case 2:
         texture->w += 8;
         Vdp2DrawCell(???, texture);
         texture->textdata -= (texture->w + 8) * 8 - 8;
         Vdp2DrawCell(???, texture);
         texture->textdata -= 8;
         Vdp2DrawCell(???, texture);
         texture->textdata -= (texture->w + 8) * 8 - 8;
         Vdp2DrawCell(???, texture);
         break;
   }
   info->x += tile.w;
   info->y += tile.h;
*/
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2PatternAddr(vdp2draw_struct *info) {
   switch(info->patterndatasize) {
      case 1: {
         u16 tmp = T1ReadWord(Vdp2Ram, info->addr);         

         info->addr += 2;
         info->specialfunction = info->supplementdata & 0x300 >> 8;

         switch(info->colornumber) {
            case 0: // in 16 colors
               info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 8;
               break;
         }

         switch(info->auxmode) {
            case 0:
               info->flipfunction = (tmp & 0xC00) >> 10;

               switch(info->patternwh) {
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

               switch(info->patternwh) {
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
         info->specialfunction = (tmp1 & 0x3000) >> 12;
         break;
      }
   }

   if (!(Vdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // selon Runik
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawPage(vdp2draw_struct *info, YglTexture *texture) {
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->pagewh;i++) {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->pagewh;j++) {
         info->y = Y;
         if ((info->x >= -(8 * info->patternwh)) && (info->y >= -(8 * info->patternwh)) && (info->x < vdp2width) && (info->y < vdp2height)) {
            Vdp2PatternAddr(info);
            Vdp2DrawPattern(info, texture);
         } else {
            info->addr += info->patterndatasize * 2;
            info->x += 8 * info->patternwh;
            info->y += 8 * info->patternwh;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawPlane(vdp2draw_struct *info, YglTexture *texture) {
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->planeh;i++) {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->planew;j++) {
         info->y = Y;
         Vdp2DrawPage(info, texture);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawMap(vdp2draw_struct *info, YglTexture *texture) {
   int i, j;
   int X, Y;
   X = info->x;

   for(i = 0;i < info->mapwh;i++) {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->mapwh;j++) {
         info->y = Y;
//         planeAddr(info->mapwh * i + j);
         Vdp2DrawPlane(info, texture);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void SetSaturnResolution(int width, int height) {
   YglChangeResolution(width, height);

   vdp2width=width;
   vdp2height=height;
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLInit(void)
{
   if (YglInit(1024, 1024, 8) != 0)
      return -1;

   SetSaturnResolution(320, 224);
   Vdp1SetTextureRatio(1, 1);

   vdp1wratio = 1;
   vdp1hratio = 1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLDeInit(void)
{
   YglDeInit();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLResize(unsigned int w, unsigned int h)
{
   SDL_SetVideoMode(w, h, 32, SDL_OPENGL | SDL_RESIZABLE);
   glViewport(0, 0, w, h);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DrawStart(void)
{
   YglCacheReset();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1NormalSpriteDraw(void)
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

   Vdp1ReadPriority(&cmd, &sprite);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   if (sprite.w > 0 && sprite.h > 1) {
      if ((c = YglIsCached(tmp)) != NULL) {
         YglCachedQuad(&sprite, c);
         return;
      } 
      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1ScaledSpriteDraw(void)
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
   switch ((cmd.CMDCTRL & 0xF00) >> 8) {
      case 0x0: // Only two coordinates
         rw = cmd.CMDXC - x;
         rh = cmd.CMDYC - y;
         break;
      case 0x5: // Upper-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         break;
      case 0x6: // Upper-Center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         break;
      case 0x7: // Upper-Right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         break;
      case 0x9: // Center-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh/2;
         break;
      case 0xA: // Center-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh/2;
         break;
      case 0xB: // Center-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh/2;
         break;
      case 0xC: // Lower-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh;
         break;
      case 0xE: // Lower-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh;
         break;
      case 0xF: // Lower-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh;
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

   Vdp1ReadPriority(&cmd, &sprite);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   if (sprite.w > 0 && sprite.h > 1) {
      if ((c = YglIsCached(tmp)) != NULL) {
         YglCachedQuad(&sprite, c);
         return;
      } 
      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1DistortedSpriteDraw(void)
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
   sprite.vertices[2] = (s32)((float)(cmd.CMDXB + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[3] = (s32)((float)(cmd.CMDYB + Vdp1Regs->localY) * vdp1hratio);
   sprite.vertices[4] = (s32)((float)(cmd.CMDXC + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[5] = (s32)((float)(cmd.CMDYC + Vdp1Regs->localY) * vdp1hratio);
   sprite.vertices[6] = (s32)((float)(cmd.CMDXD + Vdp1Regs->localX) * vdp1wratio);
   sprite.vertices[7] = (s32)((float)(cmd.CMDYD + Vdp1Regs->localY) * vdp1hratio);

   Vdp1ReadPriority(&cmd, &sprite);

   tmp = cmd.CMDSRCA;

   tmp <<= 16;
   tmp |= cmd.CMDCOLR;

   if (sprite.w > 0 && sprite.h > 1) {
      if ((c = YglIsCached(tmp)) != NULL) {
         YglCachedQuad(&sprite, c);
         return;
      } 

      c = YglQuad(&sprite, &texture);
      YglCache(tmp, c);

      Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1PolygonDraw(void)
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

   if ((color & 0x8000) == 0)
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

   *texture.textdata = SAT2YAB1(alpha,color);
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
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1SystemClipping(void)
{
   Vdp1Regs->systemclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->systemclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp1LocalCoordinate(void)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLGLVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawStart(void)
{
   YglReset();
   YglCacheReset();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawEnd(void)
{
/*
   if (fpstoggle) {
      //onScreenDebugMessage(-0.9, -0.85, "%02d/60 FPS", fps);
      ygl.onScreenDebugMessage("%02d/60 FPS", fps);

      frameCount++;
      if(SDL_GetTicks() >= ticks + 1000) {
         fps = frameCount;
         frameCount = 0;
         ticks = SDL_GetTicks();
      }
   }
*/
   YglRender();
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawBackScreen(void)
{
   u32 scrAddr;
   int dot, y;

   if (Vdp2Regs->VRSIZE & 0x8000)
      scrAddr = (((Vdp2Regs->BKTAU & 0x7) << 16) | Vdp2Regs->BKTAL) * 2;
   else
      scrAddr = (((Vdp2Regs->BKTAU & 0x3) << 16) | Vdp2Regs->BKTAL) * 2;

   if (Vdp2Regs->BKTAU & 0x8000) {
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
   else {
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

void Vdp2DrawLineColorScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;
   YglTexture texture;

   /* FIXME should start by checking if it's a normal
    * or rotate scroll screen
    */
   info.enable = Vdp2Regs->BGON & 0x1;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);

   info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % 512);
   info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % 512);

   info.colornumber = (Vdp2Regs->CHCTLA & 0x70) >> 4;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0) {
      // Bitmap Mode

      switch((Vdp2Regs->CHCTLA & 0xC) >> 2) {
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

      info.charaddr = (Vdp2Regs->MPOFN & 0x7) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else {
      // Tile Mode
      info.mapwh = 2;

      switch(Vdp2Regs->PLSZ & 0x3) {
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
      }

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

   u16 colorCalc = Vdp2Regs->CCCTL;
   if (colorCalc & 0x1)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = Vdp2Regs->CRAOFA & 0x7;

   if (Vdp2Regs->CLOFEN & 0x1) {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x1) {
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
      else {
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
   }
   else { // color offset disable
      info.cor = info.cog = info.cob = 0;
   }

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN0.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMYN0.all & 0x7FF00);

   info.priority = nbg0priority;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   if (info.isbitmap) {
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
   else {
      Vdp2DrawMap(&info, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;
   YglTexture texture;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % 512);
   info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % 512);

   info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x200) != 0) {
      switch((Vdp2Regs->CHCTLA & 0xC00) >> 10) {
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

      info.charaddr = ((Vdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNA & 0x700) >> 4;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else {
      info.mapwh = 2;

      switch((Vdp2Regs->PLSZ & 0xC) >> 2) {
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
      }

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

   u16 colorCalc = Vdp2Regs->CCCTL;
   if (colorCalc & 0x2) {
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
   }
   else {
      info.alpha = 0xFF;
   }

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) >> 4;

   if (Vdp2Regs->CLOFEN & 0x2) {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x2) {
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
      else {
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
   }
   else { // color offset disable
      info.cor = info.cog = info.cob = 0;
   }

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);

   info.priority = nbg1priority;

   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
      return;

   if (info.isbitmap) {
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
   else {
      Vdp2DrawMap(&info, &texture);
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawNBG2(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawNBG3(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawRBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLGLVdp2DrawScreens(void)
{
   Vdp2DrawBackScreen();
   Vdp2DrawLineColorScreen();
   Vdp2DrawNBG0();
   Vdp2DrawNBG1();
   Vdp2DrawNBG2();
   Vdp2DrawNBG3();
   Vdp2DrawRBG0();
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLGLVdp2SetPriorityNBG0(int priority)
{
   nbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLGLVdp2SetPriorityNBG1(int priority)
{
   nbg1priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLGLVdp2SetPriorityNBG2(int priority)
{
   nbg2priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLGLVdp2SetPriorityNBG3(int priority)
{
   nbg3priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLGLVdp2SetPriorityRBG0(int priority)
{
   rbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////
