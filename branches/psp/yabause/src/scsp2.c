/*  src/scsp2.c: New, threadable SCSP implementation for Yabause
    Copyright 2004 Stephane Dallongeville
    Copyright 2004-2007 Theo Berkau
    Copyright 2006 Guillaume Duhamel
    Copyright 2010 Andrew Church

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "core.h"
#include "debug.h"
#include "error.h"
#include "m68kcore.h"
#include "memory.h"
#include "scsp.h"
#include "yabause.h"

#include <math.h>
#include <stdlib.h>

#undef round  // In case math.h defines it
#define round(x)  ((int) (floor((x) + 0.5)))

#undef ScspInit  // Disable compatibility alias

///////////////////////////////////////////////////////////////////////////

// This SCSP implementation is designed to be runnable as an independent
// thread, encompassing the SCSP emulation itself as well as the M68000
// sound processor and the actual generation of PCM audio data.  For this
// reason, all SCSP or sound RAM reads and writes initiated from the main
// SH-2 CPUs are (depending on ScspSetThreadMode()) passed through an I/O
// buffer, and all effects of those accesses are processed separately
// within the SCSP emulation.

//-------------------------------------------------------------------------
// SCSP constants

// SCSP hardware version (4 bits)
#define SCSP_VERSION            0

// SCSP output frequency
#define SCSP_OUTPUT_FREQ        44100

// Sound RAM size
#define SCSP_RAM_SIZE           0x80000
#define SCSP_RAM_MASK           (SCSP_RAM_SIZE - 1)

// MIDI flags
#define SCSP_MIDI_IN_EMP        0x01
#define SCSP_MIDI_IN_FUL        0x02
#define SCSP_MIDI_IN_OVF        0x04
#define SCSP_MIDI_OUT_EMP       0x08
#define SCSP_MIDI_OUT_FUL       0x10

// Envelope phases
#define SCSP_ENV_RELEASE        0
#define SCSP_ENV_SUSTAIN        1
#define SCSP_ENV_DECAY          2
#define SCSP_ENV_ATTACK         3

// Bit sizes of fixed-point counters
#define SCSP_FREQ_LOW_BITS      10      // Fractional part of frequency counter
#define SCSP_ENV_HIGH_BITS      10      // Integer part of envelope counter
#define SCSP_ENV_LOW_BITS       10      // Fractional part of envelope counter
#define SCSP_LFO_HIGH_BITS      10      // Integer part of LFO counter
#define SCSP_LFO_LOW_BITS       10      // Fractional part of LFO counter

// Lookup table sizes
#define SCSP_ENV_LEN            (1 << SCSP_ENV_HIGH_BITS)  // Envelope table length
#define SCSP_ENV_MASK           (SCSP_ENV_LEN - 1)         // Envelope table mask
#define SCSP_LFO_LEN            (1 << SCSP_LFO_HIGH_BITS)  // LFO table length
#define SCSP_LFO_MASK           (SCSP_LFO_LEN - 1)         // LFO table mask

// Envelope attack/decay points (counter values)
#define SCSP_ENV_AS             0                                    // Attack start
#define SCSP_ENV_DS             (SCSP_ENV_LEN << SCSP_ENV_LOW_BITS)  // Decay start
#define SCSP_ENV_AE             (SCSP_ENV_DS - 1)                    // Attack end
#define SCSP_ENV_DE             (((2 * SCSP_ENV_LEN) << SCSP_ENV_LOW_BITS) - 1)  // Decay end

// Envelope attack/decay rates
#define SCSP_ATTACK_RATE        ((u32) (8 * 44100))
#define SCSP_DECAY_RATE         ((u32) (12 * SCSP_ATTACK_RATE))

// Interrupt bit numbers
#define SCSP_INTERRUPT_MIDI_IN  3       // Data available in MIDI input buffer
#define SCSP_INTERRUPT_DMA      4       // DMA complete
#define SCSP_INTERRUPT_MANUAL   5       // 1 written to bit 5 of [MS]CIPD
#define SCSP_INTERRUPT_TIMER_A  6       // Timer A reached 0xFF
#define SCSP_INTERRUPT_TIMER_B  7       // Timer B reached 0xFF
#define SCSP_INTERRUPT_TIMER_C  8       // Timer C reached 0xFF
#define SCSP_INTERRUPT_MIDI_OUT 9       // MIDI output buffer became empty
#define SCSP_INTERRUPT_SAMPLE   10      // Raised once per output sample

// Interrupt target flags
#define SCSP_INTTARGET_MAIN     (1 << 0)  // Interrupt to main CPU (SCU)
#define SCSP_INTTARGET_SOUND    (1 << 1)  // Interrupt to sound CPU
#define SCSP_INTTARGET_BOTH     (SCSP_INTTARGET_MAIN | SCSP_INTTARGET_SOUND)

//-------------------------------------------------------------------------
// Internal state data

// Per-slot data structure

typedef struct SlotState_struct
{
   ////////////
   // Register fields

   // ISR $00
                        // [12] Write 1 to execute KEY state change
   u8   key;            // [11] KEY state (on/off)
   u8   sbctl;          // [10:9] Source bit control
   u8   ssctl;          // [8:7] Sound source control
   u8   lpctl;          // [6:5] Loop control
   u8   pcm8b;          // [4] PCM sound format
                        // [3:0] Start address (in bytes), high bits (19:16)

   // ISR $02
   u32  sa;             // [15:0] Start address (in bytes), low bits (15:0)

   // ISR $04
   u16  lsa;            // [15:0] Loop start address (in samples)

   // ISR $06
   u16  lea;            // [15:0] Loop end address (in samples)

   // ISR $08
   u8   sr;             // [15:11] Sustain rate
   u8   dr;             // [10:6] Decay rate
   u8   eghold;         // [5] Envelope hold (attack rate 0) flag
   u8   ar;             // [4:0] Attack rate

   // ISR $0A
   u8   lpslnk;         // [14] Loop start link (start decay on reaching LSA)
   u8   krs;            // [13:10] Key rate scale
   u8   sl;             // [9:5] Sustain level
   u8   rr;             // [4:0] Release rate

   // ISR $0C
   u8   stwinh;         // [9] Stack write inhibit flag
   u8   sdir;           // [8] Sound direct output flag
   u8   tl;             // [7:0] Total level

   // ISR $0E
   u8   mdl;            // [15:12] Modulation level
   u8   mdx;            // [11:6] Modulation source X
   u8   mdy;            // [5:0] Modulation source Y

   // ISR $10
   u8   oct;            // [14:11] Octave (treated as signed -8..7)
   u16  fns;            // [9:0] Frequency number switch

   // ISR $12
   u8   lfore;          // [15] LFO reset flag (1 = reset, 0 = count)
   u8   lfof;           // [14:10] LFO frequency index
   u8   plfows;         // [9:8] Pitch LFO waveform select
   u8   plfos;          // [7:5] Pitch LFO sensitivity
   u8   alfows;         // [4:3] Amplitude LFO waveform select
   u8   alfos;          // [2:0] Amplitude LFO sensitivity

   // ISR $14
   u8   isel;           // [6:3] Input selector
   u8   imxl;           // [2:0] Input mix level

   // ISR $16
   u8   disdl;          // [15:13] Direct data send level
   u8   dipan;          // [12:8] Direct data pan position
   u8   efsdl;          // [7:5] Effect data send level
   u8   efpan;          // [2:0] Effect data pan position

   ////////////
   // Internal state

   const void *buf;     // Sample buffer

   u32  fcnt;           // Frequency (playback) counter
   u32  finc;           // Frequency counter increment
   u8   fsft;           // Octave shift amount
   u32  lsa_shifted;    // lsa << SCSP_FREQ_LOW_BITS (for storing in fcnt)
   u32  lea_shifted;    // lea << SCSP_FREQ_LOW_BITS (for comparing with fcnt)

   u32  ecurp;          // Current envelope phase (attack/decay/...)
   s32  ecnt;           // Envelope counter
   s32  einc;           // Envelope counter increment for current phase
   s32  einca;          // Envelope counter increment for attack phase
   s32  eincd;          // Envelope counter increment for decay phase
   s32  eincs;          // Envelope counter increment for sustain phase
   s32  eincr;          // Envelope counter increment for release phase
   s32  ecmp;           // Envelope compare value
   s32  slcmp;          // Compare value corresponding to SL

   u32  lfocnt;         // LFO counter
   s32  lfoinc;         // LFO increment

   s32  *arp;           // Attack rate table pointer
   s32  *drp;           // Decay rate table pointer
   s32  *srp;           // Sustain rate table pointer
   s32  *rrp;           // Release rate table pointer

   s32  *lfofmw;        // LFO frequency modulation waveform pointer
   s32  *lfoemw;        // LFO envelope modulation waveform pointer
   u8   lfofms;         // LFO frequency modulation sensitivity (shift count)
   u8   lfoems;         // LFO envelope modulation sensitivity (shift count)

   u8   disll;          // Direct sound level (left)
   u8   dislr;          // Direct sound level (right)
   u8   efsll;          // Effect sound level (left)
   u8   efslr;          // Effect sound level (right)

   s32  tl_mult;        // Multiplier for TL
   u8   krs_shift;      // Shift count for KRS
   u8   imxl_shift;     // Shift count for IMXL

   u8   keyx;           // FIXME/SCSP1: unused field from scsp.c
   u32  finct;          // FIXME/SCSP1: no longer used from scsp.c

} SlotState;

//------------------------------------

// Overall SCSP data structure

typedef struct ScspState_struct {

   ////////////
   // Register fields

   // $400
   u8   mem4mb;         // [9] Sound RAM memory size flag (4Mbit vs. 2Mbit)
   u8   dac18b;         // [8] DAC 18-bit output flag (ignored)
   u8   ver;            // [7:4] Hardware version (fixed at 0)
   u8   mvol;           // [3:0] Master volume

   // $402
   u8   rbl;            // [8:7] Ring buffer length (8192<<RBL words)
   u32  rbp;            // [6:0] Ring buffer pointer 19:13 (low bits are zero)

   // $404
   u8   mofull;         // [12] MIDI output FIFO full flag
   u8   moemp;          // [11] MIDI output FIFO empty flag
   u8   miovf;          // [10] MIDI input FIFO overflow flag
   u8   mifull;         // [9] MIDI input FIFO full flag
   u8   miemp;          // [8] MIDI input FIFO empty flag
   u8   mibuf;          // [7:0] MIDI input data buffer

   // $406
   u8   mobuf;          // [7:0] MIDI output data buffer

   // $408
   u8   mslc;           // [15:11] Monitor slot
   u8   ca;             // [10:7] Call address

   // $40A..$410 unused (possibly used in the model 2 SCSP?)

   // $412              // [15:1] DMA transfer start address 15:1
   u32  dmea;

   // $414
                        // [15:12] DMA transfer start address 19:16
   u16  drga;           // [11:1] DMA register address 11:1

   // $416
   u8   dgate;          // [14] DMA gate (1 = zero-clear target)
   u8   ddir;           // [13] DMA direction (0 = from, 1 = to sound RAM)
   u8   dexe;           // [12] DMA execute (write 1 to start; returns to 0 when done)
   u16  dtlg;           // [11:1] DMA transfer length 11:1

   // $418
   u8   tactl;          // [10:8] Timer A step divisor (step every 1<<tactl output samples)
   u16  tima;           // [7:0] Timer A counter (sends an IRQ at 0xFF), as 8.8 fixed point

   // $41A
   u8   tbctl;          // [10:8] Timer B step divisor
   u16  timb;           // [7:0] Timer B counter, as 8.8 fixed point

   // $41C
   u8   tcctl;          // [10:8] Timer C step divisor
   u16  timc;           // [7:0] Timer C counter, as 8.8 fixed point

   // $41E
   u16  scieb;          // [10:0] Sound CPU interrupt enable

   // $420
   u16  scipd;          // [10:0] Sound CPU interrupt pending

   // $422
   //   scire;          // [10:0] Sound CPU interrupt reset (not readable)

   // $424
   u8   scilv0;         // [7:0] Sound CPU interrupt levels, bit 0

   // $426
   u8   scilv1;         // [7:0] Sound CPU interrupt levels, bit 1

   // $428
   u8   scilv2;         // [7:0] Sound CPU interrupt levels, bit 2

   // $42A
   u16  mcieb;          // [10:0] Main CPU interrupt enable

   // $42C
   u16  mcipd;          // [10:0] Main CPU interrupt pending

   // $42E
   //   mcire;          // [10:0] Main CPU interrupt reset (not readable)

   ////////////
   // Internal state

   u16  regcache[0x1000/2];  // Register value cache (for faster reads)
   s32  stack[32*2];    // 2 generations of sound output data ($600..$67E)

   SlotState slot[32];  // Data for each slot

   u32  scanline_count; // Accumulated number of scanlines
   u32  sample_timer;   // Fraction of a sample counted against timers (16.16 fixed point)

   u32  sound_ram_mask; // Sound RAM address mask (tracks mem4mb)

   u8   midi_in_buf[4]; // MIDI in buffer
   u8   midi_out_buf[4];// MIDI out buffer
   u8   midi_in_cnt;    // MIDI in data count
   u8   midi_out_cnt;   // MIDI out data count
   u8   midflag;        // FIXME/SCSP1: no longer used from scsp.c
   u8   midflag2;       // FIXME/SCSP1: unused field from scsp.c

} ScspState;

//-------------------------------------------------------------------------
// Exported data

u8 *SoundRam;

//-------------------------------------------------------------------------
// Local data

// Current SCSP state
static ScspState scsp;

// Selected sound output module
static SoundInterface_struct *SNDCore;

// Main CPU (SCU) interrupt function pointer
static void (*scsp_interrupt_handler)(void);

// Threading mode for external accesses
static ScspThreadMode scsp_thread_mode;

// Flag: Generate sound with frame-accurate timing?
static u8 scsp_frame_accurate;

// PCM output buffers and related data
static s32 *scsp_buffer_L;        // Sample buffer for left channel
static s32 *scsp_buffer_R;        // Sample buffer for right channel
static u32 scsp_sound_len;        // Samples to output per frame
static u32 scsp_sound_bufs;       // Number of "scsp_sound_len"-sample buffers
static u32 scsp_sound_bufsize;    // scsp_sound_len * scsp_sound_bufs
static u32 scsp_sound_genpos;     // Offset of next sample to generate
static u32 scsp_sound_left;       // Samples not yet sent to host driver

// Parameters for audio data generation (these are file-scope to minimize
// parameter passing overhead)
static s32 *scsp_bufL;            // Base pointer for left channel
static s32 *scsp_bufR;            // Base pointer for right channel
static u32 scsp_buf_len;          // Samples to generate

// CDDA input buffer (2 sectors' worth)
static union {
   u8 sectors[2][2352];
   u8 data[2*2352];
} cdda_buf;
static unsigned int cdda_next_in;  // Next sector buffer to receive into (0/1)
static u32 cdda_out_left;          // Bytes of CDDA left to output

// M68K-related data
static s32 FASTCALL (*m68k_execf)(s32 cycles);  // M68K->Exec or M68KExecBP
static s32 m68k_saved_cycles; // Cycles left over from the last M68KExec() call
static M68KBreakpointInfo m68k_breakpoint[M68K_MAX_BREAKPOINTS];
static int m68k_num_breakpoints;
static void (*M68KBreakpointCallback)(u32);
static int m68k_in_breakpoint;

//-------------------------------------------------------------------------
// Lookup tables

static s32 scsp_env_table[SCSP_ENV_LEN * 2];    // Envelope curve table (attack & decay)

static s32 scsp_lfo_sawt_e[SCSP_LFO_LEN];       // lfo sawtooth waveform for enveloppe
static s32 scsp_lfo_squa_e[SCSP_LFO_LEN];       // lfo square waveform for enveloppe
static s32 scsp_lfo_tri_e[SCSP_LFO_LEN];        // lfo triangle waveform for enveloppe
static s32 scsp_lfo_noi_e[SCSP_LFO_LEN];        // lfo noise waveform for enveloppe

static s32 scsp_lfo_sawt_f[SCSP_LFO_LEN];       // lfo sawtooth waveform for frequency
static s32 scsp_lfo_squa_f[SCSP_LFO_LEN];       // lfo square waveform for frequency
static s32 scsp_lfo_tri_f[SCSP_LFO_LEN];        // lfo triangle waveform for frequency
static s32 scsp_lfo_noi_f[SCSP_LFO_LEN];        // lfo noise waveform frequency

static s32 scsp_attack_rate[0x40 + 0x20];       // enveloppe step for attack
static s32 scsp_decay_rate[0x40 + 0x20];        // enveloppe step for decay
static s32 scsp_null_rate[0x20];                // null enveloppe step

static s32 scsp_lfo_step[32];                   // directly give the lfo counter step

static s32 scsp_tl_table[256];                  // table of values for total level attentuation

//-------------------------------------------------------------------------
// Local function declarations

static void ScspUpdateTimer(u32 samples, u16 *timer_ptr, u8 timer_scale,
                            int interrupt);
static void ScspGenerateAudio(s32 *bufL, s32 *bufR, u32 samples);
static void ScspGenerateAudioForSlot(SlotState *slot);
static void ScspGenerateAudioForCDDA(s32 *bufL, s32 *bufR, u32 samples);

static u8 FASTCALL ScspReadByteDirect(u32 address);
static u16 FASTCALL ScspReadWordDirect(u32 address);
static void FASTCALL ScspWriteByteDirect(u32 address, u8 data);
static void FASTCALL ScspWriteWordDirect(u32 address, u16 data);
static void ScspDoKeyOnOff(void);
static void ScspKeyOn(SlotState *slot);
static void ScspKeyOff(SlotState *slot);
static void ScspUpdateSlotAddress(SlotState *slot);
static u16 ScspMidiIn(void);
static void ScspMidiOut(u8 data);
static void ScspDoDMA(void);

static void ScspRaiseInterrupt(int which, int target);
static void ScspCheckInterrupts(u16 mask, int target);
static void ScspClearInterrupts(u16 mask, int target);

static s32 FASTCALL M68KExecBP(s32 cycles);

///////////////////////////////////////////////////////////////////////////
// FIXME: these slot update functions are copied directly from scsp1 for
// the moment (except for identifier changes and the fact that scsp_buf_pos
// the now local), until we clean them up
///////////////////////////////////////////////////////////////////////////

#ifdef WORDS_BIGENDIAN
#define SCSP_GET_OUT_8B \
   out = (s32) ((s8 *)slot->buf)[(slot->fcnt >> SCSP_FREQ_LOW_BITS)];
#else
#define SCSP_GET_OUT_8B \
   out = (s32) ((s8 *)slot->buf)[(slot->fcnt >> SCSP_FREQ_LOW_BITS) ^ 1];
#endif

#define SCSP_GET_OUT_16B        \
                out = (s32) ((s16 *)slot->buf)[slot->fcnt >> SCSP_FREQ_LOW_BITS];

#define SCSP_GET_ENV            \
                env = scsp_env_table[slot->ecnt >> SCSP_ENV_LOW_BITS] * slot->tl_mult / 1024;

#define SCSP_GET_ENV_LFO        \
                env = (scsp_env_table[slot->ecnt >> SCSP_ENV_LOW_BITS] * slot->tl_mult / 1024) - (slot->lfoemw[(slot->lfocnt >> SCSP_LFO_LOW_BITS) & SCSP_LFO_MASK] >> slot->lfoems);

#define SCSP_OUT_8B_L		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8);	\
		}

#define SCSP_OUT_8B_R		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8);	\
		}

#define SCSP_OUT_8B_LR		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8);	\
			scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8);	\
		}

#define SCSP_OUT_16B_L		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufL[scsp_buf_pos] += out >> slot->disll;	\
		}

#define SCSP_OUT_16B_R		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufR[scsp_buf_pos] += out >> slot->dislr;	\
		}

#define SCSP_OUT_16B_LR		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufL[scsp_buf_pos] += out >> slot->disll;	\
			scsp_bufR[scsp_buf_pos] += out >> slot->dislr;	\
		}

#define SCSP_UPDATE_PHASE	\
		if ((slot->fcnt += slot->finc) > slot->lea_shifted)		\
		{									\
			if (slot->lpctl) slot->fcnt = slot->lsa_shifted;		\
			else								\
			{								\
				slot->ecnt = SCSP_ENV_DE;			\
				return;						\
			}								\
		}

#define SCSP_UPDATE_PHASE_LFO	\
                slot->fcnt += ((slot->lfofmw[(slot->lfocnt >> SCSP_LFO_LOW_BITS) & SCSP_LFO_MASK] << (slot->lfofms-7)) >> (slot->fsft+1));       \
		if ((slot->fcnt += slot->finc) > slot->lea_shifted)		\
		{									\
			if (slot->lpctl) slot->fcnt = slot->lsa_shifted;		\
			else								\
			{								\
				slot->ecnt = SCSP_ENV_DE;			\
				return;						\
			}								\
		}

#define SCSP_UPDATE_ENV		\
		if ((slot->ecnt += slot->einc) >= slot->ecmp)	\
		{								\
			switch (slot->ecurp) \
			{ \
				case SCSP_ENV_ATTACK: \
					slot->ecnt = SCSP_ENV_DS; \
					slot->einc = slot->eincd; \
					slot->ecmp = slot->slcmp; \
					slot->ecurp = SCSP_ENV_DECAY; \
					break; \
				case SCSP_ENV_DECAY: \
					slot->ecnt = slot->slcmp; \
					slot->einc = slot->eincs; \
					slot->ecmp = SCSP_ENV_DE; \
					slot->ecurp = SCSP_ENV_SUSTAIN; \
					break; \
				default: \
					slot->ecnt = SCSP_ENV_DE; \
					slot->einc = 0; \
					slot->ecmp = SCSP_ENV_DE + 1; \
					return; \
			} \
		}

#define SCSP_UPDATE_LFO		\
                slot->lfocnt += slot->lfoinc;

////////////////////////////////////////////////////////////////

static void scsp_slot_update_null(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
        s32 env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
                SCSP_GET_ENV

		SCSP_UPDATE_PHASE
                SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Normal 8 bits

static void scsp_slot_update_8B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		// env = [0..0x3FF] - slot->tl
                SCSP_GET_OUT_8B
		SCSP_GET_ENV

		// don't waste time if no sound...
		SCSP_OUT_8B_L

		// calculate new frequency (phase) counter and enveloppe counter
		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_8B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_8B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
                SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe LFO modulation 8 bits

static void scsp_slot_update_E_8B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_8B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_8B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 8 bits

static void scsp_slot_update_F_8B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_8B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_8B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe & Frequency LFO modulation 8 bits

static void scsp_slot_update_F_E_8B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_8B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_8B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Normal 16 bits

static void scsp_slot_update_16B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_16B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_16B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe LFO modulation 16 bits

static void scsp_slot_update_E_16B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_16B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_R

                SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_16B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 16 bits

static void scsp_slot_update_F_16B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_16B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_16B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe & Frequency LFO modulation 16 bits

static void scsp_slot_update_F_E_16B_L(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_16B_R(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_16B_LR(SlotState *slot)
{
	u32 scsp_buf_pos = 0;
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void (*scsp_slot_update_p[2][2][2][2][2])(SlotState *slot) =
{
	// NO FMS
	{	// NO EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_8B_L,
					// RIGHT
					scsp_slot_update_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_16B_L,
					// RIGHT
					scsp_slot_update_16B_LR
				},
			}
		},
		// EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_E_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_E_8B_L,
					// RIGHT
					scsp_slot_update_E_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_E_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_E_16B_L,
					// RIGHT
					scsp_slot_update_E_16B_LR
				},
			}
		}
	},
	// FMS
	{	// NO EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_8B_L,
					// RIGHT
					scsp_slot_update_F_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_16B_L,
					// RIGHT
					scsp_slot_update_F_16B_LR
				},
			}
		},
		// EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_E_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_E_8B_L,
					// RIGHT
					scsp_slot_update_F_E_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_E_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_E_16B_L,
					// RIGHT
					scsp_slot_update_F_E_16B_LR
				},
			}
		}
	}
};

///////////////////////////////////////////////////////////////////////////
// Initialization, configuration, and cleanup routines
///////////////////////////////////////////////////////////////////////////

// ScspInit:  Initialize the SCSP emulation.  interrupt_handler should
// specify a function to handle interrupts delivered to the SCU.
// Must be called after M68KInit(); returns 0 on success, -1 on failure.

int ScspInit(int coreid, void (*interrupt_handler)(void))
{
   int i, j;
   double x;

   if ((SoundRam = T2MemoryInit(0x80000)) == NULL)
      return -1;

   // Fill in lookup tables

   for (i = 0; i < SCSP_ENV_LEN; i++)
   {
      // Attack Curve (x^4 ?)
      x = pow(((double) (SCSP_ENV_MASK - i) / (double) (SCSP_ENV_LEN)), 4);
      x *= (double) SCSP_ENV_LEN;
      scsp_env_table[i] = SCSP_ENV_MASK - (s32) x;

      // Decay curve (x = linear)
      x = pow(((double) (i) / (double) (SCSP_ENV_LEN)), 1);
      x *= (double) SCSP_ENV_LEN;
      scsp_env_table[i + SCSP_ENV_LEN] = SCSP_ENV_MASK - (s32) x;
   }

   for (i = 0, j = 0; i < 32; i++)
   {
      j += 1 << (i >> 2);
      // lfo freq
      x = 172.3 / (double) (j);
      // converting lfo freq in lfo step
      scsp_lfo_step[31 - i] = round(x * ((double) (SCSP_LFO_LEN) / (double) (SCSP_OUTPUT_FREQ)) * (double) (1 << SCSP_LFO_LOW_BITS));
   }

   for (i = 0; i < SCSP_LFO_LEN; i++)
   {
      scsp_lfo_sawt_e[i] = SCSP_LFO_MASK - i;
      if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_squa_e[i] = SCSP_LFO_MASK;
      else scsp_lfo_squa_e[i] = 0;
      if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_tri_e[i] = SCSP_LFO_MASK - (i * 2);
      else scsp_lfo_tri_e[i] = (i - (SCSP_LFO_LEN / 2)) * 2;
      scsp_lfo_noi_e[i] = rand() & SCSP_LFO_MASK;

      scsp_lfo_sawt_f[(i + 512) & SCSP_LFO_MASK] = i - (SCSP_LFO_LEN / 2);
      if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_squa_f[i] = SCSP_LFO_MASK - (SCSP_LFO_LEN / 2) - 128;
      else scsp_lfo_squa_f[i] = 0 - (SCSP_LFO_LEN / 2) + 128;
      if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_tri_f[(i + 768) & SCSP_LFO_MASK] = (i * 2) - (SCSP_LFO_LEN / 2);
      else scsp_lfo_tri_f[(i + 768) & SCSP_LFO_MASK] = (SCSP_LFO_MASK - ((i - (SCSP_LFO_LEN / 2)) * 2)) - (SCSP_LFO_LEN / 2) + 1;
      scsp_lfo_noi_f[i] = scsp_lfo_noi_e[i] - (SCSP_LFO_LEN / 2);
   }

   for (i = 0; i < 4; i++)
   {
      scsp_attack_rate[i] = 0;
      scsp_decay_rate[i] = 0;
   }

   for (i = 0; i < 60; i++)
   {
      x = 1.0 + ((i & 3) * 0.25);      // bits 0-1 : x1.00, x1.25, x1.50, x1.75
      x *= (double) (1 << ((i >> 2))); // bits 2-5 : shift bits (x2^0 - x2^15)
      x *= (double) (SCSP_ENV_LEN << SCSP_ENV_LOW_BITS); // adjust for table scsp_env_table

      scsp_attack_rate[i + 4] = round(x / (double) SCSP_ATTACK_RATE);
      scsp_decay_rate[i + 4] = round(x / (double) SCSP_DECAY_RATE);

      if (scsp_attack_rate[i + 4] == 0) scsp_attack_rate[i + 4] = 1;
      if (scsp_decay_rate[i + 4] == 0) scsp_decay_rate[i + 4] = 1;
   }

   scsp_attack_rate[63] = SCSP_ENV_AE;
   scsp_decay_rate[61] = scsp_decay_rate[60];
   scsp_decay_rate[62] = scsp_decay_rate[60];
   scsp_decay_rate[63] = scsp_decay_rate[60];

   for (i = 64; i < 96; i++)
   {
      scsp_attack_rate[i] = scsp_attack_rate[63];
      scsp_decay_rate[i] = scsp_decay_rate[63];
      scsp_null_rate[i - 64] = 0;
   }

   for (i = 0; i < 96; i++)
   {
      SCSPLOG("attack rate[%d] = %.8X -> %.8X\n", i, scsp_attack_rate[i], scsp_attack_rate[i] >> SCSP_ENV_LOW_BITS);
      SCSPLOG("decay rate[%d] = %.8X -> %.8X\n", i, scsp_decay_rate[i], scsp_decay_rate[i] >> SCSP_ENV_LOW_BITS);
   }

   for (i = 0; i < 256; i++)
      scsp_tl_table[i] = round(pow(10, ((double)i * -0.3762) / 20) * 1024.0);

   // Initialize the SCSP state

   scsp_interrupt_handler = interrupt_handler;

   ScspReset();

   // Initialize the M68K state

   if (M68K->Init() != 0)
      return -1;

   M68K->SetReadB(M68KReadByte);
   M68K->SetReadW(M68KReadWord);
   M68K->SetWriteB(M68KWriteByte);
   M68K->SetWriteW(M68KWriteWord);

   M68K->SetFetch(0x000000, 0x040000, (pointer)SoundRam);
   M68K->SetFetch(0x040000, 0x080000, (pointer)SoundRam);
   M68K->SetFetch(0x080000, 0x0C0000, (pointer)SoundRam);
   M68K->SetFetch(0x0C0000, 0x100000, (pointer)SoundRam);

   yabsys.IsM68KRunning = 0;

   m68k_execf = M68K->Exec;
   m68k_saved_cycles = 0;
   for (i = 0; i < MAX_BREAKPOINTS; i++)
      m68k_breakpoint[i].addr = 0xFFFFFFFF;
   m68k_num_breakpoints = 0;
   M68KBreakpointCallback = NULL;
   m68k_in_breakpoint = 0;

   // Set up sound output

   scsp_sound_len = SCSP_OUTPUT_FREQ / 60;  // Assume it's NTSC timing
   scsp_sound_bufs = 10;  // Should be enough to prevent skipping
   scsp_sound_bufsize = scsp_sound_len * scsp_sound_bufs;
   scsp_sound_genpos = 0;
   scsp_sound_left = 0;

   i = (SCSP_OUTPUT_FREQ / 50);  // Allocate space for largest possible buffers
   scsp_buffer_L = (s32 *)calloc(i * scsp_sound_bufs, sizeof(s32));
   scsp_buffer_R = (s32 *)calloc(i * scsp_sound_bufs, sizeof(s32));
   if (scsp_buffer_L == NULL || scsp_buffer_R == NULL)
      return -1;

   return ScspChangeSoundCore(coreid);
}

//-------------------------------------------------------------------------

// ScspReset:  Reset the SCSP to its power-on state.

void ScspReset(void)
{
   int slotnum;

   scsp.mem4mb  = 0;
   scsp.dac18b  = 0;
   scsp.ver     = 0;
   scsp.mvol    = 0;

   scsp.rbl     = 0;
   scsp.rbp     = 0;

   scsp.mofull  = 0;
   scsp.moemp   = 1;
   scsp.miovf   = 0;
   scsp.mifull  = 0;
   scsp.miemp   = 1;
   scsp.mibuf   = 0;
   scsp.mobuf   = 0;

   scsp.mslc    = 0;
   scsp.ca      = 0;

   scsp.dmea    = 0;
   scsp.drga    = 0;
   scsp.dgate   = 0;
   scsp.ddir    = 0;
   scsp.dexe    = 0;
   scsp.dtlg    = 0;

   scsp.tactl   = 0;
   scsp.tima    = 0xFF00;
   scsp.tbctl   = 0;
   scsp.timb    = 0xFF00;
   scsp.tcctl   = 0;
   scsp.timc    = 0xFF00;

   scsp.mcieb   = 0;
   scsp.mcipd   = 0;
   scsp.scilv0  = 0;
   scsp.scilv1  = 0;
   scsp.scilv2  = 0;
   scsp.scieb   = 0;
   scsp.scipd   = 0;

   memset(scsp.regcache, 0, sizeof(scsp.regcache));
   scsp.regcache[0x400>>1] = SCSP_VERSION << 4;
   memset(scsp.stack, 0, sizeof(scsp.stack));

   for (slotnum = 0; slotnum < 32; slotnum++)
   {
      memset(&scsp.slot[slotnum], 0, sizeof(scsp.slot[slotnum]));
      scsp.slot[slotnum].ecnt = SCSP_ENV_DE;  // Slot off
      scsp.slot[slotnum].disll = 31;          // Direct sound level off
      scsp.slot[slotnum].dislr = 31;
      scsp.slot[slotnum].efsll = 31;          // Effect sound level off
      scsp.slot[slotnum].efslr = 31;
   }

   scsp.sound_ram_mask = 0x3FFFF;
   scsp.scanline_count = 0;
   scsp.sample_timer = 0;
   scsp.midi_in_cnt = scsp.midi_out_cnt = 0;
}

//-------------------------------------------------------------------------

// ScspChangeSoundCore:  Change the module used for sound output.  Returns
// 0 on success, -1 on error.

int ScspChangeSoundCore(int coreid)
{
   int i;

   // Make sure the old core is freed
   if (SNDCore)
      SNDCore->DeInit();

   // So which core do we want?
   if (coreid == SNDCORE_DEFAULT)
      // FIXME/SCSP1: replace with: coreid = SNDCoreList[0] ? SNDCoreList[0]->id : 0;
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; SNDCoreList[i] != NULL; i++)
   {
      if (SNDCoreList[i]->id == coreid)
      {
         // Set to current core
         SNDCore = SNDCoreList[i];
         break;
      }
   }

   if (SNDCore == NULL)
   {
      SNDCore = &SNDDummy;
      return -1;
   }

   if (SNDCore->Init() == -1)
   {
      // Since it failed, instead of it being fatal, we'll just use the dummy
      // core instead

      // This might be helpful though.
      YabSetError(YAB_ERR_CANNOTINIT, (void *)SNDCore->Name);

      SNDCore = &SNDDummy;
   }

   return 0;
}

//-------------------------------------------------------------------------

// ScspChangeVideoFormat:  Update SCSP parameters for a change in video
// format.  type is nonzero for PAL (50Hz), zero for NTSC (59.94Hz) video.
// Always returns 0 for success.

int ScspChangeVideoFormat(int type)
{
   scsp_sound_len = 44100 / (type ? 50 : 60);
   scsp_sound_bufsize = scsp_sound_len * scsp_sound_bufs;

   SNDCore->ChangeVideoFormat(type ? 50 : 60);

   return 0;
}


//-------------------------------------------------------------------------

// ScspSetFrameAccurate:  Set whether sound should be generated with
// frame-accurate timing.

void ScspSetFrameAccurate(int on)
{
   scsp_frame_accurate = (on != 0);
}

//-------------------------------------------------------------------------

// ScspSetThreadMode:  Set the threading mode for SCSP external read/write
// accesses.

void ScspSetThreadMode(ScspThreadMode mode)
{
   scsp_thread_mode = mode;
}

//-------------------------------------------------------------------------

// ScspMuteAudio, ScspUnMuteAudio:  Mute or unmute the sound output.  Does
// not affect actual SCSP processing.

void ScspMuteAudio(void)
{
   if (SNDCore)
      SNDCore->MuteAudio();
}

void ScspUnMuteAudio(void)
{
   if (SNDCore)
      SNDCore->UnMuteAudio();
}

//-------------------------------------------------------------------------

// ScspSetVolume:  Set the sound output volume.  Does not affect actual
// SCSP processing.

void ScspSetVolume(int volume)
{
   if (SNDCore)
      SNDCore->SetVolume(volume);
}

//-------------------------------------------------------------------------

// ScspDeInit:  Free all resources used by the SCSP emulation.

void ScspDeInit(void)
{
   if (scsp_buffer_L)
      free(scsp_buffer_L);
   scsp_buffer_L = NULL;

   if (scsp_buffer_R)
      free(scsp_buffer_R);
   scsp_buffer_R = NULL;

   if (SNDCore)
      SNDCore->DeInit();
   SNDCore = NULL;

   if (SoundRam)
      T2MemoryDeInit(SoundRam);
   SoundRam = NULL;
}

///////////////////////////////////////////////////////////////////////////
// Main SCSP processing routine and internal helpers
///////////////////////////////////////////////////////////////////////////

// ScspExec:  Main SCSP processing routine.  Updates timers and generates
// samples for one scanline's worth of time.

void ScspExec(void)
{
#ifdef WIN32
   s16 stereodata16[(44100 / 60) * 16]; //11760
#endif
   u32 samples_for_timer;
   u32 audio_free;

   scsp.scanline_count++;
   scsp.sample_timer += ((735<<16) + 263/2) / 263;
   samples_for_timer = scsp.sample_timer >> 16;  // Pass integer part
   scsp.sample_timer &= 0xFFFF;                  // Keep fractional part
   ScspUpdateTimer(samples_for_timer, &scsp.tima, scsp.tactl,
                   SCSP_INTERRUPT_TIMER_A);
   ScspUpdateTimer(samples_for_timer, &scsp.timb, scsp.tbctl,
                   SCSP_INTERRUPT_TIMER_B);
   ScspUpdateTimer(samples_for_timer, &scsp.timc, scsp.tcctl,
                   SCSP_INTERRUPT_TIMER_C);

   if (scsp.scanline_count >= 263)
   {
      s32 *bufL, *bufR;

      scsp.scanline_count -= 263;
      scsp.sample_timer = 0;

      if (scsp_frame_accurate) {
         // Update sound buffers
         if (scsp_sound_genpos + scsp_sound_len > scsp_sound_bufsize) {
            scsp_sound_genpos = 0;
         }
         if (scsp_sound_left + scsp_sound_len > scsp_sound_bufsize) {
            u32 overrun = (scsp_sound_left + scsp_sound_len) - scsp_sound_bufsize;
            SCSPLOG("WARNING: Sound buffer overrun, %lu samples\n", (long)overrun);
            scsp_sound_left -= overrun;
         }
         bufL = &scsp_buffer_L[scsp_sound_genpos];
         bufR = &scsp_buffer_R[scsp_sound_genpos];
         ScspGenerateAudio(bufL, bufR, scsp_sound_len);
         scsp_sound_genpos += scsp_sound_len;
         scsp_sound_left += scsp_sound_len;
      }
   }

   if (scsp_frame_accurate)
   {
      while (scsp_sound_left > 0 && (audio_free = SNDCore->GetAudioSpace()) > 0)
      {
         s32 out_start = (s32)scsp_sound_genpos - (s32)scsp_sound_left;
         if (out_start < 0)
            out_start += scsp_sound_bufsize;
         if (audio_free > scsp_sound_left)
            audio_free = scsp_sound_left;
         if (audio_free > scsp_sound_bufsize - out_start)
            audio_free = scsp_sound_bufsize - out_start;
         SNDCore->UpdateAudio((u32 *)&scsp_buffer_L[out_start],
                              (u32 *)&scsp_buffer_R[out_start], audio_free);
         scsp_sound_left -= audio_free;
#ifdef WIN32
         ScspConvert32uto16s(&scsp_buffer_L[out_start], &scsp_buffer_R[out_start], (s16 *)stereodata16, audio_free);
         DRV_AviSoundUpdate(stereodata16, audio_free);
#endif
      }
   }
   else
   {
      if ((audio_free = SNDCore->GetAudioSpace()))
      {
         if (audio_free > scsp_sound_len)
            audio_free = scsp_sound_len;
         ScspGenerateAudio(scsp_buffer_L, scsp_buffer_R, audio_free);
         SNDCore->UpdateAudio((u32 *)scsp_buffer_L,
                              (u32 *)scsp_buffer_R, audio_free);
#ifdef WIN32
         ScspConvert32uto16s((s32 *)scsp_buffer_L, (s32 *)scsp_buffer_R, (s16 *)stereodata16, audio_free);
         DRV_AviSoundUpdate(stereodata16, audio_free);
#endif
      }
   }  // if (scsp_frame_accurate)
}

//-------------------------------------------------------------------------

// ScspUpdateTimer:  Update an SCSP timer (A, B, or C) by the given number
// of output samples, and raise an interrupt if the timer reaches 0xFF.

static void ScspUpdateTimer(u32 samples, u16 *timer_ptr, u8 timer_scale,
                            int interrupt)
{
   u32 timer_new = *timer_ptr + (samples << (8 - timer_scale));
   if (timer_new >= 0xFF00)
   {
      ScspRaiseInterrupt(interrupt, SCSP_INTTARGET_BOTH);
      timer_new -= 0xFF00;  // We won't pass 0xFF00 multiple times at once
   }
   *timer_ptr = timer_new;
}

//-------------------------------------------------------------------------

// ScspGenerateAudio:  Generate the given number of audio samples based on
// the current SCSP state, and update the sound slot counters.

static void ScspGenerateAudio(s32 *bufL, s32 *bufR, u32 samples)
{
   int slotnum;

   memset(bufL, 0, sizeof(s32) * samples);
   memset(bufR, 0, sizeof(s32) * samples);

   scsp_bufL = bufL;
   scsp_bufR = bufR;
   scsp_buf_len = samples;
   for (slotnum = 0; slotnum < 32; slotnum++) {
      ScspGenerateAudioForSlot(&scsp.slot[slotnum]);
   }

   if (cdda_out_left > 0) {
      ScspGenerateAudioForCDDA(bufL, bufR, samples);
   }
}

//----------------------------------//

// ScspGenerateAudioForSlot:  Generate audio samples and update counters for
// a single slot.  scsp_bufL, scsp_bufR, and scsp_buf_len are assumed to be
// set properly.

static void ScspGenerateAudioForSlot(SlotState *slot)
{
   if (slot->ecnt >= SCSP_ENV_DE)  // I.e., no sound is currently playing
      return;

   if (slot->ssctl)
   {
      // Update the address counter (fcnt), since some games read it via
      // the CA register even if the data itself isn't played
      u32 pos;
      for (pos = 0; pos < scsp_buf_len; pos++)
      {
         if ((slot->fcnt += slot->finc) > slot->lea_shifted)
         {
            if (slot->lpctl)
               slot->fcnt = slot->lsa_shifted;
            else
            {
               slot->ecnt = SCSP_ENV_DE;
               break;
            }
         }
      }

      // FIXME: noise (ssctl==1) not implemented
      return;
   }

   // Extract the left and right volumes (shift count); if the direct sound
   // output is muted, we assume the data is being passed through the DSP
   // (which we don't currently implement) and take the effect output level
   // instead
   // FIXME/SCSP1: we shouldn't stomp on the DI registers, but we do for now
   // because scsp1 did (and it's a convenient way to avoid passing more
   // parameters around)
   if (slot->disll == 31 && slot->dislr == 31)
   {
      slot->disll = slot->efsll;
      slot->dislr = slot->efslr;
   }

   // Actually generate the audio
   scsp_slot_update_p[(slot->lfofms == 31)?0:1][(slot->lfoems == 31)?0:1][(slot->pcm8b == 0)?1:0][(slot->disll == 31)?0:1][(slot->dislr == 31)?0:1](slot);
}

//----------------------------------//

// ScspGenerateAudioForCDDA:  Generate audio samples for buffered CDDA data.
// The CDDA buffer is assumed to be non-empty.

static void ScspGenerateAudioForCDDA(s32 *bufL, s32 *bufR, u32 samples)
{
   u32 pos, len;

   if (samples > cdda_out_left / 4)
      len = cdda_out_left / 4;
   else
      len = samples;

   /* May need to wrap around the buffer, so use nested loops */
   while (pos < len)
   {
      s32 temp = (cdda_next_in * 2352) - cdda_out_left;
      s32 outpos = (temp < 0) ? temp + sizeof(cdda_buf.data) : temp;
      u8 *buf = &cdda_buf.data[outpos];

      u32 target;
      u32 this_len = len - pos;
      if (this_len > (sizeof(cdda_buf.data) - outpos) / 4)
         this_len = (sizeof(cdda_buf.data) - outpos) / 4;
      target = pos + this_len;

      for(; pos < target; pos++, buf += 4)
      {
         s32 out;

         out = (s32)(s16)((buf[1] << 8) | buf[0]);
         if (out)
            bufL[pos] += out;

         out = (s32)(s16)((buf[3] << 8) | buf[2]);
         if (out)
            bufR[pos] += out;
      }

      cdda_out_left -= this_len * 4;
   }
}

