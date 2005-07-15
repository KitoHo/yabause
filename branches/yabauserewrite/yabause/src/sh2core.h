#ifndef SH2_H
#define SH2_H

#include "core.h"

#define SH2CORE_DEFAULT     -1
#define MAX_INTERRUPTS 50

typedef struct
{
   u32 R[16];
   union
   {
      struct
      {
         u32 T:1;
         u32 S:1;
         u32 reserved0:2;
         u32 I:4;
         u32 Q:1;
         u32 M:1;
         u32 reserved1:22;
      } part;
      u32 all;
   } SR;

   u32 GBR;
   u32 VBR;
   u32 MACH;
   u32 MACL;
   u32 PR;
   u32 PC;
} sh2regs_struct;

typedef struct
{
   unsigned char SMR;     // 0xFFFFFE00
   unsigned char BRR;     // 0xFFFFFE01
   unsigned char SCR;     // 0xFFFFFE02
   unsigned char TDR;     // 0xFFFFFE03
   unsigned char SSR;     // 0xFFFFFE04
   unsigned char RDR;     // 0xFFFFFE05
   unsigned char TIER;    // 0xFFFFFE10
   unsigned char FTCSR;   // 0xFFFFFE11
   unsigned short FRC;    // 0xFFFFFE12
                          // 0xFFFFFE13
   unsigned char TCR;     // 0xFFFFFE16
   unsigned short FICR;   // 0xFFFFFE18
                          // 0xFFFFFE19
   unsigned short IPRB;   // 0xFFFFFE60
   unsigned short VCRA;   // 0xFFFFFE62
   unsigned short VCRB;   // 0xFFFFFE64
   unsigned short VCRC;   // 0xFFFFFE66
   unsigned short VCRD;   // 0xFFFFFE68
   unsigned char DRCR0;   // 0xFFFFFE71
   unsigned char DRCR1;   // 0xFFFFFE72
   unsigned char WTCSR;   // 0xFFFFFE80
   unsigned char WTCNT;   // 0xFFFFFE81
   unsigned char RSTCSR;  // 0xFFFFFE83
   unsigned char SBYCR;   // 0xFFFFFE91
   unsigned char CCR;     // 0xFFFFFE92
   unsigned short ICR;    // 0xFFFFFEE0
   unsigned short IPRA;   // 0xFFFFFEE2
   unsigned short VCRWDT; // 0xFFFFFEE4
   unsigned long DVSR;    // 0xFFFFFF00
   unsigned long DVDNT;   // 0xFFFFFF04
   unsigned long DVCR;    // 0xFFFFFF08
   unsigned long VCRDIV;  // 0xFFFFFF0C
   unsigned long DVDNTH;  // 0xFFFFFF10
   unsigned long DVDNTL;  // 0xFFFFFF14
   unsigned long SAR0;    // 0xFFFFFF80
   unsigned long DAR0;    // 0xFFFFFF84
   unsigned long TCR0;    // 0xFFFFFF88
   unsigned long CHCR0;   // 0xFFFFFF8C
   unsigned long SAR1;    // 0xFFFFFF90
   unsigned long DAR1;    // 0xFFFFFF94
   unsigned long TCR1;    // 0xFFFFFF98
   unsigned long CHCR1;   // 0xFFFFFF9C
   unsigned long VCRDMA0; // 0xFFFFFFA0
   unsigned long VCRDMA1; // 0xFFFFFFA8
   unsigned long DMAOR;   // 0xFFFFFFB0
   unsigned short BCR1;   // 0xFFFFFFE0
   unsigned short BCR2;   // 0xFFFFFFE4
   unsigned short WCR;    // 0xFFFFFFE8
   unsigned short MCR;    // 0xFFFFFFEC
   unsigned short RTCSR;  // 0xFFFFFFF0
   unsigned short RTCNT;  // 0xFFFFFFF4
   unsigned short RTCOR;  // 0xFFFFFFF8
} Onchip_struct;

typedef struct
{
   unsigned char vector;
   unsigned char level;
} interrupt_struct;

typedef struct
{
   sh2regs_struct regs;

   u32 delay;
   u32 cycles;
   unsigned char isslave;
   unsigned short instruction;
   Onchip_struct onchip;
   interrupt_struct interrupts[MAX_INTERRUPTS];
   unsigned long NumberOfInterrupts;
} SH2_struct;

typedef struct
{
   int id;
   const char *Name;
   int (*Init)();
   void (*DeInit)();
   int (*Reset)();
   FASTCALL u32 (*Exec)(SH2_struct *context, u32 cycles);
} SH2Interface_struct;

extern SH2_struct *MSH2;
extern SH2_struct *SSH2;

int SH2Init(int coreid);
void SH2DeInit();
void SH2Reset(SH2_struct *context);
FASTCALL u32 SH2Exec(SH2_struct *context, u32 cycles);
void SH2Step(SH2_struct *context);
void SH2GetRegisters(SH2_struct *context, sh2regs_struct * r);
void SH2SetRegisters(SH2_struct *context, sh2regs_struct * r);

FASTCALL u8 OnchipReadByte(u32 addr);
FASTCALL u16 OnchipReadWord(u32 addr);
FASTCALL u32 OnchipReadLong(u32 addr);
FASTCALL void OnchipWriteByte(u32 addr, u8 val);
FASTCALL void OnchipWriteWord(u32 addr, u16 val);
FASTCALL void OnchipWriteLong(u32 addr, u32 val);

#endif
