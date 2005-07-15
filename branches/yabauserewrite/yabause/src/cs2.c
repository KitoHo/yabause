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

#include <stdlib.h>
#include "cs2.h"
#include "yui.h"

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
  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF); \
  Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | Cs2Area->track; \
  Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) & 0xFF); \
  Cs2Area->reg.CR4 = (u16) Cs2Area->FAD; 

#define doMPEGReport() \
  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->actionstatus; \
  Cs2Area->reg.CR2 = Cs2Area->vcounter; \
  Cs2Area->reg.CR3 = (Cs2Area->pictureinfo << 8) | Cs2Area->mpegaudiostatus; \
  Cs2Area->reg.CR4 = Cs2Area->mpegvideostatus; 

Cs2 * Cs2Area;

extern CDInterface *CDCoreList[];

FASTCALL u8 Cs2GetByte(u32 addr) {
  if(Cs2Area->carttype == CART_NETLINK) {
/*
     switch (addr) {
        case 0x95019: // Modem Status Register
           // Send interrupt here?
           return T3ReadByte(Cs2Area->mem, addr);
        case 0x9501D: // Scratch
           return T3ReadByte(Cs2Area->mem, addr);
        default:
           return T3ReadByte(Cs2Area->mem, addr);

     }
*/
  }

  return 0xFF; // only netlink seems to use byte-access
}

FASTCALL void Cs2WriteByte(u32 addr, u8 val) {
  if(Cs2Area->carttype == CART_NETLINK) {
/*
     switch (addr) {
        case 0x2503D: // ???
           T3WriteByte(Cs2Area->mem, addr, val);
           break;
        case 0x95019: // Modem Status Register(read-only)
           break;
        case 0x9501D: // Scratch
           T3WriteByte(Cs2Area->mem, addr, val);
           break;
        default:
           break;
      }
*/
   }
   else
   {
       // only netlink seems to use byte-access
//      Memory::setByte(addr, val);
   }
}

FASTCALL u16 Cs2ReadWord(u32 addr) {
  u16 val = 0;

  switch(addr) {
    case 0x90008:
    case 0x9000A:
                  val = Cs2Area->reg.HIRQ;

                  if (Cs2Area->isbufferfull)
                    val |= CDB_HIRQ_BFUL;
                  else
                    val &= ~CDB_HIRQ_BFUL;

                  if (Cs2Area->isdiskchanged)
                    val |= CDB_HIRQ_DCHG;
                  else
                    val &= ~CDB_HIRQ_DCHG;

                  if (Cs2Area->isonesectorstored)
                    val |= CDB_HIRQ_CSCT;
                  else
                    val &= ~CDB_HIRQ_CSCT;

                  Cs2Area->reg.HIRQ = val;

                  val &= Cs2Area->reg.HIRQMASK;
//#if CDDEBUG
//                  fprintf(stderr, "cs2\t: Hirq read, Hirq mask = %x - ret: %x\n", Memory::getWord(0x9000C), val);
//#endif
                  return val;
    case 0x9000C: 
    case 0x9000E: return Cs2Area->reg.HIRQMASK;
    case 0x90018:
    case 0x9001A: return Cs2Area->reg.CR1;
    case 0x9001C:
    case 0x9001E: return Cs2Area->reg.CR2;
    case 0x90020:
    case 0x90022: return Cs2Area->reg.CR3;
    case 0x90024:
    case 0x90026: Cs2Area->_command = 0;
                  return Cs2Area->reg.CR4;
    case 0x90028:
    case 0x9002A: return Cs2Area->reg.MPEGRGB;
    case 0x98000:
                  // transfer info
                  switch (Cs2Area->infotranstype) {
                     case 0:
                             // Get Toc Data
                             if (Cs2Area->transfercount % 4 == 0)
                                val = (Cs2Area->TOC[Cs2Area->transfercount >> 2] & 0xFFFF0000) >> 16;
                             else
                                val = Cs2Area->TOC[Cs2Area->transfercount >> 2] & 0x0000FFFF;

                             Cs2Area->transfercount += 2;
                             Cs2Area->cdwnum += 2;

                             if (Cs2Area->transfercount > (0xCC * 2))
                             {
                                Cs2Area->transfercount = 0;
                                Cs2Area->infotranstype = -1;
                             }
                             break;
                     case 1:
                             // Get File Info(1 file info)
                             val = (Cs2Area->transfileinfo[Cs2Area->transfercount] << 8) |
                                    Cs2Area->transfileinfo[Cs2Area->transfercount + 1];
                             Cs2Area->transfercount += 2;
                             Cs2Area->cdwnum += 2;

                             if (Cs2Area->transfercount > (0x6 * 2))
                             {
                                Cs2Area->transfercount = 0;
                                Cs2Area->infotranstype = -1;
                             }

                             break;
                     case 2:
                             // Get File Info(254 file info)

                             // Do we need to retrieve the next file info?
                             if (Cs2Area->transfercount % (0x6 * 2) == 0) {
                               // yes we do
                               Cs2SetupFileInfoTransfer(2 + (Cs2Area->transfercount / (0x6 * 2)));
                             }

                             val = (Cs2Area->transfileinfo[Cs2Area->transfercount % (0x6 * 2)] << 8) |
                                    Cs2Area->transfileinfo[Cs2Area->transfercount % (0x6 * 2) + 1];

                             Cs2Area->transfercount += 2;
                             Cs2Area->cdwnum += 2;

                             if (Cs2Area->transfercount > (254 * (0x6 * 2)))
                             {
                                Cs2Area->transfercount = 0;
                                Cs2Area->infotranstype = -1;
                             }

                             break;
                     default: break;
                  }
                  break;
    default:
#if DEBUG
             cerr << "cs2\t: Undocumented register read " << hex << addr << endl;
#endif
//             val = T3ReadWord(Cs2Area->mem, addr);
             break;
  }

  return val;
}

FASTCALL void Cs2WriteWord(u32 addr, u16 val) {
  switch(addr) {
    case 0x90008:
    case 0x9000A:
                  Cs2Area->reg.HIRQ &= val;
                  return;
    case 0x9000C: 
    case 0x9000E: Cs2Area->reg.HIRQMASK = val;
                  return;
    case 0x90018: 
    case 0x9001A: Cs2Area->status &= ~CDB_STAT_PERI;
                  Cs2Area->_command = 1;
                  Cs2Area->reg.CR1 = val;
                  return;
    case 0x9001C:
    case 0x9001E: Cs2Area->reg.CR2 = val;
                  return;
    case 0x90020:
    case 0x90022: Cs2Area->reg.CR3 = val;
                  return;
    case 0x90024:
    case 0x90026: Cs2Area->reg.CR4 = val;
                  Cs2SetCommandTiming(Cs2Area->reg.CR1 >> 8);
                  return;
    case 0x90028:
    case 0x9002A: Cs2Area->reg.MPEGRGB = val;
                  return;
    default:
#if DEBUG
             cerr << "cs2\t:Undocumented register write " << hex << addr << endl;
#endif
//                  T3WriteWord(Cs2Area->mem, addr, val);
                  break;
  }
}

FASTCALL u32 Cs2ReadLong(u32 addr) {
  long i;
  u32 val = 0;

  switch(addr) {
    case 0x90008:
                  val = Cs2Area->reg.HIRQ;

                  if (Cs2Area->isbufferfull)
                    val |= CDB_HIRQ_BFUL;
                  else
                    val &= ~CDB_HIRQ_BFUL;

                  if (Cs2Area->isdiskchanged)
                    val |= CDB_HIRQ_DCHG;
                  else
                    val &= ~CDB_HIRQ_DCHG;

                  if (Cs2Area->isonesectorstored)
                    val |= CDB_HIRQ_CSCT;
                  else
                    val &= ~CDB_HIRQ_CSCT;

                  Cs2Area->reg.HIRQ = val;

                  val &= Cs2Area->reg.HIRQMASK;
                  val |= (val << 16);
                  return val;
    case 0x9000C: return ((Cs2Area->reg.HIRQMASK << 16) | Cs2Area->reg.HIRQMASK);
    case 0x90018: return ((Cs2Area->reg.CR1 << 16) | Cs2Area->reg.CR1);
    case 0x9001C: return ((Cs2Area->reg.CR2 << 16) | Cs2Area->reg.CR2);
    case 0x90020: return ((Cs2Area->reg.CR3 << 16) | Cs2Area->reg.CR3);
    case 0x90024: Cs2Area->_command = 0;
                  return ((Cs2Area->reg.CR4 << 16) | Cs2Area->reg.CR4);
    case 0x90028: return ((Cs2Area->reg.MPEGRGB << 16) | Cs2Area->reg.MPEGRGB);
    case 0x18000:
                  // transfer data
                  switch (Cs2Area->datatranstype) {
                    case 0:
                            // get sector

                            // Make sure we still have sectors to transfer
                            if (Cs2Area->datanumsecttrans < Cs2Area->datasectstotrans)
                            {
                               // Transfer Data
                               val = (Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset] << 24) +
                                     (Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 1] << 16) +
                                     (Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 2] << 8) +
                                      Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 3];

                               // increment datatransoffset/cdwnum
                               Cs2Area->cdwnum += 4;
                               Cs2Area->datatransoffset += 4;

                               // Make sure we're not beyond the sector size boundry
                               if (Cs2Area->datatransoffset >= Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->size)
                               {
                                  Cs2Area->datatransoffset = 0;
                                  Cs2Area->datanumsecttrans++;
                               }
                            }

                            break;
                    case 2:
                            // get then delete sector

                            // Make sure we still have sectors to transfer
                            if (Cs2Area->datanumsecttrans < Cs2Area->datasectstotrans)
                            {
                               // Transfer Data
                               val = (Cs2Area->datatranspartition->block[Cs2Area->datatranssectpos+Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset] << 24) +
                                     (Cs2Area->datatranspartition->block[Cs2Area->datatranssectpos+Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 1] << 16) +
                                     (Cs2Area->datatranspartition->block[Cs2Area->datatranssectpos+Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 2] << 8) +
                                      Cs2Area->datatranspartition->block[Cs2Area->datatranssectpos+Cs2Area->datanumsecttrans]->data[Cs2Area->datatransoffset + 3];

                               // increment datatransoffset/cdwnum
                               Cs2Area->cdwnum += 4;
                               Cs2Area->datatransoffset += 4;

                               // Make sure we're not beyond the sector size boundry
                               if (Cs2Area->datatransoffset >= Cs2Area->datatranspartition->block[Cs2Area->datanumsecttrans]->size)
                               {
                                  Cs2Area->datatransoffset = 0;
                                  Cs2Area->datanumsecttrans++;
                               }
                            }
                            else
                            {
                               // Ok, so we don't have any more sectors to
                               // transfer, might as well delete them all.

                               Cs2Area->datatranstype = -1;

                               // free blocks
                               for (i = Cs2Area->datatranssectpos; i < (Cs2Area->datatranssectpos+Cs2Area->datasectstotrans); i++)
                               {
                                  Cs2FreeBlock(Cs2Area->datatranspartition->block[i]);
                                  Cs2Area->datatranspartition->block[i] = NULL;
                                  Cs2Area->datatranspartition->blocknum[i] = 0xFF;
                               }

                               // sort remaining blocks
                               Cs2SortBlocks(Cs2Area->datatranspartition);

                               Cs2Area->datatranspartition->size -= Cs2Area->cdwnum;
                               Cs2Area->datatranspartition->numblocks -= Cs2Area->datasectstotrans;

#if CDDEBUG
                               fprintf(stderr, "cs2\t: datatranspartition->size = %x\n", Cs2Area->datatranspartition->size);
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
//             val = T3ReadLong(Cs2Area->mem, addr);
             break;
  }

  return val;
}

FASTCALL void Cs2WriteLong(u32 addr, u32 val) {
#if CDDEBUG
   fprintf(stderr, "cs2\t: Long writing isn't implemented\n");
#endif
//   T3WriteLong(Cs2Area->mem, addr, val);
}

