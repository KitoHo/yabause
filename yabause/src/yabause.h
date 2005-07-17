#ifndef YABAUSE_H
#define YABAUSE_H

#include "core.h"

#define CLKTYPE_26MHZNTSC       0
#define CLKTYPE_28MHZNTSC       1
#define CLKTYPE_26MHZPAL        2
#define CLKTYPE_28MHZPAL        3

int YabauseInit(int sh2coretype, int gfxcoretype, int sndcoretype,
                int cdcoretype, u8 regionid, const char *biospath,
                const char *cdpath, const char *savepath, const char *mpegpath);
void YabauseDeInit();
void YabauseReset();
int YabauseExec();
void YabStartSlave(void);
void YabStopSlave(void);

typedef struct
{
   int DecilineCount;
   int LineCount;
   int DecilineStop;
   u32 Duf;
   u32 CycleCountII;
   int IsSSH2Running;
} yabsys_struct;

extern yabsys_struct yabsys;
extern unsigned short buttonbits;

#endif
