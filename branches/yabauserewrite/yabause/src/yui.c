#include "yui.h"
#include "sndsdl.h"
#include "vidsdlgl.h"
#include "vidsdlsoft.h"
#include "persdl.h"
#include "SDL.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERSDL,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDSDL,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSDLGL,
&VIDSDLSoft,
NULL
};

int stop;

const char * bios = "jap.rom";
const char * iso_or_cd = 0;
const char * backup_ram = "backup.ram";
int cdcore = CDCORE_DEFAULT;

void YuiSetBiosFilename(const char * biosfilename) {
        bios = biosfilename;
}

void YuiSetIsoFilename(const char * isofilename) {
	cdcore = CDCORE_ISO;
	iso_or_cd = isofilename;
}

void YuiSetCdromFilename(const char * cdromfilename) {
	cdcore = CDCORE_ARCH;
	iso_or_cd = cdromfilename;
}

void YuiHideShow(void) {
}

void YuiQuit(void) {
   stop = 1;
}

int YuiInit(void) {
   stop = 0;

   atexit(SDL_Quit);

   if (YabauseInit(PERCORE_SDL, SH2CORE_DEFAULT, VIDCORE_SDLGL, SNDCORE_SDL,
                   cdcore, REGION_AUTODETECT, bios, iso_or_cd,
                   backup_ram, NULL) != 0)
      return -1;

   while (!stop)
   {
      if (PERCore->HandleEvents() != 0)
         return -1;
   }

   return 0;
}

void YuiErrorMsg(const char *string) {
   fprintf(stderr, string);
   stop = 1;
}

