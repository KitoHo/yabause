#include <stdlib.h>
#include "vdp1.h"
#include "debug.h"

u8 * Vdp1Ram;

u8 FASTCALL Vdp1RamReadByte(u32 addr) {
	return T1ReadByte(Vdp1Ram, addr);
}

u16 FASTCALL Vdp1RamReadWord(u32 addr) {
	return T1ReadWord(Vdp1Ram, addr);
}

u32 FASTCALL Vdp1RamReadLong(u32 addr) {
	return T1ReadLong(Vdp1Ram, addr);
}

void FASTCALL Vdp1RamWriteByte(u32 addr, u8 val) {
	T1WriteByte(Vdp1Ram, addr, val);
}

void FASTCALL Vdp1RamWriteWord(u32 addr, u16 val) {
	T1WriteWord(Vdp1Ram, addr, val);
}

void FASTCALL Vdp1RamWriteLong(u32 addr, u32 val) {
	T1WriteLong(Vdp1Ram, addr, val);
}

Vdp1 * Vdp1Regs;

int Vdp1Init(void) {
	Vdp1Regs = (Vdp1 *) malloc(sizeof(Vdp1));
	if (Vdp1Regs == NULL)
		return -1;

	Vdp1Reset();

	return 0;
}

void Vdp1DeInit(void) {
	free(Vdp1Regs);
}

void Vdp1Reset(void) {
	Vdp1Regs->PTMR = 0;
}

u8 FASTCALL Vdp1ReadByte(u32 addr) {
	LOG("trying to byte-read a Vdp1 register\n");
	return 0;
}

u16 FASTCALL Vdp1ReadWord(u32 addr) {
	switch(addr) {
	case 0x10:
		return Vdp1Regs->EDSR;
	case 0x12:
		return Vdp1Regs->LOPR;
	case 0x14:
		return Vdp1Regs->COPR;
	case 0x16:
		return Vdp1Regs->MODR;
	default:
		LOG("trying to read a Vdp1 write-only register\n");
	}
	return 0;
}

u32 FASTCALL Vdp1ReadLong(u32 addr) {
	LOG("trying to long-read a Vdp1 register\n");
	return 0;
}

void FASTCALL Vdp1WriteByte(u32 addr, u8 val) {
	LOG("trying to byte-write a Vdp1 register\n");
}

void FASTCALL Vdp1WriteWord(u32 addr, u16 val) {
	switch(addr) {
	case 0x0:
		Vdp1Regs->TVHR = val;
		break;
	case 0x2:
		Vdp1Regs->FBCR = val;
		break;
	case 0x4:
		Vdp1Regs->PTMR = val;
		break;
	case 0x6:
		Vdp1Regs->EWDR = val;
		break;
	case 0x8:
		Vdp1Regs->EWLR = val;
		break;
	case 0xA:
		Vdp1Regs->EWRR = val;
		break;
	case 0xC:
		Vdp1Regs->ENDR = val;
		break;
	default:
		LOG("trying to write a Vdp1 read-only register\n");
	}
}

void FASTCALL Vdp1WriteLong(u32 addr, u32 val) {
	LOG("trying to long-write a Vdp1 register\n");
}
