/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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

#include "vdp1.hh"
#include "vdp2.hh"
#include <unistd.h>
#include "exception.hh"
#include "superh.hh"
#ifdef _arch_dreamcast
#include <kos.h>
#endif
#include "yui.hh"
#include "registres.hh"

SuperH::SuperH(bool slave, SaturnMemory *sm) {
  onchip = new Onchip(slave, sm, this);
  purgeArea   = new Dummy(0xFFFFFFFF);
  adressArray = new Memory(0x3FF, 0x400);
  dataArray   = new Memory(0xFFF, 0x1000);
  modeSdram = new Memory(0xFFF, 0x4FFF);

  memoire = sm;

  regs = (sh2regs_struct *) &regs_array;

  reset();

  for(int i = 0;i < 0x10000;i++) {
    instruction = i;
    opcodes[i] = decode();
  }

  for (int i = 0; i < MAX_BREAKPOINTS; i++) {
     codebreakpoint[i].addr = 0xFFFFFFFF;
     codebreakpoint[i].oldopcode = 0xFFFF;
  }
  numcodebreakpoints = 0;
  BreakpointCallBack=NULL;
  inbreakpoint=false;

  isslave = slave;
  cycleCount = 0;

#ifdef DYNAREC
  blockEnd = true;
  lru = lru_new(6);
  hash = hash_new(7);
  block = (struct Block *) malloc(sizeof(struct Block) * 7);

  for(int i = 0;i <  23;i++)
    sh2reg_jit[i] = -1;

  for(int i = 0;i < 6;i++) {
    jitreg[i].sh2reg = -1;
  }

  jitreg[0].reg = JIT_R0;
  jitreg[1].reg = JIT_R1;
  jitreg[2].reg = JIT_R2;
  jitreg[3].reg = JIT_V0;
  jitreg[4].reg = JIT_V1;
  jitreg[5].reg = JIT_V2;

  verbose = true;
  compile = false;
  compile_only = false;
#endif
}

SuperH::~SuperH(void) {
#ifdef DYNAREC
  free(block);
  hash_delete(hash);
  lru_delete(lru);
#endif
  delete onchip;
  delete purgeArea;
  delete adressArray;
  delete dataArray;
  delete modeSdram;
}

#ifdef DYNAREC
void SuperH::mapReg(int regNum) {
        if (sh2reg_jit[regNum] != -1) {
                lru_use_reg(lru, sh2reg_jit[regNum]);
                return;
        }
        else {
                int i = lru_new_reg(lru);
                if (jitreg[i].sh2reg != -1) {
                        jit_sti_ul(&regs_array[jitreg[i].sh2reg], jitreg[i].reg);
                        sh2reg_jit[jitreg[i].sh2reg] = -1;
                }
                jitreg[i].sh2reg = regNum;
                sh2reg_jit[regNum] = i;
                jit_ldi_ul(jitreg[i].reg, &regs_array[regNum]);
        }
}

void SuperH::flushRegs(void) {
	for(int i = 0;i < 6;i++) {
		if (jitreg[i].sh2reg != -1) {
			jit_sti_i(&regs_array[jitreg[i].sh2reg], jitreg[i].reg);
			sh2reg_jit[jitreg[i].sh2reg] = -1;
			jitreg[i].sh2reg = -1;
		}
	}
}

int SuperH::reg(int i) {
        return jitreg[sh2reg_jit[i]].reg;
}

void SuperH::beginBlock(void) {
	block[currentBlock].execute = (pifi) (jit_set_ip(block[currentBlock].codeBuffer).iptr);
	startAddress = jit_get_ip().ptr;
	jit_prolog(0);
	block[currentBlock].cycleCount = 0;
	block[currentBlock].length = 0;
}

void SuperH::endBlock(void) {
	flushRegs();
	jit_ret();
	jit_flush_code(block[currentBlock].codeBuffer, jit_get_ip().ptr);
}
#endif

void SuperH::reset(void) {
  regs->SR.part.T = regs->SR.part.S = regs->SR.part.Q = regs->SR.part.M = 0;
  regs->SR.part.I = 0xF;
  regs->SR.part.useless1 = 0;
  regs->SR.part.useless2 = 0;
  regs->VBR = 0;
  for(int i = 0;i < 16;i++) regs->R[i] = 0;

  // reset interrupts
  while ( !interrupts.empty() ) {
        interrupts.pop();
  }

  ((Onchip*)onchip)->reset();
 
  timing = 0;
}

void SuperH::PowerOn() {
  regs->PC = memoire->getLong(regs->VBR);
  regs->R[15] = memoire->getLong(regs->VBR + 4);
}

Memory *SuperH::getMemory(void) {
  return memoire;
}

void SuperH::send(const Interrupt& i) {
	interrupts.push(i);
}

void SuperH::sendNMI(void) {
	send(Interrupt(16, 11));
}

void SuperH::delay(unsigned long addr) {
#ifdef DYNAREC
	if (compile && verbose && (block[currentBlock].status == BLOCK_FAILED)) {
		cerr << "failed (delay) opcode = " << hex << instruction << endl;
		verbose = false;
	}
#endif
        switch ((addr >> 20) & 0x0FF) {
           case 0x000: // Bios              
                       instruction = readWord(memoire->rom, addr);
                       break;
           case 0x002: // Low Work Ram
                       instruction = readWord(memoire->ramLow, addr);
                       break;
           case 0x020: // CS0
                       instruction = memoire->getWord(addr);
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
                       instruction = readWord(memoire->ramHigh, addr);
                       break;
           default:
                       break;
        }

        (*opcodes[instruction])(this);
#ifdef DYNAREC
        if (!compile_only)
#endif
	regs->PC -= 2;
}

