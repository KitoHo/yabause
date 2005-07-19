/*  Copyright 2003-2005 Guillaume Duhamel
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

#ifndef VDP1_H
#define VDP1_H

#include "memory.h"

#define GFXCORE_DEFAULT         -1

typedef struct
{
   int id;
   const char *Name;
   int (*Init)();
   void (*DeInit)();
   int (*Reset)();
   void FASTCALL (*DrawBitmap)(); // fix me
   void FASTCALL (*DrawTilemap)(); // fix me
   void FASTCALL (*DrawRotatedBitmap)(); // fix me
   void FASTCALL (*DrawRotatedTilemap)(); // fix me
   void FASTCALL (*DrawSprite)(); // fix me
   void FASTCALL (*DrawPolygon)(); // fix me
   void FASTCALL (*DrawLine)(); // fix me
} VideoInterface_struct;

extern u8 * Vdp1Ram;

u8 FASTCALL	Vdp1RamReadByte(u32);
u16 FASTCALL	Vdp1RamReadWord(u32);
u32 FASTCALL	Vdp1RamReadLong(u32);
void FASTCALL	Vdp1RamWriteByte(u32, u8);
void FASTCALL	Vdp1RamWriteWord(u32, u16);
void FASTCALL	Vdp1RamWriteLong(u32, u32);

typedef struct {
	u16 TVHR;
	u16 FBCR;
	u16 PTMR;
	u16 EWDR;
	u16 EWLR;
	u16 EWRR;
	u16 ENDR;
	u16 EDSR;
	u16 LOPR;
	u16 COPR;
	u16 MODR;

	u32 addr;
	int disptoggle;
} Vdp1;

extern Vdp1 * Vdp1Regs;

int Vdp1Init(int coreid);
void Vdp1DeInit(void);
void Vdp1Reset(void);

u8 FASTCALL	Vdp1ReadByte(u32);
u16 FASTCALL	Vdp1ReadWord(u32);
u32 FASTCALL	Vdp1ReadLong(u32);
void FASTCALL	Vdp1WriteByte(u32, u8);
void FASTCALL	Vdp1WriteWord(u32, u16);
void FASTCALL	Vdp1WriteLong(u32, u32);

void Vdp1Draw(void);

void Vdp1NormalSpriteDraw(void);
void Vdp1ScaledSpriteDraw(void);
void Vdp1DistortedSpriteDraw(void);
void Vdp1PolygonDraw(void);
void Vdp1PolylineDraw(void);
void Vdp1LineDraw(void);
void Vdp1UserClipping(void);
void Vdp1SystemClipping(void);
void Vdp1LocalCoordinate(void);

#endif