///////////////////////////////////////////////////////////////////////////
// SCSP register/memory access and I/O interface routines
///////////////////////////////////////////////////////////////////////////

// SoundRam{Read,Write}{Byte,Word,Long}:  Read or write sound RAM.
// Intended for calling from external sources.

u8 FASTCALL SoundRamReadByte(u32 address)
{
   // FIXME/SCSP1: shouldn't the 0x7FFFF comparison below be a mask operation?
   // If so, this whole routine could turn into just:
   //    return T2ReadByte(SoundRam, address & scsp.sound_ram_mask);
   // (and likewise for other read/write routines below)
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return 0xFF;
   }
   return T2ReadByte(SoundRam, address);
}

u16 FASTCALL SoundRamReadWord(u32 address)
{
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return 0xFFFF;
   }
   return T2ReadWord(SoundRam, address);
}

u32 FASTCALL SoundRamReadLong(u32 address)
{
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return 0xFFFFFFFF;
   }
   return T2ReadLong(SoundRam, address);
}

//----------------------------------//

void FASTCALL SoundRamWriteByte(u32 address, u8 data)
{
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return;
   }
   T2WriteByte(SoundRam, address, data);
   M68K->WriteNotify(address, 1);
}

void FASTCALL SoundRamWriteWord(u32 address, u16 data)
{
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return;
   }
   T2WriteWord(SoundRam, address, data);
   M68K->WriteNotify(address, 2);
}

