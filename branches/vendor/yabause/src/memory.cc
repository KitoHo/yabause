/*  Copyright 2003 Guillaume Duhamel

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "registres.hh"
#include "superh.hh"
#include "timer.hh"

Memory::Memory(unsigned long size) {
  this->size = size;
  if (size == 0) return;
  base_mem = new unsigned char[size];
  memset(base_mem, 0, size);
#ifdef WORDS_BIGENDIAN
  memory = base_mem;
#else
  memory = base_mem + size;
#endif
}

Memory::~Memory(void) {
	if (size == 0) return;
	delete [] base_mem;
}

unsigned char Memory::getByte(unsigned long addr) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	return (memory + addr)[0];
#else
	return (memory - addr - 1)[0];
#endif
}

void Memory::setByte(unsigned long addr, unsigned char val) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	(memory + addr)[0] = val;
#else
	(memory - addr - 1)[0] = val;
#endif
}

unsigned short Memory::getWord(unsigned long addr) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
  return ((unsigned short *) (memory + addr))[0];
#else
  return ((unsigned short *) (memory - addr - 2))[0];
#endif
}

void Memory::setWord(unsigned long addr, unsigned short val) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	((unsigned short *) (memory + addr))[0] = val;
#else
	((unsigned short *) (memory - addr - 2))[0] = val;
#endif
}

unsigned long Memory::getLong(unsigned long addr) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
	return ((unsigned long *) (memory + addr))[0];
#else
	return ((unsigned long *) (memory - addr - 4))[0];
#endif
}

void Memory::setLong(unsigned long addr, unsigned long val) {
	if (addr >= size) {
#ifndef _arch_dreamcast
		throw BadMemoryAccess(addr);
#else
		printf("Bad memory access: %8x", addr);
#endif
	}
#ifdef WORDS_BIGENDIAN
  ((unsigned long *) (memory + addr))[0] = val;
#else
  ((unsigned long *) (memory - addr - 4))[0] = val;
#endif
}

unsigned long Memory::getSize(void) const {
  return size;
}

void Memory::load(const char *filename, unsigned long addr) {
#ifndef _arch_dreamcast
	struct stat infos;
	unsigned long i;
	char buffer;

	if (stat(filename,&infos) == -1)
		throw FileNotFound(filename);

	if (((unsigned int) infos.st_size) > size) {
		cerr << filename << " : File too big" << endl;
		throw BadMemoryAccess(addr);
	}

	ifstream exe(filename, ios::in | ios::binary);
	for(i = addr;i < (infos.st_size + addr);i++) {
		exe.read(&buffer, 1);
		setByte(i, buffer);
	}
	exe.close();
#else
	file_t file = fs_open(filename, O_RDONLY);
	if(!file) {
		printf("File Not Found: %s\n", filename);
		return;
	}
	if(fs_total(file) > size)     {
		printf("File Too Large...\n");
		return;
	}
	
	unsigned long i;
	char buffer;
	for(i = addr;i < fs_total(file) + addr;i++) {
		fs_read(file, &buffer, 1);
		setByte(i, buffer);
	}
	fs_close(file);
#endif
}

#ifndef _arch_dreamcast
ostream& operator<<(ostream& os, const Memory& mem) {
  for(unsigned long i = 0;i < mem.size;i++) {
    os << hex << setw(6) << i << " -> " << setw(10) << mem.memory[i] << endl;
  }
  return os;
}
#endif

LoggedMemory::LoggedMemory(const char *n, Memory *mem, bool d) : Memory(0) {
  strcpy(nom, n);
  this->mem = mem;
  destroy = d;
  size = mem->getSize();
}

LoggedMemory::~LoggedMemory(void) {
  if(destroy) delete mem;
}

Memory *LoggedMemory::getMemory(void) {
  return mem;
}

unsigned char LoggedMemory::getByte(unsigned long addr) {
  unsigned char tmp = mem->getByte(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << (int) tmp << '\n';
#endif
  return tmp;
}

void LoggedMemory::setByte(unsigned long addr, unsigned char val) {
  mem->setByte(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << (int) val << '\n';
#endif
}

unsigned short LoggedMemory::getWord(unsigned long addr) {
  unsigned short ret = mem->getWord(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setWord(unsigned long addr, unsigned short val) {
  mem->setWord(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << val << '\n';
#endif
}

unsigned long LoggedMemory::getLong(unsigned long addr) {
  unsigned long ret = mem->getLong(addr);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " -> " << setw(10) << ret << '\n';
#endif
  return ret;
}

void LoggedMemory::setLong(unsigned long addr, unsigned long val) {
  mem->setLong(addr, val);
#if DEBUG
  cerr << hex << nom << "\t: " << setw(10) << addr
	      << " <- " << setw(10) << val << '\n';
#endif
}

/*********************/
/*                   */
/* Saturn Memory     */
/*                   */
/*********************/


