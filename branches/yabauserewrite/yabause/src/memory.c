#include "memory.h"

#include <stdlib.h>

Memory * MemoryNew(u32 size) {
	Memory * mem;

	mem = (Memory *) malloc(sizeof(Memory));
	mem->size = size;
	mem->base_mem = (u8 *) malloc(sizeof(u8) * size);
	mem->mem = mem->base_mem + size;

	return mem;
}

void MemoryDelete(Memory * mem) {
	free(mem->base_mem);
	free(mem);
}

FASTCALL u8 ReadByte(Memory * mem, u32 addr) {
	return (mem->mem - addr - 1)[0];
}

FASTCALL u16 ReadWord(Memory * mem, u32 addr) {
	return ((u16 *) (mem->mem - addr - 2))[0];
}

FASTCALL u32 ReadLong(Memory * mem, u32 addr)  {
	return ((u32 *) (mem->mem - addr - 4))[0];
}

FASTCALL void WriteByte(Memory * mem, u32 addr, u8 val)  {
	(mem->mem - addr - 1)[0] = val;
}

FASTCALL void WriteWord(Memory * mem, u32 addr, u16 val)  {
	((u16 *) (mem->mem - addr - 2))[0] = val;
}

FASTCALL void WriteLong(Memory * mem, u32 addr, u32 val)  {
	((u32 *) (mem->mem - addr - 4))[0] = val;
}