void FASTCALL SoundRamWriteLong(u32 address, u32 data)
{
   if (scsp.mem4mb == 0)
      address &= 0x3FFFF;
   else
   {
      address &= 0xFFFFF;
      if (address > 0x7FFFF)
         return;
   }
   T2WriteLong(SoundRam, address, data);
   M68K->WriteNotify(address, 4);
}

//-------------------------------------------------------------------------

// Scsp{Read,Write}{Byte,Word,Long}:  Read or write SCSP registers.
// Intended for calling from external sources.

// FIXME: threading

u8 FASTCALL ScspReadByte(u32 address)
{
   return ScspReadByteDirect(address & 0xFFF);
}

u16 FASTCALL ScspReadWord(u32 address)
{
   return ScspReadWordDirect(address & 0xFFF);
}

u32 FASTCALL ScspReadLong(u32 address)
{
   return (u32)ScspReadWordDirect(address & 0xFFF) << 16
          | ScspReadWordDirect((address+2) & 0xFFF);
}

//----------------------------------//

void FASTCALL ScspWriteByte(u32 address, u8 data)
{
   ScspWriteByteDirect(address & 0xFFF, data);
}

void FASTCALL ScspWriteWord(u32 address, u16 data)
{
   ScspWriteWordDirect(address & 0xFFF, data);
}

