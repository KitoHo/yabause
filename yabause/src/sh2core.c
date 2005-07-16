/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// SH2 Shared Code
#include <stdlib.h>
#include "sh2core.h"
#include "memory.h"

SH2_struct *MSH2=NULL;
SH2_struct *SSH2=NULL;
static SH2_struct *CurrentSH2;
SH2Interface_struct *SH2Core=NULL;
extern SH2Interface_struct *SH2CoreList[];

void OnchipReset(SH2_struct *context);
void FRTExec(u32 cycles);
void WDTExec(u32 cycles);

//////////////////////////////////////////////////////////////////////////////

int SH2Init(int coreid)
{
   int i;

   // MSH2
   if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   MSH2->onchip.BCR1 = 0x0000;
   MSH2->isslave = 0;

   // SSH2
   if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   SSH2->onchip.BCR1 = 0x8000;
   SSH2->isslave = 1;

   // So which core do we want?
   if (coreid == SH2CORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; SH2CoreList[i] != NULL; i++)
   {
      if (SH2CoreList[i]->id == coreid)
      {
         // Set to current core
         SH2Core = SH2CoreList[i];
         break;
      }
   }

   if (SH2Core)
      SH2Core->Init();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2DeInit()
{
   if (SH2Core)
      SH2Core->DeInit();

   if (MSH2)
      free(MSH2);

   if (SSH2)
      free(SSH2);
}

//////////////////////////////////////////////////////////////////////////////

void SH2Reset(SH2_struct *context)
{
   int i;

   // Reset general registers
   for (i = 0; i < 15; i++)
      context->regs.R[i] = 0x00000000;
                   
   context->regs.SR.all = 0x000000F0;
   context->regs.GBR = 0x00000000;
   context->regs.VBR = 0x00000000;
   context->regs.MACH = 0x00000000;
   context->regs.MACL = 0x00000000;
   context->regs.PR = 0x00000000;

   // Get PC/Stack initial values
   context->regs.PC = T2ReadLong(BiosRom, 0);
   context->regs.R[15] = T2ReadLong(BiosRom, 4);

   // Internal variables
   context->delay = 0x00000000;
   context->cycles = 0;

   // Reset Interrupts
   memset((void *)context->interrupts, 0, sizeof(interrupt_struct) * MAX_INTERRUPTS);
   context->NumberOfInterrupts = 0;

   // Core specific reset
   SH2Core->Reset();

   // Reset Onchip modules
   OnchipReset(context);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SH2Exec(SH2_struct *context, u32 cycles)
{
   CurrentSH2 = context;

   // Handle interrupts
   if (context->NumberOfInterrupts != 0)
   {
      if (context->interrupts[context->NumberOfInterrupts-1].level > context->regs.SR.part.I)
      {
         context->regs.R[15] -= 4;
         MappedMemoryWriteLong(context->regs.R[15], context->regs.SR.all);
         context->regs.R[15] -= 4;
         MappedMemoryWriteLong(context->regs.R[15], context->regs.PC);
         context->regs.SR.part.I = context->interrupts[context->NumberOfInterrupts-1].level;
         context->regs.PC = MappedMemoryReadLong(context->regs.VBR + (context->interrupts[context->NumberOfInterrupts-1].vector << 2));
         context->NumberOfInterrupts--;
      }
   }

   SH2Core->Exec(context, cycles);

   FRTExec(cycles);
   WDTExec(cycles);

   return 0; // fix me
}

//////////////////////////////////////////////////////////////////////////////

void SH2SendInterrupt(SH2_struct *context, u8 vector, u8 level)
{
   unsigned long i, i2;
   interrupt_struct tmp;

   // Make sure interrupt doesn't already exist
   for (i = 0; i < context->NumberOfInterrupts; i++)
   {
      if (context->interrupts[i].vector == vector)
         return;
   }

   context->interrupts[context->NumberOfInterrupts].level = level;
   context->interrupts[context->NumberOfInterrupts].vector = vector;
   context->NumberOfInterrupts++;

   // Sort interrupts
   for (i = 0; i < (context->NumberOfInterrupts-1); i++)
   {
      for (i2 = i+1; i2 < context->NumberOfInterrupts; i2++)
      {
         if (context->interrupts[i].level > context->interrupts[i2].level)
         {
            tmp.level = context->interrupts[i].level;
            tmp.vector = context->interrupts[i].vector;
            context->interrupts[i].level = context->interrupts[i2].level;
            context->interrupts[i].vector = context->interrupts[i2].vector;
            context->interrupts[i2].level = tmp.level;
            context->interrupts[i2].vector = tmp.vector;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2Step(SH2_struct *context)
{
   unsigned long tmp = context->regs.PC;

   // Execute 1 instruction
   SH2Exec(context, context->cycles+1);

   // Sometimes it doesn't always execute one instruction,
   // let's make sure it did
   if (tmp == context->regs.PC)
      SH2Exec(context, context->cycles+1);
}

//////////////////////////////////////////////////////////////////////////////

void SH2GetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      memcpy(r, &context->regs, sizeof(context->regs));
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      memcpy(&context->regs, r, sizeof(context->regs));
   }
}

//////////////////////////////////////////////////////////////////////////////
// Onchip specific
//////////////////////////////////////////////////////////////////////////////

void OnchipReset(SH2_struct *context) {
   context->onchip.SMR = 0x00;
   context->onchip.BRR = 0xFF;
   context->onchip.SCR = 0x00;
   context->onchip.TDR = 0xFF;
   context->onchip.SSR = 0x84;
   context->onchip.RDR = 0x00;
   context->onchip.TIER = 0x01;
   context->onchip.FTCSR = 0x00;
   context->onchip.FRC = 0x0000;
   context->onchip.TCR = 0x00;
   context->onchip.FICR = 0x0000;
   context->onchip.IPRB = 0x0000;
   context->onchip.VCRA = 0x0000;
   context->onchip.VCRB = 0x0000;
   context->onchip.VCRC = 0x0000;
   context->onchip.VCRD = 0x0000;
   context->onchip.DRCR0 = 0x00;
   context->onchip.DRCR1 = 0x00;
   context->onchip.WTCSR = 0x18;
   context->onchip.WTCNT = 0x00;
   context->onchip.RSTCSR = 0x1F;
   context->onchip.SBYCR = 0x60;
   context->onchip.CCR = 0x00;
   context->onchip.ICR = 0x0000;
   context->onchip.IPRA = 0x0000;
   context->onchip.VCRWDT = 0x0000;
   context->onchip.DVDNT = 0x00000000;
   context->onchip.DVCR = 0x00000000;
   context->onchip.VCRDIV = 0x00000000;
   context->onchip.CHCR0 = 0x00000000;
   context->onchip.CHCR1 = 0x00000000;
   context->onchip.VCRDMA0 = 0x00000000;
   context->onchip.VCRDMA1 = 0x00000000;
   context->onchip.DMAOR = 0x00000000;
   context->onchip.BCR1 &= 0x8000; // preserve MASTER bit
   context->onchip.BCR1 |= 0x03F0;
   context->onchip.BCR2 = 0x00FC;
   context->onchip.WCR = 0xAAFF;
   context->onchip.MCR = 0x0000;
   context->onchip.RTCSR = 0x0000;
   context->onchip.RTCNT = 0x0000;
   context->onchip.RTCOR = 0x0000;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL OnchipReadByte(u32 addr) {
   // stub
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL OnchipReadWord(u32 addr) {
   // stub
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL OnchipReadLong(u32 addr) {
   switch(addr)
   {
      case 0x1E0:
         return CurrentSH2->onchip.BCR1;
      default:
         fprintf(stderr, "Unhandled Onchip long read %08X\n", addr);
         return 0;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteByte(u32 addr, u8 val) {
   // stub
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteWord(u32 addr, u16 val) {
   // stub
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteLong(u32 addr, u32 val)  {
   switch (addr)
   {
      case 0x1E0:
         CurrentSH2->onchip.BCR1 &= 0x8000;
         CurrentSH2->onchip.BCR1 |= val & 0x1FF7;
         return;
      case 0x1E4:
         CurrentSH2->onchip.BCR2 = val & 0xFC;
         return;
      case 0x1E8:
         CurrentSH2->onchip.WCR = val;
         return;
      default:
         fprintf(stderr, "Unhandled Onchip long write %08X\n", addr);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL AddressArrayReadLong(u32 addr) {
   return CurrentSH2->AddressArray[(addr & 0x3FC) >> 2];
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AddressArrayWriteLong(u32 addr, u32 val)  {
   CurrentSH2->AddressArray[(addr & 0x3FC) >> 2] = val;
}

//////////////////////////////////////////////////////////////////////////////

void FRTExec(u32 cycles) {
   // stub
}

//////////////////////////////////////////////////////////////////////////////

void WDTExec(u32 cycles) {
   // stub
}

//////////////////////////////////////////////////////////////////////////////
