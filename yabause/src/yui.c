#include "yui.h"
#include "sndsdl.h"
#include "vidsdlgl.h"
#include "vidsdlsoft.h"
#include "persdl.h"

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

   if (YabauseInit(PERCORE_SDL, SH2CORE_DEFAULT, VIDCORE_SDLGL, SNDCORE_SDL,
                   cdcore, REGION_AUTODETECT, bios, iso_or_cd,
                   NULL, NULL) != 0)
      return -1;

   while (!stop)
   {
      if (PERCore->HandleEvents() != 0)
         return -1;
   }

   return 0;
}

void YuiErrorMsg(const char *error_text, SH2_struct *sh2opcodes) {
   fprintf(stderr, "Error: %s\n", error_text);

   fprintf(stderr, "R0 = %08X\tR12 = %08X\n", sh2opcodes->regs.R[0], sh2opcodes->regs.R[12]);
   fprintf(stderr, "R1 = %08X\tR13 = %08X\n", sh2opcodes->regs.R[1], sh2opcodes->regs.R[13]);
   fprintf(stderr, "R2 = %08X\tR14 = %08X\n", sh2opcodes->regs.R[2], sh2opcodes->regs.R[14]);
   fprintf(stderr, "R3 = %08X\tR15 = %08X\n", sh2opcodes->regs.R[3], sh2opcodes->regs.R[15]);
   fprintf(stderr, "R4 = %08X\tSR = %08X\n", sh2opcodes->regs.R[4], sh2opcodes->regs.SR.all);
   fprintf(stderr, "R5 = %08X\tGBR = %08X\n", sh2opcodes->regs.R[5], sh2opcodes->regs.GBR);
   fprintf(stderr, "R6 = %08X\tVBR = %08X\n", sh2opcodes->regs.R[6], sh2opcodes->regs.VBR);
   fprintf(stderr, "R7 = %08X\tMACH = %08X\n", sh2opcodes->regs.R[7], sh2opcodes->regs.MACH);
   fprintf(stderr, "R8 = %08X\tMACL = %08X\n", sh2opcodes->regs.R[8], sh2opcodes->regs.MACL);
   fprintf(stderr, "R9 = %08X\tPR = %08X\n", sh2opcodes->regs.R[9], sh2opcodes->regs.PR);
   fprintf(stderr, "R10 = %08X\tPC = %08X\n", sh2opcodes->regs.R[10], sh2opcodes->regs.PC);
   fprintf(stderr, "R11 = %08X\n", sh2opcodes->regs.R[11]);
   stop = 1;
}

