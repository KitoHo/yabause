#ifndef SCU_H
#define SCU_H

#include "core.h"

typedef struct {
	/* DMA registers */
	u32 D0R;
	u32 W0R;
	u32 D0C;
	u32 D0AD;
	u32 D0EN;
	u32 D0MD;

	u32 D1R;
	u32 W1R;
	u32 D1C;
	u32 D1AD;
	u32 D1EN;
	u32 D1MD;

	u32 D2R;
	u32 W2R;
	u32 D2C;
	u32 D2AD;
	u32 D2EN;
	u32 D2MD;

	u32 DSTP;
	u32 DSTA;

	/* DSP registers */
	u32 PPAF;
	u32 PPD;
	u32 PDA;
	u32 PDD;

	/* Timer registers */
	u32 T0C;
	u32 T1S;
	u32 T1MD;

	/* Interrupt registers */
	u32 IMS;
	u32 IST;

	/* A-bus registers */
	u32 AIACK;
	u32 ASR0;
	u32 ASR1;
	u32 AREF;

	/* SCU registers */
	u32 RSEL;
	u32 VER;
} Scu;

extern Scu * ScuRegs;

int ScuInit(void);
void ScuDeInit(void);

void ScuReset(void);

FASTCALL u8	ScuReadByte(u32);
FASTCALL u16	ScuReadWord(u32);
FASTCALL u32	ScuReadLong(u32);
FASTCALL void	ScuWriteByte(u32, u8);
FASTCALL void	ScuWriteWord(u32, u16);
FASTCALL void	ScuWriteLong(u32, u32);

#endif