void FASTCALL ScspWriteLong(u32 address, u32 data)
{
   ScspWriteWordDirect(address & 0xFFF, data >> 16);
   ScspWriteWordDirect((address+2) & 0xFFF, data & 0xFFFF);
}

//-------------------------------------------------------------------------

// ScspReceiveCDDA:  Receive and buffer a sector (2352 bytes) of CDDA audio
// data.  Intended to be called by the CD driver when an audio sector has
// been read in for playback.

void ScspReceiveCDDA(const u8 *sector)
{
   memcpy(cdda_buf.sectors[cdda_next_in], sector, 2352);
   cdda_next_in = (cdda_next_in + 1)
                  % (sizeof(cdda_buf.sectors) / sizeof(cdda_buf.sectors[0]));
   cdda_out_left += 2352;
   if (cdda_out_left > sizeof(cdda_buf.data))
   {
      SCSPLOG("WARNING: CDDA buffer overrun\n");
      cdda_out_left = sizeof(cdda_buf.data);
   }
}

///////////////////////////////////////////////////////////////////////////
// Miscellaneous SCSP interface routines
///////////////////////////////////////////////////////////////////////////

// SoundSaveState:  Save the current SCSP state to the given file.

int SoundSaveState(FILE *fp)
{
   int i;
   u32 temp;
   u8 temp8;
   int offset;
   IOCheck_struct check;

   offset = StateWriteHeader(fp, "SCSP", 2);

   // Save 68k registers first
   ywrite(&check, (void *)&yabsys.IsM68KRunning, 1, 1, fp);
   for (i = 0; i < 8; i++)
   {
      temp = M68K->GetDReg(i);
      ywrite(&check, (void *)&temp, 4, 1, fp);
   }
   for (i = 0; i < 8; i++)
   {
      temp = M68K->GetAReg(i);
      ywrite(&check, (void *)&temp, 4, 1, fp);
   }
   temp = M68K->GetSR();
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = M68K->GetPC();
   ywrite(&check, (void *)&temp, 4, 1, fp);

   // Now for the SCSP registers
   ywrite(&check, (void *)scsp.regcache, 0x1000, 1, fp);

   // Sound RAM is important
   ywrite(&check, (void *)SoundRam, 0x80000, 1, fp);

   // Write slot internal variables
   for (i = 0; i < 32; i++)
   {
      ywrite(&check, (void *)&scsp.slot[i].key, 1, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].fcnt, 4, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].ecnt, 4, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].einc, 4, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].ecmp, 4, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].ecurp, 4, 1, fp);

      // Was enxt in scsp1; we don't use it, so just derive the proper
      // value from ecurp
      if (scsp.slot[i].ecurp == SCSP_ENV_RELEASE)
         temp8 = 1;
      else if (scsp.slot[i].ecurp == SCSP_ENV_SUSTAIN)
         temp8 = 2;
      else if (scsp.slot[i].ecurp == SCSP_ENV_DECAY)
         temp8 = 3;
      else if (scsp.slot[i].ecurp == SCSP_ENV_ATTACK)
         temp8 = 4;
      else  // impossible, but avoid "undefined value" warnings
         temp8 = 0;
      ywrite(&check, (void *)&temp8, 1, 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].lfocnt, 4, 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lfoinc, 4, 1, fp);
   }

   // Write main internal variables
   // FIXME/SCSP1: need to write a lot of these from temporary variables
   // to maintain save state compatibility
   temp = scsp.mem4mb;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.mvol;
   ywrite(&check, (void *)&temp, 4, 1, fp);

   temp = scsp.rbl;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   ywrite(&check, (void *)&scsp.rbp, 4, 1, fp);

   temp = scsp.mslc;
   ywrite(&check, (void *)&temp, 4, 1, fp);

   ywrite(&check, (void *)&scsp.dmea, 4, 1, fp);
   temp = scsp.drga;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.dgate<<6 | scsp.ddir<<5 | scsp.dexe<<4;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.dtlg;
   ywrite(&check, (void *)&temp, 4, 1, fp);

   ywrite(&check, (void *)scsp.midi_in_buf, 1, 4, fp);
   ywrite(&check, (void *)scsp.midi_out_buf, 1, 4, fp);
   ywrite(&check, (void *)&scsp.midi_in_cnt, 1, 1, fp);
   ywrite(&check, (void *)&scsp.midi_out_cnt, 1, 1, fp);
   temp8 = scsp.mofull<<4 | scsp.moemp<<3
         | scsp.miovf<<2 | scsp.mifull<<1 | scsp.miemp<<0;
   ywrite(&check, (void *)&temp8, 1, 1, fp);

   temp = scsp.tima;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.tactl;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.timb;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.tbctl;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.timc;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.tcctl;
   ywrite(&check, (void *)&temp, 4, 1, fp);

   temp = scsp.scieb;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.scipd;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.scilv0;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.scilv1;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.scilv2;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.mcieb;
   ywrite(&check, (void *)&temp, 4, 1, fp);
   temp = scsp.mcipd;
   ywrite(&check, (void *)&temp, 4, 1, fp);

   ywrite(&check, (void *)scsp.stack, 4, 32 * 2, fp);

   return StateFinishHeader(fp, offset);
}

//-------------------------------------------------------------------------

// SoundSaveState:  Load the current SCSP state from the given file.

