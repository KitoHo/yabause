#ifndef SMPC_H
#define SMPC_H

#include "memory.h"

typedef struct {
	u16 IREG0;
	u16 IREG1;
	u16 IREG2;
	u16 IREG3;
	u16 IREG4;
	u16 IREG5;
	u16 IREG6;

	u16 padding[8];

	u16 COMREG;

	u16 OREG0;
	u16 OREG1;
	u16 OREG2;
	u16 OREG3;
	u16 OREG4;
	u16 OREG5;
	u16 OREG6;
	u16 OREG7;
	u16 OREG8;
	u16 OREG9;
	u16 OREG10;
	u16 OREG11;
	u16 OREG12;
	u16 OREG13;
	u16 OREG14;
	u16 OREG15;
	u16 OREG16;
	u16 OREG17;
	u16 OREG18;
	u16 OREG19;
	u16 OREG20;
	u16 OREG21;
	u16 OREG22;
	u16 OREG23;
	u16 OREG24;
	u16 OREG25;
	u16 OREG26;
	u16 OREG27;
	u16 OREG28;
	u16 OREG29;
	u16 OREG30;
	u16 OREG31;

	u16 SR;
	u16 SF;
} Smpc;

extern Smpc * SmpcRegs;
extern u8 * SmpcRegsT;

void SmpcNew(void);
void SmpcDelete(void);

void SmpcReset(void);

FASTCALL u8	SmpcReadByte(u32);
FASTCALL u16	SmpcReadWord(u32);
FASTCALL u32	SmpcReadLong(u32);
FASTCALL void	SmpcWriteByte(u32, u8);
FASTCALL void	SmpcWriteWord(u32, u16);
FASTCALL void	SmpcWriteLong(u32, u32);

#endif
