/*  Copyright 2004-2005 Theo Berkau

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
#include "cs0.h"

//Cs0 * Cs0Area;

int Cs0Init(const char * file, int type) {
/*
	Cs0Area = (Cs0 *) malloc(sizeof(Cs0));

	if (Cs0Area == NULL)
		return -1;

	switch (type) {
	case CART_PAR: // Action Replay, Rom carts?(may need to change this)
		Cs0Area->biosmi = T3Interface();
		Cs0Area->drammi = DummyInterface();
		// allocate memory for Cart
		Cs0Area->biosarea = Cs0Area->biosmi->New(0x40000);
		MemoryLoad(Cs0Area->biosmi, Cs0Area->biosarea, file, 0);
		Cs0Area->dramarea = Cs0Area->drammi->New(0x400000);
		break;
	case CART_DRAM32MBIT: // 32 Mbit Dram Cart
		Cs0Area->biosmi = DummyInterface();
		Cs0Area->drammi = T3Interface();
		// Allocate memory for dram(fix me)
		Cs0Area->biosarea = Cs0Area->biosmi->New(0x40000); // may need to change this
		Cs0Area->dramarea = Cs0Area->drammi->New(0x400000);
		break;
	case CART_DRAM8MBIT: // 8 Mbit Dram Cart
		Cs0Area->biosmi = DummyInterface();
		Cs0Area->drammi = T3Interface();
		// Allocate memory for dram
		Cs0Area->biosarea = Cs0Area->biosmi->New(0x40000); // may need to change this
		Cs0Area->dramarea = Cs0Area->drammi->New(0x100000);
		break;
	default: // No Cart
		Cs0Area->biosmi = DummyInterface();
		Cs0Area->drammi = DummyInterface();
		Cs0Area->biosarea = Cs0Area->biosmi->New(0x40000); // may need to change this
		Cs0Area->dramarea = Cs0Area->drammi->New(0x400000);
		break;
	}

	Cs0Area->carttype = type;
*/
	return 0;
}

void Cs0DeInit(void) {
/*
	Cs0Area->biosmi->Delete(Cs0Area->biosarea);
	Cs0Area->drammi->Delete(Cs0Area->dramarea);
	free(Cs0Area->biosmi);
	free(Cs0Area->drammi);
	free(Cs0Area);
*/
}

void FASTCALL Cs0WriteByte(u32 addr, u8 val) {
/*
	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			Cs0Area->biosmi->WriteByte(Cs0Area->biosarea, addr, val);
//			fprintf(stderr, "EEPROM write to %08X = %02X\n", addr, val);
		}
		else // Outport
		{   
//			Memory::setByte(addr, val);
//			fprintf(stderr, "Commlink Outport byte write\n");
                 }
                 break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			Memory::setByte(addr, val);
//			fprintf(stderr, "Commlink Status Flag write\n");
		}
		else // Inport for Commlink
		{   
//			Memory::setByte(addr, val);
//			fprintf(stderr, "Commlink Inport Byte write\n");
                 }
                 break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		Cs0Area->drammi->WriteByte(Cs0Area->dramarea, addr, val);
		break;
	default:   // The rest doesn't matter
		break;
	}
*/
}

u8 FASTCALL Cs0ReadByte(u32 addr) {
/*
	u8 val = 0xFF;

	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			val = Cs0Area->biosmi->ReadByte(Cs0Area->biosarea, addr);
//			fprintf(stderr, "EEPROM read from %08X = %02X\n", addr, val);
		}
		else // Outport
		{   
//			val = Memory::getByte(addr);
//			fprintf(stderr, "Commlink Outport Byte read\n");
		}
		break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			val = Memory::getByte(addr);
//			fprintf(stderr, "Commlink Status Flag read\n");
		}
		else // Inport for Commlink
		{   
//			val = Memory::getByte(addr);
//			fprintf(stderr, "Commlink Inport Byte read\n");
		}
		break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		Cs0Area->drammi->WriteByte(Cs0Area->dramarea, addr, val);
		val = Cs0Area->biosmi->ReadByte(Cs0Area->biosarea, addr);
		break;
//	case 0x12:
//	case 0x1E:
//		break;
//	case 0x13:
//	case 0x16:
//	case 0x17:
//	case 0x1A:
//	case 0x1B:
//	case 0x1F:
//		break;
	default:   // The rest doesn't matter
		break;
	}

	return val;
*/
   return 0;
}

