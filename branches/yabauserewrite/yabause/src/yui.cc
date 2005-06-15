#include "cdbase.hh"
#include "yui.hh"

int stop;

const char *bios = "jap.rom";
const char *iso = 0;

void yui_set_bios_filename(const char *biosfilename) {
	bios = biosfilename;
}

void yui_set_iso_filename(const char *isofilename) {
	iso = isofilename;
}

const char *yui_bios(void) {
	return bios;
}

CDInterface *yui_cd(void) {
        CDInterface *cd = 0;
        if (iso)
		cd = new ISOCDDrive(iso);
	else
		cd = new DummyCDDrive();
        return cd;
}

const char * yui_saveram(void) {
        return NULL;
}

const char * yui_mpegrom(void) {
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

	while (!stop) yab_main(mem);
	delete(mem);
}

void yui_errormsg(Exception error, SuperH sh2opcodes) {
   cerr << error << endl;
   cerr << sh2opcodes << endl;
}

