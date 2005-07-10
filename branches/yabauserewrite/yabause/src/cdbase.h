/*  Copyright 2004-2005 Theo Berkau
    Copyright 2005 Joost Peters

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

#ifndef CDBASE_H
#define CDBASE_H

#include <stdio.h>

typedef struct
{
	void * (*New)(const char *);
	int (*Delete)(void *);
	long (*ReadTOC)(void *, unsigned long *);
	int (*GetStatus)(void *);
	int (*ReadSectorFAD)(void *, unsigned long, void *);
	const char * (*DeviceName)(void *);
} CDInterface;

void * DummyCDDriveNew(const char *);
int DummyCDDriveDelete(void *);
const char * DummyCDDriveDeviceName(void *);
int DummyCDDriveGetStatus(void *);
long DummyCDDriveReadTOC(void *, unsigned long *);
int DummyCDDriveReadSectorFAD(void *, unsigned long, void *);

CDInterface * DummyCDDrive(void);

typedef struct
{
	FILE * _isoFile;
	int _fileSize;
} _ISOCDDrive;

_ISOCDDrive * ISOCDDriveNew(const char *);
int ISOCDDriveDelete(_ISOCDDrive *);
const char * ISOCDDriveDeviceName(_ISOCDDrive *);
long ISOCDDriveReadTOC(_ISOCDDrive *, unsigned long *);
int ISOCDDriveGetStatus(_ISOCDDrive *);
int ISOCDDriveReadSectorFAD(_ISOCDDrive *, unsigned long, void *);

CDInterface * ISOCDDrive(void);

#endif
