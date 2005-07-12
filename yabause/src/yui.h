#include "cdbase.h"

/* this function should be the "main" function of the program,
 * the parameter is a function pointer, this function wants a
 * SaturnMemory as parameter, the interface must create it
 * (ie: saturn = new SaturnMemory();) after the user has choose
 * the bios file and all others possible configurations.
 * The function should be called as often as possible */
void YuiInit(int (*)(void *));

/* quit yui_init */
void YuiQuit(void);

/* hide or show the interface */
void YuiHideShow(void);

/* returns a non NULL BIOS filename */
const char * YuiBios(void);

/* returns a cd device name or NULL if no cd device is used */
const char * YuiCdrom(void);

/* returns an instance derived from CDInterface, NULL if nothing instantiated */
CDInterface * YuiCd(void);

/* returns the region of system or 0 if autodetection is used */
unsigned char YuiRegion(void);

/* returns a save ram filename. Can be set to NULL if not present */
const char * YuiSaveram(void);

/* returns a mpeg rom filename. Can be set to NULL if not present */
const char * YuiMpegrom(void);

/* If Yabause encounters any fatal errors, it sends the error text to this function */
//void YuiErrormsg(Exception error, SuperH sh2opcodes);

/* Sets bios filename in the yui - used to specify bios from the commandline */
void YuiSetBiosFilename(const char *);

/* Sets ISO filename in the yui - used to specify an ISO from the commandline */
void YuiSetIsoFilename(const char *);

