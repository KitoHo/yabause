/*  Copyright 2007 Guillaume Duhamel

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

#include "m68kcore.h"
#include "m68kc68k.h"
#include "memory.h"

extern u8 * SoundRam;

M68K_struct * M68K = NULL;

extern M68K_struct * M68KCoreList[];

int M68KInit(int coreid) {
   int i;

   M68K = &M68KDummy;

   // Go through core list and find the id
   for (i = 0; M68KCoreList[i] != NULL; i++)
   {
      if (M68KCoreList[i]->id == coreid)
      {
         // Set to current core
         M68K = M68KCoreList[i];
         break;
      }
   }

   return 0;
}

void M68KDummyInit(void) {
}

void M68KDummyDeInit(void) {
}

void M68KDummyReset(void) {
}

s32 FASTCALL M68KDummyExec(s32 cycle) {
	T2WriteWord(SoundRam, 0x700, 0);
	T2WriteWord(SoundRam, 0x710, 0);
	T2WriteWord(SoundRam, 0x720, 0);
	T2WriteWord(SoundRam, 0x730, 0);
	T2WriteWord(SoundRam, 0x740, 0);
	T2WriteWord(SoundRam, 0x750, 0);
	T2WriteWord(SoundRam, 0x760, 0);
	T2WriteWord(SoundRam, 0x770, 0);

	T2WriteWord(SoundRam, 0x790, 0);
	T2WriteWord(SoundRam, 0x792, 0);
	return 0;
}

u32 M68KDummyGetDReg(u32 num) {
	return 0;
}

u32 M68KDummyGetAReg(u32 num) {
	return 0;
}

u32 M68KDummyGetPC(void) {
	return 0;
}

u32 M68KDummyGetSR(void) {
	return 0;
}

u32 M68KDummyGetUSP(void) {
	return 0;
}

u32 M68KDummyGetMSP(void) {
	return 0;
}

void M68KDummySetDReg(u32 num, u32 val) {
}

void M68KDummySetAReg(u32 num, u32 val) {
}

void M68KDummySetPC(u32 val) {
}

void M68KDummySetSR(u32 val) {
}

void M68KDummySetUSP(u32 val) {
}

void M68KDummySetMSP(u32 val) {
}

void M68KDummySetFetch(u32 low_adr, u32 high_adr, pointer fetch_adr) {
}

void FASTCALL M68KDummySetIRQ(s32 level) {
}

void M68KDummySetReadB(M68K_READ *Func) {
}

void M68KDummySetReadW(M68K_READ *Func) {
}

void M68KDummySetWriteB(M68K_WRITE *Func) {
}

void M68KDummySetWriteW(M68K_WRITE *Func) {
}

M68K_struct M68KDummy = {
	0,
	"Dummy 68k Interface",
	M68KDummyInit,
	M68KDummyDeInit,
	M68KDummyReset,
	M68KDummyExec,
	M68KDummyGetDReg,
	M68KDummyGetAReg,
	M68KDummyGetPC,
	M68KDummyGetSR,
	M68KDummyGetUSP,
	M68KDummyGetMSP,
	M68KDummySetDReg,
	M68KDummySetAReg,
	M68KDummySetPC,
	M68KDummySetSR,
	M68KDummySetUSP,
	M68KDummySetMSP,
	M68KDummySetFetch,
	M68KDummySetIRQ,
	M68KDummySetReadB,
	M68KDummySetReadW,
	M68KDummySetWriteB,
	M68KDummySetWriteW
};