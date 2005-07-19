#ifndef SCU_H
#define SCU_H

#include "core.h"

typedef struct
{
  u32 addr;
} scucodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct {
	/* DMA registers */
	u32 D0R;
        u32 D0W;
	u32 D0C;
	u32 D0AD;
	u32 D0EN;
	u32 D0MD;

	u32 D1R;
        u32 D1W;
	u32 D1C;
	u32 D1AD;
	u32 D1EN;
	u32 D1MD;

	u32 D2R;
        u32 D2W;
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
        scucodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
        int numcodebreakpoints;
        void (*BreakpointCallBack)(unsigned long);
        unsigned char inbreakpoint;
} Scu;

extern Scu * ScuRegs;

typedef struct {
  u32 ProgramRam[256];
  u32 MD[4][64];
#ifdef WORDS_BIGENDIAN
  union {
    struct {
       u32 unused1:5;
       u32 PR:1; // Pause cancel flag
       u32 EP:1; // Temporary stop execution flag
       u32 unused2:1;
       u32 T0:1; // D0 bus use DMA execute flag
       u32 S:1;  // Sine flag
       u32 Z:1;  // Zero flag
       u32 C:1;  // Carry flag
       u32 V:1;  // Overflow flag
       u32 E:1;  // Program end interrupt flag
       u32 ES:1; // Program step execute control bit
       u32 EX:1; // Program execute control bit
       u32 LE:1; // Program counter load enable bit
       u32 unused3:7;
       u32 P:8;  // Program Ram Address
    } part;
    u32 all;
  } ProgControlPort;
#else
  union {
    struct {
       u32 P:8;  // Program Ram Address
       u32 unused3:7;
       u32 LE:1; // Program counter load enable bit
       u32 EX:1; // Program execute control bit
       u32 ES:1; // Program step execute control bit
       u32 E:1;  // Program end interrupt flag
       u32 V:1;  // Overflow flag
       u32 C:1;  // Carry flag
       u32 Z:1;  // Zero flag
       u32 S:1;  // Sine flag
       u32 T0:1; // D0 bus use DMA execute flag
       u32 unused2:1;
       u32 EP:1; // Temporary stop execution flag
       u32 PR:1; // Pause cancel flag
       u32 unused1:5;
    } part;
    u32 all;
  } ProgControlPort;
#endif
  u8 PC;
  u8 TOP;
  u16 LOP;
  s32 jmpaddr;
  unsigned char delayed;
  u8 DataRamPage;
  u8 DataRamReadAddress;
  u8 CT[4];
  u32 RX;
  u32 RY;
  u32 RA0;
  u32 WA0;

#ifdef WORDS_BIGENDIAN
  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } AC;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } P;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } ALU;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } MUL;
#else
  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } AC;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } P;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } ALU;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } MUL;
#endif

} scudspregs_struct;

typedef struct
{
   int mode;
   u32 ReadAddress;
   u32 WriteAddress;
   u32 TransferNumber;
   u32 AddValue;
   u32 ModeAddressUpdate;
} scudmainfo_struct;

int ScuInit(void);
void ScuDeInit(void);
void ScuReset(void);
void ScuExec(u32 timing);

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
