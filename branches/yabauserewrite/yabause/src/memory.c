/*  Copyright 2005 Guillaume Duhamel
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

#include "memory.h"
#include "debug.h"
#include "sh2core.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////

typedef FASTCALL void (*writebytefunc)(u32, u8);
typedef FASTCALL void (*writewordfunc)(u32, u16);
typedef FASTCALL void (*writelongfunc)(u32, u32);

typedef FASTCALL u8 (*readbytefunc)(u32);
typedef FASTCALL u16 (*readwordfunc)(u32);
typedef FASTCALL u32 (*readlongfunc)(u32);

static writebytefunc WriteByteList[0x1000];
static writewordfunc WriteWordList[0x1000];
static writelongfunc WriteLongList[0x1000];

static readbytefunc ReadByteList[0x1000];
static readwordfunc ReadWordList[0x1000];
static readlongfunc ReadLongList[0x1000];

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;

////////////////////////////////////////////////////////////////

u8 * T1MemoryInit(u32 size) {
   unsigned char *mem;
   if ((mem = (u8 *) calloc(size, sizeof(u8))) == NULL)
      return NULL;
   return mem;
}

////////////////////////////////////////////////////////////////

void T1MemoryDeInit(u8 * mem) {
   free(mem);
}

////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size) {
	T3Memory * mem;

	mem = (T3Memory *) malloc(sizeof(T3Memory));
	if (mem == NULL)
		return NULL;

        mem->base_mem = (u8 *) calloc(size, sizeof(u8));
	if (mem->base_mem == NULL)
		return NULL;

	mem->mem = mem->base_mem + size;

	return mem;
}

////////////////////////////////////////////////////////////////

void T3MemoryDeInit(T3Memory * mem) {
	free(mem->base_mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

Dummy * DummyInit(u32 s) { return NULL; }
void DummyDeInit(Dummy * d) {}

////////////////////////////////////////////////////////////////

FASTCALL u8 UnhandledMemoryReadByte(u32 addr) {
   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL u16 UnhandledMemoryReadWord(u32 addr) {
   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL u32 UnhandledMemoryReadLong(u32 addr)  {
   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL void UnhandledMemoryWriteByte(u32 addr, u8 val)  {
}

////////////////////////////////////////////////////////////////

FASTCALL void UnhandledMemoryWriteWord(u32 addr, u16 val)  {
}

////////////////////////////////////////////////////////////////

FASTCALL void UnhandledMemoryWriteLong(u32 addr, u32 val)  {
}

////////////////////////////////////////////////////////////////

FASTCALL u8 HighWramMemoryReadByte(u32 addr) {   
   return T2ReadByte(HighWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u16 HighWramMemoryReadWord(u32 addr) {
   return T2ReadWord(HighWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u32 HighWramMemoryReadLong(u32 addr)  {
   return T2ReadLong(HighWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL void HighWramMemoryWriteByte(u32 addr, u8 val)  {
   T2WriteByte(HighWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL void HighWramMemoryWriteWord(u32 addr, u16 val)  {
   T2WriteWord(HighWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL void HighWramMemoryWriteLong(u32 addr, u32 val)  {
   T2WriteLong(HighWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 LowWramMemoryReadByte(u32 addr) {   
   return T2ReadByte(LowWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u16 LowWramMemoryReadWord(u32 addr) {
   return T2ReadWord(LowWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u32 LowWramMemoryReadLong(u32 addr)  {
   return T2ReadLong(LowWram, addr & 0xFFFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL void LowWramMemoryWriteByte(u32 addr, u8 val)  {
   T2WriteByte(LowWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL void LowWramMemoryWriteWord(u32 addr, u16 val)  {
   T2WriteWord(LowWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL void LowWramMemoryWriteLong(u32 addr, u32 val)  {
   T2WriteLong(LowWram, addr & 0xFFFFF, val);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 BiosRomMemoryReadByte(u32 addr) {   
   return T2ReadByte(BiosRom, addr & 0x7FFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u16 BiosRomMemoryReadWord(u32 addr) {
   return T2ReadWord(BiosRom, addr & 0x7FFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL u32 BiosRomMemoryReadLong(u32 addr)  {
   return T2ReadLong(BiosRom, addr & 0x7FFFF);
}

////////////////////////////////////////////////////////////////

FASTCALL void BiosRomMemoryWriteByte(u32 addr, u8 val)  {
   // read-only
}

////////////////////////////////////////////////////////////////

FASTCALL void BiosRomMemoryWriteWord(u32 addr, u16 val)  {
   // read-only
}

////////////////////////////////////////////////////////////////

FASTCALL void BiosRomMemoryWriteLong(u32 addr, u32 val)  {
   // read-only
}

////////////////////////////////////////////////////////////////

void FillMemoryArea(unsigned short start, unsigned short end,
                    readbytefunc r8func, readwordfunc r16func,
                    readlongfunc r32func, writebytefunc w8func,
                    writewordfunc w16func, writelongfunc w32func)
{
   int i;

   for (i=start; i < (end+1); i++)
   {
      ReadByteList[i] = r8func;
      ReadWordList[i] = r16func;
      ReadLongList[i] = r32func;
      WriteByteList[i] = w8func;
      WriteWordList[i] = w16func;
      WriteLongList[i] = w32func;
   }
}

////////////////////////////////////////////////////////////////

void MappedMemoryInit() {
   // Initialize everyting to unhandled to begin with
   FillMemoryArea(0x000, 0xFFF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &UnhandledMemoryWriteWord,
                                &UnhandledMemoryWriteLong);

   // Fill the rest
   FillMemoryArea(0x000, 0x00F, &BiosRomMemoryReadByte,
                                &BiosRomMemoryReadWord,
                                &BiosRomMemoryReadLong,
                                &BiosRomMemoryWriteByte,
                                &BiosRomMemoryWriteWord,
                                &BiosRomMemoryWriteLong);
   FillMemoryArea(0x020, 0x02F, &LowWramMemoryReadByte,
                                &LowWramMemoryReadWord,
                                &LowWramMemoryReadLong,
                                &LowWramMemoryWriteByte,
                                &LowWramMemoryWriteWord,
                                &LowWramMemoryWriteLong);
   FillMemoryArea(0x600, 0x7FF, &HighWramMemoryReadByte,
                                &HighWramMemoryReadWord,
                                &HighWramMemoryReadLong,
                                &HighWramMemoryWriteByte,
                                &HighWramMemoryWriteWord,
                                &HighWramMemoryWriteLong);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 MappedMemoryReadByte(u32 addr) {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return (*ReadByteList[(addr >> 16) & 0xFFF])(addr);
      }
      case 0x2:
      {
         // Purge Area
         break;
      }
      case 0x3:
      {
         // Address Array
         break;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         break;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadByte(addr);
      }
   }

   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL u16 MappedMemoryReadWord(u32 addr) {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return (*ReadWordList[(addr >> 16) & 0xFFF])(addr);
      }
      case 0x2:
      {
         // Purge Area
         break;
      }
      case 0x3:
      {
         // Address Array
         break;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         break;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadWord(addr);
      }
   }

   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL u32 MappedMemoryReadLong(u32 addr)  {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return (*ReadLongList[(addr >> 16) & 0xFFF])(addr);
      }
      case 0x2:
      {
         // Purge Area
         break;
      }
      case 0x3:
      {
         // Address Array
         break;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         break;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadLong(addr);
      }
   }
   return 0;
}

////////////////////////////////////////////////////////////////

FASTCALL void MappedMemoryWriteByte(u32 addr, u8 val)  {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         (*WriteByteList[(addr >> 16) & 0xFFF])(addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         return;
      }
      case 0x3:
      {
         // Address Array
         return;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipWriteByte(addr, val);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteByte(addr, val);
         return;
      }
   }
}

////////////////////////////////////////////////////////////////

FASTCALL void MappedMemoryWriteWord(u32 addr, u16 val)  {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         (*WriteWordList[(addr >> 16) & 0xFFF])(addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         return;
      }
      case 0x3:
      {
         // Address Array
         return;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipWriteWord(addr, val);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteWord(addr, val);
         return;
      }
   }
}

////////////////////////////////////////////////////////////////

FASTCALL void MappedMemoryWriteLong(u32 addr, u32 val)  {
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         (*WriteLongList[(addr >> 16) & 0xFFF])(addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         return;
      }
      case 0x3:
      {
         // Address Array
         return;
      }
      case 0x4:
      case 0x6:
      {
         // Data Array
         return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipWriteLong(addr, val);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteLong(addr, val);
         return;
      }
   }
}

////////////////////////////////////////////////////////////////

int LoadBios(const char *filename) {
   FILE *fp;
   unsigned long filesize;
   unsigned char *buffer;
   unsigned long i;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Calculate file size
   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   // Make sure bios size is valid
   if (filesize != 0x80000)
   {
      fclose(fp);
      return -2;
   }

   if ((buffer = (unsigned char *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -3;
   }

   fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   for (i = 0; i < filesize; i++)
      T2WriteByte(BiosRom, i, buffer[i]);

   free(buffer);

   return 0;
}

////////////////////////////////////////////////////////////////

int LoadBackupRam(const char *filename) {
   return 0;
}

////////////////////////////////////////////////////////////////

