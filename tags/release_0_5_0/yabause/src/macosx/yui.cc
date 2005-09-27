/*  Copyright 2004 Lucas Newman

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

#include "yui.hh"

int stop;
char bios[1024];

void set_bios(char *file) {
	strcpy(bios, file);
}

void yui_init(int (*yab_main)(void*)) {
	SaturnMemory *mem;

	stop = 0;
	mem = new SaturnMemory();

	while (!stop) yab_main(mem);
	delete(mem);
}

void yui_quit(void) {
	stop = 1;
}

void yui_fps(int i) {
	//Uncomment for fps display
	//fprintf(stderr, "%d\n", i);
}

void yui_hide_show(void) {
}

char * yui_bios(void) {
	return bios;
}

char * yui_cdrom(void) {
	return "Saturn CDRom";
}

unsigned char yui_region(void) {
	return 0;
}

char * yui_saveram(void) {
	return NULL;
}

char * yui_mpegrom(void) {
	return NULL;
}