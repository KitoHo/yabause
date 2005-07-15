#include <stdlib.h>
#include "smpc.h"
#include "debug.h"

Smpc * SmpcRegs;
u8 * SmpcRegsT;

int SmpcInit(void) {
	SmpcRegsT = (u8 *) malloc(sizeof(SmpcRegs));
	if (SmpcRegsT == NULL)
		return -1;

	SmpcRegs = (Smpc *) SmpcRegsT;

	SmpcReset();

	return 0;
}

void SmpcDeInit(void) {
	free(SmpcRegsT);
}

void SmpcReset(void) {
}

u8 FASTCALL SmpcReadByte(u32 addr) {
	return SmpcRegsT[addr >> 1];
}

u16 FASTCALL SmpcReadWord(u32 addr) {
	LOG("Trying to word-read a Smpc register\n");
	return 0;
}

u32 FASTCALL SmpcReadLong(u32 addr) {
	LOG("Trying to long-read a Smpc register\n");
	return 0;
}

void FASTCALL SmpcWriteByte(u32 addr, u8 val) {
	SmpcRegsT[addr >> 1] = val;
}

void FASTCALL SmpcWriteWord(u32 addr, u16 val) {
	LOG("Trying to word-write a Smpc register\n");
}

void FASTCALL SmpcWriteLong(u32 addr, u32 val) {
	LOG("Trying to long-write a Smpc register\n");
}
