#include "yui.h"
#include "sndsdl.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDSDL,
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

void YuiHideShow(void) {
}

void YuiQuit(void) {
   stop = 1;
}

int YuiInit(int (*yab_main)()) {
   stop = 0;

   if (YabauseInit(SH2CORE_DEFAULT, GFXCORE_DEFAULT, SNDCORE_DEFAULT,
                   CDCORE_DEFAULT, REGION_AUTODETECT, bios, iso,
                   NULL, NULL) == -1)
      return -1;

   while (!stop)
   {
      if (yab_main() != 0)
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

