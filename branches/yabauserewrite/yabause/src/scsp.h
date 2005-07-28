#ifndef SCSP_H
#define SCSP_H

#include "core.h"

#define SNDCORE_DEFAULT -1
#define SNDCORE_DUMMY   0

typedef struct
{
   int id;
   const char *Name;
   int (*Init)();
   void (*DeInit)();
   int (*Reset)();
   int (*ChangeVerticalFrequency)(int vertfreq);
   void (*UpdateAudio)(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
   void (*MuteAudio)();
   void (*UnMuteAudio)();
} SoundInterface_struct;

typedef struct
{
   u32 D[8];
   u32 A[8];
   u32 SR;
   u32 PC;
} m68kregs_struct;

typedef struct
{
  u32 addr;
} m68kcodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct
{
  u32 scsptiming1;
  float scsptiming2;

  m68kcodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(unsigned long);
  unsigned char inbreakpoint;
} ScspInternal;

extern SoundInterface_struct SNDDummy;

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
void ScspConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len);

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
