/*  Copyright 2003-2005 Guillaume Duhamel
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

#include <stdlib.h>
#include <time.h>
#include "smpc.h"
#include "cs2.h"
#include "debug.h"
#include "scu.h"
#include "vdp1.h"
#include "vdp2.h"
#include "yabause.h"

Smpc * SmpcRegs;
u8 * SmpcRegsT;
SmpcInternal * SmpcInternalVars;

//////////////////////////////////////////////////////////////////////////////

int SmpcInit(u8 regionid) {
   if ((SmpcRegsT = (u8 *) calloc(1, sizeof(Smpc))) == NULL)
      return -1;

   SmpcRegs = (Smpc *) SmpcRegsT;

   if ((SmpcInternalVars = (SmpcInternal *) calloc(1, sizeof(SmpcInternal))) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcDeInit(void) {
   if (SmpcRegsT)
      free(SmpcRegsT);

   if (SmpcInternalVars)
      free(SmpcInternalVars);
}

//////////////////////////////////////////////////////////////////////////////

void SmpcReset(void) {
   memset((void *)SmpcRegs, 0, sizeof(Smpc));
   memset((void *)SmpcInternalVars->SMEM, 0, 4);

   if (SmpcInternalVars->regionid == 0)
   {
      // Time to autodetect the region using the cd block
      SmpcInternalVars->regionid = Cs2GetRegionID();          
   }

   SmpcInternalVars->dotsel = 0;
   SmpcInternalVars->mshnmi = 0;
   SmpcInternalVars->sysres = 0;
   SmpcInternalVars->sndres = 0;
   SmpcInternalVars->cdres = 0;
   SmpcInternalVars->resd = 1;
   SmpcInternalVars->ste = 0;
   SmpcInternalVars->resb = 0;

   SmpcInternalVars->intback=0;
   SmpcInternalVars->intbackIreg0=0;
   SmpcInternalVars->firstPeri=0;

   SmpcInternalVars->timing=0;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSSHON() {
   YabStartSlave();
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSSHOFF() {
   YabStopSlave();
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSNDON() {
//   sm->soundr->reset68k();  
//   sm->soundr->is68kOn = true;
   SmpcRegs->OREG[31] = 0x6;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSNDOFF() {
//   sm->soundr->is68kOn = false;
   SmpcRegs->OREG[31] = 0x7;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcCKCHG352() {
   // Reset VDP1, VDP2, SCU, and SCSP
   Vdp1Reset();  
   Vdp2Reset();  
   ScuReset();  
//   ((Scsp *) sm->getScsp())->reset();  

   // Clear VDP1/VDP2 ram

   YabStopSlave();

   // change clock
}

//////////////////////////////////////////////////////////////////////////////

void SmpcCKCHG320() {
   // Reset VDP1, VDP2, SCU, and SCSP
   Vdp1Reset();  
   Vdp2Reset();  
   ScuReset();  
//   ((Scsp *) sm->getScsp())->reset();  

   // Clear VDP1/VDP2 ram

   YabStopSlave();

   // change clock
}

//////////////////////////////////////////////////////////////////////////////

void SmpcINTBACKStatus(void) {
   // return time, cartidge, zone, etc. data
   int i;
   struct tm times;
   unsigned char year[4];

   SmpcRegs->OREG[0] = 0x80;   // goto normal startup
   //SmpcRegs->OREG[0] = 0x0;  // goto setclock/setlanguage screen
    
   // write time data in OREG1-7
   time_t tmp = time(NULL);
#ifdef WIN32
   memcpy(&times, localtime(&tmp), sizeof(times));
#else
   localtime_r(&tmp, &times);
#endif
   year[0] = (1900 + times.tm_year) / 1000;
   year[1] = ((1900 + times.tm_year) % 1000) / 100;
   year[2] = (((1900 + times.tm_year) % 1000) % 100) / 10;
   year[3] = (((1900 + times.tm_year) % 1000) % 100) % 10;
   SmpcRegs->OREG[1] = (year[0] << 4) | year[1];
   SmpcRegs->OREG[2] = (year[2] << 4) | year[3];
   SmpcRegs->OREG[3] = (times.tm_wday << 4) | (times.tm_mon + 1);
   SmpcRegs->OREG[4] = ((times.tm_mday / 10) << 4) | (times.tm_mday % 10);
   SmpcRegs->OREG[5] = ((times.tm_hour / 10) << 4) | (times.tm_hour % 10);
   SmpcRegs->OREG[6] = ((times.tm_min / 10) << 4) | (times.tm_min % 10);
   SmpcRegs->OREG[7] = ((times.tm_sec / 10) << 4) | (times.tm_sec % 10);

   // write cartidge data in OREG8
   SmpcRegs->OREG[8] = 0; // FIXME : random value
    
   // write zone data in OREG9 bits 0-7
   // 1 -> japan
   // 2 -> asia/ntsc
   // 4 -> north america
   // 5 -> central/south america/ntsc
   // 6 -> corea
   // A -> asia/pal
   // C -> europe + others/pal
   // D -> central/south america/pal
   SmpcRegs->OREG[9] = SmpcInternalVars->regionid;

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
   SmpcRegs->OREG[10] = 0x34|(SmpcInternalVars->dotsel<<6)|(SmpcInternalVars->mshnmi<<3)|(SmpcInternalVars->sysres<<1)|SmpcInternalVars->sndres;
    
   // Etat du syst�me, deuxi�me partie dans OREG11, bit 6
   // system state, second part in OREG11, bit 6
   // bit 6 -> CDRES
   SmpcRegs->OREG[11] = SmpcInternalVars->cdres << 6; // FIXME
    
   // SMEM
   for(i = 0;i < 4;i++)
      SmpcRegs->OREG[12+i] = SmpcInternalVars->SMEM[i];
    
   SmpcRegs->OREG[31] = 0x10; // set to intback command
}

//////////////////////////////////////////////////////////////////////////////

void SmpcINTBACKPeripheral(void) {
  if (SmpcInternalVars->firstPeri)
    SmpcRegs->SR = 0xC0 | (SmpcRegs->IREG[1] >> 4);
  else
    SmpcRegs->SR = 0x80 | (SmpcRegs->IREG[1] >> 4);

  SmpcInternalVars->firstPeri = 0;

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

  /* Special Notes(for potential future uses):

  If a peripheral is disconnected from a port, you only return 1 byte for
  that port(which is the port status 0xF0), at the next OREG you then return
  the port status of the next port.

  e.g. If Port 1 has nothing connected, and Port 2 has a controller
       connected:

  OREG0 = 0xF0
  OREG1 = 0xF1
  OREG2 = 0x02
  etc.
  */

  // Port 1
  SmpcRegs->OREG[0] = 0xF1; //Port Status(Directly Connected)
  SmpcRegs->OREG[1] = 0x02; //PeripheralID(Standard Pad)
  SmpcRegs->OREG[2] = buttonbits >> 8;   //First Data
  SmpcRegs->OREG[3] = buttonbits & 0xFF;  //Second Data

  // Port 2
  SmpcRegs->OREG[4] = 0xF0; //Port Status(Not Connected)
