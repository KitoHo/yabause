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

// SH2 Interpreter Core

#include "sh2core.h"
#include "sh2int.h"
#include "memory.h"

#define INSTRUCTION_A(x) ((x & 0xF000) >> 12)
#define INSTRUCTION_B(x) ((x & 0x0F00) >> 8)
#define INSTRUCTION_C(x) ((x & 0x00F0) >> 4)
#define INSTRUCTION_D(x) (x & 0x000F)
#define INSTRUCTION_CD(x) (x & 0x00FF)
#define INSTRUCTION_BCD(x) (x & 0x0FFF)

typedef void (FASTCALL *opcodefunc)(SH2_struct *);
static opcodefunc opcodes[0x10000];

SH2Interface_struct SH2Interpreter = {
   SH2CORE_INTERPRETER,
   "SH2 Interpreter",
   SH2InterpreterInit,
   SH2InterpreterDeInit,
   SH2InterpreterReset,
   SH2InterpreterExec
};

//////////////////////////////////////////////////////////////////////////////

void FASTCALL delay(SH2_struct * sh, unsigned long addr) {
        switch ((addr >> 20) & 0x0FF) {
           case 0x000: // Bios              
                       sh->instruction = T2ReadWord(BiosRom, addr & 0x7FFFF);
                       break;
           case 0x002: // Low Work Ram
                       sh->instruction = T2ReadWord(LowWram, addr & 0xFFFFF);
                       break;
           case 0x020: // CS0
//                       sh->instruction = memoire->getWord(addr);
                       break;
           case 0x060: // High Work Ram
           case 0x061: 
           case 0x062: 
           case 0x063: 
           case 0x064: 
           case 0x065: 
           case 0x066: 
           case 0x067: 
           case 0x068: 
           case 0x069: 
           case 0x06A: 
           case 0x06B: 
           case 0x06C: 
           case 0x06D: 
           case 0x06E: 
           case 0x06F:
                       sh->instruction = T2ReadWord(HighWram, addr & 0xFFFFF);
                       break;
           default:
                       break;
        }

        opcodes[sh->instruction](sh);
        sh->regs.PC -= 2;
}

