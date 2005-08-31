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
int YuiInit(void);

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

/* Sets CDROM filename in the yui - used to specify the CDROM from the commandline */
void YuiSetCdromFilename(const char *);

// Helper functions(you can use these in your own port)

/* int MappedMemoryLoad(const char *filename, u32 addr);

   Loads the specified file(filename) to specified address(addr). Returns zero
   on success, less than zero if an error has occured.
   Note: Some areas in memory are read-only and won't acknowledge any writes.
*/

/* int MappedMemorySave(const char *filename, u32 addr, u32 size);

   Saves data from specified address(addr) by specified amount of bytes(size)
   to specified file(filename). Returns zero on success, less than zero if
   an error has occured.
   Note: Some areas in memory are write-only and will only return zero on
   reads.
*/

/* void MappedMemoryLoadExec(const char *filename, u32 pc);

   Loads the specified file(filename) to specified address(pc) and sets
   Master SH2 to execute from there.
   Note: Some areas in memory are read-only and won't acknowledge any writes.
*/

/* void FormatBackupRam(void *mem, u32 size);

   Formats the specified Backup Ram memory area(mem) of specified size(size).
*/

/* void SH2Disasm(u32 v_addr, u16 op, int mode, char *string);

   Generates a disassembled instruction into specified string(string) based
   on specified address(v_addr) and specified opcode(op). mode should always
   be 0.
*/

/* void SH2Step(SH2_struct *context);

   For the specified SH2 context(context), it executes 1 instruction. context
   should be either MSH2 or SSH2.
*/

/* void SH2GetRegisters(SH2_struct *context, sh2regs_struct * r);

   For the specified SH2 context(context), copies the current registers into
   the specified structure(r). context should be either MSH2 or SSH2.
*/

/* void SH2SetRegisters(SH2_struct *context, sh2regs_struct * r);

   For the specified SH2 context(context), copies the specified structure(r)
   to the current registers. context should be either MSH2 or SSH2.
*/

/* void SH2SetBreakpointCallBack(SH2_struct *context, void (*func)(void *, u32));

   For the specified SH2 context(context), it sets the breakpoint handler
   function(func). context should be either MSH2 or SSH2.
*/

/* int SH2AddCodeBreakpoint(SH2_struct *context, u32 addr);

   For the specified SH2 context(context), it adds a code breakpoint for
   specified address(addr). context should be either MSH2 or SSH2. Returns
   zero on success, or less than zero if an error has occured(such as the
   breakpoint list being full)
*/

/* int SH2DelCodeBreakpoint(SH2_struct *context, u32 addr);

   For the specified SH2 context(context), it deletes a code breakpoint for
   specified address(addr). context should be either MSH2 or SSH2. Returns
   zero on success, or less than zero if an error has occured.
*/

/* codebreakpoint_struct *SH2GetBreakpointList(SH2_struct *context);

   For the specified SH2 context(context), it returns a pointer to the
   code breakpoint list for the processor. context should be either MSH2 or
   SSH2.
*/

/* void SH2ClearCodeBreakpoints(SH2_struct *context);

   For the specified SH2 context(context), it deletes every code breakpoint
   entry. context should be either MSH2 or SSH2.
*/

/* void Vdp2DebugStatsRBG0(char *outstring, int *isenabled);
   void Vdp2DebugStatsNBG0(char *outstring, int *isenabled);
   void Vdp2DebugStatsNBG1(char *outstring, int *isenabled);
   void Vdp2DebugStatsNBG2(char *outstring, int *isenabled);
   void Vdp2DebugStatsNBG3(char *outstring, int *isenabled);

   Fills a specified string pointer(outstring) with debug information for the
   specified screen. It also fills a variable(isenabled) with the screen's
   current status 
*/

#endif