int Cs2Init(int carttype, int coreid, const char *cdrompath, const char *mpegpath) {
   int i;
   if ((Cs2Area = (Cs2 *) malloc(sizeof(Cs2))) == NULL)
      return -1;

   Cs2Area->carttype = carttype;
   Cs2Area->mpegpath = mpegpath;

   // So which core do we want?
   if (coreid == CDCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; CDCoreList[i] != NULL; i++)
   {
      if (CDCoreList[i]->id == coreid)
      {
         // Set to current core
         Cs2Area->cdi = CDCoreList[i];
         break;
      }
   }

   Cs2Reset();

   // If Modem is connected, set the registers
   if(Cs2Area->carttype == CART_NETLINK)
   {
      Cs2Area->nlreg.RBR = 0x00;
//     Cs2Area->nlreg.THR = 0x??; // have no idea
      Cs2Area->nlreg.IER = 0x00;
//     Cs2Area->nlreg.DLL = 0x??; // need to check
//     Cs2Area->nlreg.DLM = 0x??; // need to check
     Cs2Area->nlreg.IIR = 0x01;
//      Cs2Area->nlreg.FCR = 0x??; // have no idea
      Cs2Area->nlreg.LCR = 0x00;
      Cs2Area->nlreg.MCR = 0x00;
      Cs2Area->nlreg.LSR = 0x60;
      Cs2Area->nlreg.MSR = 0x30;
      Cs2Area->nlreg.SCR = 0x01;
   }

   return 0;
}

void Cs2DeInit(void) {
//   if (Cs2Area->cdi != NULL) {
//      Cs2Area->cdi->DeInit();
//   }
//   free(Cs2Area->cdi);
//   T3MemoryDelete(Cs2Area->mem);
   free(Cs2Area);
}

void Cs2Reset(void) {
  unsigned long i, i2;

  switch (Cs2Area->cdi->GetStatus())
  {
     case 0:   
     case 1:
             Cs2Area->status = CDB_STAT_PAUSE;
             Cs2Area->FAD = 150;
             Cs2Area->options = 0;
             Cs2Area->repcnt = 0;
             Cs2Area->ctrladdr = 0x41;
             Cs2Area->track = 1;
             Cs2Area->index = 1;
             break;
     case 2:
             Cs2Area->status = CDB_STAT_NODISC;

             Cs2Area->FAD = 0xFFFFFFFF;
             Cs2Area->options = 0xFF;
             Cs2Area->repcnt = 0xFF;
             Cs2Area->ctrladdr = 0xFF;
             Cs2Area->track = 0xFF;
             Cs2Area->index = 0xFF;
             break;
     case 3:
             Cs2Area->status = CDB_STAT_OPEN;

             Cs2Area->FAD = 0xFFFFFFFF;
             Cs2Area->options = 0xFF;
             Cs2Area->repcnt = 0xFF;
             Cs2Area->ctrladdr = 0xFF;
             Cs2Area->track = 0xFF;
             Cs2Area->index = 0xFF;
             break;
     default: break;
  }

  Cs2Area->infotranstype = -1;
  Cs2Area->datatranstype = -1;
  Cs2Area->transfercount = 0;
  Cs2Area->cdwnum = 0;
  Cs2Area->getsectsize = Cs2Area->putsectsize = 2048;
  Cs2Area->isdiskchanged = 1;
  Cs2Area->isbufferfull = 0;
  Cs2Area->isonesectorstored = 0;

  Cs2Area->reg.CR1 = ( 0 <<8) | 'C';
  Cs2Area->reg.CR2 = ('D'<<8) | 'B';
  Cs2Area->reg.CR3 = ('L'<<8) | 'O';
  Cs2Area->reg.CR4 = ('C'<<8) | 'K';
  Cs2Area->reg.HIRQ = 0xFFFF;
  Cs2Area->reg.HIRQMASK = 0xFFFF;

  Cs2Area->playFAD = 0xFFFFFFFF;
  Cs2Area->playendFAD = 0xFFFFFFFF;

  // set authentication variables to 0(not authenticated)
  Cs2Area->satauth = 0;
  Cs2Area->mpgauth = 0;

  // clear filter conditions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     Cs2Area->filter[i].FAD = 0;
     Cs2Area->filter[i].range = 0xFFFFFFFF;
     Cs2Area->filter[i].mode = 0;
     Cs2Area->filter[i].chan = 0;
     Cs2Area->filter[i].smmask = 0;
     Cs2Area->filter[i].cimask = 0;
     Cs2Area->filter[i].fid = 0;
     Cs2Area->filter[i].smval = 0;
     Cs2Area->filter[i].cival = 0;
     Cs2Area->filter[i].condtrue = 0;
     Cs2Area->filter[i].condfalse = 0xFF;
  }

  // clear partitions
  for (i = 0; i < MAX_SELECTORS; i++)
  {
     Cs2Area->partition[i].size = -1;
     Cs2Area->partition[i].numblocks = 0;

     for (i2 = 0; i2 < MAX_BLOCKS; i2++)
     {
        Cs2Area->partition[i].block[i2] = NULL;
        Cs2Area->partition[i].blocknum[i2] = 0xFF;
     }
  }

  // clear blocks
  for (i = 0; i < MAX_BLOCKS; i++)
  {
     Cs2Area->block[i].size = -1;
     memset(Cs2Area->block[i].data, 0, 2352);
  }

  Cs2Area->blockfreespace = 200;

  // initialize TOC
  memset(Cs2Area->TOC, 0xFF, sizeof(Cs2Area->TOC));

  // clear filesystem stuff
  Cs2Area->curdirsect = 0;
  memset(&Cs2Area->fileinfo, 0, sizeof(Cs2Area->fileinfo));
  Cs2Area->numfiles = 0;

  Cs2Area->lastbuffer = 0xFF;

  Cs2Area->_command = 0;
  Cs2Area->_periodiccycles = 0;
  Cs2Area->_commandtiming = 0;
  Cs2SetTiming(0);

  // MPEG specific stuff
  Cs2Area->mpegcon[0].audcon = Cs2Area->mpegcon[0].vidcon = 0x00;
  Cs2Area->mpegcon[0].audlay = Cs2Area->mpegcon[0].vidlay = 0x00;
  Cs2Area->mpegcon[0].audbufdivnum = Cs2Area->mpegcon[0].vidbufdivnum = 0xFF;
  Cs2Area->mpegcon[1].audcon = Cs2Area->mpegcon[1].vidcon = 0x00;
  Cs2Area->mpegcon[1].audlay = Cs2Area->mpegcon[1].vidlay = 0x00;
  Cs2Area->mpegcon[1].audbufdivnum = Cs2Area->mpegcon[1].vidbufdivnum = 0xFF;

  // should verify the following
  Cs2Area->mpegstm[0].audstm = Cs2Area->mpegstm[0].vidstm = 0x00; 
  Cs2Area->mpegstm[0].audstmid = Cs2Area->mpegstm[0].vidstmid = 0x00; 
  Cs2Area->mpegstm[0].audchannum = Cs2Area->mpegstm[0].vidchannum = 0x00; 
  Cs2Area->mpegstm[1].audstm = Cs2Area->mpegstm[1].vidstm = 0x00;
  Cs2Area->mpegstm[1].audstmid = Cs2Area->mpegstm[1].vidstmid = 0x00; 
  Cs2Area->mpegstm[1].audchannum = Cs2Area->mpegstm[1].vidchannum = 0x00;
}

