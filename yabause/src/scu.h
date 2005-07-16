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

        /* internal variables */
        u32 timer0;
        u32 timer1;
} Scu;

extern Scu * ScuRegs;

int ScuInit(void);
void ScuDeInit(void);

void ScuReset(void);

u8 FASTCALL	ScuReadByte(u32);
u16 FASTCALL	ScuReadWord(u32);
u32 FASTCALL	ScuReadLong(u32);
void FASTCALL	ScuWriteByte(u32, u8);
void FASTCALL	ScuWriteWord(u32, u16);
void FASTCALL	ScuWriteLong(u32, u32);

void ScuSendVBlankIN(void);
void ScuSendVBlankOUT(void);
void ScuSendHBlankIN(void);
void ScuSendTimer0(void);
void ScuSendTimer1(void);
void ScuSendDSPEnd(void);
void ScuSendSoundRequest(void);
void ScuSendSystemManager(void);
void ScuSendPadInterrupt(void);
void ScuSendLevel2DMAEnd(void);
void ScuSendLevel1DMAEnd(void);
void ScuSendLevel0DMAEnd(void);
void ScuSendDMAIllegal(void);
void ScuSendDrawEnd(void);
void ScuSendExternalInterrupt00(void);
void ScuSendExternalInterrupt01(void);
void ScuSendExternalInterrupt02(void);
void ScuSendExternalInterrupt03(void);
void ScuSendExternalInterrupt04(void);
void ScuSendExternalInterrupt05(void);
void ScuSendExternalInterrupt06(void);
void ScuSendExternalInterrupt07(void);
void ScuSendExternalInterrupt08(void);
void ScuSendExternalInterrupt09(void);
void ScuSendExternalInterrupt10(void);
void ScuSendExternalInterrupt11(void);
void ScuSendExternalInterrupt12(void);
void ScuSendExternalInterrupt13(void);
void ScuSendExternalInterrupt14(void);
void ScuSendExternalInterrupt15(void);

#endif
