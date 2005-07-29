/*  Copyright 2003-2005 Guillaume Duhamel
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

#include <stdlib.h>
#include "vdp1.h"
#include "debug.h"
#include "scu.h"
#include "vdp2.h"

u8 * Vdp1Ram;

VideoInterface_struct *VIDCore=NULL;
extern VideoInterface_struct *VIDCoreList[];

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1RamReadByte(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadByte(Vdp1Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1RamReadWord(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadWord(Vdp1Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1RamReadLong(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadLong(Vdp1Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteByte(u32 addr, u8 val) {
   addr &= 0x7FFFF;
   T1WriteByte(Vdp1Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteWord(u32 addr, u16 val) {
   addr &= 0x7FFFF;
   T1WriteWord(Vdp1Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1RamWriteLong(u32 addr, u32 val) {
   addr &= 0x7FFFF;
   T1WriteLong(Vdp1Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

Vdp1 * Vdp1Regs;

//////////////////////////////////////////////////////////////////////////////

int Vdp1Init(int coreid) {
   int i;

   if ((Vdp1Regs = (Vdp1 *) malloc(sizeof(Vdp1))) == NULL)
      return -1;

   if ((Vdp1Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   // So which core do we want?
   if (coreid == VIDCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; VIDCoreList[i] != NULL; i++)
   {
      if (VIDCoreList[i]->id == coreid)
      {
         // Set to current core
         VIDCore = VIDCoreList[i];
         break;
      }
   }

   if (VIDCore == NULL)
      return -1;

   VIDCore->Init();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1DeInit(void) {
   if (Vdp1Regs)
      free(Vdp1Regs);

   if (Vdp1Ram)
      T1MemoryDeInit(Vdp1Ram);

   if (VIDCore)
      VIDCore->DeInit();
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Reset(void) {
   Vdp1Regs->PTMR = 0;
   VIDCore->Vdp1Reset();
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp1ReadByte(u32 addr) {
   addr &= 0xFF;
   LOG("trying to byte-read a Vdp1 register\n");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp1ReadWord(u32 addr) {
   addr &= 0xFF;
   switch(addr) {
      case 0x10:
         return Vdp1Regs->EDSR;
      case 0x12:
         return Vdp1Regs->LOPR;
      case 0x14:
         return Vdp1Regs->COPR;
      case 0x16:
         return Vdp1Regs->MODR;
      default:
         LOG("trying to read a Vdp1 write-only register\n");
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1ReadLong(u32 addr) {
   addr &= 0xFF;
   LOG("trying to long-read a Vdp1 register - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteByte(u32 addr, u8 val) {
   addr &= 0xFF;
   LOG("trying to byte-write a Vdp1 register - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteWord(u32 addr, u16 val) {
   addr &= 0xFF;
   switch(addr) {
      case 0x0:
         Vdp1Regs->TVHR = val;
         break;
      case 0x2:
         Vdp1Regs->FBCR = val;
         break;
      case 0x4:
         Vdp1Regs->PTMR = val;
         break;
      case 0x6:
         Vdp1Regs->EWDR = val;
         break;
      case 0x8:
         Vdp1Regs->EWLR = val;
         break;
      case 0xA:
         Vdp1Regs->EWRR = val;
         break;
      case 0xC:
         Vdp1Regs->ENDR = val;
         break;
      default:
         LOG("trying to write a Vdp1 read-only register - %08X\n", addr);
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteLong(u32 addr, u32 val) {
   addr &= 0xFF;
   LOG("trying to long-write a Vdp1 register - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Draw(void) {
   u32 returnAddr;
   u32 commandCounter;
   u16 command = T1ReadWord(Vdp1Ram, Vdp1Regs->addr);

   VIDCore->Vdp1DrawStart();

   Vdp1Regs->addr = 0;
   returnAddr = 0xFFFFFFFF;
   commandCounter = 0;

   if (!Vdp1Regs->PTMR)
      return;

   if (!Vdp1Regs->disptoggle)
      return;

   // beginning of a frame (ST-013-R3-061694 page 53)
   // BEF <- CEF
   // CEF <- 0
   Vdp1Regs->EDSR >>= 1;

   while (!(command & 0x8000) && commandCounter < 2000) { // fix me
      // First, process the command
      if (!(command & 0x4000)) { // if (!skip)
         switch (command & 0x000F) {
            case 0: // normal sprite draw
               VIDCore->Vdp1NormalSpriteDraw();
               break;
            case 1: // scaled sprite draw
               VIDCore->Vdp1ScaledSpriteDraw();
               break;
            case 2: // distorted sprite draw
               VIDCore->Vdp1DistortedSpriteDraw();
               break;
            case 4: // polygon draw
               VIDCore->Vdp1PolygonDraw();
               break;
            case 5: // polyline draw
               VIDCore->Vdp1PolylineDraw();
               break;
            case 6: // line draw
               VIDCore->Vdp1LineDraw();
               break;
            case 8: // user clipping coordinates
               VIDCore->Vdp1UserClipping();
               break;
            case 9: // system clipping coordinates
               VIDCore->Vdp1SystemClipping();
               break;
            case 10: // local coordinate
               VIDCore->Vdp1LocalCoordinate();
               break;
            default:
               VDP1LOG("vdp1\t: Bad command: %x\n",  command);
               break;
         }
      }

      // Next, determine where to go next
      switch ((command & 0x3000) >> 12) {
         case 0: // NEXT, jump to following table
            Vdp1Regs->addr += 0x20;
            break;
         case 1: // ASSIGN, jump to CMDLINK
            Vdp1Regs->addr = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 2) * 8;
            break;
         case 2: // CALL, call a subroutine
            if (returnAddr == 0xFFFFFFFF)
               returnAddr = Vdp1Regs->addr + 0x20;
	
            Vdp1Regs->addr = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 2) * 8;
            break;
         case 3: // RETURN, return from subroutine
            if (returnAddr != 0xFFFFFFFF) {
               Vdp1Regs->addr = returnAddr;
               returnAddr = 0xFFFFFFFF;
            }
            else
               Vdp1Regs->addr += 0x20;
            break;
      }

      command = T1ReadWord(Vdp1Ram, Vdp1Regs->addr);
      commandCounter++;    
   }

   // we set two bits to 1
   Vdp1Regs->EDSR |= 2;
   ScuSendDrawEnd();
   VIDCore->Vdp1DrawEnd();
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr) {
   cmd->CMDCTRL = T1ReadWord(Vdp1Ram, addr);
   cmd->CMDLINK = T1ReadWord(Vdp1Ram, addr + 0x2);
   cmd->CMDPMOD = T1ReadWord(Vdp1Ram, addr + 0x4);
   cmd->CMDCOLR = T1ReadWord(Vdp1Ram, addr + 0x5);
   cmd->CMDSRCA = T1ReadWord(Vdp1Ram, addr + 0x8);
   cmd->CMDSIZE = T1ReadWord(Vdp1Ram, addr + 0xA);
   cmd->CMDXA = T1ReadWord(Vdp1Ram, addr + 0xC);
   cmd->CMDYA = T1ReadWord(Vdp1Ram, addr + 0xE);
   cmd->CMDXB = T1ReadWord(Vdp1Ram, addr + 0x10);
   cmd->CMDYB = T1ReadWord(Vdp1Ram, addr + 0x12);
   cmd->CMDXC = T1ReadWord(Vdp1Ram, addr + 0x14);
   cmd->CMDYC = T1ReadWord(Vdp1Ram, addr + 0x16);
   cmd->CMDXD = T1ReadWord(Vdp1Ram, addr + 0x18);
   cmd->CMDYD = T1ReadWord(Vdp1Ram, addr + 0x1A);
   cmd->CMDGRDA = T1ReadWord(Vdp1Ram, addr + 0x1B);
}

//////////////////////////////////////////////////////////////////////////////
/*
void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, u32 * textdata, u32 z) {
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

         for(i = 0;i < h;i++) {
            u16 j;
            j = 0;
            while(j < w) {
               dot = T1ReadByte(Vdp1Ram, charAddr);

               // Pixel 1
               if (((dot >> 4) == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = cram->getColor((dot >> 4) + colorBank, alpha, colorOffset);
               j += 1;

               // Pixel 2
               if (((dot & 0xF) == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = cram->getColor((dot & 0xF) + colorBank, alpha, colorOffset);
               j += 1;

               charAddr += 1;
            }
            textdata += z;
         }
         break;
      }
      case 1:
      {
         // 4 bpp LUT mode
         u32 temp;
         u32 colorLut = cmd->CMDCOLR * 8;
         u16 i;

         for(i = 0;i < h;i++) {
            u16 j;
            j = 0;
            while(j < w) {
               dot = T1ReadByte(Vdp1Ram, charAddr);

               if (((dot >> 4) == 0) && !SPD) *textdata++ = 0;
               else {
                  temp = vram->getWord((dot >> 4) * 2 + colorLut);
                  *textdata++ = SAT2YAB1(alpha, temp);
               }

               j += 1;

               if (((dot & 0xF) == 0) && !SPD) *textdata++ = 0;
               else {
                  temp = vram->getWord((dot & 0xF) * 2 + colorLut);
                  *textdata++ = SAT2YAB1(alpha, temp);
               }

               j += 1;

               charAddr += 1;
            }
            textdata += z;
         }
         break;
      }
      case 2:
      {
         // 8 bpp(64 color) Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFFC0;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < h;i++) {
            for(j = 0;j < w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr) & 0x3F;               
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
            }
            textdata += z;
         }

         break;
      }
      case 3:
      {
         // 8 bpp(128 color) Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFF80;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < h;i++) {
            for(j = 0;j < w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr) & 0x7F;               
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
            }
            textdata += z;
         }
         break;
      }
      case 4:
      {
         // 8 bpp(256 color) Bank mode
         u32 colorBank = cmd->CMDCOLR & 0xFF00;
         u32 colorOffset = (Vdp2Regs->CRAOFB >> 4) & 0x7;
         u16 i, j;

         for(i = 0;i < h;i++) {
            for(j = 0;j < w;j++) {
               dot = T1ReadByte(Vdp1Ram, charAddr);               
               charAddr++;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
            }
            textdata += z;
         }

         break;
      }
      case 5:
      {
         // 16 bpp Bank mode
         u16 i, j;

         for(i = 0;i < h;i++) {
            for(j = 0;j < w;j++) {
               dot = T1ReadWord(Vdp1Ram, charAddr);               
               charAddr += 2;

               if ((dot == 0) && !SPD) *textdata++ = 0;
               else *textdata++ = SAT2YAB1(alpha, dot);
            }
            textdata += z;
         }
         break;
      }
      default:
         VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
         break;
   }
}
*/
//////////////////////////////////////////////////////////////////////////////
/*
void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd) {
   u8 SPCLMD = Vdp2Regs->SPCTL;
   u8 sprite_register;

   // if we don't know what to do with a sprite, we put it on top
   priority = 7;

   if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000)) {
      // RGB data, use register 0
      priority = vdp2reg->getWord(0xF0) & 0x7;
   } else {
      u8 sprite_type = SPCLMD & 0xF;
      switch(sprite_type) {
         case 0:
            sprite_register = ((cmd->CMDCOLR & 0x8000) | (~cmd->CMDCOLR & 0x4000)) >> 14;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 1:
            sprite_register = ((cmd->CMDCOLR & 0xC000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 3:
            sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 4:
            sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 5:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 6:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         case 7:
            sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
//            priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
            break;
         default:
            VDP1LOG("sprite type %d not implemented\n", sprite_type);
            break;
      }
   }
}
*/

