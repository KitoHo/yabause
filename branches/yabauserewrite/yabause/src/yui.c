#include "yui.h"
#include "sh2core.h"
#include "sh2int.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

int stop;

const char * bios = "jap.rom";
const char * iso = 0;

void YuiSetBiosFilename(const char * biosfilename) {
	bios = biosfilename;
}

void YuiSetIsoFilename(const char * isofilename) {
	iso = isofilename;
}

const char * YuiBios(void) {
	return bios;
}

CDInterface * YuiCd(void) {
        CDInterface * cd = 0;
        if (iso)
		cd = &ISOCD;
	else
		cd = &DummyCD;

	cd->Init(iso);
        return cd;
}

const char * YuiSaveram(void) {
        return NULL;
}

const char * YuiMpegrom(void) {
	return NULL;
}

unsigned char YuiRegion(void) {
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

void YuiHideShow(void) {
}

void YuiQuit(void) {
	stop = 1;
}

void YuiInit(int (*yab_main)(void*)) {
#if 0
	SaturnMemory *mem;

	stop = 0;
	mem = new SaturnMemory();

	while (!stop) yab_main(mem);
	delete(mem);
#endif
}

#if 0
void YuiErrormsg(Exception error, SuperH sh2opcodes) {
   cerr << error << endl;
   cerr << sh2opcodes << endl;
}
#endif

