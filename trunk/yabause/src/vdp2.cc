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

#include "vdp2.hh"
#include "saturn.hh"
#include "scu.hh"
#include "timer.hh"
#include "yui.hh"

/****************************************/
/*					*/
/*		VDP2 Registers		*/
/*					*/
/****************************************/

void Vdp2::setWord(unsigned long addr, unsigned short val) {
  switch(addr) {
    case 0:
      Memory::setWord(addr, val);
      break;
    case 0xE:
      Memory::setWord(addr, val);
      updateRam();
      break;
    case 0xE0:
      Memory::setWord(addr, val);
      cerr << "sprite type modified" << endl;
      break;
    case 0xF8:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    case 0xFA:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    case 0xFC:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    default:
      Memory::setWord(addr, val);
      break;
  }
}

/****************************************/
/*					*/
/*		VDP2 Color RAM		*/
/*					*/
/****************************************/

void Vdp2ColorRam::setMode(int v) {
	mode = v;
}

unsigned long Vdp2ColorRam::getColor(unsigned long addr, int alpha, int colorOffset) {
  switch(mode) {
  case 0: {
    addr *= 2; // thanks Runik!
    addr += colorOffset * 0x200;
    unsigned long tmp = getWord(addr);
    return SAT2YAB1(alpha, tmp);
  }
  case 1: {
    addr *= 2; // thanks Runik!
    addr += colorOffset * 0x400;
    unsigned long tmp = getWord(addr);
    return SAT2YAB1(alpha, tmp);
  }
  case 2: {
    addr *= 2; // thanks Runik!
    addr += colorOffset * 0x200;
    unsigned long tmp1 = getWord(addr);
    unsigned long tmp2 = getWord(addr + 2);
    return SAT2YAB2(alpha, tmp1, tmp2);
  }
  }
}

/****************************************/
/*					*/
/*		VDP2 Screen		*/
/*					*/
/****************************************/

Vdp2Screen::Vdp2Screen(Vdp2 *r, Vdp2Ram *v, Vdp2ColorRam *c, unsigned long *s) {
    reg = r;
    vram = v;
    cram = c;
    surface = s;
}

int Vdp2Screen::comparePriority(const void *arg1, const void *arg2) {
  Vdp2Screen **screen1 = (Vdp2Screen **) arg1;
  Vdp2Screen **screen2 = (Vdp2Screen **) arg2;
  int s1 = (*screen1)->getPriority();
  int is1 = (*screen1)->getInnerPriority();
  int s2 = (*screen2)->getPriority();
  int is2 = (*screen2)->getInnerPriority();

  if (s1 == s2) return is1 - is2;
  else return s1 - s2;
}

void Vdp2Screen::draw(void) {
	init();
	if (!enable || (getPriority() == 0)) return;

	if (bitmap) {
		drawCell();
	}
	else {
		drawMap();
	}
        
	if (*texture ==0) glGenTextures(1, texture );
	glBindTexture(GL_TEXTURE_2D, texture[0] );
#ifndef _arch_dreamcast
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface);
  
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB4444, 512, 256, 0, GL_ARGB4444, GL_UNSIGNED_BYTE, surface);
#endif

	int p = getPriority();
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex3f(-1, 1, p);
	glTexCoord2f(0.625, 0); glVertex3f(1, 1, p);
	glTexCoord2f(0.625, 0.875); glVertex3f(1, -1, p);
	glTexCoord2f(0, 0.875); glVertex3f(-1, -1, p);
	glEnd();
	glDisable( GL_TEXTURE_2D );
}

void Vdp2Screen::drawMap(void) {
	int X, Y;
	X = x;
	//for(int i = 0;i < mapWH;i++) {
		Y = y;
		x = X;
		//for(int j = 0;j < mapWH;j++) {
			y = Y;
			planeAddr(0); //planeAddr(mapWH * i + j);
			drawPlane();
		//}
	//}
}

void Vdp2Screen::drawPlane(void) {
	int X, Y;

	X = x;
	for(int i = 0;i < planeH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < planeW;j++) {
			y = Y;
			drawPage();
		}
	}
}

void Vdp2Screen::drawPage(void) {
	int X, Y;

	X = x;
	for(int i = 0;i < pageWH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < pageWH;j++) {
			y = Y;
			patternAddr();
			drawPattern();
		}
	}
}

