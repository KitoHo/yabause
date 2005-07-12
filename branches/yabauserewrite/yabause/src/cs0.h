/*  Copyright 2004 Theo Berkau

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

#ifndef CS0_H
#define CS0_H

#include <stdio.h>
#include "memory.h"

#define CART_NONE               0
#define CART_PAR                1
#define CART_BACKUPRAM4MBIT     2
#define CART_BACKUPRAM8MBIT     3
#define CART_BACKUPRAM16MBIT    4
#define CART_BACKUPRAM32MBIT    5
#define CART_DRAM8MBIT          6
#define CART_DRAM32MBIT         7
#define CART_NETLINK            8

typedef struct {
	int carttype;
	void * biosarea;
	void * dramarea;
	MemoryInterface * biosmi;
	MemoryInterface * drammi;
} Cs0;

extern Cs0 * Cs0Area;

void Cs0Init(const char *, int);
void Cs0DeInit(void);

FASTCALL u8	Cs0ReadByte(u32);
FASTCALL u16	Cs0ReadWord(u32);
FASTCALL u32	Cs0ReadLong(u32);
FASTCALL void	Cs0WriteByte(u32, u8);
FASTCALL void	Cs0WriteWord(u32, u16);
FASTCALL void	Cs0WriteLong(u32, u32);

int Cs0SaveState(FILE *);
int Cs0LoadState(FILE *, int, int);

#endif