int SoundLoadState(FILE *fp, int version, int size)
{
   int i, i2;
   u32 temp;
   u8 temp8;
   IOCheck_struct check;

   // Read 68k registers first
   yread(&check, (void *)&yabsys.IsM68KRunning, 1, 1, fp);
   for (i = 0; i < 8; i++) {
      yread(&check, (void *)&temp, 4, 1, fp);
      M68K->SetDReg(i, temp);
   }
   for (i = 0; i < 8; i++) {
      yread(&check, (void *)&temp, 4, 1, fp);
      M68K->SetAReg(i, temp);
   }
   yread(&check, (void *)&temp, 4, 1, fp);
   M68K->SetSR(temp);
   yread(&check, (void *)&temp, 4, 1, fp);
   M68K->SetPC(temp);

   // Now for the SCSP registers
   yread(&check, (void *)scsp.regcache, 0x1000, 1, fp);

   // And sound RAM
   yread(&check, (void *)SoundRam, 0x80000, 1, fp);

   // Break out slot registers into their respective fields
   for (i = 0; i < 32; i++)
   {
      for (i2 = 0; i2 < 0x18; i2 += 2)
         ScspWriteWordDirect(i<<5 | i2, scsp.regcache[(i<<5 | i2) >> 1]);
      ScspUpdateSlotAddress(&scsp.slot[i]);
   }

   if (version > 1)
   {
      // Read slot internal variables
      for (i = 0; i < 32; i++)
      {
         yread(&check, (void *)&scsp.slot[i].key, 1, 1, fp);
         yread(&check, (void *)&scsp.slot[i].fcnt, 4, 1, fp);
         yread(&check, (void *)&scsp.slot[i].ecnt, 4, 1, fp);
         yread(&check, (void *)&scsp.slot[i].einc, 4, 1, fp);
         yread(&check, (void *)&scsp.slot[i].ecmp, 4, 1, fp);
         yread(&check, (void *)&scsp.slot[i].ecurp, 4, 1, fp);

         // Was enxt in scsp1; we don't use it, so just read and ignore
         yread(&check, (void *)&temp8, 1, 1, fp);

         yread(&check, (void *)&scsp.slot[i].lfocnt, 4, 1, fp);
         yread(&check, (void *)&scsp.slot[i].lfoinc, 4, 1, fp);
      }

      // Read main internal variables
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.mem4mb = temp;
      // This one isn't saved in the state file (though it's not used anyway)
      scsp.dac18b = (scsp.regcache[0x400>>1] >> 8) & 1;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.mvol = temp;

      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.rbl = temp;
      yread(&check, (void *)&scsp.rbp, 4, 1, fp);

      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.mslc = temp;

      yread(&check, (void *)&scsp.dmea, 4, 1, fp);
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.drga = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.dgate = temp>>6 & 1;
      scsp.ddir  = temp>>5 & 1;
      scsp.dexe  = temp>>4 & 1;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.dtlg = temp;

      yread(&check, (void *)scsp.midi_in_buf, 1, 4, fp);
      yread(&check, (void *)scsp.midi_out_buf, 1, 4, fp);
      yread(&check, (void *)&scsp.midi_in_cnt, 1, 1, fp);
      yread(&check, (void *)&scsp.midi_out_cnt, 1, 1, fp);
      yread(&check, (void *)&temp8, 1, 1, fp);
      scsp.mofull = temp8>>4 & 1;
      scsp.moemp  = temp8>>3 & 1;
      scsp.miovf  = temp8>>2 & 1;
      scsp.mifull = temp8>>1 & 1;
      scsp.miemp  = temp8>>0 & 1;

      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.tima = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.tactl = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.timb = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.tbctl = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.timc = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.tcctl = temp;

      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.scieb = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.scipd = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.scilv0 = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.scilv1 = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.scilv2 = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.mcieb = temp;
      yread(&check, (void *)&temp, 4, 1, fp);
      scsp.mcipd = temp;

      yread(&check, (void *)scsp.stack, 4, 32 * 2, fp);
   }

   return size;
}

//-------------------------------------------------------------------------

// ScspSlotDebugStats:  Generate a string describing the given slot's state
// and store it in the passed-in buffer (which is assumed to be large enough
// to hold the result).

// Helper functions (defined below)
static char *AddSoundLFO(char *outstring, const char *string, u16 level, u16 waveform);
static char *AddSoundPan(char *outstring, u16 pan);
static char *AddSoundLevel(char *outstring, u16 level);

void ScspSlotDebugStats(u8 slotnum, char *outstring)
{
   AddString(outstring, "Sound Source = ");
   switch (scsp.slot[slotnum].ssctl)
   {
      case 0:
      {
         AddString(outstring, "External DRAM data\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Internal(Noise)\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Internal(0's)\r\n");
         break;
      }
      default:
      {
         AddString(outstring, "Invalid setting\r\n");
         break;
      }
   }
   AddString(outstring, "Source bit = ");
   switch (scsp.slot[slotnum].sbctl)
   {
      case 0:
      {
         AddString(outstring, "No bit reversal\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Reverse other bits\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Reverse sign bit\r\n");
         break;
      }
      case 3:
      {
         AddString(outstring, "Reverse sign and other bits\r\n");
         break;
      }
   }

   // Loop Control
   AddString(outstring, "Loop Mode = ");
   switch (scsp.slot[slotnum].lpctl)
   {
      case 0:
      {
         AddString(outstring, "Off\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Normal\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Reverse\r\n");
         break;
      }
      case 3:
      {
         AddString(outstring, "Alternating\r\n");
         break;
      }
   }
   // PCM8B
   if (scsp.slot[slotnum].pcm8b)
   {
      AddString(outstring, "8-bit samples\r\n");
   }
   else
   {
      AddString(outstring, "16-bit samples\r\n");
   }

   AddString(outstring, "Start Address = %05lX\r\n", (unsigned long)scsp.slot[slotnum].sa);
   AddString(outstring, "Loop Start Address = %04X\r\n", scsp.slot[slotnum].lsa);
   AddString(outstring, "Loop End Address = %04X\r\n", scsp.slot[slotnum].lea);
   AddString(outstring, "Decay 1 Rate = %d\r\n", scsp.slot[slotnum].dr);
   AddString(outstring, "Decay 2 Rate = %d\r\n", scsp.slot[slotnum].sr);
   if (scsp.slot[slotnum].eghold)
   {
      AddString(outstring, "EG Hold Enabled\r\n");
   }
   AddString(outstring, "Attack Rate = %d\r\n", scsp.slot[slotnum].ar);

   if (scsp.slot[slotnum].lpslnk)
   {
      AddString(outstring, "Loop Start Link Enabled\r\n");
   }

   if (scsp.slot[slotnum].krs != 0)
   {
      AddString(outstring, "Key rate scaling = %d\r\n", scsp.slot[slotnum].krs);
   }

   AddString(outstring, "Decay Level = %d\r\n", scsp.slot[slotnum].sl);
   AddString(outstring, "Release Rate = %d\r\n", scsp.slot[slotnum].rr);

   if (scsp.slot[slotnum].stwinh)
   {
      AddString(outstring, "Stack Write Inhibited\r\n");
   }

   if (scsp.slot[slotnum].sdir)
   {
      AddString(outstring, "Sound Direct Enabled\r\n");
   }

   AddString(outstring, "Total Level = %d\r\n", scsp.slot[slotnum].tl);

   AddString(outstring, "Modulation Level = %d\r\n", scsp.slot[slotnum].mdl);
   AddString(outstring, "Modulation Input X = %d\r\n", scsp.slot[slotnum].mdx);
   AddString(outstring, "Modulation Input Y = %d\r\n", scsp.slot[slotnum].mdy);

   AddString(outstring, "Octave = %d\r\n", scsp.slot[slotnum].oct);
   AddString(outstring, "Frequency Number Switch = %d\r\n", scsp.slot[slotnum].fns);

   AddString(outstring, "LFO Reset = %s\r\n", scsp.slot[slotnum].lfore ? "TRUE" : "FALSE");
   AddString(outstring, "LFO Frequency = %d\r\n", scsp.slot[slotnum].lfof);
   outstring = AddSoundLFO(outstring, "LFO Frequency modulation waveform =",
                           scsp.slot[slotnum].plfos, scsp.slot[slotnum].plfows);
   AddString(outstring, "LFO Frequency modulation level = %d\r\n", scsp.slot[slotnum].plfos);
   outstring = AddSoundLFO(outstring, "LFO Amplitude modulation waveform =",
                           scsp.slot[slotnum].alfos, scsp.slot[slotnum].alfows);
   AddString(outstring, "LFO Amplitude modulation level = %d\r\n", scsp.slot[slotnum].alfos);

   AddString(outstring, "Input mix level = ");
   outstring = AddSoundLevel(outstring, scsp.slot[slotnum].imxl);
   AddString(outstring, "Input Select = %d\r\n", scsp.slot[slotnum].isel);

   AddString(outstring, "Direct data send level = ");
   outstring = AddSoundLevel(outstring, scsp.slot[slotnum].disdl);
   AddString(outstring, "Direct data panpot = ");
   outstring = AddSoundPan(outstring, scsp.slot[slotnum].dipan);

   AddString(outstring, "Effect data send level = ");
   outstring = AddSoundLevel(outstring, scsp.slot[slotnum].efsdl);
   AddString(outstring, "Effect data panpot = ");
   outstring = AddSoundPan(outstring, scsp.slot[slotnum].efpan);
}

//----------------------------------//

static char *AddSoundLFO(char *outstring, const char *string, u16 level, u16 waveform)
{
   if (level > 0)
   {
      switch (waveform)
      {
         case 0:
            AddString(outstring, "%s Sawtooth\r\n", string);
            break;
         case 1:
            AddString(outstring, "%s Square\r\n", string);
            break;
         case 2:
            AddString(outstring, "%s Triangle\r\n", string);
            break;
         case 3:
            AddString(outstring, "%s Noise\r\n", string);
            break;
      }
   }

   return outstring;
}

//----------------------------------//

static char *AddSoundPan(char *outstring, u16 pan)
{
   if (pan == 0x0F)
   {
      AddString(outstring, "Left = -MAX dB, Right = -0 dB\r\n");
   }
   else if (pan == 0x1F)
   {
      AddString(outstring, "Left = -0 dB, Right = -MAX dB\r\n");
   }
   else
   {
      AddString(outstring, "Left = -%d dB, Right = -%d dB\r\n", (pan & 0xF) * 3, (pan >> 4) * 3);
   }

   return outstring;
}

//----------------------------------//

static char *AddSoundLevel(char *outstring, u16 level)
{
   if (level == 0)
   {
      AddString(outstring, "-MAX dB\r\n");
   }
   else
   {
      AddString(outstring, "-%d dB\r\n", (7-level) *  6);
   }

   return outstring;
}

//-------------------------------------------------------------------------

// ScspCommonControlRegisterDebugStats:  Generate a string describing the
// SCSP common state registers and store it in the passed-in buffer (which
// is assumed to be large enough to hold the result).

void ScspCommonControlRegisterDebugStats(char *outstring)
{
   AddString(outstring, "Memory: %s\r\n", scsp.mem4mb ? "4 Mbit" : "2 Mbit");
   AddString(outstring, "Master volume: %d\r\n", scsp.mvol);
   AddString(outstring, "Ring buffer length: %d\r\n", scsp.rbl);
   AddString(outstring, "Ring buffer address: %08lX\r\n", (unsigned long)scsp.rbp);
   AddString(outstring, "\r\n");

   AddString(outstring, "Slot Status Registers\r\n");
   AddString(outstring, "-----------------\r\n");
   AddString(outstring, "Monitor slot: %d\r\n", scsp.mslc);
   AddString(outstring, "Call address: %d\r\n", (ScspReadWordDirect(0x408) >> 7) & 0xF);
   AddString(outstring, "\r\n");

   AddString(outstring, "DMA Registers\r\n");
   AddString(outstring, "-----------------\r\n");
   AddString(outstring, "DMA memory address start: %08lX\r\n", (unsigned long)scsp.dmea);
   AddString(outstring, "DMA register address start: %03X\r\n", scsp.drga);
   AddString(outstring, "DMA Flags: %02X (%cDGATE %cDDIR %cDEXE)\r\n",
             scsp.dgate<<6 | scsp.ddir<<5 | scsp.dexe<<4,
             scsp.dgate ? '+' : '-', scsp.ddir ? '+' : '-',
             scsp.dexe ? '+' : '-');
   AddString(outstring, "\r\n");

   AddString(outstring, "Timer Registers\r\n");
   AddString(outstring, "-----------------\r\n");
   AddString(outstring, "Timer A counter: %02X\r\n", scsp.tima >> 8);
   AddString(outstring, "Timer A increment: Every %d sample(s)\r\n", 1 << scsp.tactl);
   AddString(outstring, "Timer B counter: %02X\r\n", scsp.timb >> 8);
   AddString(outstring, "Timer B increment: Every %d sample(s)\r\n", 1 << scsp.tbctl);
   AddString(outstring, "Timer C counter: %02X\r\n", scsp.timc >> 8);
   AddString(outstring, "Timer C increment: Every %d sample(s)\r\n", 1 << scsp.tcctl);
   AddString(outstring, "\r\n");

   AddString(outstring, "Interrupt Registers\r\n");
   AddString(outstring, "-----------------\r\n");
   AddString(outstring, "Sound cpu interrupt pending: %04X\r\n", scsp.scipd);
   AddString(outstring, "Sound cpu interrupt enable: %04X\r\n", scsp.scieb);
   AddString(outstring, "Sound cpu interrupt level 0: %04X\r\n", scsp.scilv0);
   AddString(outstring, "Sound cpu interrupt level 1: %04X\r\n", scsp.scilv1);
   AddString(outstring, "Sound cpu interrupt level 2: %04X\r\n", scsp.scilv2);
   AddString(outstring, "Main cpu interrupt pending: %04X\r\n", scsp.mcipd);
   AddString(outstring, "Main cpu interrupt enable: %04X\r\n", scsp.mcieb);
   AddString(outstring, "\r\n");
}