//////////////////////////////////////////////////////////////////////////////
// Dummy Video Interface
//////////////////////////////////////////////////////////////////////////////

int VIDDummyInit(void);
void VIDDummyDeInit(void);
int VIDDummyVdp1Reset(void);
void VIDDummyVdp1DrawStart(void);
void VIDDummyVdp1DrawEnd(void);
void VIDDummyVdp1NormalSpriteDraw(void);
void VIDDummyVdp1ScaledSpriteDraw(void);
void VIDDummyVdp1DistortedSpriteDraw(void);
void VIDDummyVdp1PolygonDraw(void);
void VIDDummyVdp1PolylineDraw(void);
void VIDDummyVdp1LineDraw(void);
void VIDDummyVdp1UserClipping(void);
void VIDDummyVdp1SystemClipping(void);
void VIDDummyVdp1LocalCoordinate(void);
int VIDDummyVdp2Reset(void);
void VIDDummyVdp2DrawEnd(void);
void VIDDummyVdp2DrawBackScreen(void);
void VIDDummyVdp2DrawLineColorScreen(void);
void VIDDummyVdp2DrawNBG0(void);
void VIDDummyVdp2DrawNBG1(void);
void VIDDummyVdp2DrawNBG2(void);
void VIDDummyVdp2DrawNBG3(void);
void VIDDummyVdp2DrawRBG0(void);

