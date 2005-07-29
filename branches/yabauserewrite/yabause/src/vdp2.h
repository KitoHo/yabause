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

#ifndef VDP2_H
#define VDP2_H

#include "memory.h"

extern u8 * Vdp2Ram;

u8 FASTCALL     Vdp2RamReadByte(u32);
u16 FASTCALL    Vdp2RamReadWord(u32);
u32 FASTCALL    Vdp2RamReadLong(u32);
void FASTCALL   Vdp2RamWriteByte(u32, u8);
void FASTCALL   Vdp2RamWriteWord(u32, u16);
void FASTCALL   Vdp2RamWriteLong(u32, u32);

u8 FASTCALL     Vdp2ColorRamReadByte(u32);
u16 FASTCALL    Vdp2ColorRamReadWord(u32);
u32 FASTCALL    Vdp2ColorRamReadLong(u32);
void FASTCALL   Vdp2ColorRamWriteByte(u32, u8);
void FASTCALL   Vdp2ColorRamWriteWord(u32, u16);
void FASTCALL   Vdp2ColorRamWriteLong(u32, u32);

typedef struct {
   unsigned short TVMD;   // 0x25F80000
   unsigned short EXTEN;  // 0x25F80002
   unsigned short TVSTAT; // 0x25F80004
   unsigned short HCNT;   // 0x25F80008
   unsigned short VCNT;   // 0x25F8000A
   unsigned short RAMCTL; // 0x25F8000E
   unsigned short BGON;   // 0x25F80020
   unsigned short CHCTLA; // 0x25F80028
   unsigned short CHCTLB; // 0x25F8002A
   unsigned short BMPNA;  // 0x25F8002C
   unsigned short MPOFN;  // 0x25F8003C
   unsigned short MPABN2; // 0x25F80048
   unsigned short MPCDN2; // 0x25F8004A
   unsigned short BKTAU;  // 0x25F800AC
   unsigned short BKTAL;  // 0x25F800AE
   unsigned short CRAOFA; // 0x25F800E4
   unsigned short CRAOFB; // 0x25F800E6
   unsigned short PRINA;  // 0x25F800F8
   unsigned short PRINB;  // 0x25F800FA
   unsigned short PRIR;   // 0x25F800FC
} Vdp2;

extern Vdp2 * Vdp2Regs;

int Vdp2Init(int coreid);
void Vdp2DeInit(void);
void Vdp2Reset(void);
void Vdp2VBlankIN(void);
void Vdp2HBlankIN(void);
void Vdp2HBlankOUT(void);
void Vdp2VBlankOUT(void);

u8 FASTCALL     Vdp2ReadByte(u32);
u16 FASTCALL    Vdp2ReadWord(u32);
u32 FASTCALL    Vdp2ReadLong(u32);
void FASTCALL   Vdp2WriteByte(u32, u8);
void FASTCALL   Vdp2WriteWord(u32, u16);
void FASTCALL   Vdp2WriteLong(u32, u32);

#endif
