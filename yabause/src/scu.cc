/*  Copyright 2003 Guillaume Duhamel

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

#include "scu.hh"
#include "superh.hh"
#include "registres.hh"

unsigned long Scu::getLong(unsigned long addr) {
	if (addr == 0x80) return 0;        
	return Memory::getLong(addr);
}

void Scu::setLong(unsigned long addr, unsigned long val) {
	switch(addr) {
                case 0x10: if (val & 0x1) DMA(0);
			   Memory::setLong(addr, val);
			   break;
                case 0x14: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 0 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x30: if (val & 0x1) DMA(1);
			   Memory::setLong(addr, val);
			   break;
                case 0x34: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 1 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x50: if (val & 0x1) DMA(2);
			   Memory::setLong(addr, val);
			   break;
                case 0x54: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 2 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x80: if (val & 0x10000) {                           
#if DEBUG
                             cerr << "scu\t: DSP execution not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x90: 
#if DEBUG
                             cerr << "scu\t: Timer 0 Compare Register set to " << hex << val << endl;
#endif
			   Memory::setLong(addr, val);
                           break;
                case 0xA4: 
                           Memory::setLong(addr, Memory::getLong(addr) & val);
                           break;
		default:
                           Memory::setLong(addr, val);
	}
}

Scu::Scu(SaturnMemory *i) : Memory(0xFF, 0xD0) {
 	satmem = i;
        reset();
}

void Scu::reset(void) {
	Memory::setLong(0xA0, 0x0000BFFF);
	Memory::setLong(0xA4, 0);
        timer0 = timer1 = 0;
}

void Scu::DMA(int mode) {
	int i = mode * 0x20;
	unsigned long readAddress = getLong(i);
	unsigned long writeAddress = getLong(i + 0x4);
	unsigned long transferNumber = getLong(i + 0x8);
	unsigned long addValue = getLong(i + 0xC);
	unsigned char readAdd, writeAdd;

	if (addValue & 0x100) readAdd = 4;
	else readAdd = 0;
	switch(addValue & 0x7) {
		case 0x0: writeAdd = 0; break;
		case 0x1: writeAdd = 2; break;
		case 0x2: writeAdd = 4; break;
		case 0x3: writeAdd = 8; break;
		case 0x4: writeAdd = 16; break;
		case 0x5: writeAdd = 32; break;
		case 0x6: writeAdd = 64; break;
		case 0x7: writeAdd = 128; break;
	}
	if (getLong(i + 0x14) & 0x1000000) {
                unsigned long tempreadAddress;
                unsigned long tempwriteAddress;
                unsigned long temptransferNumber;
                unsigned long test, test2;

                for (;;) {
                   unsigned long counter = 0;

                   temptransferNumber=satmem->getLong(writeAddress);
                   tempwriteAddress=satmem->getLong(writeAddress+4);
                   tempreadAddress=satmem->getLong(writeAddress+8);
                   test = tempwriteAddress & 0x1FFFFFFF;
                   test2 = tempreadAddress & 0x80000000;

                   if (mode > 0) temptransferNumber &= 0xFFF;

                   tempreadAddress &= 0x7FFFFFFF;
    
                   if ((test >= 0x5A00000) && (test < 0x5FF0000)) {
                        while(counter < temptransferNumber) {
                                unsigned long tmp = satmem->getLong(tempreadAddress);
                                satmem->setWord(tempwriteAddress, tmp >> 16);
                                tempwriteAddress += writeAdd;
                                satmem->setWord(tempwriteAddress, tmp & 0xFFFF);
                                tempwriteAddress += writeAdd;
                                tempreadAddress += readAdd;
				counter += 4;
			}
                   }
                   else {
#if DEBUG
                        cerr << "indirect DMA, A Bus, not implemented" << endl;
#endif
                   }


                   if (test2) break;

                   writeAddress+= 0xC;
                }

		switch(mode) {
			case 0: sendLevel0DMAEnd(); break;
			case 1: sendLevel1DMAEnd(); break;
			case 2: sendLevel2DMAEnd(); break;
		}
	}
	else {
		unsigned long counter = 0;
		unsigned long test = writeAddress & 0x1FFFFFFF;

                if (mode > 0) transferNumber &= 0xFFF;

		if ((test >= 0x5A00000) && (test < 0x5FF0000)) {
			while(counter < transferNumber) {
				unsigned long tmp = satmem->getLong(readAddress);
				satmem->setWord(writeAddress, tmp >> 16);
				writeAddress += writeAdd;
				satmem->setWord(writeAddress, tmp & 0xFFFF);
				writeAddress += writeAdd;
				readAddress += readAdd;
				counter += 4;
			}
		}
		else {
#if DEBUG
                        cerr << "direct DMA, A Bus, not tested yet" << endl;
#endif
			while(counter < transferNumber) {
				satmem->setLong(writeAddress, satmem->getLong(readAddress));
				readAddress += readAdd;
				writeAddress += writeAdd;
				counter += 4;
			}
		}
		switch(mode) {
			case 0: sendLevel0DMAEnd(); break;
			case 1: sendLevel1DMAEnd(); break;
			case 2: sendLevel2DMAEnd(); break;
		}
	}
}

void Scu::sendVBlankIN(void)      { sendInterrupt<0x40, 0xF, 0x0001>(); }
void Scu::sendVBlankOUT(void) {
   sendInterrupt<0x41, 0xE, 0x0002>();
   timer0 = 0;
}
void Scu::sendHBlankIN(void) {
   sendInterrupt<0x42, 0xD, 0x0004>();
   timer0++;
   // if timer0 equals timer 0 compare register, do an interrupt
   if (timer0 == Memory::getLong(0x90)) sendTimer0(); 
}
void Scu::sendTimer0(void)        { sendInterrupt<0x43, 0xC, 0x0008>(); }
void Scu::sendTimer1(void)        { sendInterrupt<0x44, 0xB, 0x0010>(); }
void Scu::sendDSPEnd(void)        { sendInterrupt<0x45, 0xA, 0x0020>(); }
void Scu::sendSoundRequest(void)  { sendInterrupt<0x46, 0x9, 0x0040>(); }
void Scu::sendSystemManager(void) { sendInterrupt<0x47, 0x8, 0x0080>(); }
void Scu::sendPadInterrupt(void)  { sendInterrupt<0x48, 0x8, 0x0100>(); }
void Scu::sendLevel2DMAEnd(void)  { sendInterrupt<0x49, 0x6, 0x0200>(); }
void Scu::sendLevel1DMAEnd(void)  { sendInterrupt<0x4A, 0x6, 0x0400>(); }
void Scu::sendLevel0DMAEnd(void)  { sendInterrupt<0x4B, 0x5, 0x0800>(); }
void Scu::sendDMAIllegal(void)    { sendInterrupt<0x4C, 0x3, 0x1000>(); }
void Scu::sendDrawEnd(void)       { sendInterrupt<0x4D, 0x2, 0x2000>(); }
