/*  Copyright 2003 Guillaume Duhamel
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

#include "cs2.hh"
#include "cdbase.hh"
#include "yui.hh"

#define CDB_HIRQ_CMOK      0x0001
#define CDB_HIRQ_DRDY      0x0002
#define CDB_HIRQ_CSCT      0x0004
#define CDB_HIRQ_BFUL      0x0008
#define CDB_HIRQ_PEND      0x0010
#define CDB_HIRQ_DCHG      0x0020
#define CDB_HIRQ_ESEL      0x0040
#define CDB_HIRQ_EHST      0x0080
#define CDB_HIRQ_ECPY      0x0100
#define CDB_HIRQ_EFLS      0x0200
#define CDB_HIRQ_SCDQ      0x0400
#define CDB_HIRQ_MPED      0x0800
#define CDB_HIRQ_MPCM      0x1000
#define CDB_HIRQ_MPST      0x2000

#define CDB_STAT_BUSY      0x00
#define CDB_STAT_PAUSE     0x01
#define CDB_STAT_STANDBY   0x02
#define CDB_STAT_PLAY      0x03
#define CDB_STAT_SEEK      0x04
#define CDB_STAT_SCAN      0x05
#define CDB_STAT_OPEN      0x06
#define CDB_STAT_NODISC    0x07
#define CDB_STAT_RETRY     0x08
#define CDB_STAT_ERROR     0x09
#define CDB_STAT_FATAL     0x0A
#define CDB_STAT_PERI      0x20
#define CDB_STAT_TRNS      0x40
#define CDB_STAT_WAIT      0x80
#define CDB_STAT_REJECT    0xFF

#define doCDReport() \
  reg.CR1 = (status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF); \
  reg.CR2 = (ctrladdr << 8) | track; \
  reg.CR3 = (index << 8) | ((FAD >> 16) & 0xFF); \
  reg.CR4 = (unsigned short) FAD; 

#define doMPEGReport() \
  reg.CR1 = (status << 8) | actionstatus; \
  reg.CR2 = vcounter; \
  reg.CR3 = (pictureinfo << 8) | mpegaudiostatus; \
  reg.CR4 = mpegvideostatus; 

unsigned char Cs2::getByte(unsigned long addr) {

  if(carttype == CART_NETLINK) {
     switch (addr) {
        case 0x95019: // Modem Status Register
           // Send interrupt here?
           return Memory::getByte(addr);
        case 0x9501D: // Scratch
           return Memory::getByte(addr);
        default:
           return Memory::getByte(addr);
     }
  }
  else
  {
     return 0xFF; // only netlink seems to use byte-access
//     return Memory::getByte(addr);
  }
}

void Cs2::setByte(unsigned long addr, unsigned char val) {
  if(carttype == CART_NETLINK) {
     switch (addr) {
        case 0x2503D: // ???
           Memory::setByte(addr, val);
           break;
        case 0x95019: // Modem Status Register(read-only)
           break;
        case 0x9501D: // Scratch
           Memory::setByte(addr, val);
           break;
        default:
           break;
      }
   }
   else
   {
       // only netlink seems to use byte-access
//      Memory::setByte(addr, val);
   }
}

unsigned short Cs2::getWord(unsigned long addr) {
  unsigned short val=0;
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x90008:
    case 0x9000A:
                  val = reg.HIRQ;

                  if (isbufferfull)
                    val |= CDB_HIRQ_BFUL;
                  else
                    val &= ~CDB_HIRQ_BFUL;

                  if (isdiskchanged)
                    val |= CDB_HIRQ_DCHG;
                  else
                    val &= ~CDB_HIRQ_DCHG;

                  if (isonesectorstored)
                    val |= CDB_HIRQ_CSCT;
                  else
                    val &= ~CDB_HIRQ_CSCT;

                  reg.HIRQ = val;

                  val &= reg.HIRQMASK;
//#if CDDEBUG
//                  fprintf(stderr, "cs2\t: Hirq read, Hirq mask = %x - ret: %x\n", Memory::getWord(0x9000C), val);
//#endif
                  return val;
    case 0x9000C: 
    case 0x9000E: return reg.HIRQMASK;
    case 0x90018:
    case 0x9001A: return reg.CR1;
    case 0x9001C:
    case 0x9001E: return reg.CR2;
    case 0x90020:
    case 0x90022: return reg.CR3;
    case 0x90024:
    case 0x90026: _command = false;
                  return reg.CR4;
    case 0x90028:
    case 0x9002A: return reg.MPEGRGB;
    case 0x98000:
                  // transfer info
                  switch (infotranstype) {
                     case 0:
                             // Get Toc Data
                             if (transfercount % 4 == 0)
                                val = (TOC[transfercount >> 2] & 0xFFFF0000) >> 16;
                             else
                                val = TOC[transfercount >> 2] & 0x0000FFFF;

                             transfercount += 2;
                             cdwnum += 2;

                             if (transfercount > (0xCC * 2))
                             {
                                transfercount = 0;
                                infotranstype = -1;
                             }
                             break;
                     case 1:
                             // Get File Info(1 file info)
                             val = (transfileinfo[transfercount] << 8) |
                                    transfileinfo[transfercount + 1];
                             transfercount += 2;
                             cdwnum += 2;

                             if (transfercount > (0x6 * 2))
                             {
                                transfercount = 0;
                                infotranstype = -1;
                             }

                             break;
                     case 2:
                             // Get File Info(254 file info)

                             // Do we need to retrieve the next file info?
                             if (transfercount % (0x6 * 2) == 0) {
                               // yes we do
                               SetupFileInfoTransfer(2 + (transfercount / (0x6 * 2)));
                             }

                             val = (transfileinfo[transfercount % (0x6 * 2)] << 8) |
                                    transfileinfo[transfercount % (0x6 * 2) + 1];

                             transfercount += 2;
                             cdwnum += 2;

                             if (transfercount > (254 * (0x6 * 2)))
                             {
                                transfercount = 0;
                                infotranstype = -1;
                             }

                             break;
                     default: break;
                  }
                  break;
    default:
#if DEBUG
             cerr << "cs2\t: Undocumented register read " << hex << addr << endl;
#endif
             val = Memory::getWord(addr);
             break;
  }

  return val;
}

void Cs2::setWord(unsigned long addr, unsigned short val) {
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x90008:
    case 0x9000A:
                  reg.HIRQ &= val;
                  return;
    case 0x9000C: 
    case 0x9000E: reg.HIRQMASK = val;
                  return;
    case 0x90018: 
    case 0x9001A: status &= ~CDB_STAT_PERI;
                  _command = true;
                  reg.CR1 = val;
                  return;
    case 0x9001C:
    case 0x9001E: reg.CR2 = val;
                  return;
    case 0x90020:
    case 0x90022: reg.CR3 = val;
                  return;
    case 0x90024:
    case 0x90026: reg.CR4 = val;
                  setCommandTiming(reg.CR1 >> 8);
                  return;
    case 0x90028:
    case 0x9002A: reg.MPEGRGB = val;
                  return;
    default:
#if DEBUG
             cerr << "cs2\t:Undocumented register write " << hex << addr << endl;
#endif
                  Memory::setWord(addr, val);
                  break;
  }
}

unsigned long Cs2::getLong(unsigned long addr) {
  unsigned long val=0;
  if (addr >= size) { 
#ifndef _arch_dreamcast
	throw BadMemoryAccess(addr);
#else
	printf("Bad Memory Access: %x\n", addr);
#endif
  }
  switch(addr) {
    case 0x90008:
                  val = reg.HIRQ;

                  if (isbufferfull)
                    val |= CDB_HIRQ_BFUL;
                  else
                    val &= ~CDB_HIRQ_BFUL;

                  if (isdiskchanged)
                    val |= CDB_HIRQ_DCHG;
                  else
                    val &= ~CDB_HIRQ_DCHG;

                  if (isonesectorstored)
                    val |= CDB_HIRQ_CSCT;
                  else
                    val &= ~CDB_HIRQ_CSCT;

                  reg.HIRQ = val;

                  val &= reg.HIRQMASK;
                  val |= (val << 16);
                  return val;
    case 0x9000C: return ((reg.HIRQMASK << 16) | reg.HIRQMASK);
    case 0x90018: return ((reg.CR1 << 16) | reg.CR1);
    case 0x9001C: return ((reg.CR2 << 16) | reg.CR2);
    case 0x90020: return ((reg.CR3 << 16) | reg.CR3);
    case 0x90024: _command = false;
                  return ((reg.CR4 << 16) | reg.CR4);
    case 0x90028: return ((reg.MPEGRGB << 16) | reg.MPEGRGB);
    case 0x18000:
                  // transfer data
                  switch (datatranstype) {
                    case 0:
                            // get sector

                            // Make sure we still have sectors to transfer
                            if (datanumsecttrans < datasectstotrans)
                            {
                               // Transfer Data
                               val = (datatranspartition->block[datanumsecttrans]->data[datatransoffset] << 24) +
                                     (datatranspartition->block[datanumsecttrans]->data[datatransoffset + 1] << 16) +
                                     (datatranspartition->block[datanumsecttrans]->data[datatransoffset + 2] << 8) +
                                      datatranspartition->block[datanumsecttrans]->data[datatransoffset + 3];

                               // increment datatransoffset/cdwnum
                               cdwnum += 4;
                               datatransoffset += 4;

                               // Make sure we're not beyond the sector size boundry
                               if (datatransoffset >= datatranspartition->block[datanumsecttrans]->size)
                               {
                                  datatransoffset = 0;
                                  datanumsecttrans++;
                               }
                            }

                            break;
                    case 2:
                            // get then delete sector

                            // Make sure we still have sectors to transfer
                            if (datanumsecttrans < datasectstotrans)
                            {
                               // Transfer Data
                               val = (datatranspartition->block[datatranssectpos+datanumsecttrans]->data[datatransoffset] << 24) +
                                     (datatranspartition->block[datatranssectpos+datanumsecttrans]->data[datatransoffset + 1] << 16) +
                                     (datatranspartition->block[datatranssectpos+datanumsecttrans]->data[datatransoffset + 2] << 8) +
                                      datatranspartition->block[datatranssectpos+datanumsecttrans]->data[datatransoffset + 3];

                               // increment datatransoffset/cdwnum
                               cdwnum += 4;
                               datatransoffset += 4;

                               // Make sure we're not beyond the sector size boundry
                               if (datatransoffset >= datatranspartition->block[datanumsecttrans]->size)
                               {
                                  datatransoffset = 0;
                                  datanumsecttrans++;
                               }
                            }
                            else
                            {
                               // Ok, so we don't have any more sectors to
                               // transfer, might as well delete them all.

                               datatranstype = -1;

                               // free blocks
                               for (long i = datatranssectpos; i < (datatranssectpos+datasectstotrans); i++)
                               {
                                  FreeBlock(datatranspartition->block[i]);
                                  datatranspartition->block[i] = NULL;
                                  datatranspartition->blocknum[i] = 0xFF;
                               }

                               // sort remaining blocks
                               SortBlocks(datatranspartition);

                               datatranspartition->size -= cdwnum;
                               datatranspartition->numblocks -= datasectstotrans;

#if CDDEBUG
                               fprintf(stderr, "cs2\t: datatranspartition->size = %x\n", datatranspartition->size);
#endif
                            }
                            break;
                    default: break;
                  }

	          break;
    default:
#if DEBUG
             cerr << "cs2\t: Undocumented register read " << hex << addr << endl;
#endif
             val = Memory::getLong(addr);
             break;
  }

  return val;
}

void Cs2::setLong(unsigned long addr, unsigned long val) {
#if CDDEBUG
   fprintf(stderr, "cs2\t: Long writing isn't implemented\n");
#endif
   Memory::setLong(addr, val);
}

Cs2::Cs2(SaturnMemory *v, int type) : Memory(0xFFFFF, 0x100000) {
  satmem = v;
  scuint = satmem->scu;
  carttype = type;

  cd = yui_cd();
  if (cd == NULL) {
	cerr << "Unable to initialize cdrom!\n";
  }
  reset();

  // If Modem is connected, set the registers
  if(carttype == CART_NETLINK)
  {
     Memory::setByte(0x95001, 0x00);
     Memory::setByte(0x95005, 0x00);
     Memory::setByte(0x95009, 0x01);
     Memory::setByte(0x9500D, 0x00);
     Memory::setByte(0x95011, 0x00);
     Memory::setByte(0x95015, 0x60);
     Memory::setByte(0x95019, 0x30);
     Memory::setByte(0x9501D, 0x01);
  }
}

Cs2::~Cs2(void) {

   if (cd != NULL) {
      delete cd;
      cd = 0;
   }
}

void Cs2::reset(void) {
  unsigned long i, i2;

  switch (cd->getStatus())
  {
     case 0:   
     case 1:
             status = CDB_STAT_PAUSE;
             FAD = 150;
             options = 0;
             repcnt = 0;
             ctrladdr = 0x41;
             track = 1;
             index = 1;
             break;
     case 2:
             status = CDB_STAT_NODISC;

             FAD = 0xFFFFFFFF;
             options = 0xFF;
             repcnt = 0xFF;
             ctrladdr = 0xFF;
             track = 0xFF;
             index = 0xFF;
             break;
     case 3:
             status = CDB_STAT_OPEN;

             FAD = 0xFFFFFFFF;
             options = 0xFF;
             repcnt = 0xFF;
             ctrladdr = 0xFF;
             track = 0xFF;
             index = 0xFF;
             break;
     default: break;
  }

  infotranstype = -1;
  datatranstype = -1;
  transfercount = 0;
  cdwnum = 0;
  getsectsize = putsectsize = 2048;
  isdiskchanged = true;
  isbufferfull = false;
  isonesectorstored = false;

  reg.CR1 = ( 0 <<8) | 'C';
  reg.CR2 = ('D'<<8) | 'B';
  reg.CR3 = ('L'<<8) | 'O';
  reg.CR4 = ('C'<<8) | 'K';
  reg.HIRQ = 0xFFFF;
  reg.HIRQMASK = 0xFFFF;

  playFAD = 0xFFFFFFFF;
  playendFAD = 0xFFFFFFFF;

  // set authentication variables to 0(not authenticated)
  satauth = 0;
  mpgauth = 0;

  // clear filter conditions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     filter[i].FAD = 0;
     filter[i].range = 0xFFFFFFFF;
     filter[i].mode = 0;
     filter[i].chan = 0;
     filter[i].smmask = 0;
     filter[i].cimask = 0;
     filter[i].fid = 0;
     filter[i].smval = 0;
     filter[i].cival = 0;
     filter[i].condtrue = 0;
     filter[i].condfalse = 0xFF;
  }

  // clear partitions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     partition[i].size = -1;
     partition[i].numblocks = 0;

     for (i2 = 0; i2 < MAX_BLOCKS; i2++)
     {
        partition[i].block[i2] = NULL;
        partition[i].blocknum[i2] = 0xFF;
     }
  }

  // clear blocks
  for (i = 0; i < MAX_BLOCKS; i++)
  {
     block[i].size = -1;
     memset(block[i].data, 0, 2352);
  }

  blockfreespace = 200;

  // initialize TOC
  memset(TOC, 0xFF, sizeof(TOC));

  // clear filesystem stuff
  curdirsect = 0;
  memset(&fileinfo, 0, sizeof(fileinfo));
  numfiles = 0;

  lastbuffer = 0xFF;

  _command = false;
  _periodiccycles = 0;
  _commandtiming = 0;
  SetTiming(false);

  // MPEG specific stuff
  mpegcon[0].audcon = mpegcon[0].vidcon = 0x00;
  mpegcon[0].audlay = mpegcon[0].vidlay = 0x00;
  mpegcon[0].audbufdivnum = mpegcon[0].vidbufdivnum = 0xFF;
  mpegcon[1].audcon = mpegcon[1].vidcon = 0x00;
  mpegcon[1].audlay = mpegcon[1].vidlay = 0x00;
  mpegcon[1].audbufdivnum = mpegcon[1].vidbufdivnum = 0xFF;

  // should verify the following
  mpegstm[0].audstm = mpegstm[0].vidstm = 0x00; 
  mpegstm[0].audstmid = mpegstm[0].vidstmid = 0x00; 
  mpegstm[0].audchannum = mpegstm[0].vidchannum = 0x00; 
  mpegstm[1].audstm = mpegstm[1].vidstm = 0x00;
  mpegstm[1].audstmid = mpegstm[1].vidstmid = 0x00; 
  mpegstm[1].audchannum = mpegstm[1].vidchannum = 0x00;
}

void Cs2::run(unsigned long timing) {
    _periodiccycles+=timing;

    if (_commandtiming > 0) {
       if (_commandtiming < timing)
       {
          execute();
          _commandtiming = 0;
       }
       else
          _commandtiming -= timing;
    }

    if (_periodiccycles >= _periodictiming)
    {
      _periodiccycles -= _periodictiming; 

      // Get Drive's current status and compare with old status
//      switch(cd->getStatus()) // this shouldn't be called every periodic response
      switch(0)
      {
         case 0:
         case 1:
                 if ((status & 0xF) == CDB_STAT_NODISC ||
                     (status & 0xF) == CDB_STAT_OPEN)
                 {
                    status = CDB_STAT_PAUSE;
                    isdiskchanged = true;
                 }
                 break;
         case 2:
                 // may need to change this
                 if ((status & 0xF) != CDB_STAT_NODISC)
                    status = CDB_STAT_NODISC;
                 break;
         case 3:
                 // may need to change this
                 if ((status & 0xF) != CDB_STAT_OPEN)
                    status = CDB_STAT_OPEN;
                 break;
         default: break;
      }

      if (_command) {
         return;
      }

      switch (status & 0xF) {
         case CDB_STAT_PAUSE:
         {
//            if (FAD >= playFAD && FAD < playendFAD)
//               status = CDB_STAT_PLAY;
//            else
               break;
         }
         case CDB_STAT_PLAY:
         {
            partition_struct *playpartition;
            playpartition = ReadFilteredSector(FAD);

            if (playpartition != NULL)
            {
               FAD++;
#if CDDEBUG
               fprintf(stderr, "blocks = %d blockfreespace = %d fad = %x playpartition->size = %x isbufferfull = %x\n", playpartition->numblocks, blockfreespace, FAD, playpartition->size, isbufferfull);
#endif
               reg.HIRQ |= CDB_HIRQ_CSCT;
               isonesectorstored = true;

               if (FAD >= playendFAD) {
                  // we're done
                  status = CDB_STAT_PAUSE;
                  SetTiming(false);
                  reg.HIRQ |= CDB_HIRQ_PEND;

#if CDDEBUG
                  fprintf(stderr, "PLAY HAS ENDED\n");
#endif
               }
               if (isbufferfull) {
#if CDDEBUG
                  fprintf(stderr, "BUFFER IS FULL\n");
#endif
//                  status = CDB_STAT_PAUSE;
               }
            }

            break;
         }
         case CDB_STAT_SEEK:
            break;
         case CDB_STAT_SCAN:
            break;
         case CDB_STAT_RETRY:
            break;
         default: break;
      }

      status |= CDB_STAT_PERI;

      // adjust registers appropriately here(fix me)
      doCDReport();
      reg.HIRQ |= CDB_HIRQ_SCDQ;
    }
}

void Cs2::command(void) {
  _command = true;
}

void Cs2::SetTiming(bool playing) {
  if (playing) {
     if (speed1x == false) // should also verify to make sure it's not reading cd audio
        _periodictiming = 13300;
     else
        _periodictiming = 6700;
  }
  else {
     _periodictiming = 16700;
  }
}

void Cs2::setCommandTiming(unsigned char cmd) {
   switch(cmd) {
      default:
               _commandtiming = 1;
               break;
   }
}

void Cs2::execute(void) {
  reg.HIRQ &= ~CDB_HIRQ_CMOK;

  unsigned short instruction = reg.CR1 >> 8;

  switch (instruction) {
    case 0x00:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getStatus\n");
#endif
      getStatus();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x01:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getHardwareInfo\n");
#endif
      getHardwareInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x02:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getToc\n");
#endif
      getToc();
      break;
    case 0x03:
    {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSessionInfo\n");
#endif
       getSessionInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
       break;
    }
    case 0x04:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: initializeCDSystem %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      initializeCDSystem();
      break;
    case 0x06:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: endDataTransfer %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      endDataTransfer();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x10:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: playDisc %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      playDisc();
      break;
    case 0x11:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: seekDisc %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      seekDisc();
      break;
    case 0x20:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSubcodeQRW %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getSubcodeQRW();
      break;
    case 0x30:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setCDDeviceConnection %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      setCDDeviceConnection();
      break;
    case 0x32:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getLastBufferDestination %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getLastBufferDestination();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x40:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterRange\n");
#endif
      setFilterRange();
      break;
    case 0x42:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterSubheaderConditions\n");
#endif
      setFilterSubheaderConditions();
      break;
    case 0x43:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFilterSubheaderConditions\n");
#endif
      getFilterSubheaderConditions();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x44:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterMode\n");
#endif
      setFilterMode();
      break;
    case 0x45:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFilterMode\n");
#endif
      getFilterMode();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x46:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterConnection\n");
#endif
      setFilterConnection();
      break;
    case 0x48:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: resetSelector %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      resetSelector();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x50:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getBufferSize\n");
#endif
      getBufferSize();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x51:
//#if CDDEBUG
//      fprintf(stderr, "cs2\t: Command: getSectorNumber %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
//#endif
      getSectorNumber();

//#if CDDEBUG
//      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
//#endif
      break;
    case 0x52:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: calculateActualSize %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      calculateActualSize();

      break;
    case 0x53:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getActualSize\n");
#endif
      getActualSize();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x54:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSectorInfo %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getSectorInfo();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x60:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setSectorLength %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      setSectorLength();
      break;
    case 0x61:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSectorData %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x62:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: deleteSectorData %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      deleteSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x63:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getThenDeleteSectorData %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getThenDeleteSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x64:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: putSectorData %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      putSectorData();
      break;
    case 0x67:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getCopyError\n");
#endif
      getCopyError();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x70:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: changeDirectory %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      changeDirectory();

      break;
    case 0x71:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: readDirectory %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      readDirectory();

      break;
    case 0x72:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileSystemScope\n");
#endif
      getFileSystemScope();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x73:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileInfo %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      getFileInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x74:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: readFile\n");
#endif
      readFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x75:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: abortFile\n");
#endif
      abortFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x90:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetStatus\n");
#endif
      mpegGetStatus();
      break;
    case 0x91:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetInterrupt\n");
#endif
       mpegGetInterrupt();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      break;
    case 0x92:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetInterruptMask\n");
#endif
      mpegSetInterruptMask();
      break;
    case 0x93: 
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegInit\n");
#endif
      mpegInit();
      break;
    case 0x94:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetMode\n");
#endif
      mpegSetMode();

      break;
    case 0x95:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegPlay\n");
#endif
      mpegPlay();
      break;
    case 0x96:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetDecodingMethod\n");
#endif
       mpegSetDecodingMethod();
       break;
    case 0x9A:      
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetConnection\n");
#endif
       mpegSetConnection();
       break;
    case 0x9B:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetConnection\n");
#endif
      mpegGetConnection();

      break;
    case 0x9D:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetStream\n");
#endif
      mpegSetStream();
      break;
    case 0x9E:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetStream\n");
#endif
      mpegGetStream();
      break;
    case 0xA0:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegDisplay\n");
#endif
      mpegDisplay();
      break;
    case 0xA1:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetWindow\n");
#endif
       mpegSetWindow();
       break;
    case 0xA2:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetBorderColor\n");
#endif
      mpegSetBorderColor();
      break;
    case 0xA3:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetFade\n");
#endif
      mpegSetFade();
      break;
    case 0xA4:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetVideoEffects\n");
#endif
      mpegSetVideoEffects();
      break;
    case 0xAF:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetLSI\n");
#endif
      mpegSetLSI();
      break;
    case 0xE0:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE0 %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif
      cmdE0();
      break;
    case 0xE1:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE1 %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif 
      cmdE1();
      break;
    case 0xE2:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE2 %04x %04x %04x %04x %04x\n", reg.HIRQ, reg.CR1, reg.CR2, reg.CR3, reg.CR4);
#endif 
      cmdE2();
      break;
    default:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command %02x not implemented\n", instruction);
#endif
      break;
  }
}

void Cs2::getStatus(void) {
  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::getHardwareInfo(void) {
  if ((status & 0xF) != CDB_STAT_OPEN && (status & 0xF) != CDB_STAT_NODISC)
     isdiskchanged = false;

  reg.CR1 = status << 8;
  // hardware flags/CD Version
  reg.CR2 = 0x0201; // mpeg card exists
  // mpeg version, it actually is required(at least by the bios)

  if (mpgauth)
     reg.CR3 = 0x1;
  else
     reg.CR3 = 0;

  // drive info/revision
  reg.CR4 = 0x0400;
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::getToc(void) {
  cd->readTOC(TOC);

  transfercount = 0;
  infotranstype = 0;

  reg.CR1 = status << 8;
  reg.CR2 = 0xCC;
  reg.CR3 = 0x0;
  reg.CR4 = 0x0; 
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
  status = CDB_STAT_PAUSE;
}

void Cs2::getSessionInfo(void) {

  switch (reg.CR1 & 0xFF) {
    case 0:
            reg.CR3 = 0x0100 | ((TOC[101] & 0xFF0000) >> 16);
            reg.CR4 = TOC[101] & 0x00FFFF;
            break;
    case 1:
            reg.CR3 = 0x0100; // return Session number(high byte)/and first byte of Session lba
            reg.CR4 = 0; // lower word of Session lba
            break;
    default:
            reg.CR3 = 0xFFFF;
            reg.CR4 = 0xFFFF;
            break;
  }

  status = CDB_STAT_PAUSE;
  reg.CR1 = status << 8;
  reg.CR2 = 0;

  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::initializeCDSystem(void) {
  unsigned short val=0;
  unsigned char initflag = reg.CR1 & 0xFF;

  if (status & 0xF != CDB_STAT_OPEN && status & 0xF != CDB_STAT_NODISC)
  {
     status = CDB_STAT_PAUSE;
     FAD = 150;
  }

  if (initflag & 0x1)
  {
     // Reset CD block software
  }

  if (initflag & 0x2)
  {
     // Decode RW subcode
  }

  if (initflag & 0x4)
  {
     // Don't confirm Mode 2 subheader
  }

  if (initflag & 0x8)
  {
     // Retry reading Form 2 sectors
  }

  if (initflag & 0x10)
     speed1x = true;
  else
     speed1x = false;

  val = reg.HIRQ & 0xFFE5;
  isbufferfull = false;

  if (isdiskchanged)
     val |= CDB_HIRQ_DCHG;
  else
     val &= ~CDB_HIRQ_DCHG;

  reg.CR1 = (status << 8) | (repcnt & 0xF);
  reg.CR2 = (ctrladdr << 8) | (track & 0xFF);
  reg.CR3 = (index << 8) | ((FAD >> 16) &0xFF);
  reg.CR4 = (unsigned short) FAD;
  reg.HIRQ = val | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::endDataTransfer(void) {
  if (cdwnum)
  {
     reg.CR1 = (status << 8) | ((cdwnum >> 17) & 0xFF);
     reg.CR2 = (unsigned short)(cdwnum >> 1);
     reg.CR3 = 0;
     reg.CR4 = 0;
  }
  else
  {
     reg.CR1 = (status << 8) | 0xFF; // FIXME
     reg.CR2 = 0xFFFF;
     reg.CR3 = 0;
     reg.CR4 = 0;
  }

  // stop any transfers that may be going(this is still probably wrong)

  switch (datatranstype) {    
     case 2: {
        // Get Then Delete Sector

        // Make sure we actually have to free something
        if (datatranspartition->size <= 0) break;

        datatranstype = -1;

        // free blocks
        for (long i = datatranssectpos; i < (datatranssectpos+datasectstotrans); i++)
        {
           FreeBlock(datatranspartition->block[i]);
           datatranspartition->block[i] = NULL;
           datatranspartition->blocknum[i] = 0xFF;
        }

        // sort remaining blocks
        SortBlocks(datatranspartition);

        datatranspartition->size -= cdwnum;
        datatranspartition->numblocks -= datasectstotrans;

        if (blockfreespace == 200) isonesectorstored = false;

        break;
     }
     default: break;
  }

  cdwnum = 0;

  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::playDisc(void) {
  unsigned long pdspos;
  unsigned long pdepos;
  unsigned long pdpmode;

  // Get all the arguments
  pdspos = ((reg.CR1 & 0xFF) << 16) | reg.CR2;
  pdepos = ((reg.CR3 & 0xFF) << 16) | reg.CR4;
  pdpmode = reg.CR3 >> 8;

  // Convert Start Position to playFAD
  if (pdspos == 0xFFFFFF)
  {
     // No Change
  }
  else if (pdspos & 0x800000)
  {
     // FAD Mode
     playFAD = (pdspos & 0xFFFFF);
     SetupDefaultPlayStats(FADToTrack(playFAD));
     FAD = playFAD;
  }
  else if (pdspos != 0)
  {
     // Track Mode
     SetupDefaultPlayStats((pdspos & 0xFF00) >> 8);
     FAD = playFAD = FAD + (pdspos & 0xFF); // is this right?
  }
  else
  {
     // Default Mode
#if CDDEBUG
     fprintf(stderr, "playdisc Default Mode is not implemented\n");
#endif
  }

  // Convert End Position to playendFAD
  if (pdepos == 0xFFFFFF)
  {
     // No Change
  }
  else if (pdepos & 0x800000)
  {
     // FAD Mode
     playendFAD = playFAD+(pdepos & 0xFFFFF);
  }
  else if (pdepos != 0)
  {
     // Track Mode
     playendFAD = TrackToFad(pdepos);
  }
  else
  {
     // Default Mode
     playendFAD = TrackToFad(0xFFFF);
  }

  // setup play mode here
#if CDDEBUG
  if (pdpmode != 0)
     fprintf(stderr, "cs2\t: playDisc: Unsupported play mode = %02X\n", pdpmode);
#endif

  SetTiming(true);

  status = CDB_STAT_PLAY;

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::seekDisc(void) {
  if (reg.CR1 & 0x80)
  {
     // Seek by FAD
     unsigned long sdFAD;

     sdFAD = ((reg.CR1 & 0xFF) << 16) | reg.CR2;

     if (sdFAD == 0xFFFFFF)
        status = CDB_STAT_PAUSE;
     else
     {
#if CDDEBUG
        fprintf(stderr, "cs2\t: seekDisc - FAD Mode not supported\n");
#endif
     }
  }
  else
  {
     // Were we given a valid track number?
     if (reg.CR2 >> 8)
     {
        // Seek by index
        status = CDB_STAT_PAUSE;
        SetupDefaultPlayStats((reg.CR2 >> 8));
        index = reg.CR2 & 0xFF;
     }
     else
     {
        // Error
        status = CDB_STAT_STANDBY;
        options = 0xFF;
        repcnt = 0xFF;
        ctrladdr = 0xFF;
        track = 0xFF;
        index = 0xFF;
        FAD = 0xFFFFFFFF;
     }
  }

  SetTiming(false);

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::getSubcodeQRW(void) {
  // According to Tyranid's doc, the subcode type is stored in the low byte
  // of CR2. However, Sega's CDC library writes the type to the low byte
  // of CR1. Somehow I'd sooner believe Sega is right.
  switch(reg.CR1 & 0xFF) {
     case 0:
             // Get Q Channel
             reg.CR1 = (status << 8) | 0;
             reg.CR2 = 5;
             reg.CR3 = 0;
             reg.CR4 = 0;

             // setup transfer here(fix me)
             break;
     case 1:
             // Get RW Channel
             reg.CR1 = (status << 8) | 0;
             reg.CR2 = 12;
             reg.CR3 = 0;
             reg.CR4 = 0;

             // setup transfer here(fix me)
             break;
     default: break;
  }

  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
}

void Cs2::setCDDeviceConnection(void) {
  unsigned long scdcfilternum;

  scdcfilternum = (reg.CR3 >> 8);

  if (scdcfilternum == 0xFF)
     outconcddev = NULL;
  else if (scdcfilternum < 0x24)
     outconcddev = filter + scdcfilternum;

  outconcddevnum = scdcfilternum;

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getLastBufferDestination(void) {
  reg.CR1 = (status << 8);
  reg.CR2 = 0;
  reg.CR3 = lastbuffer << 8; 
  reg.CR4 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::setFilterRange(void) {
  unsigned char sfrfilternum;

  sfrfilternum = reg.CR3 >> 8;

  filter[sfrfilternum].FAD = ((reg.CR1 & 0xFF) << 16) | reg.CR2;
  filter[sfrfilternum].range = ((reg.CR3 & 0xFF) << 16) | reg.CR4;

  // return default cd stats
  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::setFilterSubheaderConditions(void) {
  unsigned char sfscfilternum;

  sfscfilternum = reg.CR3 >> 8;

  filter[sfscfilternum].chan = reg.CR1 & 0xFF;
  filter[sfscfilternum].smmask = reg.CR2 >> 8;
  filter[sfscfilternum].cimask = reg.CR2 & 0xFF;
  filter[sfscfilternum].fid = reg.CR3 & 0xFF;;
  filter[sfscfilternum].smval = reg.CR4 >> 8;
  filter[sfscfilternum].cival = reg.CR4 & 0xFF;

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getFilterSubheaderConditions(void) {
  unsigned char gfscfilternum;

  gfscfilternum = reg.CR3 >> 8;

  reg.CR1 = (status << 8) | filter[gfscfilternum].chan;
  reg.CR2 = (filter[gfscfilternum].smmask << 8) | filter[gfscfilternum].cimask;
  reg.CR3 = filter[gfscfilternum].fid;
  reg.CR4 = (filter[gfscfilternum].smval << 8) | filter[gfscfilternum].cival;
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::setFilterMode(void) {
  unsigned char sfmfilternum;

  sfmfilternum = reg.CR3 >> 8;

  filter[sfmfilternum].mode = reg.CR1 & 0xFF;

  if (filter[sfmfilternum].mode & 0x80)
  {
     // Initialize filter conditions
     filter[sfmfilternum].mode = 0;
     filter[sfmfilternum].FAD = 0;
     filter[sfmfilternum].range = 0;
     filter[sfmfilternum].chan = 0;
     filter[sfmfilternum].smmask = 0;
     filter[sfmfilternum].cimask = 0;
     filter[sfmfilternum].smval = 0;
     filter[sfmfilternum].cival = 0;
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getFilterMode(void) {
  unsigned char gfmfilternum;

  gfmfilternum = reg.CR3 >> 8;

  reg.CR1 = (status << 8) | filter[gfmfilternum].mode;
  reg.CR2 = 0;
  reg.CR3 = 0;
  reg.CR4 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::setFilterConnection(void) {
  unsigned char sfcfilternum;

  sfcfilternum = reg.CR3 >> 8;

  if (reg.CR1 & 0x1)
  {
     // Set connection for true condition
     filter[sfcfilternum].condtrue = reg.CR2 >> 8;
  }

  if (reg.CR1 & 0x2)
  {
     // Set connection for false condition
     filter[sfcfilternum].condfalse = reg.CR2 & 0xFF;
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::resetSelector(void) {
  // still needs a bit of work
  unsigned long i, i2;

  if ((reg.CR1 & 0xFF) == 0)
  {
     // Reset specified partition buffer only
     unsigned long rsbufno=reg.CR3 >> 8;

     // sort remaining blocks
     if (rsbufno < MAX_SELECTORS)
     {
        // clear partition
        for (i = 0; i < partition[rsbufno].numblocks; i++)
        {
           FreeBlock(partition[rsbufno].block[i]);
           partition[rsbufno].block[i] = NULL;
           partition[rsbufno].blocknum[i] = 0xFF;
        }

        partition[rsbufno].size = -1;
        partition[rsbufno].numblocks = 0;
     }

     if (blockfreespace > 0) isbufferfull = false;
     if (blockfreespace == 200) isonesectorstored = false;

     doCDReport();
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
     return;
  }

  // parse flags and reset the specified area(fix me)
  if (reg.CR1 & 0x80)
  {
     // reset false filter output connections
     for (i = 0; i < MAX_SELECTORS; i++)
        filter[i].condfalse = 0xFF;
  }

  if (reg.CR1 & 0x40)
  {
     // reset true filter output connections
     for (i = 0; i < MAX_SELECTORS; i++)
        filter[i].condtrue = i;
  }

  if (reg.CR1 & 0x10)
  {
     // reset filter conditions
     for (i = 0; i < MAX_SELECTORS; i++)
     {
        filter[i].FAD = 0;
        filter[i].range = 0xFFFFFFFF;
        filter[i].mode = 0;
        filter[i].chan = 0;
        filter[i].smmask = 0;
        filter[i].cimask = 0;
        filter[i].fid = 0;
        filter[i].smval = 0;
        filter[i].cival = 0;
     }
  }

  if (reg.CR1 & 0x8)
  {
     // reset partition output connectors
  }

  if (reg.CR1 & 0x4)
  {
     // reset partitions buffer data
     isbufferfull = false;

     // clear partitions
     for (i = 0; i < MAX_SELECTORS; i++)
     {
        partition[i].size = -1;
        partition[i].numblocks = 0;

        for (i2 = 0; i2 < MAX_BLOCKS; i2++)
        {
           partition[i].block[i2] = NULL;
           partition[i].blocknum[i2] = 0xFF;
        }
     }

     // clear blocks
     for (i = 0; i < MAX_BLOCKS; i++)
     {
        block[i].size = -1;
        memset(block[i].data, 0, 2352);
     }

     isonesectorstored = false;
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getBufferSize(void) {
  reg.CR1 = status << 8;
  reg.CR2 = blockfreespace;
  reg.CR3 = MAX_SELECTORS << 8;
  reg.CR4 = MAX_BLOCKS;
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::getSectorNumber(void) {
  unsigned long gsnbufno;

  gsnbufno = reg.CR3 >> 8;

  if (partition[gsnbufno].size == -1)
     reg.CR4 = 0;
  else
     reg.CR4 = partition[gsnbufno].numblocks;

  reg.CR1 = status << 8;
  reg.CR2 = 0;
  reg.CR3 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
}

void Cs2::calculateActualSize(void) {
  unsigned long i;
  unsigned long casbufno;
  unsigned short cassectoffset;
  unsigned short casnumsect;

  cassectoffset = reg.CR2;
  casbufno = reg.CR3 >> 8;
  casnumsect = reg.CR4;

  if (partition[casbufno].size != 0)
  {
     calcsize = 0;

     for (i = 0; i < casnumsect; i++)
     {
        if (partition[casbufno].block[cassectoffset])
           calcsize += (partition[casbufno].block[cassectoffset]->size / 2);
     }
  }
  else
     calcsize = 0;

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getActualSize(void) {
  reg.CR1 = (status << 8) | ((calcsize >> 16) & 0xFF);
  reg.CR2 = calcsize & 0xFFFF;
  reg.CR3 = 0;
  reg.CR4 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getSectorInfo(void) {
  unsigned long gsisctnum;
  unsigned long gsibufno;

  gsisctnum=reg.CR2 & 0xFF;
  gsibufno=reg.CR3 >> 8;
  if (gsibufno < MAX_SELECTORS) {
     if (gsisctnum < partition[gsibufno].numblocks) {
        reg.CR1 = (status << 8) | ((partition[gsibufno].block[gsisctnum]->FAD >> 16) & 0xFF);
        reg.CR2 = partition[gsibufno].block[gsisctnum]->FAD & 0xFFFF;
        reg.CR3 = (partition[gsibufno].block[gsisctnum]->fn << 8) | partition[gsibufno].block[gsisctnum]->cn;
        reg.CR4 = (partition[gsibufno].block[gsisctnum]->sm << 8) | partition[gsibufno].block[gsisctnum]->ci;
        reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
        return;
     }
     else
     {
#if CDDEBUG
        fprintf(stderr, "cs2\t: getSectorInfo: Unsupported Partition Number\n");
#endif
     }
  }

  reg.CR1 = (CDB_STAT_REJECT << 8) | reg.CR1 & 0xFF;
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::setSectorLength(void) {
  switch (reg.CR1 & 0xFF) {
    case 0:
            getsectsize = 2048;
            break;
    case 1:
            getsectsize = 2336;
            break;
    case 2:
            getsectsize = 2340;
            break;
    case 3:
            getsectsize = 2352;
            break;
    default: break;
  }

  switch (reg.CR2 >> 8) {
    case 0:
            putsectsize = 2048;
            break;
    case 1:
            putsectsize = 2336;
            break;
    case 2:
            putsectsize = 2340;
            break;
    case 3:
            putsectsize = 2352;
            break;
    default: break;
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2::getSectorData(void) {
  unsigned long gsdbufno;

  gsdbufno = reg.CR3 >> 8;

  if (gsdbufno < MAX_SELECTORS)
  {
     // Setup Data Transfer
     cdwnum = 0;
     datatranstype = 0;
     datatranspartition = partition + gsdbufno;
     datatranspartitionnum = gsdbufno;
     datatransoffset = 0;
     datanumsecttrans = 0;
     datatranssectpos = reg.CR2;
     datasectstotrans = reg.CR4;

     doCDReport();
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY | CDB_HIRQ_EHST;
  }
  else
  {
     reg.CR1 = (CDB_STAT_REJECT << 8) | ((options & 0xF) << 4) | (repcnt & 0xF);
     reg.CR2 = (ctrladdr << 8) | (track & 0xFF);
     reg.CR3 = (index << 8) | ((FAD >> 16) &0xFF);
     reg.CR4 = (unsigned short) FAD;
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2::deleteSectorData(void) {
  unsigned long dsdsectoffset=0;
  unsigned long dsdbufno=0;
  unsigned long dsdsectnum=0;

  dsdsectoffset = reg.CR2;
  dsdbufno = reg.CR3 >> 8;
  dsdsectnum = reg.CR4;

  if (dsdbufno < MAX_SELECTORS)
  {
     if (dsdsectoffset == 0xFFFF)
     {
        // delete last sector 
#if CDDEBUG
        fprintf(stderr, "cs2\t:deleteSectorData: FIXME - sector offset of 0xFFFF not supported\n");
#endif
     }
     else if (dsdsectnum == 0xFFFF)
     {
        // delete sector x sectors from last?
#if CDDEBUG
        fprintf(stderr, "cs2\t:deleteSectorData: FIXME - sector number of 0xFFFF not supported\n");
#endif
        // calculate which sector we want to delete
        dsdsectoffset = partition[dsdbufno].numblocks - dsdsectoffset - 1;
        // I think this is right
        dsdsectnum = 1; 
     }

     for (unsigned long i = dsdsectoffset; i < (dsdsectoffset+dsdsectnum); i++)
     {
        partition[dsdbufno].size -= partition[dsdbufno].block[i]->size;
        FreeBlock(partition[dsdbufno].block[i]);
        partition[dsdbufno].block[i] = NULL;
        partition[dsdbufno].blocknum[i] = 0xFF;
     }

     // sort remaining blocks
     SortBlocks(&partition[dsdbufno]);

     partition[dsdbufno].numblocks -= dsdsectnum;

     if (blockfreespace == 200) isonesectorstored = false;

     doCDReport();
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
  else
  {
     reg.CR1 = (CDB_STAT_REJECT << 8) | ((options & 0xF) << 4) | (repcnt & 0xF);
     reg.CR2 = (ctrladdr << 8) | (track & 0xFF);
     reg.CR3 = (index << 8) | ((FAD >> 16) &0xFF);
     reg.CR4 = (unsigned short) FAD;
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2::getThenDeleteSectorData(void) {
  unsigned long gtdsdbufno;

  gtdsdbufno = reg.CR3 >> 8;

  if (gtdsdbufno < MAX_SELECTORS)
  {
     // Setup Data Transfer
     cdwnum = 0;
     datatranstype = 2;
     datatranspartition = partition + gtdsdbufno;
     datatransoffset = 0;
     datanumsecttrans = 0;
     datatranssectpos = reg.CR2;
     datasectstotrans = reg.CR4;

     doCDReport();
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY | CDB_HIRQ_EHST;
  }
  else
  {
     reg.CR1 = (CDB_STAT_REJECT << 8) | ((options & 0xF) << 4) | (repcnt & 0xF);
     reg.CR2 = (ctrladdr << 8) | (track & 0xFF);
     reg.CR3 = (index << 8) | ((FAD >> 16) &0xFF);
     reg.CR4 = (unsigned short) FAD;
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2::putSectorData(void) {
  unsigned long psdfiltno;

  psdfiltno = reg.CR3 >> 8;

  if (psdfiltno < MAX_SELECTORS)
  {
     // I'm not really sure what I'm supposed to really be doing or returning
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
  else
  {
     reg.CR1 = (CDB_STAT_REJECT << 8) | ((options & 0xF) << 4) | (repcnt & 0xF);
     reg.CR2 = (ctrladdr << 8) | (track & 0xFF);
     reg.CR3 = (index << 8) | ((FAD >> 16) &0xFF);
     reg.CR4 = (unsigned short) FAD;
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2::getCopyError(void) {
  reg.CR1 = status << 8;
  reg.CR2 = 0;
  reg.CR3 = 0;
  reg.CR4 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::changeDirectory(void) {
  unsigned long cdfilternum;

  cdfilternum = (reg.CR3 >> 8);

  if (cdfilternum == 0xFF)
  {
     // fix me
  }
  else if (cdfilternum < 0x24)
  {
     if (ReadFileSystem(filter + cdfilternum, ((reg.CR3 & 0xFF) << 16) | reg.CR4, false) != 0)
     {
        // fix me
#if CDDEBUG
        fprintf(stderr, "cs2\t: ReadFileSystem failed\n");
#endif
     }
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2::readDirectory(void) {
  unsigned long rdfilternum;

  rdfilternum = (reg.CR3 >> 8);

  if (rdfilternum == 0xFF)
  {
     // fix me
  }
  else if (rdfilternum < 0x24)
  {
     if (ReadFileSystem(filter + rdfilternum, ((reg.CR3 & 0xFF) << 8) | reg.CR4, true) != 0)
     {
        // fix me
#if CDDEBUG
        fprintf(stderr, "cs2\t: ReadFileSystem failed\n");
#endif
     }
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2::getFileSystemScope(void) {
  // may need to fix this
  reg.CR1 = status << 8;
  reg.CR2 = numfiles - 2;
  reg.CR3 = 0x0100;
  reg.CR4 = 0x0002;

  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2::getFileInfo(void) {
  unsigned long gfifid;

  gfifid = ((reg.CR3 & 0xFF) << 16) | reg.CR4;

  if (gfifid == 0xFFFFFF)
  {
     transfercount = 0;
     infotranstype = 2;

     reg.CR1 = status << 8;
     reg.CR2 = 0x05F4;
     reg.CR3 = 0;
     reg.CR4 = 0;
  }
  else
  {
     SetupFileInfoTransfer(gfifid);

     transfercount = 0;
     infotranstype = 1;

     reg.CR1 = status << 8;
     reg.CR2 = 0x06;
     reg.CR3 = 0;
     reg.CR4 = 0;
  }

  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;

}

void Cs2::readFile(void) {
  unsigned long rffid, rfoffset, rfsize;

  rffid = ((reg.CR3 & 0xFF) << 8) | reg.CR4;
  rfoffset = ((reg.CR1 & 0xFF) << 8) | reg.CR2;
  rfsize = ((fileinfo[rffid].size + getsectsize - 1) /
           getsectsize) - rfoffset;

  SetupDefaultPlayStats(FADToTrack(fileinfo[rffid].lba + rfoffset));

  playFAD = FAD = fileinfo[rffid].lba + rfoffset;
  playendFAD = playFAD + rfsize;

  options = 0x8;

  SetTiming(true);

  status = CDB_STAT_PLAY;

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2::abortFile(void) {
  if ((status & 0xF) != CDB_STAT_OPEN &&
      (status & 0xF) != CDB_STAT_NODISC)
     status = CDB_STAT_PAUSE;
  isonesectorstored = false;
  datatranstype = -1;
  cdwnum = 0;
  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2::mpegGetStatus(void) {
   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegGetInterrupt(void) {
   unsigned long mgiworkinterrupt;

   // mpeg interrupt should be retrieved here
   mgiworkinterrupt = 0;

   // mask interupt
   mgiworkinterrupt &= mpegintmask;

   reg.CR1 = (status << 8) | ((mgiworkinterrupt >> 16) & 0xFF);
   reg.CR2 = (unsigned short)mgiworkinterrupt;
   reg.CR3 = 0;
   reg.CR4 = 0;
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetInterruptMask(void) {
   mpegintmask = ((reg.CR1 & 0xFF) << 16) | reg.CR2;

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegInit(void) {

  if (mpgauth)
     reg.CR1 = status << 8;
  else
     reg.CR1 = 0xFF00;

  // double-check this
  if (reg.CR2 == 0x0001) // software timer/reset?
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM | CDB_HIRQ_MPED | CDB_HIRQ_MPST; 
  else
     reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPED | CDB_HIRQ_MPST;

  reg.CR2 = 0;
  reg.CR3 = 0;
  reg.CR4 = 0;

  // future mpeg-related variables should be initialized here
}

void Cs2::mpegSetMode(void) {
   // fix me

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegPlay(void) {
   // fix me

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetDecodingMethod(void) {
   // fix me

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetConnection(void) {
   bool mscnext = (reg.CR3 >> 8);

   if (mscnext == false)
   {
      // Current
      mpegcon[0].audcon = reg.CR1 & 0xFF;
      mpegcon[0].audlay = reg.CR2 >> 8;
      mpegcon[0].audbufdivnum = reg.CR2 & 0xFF;
      mpegcon[0].vidcon = reg.CR3 & 0xFF;
      mpegcon[0].vidlay = reg.CR4 >> 8;
      mpegcon[0].vidbufdivnum = reg.CR4 & 0xFF;
   }
   else
   {
      // Next
      mpegcon[1].audcon = reg.CR1 & 0xFF;
      mpegcon[1].audlay = reg.CR2 >> 8;
      mpegcon[1].audbufdivnum = reg.CR2 & 0xFF;
      mpegcon[1].vidcon = reg.CR3 & 0xFF;
      mpegcon[1].vidlay = reg.CR4 >> 8;
      mpegcon[1].vidbufdivnum = reg.CR4 & 0xFF;
   }

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegGetConnection(void) {
   bool mgcnext = (reg.CR3 >> 8);

   if (mgcnext == false)
   {
      // Current
      reg.CR1 = (status << 8) | mpegcon[0].audcon;
      reg.CR2 = (mpegcon[0].audlay << 8) | mpegcon[0].audbufdivnum;
      reg.CR3 = mpegcon[0].vidcon;
      reg.CR4 = (mpegcon[0].vidlay << 8) | mpegcon[0].vidbufdivnum;
   }
   else
   {
      // Next
      reg.CR1 = (status << 8) | mpegcon[1].audcon;
      reg.CR2 = (mpegcon[1].audlay << 8) | mpegcon[1].audbufdivnum;
      reg.CR3 = mpegcon[1].vidcon;
      reg.CR4 = (mpegcon[1].vidlay << 8) | mpegcon[1].vidbufdivnum;
   }

   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetStream(void) {
   bool mssnext = (reg.CR3 >> 8);

   if (mssnext == false)
   {
      // Current
      mpegstm[0].audstm = reg.CR1 & 0xFF;
      mpegstm[0].audstmid = reg.CR2 >> 8;
      mpegstm[0].audchannum = reg.CR2 & 0xFF;
      mpegstm[0].vidstm = reg.CR3 & 0xFF;
      mpegstm[0].vidstmid = reg.CR4 >> 8;
      mpegstm[0].vidchannum = reg.CR4 & 0xFF;
   }
   else
   {
      // Next
      mpegstm[1].audstm = reg.CR1 & 0xFF;
      mpegstm[1].audstmid = reg.CR2 >> 8;
      mpegstm[1].audchannum = reg.CR2 & 0xFF;
      mpegstm[1].vidstm = reg.CR3 & 0xFF;
      mpegstm[1].vidstmid = reg.CR4 >> 8;
      mpegstm[1].vidchannum = reg.CR4 & 0xFF;
   }

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegGetStream(void) {
   bool mgsnext = (reg.CR3 >> 8);

   if (mgsnext == false)
   {
      // Current
      reg.CR1 = (status << 8) | mpegstm[0].audstm;
      reg.CR2 = (mpegstm[0].audstmid << 8) | mpegstm[0].audchannum;
      reg.CR3 = mpegstm[0].vidstm;
      reg.CR4 = (mpegstm[0].vidstmid << 8) | mpegstm[0].vidchannum;
   }
   else
   {
      // Next
      reg.CR1 = (status << 8) | mpegstm[1].audstm;
      reg.CR2 = (mpegstm[1].audstmid << 8) | mpegstm[1].audchannum;
      reg.CR3 = mpegstm[1].vidstm;
      reg.CR4 = (mpegstm[1].vidstmid << 8) | mpegstm[1].vidchannum;
   }

   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegDisplay(void) {
   // fix me(should be setting display setting)

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetWindow(void) {
   // fix me(should be setting windows settings)

   // return default mpeg stats
   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetBorderColor() {
   // fix me(should be setting border color)

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetFade() {
   // fix me(should be setting fade setting)

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetVideoEffects(void) {
   // fix me(should be setting video effects settings)

   doMPEGReport();
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::mpegSetLSI() {
   // fix me(should be setting the LSI, among other things)
   reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2::cmdE0(void) {
  bool mpegauth;

  mpegauth = reg.CR2 & 0xFF;


  if ((status & 0xF) != CDB_STAT_NODISC &&
      (status & 0xF) != CDB_STAT_OPEN)
  {
     // Set registers all to invalid values(aside from status)
     status = CDB_STAT_BUSY;

     reg.CR1 = (status << 8) | 0xFF;
     reg.CR2 = 0xFFFF;
     reg.CR3 = 0xFFFF;
     reg.CR4 = 0xFFFF;

     if (mpegauth == 1)
     {
        reg.HIRQ |= CDB_HIRQ_MPED;
        mpgauth = 2;
     }
     else
     {     
        // if authentication passes(obviously it always does), CDB_HIRQ_CSCT is set
        isonesectorstored = true;
        reg.HIRQ |= CDB_HIRQ_EFLS | CDB_HIRQ_CSCT;
        satauth = 4;
     }

     // Set registers all back to normal values

     status = CDB_STAT_PAUSE;
  }
  else
  {
     if (mpegauth == 1)
     {
        reg.HIRQ |= CDB_HIRQ_MPED;
        mpgauth = 2;
     }
     else
        reg.HIRQ |= CDB_HIRQ_EFLS | CDB_HIRQ_CSCT;
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::cmdE1(void) {
  reg.CR1 = (status << 8);
  if (reg.CR2)
     reg.CR2 = mpgauth;
  else
     reg.CR2 = satauth;
  reg.CR3 = 0;
  reg.CR4 = 0;
  reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2::cmdE2(void) {
  FILE *mpgfp;
  partition_struct *mpgpartition;

  // fix me
  mpgauth |= 0x300;

  outconmpegrom = filter + 0;
  outconmpegromnum = 0;

  if ((mpgfp = fopen(yui_mpegrom(), "rb")) != NULL)
  {
     unsigned long readoffset = ((reg.CR1 & 0xFF) << 8) | reg.CR2;
     unsigned short readsize = reg.CR4;

     fseek(mpgfp, readoffset * getsectsize, SEEK_SET);
     if ((mpgpartition = GetPartition(outconmpegrom)) != NULL && !isbufferfull)
     {
        mpgpartition->size = 0;

        for (int i=0; i < readsize; i++)
        {
           mpgpartition->block[mpgpartition->numblocks] = AllocateBlock(&mpgpartition->blocknum[mpgpartition->numblocks]);

           if (mpgpartition->block[mpgpartition->numblocks] != NULL) {
              // read data
              fread((void *)mpgpartition->block[mpgpartition->numblocks]->data, 1, getsectsize, mpgfp);

              mpgpartition->numblocks++;
              mpgpartition->size += getsectsize;
           }
        }

        isonesectorstored = true;
        reg.HIRQ |= CDB_HIRQ_CSCT;
     }

     fclose(mpgfp);
  }

  doCDReport();
  reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPED;
}

unsigned char Cs2::FADToTrack(unsigned long val) {
  for (int i = 0; i < 99; i++)
  {
     if (TOC[i] == 0xFFFFFFFF) return 0xFF;

     if (val >= (TOC[i] & 0xFFFFFF) && val < (TOC[i+1] & 0xFFFFFF))
        return (i+1);
  }

  return 0;
}

unsigned long Cs2::TrackToFad(unsigned short trackandindex) {
  if (trackandindex == 0xFFFF)
     // leadout position
     return (TOC[101] & 0x00FFFFFF); 
  if (trackandindex != 0x0000)
     // regular track
     return (TOC[(trackandindex >> 8) - 1] & 0x00FFFFFF) + (trackandindex & 0xFF);

  // assume it's leadin
  return 0;
}

void Cs2::SetupDefaultPlayStats(unsigned char track_number) {
  if (track_number != 0xFF)
  {
     options = 0;
     repcnt = 0;
     ctrladdr = (TOC[track_number - 1] & 0xFF000000) >> 24;
     index = 1;
     track = track_number;
     FAD = TOC[track_number - 1] & 0x00FFFFFF;
  }
}

block_struct *Cs2::AllocateBlock(unsigned char *blocknum) {
  // find a free block
  for(unsigned long i = 0; i < 200; i++)
  {
     if (block[i].size == -1)
     {
        blockfreespace--;

        if (blockfreespace <= 0) isbufferfull = true;

        block[i].size = getsectsize;

        *blocknum = i;
        return (block + i);
     }
  }

  isbufferfull = true;

  return NULL;
}

void Cs2::FreeBlock(block_struct *blk) {
  blk->size = -1;
  blockfreespace++;
  isbufferfull = false;
  reg.HIRQ &= ~CDB_HIRQ_BFUL;
}

void Cs2::SortBlocks(partition_struct *part) {
  unsigned long i, i2;
  block_struct *blktmp;
  unsigned char blktmp2;

  for (i = 0; i < (MAX_BLOCKS-1); i++)
  {
     for (i2 = i+1; i2 < MAX_BLOCKS; i2++)
     {
        if (part->block[i] == NULL && part->block[i2] != NULL)
        {
           blktmp = part->block[i];
           part->block[i] = part->block[i2];
           part->block[i2] = blktmp;
           blktmp2 = part->blocknum[i];
           part->blocknum[i] = part->blocknum[i2];
           part->blocknum[i2] = blktmp2;
        }
     }
  }
}

partition_struct *Cs2::GetPartition(filter_struct *curfilter)
{
  // go through various filter conditions here(fix me)

  return &partition[curfilter->condtrue];
}

partition_struct *Cs2::FilterData(filter_struct *curfilter, bool isaudio)
{
  bool condresults=true;
  partition_struct *fltpartition = NULL;

  // fix me, this is pretty bad. Though I guess it's a start

  for (;;)
  {
     // detect which type of sector we're dealing with
     // If it's not mode 2, ignore the subheader conditions
     if (workblock.data[0xF] == 0x02 && !isaudio)
     {
        // Mode 2
        // go through various subheader filter conditions here(fix me)
   
        if (curfilter->mode & 0x01)
        {
           // File Number Check
           if (workblock.fn != curfilter->fid)
              condresults = false;
        }

        if (curfilter->mode & 0x02)
        {
           // Channel Number Check
           if (workblock.cn != curfilter->chan)
              condresults = false;
        }

        if (curfilter->mode & 0x04)
        {
           // Sub Mode Check
           if ((workblock.sm & curfilter->smmask) != curfilter->smval)
              condresults = false;
        }

        if (curfilter->mode & 0x08)
        {
           // Coding Information Check
#if CDDEBUG
           fprintf(stderr, "cs2\t: FilterData: Coding Information Check. Coding Information = %02X. Filter's Coding Information Mask = %02X, Coding Information Value = %02X\n", workblock.ci, curfilter->cimask, curfilter->cival);
#endif
           if ((workblock.ci & curfilter->cimask) != curfilter->cival)
              condresults = false;
        }

        if (curfilter->mode & 0x10)
        {
           // Reverse Subheader Conditions
#if CDDEBUG
           fprintf(stderr, "cs2\t: FilterData: Reverse Subheader Conditions\n");
#endif
           condresults ^= true;
        }
     }

     if (curfilter->mode & 0x40)
     {
        // FAD Range Check
        if (workblock.FAD < curfilter->FAD ||
            workblock.FAD > (curfilter->FAD+curfilter->range))
            condresults = false;
     }

     if (condresults == true)
     {
        lastbuffer = curfilter->condtrue;
        fltpartition = &partition[curfilter->condtrue];
        break;
     }
     else
     {
        lastbuffer = curfilter->condfalse;

        if (curfilter->condfalse == 0xFF)
           return NULL;
        // loop and try filter that was connected to the false connector
        curfilter = &filter[curfilter->condfalse];
     }
  }

  // Allocate block
  fltpartition->block[fltpartition->numblocks] = AllocateBlock(&fltpartition->blocknum[fltpartition->numblocks]);

  if (fltpartition->block[fltpartition->numblocks] == NULL)
    return NULL;

  // Copy workblock settings to allocated block
  fltpartition->block[fltpartition->numblocks]->size = workblock.size;
  fltpartition->block[fltpartition->numblocks]->FAD = workblock.FAD;
  fltpartition->block[fltpartition->numblocks]->cn = workblock.cn;
  fltpartition->block[fltpartition->numblocks]->fn = workblock.fn;
  fltpartition->block[fltpartition->numblocks]->sm = workblock.sm;
  fltpartition->block[fltpartition->numblocks]->ci = workblock.ci;

  // convert raw sector to type specified in getsectsize
  switch(workblock.size)
  {
     case 2048: // user data only
                if (workblock.data[0xF] == 0x02)
                   // m2f1
                   memcpy(fltpartition->block[fltpartition->numblocks]->data,
                          workblock.data + 24, workblock.size);
                else
                   // m1
                   memcpy(fltpartition->block[fltpartition->numblocks]->data,
                             workblock.data + 16, workblock.size);
                break;
     case 2324: // m2f2 user data only
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                       workblock.data + 24, workblock.size);
                break;
     case 2336: // m2f2 skip sync+header data
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                workblock.data + 16, workblock.size);
                break;
     case 2340: // m2f2 skip sync data
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                workblock.data + 12, workblock.size);
                break;
     case 2352: // no conversion needed
                break;
     default: break;
  }

  // Modify Partition values
  if (fltpartition->size == -1) fltpartition->size = 0;
  fltpartition->size += fltpartition->block[fltpartition->numblocks]->size;
  fltpartition->numblocks++;

  return fltpartition;
}


int Cs2::CopyDirRecord(unsigned char *buffer, dirrec_struct *dirrec)
{
  unsigned char *temp_pointer;

  temp_pointer = buffer;

  memcpy(&dirrec->recordsize, buffer, sizeof(dirrec->recordsize));
  buffer += sizeof(dirrec->recordsize);

  memcpy(&dirrec->xarecordsize, buffer, sizeof(dirrec->xarecordsize));
  buffer += sizeof(dirrec->xarecordsize);

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->lba);
  memcpy(&dirrec->lba, buffer, sizeof(dirrec->lba));
  buffer += sizeof(dirrec->lba);
#else
  memcpy(&dirrec->lba, buffer, sizeof(dirrec->lba));
  buffer += (sizeof(dirrec->lba) * 2);
#endif

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->size);
  memcpy(&dirrec->size, buffer, sizeof(dirrec->size));
  buffer += sizeof(dirrec->size);
#else
  memcpy(&dirrec->size, buffer, sizeof(dirrec->size));
  buffer += (sizeof(dirrec->size) * 2);
#endif

  dirrec->dateyear = buffer[0];
  dirrec->datemonth = buffer[1];
  dirrec->dateday = buffer[2];
  dirrec->datehour = buffer[3];
  dirrec->dateminute = buffer[4];
  dirrec->datesecond = buffer[5];
  dirrec->gmtoffset = buffer[6];
  buffer += 7;

  dirrec->flags = buffer[0];
  buffer += sizeof(dirrec->flags);

  dirrec->fileunitsize = buffer[0];
  buffer += sizeof(dirrec->fileunitsize);

  dirrec->interleavegapsize = buffer[0];
  buffer += sizeof(dirrec->interleavegapsize);

#ifdef WORDS_BIGENDIAN
  buffer += sizeof(dirrec->volumesequencenumber);
  memcpy(&dirrec->volumesequencenumber, buffer, sizeof(dirrec->volumesequencenumber));
  buffer += sizeof(dirrec->volumesequencenumber);
#else
  memcpy(&dirrec->volumesequencenumber, buffer, sizeof(dirrec->volumesequencenumber));
  buffer += (sizeof(dirrec->volumesequencenumber) * 2);
#endif

  dirrec->namelength = buffer[0];
  buffer += sizeof(dirrec->namelength);

  memset(dirrec->name, 0, sizeof(dirrec->name));
  memcpy(dirrec->name, buffer, dirrec->namelength);
  buffer += dirrec->namelength;

  // handle padding
  buffer += (1 - dirrec->namelength % 2);

  memset(&dirrec->xarecord, 0, sizeof(dirrec->xarecord));

  // sadily, this is the best way I can think of for detecting XA records

  if ((dirrec->recordsize - (buffer - temp_pointer)) == 14)
  {
     memcpy(&dirrec->xarecord.groupid, buffer, sizeof(dirrec->xarecord.groupid));
     buffer += sizeof(dirrec->xarecord.groupid);

     memcpy(&dirrec->xarecord.userid, buffer, sizeof(dirrec->xarecord.userid));
     buffer += sizeof(dirrec->xarecord.userid);

     memcpy(&dirrec->xarecord.attributes, buffer, sizeof(dirrec->xarecord.attributes));
     buffer += sizeof(dirrec->xarecord.attributes);

#ifndef WORDS_BIGENDIAN
     // byte swap it
     dirrec->xarecord.attributes = ((dirrec->xarecord.attributes & 0xFF00) >> 8) +
                                   ((dirrec->xarecord.attributes & 0x00FF) << 8);
#endif

     memcpy(&dirrec->xarecord.signature, buffer, sizeof(dirrec->xarecord.signature));
     buffer += sizeof(dirrec->xarecord.signature);

     memcpy(&dirrec->xarecord.filenumber, buffer, sizeof(dirrec->xarecord.filenumber));
     buffer += sizeof(dirrec->xarecord.filenumber);

     memcpy(dirrec->xarecord.reserved, buffer, sizeof(dirrec->xarecord.reserved));
     buffer += sizeof(dirrec->xarecord.reserved);
  }

  return 0;
}

int Cs2::ReadFileSystem(filter_struct *curfilter, unsigned long fid, bool isoffset)
{
   unsigned long fid_offset=0;
   unsigned char *workbuffer;
   unsigned long i;
   dirrec_struct dirrec;
   unsigned char numsectorsleft=0;
   unsigned long curdirlba=0;
   partition_struct *rfspartition;
   unsigned long blocksectsize=getsectsize;
 
   outconcddev = curfilter;

   if (isoffset)
   {
      // readDirectory operation

      // make sure we have a valid current directory
      if (curdirsect == 0) return -1;

//      rfspartition = ReadUnFilteredSector(??);
#if CDDEBUG
      fprintf(stderr, "cs2\t: fix me: readDirectory not working\n");
#endif
      return -1;

      fid_offset = fid;
   }
   else
   {
      // changeDirectory operation

      if (fid == 0xFFFFFF)
      {
         // Figure out root directory's location

         // Read sector 16
         if ((rfspartition = ReadUnFilteredSector(166)) == NULL)
            return -2;

         blocksectsize = rfspartition->block[rfspartition->numblocks - 1]->size;

         // Retrieve directory record's lba
         CopyDirRecord(rfspartition->block[rfspartition->numblocks - 1]->data + 0x9C, &dirrec);

         // Free Block
         rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
         FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
         rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

         // Sort remaining blocks
         SortBlocks(rfspartition);
         rfspartition->numblocks -= 1;

         curdirlba = curdirsect = dirrec.lba;
         numsectorsleft = (dirrec.size / blocksectsize) - 1;
      }
      else
      {
         // Read in new directory record of specified directory
#if CDDEBUG
         fprintf(stderr, "cs2\t: fix me: only root directory supported for changeDirectory\n");
#endif
         return -1;
      }
   }

  // now read in first sector of directory record
  if ((rfspartition = ReadUnFilteredSector(curdirlba+150)) == NULL)
        return -2;

  curdirlba++;
  workbuffer = rfspartition->block[rfspartition->numblocks - 1]->data;

  // Fill in first two entries of fileinfo
  for (i = 0; i < 2; i++)
  {
     CopyDirRecord(workbuffer, fileinfo + i);
     fileinfo[i].lba += 150;
     workbuffer+=fileinfo[i].recordsize;

     if (workbuffer[0] == 0)
     {
        numfiles = i;
        break;
     }
  }

  // If fid_offset != 0, parse sector entries until we've found the fid that
  // matches fid_offset

  // implement me

  // Now generate the last 254 entries(the first two should've already been
  // generated earlier)
  for (i = 2; i < MAX_FILES; i++)
  {
     CopyDirRecord(workbuffer, fileinfo + i);
     fileinfo[i].lba += 150;
     workbuffer+=fileinfo[i].recordsize;

     if (workbuffer[0] == 0)
     {
        if (numsectorsleft > 0)
        {
           // Free previous read sector
           rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
           FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
           rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

           // Sort remaining blocks
           SortBlocks(rfspartition);
           rfspartition->numblocks -= 1;

           // Read in next sector of directory record
           if ((rfspartition = ReadUnFilteredSector(dirrec.lba+150)) == NULL)
              return -2;
           dirrec.lba++;
           numsectorsleft--;
           workbuffer = rfspartition->block[rfspartition->numblocks - 1]->data;
        }
        else
        {
           numfiles = i;
           break;
        }
     }
  }

  // Free the remaining sector
  rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
  FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
  rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

  // Sort remaining blocks
  SortBlocks(rfspartition);
  rfspartition->numblocks -= 1;

//#if CDDEBUG
//  for (i = 0; i < MAX_FILES; i++)
//  {
//     fprintf(stderr, "fileinfo[%d].name = %s\n", i, fileinfo[i].name);
//  }
//#endif

  return 0;
}

void Cs2::SetupFileInfoTransfer(unsigned long fid) {
  transfileinfo[0] = (fileinfo[fid].lba & 0xFF000000) >> 24;
  transfileinfo[1] = (fileinfo[fid].lba & 0x00FF0000) >> 16;
  transfileinfo[2] = (fileinfo[fid].lba & 0x0000FF00) >> 8;
  transfileinfo[3] =  fileinfo[fid].lba & 0x000000FF;

  transfileinfo[4] = (fileinfo[fid].size & 0xFF000000) >> 24;
  transfileinfo[5] = (fileinfo[fid].size & 0x00FF0000) >> 16;
  transfileinfo[6] = (fileinfo[fid].size & 0x0000FF00) >> 8;
  transfileinfo[7] =  fileinfo[fid].size & 0x000000FF;

  transfileinfo[8] = fileinfo[fid].interleavegapsize;
  transfileinfo[9] = fileinfo[fid].fileunitsize;
  transfileinfo[10] = (unsigned char)fid;
  transfileinfo[11] = fileinfo[fid].flags;
}

partition_struct *Cs2::ReadUnFilteredSector(unsigned long rufsFAD) {
  partition_struct *rufspartition;
  char syncheader[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0x00};

  if ((rufspartition = GetPartition(outconcddev)) != NULL && !isbufferfull)
  {
     // Allocate Block
     rufspartition->block[rufspartition->numblocks] = AllocateBlock(&rufspartition->blocknum[rufspartition->numblocks]);

     if (rufspartition->block[rufspartition->numblocks] == NULL)
        return NULL;

     // read a sector using cd interface function
     if (!cd->readSectorFAD(rufsFAD, workblock.data))
        return NULL;

     // convert raw sector to type specified in getsectsize
     switch(getsectsize)
     {
        case 2048: // user data only
                   if (workblock.data[0xF] == 0x02)
                   {
                      // is it form1/form2 data?
                      if (!(workblock.data[0x12] & 0x20))
                      {
                         // form 1
                         memcpy(rufspartition->block[rufspartition->numblocks]->data,
                                workblock.data + 24, 2048);
                         workblock.size = getsectsize;
                      }
                      else
                      {
                         // form 2
                         memcpy(rufspartition->block[rufspartition->numblocks]->data,
                                workblock.data + 24, 2324);
                         workblock.size = 2324;
                      }
                   }
                   else
                   {
                      memcpy(rufspartition->block[rufspartition->numblocks]->data,
                             workblock.data + 16, 2048);
                      workblock.size = getsectsize;
                   }
                   break;
        case 2336: // skip sync+header data
                   memcpy(rufspartition->block[rufspartition->numblocks]->data,
                   workblock.data + 16, 2336);
                   workblock.size = getsectsize;
                   break;
        case 2340: // skip sync data
                   memcpy(rufspartition->block[rufspartition->numblocks]->data,
                   workblock.data + 12, 2340);
                   workblock.size = getsectsize;
                   break;
        case 2352: // no conversion needed
                   workblock.size = getsectsize;
                   break;
        default: break;
     }

     // if mode 2 track, setup the subheader values
     if (memcmp(syncheader, workblock.data, 12) == 0 &&
         workblock.data[0xF] == 0x02)
     {
        rufspartition->block[rufspartition->numblocks]->fn = workblock.data[0x10];
        rufspartition->block[rufspartition->numblocks]->cn = workblock.data[0x11];
        rufspartition->block[rufspartition->numblocks]->sm = workblock.data[0x12];
        rufspartition->block[rufspartition->numblocks]->ci = workblock.data[0x13];
     }

     workblock.FAD = rufsFAD;

     // Modify Partition values
     if (rufspartition->size == -1) rufspartition->size = 0;
     rufspartition->size += rufspartition->block[rufspartition->numblocks]->size;
     rufspartition->numblocks++;

     return rufspartition;
  }

  return NULL;
}

partition_struct *Cs2::ReadFilteredSector(unsigned long rfsFAD) {
  char syncheader[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0x00};
  bool isaudio=false;

  if (outconcddev != NULL && !isbufferfull)
  {     
     // read a sector using cd interface function to workblock.data
     if (!cd->readSectorFAD(rfsFAD, workblock.data))
        return NULL;

     workblock.size = getsectsize;
     workblock.FAD = rfsFAD;

     if (memcmp(syncheader, workblock.data, 12) != 0) isaudio=true;

     // if mode 2 track, setup the subheader values
     if (!isaudio &&
         workblock.data[0xF] == 0x02)
     {
        // if it's form 2 data the sector size should be 2324
        if (workblock.data[0x12] & 0x20) workblock.size = 2324;

        workblock.fn = workblock.data[0x10];
        workblock.cn = workblock.data[0x11];
        workblock.sm = workblock.data[0x12];
        workblock.ci = workblock.data[0x13];
     }

     // pass workblock to filter function(after it identifies partition,
     // it should allocate the partition block, setup/change the partition
     // values, and copy workblock to the allocated block)
     return FilterData(outconcddev, isaudio);
  }

  return NULL;
}

unsigned char Cs2::GetRegionID() {
   partition_struct *gripartition;
   char ret=0;

   outconcddev = filter + 0;
   outconcddevnum = 0;

   // read in lba 0/FAD 150
   if ((gripartition = ReadUnFilteredSector(150)) != NULL)
   {
      // Make sure we're dealing with a saturn game
      if (memcmp(gripartition->block[gripartition->numblocks - 1]->data,
          "SEGA SEGASATURN", 15) == 0)
      {
         // read offset 0x40 and convert region character to number
         switch (gripartition->block[gripartition->numblocks - 1]->data[0x40])
         {
            case 'J':
                      ret = 1;
                      break;
            case 'T':
                      ret = 2;
                      break;
            case 'U':
                      ret = 4;
                      break;
            case 'B':
                      ret = 5;
                      break;
            case 'K':
                      ret = 6;
                      break;
            case 'A':
                      ret = 0xA;
                      break;
            case 'E':
                      ret = 0xC;
                      break;
            case 'L':
                      ret = 0xD;
                      break;
            default: break;
         }
      }

      // Free Block
      gripartition->size -= gripartition->block[gripartition->numblocks - 1]->size;
      FreeBlock(gripartition->block[gripartition->numblocks - 1]);
      gripartition->blocknum[gripartition->numblocks - 1] = 0xFF;

      // Sort remaining blocks
      SortBlocks(gripartition);
      gripartition->numblocks -= 1;
   }

   return ret;
}

int Cs2::saveState(FILE *fp) {
   int offset, i;
   unsigned char tempbyte;

   // This is mostly kludge, but it will have to do until I have time to rewrite it all

   offset = stateWriteHeader(fp, "CS2 ", 1);

   // Write cart type
   fwrite((void *)&carttype, 4, 1, fp);

   // Write cd block registers
   fwrite((void *)&reg, sizeof(blockregs_struct), 1, fp);

   // Write current Status variables(needs a rewrite)
   fwrite((void *)&FAD, 4, 1, fp);
   fwrite((void *)&status, 1, 1, fp);
   fwrite((void *)&options, 1, 1, fp);
   fwrite((void *)&repcnt, 1, 1, fp);
   fwrite((void *)&ctrladdr, 1, 1, fp);
   fwrite((void *)&track, 1, 1, fp);
   fwrite((void *)&index, 1, 1, fp);

   // Write other cd block internal variables
   fwrite((void *)&satauth, 2, 1, fp);
   fwrite((void *)&mpgauth, 2, 1, fp);
   fwrite((void *)&transfercount, 4, 1, fp);
   fwrite((void *)&cdwnum, 4, 1, fp);
   fwrite((void *)TOC, 4, 102, fp);
   fwrite((void *)&playFAD, 4, 1, fp);
   fwrite((void *)&playendFAD, 4, 1, fp);
   fwrite((void *)&getsectsize, 4, 1, fp);
   fwrite((void *)&putsectsize, 4, 1, fp);
   fwrite((void *)&calcsize, 4, 1, fp);
   fwrite((void *)&infotranstype, 4, 1, fp);
   fwrite((void *)&datatranstype, 4, 1, fp);
   fwrite((void *)&isonesectorstored, 1, 1, fp);
   fwrite((void *)&isdiskchanged, 1, 1, fp);
   fwrite((void *)&isbufferfull, 1, 1, fp);
   fwrite((void *)&speed1x, 1, 1, fp);
   fwrite((void *)&transfileinfo, 1, 12, fp);
   fwrite((void *)&lastbuffer, 1, 1, fp);
   fwrite((void *)&_command, 1, 1, fp);
   fwrite((void *)&_periodictiming, 4, 1, fp);
   fwrite((void *)&_commandtiming, 4, 1, fp);
   fwrite((void *)&outconcddevnum, 1, 1, fp);
   fwrite((void *)&outconmpegfbnum, 1, 1, fp);
   fwrite((void *)&outconmpegbufnum, 1, 1, fp);
   fwrite((void *)&outconmpegromnum, 1, 1, fp);
   fwrite((void *)&outconhostnum, 1, 1, fp);
   fwrite((void *)&datatranspartitionnum, 1, 1, fp);
   fwrite((void *)&datatransoffset, 4, 1, fp);
   fwrite((void *)&datanumsecttrans, 4, 1, fp);
   fwrite((void *)&datatranssectpos, 2, 1, fp);
   fwrite((void *)&datasectstotrans, 2, 1, fp);
   fwrite((void *)&blockfreespace, 4, 1, fp);
   fwrite((void *)&curdirsect, 4, 1, fp);

   // Write CD buffer
   fwrite((void *)block, sizeof(block_struct), MAX_BLOCKS, fp);

   // Write partition data
   for (i = 0; i < MAX_SELECTORS; i++)
   {
      fwrite((void *)&partition[i].size, 4, 1, fp);
      fwrite((void *)partition[i].blocknum, 1, MAX_BLOCKS, fp);
      fwrite((void *)&partition[i].numblocks, 1, 1, fp);
   }

   // Write filter data
   fwrite((void *)filter, sizeof(filter_struct), MAX_SELECTORS, fp);

   // Write File Info Table
   fwrite((void *)fileinfo, sizeof(dirrec_struct), MAX_FILES, fp);

   // Write MPEG card registers here

   // Write current MPEG card status variables
   fwrite((void *)&actionstatus, 1, 1, fp);
   fwrite((void *)&pictureinfo, 1, 1, fp);
   fwrite((void *)&mpegaudiostatus, 1, 1, fp);
   fwrite((void *)&mpegvideostatus, 2, 1, fp);
   fwrite((void *)&vcounter, 2, 1, fp);

   // Write other MPEG card internal variables
   fwrite((void *)&mpegintmask, 4, 1, fp);
   fwrite((void *)mpegcon, sizeof(mpegcon_struct), 2, fp);
   fwrite((void *)mpegstm, sizeof(mpegstm_struct), 2, fp);

   return stateFinishHeader(fp, offset);
}

int Cs2::loadState(FILE *fp, int version, int size) {
   int i, i2;

   // This is mostly kludge, but it will have to do until I have time to rewrite it all

   // Read cart type
   fread((void *)&carttype, 4, 1, fp);

   // Read cd block registers
   fread((void *)&reg, sizeof(blockregs_struct), 1, fp);

   // Read current Status variables(needs a reRead)
   fread((void *)&FAD, 4, 1, fp);
   fread((void *)&status, 1, 1, fp);
   fread((void *)&options, 1, 1, fp);
   fread((void *)&repcnt, 1, 1, fp);
   fread((void *)&ctrladdr, 1, 1, fp);
   fread((void *)&track, 1, 1, fp);
   fread((void *)&index, 1, 1, fp);

   // Read other cd block internal variables
   fread((void *)&satauth, 2, 1, fp);
   fread((void *)&mpgauth, 2, 1, fp);
   fread((void *)&transfercount, 4, 1, fp);
   fread((void *)&cdwnum, 4, 1, fp);
   fread((void *)TOC, 4, 102, fp);
   fread((void *)&playFAD, 4, 1, fp);
   fread((void *)&playendFAD, 4, 1, fp);
   fread((void *)&getsectsize, 4, 1, fp);
   fread((void *)&putsectsize, 4, 1, fp);
   fread((void *)&calcsize, 4, 1, fp);
   fread((void *)&infotranstype, 4, 1, fp);
   fread((void *)&datatranstype, 4, 1, fp);
   fread((void *)&isonesectorstored, 1, 1, fp);
   fread((void *)&isdiskchanged, 1, 1, fp);
   fread((void *)&isbufferfull, 1, 1, fp);
   fread((void *)&speed1x, 1, 1, fp);
   fread((void *)&transfileinfo, 1, 12, fp);
   fread((void *)&lastbuffer, 1, 1, fp);
   fread((void *)&_command, 1, 1, fp);
   fread((void *)&_periodictiming, 4, 1, fp);
   fread((void *)&_commandtiming, 4, 1, fp);
   fread((void *)&outconcddevnum, 1, 1, fp);
   if (outconcddevnum == 0xFF)
      outconcddev = NULL;
   else
      outconcddev = filter + outconcddevnum;

   fread((void *)&outconmpegfbnum, 1, 1, fp);
   if (outconmpegfbnum == 0xFF)
      outconmpegfb = NULL;
   else
      outconmpegfb = filter + outconmpegfbnum;

   fread((void *)&outconmpegbufnum, 1, 1, fp);
   if (outconmpegbufnum == 0xFF)
      outconmpegbuf = NULL;
   else
      outconmpegbuf = filter + outconmpegbufnum;

   fread((void *)&outconmpegromnum, 1, 1, fp);
   if (outconmpegromnum == 0xFF)
      outconmpegrom = NULL;
   else
      outconmpegrom = filter + outconmpegromnum;

   fread((void *)&outconhostnum, 1, 1, fp);
   if (outconhostnum == 0xFF)
      outconhost = NULL;
   else
      outconhost = filter + outconhostnum;

   fread((void *)&datatranspartitionnum, 1, 1, fp);
   fread((void *)&datatransoffset, 4, 1, fp);
   fread((void *)&datanumsecttrans, 4, 1, fp);
   fread((void *)&datatranssectpos, 2, 1, fp);
   fread((void *)&datasectstotrans, 2, 1, fp);
   fread((void *)&blockfreespace, 4, 1, fp);
   fread((void *)&curdirsect, 4, 1, fp);

   // Read CD buffer
   fread((void *)block, sizeof(block_struct), MAX_BLOCKS, fp);

   // Read partition data
   for (i = 0; i < MAX_SELECTORS; i++)
   {
      fread((void *)&partition[i].size, 4, 1, fp);
      fread((void *)partition[i].blocknum, 1, MAX_BLOCKS, fp);
      fread((void *)&partition[i].numblocks, 1, 1, fp);

      for (i2 = 0; i2 < MAX_BLOCKS; i2++)
      {
         if (partition[i].blocknum[i2] == 0xFF)
            partition[i].block[i2] = NULL;
         else
            partition[i].block[i2] = block + partition[i].blocknum[i2];
      }
   }

   // Read filter data
   fread((void *)filter, sizeof(filter_struct), MAX_SELECTORS, fp);

   // Read File Info Table
   fread((void *)fileinfo, sizeof(dirrec_struct), MAX_FILES, fp);

   // Read MPEG card registers here

   // Read current MPEG card status variables
   fread((void *)&actionstatus, 1, 1, fp);
   fread((void *)&pictureinfo, 1, 1, fp);
   fread((void *)&mpegaudiostatus, 1, 1, fp);
   fread((void *)&mpegvideostatus, 2, 1, fp);
   fread((void *)&vcounter, 2, 1, fp);

   // Read other MPEG card internal variables
   fread((void *)&mpegintmask, 4, 1, fp);
   fread((void *)mpegcon, sizeof(mpegcon_struct), 2, fp);
   fread((void *)mpegstm, sizeof(mpegstm_struct), 2, fp);

   return size;
}