void Vdp2Screen::patternAddr(void) {
	switch(patternDataSize) {
	case 1: {
		unsigned short tmp = vram->getWord(addr);
		addr += 2;
		specialFunction = supplementData & 0x300 >> 8;
    		switch(colorNumber) {
      		case 0: // in 16 colors
			palAddr = ((tmp & 0xF000) >> 12) | ((supplementData & 0xE0) >> 1);
        		break;
      		default: // not in 16 colors
			palAddr = (tmp & 0x7000) >> 8;
			break;
    		}
    		switch(auxMode) {
    		case 0:
      			flipFunction = (tmp & 0xC00) >> 10;
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
      			case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
      			}
      			break;
    		case 1:
      			flipFunction = 0;
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
      			case 4:
				charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
				break;
      			}
      			break;
    		}
    		break;
	}
  	case 2: {
    		unsigned short tmp1 = vram->getWord(addr);
    		addr += 2;
    		unsigned short tmp2 = vram->getWord(addr);
    		addr += 2;
    		charAddr = tmp2 & 0x7FFF;
    		flipFunction = (tmp1 & 0xC000) >> 14;
                palAddr = (tmp1 & 0x7F);
    		specialFunction = (tmp1 & 0x3000) >> 12;
    		break;
	}
	}
	if (!(reg->getWord(0x6) & 0x8000)) charAddr &= 0x3FFF;
	charAddr *= 0x20; // selon Runik
}

void Vdp2Screen::drawPattern(void) {
	int X, Y;
	int xEnd, yEnd;

	if(flipFunction & 0x1) { // vertical flip
		x += patternWH * 8 - 1;
    		xEnd = patternWH * 8 + 1;
  	}
 	else {
    		xEnd = 0;
  	}

  	if(flipFunction & 0x2) { // horizontal flip
    		y += patternWH * 8 - 1;
    		yEnd = patternWH * 8 + 1;
  	}
  	else {
    		yEnd = 0;
  	}

	X = x;
	for(int i = 0;i < patternWH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < patternWH;j++) {
			y = Y;
			drawCell();
		}
	}
	x += xEnd;
	y += yEnd;
}

void Vdp2Screen::drawCell(void) {
  unsigned long color;
  int X;
  int xInc, yInc;

  if(flipFunction & 0x1) { // vertical flip
    xInc = -1;
  }
  else {
    xInc = 1;
  }

  if(flipFunction & 0x2) { // horizontal flip
    yInc = -1;
  }
  else {
    yInc = 1;
  }

  switch(colorNumber) {
    case 0:
      // 4-bit Mode(16 colors)
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j+=4) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if (!(dot & 0xF000) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | ((dot & 0xF000) >> 12), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF00) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | ((dot & 0xF00) >> 8), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF0) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | ((dot & 0xF0) >> 4), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | (dot & 0xF), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 1:
      // 8-bit Mode(256 colors)
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j+=2) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if (!(dot & 0xFF00) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | ((dot & 0xFF00) >> 8), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xFF) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor((palAddr << 4) | (dot & 0xFF), alpha, colorOffset);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 2:
      // 16-bit Mode(2048 colors)
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot = vram->getWord(charAddr);
	  if ((dot == 0) && transparencyEnable) color = 0x00000000;
          else color = cram->getColor(dot, alpha, colorOffset);
	  charAddr += 2;
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 3:
      // 16-bit Mode(32,786 colors)
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if ((dot & 0x8000) && transparencyEnable) color = 0x00000000;
	  else color = SAT2YAB1(0xFF, dot);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 4:
      // 32-bit Mode(16,770,000 colors)
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot1 = vram->getWord(charAddr);
	  charAddr += 2;
	  unsigned short dot2 = vram->getWord(charAddr);
	  charAddr += 2;
	  if ((dot1 & 0x8000) && transparencyEnable) color = 0x00000000;
	  else color = SAT2YAB2(alpha, dot1, dot2);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
  }
}

void Vdp2Screen::drawPixel(unsigned long *surface, Sint16 x, Sint16 y, Uint32 tmpcolor) {
    if ((x >= 0) && (y >= 0) && (x < 512) && (y < 256)) surface[y * 512 + x] = tmpcolor;
}

void RBG0::init(void) {
	enable = false;
}

void RBG0::planeAddr(int i) {
	addr = 0;
}

int RBG0::getPriority(void) {
	return (reg->getWord(0xFC) & 0x7);
}

int RBG0::getInnerPriority(void) {
	return 4;
}

