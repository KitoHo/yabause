#include "yui.hh"

int stop;

void yui_fps(int i) {
	fprintf(stderr, "%d\n", i);
}

char * yui_bios(void) {
	return "jap.rom";
}

char * yui_cdrom(void) {
	return NULL;
}

char * yui_saveram(void) {
        return NULL;
}

char * yui_mpegrom(void) {
	return NULL;
}

unsigned char yui_region(void) {
   // Should return one of the following values:
   // 0 - Autodetect
   // 1 - Japan
   // 2 - Asia/NTSC
   // 4 - North America
   // 5 - Central/South America/NTSC
   // 6 - Korea
   // A - Asia/PAL
   // C - Europe + others/PAL
   // D - Central/South America/PAL
   return 0;
}

void yui_hide_show(void) {
}

void yui_quit(void) {
	stop = 1;
}

void yui_init(int (*yab_main)(void*)) {
	SaturnMemory *mem;

	stop = 0;
	mem = new SaturnMemory();
	mem->start();

	while (!stop) yab_main(mem);
	delete(mem);
}