SaturnMemory::SaturnMemory(const char *bios, const char *exe) : Memory(0) {
  msh = new SuperH();

  Timer::initSuperH(msh);

  rom         = new Memory(0x80000);
  onchip      = new OnchipRegisters(msh);

  Intc * intc = ((OnchipRegisters *) onchip)->getIntc();
  
  ram         = new Memory(0x10000);
  ramLow      = new Memory(0x100000);
  cs0	      = new LoggedMemory("cs0", new Dummy(), 1);
  cs1         = new LoggedMemory("cs1", new Cs1(), 1);
  cs2         = new Cs2();
  sound       = new ScspRam(); //Memory(0x7FFFF);
  soundr      = new LoggedMemory("soundr", new Memory(0xEE4), 1);
  scu         = new ScuRegisters(intc);

  Scu * scup = ((ScuRegisters *) scu)->getScu();
  
  vdp1_1      = new Memory(0xC0000);
  vdp1_2      = new Vdp1Registers(vdp1_1, scup);
  vdp2_1      = new Vdp2Ram();
  vdp2_2      = new Vdp2ColorRam();
  vdp2_3      = new LoggedMemory("vdp2", new Vdp2Registers((Vdp2Ram *) vdp2_1,
			  	    (Vdp2ColorRam *) vdp2_2, scup,
				    ((Vdp1Registers *)vdp1_2)->getVdp1()), 1);
  smpc        = new SmpcRegisters(scup, this);
  ramHigh     = new Memory(0x100000);
  purgeArea   = new Dummy();
  adressArray = new Memory(0x3FF);
  modeSdram   = new Memory(0x4FFF);

  if (bios != NULL) rom->load(bios, 0);
  if (exe != NULL) {
    ramHigh->load(exe, 0x4000);
    rom->setLong(0, 0x06004000);
    rom->setLong(4, 0x06002000);
  }

  msh->setMemory(this);

  mshThread = SDL_CreateThread((int (*)(void*)) &SuperH::lancer, msh);
}

SaturnMemory::~SaturnMemory(void) {
#if DEBUG
  cerr << "stopping master sh2\n";
#endif
  msh->stop();
  SDL_WaitThread(mshThread, NULL);
#if DEBUG
  cerr << "master sh2 stopped\n";
#endif

  delete rom;
  delete smpc;
  delete ram;
  delete ramLow;
  delete cs0;
  delete cs1;
  delete cs2;
  delete sound;
  delete soundr;
  delete vdp2_3;
  delete vdp2_1;
  delete vdp2_2;
  delete vdp1_2;
  delete vdp1_1;
  delete scu;
  delete ramHigh;
  delete purgeArea;
  delete adressArray;
  delete modeSdram;
  delete onchip;

  delete msh;
}

