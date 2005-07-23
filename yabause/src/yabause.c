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

#include <sys/types.h>
#include "yabause.h"
#include "cs0.h"
#include "cs2.h"
#include "debug.h"
#include "memory.h"
#include "SDL.h"
#include "scsp.h"
#include "scu.h"
#include "sh2core.h"
#include "smpc.h"
#include "vdp2.h"
#include "yui.h"

//////////////////////////////////////////////////////////////////////////////

unsigned short buttonbits = 0xFFFF;

yabsys_struct yabsys;

//////////////////////////////////////////////////////////////////////////////

void keyDown(int key)
{
  switch (key) {
        case SDLK_RIGHT:
		buttonbits &= 0x7FFF;
                LOG("Right\n");
		break;
        case SDLK_LEFT:
		buttonbits &= 0xBFFF;
                LOG("Left\n");
		break;
        case SDLK_DOWN:
		buttonbits &= 0xDFFF;
                LOG("Down\n");
		break;
        case SDLK_UP:
		buttonbits &= 0xEFFF;
                LOG("Up\n");
		break;
	case SDLK_j:
		buttonbits &= 0xF7FF;
                LOG("Start\n");
		break;
	case SDLK_k:
		buttonbits &= 0xFBFF;
                LOG("A\n");
		break;
	case SDLK_m:
		buttonbits &= 0xFDFF;
                LOG("C\n");
		break;
	case SDLK_l:
		buttonbits &= 0xFEFF;
                LOG("B\n");
		break;
	case SDLK_z:
		buttonbits &= 0xFF7F;
                LOG("Right Trigger\n");
		break;
	case SDLK_u:
		buttonbits &= 0xFFBF;
                LOG("X\n");
		break;
	case SDLK_i:
		buttonbits &= 0xFFDF;
                LOG("Y\n");
		break;
	case SDLK_o:
		buttonbits &= 0xFFEF;
                LOG("Z\n");
		break;
	case SDLK_x:
		buttonbits &= 0xFFF7;
                LOG("Left Trigger\n");
		break;
	default:
		break;
  }
}

//////////////////////////////////////////////////////////////////////////////

void keyUp(int key)
{
  switch (key) {
        case SDLK_RIGHT:
		buttonbits |= ~0x7FFF;
		break;
        case SDLK_LEFT:
		buttonbits |= ~0xBFFF;
		break;
        case SDLK_DOWN:
		buttonbits |= ~0xDFFF;
		break;
        case SDLK_UP:
		buttonbits |= ~0xEFFF;
		break;
	case SDLK_j:
		buttonbits |= ~0xF7FF;
		break;
	case SDLK_k:
		buttonbits |= ~0xFBFF;
		break;
	case SDLK_m:
		buttonbits |= ~0xFDFF;
		break;
	case SDLK_l:
		buttonbits |= ~0xFEFF;
		break;
	case SDLK_z:
		buttonbits |= ~0xFF7F;
		break;
	case SDLK_u:
		buttonbits |= ~0xFFBF;
		break;
	case SDLK_i:
		buttonbits |= ~0xFFDF;
		break;
	case SDLK_o:
		buttonbits |= ~0xFFEF;
		break;
	case SDLK_x:
		buttonbits |= ~0xFFF7;
		break;
	default:
		break;
  }
}

//////////////////////////////////////////////////////////////////////////////

int handleEvents() {
    SDL_Event event;
      if (SDL_PollEvent(&event)) {
	switch(event.type) {
	case SDL_QUIT:
                YuiQuit();
		break;
	case SDL_KEYDOWN:
	  switch(event.key.keysym.sym) {
                case SDLK_F1:
                        break;

                case SDLK_F2:
                        break;

                case SDLK_F4:
                        break;

                case SDLK_1:
                        break;

                case SDLK_2:
			break;

                case SDLK_3:
			break;

                case SDLK_4:
			break;

                case SDLK_5:
			break;

                case SDLK_6:
			break;

		case SDLK_h:
                        YuiHideShow();
			break;
			
		case SDLK_q:
                        YuiQuit();
                        break;
		case SDLK_p:
			break;
		case SDLK_t:
			break;
		case SDLK_r:
			break;
		default:
			keyDown(event.key.keysym.sym);
			break;
	  }
	  break;
	case SDL_KEYUP:
	  switch(event.key.keysym.sym) {
		case SDLK_q:
		case SDLK_p:
		case SDLK_r:
			break;
		default:
			keyUp(event.key.keysym.sym);
			break;
	  }
	  break;
	default:
		break;
	}
      }
      else {
         if (YabauseExec() != 0)
            return -1;
      }
      return 0;
}

//////////////////////////////////////////////////////////////////////////////

void print_usage(const char *program_name) {
}

//////////////////////////////////////////////////////////////////////////////