void Cs2Run(unsigned long timing) {
    Cs2Area->_periodiccycles += timing;

    if (Cs2Area->_commandtiming > 0) {
       if (Cs2Area->_commandtiming < timing)
       {
          Cs2Execute();
          Cs2Area->_commandtiming = 0;
       }
       else
          Cs2Area->_commandtiming -= timing;
    }

    if (Cs2Area->_periodiccycles >= Cs2Area->_periodictiming)
    {
      Cs2Area->_periodiccycles -= Cs2Area->_periodictiming; 

      // Get Drive's current status and compare with old status
//      switch(cd->getStatus()) // this shouldn't be called every periodic response
      switch(0)
      {
         case 0:
         case 1:
                 if ((Cs2Area->status & 0xF) == CDB_STAT_NODISC ||
                     (Cs2Area->status & 0xF) == CDB_STAT_OPEN)
                 {
                    Cs2Area->status = CDB_STAT_PAUSE;
                    Cs2Area->isdiskchanged = 1;
                 }
                 break;
         case 2:
                 // may need to change this
                 if ((Cs2Area->status & 0xF) != CDB_STAT_NODISC)
                    Cs2Area->status = CDB_STAT_NODISC;
                 break;
         case 3:
                 // may need to change this
                 if ((Cs2Area->status & 0xF) != CDB_STAT_OPEN)
                    Cs2Area->status = CDB_STAT_OPEN;
                 break;
         default: break;
      }

      if (Cs2Area->_command) {
         return;
      }

      switch (Cs2Area->status & 0xF) {
         case CDB_STAT_PAUSE:
         {
//            if (FAD >= playFAD && FAD < playendFAD)
//               status = CDB_STAT_PLAY;
//            else
               break;
         }
         case CDB_STAT_PLAY:
         {
            partition_struct * playpartition;
            playpartition = Cs2ReadFilteredSector(Cs2Area->FAD);

            if (playpartition != NULL)
            {
               Cs2Area->FAD++;
#if CDDEBUG
               fprintf(stderr, "blocks = %d blockfreespace = %d fad = %x playpartition->size = %x isbufferfull = %x\n", Cs2Area->playpartition->numblocks, Cs2Area->blockfreespace, Cs2Area->FAD, Cs2Area->playpartition->size, Cs2Area->isbufferfull);
#endif
               Cs2Area->reg.HIRQ |= CDB_HIRQ_CSCT;
               Cs2Area->isonesectorstored = 1;

               if (Cs2Area->FAD >= Cs2Area->playendFAD) {
                  // we're done
                  Cs2Area->status = CDB_STAT_PAUSE;
                  Cs2SetTiming(0);
                  Cs2Area->reg.HIRQ |= CDB_HIRQ_PEND;

#if CDDEBUG
                  fprintf(stderr, "PLAY HAS ENDED\n");
#endif
               }
               if (Cs2Area->isbufferfull) {
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

      Cs2Area->status |= CDB_STAT_PERI;

      // adjust registers appropriately here(fix me)
      doCDReport();
      Cs2Area->reg.HIRQ |= CDB_HIRQ_SCDQ;
    }
}

void Cs2Command(void) {
  Cs2Area->_command = 1;
}

void Cs2SetTiming(int playing) {
  if (playing) {
     if (Cs2Area->speed1x == 0) // should also verify to make sure it's not reading cd audio
        Cs2Area->_periodictiming = 13300;
     else
        Cs2Area->_periodictiming = 6700;
  }
  else {
     Cs2Area->_periodictiming = 16700;
  }
}

void Cs2SetCommandTiming(unsigned char cmd) {
   switch(cmd) {
      default:
               Cs2Area->_commandtiming = 1;
               break;
   }
}

void Cs2Execute(void) {
  Cs2Area->reg.HIRQ &= ~CDB_HIRQ_CMOK;

  u16 instruction = Cs2Area->reg.CR1 >> 8;

  switch (instruction) {
    case 0x00:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getStatus\n");
#endif
      Cs2GetStatus();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x01:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getHardwareInfo\n");
#endif
      Cs2GetHardwareInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x02:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getToc\n");
#endif
      Cs2GetToc();
      break;
    case 0x03:
    {
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSessionInfo\n");
#endif
       Cs2GetSessionInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
       break;
    }
    case 0x04:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: initializeCDSystem %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2InitializeCDSystem();
      break;
    case 0x06:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: endDataTransfer %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2EndDataTransfer();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x10:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: playDisc %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2PlayDisc();
      break;
    case 0x11:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: seekDisc %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2SeekDisc();
      break;
    case 0x20:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSubcodeQRW %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetSubcodeQRW();
      break;
    case 0x30:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setCDDeviceConnection %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2SetCDDeviceConnection();
      break;
    case 0x32:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getLastBufferDestination %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetLastBufferDestination();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x40:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterRange\n");
#endif
      Cs2SetFilterRange();
      break;
    case 0x42:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterSubheaderConditions\n");
#endif
      Cs2SetFilterSubheaderConditions();
      break;
    case 0x43:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFilterSubheaderConditions\n");
#endif
      Cs2GetFilterSubheaderConditions();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x44:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterMode\n");
#endif
      Cs2SetFilterMode();
      break;
    case 0x45:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFilterMode\n");
#endif
      Cs2GetFilterMode();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x46:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setFilterConnection\n");
#endif
      Cs2SetFilterConnection();
      break;
    case 0x48:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: resetSelector %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2ResetSelector();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x50:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getBufferSize\n");
#endif
      Cs2GetBufferSize();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x51:
//#if CDDEBUG
//      fprintf(stderr, "cs2\t: Command: getSectorNumber %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
//#endif
      Cs2GetSectorNumber();

//#if CDDEBUG
//      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
//#endif
      break;
    case 0x52:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: calculateActualSize %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2CalculateActualSize();

      break;
    case 0x53:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getActualSize\n");
#endif
      Cs2GetActualSize();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x54:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSectorInfo %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetSectorInfo();

#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x60:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: setSectorLength %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2SetSectorLength();
      break;
    case 0x61:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getSectorData %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x62:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: deleteSectorData %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2DeleteSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x63:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getThenDeleteSectorData %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetThenDeleteSectorData();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x64:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: putSectorData %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2PutSectorData();
      break;
    case 0x67:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getCopyError\n");
#endif
      Cs2GetCopyError();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x70:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: changeDirectory %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2ChangeDirectory();

      break;
    case 0x71:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: readDirectory %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2ReadDirectory();

      break;
    case 0x72:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileSystemScope\n");
#endif
      Cs2GetFileSystemScope();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x73:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: getFileInfo %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2GetFileInfo();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x74:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: readFile\n");
#endif
      Cs2ReadFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x75:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: abortFile\n");
#endif
      Cs2AbortFile();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x90:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetStatus\n");
#endif
      Cs2MpegGetStatus();
      break;
    case 0x91:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetInterrupt\n");
#endif
       Cs2MpegGetInterrupt();
#if CDDEBUG
      fprintf(stderr, "cs2\t: ret: %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      break;
    case 0x92:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetInterruptMask\n");
#endif
      Cs2MpegSetInterruptMask();
      break;
    case 0x93: 
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegInit\n");
#endif
      Cs2MpegInit();
      break;
    case 0x94:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetMode\n");
#endif
      Cs2MpegSetMode();

      break;
    case 0x95:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegPlay\n");
#endif
      Cs2MpegPlay();
      break;
    case 0x96:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetDecodingMethod\n");
#endif
       Cs2MpegSetDecodingMethod();
       break;
    case 0x9A:      
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetConnection\n");
#endif
       Cs2MpegSetConnection();
       break;
    case 0x9B:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetConnection\n");
#endif
      Cs2MpegGetConnection();

      break;
    case 0x9D:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetStream\n");
#endif
      Cs2MpegSetStream();
      break;
    case 0x9E:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegGetStream\n");
#endif
      Cs2MpegGetStream();
      break;
    case 0xA0:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegDisplay\n");
#endif
      Cs2MpegDisplay();
      break;
    case 0xA1:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetWindow\n");
#endif
       Cs2MpegSetWindow();
       break;
    case 0xA2:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetBorderColor\n");
#endif
      Cs2MpegSetBorderColor();
      break;
    case 0xA3:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetFade\n");
#endif
      Cs2MpegSetFade();
      break;
    case 0xA4:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetVideoEffects\n");
#endif
      Cs2MpegSetVideoEffects();
      break;
    case 0xAF:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: mpegSetLSI\n");
#endif
      Cs2MpegSetLSI();
      break;
    case 0xE0:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE0 %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif
      Cs2CmdE0();
      break;
    case 0xE1:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE1 %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif 
      Cs2CmdE1();
      break;
    case 0xE2:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command: cmdE2 %04x %04x %04x %04x %04x\n", Cs2Area->reg.HIRQ, Cs2Area->reg.CR1, Cs2Area->reg.CR2, Cs2Area->reg.CR3, Cs2Area->reg.CR4);
#endif 
      Cs2CmdE2();
      break;
    default:
#if CDDEBUG
      fprintf(stderr, "cs2\t: Command %02x not implemented\n", instruction);
#endif
      break;
  }
}

void Cs2GetStatus(void) {
  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2GetHardwareInfo(void) {
  if ((Cs2Area->status & 0xF) != CDB_STAT_OPEN && (Cs2Area->status & 0xF) != CDB_STAT_NODISC)
     Cs2Area->isdiskchanged = 0;

  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  // hardware flags/CD Version
  Cs2Area->reg.CR2 = 0x0201; // mpeg card exists
  // mpeg version, it actually is required(at least by the bios)

  if (Cs2Area->mpgauth)
     Cs2Area->reg.CR3 = 0x1;
  else
     Cs2Area->reg.CR3 = 0;

  // drive info/revision
  Cs2Area->reg.CR4 = 0x0400;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2GetToc(void) {
  Cs2Area->cdi->ReadTOC(Cs2Area->TOC);

  Cs2Area->transfercount = 0;
  Cs2Area->infotranstype = 0;

  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = 0xCC;
  Cs2Area->reg.CR3 = 0x0;
  Cs2Area->reg.CR4 = 0x0; 
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
  Cs2Area->status = CDB_STAT_PAUSE;
}

void Cs2GetSessionInfo(void) {

  switch (Cs2Area->reg.CR1 & 0xFF) {
    case 0:
            Cs2Area->reg.CR3 = 0x0100 | ((Cs2Area->TOC[101] & 0xFF0000) >> 16);
            Cs2Area->reg.CR4 = Cs2Area->TOC[101] & 0x00FFFF;
            break;
    case 1:
            Cs2Area->reg.CR3 = 0x0100; // return Session number(high byte)/and first byte of Session lba
            Cs2Area->reg.CR4 = 0; // lower word of Session lba
            break;
    default:
            Cs2Area->reg.CR3 = 0xFFFF;
            Cs2Area->reg.CR4 = 0xFFFF;
            break;
  }

  Cs2Area->status = CDB_STAT_PAUSE;
  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = 0;

  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2InitializeCDSystem(void) {
  u16 val = 0;
  u8 initflag = Cs2Area->reg.CR1 & 0xFF;

  if ((Cs2Area->status & 0xF) != CDB_STAT_OPEN && (Cs2Area->status & 0xF) != CDB_STAT_NODISC)
  {
     Cs2Area->status = CDB_STAT_PAUSE;
     Cs2Area->FAD = 150;
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
     Cs2Area->speed1x = 1;
  else
     Cs2Area->speed1x = 0;

  val = Cs2Area->reg.HIRQ & 0xFFE5;
  Cs2Area->isbufferfull = 0;

  if (Cs2Area->isdiskchanged)
     val |= CDB_HIRQ_DCHG;
  else
     val &= ~CDB_HIRQ_DCHG;

  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | (Cs2Area->repcnt & 0xF);
  Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | (Cs2Area->track & 0xFF);
  Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) &0xFF);
  Cs2Area->reg.CR4 = (u16) Cs2Area->FAD;
  Cs2Area->reg.HIRQ = val | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2EndDataTransfer(void) {
  long i;
  if (Cs2Area->cdwnum)
  {
     Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->cdwnum >> 17) & 0xFF);
     Cs2Area->reg.CR2 = (unsigned short)(Cs2Area->cdwnum >> 1);
     Cs2Area->reg.CR3 = 0;
     Cs2Area->reg.CR4 = 0;
  }
  else
  {
     Cs2Area->reg.CR1 = (Cs2Area->status << 8) | 0xFF; // FIXME
     Cs2Area->reg.CR2 = 0xFFFF;
     Cs2Area->reg.CR3 = 0;
     Cs2Area->reg.CR4 = 0;
  }

  // stop any transfers that may be going(this is still probably wrong)

  switch (Cs2Area->datatranstype) {    
     case 2: {
        // Get Then Delete Sector

        // Make sure we actually have to free something
        if (Cs2Area->datatranspartition->size <= 0) break;

        Cs2Area->datatranstype = -1;

        // free blocks
        for (i = Cs2Area->datatranssectpos; i < (Cs2Area->datatranssectpos + Cs2Area->datasectstotrans); i++)
        {
           Cs2FreeBlock(Cs2Area->datatranspartition->block[i]);
           Cs2Area->datatranspartition->block[i] = NULL;
           Cs2Area->datatranspartition->blocknum[i] = 0xFF;
        }

        // sort remaining blocks
        Cs2SortBlocks(Cs2Area->datatranspartition);

        Cs2Area->datatranspartition->size -= Cs2Area->cdwnum;
        Cs2Area->datatranspartition->numblocks -= Cs2Area->datasectstotrans;

        if (Cs2Area->blockfreespace == 200) Cs2Area->isonesectorstored = 0;

        break;
     }
     default: break;
  }

  Cs2Area->cdwnum = 0;

  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2PlayDisc(void) {
  u32 pdspos;
  u32 pdepos;
  u32 pdpmode;

  // Get all the arguments
  pdspos = ((Cs2Area->reg.CR1 & 0xFF) << 16) | Cs2Area->reg.CR2;
  pdepos = ((Cs2Area->reg.CR3 & 0xFF) << 16) | Cs2Area->reg.CR4;
  pdpmode = Cs2Area->reg.CR3 >> 8;

  // Convert Start Position to playFAD
  if (pdspos == 0xFFFFFF)
  {
     // No Change
  }
  else if (pdspos & 0x800000)
  {
     // FAD Mode
     Cs2Area->playFAD = (pdspos & 0xFFFFF);
     Cs2SetupDefaultPlayStats(Cs2FADToTrack(Cs2Area->playFAD));
     Cs2Area->FAD = Cs2Area->playFAD;
  }
  else if (pdspos != 0)
  {
     // Track Mode
     Cs2SetupDefaultPlayStats((pdspos & 0xFF00) >> 8);
     Cs2Area->FAD = Cs2Area->playFAD = Cs2Area->FAD + (pdspos & 0xFF); // is this right?
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
     Cs2Area->playendFAD = Cs2Area->playFAD+(pdepos & 0xFFFFF);
  }
  else if (pdepos != 0)
  {
     // Track Mode
     Cs2Area->playendFAD = Cs2TrackToFAD(pdepos);
  }
  else
  {
     // Default Mode
     Cs2Area->playendFAD = Cs2TrackToFAD(0xFFFF);
  }

  // setup play mode here
#if CDDEBUG
  if (pdpmode != 0)
     fprintf(stderr, "cs2\t: playDisc: Unsupported play mode = %02X\n", pdpmode);
#endif

  Cs2SetTiming(1);

  Cs2Area->status = CDB_STAT_PLAY;

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2SeekDisc(void) {
  if (Cs2Area->reg.CR1 & 0x80)
  {
     // Seek by FAD
     u32 sdFAD;

     sdFAD = ((Cs2Area->reg.CR1 & 0xFF) << 16) | Cs2Area->reg.CR2;

     if (sdFAD == 0xFFFFFF)
        Cs2Area->status = CDB_STAT_PAUSE;
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
     if (Cs2Area->reg.CR2 >> 8)
     {
        // Seek by index
        Cs2Area->status = CDB_STAT_PAUSE;
        Cs2SetupDefaultPlayStats((Cs2Area->reg.CR2 >> 8));
        Cs2Area->index = Cs2Area->reg.CR2 & 0xFF;
     }
     else
     {
        // Error
        Cs2Area->status = CDB_STAT_STANDBY;
        Cs2Area->options = 0xFF;
        Cs2Area->repcnt = 0xFF;
        Cs2Area->ctrladdr = 0xFF;
        Cs2Area->track = 0xFF;
        Cs2Area->index = 0xFF;
        Cs2Area->FAD = 0xFFFFFFFF;
     }
  }

  Cs2SetTiming(0);

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2GetSubcodeQRW(void) {
  // According to Tyranid's doc, the subcode type is stored in the low byte
  // of CR2. However, Sega's CDC library writes the type to the low byte
  // of CR1. Somehow I'd sooner believe Sega is right.
  switch(Cs2Area->reg.CR1 & 0xFF) {
     case 0:
             // Get Q Channel
             Cs2Area->reg.CR1 = (Cs2Area->status << 8) | 0;
             Cs2Area->reg.CR2 = 5;
             Cs2Area->reg.CR3 = 0;
             Cs2Area->reg.CR4 = 0;

             // setup transfer here(fix me)
             break;
     case 1:
             // Get RW Channel
             Cs2Area->reg.CR1 = (Cs2Area->status << 8) | 0;
             Cs2Area->reg.CR2 = 12;
             Cs2Area->reg.CR3 = 0;
             Cs2Area->reg.CR4 = 0;

             // setup transfer here(fix me)
             break;
     default: break;
  }

  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
}

void Cs2SetCDDeviceConnection(void) {
  u32 scdcfilternum;

  scdcfilternum = (Cs2Area->reg.CR3 >> 8);

  if (scdcfilternum == 0xFF)
     Cs2Area->outconcddev = NULL;
  else if (scdcfilternum < 0x24)
     Cs2Area->outconcddev = Cs2Area->filter + scdcfilternum;

  Cs2Area->outconcddevnum = scdcfilternum;

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetLastBufferDestination(void) {
  Cs2Area->reg.CR1 = (Cs2Area->status << 8);
  Cs2Area->reg.CR2 = 0;
  Cs2Area->reg.CR3 = Cs2Area->lastbuffer << 8; 
  Cs2Area->reg.CR4 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2SetFilterRange(void) {
  unsigned char sfrfilternum;

  sfrfilternum = Cs2Area->reg.CR3 >> 8;

  Cs2Area->filter[sfrfilternum].FAD = ((Cs2Area->reg.CR1 & 0xFF) << 16) | Cs2Area->reg.CR2;
  Cs2Area->filter[sfrfilternum].range = ((Cs2Area->reg.CR3 & 0xFF) << 16) | Cs2Area->reg.CR4;

  // return default cd stats
  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2SetFilterSubheaderConditions(void) {
  u8 sfscfilternum;

  sfscfilternum = Cs2Area->reg.CR3 >> 8;

  Cs2Area->filter[sfscfilternum].chan = Cs2Area->reg.CR1 & 0xFF;
  Cs2Area->filter[sfscfilternum].smmask = Cs2Area->reg.CR2 >> 8;
  Cs2Area->filter[sfscfilternum].cimask = Cs2Area->reg.CR2 & 0xFF;
  Cs2Area->filter[sfscfilternum].fid = Cs2Area->reg.CR3 & 0xFF;;
  Cs2Area->filter[sfscfilternum].smval = Cs2Area->reg.CR4 >> 8;
  Cs2Area->filter[sfscfilternum].cival = Cs2Area->reg.CR4 & 0xFF;

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetFilterSubheaderConditions(void) {
  u8 gfscfilternum;

  gfscfilternum = Cs2Area->reg.CR3 >> 8;

  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->filter[gfscfilternum].chan;
  Cs2Area->reg.CR2 = (Cs2Area->filter[gfscfilternum].smmask << 8) | Cs2Area->filter[gfscfilternum].cimask;
  Cs2Area->reg.CR3 = Cs2Area->filter[gfscfilternum].fid;
  Cs2Area->reg.CR4 = (Cs2Area->filter[gfscfilternum].smval << 8) | Cs2Area->filter[gfscfilternum].cival;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2SetFilterMode(void) {
  u8 sfmfilternum;

  sfmfilternum = Cs2Area->reg.CR3 >> 8;

  Cs2Area->filter[sfmfilternum].mode = Cs2Area->reg.CR1 & 0xFF;

  if (Cs2Area->filter[sfmfilternum].mode & 0x80)
  {
     // Initialize filter conditions
     Cs2Area->filter[sfmfilternum].mode = 0;
     Cs2Area->filter[sfmfilternum].FAD = 0;
     Cs2Area->filter[sfmfilternum].range = 0;
     Cs2Area->filter[sfmfilternum].chan = 0;
     Cs2Area->filter[sfmfilternum].smmask = 0;
     Cs2Area->filter[sfmfilternum].cimask = 0;
     Cs2Area->filter[sfmfilternum].smval = 0;
     Cs2Area->filter[sfmfilternum].cival = 0;
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetFilterMode(void) {
  u8 gfmfilternum;

  gfmfilternum = Cs2Area->reg.CR3 >> 8;

  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->filter[gfmfilternum].mode;
  Cs2Area->reg.CR2 = 0;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.CR4 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2SetFilterConnection(void) {
  u8 sfcfilternum;

  sfcfilternum = Cs2Area->reg.CR3 >> 8;

  if (Cs2Area->reg.CR1 & 0x1)
  {
     // Set connection for true condition
     Cs2Area->filter[sfcfilternum].condtrue = Cs2Area->reg.CR2 >> 8;
  }

  if (Cs2Area->reg.CR1 & 0x2)
  {
     // Set connection for false condition
     Cs2Area->filter[sfcfilternum].condfalse = Cs2Area->reg.CR2 & 0xFF;
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2ResetSelector(void) {
  // still needs a bit of work
  unsigned long i, i2;

  if ((Cs2Area->reg.CR1 & 0xFF) == 0)
  {
     // Reset specified partition buffer only
     u32 rsbufno = Cs2Area->reg.CR3 >> 8;

     // sort remaining blocks
     if (rsbufno < MAX_SELECTORS)
     {
        // clear partition
        for (i = 0; i < Cs2Area->partition[rsbufno].numblocks; i++)
        {
           Cs2FreeBlock(Cs2Area->partition[rsbufno].block[i]);
           Cs2Area->partition[rsbufno].block[i] = NULL;
           Cs2Area->partition[rsbufno].blocknum[i] = 0xFF;
        }

        Cs2Area->partition[rsbufno].size = -1;
        Cs2Area->partition[rsbufno].numblocks = 0;
     }

     if (Cs2Area->blockfreespace > 0) Cs2Area->isbufferfull = 0;
     if (Cs2Area->blockfreespace == 200) Cs2Area->isonesectorstored = 0;

     doCDReport();
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
     return;
  }

  // parse flags and reset the specified area(fix me)
  if (Cs2Area->reg.CR1 & 0x80)
  {
     // reset false filter output connections
     for (i = 0; i < MAX_SELECTORS; i++)
        Cs2Area->filter[i].condfalse = 0xFF;
  }

  if (Cs2Area->reg.CR1 & 0x40)
  {
     // reset true filter output connections
     for (i = 0; i < MAX_SELECTORS; i++)
        Cs2Area->filter[i].condtrue = i;
  }

  if (Cs2Area->reg.CR1 & 0x10)
  {
     // reset filter conditions
     for (i = 0; i < MAX_SELECTORS; i++)
     {
        Cs2Area->filter[i].FAD = 0;
        Cs2Area->filter[i].range = 0xFFFFFFFF;
        Cs2Area->filter[i].mode = 0;
        Cs2Area->filter[i].chan = 0;
        Cs2Area->filter[i].smmask = 0;
        Cs2Area->filter[i].cimask = 0;
        Cs2Area->filter[i].fid = 0;
        Cs2Area->filter[i].smval = 0;
        Cs2Area->filter[i].cival = 0;
     }
  }

  if (Cs2Area->reg.CR1 & 0x8)
  {
     // reset partition output connectors
  }

  if (Cs2Area->reg.CR1 & 0x4)
  {
     // reset partitions buffer data
     Cs2Area->isbufferfull = 0;

     // clear partitions
     for (i = 0; i < MAX_SELECTORS; i++)
     {
        Cs2Area->partition[i].size = -1;
        Cs2Area->partition[i].numblocks = 0;

        for (i2 = 0; i2 < MAX_BLOCKS; i2++)
        {
           Cs2Area->partition[i].block[i2] = NULL;
           Cs2Area->partition[i].blocknum[i2] = 0xFF;
        }
     }

     // clear blocks
     for (i = 0; i < MAX_BLOCKS; i++)
     {
        Cs2Area->block[i].size = -1;
        memset(Cs2Area->block[i].data, 0, 2352);
     }

     Cs2Area->isonesectorstored = 0;
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetBufferSize(void) {
  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = Cs2Area->blockfreespace;
  Cs2Area->reg.CR3 = MAX_SELECTORS << 8;
  Cs2Area->reg.CR4 = MAX_BLOCKS;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2GetSectorNumber(void) {
  u32 gsnbufno;

  gsnbufno = Cs2Area->reg.CR3 >> 8;

  if (Cs2Area->partition[gsnbufno].size == -1)
     Cs2Area->reg.CR4 = 0;
  else
     Cs2Area->reg.CR4 = Cs2Area->partition[gsnbufno].numblocks;

  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = 0;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;
}

void Cs2CalculateActualSize(void) {
  u16 i;
  u32 casbufno;
  u16 cassectoffset;
  u16 casnumsect;

  cassectoffset = Cs2Area->reg.CR2;
  casbufno = Cs2Area->reg.CR3 >> 8;
  casnumsect = Cs2Area->reg.CR4;

  if (Cs2Area->partition[casbufno].size != 0)
  {
     Cs2Area->calcsize = 0;

     for (i = 0; i < casnumsect; i++)
     {
        if (Cs2Area->partition[casbufno].block[cassectoffset])
           Cs2Area->calcsize += (Cs2Area->partition[casbufno].block[cassectoffset]->size / 2);
     }
  }
  else
     Cs2Area->calcsize = 0;

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetActualSize(void) {
  Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->calcsize >> 16) & 0xFF);
  Cs2Area->reg.CR2 = Cs2Area->calcsize & 0xFFFF;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.CR4 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetSectorInfo(void) {
  u32 gsisctnum;
  u32 gsibufno;

  gsisctnum = Cs2Area->reg.CR2 & 0xFF;
  gsibufno = Cs2Area->reg.CR3 >> 8;
  if (gsibufno < MAX_SELECTORS) {
     if (gsisctnum < Cs2Area->partition[gsibufno].numblocks) {
        Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->partition[gsibufno].block[gsisctnum]->FAD >> 16) & 0xFF);
        Cs2Area->reg.CR2 = Cs2Area->partition[gsibufno].block[gsisctnum]->FAD & 0xFFFF;
        Cs2Area->reg.CR3 = (Cs2Area->partition[gsibufno].block[gsisctnum]->fn << 8) | Cs2Area->partition[gsibufno].block[gsisctnum]->cn;
        Cs2Area->reg.CR4 = (Cs2Area->partition[gsibufno].block[gsisctnum]->sm << 8) | Cs2Area->partition[gsibufno].block[gsisctnum]->ci;
        Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
        return;
     }
     else
     {
#if CDDEBUG
        fprintf(stderr, "cs2\t: getSectorInfo: Unsupported Partition Number\n");
#endif
     }
  }

  Cs2Area->reg.CR1 = (CDB_STAT_REJECT << 8) | (Cs2Area->reg.CR1 & 0xFF);
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2SetSectorLength(void) {
  switch (Cs2Area->reg.CR1 & 0xFF) {
    case 0:
            Cs2Area->getsectsize = 2048;
            break;
    case 1:
            Cs2Area->getsectsize = 2336;
            break;
    case 2:
            Cs2Area->getsectsize = 2340;
            break;
    case 3:
            Cs2Area->getsectsize = 2352;
            break;
    default: break;
  }

  switch (Cs2Area->reg.CR2 >> 8) {
    case 0:
            Cs2Area->putsectsize = 2048;
            break;
    case 1:
            Cs2Area->putsectsize = 2336;
            break;
    case 2:
            Cs2Area->putsectsize = 2340;
            break;
    case 3:
            Cs2Area->putsectsize = 2352;
            break;
    default: break;
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_ESEL;
}

void Cs2GetSectorData(void) {
  u32 gsdbufno;

  gsdbufno = Cs2Area->reg.CR3 >> 8;

  if (gsdbufno < MAX_SELECTORS)
  {
     // Setup Data Transfer
     Cs2Area->cdwnum = 0;
     Cs2Area->datatranstype = 0;
     Cs2Area->datatranspartition = Cs2Area->partition + gsdbufno;
     Cs2Area->datatranspartitionnum = gsdbufno;
     Cs2Area->datatransoffset = 0;
     Cs2Area->datanumsecttrans = 0;
     Cs2Area->datatranssectpos = Cs2Area->reg.CR2;
     Cs2Area->datasectstotrans = Cs2Area->reg.CR4;

     doCDReport();
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY | CDB_HIRQ_EHST;
  }
  else
  {
     Cs2Area->reg.CR1 = (CDB_STAT_REJECT << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
     Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | (Cs2Area->track & 0xFF);
     Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) &0xFF);
     Cs2Area->reg.CR4 = (u16) Cs2Area->FAD;
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2DeleteSectorData(void) {
  u32 dsdsectoffset = 0;
  u32 dsdbufno = 0;
  u32 dsdsectnum = 0;
  u32 i;

  dsdsectoffset = Cs2Area->reg.CR2;
  dsdbufno = Cs2Area->reg.CR3 >> 8;
  dsdsectnum = Cs2Area->reg.CR4;

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
        dsdsectoffset = Cs2Area->partition[dsdbufno].numblocks - dsdsectoffset - 1;
        // I think this is right
        dsdsectnum = 1; 
     }

     for (i = dsdsectoffset; i < (dsdsectoffset+dsdsectnum); i++)
     {
        Cs2Area->partition[dsdbufno].size -= Cs2Area->partition[dsdbufno].block[i]->size;
        Cs2FreeBlock(Cs2Area->partition[dsdbufno].block[i]);
        Cs2Area->partition[dsdbufno].block[i] = NULL;
        Cs2Area->partition[dsdbufno].blocknum[i] = 0xFF;
     }

     // sort remaining blocks
     Cs2SortBlocks(&Cs2Area->partition[dsdbufno]);

     Cs2Area->partition[dsdbufno].numblocks -= dsdsectnum;

     if (Cs2Area->blockfreespace == 200) Cs2Area->isonesectorstored = 0;

     doCDReport();
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
  else
  {
     Cs2Area->reg.CR1 = (CDB_STAT_REJECT << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
     Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | (Cs2Area->track & 0xFF);
     Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) &0xFF);
     Cs2Area->reg.CR4 = (u16) Cs2Area->FAD;
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2GetThenDeleteSectorData(void) {
  u32 gtdsdbufno;

  gtdsdbufno = Cs2Area->reg.CR3 >> 8;

  if (gtdsdbufno < MAX_SELECTORS)
  {
     // Setup Data Transfer
     Cs2Area->cdwnum = 0;
     Cs2Area->datatranstype = 2;
     Cs2Area->datatranspartition = Cs2Area->partition + gtdsdbufno;
     Cs2Area->datatransoffset = 0;
     Cs2Area->datanumsecttrans = 0;
     Cs2Area->datatranssectpos = Cs2Area->reg.CR2;
     Cs2Area->datasectstotrans = Cs2Area->reg.CR4;

     doCDReport();
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY | CDB_HIRQ_EHST;
  }
  else
  {
     Cs2Area->reg.CR1 = (CDB_STAT_REJECT << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
     Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | (Cs2Area->track & 0xFF);
     Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) &0xFF);
     Cs2Area->reg.CR4 = (u16) Cs2Area->FAD;
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2PutSectorData(void) {
  u32 psdfiltno;

  psdfiltno = Cs2Area->reg.CR3 >> 8;

  if (psdfiltno < MAX_SELECTORS)
  {
     // I'm not really sure what I'm supposed to really be doing or returning
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
  else
  {
     Cs2Area->reg.CR1 = (CDB_STAT_REJECT << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
     Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | (Cs2Area->track & 0xFF);
     Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) &0xFF);
     Cs2Area->reg.CR4 = (u16) Cs2Area->FAD;
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EHST;
  }
}

void Cs2GetCopyError(void) {
  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = 0;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.CR4 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2ChangeDirectory(void) {
  u32 cdfilternum;

  cdfilternum = (Cs2Area->reg.CR3 >> 8);

  if (cdfilternum == 0xFF)
  {
     // fix me
  }
  else if (cdfilternum < 0x24)
  {
     if (Cs2ReadFileSystem(Cs2Area->filter + cdfilternum, ((Cs2Area->reg.CR3 & 0xFF) << 16) | Cs2Area->reg.CR4, 0) != 0)
     {
        // fix me
#if CDDEBUG
        fprintf(stderr, "cs2\t: ReadFileSystem failed\n");
#endif
     }
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2ReadDirectory(void) {
  u32 rdfilternum;

  rdfilternum = (Cs2Area->reg.CR3 >> 8);

  if (rdfilternum == 0xFF)
  {
     // fix me
  }
  else if (rdfilternum < 0x24)
  {
     if (Cs2ReadFileSystem(Cs2Area->filter + rdfilternum, ((Cs2Area->reg.CR3 & 0xFF) << 8) | Cs2Area->reg.CR4, 1) != 0)
     {
        // fix me
#if CDDEBUG
        fprintf(stderr, "cs2\t: ReadFileSystem failed\n");
#endif
     }
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2GetFileSystemScope(void) {
  // may need to fix this
  Cs2Area->reg.CR1 = Cs2Area->status << 8;
  Cs2Area->reg.CR2 = Cs2Area->numfiles - 2;
  Cs2Area->reg.CR3 = 0x0100;
  Cs2Area->reg.CR4 = 0x0002;

  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2GetFileInfo(void) {
  u32 gfifid;

  gfifid = ((Cs2Area->reg.CR3 & 0xFF) << 16) | Cs2Area->reg.CR4;

  if (gfifid == 0xFFFFFF)
  {
     Cs2Area->transfercount = 0;
     Cs2Area->infotranstype = 2;

     Cs2Area->reg.CR1 = Cs2Area->status << 8;
     Cs2Area->reg.CR2 = 0x05F4;
     Cs2Area->reg.CR3 = 0;
     Cs2Area->reg.CR4 = 0;
  }
  else
  {
     Cs2SetupFileInfoTransfer(gfifid);

     Cs2Area->transfercount = 0;
     Cs2Area->infotranstype = 1;

     Cs2Area->reg.CR1 = Cs2Area->status << 8;
     Cs2Area->reg.CR2 = 0x06;
     Cs2Area->reg.CR3 = 0;
     Cs2Area->reg.CR4 = 0;
  }

  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_DRDY;

}

void Cs2ReadFile(void) {
  u32 rffid, rfoffset, rfsize;

  rffid = ((Cs2Area->reg.CR3 & 0xFF) << 8) | Cs2Area->reg.CR4;
  rfoffset = ((Cs2Area->reg.CR1 & 0xFF) << 8) | Cs2Area->reg.CR2;
  rfsize = ((Cs2Area->fileinfo[rffid].size + Cs2Area->getsectsize - 1) /
           Cs2Area->getsectsize) - rfoffset;

  Cs2SetupDefaultPlayStats(Cs2FADToTrack(Cs2Area->fileinfo[rffid].lba + rfoffset));

  Cs2Area->playFAD = Cs2Area->FAD = Cs2Area->fileinfo[rffid].lba + rfoffset;
  Cs2Area->playendFAD = Cs2Area->playFAD + rfsize;

  Cs2Area->options = 0x8;

  Cs2SetTiming(1);

  Cs2Area->status = CDB_STAT_PLAY;

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2AbortFile(void) {
  if ((Cs2Area->status & 0xF) != CDB_STAT_OPEN &&
      (Cs2Area->status & 0xF) != CDB_STAT_NODISC)
     Cs2Area->status = CDB_STAT_PAUSE;
  Cs2Area->isonesectorstored = 0;
  Cs2Area->datatranstype = -1;
  Cs2Area->cdwnum = 0;
  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_EFLS;
}

void Cs2MpegGetStatus(void) {
   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegGetInterrupt(void) {
   u32 mgiworkinterrupt;

   // mpeg interrupt should be retrieved here
   mgiworkinterrupt = 0;

   // mask interupt
   mgiworkinterrupt &= Cs2Area->mpegintmask;

   Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((mgiworkinterrupt >> 16) & 0xFF);
   Cs2Area->reg.CR2 = (u16) mgiworkinterrupt;
   Cs2Area->reg.CR3 = 0;
   Cs2Area->reg.CR4 = 0;
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetInterruptMask(void) {
   Cs2Area->mpegintmask = ((Cs2Area->reg.CR1 & 0xFF) << 16) | Cs2Area->reg.CR2;

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegInit(void) {

  if (Cs2Area->mpgauth)
     Cs2Area->reg.CR1 = Cs2Area->status << 8;
  else
     Cs2Area->reg.CR1 = 0xFF00;

  // double-check this
  if (Cs2Area->reg.CR2 == 0x0001) // software timer/reset?
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM | CDB_HIRQ_MPED | CDB_HIRQ_MPST; 
  else
     Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPED | CDB_HIRQ_MPST;

  Cs2Area->reg.CR2 = 0;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.CR4 = 0;

  // future mpeg-related variables should be initialized here
}

void Cs2MpegSetMode(void) {
   // fix me

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegPlay(void) {
   // fix me

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetDecodingMethod(void) {
   // fix me

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetConnection(void) {
   int mscnext = (Cs2Area->reg.CR3 >> 8);

   if (mscnext == 0)
   {
      // Current
      Cs2Area->mpegcon[0].audcon = Cs2Area->reg.CR1 & 0xFF;
      Cs2Area->mpegcon[0].audlay = Cs2Area->reg.CR2 >> 8;
      Cs2Area->mpegcon[0].audbufdivnum = Cs2Area->reg.CR2 & 0xFF;
      Cs2Area->mpegcon[0].vidcon = Cs2Area->reg.CR3 & 0xFF;
      Cs2Area->mpegcon[0].vidlay = Cs2Area->reg.CR4 >> 8;
      Cs2Area->mpegcon[0].vidbufdivnum = Cs2Area->reg.CR4 & 0xFF;
   }
   else
   {
      // Next
      Cs2Area->mpegcon[1].audcon = Cs2Area->reg.CR1 & 0xFF;
      Cs2Area->mpegcon[1].audlay = Cs2Area->reg.CR2 >> 8;
      Cs2Area->mpegcon[1].audbufdivnum = Cs2Area->reg.CR2 & 0xFF;
      Cs2Area->mpegcon[1].vidcon = Cs2Area->reg.CR3 & 0xFF;
      Cs2Area->mpegcon[1].vidlay = Cs2Area->reg.CR4 >> 8;
      Cs2Area->mpegcon[1].vidbufdivnum = Cs2Area->reg.CR4 & 0xFF;
   }

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegGetConnection(void) {
   int mgcnext = (Cs2Area->reg.CR3 >> 8);

   if (mgcnext == 0)
   {
      // Current
      Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->mpegcon[0].audcon;
      Cs2Area->reg.CR2 = (Cs2Area->mpegcon[0].audlay << 8) | Cs2Area->mpegcon[0].audbufdivnum;
      Cs2Area->reg.CR3 = Cs2Area->mpegcon[0].vidcon;
      Cs2Area->reg.CR4 = (Cs2Area->mpegcon[0].vidlay << 8) | Cs2Area->mpegcon[0].vidbufdivnum;
   }
   else
   {
      // Next
      Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->mpegcon[1].audcon;
      Cs2Area->reg.CR2 = (Cs2Area->mpegcon[1].audlay << 8) | Cs2Area->mpegcon[1].audbufdivnum;
      Cs2Area->reg.CR3 = Cs2Area->mpegcon[1].vidcon;
      Cs2Area->reg.CR4 = (Cs2Area->mpegcon[1].vidlay << 8) | Cs2Area->mpegcon[1].vidbufdivnum;
   }

   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetStream(void) {
   int mssnext = (Cs2Area->reg.CR3 >> 8);

   if (mssnext == 0)
   {
      // Current
      Cs2Area->mpegstm[0].audstm = Cs2Area->reg.CR1 & 0xFF;
      Cs2Area->mpegstm[0].audstmid = Cs2Area->reg.CR2 >> 8;
      Cs2Area->mpegstm[0].audchannum = Cs2Area->reg.CR2 & 0xFF;
      Cs2Area->mpegstm[0].vidstm = Cs2Area->reg.CR3 & 0xFF;
      Cs2Area->mpegstm[0].vidstmid = Cs2Area->reg.CR4 >> 8;
      Cs2Area->mpegstm[0].vidchannum = Cs2Area->reg.CR4 & 0xFF;
   }
   else
   {
      // Next
      Cs2Area->mpegstm[1].audstm = Cs2Area->reg.CR1 & 0xFF;
      Cs2Area->mpegstm[1].audstmid = Cs2Area->reg.CR2 >> 8;
      Cs2Area->mpegstm[1].audchannum = Cs2Area->reg.CR2 & 0xFF;
      Cs2Area->mpegstm[1].vidstm = Cs2Area->reg.CR3 & 0xFF;
      Cs2Area->mpegstm[1].vidstmid = Cs2Area->reg.CR4 >> 8;
      Cs2Area->mpegstm[1].vidchannum = Cs2Area->reg.CR4 & 0xFF;
   }

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegGetStream(void) {
   int mgsnext = (Cs2Area->reg.CR3 >> 8);

   if (mgsnext == 0)
   {
      // Current
      Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->mpegstm[0].audstm;
      Cs2Area->reg.CR2 = (Cs2Area->mpegstm[0].audstmid << 8) | Cs2Area->mpegstm[0].audchannum;
      Cs2Area->reg.CR3 = Cs2Area->mpegstm[0].vidstm;
      Cs2Area->reg.CR4 = (Cs2Area->mpegstm[0].vidstmid << 8) | Cs2Area->mpegstm[0].vidchannum;
   }
   else
   {
      // Next
      Cs2Area->reg.CR1 = (Cs2Area->status << 8) | Cs2Area->mpegstm[1].audstm;
      Cs2Area->reg.CR2 = (Cs2Area->mpegstm[1].audstmid << 8) | Cs2Area->mpegstm[1].audchannum;
      Cs2Area->reg.CR3 = Cs2Area->mpegstm[1].vidstm;
      Cs2Area->reg.CR4 = (Cs2Area->mpegstm[1].vidstmid << 8) | Cs2Area->mpegstm[1].vidchannum;
   }

   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegDisplay(void) {
   // fix me(should be setting display setting)

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetWindow(void) {
   // fix me(should be setting windows settings)

   // return default mpeg stats
   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetBorderColor() {
   // fix me(should be setting border color)

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetFade() {
   // fix me(should be setting fade setting)

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetVideoEffects(void) {
   // fix me(should be setting video effects settings)

   doMPEGReport();
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2MpegSetLSI() {
   // fix me(should be setting the LSI, among other things)
   Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPCM;
}

void Cs2CmdE0(void) {
  int mpegauth;

  mpegauth = Cs2Area->reg.CR2 & 0xFF;


  if ((Cs2Area->status & 0xF) != CDB_STAT_NODISC &&
      (Cs2Area->status & 0xF) != CDB_STAT_OPEN)
  {
     // Set registers all to invalid values(aside from status)
     Cs2Area->status = CDB_STAT_BUSY;

     Cs2Area->reg.CR1 = (Cs2Area->status << 8) | 0xFF;
     Cs2Area->reg.CR2 = 0xFFFF;
     Cs2Area->reg.CR3 = 0xFFFF;
     Cs2Area->reg.CR4 = 0xFFFF;

     if (mpegauth == 1)
     {
        Cs2Area->reg.HIRQ |= CDB_HIRQ_MPED;
        Cs2Area->mpgauth = 2;
     }
     else
     {     
        // if authentication passes(obviously it always does), CDB_HIRQ_CSCT is set
        Cs2Area->isonesectorstored = 1;
        Cs2Area->reg.HIRQ |= CDB_HIRQ_EFLS | CDB_HIRQ_CSCT;
        Cs2Area->satauth = 4;
     }

     // Set registers all back to normal values

     Cs2Area->status = CDB_STAT_PAUSE;
  }
  else
  {
     if (mpegauth == 1)
     {
        Cs2Area->reg.HIRQ |= CDB_HIRQ_MPED;
        Cs2Area->mpgauth = 2;
     }
     else
        Cs2Area->reg.HIRQ |= CDB_HIRQ_EFLS | CDB_HIRQ_CSCT;
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2CmdE1(void) {
  Cs2Area->reg.CR1 = (Cs2Area->status << 8);
  if (Cs2Area->reg.CR2)
     Cs2Area->reg.CR2 = Cs2Area->mpgauth;
  else
     Cs2Area->reg.CR2 = Cs2Area->satauth;
  Cs2Area->reg.CR3 = 0;
  Cs2Area->reg.CR4 = 0;
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK;
}

void Cs2CmdE2(void) {
  u16 i;
  FILE * mpgfp;
  partition_struct * mpgpartition;

  // fix me
  Cs2Area->mpgauth |= 0x300;

  Cs2Area->outconmpegrom = Cs2Area->filter + 0;
  Cs2Area->outconmpegromnum = 0;

  if ((mpgfp = fopen(Cs2Area->mpegpath, "rb")) != NULL)
  {
     u32 readoffset = ((Cs2Area->reg.CR1 & 0xFF) << 8) | Cs2Area->reg.CR2;
     u16 readsize = Cs2Area->reg.CR4;

     fseek(mpgfp, readoffset * Cs2Area->getsectsize, SEEK_SET);
     if ((mpgpartition = Cs2GetPartition(Cs2Area->outconmpegrom)) != NULL && !Cs2Area->isbufferfull)
     {
        mpgpartition->size = 0;

        for (i = 0; i < readsize; i++)
        {
           mpgpartition->block[mpgpartition->numblocks] = Cs2AllocateBlock(&mpgpartition->blocknum[mpgpartition->numblocks]);

           if (mpgpartition->block[mpgpartition->numblocks] != NULL) {
              // read data
              fread((void *)mpgpartition->block[mpgpartition->numblocks]->data, 1, Cs2Area->getsectsize, mpgfp);

              mpgpartition->numblocks++;
              mpgpartition->size += Cs2Area->getsectsize;
           }
        }

        Cs2Area->isonesectorstored = 1;
        Cs2Area->reg.HIRQ |= CDB_HIRQ_CSCT;
     }

     fclose(mpgfp);
  }

  doCDReport();
  Cs2Area->reg.HIRQ |= CDB_HIRQ_CMOK | CDB_HIRQ_MPED;
}

u8 Cs2FADToTrack(u32 val) {
  int i;
  for (i = 0; i < 99; i++)
  {
     if (Cs2Area->TOC[i] == 0xFFFFFFFF) return 0xFF;

     if (val >= (Cs2Area->TOC[i] & 0xFFFFFF) && val < (Cs2Area->TOC[i + 1] & 0xFFFFFF))
        return (i + 1);
  }

  return 0;
}

u32 Cs2TrackToFAD(u16 trackandindex) {
  if (trackandindex == 0xFFFF)
     // leadout position
     return (Cs2Area->TOC[101] & 0x00FFFFFF); 
  if (trackandindex != 0x0000)
     // regular track
     return (Cs2Area->TOC[(trackandindex >> 8) - 1] & 0x00FFFFFF) + (trackandindex & 0xFF);

  // assume it's leadin
  return 0;
}

void Cs2SetupDefaultPlayStats(u8 track_number) {
  if (track_number != 0xFF)
  {
     Cs2Area->options = 0;
     Cs2Area->repcnt = 0;
     Cs2Area->ctrladdr = (Cs2Area->TOC[track_number - 1] & 0xFF000000) >> 24;
     Cs2Area->index = 1;
     Cs2Area->track = track_number;
     Cs2Area->FAD = Cs2Area->TOC[track_number - 1] & 0x00FFFFFF;
  }
}

block_struct * Cs2AllocateBlock(u8 * blocknum) {
  unsigned long i;
  // find a free block
  for(i = 0; i < 200; i++)
  {
     if (Cs2Area->block[i].size == -1)
     {
        Cs2Area->blockfreespace--;

        if (Cs2Area->blockfreespace <= 0) Cs2Area->isbufferfull = 1;

        Cs2Area->block[i].size = Cs2Area->getsectsize;

        *blocknum = i;
        return (Cs2Area->block + i);
     }
  }

  Cs2Area->isbufferfull = 1;

  return NULL;
}

void Cs2FreeBlock(block_struct * blk) {
  blk->size = -1;
  Cs2Area->blockfreespace++;
  Cs2Area->isbufferfull = 0;
  Cs2Area->reg.HIRQ &= ~CDB_HIRQ_BFUL;
}

void Cs2SortBlocks(partition_struct * part) {
  unsigned long i, i2;
  block_struct * blktmp;
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

partition_struct * Cs2GetPartition(filter_struct * curfilter)
{
  // go through various filter conditions here(fix me)

  return &Cs2Area->partition[curfilter->condtrue];
}

partition_struct * Cs2FilterData(filter_struct * curfilter, int isaudio)
{
  int condresults = 1;
  partition_struct * fltpartition = NULL;

  // fix me, this is pretty bad. Though I guess it's a start

  for (;;)
  {
     // detect which type of sector we're dealing with
     // If it's not mode 2, ignore the subheader conditions
     if (Cs2Area->workblock.data[0xF] == 0x02 && !isaudio)
     {
        // Mode 2
        // go through various subheader filter conditions here(fix me)
   
        if (curfilter->mode & 0x01)
        {
           // File Number Check
           if (Cs2Area->workblock.fn != curfilter->fid)
              condresults = 0;
        }

        if (curfilter->mode & 0x02)
        {
           // Channel Number Check
           if (Cs2Area->workblock.cn != curfilter->chan)
              condresults = 0;
        }

        if (curfilter->mode & 0x04)
        {
           // Sub Mode Check
           if ((Cs2Area->workblock.sm & curfilter->smmask) != curfilter->smval)
              condresults = 0;
        }

        if (curfilter->mode & 0x08)
        {
           // Coding Information Check
#if CDDEBUG
           fprintf(stderr, "cs2\t: FilterData: Coding Information Check. Coding Information = %02X. Filter's Coding Information Mask = %02X, Coding Information Value = %02X\n", Cs2Area->workblock.ci, curfilter->cimask, curfilter->cival);
#endif
           if ((Cs2Area->workblock.ci & curfilter->cimask) != curfilter->cival)
              condresults = 0;
        }

        if (curfilter->mode & 0x10)
        {
           // Reverse Subheader Conditions
#if CDDEBUG
           fprintf(stderr, "cs2\t: FilterData: Reverse Subheader Conditions\n");
#endif
           condresults ^= 1;
        }
     }

     if (curfilter->mode & 0x40)
     {
        // FAD Range Check
        if (Cs2Area->workblock.FAD < curfilter->FAD ||
            Cs2Area->workblock.FAD > (curfilter->FAD+curfilter->range))
            condresults = 0;
     }

     if (condresults == 1)
     {
        Cs2Area->lastbuffer = curfilter->condtrue;
        fltpartition = &Cs2Area->partition[curfilter->condtrue];
        break;
     }
     else
     {
        Cs2Area->lastbuffer = curfilter->condfalse;

        if (curfilter->condfalse == 0xFF)
           return NULL;
        // loop and try filter that was connected to the false connector
        curfilter = &Cs2Area->filter[curfilter->condfalse];
     }
  }

  // Allocate block
  fltpartition->block[fltpartition->numblocks] = Cs2AllocateBlock(&fltpartition->blocknum[fltpartition->numblocks]);

  if (fltpartition->block[fltpartition->numblocks] == NULL)
    return NULL;

  // Copy workblock settings to allocated block
  fltpartition->block[fltpartition->numblocks]->size = Cs2Area->workblock.size;
  fltpartition->block[fltpartition->numblocks]->FAD = Cs2Area->workblock.FAD;
  fltpartition->block[fltpartition->numblocks]->cn = Cs2Area->workblock.cn;
  fltpartition->block[fltpartition->numblocks]->fn = Cs2Area->workblock.fn;
  fltpartition->block[fltpartition->numblocks]->sm = Cs2Area->workblock.sm;
  fltpartition->block[fltpartition->numblocks]->ci = Cs2Area->workblock.ci;

  // convert raw sector to type specified in getsectsize
  switch(Cs2Area->workblock.size)
  {
     case 2048: // user data only
                if (Cs2Area->workblock.data[0xF] == 0x02)
                   // m2f1
                   memcpy(fltpartition->block[fltpartition->numblocks]->data,
                          Cs2Area->workblock.data + 24, Cs2Area->workblock.size);
                else
                   // m1
                   memcpy(fltpartition->block[fltpartition->numblocks]->data,
                             Cs2Area->workblock.data + 16, Cs2Area->workblock.size);
                break;
     case 2324: // m2f2 user data only
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                       Cs2Area->workblock.data + 24, Cs2Area->workblock.size);
                break;
     case 2336: // m2f2 skip sync+header data
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                Cs2Area->workblock.data + 16, Cs2Area->workblock.size);
                break;
     case 2340: // m2f2 skip sync data
                memcpy(fltpartition->block[fltpartition->numblocks]->data,
                Cs2Area->workblock.data + 12, Cs2Area->workblock.size);
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


int Cs2CopyDirRecord(u8 * buffer, dirrec_struct * dirrec)
{
  u8 * temp_pointer;

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

int Cs2ReadFileSystem(filter_struct * curfilter, u32 fid, int isoffset)
{
   u32 fid_offset = 0;
   u8 * workbuffer;
   u32 i;
   dirrec_struct dirrec;
   u8 numsectorsleft = 0;
   u32 curdirlba = 0;
   partition_struct * rfspartition;
   u32 blocksectsize = Cs2Area->getsectsize;
 
   Cs2Area->outconcddev = curfilter;

   if (isoffset)
   {
      // readDirectory operation

      // make sure we have a valid current directory
      if (Cs2Area->curdirsect == 0) return -1;

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
         if ((rfspartition = Cs2ReadUnFilteredSector(166)) == NULL)
            return -2;

         blocksectsize = rfspartition->block[rfspartition->numblocks - 1]->size;

         // Retrieve directory record's lba
         Cs2CopyDirRecord(rfspartition->block[rfspartition->numblocks - 1]->data + 0x9C, &dirrec);

         // Free Block
         rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
         Cs2FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
         rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

         // Sort remaining blocks
         Cs2SortBlocks(rfspartition);
         rfspartition->numblocks -= 1;

         curdirlba = Cs2Area->curdirsect = dirrec.lba;
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
  if ((rfspartition = Cs2ReadUnFilteredSector(curdirlba+150)) == NULL)
        return -2;

  curdirlba++;
  workbuffer = rfspartition->block[rfspartition->numblocks - 1]->data;

  // Fill in first two entries of fileinfo
  for (i = 0; i < 2; i++)
  {
     Cs2CopyDirRecord(workbuffer, Cs2Area->fileinfo + i);
     Cs2Area->fileinfo[i].lba += 150;
     workbuffer += Cs2Area->fileinfo[i].recordsize;

     if (workbuffer[0] == 0)
     {
        Cs2Area->numfiles = i;
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
     Cs2CopyDirRecord(workbuffer, Cs2Area->fileinfo + i);
     Cs2Area->fileinfo[i].lba += 150;
     workbuffer += Cs2Area->fileinfo[i].recordsize;

     if (workbuffer[0] == 0)
     {
        if (numsectorsleft > 0)
        {
           // Free previous read sector
           rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
           Cs2FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
           rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

           // Sort remaining blocks
           Cs2SortBlocks(rfspartition);
           rfspartition->numblocks -= 1;

           // Read in next sector of directory record
           if ((rfspartition = Cs2ReadUnFilteredSector(dirrec.lba+150)) == NULL)
              return -2;
           dirrec.lba++;
           numsectorsleft--;
           workbuffer = rfspartition->block[rfspartition->numblocks - 1]->data;
        }
        else
        {
           Cs2Area->numfiles = i;
           break;
        }
     }
  }

  // Free the remaining sector
  rfspartition->size -= rfspartition->block[rfspartition->numblocks - 1]->size;
  Cs2FreeBlock(rfspartition->block[rfspartition->numblocks - 1]);
  rfspartition->blocknum[rfspartition->numblocks - 1] = 0xFF;

  // Sort remaining blocks
  Cs2SortBlocks(rfspartition);
  rfspartition->numblocks -= 1;

//#if CDDEBUG
//  for (i = 0; i < MAX_FILES; i++)
//  {
//     fprintf(stderr, "fileinfo[%d].name = %s\n", i, fileinfo[i].name);
//  }
//#endif

  return 0;
}

void Cs2SetupFileInfoTransfer(unsigned long fid) {
  Cs2Area->transfileinfo[0] = (Cs2Area->fileinfo[fid].lba & 0xFF000000) >> 24;
  Cs2Area->transfileinfo[1] = (Cs2Area->fileinfo[fid].lba & 0x00FF0000) >> 16;
  Cs2Area->transfileinfo[2] = (Cs2Area->fileinfo[fid].lba & 0x0000FF00) >> 8;
  Cs2Area->transfileinfo[3] =  Cs2Area->fileinfo[fid].lba & 0x000000FF;

  Cs2Area->transfileinfo[4] = (Cs2Area->fileinfo[fid].size & 0xFF000000) >> 24;
  Cs2Area->transfileinfo[5] = (Cs2Area->fileinfo[fid].size & 0x00FF0000) >> 16;
  Cs2Area->transfileinfo[6] = (Cs2Area->fileinfo[fid].size & 0x0000FF00) >> 8;
  Cs2Area->transfileinfo[7] =  Cs2Area->fileinfo[fid].size & 0x000000FF;

  Cs2Area->transfileinfo[8] = Cs2Area->fileinfo[fid].interleavegapsize;
  Cs2Area->transfileinfo[9] = Cs2Area->fileinfo[fid].fileunitsize;
  Cs2Area->transfileinfo[10] = (u8) fid;
  Cs2Area->transfileinfo[11] = Cs2Area->fileinfo[fid].flags;
}

partition_struct * Cs2ReadUnFilteredSector(unsigned long rufsFAD) {
  partition_struct * rufspartition;
  char syncheader[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0x00};

  if ((rufspartition = Cs2GetPartition(Cs2Area->outconcddev)) != NULL && !Cs2Area->isbufferfull)
  {
     // Allocate Block
     rufspartition->block[rufspartition->numblocks] = Cs2AllocateBlock(&rufspartition->blocknum[rufspartition->numblocks]);

     if (rufspartition->block[rufspartition->numblocks] == NULL)
        return NULL;

     // read a sector using cd interface function
     if (!Cs2Area->cdi->ReadSectorFAD(rufsFAD, Cs2Area->workblock.data))
        return NULL;

     // convert raw sector to type specified in getsectsize
     switch(Cs2Area->getsectsize)
     {
        case 2048: // user data only
                   if (Cs2Area->workblock.data[0xF] == 0x02)
                   {
                      // is it form1/form2 data?
                      if (!(Cs2Area->workblock.data[0x12] & 0x20))
                      {
                         // form 1
                         memcpy(rufspartition->block[rufspartition->numblocks]->data,
                                Cs2Area->workblock.data + 24, 2048);
                         Cs2Area->workblock.size = Cs2Area->getsectsize;
                      }
                      else
                      {
                         // form 2
                         memcpy(rufspartition->block[rufspartition->numblocks]->data,
                                Cs2Area->workblock.data + 24, 2324);
                         Cs2Area->workblock.size = 2324;
                      }
                   }
                   else
                   {
                      memcpy(rufspartition->block[rufspartition->numblocks]->data,
                             Cs2Area->workblock.data + 16, 2048);
                      Cs2Area->workblock.size = Cs2Area->getsectsize;
                   }
                   break;
        case 2336: // skip sync+header data
                   memcpy(rufspartition->block[rufspartition->numblocks]->data,
                   Cs2Area->workblock.data + 16, 2336);
                   Cs2Area->workblock.size = Cs2Area->getsectsize;
                   break;
        case 2340: // skip sync data
                   memcpy(rufspartition->block[rufspartition->numblocks]->data,
                   Cs2Area->workblock.data + 12, 2340);
                   Cs2Area->workblock.size = Cs2Area->getsectsize;
                   break;
        case 2352: // no conversion needed
                   Cs2Area->workblock.size = Cs2Area->getsectsize;
                   break;
        default: break;
     }

     // if mode 2 track, setup the subheader values
     if (memcmp(syncheader, Cs2Area->workblock.data, 12) == 0 &&
         Cs2Area->workblock.data[0xF] == 0x02)
     {
        rufspartition->block[rufspartition->numblocks]->fn = Cs2Area->workblock.data[0x10];
        rufspartition->block[rufspartition->numblocks]->cn = Cs2Area->workblock.data[0x11];
        rufspartition->block[rufspartition->numblocks]->sm = Cs2Area->workblock.data[0x12];
        rufspartition->block[rufspartition->numblocks]->ci = Cs2Area->workblock.data[0x13];
     }

     Cs2Area->workblock.FAD = rufsFAD;

     // Modify Partition values
     if (rufspartition->size == -1) rufspartition->size = 0;
     rufspartition->size += rufspartition->block[rufspartition->numblocks]->size;
     rufspartition->numblocks++;

     return rufspartition;
  }

  return NULL;
}

partition_struct * Cs2ReadFilteredSector(unsigned long rfsFAD) {
  char syncheader[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0x00};
  int isaudio = 0;

  if (Cs2Area->outconcddev != NULL && !Cs2Area->isbufferfull)
  {     
     // read a sector using cd interface function to workblock.data
     if (!Cs2Area->cdi->ReadSectorFAD(rfsFAD, Cs2Area->workblock.data)) 
        return NULL;

     Cs2Area->workblock.size = Cs2Area->getsectsize;
     Cs2Area->workblock.FAD = rfsFAD;

     if (memcmp(syncheader, Cs2Area->workblock.data, 12) != 0) isaudio = 1;

     // if mode 2 track, setup the subheader values
     if (!isaudio &&
         Cs2Area->workblock.data[0xF] == 0x02)
     {
        // if it's form 2 data the sector size should be 2324
        if (Cs2Area->workblock.data[0x12] & 0x20) Cs2Area->workblock.size = 2324;

        Cs2Area->workblock.fn = Cs2Area->workblock.data[0x10];
        Cs2Area->workblock.cn = Cs2Area->workblock.data[0x11];
        Cs2Area->workblock.sm = Cs2Area->workblock.data[0x12];
        Cs2Area->workblock.ci = Cs2Area->workblock.data[0x13];
     }

     // pass workblock to filter function(after it identifies partition,
     // it should allocate the partition block, setup/change the partition
     // values, and copy workblock to the allocated block)
     return Cs2FilterData(Cs2Area->outconcddev, isaudio);
  }

  return NULL;
}

unsigned char Cs2GetRegionID() {
   partition_struct * gripartition;
   char ret = 0;

   Cs2Area->outconcddev = Cs2Area->filter + 0;
   Cs2Area->outconcddevnum = 0;

   // read in lba 0/FAD 150
   if ((gripartition = Cs2ReadUnFilteredSector(150)) != NULL)
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
      Cs2FreeBlock(gripartition->block[gripartition->numblocks - 1]);
      gripartition->blocknum[gripartition->numblocks - 1] = 0xFF;

      // Sort remaining blocks
      Cs2SortBlocks(gripartition);
      gripartition->numblocks -= 1;
   }

   return ret;
}

int Cs2saveState(FILE * fp) {
   int offset, i;

   // This is mostly kludge, but it will have to do until I have time to rewrite it all

   offset = StateWriteHeader(fp, "CS2 ", 1);

   // Write cart type
   fwrite((void *) &Cs2Area->carttype, 4, 1, fp);

   // Write cd block registers
   fwrite((void *) &Cs2Area->reg, sizeof(blockregs_struct), 1, fp);

   // Write current Status variables(needs a rewrite)
   fwrite((void *) &Cs2Area->FAD, 4, 1, fp);
   fwrite((void *) &Cs2Area->status, 1, 1, fp);
   fwrite((void *) &Cs2Area->options, 1, 1, fp);
   fwrite((void *) &Cs2Area->repcnt, 1, 1, fp);
   fwrite((void *) &Cs2Area->ctrladdr, 1, 1, fp);
   fwrite((void *) &Cs2Area->track, 1, 1, fp);
   fwrite((void *) &Cs2Area->index, 1, 1, fp);

   // Write other cd block internal variables
   fwrite((void *) &Cs2Area->satauth, 2, 1, fp);
   fwrite((void *) &Cs2Area->mpgauth, 2, 1, fp);
   fwrite((void *) &Cs2Area->transfercount, 4, 1, fp);
   fwrite((void *) &Cs2Area->cdwnum, 4, 1, fp);
   fwrite((void *) Cs2Area->TOC, 4, 102, fp);
   fwrite((void *) &Cs2Area->playFAD, 4, 1, fp);
   fwrite((void *) &Cs2Area->playendFAD, 4, 1, fp);
   fwrite((void *) &Cs2Area->getsectsize, 4, 1, fp);
   fwrite((void *) &Cs2Area->putsectsize, 4, 1, fp);
   fwrite((void *) &Cs2Area->calcsize, 4, 1, fp);
   fwrite((void *) &Cs2Area->infotranstype, 4, 1, fp);
   fwrite((void *) &Cs2Area->datatranstype, 4, 1, fp);
   fwrite((void *) &Cs2Area->isonesectorstored, 1, 1, fp);
   fwrite((void *) &Cs2Area->isdiskchanged, 1, 1, fp);
   fwrite((void *) &Cs2Area->isbufferfull, 1, 1, fp);
   fwrite((void *) &Cs2Area->speed1x, 1, 1, fp);
   fwrite((void *) &Cs2Area->transfileinfo, 1, 12, fp);
   fwrite((void *) &Cs2Area->lastbuffer, 1, 1, fp);
   fwrite((void *) &Cs2Area->_command, 1, 1, fp);
   fwrite((void *) &Cs2Area->_periodictiming, 4, 1, fp);
   fwrite((void *) &Cs2Area->_commandtiming, 4, 1, fp);
   fwrite((void *) &Cs2Area->outconcddevnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->outconmpegfbnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->outconmpegbufnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->outconmpegromnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->outconhostnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->datatranspartitionnum, 1, 1, fp);
   fwrite((void *) &Cs2Area->datatransoffset, 4, 1, fp);
   fwrite((void *) &Cs2Area->datanumsecttrans, 4, 1, fp);
   fwrite((void *) &Cs2Area->datatranssectpos, 2, 1, fp);
   fwrite((void *) &Cs2Area->datasectstotrans, 2, 1, fp);
   fwrite((void *) &Cs2Area->blockfreespace, 4, 1, fp);
   fwrite((void *) &Cs2Area->curdirsect, 4, 1, fp);

   // Write CD buffer
   fwrite((void *)Cs2Area->block, sizeof(block_struct), MAX_BLOCKS, fp);

   // Write partition data
   for (i = 0; i < MAX_SELECTORS; i++)
   {
      fwrite((void *)&Cs2Area->partition[i].size, 4, 1, fp);
      fwrite((void *)Cs2Area->partition[i].blocknum, 1, MAX_BLOCKS, fp);
      fwrite((void *)&Cs2Area->partition[i].numblocks, 1, 1, fp);
   }

   // Write filter data
   fwrite((void *)Cs2Area->filter, sizeof(filter_struct), MAX_SELECTORS, fp);

   // Write File Info Table
   fwrite((void *)Cs2Area->fileinfo, sizeof(dirrec_struct), MAX_FILES, fp);

   // Write MPEG card registers here

   // Write current MPEG card status variables
   fwrite((void *)&Cs2Area->actionstatus, 1, 1, fp);
   fwrite((void *)&Cs2Area->pictureinfo, 1, 1, fp);
   fwrite((void *)&Cs2Area->mpegaudiostatus, 1, 1, fp);
   fwrite((void *)&Cs2Area->mpegvideostatus, 2, 1, fp);
   fwrite((void *)&Cs2Area->vcounter, 2, 1, fp);

   // Write other MPEG card internal variables
   fwrite((void *)&Cs2Area->mpegintmask, 4, 1, fp);
   fwrite((void *)Cs2Area->mpegcon, sizeof(mpegcon_struct), 2, fp);
   fwrite((void *)Cs2Area->mpegstm, sizeof(mpegstm_struct), 2, fp);

   return StateFinishHeader(fp, offset);
}

int Cs2loadState(FILE * fp, int version, int size) {
   int i, i2;

   // This is mostly kludge, but it will have to do until I have time to rewrite it all

   // Read cart type
   fread((void *)&Cs2Area->carttype, 4, 1, fp);

   // Read cd block registers
   fread((void *)&Cs2Area->reg, sizeof(blockregs_struct), 1, fp);

   // Read current Status variables(needs a reRead)
   fread((void *)&Cs2Area->FAD, 4, 1, fp);
   fread((void *)&Cs2Area->status, 1, 1, fp);
   fread((void *)&Cs2Area->options, 1, 1, fp);
   fread((void *)&Cs2Area->repcnt, 1, 1, fp);
   fread((void *)&Cs2Area->ctrladdr, 1, 1, fp);
   fread((void *)&Cs2Area->track, 1, 1, fp);
   fread((void *)&Cs2Area->index, 1, 1, fp);

   // Read other cd block internal variables
   fread((void *)&Cs2Area->satauth, 2, 1, fp);
   fread((void *)&Cs2Area->mpgauth, 2, 1, fp);
   fread((void *)&Cs2Area->transfercount, 4, 1, fp);
   fread((void *)&Cs2Area->cdwnum, 4, 1, fp);
   fread((void *)Cs2Area->TOC, 4, 102, fp);
   fread((void *)&Cs2Area->playFAD, 4, 1, fp);
   fread((void *)&Cs2Area->playendFAD, 4, 1, fp);
   fread((void *)&Cs2Area->getsectsize, 4, 1, fp);
   fread((void *)&Cs2Area->putsectsize, 4, 1, fp);
   fread((void *)&Cs2Area->calcsize, 4, 1, fp);
   fread((void *)&Cs2Area->infotranstype, 4, 1, fp);
   fread((void *)&Cs2Area->datatranstype, 4, 1, fp);
   fread((void *)&Cs2Area->isonesectorstored, 1, 1, fp);
   fread((void *)&Cs2Area->isdiskchanged, 1, 1, fp);
   fread((void *)&Cs2Area->isbufferfull, 1, 1, fp);
   fread((void *)&Cs2Area->speed1x, 1, 1, fp);
   fread((void *)&Cs2Area->transfileinfo, 1, 12, fp);
   fread((void *)&Cs2Area->lastbuffer, 1, 1, fp);
   fread((void *)&Cs2Area->_command, 1, 1, fp);
   fread((void *)&Cs2Area->_periodictiming, 4, 1, fp);
   fread((void *)&Cs2Area->_commandtiming, 4, 1, fp);
   fread((void *)&Cs2Area->outconcddevnum, 1, 1, fp);
   if (Cs2Area->outconcddevnum == 0xFF)
      Cs2Area->outconcddev = NULL;
   else
      Cs2Area->outconcddev = Cs2Area->filter + Cs2Area->outconcddevnum;

   fread((void *)&Cs2Area->outconmpegfbnum, 1, 1, fp);
   if (Cs2Area->outconmpegfbnum == 0xFF)
      Cs2Area->outconmpegfb = NULL;
   else
      Cs2Area->outconmpegfb = Cs2Area->filter + Cs2Area->outconmpegfbnum;

   fread((void *)&Cs2Area->outconmpegbufnum, 1, 1, fp);
   if (Cs2Area->outconmpegbufnum == 0xFF)
      Cs2Area->outconmpegbuf = NULL;
   else
      Cs2Area->outconmpegbuf = Cs2Area->filter + Cs2Area->outconmpegbufnum;

   fread((void *)&Cs2Area->outconmpegromnum, 1, 1, fp);
   if (Cs2Area->outconmpegromnum == 0xFF)
      Cs2Area->outconmpegrom = NULL;
   else
      Cs2Area->outconmpegrom = Cs2Area->filter + Cs2Area->outconmpegromnum;

   fread((void *)&Cs2Area->outconhostnum, 1, 1, fp);
   if (Cs2Area->outconhostnum == 0xFF)
      Cs2Area->outconhost = NULL;
   else
      Cs2Area->outconhost = Cs2Area->filter + Cs2Area->outconhostnum;

   fread((void *)&Cs2Area->datatranspartitionnum, 1, 1, fp);
   fread((void *)&Cs2Area->datatransoffset, 4, 1, fp);
   fread((void *)&Cs2Area->datanumsecttrans, 4, 1, fp);
   fread((void *)&Cs2Area->datatranssectpos, 2, 1, fp);
   fread((void *)&Cs2Area->datasectstotrans, 2, 1, fp);
   fread((void *)&Cs2Area->blockfreespace, 4, 1, fp);
   fread((void *)&Cs2Area->curdirsect, 4, 1, fp);

   // Read CD buffer
   fread((void *)Cs2Area->block, sizeof(block_struct), MAX_BLOCKS, fp);

   // Read partition data
   for (i = 0; i < MAX_SELECTORS; i++)
   {
      fread((void *)&Cs2Area->partition[i].size, 4, 1, fp);
      fread((void *)Cs2Area->partition[i].blocknum, 1, MAX_BLOCKS, fp);
      fread((void *)&Cs2Area->partition[i].numblocks, 1, 1, fp);

      for (i2 = 0; i2 < MAX_BLOCKS; i2++)
      {
         if (Cs2Area->partition[i].blocknum[i2] == 0xFF)
            Cs2Area->partition[i].block[i2] = NULL;
         else
            Cs2Area->partition[i].block[i2] = Cs2Area->block + Cs2Area->partition[i].blocknum[i2];
      }
   }

   // Read filter data
   fread((void *)Cs2Area->filter, sizeof(filter_struct), MAX_SELECTORS, fp);

   // Read File Info Table
   fread((void *)Cs2Area->fileinfo, sizeof(dirrec_struct), MAX_FILES, fp);

   // Read MPEG card registers here

   // Read current MPEG card status variables
   fread((void *)&Cs2Area->actionstatus, 1, 1, fp);
   fread((void *)&Cs2Area->pictureinfo, 1, 1, fp);
   fread((void *)&Cs2Area->mpegaudiostatus, 1, 1, fp);
   fread((void *)&Cs2Area->mpegvideostatus, 2, 1, fp);
   fread((void *)&Cs2Area->vcounter, 2, 1, fp);

   // Read other MPEG card internal variables
   fread((void *)&Cs2Area->mpegintmask, 4, 1, fp);
   fread((void *)Cs2Area->mpegcon, sizeof(mpegcon_struct), 2, fp);
   fread((void *)Cs2Area->mpegstm, sizeof(mpegstm_struct), 2, fp);

   return size;
}

