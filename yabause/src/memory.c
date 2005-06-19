#include "memory.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

T1Memory * T1MemoryNew(u32 size) {
	T1Memory * mem;

	mem = (T1Memory *) malloc(sizeof(T1Memory));
	mem->size = size;
	mem->mem = (u8 *) malloc(sizeof(u8) * size);

	return mem;
}

////////////////////////////////////////////////////////////////

void T1MemoryDelete(T1Memory * mem) {
	free(mem->mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 T1ReadByte(T1Memory * mem, u32 addr) {
	return mem->mem[addr];
}

////////////////////////////////////////////////////////////////

FASTCALL u16 T1ReadWord(T1Memory * mem, u32 addr) {
	return (mem->mem[addr] << 8) | mem->mem[addr + 1];
}

////////////////////////////////////////////////////////////////

FASTCALL u32 T1ReadLong(T1Memory * mem, u32 addr)  {
	return (mem->mem[addr] << 24 | mem->mem[addr + 1] << 16 | mem->mem[addr + 2] << 8 | mem->mem[addr + 3]);
}

////////////////////////////////////////////////////////////////

FASTCALL void T1WriteByte(T1Memory * mem, u32 addr, u8 val)  {
	mem->mem[addr] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void T1WriteWord(T1Memory * mem, u32 addr, u16 val)  {
	mem->mem[addr] = val >> 8;
	mem->mem[addr + 1] = val & 0xFF;
}

////////////////////////////////////////////////////////////////

FASTCALL void T1WriteLong(T1Memory * mem, u32 addr, u32 val)  {
	mem->mem[addr] = val >> 24;
	mem->mem[addr + 1] = (val >> 16) & 0xFF;
	mem->mem[addr + 2] = (val >> 8) & 0xFF;
	mem->mem[addr + 3] = val & 0xFF;
}

////////////////////////////////////////////////////////////////

int T1MemoryLoad(T1Memory * mem, const char * filename, u32 addr) {
	struct stat infos;
	u32 i;
	u8 * buffer;
	FILE * exe;

	if (stat(filename, &infos) == -1) {
		LOG("file not found: %s\n", filename);
		return -1;
	}

	buffer = (u8 *) malloc(sizeof(u8) * infos.st_size);

	exe = fopen(filename, "r");
	fread(buffer, 1, infos.st_size, exe);
	for(i = addr;i < (infos.st_size + addr);i++) {
		T1WriteByte(mem, i, buffer[i]);
	}
	fclose(exe);

	free(buffer);
	return 0;
}

////////////////////////////////////////////////////////////////

int T1MemorySave(T1Memory * mem, const char * filename, u32 addr, u32 size) {
        u32 i;
        u8 * buffer;
	FILE * exe;

	buffer = (u8 *) malloc(sizeof(u8) * size);

	exe = fopen(filename, "w");
        for(i = addr;i < (addr + size);i++) {
                buffer[i] = T1ReadByte(mem, i);
        }
        fwrite(buffer, 1, size, exe);
        fclose(exe);

	free(buffer);
}

////////////////////////////////////////////////////////////////

T2Memory * T2MemoryNew(u32 size) {
	T2Memory * mem;

	mem = (T2Memory *) malloc(sizeof(T2Memory));
	mem->size = size;
	mem->mem = (u8 *) malloc(sizeof(u8) * size);

	return mem;
}

////////////////////////////////////////////////////////////////

void T2MemoryDelete(T2Memory * mem) {
	free(mem->mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 T2ReadByte(T2Memory * mem, u32 addr) {
	return mem->mem[addr ^ 1];
}

////////////////////////////////////////////////////////////////

FASTCALL u16 T2ReadWord(T2Memory * mem, u32 addr) {
	return *((u16 *) (mem->mem + addr));
}

////////////////////////////////////////////////////////////////

FASTCALL u32 T2ReadLong(T2Memory * mem, u32 addr)  {
	return *((u16 *) (mem->mem + addr)) << 16 | *((u16 *) (mem->mem + addr + 2));
}

////////////////////////////////////////////////////////////////

FASTCALL void T2WriteByte(T2Memory * mem, u32 addr, u8 val)  {
	mem->mem[addr ^ 1] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void T2WriteWord(T2Memory * mem, u32 addr, u16 val)  {
	*((u16 *) (mem->mem + addr)) = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void T2WriteLong(T2Memory * mem, u32 addr, u32 val)  {
	*((u16 *) (mem->mem + addr)) = val >> 16;
	*((u16 *) (mem->mem + addr + 2)) = val & 0xFFFF;
}

////////////////////////////////////////////////////////////////

int T2MemoryLoad(T2Memory * mem, const char * filename, u32 addr) {
	struct stat infos;
	u32 i;
	u8 * buffer;
	FILE * exe;

	if (stat(filename, &infos) == -1) {
		LOG("file not found: %s\n", filename);
		return -1;
	}

	buffer = (u8 *) malloc(sizeof(u8) * infos.st_size);

	exe = fopen(filename, "r");
	fread(buffer, 1, infos.st_size, exe);
	for(i = addr;i < (infos.st_size + addr);i++) {
		T2WriteByte(mem, i, buffer[i]);
	}
	fclose(exe);

	free(buffer);
	return 0;
}

////////////////////////////////////////////////////////////////

int T2MemorySave(T2Memory * mem, const char * filename, u32 addr, u32 size) {
        u32 i;
        u8 * buffer;
	FILE * exe;

	buffer = (u8 *) malloc(sizeof(u8) * size);

	exe = fopen(filename, "w");
        for(i = addr;i < (addr + size);i++) {
                buffer[i] = T2ReadByte(mem, i);
        }
        fwrite(buffer, 1, size, exe);
        fclose(exe);

	free(buffer);
}

////////////////////////////////////////////////////////////////

T3Memory * T3MemoryNew(u32 size) {
	T3Memory * mem;

	mem = (T3Memory *) malloc(sizeof(T3Memory));
	mem->size = size;
	mem->base_mem = (u8 *) malloc(sizeof(u8) * size);
	mem->mem = mem->base_mem + size;

	return mem;
}

////////////////////////////////////////////////////////////////

void T3MemoryDelete(T3Memory * mem) {
	free(mem->base_mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

FASTCALL u8 T3ReadByte(T3Memory * mem, u32 addr) {
	return (mem->mem - addr - 1)[0];
}

////////////////////////////////////////////////////////////////

FASTCALL u16 T3ReadWord(T3Memory * mem, u32 addr) {
	return ((u16 *) (mem->mem - addr - 2))[0];
}

////////////////////////////////////////////////////////////////

FASTCALL u32 T3ReadLong(T3Memory * mem, u32 addr)  {
	return ((u32 *) (mem->mem - addr - 4))[0];
}

////////////////////////////////////////////////////////////////

FASTCALL void T3WriteByte(T3Memory * mem, u32 addr, u8 val)  {
	(mem->mem - addr - 1)[0] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void T3WriteWord(T3Memory * mem, u32 addr, u16 val)  {
	((u16 *) (mem->mem - addr - 2))[0] = val;
}

////////////////////////////////////////////////////////////////

FASTCALL void T3WriteLong(T3Memory * mem, u32 addr, u32 val)  {
	((u32 *) (mem->mem - addr - 4))[0] = val;
}

////////////////////////////////////////////////////////////////

int T3MemoryLoad(T3Memory * mem, const char * filename, u32 addr) {
	struct stat infos;
	u32 i;
	u8 * buffer;
	FILE * exe;

	if (stat(filename, &infos) == -1) {
		LOG("file not found: %s\n", filename);
		return -1;
	}

	buffer = (u8 *) malloc(sizeof(u8) * infos.st_size);

	exe = fopen(filename, "r");
	fread(buffer, 1, infos.st_size, exe);
	for(i = addr;i < (infos.st_size + addr);i++) {
		T3WriteByte(mem, i, buffer[i]);
	}
	fclose(exe);

	free(buffer);
	return 0;
}

////////////////////////////////////////////////////////////////

int T3MemorySave(T3Memory * mem, const char * filename, u32 addr, u32 size) {
        u32 i;
        u8 * buffer;
	FILE * exe;

	buffer = (u8 *) malloc(sizeof(u8) * size);

	exe = fopen(filename, "w");
        for(i = addr;i < (addr + size);i++) {
                buffer[i] = T3ReadByte(mem, i);
        }
        fwrite(buffer, 1, size, exe);
        fclose(exe);

	free(buffer);
}
