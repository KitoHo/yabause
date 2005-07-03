#include "scu.h"
#include "debug.h"

Scu * ScuRegs;

void ScuNew(void) {
	ScuRegs = (Scu *) malloc(sizeof(scu));

	ScuReset();
}

void ScuDelete(void) {
	free(ScuRegs);
}

void ScuReset(void) {
	ScuRegs->D0AD = ScuRegs->D1AD = ScuRegs->D2AD = 0x101;
	ScuRegs->D0EN = ScuRegs->D1EN = ScuRegs->D2EN = 0x0;
	ScuRegs->D0MD = ScuRegs->D1MD = ScuRegs->D2MD = 0x7;
	ScuRegs->DSTP = 0x0;
	ScuRegs->DSTA = 0x0;

	ScuRegs->PPAF = 0x0;
	ScuRegs->PDA = 0x0;

	ScuRegs->T1MD = 0x0;

	ScuRegs->IMS = 0xBFFF;
	ScuRegs->IST = 0x0;

	ScuRegs->AIACK = 0x0;
	ScuRegs->ASR0 = ScuRegs->ASR1 = 0x0;
	ScuRegs->AREF = 0x0;

	ScuRegs->RSEL = 0x0;
	ScuRegs->VER = 0x0;
}

FASTCALL u8 ScuReadByte(u32 addr) {
	LOG("trying to byte-read a Scu register\n");
	return 0;
}

FASTCALL u16 ScuReadWord(u32 addr) {
	LOG("trying to word-read a Scu register\n");
	return 0;
}

FASTCALL u32 ScuReadLong(u32 addr) {
	switch(addr) {
	case 0:
		return ScuRegs->D0R;
	case 4:
		return ScuRegs->W0R;
	case 8:
		return ScuRegs->D0C;
	case 0x20:
		return ScuRegs->D1R;
	case 0x24:
		return ScuRegs->W1R;
	case 0x28:
		return ScuRegs->D1C;
	case 0x40:
		return ScuRegs->D2R;
	case 0x44:
		return ScuRegs->W2R;
	case 0x48:
		return ScuRegs->D2C;
	case 0x7C:
		return ScuRegs->DSTA;
	case 0x80:
		return ScuRegs->PPAF;
	case 0x8C:
		return ScuRegs->PDD;
	case 0xA4:
		return ScuRegs->IST;
	case 0xA8:
		return ScuRegs->AIACK;
	case 0xC4:
		return ScuRegs->RSEL;
	case 0xC8:
		return ScuRegs->VER;
	default:
		LOG("bad Scu register read\n");
		return 0;
	}
}

FASTCALL void ScuWriteByte(u32 addr, u8 val) {
	LOG("trying to byte-write a Scu register\n");
}

FASTCALL void ScuWriteWord(u32 addr, u16 val) {
	LOG("trying to word-write a Scu register\n");
}

FASTCALL void ScuWriteLong(u32 addr, u32 val) {
	switch(addr) {
	case 0:
		ScuRegs->D0R = val;
		break;
	case 4:
		ScuRegs->W0R = val;
		break;
	case 8:
		ScuRegs->D0C = val;
		break;
	case 0xC:
		ScuRegs->D0AD = val;
		break;
	case 0x10:
		ScuRegs->D0EN = val;
		break;
	case 0x14:
		ScuRegs->D0MD = val;
		break;
	case 0x20:
		ScuRegs->D1R = val;
		break;
	case 0x24:
		ScuRegs->W1R = val;
		break;
	case 0x28:
		ScuRegs->D1C = val;
		break;
	case 0x2C:
		ScuRegs->D1AD = val;
		break;
	case 0x30:
		ScuRegs->D1EN = val;
		break;
	case 0x34:
		ScuRegs->D1MD = val;
		break;
	case 0x40:
		ScuRegs->D2R = val;
		break;
	case 0x44:
		ScuRegs->W2R = val;
		break;
	case 0x48:
		ScuRegs->D2C = val;
		break;
	case 0x4C:
		ScuRegs->D2AD = val;
		break;
	case 0x50:
		ScuRegs->D2EN = val;
		break;
	case 0x54:
		ScuRegs->D2MD = val;
		break;
	case 0x60:
		ScuRegs->DSTP = val;
		break;
	case 0x80:
		ScuRegs->PPAF = val;
		break;
	case 0x84:
		ScuRegs->PPD = val;
		break;
	case 0x88:
		ScuRegs->PPA = val;
		break;
	case 0x8C:
		ScuRegs->PDD = val;
		break;
	case 0x90:
		ScuRegs->T0C = val;
		break;
	case 0x94:
		ScuRegs->T1S = val;
		break;
	case 0x98:
		ScuRegs->T1MD = val;
		break;
	case 0xA0:
		ScuRegs->IMS = val;
		break;
	case 0xA4:
		ScuRegs->IST = val;
		break;
	case 0xA8:
		ScuRegs->AIACK = val;
		break;
	case 0xB0:
		ScuRegs->ASR0 = val;
		break;
	case 0xB4:
		ScuRegs->ASR1 = val;
		break;
	case 0xB8:
		ScuRegs->AREF = val;
		break;
	case 0xC4:
		ScuRegs->RSEL = val;
		break;
	default:
		LOG("bad Scu register write\n");
	}
}
