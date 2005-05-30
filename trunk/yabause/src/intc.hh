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

#ifndef INTC_HH
#define INTC_HH

#include "memory.hh"

#ifndef _arch_dreamcast
#include <queue>
#endif

class Interrupt {
private:
  unsigned char _level;
  unsigned char _vect;
public:
  Interrupt(unsigned char, unsigned char);
  unsigned char level(void) const;
  unsigned char vector(void) const;
};

#ifndef _arch_dreamcast
namespace std {
template<> struct less<Interrupt> {
public:
  bool operator()(const Interrupt& a, const Interrupt& b) {
    return a.level() < b.level();
  }
};
}
#endif

class SaturnMemory;
class SuperH;

typedef struct
{
   unsigned char SMR;
   unsigned char BRR;
   unsigned char SCR;
   unsigned char TDR;
   unsigned char SSR;
   unsigned char RDR;
   unsigned char TIER;
   unsigned char FTCSR;
   unsigned short FRC;
   unsigned short OCRA;
   unsigned short OCRB;
   unsigned char TCR;
   unsigned char TOCR;
   unsigned short FICR;
   unsigned short IPRB;
   unsigned short VCRA;
   unsigned short VCRB;
   unsigned short VCRC;
   unsigned short VCRD;
   unsigned char DRCR0;
   unsigned char DRCR1;
   unsigned char WTCSR;
   unsigned char WTCNT;
   unsigned char RSTCSR;
   unsigned char SBYCR;
   unsigned char CCR;
   unsigned short ICR;
   unsigned short IPRA;
   unsigned short VCRWDT;
   unsigned long DVSR;
   unsigned long DVCR;
   unsigned long VCRDIV;
   unsigned long DVDNTH;
   unsigned long DVDNTL;
   unsigned long BARA;
   unsigned long BAMRA;
   unsigned short BBRA;
   unsigned long BARB;
   unsigned long BAMRB;
   unsigned short BBRB;
   unsigned long BDRB;
   unsigned long BDMRB;
   unsigned short BRCR;
   unsigned long SAR0;
   unsigned long DAR0;
   unsigned long TCR0;
   unsigned long CHCR0;
   unsigned long SAR1;
   unsigned long DAR1;
   unsigned long TCR1;
   unsigned long CHCR1;
   unsigned long VCRDMA0;
   unsigned long VCRDMA1;
   unsigned long DMAOR;
   unsigned short BCR1;
   unsigned short BCR2;
   unsigned short WCR;
   unsigned short MCR;
   unsigned short RTCSR;
   unsigned short RTCNT;
   unsigned short RTCOR;
} onchipregs_struct;

class Onchip : public Dummy {
private:
	SaturnMemory *memory;
        SuperH *shparent;
	unsigned long timing;
        bool isslave;

        onchipregs_struct reg;

        /* FRT */
        unsigned long ccleftover;
        unsigned long frcdiv;
        unsigned short ocra;
        unsigned short ocrb;

        /* WDT */
        unsigned long wdtdiv;
        bool wdtenable;
        bool wdtinterval;
        unsigned long wdtleftover;

        /* DMAC */
       inline void DMATransfer(unsigned long *CHCR, unsigned long *SAR,
                               unsigned long *DAR, unsigned long *TCR,
                               unsigned long *VCRDMA);

	/* INTC */

public:
#ifndef _arch_dreamcast
	//priority_queue<Interrupt> interrupts;
#endif

        Onchip(bool, SaturnMemory *, SuperH *);
        void reset(void);

        void setByte(unsigned long, unsigned char);
        unsigned char getByte(unsigned long);
	void setWord(unsigned long, unsigned short);
        unsigned short getWord(unsigned long);
	void setLong(unsigned long, unsigned long);
        unsigned long getLong(unsigned long);

	void run(unsigned long);

	/* DMAC */
	void runDMA(void);

	/* INTC */
/*
	void sendNMI(void);
	void sendUserBreak(void);
	void send(const Interrupt&);
*/
	//void sendOnChip(...);

        /* FRT */
        void runFRT(unsigned long cc);
        void inputCaptureSignal(void);

        /* WDT */
        void runWDT(unsigned long cc);
};

class InputCaptureSignal : public Memory {
private:
        Onchip *onchip;
public:
        InputCaptureSignal(SuperH *icsh);
        void setByte(unsigned long, unsigned char);
	void setWord(unsigned long, unsigned short);
        void setLong(unsigned long, unsigned long);
};

#endif
