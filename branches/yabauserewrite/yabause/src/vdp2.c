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
#include "vdp2.h"
#include "debug.h"
#include "scu.h"
#include "sh2core.h"
#include "vdp1.h"
#include "yabause.h"

u8 * Vdp2Ram;
u8 * Vdp2ColorRam;
Vdp2 * Vdp2Regs;
Vdp2Internal_struct Vdp2Internal;

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2RamReadByte(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadByte(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2RamReadWord(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadWord(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2RamReadLong(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadLong(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteByte(u32 addr, u8 val) {
   addr &= 0x7FFFF;
   T1WriteByte(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteWord(u32 addr, u16 val) {
   addr &= 0x7FFFF;
   T1WriteWord(Vdp2Ram, addr, val);

//   if (Vdp2Internal.ColorMode == 0)
//      T1WriteWord(Vdp2Ram, addr + 0x800, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteLong(u32 addr, u32 val) {
   addr &= 0x7FFFF;
   T1WriteLong(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2ColorRamReadByte(u32 addr) {
   addr &= 0xFFF;
   return T2ReadByte(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ColorRamReadWord(u32 addr) {
   addr &= 0xFFF;
   return T2ReadWord(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ColorRamReadLong(u32 addr) {
   addr &= 0xFFF;
   return T2ReadLong(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteByte(u32 addr, u8 val) {
   addr &= 0xFFF;
   T2WriteByte(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteWord(u32 addr, u16 val) {
   addr &= 0xFFF;
   T2WriteWord(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteLong(u32 addr, u32 val) {
   addr &= 0xFFF;
   T2WriteLong(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2Init(int coreid) {
   if ((Vdp2Regs = (Vdp2 *) calloc(1, sizeof(Vdp2))) == NULL)
      return -1;

   if ((Vdp2Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   if ((Vdp2ColorRam = T2MemoryInit(0x1000)) == NULL)
      return -1;

   Vdp2Reset();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DeInit(void) {
   if (Vdp2Regs)
      free(Vdp2Regs);

   if (Vdp2Ram)
      T1MemoryDeInit(Vdp2Ram);

   if (Vdp2ColorRam)
      T2MemoryDeInit(Vdp2ColorRam);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2Reset(void) {
   Vdp2Regs->TVMD = 0x0000;
   Vdp2Regs->EXTEN = 0x0000;
   Vdp2Regs->TVSTAT = Vdp2Regs->TVSTAT & 0x1;
   Vdp2Regs->VRSIZE = 0x0000; // fix me(version should be set)
   Vdp2Regs->RAMCTL = 0x0000;
   Vdp2Regs->BGON = 0x0000;
   Vdp2Regs->CHCTLA = 0x0000;
   Vdp2Regs->CHCTLB = 0x0000;
   Vdp2Regs->BMPNA = 0x0000;
   Vdp2Regs->MPOFN = 0x0000;
   Vdp2Regs->MPABN2 = 0x0000;
   Vdp2Regs->MPCDN2 = 0x0000;
   Vdp2Regs->SCXIN0 = 0x0000;
   Vdp2Regs->SCXDN0 = 0x0000;
   Vdp2Regs->SCYIN0 = 0x0000;
   Vdp2Regs->SCYDN0 = 0x0000;
   Vdp2Regs->ZMXN0.all = 0x00000000;
   Vdp2Regs->ZMYN0.all = 0x00000000;
   Vdp2Regs->SCXIN1 = 0x0000;
   Vdp2Regs->SCXDN1 = 0x0000;
   Vdp2Regs->SCYIN1 = 0x0000;
   Vdp2Regs->SCYDN1 = 0x0000;
   Vdp2Regs->ZMXN1.all = 0x00000000;
   Vdp2Regs->ZMYN1.all = 0x00000000;
   Vdp2Regs->SCXN2 = 0x0000;
   Vdp2Regs->SCYN2 = 0x0000;
   Vdp2Regs->SCXN3 = 0x0000;
   Vdp2Regs->SCYN3 = 0x0000;
   Vdp2Regs->ZMCTL = 0x0000;
   Vdp2Regs->SCRCTL = 0x0000;
   Vdp2Regs->VCSTAU = 0x0000;
   Vdp2Regs->VCSTAL = 0x0000;
   Vdp2Regs->BKTAU = 0x0000;
   Vdp2Regs->BKTAL = 0x0000;
   Vdp2Regs->RPMD = 0x0000;
   Vdp2Regs->RPRCTL = 0x0000;
   Vdp2Regs->KTCTL = 0x0000;
   Vdp2Regs->KTAOF = 0x0000;
   Vdp2Regs->OVPNRA = 0x0000;
   Vdp2Regs->OVPNRB = 0x0000;
   Vdp2Regs->WPSX0 = 0x0000;
   Vdp2Regs->WPSY0 = 0x0000;
   Vdp2Regs->WPEX0 = 0x0000;
   Vdp2Regs->WPEY0 = 0x0000;
   Vdp2Regs->WPSX1 = 0x0000;
   Vdp2Regs->WPSY1 = 0x0000;
   Vdp2Regs->WPEX1 = 0x0000;
   Vdp2Regs->WPEY1 = 0x0000;
   Vdp2Regs->WCTLA = 0x0000;
   Vdp2Regs->WCTLB = 0x0000;
   Vdp2Regs->WCTLC = 0x0000;
   Vdp2Regs->WCTLD = 0x0000;
   Vdp2Regs->SPCTL = 0x0000;
   Vdp2Regs->SDCTL = 0x0000;
   Vdp2Regs->CRAOFA = 0x0000;
   Vdp2Regs->CRAOFB = 0x0000;
   Vdp2Regs->LNCLEN = 0x0000;
   Vdp2Regs->SFPRMD = 0x0000;
   Vdp2Regs->CCCTL = 0x0000;
   Vdp2Regs->SFCCMD = 0x0000;
   Vdp2Regs->PRISA = 0x0000;
   Vdp2Regs->PRISB = 0x0000;
   Vdp2Regs->PRISC = 0x0000;
   Vdp2Regs->PRISD = 0x0000;
   Vdp2Regs->PRINA = 0x0000;
   Vdp2Regs->PRINB = 0x0000;
   Vdp2Regs->PRIR = 0x0000;
   Vdp2Regs->CCRNA = 0x0000;
   Vdp2Regs->CCRNB = 0x0000;
   Vdp2Regs->CLOFEN = 0x0000;
   Vdp2Regs->CLOFSL = 0x0000;
   Vdp2Regs->COAR = 0x0000;
   Vdp2Regs->COAG = 0x0000;
   Vdp2Regs->COAB = 0x0000;
   Vdp2Regs->COBR = 0x0000;
   Vdp2Regs->COBG = 0x0000;
   Vdp2Regs->COBB = 0x0000;

   Vdp2Internal.ColorMode = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2VBlankIN(void) {
   Vdp2Regs->TVSTAT |= 0x0008;
   ScuSendVBlankIN();

   if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x43, 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankIN(void) {
   Vdp2Regs->TVSTAT |= 0x0004;
   ScuSendHBlankIN();

   if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x41, 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankOUT(void) {
   Vdp2Regs->TVSTAT &= ~0x0004;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2VBlankOUT(void) {
   Vdp2Regs->TVSTAT = (Vdp2Regs->TVSTAT & ~0x0008) | 0x0002;

   VIDCore->Vdp2DrawStart();

   if (Vdp2Regs->TVMD & 0x8000) {
      VIDCore->Vdp2DrawScreens();
      Vdp1Draw();
   }

   VIDCore->Vdp2DrawEnd();
   ScuSendVBlankOUT();
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2ReadByte(u32 addr) {
   LOG("VDP2 register byte read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ReadWord(u32 addr) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         return Vdp2Regs->TVMD;
      case 0x002:
         return Vdp2Regs->EXTEN;
      case 0x004:
         // if TVMD's DISP bit is cleared, TVSTAT's VBLANK bit is always set
         if (Vdp2Regs->TVMD & 0x8000)
            return Vdp2Regs->TVSTAT;
         else
            return (Vdp2Regs->TVSTAT | 0x8);
      case 0x006:         
         return Vdp2Regs->VRSIZE;
      case 0x008:
         return Vdp2Regs->HCNT;
      case 0x00A:
         return Vdp2Regs->VCNT;
      default:
      {
         LOG("Unhandled VDP2 word read: %08X\n", addr);
         break;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ReadLong(u32 addr) {
   LOG("VDP2 register long read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteByte(u32 addr, u8 val) {
   LOG("VDP2 register byte write = %08X\n", addr);
   addr &= 0x1FF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteWord(u32 addr, u16 val) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         VIDCore->Vdp2SetResolution(val);
         Vdp2Regs->TVMD = val;
         return;
      case 0x002:
         Vdp2Regs->EXTEN = val;
         return;
      case 0x004:
         // TVSTAT is read-only
         return;
      case 0x006:
         Vdp2Regs->VRSIZE = val;
         return;
      case 0x008:
         // HCNT is read-only
         return;
      case 0x00A:
         // VCNT is read-only
         return;
      case 0x00C:
         // Reserved
         return;
      case 0x00E:
         Vdp2Regs->RAMCTL = val;
         Vdp2Internal.ColorMode = (val >> 12) & 0x3;
         return;
      case 0x010:
         Vdp2Regs->CYCA0L = val;
         return;
      case 0x012:
         Vdp2Regs->CYCA0U = val;
         return;
      case 0x014:
         Vdp2Regs->CYCA1L = val;
         return;
      case 0x016:
         Vdp2Regs->CYCA1U = val;
         return;
      case 0x018:
         Vdp2Regs->CYCB0L = val;
         return;
      case 0x01A:
         Vdp2Regs->CYCB0U = val;
         return;
      case 0x01C:
         Vdp2Regs->CYCB1L = val;
         return;
      case 0x01E:
         Vdp2Regs->CYCB1U = val;
         return;
      case 0x020:
         Vdp2Regs->BGON = val;
         return;
      case 0x022:
         Vdp2Regs->MZCTL = val;
         return;
      case 0x024:
         Vdp2Regs->SFSEL = val;
         return;
      case 0x026:
         Vdp2Regs->SFCODE = val;
         return;
      case 0x028:
         Vdp2Regs->CHCTLA = val;
         return;
      case 0x02A:
         Vdp2Regs->CHCTLB = val;
         return;
      case 0x02C:
         Vdp2Regs->BMPNA = val;
         return;
      case 0x02E:
         Vdp2Regs->BMPNB = val;
         return;
      case 0x030:
         Vdp2Regs->PNCN0 = val;
         return;
      case 0x032:
         Vdp2Regs->PNCN1 = val;
         return;
      case 0x034:
         Vdp2Regs->PNCN2 = val;
         return;
      case 0x036:
         Vdp2Regs->PNCN3 = val;
         return;
      case 0x038:
         Vdp2Regs->PNCR = val;
         return;
      case 0x03A:
         Vdp2Regs->PLSZ = val;
         return;
      case 0x03C:
         Vdp2Regs->MPOFN = val;
         return;
      case 0x03E:
         Vdp2Regs->MPOFR = val;
         return;
      case 0x040:
         Vdp2Regs->MPABN0 = val;
         return;
      case 0x042:
         Vdp2Regs->MPCDN0 = val;
         return;
      case 0x044:
         Vdp2Regs->MPABN1 = val;
         return;
      case 0x046:
         Vdp2Regs->MPCDN1 = val;
         return;
      case 0x048:
         Vdp2Regs->MPABN2 = val;
         return;
      case 0x04A:
         Vdp2Regs->MPCDN2 = val;
         return;
      case 0x04C:
         Vdp2Regs->MPABN3 = val;
         return;
      case 0x04E:
         Vdp2Regs->MPCDN3 = val;
         return;
      case 0x050:
         Vdp2Regs->MPABRA = val;
         return;
      case 0x052:
         Vdp2Regs->MPCDRA = val;
         return;
      case 0x054:
         Vdp2Regs->MPEFRA = val;
         return;
      case 0x056:
         Vdp2Regs->MPGHRA = val;
         return;
      case 0x058:
         Vdp2Regs->MPIJRA = val;
         return;
      case 0x05A:
         Vdp2Regs->MPKLRA = val;
         return;
      case 0x05C:
         Vdp2Regs->MPMNRA = val;
         return;
      case 0x05E:
         Vdp2Regs->MPOPRA = val;
         return;
      case 0x060:
         Vdp2Regs->MPABRB = val;
         return;
      case 0x062:
         Vdp2Regs->MPCDRB = val;
         return;
      case 0x064:
         Vdp2Regs->MPEFRB = val;
         return;
      case 0x066:
         Vdp2Regs->MPGHRB = val;
         return;
      case 0x068:
         Vdp2Regs->MPIJRB = val;
         return;
      case 0x06A:
         Vdp2Regs->MPKLRB = val;
         return;
      case 0x06C:
         Vdp2Regs->MPMNRB = val;
         return;
      case 0x06E:
         Vdp2Regs->MPOPRB = val;
         return;
      case 0x070:
         Vdp2Regs->SCXIN0 = val;
         return;
      case 0x072:
         Vdp2Regs->SCXDN0 = val;
         return;
      case 0x074:
         Vdp2Regs->SCYIN0 = val;
         return;
      case 0x076:
         Vdp2Regs->SCYDN0 = val;
         return;
      case 0x078:
         Vdp2Regs->ZMXN0.part.I = val;
         return;
      case 0x07A:
         Vdp2Regs->ZMXN0.part.D = val;
         return;
      case 0x07C:
         Vdp2Regs->ZMYN0.part.I = val;
         return;
      case 0x07E:
         Vdp2Regs->ZMYN0.part.D = val;
         return;
      case 0x080:
         Vdp2Regs->SCXIN1 = val;
         return;
      case 0x082:
         Vdp2Regs->SCXDN1 = val;
         return;
      case 0x084:
         Vdp2Regs->SCYIN1 = val;
         return;
      case 0x086:
         Vdp2Regs->SCYDN1 = val;
         return;
      case 0x088:
         Vdp2Regs->ZMXN1.part.I = val;
         return;
      case 0x08A:
         Vdp2Regs->ZMXN1.part.D = val;
         return;
      case 0x08C:
         Vdp2Regs->ZMYN1.part.I = val;
         return;
      case 0x08E:
         Vdp2Regs->ZMYN1.part.D = val;
         return;
      case 0x090:
         Vdp2Regs->SCXN2 = val;
         return;
      case 0x092:
         Vdp2Regs->SCYN2 = val;
         return;
      case 0x094:
         Vdp2Regs->SCXN3 = val;
         return;
      case 0x096:
         Vdp2Regs->SCYN3 = val;
         return;
      case 0x098:
         Vdp2Regs->ZMCTL = val;
         return;
      case 0x09A:
         Vdp2Regs->SCRCTL = val;
         return;
      case 0x09C:
         Vdp2Regs->VCSTAU = val;
         return;
      case 0x09E:
         Vdp2Regs->VCSTAL = val;
         return;
      case 0x0A0:
         Vdp2Regs->LSTA0.part.U = val;
         return;
      case 0x0A2:
         Vdp2Regs->LSTA0.part.L = val;
         return;
      case 0x0A4:
         Vdp2Regs->LSTA1.part.U = val;
         return;
      case 0x0A6:
         Vdp2Regs->LSTA1.part.L = val;
         return;
      case 0x0A8:
         Vdp2Regs->LCTA.part.U = val;
         return;
      case 0x0AA:
         Vdp2Regs->LCTA.part.L = val;
         return;
      case 0x0AC:
         Vdp2Regs->BKTAU = val;
         return;
      case 0x0AE:
         Vdp2Regs->BKTAL = val;
         return;
      case 0x0B0:
         Vdp2Regs->RPMD = val;
         return;
      case 0x0B2:
         Vdp2Regs->RPRCTL = val;
         return;
      case 0x0B4:
         Vdp2Regs->KTCTL = val;
         return;
      case 0x0B6:
         Vdp2Regs->KTAOF = val;
         return;
      case 0x0B8:
         Vdp2Regs->OVPNRA = val;
         return;
      case 0x0BA:
         Vdp2Regs->OVPNRB = val;
         return;
      case 0x0BC:
         Vdp2Regs->RPTA.part.U = val;
         return;
      case 0x0BE:
         Vdp2Regs->RPTA.part.L = val;
         return;
      case 0x0C0:
         Vdp2Regs->WPSX0 = val;
         return;
      case 0x0C2:
         Vdp2Regs->WPSY0 = val;
         return;
      case 0x0C4:
         Vdp2Regs->WPEX0 = val;
         return;
      case 0x0C6:
         Vdp2Regs->WPEY0 = val;
         return;
      case 0x0C8:
         Vdp2Regs->WPSX1 = val;
         return;
      case 0x0CA:
         Vdp2Regs->WPSY1 = val;
         return;
      case 0x0CC:
         Vdp2Regs->WPEX1 = val;
         return;
      case 0x0CE:
         Vdp2Regs->WPEY1 = val;
         return;
      case 0x0D0:
         Vdp2Regs->WCTLA = val;
         return;
      case 0x0D2:
         Vdp2Regs->WCTLB = val;
         return;
      case 0x0D4:
         Vdp2Regs->WCTLC = val;
         return;
      case 0x0D6:
         Vdp2Regs->WCTLD = val;
         return;
      case 0x0D8:
         Vdp2Regs->LWTA0.part.U = val;
         return;
      case 0x0DA:
         Vdp2Regs->LWTA0.part.L = val;
         return;
      case 0x0DC:
         Vdp2Regs->LWTA1.part.U = val;
         return;
      case 0x0DE:
         Vdp2Regs->LWTA1.part.L = val;
         return;
      case 0x0E0:
         Vdp2Regs->SPCTL = val;
         return;
      case 0x0E2:
         Vdp2Regs->SDCTL = val;
         return;
      case 0x0E4:
         Vdp2Regs->CRAOFA = val;
         return;
      case 0x0E6:
         Vdp2Regs->CRAOFB = val;
         return;     
      case 0x0E8:
         Vdp2Regs->LNCLEN = val;
         return;
      case 0x0EA:
         Vdp2Regs->SFPRMD = val;
         return;
      case 0x0EC:
         Vdp2Regs->CCCTL = val;
         return;     
      case 0x0EE:
         Vdp2Regs->SFCCMD = val;
         return;
      case 0x0F0:
         Vdp2Regs->PRISA = val;
         return;
      case 0x0F2:
         Vdp2Regs->PRISB = val;
         return;
      case 0x0F4:
         Vdp2Regs->PRISC = val;
         return;
      case 0x0F6:
         Vdp2Regs->PRISD = val;
         return;
      case 0x0F8:
         VIDCore->Vdp2SetPriorityNBG0(val & 0x7);
         VIDCore->Vdp2SetPriorityNBG1((val >> 8) & 0x7);
         Vdp2Regs->PRINA = val;
         return;
      case 0x0FA:
         VIDCore->Vdp2SetPriorityNBG2(val & 0x7);
         VIDCore->Vdp2SetPriorityNBG3((val >> 8) & 0x7);
         Vdp2Regs->PRINB = val;
         return;
      case 0x0FC:
         VIDCore->Vdp2SetPriorityRBG0(val & 0x7);
         Vdp2Regs->PRIR = val;
         return;
      case 0x0FE:
         // Reserved
         return;
      case 0x100:
         Vdp2Regs->CCRSA = val;
         return;
      case 0x102:
         Vdp2Regs->CCRSB = val;
         return;
      case 0x104:
         Vdp2Regs->CCRSC = val;
         return;
      case 0x106:
         Vdp2Regs->CCRSD = val;
         return;
      case 0x108:
         Vdp2Regs->CCRNA = val;
         return;
      case 0x10A:
         Vdp2Regs->CCRNB = val;
         return;
      case 0x10C:
         Vdp2Regs->CCRR = val;
         return;
      case 0x10E:
         Vdp2Regs->CCRLB = val;
         return;
      case 0x110:
         Vdp2Regs->CLOFEN = val;
         return;
      case 0x112:
         Vdp2Regs->CLOFSL = val;
         return;
      case 0x114:
         Vdp2Regs->COAR = val;
         return;
      case 0x116:
         Vdp2Regs->COAG = val;
         return;
      case 0x118:
         Vdp2Regs->COAB = val;
         return;
      case 0x11A:
         Vdp2Regs->COBR = val;
         return;
      case 0x11C:
         Vdp2Regs->COBG = val;
         return;
      case 0x11E:
         Vdp2Regs->COBB = val;
         return;
      default:
      {
         LOG("Unhandled VDP2 word write: %08X\n", addr);
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteLong(u32 addr, u32 val) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         Vdp2Regs->TVMD = val >> 16;
         Vdp2Regs->EXTEN = val & 0xFFFF;
         VIDCore->Vdp2SetResolution(Vdp2Regs->TVMD);
         return;
      case 0x004:
         Vdp2Regs->VRSIZE = val & 0xFFFF;
         return;
      case 0x008:
         // read-only
         return;
      case 0x00C:
         Vdp2Regs->RAMCTL = val & 0xFFFF;
         return;
      case 0x010:
         Vdp2Regs->CYCA0L = val >> 16;
         Vdp2Regs->CYCA0U = val & 0xFFFF;
         return;
      case 0x014:
         Vdp2Regs->CYCA1L = val >> 16;
         Vdp2Regs->CYCA1U = val & 0xFFFF;
         return;
      case 0x018:
         Vdp2Regs->CYCB0L = val >> 16;
         Vdp2Regs->CYCB0U = val & 0xFFFF;
         return;
      case 0x01C:
         Vdp2Regs->CYCB1L = val >> 16;
         Vdp2Regs->CYCB1U = val & 0xFFFF;
         return;
      case 0x020:
         Vdp2Regs->BGON = val >> 16;
         Vdp2Regs->MZCTL = val & 0xFFFF;
         return;
      case 0x024:
         Vdp2Regs->SFSEL = val >> 16;
         Vdp2Regs->SFCODE = val & 0xFFFF;
         return;
      case 0x028:
         Vdp2Regs->CHCTLA = val >> 16;
         Vdp2Regs->CHCTLB = val & 0xFFFF;
         return;
      case 0x02C:
         Vdp2Regs->BMPNA = val >> 16;
         Vdp2Regs->BMPNB = val & 0xFFFF;
         return;
      case 0x030:
         Vdp2Regs->PNCN0 = val >> 16;
         Vdp2Regs->PNCN1 = val & 0xFFFF;
         return;
      case 0x034:
         Vdp2Regs->PNCN2 = val >> 16;
         Vdp2Regs->PNCN3 = val & 0xFFFF;
         return;
      case 0x038:
         Vdp2Regs->PNCR = val >> 16;
         Vdp2Regs->PLSZ = val & 0xFFFF;
         return;
      case 0x03C:
         Vdp2Regs->MPOFN = val >> 16;
         Vdp2Regs->MPOFR = val & 0xFFFF;
         return;
      case 0x040:
         Vdp2Regs->MPABN0 = val >> 16;
         Vdp2Regs->MPCDN0 = val & 0xFFFF;
         return;
      case 0x044:
         Vdp2Regs->MPABN1 = val >> 16;
         Vdp2Regs->MPCDN1 = val & 0xFFFF;
         return;
      case 0x048:
         Vdp2Regs->MPABN2 = val >> 16;
         Vdp2Regs->MPCDN2 = val & 0xFFFF;
         return;
      case 0x04C:
         Vdp2Regs->MPABN3 = val >> 16;
         Vdp2Regs->MPCDN3 = val & 0xFFFF;
         return;
      case 0x050:
         Vdp2Regs->MPABRA = val >> 16;
         Vdp2Regs->MPCDRA = val & 0xFFFF;
         return;
      case 0x054:
         Vdp2Regs->MPEFRA = val >> 16;
         Vdp2Regs->MPGHRA = val & 0xFFFF;
         return;
      case 0x058:
         Vdp2Regs->MPIJRA = val >> 16;
         Vdp2Regs->MPKLRA = val & 0xFFFF;
         return;
      case 0x05C:
         Vdp2Regs->MPMNRA = val >> 16;
         Vdp2Regs->MPOPRA = val & 0xFFFF;
         return;
      case 0x060:
         Vdp2Regs->MPABRB = val >> 16;
         Vdp2Regs->MPCDRB = val & 0xFFFF;
         return;
      case 0x064:
         Vdp2Regs->MPEFRB = val >> 16;
         Vdp2Regs->MPGHRB = val & 0xFFFF;
         return;
      case 0x068:
         Vdp2Regs->MPIJRB = val >> 16;
         Vdp2Regs->MPKLRB = val & 0xFFFF;
         return;
      case 0x06C:
         Vdp2Regs->MPMNRB = val >> 16;
         Vdp2Regs->MPOPRB = val & 0xFFFF;
         return;
      case 0x070:
         Vdp2Regs->SCXIN0 = val >> 16;
         // SCXDN0 here
         return;
      case 0x074:
         Vdp2Regs->SCYIN0 = val >> 16;
         // SCYDN0 here
         return;
      case 0x078:
         Vdp2Regs->ZMXN0.all = val;
         return;
      case 0x07C:
         Vdp2Regs->ZMYN0.all = val;
         return;
      case 0x080:
         Vdp2Regs->SCXIN1 = val >> 16;
         // SCXDN1 here
         return;
      case 0x084:
         Vdp2Regs->SCYIN1 = val >> 16;
         // SCYDN1 here
         return;
      case 0x088:
         Vdp2Regs->ZMXN1.all = val;
         return;
      case 0x08C:
         Vdp2Regs->ZMYN1.all = val;
         return;
      case 0x090:
         Vdp2Regs->SCXN2 = val >> 16;
         Vdp2Regs->SCYN2 = val & 0xFFFF;
         return;
      case 0x094:
         Vdp2Regs->SCXN3 = val >> 16;
         Vdp2Regs->SCYN3 = val & 0xFFFF;
         return;
      case 0x0AC:
         Vdp2Regs->BKTAU = val >> 16;
         Vdp2Regs->BKTAL = val & 0xFFFF;
         return;
      case 0x0D0:
         Vdp2Regs->WCTLA = val >> 16;
         Vdp2Regs->WCTLB = val & 0xFFFF;
         return;
      case 0x0E0:
         Vdp2Regs->SPCTL = val >> 16;
         Vdp2Regs->SDCTL = val & 0xFFFF;
         return;
      case 0x0E4:
         Vdp2Regs->CRAOFA = val >> 16;
         Vdp2Regs->CRAOFB = val & 0xFFFF;
         return;
      case 0x0E8:
         Vdp2Regs->LNCLEN = val >> 16;
         Vdp2Regs->SFPRMD = val & 0xFFFF;
         return;
      case 0x0EC:
         Vdp2Regs->CCCTL = val >> 16;
         Vdp2Regs->SFCCMD = val & 0xFFFF;
         return;
      case 0x0F0:
         Vdp2Regs->PRISA = val >> 16;
         Vdp2Regs->PRISB = val & 0xFFFF;
         return;
      case 0x0F4:
         Vdp2Regs->PRISC = val >> 16;
         Vdp2Regs->PRISD = val & 0xFFFF;
         return;
      case 0x0F8:
         Vdp2Regs->PRINA = val >> 16;
         Vdp2Regs->PRINB = val & 0xFFFF;
         VIDCore->Vdp2SetPriorityNBG0(Vdp2Regs->PRINA & 0x7);
         VIDCore->Vdp2SetPriorityNBG1((Vdp2Regs->PRINA >> 8) & 0x7);
         VIDCore->Vdp2SetPriorityNBG2(Vdp2Regs->PRINB & 0x7);
         VIDCore->Vdp2SetPriorityNBG3((Vdp2Regs->PRINB >> 8) & 0x7);
         return;
      case 0x100:
         Vdp2Regs->CCRSA = val >> 16;
         Vdp2Regs->CCRSB = val & 0xFFFF;
         return;
      case 0x104:
         Vdp2Regs->CCRSC = val >> 16;
         Vdp2Regs->CCRSD = val & 0xFFFF;
         return;
      case 0x108:
         Vdp2Regs->CCRNA = val >> 16;
         Vdp2Regs->CCRNB = val & 0xFFFF;
         return;
      case 0x10C:
         Vdp2Regs->CCRR = val >> 16;
         Vdp2Regs->CCRLB = val & 0xFFFF;
         return;
      case 0x110:
         Vdp2Regs->CLOFEN = val >> 16;
         Vdp2Regs->CLOFSL = val & 0xFFFF;
         return;
      case 0x114:
         Vdp2Regs->COAR = val >> 16;
         Vdp2Regs->COAG = val & 0xFFFF;
         return;
      case 0x118:
         Vdp2Regs->COAB = val >> 16;
         Vdp2Regs->COBR = val & 0xFFFF;
         return;
      case 0x11C:
         Vdp2Regs->COBG = val >> 16;
         Vdp2Regs->COBB = val & 0xFFFF;
         return;
      case 0x120:
      case 0x124:
      case 0x128:
      case 0x12C:
      case 0x130:
      case 0x134:
      case 0x138:
      case 0x13C:
      case 0x140:
      case 0x144:
      case 0x148:
      case 0x14C:
      case 0x150:
      case 0x154:
      case 0x158:
      case 0x15C:
      case 0x160:
      case 0x164:
      case 0x168:
      case 0x16C:
      case 0x170:
      case 0x174:
      case 0x178:
      case 0x17C:
      case 0x180:
      case 0x184:
      case 0x188:
      case 0x18C:
      case 0x190:
      case 0x194:
      case 0x198:
      case 0x19C:
      case 0x1A0:
      case 0x1A4:
      case 0x1A8:
      case 0x1AC:
      case 0x1B0:
      case 0x1B4:
      case 0x1B8:
      case 0x1BC:
      case 0x1C0:
      case 0x1C4:
      case 0x1C8:
      case 0x1CC:
      case 0x1D0:
      case 0x1D4:
      case 0x1D8:
      case 0x1DC:
      case 0x1E0:
      case 0x1E4:
      case 0x1E8:
      case 0x1EC:
      case 0x1F0:
      case 0x1F4:
      case 0x1F8:
      case 0x1FC:
         // Reserved
         return;
      default:
      {
         LOG("Unhandled VDP2 long write: %08X\n", addr);
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2SaveState(FILE *fp)
{
   int offset;

   offset = StateWriteHeader(fp, "VDP2", 1);
/*
   // Write registers
   fwrite((void *)this->getBuffer(), 0x200, 1, fp);

   // Write VDP2 ram
   fwrite((void *)vram->getBuffer(), 0x80000, 1, fp);

   // Write CRAM
   fwrite((void *)cram->getBuffer(), 0x1000, 1, fp);
*/
   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2LoadState(FILE *fp, int version, int size)
{
/*
   unsigned short reg;

   // Read registers
   fread((void *)this->getBuffer(), 0x200, 1, fp);

   // Read VDP2 ram
   fread((void *)vram->getBuffer(), 0x80000, 1, fp);

   // Read CRAM
   fread((void *)cram->getBuffer(), 0x1000, 1, fp);

   // Update internal variables
   updateRam();

   reg = Memory::getWord(0xF8);
   nbg0->setPriority(reg & 0x7);
   nbg1->setPriority((reg >> 8) & 0x7);

   reg = Memory::getWord(0xFA);
   nbg2->setPriority(reg & 0x7);
   nbg3->setPriority((reg >> 8) & 0x7);

   reg = Memory::getWord(0xFC);
   rbg0->setPriority(reg & 0x7);
*/
   return size;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsRBG0(char *outstring, int *isenabled)
{
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG0(char *outstring, int *isenabled)
{
/*
  unsigned short screenDisplayReg = readWord(reg, 0x20);
  unsigned short mosaicReg = readWord(reg, 0x22);
  unsigned short lineVerticalScrollReg = readWord(reg, 0x9A) & 0x3F;
  unsigned long tmp=0;

  init();

  if (screenDisplayReg & 0x1 || screenDisplayReg & 0x20) {
     // enabled
     *isenabled = true;

     // Generate specific Info for NBG0/RBG1
     if (screenDisplayReg & 0x20)
     {
         sprintf(outstring, "RBG1 mode\r\n");
         outstring += strlen(outstring);
     }
     else
     {
         sprintf(outstring, "NBG0 mode\r\n");
         outstring += strlen(outstring);
     }

     // Mosaic
     if (mosaicReg & 0x1)
     {
        sprintf(outstring, "Mosaic Size = width %d height %d\r\n", (mosaicReg >> 8) & 0xf + 1, (mosaicReg >> 12) + 1);
        outstring += strlen(outstring);        
     }

     switch (colorNumber) {
        case 0:
                sprintf(outstring, "4-bit(16 colors)\r\n");
                break;
        case 1:
                sprintf(outstring, "8-bit(256 colors)\r\n");
                break;
        case 2:
                sprintf(outstring, "16-bit(2048 colors)\r\n");
                break;
        case 3:
                sprintf(outstring, "16-bit(32,768 colors)\r\n");
                break;
        case 4:
                sprintf(outstring, "32-bit(16.7 mil colors)\r\n");
                break;
        default:
                sprintf(outstring, "Unsupported BPP\r\n");
                break;
     }
     outstring += strlen(outstring);

     // Bitmap or Tile mode?(RBG1 can only do Tile mode)
     
     if (bitmap && !(screenDisplayReg & 0x20))
     {
        unsigned short bmpPalNumberReg = readWord(reg, 0x2C);

        // Bitmap
        sprintf(outstring, "Bitmap(%dx%d)\r\n", cellW, cellH);
        outstring += strlen(outstring);

        if (bmpPalNumberReg & 0x20)
        {
           sprintf(outstring, "Bitmap Special Priority enabled\r\n");
           outstring += strlen(outstring);
        }

        if (bmpPalNumberReg & 0x10)
        {
           sprintf(outstring, "Bitmap Special Color Calculation enabled\r\n");
           outstring += strlen(outstring);
        }

        sprintf(outstring, "Bitmap Address = %X\r\n", charAddr);
        outstring += strlen(outstring);

        sprintf(outstring, "Bitmap Palette Address = %X\r\n", palAddr);
        outstring += strlen(outstring);
     }
     else
     {
        // Tile
        sprintf(outstring, "Tile(%dH x %dV)\r\n", patternWH, patternWH);
        outstring += strlen(outstring);

        // Pattern Name Control stuff
        if (patternDataSize == 2) 
        {
           sprintf(outstring, "Pattern Name data size = 2 words\r\n");
           outstring += strlen(outstring);
        }
        else
        {
           sprintf(outstring, "Pattern Name data size = 1 word\r\n");
           outstring += strlen(outstring);
           sprintf(outstring, "Character Number Supplement bit = %d\r\n", (supplementData >> 16));
           outstring += strlen(outstring);
           sprintf(outstring, "Special Priority bit = %d\r\n", (supplementData >> 9) & 0x1);
           outstring += strlen(outstring);
           sprintf(outstring, "Special Color Calculation bit = %d\r\n", (supplementData >> 8) & 0x1);
           outstring += strlen(outstring);
           sprintf(outstring, "Supplementary Palette number = %d\r\n", (supplementData >> 5) & 0x7);
           outstring += strlen(outstring);
           sprintf(outstring, "Supplementary Color number = %d\r\n", supplementData & 0x1f);
           outstring += strlen(outstring);
        }

        // Figure out Cell start address
        switch(patternDataSize) {
           case 1: {
                   tmp = readWord(vram, addr);

                   switch(auxMode) {
                      case 0:
                         switch(patternWH) {
                            case 1:
                                charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
                            case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
                         }
                         break;
                      case 1:
                         switch(patternWH) {
                         case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
                         case 4:
                                charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
                                break;
                         }
                         break;
                   }
                   break;
           }
           case 2: {
                   unsigned short tmp1 = readWord(vram, addr);
                   unsigned short tmp2 = readWord(vram, addr+2);

                   charAddr = tmp2 & 0x7FFF;
                   break;
           }
        }
        if (!(readWord(reg, 0x6) & 0x8000)) charAddr &= 0x3FFF;

        charAddr *= 0x20; // selon Runik
  
        sprintf(outstring, "Cell Data Address = %X\r\n", charAddr);
        outstring += strlen(outstring);
     }
     
     sprintf(outstring, "Plane Size = %dH x %dV\r\n", planeW, planeH);
     outstring += strlen(outstring);

     if (screenDisplayReg & 0x20)
     {
//        unsigned long mapOffsetReg=(readWord(reg, 0x3E) & 0x70) << 2;
        unsigned short rotParaControlReg=readWord(reg, 0xB2);

        // RBG1

        // Map Planes A-P here

        // Rotation Parameter Read Control
        if (rotParaControlReg & 0x400)
           sprintf(outstring, "Read KAst Parameter = TRUE\r\n");
        else
           sprintf(outstring, "Read KAst Parameter = FALSE\r\n");
        outstring += strlen(outstring);

        if (rotParaControlReg & 0x200)
           sprintf(outstring, "Read Yst Parameter = TRUE\r\n");
        else
           sprintf(outstring, "Read Yst Parameter = FALSE\r\n");
        outstring += strlen(outstring);

        if (rotParaControlReg & 0x100)
           sprintf(outstring, "Read Xst Parameter = TRUE\r\n");
        else
           sprintf(outstring, "Read Xst Parameter = FALSE\r\n");
        outstring += strlen(outstring);

        // Coefficient Table Control

        // Coefficient Table Address Offset

        // Screen Over Pattern Name(should this be moved?)

        // Rotation Parameter Table Address
     }
     else
     {
        // NBG0
        unsigned long mapOffsetReg=(readWord(reg, 0x3C) & 0x7) << 6;
        int deca = planeH + planeW - 2;
        int multi = planeH * planeW;

        if (!bitmap) {
           // Map Planes A-D
           for (int i=0; i < 4; i++) {
              tmp = mapOffsetReg | reg->getByte(0x40 + (i ^ 1));

              if (patternDataSize == 1) {
                 if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
                 else addr = (tmp >> deca) * (multi * 0x800);
              }
              else {
                 if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
                 else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
              }
  
              sprintf(outstring, "Plane %C Address = %08X\r\n", 0x41+i, addr);
              outstring += strlen(outstring);
           }
        }

        // Screen scroll values
        sprintf(outstring, "Screen Scroll x = %f, y = %f\r\n", (float)(reg->getLong(0x70) & 0x7FFFF00) / 65536, (float)(reg->getLong(0x74) & 0x7FFFF00) / 65536);
        outstring += strlen(outstring);

        // Coordinate Increments
        sprintf(outstring, "Coordinate Increments x = %f, y = %f\r\n", coordIncX, coordIncY);
        outstring += strlen(outstring);

        // Reduction Enable
        switch (readWord(reg, 0x98) & 3)
        {
           case 1:
                   sprintf(outstring, "Horizontal Reduction = 1/2\r\n");
                   outstring += strlen(outstring);
                   break;
           case 2:
           case 3:
                   sprintf(outstring, "Horizontal Reduction = 1/4\r\n");
                   outstring += strlen(outstring);
                   break;
           default: break;
        }

        switch (lineVerticalScrollReg >> 4)
        {
           case 0:
                   sprintf(outstring, "Line Scroll Interval = Each Line\r\n");
                   break;
           case 1:
                   sprintf(outstring, "Line Scroll Interval = Every 2 Lines\r\n");
                   break;
           case 2:
                   sprintf(outstring, "Line Scroll Interval = Every 4 Lines\r\n");
                   break;
           case 3:
                   sprintf(outstring, "Line Scroll Interval = Every 8 Lines\r\n");
                   break;
        }

        outstring += strlen(outstring);
   
        if (lineVerticalScrollReg & 0x8)
        {
           sprintf(outstring, "Line Zoom enabled\r\n");
           outstring += strlen(outstring);
        }

        if (lineVerticalScrollReg & 0x4)
        {
           sprintf(outstring, "Line Scroll Vertical enabled\r\n");
           outstring += strlen(outstring);
        }
   
        if (lineVerticalScrollReg & 0x2)
        {
           sprintf(outstring, "Line Scroll Horizontal enabled\r\n");
           outstring += strlen(outstring);
        }

        if (lineVerticalScrollReg & 0x6) 
        {
           sprintf(outstring, "Line Scroll Enabled\r\n");
           outstring += strlen(outstring);
           sprintf(outstring, "Line Scroll Table Address = %08X\r\n", 0x05E00000 + ((((readWord(reg, 0xA0) & 0x7) << 16) | (readWord(reg, 0xA2) & 0xFFFE)) << 1));
           outstring += strlen(outstring);
        }

        if (lineVerticalScrollReg & 0x1)
        {
           sprintf(outstring, "Vertical Cell Scroll enabled\r\n");
           outstring += strlen(outstring);
           sprintf(outstring, "Vertical Cell Scroll Table Address = %08X\r\n", 0x05E00000 + ((((readWord(reg, 0x9C) & 0x7) << 16) | (readWord(reg, 0x9E) & 0xFFFE)) << 1));
           outstring += strlen(outstring);
        }
     }

     // Window Control here

     // Shadow Control here

     // Color Ram Address Offset here

     // Special Priority Mode here

     // Color Calculation Control here

     // Special Color Calculation Mode here

     // Priority Number
     sprintf(outstring, "Priority = %d\r\n", priority);
     outstring += strlen(outstring);

     // Color Calculation here

     // Color Offset Enable here

     // Color Offset Select here
  }
  else {
     // disabled
     *isenabled = false;
  }
*/
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG1(char *outstring, int *isenabled)
{
/*
  unsigned short mosaicReg = readWord(reg, 0x22);
  unsigned short lineVerticalScrollReg = (readWord(reg, 0x9A) >> 8) & 0x3F;
  unsigned long mapOffsetReg=(readWord(reg, 0x3C) & 0x70) << 2;
  unsigned long tmp=0;
  int deca;
  int multi;

  init();

  if (enable) {
     // enabled
     *isenabled = true;

     // Mosaic
     if (mosaicReg & 0x2)
     {
        sprintf(outstring, "Mosaic Size = width %d height %d\r\n", (mosaicReg >> 8) & 0xf + 1, (mosaicReg >> 12) + 1);
        outstring += strlen(outstring);        
     }

     switch (colorNumber) {
        case 0:
                sprintf(outstring, "4-bit(16 colors)\r\n");
                break;
        case 1:
                sprintf(outstring, "8-bit(256 colors)\r\n");
                break;
        case 2:
                sprintf(outstring, "16-bit(2048 colors)\r\n");
                break;
        case 3:
                sprintf(outstring, "16-bit(32,768 colors)\r\n");
                break;
        case 4:
                sprintf(outstring, "32-bit(16.7 mil colors)\r\n");
                break;
        default:
                sprintf(outstring, "Unsupported BPP\r\n");
                break;
     }
     outstring += strlen(outstring);

     // Bitmap or Tile mode?     
     if (bitmap)
     {
        unsigned short bmpPalNumberReg = readWord(reg, 0x2C) >> 8;

        // Bitmap
        sprintf(outstring, "Bitmap(%dx%d)\r\n", cellW, cellH);
        outstring += strlen(outstring);

        if (bmpPalNumberReg & 0x20)
        {
           sprintf(outstring, "Bitmap Special Priority enabled\r\n");
           outstring += strlen(outstring);
        }

        if (bmpPalNumberReg & 0x10)
        {
           sprintf(outstring, "Bitmap Special Color Calculation enabled\r\n");
           outstring += strlen(outstring);
        }

        sprintf(outstring, "Bitmap Address = %X\r\n", charAddr);
        outstring += strlen(outstring);

        sprintf(outstring, "Bitmap Palette Address = %X\r\n", palAddr);
        outstring += strlen(outstring);
     }
     else
     {
        // Tile
        sprintf(outstring, "Tile(%dH x %dV)\r\n", patternWH, patternWH);
        outstring += strlen(outstring);

        // Pattern Name Control stuff
        if (patternDataSize == 2) 
        {
           sprintf(outstring, "Pattern Name data size = 2 words\r\n");
           outstring += strlen(outstring);
        }
        else
        {
           sprintf(outstring, "Pattern Name data size = 1 word\r\n");
           outstring += strlen(outstring);
           sprintf(outstring, "Character Number Supplement bit = %d\r\n", (supplementData >> 16));
           outstring += strlen(outstring);
           sprintf(outstring, "Special Priority bit = %d\r\n", (supplementData >> 9) & 0x1);
           outstring += strlen(outstring);
           sprintf(outstring, "Special Color Calculation bit = %d\r\n", (supplementData >> 8) & 0x1);
           outstring += strlen(outstring);
           sprintf(outstring, "Supplementary Palette number = %d\r\n", (supplementData >> 5) & 0x7);
           outstring += strlen(outstring);
           sprintf(outstring, "Supplementary Color number = %d\r\n", supplementData & 0x1f);
           outstring += strlen(outstring);
        }

        deca = planeH + planeW - 2;
        multi = planeH * planeW;

        // Map Planes A-D
        for (int i=0; i < 4; i++) {
           tmp = mapOffsetReg | reg->getByte(0x44 + (i ^ 1));

           if (patternDataSize == 1) {
              if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
              else addr = (tmp >> deca) * (multi * 0x800);
           }
           else {
              if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
              else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
           }

           sprintf(outstring, "Plane %C Address = %08X\r\n", 0x41+i, addr);
           outstring += strlen(outstring);
        }

        // Figure out Cell start address
        switch(patternDataSize) {
           case 1: {
                   tmp = readWord(vram, addr);

                   switch(auxMode) {
                      case 0:
                         switch(patternWH) {
                            case 1:
                                charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
                            case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
                         }
                         break;
                      case 1:
                         switch(patternWH) {
                         case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
                         case 4:
                                charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
                                break;
                         }
                         break;
                   }
                   break;
           }
           case 2: {
                   unsigned short tmp1 = readWord(vram, addr);
                   unsigned short tmp2 = readWord(vram, addr+2);

                   charAddr = tmp2 & 0x7FFF;
                   break;
           }
        }
        if (!(readWord(reg, 0x6) & 0x8000)) charAddr &= 0x3FFF;

        charAddr *= 0x20; // selon Runik
  
        sprintf(outstring, "Cell Data Address = %X\r\n", charAddr);
        outstring += strlen(outstring);
     }
     
     sprintf(outstring, "Plane Size = %dH x %dV\r\n", planeW, planeH);
     outstring += strlen(outstring);

     // Screen scroll values
     sprintf(outstring, "Screen Scroll x = %f, y = %f\r\n", (float)(reg->getLong(0x80) & 0x7FFFF00) / 65536, (float)(reg->getLong(0x84) & 0x7FFFF00) / 65536);
     outstring += strlen(outstring);

     // Coordinate Increments
     sprintf(outstring, "Coordinate Increments x = %f, y = %f\r\n", coordIncX, coordIncY);
     outstring += strlen(outstring);

     // Reduction Enable
     switch ((readWord(reg, 0x98) >> 8) & 3)
     {
        case 1:
                sprintf(outstring, "Horizontal Reduction = 1/2\r\n");
                outstring += strlen(outstring);
                break;
        case 2:
        case 3:
                sprintf(outstring, "Horizontal Reduction = 1/4\r\n");
                outstring += strlen(outstring);
                break;
        default: break;
     }

     switch (lineVerticalScrollReg >> 4)
     {
        case 0:
                sprintf(outstring, "Line Scroll Interval = Each Line\r\n");
                break;
        case 1:
                sprintf(outstring, "Line Scroll Interval = Every 2 Lines\r\n");
                break;
        case 2:
                sprintf(outstring, "Line Scroll Interval = Every 4 Lines\r\n");
                break;
        case 3:
                sprintf(outstring, "Line Scroll Interval = Every 8 Lines\r\n");
                break;
     }

     outstring += strlen(outstring);

     if (lineVerticalScrollReg & 0x8)
     {
        sprintf(outstring, "Line Zoom X enabled\r\n");
        outstring += strlen(outstring);
     }

     if (lineVerticalScrollReg & 0x4)
     {
        sprintf(outstring, "Line Scroll Vertical enabled\r\n");
        outstring += strlen(outstring);
     }
   
     if (lineVerticalScrollReg & 0x2)
     {
        sprintf(outstring, "Line Scroll Horizontal enabled\r\n");
        outstring += strlen(outstring);
     }

     if (lineVerticalScrollReg & 0x6) 
     {
        sprintf(outstring, "Line Scroll Enabled\r\n");
        outstring += strlen(outstring);
        sprintf(outstring, "Line Scroll Table Address = %08X\r\n", 0x05E00000 + ((((readWord(reg, 0xA4) & 0x7) << 16) | (readWord(reg, 0xA6) & 0xFFFE)) << 1));
        outstring += strlen(outstring);
     }

     if (lineVerticalScrollReg & 0x1)
     {
        sprintf(outstring, "Vertical Cell Scroll enabled\r\n");
        outstring += strlen(outstring);
        sprintf(outstring, "Vertical Cell Scroll Table Address = %08X\r\n", 0x05E00000 + ((((readWord(reg, 0x9C) & 0x7) << 16) | (readWord(reg, 0x9E) & 0xFFFE)) << 1));
        outstring += strlen(outstring);
     }

     // Window Control here

     // Shadow Control here

     // Color Ram Address Offset here

     // Special Priority Mode here

     // Color Calculation Control here

     // Special Color Calculation Mode here

     // Priority Number
     sprintf(outstring, "Priority = %d\r\n", priority);
     outstring += strlen(outstring);

     // Color Calculation here

     // Color Offset Enable here

     // Color Offset Select here
  }
  else {
     // disabled
     *isenabled = false;
  }
*/
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG2(char *outstring, int *isenabled)
{
/*
  unsigned short mosaicReg = readWord(reg, 0x22);
  unsigned long mapOffsetReg=(readWord(reg, 0x3C) & 0x700) >> 2;
  unsigned long tmp=0;
  int deca;
  int multi;

  init();

  if (enable) {
     // enabled
     *isenabled = true;

     // Mosaic
     if (mosaicReg & 0x4)
     {
        sprintf(outstring, "Mosaic Size = width %d height %d\r\n", (mosaicReg >> 8) & 0xf + 1, (mosaicReg >> 12) + 1);
        outstring += strlen(outstring);        
     }

     switch (colorNumber) {
        case 0:
                sprintf(outstring, "4-bit(16 colors)\r\n");
                break;
        case 1:
                sprintf(outstring, "8-bit(256 colors)\r\n");
                break;
        case 2:
                sprintf(outstring, "16-bit(2048 colors)\r\n");
                break;
        case 3:
                sprintf(outstring, "16-bit(32,768 colors)\r\n");
                break;
        case 4:
                sprintf(outstring, "32-bit(16.7 mil colors)\r\n");
                break;
        default:
                sprintf(outstring, "Unsupported BPP\r\n");
                break;
     }
     outstring += strlen(outstring);

     sprintf(outstring, "Tile(%dH x %dV)\r\n", patternWH, patternWH);
     outstring += strlen(outstring);

     // Pattern Name Control stuff
     if (patternDataSize == 2) 
     {
        sprintf(outstring, "Pattern Name data size = 2 words\r\n");
        outstring += strlen(outstring);
     }
     else
     {
        sprintf(outstring, "Pattern Name data size = 1 word\r\n");
        outstring += strlen(outstring);
        sprintf(outstring, "Character Number Supplement bit = %d\r\n", (supplementData >> 16));
        outstring += strlen(outstring);
        sprintf(outstring, "Special Priority bit = %d\r\n", (supplementData >> 9) & 0x1);
        outstring += strlen(outstring);
        sprintf(outstring, "Special Color Calculation bit = %d\r\n", (supplementData >> 8) & 0x1);
        outstring += strlen(outstring);
        sprintf(outstring, "Supplementary Palette number = %d\r\n", (supplementData >> 5) & 0x7);
        outstring += strlen(outstring);
        sprintf(outstring, "Supplementary Color number = %d\r\n", supplementData & 0x1f);
        outstring += strlen(outstring);
     }
     
     sprintf(outstring, "Plane Size = %dH x %dV\r\n", planeW, planeH);
     outstring += strlen(outstring);

     deca = planeH + planeW - 2;
     multi = planeH * planeW;

     // Map Planes A-D
     for (int i=0; i < 4; i++) {
        tmp = mapOffsetReg | reg->getByte(0x48 + (i ^ 1));

        if (patternDataSize == 1) {
           if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
           else addr = (tmp >> deca) * (multi * 0x800);
        }
        else {
           if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
           else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
        }

        sprintf(outstring, "Plane %C Address = %08X\r\n", 0x41+i, addr);
        outstring += strlen(outstring);
     }

     // Figure out Cell start address
     switch(patternDataSize) {
        case 1: {
                tmp = readWord(vram, addr);

    		switch(auxMode) {
    		case 0:
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
      			case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
      			}
      			break;
    		case 1:
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
      			case 4:
				charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
				break;
      			}
      			break;
    		}
    		break;
	}
  	case 2: {
                unsigned short tmp1 = readWord(vram, addr);
                unsigned short tmp2 = readWord(vram, addr+2);

    		charAddr = tmp2 & 0x7FFF;
    		break;
	}
     }
     if (!(readWord(reg, 0x6) & 0x8000)) charAddr &= 0x3FFF;

     charAddr *= 0x20; // selon Runik

     sprintf(outstring, "Cell Data Address = %X\r\n", charAddr);
     outstring += strlen(outstring);

     // Screen scroll values
     sprintf(outstring, "Screen Scroll x = %d, y = %d\r\n", readWord(reg, 0x90) & 0x7FF, readWord(reg, 0x92) & 0x7FF);
     outstring += strlen(outstring);
 
     // Window Control here

     // Shadow Control here

     // Color Ram Address Offset here

     // Special Priority Mode here

     // Color Calculation Control here

     // Special Color Calculation Mode here

     // Priority Number
     sprintf(outstring, "Priority = %d\r\n", priority);
     outstring += strlen(outstring);

     // Color Calculation here

     // Color Offset Enable here

     // Color Offset Select here
  }
  else {
     // disabled
     *isenabled = false;
  }
*/
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG3(char *outstring, int *isenabled)
{
/*
  unsigned short mosaicReg = readWord(reg, 0x22);
  unsigned long mapOffsetReg=(readWord(reg, 0x3C) & 0x7000) >> 6;
  unsigned long tmp=0;
  int deca;
  int multi;

  init();

  if (enable) {
     // enabled
     *isenabled = true;

     // Mosaic
     if (mosaicReg & 0x8)
     {
        sprintf(outstring, "Mosaic Size = width %d height %d\r\n", (mosaicReg >> 8) & 0xf + 1, (mosaicReg >> 12) + 1);
        outstring += strlen(outstring);        
     }

     switch (colorNumber) {
        case 0:
                sprintf(outstring, "4-bit(16 colors)\r\n");
                break;
        case 1:
                sprintf(outstring, "8-bit(256 colors)\r\n");
                break;
        case 2:
                sprintf(outstring, "16-bit(2048 colors)\r\n");
                break;
        case 3:
                sprintf(outstring, "16-bit(32,768 colors)\r\n");
                break;
        case 4:
                sprintf(outstring, "32-bit(16.7 mil colors)\r\n");
                break;
        default:
                sprintf(outstring, "Unsupported BPP\r\n");
                break;
     }
     outstring += strlen(outstring);

     sprintf(outstring, "Tile(%dH x %dV)\r\n", patternWH, patternWH);
     outstring += strlen(outstring);

     // Pattern Name Control stuff
     if (patternDataSize == 2) 
     {
        sprintf(outstring, "Pattern Name data size = 2 words\r\n");
        outstring += strlen(outstring);
     }
     else
     {
        sprintf(outstring, "Pattern Name data size = 1 word\r\n");
        outstring += strlen(outstring);
        sprintf(outstring, "Character Number Supplement bit = %d\r\n", (supplementData >> 16));
        outstring += strlen(outstring);
        sprintf(outstring, "Special Priority bit = %d\r\n", (supplementData >> 9) & 0x1);
        outstring += strlen(outstring);
        sprintf(outstring, "Special Color Calculation bit = %d\r\n", (supplementData >> 8) & 0x1);
        outstring += strlen(outstring);
        sprintf(outstring, "Supplementary Palette number = %d\r\n", (supplementData >> 5) & 0x7);
        outstring += strlen(outstring);
        sprintf(outstring, "Supplementary Color number = %d\r\n", supplementData & 0x1f);
        outstring += strlen(outstring);
     }
     
     sprintf(outstring, "Plane Size = %dH x %dV\r\n", planeW, planeH);
     outstring += strlen(outstring);

     deca = planeH + planeW - 2;
     multi = planeH * planeW;

     // Map Planes A-D
     for (int i=0; i < 4; i++) {
        tmp = mapOffsetReg | reg->getByte(0x4C + (i ^ 1));

        if (patternDataSize == 1) {
           if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
           else addr = (tmp >> deca) * (multi * 0x800);
        }
        else {
           if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
           else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
        }

        sprintf(outstring, "Plane %C Address = %08X\r\n", 0x41+i, addr);
        outstring += strlen(outstring);
     }

     // Figure out Cell start address
     switch(patternDataSize) {
        case 1: {
                tmp = readWord(vram, addr);

    		switch(auxMode) {
    		case 0:
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
      			case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
      			}
      			break;
    		case 1:
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
      			case 4:
				charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
				break;
      			}
      			break;
    		}
    		break;
	}
  	case 2: {
                unsigned short tmp1 = readWord(vram, addr);
                unsigned short tmp2 = readWord(vram, addr+2);

    		charAddr = tmp2 & 0x7FFF;
    		break;
	}
     }
     if (!(readWord(reg, 0x6) & 0x8000)) charAddr &= 0x3FFF;

     charAddr *= 0x20; // selon Runik

     sprintf(outstring, "Cell Data Address = %X\r\n", charAddr);
     outstring += strlen(outstring);

     // Screen scroll values
     sprintf(outstring, "Screen Scroll x = %d, y = %d\r\n", readWord(reg, 0x94) & 0x7FF, readWord(reg, 0x96) & 0x7FF);
     outstring += strlen(outstring);
 
     // Window Control here

     // Shadow Control here

     // Color Ram Address Offset here

     // Special Priority Mode here

     // Color Calculation Control here

     // Special Color Calculation Mode here

     // Priority Number
     sprintf(outstring, "Priority = %d\r\n", priority);
     outstring += strlen(outstring);

     // Color Calculation here

     // Color Offset Enable here

     // Color Offset Select here
  }
  else {
     // disabled
     *isenabled = false;
  }
*/
}

//////////////////////////////////////////////////////////////////////////////