void SuperH::run(int t) {
	if (timing > 0) {
		timing -= t;
		if (timing <= 0) {
			sendNMI();
		}
	}
}

void SuperH::runCycles(unsigned long cc) {
	if ( !interrupts.empty() ) {
		Interrupt interrupt = interrupts.top();
		if (interrupt.level() > regs->SR.part.I) {
			interrupts.pop();

			regs->R[15] -= 4;
			memoire->setLong(regs->R[15], regs->SR.all);
			regs->R[15] -= 4;
			memoire->setLong(regs->R[15], regs->PC);
			regs->SR.part.I = interrupt.level();
			regs->PC = memoire->getLong(regs->VBR + (interrupt.vector() << 2));
		}
	}

	while(cycleCount < cc) {
#ifdef DYNAREC
		blockEnd = false;
		if (hash_add_addr(hash, regs->PC, &currentBlock) == 1) {
			//if (memoire->isDirty(regs->PC, block[currentBlock].length)) {
			if (memoire->isDirty(regs->PC, 1)) {
				block[currentBlock].status = BLOCK_FIRST;
			} else {
				if (block[currentBlock].status == BLOCK_FIRST) {
					compile = true;
				}
			}
		} else {
			block[currentBlock].status = BLOCK_FIRST;
		}
		if (block[currentBlock].status == BLOCK_COMPILED) {
			block[currentBlock].execute();
			cycleCount += block[currentBlock].cycleCount;
		}
		else {
		if (compile) {
			beginBlock();
		}
		while(!blockEnd) {
#endif
		// Make sure it isn't one of our breakpoints
		for (int i=0; i < numcodebreakpoints; i++) {
			if ((regs->PC == codebreakpoint[i].addr) && inbreakpoint == false) {

				inbreakpoint = true;
				if (BreakpointCallBack) BreakpointCallBack(isslave, codebreakpoint[i].addr);
				inbreakpoint = false;
			}
		}

		switch ((regs->PC >> 20) & 0x0FF) {
		case 0x000: // Bios
			instruction = readWord(memoire->rom, regs->PC);
			break;
		case 0x002: // Low Work Ram
			instruction = readWord(memoire->ramLow, regs->PC);
			break;
		case 0x020: // CS0(fix me)
			instruction = memoire->getWord(regs->PC);
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
			instruction = readWord(memoire->ramHigh, regs->PC);
			break;
		default:
			break;
		}

		(*opcodes[instruction])(this);
#ifdef DYNAREC
		if (compile && verbose && (block[currentBlock].status == BLOCK_FAILED)) {
			cerr << "failed opcode = " << hex << instruction << endl;
			verbose = false;
		}
		}
		if (compile) {
			if (block[currentBlock].status != BLOCK_FAILED) {
				cerr << "we compiled a block ! \\o/ " << hex << regs->PC << endl;
				block[currentBlock].status = BLOCK_COMPILED;
				endBlock();
			}
			else {
				endBlock();
				verbose = false;
			}
			compile = false;
		}
		}
#endif
	}

	((Onchip *)onchip)->runFRT(cc);
	((Onchip *)onchip)->runWDT(cc);
}

void SuperH::step(void) {
   unsigned long tmp = regs->PC;

   // Execute 1 instruction
   runCycles(cycleCount+1);

   // Sometimes it doesn't always seem to execute one instruction,
   // let's make sure it did
   if (tmp == regs->PC)
      runCycles(cycleCount+1);
}

#ifndef _arch_dreamcast
ostream& operator<<(ostream& os, const SuperH& sh) {
  for(int j = 0;j < 4;j++) {
    os << "\t";
    for(int i = 4*j;i < 4 * (j + 1);i++) {
      os << "R" << dec
#ifdef STL_LEFT_RIGHT
		<< left
#endif
		<< setw(4) << i << hex
#ifdef STL_LEFT_RIGHT
		<< right
#endif
		<< setw(10) << sh.regs->R[i] << "  ";
    }
    os << "\n";
  }

  os << dec
#ifdef STL_LEFT_RIGHT
     << left
#endif
                     << "\tregs->SR    M  " << setw(11) << sh.regs->SR.part.M
                            << "Q  " << setw(11) << sh.regs->SR.part.Q
                     << hex << "I  " << setw(11) << sh.regs->SR.part.I
                     << dec << "S  " << setw(11) << sh.regs->SR.part.S
                            << "T  " <<             sh.regs->SR.part.T << endl;

  os << hex
#ifdef STL_LEFT_RIGHT
     << right
#endif
                     << "\tregs->GBR\t" << setw(10) << sh.regs->GBR
                     << "\tregs->MACH\t" << setw(10) << sh.regs->MACH
                     << "\tregs->PR\t" << setw(10) << sh.regs->PR << endl
                     << "\tregs->VBR\t" << setw(10) << sh.regs->VBR
                     << "\tregs->MACL\t" << setw(10) << sh.regs->MACL
                     << "\tregs->PC\t" << setw(10) << sh.regs->PC << endl;

  return os;
}
#endif