void NBG0::init(void) {
	unsigned short patternNameReg = reg->getWord(0x30);
	unsigned short patternReg = reg->getWord(0x28);
	/* FIXME should start by checking if it's a normal
	 * or rotate scroll screen
	*/
	enable = reg->getWord(0x20) & 0x1;
	transparencyEnable = !(reg->getWord(0x20) & 0x100);
	x = - reg->getWord(0x70);
	y = - reg->getWord(0x74);

  	colorNumber = (patternReg & 0x70) >> 4;
	if(bitmap = patternReg & 0x2) {
		switch((patternReg & 0xC) >> 2) {
			case 0: cellW = 512;
				cellH = 256;
				break;
			case 1: cellW = 512;
				cellH = 512;
				break;
			case 2: cellW = 1024;
				cellH = 256;
				break;
			case 3: cellW = 1024;
				cellH = 512;
				break;
		}
		charAddr = (reg->getWord(0x3C) & 0x7) * 0x20000;
		palAddr = (reg->getWord(0x2C) & 0x7) << 4;
		flipFunction = 0;
		specialFunction = 0;
  	}
  	else {
		mapWH = 2;
  		switch(reg->getWord(0x3A) & 0x3) {
  			case 0: planeW = planeH = 1; break;
  			case 1: planeW = 2; planeH = 1; break;
  			case 2: planeW = planeH = 2; break;
  		}
  		if(patternNameReg & 0x8000) patternDataSize = 1;
  		else patternDataSize = 2;
  		if(patternReg & 0x1) patternWH = 2;
  		else patternWH = 1;
  		pageWH = 64/patternWH;
  		cellW = cellH = 8;
  		supplementData = patternNameReg & 0x3FF;
  		auxMode = (patternNameReg & 0x4000) >> 14;
  	}
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x1) {
		alpha = ((~reg->getWord(0x108) & 0x1F) << 3) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = reg->getWord(0xE4) & 0x7;
}

void NBG0::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x7) << 6;
	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x41); break;
    		case 1: tmp = offset | reg->getByte(0x40); break;
    		case 2: tmp = offset | reg->getByte(0x43); break;
   		case 3: tmp = offset | reg->getByte(0x42); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;
	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG0::getPriority(void) {
  return reg->getByte(0xF9) & 0x7;
}

int NBG0::getInnerPriority(void) {
  return 3;
}

void NBG1::init(void) {
  	unsigned short patternNameReg = reg->getWord(0x32);
  	unsigned short patternReg = reg->getWord(0x28);

  	enable = reg->getWord(0x20) & 0x2;
	transparencyEnable = !(reg->getWord(0x20) & 0x200);
	x = - reg->getWord(0x80);
	y = - reg->getWord(0x84);
	
  	colorNumber = (patternReg & 0x3000) >> 12;
  	if(bitmap = patternReg & 0x200) {
		switch((patternReg & 0xC00) >> 10) {
			case 0: cellW = 512;
				cellH = 256;
				break;
			case 1: cellW = 512;
				cellH = 512;
				break;
			case 2: cellW = 1024;
				cellH = 256;
				break;
			case 3: cellW = 1024;
				cellH = 512;
				break;
		}
		charAddr = ((reg->getWord(0x3C) & 0x70) >> 4) * 0x20000;
		palAddr = (reg->getWord(0x2C) & 0x700) >> 4;
		flipFunction = 0;
		specialFunction = 0;
  	}
  	else {
  		mapWH = 2;
  		switch((reg->getWord(0x3A) & 0xC) >> 2) {
  			case 0: planeW = planeH = 1; break;
  			case 1: planeW = 2; planeH = 1; break;
  			case 2: planeW = planeH = 2; break;
  		}
  		if(patternNameReg & 0x8000) patternDataSize = 1;
  		else patternDataSize = 2;
  		if(patternReg & 0x100) patternWH = 2;
  		else patternWH = 1;
  		pageWH = 64/patternWH;
  		cellW = cellH = 8;
  		supplementData = patternNameReg & 0x3FF;
  		auxMode = (patternNameReg & 0x4000) >> 14;
	}

	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x2) {
		alpha = ((~reg->getWord(0x108) & 0x1F00) >> 5) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x70) >> 4;
}

void NBG1::planeAddr(int i) {
  	unsigned long offset = (reg->getWord(0x3C) & 0x70) << 2;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x45); break;
    		case 1: tmp = offset | reg->getByte(0x44); break;
    		case 2: tmp = offset | reg->getByte(0x47); break;
    		case 3: tmp = offset | reg->getByte(0x46); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;
	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG1::getPriority(void) {
  return reg->getByte(0xF8) & 0x7;
}

int NBG1::getInnerPriority(void) {
  return 2;
}

