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

#include "smpc.hh"
#include "cs2.hh"
#include "scsp.hh"
#include "scu.hh"
#include "timer.hh"
#include "vdp1.hh"
#include "vdp2.hh"
#include "yui.hh"
#include <time.h>

unsigned char Smpc::getIREG(int i) {
  return getByte(1 + 2 * i);
}
unsigned char Smpc::getCOMREG(void) {
  return getByte(0x1F);
}
unsigned char Smpc::getOREG(int i) {
  return getByte(0x21 + 2 * i);
}
unsigned char Smpc::getSR(void) {
  return getByte(0x61);
}
unsigned char Smpc::getSF(void) {
  return getByte(0x63);
}
void Smpc::setIREG(int i, unsigned char val) {
  setByte(i + 2 * i, val);
}
void Smpc::setCOMREG(unsigned char val) {
  setByte(0x1F, val);
}
void Smpc::setOREG(int i, unsigned char val) {
  setByte(0x21 + 2 * i, val);
}
void Smpc::setSR(unsigned char val) {
  setByte(0x61, val);
}
void Smpc::setSF(unsigned char val) {
  setByte(0x63, val);
}

void Smpc::setByte(unsigned long addr, unsigned char value) {
  Memory::setByte(addr, value);

  if (addr == 1) {  // Maybe an INTBACK continue/break request
    if ((intbackIreg0 & 0x80) != (getIREG(0) & 0x80)) {
      // Continue
      setTiming();
      setSF(1);
    }
    if ((intbackIreg0 & 0x40) != (getIREG(0) & 0x40)) {
      // Break
      intback = false;
      setSR(getSR() & 0x0F);
    }
  }
  if (addr == 0x1F) {
    setTiming();
  }
}

Smpc::Smpc(SaturnMemory *sm) : Memory(0xFF, 0x80) {
  dotsel = false;
  mshnmi = false;
  sndres = false;
  cdres = false;
  sysres = false;
  resb = false;
  ste = false;
  resd = false;
  intback = false;
  firstPeri = false;

  // get region
  regionid = yui_region();

  if (regionid == 0)
  {
     // Time to autodetect the region using the cd block
     regionid = ((Cs2 *)sm->getCS2())->GetRegionID();          
  }
  
  this->sm = sm;
  reset();
}

void Smpc::reset(void) {
  memset(SMEM, 0, 4); // This should be loaded from file
  timing = 0;

  // SMPC registers should be initialized here
}

void Smpc::setTiming(void) {
	switch(getCOMREG()) {
                case 0x0:
#if DEBUG
                        cerr << "smpc\t: MSHON not implemented\n";
#endif
			timing = 30;
                        break;
                case 0x8:
#if DEBUG
                        cerr << "smpc\t: CDON not implemented\n";
#endif
                        timing = 40;
                        break;
                case 0x9:
#if DEBUG
                        cerr << "smpc\t: CDOFF not implemented\n";
#endif
                        timing = 40;
                        break;
                case 0xD:
                case 0xE:
                case 0xF:
//                        timing = 1000;
                        timing = 1; // this is obviously wrong, but if I don't do it, it never gets called
                        break;
		case 0x10:
			if (intback) timing = 20;
			else timing = 3000;
			break;
                case 0x17:
                        timing = 40;
                        break;
                case 0x2:
                case 0x3:
		case 0x6:
		case 0x7:
		case 0x19:
		case 0x1A:
			timing = 30;
			break;
		default:
                        cerr << "smpc\t: unimplemented command: " << hex << (int) getCOMREG() << endl;
			break;
	}
}

void Smpc::execute2(unsigned long t) {
	if (timing > 0) {
		timing -= t;
		if (timing <= 0) {
			execute(this);
		}
	}
}

