#ifndef YABAUSE_H
#define YABAUSE_H

#include "core.h"

typedef struct
{
   int id;
   const char *Name;
   int (*Init)();
   void (*DeInit)();
   void (*SetPeripheralType)(int port1type, int port2type);

} PeripheralInterface_struct;

#define CLKTYPE_26MHZNTSC       0
#define CLKTYPE_28MHZNTSC       1
#define CLKTYPE_26MHZPAL        2
#define CLKTYPE_28MHZPAL        3

void YabauseChangeTiming(int freqtype);
int YabauseInit(int percorettype,
		int sh2coretype, int vidcoretype, int sndcoretype,
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
   int IsM68KRunning;
} yabsys_struct;

extern yabsys_struct yabsys;

#endif