void FASTCALL Cs0WriteWord(u32 addr, u16 val) {
/*
	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			Cs0Area->biosmi->WriteWord(Cs0Area->biosarea, addr, val);
//			fprintf(stderr, "EEPROM write to %08X = %04X\n", addr, val);
		}
		else // Outport
		{   
//			Memory::setWord(addr, val);
//			fprintf(stderr, "Commlink Outport Word write\n");
		}
		break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			Memory::setWord(addr, val);
//			fprintf(stderr, "Commlink Status Flag write\n");
		}
		else // Inport for Commlink
		{   
//			Memory::setWord(addr, val);
//			fprintf(stderr, "Commlink Inport Word write\n");
		}
		break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		Cs0Area->drammi->WriteWord(Cs0Area->dramarea, addr, val);
		break;
	default:   // The rest doesn't matter
		break;
	}
*/
}

u16 FASTCALL Cs0ReadWord(u32 addr) {
/*
	u16 val = 0xFFFF;

	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			val = Cs0Area->biosmi->ReadWord(Cs0Area->biosarea, addr);
//			fprintf(stderr, "EEPROM read from %08X = %04X\n", addr, val);
		}
		else // Outport
		{   
//			val = Memory::getWord(addr);
//			fprintf(stderr, "Commlink Outport Word read\n");
		}
		break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			val = Memory::getWord(addr);
//			fprintf(stderr, "Commlink Status Flag read\n");
		}
		else // Inport for Commlink
		{   
//			val = Memory::getWord(addr);
//			fprintf(stderr, "Commlink Inport Word read\n");
		}
		break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		val = Cs0Area->drammi->ReadWord(Cs0Area->dramarea, addr);
		break;
	case 0x12:
	case 0x1E:
		if (0x80000)
			val = 0xFFFD;
		break;
	case 0x13:
	case 0x16:
	case 0x17:
	case 0x1A:
	case 0x1B:
	case 0x1F:
		val = 0xFFFD;
		break;
	default:   // The rest doesn't matter
                 break;
	}

	return val;
*/
   return 0;
}

void FASTCALL Cs0WriteLong(u32 addr, u32 val) {
/*
	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			Cs0Area->biosmi->WriteLong(Cs0Area->biosarea, addr, val);
//			fprintf(stderr, "EEPROM write to %08X = %08X\n", addr, val);
		}
		else // Outport
		{   
//			Memory::setLong(addr, val);
//			fprintf(stderr, "Commlink Outport Long write\n");
		}
		break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			Memory::setLong(addr, val);
//			fprintf(stderr, "Commlink Status Flag write\n");
		}
		else // Inport for Commlink
		{   
//			Memory::setLong(addr, val);
//			fprintf(stderr, "Commlink Inport Long write\n");
		}
		break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		Cs0Area->drammi->WriteLong(Cs0Area->dramarea, addr, val);
		break;
	default:   // The rest doesn't matter
		break;
	}
*/
}

u32 FASTCALL Cs0ReadLong(u32 addr) {
/*
	u32 val = 0xFFFFFFFF;

	switch (addr >> 20) {
	case 0x00:  
		if ((addr & 0x80000) == 0) // EEPROM
		{
			val = Cs0Area->biosmi->ReadLong(Cs0Area->biosarea, addr);
//			fprintf(stderr, "EEPROM read from %08X = %08X\n", addr, val);
		}
		else // Outport
		{   
//			val = Memory::getLong(addr);
//			fprintf(stderr, "Commlink Outport Long read\n");
		}
		break;
	case 0x01:
		if ((addr & 0x80000) == 0) // Commlink Status flag
		{
//			val = Memory::getLong(addr);
//			fprintf(stderr, "Commlink Status Flag read\n");
		}
		else // Inport for Commlink
		{   
//			val = Memory::getLong(addr);
//			fprintf(stderr, "Commlink Inport Long read\n");
		}
		break;
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07: // Ram cart area
		val = Cs0Area->drammi->ReadLong(Cs0Area->dramarea, addr);
		break;
	case 0x12:
	case 0x1E:
		if (0x80000)
			val = 0xFFFDFFFD;
		break;
	case 0x13:
	case 0x16:
	case 0x17:
	case 0x1A:
	case 0x1B:
	case 0x1F:
		val = 0xFFFDFFFD;
		break;
	default:   // The rest doesn't matter
		break;
	}

	return val;
*/

   return 0;
}

int Cs0SaveState(FILE * fp) {
/*
	int offset;

	offset = StateWriteHeader(fp, "CS0 ", 1);

	// Write cart type
	fwrite((void *) &Cs0Area->carttype, 4, 1, fp);

	// Write the areas associated with the cart type here

	return StateFinishHeader(fp, offset);
*/
   return 0;
}

int Cs0LoadState(FILE * fp, int version, int size) {
/*
	int oldtype = Cs0Area->carttype;
	// Read cart type
	fread((void *) &Cs0Area->carttype, 4, 1, fp);

	// Check to see if old cart type and new cart type match, if they don't,
	// reallocate memory areas

	// Read the areas associated with the cart type here

	return size;
*/
   return 0;
}
