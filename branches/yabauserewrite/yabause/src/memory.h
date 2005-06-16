#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

typedef struct {
	u8 * base_mem;
	u8 * mem;
	u32 size;
} Memory;

Memory * MemoryNew(u32);
void MemoryDelete(Memory *);

FASTCALL u8	ReadByte(Memory *, u32);
FASTCALL u16	ReadWord(Memory *, u32);
FASTCALL u32	ReadLong(Memory *, u32);

FASTCALL void	WriteByte(Memory *, u32 , u8);
FASTCALL void	WriteWord(Memory *, u32 , u16);
FASTCALL void	WriteLong(Memory *, u32 , u32);

#endif
