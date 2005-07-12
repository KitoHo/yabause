/*  Copyright 2003 Guillaume Duhamel
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
#include "cs1.h"

Cs1 * Cs1Area;

void Cs1Init(const char * file, int type) {
	Cs1Area = (Cs1 *) malloc(sizeof(Cs1));

	if (Cs1Area == NULL)
		return -1;

	switch (type) {
	case CART_PAR: // Action Replay, Rom carts?(may need to change this)
		Cs1Area->cartid = 0x5C; // fix me
		Cs1Area->srammi = DummyInterface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0);
		break;
	case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
		Cs1Area->cartid = 0x21;
		Cs1Area->srammi = T3Interface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0x100000);
		if (MemoryLoad(Cs1Area->srammi, Cs1Area->sramarea, file, 0) != 0) {
			// Should Probably format Ram here
		}
		break;
	case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
		Cs1Area->cartid = 0x22;
		Cs1Area->srammi = T3Interface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0x200000);
		if (MemoryLoad(Cs1Area->srammi, Cs1Area->sramarea, file, 0) != 0) {
			// Should Probably format Ram here
		}
		break;
	case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
		Cs1Area->cartid = 0x23;
		Cs1Area->srammi = T3Interface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0x400000);
		if (MemoryLoad(Cs1Area->srammi, Cs1Area->sramarea, file, 0) != 0) {
			// Should Probably format Ram here
		}
		break;
	case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
		Cs1Area->cartid = 0x24;
		Cs1Area->srammi = T3Interface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0x800000);
		if (MemoryLoad(Cs1Area->srammi, Cs1Area->sramarea, file, 0) != 0) {
			// Should Probably format Ram here
		}
		break;
	case CART_DRAM8MBIT: // 8 Mbit Dram Cart
		Cs1Area->cartid = 0x5A;
		Cs1Area->srammi = DummyInterface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0);
		break;
	case CART_DRAM32MBIT: // 32 Mbit Dram Cart
		Cs1Area->cartid = 0x5C;
		Cs1Area->srammi = DummyInterface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0);
		break;
	default: // No Cart
		Cs1Area->cartid = 0xFF;
		Cs1Area->srammi = DummyInterface();
		Cs1Area->sramarea = Cs1Area->srammi->New(0);
		break;
	}

	Cs1Area->carttype = type;
	Cs1Area->cartfile = file;

	return 0;
}

void Cs1DeInit(void) {
	switch (Cs1Area->carttype) {
	case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
		MemorySave(Cs1Area->srammi, Cs1Area->sramarea, Cs1Area->cartfile, 0, 0x100000);
		break;
	case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
		MemorySave(Cs1Area->srammi, Cs1Area->sramarea, Cs1Area->cartfile, 0, 0x200000);
		break;
	case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
		MemorySave(Cs1Area->srammi, Cs1Area->sramarea, Cs1Area->cartfile, 0, 0x400000);
		break;
	case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
		MemorySave(Cs1Area->srammi, Cs1Area->sramarea, Cs1Area->cartfile, 0, 0x800000);
		break;
	default:
		break;
	}
	Cs1Area->srammi->Delete(Cs1Area->sramarea);
	free(Cs1Area->srammi);
	free(Cs1Area);
}

FASTCALL u8 Cs1ReadByte(u32 addr) {
	if (addr == 0xFFFFFF)
		return Cs1Area->cartid;

	return Cs1Area->srammi->ReadByte(Cs1Area->sramarea, addr);
}

FASTCALL u16 Cs1ReadWord(u32 addr) {
	if (addr == 0xFFFFFE)
		return (0xFF00 | Cs1Area->cartid);

	return Cs1Area->srammi->ReadWord(Cs1Area->sramarea, addr);
}

FASTCALL u32 Cs1ReadLong(u32 addr) {
	if (addr == 0xFFFFFC)
		return (0xFF00FF00 | (Cs1Area->cartid << 16) | Cs1Area->cartid);

	return Cs1Area->srammi->ReadLong(Cs1Area->sramarea, addr);
}

FASTCALL void Cs1WriteByte(u32 addr, u8 val) {
	if (addr == 0xFFFFFF)
		return;
	Cs1Area->srammi->WriteByte(Cs1Area->sramarea, addr, val);
}

FASTCALL void Cs1WriteWord(u32 addr, u16 val) {
	if (addr == 0xFFFFFF)
		return;
	Cs1Area->srammi->WriteWord(Cs1Area->sramarea, addr, val);
}

FASTCALL void Cs1WriteLong(u32 addr, u32 val) {
	if (addr == 0xFFFFFF)
		return;
	Cs1Area->srammi->WriteLong(Cs1Area->sramarea, addr, val);
}

int Cs1SaveState(FILE * fp) {
	int offset;

	offset = StateWriteHeader(fp, "CS1 ", 1);

	// Write cart type
	fwrite((void *) &Cs1Area->carttype, 4, 1, fp);

	// Write the areas associated with the cart type here

	return StateFinishHeader(fp, offset);
}

int Cs1LoadState(FILE * fp, int version, int size) {
	int oldtype = Cs1Area->carttype;
	// Read cart type
	fread((void *) &Cs1Area->carttype, 4, 1, fp);

	// Check to see if old cart type and new cart type match, if they don't,
	// reallocate memory areas

	// Read the areas associated with the cart type here

	return size;
}
