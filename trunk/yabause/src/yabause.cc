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

#include "superh.hh"
#include "smpc.hh"
#include "cs2.hh"
#include "intc.hh"
#include "vdp1.hh"
#include "vdp2.hh"
#include <sys/types.h>
#include "cd.hh"
#include "yui.hh"

unsigned short buttonbits = 0xFFFF;

void keyDown(int key)
{
  switch (key) {
	case SDLK_f:
		buttonbits &= 0x7FFF;
#ifdef DEBUG
		cerr << "Right" << endl;
#endif
		break;
	case SDLK_s:
		buttonbits &= 0xBFFF;
#ifdef DEBUG
		cerr << "Left" << endl;
#endif
		break;
	case SDLK_d:
		buttonbits &= 0xDFFF;
#ifdef DEBUG
		cerr << "Down" << endl;
#endif
		break;
	case SDLK_e:
		buttonbits &= 0xEFFF;
#ifdef DEBUG
		cerr << "Up" << endl;
#endif
		break;
	case SDLK_j:
		buttonbits &= 0xF7FF;
#ifdef DEBUG
		cerr << "Start" << endl;
#endif
		break;
	case SDLK_k:
		buttonbits &= 0xFBFF;
#ifdef DEBUG
		cerr << "A" << endl;
#endif
		break;
	case SDLK_m:
		buttonbits &= 0xFDFF;
#ifdef DEBUG
		cerr << "C" << endl;
#endif
		break;
	case SDLK_l:
		buttonbits &= 0xFEFF;
#ifdef DEBUG
		cerr << "B" << endl;
#endif
		break;
	case SDLK_z:
		buttonbits &= 0xFF7F;
#ifdef DEBUG
		cerr << "Right Trigger" << endl;
#endif
		break;
	case SDLK_u:
		buttonbits &= 0xFFBF;
#ifdef DEBUG
		cerr << "X" << endl;
#endif
		break;
	case SDLK_i:
		buttonbits &= 0xFFDF;
#ifdef DEBUG
		cerr << "Y" << endl;
#endif
		break;
	case SDLK_o:
		buttonbits &= 0xFFEF;
#ifdef DEBUG
		cerr << "Z" << endl;
#endif
		break;
	case SDLK_x:
		buttonbits &= 0xFFF7;
#ifdef DEBUG
		cerr << "LTrig" << endl;
#endif
		break;
	default:
		break;
  }
}

void keyUp(int key)
{
  switch (key) {
	case SDLK_f:
		buttonbits |= ~0x7FFF;
		break;
	case SDLK_s:
		buttonbits |= ~0xBFFF;
		break;
	case SDLK_d:
		buttonbits |= ~0xDFFF;
		break;
	case SDLK_e:
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

int handleEvents(SaturnMemory *mem) {
    SDL_Event event;
    SuperH *msh = mem->getMasterSH();
      if (SDL_PollEvent(&event)) {
	switch(event.type) {
	case SDL_QUIT:
		yui_quit();
		break;
	case SDL_KEYDOWN:
	  switch(event.key.keysym.sym) {
		case SDLK_h:
			yui_hide_show();
			break;
			
		case SDLK_q:
			mem->stop();
			yui_quit();
			break;
		case SDLK_p:
#ifdef DEBUG
			cerr << "Pause" << endl;
#endif
			msh->pause();

			break;
		case SDLK_r:
#ifdef DEBUG
			cerr << "Run" << endl;
#endif
			if (mem->running()) {
				msh->run();
			}
			else {
				mem->start();
			}
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
	if (msh->paused());
	else ((Vdp2 *) mem->getVdp2())->executer();
      }
      return 1;
}

int main(int argc, char **argv) {
	SDL_Init(SDL_INIT_EVENTTHREAD);

	yui_init((int (*)(void*)) &handleEvents);

#if DEBUG
	cerr << "stopping yabause\n";
#endif

	SDL_Quit();
}