void Smpc::execute(Smpc *smpc) {
  switch(smpc->getCOMREG()) {
  case 0x0:
#if DEBUG
    cerr << "smpc\t: MSHON not implemented\n";
#endif
    break;
  case 0x2:
#if DEBUG
    cerr << "smpc\t: SSHON\n";
#endif
    smpc->SSHON();
    break;
  case 0x3:
#if DEBUG
    cerr << "smpc\t: SSHOFF\n";
#endif
    smpc->SSHOFF();
    break;
  case 0x6:
#if DEBUG
    cerr << "smpc\t: SNDON\n";
#endif
    smpc->SNDON();
    break;
  case 0x7:
#if DEBUG
    cerr << "smpc\t: SNDOFF\n";
#endif
    smpc->SNDOFF();
    break;
  case 0x8:
#if DEBUG
    cerr << "smpc\t: CDON not implemented\n";
#endif
    break;
  case 0x9:
#if DEBUG
    cerr << "smpc\t: CDOFF not implemented\n";
#endif
    break;
  case 0xD:
#if DEBUG
    cerr << "smpc\t: SYSRES not implemented\n";
#endif
    break;
  case 0xE:
#if DEBUG
    cerr << "smpc\t: CKCHG352\n";
#endif
    smpc->CKCHG352();
    break;
  case 0xF:
#if DEBUG
    cerr << "smpc\t: CKCHG320\n";
#endif
    smpc->CKCHG320();
    break;
  case 0x10:
#if DEBUG
    //cerr << "smpc\t: INTBACK\n";
#endif
    smpc->INTBACK();
    break;
  case 0x17:
#if DEBUG
    cerr << "smpc\t: SETSMEM\n";
#endif
    smpc->SETSMEM();
    break;
  case 0x19:
#if DEBUG
    cerr << "smpc\t: RESENAB\n";
#endif
    smpc->RESENAB();
    break;
  case 0x1A:
#if DEBUG
    cerr << "smpc\t: RESDISA\n";
#endif
    smpc->RESDISA();
    break;
  default:
#if DEBUG
    cerr << "smpc\t: command " << ((int) smpc->getCOMREG())
         << " not implemented\n";
#endif
    break;
  }
  
  smpc->setSF(0);
}

void Smpc::INTBACK(void) {
  setSF(1);
  if (intback) {
    INTBACKPeripheral();
    //intback = false;
    ((Scu *) sm->getScu())->sendSystemManager();
    return;
  }
  if (intbackIreg0 = getIREG(0)) {
    // Return non-peripheral data
    firstPeri = true;
    intback = getIREG(1) & 0x8; // does the program want peripheral data too?
    setSR(0x40 | (intback << 5));
    INTBACKStatus();
    ((Scu *) sm->getScu())->sendSystemManager();
    return;
  }
  if (getIREG(1) & 0x8) {
    firstPeri = true;
    intback = true;
    setSR(0x40);
    INTBACKPeripheral();
    ((Scu *) sm->getScu())->sendSystemManager();
    return;
  }
}

void Smpc::INTBACKStatus(void) {
  //Timer t;
  //t.wait(3000);
#if DEBUG
  //cerr << "INTBACK status\n";
#endif
    // return time, cartidge, zone, etc. data
    
    setOREG(0, 0x80);	// goto normal startup
    //setOREG(0, 0x0);	// goto setclock/setlanguage screen
    
    // write time data in OREG1-7
    time_t tmp = time(NULL);
    struct tm times;
#ifdef WIN32
    memcpy(&times, localtime(&tmp), sizeof(times));
#else
    localtime_r(&tmp, &times);
#endif
    unsigned char year[4] = {
      (1900 + times.tm_year) / 1000,
      ((1900 + times.tm_year) % 1000) / 100,
      (((1900 + times.tm_year) % 1000) % 100) / 10,
      (((1900 + times.tm_year) % 1000) % 100) % 10 };
    setOREG(1, (year[0] << 4) | year[1]);
    setOREG(2, (year[2] << 4) | year[3]);
    setOREG(3, (times.tm_wday << 4) | (times.tm_mon + 1));
    setOREG(4, ((times.tm_mday / 10) << 4) | (times.tm_mday % 10));
    setOREG(5, ((times.tm_hour / 10) << 4) | (times.tm_hour % 10));
    setOREG(6, ((times.tm_min / 10) << 4) | (times.tm_min % 10));
    setOREG(7, ((times.tm_sec / 10) << 4) | (times.tm_sec % 10));

    // write cartidge data in OREG8
    setOREG(8, 0); // FIXME : random value
    
    // write zone data in OREG9 bits 0-7
    // 1 -> japan
    // 2 -> asia/ntsc
    // 4 -> north america
    // 5 -> central/south america/ntsc
    // 6 -> corea
    // A -> asia/pal
    // C -> europe + others/pal
    // D -> central/south america/pal
    setOREG(9, regionid);

    // system state, first part in OREG10, bits 0-7
    // bit | value  | comment
    // ---------------------------
    // 7   | 0      |
    // 6   | DOTSEL |
    // 5   | 1      |
    // 4   | 1      |
    // 3   | MSHNMI |
    // 2   | 1      |
    // 1   | SYSRES | 
    // 0   | SNDRES |
    setOREG(10, 0x34|(dotsel<<6)|(mshnmi<<3)|(sysres<<1)|sndres);
    
    // Etat du syst�me, deuxi�me partie dans OREG11, bit 6
    // system state, second part in OREG11, bit 6
    // bit 6 -> CDRES
    setOREG(11, cdres << 6); // FIXME
    
    // SMEM
    for(int i = 0;i < 4;i++) setOREG(12+i, SMEM[i]);
    
    setOREG(31, 0);
}