//-------------------------------------------------------------------------

// ScspSlotDebugSaveRegisters:  Write the values of a single slot's
// registers to a file.

int ScspSlotDebugSaveRegisters(u8 slotnum, const char *filename)
{
   FILE *fp;
   int i;
   IOCheck_struct check;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   for (i = (slotnum * 0x20); i < ((slotnum+1) * 0x20); i += 2)
   {
#ifdef WORDS_BIGENDIAN
      ywrite(&check, (void *)&scsp.regcache[i], 1, 2, fp);
#else
      ywrite(&check, (void *)&scsp.regcache[i+1], 1, 1, fp);
      ywrite(&check, (void *)&scsp.regcache[i], 1, 1, fp);
#endif
   }

   fclose(fp);
   return 0;
}

//-------------------------------------------------------------------------

// ScspSlotDebugAudioSaveWav:  Generate audio for a single slot and save it
// to a WAV file.

// Helper function to generate audio (defined below)
static u32 ScspSlotDebugAudio(SlotState *slot, s32 *workbuf, s16 *buf, u32 len);

int ScspSlotDebugAudioSaveWav(u8 slotnum, const char *filename)
{
   typedef struct {
      char id[4];
      u32 size;
   } chunk_struct;

   typedef struct {
      chunk_struct riff;
      char rifftype[4];
   } waveheader_struct;

   typedef struct {
      chunk_struct chunk;
      u16 compress;
      u16 numchan;
      u32 rate;
      u32 bytespersec;
      u16 blockalign;
      u16 bitspersample;
   } fmt_struct;

   s32 workbuf[512*2*2];
   s16 buf[512*2];
   SlotState slot;
   FILE *fp;
   u32 counter = 0;
   waveheader_struct waveheader;
   fmt_struct fmt;
   chunk_struct data;
   long length;
   IOCheck_struct check;

   if (scsp.slot[slotnum].lea == 0)
      return 0;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   // Do wave header
   memcpy(waveheader.riff.id, "RIFF", 4);
   waveheader.riff.size = 0; // we'll fix this after the file is closed
   memcpy(waveheader.rifftype, "WAVE", 4);
   ywrite(&check, (void *)&waveheader, 1, sizeof(waveheader_struct), fp);

   // fmt chunk
   memcpy(fmt.chunk.id, "fmt ", 4);
   fmt.chunk.size = 16; // we'll fix this at the end
   fmt.compress = 1; // PCM
   fmt.numchan = 2; // Stereo
   fmt.rate = 44100;
   fmt.bitspersample = 16;
   fmt.blockalign = fmt.bitspersample / 8 * fmt.numchan;
   fmt.bytespersec = fmt.rate * fmt.blockalign;
   ywrite(&check, (void *)&fmt, 1, sizeof(fmt_struct), fp);

   // data chunk
   memcpy(data.id, "data", 4);
   data.size = 0; // we'll fix this at the end
   ywrite(&check, (void *)&data, 1, sizeof(chunk_struct), fp);

   memcpy(&slot, &scsp.slot[slotnum], sizeof(slot));

   // Clear out the phase counter, etc.
   slot.fcnt = 0;
   slot.ecnt = SCSP_ENV_AS;
   slot.einc = slot.einca;
   slot.ecmp = SCSP_ENV_AE;
   slot.ecurp = SCSP_ENV_ATTACK;

   // Mix the audio, and then write it to the file
   for(;;)
   {
      if (ScspSlotDebugAudio(&slot, workbuf, buf, 512) == 0)
         break;

      counter += 512;
      ywrite(&check, (void *)buf, 2, 512 * 2, fp);
      if (slot.lpctl != 0 && counter >= (44100 * 2 * 5))
         break;
   }

   length = ftell(fp);

   // Let's fix the riff chunk size and the data chunk size
   fseek(fp, sizeof(waveheader_struct)-0x8, SEEK_SET);
   length -= 0x4;
   ywrite(&check, (void *)&length, 1, 4, fp);

   fseek(fp, sizeof(waveheader_struct)+sizeof(fmt_struct)+0x4, SEEK_SET);
   length -= sizeof(waveheader_struct)+sizeof(fmt_struct);
   ywrite(&check, (void *)&length, 1, 4, fp);
   fclose(fp);
   return 0;
}

//----------------------------------//

static u32 ScspSlotDebugAudio(SlotState *slot, s32 *workbuf, s16 *buf, u32 len)
{
   s32 *bufL, *bufR;

   bufL = workbuf;
   bufR = workbuf+len;
   scsp_bufL = bufL;
   scsp_bufR = bufR;
   scsp_buf_len = len;

   if (slot->ecnt >= SCSP_ENV_DE)
      return 0;  // Not playing

   if (slot->ssctl)
      return 0; // not yet supported!

   memset(bufL, 0, sizeof(u32) * len);
   memset(bufR, 0, sizeof(u32) * len);
   ScspGenerateAudioForSlot(slot);
   ScspConvert32uto16s(bufL, bufR, buf, len);
   return len;
}

//-------------------------------------------------------------------------

// ScspConvert32uto16s:  Saturate two 32-bit input sample buffers to 16-bit
// and interleave them into a single output buffer.

void ScspConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dest, u32 len)
{
   u32 i;

   for (i = 0; i < len; i++, srcL++, srcR++, dest += 2)
   {
      // Left channel
      if (*srcL > 0x7FFF)
         dest[0] = 0x7FFF;
      else if (*srcL < -0x8000)
         dest[0] = -0x8000;
      else
         dest[0] = *srcL;
      // Right channel
      if (*srcR > 0x7FFF)
         dest[1] = 0x7FFF;
      else if (*srcR < -0x8000)
         dest[1] = -0x8000;
      else
         dest[1] = *srcR;
   }
}

///////////////////////////////////////////////////////////////////////////
// SCSP register read/write routines and internal helpers
///////////////////////////////////////////////////////////////////////////

// Scsp{Read,Write}{Byte,Word}Direct:  Perform an SCSP register read or
// write.  These are internal routines that implement the actual register
// read/write logic.  The address must be in the range [0,0xFFF].

static u8 FASTCALL ScspReadByteDirect(u32 address)
{
   const u16 data = ScspReadWordDirect(address & ~1);
   if (address & 1)
      return data & 0xFF;
   else
      return data >> 8;
}

//----------------------------------//

static u16 FASTCALL ScspReadWordDirect(u32 address)
{
   switch (address) {
      case 0x404:  // MIDI in
         return ScspMidiIn();
      case 0x408:  // MSLC/CA
         return scsp.mslc   << 11
              | ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LOW_BITS + 12))
                 & 0xF)     <<  7;
      default:
         return scsp.regcache[address >> 1];
   }
}

//----------------------------------//

static void FASTCALL ScspWriteByteDirect(u32 address, u8 data)
{
   switch (address >> 8)
   {
      case 0x0:
      case 0x1:
      case 0x2:
      case 0x3:
      {
         const int slotnum = (address & 0x3E0) >> 5;
         SlotState *slot = &scsp.slot[slotnum];
         switch (address & 0x1F)
         {
            case 0x00:
               // FIXME/SCSP1: this is needed to mimic scsp1 and avoid
               // ScspUpdateSlotAddress() if not touching PCM8B/SA;
               // it might not be necessary
               slot->key    = (data >> 3) & 0x1;
               slot->sbctl  = (data >> 1) & 0x3;
               slot->ssctl  =((data >> 0) & 0x1) << 1 | (slot->ssctl & 0x1);
               if (data & (1<<4))
                  ScspDoKeyOnOff();
               data &= 0x0F;  // Don't save KYONEX
               break;

            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x0F:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            write_as_word:
            {
               // These can be treated as word writes, borrowing the missing
               // 8 bits from the register cache
               u16 word_data;
               if (address & 1)
                  word_data = (scsp.regcache[address >> 1] & 0xFF00) | data;
               else
                  word_data = (scsp.regcache[address >> 1] & 0x00FF) | (data << 8);
               ScspWriteWordDirect(address & ~1, word_data);
               return;
            }

            default:
               goto unhandled_write;
         }
         break;
      }

      case 0x4:
         switch (address & 0xFF)
         {
            // FIXME: if interrupts are only triggered on 0->1 changes in
            // [SM]CIEB, we can skip 0x1E/0x1F/0x26/0x27 (see FIXME in
            // ScspWriteWordDirect())

            case 0x1E:
               data &= 0x07;
               scsp.scieb = (data << 8) | (scsp.scieb & 0x00FF);
               ScspCheckInterrupts(0x700, SCSP_INTTARGET_SOUND);
               break;

            case 0x1F:
               scsp.scieb = (scsp.scieb & 0xFF00) | data;
               ScspCheckInterrupts(0x700, SCSP_INTTARGET_SOUND);
               break;

            case 0x20:
               return;  // Not writable

            case 0x21:
               if (data & (1<<5))
                  ScspRaiseInterrupt(5, SCSP_INTTARGET_SOUND);
               return;  // Not writable

            case 0x2A:
               data &= 0x07;
               scsp.mcieb = (data << 8) | (scsp.mcieb & 0x00FF);
               ScspCheckInterrupts(0x700, SCSP_INTTARGET_MAIN);
               break;

            case 0x2B:
               scsp.mcieb = (scsp.mcieb & 0xFF00) | data;
               ScspCheckInterrupts(0x700, SCSP_INTTARGET_MAIN);
               break;

            case 0x2C:
               return;  // Not writable

            case 0x2D:
               if (data & (1<<5))
                  ScspRaiseInterrupt(5, SCSP_INTTARGET_MAIN);
               return;  // Not writable

            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
            case 0x2E:
            case 0x2F:
               goto write_as_word;

            default:
               goto unhandled_write;
         }
         break;

      default:
      unhandled_write:
         SCSPLOG("ScspWriteByteDirect(): unhandled write %02X to 0x%03X\n",
                 data, address);
         break;
   }

   if (address & 1)
   {
      scsp.regcache[address >> 1] &= 0xFF00;
      scsp.regcache[address >> 1] |= data;
   }
   else
   {
      scsp.regcache[address >> 1] &= 0x00FF;
      scsp.regcache[address >> 1] |= data << 8;
   }
}

//----------------------------------//

