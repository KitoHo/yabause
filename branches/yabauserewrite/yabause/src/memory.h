#ifndef MEMORY_H
#define MEMORY_H

#include "core.h"

/* Type 1 Memory, faster for byte (8 bits) accesses */

typedef struct {
	u8 * mem;
	u32 size;
} T1Memory;

T1Memory * T1MemoryNew(u32);
void T1MemoryDelete(T1Memory *);

FASTCALL u8	T1ReadByte(T1Memory *, u32);
FASTCALL u16	T1ReadWord(T1Memory *, u32);
FASTCALL u32	T1ReadLong(T1Memory *, u32);

FASTCALL void	T1WriteByte(T1Memory *, u32 , u8);
FASTCALL void	T1WriteWord(T1Memory *, u32 , u16);
FASTCALL void	T1WriteLong(T1Memory *, u32 , u32);

int T1MemoryLoad(T1Memory *, const char *, u32);
int T1MemorySave(T1Memory *, const char *, u32, u32);

/* Type 2 Memory, faster for word (16 bits) accesses */

typedef struct {
	u8 * mem;
	u32 size;
} T2Memory;

T2Memory * T2MemoryNew(u32);
void T2MemoryDelete(T2Memory *);

FASTCALL u8	T2ReadByte(T2Memory *, u32);
FASTCALL u16	T2ReadWord(T2Memory *, u32);
FASTCALL u32	T2ReadLong(T2Memory *, u32);

FASTCALL void	T2WriteByte(T2Memory *, u32 , u8);
FASTCALL void	T2WriteWord(T2Memory *, u32 , u16);
FASTCALL void	T2WriteLong(T2Memory *, u32 , u32);

int T2MemoryLoad(T2Memory *, const char *, u32);
int T2MemorySave(T2Memory *, const char *, u32, u32);

/* Type 3 Memory, faster for long (32 bits) accesses */

typedef struct {
	u8 * base_mem;
	u8 * mem;
	u32 size;
} T3Memory;

T3Memory * T3MemoryNew(u32);
void T3MemoryDelete(T3Memory *);

FASTCALL u8	T3ReadByte(T3Memory *, u32);
FASTCALL u16	T3ReadWord(T3Memory *, u32);
FASTCALL u32	T3ReadLong(T3Memory *, u32);

FASTCALL void	T3WriteByte(T3Memory *, u32 , u8);
FASTCALL void	T3WriteWord(T3Memory *, u32 , u16);
FASTCALL void	T3WriteLong(T3Memory *, u32 , u32);

int T3MemoryLoad(T3Memory *, const char *, u32);
int T3MemorySave(T3Memory *, const char *, u32, u32);

/* Dummy memory, always returns 0 */

typedef void Dummy;

Dummy * DummyNew(u32);
void DummyDelete(Dummy *);

FASTCALL u8	DummyReadByte(Dummy *, u32);
FASTCALL u16	DummyReadWord(Dummy *, u32);
FASTCALL u32	DummyReadLong(Dummy *, u32);

FASTCALL void	DummyWriteByte(Dummy *, u32 , u8);
FASTCALL void	DummyWriteWord(Dummy *, u32 , u16);
FASTCALL void	DummyWriteLong(Dummy *, u32 , u32);

#endif
