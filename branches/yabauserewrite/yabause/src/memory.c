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

////////////////////////////////////////////////////////////////

u8 MappedMemoryReadByte(u32 addr) {
}

////////////////////////////////////////////////////////////////

u16 MappedMemoryReadWord(u32 addr) {
}

////////////////////////////////////////////////////////////////

u32 MappedMemoryReadLong(u32 addr)  {
}

////////////////////////////////////////////////////////////////

void MappedMemoryWriteByte(u32 addr, u8 val)  {
}

////////////////////////////////////////////////////////////////

void MappedMemoryWriteWord(u32 addr, u16 val)  {
}

////////////////////////////////////////////////////////////////

void MappedMemoryWriteLong(u32 addr, u32 val)  {
}

////////////////////////////////////////////////////////////////

