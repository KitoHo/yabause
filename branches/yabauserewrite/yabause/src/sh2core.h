#ifndef SH2_H
#define SH2_H

#include "core.h"

#define SH2CORE_DEFAULT     -1

typedef struct
{
   // fix me
} Onchip_struct;

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
      } bits;
      u32 full;
   } SR;

   u32 GBR;
   u32 VBR;
   u32 MACH;
   u32 MACL;
   u32 PR;
   u32 PC;
   u32 jump_addr;
   int cycles;
   Onchip_struct *onchip;
//   interrupt_struct interrupts[MAX_INTERRUPTS];
//   unsigned long NumberOfInterrupts;
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

int SH2Init(int coreid);
void SH2DeInit();
void SH2Reset(SH2_struct *context);

#endif