void NBG2::init(void) {
	unsigned short patternNameReg = reg->getWord(0x34);
	unsigned short patternReg = reg->getWord(0x2A);

	enable = reg->getWord(0x20) & 0x4;
	transparencyEnable = !(reg->getWord(0x20) & 0x400);
	x = - reg->getWord(0x90);
	y = - reg->getWord(0x92);

  	colorNumber = (patternReg & 0x2) >> 1;
	bitmap = false; // NBG2 can only use cell mode
	
	mapWH = 2;
  	switch((reg->getWord(0x3A) & 0x30) >> 4) {
  		case 0: planeW = planeH = 1; break;
  		case 1: planeW = 2; planeH = 1; break;
  		case 2: planeW = planeH = 2; break;
  	}
  	if(patternNameReg & 0x8000) patternDataSize = 1;
  	else patternDataSize = 2;
  	if(patternReg & 0x1) patternWH = 2;
  	else patternWH = 1;
  	pageWH = 64/patternWH;
  	cellW = cellH = 8;
  	supplementData = patternNameReg & 0x3FF;
  	auxMode = (patternNameReg & 0x4000) >> 14;
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x4) {
		alpha = ((~reg->getWord(0x10A) & 0x1F) << 3) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x700) >> 8;
}

void NBG2::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x700) >> 2;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x49); break;
    		case 1: tmp = offset | reg->getByte(0x48); break;
    		case 2: tmp = offset | reg->getByte(0x4B); break;
    		case 3: tmp = offset | reg->getByte(0x4A); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;

	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) {
				cerr << "ici 1" << endl;
				addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
			}
	  		else {
				cerr << "ici 2" << endl;
				addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
			}
  		}
  		else {
	  		if (patternWH == 1) {
				cerr << "ici 3" << endl;
				addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
			}
	  		else {
				cerr << "ici 4" << endl;
				addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
			}
  		}
	}*/
}

int NBG2::getPriority(void) {
  return reg->getByte(0xFB) & 0x7;
}

int NBG2::getInnerPriority(void) {
  return 1;
}

void NBG3::init(void) {
	unsigned short patternNameReg = reg->getWord(0x36);
	unsigned short patternReg = reg->getWord(0x2A);

	enable = reg->getWord(0x20) & 0x8;
	transparencyEnable = !(reg->getWord(0x20) & 0x800);
	x = - reg->getWord(0x94);
	y = - reg->getWord(0x96);

  	colorNumber = (patternReg & 0x20) >> 5;
	bitmap = false; // NBG2 can only use cell mode
	
	mapWH = 2;
  	switch((reg->getWord(0x3A) & 0xC0) >> 6) {
  		case 0: planeW = planeH = 1; break;
  		case 1: planeW = 2; planeH = 1; break;
  		case 2: planeW = planeH = 2; break;
  	}
  	if(patternNameReg & 0x8000) patternDataSize = 1;
  	else patternDataSize = 2;
  	if(patternReg & 0x10) patternWH = 2;
  	else patternWH = 1;
  	pageWH = 64/patternWH;
  	cellW = cellH = 8;
  	supplementData = patternNameReg & 0x3FF;
  	auxMode = (patternNameReg & 0x4000) >> 14;
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x8) {
		alpha = ((~reg->getWord(0x10A) & 0x1F00) >> 5) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x7000) >> 12;
}

void NBG3::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x7000) >> 6;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x4D); break;
    		case 1: tmp = offset | reg->getByte(0x4C); break;
    		case 2: tmp = offset | reg->getByte(0x4F); break;
    		case 3: tmp = offset | reg->getByte(0x4E); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;

	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG3::getPriority(void) {
	return (reg->getByte(0xFA) & 0x7);
}

int NBG3::getInnerPriority(void) {
	return 0;
}

/****************************************/
/*					*/
/*		VDP2 			*/
/*					*/
/****************************************/

Vdp2::Vdp2(SaturnMemory *v) : Memory(0xFFF, 0x120) {
  satmem = v;
  _stop = false;
  vram = new Vdp2Ram;
  cram = new Vdp2ColorRam;
  setWord(0x4, 0); //setWord(0x4, 0x302);
  setWord(0x20, 0);

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_SetVideoMode(320,224,32, SDL_OPENGL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glOrtho(-1, 1, -1, 1, -10, 10);
#ifndef _arch_dreamcast
  surface = new unsigned long [512 * 256];
#else
  surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 256, 16, 0x000f, 0x00f0, 0x0f00, 0xf000);
  free(surface->pixels);
  surface->pixels = memalign(32, 256 * 512 * 2);
#endif
  	screens[4] = new RBG0(this, vram, cram, surface);
  	screens[3] = new NBG0(this, vram, cram, surface);
  	screens[2] = new NBG1(this, vram, cram, surface);
  	screens[1] = new NBG2(this, vram, cram, surface);
  	screens[0] = new NBG3(this, vram, cram, surface);
}

