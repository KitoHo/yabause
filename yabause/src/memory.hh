/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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

#ifndef MEMORY_HH
#define MEMORY_HH

#include "exception.hh"

class Memory {
private:
  unsigned char *base_mem;
protected:
  unsigned long size;
public:
  unsigned char *memory;
  unsigned long mask;
  Memory(unsigned long, unsigned long);
  virtual ~Memory(void);

  virtual unsigned char  getByte (unsigned long);
  virtual void           setByte (unsigned long, unsigned char);
  virtual unsigned short getWord (unsigned long);
  virtual void           setWord (unsigned long, unsigned short);
  virtual unsigned long  getLong (unsigned long);
  virtual void           setLong (unsigned long, unsigned long);

  unsigned long getSize() const;
  unsigned char *getBuffer(void) const;
  virtual void load(const char *, unsigned long);
  virtual void save(const char *, unsigned long, unsigned long);

#ifndef _arch_dreamcast
  friend ostream& operator<<(ostream&, const Memory&);
#endif
};

//unsigned short __attribute__((regparm(2))) readWord(Memory *, unsigned long);
unsigned short inline readWord(Memory * mem, unsigned long addr) {
  addr &= mem->mask;
#ifdef WORDS_BIGENDIAN
  return ((unsigned short *) (mem->memory + addr))[0];
#else
  return ((unsigned short *) (mem->memory - addr - 2))[0];
#endif
}

class LoggedMemory : public Memory {
protected:
  Memory *mem;
  char nom[11];
  bool destroy;
public:
  LoggedMemory(const char *, Memory *, bool);
  virtual ~LoggedMemory(void);
  Memory *getMemory(void);
  unsigned char  getByte (unsigned long);
  void           setByte (unsigned long, unsigned char);
  unsigned short getWord (unsigned long);
  void           setWord (unsigned long, unsigned short);
  unsigned long  getLong (unsigned long);
  void           setLong (unsigned long, unsigned long);
};

class Dummy : public Memory {
public:
  Dummy(unsigned long m) : Memory(m, 0) {}
  ~Dummy(void) {}
  unsigned char getByte(unsigned long) { return 0; }
  void setByte(unsigned long, unsigned char) {}
  unsigned short getWord(unsigned long) { return 0; }
  void setWord(unsigned long, unsigned short) {}
  unsigned long getLong(unsigned long) { return 0; }
  void setLong(unsigned long, unsigned long) {}
};

#endif