void SaturnMemory::mappage(unsigned long adr) {
  switch (adr >> 29) {
  case 0:
  case 1: {
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x80000) { mapMem = rom; mapAdr = nadr; return; }
    if ((nadr >=  0x100000) && (nadr <  0x100080)) { mapMem = smpc;    mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >=  0x180000) && (nadr <  0x190000)) { mapMem = ram;     mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >=  0x200000) && (nadr <  0x300000)) { mapMem = ramLow;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x2000000) && (nadr < 0x4000000)) { mapMem = cs0;     mapAdr = nadr & 0x00FFFFFF; return;}
    if ((nadr >= 0x4000000) && (nadr < 0x5000000)) { mapMem = cs1;     mapAdr = nadr & 0x00FFFFFF; return;}
    if ((nadr >= 0x5800000) && (nadr < 0x5900000)) { mapMem = cs2;     mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5A00000) && (nadr < 0x5A80000)) { mapMem = sound;   mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >= 0x5B00000) && (nadr < 0x5B00EE4)) { mapMem = soundr;  mapAdr = nadr & 0x00000FFF; return;}
    if ((nadr >= 0x5C00000) && (nadr < 0x5CC0000)) { mapMem = vdp1_1;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5D00000) && (nadr < 0x5D00018)) { mapMem = vdp1_2;  mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >= 0x5E00000) && (nadr < 0x5E80000)) { mapMem = vdp2_1;  mapAdr = nadr & 0x000FFFFF; return;}
    if ((nadr >= 0x5F00000) && (nadr < 0x5F01000)) { mapMem = vdp2_2;  mapAdr = nadr & 0x0000FFFF; return;}
    if ((nadr >= 0x5F80000) && (nadr < 0x5F80120)) { mapMem = vdp2_3;  mapAdr = nadr & 0x00000FFF; return;}
    if ((nadr >= 0x5FE0000) && (nadr < 0x5FE00D0)) { mapMem = scu;     mapAdr = nadr & 0x000000FF; return;}
    if ((nadr >= 0x6000000) && (nadr < 0x6100000)) { mapMem = ramHigh; mapAdr = nadr & 0x000FFFFF; return;}
    }
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  case 2:
    mapMem = purgeArea; mapAdr = adr; return;
  case 3: {
    unsigned long nadr = adr & 0x1FFFFFFF;
    if (nadr < 0x3FF) { mapMem = adressArray; mapAdr = nadr; return;}
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
  case 7:
    if ((adr >= 0xFFFF8000) && (adr < 0xFFFFC000)) {mapMem = modeSdram; mapAdr = adr & 0x00000FFF; return;}
    if (adr >= 0xFFFFFE00) { mapMem = onchip; mapAdr = adr & 0x000001FF; return;}
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  default:
#ifndef _arch_dreamcast
    throw BadMemoryAccess(adr);
#else
    printf("Bad memory access: %8x", adr);
#endif
  }
}

unsigned char SaturnMemory::getByte(unsigned long adr) {
  mappage(adr);
  return mapMem->getByte(mapAdr);
}

void SaturnMemory::setByte(unsigned long adr, unsigned char valeur) {
  mappage(adr);
  mapMem->setByte(mapAdr, valeur);
}

unsigned short SaturnMemory::getWord(unsigned long addr) {
  mappage(addr);
  return mapMem->getWord(mapAdr);
}

void SaturnMemory::setWord(unsigned long adr, unsigned short valeur) {
  mappage(adr);
  mapMem->setWord(mapAdr, valeur);
}

unsigned long SaturnMemory::getLong(unsigned long adr) {
  mappage(adr);
  return mapMem->getLong(mapAdr);
}

void SaturnMemory::setLong(unsigned long adr, unsigned long valeur) {
  mappage(adr);
  mapMem->setLong(mapAdr, valeur);
}

void SaturnMemory::loadBios(const char *fichier) {
  rom->load(fichier, 0);
}

void SaturnMemory::load(const char *fichier, unsigned long adr) {
  mappage(adr);
  mapMem->load(fichier, mapAdr);
}

SuperH *SaturnMemory::getMasterSH(void) {
  return msh;
}