/*  Copyright 2003-2004 Guillaume Duhamel
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

// SH2 Shared Code
#include <stdlib.h>
#include "sh2core.h"

SH2_struct *MSH2=NULL;
SH2_struct *SSH2=NULL;
SH2Interface_struct *SH2Core=NULL;
extern SH2Interface_struct *SH2CoreList[];

//////////////////////////////////////////////////////////////////////////////

int SH2Init(int coreid)
{
   // MSH2
   if ((MSH2 = (SH2_struct *)malloc(sizeof(SH2_struct))) == NULL)
      return -1;

   if ((MSH2->onchip = (Onchip_struct *)malloc(sizeof(Onchip_struct))) == NULL)
      return -1;

   // SSH2
   if ((SSH2 = (SH2_struct *)malloc(sizeof(SH2_struct))) == NULL)
      return -1;

   if ((SSH2->onchip = (Onchip_struct *)malloc(sizeof(Onchip_struct))) == NULL)
      return -1;

   if ((SH2Core = (SH2Interface_struct *)malloc(sizeof(SH2Interface_struct))) == NULL)
      return -1;

   // So which core do we want?
   if (coreid == SH2CORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (int i = 0; SH2CoreList[i] != NULL; i++)
   {
      if (SH2CoreList[i]->id == coreid)
      {
         // Set to current core
         SH2Core = SH2CoreList[i];
         break;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2DeInit()
{
   if (MSH2)
   {
      if (MSH2->onchip)
         free(MSH2->onchip);

      free(MSH2);
   }

   if (SSH2)
   {
      if (SSH2->onchip)
         free(SSH2->onchip);

      free(SSH2);
   }

   if (SH2Core)
      free(SH2Core);
}

//////////////////////////////////////////////////////////////////////////////

void SH2Reset(SH2_struct *context)
{
   // Reset general registers
   for (int i = 0; i < 15; i++)
      context->regs.R[i] = 0x00000000;
                   
   context->regs.SR.all = 0x000000F0;
   context->regs.GBR = 0x00000000;
   context->regs.VBR = 0x00000000;
   context->regs.MACH = 0x00000000;
   context->regs.MACL = 0x00000000;
   context->regs.PR = 0x00000000;

   // Get PC/Stack initial values(fix me)

   // Internal variables
   context->delay = 0x00000000;
   context->cycles = 0;

   // Core specific reset
   SH2Core->Reset();
}

//////////////////////////////////////////////////////////////////////////////