void Smpc::RESENAB(void) {
  resd = false;
  setOREG(31, 0x19);
}

void Smpc::RESDISA(void) {
  resd = true;
  setOREG(31, 0x1A);
}

void Smpc::SNDON(void) {
    ((Scsp *) sm->getScsp())->reset68k();  
    ((Scsp *) sm->getScsp())->is68kOn = true;
    setOREG(31, 0x6);
}

void Smpc::SNDOFF(void) {
    ((Scsp *) sm->getScsp())->is68kOn = false;
    setOREG(31, 0x7);
}

void Smpc::INTBACKPeripheral(void) {
#if DEBUG
  //cerr << "INTBACK peripheral\n";
#endif
  if (firstPeri)
    setSR(0xC0 | (getIREG(1) >> 4));
  else
    setSR(0x80 | (getIREG(1) >> 4));

  //Timer t;
  //t.wait(20);

  /* Port Status:
  0x04 - Sega-tap is connected
  0x16 - Multi-tap is connected
  0x21-0x2F - Clock serial peripheral is connected
  0xF0 - Not Connected or Unknown Device
  0xF1 - Peripheral is directly connected */

  /* PeripheralID:
  0x02 - Digital Device Standard Format
  0x13 - Racing Device Standard Format
  0x15 - Analog Device Standard Format
  0x23 - Pointing Device Standard Format
  0x23 - Shooting Device Standard Format
  0x34 - Keyboard Device Standard Format
  0xE1 - Mega Drive 3-Button Pad
  0xE2 - Mega Drive 6-Button Pad
  0xE3 - Saturn Mouse
  0xFF - Not Connected */

  // Port 1
  setOREG(0, 0xF1); //Port Status(Directly Connected)
  setOREG(1, 0x02); //PeripheralID(Standard Pad)
  setOREG(2, buttonbits >> 8);   //First Data
  setOREG(3, buttonbits & 0xFF);  //Second Data

  // Port 2
  setOREG(4, 0xF0); //Port Status(Not Connected)
  setOREG(5, 0xFF); //PeripheralID(Not Connected)
  for(int i = 6;i < 32;i++) setOREG(i, 0);

/*
  Use this as a reference for implementing other peripherals
  // Port 1
  setOREG(0, 0xF1); //Port Status(Directly Connected)
  setOREG(1, 0xE3); //PeripheralID(Shuttle Mouse)
  setOREG(2, 0x00); //First Data
  setOREG(3, 0x00); //Second Data
  setOREG(4, 0x00); //Third Data

  // Port 2
  setOREG(5, 0xF0); //Port Status(Not Connected)
  setOREG(6, 0xFF); //PeripheralID(Not Connected)
  for(int i = 7;i < 32;i++) setOREG(i, 0);
*/
}

void Smpc::SETSMEM(void) {
  for(int i = 0;i < 4;i++) SMEM[i] = getIREG(i);
  setOREG(31, 0x17);
}

void Smpc::SSHOFF(void) {
        sm->stopSlave();
}

void Smpc::SSHON(void) {
	sm->startSlave();
}

void Smpc::CKCHG352(void) {
  // Reset VDP1, VDP2, SCU, and SCSP
  ((Vdp1 *) sm->getVdp1())->reset();  
  ((Vdp2 *) sm->getVdp2())->reset();  
  ((Scu *) sm->getScu())->reset();  
//  ((Scsp *) sm->getScsp())->reset();  

  // Clear VDP1/VDP2 ram

  sm->stopSlave();

  // change clock
}

void Smpc::CKCHG320(void) {
  // Reset VDP1, VDP2, SCU, and SCSP
  ((Vdp1 *) sm->getVdp1())->reset();  
  ((Vdp2 *) sm->getVdp2())->reset();  
  ((Scu *) sm->getScu())->reset();  
//  ((Scsp *) sm->getScsp())->reset();  

  // Clear VDP1/VDP2 ram

  sm->stopSlave();

  // change clock
}
