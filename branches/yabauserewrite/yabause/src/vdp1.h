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

#define VIDCORE_DEFAULT         -1
#define VIDCORE_DUMMY           0

typedef struct
{
   int id;
   const char *Name;
   int (*Init)(void);
   void (*DeInit)(void);
   void (*Resize)(unsigned int, unsigned int);
   // VDP1 specific
   int (*Vdp1Reset)(void);
   void (*Vdp1DrawStart)(void);
   void (*Vdp1DrawEnd)(void);
   void (*Vdp1NormalSpriteDraw)(void);
   void (*Vdp1ScaledSpriteDraw)(void);
   void (*Vdp1DistortedSpriteDraw)(void);
   void (*Vdp1PolygonDraw)(void);
   void (*Vdp1PolylineDraw)(void);
   void (*Vdp1LineDraw)(void);
   void (*Vdp1UserClipping)(void);
   void (*Vdp1SystemClipping)(void);
   void (*Vdp1LocalCoordinate)(void);
   // VDP2 specific
   int (*Vdp2Reset)(void);
   void (*Vdp2DrawStart)(void);
   void (*Vdp2DrawEnd)(void);
   void (*Vdp2DrawScreens)(void);
   void (*Vdp2SetResolution)(u16);
   void FASTCALL (*Vdp2SetPriorityNBG0)(int);
   void FASTCALL (*Vdp2SetPriorityNBG1)(int);
   void FASTCALL (*Vdp2SetPriorityNBG2)(int);
   void FASTCALL (*Vdp2SetPriorityNBG3)(int);
   void FASTCALL (*Vdp2SetPriorityRBG0)(int);
   void (*Vdp2ToggleDisplayNBG0)(void);
   void (*Vdp2ToggleDisplayNBG1)(void);
   void (*Vdp2ToggleDisplayNBG2)(void);
   void (*Vdp2ToggleDisplayNBG3)(void);
   void (*Vdp2ToggleDisplayRBG0)(void);
} VideoInterface_struct;

extern VideoInterface_struct *VIDCore;
extern VideoInterface_struct VIDDummy;

extern u8 * Vdp1Ram;

u8 FASTCALL	Vdp1RamReadByte(u32);
u16 FASTCALL	Vdp1RamReadWord(u32);
u32 FASTCALL	Vdp1RamReadLong(u32);
void FASTCALL	Vdp1RamWriteByte(u32, u8);
void FASTCALL	Vdp1RamWriteWord(u32, u16);
void FASTCALL	Vdp1RamWriteLong(u32, u32);
u8 FASTCALL Vdp1FrameBufferReadByte(u32);
u16 FASTCALL Vdp1FrameBufferReadWord(u32);
u32 FASTCALL Vdp1FrameBufferReadLong(u32);
void FASTCALL Vdp1FrameBufferWriteByte(u32, u8);
void FASTCALL Vdp1FrameBufferWriteWord(u32, u16);
void FASTCALL Vdp1FrameBufferWriteLong(u32, u32);

typedef struct {
   u16 TVMR;
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

   u16 localX;
   u16 localY;

   u16 systemclipX1;
   u16 systemclipY1;
   u16 systemclipX2;
   u16 systemclipY2;

   u16 userclipX1;
   u16 userclipY1;
   u16 userclipX2;
   u16 userclipY2;
} Vdp1;

extern Vdp1 * Vdp1Regs;

typedef struct
{
   u16 CMDCTRL;
   u16 CMDLINK;
   u16 CMDPMOD;
   u16 CMDCOLR;
   u16 CMDSRCA;
   u16 CMDSIZE;
   s16 CMDXA;
   s16 CMDYA;
   s16 CMDXB;
   s16 CMDYB;
   s16 CMDXC;
   s16 CMDYC;
   s16 CMDXD;
   s16 CMDYD;
   u16 CMDGRDA;   
} vdp1cmd_struct;

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
void FASTCALL Vdp1ReadCommand(vdp1cmd_struct *cmd, u32 addr);
#endif
