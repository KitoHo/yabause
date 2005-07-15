#ifndef YUI_H
#define YUI_H

#include "cdbase.h"
#include "sh2core.h"
#include "sh2int.h"
#include "scsp.h"
#include "smpc.h"
#include "vdp1.h"
#include "yabause.h"

/* This function should be the "main" function of the program.
 * The function passed as an argument is the main emulation
   function and should be called as often as possible */
int YuiInit(int (*)());

/* quit yui_init */
void YuiQuit(void);

/* hide or show the interface */
void YuiHideShow(void);

/* If Yabause encounters any fatal errors, it sends the error text to this function */
void YuiErrorMsg(const char *error_text, SH2_struct *sh2opcodes);

/* Sets bios filename in the yui - used to specify bios from the commandline */
void YuiSetBiosFilename(const char *);

/* Sets ISO filename in the yui - used to specify an ISO from the commandline */
void YuiSetIsoFilename(const char *);

#endif
