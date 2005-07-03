#ifndef VDP1_H
#define VDP1_H

#include "memory.h"

extern T1Memory * Vdp1Ram;

FASTCALL u8	Vdp1RamReadByte(u32);
FASTCALL u16	Vdp1RamReadWord(u32);
FASTCALL u32	Vdp1RamReadLong(u32);
FASTCALL void	Vdp1RamWriteByte(u32, u8);
FASTCALL void	Vdp1RamWriteWord(u32, u16);
FASTCALL void	Vdp1RamWriteLong(u32, u32);

typedef struct {
	u16 TVHR;
	u16 FBCR;
	u16 PTMR;
	u16 EWDR;
	u16 EWLR;
	u16 EWRR;
	u16 ENDR;
	u16 EDSR;
	u16 LOPR;
	u16 COPR;
	u16 MODR;
} Vdp1;

extern Vdp1 * Vdp1Regs;

void Vdp1New(void);
void Vdp1Delete(void);

void Vdp1Reset(void);

FASTCALL u8	Vdp1ReadByte(u32);
FASTCALL u16	Vdp1ReadWord(u32);
FASTCALL u32	Vdp1ReadLong(u32);
FASTCALL void	Vdp1WriteByte(u32, u8);
FASTCALL void	Vdp1WriteWord(u32, u16);
FASTCALL void	Vdp1WriteLong(u32, u32);

#endif
