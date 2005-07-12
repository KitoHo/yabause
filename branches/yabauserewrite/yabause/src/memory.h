#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

typedef struct {
	void *	(*New)(u32);
	void	(*Delete)(void *);
	u8	(*ReadByte)(void *, u32);
	u16	(*ReadWord)(void *, u32);
	u32	(*ReadLong)(void *, u32);
	void	(*WriteByte)(void *, u32, u8);
	void	(*WriteWord)(void *, u32, u16);
	void	(*WriteLong)(void *, u32, u32);
} MemoryInterface;

int MemoryLoad(MemoryInterface *, void *, const char *, u32);
int MemorySave(MemoryInterface *, void *, const char *, u32, u32);

/* Type 1 Memory, faster for byte (8 bits) accesses */

typedef struct {
	u8 * mem;
} T1Memory;

T1Memory * T1MemoryNew(u32);
void T1MemoryDelete(T1Memory *);

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

MemoryInterface * T1Interface(void);

int T1MemoryLoad(T1Memory *, const char *, u32);
int T1MemorySave(T1Memory *, const char *, u32, u32);

/* Type 2 Memory, faster for word (16 bits) accesses */

typedef struct {
	u8 * mem;
} T2Memory;

T2Memory * T2MemoryNew(u32);
void T2MemoryDelete(T2Memory *);

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

MemoryInterface * T2Interface(void);

int T2MemoryLoad(T2Memory *, const char *, u32);
int T2MemorySave(T2Memory *, const char *, u32, u32);

/* Type 3 Memory, faster for long (32 bits) accesses */

typedef struct {
	u8 * base_mem;
	u8 * mem;
} T3Memory;

T3Memory * T3MemoryNew(u32);
void T3MemoryDelete(T3Memory *);

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

MemoryInterface * T3Interface(void);

int T3MemoryLoad(T3Memory *, const char *, u32);
int T3MemorySave(T3Memory *, const char *, u32, u32);

/* Dummy memory, always returns 0 */

typedef void Dummy;

Dummy * DummyNew(u32);
void DummyDelete(Dummy *);

static inline u8 DummyReadByte(Dummy * d, u32 a) { return 0; }
static inline u16 DummyReadWord(Dummy * d, u32 a) { return 0; }
static inline u32 DummyReadLong(Dummy * d, u32 a) { return 0; }

static inline void DummyWriteByte(Dummy * d, u32 a, u8 v) {}
static inline void DummyWriteWord(Dummy * d, u32 a, u16 v) {}
static inline void DummyWriteLong(Dummy * d, u32 a, u32 v) {}

MemoryInterface * DummyInterface(void);

#endif
