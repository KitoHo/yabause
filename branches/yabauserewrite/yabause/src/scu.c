/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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
#include "scu.h"
#include "debug.h"
#include "sh2core.h"

Scu * ScuRegs;

//////////////////////////////////////////////////////////////////////////////

int ScuInit(void) {        
        if ((ScuRegs = (Scu *) calloc(1, sizeof(Scu))) == NULL)
		return -1;

	ScuReset();

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void ScuDeInit(void) {
        if (ScuRegs)
           free(ScuRegs);
}

//////////////////////////////////////////////////////////////////////////////

void ScuReset(void) {
	ScuRegs->D0AD = ScuRegs->D1AD = ScuRegs->D2AD = 0x101;
	ScuRegs->D0EN = ScuRegs->D1EN = ScuRegs->D2EN = 0x0;
	ScuRegs->D0MD = ScuRegs->D1MD = ScuRegs->D2MD = 0x7;
	ScuRegs->DSTP = 0x0;
	ScuRegs->DSTA = 0x0;

	ScuRegs->PPAF = 0x0;
	ScuRegs->PDA = 0x0;

	ScuRegs->T1MD = 0x0;

	ScuRegs->IMS = 0xBFFF;
	ScuRegs->IST = 0x0;

	ScuRegs->AIACK = 0x0;
	ScuRegs->ASR0 = ScuRegs->ASR1 = 0x0;
	ScuRegs->AREF = 0x0;

	ScuRegs->RSEL = 0x0;
	ScuRegs->VER = 0x0;

        ScuRegs->timer0 = 0;
        ScuRegs->timer1 = 0;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL ScuReadByte(u32 addr) {
	LOG("trying to byte-read a Scu register\n");
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL ScuReadWord(u32 addr) {
	LOG("trying to word-read a Scu register\n");
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL ScuReadLong(u32 addr) {
	switch(addr) {
	case 0:
		return ScuRegs->D0R;
	case 4:
		return ScuRegs->W0R;
	case 8:
		return ScuRegs->D0C;
	case 0x20:
		return ScuRegs->D1R;
	case 0x24:
		return ScuRegs->W1R;
	case 0x28:
		return ScuRegs->D1C;
	case 0x40:
		return ScuRegs->D2R;
	case 0x44:
		return ScuRegs->W2R;
	case 0x48:
		return ScuRegs->D2C;
	case 0x7C:
		return ScuRegs->DSTA;
	case 0x80:
		return ScuRegs->PPAF;
	case 0x8C:
		return ScuRegs->PDD;
	case 0xA4:
		return ScuRegs->IST;
	case 0xA8:
		return ScuRegs->AIACK;
	case 0xC4:
		return ScuRegs->RSEL;
	case 0xC8:
		return ScuRegs->VER;
	default:
		LOG("bad Scu register read\n");
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ScuWriteByte(u32 addr, u8 val) {
	LOG("trying to byte-write a Scu register\n");
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ScuWriteWord(u32 addr, u16 val) {
	LOG("trying to word-write a Scu register\n");
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ScuWriteLong(u32 addr, u32 val) {
	switch(addr) {
	case 0:
		ScuRegs->D0R = val;
		break;
	case 4:
		ScuRegs->W0R = val;
		break;
	case 8:
		ScuRegs->D0C = val;
		break;
	case 0xC:
		ScuRegs->D0AD = val;
		break;
	case 0x10:
		ScuRegs->D0EN = val;
		break;
	case 0x14:
		ScuRegs->D0MD = val;
		break;
	case 0x20:
		ScuRegs->D1R = val;
		break;
	case 0x24:
		ScuRegs->W1R = val;
		break;
	case 0x28:
		ScuRegs->D1C = val;
		break;
	case 0x2C:
		ScuRegs->D1AD = val;
		break;
	case 0x30:
		ScuRegs->D1EN = val;
		break;
	case 0x34:
		ScuRegs->D1MD = val;
		break;
	case 0x40:
		ScuRegs->D2R = val;
		break;
	case 0x44:
		ScuRegs->W2R = val;
		break;
	case 0x48:
		ScuRegs->D2C = val;
		break;
	case 0x4C:
		ScuRegs->D2AD = val;
		break;
	case 0x50:
		ScuRegs->D2EN = val;
		break;
	case 0x54:
		ScuRegs->D2MD = val;
		break;
	case 0x60:
		ScuRegs->DSTP = val;
		break;
	case 0x80:
		ScuRegs->PPAF = val;
		break;
	case 0x84:
		ScuRegs->PPD = val;
		break;
	case 0x88:
		ScuRegs->PDA = val;
		break;
	case 0x8C:
		ScuRegs->PDD = val;
		break;
	case 0x90:
		ScuRegs->T0C = val;
		break;
	case 0x94:
		ScuRegs->T1S = val;
		break;
	case 0x98:
		ScuRegs->T1MD = val;
		break;
	case 0xA0:
		ScuRegs->IMS = val;
		break;
	case 0xA4:
		ScuRegs->IST = val;
		break;
	case 0xA8:
		ScuRegs->AIACK = val;
		break;
	case 0xB0:
		ScuRegs->ASR0 = val;
		break;
	case 0xB4:
		ScuRegs->ASR1 = val;
		break;
	case 0xB8:
		ScuRegs->AREF = val;
		break;
	case 0xC4:
		ScuRegs->RSEL = val;
		break;
	default:
		LOG("bad Scu register write\n");
	}
}

//////////////////////////////////////////////////////////////////////////////

static inline void SendInterrupt(u8 vector, u8 level, u16 mask, u32 statusbit) {
   ScuRegs->IST |= statusbit;

   if (!(ScuRegs->IMS & mask))
      SH2SendInterrupt(MSH2, vector, level);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendVBlankIN(void) {
   SendInterrupt(0x40, 0xF, 0x0001, 0x0001);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendVBlankOUT(void) {
   SendInterrupt(0x41, 0xE, 0x0002, 0x0002);
   ScuRegs->timer0 = 0;
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendHBlankIN(void) {
   SendInterrupt(0x42, 0xD, 0x0004, 0x0004);
   ScuRegs->timer0++;
   // if timer0 equals timer 0 compare register, do an interrupt
   if (ScuRegs->timer0 == ScuRegs->T0C)
      ScuSendTimer0();
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendTimer0(void) {
   SendInterrupt(0x43, 0xC, 0x0008, 0x00000008);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendTimer1(void) {
   SendInterrupt(0x44, 0xB, 0x0010, 0x00000010);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendDSPEnd(void) {
   SendInterrupt(0x45, 0xA, 0x0020, 0x00000020);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendSoundRequest(void) {
   SendInterrupt(0x46, 0x9, 0x0040, 0x00000040);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendSystemManager(void) {
   SendInterrupt(0x47, 0x8, 0x0080, 0x00000080);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendPadInterrupt(void) {
   SendInterrupt(0x48, 0x8, 0x0100, 0x00000100);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendLevel2DMAEnd(void) {
   SendInterrupt(0x49, 0x6, 0x0200, 0x00000200);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendLevel1DMAEnd(void) {
   SendInterrupt(0x4A, 0x6, 0x0400, 0x00000400);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendLevel0DMAEnd(void) {
   SendInterrupt(0x4B, 0x5, 0x0800, 0x00000800);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendDMAIllegal(void) {
   SendInterrupt(0x4C, 0x3, 0x1000, 0x00001000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendDrawEnd(void) {
   SendInterrupt(0x4D, 0x2, 0x2000, 0x00002000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt00(void) {
   SendInterrupt(0x50, 0x7, 0x8000, 0x00010000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt01(void) {
   SendInterrupt(0x51, 0x7, 0x8000, 0x00020000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt02(void) {
   SendInterrupt(0x52, 0x7, 0x8000, 0x00040000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt03(void) {
   SendInterrupt(0x53, 0x7, 0x8000, 0x00080000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt04(void) {
   SendInterrupt(0x54, 0x4, 0x8000, 0x00100000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt05(void) {
   SendInterrupt(0x55, 0x4, 0x8000, 0x00200000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt06(void) {
   SendInterrupt(0x56, 0x4, 0x8000, 0x00400000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt07(void) {
   SendInterrupt(0x57, 0x4, 0x8000, 0x00800000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt08(void) {
   SendInterrupt(0x58, 0x1, 0x8000, 0x01000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt09(void) {
   SendInterrupt(0x59, 0x1, 0x8000, 0x02000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt10(void) {
   SendInterrupt(0x5A, 0x1, 0x8000, 0x04000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt11(void) {
   SendInterrupt(0x5B, 0x1, 0x8000, 0x08000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt12(void) {
   SendInterrupt(0x5C, 0x1, 0x8000, 0x10000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt13(void) {
   SendInterrupt(0x5D, 0x1, 0x8000, 0x20000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt14(void) {
   SendInterrupt(0x5E, 0x1, 0x8000, 0x40000000);
}

//////////////////////////////////////////////////////////////////////////////

void ScuSendExternalInterrupt15(void) {
   SendInterrupt(0x5F, 0x1, 0x8000, 0x80000000);
}

//////////////////////////////////////////////////////////////////////////////

