#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

/* Type 1 Memory, faster for byte (8 bits) accesses */

u8 * T1MemoryInit(u32);
void T1MemoryDeInit(u8 *);

static inline u8 T1ReadByte(u8 * mem, u32 addr) {
        return mem[addr];
}

static inline u16 T1ReadWord(u8 * mem, u32 addr) {
        return (mem[addr] << 8) | mem[addr + 1];
}

static inline u32 T1ReadLong(u8 * mem, u32 addr)  {
        return (mem[addr] << 24 | mem[addr + 1] << 16 |
                mem[addr + 2] << 8 | mem[addr + 3]);
}

static inline void T1WriteByte(u8 * mem, u32 addr, u8 val)  {
        mem[addr] = val;
}

static inline void T1WriteWord(u8 * mem, u32 addr, u16 val)  {
        mem[addr] = val >> 8;
        mem[addr + 1] = val & 0xFF;
}

static inline void T1WriteLong(u8 * mem, u32 addr, u32 val)  {
        mem[addr] = val >> 24;
        mem[addr + 1] = (val >> 16) & 0xFF;
        mem[addr + 2] = (val >> 8) & 0xFF;
        mem[addr + 3] = val & 0xFF;
}

/* Type 2 Memory, faster for word (16 bits) accesses */

#define T2MemoryInit(x) (T1MemoryInit(x))
#define T2MemoryDeInit(x) (T1MemoryDeInit(x))

static inline u8 T2ReadByte(u8 * mem, u32 addr) {
        return mem[addr ^ 1];
}

static inline u16 T2ReadWord(u8 * mem, u32 addr) {
        return *((u16 *) (mem + addr));
}

static inline u32 T2ReadLong(u8 * mem, u32 addr)  {
        return *((u16 *) (mem + addr)) << 16 | *((u16 *) (mem + addr + 2));
}

static inline void T2WriteByte(u8 * mem, u32 addr, u8 val)  {
        mem[addr ^ 1] = val;
}

static inline void T2WriteWord(u8 * mem, u32 addr, u16 val)  {
        *((u16 *) (mem + addr)) = val;
}

static inline void T2WriteLong(u8 * mem, u32 addr, u32 val)  {
        *((u16 *) (mem + addr)) = val >> 16;
        *((u16 *) (mem + addr + 2)) = val & 0xFFFF;
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

void MappedMemoryInit();
u8 FASTCALL MappedMemoryReadByte(u32 addr);
u16 FASTCALL MappedMemoryReadWord(u32 addr);
u32 FASTCALL MappedMemoryReadLong(u32 addr);
void FASTCALL MappedMemoryWriteByte(u32 addr, u8 val);
void FASTCALL MappedMemoryWriteWord(u32 addr, u16 val);
void FASTCALL MappedMemoryWriteLong(u32 addr, u32 val);

extern u8 *HighWram;
extern u8 *LowWram;
extern u8 *BiosRom;

int LoadBios(const char *filename);
int LoadBackupRam(const char *filename);

#endif