Vdp2::~Vdp2(void) {
    for(int i = 0;i < 5;i++) delete screens[i];
    delete [] surface;
    delete vram;
    delete cram;
}

Memory *Vdp2::getCRam(void) {
	return cram;
}

Memory *Vdp2::getVRam(void) {
	return vram;
}

void Vdp2::lancer(Vdp2 *vdp2) {
	while(!vdp2->_stop) vdp2->executer();
}

void Vdp2::executer(void) {
  Timer t;
  for(int i = 0;i < 224;i++) { // FIXME depends on SuperH::synchroStart
    t.waitHBlankIN();
    setWord(0x4, getWord(0x4) | 0x0004);
    t.waitHBlankOUT();
    setWord(0x4, getWord(0x4) & 0xFFFB);
  }
  t.waitVBlankIN();
  setWord(0x4, getWord(0x4) | 0x0008);
  ((Scu *) satmem->getScu())->sendVBlankIN();
  for(int i = 0;i < 39;i++) { // FIXME depends on SuperH::synchroStart
    t.waitHBlankIN();
    setWord(0x4, getWord(0x4) | 0x0004);
    t.waitHBlankOUT();
    setWord(0x4, getWord(0x4) & 0xFFFB);
  }
  t.waitVBlankOUT();
  setWord(0x4, getWord(0x4) & 0xFFF7);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawBackScreen();
  if (getWord(0) & 0x8000) {
    screens[0]->draw();
    screens[1]->draw();
    screens[2]->draw();
    screens[3]->draw();
    screens[4]->draw();
  }
  
#ifdef _arch_dreamcast
  glKosBeginFrame();
#endif

  ((Vdp1 *) satmem->getVdp1())->execute(0);
  glFlush();
#ifndef _arch_dreamcast
  SDL_GL_SwapBuffers();
#else
  glKosFinishFrame();
#endif
  //colorOffset();
  ((Scu *) satmem->getScu())->sendVBlankOUT();
}

Vdp2Screen *Vdp2::getScreen(int i) {
  return screens[i];
}

void Vdp2::sortScreens(void) {
  qsort(screens, 5, sizeof(Vdp2Screen *), &Vdp2Screen::comparePriority);
}

void Vdp2::updateRam(void) {
  cram->setMode(getWord(0xE) & 0xC000);
}

void Vdp2::drawBackScreen(void) {
/*
	unsigned long BKTAU = getWord(0xAC);
	unsigned long BKTAL = getWord(0xAE);
	unsigned long scrAddr;

	if (getWord(0x6) & 0x8000)
		scrAddr = ((BKTAU & 0x7 << 16) | BKTAL) * 2;
	else
		scrAddr = ((BKTAU & 0x3 << 16) | BKTAL) * 2;

	unsigned short dot;
	if (BKTAU & 0x8000) {
		dot = vram->getWord(scrAddr);
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, (dot & 0x1F) << 3, (dot & 0xCE0) >> 2, (dot & 0x7C00) >> 7));
	}
	else {
		SDL_Rect rect;
		rect.x = 0;
		rect.w = 320;
		rect.h = 1;
		for(rect.y = 0;rect.y < 240;rect.y++) {
			dot = vram->getWord(scrAddr);
			scrAddr += 2;
			SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, (dot & 0x1F) << 3, (dot & 0xCE0) >> 2, (dot & 0x7C00) >> 7));
		}
	}
*/
}

void Vdp2::priorityFunction(void) {
}

/*
void Vdp2::colorOffset(void) {
  SDL_Surface *tmp1, *tmp2;

  tmp1 = SDL_CreateRGBSurface(SDL_HWSURFACE, 400, 400, 16, 0, 0, 0, 0);
  tmp2 = SDL_CreateRGBSurface(SDL_HWSURFACE, 400, 400, 16, 0, 0, 0, 0);

  SDL_FillRect(tmp2, NULL, SDL_MapRGB(tmp2->format, getWord(0x114),
			  			    getWord(0x116),
						    getWord(0x118)));
  SDL_BlitSurface(surface, NULL, tmp1, NULL);

  SDL_imageFilterAdd((unsigned char *) tmp1->pixels,
		     (unsigned char *) tmp2->pixels,
		     (unsigned char *) surface->pixels, surface->pitch);

  SDL_FreeSurface(tmp1);
  SDL_FreeSurface(tmp2);
}
*/
