#ifndef SMPC_H
#define SMPC_H

#include "memory.h"

typedef struct {
	u8 IREG0;
	u8 IREG1;
	u8 IREG2;
	u8 IREG3;
	u8 IREG4;
	u8 IREG5;
	u8 IREG6;

	u8 padding[8];

	u8 COMREG;

	u8 OREG0;
	u8 OREG1;
	u8 OREG2;
	u8 OREG3;
	u8 OREG4;
	u8 OREG5;
	u8 OREG6;
	u8 OREG7;
	u8 OREG8;
	u8 OREG9;
	u8 OREG10;
	u8 OREG11;
	u8 OREG12;
	u8 OREG13;
	u8 OREG14;
	u8 OREG15;
	u8 OREG16;
	u8 OREG17;
	u8 OREG18;
	u8 OREG19;
	u8 OREG20;
	u8 OREG21;
	u8 OREG22;
	u8 OREG23;
	u8 OREG24;
	u8 OREG25;
	u8 OREG26;
	u8 OREG27;
	u8 OREG28;
	u8 OREG29;
	u8 OREG30;
	u8 OREG31;

	u8 SR;
	u8 SF;
} Smpc;

extern Smpc * SmpcRegs;
extern u8 * SmpcRegsT;

int SmpcInit(void);
void SmpcDeInit(void);

void SmpcReset(void);

FASTCALL u8	SmpcReadByte(u32);
FASTCALL u16	SmpcReadWord(u32);
FASTCALL u32	SmpcReadLong(u32);
FASTCALL void	SmpcWriteByte(u32, u8);
FASTCALL void	SmpcWriteWord(u32, u16);
FASTCALL void	SmpcWriteLong(u32, u32);

#endif
