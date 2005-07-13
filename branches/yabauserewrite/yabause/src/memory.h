#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

/* Type 1 Memory, faster for byte (8 bits) accesses */

typedef struct {
	u8 * mem;
} T1Memory;

T1Memory * T1MemoryInit(u32);
void T1MemoryDeInit(T1Memory *);

static inline u8 T1ReadByte(T1Memory * mem, u32 addr) {
	return mem->mem[addr];
}

static inline u16 T1ReadWord(T1Memory * mem, u32 addr) {
	return (mem->mem[addr] << 8) | mem->mem[addr + 1];
}

static inline u32 T1ReadLong(T1Memory * mem, u32 addr)  {
	return (mem->mem[addr] << 24 | mem->mem[addr + 1] << 16 |
		mem->mem[addr + 2] << 8 | mem->mem[addr + 3]);
}

static inline void T1WriteByte(T1Memory * mem, u32 addr, u8 val)  {
	mem->mem[addr] = val;
}

static inline void T1WriteWord(T1Memory * mem, u32 addr, u16 val)  {
	mem->mem[addr] = val >> 8;
	mem->mem[addr + 1] = val & 0xFF;
}

static inline void T1WriteLong(T1Memory * mem, u32 addr, u32 val)  {
	mem->mem[addr] = val >> 24;
	mem->mem[addr + 1] = (val >> 16) & 0xFF;
	mem->mem[addr + 2] = (val >> 8) & 0xFF;
	mem->mem[addr + 3] = val & 0xFF;
}

/* Type 2 Memory, faster for word (16 bits) accesses */

typedef struct {
	u8 * mem;
} T2Memory;

T2Memory * T2MemoryInit(u32);
void T2MemoryDeInit(T2Memory *);

static inline u8 T2ReadByte(T2Memory * mem, u32 addr) {
	return mem->mem[addr ^ 1];
}

static inline u16 T2ReadWord(T2Memory * mem, u32 addr) {
	return *((u16 *) (mem->mem + addr));
}

static inline u32 T2ReadLong(T2Memory * mem, u32 addr)  {
	return *((u16 *) (mem->mem + addr)) << 16 | *((u16 *) (mem->mem + addr + 2));
}

static inline void T2WriteByte(T2Memory * mem, u32 addr, u8 val)  {
	mem->mem[addr ^ 1] = val;
}

static inline void T2WriteWord(T2Memory * mem, u32 addr, u16 val)  {
	*((u16 *) (mem->mem + addr)) = val;
}

static inline void T2WriteLong(T2Memory * mem, u32 addr, u32 val)  {
	*((u16 *) (mem->mem + addr)) = val >> 16;
	*((u16 *) (mem->mem + addr + 2)) = val & 0xFFFF;
}

/* Type 3 Memory, faster for long (32 bits) accesses */

typedef struct {
	u8 * base_mem;
	u8 * mem;
} T3Memory;

T3Memory * T3MemoryInit(u32);
void T3MemoryDeInit(T3Memory *);

static inline u8 T3ReadByte(T3Memory * mem, u32 addr) {
	return (mem->mem - addr - 1)[0];
}

static inline u16 T3ReadWord(T3Memory * mem, u32 addr) {
	return ((u16 *) (mem->mem - addr - 2))[0];
}

static inline u32 T3ReadLong(T3Memory * mem, u32 addr)  {
	return ((u32 *) (mem->mem - addr - 4))[0];
}

static inline void T3WriteByte(T3Memory * mem, u32 addr, u8 val)  {
	(mem->mem - addr - 1)[0] = val;
}

static inline void T3WriteWord(T3Memory * mem, u32 addr, u16 val)  {
	((u16 *) (mem->mem - addr - 2))[0] = val;
}

static inline void T3WriteLong(T3Memory * mem, u32 addr, u32 val)  {
	((u32 *) (mem->mem - addr - 4))[0] = val;
}

/* Dummy memory, always returns 0 */

typedef void Dummy;

Dummy * DummyInit(u32);
void DummyDeInit(Dummy *);

static inline u8 DummyReadByte(Dummy * d, u32 a) { return 0; }
static inline u16 DummyReadWord(Dummy * d, u32 a) { return 0; }
static inline u32 DummyReadLong(Dummy * d, u32 a) { return 0; }

static inline void DummyWriteByte(Dummy * d, u32 a, u8 v) {}
static inline void DummyWriteWord(Dummy * d, u32 a, u16 v) {}
static inline void DummyWriteLong(Dummy * d, u32 a, u32 v) {}

u8 MappedMemoryReadByte(u32 addr);
u16 MappedMemoryReadWord(u32 addr);
u32 MappedMemoryReadLong(u32 addr);
void MappedMemoryWriteByte(u32 addr, u8 val);
void MappedMemoryWriteWord(u32 addr, u16 val);
void MappedMemoryWriteLong(u32 addr, u32 val);

#endif