void YabauseChangeTiming(int freqtype) {
   // Setup all the variables related to timing
   yabsys.DecilineCount = 0;
   yabsys.LineCount = 0;

   switch (freqtype)
   {
      case CLKTYPE_26MHZNTSC:
         yabsys.DecilineStop = 26846587 / 263 / 10 / 60; // I'm guessing this is wrong
         yabsys.Duf = 26846587 / 100000;
         break;
      case CLKTYPE_28MHZNTSC:
         yabsys.DecilineStop = 28636360 / 263 / 10 / 60; // I'm guessing this is wrong
         yabsys.Duf = 28636360 / 100000;                         
         break;
      case CLKTYPE_26MHZPAL:
         yabsys.DecilineStop = 26846587 / 312 / 10 / 50; // I'm guessing this is wrong
         yabsys.Duf = 26846587 / 100000;
         break;
      case CLKTYPE_28MHZPAL:
         yabsys.DecilineStop = 28636360 / 312 / 10 / 50; // I'm guessing this is wrong
         yabsys.Duf = 28636360 / 100000;
         break;
      default: break;
   }

   yabsys.CycleCountII = 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabauseInit(int sh2coretype, int gfxcoretype, int sndcoretype,
                int cdcoretype, unsigned char regionid, const char *biospath,
                const char *cdpath, const char *savepath, const char *mpegpath) {
   // Initialize Both cores
   if (SH2Init(sh2coretype) != 0)
      return -1;

   if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
      return -1;

   if ((HighWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if ((LowWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   // Initialize CS0 area here
   // Initialize CS1 area here
   if (Cs2Init(CART_NONE, cdcoretype, cdpath, mpegpath) != 0)
      return -1;

   if (ScuInit() != 0)
      return -1;

   if (ScspInit(sndcoretype) != 0)
      return -1;

   if (Vdp1Init(gfxcoretype) != 0)
      return -1;

   if (Vdp2Init(gfxcoretype) != 0)
      return -1;

   if (SmpcInit(regionid) != 0)
      return -1;

   MappedMemoryInit();

   YabauseChangeTiming(CLKTYPE_26MHZNTSC);

   if (LoadBios(biospath) != 0)
      return -2;

   // Load save ram here

   YabauseReset();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabauseDeInit() {
   SH2DeInit();

   if (BiosRom)
      T2MemoryDeInit(BiosRom);

   if (HighWram)
      T2MemoryDeInit(HighWram);

   if (LowWram)
      T2MemoryDeInit(LowWram);

   // Free CS0 area here
   // Free CS1 area here
   Cs2DeInit();
   ScuDeInit();
   ScspDeInit();
   Vdp1DeInit();
   Vdp2DeInit();
   SmpcDeInit();
}

//////////////////////////////////////////////////////////////////////////////

void YabauseReset() {
   SH2Reset(MSH2);
   YabStopSlave();
   memset(HighWram, 0, 0x100000);
   memset(LowWram, 0, 0x100000);
   // Reset CS0 area here
   // Reset CS1 area here
   Cs2Reset();
   ScuReset();
   ScspReset();
   yabsys.IsM68KRunning = 0;
   Vdp1Reset();
   Vdp2Reset();
   SmpcReset();

   SH2PowerOn(MSH2);
}

//////////////////////////////////////////////////////////////////////////////

int YabauseExec() {
   SH2Exec(MSH2, yabsys.DecilineStop);
   if (yabsys.IsSSH2Running)
      SH2Exec(SSH2, yabsys.DecilineStop);

   yabsys.DecilineCount++;
   if(yabsys.DecilineCount == 9)
   {
      // HBlankIN
      Vdp2HBlankIN();
   }
   else if (yabsys.DecilineCount == 10)
   {
      // HBlankOUT
      Vdp2HBlankOUT();
      ScspExec();
      yabsys.DecilineCount = 0;
      yabsys.LineCount++;
      if (yabsys.LineCount == 224)
      {
         // VBlankIN
         SmpcINTBACKEnd();
         Vdp2VBlankIN();
      }
      else if (yabsys.LineCount == 263)
      {
         // VBlankOUT
         Vdp2VBlankOUT();
         yabsys.LineCount = 0;
      }
   }

   yabsys.CycleCountII += MSH2->cycles;

   while (yabsys.CycleCountII > yabsys.Duf)
   {
      SmpcExec(10);
      Cs2Exec(10);
//      msh->run(10);
//      ssh->run(10);
      ScuExec(10);
      yabsys.CycleCountII %= yabsys.Duf;
   }

   M68KExec(170);

   MSH2->cycles %= yabsys.DecilineStop;
   if (yabsys.IsSSH2Running) 
      SSH2->cycles %= yabsys.DecilineStop;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabStartSlave(void) {
   SH2PowerOn(SSH2);
   yabsys.IsSSH2Running = 1;
}

//////////////////////////////////////////////////////////////////////////////

void YabStopSlave(void) {
   SH2Reset(SSH2);
   yabsys.IsSSH2Running = 0;
}

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
   int i;
//   LogStart();

   //handle command line arguments
   for (i = 1; i < argc; ++i) {
      if (argv[i]) {
         //show usage
         //if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
         //   print_usage();
         //   return 0;
         //}
			
         //set bios
         if (0 == strcmp(argv[i], "-b") && argv[i + 1])
            YuiSetBiosFilename(argv[i + 1]);
         else if (strstr(argv[i], "--bios="))
            YuiSetBiosFilename(argv[i] + strlen("--bios="));
	
         //set iso
         else if (0 == strcmp(argv[i], "-i") && argv[i + 1])
            YuiSetIsoFilename(argv[i + 1]);
         else if (strstr(argv[i], "--iso="))
            YuiSetIsoFilename(argv[i] + strlen("--iso="));
      }
   }

   if (YuiInit(&handleEvents) != 0)
      fprintf(stderr, "Error running Yabause\n");

   YabauseDeInit();
   SDL_Quit();
//   LogStop();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