static void FASTCALL ScspWriteWordDirect(u32 address, u16 data)
{
if(address>=0x412&&address<0x41E)SCSPLOG("write(%03X,%04X)\n",address,data);
   switch (address >> 8)
   {
      case 0x0:
      case 0x1:
      case 0x2:
      case 0x3:
      {
         const int slotnum = (address & 0x3E0) >> 5;
         SlotState *slot = &scsp.slot[slotnum];
         switch (address & 0x1F)
         {
            case 0x00:
               slot->key    = (data >> 11) & 0x1;
               slot->sbctl  = (data >>  9) & 0x3;
               slot->ssctl  = (data >>  7) & 0x3;
               slot->lpctl  = (data >>  5) & 0x3;
               slot->pcm8b  = (data >>  4) & 0x1;
               slot->sa     =((data >>  0) & 0xF) << 16 | (slot->sa & 0xFFFF);
               if (slot->ecnt < SCSP_ENV_DE)
                  ScspUpdateSlotAddress(slot);
               if (data & (1<<12))
                  ScspDoKeyOnOff();
               data &= 0x0FFF;  // Don't save KYONEX
               break;

            case 0x02:
               slot->sa     = (slot->sa & 0xF0000) | data;
               if (slot->ecnt < SCSP_ENV_DE)
                  ScspUpdateSlotAddress(slot);
               break;

            case 0x04:
               slot->lsa    = data;
               // FIXME/SCSP1: disabled because scsp1 didn't do it
               //if (slot->ecnt < SCSP_ENV_DE)
               //   ScspUpdateSlotAddress(slot);
               // FIXME/SCSP1: the next line can be dropped if the above is
               // uncommented
               slot->lsa_shifted = slot->lsa << SCSP_FREQ_LOW_BITS;
               break;

            case 0x06:
               slot->lea    = data;
               // FIXME/SCSP1: disabled because scsp1 didn't do it
               //if (slot->ecnt < SCSP_ENV_DE)
               //   ScspUpdateSlotAddress(slot);
               // FIXME/SCSP1: the next line can be dropped if the above is
               // uncommented
               slot->lea_shifted = slot->lea << SCSP_FREQ_LOW_BITS;
               break;

            case 0x08:
               slot->sr     = (data >> 11) & 0x1F;
               slot->dr     = (data >>  6) & 0x1F;
               slot->eghold = (data >>  5) & 0x1;
               slot->ar     = (data >>  0) & 0x1F;

               if (slot->sr)
                  slot->srp = &scsp_decay_rate[slot->sr << 1];
               else
                  slot->srp = &scsp_null_rate[0];
               slot->eincs = slot->srp[(14 - slot->fsft) >> slot->krs_shift];

               if (slot->dr)
                  slot->drp = &scsp_decay_rate[slot->dr << 1];
               else
                  slot->drp = &scsp_null_rate[0];
               slot->eincd = slot->drp[(14 - slot->fsft) >> slot->krs_shift];

               if (slot->ar)
                  slot->arp = &scsp_attack_rate[slot->ar << 1];
               else
                  slot->arp = &scsp_null_rate[0];
               slot->einca = slot->arp[(14 - slot->fsft) >> slot->krs_shift];

               break;

            case 0x0A:
               data &= 0x7FFF;
               slot->lpslnk = (data >> 14) & 0x1;
               slot->krs    = (data >> 10) & 0xF;
               slot->sl     = (data >>  5) & 0x1F;
               slot->rr     = (data >>  0) & 0x1F;

               if (slot->krs == 0xF)
                  slot->krs_shift = 4;
               else
                  slot->krs_shift = slot->krs >> 2;

               slot->slcmp = (slot->sl << (5 + SCSP_ENV_LOW_BITS)) + SCSP_ENV_DS;

               if (slot->rr)
                  slot->rrp = &scsp_decay_rate[slot->rr << 1];
               else
                  slot->rrp = &scsp_null_rate[0];
               slot->eincr = slot->rrp[(14 - slot->fsft) >> slot->krs_shift];

               break;

            case 0x0C:
               data &= 0x03FF;
               slot->stwinh = (data >>  9) & 0x1;
               slot->sdir   = (data >>  8) & 0x1;
               slot->tl     = (data >>  0) & 0xFF;

               slot->tl_mult = scsp_tl_table[slot->tl];

               break;

            case 0x0E:
               slot->mdl    = (data >> 12) & 0xF;
               slot->mdx    = (data >>  6) & 0x3F;
               slot->mdy    = (data >>  0) & 0x3F;
               break;

            case 0x10:
               data &= 0x7BFF;
               slot->oct    = (data >> 11) & 0xF;
               slot->fns    = (data >>  0) & 0x3FF;

               if (slot->oct & 8)
                  slot->fsft = 23 - slot->oct;
               else
                  slot->fsft = 7 - slot->oct;
               slot->finc = ((0x400 + slot->fns) << 7) >> slot->fsft;

               break;

            case 0x12:
               slot->lfore  = (data >> 15) & 0x1;
               slot->lfof   = (data >> 10) & 0x1F;
               slot->plfows = (data >>  8) & 0x3;
               slot->plfos  = (data >>  5) & 0x7;
               slot->alfows = (data >>  3) & 0x3;
               slot->alfos  = (data >>  0) & 0x7;

               if (slot->lfore)
               {
                  slot->lfoinc = -1;
               }
               else
               {
                  // FIXME/SCSP1: would be simpler to clear this in the LFORE branch
                  if (slot->lfoinc == -1)
                     slot->lfocnt = 0;
               }

               slot->lfoinc = scsp_lfo_step[slot->lfof];
               if (slot->plfos)
                  slot->lfofms = slot->plfos + 7;
               else
                  slot->lfofms = 31;
               if (slot->alfos)
                  slot->lfoems = 11 - slot->alfos;
               else
                  slot->lfoems = 31;

               switch (slot->plfows)
               {
                  case 0:
                     slot->lfofmw = scsp_lfo_sawt_f;
                     break;
                  case 1:
                     slot->lfofmw = scsp_lfo_squa_f;
                     break;
                  case 2:
                     slot->lfofmw = scsp_lfo_tri_f;
                     break;
                  case 3:
                     slot->lfofmw = scsp_lfo_noi_f;
                     break;
               }

               switch (slot->alfows)
               {
                  case 0:
                     slot->lfoemw = scsp_lfo_sawt_e;
                     break;
                  case 1:
                     slot->lfoemw = scsp_lfo_squa_e;
                     break;
                  case 2:
                     slot->lfoemw = scsp_lfo_tri_e;
                     break;
                  case 3:
                     slot->lfoemw = scsp_lfo_noi_e;
                     break;
               }

               break;

            case 0x14:
               data &= 0x007F;
               slot->isel   = (data >>  3) & 0xF;
               slot->imxl   = (data >>  0) & 0x7;

               if (slot->imxl)
                  slot->imxl_shift = (7 - slot->imxl) + SCSP_ENV_HIGH_BITS;
               else
                  slot->imxl_shift = 31;

               break;

            case 0x16:
               slot->disdl  = (data >> 13) & 0x7;
               slot->dipan  = (data >>  8) & 0x1F;
               slot->efsdl  = (data >>  5) & 0x7;
               slot->efpan  = (data >>  0) & 0x1F;

               if (slot->disdl)
               {
                  slot->dislr = slot->disll = (7 - slot->disdl) + SCSP_ENV_HIGH_BITS;
                  if (slot->dipan & 0x10)  // Pan left
                  {
                     if (slot->dipan == 0x1F)
                        slot->dislr = 31;
                     else
                        slot->dislr += (slot->dipan >> 1) & 7;
                  }
                  else  // Pan right
                  {
                     if (slot->dipan == 0xF)
                        slot->disll = 31;
                     else
                        slot->disll += (slot->dipan >> 1) & 7;
                  }
               }
               else
                  slot->disll = slot->dislr = 31;  // Muted

               if (slot->efsdl)
               {
                  slot->efslr = slot->efsll = (7 - slot->efsdl) + SCSP_ENV_HIGH_BITS;
                  if (slot->efpan & 0x10)  // Pan left
                  {
                     if (slot->efpan == 0x1F)
                        slot->efslr = 31;
                     else
                        slot->efslr += (slot->efpan >> 1) & 7;
                  }
                  else  // Pan right
                  {
                     if (slot->efpan == 0xF)
                        slot->efsll = 31;
                     else
                        slot->efsll += (slot->efpan >> 1) & 7;
                  }
               }
               else
                  slot->efsll = slot->efslr = 31;  // Muted

               break;

            default:
               goto unhandled_write;
         }
         break;
      }

      case 0x4:
         switch (address & 0xFF)
         {
            case 0x00:
               data &= 0x030F;  // VER is hardwired
               data |= SCSP_VERSION << 4;
               scsp.mem4mb = (data >>  9) & 0x1;
               scsp.dac18b = (data >>  8) & 0x1;
               scsp.mvol   = (data >>  0) & 0xF;

               if (scsp.mem4mb)
                  M68K->SetFetch(0x000000, 0x080000, (pointer)SoundRam);
               else
               {
                  M68K->SetFetch(0x000000, 0x040000, (pointer)SoundRam);
                  M68K->SetFetch(0x040000, 0x080000, (pointer)SoundRam);
                  M68K->SetFetch(0x080000, 0x0C0000, (pointer)SoundRam);
                  M68K->SetFetch(0x0C0000, 0x100000, (pointer)SoundRam);
               }
               scsp.sound_ram_mask = scsp.mem4mb ? 0x7FFFF : 0x3FFFF;

               break;

            case 0x02:
               data &= 0x01FF;
               scsp.rbl    = (data >>  7) & 0x3;
               scsp.rbp    =((data >>  0) & 0x7F) << 13;
               break;

            case 0x04:
               return;  // Not writable

            case 0x06:
               data &= 0x00FF;
               ScspMidiOut(data);
               break;

            case 0x08:
               data &= 0x7800;  // CA is not writable
               scsp.mslc   = (data >> 11) & 0x1F;
               break;

            case 0x12:
               data &= 0xFFFE;
               scsp.dmea   = (scsp.dmea & 0xF0000) | data;
               break;

            case 0x14:
               data &= 0xFFFE;
               scsp.dmea   =((data >> 12) & 0xF) << 16 | (scsp.dmea & 0xFFFF);
               scsp.drga   = (data >>  0) & 0xFFF;
               break;

            case 0x16:
               data &= 0x7FFE;
               scsp.dgate  = (data >> 14) & 0x1;
               scsp.ddir   = (data >> 13) & 0x1;
               scsp.dexe  |= (data >> 12) & 0x1;  // Writing 0 not allowed
               scsp.dtlg   = (data >>  0) & 0xFFF;
               if (data & (1<<12))
                  ScspDoDMA();
               break;

            case 0x18:
               data &= 0x07FF;
               scsp.tactl  = (data >>  8) & 0x7;
               scsp.tima   =((data >>  0) & 0xFF) << 8;
               break;

            case 0x1A:
               data &= 0x07FF;
               scsp.tbctl  = (data >>  8) & 0x7;
               scsp.timb   =((data >>  0) & 0xFF) << 8;
               break;

            case 0x1C:
               data &= 0x07FF;
               scsp.tcctl  = (data >>  8) & 0x7;
               scsp.timc   =((data >>  0) & 0xFF) << 8;
               break;

            case 0x1E:
               data &= 0x07FF;
               scsp.scieb = data;
               // FIXME: If a bit is already 1 in both SCIEB and SCIPD,
               // does writing another 1 here (no change) trigger another
               // interrupt or not?
               ScspCheckInterrupts(0x7FF, SCSP_INTTARGET_SOUND);
               break;

            case 0x20:
               if (data & (1<<5))
                  ScspRaiseInterrupt(5, SCSP_INTTARGET_SOUND);
               return;  // Not writable

            case 0x22:
               ScspClearInterrupts(data, SCSP_INTTARGET_SOUND);
               return;  // Not writable

            case 0x24:
               data &= 0x00FF;
               scsp.scilv0 = data;
               break;

            case 0x26:
               data &= 0x00FF;
               scsp.scilv1 = data;
               break;

            case 0x28:
               data &= 0x00FF;
               scsp.scilv2 = data;
               break;

            case 0x2A:
               data &= 0x07FF;
               scsp.mcieb = data;
               // FIXME: as above (SCIEB)
               ScspCheckInterrupts(0x7FF, SCSP_INTTARGET_MAIN);
               break;

            case 0x2C:
               if (data & (1<<5))
                  ScspRaiseInterrupt(5, SCSP_INTTARGET_MAIN);
               return;  // Not writable

            case 0x2E:
               ScspClearInterrupts(data, SCSP_INTTARGET_MAIN);
               return;  // Not writable

            default:
               goto unhandled_write;
         }
         break;

      default:
      unhandled_write:
         SCSPLOG("ScspWriteWordDirect(): unhandled write %04X to 0x%03X\n",
                 data, address);
         break;
   }

   scsp.regcache[address >> 1] = data;
}

//-------------------------------------------------------------------------

// ScspDoKeyOnOff:  Apply the key-on/key-off setting for all slots.
// Implements the KYONEX trigger.

static void ScspDoKeyOnOff(void)
{
   int slotnum;
   for (slotnum = 0; slotnum < 32; slotnum++)
   {
      if (scsp.slot[slotnum].key)
         ScspKeyOn(&scsp.slot[slotnum]);
      else
         ScspKeyOff(&scsp.slot[slotnum]);
   }
}

//----------------------------------//

// ScspKeyOn:  Execute a key-on event for a single slot.

static void ScspKeyOn(SlotState *slot)
{
   if (slot->ecurp != SCSP_ENV_RELEASE)
      return;  // Can't key a sound that's already playing

   ScspUpdateSlotAddress(slot);

   slot->fcnt = 0;                 // Start at beginning of sample
   slot->ecurp = SCSP_ENV_ATTACK;  // Begin attack phase
   slot->ecnt = SCSP_ENV_AS;       // Reset envelope (FIXME: should start at current if old sound is still decaying?)
   slot->einc = slot->einca;       // Load precomputed attack step value
   slot->ecmp = SCSP_ENV_AE;       // Target ecnt value for advancing phase
}

//----------------------------------//

// ScspKeyOff:  Execute a key-off event for a single slot.

static void ScspKeyOff(SlotState *slot)
{
   if (slot->ecurp == SCSP_ENV_RELEASE)
      return;  // Can't release a sound that's already released

   // If we still are in attack phase at release time, convert attack to decay
   if (slot->ecurp == SCSP_ENV_ATTACK)
      slot->ecnt = SCSP_ENV_DE - slot->ecnt;

   slot->ecurp = SCSP_ENV_RELEASE;
   slot->einc = slot->eincr;
   slot->ecmp = SCSP_ENV_DE;
}

//-------------------------------------------------------------------------

// ScspUpdateSlotAddress:  Update the sample data pointer for the given
// slot, bound slot->lea to the end of sound RAM, and cache the shifted
// values of slot->lsa and slot->lea in slot->{lea,lsa}_shifted.

static void ScspUpdateSlotAddress(SlotState *slot)
{
   u32 max_samples;

   if (slot->pcm8b)
      slot->sa &= ~1;
   // FIXME/SCSP1: should slot->sa be masked by scsp.sound_ram_mask?
   slot->buf = &SoundRam[slot->sa];
   // FIXME/SCSP1: should this likewise use scsp.sound_ram_mask?
   max_samples = (SCSP_RAM_MASK + 1) - slot->sa;
   if (slot->pcm8b)
      max_samples >>= 1;

   // FIXME/SCSP1: should tweak slot->lsa as well
   //if (slot->lsa > max_samples)
   //   slot->lsa = max_samples;
   slot->lsa_shifted = slot->lsa << SCSP_FREQ_LOW_BITS;

   if (slot->lea > max_samples)
      slot->lea = max_samples;
   slot->lea_shifted = slot->lea << SCSP_FREQ_LOW_BITS;

   // FIXME/SCSP1: scsp1 advanced the address counter (fcnt) here, but that
   // looks wrong to me -- shouldn't it only be advanced during ScspExec()?
}

//-------------------------------------------------------------------------

