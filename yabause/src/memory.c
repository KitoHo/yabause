#include "memory.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

////////////////////////////////////////////////////////////////

T1Memory * T1MemoryInit(u32 size) {
	T1Memory * mem;

	mem = (T1Memory *) malloc(sizeof(T1Memory));
	if (mem == NULL)
		return NULL;

	mem->mem = (u8 *) malloc(sizeof(u8) * size);
	if (mem->mem == NULL)
		return NULL;

	return mem;
}

////////////////////////////////////////////////////////////////

void T1MemoryDeInit(T1Memory * mem) {
	free(mem->mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

T2Memory * T2MemoryInit(u32 size) {
	T2Memory * mem;

	mem = (T2Memory *) malloc(sizeof(T2Memory));
	if (mem == NULL)
		return NULL;

	mem->mem = (u8 *) malloc(sizeof(u8) * size);
	if (mem->mem == NULL)
		return NULL;

	return mem;
}

////////////////////////////////////////////////////////////////

void T2MemoryDeInit(T2Memory * mem) {
	free(mem->mem);
	free(mem);
}

////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size) {
	T3Memory * mem;

	mem = (T3Memory *) malloc(sizeof(T3Memory));
	if (mem == NULL)
		return NULL;

	mem->base_mem = (u8 *) malloc(sizeof(u8) * size);
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