void FASTCALL undecoded(SH2_struct * sh) {
        if (sh->isslave)
        {
           int vectnum;

           // Save regs.SR on stack
           sh->regs.R[15]-=4;
           MappedMemoryWriteLong(sh->regs.R[15],sh->regs.SR.all);

           // Save regs.PC on stack
           sh->regs.R[15]-=4;
           MappedMemoryWriteLong(sh->regs.R[15],sh->regs.PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs.PC = MappedMemoryReadLong(sh->regs.VBR+(vectnum<<2));
           sh->cycles++;
        }
        else
        {
           int vectnum;

           fprintf(stderr, "Master SH2 Illegal Opcode: %04X, regs.PC: %08X. Jumping to Exception Service Routine.\n", sh->instruction, sh->regs.PC);

           // Save regs.SR on stack
           sh->regs.R[15]-=4;
           MappedMemoryWriteLong(sh->regs.R[15],sh->regs.SR.all);

           // Save regs.PC on stack
           sh->regs.R[15]-=4;
           MappedMemoryWriteLong(sh->regs.R[15],sh->regs.PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs.PC = MappedMemoryReadLong(sh->regs.VBR+(vectnum<<2));
           sh->cycles++;
        }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL add(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] += sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL addi(SH2_struct * sh) {
  long cd = (long)(signed char)INSTRUCTION_CD(sh->instruction);
  long b = INSTRUCTION_B(sh->instruction);

  sh->regs.R[b] += cd;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL addc(SH2_struct * sh) {
  unsigned long tmp0, tmp1;
  long source = INSTRUCTION_C(sh->instruction);
  long dest = INSTRUCTION_B(sh->instruction);

  tmp1 = sh->regs.R[source] + sh->regs.R[dest];
  tmp0 = sh->regs.R[dest];

  sh->regs.R[dest] = tmp1 + sh->regs.SR.part.T;
  if (tmp0 > tmp1)
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  if (tmp1 > sh->regs.R[dest])
    sh->regs.SR.part.T = 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL addv(SH2_struct * sh) {
  long dest,src,ans;
  long n = INSTRUCTION_B(sh->instruction);
  long m = INSTRUCTION_C(sh->instruction);

  if ((long) sh->regs.R[n] >= 0) dest = 0; else dest = 1;
  
  if ((long) sh->regs.R[m] >= 0) src = 0; else src = 1;
  
  src += dest;
  sh->regs.R[n] += sh->regs.R[m];

  if ((long) sh->regs.R[n] >= 0) ans = 0; else ans = 1;

  ans += dest;
  
  if (src == 0 || src == 2)
    if (ans == 1) sh->regs.SR.part.T = 1;
    else sh->regs.SR.part.T = 0;
  else sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL y_and(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] &= sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL andi(SH2_struct * sh) {
  sh->regs.R[0] &= INSTRUCTION_CD(sh->instruction);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL andm(SH2_struct * sh) {
  long temp;
  long source = INSTRUCTION_CD(sh->instruction);

  temp = (long) MappedMemoryReadByte(sh->regs.GBR + sh->regs.R[0]);
  temp &= source;
  MappedMemoryWriteByte((sh->regs.GBR + sh->regs.R[0]),temp);
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bf(SH2_struct * sh) {
  long disp;
  long d = INSTRUCTION_CD(sh->instruction);

  disp = (long)(signed char)d;

  if (sh->regs.SR.part.T == 0) {
    sh->regs.PC = sh->regs.PC+(disp<<1)+4;
    sh->cycles += 3;
  }
  else {
    sh->regs.PC+=2;
    sh->cycles++;
  }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bfs(SH2_struct * sh) {
  long disp;
  unsigned long temp;
  long d = INSTRUCTION_CD(sh->instruction);

  temp = sh->regs.PC;
  disp = (long)(signed char)d;

  if (sh->regs.SR.part.T == 0) {
    sh->regs.PC = sh->regs.PC + (disp << 1) + 4;

    sh->cycles += 2;
    delay(sh, temp + 2);
  }
  else {
    sh->regs.PC += 2;
    sh->cycles++;
  }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bra(SH2_struct * sh) {
  long disp = INSTRUCTION_BCD(sh->instruction);
  unsigned long temp;

  temp = sh->regs.PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs.PC = sh->regs.PC + (disp<<1) + 4;

  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL braf(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_B(sh->instruction);

  temp = sh->regs.PC;
  sh->regs.PC += sh->regs.R[m] + 4; 

  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bsr(SH2_struct * sh) {
  unsigned long temp;
  long disp = INSTRUCTION_BCD(sh->instruction);

  temp = sh->regs.PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs.PR = sh->regs.PC + 4;
  sh->regs.PC = sh->regs.PC+(disp<<1) + 4;

  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bsrf(SH2_struct * sh) {
  unsigned long temp = sh->regs.PC;
  sh->regs.PR = sh->regs.PC + 4;
  sh->regs.PC += sh->regs.R[INSTRUCTION_B(sh->instruction)] + 4;
  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bt(SH2_struct * sh) { // FIXME ya plus rapide
  long disp;
  long d = INSTRUCTION_CD(sh->instruction);

  disp = (long)(signed char)d;
  if (sh->regs.SR.part.T == 1) {
    sh->regs.PC = sh->regs.PC+(disp<<1)+4;
    sh->cycles += 3;
  }
  else {
    sh->regs.PC += 2;
    sh->cycles++;
  }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL bts(SH2_struct * sh) {
  long disp;
  unsigned long temp;
  long d = INSTRUCTION_CD(sh->instruction);

  if (sh->regs.SR.part.T) {
    temp = sh->regs.PC;
    disp = (long)(signed char)d;
    sh->regs.PC += (disp << 1) + 4;
    sh->cycles += 2;
    delay(sh, temp + 2);
  }
  else {
    sh->regs.PC+=2;
    sh->cycles++;
  }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL clrmac(SH2_struct * sh) {
  sh->regs.MACH = 0;
  sh->regs.MACL = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL clrt(SH2_struct * sh) {
  sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmpeq(SH2_struct * sh) {
  if (sh->regs.R[INSTRUCTION_B(sh->instruction)] == sh->regs.R[INSTRUCTION_C(sh->instruction)])
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmpge(SH2_struct * sh) {
  if ((long)sh->regs.R[INSTRUCTION_B(sh->instruction)] >=
                  (long)sh->regs.R[INSTRUCTION_C(sh->instruction)])
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmpgt(SH2_struct * sh) {
  if ((long)sh->regs.R[INSTRUCTION_B(sh->instruction)]>(long)sh->regs.R[INSTRUCTION_C(sh->instruction)])
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmphi(SH2_struct * sh) {
  if ((unsigned long)sh->regs.R[INSTRUCTION_B(sh->instruction)]>
                  (unsigned long)sh->regs.R[INSTRUCTION_C(sh->instruction)])
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmphs(SH2_struct * sh) {
  if ((unsigned long)sh->regs.R[INSTRUCTION_B(sh->instruction)]>=
                  (unsigned long)sh->regs.R[INSTRUCTION_C(sh->instruction)])
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmpim(SH2_struct * sh) {
  long imm;
  long i = INSTRUCTION_CD(sh->instruction);

  imm = (long)(signed char)i;

  if (sh->regs.R[0] == (unsigned long) imm) // FIXME: ouais � doit �re bon...
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmppl(SH2_struct * sh) {
  if ((long)sh->regs.R[INSTRUCTION_B(sh->instruction)]>0)
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmppz(SH2_struct * sh) {
  if ((long)sh->regs.R[INSTRUCTION_B(sh->instruction)]>=0)
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL cmpstr(SH2_struct * sh) {
  unsigned long temp;
  long HH,HL,LH,LL;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  temp=sh->regs.R[n]^sh->regs.R[m];
  HH = (temp>>24) & 0x000000FF;
  HL = (temp>>16) & 0x000000FF;
  LH = (temp>>8) & 0x000000FF;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0)
    sh->regs.SR.part.T = 1;
  else
    sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL div0s(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  if ((sh->regs.R[n]&0x80000000)==0)
    sh->regs.SR.part.Q = 0;
  else
    sh->regs.SR.part.Q = 1;
  if ((sh->regs.R[m]&0x80000000)==0)
    sh->regs.SR.part.M = 0;
  else
    sh->regs.SR.part.M = 1;
  sh->regs.SR.part.T = !(sh->regs.SR.part.M == sh->regs.SR.part.Q);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL div0u(SH2_struct * sh) {
  sh->regs.SR.part.M = sh->regs.SR.part.Q = sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL div1(SH2_struct * sh) {
  unsigned long tmp0;
  unsigned char old_q, tmp1;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  
  old_q = sh->regs.SR.part.Q;
  sh->regs.SR.part.Q = (unsigned char)((0x80000000 & sh->regs.R[n])!=0);
  sh->regs.R[n] <<= 1;
  sh->regs.R[n]|=(unsigned long) sh->regs.SR.part.T;
  switch(old_q){
  case 0:
    switch(sh->regs.SR.part.M){
    case 0:
      tmp0 = sh->regs.R[n];
      sh->regs.R[n] -= sh->regs.R[m];
      tmp1 = (sh->regs.R[n] > tmp0);
      switch(sh->regs.SR.part.Q){
      case 0:
        sh->regs.SR.part.Q = tmp1;
	break;
      case 1:
        sh->regs.SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      }
      break;
    case 1:
      tmp0 = sh->regs.R[n];
      sh->regs.R[n] += sh->regs.R[m];
      tmp1 = (sh->regs.R[n] < tmp0);
      switch(sh->regs.SR.part.Q){
      case 0:
        sh->regs.SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      case 1:
        sh->regs.SR.part.Q = tmp1;
	break;
      }
      break;
    }
    break;
  case 1:switch(sh->regs.SR.part.M){
  case 0:
    tmp0 = sh->regs.R[n];
    sh->regs.R[n] += sh->regs.R[m];
    tmp1 = (sh->regs.R[n] < tmp0);
    switch(sh->regs.SR.part.Q){
    case 0:
      sh->regs.SR.part.Q = tmp1;
      break;
    case 1:
      sh->regs.SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    }
    break;
  case 1:
    tmp0 = sh->regs.R[n];
    sh->regs.R[n] -= sh->regs.R[m];
    tmp1 = (sh->regs.R[n] > tmp0);
    switch(sh->regs.SR.part.Q){
    case 0:
      sh->regs.SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    case 1:
      sh->regs.SR.part.Q = tmp1;
      break;
    }
    break;
  }
  break;
  }
  sh->regs.SR.part.T = (sh->regs.SR.part.Q == sh->regs.SR.part.M);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL dmuls(SH2_struct * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  
  tempn = (long)sh->regs.R[n];
  tempm = (long)sh->regs.R[m];
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;
  if ((long) (sh->regs.R[n] ^ sh->regs.R[m]) < 0) fnLmL = -1;
  else fnLmL = 0;
  
  temp1 = (unsigned long) tempn;
  temp2 = (unsigned long) tempm;

  RnL = temp1 & 0x0000FFFF;
  RnH = (temp1 >> 16) & 0x0000FFFF;
  RmL = temp2 & 0x0000FFFF;
  RmH = (temp2 >> 16) & 0x0000FFFF;
  
  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;

  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;

  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;
  
  Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;

  if (fnLmL < 0) {
    Res2 = ~Res2;
    if (Res0 == 0)
      Res2++;
    else
      Res0 =(~Res0) + 1;
  }
  sh->regs.MACH = Res2;
  sh->regs.MACL = Res0;
  sh->regs.PC += 2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL dmulu(SH2_struct * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  RnL = sh->regs.R[n] & 0x0000FFFF;
  RnH = (sh->regs.R[n] >> 16) & 0x0000FFFF;
  RmL = sh->regs.R[m] & 0x0000FFFF;
  RmH = (sh->regs.R[m] >> 16) & 0x0000FFFF;

  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;
  
  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;
  
  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;
  
  Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;

  sh->regs.MACH = Res2;
  sh->regs.MACL = Res0;
  sh->regs.PC += 2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL dt(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n]--;
  if (sh->regs.R[n] == 0) sh->regs.SR.part.T = 1;
  else sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL extsb(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (unsigned long)(signed char)sh->regs.R[m];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL extsw(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (unsigned long)(signed short)sh->regs.R[m];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL extub(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (unsigned long)(unsigned char)sh->regs.R[m];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL extuw(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (unsigned long)(unsigned short)sh->regs.R[m];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL jmp(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_B(sh->instruction);

  temp=sh->regs.PC;
  sh->regs.PC = sh->regs.R[m];
  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL jsr(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_B(sh->instruction);

  temp = sh->regs.PC;
  sh->regs.PR = sh->regs.PC + 4;
  sh->regs.PC = sh->regs.R[m];
  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcgbr(SH2_struct * sh) {
  sh->regs.GBR = sh->regs.R[INSTRUCTION_B(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcmgbr(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);

  sh->regs.GBR = MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcmsr(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);

  sh->regs.SR.all = MappedMemoryReadLong(sh->regs.R[m]) & 0x000003F3;
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcmvbr(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);

  sh->regs.VBR = MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcsr(SH2_struct * sh) {
  sh->regs.SR.all = sh->regs.R[INSTRUCTION_B(sh->instruction)]&0x000003F3;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldcvbr(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);

  sh->regs.VBR = sh->regs.R[m];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldsmach(SH2_struct * sh) {
  sh->regs.MACH = sh->regs.R[INSTRUCTION_B(sh->instruction)];
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldsmacl(SH2_struct * sh) {
  sh->regs.MACL = sh->regs.R[INSTRUCTION_B(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldsmmach(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);
  sh->regs.MACH = MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldsmmacl(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);
  sh->regs.MACL = MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldsmpr(SH2_struct * sh) {
  long m = INSTRUCTION_B(sh->instruction);
  sh->regs.PR = MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ldspr(SH2_struct * sh) {
  sh->regs.PR = sh->regs.R[INSTRUCTION_B(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL macl(SH2_struct * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  tempn = (long) MappedMemoryReadLong(sh->regs.R[n]);
  sh->regs.R[n] += 4;
  tempm = (long) MappedMemoryReadLong(sh->regs.R[m]);
  sh->regs.R[m] += 4;

  if ((long) (tempn^tempm) < 0) fnLmL =- 1;
  else fnLmL = 0;
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;

  temp1 = (unsigned long) tempn;
  temp2 = (unsigned long) tempm;

  RnL = temp1 & 0x0000FFFF;
  RnH = (temp1 >> 16) & 0x0000FFFF;
  RmL = temp2 & 0x0000FFFF;
  RmH = (temp2 >> 16) & 0x0000FFFF;

  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;

  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;

  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;

  Res2=Res2+((Res1>>16)&0x0000FFFF)+temp3;

  if(fnLmL < 0){
    Res2=~Res2;
    if (Res0==0) Res2++;
    else Res0=(~Res0)+1;
  }
  if(sh->regs.SR.part.S == 1){
    Res0=sh->regs.MACL+Res0;
    if (sh->regs.MACL>Res0) Res2++;
    if (sh->regs.MACH & 0x00008000);
    else Res2 += sh->regs.MACH | 0xFFFF0000;
    Res2+=(sh->regs.MACH&0x0000FFFF);
    if(((long)Res2<0)&&(Res2<0xFFFF8000)){
      Res2=0x00008000;
      Res0=0x00000000;
    }
    if(((long)Res2>0)&&(Res2>0x00007FFF)){
      Res2=0x00007FFF;
      Res0=0xFFFFFFFF;
    };

    sh->regs.MACH=Res2;
    sh->regs.MACL=Res0;
  }
  else {
    Res0=sh->regs.MACL+Res0;
    if (sh->regs.MACL>Res0) Res2++;
    Res2+=sh->regs.MACH;

    sh->regs.MACH=Res2;
    sh->regs.MACL=Res0;
  }

  sh->regs.PC+=2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL macw(SH2_struct * sh) {
  long tempm,tempn,dest,src,ans;
  unsigned long templ;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  tempn=(long) MappedMemoryReadWord(sh->regs.R[n]);
  sh->regs.R[n]+=2;
  tempm=(long) MappedMemoryReadWord(sh->regs.R[m]);
  sh->regs.R[m]+=2;
  templ=sh->regs.MACL;
  tempm=((long)(short)tempn*(long)(short)tempm);

  if ((long)sh->regs.MACL>=0) dest=0;
  else dest=1;
  if ((long)tempm>=0) {
    src=0;
    tempn=0;
  }
  else {
    src=1;
    tempn=0xFFFFFFFF;
  }
  src+=dest;
  sh->regs.MACL+=tempm;
  if ((long)sh->regs.MACL>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (sh->regs.SR.part.S == 1) {
    if (ans==1) {
      if (src==0) sh->regs.MACL=0x7FFFFFFF;
      if (src==2) sh->regs.MACL=0x80000000;
    }
  }
  else {
    sh->regs.MACH+=tempn;
    if (templ>sh->regs.MACL) sh->regs.MACH+=1;
  }
  sh->regs.PC+=2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL mov(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)]=sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL mova(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  sh->regs.R[0]=((sh->regs.PC+4)&0xFFFFFFFC)+(disp<<2);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbl(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed char)MappedMemoryReadByte(sh->regs.R[m]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbl0(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed char)MappedMemoryReadByte(sh->regs.R[m] + sh->regs.R[0]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbl4(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long disp = INSTRUCTION_D(sh->instruction);

  sh->regs.R[0] = (long)(signed char)MappedMemoryReadByte(sh->regs.R[m] + disp);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movblg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);
  
  sh->regs.R[0] = (long)(signed char)MappedMemoryReadByte(sh->regs.GBR + disp);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbm(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  MappedMemoryWriteByte((sh->regs.R[n] - 1),sh->regs.R[m]);
  sh->regs.R[n] -= 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbp(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed char)MappedMemoryReadByte(sh->regs.R[m]);
  if (n != m)
    sh->regs.R[m] += 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbs(SH2_struct * sh) {
  int b = INSTRUCTION_B(sh->instruction);
  int c = INSTRUCTION_C(sh->instruction);

  MappedMemoryWriteByte(sh->regs.R[b], sh->regs.R[c]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbs0(SH2_struct * sh) {
  MappedMemoryWriteByte(sh->regs.R[INSTRUCTION_B(sh->instruction)] + sh->regs.R[0],
        sh->regs.R[INSTRUCTION_C(sh->instruction)]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbs4(SH2_struct * sh) {
  long disp = INSTRUCTION_D(sh->instruction);
  long n = INSTRUCTION_C(sh->instruction);

  MappedMemoryWriteByte(sh->regs.R[n]+disp,sh->regs.R[0]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movbsg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  MappedMemoryWriteByte(sh->regs.GBR + disp,sh->regs.R[0]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movi(SH2_struct * sh) {
  long i = INSTRUCTION_CD(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed char)i;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movli(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = MappedMemoryReadLong(((sh->regs.PC + 4) & 0xFFFFFFFC) + (disp << 2));
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movll(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] =
        MappedMemoryReadLong(sh->regs.R[INSTRUCTION_C(sh->instruction)]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movll0(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] =
        MappedMemoryReadLong(sh->regs.R[INSTRUCTION_C(sh->instruction)] + sh->regs.R[0]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movll4(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long disp = INSTRUCTION_D(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = MappedMemoryReadLong(sh->regs.R[m] + (disp << 2));
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movllg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  sh->regs.R[0] = MappedMemoryReadLong(sh->regs.GBR + (disp << 2));
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movlm(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  MappedMemoryWriteLong(sh->regs.R[n] - 4,sh->regs.R[m]);
  sh->regs.R[n] -= 4;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movlp(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = MappedMemoryReadLong(sh->regs.R[m]);
  if (n != m) sh->regs.R[m] += 4;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movls(SH2_struct * sh) {
  int b = INSTRUCTION_B(sh->instruction);
  int c = INSTRUCTION_C(sh->instruction);

  MappedMemoryWriteLong(sh->regs.R[b], sh->regs.R[c]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movls0(SH2_struct * sh) {
  MappedMemoryWriteLong(sh->regs.R[INSTRUCTION_B(sh->instruction)] + sh->regs.R[0],
        sh->regs.R[INSTRUCTION_C(sh->instruction)]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movls4(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long disp = INSTRUCTION_D(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  MappedMemoryWriteLong(sh->regs.R[n]+(disp<<2),sh->regs.R[m]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movlsg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  MappedMemoryWriteLong(sh->regs.GBR+(disp<<2),sh->regs.R[0]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movt(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] = (0x00000001 & sh->regs.SR.all);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwi(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed short)MappedMemoryReadWord(sh->regs.PC + (disp<<1) + 4);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwl(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed short)MappedMemoryReadWord(sh->regs.R[m]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwl0(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed short)MappedMemoryReadWord(sh->regs.R[m]+sh->regs.R[0]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwl4(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long disp = INSTRUCTION_D(sh->instruction);

  sh->regs.R[0] = (long)(signed short)MappedMemoryReadWord(sh->regs.R[m]+(disp<<1));
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwlg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  sh->regs.R[0] = (long)(signed short)MappedMemoryReadWord(sh->regs.GBR+(disp<<1));
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwm(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  MappedMemoryWriteWord(sh->regs.R[n] - 2,sh->regs.R[m]);
  sh->regs.R[n] -= 2;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwp(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.R[n] = (long)(signed short)MappedMemoryReadWord(sh->regs.R[m]);
  if (n != m)
    sh->regs.R[m] += 2;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movws(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  MappedMemoryWriteWord(sh->regs.R[n],sh->regs.R[m]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movws0(SH2_struct * sh) {
  MappedMemoryWriteWord(sh->regs.R[INSTRUCTION_B(sh->instruction)] + sh->regs.R[0],
        sh->regs.R[INSTRUCTION_C(sh->instruction)]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movws4(SH2_struct * sh) {
  long disp = INSTRUCTION_D(sh->instruction);
  long n = INSTRUCTION_C(sh->instruction);

  MappedMemoryWriteWord(sh->regs.R[n]+(disp<<1),sh->regs.R[0]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL movwsg(SH2_struct * sh) {
  long disp = INSTRUCTION_CD(sh->instruction);

  MappedMemoryWriteWord(sh->regs.GBR+(disp<<1),sh->regs.R[0]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL mull(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.MACL = sh->regs.R[n] * sh->regs.R[m];
  sh->regs.PC+=2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL muls(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.MACL = ((long)(short)sh->regs.R[n]*(long)(short)sh->regs.R[m]);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL mulu(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  sh->regs.MACL = ((unsigned long)(unsigned short)sh->regs.R[n]
        *(unsigned long)(unsigned short)sh->regs.R[m]);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL neg(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)]=0-sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL negc(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  
  temp=0-sh->regs.R[m];
  sh->regs.R[n] = temp - sh->regs.SR.part.T;
  if (0 < temp) sh->regs.SR.part.T=1;
  else sh->regs.SR.part.T=0;
  if (temp < sh->regs.R[n]) sh->regs.SR.part.T=1;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL nop(SH2_struct * sh) {
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL y_not(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] = ~sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL y_or(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] |= sh->regs.R[INSTRUCTION_C(sh->instruction)];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ori(SH2_struct * sh) {
  sh->regs.R[0] |= INSTRUCTION_CD(sh->instruction);
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL orm(SH2_struct * sh) {
  long temp;
  long source = INSTRUCTION_CD(sh->instruction);

  temp = (long) MappedMemoryReadByte(sh->regs.GBR + sh->regs.R[0]);
  temp |= source;
  MappedMemoryWriteByte(sh->regs.GBR + sh->regs.R[0],temp);
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rotcl(SH2_struct * sh) {
  long temp;
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x80000000)==0) temp=0;
  else temp=1;
  sh->regs.R[n]<<=1;
  if (sh->regs.SR.part.T == 1) sh->regs.R[n]|=0x00000001;
  else sh->regs.R[n]&=0xFFFFFFFE;
  if (temp==1) sh->regs.SR.part.T=1;
  else sh->regs.SR.part.T=0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rotcr(SH2_struct * sh) {
  long temp;
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x00000001)==0) temp=0;
  else temp=1;
  sh->regs.R[n]>>=1;
  if (sh->regs.SR.part.T == 1) sh->regs.R[n]|=0x80000000;
  else sh->regs.R[n]&=0x7FFFFFFF;
  if (temp==1) sh->regs.SR.part.T=1;
  else sh->regs.SR.part.T=0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rotl(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x80000000)==0) sh->regs.SR.part.T=0;
  else sh->regs.SR.part.T=1;
  sh->regs.R[n]<<=1;
  if (sh->regs.SR.part.T==1) sh->regs.R[n]|=0x00000001;
  else sh->regs.R[n]&=0xFFFFFFFE;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rotr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x00000001)==0) sh->regs.SR.part.T = 0;
  else sh->regs.SR.part.T = 1;
  sh->regs.R[n]>>=1;
  if (sh->regs.SR.part.T == 1) sh->regs.R[n]|=0x80000000;
  else sh->regs.R[n]&=0x7FFFFFFF;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rte(SH2_struct * sh) {
  unsigned long temp;
  temp=sh->regs.PC;
  sh->regs.PC = MappedMemoryReadLong(sh->regs.R[15]);
  sh->regs.R[15] += 4;
  sh->regs.SR.all = MappedMemoryReadLong(sh->regs.R[15]) & 0x000003F3;
  sh->regs.R[15] += 4;
  sh->cycles += 4;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL rts(SH2_struct * sh) {
  unsigned long temp;

  temp = sh->regs.PC;
  sh->regs.PC = sh->regs.PR;

  sh->cycles += 2;
  delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL sett(SH2_struct * sh) {
  sh->regs.SR.part.T = 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shal(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  if ((sh->regs.R[n] & 0x80000000) == 0) sh->regs.SR.part.T = 0;
  else sh->regs.SR.part.T = 1;
  sh->regs.R[n] <<= 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shar(SH2_struct * sh) {
  long temp;
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x00000001)==0) sh->regs.SR.part.T = 0;
  else sh->regs.SR.part.T = 1;
  if ((sh->regs.R[n]&0x80000000)==0) temp = 0;
  else temp = 1;
  sh->regs.R[n] >>= 1;
  if (temp == 1) sh->regs.R[n] |= 0x80000000;
  else sh->regs.R[n] &= 0x7FFFFFFF;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shll(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x80000000)==0) sh->regs.SR.part.T=0;
  else sh->regs.SR.part.T=1;
  sh->regs.R[n]<<=1;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shll2(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)] <<= 2;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shll8(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)]<<=8;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shll16(SH2_struct * sh) {
  sh->regs.R[INSTRUCTION_B(sh->instruction)]<<=16;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shlr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);

  if ((sh->regs.R[n]&0x00000001)==0) sh->regs.SR.part.T=0;
  else sh->regs.SR.part.T=1;
  sh->regs.R[n]>>=1;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shlr2(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]>>=2;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shlr8(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]>>=8;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL shlr16(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]>>=16;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcgbr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]=sh->regs.GBR;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcmgbr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]-=4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.GBR);
  sh->regs.PC+=2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcmsr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]-=4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.SR.all);
  sh->regs.PC+=2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcmvbr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]-=4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.VBR);
  sh->regs.PC+=2;
  sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcsr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n] = sh->regs.SR.all;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stcvbr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]=sh->regs.VBR;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stsmach(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]=sh->regs.MACH;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stsmacl(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]=sh->regs.MACL;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stsmmach(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n] -= 4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.MACH); 
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stsmmacl(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n] -= 4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.MACL);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stsmpr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n] -= 4;
  MappedMemoryWriteLong(sh->regs.R[n],sh->regs.PR);
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL stspr(SH2_struct * sh) {
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n] = sh->regs.PR;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL sub(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  sh->regs.R[n]-=sh->regs.R[m];
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL subc(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  unsigned long tmp0,tmp1;
  
  tmp1 = sh->regs.R[n] - sh->regs.R[m];
  tmp0 = sh->regs.R[n];
  sh->regs.R[n] = tmp1 - sh->regs.SR.part.T;
  if (tmp0 < tmp1) sh->regs.SR.part.T = 1;
  else sh->regs.SR.part.T = 0;
  if (tmp1 < sh->regs.R[n]) sh->regs.SR.part.T = 1;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL subv(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  long dest,src,ans;

  if ((long)sh->regs.R[n]>=0) dest=0;
  else dest=1;
  if ((long)sh->regs.R[m]>=0) src=0;
  else src=1;
  src+=dest;
  sh->regs.R[n]-=sh->regs.R[m];
  if ((long)sh->regs.R[n]>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (src==1) {
    if (ans==1) sh->regs.SR.part.T=1;
    else sh->regs.SR.part.T=0;
  }
  else sh->regs.SR.part.T=0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL swapb(SH2_struct * sh) {
  unsigned long temp0,temp1;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  temp0=sh->regs.R[m]&0xffff0000;
  temp1=(sh->regs.R[m]&0x000000ff)<<8;
  sh->regs.R[n]=(sh->regs.R[m]>>8)&0x000000ff;
  sh->regs.R[n]=sh->regs.R[n]|temp1|temp0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL swapw(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  temp=(sh->regs.R[m]>>16)&0x0000FFFF;
  sh->regs.R[n]=sh->regs.R[m]<<16;
  sh->regs.R[n]|=temp;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL tas(SH2_struct * sh) {
  long temp;
  long n = INSTRUCTION_B(sh->instruction);

  temp=(long) MappedMemoryReadByte(sh->regs.R[n]);
  if (temp==0) sh->regs.SR.part.T=1;
  else sh->regs.SR.part.T=0;
  temp|=0x00000080;
  MappedMemoryWriteByte(sh->regs.R[n],temp);
  sh->regs.PC+=2;
  sh->cycles += 4;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL trapa(SH2_struct * sh) {
  long imm = INSTRUCTION_CD(sh->instruction);

  sh->regs.R[15]-=4;
  MappedMemoryWriteLong(sh->regs.R[15],sh->regs.SR.all);
  sh->regs.R[15]-=4;
  MappedMemoryWriteLong(sh->regs.R[15],sh->regs.PC + 2);
  sh->regs.PC = MappedMemoryReadLong(sh->regs.VBR+(imm<<2));
  sh->cycles += 8;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL tst(SH2_struct * sh) {
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);
  if ((sh->regs.R[n]&sh->regs.R[m])==0) sh->regs.SR.part.T = 1;
  else sh->regs.SR.part.T = 0;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL tsti(SH2_struct * sh) {
  long temp;
  long i = INSTRUCTION_CD(sh->instruction);

  temp=sh->regs.R[0]&i;
  if (temp==0) sh->regs.SR.part.T = 1;
  else sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL tstm(SH2_struct * sh) {
  long temp;
  long i = INSTRUCTION_CD(sh->instruction);

  temp=(long) MappedMemoryReadByte(sh->regs.GBR+sh->regs.R[0]);
  temp&=i;
  if (temp==0) sh->regs.SR.part.T = 1;
  else sh->regs.SR.part.T = 0;
  sh->regs.PC+=2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL y_xor(SH2_struct * sh) {
  int b = INSTRUCTION_B(sh->instruction);
  int c = INSTRUCTION_C(sh->instruction);

  sh->regs.R[b] ^= sh->regs.R[c];
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL xori(SH2_struct * sh) {
  long source = INSTRUCTION_CD(sh->instruction);
  sh->regs.R[0] ^= source;
  sh->regs.PC += 2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL xorm(SH2_struct * sh) {
  long source = INSTRUCTION_CD(sh->instruction);
  long temp;

  temp = (long) MappedMemoryReadByte(sh->regs.GBR + sh->regs.R[0]);
  temp ^= source;
  MappedMemoryWriteByte(sh->regs.GBR + sh->regs.R[0],temp);
  sh->regs.PC += 2;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL xtrct(SH2_struct * sh) {
  unsigned long temp;
  long m = INSTRUCTION_C(sh->instruction);
  long n = INSTRUCTION_B(sh->instruction);

  temp=(sh->regs.R[m]<<16)&0xFFFF0000;
  sh->regs.R[n]=(sh->regs.R[n]>>16)&0x0000FFFF;
  sh->regs.R[n]|=temp;
  sh->regs.PC+=2;
  sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL sleep(SH2_struct * sh) {
  //cerr << "SLEEP" << endl;
  sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

opcodefunc decode(unsigned short instruction) {  
  switch (INSTRUCTION_A(instruction)) {
  case 0:
    switch (INSTRUCTION_D(instruction)) {
    case 2:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &stcsr;
      case 1: return &stcgbr;
      case 2: return &stcvbr;
      default: return &undecoded;
      }
     
    case 3:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &bsrf;
      case 2: return &braf;
      default: return &undecoded;
      }
     
    case 4: return &movbs0;
    case 5: return &movws0;
    case 6: return &movls0;
    case 7: return &mull;
    case 8:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &clrt;
      case 1: return &sett;
      case 2: return &clrmac;
      default: return &undecoded;
      }
     
    case 9:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &nop;
      case 1: return &div0u;
      case 2: return &movt;
      default: return &undecoded;
      }
     
    case 10:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &stsmach;
      case 1: return &stsmacl;
      case 2: return &stspr;
      default: return &undecoded;
      }
     
    case 11:
      switch (INSTRUCTION_C(instruction)) {
      case 0: return &rts;
      case 1: return &sleep;
      case 2: return &rte;
      default: return &undecoded;
      }
     
    case 12: return &movbl0;
    case 13: return &movwl0;
    case 14: return &movll0;
    case 15: return &macl;
    default: return &undecoded;
    }
   
  case 1: return &movls4;
  case 2:
    switch (INSTRUCTION_D(instruction)) {
    case 0: return &movbs;
    case 1: return &movws;
    case 2: return &movls;
    case 4: return &movbm;
    case 5: return &movwm;
    case 6: return &movlm;
    case 7: return &div0s;
    case 8: return &tst;
    case 9: return &y_and;
    case 10: return &y_xor;
    case 11: return &y_or;
    case 12: return &cmpstr;
    case 13: return &xtrct;
    case 14: return &mulu;
    case 15: return &muls;
    default: return &undecoded;
    }
   
  case 3:
    switch(INSTRUCTION_D(instruction)) {
    case 0:  return &cmpeq;
    case 2:  return &cmphs;
    case 3:  return &cmpge;
    case 4:  return &div1;
    case 5:  return &dmulu;
    case 6:  return &cmphi;
    case 7:  return &cmpgt;
    case 8:  return &sub;
    case 10: return &subc;
    case 11: return &subv;
    case 12: return &add;
    case 13: return &dmuls;
    case 14: return &addc;
    case 15: return &addv;
    default: return &undecoded;
    }
   
  case 4:
    switch(INSTRUCTION_D(instruction)) {
    case 0:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &shll;
      case 1: return &dt;
      case 2: return &shal;
      default: return &undecoded;
      }
     
    case 1:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &shlr;
      case 1: return &cmppz;
      case 2: return &shar;
      default: return &undecoded;
      }
     
    case 2:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &stsmmach;
      case 1: return &stsmmacl;
      case 2: return &stsmpr;
      default: return &undecoded;
      }
     
    case 3:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &stcmsr;
      case 1: return &stcmgbr;
      case 2: return &stcmvbr;
      default: return &undecoded;
      }
     
    case 4:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &rotl;
      case 2: return &rotcl;
      default: return &undecoded;
      }
     
    case 5:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &rotr;
      case 1: return &cmppl;
      case 2: return &rotcr;
      default: return &undecoded;
      }
     
    case 6:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &ldsmmach;
      case 1: return &ldsmmacl;
      case 2: return &ldsmpr;
      default: return &undecoded;
      }
     
    case 7:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &ldcmsr;
      case 1: return &ldcmgbr;
      case 2: return &ldcmvbr;
      default: return &undecoded;
      }
     
    case 8:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &shll2;
      case 1: return &shll8;
      case 2: return &shll16;
      default: return &undecoded;
      }
     
    case 9:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &shlr2;
      case 1: return &shlr8;
      case 2: return &shlr16;
      default: return &undecoded;
      }
     
    case 10:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &ldsmach;
      case 1: return &ldsmacl;
      case 2: return &ldspr;
      default: return &undecoded;
      }
     
    case 11:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &jsr;
      case 1: return &tas;
      case 2: return &jmp;
      default: return &undecoded;
      }
     
    case 14:
      switch(INSTRUCTION_C(instruction)) {
      case 0: return &ldcsr;
      case 1: return &ldcgbr;
      case 2: return &ldcvbr;
      default: return &undecoded;
      }
     
    case 15: return &macw;
    default: return &undecoded;
    }
   
  case 5: return &movll4;
  case 6:
    switch (INSTRUCTION_D(instruction)) {
    case 0:  return &movbl;
    case 1:  return &movwl;
    case 2:  return &movll;
    case 3:  return &mov;
    case 4:  return &movbp;
    case 5:  return &movwp;
    case 6:  return &movlp;
    case 7:  return &y_not;
    case 8:  return &swapb;
    case 9:  return &swapw;
    case 10: return &negc;
    case 11: return &neg;
    case 12: return &extub;
    case 13: return &extuw;
    case 14: return &extsb;
    case 15: return &extsw;
    }
   
  case 7: return &addi;
  case 8:
    switch (INSTRUCTION_B(instruction)) {
    case 0:  return &movbs4;
    case 1:  return &movws4;
    case 4:  return &movbl4;
    case 5:  return &movwl4;
    case 8:  return &cmpim;
    case 9:  return &bt;
    case 11: return &bf;
    case 13: return &bts;
    case 15: return &bfs;
    default: return &undecoded;
    }
   
  case 9: return &movwi;
  case 10: return &bra;
  case 11: return &bsr;
  case 12:
    switch(INSTRUCTION_B(instruction)) {
    case 0:  return &movbsg;
    case 1:  return &movwsg;
    case 2:  return &movlsg;
    case 3:  return &trapa;
    case 4:  return &movblg;
    case 5:  return &movwlg;
    case 6:  return &movllg;
    case 7:  return &mova;
    case 8:  return &tsti;
    case 9:  return &andi;
    case 10: return &xori;
    case 11: return &ori;
    case 12: return &tstm;
    case 13: return &andm;
    case 14: return &xorm;
    case 15: return &orm;
    }
   
  case 13: return &movli;
  case 14: return &movi;
  default: return &undecoded;
  }
}

//////////////////////////////////////////////////////////////////////////////

int SH2InterpreterInit() {
   int i;

   // Initialize any internal variables
   for(i = 0;i < 0x10000;i++) {
      opcodes[i] = decode(i);
   }

   SH2ClearCodeBreakpoints(MSH2);
   SH2ClearCodeBreakpoints(SSH2);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2InterpreterDeInit() {
   // DeInitialize any internal variables here
}

//////////////////////////////////////////////////////////////////////////////

int SH2InterpreterReset() {
   // Reset any internal variables here
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

FASTCALL u32 SH2InterpreterExec(SH2_struct *context, u32 cycles) {
   while(context->cycles < cycles) {
      SH2HandleBreakpoints(context);

      switch ((context->regs.PC >> 20) & 0x0FF) {
         case 0x000: // Bios
            context->instruction = T2ReadWord(BiosRom, context->regs.PC & 0x7FFFF);
            break;
         case 0x002: // Low Work Ram
            context->instruction = T2ReadWord(LowWram, context->regs.PC & 0xFFFFF);
            break;
         case 0x020: // CS0(fix me)
//            context->instruction = memoire->getWord(regs->PC);
            break;
         case 0x060: // High Work Ram
         case 0x061: 
         case 0x062: 
         case 0x063: 
         case 0x064: 
         case 0x065: 
         case 0x066: 
         case 0x067: 
         case 0x068: 
         case 0x069: 
         case 0x06A: 
         case 0x06B: 
         case 0x06C: 
         case 0x06D: 
         case 0x06E: 
         case 0x06F:
            context->instruction = T2ReadWord(HighWram, context->regs.PC & 0xFFFFF);
            break;
         default:
            break;
      }

      opcodes[context->instruction](context);
   }


   return 0; // fix me
}

//////////////////////////////////////////////////////////////////////////////