VideoInterface_struct VIDDummy = {
VIDCORE_DUMMY,
"Dummy Video Interface",
VIDDummyInit,
VIDDummyDeInit,
VIDDummyVdp1Reset,
VIDDummyVdp1DrawStart,
VIDDummyVdp1DrawEnd,
VIDDummyVdp1NormalSpriteDraw,
VIDDummyVdp1ScaledSpriteDraw,
VIDDummyVdp1DistortedSpriteDraw,
VIDDummyVdp1PolygonDraw,
VIDDummyVdp1PolylineDraw,
VIDDummyVdp1LineDraw,
VIDDummyVdp1UserClipping,
VIDDummyVdp1SystemClipping,
VIDDummyVdp1LocalCoordinate,
VIDDummyVdp2Reset,
VIDDummyVdp2DrawEnd,
VIDDummyVdp2DrawBackScreen,
VIDDummyVdp2DrawLineColorScreen,
VIDDummyVdp2DrawNBG0,
VIDDummyVdp2DrawNBG1,
VIDDummyVdp2DrawNBG2,
VIDDummyVdp2DrawNBG3,
VIDDummyVdp2DrawRBG0
};

//////////////////////////////////////////////////////////////////////////////

int VIDDummyInit(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyDeInit(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDDummyVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1DrawStart(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1NormalSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1ScaledSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1DistortedSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1PolygonDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1PolylineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1LineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1UserClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1SystemClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp1LocalCoordinate(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDDummyVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawBackScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawLineColorScreen(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawNBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawNBG1(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawNBG2(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawNBG3(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDDummyVdp2DrawRBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////

