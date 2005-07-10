/*  Copyright 2003 Guillaume Duhamel

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

#ifndef CS1_H
#define CS1_H

#include "cs0.h"
#include "memory.h"

typedef struct {
	int cartid;
	int carttype;
	const char * cartfile;
	void * sramarea;
	MemoryInterface * srammi;
} Cs1;

extern Cs1 * Cs1Area;

void Cs1New(const char * file, int type);
void Cs1Delete(void);

FASTCALL u8	Cs1ReadByte(u32);
FASTCALL u16	Cs1ReadWord(u32);
FASTCALL u32	Cs1ReadLong(u32);
FASTCALL void	Cs1WriteByte(u32, u8);
FASTCALL void	Cs1WriteWord(u32, u16);
FASTCALL void	Cs1WriteLong(u32, u32);

int Cs1SaveState(FILE *);
int Cs1LoadState(FILE *, int, int);

#endif
