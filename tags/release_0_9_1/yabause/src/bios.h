/*  Copyright 2006 Theo Berkau

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

#ifndef BIOS_H
#define BIOS_H

#include "sh2core.h"

typedef struct
{
   char filename[12];
   char comment[11];
   u8 language;
   u8 year;
   u8 month;
   u8 day;
   u8 hour;
   u8 minute;
   u8 week;
   u32 datasize;
   u16 blocksize;
} saveinfo_struct;

typedef struct
{
   u8 id;
   char name[32];
} deviceinfo_struct;

void BiosInit(void);
int FASTCALL BiosHandleFunc(SH2_struct * sh);

deviceinfo_struct *BupGetDeviceList(int *numdevices);
int BupGetStats(u32 device, u32 *freespace, u32 *maxspace);
saveinfo_struct *BupGetSaveList(u32 device, int *numsaves);
int BupDeleteSave(u32 device, const char *savename);
void BupFormat(u32 device);
int BupCopySave(u32 srcdevice, u32 dstdevice, const char *savename);
int BupImportSave(u32 device, const char *filename);
int BupExportSave(u32 device, const char *savename, const char *filename);
#endif
