#ifndef SCSP_H
#define SCSP_H

#include "core.h"

#define SNDCORE_DEFAULT -1

typedef struct
{
   int id;
   const char *Name;
   int (*Init)();
   void (*DeInit)();
   int (*Reset)();
   void (*MixAudio)(u16 num_samples, u16 *buffer);
   void (*MuteAudioToggle)();
} SoundInterface_struct;

typedef struct
{
   unsigned long D[8];
   unsigned long A[8];
   unsigned long SR;
   unsigned long PC;
} m68kregs_struct;

typedef struct
{
  unsigned long addr;
} m68kcodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct
{
  unsigned long scsptiming1;
  float scsptiming2;

  m68kcodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(unsigned long);
  unsigned char inbreakpoint;
} ScspInternal;

u8 FASTCALL SoundRamReadByte(u32 addr);
u16 FASTCALL SoundRamReadWord(u32 addr);
u32 FASTCALL SoundRamReadLong(u32 addr);
void FASTCALL SoundRamWriteByte(u32 addr, u8 val);
void FASTCALL SoundRamWriteWord(u32 addr, u16 val);
void FASTCALL SoundRamWriteLong(u32 addr, u32 val);

int ScspInit(int coreid);
void ScspDeInit(void);
void M68KReset(void);
void ScspReset(void);
void M68KExec(unsigned long cycles);
void ScspExec(void);

void FASTCALL scsp_w_b(u32, u8);
void FASTCALL scsp_w_w(u32, u16);
void FASTCALL scsp_w_d(u32, u32);
u8 FASTCALL scsp_r_b(u32);
u16 FASTCALL scsp_r_w(u32);
u32 FASTCALL scsp_r_d(u32);

void scsp_init(u8 *scsp_ram, void (*sint_hand)(u32), void (*mint_hand)(void));
void scsp_shutdown(void);
void scsp_reset(void);

void scsp_midi_in_send(u8 data);
void scsp_midi_out_send(u8 data);
u8 scsp_midi_in_read(void);
u8 scsp_midi_out_read(void);
void scsp_update(s32 *bufL, s32 *bufR, u32 len);
void scsp_update_timer(u32 len);

#endif