/*
  Use this as a reference for implementing other peripherals
  // Port 1
  SmpcRegs->OREG[0] = 0xF1; //Port Status(Directly Connected)
  SmpcRegs->OREG[1] = 0xE3; //PeripheralID(Shuttle Mouse)
  SmpcRegs->OREG[2] = 0x00; //First Data
  SmpcRegs->OREG[3] = 0x00; //Second Data
  SmpcRegs->OREG[4] = 0x00; //Third Data

  // Port 2
  SmpcRegs->OREG[5] = 0xF0; //Port Status(Not Connected)
*/
}

//////////////////////////////////////////////////////////////////////////////

void SmpcINTBACK() {
   SmpcRegs->SF = 1;

   if (SmpcInternalVars->intback) {
      SmpcINTBACKPeripheral();
      ScuSendSystemManager();
      return;
   }
   if (SmpcInternalVars->intbackIreg0 = SmpcRegs->IREG[0]) {
      // Return non-peripheral data
      SmpcInternalVars->firstPeri = 1;
      SmpcInternalVars->intback = SmpcRegs->IREG[1] & 0x8; // does the program want peripheral data too?
      SmpcINTBACKStatus();
      SmpcRegs->SR = 0x4F | (SmpcInternalVars->intback << 5); // the low nibble is undefined(or 0xF)
      ScuSendSystemManager();
      return;
   }
   if (SmpcRegs->IREG[1] & 0x8) {
      SmpcInternalVars->firstPeri = 1;
      SmpcInternalVars->intback = 1;
      SmpcRegs->SR = 0x40;
      SmpcINTBACKPeripheral();
      SmpcRegs->OREG[31] = 0x10; // may need to be changed
      ScuSendSystemManager();
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void SmpcINTBACKEnd() {
   SmpcInternalVars->intback = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSETSMEM() {
   int i;

   for(i = 0;i < 4;i++)
      SmpcInternalVars->SMEM[i] = SmpcRegs->IREG[i];

   SmpcRegs->OREG[31] = 0x17;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcRESENAB() {
  SmpcInternalVars->resd = 0;
  SmpcRegs->OREG[31] = 0x19;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcRESDISA() {
  SmpcInternalVars->resd = 1;
  SmpcRegs->OREG[31] = 0x1A;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcExec(s32 t) {
   if (SmpcInternalVars->timing > 0) {
      SmpcInternalVars->timing -= t;
      if (SmpcInternalVars->timing <= 0) {
         switch(SmpcRegs->COMREG) {
            case 0x0:
#if DEBUG
               fprintf(stderr, "smpc\t: MSHON not implemented\n");
#endif
               break;
            case 0x2:
#if DEBUG
               fprintf(stderr, "smpc\t: SSHON\n");
#endif
               SmpcSSHON();
               break;
            case 0x3:
#if DEBUG
               fprintf(stderr, "smpc\t: SSHOFF\n");
#endif
               SmpcSSHOFF();
               break;
            case 0x6:
#if DEBUG
               fprintf(stderr, "smpc\t: SNDON\n");
#endif
               SmpcSNDON();
               break;
            case 0x7:
#if DEBUG
               fprintf(stderr, "smpc\t: SNDOFF\n");
#endif
               SmpcSNDOFF();
               break;
            case 0x8:
#if DEBUG
               fprintf(stderr, "smpc\t: CDON not implemented\n");
#endif
               break;
            case 0x9:
#if DEBUG
               fprintf(stderr, "smpc\t: CDOFF not implemented\n");
#endif
               break;
            case 0xD:
#if DEBUG
               fprintf(stderr, "smpc\t: SYSRES not implemented\n");
#endif
               break;
            case 0xE:
#if DEBUG
               fprintf(stderr, "smpc\t: CKCHG352\n");
#endif
               SmpcCKCHG352();
               break;
            case 0xF:
#if DEBUG
               fprintf(stderr, "smpc\t: CKCHG320\n");
#endif
               SmpcCKCHG320();
               break;
            case 0x10:
#if DEBUG
               fprintf(stderr, "smpc\t: INTBACK\n");
#endif
               SmpcINTBACK();
               break;
            case 0x17:
#if DEBUG
               fprintf(stderr, "smpc\t: SETSMEM\n");
#endif
               SmpcSETSMEM();
               break;
            case 0x19:
#if DEBUG
               fprintf(stderr, "smpc\t: RESENAB\n");
#endif
               SmpcRESENAB();
               break;
            case 0x1A:
#if DEBUG
               fprintf(stderr, "smpc\t: RESDISA\n");
#endif
               SmpcRESDISA();
               break;
            default:
#if DEBUG
               fprintf(stderr, "smpc\t: Command %02X not implemented\n", SmpcRegs->COMREG);
#endif
               break;
         }
  
         SmpcRegs->SF = 0;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL SmpcReadByte(u32 addr) {
   addr &= 0x7F;
   return SmpcRegsT[addr >> 1];
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL SmpcReadWord(u32 addr) {
   // byte access only
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SmpcReadLong(u32 addr) {
   // byte access only
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SmpcSetTiming(void) {
   switch(SmpcRegs->COMREG) {
      case 0x0:
#if DEBUG
         fprintf(stderr, "smpc\t: MSHON not implemented\n");
#endif
         SmpcInternalVars->timing = 1;
         return;
      case 0x8:
#if DEBUG
         fprintf(stderr, "smpc\t: CDON not implemented\n");
#endif
         SmpcInternalVars->timing = 1;
         return;
      case 0x9:
#if DEBUG
         fprintf(stderr, "smpc\t: CDOFF not implemented\n");
#endif
         SmpcInternalVars->timing = 1;
         return;
      case 0xD:
      case 0xE:
      case 0xF:
         SmpcInternalVars->timing = 1; // this has to be tested on a real saturn
         return;
      case 0x10:
         if (SmpcInternalVars->intback)
            SmpcInternalVars->timing = 20; // this will need to be verified
         else {
            // Calculate timing based on what data is being retrieved

            SmpcInternalVars->timing = 1;

            // If retrieving non-peripheral data, add 0.2 milliseconds
            if (SmpcRegs->IREG[0] == 0x01)
               SmpcInternalVars->timing += 2;

            // If retrieving peripheral data, add 15 milliseconds
            if (SmpcRegs->IREG[1] & 0x8)
               SmpcInternalVars->timing += 150;
         }
         return;
      case 0x17:
         SmpcInternalVars->timing = 1;
         return;
      case 0x2:
         SmpcInternalVars->timing = 1;
         return;
      case 0x3:
         SmpcInternalVars->timing = 1;                        
         return;
      case 0x6:
      case 0x7:
      case 0x19:
      case 0x1A:
         SmpcInternalVars->timing = 1;
         return;
      default:
         fprintf(stderr, "smpc\t: unimplemented command: %02X\n", SmpcRegs->COMREG);
         SmpcRegs->SF = 0;
         break;
   }
}


//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteByte(u32 addr, u8 val) {
   addr &= 0x7F;
   SmpcRegsT[addr >> 1] = val;

   switch(addr) {
      case 0x01: // Maybe an INTBACK continue/break request
         if (SmpcInternalVars->intback)
         {
            if (SmpcRegs->IREG[0] & 0x40) {
               // Break
               SmpcInternalVars->intback = 0;
               SmpcRegs->SR &= 0x0F;
               break;
            }
            else if (SmpcRegs->IREG[0] & 0x80) {                    
               // Continue
               SmpcRegs->COMREG = 0x10;
               SmpcSetTiming();
               SmpcRegs->SF = 1;
            }
         }
         return;
      case 0x1F:
         SmpcSetTiming();
         return;
      case 0x63:
         SmpcRegs->SF &= 0x1;
         return;
      case 0x75:
         // FIX ME (should support other peripherals)

         switch (SmpcRegs->DDR[0] & 0x7F) { // Which Control Method do we use?
            case 0x40:
#if DEBUG
               fprintf(stderr, "smpc\t: Peripheral TH Control Method not implemented\n");
#endif
               break;
            case 0x60:
               switch (val & 0x60) {
                  case 0x60: // 1st Data
                     val = (val & 0x80) | 0x14 | (buttonbits & 0x8);
                     break;
                  case 0x20: // 2nd Data
                     val = (val & 0x80) | 0x10 | (buttonbits >> 12);
                     break;
                  case 0x40: // 3rd Data
                     val = (val & 0x80) | 0x10 | ((buttonbits >> 8) & 0xF);
                     break;
                  case 0x00: // 4th Data
                     val = (val & 0x80) | 0x10 | ((buttonbits >> 4) & 0xF);
                     break;
                  default: break;
               }

               SmpcRegs->PDR[0] = val;
               break;
            default:
#if DEBUG
               fprintf(stderr, "smpc\t: Peripheral Unknown Control Method not implemented\n");
#endif
               break;
         }

         return;
      default:
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteWord(u32 addr, u16 val) {
   // byte access only
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SmpcWriteLong(u32 addr, u32 val) {
   // byte access only
}

//////////////////////////////////////////////////////////////////////////////

