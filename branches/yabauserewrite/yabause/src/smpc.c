#include "smpc.h"
#include "debug.h"

Smpc * SmpcRegs;
u8 * SmpcRegsT;

void SmpcNew(void) {
	SmpcRegsT = (u8 *) malloc(sizeof(SmpcRegs) + sizeof(u8));
	SmpcRegs = (Smpc *) (SmpcRegsT + 1);

	SmpcReset(SmpcRegs);
}

void SmpcDelete(void) {
	free(SmpcRegsT);
}

void SmpcReset(void) {
}

FASTCALL u8 SmpcReadByte(u32 addr) {
	return SmpcRegsT[addr];
}

FASTCALL u16 SmpcReadWord(u32 addr) {
	LOG("Trying to word-read a Smpc register\n");
	return 0;
}

FASTCALL u32 SmpcReadLong(u32 addr) {
	LOG("Trying to long-read a Smpc register\n");
	return 0;
}

FASTCALL void SmpcWriteByte(u32 addr, u8 val) {
	SmpcRegsT[addr] = val;
}

FASTCALL void SmpcWriteWord(u32 addr, u16 val) {
	LOG("Trying to word-write a Smpc register\n");
}

FASTCALL void SmpcWriteLong(u32 addr, u32 val) {
	LOG("Trying to long-write a Smpc register\n");
}
