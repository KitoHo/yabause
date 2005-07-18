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

u8 * Vdp1Ram;

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
   if ((Vdp1Regs = (Vdp1 *) malloc(sizeof(Vdp1))) == NULL)
      return -1;

   if ((Vdp1Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1DeInit(void) {
   if (Vdp1Regs)
      free(Vdp1Regs);

   if (Vdp1Ram)
      T1MemoryDeInit(Vdp1Ram);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Reset(void) {
   Vdp1Regs->PTMR = 0;
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
   LOG("trying to long-read a Vdp1 register\n");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteByte(u32 addr, u8 val) {
   addr &= 0xFF;
   LOG("trying to byte-write a Vdp1 register\n");
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
         LOG("trying to write a Vdp1 read-only register\n");
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1WriteLong(u32 addr, u32 val) {
   addr &= 0xFF;
   LOG("trying to long-write a Vdp1 register\n");
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1Draw(void) {
	u32 returnAddr;
	unsigned long commandCounter;
	u16 command = T1ReadWord(Vdp1Ram, Vdp1Regs->addr);

	//YglReset();

	Vdp1Regs->addr = 0;
	returnAddr = 0xFFFFFFFF;
	commandCounter = 0;

	if (!Vdp1Regs->PTMR) return;

	if (!Vdp1Regs->disptoggle) return;

	// beginning of a frame (ST-013-R3-061694 page 53)
	// BEF <- CEF
	// CEF <- 0
	Vdp1Regs->EDSR >>= 1;

	while (!(command & 0x8000) && commandCounter < 2000) { // fix me

		// First, process the command
		if (!(command & 0x4000)) { // if (!skip)
			switch (command & 0x000F) {
			case 0: // normal sprite draw
				Vdp1NormalSpriteDraw();
				break;
			case 1: // scaled sprite draw
				Vdp1ScaledSpriteDraw();
				break;
			case 2: // distorted sprite draw
				Vdp1DistortedSpriteDraw();
				break;
			case 4: // polygon draw
				Vdp1PolygonDraw();
				break;
			case 5: // polyline draw
				Vdp1PolylineDraw();
				break;
			case 6: // line draw
				Vdp1LineDraw();
				break;
			case 8: // user clipping coordinates
				Vdp1UserClipping();
				break;
			case 9: // system clipping coordinates
				Vdp1SystemClipping();
				break;
			case 10: // local coordinate
				Vdp1LocalCoordinate();
				break;
			default:
#ifdef VDP1_DEBUG
				DEBUG("vdp1\t: Bad command: %x\n",  command);
#endif
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
}

void Vdp1NormalSpriteDraw(void) {
}

void Vdp1ScaledSpriteDraw(void) {
}

void Vdp1DistortedSpriteDraw(void) {
}

void Vdp1PolygonDraw(void) {
}

void Vdp1PolylineDraw(void) {
}

void Vdp1LineDraw(void) {
}

void Vdp1UserClipping(void) {
}

void Vdp1SystemClipping(void) {
}

void Vdp1LocalCoordinate(void) {
}