void undecoded(SuperH * sh) {
        if (sh->isslave)
        {
           int vectnum;

           fprintf(stderr, "Slave SH2 Illegal Opcode: %04X, regs->PC: %08X. Jumping to Exception Service Routine.\n", sh->instruction, sh->regs->PC);

           // Save regs->SR on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);

           // Save regs->PC on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(vectnum<<2));
           sh->cycleCount++;
        }
        else
        {
           int vectnum;

           fprintf(stderr, "Master SH2 Illegal Opcode: %04X, regs->PC: %08X. Jumping to Exception Service Routine.\n", sh->instruction, sh->regs->PC);

           // Save regs->SR on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);

           // Save regs->PC on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(vectnum<<2));
           sh->cycleCount++;
        }
}

void add(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] += sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void addi(SuperH * sh) {
  long cd = (long)(signed char)Instruction::cd(sh->instruction);
  long b = Instruction::b(sh->instruction);

#ifdef DYNAREC
  if (!sh->compile_only) {
#endif

  sh->regs->R[b] += cd;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {
    sh->block[sh->currentBlock].cycleCount += 1;

    sh->mapReg(b);
    jit_addi_ul(sh->reg(b), sh->reg(b), cd);
    sh->mapReg(R_PC);
    jit_addi_i(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif
}

void addc(SuperH * sh) {
  unsigned long tmp0, tmp1;
  long source = Instruction::c(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  tmp1 = sh->regs->R[source] + sh->regs->R[dest];
  tmp0 = sh->regs->R[dest];

  sh->regs->R[dest] = tmp1 + sh->regs->SR.part.T;
  if (tmp0 > tmp1)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  if (tmp1 > sh->regs->R[dest])
    sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void addv(SuperH * sh) {
  long dest,src,ans;
  long n = Instruction::b(sh->instruction);
  long m = Instruction::c(sh->instruction);

  if ((long) sh->regs->R[n] >= 0) dest = 0; else dest = 1;
  
  if ((long) sh->regs->R[m] >= 0) src = 0; else src = 1;
  
  src += dest;
  sh->regs->R[n] += sh->regs->R[m];

  if ((long) sh->regs->R[n] >= 0) ans = 0; else ans = 1;

  ans += dest;
  
  if (src == 0 || src == 2)
    if (ans == 1) sh->regs->SR.part.T = 1;
    else sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void y_and(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] &= sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void andi(SuperH * sh) {
  sh->regs->R[0] &= Instruction::cd(sh->instruction);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void andm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp &= source;
  sh->memoire->setByte((sh->regs->GBR + sh->regs->R[0]),temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bf(SuperH * sh) {
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;

  if (sh->regs->SR.part.T == 0) {
    sh->regs->PC = sh->regs->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->regs->PC+=2;
    sh->cycleCount++;
  }

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bfs(SuperH * sh) {
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);

  temp = sh->regs->PC;
  disp = (long)(signed char)d;

#ifdef DYNAREC
  jit_insn  *ref_true, *ref_quit;
  if (sh->compile) {
    sh->block[sh->currentBlock].cycleCount += 1;

    sh->flushRegs();
    jit_ldi_ul(JIT_R0, &sh->regs_array[R_SR]);
    ref_true = jit_bmsi_ul(jit_forward(), JIT_R0, 1);

    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), (disp << 1) + 4);

    jit_pushr_ul(sh->reg(R_PC));
  }
#endif

  if (sh->regs->SR.part.T == 0) {
    sh->regs->PC = sh->regs->PC + (disp << 1) + 4;

    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->regs->PC += 2;
    sh->cycleCount++;
#ifdef DYNAREC
    if (sh->compile) {
      sh->compile_only = true;
      sh->delay(temp + 2);
      sh->compile_only = false;
    }
#endif
  }

#ifdef DYNAREC
  if (sh->compile) {
    sh->mapReg(R_PC);
    jit_popr_ul(sh->reg(R_PC));
    sh->flushRegs();
    ref_quit = jit_jmpi(jit_forward());

    jit_patch(ref_true);
    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), 2);

    sh->flushRegs();
    jit_patch(ref_quit);
  }

  sh->blockEnd = true;
#endif
}

void bra(SuperH * sh) {
  long disp = Instruction::bcd(sh->instruction);
  unsigned long temp;

  temp = sh->regs->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs->PC = sh->regs->PC + (disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void braf(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->regs->PC;
  sh->regs->PC += sh->regs->R[m] + 4; 

  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bsr(SuperH * sh) {
  unsigned long temp;
  long disp = Instruction::bcd(sh->instruction);

  temp = sh->regs->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC = sh->regs->PC+(disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bsrf(SuperH * sh) {
  unsigned long temp = sh->regs->PC;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC += sh->regs->R[Instruction::b(sh->instruction)] + 4;
  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bt(SuperH * sh) { // FIXME ya plus rapide
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;
  if (sh->regs->SR.part.T == 1) {
    sh->regs->PC = sh->regs->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->regs->PC += 2;
    sh->cycleCount++;
  }

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void bts(SuperH * sh) {
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);

  if (sh->regs->SR.part.T) {
    temp = sh->regs->PC;
    disp = (long)(signed char)d;
    sh->regs->PC += (disp << 1) + 4;
    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->regs->PC+=2;
    sh->cycleCount++;
  }

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void clrmac(SuperH * sh) {
  sh->regs->MACH = 0;
  sh->regs->MACL = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void clrt(SuperH * sh) {
  sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmpeq(SuperH * sh) {
  if (sh->regs->R[Instruction::b(sh->instruction)] == sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmpge(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)] >=
		  (long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmpgt(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>(long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmphi(SuperH * sh) {
  if ((unsigned long)sh->regs->R[Instruction::b(sh->instruction)]>
		  (unsigned long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmphs(SuperH * sh) {
  if ((unsigned long)sh->regs->R[Instruction::b(sh->instruction)]>=
		  (unsigned long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmpim(SuperH * sh) {
  long imm;
  long i = Instruction::cd(sh->instruction);

  imm = (long)(signed char)i;

  if (sh->regs->R[0] == (unsigned long) imm) // FIXME: ouais � doit �re bon...
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmppl(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmppz(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>=0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void cmpstr(SuperH * sh) {
  unsigned long temp;
  long HH,HL,LH,LL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=sh->regs->R[n]^sh->regs->R[m];
  HH = (temp>>24) & 0x000000FF;
  HL = (temp>>16) & 0x000000FF;
  LH = (temp>>8) & 0x000000FF;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void div0s(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n]&0x80000000)==0)
    sh->regs->SR.part.Q = 0;
  else
    sh->regs->SR.part.Q = 1;
  if ((sh->regs->R[m]&0x80000000)==0)
    sh->regs->SR.part.M = 0;
  else
    sh->regs->SR.part.M = 1;
  sh->regs->SR.part.T = !(sh->regs->SR.part.M == sh->regs->SR.part.Q);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void div0u(SuperH * sh) {
  sh->regs->SR.part.M = sh->regs->SR.part.Q = sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void div1(SuperH * sh) {
  unsigned long tmp0;
  unsigned char old_q, tmp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  old_q = sh->regs->SR.part.Q;
  sh->regs->SR.part.Q = (unsigned char)((0x80000000 & sh->regs->R[n])!=0);
  sh->regs->R[n] <<= 1;
  sh->regs->R[n]|=(unsigned long) sh->regs->SR.part.T;
  switch(old_q){
  case 0:
    switch(sh->regs->SR.part.M){
    case 0:
      tmp0 = sh->regs->R[n];
      sh->regs->R[n] -= sh->regs->R[m];
      tmp1 = (sh->regs->R[n] > tmp0);
      switch(sh->regs->SR.part.Q){
      case 0:
	sh->regs->SR.part.Q = tmp1;
	break;
      case 1:
	sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      }
      break;
    case 1:
      tmp0 = sh->regs->R[n];
      sh->regs->R[n] += sh->regs->R[m];
      tmp1 = (sh->regs->R[n] < tmp0);
      switch(sh->regs->SR.part.Q){
      case 0:
	sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      case 1:
	sh->regs->SR.part.Q = tmp1;
	break;
      }
      break;
    }
    break;
  case 1:switch(sh->regs->SR.part.M){
  case 0:
    tmp0 = sh->regs->R[n];
    sh->regs->R[n] += sh->regs->R[m];
    tmp1 = (sh->regs->R[n] < tmp0);
    switch(sh->regs->SR.part.Q){
    case 0:
      sh->regs->SR.part.Q = tmp1;
      break;
    case 1:
      sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    }
    break;
  case 1:
    tmp0 = sh->regs->R[n];
    sh->regs->R[n] -= sh->regs->R[m];
    tmp1 = (sh->regs->R[n] > tmp0);
    switch(sh->regs->SR.part.Q){
    case 0:
      sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    case 1:
      sh->regs->SR.part.Q = tmp1;
      break;
    }
    break;
  }
  break;
  }
  sh->regs->SR.part.T = (sh->regs->SR.part.Q == sh->regs->SR.part.M);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}


void dmuls(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  tempn = (long)sh->regs->R[n];
  tempm = (long)sh->regs->R[m];
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;
  if ((long) (sh->regs->R[n] ^ sh->regs->R[m]) < 0) fnLmL = -1;
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
  sh->regs->MACH = Res2;
  sh->regs->MACL = Res0;
  sh->regs->PC += 2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void dmulu(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  RnL = sh->regs->R[n] & 0x0000FFFF;
  RnH = (sh->regs->R[n] >> 16) & 0x0000FFFF;
  RmL = sh->regs->R[m] & 0x0000FFFF;
  RmH = (sh->regs->R[m] >> 16) & 0x0000FFFF;

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

  sh->regs->MACH = Res2;
  sh->regs->MACL = Res0;
  sh->regs->PC += 2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void dt(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

#ifdef DYNAREC
  if(!sh->compile_only) {
#endif
  sh->regs->R[n]--;
  if (sh->regs->R[n] == 0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {
    jit_insn  *ref_true, *ref_quit;

    sh->block[sh->currentBlock].cycleCount += 1;

    sh->mapReg(n);
    sh->mapReg(R_SR);

    jit_subi_ul(sh->reg(n), sh->reg(n), 1);
    ref_true = jit_beqi_ul(jit_forward(), sh->reg(n), 0);

    jit_andi_ul(sh->reg(R_SR), sh->reg(R_SR), 0xFFFFFFFE);
    ref_quit = jit_jmpi(jit_forward());

    jit_patch(ref_true);
    jit_ori_ul(sh->reg(R_SR), sh->reg(R_SR), 1);

    jit_patch(ref_quit);
    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif
}

void extsb(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(signed char)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void extsw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(signed short)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void extub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(unsigned char)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void extuw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(unsigned short)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void jmp(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp=sh->regs->PC;
  sh->regs->PC = sh->regs->R[m];
  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void jsr(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->regs->PC;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC = sh->regs->R[m];
  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcgbr(SuperH * sh) {
  sh->regs->GBR = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcmgbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->GBR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcmsr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->SR.all = sh->memoire->getLong(sh->regs->R[m]) & 0x000003F3;
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcmvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->VBR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcsr(SuperH * sh) {
  sh->regs->SR.all = sh->regs->R[Instruction::b(sh->instruction)]&0x000003F3;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldcvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->VBR = sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldsmach(SuperH * sh) {
  sh->regs->MACH = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldsmacl(SuperH * sh) {
  sh->regs->MACL = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldsmmach(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->MACH = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldsmmacl(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->MACL = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldsmpr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->PR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ldspr(SuperH * sh) {
  sh->regs->PR = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void macl(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  tempn = (long) sh->memoire->getLong(sh->regs->R[n]);
  sh->regs->R[n] += 4;
  tempm = (long) sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;

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
  if(sh->regs->SR.part.S == 1){
    Res0=sh->regs->MACL+Res0;
    if (sh->regs->MACL>Res0) Res2++;
    if (sh->regs->MACH & 0x00008000);
    else Res2 += sh->regs->MACH | 0xFFFF0000;
    Res2+=(sh->regs->MACH&0x0000FFFF);
    if(((long)Res2<0)&&(Res2<0xFFFF8000)){
      Res2=0x00008000;
      Res0=0x00000000;
    }
    if(((long)Res2>0)&&(Res2>0x00007FFF)){
      Res2=0x00007FFF;
      Res0=0xFFFFFFFF;
    };

    sh->regs->MACH=Res2;
    sh->regs->MACL=Res0;
  }
  else {
    Res0=sh->regs->MACL+Res0;
    if (sh->regs->MACL>Res0) Res2++;
    Res2+=sh->regs->MACH;

    sh->regs->MACH=Res2;
    sh->regs->MACL=Res0;
  }

  sh->regs->PC+=2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void macw(SuperH * sh) {
  long tempm,tempn,dest,src,ans;
  unsigned long templ;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  tempn=(long) sh->memoire->getWord(sh->regs->R[n]);
  sh->regs->R[n]+=2;
  tempm=(long) sh->memoire->getWord(sh->regs->R[m]);
  sh->regs->R[m]+=2;
  templ=sh->regs->MACL;
  tempm=((long)(short)tempn*(long)(short)tempm);

  if ((long)sh->regs->MACL>=0) dest=0;
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
  sh->regs->MACL+=tempm;
  if ((long)sh->regs->MACL>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (sh->regs->SR.part.S == 1) {
    if (ans==1) {
      if (src==0) sh->regs->MACL=0x7FFFFFFF;
      if (src==2) sh->regs->MACL=0x80000000;
    }
  }
  else {
    sh->regs->MACH+=tempn;
    if (templ>sh->regs->MACL) sh->regs->MACH+=1;
  }
  sh->regs->PC+=2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void mov(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]=sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void mova(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0]=((sh->regs->PC+4)&0xFFFFFFFC)+(disp<<2);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m] + sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->regs->R[0] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m] + disp);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movblg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  
  sh->regs->R[0] = (long)(signed char)sh->memoire->getByte(sh->regs->GBR + disp);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setByte((sh->regs->R[n] - 1),sh->regs->R[m]);
  sh->regs->R[n] -= 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m]);
  if (n != m)
    sh->regs->R[m] += 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbs(SuperH * sh) {
  int b = Instruction::b(sh->instruction);
  int c = Instruction::c(sh->instruction);

#ifdef DYNAREC
  if(!sh->compile_only) {
#endif

  sh->memoire->setByte(sh->regs->R[b], sh->regs->R[c]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {

    sh->block[sh->currentBlock].cycleCount += 1;

    sh->flushRegs();
    jit_ldi_ul(JIT_R0, &sh->regs_array[c]);
    jit_ldi_ul(JIT_R1, &sh->regs_array[b]);
    jit_movi_p(JIT_R2, sh->memoire);

    jit_prepare(3);
    jit_pusharg_ul(JIT_R0);
    jit_pusharg_ul(JIT_R1);
    jit_pusharg_p(JIT_R2);

    jit_finish(satmemWriteByte);

    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif
}

void movbs0(SuperH * sh) {
  sh->memoire->setByte(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbs4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setByte(sh->regs->R[n]+disp,sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movbsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setByte(sh->regs->GBR + disp,sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movi(SuperH * sh) {
  long i = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)i;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movli(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

#ifdef DYNAREC
  if(!sh->compile_only) {
#endif

  sh->regs->R[n] = sh->memoire->getLong(((sh->regs->PC + 4) & 0xFFFFFFFC) + (disp << 2));
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {
    sh->block[sh->currentBlock].cycleCount += 1;

    sh->flushRegs();

    jit_ldi_ul(JIT_R0, &sh->regs_array[R_PC]);
    jit_andi_ul(JIT_R0, JIT_R0, 0xFFFFFFFC);
    jit_addi_ul(JIT_R0, JIT_R0, (disp << 2));

    jit_movi_p(JIT_R1, sh->memoire);

    jit_prepare(2);
    jit_pusharg_ul(JIT_R0);
    jit_pusharg_p(JIT_R1);

    jit_finish(satmemReadLong);
    jit_retval(JIT_R0);

    jit_sti_ul(&sh->regs_array[n], JIT_R0);

    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif
}

void movll(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movll0(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->regs->R[Instruction::c(sh->instruction)] + sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movll4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = sh->memoire->getLong(sh->regs->R[m] + (disp << 2));
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movllg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0] = sh->memoire->getLong(sh->regs->GBR + (disp << 2));
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movlm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->regs->R[n] - 4,sh->regs->R[m]);
  sh->regs->R[n] -= 4;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movlp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = sh->memoire->getLong(sh->regs->R[m]);
  if (n != m) sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movls(SuperH * sh) {
  int b = Instruction::b(sh->instruction);
  int c = Instruction::c(sh->instruction);

#ifdef DYNAREC
  if(!sh->compile_only) {
#endif

  sh->memoire->setLong(sh->regs->R[b], sh->regs->R[c]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {
    sh->block[sh->currentBlock].cycleCount += 1;

    sh->flushRegs();

    jit_ldi_ul(JIT_R0, &sh->regs_array[c]);
    jit_ldi_ul(JIT_R1, &sh->regs_array[b]);
    jit_movi_p(JIT_R2, sh->memoire);

    jit_prepare(3);
    jit_pusharg_ul(JIT_R0);
    jit_pusharg_ul(JIT_R1);
    jit_pusharg_p(JIT_R2);

    jit_finish(satmemWriteLong);

    sh->mapReg(R_PC);
    jit_addi_ul(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif

}

void movls0(SuperH * sh) {
  sh->memoire->setLong(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movls4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->regs->R[n]+(disp<<2),sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movlsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setLong(sh->regs->GBR+(disp<<2),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movt(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] = (0x00000001 & sh->regs->SR.all);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwi(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->PC + (disp<<1) + 4);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]+sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->regs->R[0] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]+(disp<<1));
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwlg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0] = (long)(signed short)sh->memoire->getWord(sh->regs->GBR+(disp<<1));
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n] - 2,sh->regs->R[m]);
  sh->regs->R[n] -= 2;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]);
  if (n != m)
    sh->regs->R[m] += 2;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movws(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n],sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movws0(SuperH * sh) {
  sh->memoire->setWord(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movws4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n]+(disp<<1),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void movwsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setWord(sh->regs->GBR+(disp<<1),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void mull(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = sh->regs->R[n] * sh->regs->R[m];
  sh->regs->PC+=2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void muls(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = ((long)(short)sh->regs->R[n]*(long)(short)sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void mulu(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = ((unsigned long)(unsigned short)sh->regs->R[n]
	*(unsigned long)(unsigned short)sh->regs->R[m]);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void neg(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]=0-sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void negc(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  temp=0-sh->regs->R[m];
  sh->regs->R[n] = temp - sh->regs->SR.part.T;
  if (0 < temp) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  if (temp < sh->regs->R[n]) sh->regs->SR.part.T=1;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void nop(SuperH * sh) {
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void y_not(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] = ~sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void y_or(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] |= sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void ori(SuperH * sh) {
  sh->regs->R[0] |= Instruction::cd(sh->instruction);
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void orm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp |= source;
  sh->memoire->setByte(sh->regs->GBR + sh->regs->R[0],temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rotcl(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) temp=0;
  else temp=1;
  sh->regs->R[n]<<=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x00000001;
  else sh->regs->R[n]&=0xFFFFFFFE;
  if (temp==1) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rotcr(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) temp=0;
  else temp=1;
  sh->regs->R[n]>>=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x80000000;
  else sh->regs->R[n]&=0x7FFFFFFF;
  if (temp==1) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rotl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]<<=1;
  if (sh->regs->SR.part.T==1) sh->regs->R[n]|=0x00000001;
  else sh->regs->R[n]&=0xFFFFFFFE;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rotr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  sh->regs->R[n]>>=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x80000000;
  else sh->regs->R[n]&=0x7FFFFFFF;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rte(SuperH * sh) {
  unsigned long temp;
  temp=sh->regs->PC;
  sh->regs->PC = sh->memoire->getLong(sh->regs->R[15]);
  sh->regs->R[15] += 4;
  sh->regs->SR.all = sh->memoire->getLong(sh->regs->R[15]) & 0x000003F3;
  sh->regs->R[15] += 4;
  sh->cycleCount += 4;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void rts(SuperH * sh) {
  unsigned long temp;

  temp = sh->regs->PC;
  sh->regs->PC = sh->regs->PR;

  sh->cycleCount += 2;
  sh->delay(temp + 2);

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void sett(SuperH * sh) {
  sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shal(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n] & 0x80000000) == 0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  sh->regs->R[n] <<= 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shar(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  if ((sh->regs->R[n]&0x80000000)==0) temp = 0;
  else temp = 1;
  sh->regs->R[n] >>= 1;
  if (temp == 1) sh->regs->R[n] |= 0x80000000;
  else sh->regs->R[n] &= 0x7FFFFFFF;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shll(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]<<=1;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shll2(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] <<= 2;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shll8(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]<<=8;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shll16(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]<<=16;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shlr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]>>=1;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shlr2(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=2;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shlr8(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=8;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void shlr16(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=16;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->GBR;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcmgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->GBR);
  sh->regs->PC+=2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcmsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->SR.all);
  sh->regs->PC+=2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcmvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->VBR);
  sh->regs->PC+=2;
  sh->cycleCount += 2;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] = sh->regs->SR.all;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stcvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->VBR;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stsmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->MACH;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stsmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->MACL;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stsmmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->MACH); 
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stsmmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->MACL);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stsmpr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->PR);
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void stspr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] = sh->regs->PR;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void sub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=sh->regs->R[m];
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void subc(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  unsigned long tmp0,tmp1;
  
  tmp1 = sh->regs->R[n] - sh->regs->R[m];
  tmp0 = sh->regs->R[n];
  sh->regs->R[n] = tmp1 - sh->regs->SR.part.T;
  if (tmp0 < tmp1) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  if (tmp1 < sh->regs->R[n]) sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void subv(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  long dest,src,ans;

  if ((long)sh->regs->R[n]>=0) dest=0;
  else dest=1;
  if ((long)sh->regs->R[m]>=0) src=0;
  else src=1;
  src+=dest;
  sh->regs->R[n]-=sh->regs->R[m];
  if ((long)sh->regs->R[n]>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (src==1) {
    if (ans==1) sh->regs->SR.part.T=1;
    else sh->regs->SR.part.T=0;
  }
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void swapb(SuperH * sh) {
  unsigned long temp0,temp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp0=sh->regs->R[m]&0xffff0000;
  temp1=(sh->regs->R[m]&0x000000ff)<<8;
  sh->regs->R[n]=(sh->regs->R[m]>>8)&0x000000ff;
  sh->regs->R[n]=sh->regs->R[n]|temp1|temp0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void swapw(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=(sh->regs->R[m]>>16)&0x0000FFFF;
  sh->regs->R[n]=sh->regs->R[m]<<16;
  sh->regs->R[n]|=temp;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void tas(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->regs->R[n]);
  if (temp==0) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  temp|=0x00000080;
  sh->memoire->setByte(sh->regs->R[n],temp);
  sh->regs->PC+=2;
  sh->cycleCount += 4;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void trapa(SuperH * sh) {
  long imm = Instruction::cd(sh->instruction);

  sh->regs->R[15]-=4;
  sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);
  sh->regs->R[15]-=4;
  sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);
  sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(imm<<2));
  sh->cycleCount += 8;

#ifdef DYNAREC
  sh->blockEnd = true;

  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void tst(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n]&sh->regs->R[m])==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void tsti(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=sh->regs->R[0]&i;
  if (temp==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void tstm(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->regs->GBR+sh->regs->R[0]);
  temp&=i;
  if (temp==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void y_xor(SuperH * sh) {
  int b = Instruction::b(sh->instruction);
  int c = Instruction::c(sh->instruction);

#ifdef DYNAREC
  if(!sh->compile_only) {
#endif

  sh->regs->R[b] ^= sh->regs->R[c];
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  }
  if (sh->compile) {
    sh->block[sh->currentBlock].cycleCount += 1;

    sh->mapReg(b);
    sh->mapReg(c);
    jit_xorr_ul(sh->reg(b), sh->reg(b), sh->reg(c));

    sh->mapReg(R_PC);
    jit_addi_i(sh->reg(R_PC), sh->reg(R_PC), 2);
  }
#endif
}

void xori(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  sh->regs->R[0] ^= source;
  sh->regs->PC += 2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void xorm(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  long temp;

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp ^= source;
  sh->memoire->setByte(sh->regs->GBR + sh->regs->R[0],temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void xtrct(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp=(sh->regs->R[m]<<16)&0xFFFF0000;
  sh->regs->R[n]=(sh->regs->R[n]>>16)&0x0000FFFF;
  sh->regs->R[n]|=temp;
  sh->regs->PC+=2;
  sh->cycleCount++;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

void sleep(SuperH * sh) {
  //cerr << "SLEEP" << endl;
  sh->cycleCount += 3;

#ifdef DYNAREC
  if (sh->compile) {
    sh->block[sh->currentBlock].status = BLOCK_FAILED;
  }
#endif
}

SuperH::opcode SuperH::decode(void) {
  
  switch (Instruction::a(instruction)) {
  case 0:
    switch (Instruction::d(instruction)) {
    case 2:
      switch (Instruction::c(instruction)) {
      case 0: return &stcsr;
      case 1: return &stcgbr;
      case 2: return &stcvbr;
      default: return &undecoded;
      }
     
    case 3:
      switch (Instruction::c(instruction)) {
      case 0: return &bsrf;
      case 2: return &braf;
      default: return &undecoded;
      }
     
    case 4: return &movbs0;
    case 5: return &movws0;
    case 6: return &movls0;
    case 7: return &mull;
    case 8:
      switch (Instruction::c(instruction)) {
      case 0: return &clrt;
      case 1: return &sett;
      case 2: return &clrmac;
      default: return &undecoded;
      }
     
    case 9:
      switch (Instruction::c(instruction)) {
      case 0: return &nop;
      case 1: return &div0u;
      case 2: return &movt;
      default: return &undecoded;
      }
     
    case 10:
      switch (Instruction::c(instruction)) {
      case 0: return &stsmach;
      case 1: return &stsmacl;
      case 2: return &stspr;
      default: return &undecoded;
      }
     
    case 11:
      switch (Instruction::c(instruction)) {
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
    switch (Instruction::d(instruction)) {
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
    switch(Instruction::d(instruction)) {
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
    switch(Instruction::d(instruction)) {
    case 0:
      switch(Instruction::c(instruction)) {
      case 0: return &shll;
      case 1: return &dt;
      case 2: return &shal;
      default: return &undecoded;
      }
     
    case 1:
      switch(Instruction::c(instruction)) {
      case 0: return &shlr;
      case 1: return &cmppz;
      case 2: return &shar;
      default: return &undecoded;
      }
     
    case 2:
      switch(Instruction::c(instruction)) {
      case 0: return &stsmmach;
      case 1: return &stsmmacl;
      case 2: return &stsmpr;
      default: return &undecoded;
      }
     
    case 3:
      switch(Instruction::c(instruction)) {
      case 0: return &stcmsr;
      case 1: return &stcmgbr;
      case 2: return &stcmvbr;
      default: return &undecoded;
      }
     
    case 4:
      switch(Instruction::c(instruction)) {
      case 0: return &rotl;
      case 2: return &rotcl;
      default: return &undecoded;
      }
     
    case 5:
      switch(Instruction::c(instruction)) {
      case 0: return &rotr;
      case 1: return &cmppl;
      case 2: return &rotcr;
      default: return &undecoded;
      }
     
    case 6:
      switch(Instruction::c(instruction)) {
      case 0: return &ldsmmach;
      case 1: return &ldsmmacl;
      case 2: return &ldsmpr;
      default: return &undecoded;
      }
     
    case 7:
      switch(Instruction::c(instruction)) {
      case 0: return &ldcmsr;
      case 1: return &ldcmgbr;
      case 2: return &ldcmvbr;
      default: return &undecoded;
      }
     
    case 8:
      switch(Instruction::c(instruction)) {
      case 0: return &shll2;
      case 1: return &shll8;
      case 2: return &shll16;
      default: return &undecoded;
      }
     
    case 9:
      switch(Instruction::c(instruction)) {
      case 0: return &shlr2;
      case 1: return &shlr8;
      case 2: return &shlr16;
      default: return &undecoded;
      }
     
    case 10:
      switch(Instruction::c(instruction)) {
      case 0: return &ldsmach;
      case 1: return &ldsmacl;
      case 2: return &ldspr;
      default: return &undecoded;
      }
     
    case 11:
      switch(Instruction::c(instruction)) {
      case 0: return &jsr;
      case 1: return &tas;
      case 2: return &jmp;
      default: return &undecoded;
      }
     
    case 14:
      switch(Instruction::c(instruction)) {
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
    switch (Instruction::d(instruction)) {
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
    switch (Instruction::b(instruction)) {
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
    switch(Instruction::b(instruction)) {
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

void SuperH::GetRegisters(sh2regs_struct * r) {
  if (r != NULL) {
    memcpy(r->R, regs->R, sizeof(regs->R));
    r->SR.all = regs->SR.all;
    r->GBR = regs->GBR;
    r->VBR = regs->VBR;
    r->MACH = regs->MACH;
    r->MACL = regs->MACL;
    r->PR = regs->PR;
    r->PC = regs->PC;
  }
}

void SuperH::SetRegisters(sh2regs_struct * r) {
  if (regs != NULL) {
    memcpy(regs->R, r->R, sizeof(regs->R));
    regs->SR.all = r->SR.all;
    regs->GBR = r->GBR;
    regs->VBR = r->VBR;
    regs->MACH = r->MACH;
    regs->MACL = r->MACL;
    regs->PR = r->PR;
    regs->PC = r->PC;
  }
}

void SuperH::SetBreakpointCallBack(void (*func)(bool, unsigned long)) {
   BreakpointCallBack = func;
}

int SuperH::AddCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints < MAX_BREAKPOINTS) {
     codebreakpoint[numcodebreakpoints].addr = addr;
     numcodebreakpoints++;

     return 0;
  }

  return -1;
}

int SuperH::DelCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints > 0) {
     for (int i = 0; i < numcodebreakpoints; i++) {
        if (codebreakpoint[i].addr == addr)
        {
           codebreakpoint[i].addr = 0xFFFFFFFF;
           codebreakpoint[i].oldopcode = 0xFFFF;
           SortCodeBreakpoints();
           numcodebreakpoints--;
           return 0;
        }
     }
  }

  return -1;
}

codebreakpoint_struct *SuperH::GetBreakpointList() {
  return codebreakpoint;
}

void SuperH::ClearCodeBreakpoints() {
     for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        codebreakpoint[i].addr = 0xFFFFFFFF;
        codebreakpoint[i].oldopcode = 0xFFFF;
     }

     numcodebreakpoints = 0;
}

void SuperH::SortCodeBreakpoints() {
  int i, i2;
  unsigned long tmp;

  for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
  {
     for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
     {
        if (codebreakpoint[i].addr == 0xFFFFFFFF &&
            codebreakpoint[i2].addr != 0xFFFFFFFF)
        {
           tmp = codebreakpoint[i].addr;
           codebreakpoint[i].addr = codebreakpoint[i2].addr;
           codebreakpoint[i2].addr = tmp;

           tmp = codebreakpoint[i].oldopcode;
           codebreakpoint[i].oldopcode = codebreakpoint[i2].oldopcode;
           codebreakpoint[i2].oldopcode = tmp;
        }
     }
  }
}

int SuperH::saveState(FILE *fp) {
   int offset;

   // Write header
   if (isslave == false)
      offset = stateWriteHeader(fp, "MSH2", 1);
   else
   {
      offset = stateWriteHeader(fp, "SSH2", 1);
      fwrite((void *)&memoire->sshRunning, 1, 1, fp);
   }
   // Write registers
   fwrite((void *)regs_array, 4, 23, fp);

   // Write onchip registers
   fwrite((void *)onchip->getBuffer(), 0x200, 1, fp);

   return stateFinishHeader(fp, offset);
}

int SuperH::loadState(FILE *fp, int version, int size) {
   if (isslave == true)
      fread((void *)&(memoire->sshRunning), 1, 1, fp);

   // Read registers
   fread((void *)regs_array, 4, 23, fp);

   // Read onchip registers
   fread((void *)onchip->getBuffer(), 0x200, 1, fp);

   return size;
}