// ScspMidiIn:  Handle a read from the MIDI input register ($404).  Since
// there is no facility for sending MIDI data (the Saturn does not have a
// MIDI I/O port), most of this is essentially a giant no-op, but the logic
// is included for reference.

static u16 ScspMidiIn(void)
{
   scsp.miovf = 0;
   scsp.mifull = 0;
   if (scsp.midi_in_cnt > 0)
   {
      scsp.mibuf = scsp.midi_in_buf[0];
      scsp.midi_in_buf[0] = scsp.midi_in_buf[1];
      scsp.midi_in_buf[1] = scsp.midi_in_buf[2];
      scsp.midi_in_buf[2] = scsp.midi_in_buf[3];
      scsp.midi_in_cnt--;
      scsp.miemp = (scsp.midi_in_cnt == 0);
      if (!scsp.miemp)
         ScspRaiseInterrupt(SCSP_INTERRUPT_MIDI_IN, SCSP_INTTARGET_BOTH);
   }
   else  // scsp.midi_in_cnt == 0
      scsp.mibuf = 0xFF;

   return scsp.mofull << 12
        | scsp.moemp  << 11
        | scsp.miovf  << 10
        | scsp.mifull <<  9
        | scsp.miemp  <<  8
        | scsp.mibuf  <<  0;
}

//----------------------------------//

// ScspMidiOut:  Handle a write to the MIDI output register ($406).

static void ScspMidiOut(u8 data)
{
   scsp.moemp = 0;
   if (scsp.midi_out_cnt < 4)
      scsp.midi_out_buf[scsp.midi_out_cnt++] = data;
   scsp.mofull = (scsp.midi_out_cnt >= 4);
}

//-------------------------------------------------------------------------

// ScspDoDMA:  Handle a DMA request (when 1 is written to DEXE).  DMA is
// processed instantaneously regardless of transfer size.

static void ScspDoDMA(void)
{
#if 0  // FIXME/SCSP1: disabled for now because scsp1 didn't implement it
   const u32 dmea = scsp.dmea & scsp.sound_ram_mask;

   if (scsp.ddir)  // {RAM,zero} -> registers
   {
      SCSPLOG("DMA %s RAM[$%05X] -> registers[$%03X]\n",
              scsp.dgate ? "clear" : "copy", dmea, scsp.drga);
      if (scsp.dgate)
      {
         u32 i;
         for (i = 0; i < scsp.dtlg; i += 2)
            ScspWriteWordDirect(scsp.drga + i, 0);
      }
      else
      {
         u32 i;
         for (i = 0; i < scsp.dtlg; i += 2)
            ScspWriteWordDirect(scsp.drga + i, T2ReadWord(SoundRam, dmea + i));
      }
   }
   else  // !scsp.ddir
   {
      SCSPLOG("DMA %s registers[$%03X] -> RAM[$%05X]\n",
              scsp.dgate ? "clear" : "copy", scsp.drga, dmea);
      if (scsp.dgate)
         memset(&SoundRam[dmea], 0, scsp.dtlg);
      else
      {
         u32 i;
         for (i = 0; i < scsp.dtlg; i += 2)
            T2WriteWord(SoundRam, dmea + i, ScspReadWordDirect(scsp.drga + i));
      }
      M68K->WriteNotify(dmea, scsp.dtlg);
   }
#endif  // FIXME/SCSP1: see above

   scsp.dexe = 0;
   scsp.regcache[0x416>>1] &= ~(1<<12);
   ScspRaiseInterrupt(SCSP_INTERRUPT_DMA, SCSP_INTTARGET_BOTH);
}

///////////////////////////////////////////////////////////////////////////
// Other SCSP internal helper routines
///////////////////////////////////////////////////////////////////////////

// ScspRaiseInterrupt:  Raise an interrupt for the main and/or sound CPU.

static void ScspRaiseInterrupt(int which, int target)
{
   if (target & SCSP_INTTARGET_MAIN)
   {
      scsp.mcipd |= 1 << which;
      scsp.regcache[0x42C >> 1] = scsp.mcipd;
      if (scsp.mcieb & (1 << which))
         (*scsp_interrupt_handler)();
   }

   if (target & SCSP_INTTARGET_SOUND)
   {
      scsp.scipd |= 1 << which;
      scsp.regcache[0x420 >> 1] = scsp.scipd;
      if (scsp.scieb & (1 << which))
      {
         const int level_shift = (which > 7) ? 7 : which;
         const int level = ((scsp.scilv0 >> level_shift) & 1) << 0
                         | ((scsp.scilv1 >> level_shift) & 1) << 1
                         | ((scsp.scilv2 >> level_shift) & 1) << 2;
         M68K->SetIRQ(level);
      }
   }
}

//----------------------------------//

// ScspCheckInterrupts:  Check pending interrupts for the main or sound CPU
// against the interrupt enable flags, and raise any interrupts which are
// both enabled and pending.  The mask parameter indicates which interrupts
// should be checked.  Implements writes to SCIEB and MCIEB.

static void ScspCheckInterrupts(u16 mask, int target)
{
   int i;

   for (i = 0; i < 11; i++)
   {
      if ((1<<i) & mask & scsp.mcieb && scsp.mcipd)
         ScspRaiseInterrupt(i, SCSP_INTTARGET_MAIN & target);
      if ((1<<i) & mask & scsp.scieb && scsp.scipd)
         ScspRaiseInterrupt(i, SCSP_INTTARGET_SOUND & target);
   }
}

//----------------------------------//

// ScspClearInterrupts:  Clear all pending interrupts specified by the mask
// parameter for the main or sound CPU.  Implements writes to SCIRE and MCIRE.

static void ScspClearInterrupts(u16 mask, int target)
{
   if (target & SCSP_INTTARGET_MAIN)
   {
      scsp.mcipd &= ~mask;
      scsp.regcache[0x42C >> 1] = scsp.mcipd;
   }

   if (target & SCSP_INTTARGET_SOUND)
   {
      scsp.scipd &= ~mask;
      scsp.regcache[0x420 >> 1] = scsp.scipd;
   }
}

///////////////////////////////////////////////////////////////////////////
// M68K management routines
///////////////////////////////////////////////////////////////////////////

// M68KReset:  Reset the M68K processor to its power-on state.

void M68KReset(void)
{
   M68K->Reset();
   m68k_saved_cycles = 0;
}

//-------------------------------------------------------------------------

// M68KExec:  Run the M68K for the given number of M68K clock cycles.

void M68KExec(s32 cycles)
{
   s32 new_cycles = m68k_saved_cycles - cycles;
   if (LIKELY(yabsys.IsM68KRunning))
   {
      if (LIKELY(new_cycles < 0))
      {
         s32 cycles_to_exec = -new_cycles;
         new_cycles += (*m68k_execf)(cycles_to_exec);
      }
      m68k_saved_cycles = new_cycles;
   }
}

//-------------------------------------------------------------------------

// M68KExecBP:  Wrapper for M68K->Exec() which checks for breakpoints and
// calls the breakpoint callback when one is reached.  This logic is
// extracted from M68KExec() to avoid unnecessary register spillage on the
// fast (no-breakpoint) path.

static s32 FASTCALL M68KExecBP(s32 cycles)
{
   s32 cycles_to_exec = cycles;
   s32 cycles_executed = 0;
   int i;

   while (cycles_executed < cycles_to_exec)
   {
      // Make sure it isn't one of our breakpoints
      for (i = 0; i < m68k_num_breakpoints; i++) {
         if ((M68K->GetPC() == m68k_breakpoint[i].addr) && !m68k_in_breakpoint) {
            m68k_in_breakpoint = 1;
            if (M68KBreakpointCallback)
               M68KBreakpointCallback(m68k_breakpoint[i].addr);
            m68k_in_breakpoint = 0;
         }
      }

      // Execute instructions individually
      cycles_executed += M68K->Exec(1);
   }

   return cycles_executed;
}

//-------------------------------------------------------------------------

// M68KStep:  Execute a single M68K instruction.

void M68KStep(void)
{
   M68K->Exec(1);
}

//-------------------------------------------------------------------------

// M68KSync:  Wait for background M68K emulation to finish.  Used on the PSP.

void M68KSync(void)
{
   M68K->Sync();
}

//-------------------------------------------------------------------------

// M68KWriteNotify:  Notify the M68K emulator that a region of sound RAM
// has been written to by an external agent.

void M68KWriteNotify(u32 address, u32 size)
{
   M68K->WriteNotify(address, size);
}

//-------------------------------------------------------------------------

// M68KGetRegisters, M68KSetRegisters:  Get or set the current values of
// the M68K registers.

void M68KGetRegisters(M68KRegs *regs)
{
   int i;

   if (regs != NULL)
   {
      for (i = 0; i < 8; i++)
      {
         regs->D[i] = M68K->GetDReg(i);
         regs->A[i] = M68K->GetAReg(i);
      }
      regs->SR = M68K->GetSR();
      regs->PC = M68K->GetPC();
   }
}

void M68KSetRegisters(const M68KRegs *regs)
{
   int i;

   if (regs != NULL)
   {
      for (i = 0; i < 8; i++)
      {
         M68K->SetDReg(i, regs->D[i]);
         M68K->SetAReg(i, regs->A[i]);
      }
      M68K->SetSR(regs->SR);
      M68K->SetPC(regs->PC);
   }
}

//-------------------------------------------------------------------------

// M68KSetBreakpointCallback:  Set a function to be called whenever an M68K
// breakpoint is reached.

void M68KSetBreakpointCallBack(void (*func)(u32 address))
{
   M68KBreakpointCallback = func;
}

//-------------------------------------------------------------------------

// M68KAddCodeBreakpoint:  Add an M68K breakpoint on the given address.
// Returns 0 on success, -1 if the breakpoint table is full or there is
// already a breakpoint set on the address.

int M68KAddCodeBreakpoint(u32 address)
{
   int i;

   if (m68k_num_breakpoints >= MAX_BREAKPOINTS)
      return -1;

   // Make sure it isn't already on the list
   for (i = 0; i < m68k_num_breakpoints; i++)
   {
      if (m68k_breakpoint[i].addr == address)
         return -1;
   }

   m68k_breakpoint[m68k_num_breakpoints].addr = address;
   m68k_num_breakpoints++;

   // Switch to the slow exec routine so we can catch the breakpoint
   m68k_execf = M68KExecBP;

   return 0;
}

//-------------------------------------------------------------------------

// M68KDelCodeBreakpoint:  Delete an M68K breakpoint on the given address.
// Returns 0 on success, -1 if there was no breakpoint set on the address.

int M68KDelCodeBreakpoint(u32 address)
{
   int i;

   if (m68k_num_breakpoints > 0)
   {
      for (i = 0; i < m68k_num_breakpoints; i++)
      {
         if (m68k_breakpoint[i].addr == address)
         {
            // Swap with the last breakpoint in the table, so there are
            // no holes in the breakpoint list
            m68k_breakpoint[i].addr = m68k_breakpoint[m68k_num_breakpoints-1].addr;
            m68k_breakpoint[m68k_num_breakpoints-1].addr = 0xFFFFFFFF;
            m68k_num_breakpoints--;

            if (m68k_num_breakpoints == 0) {
               // Last breakpoint deleted, so go back to the fast exec routine
               m68k_execf = M68K->Exec;
            }

            return 0;
         }
      }
   }

   return -1;
}

//-------------------------------------------------------------------------

// M68KGetBreakpointList:  Return the array of breakpoints currently set.
// The array is M68K_MAX_BREAKPOINTS elements long, and an address of
// 0xFFFFFFFF indicates that no breakpoint is set in that slot.

const M68KBreakpointInfo *M68KGetBreakpointList(void)
{
   return m68k_breakpoint;
}

//-------------------------------------------------------------------------

// M68KClearCodeBreakpoints:  Clear all M68K breakpoints.

void M68KClearCodeBreakpoints(void)
{
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++)
      m68k_breakpoint[i].addr = 0xFFFFFFFF;

   m68k_num_breakpoints = 0;
   m68k_execf = M68K->Exec;
}

//-------------------------------------------------------------------------

// M68K{Read,Write}{Byte,Word}:  Memory access routines for the M68K
// emulation.  Exported for use in debugging.

// FIXME/SCSP1: replace 0x7FFFF with scsp.sound_ram_mask

u32 FASTCALL M68KReadByte(u32 address)
{
   if (address < 0x100000)
      return T2ReadByte(SoundRam, address & 0x7FFFF);
   else
      return ScspReadByteDirect(address & 0xFFF);
}

u32 FASTCALL M68KReadWord(u32 address)
{
   if (address < 0x100000)
      return T2ReadWord(SoundRam, address & 0x7FFFF);
   else
      return ScspReadWordDirect(address & 0xFFF);
}

void FASTCALL M68KWriteByte(u32 address, u32 data)
{
   if (address < 0x100000)
      T2WriteByte(SoundRam, address & 0x7FFFF, data);
   else
      ScspWriteByteDirect(address & 0xFFF, data);
}

void FASTCALL M68KWriteWord(u32 address, u32 data)
{
   if (address < 0x100000)
      T2WriteWord(SoundRam, address & 0x7FFFF, data);
   else
      ScspWriteWordDirect(address & 0xFFF, data);
}

///////////////////////////////////////////////////////////////////////////

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-basic-offset: 3
 *   c-file-offsets: ((case-label . +))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=3:
 */