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

cartridge_struct *CartridgeArea;

//////////////////////////////////////////////////////////////////////////////
// Dummy/No Cart Functions
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DummyCs0ReadByte(u32 addr)
{
   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DummyCs0ReadWord(u32 addr)
{
   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DummyCs0ReadLong(u32 addr)
{
   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs0WriteByte(u32 addr, u8 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs0WriteWord(u32 addr, u16 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs0WriteLong(u32 addr, u32 val)
{
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DummyCs1ReadByte(u32 addr)
{
   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DummyCs1ReadWord(u32 addr)
{
   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DummyCs1ReadLong(u32 addr)
{
   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs1WriteByte(u32 addr, u8 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs1WriteWord(u32 addr, u16 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs1WriteLong(u32 addr, u32 val)
{
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DummyCs2ReadByte(u32 addr)
{
   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DummyCs2ReadWord(u32 addr)
{
   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DummyCs2ReadLong(u32 addr)
{
   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs2WriteByte(u32 addr, u8 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs2WriteWord(u32 addr, u16 val)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DummyCs2WriteLong(u32 addr, u32 val)
{
}

//////////////////////////////////////////////////////////////////////////////
// Action Replay 4M Plus funcions
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL AR4MCs0ReadByte(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            return T2ReadByte(CartridgeArea->rom, addr);
//            return biosarea->getByte(addr);
//         else // Outport
//            fprintf(stderr, "Commlink Outport Byte read\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag read\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Byte read\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Dram area
         return T1ReadByte(CartridgeArea->dram, addr & 0x3FFFFF);
      default:   // The rest doesn't matter
         break;
   }

   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL AR4MCs0ReadWord(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            return T2ReadWord(CartridgeArea->rom, addr);
//         else // Outport
//            fprintf(stderr, "Commlink Outport Word read\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag read\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Word read\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         return T1ReadWord(CartridgeArea->dram, addr & 0x3FFFFF);
      case 0x12:
      case 0x1E:
         if (0x80000)
            return 0xFFFD;
         break;
      case 0x13:
      case 0x16:
      case 0x17:
      case 0x1A:
      case 0x1B:
      case 0x1F:
         return 0xFFFD;
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL AR4MCs0ReadLong(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            return T2ReadLong(CartridgeArea->rom, addr);
//         else // Outport
//            fprintf(stderr, "Commlink Outport Long read\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag read\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Long read\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         return T1ReadLong(CartridgeArea->dram, addr & 0x3FFFFF);
      case 0x12:
      case 0x1E:
         if (0x80000)
            return 0xFFFDFFFD;
         break;
      case 0x13:
      case 0x16:
      case 0x17:
      case 0x1A:
      case 0x1B:
      case 0x1F:
         return 0xFFFDFFFD;
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AR4MCs0WriteByte(u32 addr, u8 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            T2WriteByte(CartridgeArea->rom, addr, val);
//         else // Outport
//            fprintf(stderr, "Commlink Outport byte write\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag write\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Byte write\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteByte(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AR4MCs0WriteWord(u32 addr, u16 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            T2WriteWord(CartridgeArea->rom, addr, val);
//         else // Outport
//            fprintf(stderr, "Commlink Outport Word write\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag write\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Word write\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteWord(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AR4MCs0WriteLong(u32 addr, u32 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x00:
      {
         if ((addr & 0x80000) == 0) // EEPROM
            T2WriteLong(CartridgeArea->rom, addr, val);
//         else // Outport
//            fprintf(stderr, "Commlink Outport Long write\n");
         break;
      }
      case 0x01:
      {
//         if ((addr & 0x80000) == 0) // Commlink Status flag
//            fprintf(stderr, "Commlink Status Flag write\n");
//         else // Inport for Commlink
//            fprintf(stderr, "Commlink Inport Long write\n");
         break;
      }
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteLong(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////
// 8 Mbit Dram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DRAM8MBITCs0ReadByte(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         return T1ReadByte(CartridgeArea->dram, addr & 0x7FFFF);
      case 0x06: // Dram area
         return T1ReadByte(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
      default:   // The rest doesn't matter
         break;
   }

   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DRAM8MBITCs0ReadWord(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         return T1ReadWord(CartridgeArea->dram, addr & 0x7FFFF);
      case 0x06: // Dram area
         return T1ReadWord(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DRAM8MBITCs0ReadLong(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         return T1ReadLong(CartridgeArea->dram, addr & 0x7FFFF);
      case 0x06: // Dram area
         return T1ReadLong(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF));
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM8MBITCs0WriteByte(u32 addr, u8 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         T1WriteByte(CartridgeArea->dram, addr & 0x7FFFF, val);
         break;
      case 0x06: // Dram area
         T1WriteByte(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM8MBITCs0WriteWord(u32 addr, u16 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         T1WriteWord(CartridgeArea->dram, addr & 0x7FFFF, val);
         break;
      case 0x06: // Dram area
         T1WriteWord(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM8MBITCs0WriteLong(u32 addr, u32 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04: // Dram area
         T1WriteLong(CartridgeArea->dram, addr & 0x7FFFF, val);
         break;
      case 0x06: // Dram area
         T1WriteLong(CartridgeArea->dram, 0x80000 | (addr & 0x7FFFF), val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////
// 32 Mbit Dram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DRAM32MBITCs0ReadByte(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Dram area
         return T1ReadByte(CartridgeArea->dram, addr & 0x3FFFFF);
      default:   // The rest doesn't matter
         break;
   }

   return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DRAM32MBITCs0ReadWord(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         return T1ReadWord(CartridgeArea->dram, addr & 0x3FFFFF);
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFF;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DRAM32MBITCs0ReadLong(u32 addr)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         return T1ReadLong(CartridgeArea->dram, addr & 0x3FFFFF);
      default:   // The rest doesn't matter
         break;
   }

   return 0xFFFFFFFF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM32MBITCs0WriteByte(u32 addr, u8 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteByte(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM32MBITCs0WriteWord(u32 addr, u16 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteWord(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DRAM32MBITCs0WriteLong(u32 addr, u32 val)
{
   addr &= 0x1FFFFFF;

   switch (addr >> 20)
   {
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
         T1WriteLong(CartridgeArea->dram, addr & 0x3FFFFF, val);
         break;
      default:   // The rest doesn't matter
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////
// 4 Mbit Backup Ram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BUP4MBITCs1ReadByte(u32 addr)
{
   return T1ReadByte(CartridgeArea->bupram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BUP4MBITCs1ReadWord(u32 addr)
{
   return T1ReadWord(CartridgeArea->bupram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BUP4MBITCs1ReadLong(u32 addr)
{
   return T1ReadLong(CartridgeArea->bupram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP4MBITCs1WriteByte(u32 addr, u8 val)
{
   T1WriteByte(CartridgeArea->bupram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP4MBITCs1WriteWord(u32 addr, u16 val)
{
   T1WriteWord(CartridgeArea->bupram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP4MBITCs1WriteLong(u32 addr, u32 val)
{
   T1WriteLong(CartridgeArea->bupram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////
// 8 Mbit Backup Ram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BUP8MBITCs1ReadByte(u32 addr)
{
   return T1ReadByte(CartridgeArea->bupram, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BUP8MBITCs1ReadWord(u32 addr)
{
   return T1ReadWord(CartridgeArea->bupram, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BUP8MBITCs1ReadLong(u32 addr)
{
   return T1ReadLong(CartridgeArea->bupram, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP8MBITCs1WriteByte(u32 addr, u8 val)
{
   T1WriteByte(CartridgeArea->bupram, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP8MBITCs1WriteWord(u32 addr, u16 val)
{
   T1WriteWord(CartridgeArea->bupram, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP8MBITCs1WriteLong(u32 addr, u32 val)
{
   T1WriteLong(CartridgeArea->bupram, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////
// 16 Mbit Backup Ram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BUP16MBITCs1ReadByte(u32 addr)
{
   return T1ReadByte(CartridgeArea->bupram, addr & 0x3FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BUP16MBITCs1ReadWord(u32 addr)
{
   return T1ReadWord(CartridgeArea->bupram, addr & 0x3FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BUP16MBITCs1ReadLong(u32 addr)
{
   return T1ReadLong(CartridgeArea->bupram, addr & 0x3FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP16MBITCs1WriteByte(u32 addr, u8 val)
{
   T1WriteByte(CartridgeArea->bupram, addr & 0x3FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP16MBITCs1WriteWord(u32 addr, u16 val)
{
   T1WriteWord(CartridgeArea->bupram, addr & 0x3FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP16MBITCs1WriteLong(u32 addr, u32 val)
{
   T1WriteLong(CartridgeArea->bupram, addr & 0x3FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////
// 32 Mbit Backup Ram
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BUP32MBITCs1ReadByte(u32 addr)
{
   return T1ReadByte(CartridgeArea->bupram, addr & 0x7FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BUP32MBITCs1ReadWord(u32 addr)
{
   return T1ReadWord(CartridgeArea->bupram, addr & 0x7FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BUP32MBITCs1ReadLong(u32 addr)
{
   return T1ReadLong(CartridgeArea->bupram, addr & 0x7FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP32MBITCs1WriteByte(u32 addr, u8 val)
{
   T1WriteByte(CartridgeArea->bupram, addr & 0x7FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP32MBITCs1WriteWord(u32 addr, u16 val)
{
   T1WriteWord(CartridgeArea->bupram, addr & 0x7FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BUP32MBITCs1WriteLong(u32 addr, u32 val)
{
   T1WriteLong(CartridgeArea->bupram, addr & 0x7FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////
// 16 Mbit Rom
//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL ROM16MBITCs0ReadByte(u32 addr)
{
   return T1ReadByte(CartridgeArea->rom, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL ROM16MBITCs0ReadWord(u32 addr)
{
   return T1ReadWord(CartridgeArea->rom, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL ROM16MBITCs0ReadLong(u32 addr)
{
   return T1ReadLong(CartridgeArea->rom, addr & 0x1FFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ROM16MBITCs0WriteByte(u32 addr, u8 val)
{
   T1WriteByte(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ROM16MBITCs0WriteWord(u32 addr, u16 val)
{
   T1WriteWord(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ROM16MBITCs0WriteLong(u32 addr, u32 val)
{
   T1WriteLong(CartridgeArea->rom, addr & 0x1FFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////
// General Cart functions
//////////////////////////////////////////////////////////////////////////////

int CartInit(const char * filename, int type)
{
   if ((CartridgeArea = (cartridge_struct *)calloc(1, sizeof(cartridge_struct))) == NULL)
      return -1;

   CartridgeArea->carttype = type;
   CartridgeArea->filename = filename;

   switch(type)
   {
      case CART_PAR: // Action Replay 4M Plus(or equivalent)
      {
         if ((CartridgeArea->rom = T2MemoryInit(0x40000)) == NULL)
            return -1;

         if ((CartridgeArea->dram = T1MemoryInit(0x400000)) == NULL)
            return -1;

         // Use 32 Mbit Dram id
         CartridgeArea->cartid = 0x5C;

         // Load AR firmware to memory
         if (T123Load(CartridgeArea->rom, 0x40000, 2, filename) != 0)
            return -1;

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &AR4MCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &AR4MCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &AR4MCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &AR4MCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &AR4MCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &AR4MCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &DummyCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &DummyCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &DummyCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
      {
         if ((CartridgeArea->bupram = T1MemoryInit(0x100000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x21;

         // Load Backup Ram data from file
         if (T123Load(CartridgeArea->bupram, 0x100000, 1, filename) != 0)
            FormatBackupRam(CartridgeArea->bupram, 0x100000);

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DummyCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DummyCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DummyCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &BUP4MBITCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &BUP4MBITCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &BUP4MBITCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &BUP4MBITCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &BUP4MBITCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &BUP4MBITCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;

         break;
      }
      case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
      {
         if ((CartridgeArea->bupram = T1MemoryInit(0x200000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x22;

         // Load Backup Ram data from file
         if (T123Load(CartridgeArea->bupram, 0x200000, 1, filename) != 0)
            FormatBackupRam(CartridgeArea->bupram, 0x200000);

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DummyCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DummyCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DummyCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &BUP8MBITCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &BUP8MBITCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &BUP8MBITCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &BUP8MBITCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &BUP8MBITCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &BUP8MBITCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;

         break;
      }
      case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
      {
         if ((CartridgeArea->bupram = T1MemoryInit(0x400000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x23;

         // Load Backup Ram data from file
         if (T123Load(CartridgeArea->bupram, 0x400000, 1, filename) != 0)
            FormatBackupRam(CartridgeArea->bupram, 0x400000);

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DummyCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DummyCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DummyCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &BUP16MBITCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &BUP16MBITCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &BUP16MBITCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &BUP16MBITCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &BUP16MBITCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &BUP16MBITCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
      {
         if ((CartridgeArea->bupram = T1MemoryInit(0x800000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x24;

         // Load Backup Ram data from file
         if (T123Load(CartridgeArea->bupram, 0x800000, 1, filename) != 0)
            FormatBackupRam(CartridgeArea->bupram, 0x800000);

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DummyCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DummyCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DummyCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &BUP32MBITCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &BUP32MBITCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &BUP32MBITCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &BUP32MBITCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &BUP32MBITCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &BUP32MBITCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      case CART_DRAM8MBIT: // 8 Mbit Dram Cart
      {
         if ((CartridgeArea->dram = T1MemoryInit(0x100000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x5A;

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DRAM8MBITCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DRAM8MBITCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DRAM8MBITCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DRAM8MBITCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DRAM8MBITCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DRAM8MBITCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &DummyCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &DummyCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &DummyCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      case CART_DRAM32MBIT: // 32 Mbit Dram Cart
      {
         if ((CartridgeArea->dram = T1MemoryInit(0x400000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0x5C;

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DRAM32MBITCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DRAM32MBITCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DRAM32MBITCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DRAM32MBITCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DRAM32MBITCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DRAM32MBITCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &DummyCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &DummyCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &DummyCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      case CART_ROM16MBIT: // 16 Mbit Rom Cart
      {
         if ((CartridgeArea->rom = T1MemoryInit(0x200000)) == NULL)
            return -1;

         CartridgeArea->cartid = 0xFF; // I have no idea what the real id is

         // Load Rom to memory
         if (T123Load(CartridgeArea->rom, 0x200000, 1, filename) != 0)
            return -1;

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &ROM16MBITCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &ROM16MBITCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &ROM16MBITCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &ROM16MBITCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &ROM16MBITCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &ROM16MBITCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &DummyCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &DummyCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &DummyCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
      default: // No Cart
      {
         CartridgeArea->cartid = 0xFF;

         // Setup Functions
         CartridgeArea->Cs0ReadByte = &DummyCs0ReadByte;
         CartridgeArea->Cs0ReadWord = &DummyCs0ReadWord;
         CartridgeArea->Cs0ReadLong = &DummyCs0ReadLong;
         CartridgeArea->Cs0WriteByte = &DummyCs0WriteByte;
         CartridgeArea->Cs0WriteWord = &DummyCs0WriteWord;
         CartridgeArea->Cs0WriteLong = &DummyCs0WriteLong;

         CartridgeArea->Cs1ReadByte = &DummyCs1ReadByte;
         CartridgeArea->Cs1ReadWord = &DummyCs1ReadWord;
         CartridgeArea->Cs1ReadLong = &DummyCs1ReadLong;
         CartridgeArea->Cs1WriteByte = &DummyCs1WriteByte;
         CartridgeArea->Cs1WriteWord = &DummyCs1WriteWord;
         CartridgeArea->Cs1WriteLong = &DummyCs1WriteLong;

         CartridgeArea->Cs2ReadByte = &DummyCs2ReadByte;
         CartridgeArea->Cs2ReadWord = &DummyCs2ReadWord;
         CartridgeArea->Cs2ReadLong = &DummyCs2ReadLong;
         CartridgeArea->Cs2WriteByte = &DummyCs2WriteByte;
         CartridgeArea->Cs2WriteWord = &DummyCs2WriteWord;
         CartridgeArea->Cs2WriteLong = &DummyCs2WriteLong;
         break;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void CartDeInit(void)
{
   if (CartridgeArea)
   {
      if (CartridgeArea->carttype == CART_PAR)
      {
         if (CartridgeArea->rom)
            T2MemoryDeInit(CartridgeArea->rom);
      }
      else
      {
         if (CartridgeArea->rom)
            T1MemoryDeInit(CartridgeArea->rom);
      }

      if (CartridgeArea->bupram)
      {
         u32 size;

         switch (CartridgeArea->carttype)
         {
            case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
            {
               size = 0x100000;
               break;
            }
            case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
            {
               size = 0x200000;
               break;
            }
            case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
            {
               size = 0x400000;
               break;
            }
            case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
            {
               size = 0x800000;
               break;
            }
         }

         T123Save(CartridgeArea->bupram, size, 1, CartridgeArea->filename);
         T1MemoryDeInit(CartridgeArea->bupram);
      }

      if (CartridgeArea->dram)
         T1MemoryDeInit(CartridgeArea->dram);

      free(CartridgeArea);
   }
}

//////////////////////////////////////////////////////////////////////////////

int CartSaveState(FILE * fp)
{
/*
	int offset;

        offset = StateWriteHeader(fp, "Cs0 ", 1);

	// Write cart type
	fwrite((void *) &Cs0Area->carttype, 4, 1, fp);

	// Write the areas associated with the cart type here

	return StateFinishHeader(fp, offset);
*/
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CartLoadState(FILE * fp, int version, int size)
{
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

//////////////////////////////////////////////////////////////////////////////

