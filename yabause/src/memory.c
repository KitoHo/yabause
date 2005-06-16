#include "memory.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

Memory * MemoryNew(u32 size) {
	Memory * mem;

	mem = (Memory *) malloc(sizeof(Memory));
	mem->size = size;
	mem->base_mem = (u8 *) malloc(sizeof(u8) * size);
	mem->mem = mem->base_mem + size;

	return mem;
}

////////////////////////////////////////////////////////////////

void MemoryDelete(Memory * mem) {
	free(mem->base_mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 ReadByte(Memory * mem, u32 addr) {
	return (mem->mem - addr - 1)[0];
}

////////////////////////////////////////////////////////////////

FASTCALL u16 ReadWord(Memory * mem, u32 addr) {
	return ((u16 *) (mem->mem - addr - 2))[0];
}

////////////////////////////////////////////////////////////////

FASTCALL u32 ReadLong(Memory * mem, u32 addr)  {
	return ((u32 *) (mem->mem - addr - 4))[0];
}

////////////////////////////////////////////////////////////////

FASTCALL void WriteByte(Memory * mem, u32 addr, u8 val)  {
	(mem->mem - addr - 1)[0] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void WriteWord(Memory * mem, u32 addr, u16 val)  {
	((u16 *) (mem->mem - addr - 2))[0] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void WriteLong(Memory * mem, u32 addr, u32 val)  {
	((u32 *) (mem->mem - addr - 4))[0] = val;
}

////////////////////////////////////////////////////////////////

int MemoryLoad(Memory * mem, const char * filename, u32 addr) {
	struct stat infos;
	u32 i;
	u8 buffer;
	FILE * exe;

	if (stat(filename, &infos) == -1) {
		LOG("file not found: %s\n", filename);
		return -1;
	}

	exe = fopen(filename, "r");
	for(i = addr;i < (infos.st_size + addr);i++) {
		fread(&buffer, 1, 1, exe);
		WriteByte(mem, i, buffer);
	}
	fclose(exe);
	return 0;
}

////////////////////////////////////////////////////////////////

int MemorySave(Memory * mem, const char * filename, u32 addr, u32 size) {
        u32 i;
        u8 buffer;
	FILE * exe;

	exe = fopen(filename, "w");
        for(i = addr;i < (addr + size);i++) {
                buffer = ReadByte(mem, i);
                fwrite(&buffer, 1, 1, exe);
        }
        fclose(exe);
}
