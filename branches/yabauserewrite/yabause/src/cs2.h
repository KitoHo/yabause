/*  Copyright 2003 Guillaume Duhamel
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

#ifndef CS2_H
#define CS2_H

#include "memory.h"
#include "cdbase.h"
#include "cs0.h"

#define MAX_BLOCKS      200
#define MAX_SELECTORS   24
#define MAX_FILES       256

typedef struct
{
   long size;
   unsigned long FAD;
   unsigned char cn;
   unsigned char fn;
   unsigned char sm;
   unsigned char ci;
   unsigned char data[2352];
} block_struct;

typedef struct
{
   unsigned long FAD;
   unsigned long range;
   unsigned char mode;
   unsigned char chan;
   unsigned char smmask;
   unsigned char cimask;
   unsigned char fid;
   unsigned char smval;
   unsigned char cival;
   unsigned char condtrue;
   unsigned char condfalse;
} filter_struct;

typedef struct
{
   long size;
   block_struct *block[MAX_BLOCKS];
   unsigned char blocknum[MAX_BLOCKS];
   unsigned char numblocks;
} partition_struct;

typedef struct
{
  unsigned short groupid;
  unsigned short userid;
  unsigned short attributes;
  unsigned short signature;
  unsigned char filenumber;
  unsigned char reserved[5];
} xarec_struct;

typedef struct
{
  unsigned char recordsize;
  unsigned char xarecordsize;
  unsigned long lba;
  unsigned long size;
  unsigned char dateyear;
  unsigned char datemonth;
  unsigned char dateday;
  unsigned char datehour;
  unsigned char dateminute;
  unsigned char datesecond;
  unsigned char gmtoffset;
  unsigned char flags;
  unsigned char fileunitsize;
  unsigned char interleavegapsize;
  unsigned short volumesequencenumber;
  unsigned char namelength;
  char name[32];
  xarec_struct xarecord;
} dirrec_struct;

typedef struct
{
   unsigned char audcon;
   unsigned char audlay;
   unsigned char audbufdivnum;
   unsigned char vidcon;
   unsigned char vidlay;
   unsigned char vidbufdivnum;
} mpegcon_struct;

typedef struct
{
   unsigned char audstm;
   unsigned char audstmid;
   unsigned char audchannum;
   unsigned char vidstm;
   unsigned char vidstmid;
   unsigned char vidchannum;
} mpegstm_struct;

typedef struct
{
   unsigned long DTR;
   unsigned short UNKNOWN;
   unsigned short HIRQ;
   unsigned short HIRQMASK;
   unsigned short CR1;
   unsigned short CR2;
   unsigned short CR3;
   unsigned short CR4;
   unsigned short MPEGRGB;
} blockregs_struct;

typedef struct
{
   unsigned char RBR;
   unsigned char THR;
   unsigned char IER;
   unsigned char DLL;
   unsigned char DLM;
   unsigned char IIR;
   unsigned char FCR;
   unsigned char LCR;
   unsigned char MCR;
   unsigned char LSR;
   unsigned char MSR;
   unsigned char SCR;
} netlinkregs_struct;

typedef struct {
  blockregs_struct reg;
  unsigned long FAD;
  unsigned char status;

  // cd specific stats
  unsigned char options;
  unsigned char repcnt;
  unsigned char ctrladdr;
  unsigned char track;
  unsigned char index;

  // mpeg specific stats
  unsigned char actionstatus;
  unsigned char pictureinfo;
  unsigned char mpegaudiostatus;
  unsigned short mpegvideostatus;
  unsigned short vcounter;

  // authentication variables
  unsigned short satauth;
  unsigned short mpgauth;

  // internal varaibles
  unsigned long transfercount;
  unsigned long cdwnum;
  unsigned long TOC[102];
  unsigned long playFAD;
  unsigned long playendFAD;
  unsigned long getsectsize;
  unsigned long putsectsize;
  unsigned long calcsize;
  long infotranstype;
  long datatranstype;
  int isonesectorstored;
  int isdiskchanged;
  int isbufferfull;
  int speed1x;
  unsigned char transfileinfo[12];
  unsigned char lastbuffer;

  filter_struct filter[MAX_SELECTORS];
  filter_struct *outconcddev;
  filter_struct *outconmpegfb;
  filter_struct *outconmpegbuf;
  filter_struct *outconmpegrom;
  filter_struct *outconhost;
  unsigned char outconcddevnum;
  unsigned char outconmpegfbnum;
  unsigned char outconmpegbufnum;
  unsigned char outconmpegromnum;
  unsigned char outconhostnum;

  partition_struct partition[MAX_SELECTORS];

  partition_struct *datatranspartition;
  unsigned char datatranspartitionnum;
  long datatransoffset;
  unsigned long datanumsecttrans;
  unsigned short datatranssectpos;
  unsigned short datasectstotrans;

  unsigned long blockfreespace;
  block_struct block[MAX_BLOCKS];
  block_struct workblock;

  unsigned long curdirsect;
  dirrec_struct fileinfo[MAX_FILES];
  unsigned long numfiles;

  const char *mpegpath;

  unsigned long mpegintmask;

  mpegcon_struct mpegcon[2];
  mpegstm_struct mpegstm[2];

  int _command;
  unsigned long _periodiccycles;
  unsigned long _periodictiming;
  unsigned long _commandtiming;
  CDInterface * cdi;

  int carttype;

  netlinkregs_struct nlreg;
} Cs2;

extern Cs2 * Cs2Area;

int Cs2Init(int, int, const char *, const char *);
void Cs2DeInit(void);

u8 FASTCALL 	Cs2ReadByte(u32);
u16 FASTCALL 	Cs2ReadWord(u32);
u32 FASTCALL 	Cs2ReadLong(u32);
void FASTCALL 	Cs2WriteByte(u32, u8);
void FASTCALL 	Cs2WriteWord(u32, u16);
void FASTCALL 	Cs2WriteLong(u32, u32);

void Cs2Run(unsigned long);
void Cs2Execute(void);
void Cs2Reset(void);
void Cs2SetTiming(int);
void Cs2Command(void);
void Cs2SetCommandTiming(unsigned char cmd);

//   command name                             command code
void Cs2GetStatus(void);                   // 0x00
void Cs2GetHardwareInfo(void);             // 0x01
void Cs2GetToc(void);                      // 0x02
void Cs2GetSessionInfo();                  // 0x03
void Cs2InitializeCDSystem(void);          // 0x04
// Open Tray                               // 0x05
void Cs2EndDataTransfer(void);             // 0x06
void Cs2PlayDisc(void);                    // 0x10
void Cs2SeekDisc(void);                    // 0x11
// Scan Disc                               // 0x12
void Cs2GetSubcodeQRW(void);               // 0x20
void Cs2SetCDDeviceConnection(void);       // 0x30
// get CD Device Connection                // 0x31
void Cs2GetLastBufferDestination(void);    // 0x32
void Cs2SetFilterRange(void);              // 0x40
// get Filter Range                        // 0x41
void Cs2SetFilterSubheaderConditions(void);// 0x42
void Cs2GetFilterSubheaderConditions(void);// 0x43
void Cs2SetFilterMode(void);               // 0x44
void Cs2GetFilterMode(void);               // 0x45
void Cs2SetFilterConnection(void);         // 0x46
// Get Filter Connection                   // 0x47
void Cs2ResetSelector(void);               // 0x48
void Cs2GetBufferSize(void);               // 0x50
void Cs2GetSectorNumber(void);             // 0x51
void Cs2CalculateActualSize(void);         // 0x52
void Cs2GetActualSize(void);               // 0x53
void Cs2GetSectorInfo(void);               // 0x54
void Cs2SetSectorLength(void);             // 0x60
void Cs2GetSectorData(void);               // 0x61
void Cs2DeleteSectorData(void);            // 0x62
void Cs2GetThenDeleteSectorData(void);     // 0x63
void Cs2PutSectorData(void);               // 0x64
// Copy Sector Data                        // 0x65
// Move Sector Data                        // 0x66
void Cs2GetCopyError(void);                // 0x67
void Cs2ChangeDirectory(void);             // 0x70
void Cs2ReadDirectory(void);               // 0x71
void Cs2GetFileSystemScope(void);          // 0x72
void Cs2GetFileInfo(void);                 // 0x73
void Cs2ReadFile(void);                    // 0x74
void Cs2AbortFile(void);                   // 0x75
void Cs2MpegGetStatus(void);               // 0x90
void Cs2MpegGetInterrupt(void);            // 0x91
void Cs2MpegSetInterruptMask(void);        // 0x92
void Cs2MpegInit(void);                    // 0x93
void Cs2MpegSetMode(void);                 // 0x94
void Cs2MpegPlay(void);                    // 0x95
void Cs2MpegSetDecodingMethod(void);       // 0x96
// MPEG Out Decoding Sync                  // 0x97
// MPEG Get Timecode                       // 0x98
// MPEG Get Pts                            // 0x99
void Cs2MpegSetConnection(void);           // 0x9A
void Cs2MpegGetConnection(void);           // 0x9B
// MPEG Change Connection                  // 0x9C
void Cs2MpegSetStream(void);               // 0x9D
void Cs2MpegGetStream(void);               // 0x9E
// MPEG Get Picture Size                   // 0x9F
void Cs2MpegDisplay(void);                 // 0xA0
void Cs2MpegSetWindow(void);               // 0xA1
void Cs2MpegSetBorderColor(void);          // 0xA2
void Cs2MpegSetFade(void);                 // 0xA3
void Cs2MpegSetVideoEffects(void);         // 0xA4
// MPEG Get Image                          // 0xA5
// MPEG Set Image                          // 0xA6
// MPEG Read Image                         // 0xA7
// MPEG Write Image                        // 0xA8
// MPEG Read Sector                        // 0xA9
// MPEG Write Sector                       // 0xAA
// MPEG Get LSI                            // 0xAE
void Cs2MpegSetLSI(void);                  // 0xAF
void Cs2CmdE0(void);                       // 0xE0
void Cs2CmdE1(void);                       // 0xE1
void Cs2CmdE2(void);                       // 0xE2

u8 Cs2FADToTrack(u32);
u32 Cs2TrackToFAD(u16);
void Cs2SetupDefaultPlayStats(u8);
block_struct * Cs2AllocateBlock(unsigned char *);
void Cs2FreeBlock(block_struct *);
void Cs2SortBlocks(partition_struct *);
partition_struct * Cs2GetPartition(filter_struct *);
partition_struct * Cs2FilterData(filter_struct *, int);
int Cs2CopyDirRecord(unsigned char *, dirrec_struct *);
int Cs2ReadFileSystem(filter_struct *, unsigned long, int);
void Cs2SetupFileInfoTransfer(unsigned long);
partition_struct * Cs2ReadUnFilteredSector(unsigned long);
partition_struct * Cs2ReadFilteredSector(unsigned long);
unsigned char Cs2GetRegionID(void);

int Cs2SaveState(FILE *);
int Cs2LoadState(FILE *, int, int);

#endif
