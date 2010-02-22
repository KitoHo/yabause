/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2008 Theo Berkau
    Copyright 2006 Fabien Coulon

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

#ifdef _MSC_VER
#undef mulfixed
//sometimes A LOT of time gets spent in _allmul. this is pointless. here is a way faster way to do it
//this doesnt work as well as it could due to the emitting of code which stuffs
//values into temps and then right back into eax and edx. but it is still way faster
//INLINE fixed32 __fastcall mulfixed(const fixed32 a, const fixed32 b)
//{
//	__asm mov eax, a;
//	__asm mov edx, b;
//	__asm imul edx;
//	__asm shrd eax, edx, 16;
//}
//here is a better way to do it, which might even be roughly portable 
#include <intrin.h>
#define mulfixed(a,b) (fixed32)(__ll_rshift(__emul((a),(b)),16))
#endif

extern "C" {
#include "vidsoft.h"
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"

#ifdef HAVE_LIBGL
#define USE_OPENGL
#endif

#ifdef USE_OPENGL
#include "ygl.h"
#endif

#include "yui.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
//#undef INLINE
//#define INLINE
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
/////////////////------------------
}


class Task
{
public:
	Task();
	~Task();
	
	typedef void * (*TWork)(void *);

	//execute some work
	void execute(const TWork &work, void* param);

	//wait for the work to complete
	void* finish();

	class Impl;
	Impl *impl;

};

#ifdef _WIN32

#include <windows.h>

class Task::Impl {
public:
	Impl();
	~Impl();

	//the only real configuration option: affects the wake-up overhead for the thread
	//by determining whether it spinlocks or waits for a signal
	bool spinlock;

	//execute some work
	void execute(const TWork &work, void* param);

	//wait for the work to complete
	void* finish();

	static DWORD __stdcall s_taskProc(void *ptr);
	void taskProc();
	void init();

	//the work function that shall be executed
	TWork work;
	void* param;

	HANDLE incomingWork, workDone, hThread;
	volatile bool bIncomingWork, bWorkDone, bKill;
};

static void* killTask(void* task)
{
	((Task::Impl*)task)->bKill = true;
	return 0;
}

Task::Impl::~Impl()
{
	execute(killTask,this);
	finish();

	CloseHandle(incomingWork);
	CloseHandle(workDone);
	CloseHandle(hThread);
}

Task::Impl::Impl()
	: work(NULL)
	, bIncomingWork(false)
	, bWorkDone(true)
	, spinlock(false)
	, bKill(false)
{
	incomingWork = CreateEvent(NULL,FALSE,FALSE,NULL);
	workDone = CreateEvent(NULL,FALSE,FALSE,NULL);
	hThread = CreateThread(NULL,0,Task::Impl::s_taskProc,(void*)this, 0, NULL);
}

DWORD __stdcall Task::Impl::s_taskProc(void *ptr)
{
	//just past the buck to the instance method
	((Task::Impl*)ptr)->taskProc();
	return 0;
}

void Task::Impl::taskProc()
{
	for(;;) {
		if(bKill) break;
		
		//wait for a chunk of work
		if(spinlock) while(!bIncomingWork) Sleep(0); 
		else WaitForSingleObject(incomingWork,INFINITE); 
		
		bIncomingWork = false; 
		//execute the work
		param = work(param);
		//signal completion
		if(!spinlock) SetEvent(workDone); 
		bWorkDone = true;
	}
}

void Task::Impl::execute(const TWork &work, void* param) 
{
	//setup the work
	this->work = work;
	this->param = param;
	bWorkDone = false;
	//signal it to start
	if(!spinlock) SetEvent(incomingWork); 
	bIncomingWork = true;
}

void* Task::Impl::finish()
{
	//just wait for the work to be done
	if(spinlock) while(!bWorkDone) Sleep(0);
	else WaitForSingleObject(workDone,INFINITE); 
	return param;
}

Task::Task() : impl(new Task::Impl()) {}
Task::~Task() { delete impl; }
void Task::execute(const TWork &work, void* param) { impl->execute(work,param); }
void* Task::finish() { return impl->finish(); }


#endif

Task layerTasks[8];

//#ifdef _MSC_VER
//#define ENABLE_SSE2
//#endif
//
//#ifdef _MSC_VER
//#include <xmmintrin.h>
//#include <emmintrin.h>
//#endif
//
////these functions are an unreliable, inaccurate floor.
////it should only be used for positive numbers
////this isnt as fast as it could be if we used a visual c++ intrinsic, but those appear not to be universally available
//FORCEINLINE u32 u32floor(float f)
//{
//#ifdef _MSC_VER
//	return (u32)_mm_cvtt_ss2si(_mm_set_ss(f));
//#else
//	return (u32)f;
//#endif
//}
//FORCEINLINE u32 u32floor(double d)
//{
//#ifdef _MSC_VER
//	return (u32)_mm_cvttsd_si32(_mm_set_sd(d));
//#else
//	return (u32)d;
//#endif
//}
//
////same as above but works for negative values too.
////be sure that the results are the same thing as floorf!
//FORCEINLINE s32 s32floor(float f)
//{
//#ifdef _MSC_VER
//	return _mm_cvtss_si32( _mm_add_ss(_mm_set_ss(-0.5f),_mm_add_ss(_mm_set_ss(f), _mm_set_ss(f))) ) >> 1;
//#else
//	return (s32)floorf(f);
//#endif
//}

//fairly standard for loop macros
#define MACRODO1(TRICK,TODO) { const int X = TRICK; TODO; }
#define MACRODO2(X,TODO)   { MACRODO1((X),TODO)   MACRODO1(((X)+1),TODO) }
#define MACRODO4(X,TODO)   { MACRODO2((X),TODO)   MACRODO2(((X)+2),TODO) }
#define MACRODO8(X,TODO)   { MACRODO4((X),TODO)   MACRODO4(((X)+4),TODO) }
#define MACRODO16(X,TODO)  { MACRODO8((X),TODO)   MACRODO8(((X)+8),TODO) }
#define MACRODO32(X,TODO)  { MACRODO16((X),TODO)  MACRODO16(((X)+16),TODO) }
#define MACRODO64(X,TODO)  { MACRODO32((X),TODO)  MACRODO32(((X)+32),TODO) }
#define MACRODO128(X,TODO) { MACRODO64((X),TODO)  MACRODO64(((X)+64),TODO) }
#define MACRODO256(X,TODO) { MACRODO128((X),TODO) MACRODO128(((X)+128),TODO) }

//this one lets you loop any number of times (as long as N<256)
#define MACRODO_N(N,TODO) {\
	if((N)&0x100) MACRODO256(0,TODO); \
	if((N)&0x080) MACRODO128((N)&(0x100),TODO); \
	if((N)&0x040) MACRODO64((N)&(0x100|0x080),TODO); \
	if((N)&0x020) MACRODO32((N)&(0x100|0x080|0x040),TODO); \
	if((N)&0x010) MACRODO16((N)&(0x100|0x080|0x040|0x020),TODO); \
	if((N)&0x008) MACRODO8((N)&(0x100|0x080|0x040|0x020|0x010),TODO); \
	if((N)&0x004) MACRODO4((N)&(0x100|0x080|0x040|0x020|0x010|0x008),TODO); \
	if((N)&0x002) MACRODO2((N)&(0x100|0x080|0x040|0x020|0x010|0x008|0x004),TODO); \
	if((N)&0x001) MACRODO1((N)&(0x100|0x080|0x040|0x020|0x010|0x008|0x004|0x002),TODO); \
}


#if defined WORDS_BIGENDIAN
INLINE u32 COLSAT2YAB16(int priority,u32 temp)            { return (priority | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27); }
INLINE u32 COLSAT2YAB32(int priority,u32 temp)            { return (((temp & 0xFF) << 24) | ((temp & 0xFF00) << 8) | ((temp & 0xFF0000) >> 8) | priority); }
INLINE u32 COLSAT2YAB32_2(int priority,u32 temp1,u32 temp2)   { return (((temp2 & 0xFF) << 24) | ((temp2 & 0xFF00) << 8) | ((temp1 & 0xFF) << 8) | priority); }
INLINE u32 COLSATSTRIPPRIORITY(u32 pixel)              { return (pixel | 0xFF); }
#else
INLINE u32 COLSAT2YAB16(int priority,u32 temp) { return (priority << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9); }
INLINE u32 COLSAT2YAB32(int priority, u32 temp) { return (priority << 24 | (temp & 0xFF0000) | (temp & 0xFF00) | (temp & 0xFF)); }
INLINE u32 COLSAT2YAB32_2(int priority,u32 temp1,u32 temp2)   { return (priority << 24 | ((temp1 & 0xFF) << 16) | (temp2 & 0xFF00) | (temp2 & 0xFF)); }
INLINE u32 COLSATSTRIPPRIORITY(u32 pixel) { return (0xFF000000 | pixel); }
#endif

#define COLOR_ADDt(b)		(b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)	COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)      (COLOR_ADDb(l & 0xFF, r) << 24) | \
                                (COLOR_ADDb((l >> 8) & 0xFF, g) << 16) | \
                                (COLOR_ADDb((l >> 16) & 0xFF, b) << 8) | \
                                ((l >> 24) & 0xFF)
#else
//removing this was speedup from 19->20
#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
                                (COLOR_ADDb((l >> 8) & 0xFF, g) << 8) | \
                                (COLOR_ADDb((l >> 16) & 0xFF, b) << 16) | \
				(l & 0xFF000000)
#endif

static void PushUserClipping(int mode);
static void PopUserClipping(void);

int VIDSoftInit(void);
void VIDSoftDeInit(void);
void VIDSoftResize(unsigned int, unsigned int, int);
int VIDSoftIsFullscreen(void);
int VIDSoftVdp1Reset(void);
void VIDSoftVdp1DrawStart(void);
void VIDSoftVdp1DrawEnd(void);
void VIDSoftVdp1NormalSpriteDraw(void);
void VIDSoftVdp1ScaledSpriteDraw(void);
void VIDSoftVdp1DistortedSpriteDraw(void);
void VIDSoftVdp1PolygonDraw(void);
void VIDSoftVdp1PolylineDraw(void);
void VIDSoftVdp1LineDraw(void);
void VIDSoftVdp1UserClipping(void);
void VIDSoftVdp1SystemClipping(void);
void VIDSoftVdp1LocalCoordinate(void);
int VIDSoftVdp2Reset(void);
void VIDSoftVdp2DrawStart(void);
void VIDSoftVdp2DrawEnd(void);
void VIDSoftVdp2DrawScreens(void);
void VIDSoftVdp2SetResolution(u16 TVMD);
void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority);
void VIDSoftOnScreenDebugMessage(char *string, ...);
void VIDSoftGetGlSize(int *width, int *height);
void VIDSoftVdp1SwapFrameBuffer(void);
void VIDSoftVdp1EraseFrameBuffer(void);

VideoInterface_struct VIDSoft = {
VIDCORE_SOFT,
"Software Video Interface",
VIDSoftInit,
VIDSoftDeInit,
VIDSoftResize,
VIDSoftIsFullscreen,
VIDSoftVdp1Reset,
VIDSoftVdp1DrawStart,
VIDSoftVdp1DrawEnd,
VIDSoftVdp1NormalSpriteDraw,
VIDSoftVdp1ScaledSpriteDraw,
VIDSoftVdp1DistortedSpriteDraw,
//for the actual hardware, polygons are essentially identical to distorted sprites
//the actual hardware draws using diagonal lines, which is why using half-transparent processing
//on distorted sprites and polygons is not recommended since the hardware overdraws to prevent gaps
//thus, with half-transparent processing some pixels will be processed more than once, producing moire patterns in the drawn shapes
VIDSoftVdp1DistortedSpriteDraw,
VIDSoftVdp1PolylineDraw,
VIDSoftVdp1LineDraw,
VIDSoftVdp1UserClipping,
VIDSoftVdp1SystemClipping,
VIDSoftVdp1LocalCoordinate,
VIDSoftVdp2Reset,
VIDSoftVdp2DrawStart,
VIDSoftVdp2DrawEnd,
VIDSoftVdp2DrawScreens,
VIDSoftVdp2SetResolution,
VIDSoftVdp2SetPriorityNBG0,
VIDSoftVdp2SetPriorityNBG1,
VIDSoftVdp2SetPriorityNBG2,
VIDSoftVdp2SetPriorityNBG3,
VIDSoftVdp2SetPriorityRBG0,
VIDSoftOnScreenDebugMessage,
VIDSoftGetGlSize,
};

u32 *dispbuffer=NULL;
u8 *vdp1framebuffer[2]= { NULL, NULL };
u8 *vdp1frontframebuffer;
u8 *vdp1backframebuffer;

u32 *vdp2framebuffer=NULL;

u32 *nbg0framebuffer=NULL;
u32 *nbg1framebuffer=NULL;
u32 *nbg2framebuffer=NULL;
u32 *nbg3framebuffer=NULL;
u32 *rbg0framebuffer=NULL;

static int vdp1width;
static int vdp1height;
static int vdp1clipxstart;
static int vdp1clipxend;
static int vdp1clipystart;
static int vdp1clipyend;
static int vdp1pixelsize = 1;
static int vdp1spritetype;
int vdp2width;
int vdp2height;
static int nbg0priority=0;
static int nbg1priority=0;
static int nbg2priority=0;
static int nbg3priority=0;
static int rbg0priority=0;
#ifdef USE_OPENGL
static int outputwidth;
static int outputheight;
#endif
static int resxratio;
static int resxratio_shift;
static int resyratio;
static int resyratio_shift;

static char message[512];
static int msglength;

typedef struct { s16 x; s16 y; } vdp1vertex;

typedef struct
{
   int pagepixelwh, pagepixelwh_bits, pagepixelwh_mask;
   int planepixelwidth, planepixelwidth_bits, planepixelwidth_mask;
   int planepixelheight, planepixelheight_bits, planepixelheight_mask;
   int screenwidth;
   int screenheight;
   int oldcellx, oldcelly, oldcellcheck;
   int xmask, ymask;
   u32 planetbl[16];
} screeninfo_struct;

//////////////////////////////////////////////////////////////////////////////

static INLINE void vdp2putpixel32(s32 x, s32 y, u32 color, int priority)
{
   vdp2framebuffer[(y * vdp2width) + x] = COLSAT2YAB32(priority, color);
}


//////////////////////////////////////////////////////////////////////////////

static INLINE u8 Vdp2GetPixelPriority(u32 pixel)
{
#if defined WORDS_BIGENDIAN
   return pixel;
#else
   return pixel >> 24;
#endif
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void puthline16(s32 x, s32 y, s32 width, u16 color, int priority)
{
   u32 *buffer = vdp2framebuffer + (y * vdp2width) + x;
   u32 dot=COLSAT2YAB16(priority, color);
   int i;

   for (i = 0; i < width; i++)
      buffer[i] = dot;
}

static INLINE u32 FASTCALL Vdp2ColorRamGetColor(u32 addr);
static INLINE void Vdp2PatternAddr(vdp2draw_struct *info)
{
   switch(info->patterndatasize)
   {
      case 1:
      {
         u16 tmp = T1ReadWord(Vdp2Ram, info->addr);         

         info->addr += 2;
         info->specialfunction = (info->supplementdata >> 9) & 0x1;

         switch(info->colornumber)
         {
            case 0: // in 16 colors
               info->paladdr = ((tmp & 0xF000) >> 8) | ((info->supplementdata & 0xE0) << 3);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 4;
               break;
         }

         switch(info->auxmode)
         {
            case 0:
               info->flipfunction = (tmp & 0xC00) >> 10;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
                     break;
               }
               break;
            case 1:
               info->flipfunction = 0;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
                     break;
               }
               break;
         }

         break;
      }
      case 2: {
         u16 tmp1 = T1ReadWord(Vdp2Ram, info->addr);
         u16 tmp2 = T1ReadWord(Vdp2Ram, info->addr+2);
         info->addr += 4;
         info->charaddr = tmp2 & 0x7FFF;
         info->flipfunction = (tmp1 & 0xC000) >> 14;
         info->paladdr = (tmp1 & 0x7F) << 4;
		 //this should include the special priority and color calc bits
		 info->specialfunction = (tmp1 & 0x3000) >> 12;
         break;
      }
   }

   if (!(Vdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // selon Runik
   //TODO this looks like a hack to me
//   if (info->specialprimode == 1) {
//      info->priority = (info->priority & 0xE) | (info->specialfunction & 1);
//   }
}


//////////////////////////////////////////////////////////////////////////////

static INLINE int TestWindow(int wctl, int enablemask, int inoutmask, clipping_struct *clip, int x, int y)
{
   if (wctl & enablemask) 
   {
      if (wctl & inoutmask)
      {
		  //it looks like madou monogatari intro uses invalid windows to disable layers
		  //or does it? 

         // Draw inside of window
         if (x < clip->xstart || x > clip->xend ||
             y < clip->ystart || y > clip->yend)
            return 0;
      }
      else
      {

         // Draw outside of window
         if (x >= clip->xstart && x <= clip->xend &&
             y >= clip->ystart && y <= clip->yend)
            return 0;

		 //it seems to overflow vertically on hardware
		 if(clip->yend > vdp2height && (x >= clip->xstart && x <= clip->xend ))
			 return 0;
      }
   }
   return 1;
}

//////////////////////////////////////////////////////////////////////////////

INLINE void GeneratePlaneAddrTable(vdp2draw_struct *info, u32 *planetbl)
{
   int i;

   for (i = 0; i < (info->mapwh*info->mapwh); i++)
   {
      info->PlaneAddr(info, i);
      planetbl[i] = info->addr;
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void FASTCALL Vdp2MapCalcXY(vdp2draw_struct *info, int *x, int *y,
                                 screeninfo_struct *sinfo)
{
   int planenum;
   const int pagesize_bits=info->pagewh_bits*2;
   const int cellwh=(2 + info->patternwh);

   const int check = ((y[0] >> cellwh) << 16) | (x[0] >> cellwh);
   //if ((x[0] >> cellwh) != sinfo->oldcellx || (y[0] >> cellwh) != sinfo->oldcelly)
   if(check != sinfo->oldcellcheck)
   {
      sinfo->oldcellx = x[0] >> cellwh;
      sinfo->oldcelly = y[0] >> cellwh;
	  sinfo->oldcellcheck = (sinfo->oldcelly << 16) | sinfo->oldcellx;

      // Calculate which plane we're dealing with
      planenum = ((y[0] >> sinfo->planepixelheight_bits) * info->mapwh) + (x[0] >> sinfo->planepixelwidth_bits);
      x[0] = (x[0] & sinfo->planepixelwidth_mask);
      y[0] = (y[0] & sinfo->planepixelheight_mask);

      // Fetch and decode pattern name data
      info->addr = sinfo->planetbl[planenum];

      // Figure out which page it's on(if plane size is not 1x1)
      info->addr += ((  ((y[0] >> sinfo->pagepixelwh_bits) << pagesize_bits) << info->planew_bits) +
                     (   (x[0] >> sinfo->pagepixelwh_bits) << pagesize_bits) +
                     (((y[0] & sinfo->pagepixelwh_mask) >> cellwh) << info->pagewh_bits) +
                     ((x[0] & sinfo->pagepixelwh_mask) >> cellwh)) << (info->patterndatasize_bits+1);

      Vdp2PatternAddr(info); // Heh, this could be optimized
   }

   // Figure out which pixel in the tile we want
   if (info->patternwh == 1)
   {
      x[0] &= 8-1;
      y[0] &= 8-1;

	  switch(info->flipfunction & 0x3)
	  {
	  case 0: //none
		  break;
	  case 1: //horizontal flip
		  x[0] = 8 - 1 - x[0];
		  break;
	  case 2: // vertical flip
         y[0] = 8 - 1 - y[0];
		 break;
	  case 3: //flip both
         x[0] = 8 - 1 - x[0];
		 y[0] = 8 - 1 - y[0];
		 break;
	  }
   }
   else
   {
      if (info->flipfunction)
      {
         y[0] &= 16 - 1;
         if (info->flipfunction & 0x2)
         {
            if (!(y[0] & 8))
               y[0] = 8 - 1 - y[0] + 16;
            else
               y[0] = 16 - 1 - y[0];
         }
         else if (y[0] & 8)
            y[0] += 8;

         if (info->flipfunction & 0x1)
         {
            if (!(x[0] & 8))
               y[0] += 8;

            x[0] &= 8-1;
            x[0] = 8 - 1 - x[0];
         }
         else if (x[0] & 8)
         {
            y[0] += 8;
            x[0] &= 8-1;
         }
         else
            x[0] &= 8-1;
      }
      else
      {
         y[0] &= 16 - 1;

         if (y[0] & 8)
            y[0] += 8;
         if (x[0] & 8)
            y[0] += 8;
         x[0] &= 8-1;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void SetupScreenVars(vdp2draw_struct *info, screeninfo_struct *sinfo)
{
   if (!info->isbitmap)
   {
      sinfo->pagepixelwh=64*8;
	  sinfo->pagepixelwh_bits = 9;
	  sinfo->pagepixelwh_mask = 511;

      sinfo->planepixelwidth=info->planew*sinfo->pagepixelwh;
	  sinfo->planepixelwidth_bits = 8+info->planew;
	  sinfo->planepixelwidth_mask = (1<<(sinfo->planepixelwidth_bits))-1;

      sinfo->planepixelheight=info->planeh*sinfo->pagepixelwh;
	  sinfo->planepixelheight_bits = 8+info->planeh;
	  sinfo->planepixelheight_mask = (1<<(sinfo->planepixelheight_bits))-1;

      sinfo->screenwidth=info->mapwh*sinfo->planepixelwidth;
      sinfo->screenheight=info->mapwh*sinfo->planepixelheight;
      sinfo->oldcellx=-1;
      sinfo->oldcelly=-1;
      sinfo->xmask = sinfo->screenwidth-1;
      sinfo->ymask = sinfo->screenheight-1;
      GeneratePlaneAddrTable(info, sinfo->planetbl);
   }
   else
   {
      sinfo->pagepixelwh = 0;
	  sinfo->pagepixelwh_bits = 0;
	  sinfo->pagepixelwh_mask = 0;
      sinfo->planepixelwidth=0;
	  sinfo->planepixelwidth_bits=0;
	  sinfo->planepixelwidth_mask=0;
      sinfo->planepixelheight=0;
	  sinfo->planepixelheight_bits=0;
	  sinfo->planepixelheight_mask=0;
      sinfo->screenwidth=0;
      sinfo->screenheight=0;
      sinfo->oldcellx=0;
      sinfo->oldcelly=0;
      sinfo->xmask = info->cellw-1;
      sinfo->ymask = info->cellh-1;
   }
}

//////////////////////////////////////////////////////////////////////////////

enum LayerNames {
	_NULL=0,NBG0=1,NBG1=2,NBG2=3,NBG3=4,SPRITE=5,RBG0=6,BACK=7
};

struct pixeldata {
	//whew! these barely pack in!!!!
	//if you add more data things will slow down.
	//but we could pack priority and layer if we had to (it is better for these to be unpacked though)

	u32 pixel;
	u32 originalDot;//i have to check the pre-color lookup, conversion etc data for per pixel priority and color calc, i guess we should do the conversion to 32 bit later
	
	u8 priority;
	u8 layer;
	
	u8 colorCount:3;
	u8 spriteColorCalcRatio:5;
	
	u8 specialFunction:3;
	u8 transparent:1;
	u8 colorCalcWindow:1;
	u8 isSpriteColorCalc:1;
	u8 isNormalShadow:1;
	u8 isMSBShadow:1;

	void init() {
		if(sizeof(pixeldata)==8) *((u64*)this)=0;
		else {
			pixel = 0;
			originalDot = 0;

			priority = 0;
			layer = _NULL;

			colorCount = 0;
			spriteColorCalcRatio = 0;

			specialFunction = 0;
			transparent = TRUE;
			colorCalcWindow = FALSE;
			isSpriteColorCalc = FALSE;
			isNormalShadow = FALSE;
			isMSBShadow = FALSE;
		}
	}
};


pixeldata rbg0buf[704*512];
pixeldata nbg0buf[704*512];
pixeldata nbg1buf[704*512];
pixeldata nbg2buf[704*512];
pixeldata nbg3buf[704*512];
pixeldata backbuf[704*512];
pixeldata spritebuf[704*512];

template<int RESXRATIO, LayerNames layer>
void __Vdp2DrawScroll(vdp2draw_struct *info)
{
   int i, j;
   int x, y;
   screeninfo_struct sinfo;
   int scrollx, scrolly;
   int *mosaic_y, *mosaic_x;

   const int width = vdp2width/RESXRATIO;
   const int height = vdp2height;

   SetupScreenVars(info, &sinfo);

   scrollx = info->x;
   scrolly = info->y;

   {
	   static int tables_initialized = 0;
	   static int mosaic_table[16][1024];
	   if(!tables_initialized)
	   {
		   tables_initialized = 1;
			for(i=0;i<16;i++)
			{
				int m = i+1;
				for(j=0;j<1024;j++)
					mosaic_table[i][j] = j/m*m;
			}
		}
		mosaic_x = (int*)&mosaic_table[info->mosaicxmask-1];
		mosaic_y = (int*)&mosaic_table[info->mosaicymask-1];
	}

   int scrollincrement;

   int pixaddr=0;
	for (j = 0; j < height; j++)
	{
		int Y;
		int linescrollx = 0;
		// precalculate the coordinate for the line(it's faster) and do line
		// scroll
		if (info->islinescroll)
		{
			if (info->islinescroll & 0x1)
			{
				linescrollx = (T1ReadLong(Vdp2Ram, info->linescrolltbl) >> 16) & 0x7FF;
				info->linescrolltbl += 4;
			}
			if (info->islinescroll & 0x2)
			{
				info->y = (T1ReadWord(Vdp2Ram, info->linescrolltbl) & 0x7FF) + scrolly;
				info->linescrolltbl += 4;
				y = info->y;
			}
			else
				//y = info->y+((int)(info->coordincy *(float)(info->mosaicymask > 1 ? (j / info->mosaicymask * info->mosaicymask) : j)));
				y = info->y + info->coordincy*mosaic_y[j];
			if (info->islinescroll & 0x4)
			{
				info->coordincx = (T1ReadLong(Vdp2Ram, info->linescrolltbl) & 0x7FF00) / 65536.0f;
				info->linescrolltbl += 4;
         }
      }
      else
         //y = info->y+((int)(info->coordincy *(float)(info->mosaicymask > 1 ? (j / info->mosaicymask * info->mosaicymask) : j)));
		 y = info->y + info->coordincy*mosaic_y[j];

      y &= sinfo.ymask;

      if (info->isverticalscroll)
      {
         // this is *wrong*, vertical scroll use a different value per cell
         // info->verticalscrolltbl should be incremented by info->verticalscrollinc
         // each time there's a cell change and reseted at the end of the line...
			// or something like that :)
			y += T1ReadLong(Vdp2Ram, info->verticalscrolltbl) >> 16;
			y &= 0x1FF;
      }

      Y=y;

      for (i = 0; i < width; i++)
      {
         u32 color;

         //x = info->x+((int)(info->coordincx*(float)((info->mosaicxmask > 1) ? (i / info->mosaicxmask * info->mosaicxmask) : i)));
		 x = info->x + (mosaic_x[i]*info->coordincx);
		 x &= sinfo.xmask;

		 if (linescrollx) {
			 x += linescrollx;
			 x &= 0x3FF;
		 }

		 // Fetch Pixel, if it isn't transparent, continue
		 if (!info->isbitmap)
		 {
			 // Tile
			 y=Y;
			 Vdp2MapCalcXY(info, &x, &y, &sinfo);
		 }

		 pixeldata p;
		 p.init();

		 p.transparent = !Vdp2FetchPixel(info, x, y, &color, &p.originalDot);
		 p.pixel = color;
		 p.specialFunction = info->specialfunction;
		 p.layer = layer;

		 for(int q=0;q<RESXRATIO;q++)
		 {
			 if(layer == NBG0) nbg0buf[pixaddr++] = p;
			 if(layer == NBG1) nbg1buf[pixaddr++] = p;
			 if(layer == NBG2) nbg2buf[pixaddr++] = p;
			 if(layer == NBG3) nbg3buf[pixaddr++] = p;
		 }
	 } //i<width loop
   } //j<height loop
}


template<int RESXRATIO>
void FASTCALL _Vdp2DrawScroll(vdp2draw_struct *info, LayerNames layer)
{
	switch(layer) {
		case NBG0: __Vdp2DrawScroll<RESXRATIO,NBG0>(info); break;
		case NBG1: __Vdp2DrawScroll<RESXRATIO,NBG1>(info); break;
		case NBG2: __Vdp2DrawScroll<RESXRATIO,NBG2>(info); break;
		case NBG3: __Vdp2DrawScroll<RESXRATIO,NBG3>(info); break;
	}
}

extern "C" void FASTCALL VidSoftVdp2DrawScroll(vdp2draw_struct *info, LayerNames layer)
{
	if(resxratio==1)
		_Vdp2DrawScroll<1>(info,layer);
	else _Vdp2DrawScroll<2>(info,layer);
}


//TODO ------------- vdp2 debugging is broken
extern "C" void FASTCALL Vdp2DrawScroll(vdp2draw_struct *info, LayerNames layer, int width, int height)
{
	VidSoftVdp2DrawScroll(info,layer);
}


static bool Vdp2DrawNBG0(void);
static bool Vdp2DrawNBG1(void);
static bool Vdp2DrawNBG2(void);
static bool Vdp2DrawNBG3(void);
static bool Vdp2DrawRBG0(void);

u32 ColorCalc(u32 toppixel, u32 bottompixel, u32 alpha);

//reduce luminance of specified vdp2 pixel
u32 Shadow(u32 vdp2, u32 intensity)
{
	s16 r, g, b;

	r = (vdp2 & 0xFF);
	g = ((vdp2 >> 8) & 0xFF);
	b = ((vdp2 >> 16) & 0xFF);

	r = r - intensity;
	g = g - intensity;
	b = b - intensity;

	if(b < 0) b = 0;
	if(g < 0) g = 0;
	if(r < 0) r = 0;

	vdp2 = (b << 16) | (g << 8) | r;

	return vdp2;
}
bool isSpriteColorCalc(u8 prioritytable[], int priorityRegisterNumber, int msb) {

	//must be enabled
	if(!(Vdp2Regs->CCCTL & (1 << 6)))
		return false;

	//the color calculation enable condition number
	int colorCalculationConditionNumber = (Vdp2Regs->SPCTL & 0x700) >> 8;
	//(Vdp2Regs->SPCTL & 0x700) >> 8)
	//check if color calc is enabled for this sprite pixel
	switch ((Vdp2Regs->SPCTL & 0x3000) >> 12) {
		case 0:
			if(prioritytable[priorityRegisterNumber] <= colorCalculationConditionNumber)
				return true;
			break;
		case 1:
			if(prioritytable[priorityRegisterNumber] == colorCalculationConditionNumber)
				return true;
			break;
		case 2:
			if(prioritytable[priorityRegisterNumber] >= colorCalculationConditionNumber)
				return true;
			break;
		case 3:
			if(msb)
				return true;
			break;
	}
	return false;
}

bool isSpriteNormalShadow(int pixel) {

	//see if the color data meets the conditions for normal shadow
	//if each color data bit is set except the lsb then it's a shadow
	switch(vdp1spritetype) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 5:
	if(pixel == 0x7fe)
		return true;
	break;

	case 4:
	case 6:
	if(pixel == 0x3fe)
		return true;
	break;

	case 7:
	if(pixel == 0x1fe)
		return true;
	break;

	//6 bit color data
	case 8:
	case 0xC:
	if(pixel == 0x7e)
		return true;
	break;

	// 5 bit color data
	case 9:
	case 0xA:
	case 0xB:
	case 0xD:
	case 0xE:
	case 0xF:
	if(pixel == 0x3e)
		return true;
	break;
	}

	return false;
}

template<int RESXRATIO>
bool Vdp2DrawSprites() {
	int i, i2, pixaddr;
	u16 pixel;
	u8 prioritytable[8];
	u32 vdp1coloroffset;
	int colormode = Vdp2Regs->SPCTL & 0x20;

	if (!Vdp1Regs->disptoggle)
		return false;

	int colorCalculationRatio[8];

	//get all the sprite color calc ratios
	colorCalculationRatio[0] = Vdp2Regs->CCRSA & 0x1f;
	colorCalculationRatio[1] = Vdp2Regs->CCRSA >> 8;
	colorCalculationRatio[2] = Vdp2Regs->CCRSB & 0x1f;
	colorCalculationRatio[3] = Vdp2Regs->CCRSB >> 8;
	colorCalculationRatio[4] = Vdp2Regs->CCRSC & 0x1f;
	colorCalculationRatio[5] = Vdp2Regs->CCRSC >> 8;
	colorCalculationRatio[6] = Vdp2Regs->CCRSD & 0x1f;
	colorCalculationRatio[7] = Vdp2Regs->CCRSD >> 8;

	prioritytable[0] = Vdp2Regs->PRISA & 0x7;
	prioritytable[1] = (Vdp2Regs->PRISA >> 8) & 0x7;
	prioritytable[2] = Vdp2Regs->PRISB & 0x7;
	prioritytable[3] = (Vdp2Regs->PRISB >> 8) & 0x7;
	prioritytable[4] = Vdp2Regs->PRISC & 0x7;
	prioritytable[5] = (Vdp2Regs->PRISC >> 8) & 0x7;
	prioritytable[6] = Vdp2Regs->PRISD & 0x7;
	prioritytable[7] = (Vdp2Regs->PRISD >> 8) & 0x7;

	vdp1coloroffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
	vdp1spritetype = Vdp2Regs->SPCTL & 0xF;

	int width = vdp2width / RESXRATIO;

	for (i2 = 0, pixaddr = 0; i2 < vdp2height; i2++)
	{
		for (i = 0; i < width; i++)
		{
			pixeldata p;
			p.init();
			p.layer = SPRITE;
			if (vdp1pixelsize == 2)
			{
				// 16-bit pixel size
				//16 bit pixels will be drawn at 2x width, and sometimes 2x height
				pixel = ((u16 *)vdp1frontframebuffer)[((i2>>resyratio_shift) * vdp1width) + i];

				if (pixel == 0)
					p.transparent = true;
				else if (pixel & 0x8000 && colormode)
				{
					// 16 BPP
					// if pixel is 0x8000, only draw pixel if sprite window
					// is disabled/sprite type 2-7. sprite types 0 and 1 are
					// -always- drawn and sprite types 8-F are always
					// transparent.
					if (pixel != 0x8000 || vdp1spritetype < 2 || (vdp1spritetype < 8 && !(Vdp2Regs->SPCTL & 0x10))) {
						p.pixel = COLSAT2YAB16(0xFF, pixel);//info.PostPixelFetchCalc(&info, COLSAT2YAB16(0xFF, pixel));
						p.priority = prioritytable[0];//currentpriority;
						p.transparent = false;
					}
					else
						p.transparent = true;
				}
				else
				{
					//Color bank mode
					int priorityRegisterNumber;
					int shadow;
					int colorcalc;
					int msb = pixel & 0x8000;
					int pixelcopy = pixel;
					Vdp1ProcessSpritePixel(vdp1spritetype, &pixel, &shadow, &priorityRegisterNumber, &colorcalc);
					p.transparent = false;
					p.isMSBShadow = (shadow && (Vdp2Regs->SDCTL & 0x100));
					//i go to color ram with this i'll get garbage and break msb shadow on backgrounds
					//apparently if the pixel is 0x8000 it doesn't go to color ram and instead just uses it as is
					if(pixelcopy == 0x8000 && p.isMSBShadow) {
						p.pixel = 0x8000;
						p.layer = SPRITE;
						p.priority = prioritytable[priorityRegisterNumber];
					}
					else {
						
					p.pixel = Vdp2ColorRamGetColor(vdp1coloroffset + pixel);
					//strange crash in taromaru "fix", TODO figure out the actual issue
					if(priorityRegisterNumber>7)
					{
						p.priority = 0; 
					//	printf("priority out of bounds, fix this");
					}
					else
						p.priority = prioritytable[priorityRegisterNumber];
					p.isSpriteColorCalc = isSpriteColorCalc(prioritytable, priorityRegisterNumber, msb);
					//ok, if you open a game, then open winter heat it will crash here.
					if(colorcalc>7  || colorcalc == 0xcccccccc)
					{
						p.spriteColorCalcRatio=0; 
					//	printf("color calc out of bounds, fix this");
					}
					else
						p.spriteColorCalcRatio = colorCalculationRatio[colorcalc];
					p.isNormalShadow = isSpriteNormalShadow(pixel);
					}
					
				}
			}
			else
			{
				// 8-bit pixel size
				pixel = vdp1frontframebuffer[(i2*2 * vdp1width) + (i*2)];

				p.transparent = false;

				if (pixel == 0)
					p.transparent = true;

				p.pixel = COLSAT2YAB32(0xFF, Vdp2ColorRamGetColor(vdp1coloroffset + pixel));
				p.priority = prioritytable[0];
				
			}

			for(int q=0;q<RESXRATIO;q++)
				 spritebuf[pixaddr++] = p;
		}
	}

	return true;
}
struct ColorOffsetData {
	int red;
	int green;
	int blue;
};
struct LayerInfo
{
	//try to make sure this is packed into a convenient size
	int wctl, alpha, specialColorCalculationMode, colorOffsetMask, lineColorScreenInserts, gradationCalculation, bitmapSpecialColorCalculation, bitmapSpecialPriorityFunction, specialPriorityFunction;
	pixeldata* buf;
	u8 specialFunctionCodeB, colorCalculationEnable;
	u8 pad[2];
	ColorOffsetData colorOffset;
};
LayerInfo layerInfoCache[7];
int shadowEnabledCache[7];
u8 colorCountCache[7];
INLINE const LayerInfo* readLayerInfo(const pixeldata &first);

//need to look up format of the pixel
INLINE u8 getColorCount(LayerNames layer) {
	u8 colorCount = 0;
	if(layer == NBG0) colorCount = (Vdp2Regs->CHCTLA & 0x70) >> 4;
	if(layer == NBG1) colorCount = (Vdp2Regs->CHCTLA & 0x1800) >> 12;
	if(layer == NBG2) colorCount = (Vdp2Regs->CHCTLB & 0x1);
	if(layer == NBG3) colorCount = (Vdp2Regs->CHCTLB & (1 << 5)) >> 5;
	if(layer == RBG0) colorCount = (Vdp2Regs->CHCTLB & 0x7000) >> 12;
	return colorCount;
}

INLINE bool doTest(const int wctl, clipping_struct clip[], int i, int j) {

	//if sprite window is enabled in the wctl register but not
	//in spctl we do nothing
	//sexy parodius requires this to show transparent sprites
	//in the level briefing screens and intro
	if((wctl & (1 << 5)) && !(Vdp2Regs->SPCTL & (1 << 4))) {
		return false;
	}

	//if both windows are enabled and AND is enabled
	//if((wctl & 0x2) && (wctl & 0x8) && (wctl & 0x80) == 0x80) {
	if((wctl & 0x8A) == 0x8A) {
		// test AND window logic
		if(!TestWindow(wctl, 0x2, 0x1, &clip[0], i, j) && !TestWindow(wctl, 0x8, 0x4, &clip[1], i, j)) {
			return true;
		}
	}
	else
	{
		//or window logic
		if (!TestWindow(wctl, 0x2, 0x1, &clip[0], i, j) || !TestWindow(wctl, 0x8, 0x4, &clip[1], i,j)) {
			return true;
		}
	}
	return false;
}

//static INLINE void VidSoftReadWindowData(int wctl, clipping_struct *clip)
//{
//   if (wctl & 0x2)
//   {
//      clip[0].xstart = Vdp2Regs->WPSX0; 
//      clip[0].ystart = Vdp2Regs->WPSY0;
//      clip[0].xend = Vdp2Regs->WPEX0; 
//      clip[0].yend = Vdp2Regs->WPEY0;
//   }
//
//	else if (wctl & 0x8)
//	{
//		clip[1].xstart = Vdp2Regs->WPSX1;
//		clip[1].ystart = Vdp2Regs->WPSY1;
//		clip[1].xend = Vdp2Regs->WPEX1;
//		clip[1].yend = Vdp2Regs->WPEY1;
//	}
//
//	else if (wctl & 0x20)
//	{
//		// fix me
//
//		clip[0].xstart = clip[0].ystart = clip[0].xend = clip[0].yend = 0;
//		clip[1].xstart = clip[1].ystart = clip[1].xend = clip[1].yend = 0;
//	}
//}

template<LayerNames LAYER>
INLINE bool testWindow(pixeldata &p, int i, int j) {


	int islinewindow = false;

	p.colorCalcWindow = FALSE;

	u32 linewnd0addr, linewnd1addr;
	clipping_struct clip[2];


	linewnd0addr = linewnd1addr = 0;

	const LayerInfo* layerInfo = readLayerInfo(p);

	int wctl = layerInfo->wctl;

	//this just reads the coordinates for the two regular rectangular windows
	ReadWindowData(layerInfo->wctl, clip);

	ReadLineWindowData(&islinewindow, wctl, &linewnd0addr, &linewnd1addr);
	// if line window is enabled, adjust clipping values
	ReadLineWindowClip(islinewindow, clip, linewnd0addr+(j*4), linewnd1addr+(j*4));

	if(doTest(wctl, clip, i, j))
		return true;

	//now that we have checked for regular window and made it here
	//if either window is being used for color calc window
	//no idea if any games use color calc line or sprite window

	//winter heat has rotation parameter window enabled, not sure if it's used
	if(Vdp2Regs->WCTLD & (1 << 9) || Vdp2Regs->WCTLD & (1 << 11)) {

		wctl = Vdp2Regs->WCTLD >> 8;

		ReadWindowData(wctl, clip);

		if(doTest(wctl, clip, i, j)) {
			p.colorCalcWindow = TRUE;
			return false;
		}
	}

	return false;
}

ColorOffsetData readColorOffset(int offset){

	ColorOffsetData colorOffset;

	if (Vdp2Regs->CLOFSL & offset)
	{
		// color offset B
		colorOffset.red = Vdp2Regs->COBR & 0xFF;
		if (Vdp2Regs->COBR & 0x100)
			colorOffset.red |= 0xFFFFFF00;

		colorOffset.green = Vdp2Regs->COBG & 0xFF;
		if (Vdp2Regs->COBG & 0x100)
			colorOffset.green |= 0xFFFFFF00;

		colorOffset.blue = Vdp2Regs->COBB & 0xFF;
		if (Vdp2Regs->COBB & 0x100)
			colorOffset.blue |= 0xFFFFFF00;
	}
	else
	{
		// color offset A
		colorOffset.red = Vdp2Regs->COAR & 0xFF;
		if (Vdp2Regs->COAR & 0x100)
			colorOffset.red |= 0xFFFFFF00;

		colorOffset.green = Vdp2Regs->COAG & 0xFF;
		if (Vdp2Regs->COAG & 0x100)
			colorOffset.green |= 0xFFFFFF00;

		colorOffset.blue = Vdp2Regs->COAB & 0xFF;
		if (Vdp2Regs->COAB & 0x100)
			colorOffset.blue |= 0xFFFFFF00;
	}

	return colorOffset;
}

INLINE void readLayerInfoByLayer(LayerNames layer, LayerInfo& info)
{
	info.colorCalculationEnable = FALSE;

	switch(layer) {
	case NBG0:
		info.wctl = Vdp2Regs->WCTLA;
		info.colorCalculationEnable = Vdp2Regs->CCCTL & 0x1;
		info.alpha = Vdp2Regs->CCRNA & 0x1f;
		info.specialColorCalculationMode = Vdp2Regs->SFCCMD & 0x3;
		info.colorOffset = readColorOffset(1);
		info.colorOffsetMask = 1;
		info.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 0);
		info.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 2;
		info.buf = &nbg0buf[0];
		info.specialFunctionCodeB = (Vdp2Regs->SFSEL & (1 << 0));
		info.bitmapSpecialPriorityFunction = Vdp2Regs->BMPNA & (1 << 5);
		info.bitmapSpecialColorCalculation = Vdp2Regs->BMPNA & (1 << 4);
		info.specialPriorityFunction = Vdp2Regs->SFPRMD & 0x3;
		break;
	case NBG1:
		info.wctl = Vdp2Regs->WCTLA >> 8;
		info.colorCalculationEnable = Vdp2Regs->CCCTL & 0x2;
		info.alpha = Vdp2Regs->CCRNA >> 8;
		info.specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 2) & 0x3;
		info.colorOffset = readColorOffset(2);
		info.colorOffsetMask = 2;
		info.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 1);
		info.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 4;
		info.buf = &nbg1buf[0];
		info.specialFunctionCodeB = (Vdp2Regs->SFSEL & (1 << 1));
		info.bitmapSpecialPriorityFunction = (Vdp2Regs->BMPNA >> 8) & (1 << 5);
		info.bitmapSpecialColorCalculation = (Vdp2Regs->BMPNA >> 8) & (1 << 4);
		info.specialPriorityFunction = (Vdp2Regs->SFPRMD >> 2) & 0x3;
		break;
	case NBG2:
		info.wctl = Vdp2Regs->WCTLB;
		info.colorCalculationEnable = Vdp2Regs->CCCTL & 0x4;
		info.alpha = Vdp2Regs->CCRNB & 0x1f;
		info.specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 4) & 0x3;
		info.colorOffset = readColorOffset(4);
		info.colorOffsetMask = 4;
		info.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 2);
		info.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 5;
		info.buf = &nbg2buf[0];
		info.specialFunctionCodeB = (Vdp2Regs->SFSEL & (1 << 2));
		//this layer cannot use these
		info.bitmapSpecialPriorityFunction = 0;
		info.bitmapSpecialColorCalculation = 0;
		info.specialPriorityFunction = (Vdp2Regs->SFPRMD >> 4) & 0x3;
		break;
	case NBG3:
		info.wctl = Vdp2Regs->WCTLB >> 8;
		info.colorCalculationEnable = Vdp2Regs->CCCTL & 0x8;
		info.alpha = Vdp2Regs->CCRNB >> 8;
		info.specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 6) & 0x3;
		info.colorOffset = readColorOffset(8);
		info.colorOffsetMask = 8;
		info.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 3);
		info.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 6;
		info.buf = &nbg3buf[0];
		info.specialFunctionCodeB = (Vdp2Regs->SFSEL & (1 << 3));
		//this layer cannot use these
		info.bitmapSpecialPriorityFunction = 0;
		info.bitmapSpecialColorCalculation = 0;
		info.specialPriorityFunction = (Vdp2Regs->SFPRMD >> 6) & 0x3;
		break;
	case RBG0:
		info.wctl = Vdp2Regs->WCTLC & 0x1f;
		info.colorCalculationEnable = Vdp2Regs->CCCTL & 0x10;
		info.alpha = Vdp2Regs->CCRR & 0x1f;
		info.specialColorCalculationMode = (Vdp2Regs->SFCCMD >> 8) & 0x3;
		info.colorOffset = readColorOffset(0x10);
		info.colorOffsetMask = 0x10;
		info.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 4);
		info.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 1;
		info.buf = &rbg0buf[0];
		info.specialFunctionCodeB = (Vdp2Regs->SFSEL & (1 << 4));
		info.bitmapSpecialPriorityFunction = Vdp2Regs->BMPNB & (1 << 5);
		info.bitmapSpecialColorCalculation = Vdp2Regs->BMPNB & (1 << 4);
		info.specialPriorityFunction = (Vdp2Regs->SFPRMD >> 8) & 0x3;
		break;
	}

	readColorOffset(info.colorOffsetMask);
}

LayerInfo cacheSpriteLayerInfo;

static void updateCacheSpriteLayerInfo()
{
	cacheSpriteLayerInfo.colorOffsetMask = 0x40;
	cacheSpriteLayerInfo.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 5);
	cacheSpriteLayerInfo.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 0;
	cacheSpriteLayerInfo.buf = &spritebuf[0];
	cacheSpriteLayerInfo.colorOffset = readColorOffset(0x40);
	cacheSpriteLayerInfo.wctl = Vdp2Regs->WCTLC >> 8;
//	readColorOffset(cacheSpriteLayerInfo.colorOffsetMask);
}

INLINE const LayerInfo* readLayerInfo(const pixeldata &first)
{
	if(first.layer == SPRITE)
	{
		cacheSpriteLayerInfo.colorCalculationEnable = first.isSpriteColorCalc;
		cacheSpriteLayerInfo.alpha = first.spriteColorCalcRatio;
		//cacheSpriteLayerInfo.colorOffsetMask = 0x40;
		//cacheSpriteLayerInfo.lineColorScreenInserts = Vdp2Regs->LNCLEN & (1 << 5);
		//cacheSpriteLayerInfo.gradationCalculation = ((Vdp2Regs->CCCTL & 0x7000) >> 12) == 0;
		//cacheSpriteLayerInfo.buf = &spritebuf[0];
		//readColorOffset(cacheSpriteLayerInfo.colorOffsetMask);
		return &cacheSpriteLayerInfo;
	} 
	else return &layerInfoCache[first.layer];
}

bool isSpecialFunctionCode(int SFCODEAorB, int pixel) {

	//this might not be safe since a dev somewhere
	//may have set two of these bits to 1
	switch(SFCODEAorB) {
	case 1:
		if(pixel == 0 || pixel == 1)
			return true;
		break;
	case 2:
		if(pixel == 2 || pixel == 3)
			return true;
		break;
	case 4:
		if(pixel == 4 || pixel == 5)
			return true;
		break;
	case 8:
		if(pixel == 6 || pixel == 7)
			return true;
		break;
	case 0x10:
		if(pixel == 8 || pixel == 9)
			return true;
		break;
	case 0x20:
		if(pixel == 0xA || pixel == 0xB)
			return true;
		break;
	case 0x40:
		if(pixel == 0xC || pixel == 0xD)
			return true;
		break;
	case 0x80:
		if(pixel == 0xE || pixel == 0xF)
			return true;
		break;
	default:
		return false;
	}
	return false;
}

//---------------------------------
//access to color ram is cached through a table 
//setup once per frame based on Vdp2Internal.ColorMode
//
static INLINE u32 FASTCALL _Vdp2ColorRamGetColor(u32 addr)
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);

         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0xFC00) << 9));//changed to FC to preserve the msb for per pixel color calc in waku puyo dungeon
      }
      case 1:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);
         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0xFC00) << 9));//changed to FC to preserve the msb for per pixel color calc in radiant silvergun
      }
      case 2:
      {
         addr <<= 2;   
         return T2ReadLong(Vdp2ColorRam, addr & 0xFFF);
      }
      default: break;
   }

   return 0;
}

u32 cacheColorTable[0xFFF];

static void updateCacheColorTable()
{
	for(u32 i=0;i<0xFFF;i++)
		cacheColorTable[i] = _Vdp2ColorRamGetColor(i);
}

static INLINE u32 Vdp2ColorRamGetColor (u32 addr)
{
	return cacheColorTable[addr&0xFFF];
}

//-----------------------------

enum specialModes {SCREEN = 0, CHARACTER = 1, DOT = 2, MSB = 3};

void composite(pixeldata pixelstack[], int i, int j, int pixaddr){

	int colorCalculationEnable = FALSE;
	int alpha = 0;
	int specialColorCalculationMode = 0;

	pixeldata &first = pixelstack[0];
	pixeldata &second = pixelstack[1];
	pixeldata &third = pixelstack[2];

	const LayerInfo* layerInfo = readLayerInfo(first);

	//TODO color calculation window is broken as well as some other color calc regressions
	colorCalculationEnable = layerInfo->colorCalculationEnable;
	alpha = layerInfo->alpha;
	specialColorCalculationMode = layerInfo->specialColorCalculationMode;

	//gradation calculation
	//see the sega blur demo
	if(Vdp2Regs->CCCTL & (1 << 15) && layerInfo->gradationCalculation) {

		pixeldata left2 = layerInfo->buf[((pixaddr-2) > 0)?(pixaddr-2):0];
		pixeldata left1 = layerInfo->buf[((pixaddr-1) > 0)?(pixaddr-1):0];
		pixeldata disp = layerInfo->buf[pixaddr];
		pixeldata intermediate;
		intermediate.pixel = ColorCalc(left2.pixel,left1.pixel,7);
		disp.pixel = ColorCalc(disp.pixel,intermediate.pixel,15);
		vdp2framebuffer[pixaddr] = disp.pixel;
		return;
	}
	//extended color calculation is enabled
	//i haven't found any line color insertion test cases so don't expect those to work
	else if(Vdp2Regs->CCCTL & (1 << 10)) {

		//we have to know the bit depth of the pixels to be blended
		second.colorCount = colorCountCache[second.layer];
		third.colorCount = colorCountCache[third.layer];

		const LayerInfo *secondLayerInfo = readLayerInfo(second);
		const LayerInfo *thirdLayerInfo = readLayerInfo(third);

		//blend the second and third pixels
		//and write the result over the second

		//color ram mode 0
		if(((Vdp2Regs->RAMCTL & 0x3000) >> 12) == 0) {
			// printf("mode 0 extended color calc");

			//line color screen does not insert
			//if(!(Vdp2Regs->CCCTL & (1 << 5))) {
			//i think this must be the check
			if(!secondLayerInfo->lineColorScreenInserts) {


				//2nd and 3rd images can be palette or rbg format
				int a;
				int enabled;

				//got to check if the second pixel has color calc enabled
				// const LayerInfo *secondLayerInfo = readLayerInfo(second); //, a, enabled, a, a, a, i, j);

				if(secondLayerInfo->colorCalculationEnable) {
					//madou monogatari intro with angel activates this mode
					//2:2:0
					second.pixel = ColorCalc(second.pixel,third.pixel,15);
				}
				else {
					//2nd pixel is 100% visible
				}
			}
			//line color screen inserts
			else {
				//i get the impression line color screen enable must be on for the given layer
				//LNCLEN



				// if() {
				// printf("yay");
				// }

				//sonic R enables this

				// printf("mode 0 extended color calc + lcl");
				//for the sake of clarity let's move the slots around
				//like described in the manual

				pixeldata lineColorPixel;//TODO

				pixeldata fourth = third;
				pixeldata third = second;
				// if(third.layer == NBG0)
				// lineColorPixel.pixel = 0xffff;
				// else
				lineColorPixel.pixel = 0;

				int lineColorScreenAddress = ((Vdp2Regs->LCTA.all & 0x7FFFFUL) * 2);
				//sonic r title screen is fetching 0x0.
				//is this the correct way to look it up?
				lineColorPixel.pixel = T1ReadWord(Vdp2Ram, lineColorScreenAddress);
				// int lineColorPixelAlpha = Vdp2Regs->CCRLB & 0x1f;
				second = lineColorPixel;

				// const LayerInfo *secondLayerInfo = readLayerInfo(second);

				//uhh wait secondLayerInfo in this case is useless
				//since the second layer is the line color screen

				//so if the line color screen has color calc disabled
				if(!(Vdp2Regs->CCCTL & (1 << 5))) {//secondLayerInfo->colorCalculationEnable

					//second layer is 100% visible
					//which means we use the line color screen 100%?
				}
				else {

					//

					if(!thirdLayerInfo->colorCalculationEnable) {
						//2:2:0
						// printf("ccc");

						ColorCalc(second.pixel,third.pixel,15);
					}
					else {
						//sonic r uses this mode in races

						// printf("cccccc");
						//the manual says 2:1:0 but that doesn't make any sense
						//so i'm going to go with 2:1:1 until demonstrated otherwise
						ColorCalc(third.pixel,fourth.pixel,7);
						ColorCalc(second.pixel,third.pixel,15);
					}
				}
			}
		}

		//color ram mode 1 or 2
		else {
			//line color screen does not insert
			// if(!(Vdp2Regs->CCCTL & (1 << 5))) {
			if(!secondLayerInfo->lineColorScreenInserts) {

				//3rd image is palette format
				if(third.colorCount < 3) {
					//the second layer pixel is 100% visible
				}
				//3rd image is rgb format
				if(third.colorCount >= 3) {
					int a;
					int enabled;

					//got to check if the second pixel has color calc enabled
					// const LayerInfo* secondLayerInfo = readLayerInfo(second); //, a, enabled, a, a, a, i, j);

					if(secondLayerInfo->colorCalculationEnable) {
						//2:2:0
						second.pixel = ColorCalc(second.pixel,third.pixel,15);
					}
					else {
						//2nd pixel is 100% visible
					}
				}
			}
			//line color screen inserts
			else {
				printf("mode 1 or 2 extended + lcl");
				//the line color screen is treated as the second image
				//moving the 2nd and third pixels into 3rd and fourth slots
				//for the sake of clarity let's move the slots around
				//like described in the manual

				pixeldata lineColorPixel;//TODO

				pixeldata fourth = third;
				pixeldata third = second;
				second = lineColorPixel;

				//third image is palette format
				if(third.colorCount < 3) {
					//2nd image is 100% visible
				}
				//third image is rgb format
				if(third.colorCount >= 3) {

					if(fourth.colorCount < 3) {

						// const LayerInfo* secondLayerInfo = readLayerInfo(second);

						if(!secondLayerInfo->colorCalculationEnable) {
							//2nd image is 100% visible
						}
						else {
							//2:2:0
							second.pixel = ColorCalc(second.pixel,third.pixel,15);
						}
					}

					//if both layers are rgb
					if(fourth.colorCount >= 3) {

						// const LayerInfo* secondLayerInfo = readLayerInfo(second);

						if(!secondLayerInfo->colorCalculationEnable) {
							//2nd layer is 100% visible
						}
						else {
							// const LayerInfo* thirdLayerInfo = readLayerInfo(third);

							if(!thirdLayerInfo->colorCalculationEnable) {
								//2:2:0
								second.pixel = ColorCalc(second.pixel,third.pixel,15);
							}
							//3rd image color calc enabled
							else {
								//2:1:1
								third.pixel = ColorCalc(third.pixel,fourth.pixel,7);
								second.pixel = ColorCalc(second.pixel,third.pixel,15);
							}
						}
					}
				}
			}
		}
	}
	//if we are selecting color calc by the second screen side
	//parodius penguin boss uses this
	if(Vdp2Regs->CCCTL & (1 << 9)) {

		//i thought having color calc disabled on the second layer
		//would disable color calc but this is apparently not the case
		//see winter heat title screen

		//also it is wrong to check if the second layer has cc enabled,
		//see the title screen of sega ages memorial volume 2

		if(second.layer == NBG0)
			alpha = Vdp2Regs->CCRNA & 0x1f;

		if(second.layer == NBG1)
			alpha = Vdp2Regs->CCRNA >> 8;

		if(second.layer == NBG2)
			alpha = Vdp2Regs->CCRNB & 0x1f;

		if(second.layer == NBG3)
			alpha = Vdp2Regs->CCRNB >> 8;

		if(second.layer == RBG0)
			alpha = Vdp2Regs->CCRR & 0x1f;

		if(second.layer == SPRITE)
			alpha = second.spriteColorCalcRatio;

		if(second.layer == BACK)
			alpha = Vdp2Regs->CCRLB >> 8;
	}

	if(specialColorCalculationMode==1){
		//legend of oasis uses this mode with a bitmap scroll screen
		//so we need to check the bit map palette number register
		//well... it apparently isn't actually using this
		//it is using special color calculation mode 2

		// int bitmapSpecialPriority = 0;
		// int bitmapSpecialColorCalculation = 0;

		if(first.specialFunction || layerInfo->bitmapSpecialColorCalculation)
			colorCalculationEnable = true;

		else
			colorCalculationEnable = false;
		//legend of oasis is going to blend the back screen into the front unless i fix this
		// if(bitmapSpecialColorCalculation)
		// colorCalculationEnable = true;
		// if(first.specialFunction) {
		// printf("a");
		// }

	}
	//puzzle bobble 3 uses this to make a semi transparent layer behind the playing area
	//the special function cc bit in pattern name data must be enabled in tile mode
	if(specialColorCalculationMode==2){

		//pattern name data color calc bit must be set for this to work in tile mode
		//it should be noted i'm not actually checking the just the cc bit at the moment,
		//but both cc and special priority since they are stored in .specialFunction

		//check the special function of it is a tiled layer, and the bitmap palette number register special color calculation bit if it is a bitmap
		//there should probably be a guard against tiled layers accidentally tripping bitmapped and vice versa
		//legend of oasis uses bitmap mode 2 for the HUD
		if(first.specialFunction || layerInfo->bitmapSpecialColorCalculation) {

			//layerInfo->patternNameDataColorCalculation
			//pattern name data size
			//if it's one word we use the supplement register
			//TODO get all that supplement data working
			// if(Vdp2Regs->PNCN0 & (1 << 15))
			// printf("one word");
			// if(Vdp2Regs->PNCN1 & (1 << 15))
			// printf("one word");
			// if(Vdp2Regs->PNCN2 & (1 << 15))
			// printf("one word");
			// if(Vdp2Regs->PNCN3 & (1 << 15))
			// printf("one word");
			//otherwise we need to interpret the partern

			//it's always the lower 4 bits
			//i may need to use the original format pixels,
			//not the 32 bit ones in the buffers
			int temppixel = first.originalDot & 0xF;//first.pixel & 0xF;//(first.pixel >> 3) & 0xF;//

			//see which function code we are using, A or B
			//and then check to see if the lower 4 bits match the designated code or not
			if(!layerInfo->specialFunctionCodeB) {
				//special function code A
				if(isSpecialFunctionCode(Vdp2Regs->SFCODE & 0xFF,temppixel))
					colorCalculationEnable = true;
				else
					colorCalculationEnable = false;
			}
			else {
				//special function code B
				if(isSpecialFunctionCode((Vdp2Regs->SFCODE & 0xFF00)>> 8,temppixel)) {
					colorCalculationEnable = true;
				}
				else
					colorCalculationEnable = false;
			}
		}
		else
			colorCalculationEnable = false;
	}

	//activated based on the msb of the color data
	//in the case of radiant silvergun portraits it is looked up
	//and the value is actually from color ram transferred to first.pixel
	if(specialColorCalculationMode == 3) {
		if(first.pixel &(0x8000 << 9))//i'm not really sure pixel data gets stored like this internally but this is ok for now i guess
			colorCalculationEnable = 1;
		else
			colorCalculationEnable = 0;
	}

	///////////////////////////////////////////////////////////////////
	//temporary line color screen testing////////////////////

	int scrAddr = ((Vdp2Regs->LCTA.all & 0x7FFFFUL) * 2);
	int lineColoraddress = T1ReadWord(Vdp2Ram, scrAddr);
	int linecolor = Vdp2ColorRamGetColor(lineColoraddress);

	if((Vdp2Regs->CCCTL & (1 << 5)) && !first.colorCalcWindow && layerInfo->lineColorScreenInserts) {
		if(first.layer == SPRITE) {
			if(first.isSpriteColorCalc) {
				alpha = Vdp2Regs->CCRLB & 0x1f;
				first.pixel = ColorCalc(linecolor, first.pixel, alpha);
			}
		}
		else {
			//winter heat uses line color screen when it shouldn't, should figure out why
			// alpha = Vdp2Regs->CCRLB & 0x1f;
			// first.pixel = ColorCalc(linecolor, first.pixel, alpha);
		}
	}

	/////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////
	//we also have to check and make sure the rbg is not using line color screen data for panzer dragoon
	else if(colorCalculationEnable && !first.colorCalcWindow && !(first.layer == RBG0 && (Vdp2Regs->KTCTL & (1 << 4))))
		first.pixel = ColorCalc(first.pixel, second.pixel, alpha);
	else
		first.pixel = COLSAT2YAB32(0, first.pixel);

	//is color offset enabled?
	if (Vdp2Regs->CLOFEN & layerInfo->colorOffsetMask)// && first.layer == SPRITE
		//this seems potentially wrong to pair first.pixel with layerinfo-> if the layers got moved around before reaching here
		first.pixel = COLOR_ADD(first.pixel,layerInfo->colorOffset.red,layerInfo->colorOffset.green,layerInfo->colorOffset.blue);

	//we need to check if the layer underneath has shadow enabled
	int shadowEnabled = shadowEnabledCache[second.layer];

	//ok: if the normal or msb shadow has the highest priority
	//the sprite becomes transparent and cuts the brightness in half apparently

	//shadow is processed after color calculation and color offset functions
	//normal shadow is displayed regardless of bit 8 SDCTL
	//puyo tsuu uses msb shadow in menu
	//normal shadow is always enabled, so games should not be using the reserved color value unless they want a shadow
	if(first.layer == SPRITE && first.isNormalShadow)
	{
		if(shadowEnabled)
			first.pixel = Shadow(second.pixel,80);
		//if the shadow isn't enabled for the layer underneath
		//it must be made transparent (not visible), otherwise you get junk pixels on the screen
		//see guardian force title screen, where it disables shadow for rgb0 while casting a shadow on another layer
		else
			first.pixel = second.pixel;
	}
	//msb is only used when the sprite priority is highest apparently
	else if(first.layer == SPRITE && first.isMSBShadow) {

		//puyo tsuu has issues
		//it is not shading correctly

		//MSB shadow and the value of the sprite is greater than 0x8000, we shade the sprite instead of the background
		//sexy parodius corn boss
		if(first.pixel > 0x8000)
			first.pixel = Shadow(first.pixel,80);//&& (Vdp2Regs->SDCTL & (1 << 8)
		else//if (shadowEnabled)
			//this isn't right, taromaru looks correct if you use third.pixel, 20
			//can it skip over a layer and do a shadow?
			//second happens to be that layer of transparent mist,
			//so perhaps it skips color calculated layers?
			first.pixel = Shadow(second.pixel,80);
	}

	vdp2framebuffer[pixaddr] = first.pixel;

	// vdp2framebuffer[pixaddr] = spritebuf[(j*vdp2width)+i].pixel;
	// dst[(j*vdp2width)+i] = nbg0buf[(j*vdp2width)+i].pixel;
	// dst[(j*vdp2width)+i] = spritebuf[(j*vdp2width)+i].pixel;
	// dst[(j*vdp2width)+i] = first.pixel;
}

int pixelindex;
pixeldata blankPixel; 

static void Vdp2DrawBackScreen(void);

//fastcall was used poorly, thats why it was slow.
//but now the FASTCALL macro is gimped in this codebase so I have to make a new one
#ifdef _MSC_VER
#define MSC_SAFE_FASTCALL __fastcall
#else
#define MSC_SAFE_FASTCALL
#endif

typedef void (MSC_SAFE_FASTCALL * TLayerRender)(pixeldata pixelstack[], int &pixelindex, int i, int j, int pixaddr);

void MSC_SAFE_FASTCALL NullLayerRenderer(pixeldata pixelstack[], int &pixelindex, int i, int j, int pixaddr) {}

TLayerRender test;

template<LayerNames LAYER>
void MSC_SAFE_FASTCALL BGLayerRender(pixeldata pixelstack[], int &pixelindex, int i, int j, int pixaddr) {

	//_NULL=0,NBG0=1,NBG1=2,NBG2=3,NBG3=4,SPRITE=5,RBG0=6
	static pixeldata* const layers[] = {0,nbg0buf,nbg1buf,nbg2buf,nbg3buf,spritebuf,rbg0buf};

	pixeldata* const buf = layers[LAYER];

	pixeldata &p = buf[pixaddr];

	//is this necessary? its hard to understand
	if(p.layer == _NULL) return; 

	if(!p.transparent)
	{
		if(testWindow<LAYER>(p, i, j))
			return;
		pixelstack[pixelindex++] = p;
	}
}

INLINE void doSpecialPriority(LayerInfo &layer, int &lsbmask, int pixaddr, int lsbvalue){
	//per screen
	if(layer.specialPriorityFunction == 0) {
	}
	//per tile
	else if(layer.specialPriorityFunction == 1) {
		lsbmask |= (layer.buf[pixaddr].specialFunction&0x2)?lsbvalue:0;
	}
	//per pixel
	else if(layer.specialPriorityFunction == 2) {
		///////////////////////////////////////////
		int temppixel = layer.buf[pixaddr].originalDot & 0xF;

		//see which function code we are using, A or B
		//and then check to see if the lower 4 bits match the designated code or not
		if(!layer.specialFunctionCodeB) {
			//special function code A
			if(isSpecialFunctionCode(Vdp2Regs->SFCODE & 0xFF,temppixel))
				lsbmask |= 1;
		}
		else {
			//special function code B
			if(isSpecialFunctionCode((Vdp2Regs->SFCODE & 0xFF00)>> 8,temppixel))
				lsbmask |= 1;
		}
		////////////////////////////////////////////////
	}
}

bool en_sprites, en_rbg0,en_nbg0,en_nbg1,en_nbg2,en_nbg3;
static void* renderSprites(void*) { 
	//whether to double the sprite layer size depends on vdp1 as well as vdp2
	//in the burning rangers title screen you have low x res vdp1 and high x res vdp2
	if(resxratio==1 && vdp1pixelsize == 1) en_sprites = Vdp2DrawSprites<1>();
	else en_sprites = Vdp2DrawSprites<2>();
	return 0;
}
static void* renderRBG0(void*) { en_rbg0 = Vdp2DrawRBG0(); return 0; }
static void* renderNBG0(void*) { en_nbg0 = Vdp2DrawNBG0(); return 0;  }
static void* renderNBG1(void*) { en_nbg1 = Vdp2DrawNBG1(); return 0;  }
static void* renderNBG2(void*) { en_nbg2 = Vdp2DrawNBG2(); return 0;  }
static void* renderNBG3(void*) { en_nbg3 = Vdp2DrawNBG3(); return 0;  }
static void* renderBackScreen(void*) { Vdp2DrawBackScreen(); return 0;  }

TLayerRender tasks[32][8][8];
struct CompositeTodo
{
	int ystart, yend;
};
static void* doComposite(void* arg) {
	int pixelindex;
	pixeldata pixelstack[0x7];
	const CompositeTodo* const todo = (CompositeTodo* const)arg;
	const int yend = todo->yend;
	const int ystart = todo->ystart;
	int pixaddr = ystart * vdp2width;
	for (int j = ystart; j < yend; j++) {
		for (int i = 0; i < vdp2width; i++,pixaddr++) {

			int lsbmask = 0;

			doSpecialPriority(layerInfoCache[RBG0], lsbmask, pixaddr, 1);
			doSpecialPriority(layerInfoCache[NBG0], lsbmask, pixaddr, 2);
			doSpecialPriority(layerInfoCache[NBG1], lsbmask, pixaddr, 4);
			doSpecialPriority(layerInfoCache[NBG2], lsbmask, pixaddr, 8);
			doSpecialPriority(layerInfoCache[NBG3], lsbmask, pixaddr, 16);

	//		lsbmask |= (nbg0buf[pixaddr].specialFunction&0x2)?2:0;
	//		lsbmask |= (nbg1buf[pixaddr].specialFunction&0x2)?4:0;
	//		lsbmask |= (nbg2buf[pixaddr].specialFunction&0x2)?8:0;
	//		lsbmask |= (nbg3buf[pixaddr].specialFunction&0x2)?16:0;


			//for(int k=0;k<7;k++) pixelstack[k]=&blankPixel;
			pixelstack[1] = blankPixel; pixelstack[2] = blankPixel; 
			pixelindex = 0;

			TLayerRender* todo = &tasks[lsbmask][spritebuf[pixaddr].priority][0];
			todo[0](pixelstack,pixelindex,i,j,pixaddr);
			todo[1](pixelstack,pixelindex,i,j,pixaddr);
			todo[2](pixelstack,pixelindex,i,j,pixaddr);
			todo[3](pixelstack,pixelindex,i,j,pixaddr);
			todo[4](pixelstack,pixelindex,i,j,pixaddr);
			todo[5](pixelstack,pixelindex,i,j,pixaddr);
			todo[6](pixelstack,pixelindex,i,j,pixaddr);

			pixelstack[pixelindex] = backbuf[pixaddr];
			composite(pixelstack, i, j, pixaddr);	
		}
	}

	return 0;
}


//enter here
void VIDSoftVdp2DrawScreens(){

	//buffers are now cleared in the individual layer renderers

	//note - 
	//i have no proof whether it is faster to use a pixelstack of pointers or individual pixels
	//the difference is between 32bits and 64bits.

	//en_sprites = false;

	//bool en_rbg0 = Vdp2DrawRBG0();
	//bool en_nbg0 = Vdp2DrawNBG0();
	//bool en_nbg1 = Vdp2DrawNBG1();
	//bool en_nbg2 = Vdp2DrawNBG2();
	//bool en_nbg3 = Vdp2DrawNBG3();

	//renderRBG0(0);

	en_rbg0 = en_nbg0 = en_nbg1 = en_nbg2 = en_nbg3 = false;
	layerTasks[0].execute(renderSprites,0);
	layerTasks[1].execute(renderRBG0,0);
	layerTasks[2].execute(renderNBG0,0);
	layerTasks[3].execute(renderNBG1,0);
	layerTasks[4].execute(renderNBG2,0);
	layerTasks[5].execute(renderNBG3,0);
	layerTasks[6].execute(renderBackScreen,0);

	for(int i=0;i<7;i++)
		layerTasks[i].finish();
	


	blankPixel.init();

	updateCacheSpriteLayerInfo();
	updateCacheColorTable();

	readLayerInfoByLayer(NBG0, layerInfoCache[NBG0]);
	readLayerInfoByLayer(NBG1, layerInfoCache[NBG1]);
	readLayerInfoByLayer(NBG2, layerInfoCache[NBG2]);
	readLayerInfoByLayer(NBG3, layerInfoCache[NBG3]);
	readLayerInfoByLayer(RBG0, layerInfoCache[RBG0]);
#if 0
	printf("--------\n", Vdp2Regs->CCRNA);
	printf("CTL %x\n", Vdp2Regs->CCCTL);
	printf("A %x\n", Vdp2Regs->CCRNA);
	printf("B %x\n", Vdp2Regs->CCRNB);
	printf("R %x\n", Vdp2Regs->CCRR);
	printf("LB %x\n", Vdp2Regs->CCRLB);
#endif
//	printf("--------\n", Vdp2Regs->RPTA);
//	printf("CTL %x\n", Vdp2Regs->RPMD);
//	printf("A %x\n", Vdp2Regs->CCRNA);
//	printf("B %x\n", Vdp2Regs->CCRNB);
//	printf("R %x\n", Vdp2Regs->CCRR);
//	printf("LB %x\n", Vdp2Regs->CCRLB);

	for(int i=0;i<7;i++) {
		shadowEnabledCache[i] = 0;
		if(i == NBG0) shadowEnabledCache[i] = Vdp2Regs->SDCTL & (1 << 0);
		if(i == NBG1) shadowEnabledCache[i] = Vdp2Regs->SDCTL & (1 << 1);
		if(i == NBG2) shadowEnabledCache[i] = Vdp2Regs->SDCTL & (1 << 2);
		if(i == NBG3) shadowEnabledCache[i] = Vdp2Regs->SDCTL & (1 << 3);
		if(i == RBG0) shadowEnabledCache[i] = Vdp2Regs->SDCTL & (1 << 4);

		colorCountCache[i] = getColorCount((LayerNames)i);
	}

	for(int lsbmask=0;lsbmask<32;lsbmask++)
	{
		int toggle_rbg0 = (lsbmask)&1;
		int toggle_nbg0 = (lsbmask>>1)&1;
		int toggle_nbg1 = (lsbmask>>2)&1;
		int toggle_nbg2 = (lsbmask>>3)&1;
		int toggle_nbg3 = (lsbmask>>4)&1;

		int _rbg0priority = rbg0priority^toggle_rbg0;
		int _nbg0priority = nbg0priority^toggle_nbg0;
		int _nbg1priority = nbg1priority^toggle_nbg1;
		int _nbg2priority = nbg2priority^toggle_nbg2;
		int _nbg3priority = nbg3priority^toggle_nbg3;

		for(int sprpriority = 7; sprpriority >= 0; sprpriority--)
		{
			int ctr=0;
			//layers with 0 priority must not be drawn,
			//the intro of madou monogatari relies on this
			for(int priority = 7; priority > 0; priority--)
			{
				if(en_sprites && priority==sprpriority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<SPRITE>;
				if(en_rbg0 && _rbg0priority == priority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<RBG0>;
				if(en_nbg0 && _nbg0priority == priority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<NBG0>;
				if(en_nbg1 && _nbg1priority == priority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<NBG1>;
				if(en_nbg2 && _nbg2priority == priority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<NBG2>;
				if(en_nbg3 && _nbg3priority == priority) tasks[lsbmask][sprpriority][ctr++] = BGLayerRender<NBG3>;
			}
			for(int i=ctr;i<8;i++)
				tasks[lsbmask][sprpriority][i] = NullLayerRenderer;
		}
	}

	//CompositeTodo todo[3];
	//todo[0].ystart = 0; todo[0].yend = vdp2height/3;
	//todo[1].ystart = todo[0].yend; todo[1].yend = 2*vdp2height/3;
	//todo[2].ystart = todo[1].yend; todo[2].yend = vdp2height;

	//layerTasks[0].execute(doComposite,&todo[0]);
	//layerTasks[1].execute(doComposite,&todo[1]);
	//layerTasks[2].execute(doComposite,&todo[2]);

	//for(int i=0;i<3;i++)
	//	layerTasks[i].finish();

	CompositeTodo todo[4];
	todo[0].ystart = 0; todo[0].yend = vdp2height/4;
	todo[1].ystart = todo[0].yend; todo[1].yend = 2*vdp2height/4;
	todo[2].ystart = todo[1].yend; todo[2].yend = 3*vdp2height/4;
	todo[3].ystart = todo[2].yend; todo[3].yend = vdp2height;

	layerTasks[0].execute(doComposite,&todo[0]);
	layerTasks[1].execute(doComposite,&todo[1]);
	layerTasks[2].execute(doComposite,&todo[2]);
	layerTasks[3].execute(doComposite,&todo[3]);

	for(int i=0;i<4;i++)
		layerTasks[i].finish();

}

static INLINE u32 Vdp2FetchPixel(vdp2draw_struct *info, int x, int y, u32 *color, u32* originalDot)
{
   u32 dot;

   switch(info->colornumber)
   {
      case 0: // 4 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + ((y << info->cellw_bits) + x) / 2) & 0x7FFFF));
         if (!(x & 0x1)) dot >>= 4;
         if (!(dot & 0xF) && info->transparencyenable) return 0;
         else
         {
			 *originalDot = dot;
            *color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | (dot & 0xF)));
            return TRUE;
         }
      case 1: // 8 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + (y << info->cellw_bits) + x) & 0x7FFFF));
         if (!(dot & 0xFF) && info->transparencyenable) return 0;
         else
         {
            *color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | (dot & 0xFF)));
			*originalDot = dot;
            return TRUE;
         }
      case 2: // 16 BPP(palette)
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y << info->cellw_bits) + x) * 2) & 0x7FFFF));
         if ((dot == 0) && info->transparencyenable) return 0;
         else
         {
			 *originalDot = dot;
            *color = Vdp2ColorRamGetColor(info->coloroffset + dot);
            return TRUE;
         }
      case 3: // 16 BPP(RGB)      
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y << info->cellw_bits) + x) * 2) & 0x7FFFF));
         if (!(dot & 0x8000) && info->transparencyenable) return 0;
         else
         {
            *color = COLSAT2YAB16(0, dot);
            return TRUE;
         }
      case 4: // 32 BPP
         dot = T1ReadLong(Vdp2Ram, ((info->charaddr + ((y << info->cellw_bits) + x) * 4) & 0x7FFFF));
         if (!(dot & 0x80000000) && info->transparencyenable) return 0;
         else
         {
            *color = COLSAT2YAB32(0, dot);
            return TRUE;
         }
      default:
         return FALSE;
   }
}

//////////////////////////////////////////////////////////////////////////////

void SetupRotationInfo(vdp2draw_struct *info, vdp2rotationparameterfp_struct *p)
{
   if (info->rotatenum == 0)
   {
      Vdp2ReadRotationTableFP(0, p);
      info->PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
   }
   else
   {
      Vdp2ReadRotationTableFP(1, &p[1]);
      info->PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
   }
}

//////////////////////////////////////////////////////////////////////////////
u32 alphablend(u32 toppixel, u32 bottompixel, u32 alpha);
INLINE pixeldata rotationDrawLoop(fixed32 _cx, fixed32 _cy, bool& aborted, vdp2rotationparameterfp_struct *p, int i, int j, fixed32 xmul, fixed32 ymul, fixed32 C, fixed32 F, screeninfo_struct *sinfo, vdp2draw_struct *info){
	u32 color;

	int x = GenerateRotatedXPosFP(p, i, xmul, ymul, C) & sinfo->xmask;
	int y = GenerateRotatedYPosFP(p, i, xmul, ymul, F) & sinfo->ymask;

	// Convert coordinates into graphics
	if (!info->isbitmap)
	{
		// Tile
		Vdp2MapCalcXY(info, &x, &y, sinfo);
	}

	pixeldata pix;
	pix.init();

	//Fetchpixel
	pix.transparent=!Vdp2FetchPixel(info,x,y,&color,&pix.originalDot);

	//makes no sense
	//if(p->msb)
	//	pix.transparent=true;
	//else if ((Vdp2Regs->RPMD & 0x3) == 2)
	//	pix.transparent=false;

	if(!aborted)
	{
		if(p->msb)
		{
			if ((Vdp2Regs->RPMD & 0x3) == 2) {
				aborted = true;
				return pix;
			}
			else
				pix.transparent=true;
		} 
		else if ((Vdp2Regs->RPMD & 0x3) == 2)
			pix.transparent=false;
	}
	//if there's line color data in the coefficient
	//and the line color screen inserts rbg0
	//not sure how this interacts in other modes,
	//current implementation based on panzer dragoon
	if(p->coefenab && p->linecolordata && (Vdp2Regs->LNCLEN & (1 << 4))) {
		
		int scrAddr = ((Vdp2Regs->LCTA.all & 0x7FFFFUL) * 2);
		int linecoloraddress = T1ReadWord(Vdp2Ram, scrAddr);

		linecoloraddress &= 0x780;//get upper 4 bits

		linecoloraddress = linecoloraddress | p->linecolordata;

		//now we have the color ram address we have to look up the color
		int linecolor = Vdp2ColorRamGetColor(linecoloraddress);

		//and blend it
		color = ColorCalc(linecolor,color,15);
	}

	pix.pixel=color;

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//couldnt this have come from NBG0?
	pix.layer=RBG0;
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	return pix;
}

template<int RESXRATIO>
static void FASTCALL Vdp2DrawRotationFP(vdp2draw_struct *info, vdp2rotationparameterfp_struct *parameter, vdp2draw_struct *infoB, vdp2rotationparameterfp_struct *parameterB)
{
	int i, j, pixaddr;
	int x, y;
	screeninfo_struct sinfo, sinfoB;
	vdp2rotationparameterfp_struct *p=&parameter[info->rotatenum];
	vdp2rotationparameterfp_struct *pB=&parameterB[infoB->rotatenum];
	int doubley = 1;

	if(((Vdp2Regs->TVMD >> 6) & 0x3) == 3)
		doubley = 2;


	SetupRotationInfo(info, parameter);
	if((Vdp2Regs->RPMD & 0x3) == 2)
		SetupRotationInfo(infoB, parameterB);

	if (!p->coefenab)
	{
		fixed32 xmul, ymul, C, F;

		GenerateRotatedVarFP(p, &xmul, &ymul, &C, &F);

		// Do simple rotation
		CalculateRotationValuesFP(p);

		SetupScreenVars(info, &sinfo);

		int basewidth = vdp2width/RESXRATIO;
		for (j = 0, pixaddr=0; j < vdp2height; j++)
		{
			for (i = 0; i < basewidth; i++)
			{
				u32 color;

				x = GenerateRotatedXPosFP(p, i, xmul, ymul, C) & sinfo.xmask;
				y = GenerateRotatedYPosFP(p, i, xmul, ymul, F) & sinfo.ymask;
				xmul += p->deltaXst;

				// Convert coordinates into graphics
				if (!info->isbitmap)
				{
					// Tile
					Vdp2MapCalcXY(info, &x, &y, &sinfo);
				}
				
				for(int q=0;q<RESXRATIO;q++) {
					pixeldata &pix = rbg0buf[pixaddr++];
					pix.init();

					// Fetch pixel
					pix.transparent = !Vdp2FetchPixel(info, x, y, &color, &pix.originalDot);

					pix.pixel = color;
					pix.layer = RBG0;
				}
			}
			ymul += p->deltaYst;
		}

		return;
	}
	else
	{
		fixed32 xmul, ymul, C, F;
		fixed32 xmulB, ymulB, CB, FB;
		fixed32 xmul_save, xmulB_save;
		fixed32 coefx, coefy;
		fixed32 coefxB, coefyB;

		GenerateRotatedVarFP(p, &xmul, &ymul, &C, &F);
		if((Vdp2Regs->RPMD & 0x3) == 2)
			GenerateRotatedVarFP(pB, &xmulB, &ymulB, &CB, &FB);

		// Rotation using Coefficient Tables(now this stuff just gets wacky. It
		// has to be done in software, no exceptions)
		CalculateRotationValuesFP(p);
		if((Vdp2Regs->RPMD & 0x3) == 2)
			CalculateRotationValuesFP(pB);

		SetupScreenVars(info, &sinfo);
		if((Vdp2Regs->RPMD & 0x3) == 2)
			SetupScreenVars(infoB, &sinfoB);
		coefy = 0;
		coefyB = 0;
		int basewidth = vdp2width/2;

		xmul_save = xmul;
		if((Vdp2Regs->RPMD & 0x3) == 2)
			xmulB_save = xmulB;

		bool do_coef_a_x = (p->coefenab && p->deltaKAx != 0);
		bool do_coef_a_y = (p->coefenab && p->deltaKAx == 0);
		bool do_coef_b_x =false;
		bool do_coef_b_y = false;
		if((Vdp2Regs->RPMD & 0x3) == 2) {
			do_coef_b_x= (pB->coefenab && pB->deltaKAx != 0);
			do_coef_b_y = (pB->coefenab && pB->deltaKAx == 0);
		}

		for (int j = 0, pixaddr = 0; j < vdp2height; j++) {

			coefx = coefxB = 0;
			xmul = xmul_save;
			if((Vdp2Regs->RPMD & 0x3) == 2)
				xmulB = xmulB_save;

			//if (do_coef_a_y) Vdp2ReadCoefficientFP(p,p->coeftbladdr + toint(coefy) * p->coefdatasize);
			if (do_coef_b_y) Vdp2ReadCoefficientFP(pB,pB->coeftbladdr + toint(coefyB) * pB->coefdatasize);

			for (int i = 0; i < basewidth; i++) {
				//if (do_coef_a_x)
				Vdp2ReadCoefficientFP(p,p->coeftbladdr + toint((p->deltaKAx*i)+(p->deltaKAst*j)) * p->coefdatasize);
				if (do_coef_b_x) Vdp2ReadCoefficientFP(pB,pB->coeftbladdr + toint(coefyB+coefxB) * pB->coefdatasize);

				bool aborted = false;
				pixeldata pix = rotationDrawLoop(coefx, coefy, aborted, p, i, j, xmul, ymul, C, F, &sinfo, info);
				if(aborted)
					pix = rotationDrawLoop(coefxB, coefyB, aborted, pB, i, j, xmulB, ymulB, CB, FB, &sinfoB, infoB);

				for(int q=0;q<2;q++)
					rbg0buf[pixaddr++] = pix;

				xmul += p->deltaXst;
				if((Vdp2Regs->RPMD & 0x3) == 2)
					xmulB += pB->deltaXst;
				coefx += p->deltaKAx;
				if((Vdp2Regs->RPMD & 0x3) == 2)
					coefxB += pB->deltaKAx;
			}

			ymul += p->deltaYst;
			if((Vdp2Regs->RPMD & 0x3) == 2)
				ymulB += pB->deltaYst;
			coefy += p->deltaKAst;
			if((Vdp2Regs->RPMD & 0x3) == 2)
				coefyB += pB->deltaKAst;
		}
		return;
	}

	// Vdp2DrawScroll(info, RBG0, vdp2width, vdp2height);
}

static void Vdp2DrawBackScreen(void)
{
   int i, pixaddr;
	pixeldata p;
	p.init();
	p.pixel = 0;
	p.layer = BACK;
   // Only draw black if TVMD's DISP and BDCLMD bits are cleared
   if ((Vdp2Regs->TVMD & 0x8000) == 0 && (Vdp2Regs->TVMD & 0x100) == 0)
   {
      // Draw Black
      for (i = 0; i < (vdp2width * vdp2height); i++)
		  backbuf[i] = p;
   }
   else
   {
      // Draw Back Screen
      u32 scrAddr;
      u16 dot;

      if (Vdp2Regs->VRSIZE & 0x8000)
         scrAddr = (((Vdp2Regs->BKTAU & 0x7) << 16) | Vdp2Regs->BKTAL) * 2;
      else
         scrAddr = (((Vdp2Regs->BKTAU & 0x3) << 16) | Vdp2Regs->BKTAL) * 2;

      if (Vdp2Regs->BKTAU & 0x8000)
      {
         // Per Line
         for (i = 0, pixaddr=0; i < vdp2height; i++)
         {
            dot = T1ReadWord(Vdp2Ram, scrAddr);
            scrAddr += 2;

			p.pixel = COLSAT2YAB16(0, dot);

			for (int x = 0; x < vdp2width; x++, pixaddr++)
				backbuf[pixaddr] = p;
         }
      }
      else
      {
         // Single Color
         dot = T1ReadWord(Vdp2Ram, scrAddr);

		 p.pixel =  COLSAT2YAB16(0, dot);

		 const int todo = vdp2width * vdp2height;
		 for (i = 0; i < todo; i++) {
			 backbuf[i] = p;
			}
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static bool Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;
   vdp2rotationparameterfp_struct parameter[2];

   if (Vdp2Regs->BGON & 0x20)
   {
      // RBG1 mode
      info.enable = Vdp2Regs->BGON & 0x20;

      // Read in Parameter B
      Vdp2ReadRotationTableFP(1, &parameter[1]);

      if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode
         ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 2, 0x3);

         info.charaddr = (Vdp2Regs->MPOFR & 0x70) * 0x2000;
         info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 8;
         info.flipfunction = 0;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 4;
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 12);
         ReadPatternData(&info, Vdp2Regs->PNCN0, Vdp2Regs->CHCTLA & 0x1);
      }

      info.rotatenum = 1;
      info.rotatemode = 0;
      info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
   }
   else if (Vdp2Regs->BGON & 0x1)
   {
      // NBG0 mode
      info.enable = Vdp2Regs->BGON & 0x1;

      if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode
         ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 2, 0x3);

         info.x = Vdp2Regs->SCXIN0 & 0x7FF;
         info.y = Vdp2Regs->SCYIN0 & 0x7FF;

         info.charaddr = (Vdp2Regs->MPOFN & 0x7) * 0x20000;
         info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 8;
         info.flipfunction = 0;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 2;

         ReadPlaneSize(&info, Vdp2Regs->PLSZ);

         info.x = Vdp2Regs->SCXIN0 & 0x7FF;
         info.y = Vdp2Regs->SCYIN0 & 0x7FF;
         ReadPatternData(&info, Vdp2Regs->PNCN0, Vdp2Regs->CHCTLA & 0x1);
      }

      info.coordincx = (Vdp2Regs->ZMXN0.all & 0x7FF00) / (float) 65536;
      info.coordincy = (Vdp2Regs->ZMYN0.all & 0x7FF00) / (float) 65536;
      info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG0PlaneAddr;
   }
   else
      // Not enabled
      return false;

   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);
   info.specialprimode = Vdp2Regs->SFPRMD & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x70) >> 4;

   if (Vdp2Regs->CCCTL & 0x1)
      info.alpha = Vdp2Regs->CCRNA & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7) << 8;
   info.priority = nbg0priority;

   if (!(info.enable & Vdp2External.disptoggle))
      return false;

   //memset(nbg0buf,0,sizeof(nbg0buf));

   ReadMosaicData(&info, 0x1);
   ReadLineScrollData(&info, Vdp2Regs->SCRCTL & 0xFF, Vdp2Regs->LSTA0.all);
   if (Vdp2Regs->SCRCTL & 1)
   {
      info.isverticalscroll = 1;
      info.verticalscrolltbl = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
      if (Vdp2Regs->SCRCTL & 0x100)
         info.verticalscrollinc = 8;
      else
         info.verticalscrollinc = 4;
   }
   else
      info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLA;

   if (info.enable == 1)
   {
      // NBG0 draw
      Vdp2DrawScroll(&info, NBG0, vdp2width, vdp2height);
   }
   else
   {
      // RBG1 draw
	   if(resxratio==1)
			Vdp2DrawRotationFP<1>(&info, parameter, &info, parameter);
	   else
		   Vdp2DrawRotationFP<2>(&info, parameter, &info, parameter);
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////////

static bool Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 2) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x200) != 0)
   {
      ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> 10, 0x3);

      info.x = Vdp2Regs->SCXIN1 & 0x7FF;
      info.y = Vdp2Regs->SCYIN1 & 0x7FF;

      info.charaddr = ((Vdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
      info.paladdr = Vdp2Regs->BMPNA & 0x700;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      info.mapwh = 2;

      ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 2);

      info.x = Vdp2Regs->SCXIN1 & 0x7FF;
      info.y = Vdp2Regs->SCYIN1 & 0x7FF;

      ReadPatternData(&info, Vdp2Regs->PNCN1, Vdp2Regs->CHCTLA & 0x100);
   }

   if (Vdp2Regs->CCCTL & 0x2)
      info.alpha = (Vdp2Regs->CCRNA >> 8) & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) << 4;
   info.coordincx = (Vdp2Regs->ZMXN1.all & 0x7FF00) / (float) 65536;
   info.coordincy = (Vdp2Regs->ZMYN1.all & 0x7FF00) / (float) 65536;

   info.priority = nbg1priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG1PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return false;

   //memset(nbg1buf,0,sizeof(nbg1buf));

   ReadMosaicData(&info, 0x2);
   ReadLineScrollData(&info, Vdp2Regs->SCRCTL >> 8, Vdp2Regs->LSTA1.all);
   if (Vdp2Regs->SCRCTL & 0x100)
   {
      info.isverticalscroll = 1;
      if (Vdp2Regs->SCRCTL & 0x1)
      {
         info.verticalscrolltbl = 4 + ((Vdp2Regs->VCSTA.all & 0x7FFFE) << 1);
         info.verticalscrollinc = 8;
      }
      else
      {
         info.verticalscrolltbl = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
         info.verticalscrollinc = 4;
      }
   }
   else
      info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLA >> 8;

   VidSoftVdp2DrawScroll(&info, NBG1);

   return true;
}

//////////////////////////////////////////////////////////////////////////////

static bool Vdp2DrawNBG2(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x4;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x400);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 4) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x2) >> 1;	
   info.mapwh = 2;

   ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 4);
   info.x = Vdp2Regs->SCXN2 & 0x7FF;
   info.y = Vdp2Regs->SCYN2 & 0x7FF;
   ReadPatternData(&info, Vdp2Regs->PNCN2, Vdp2Regs->CHCTLB & 0x1);
    
   if (Vdp2Regs->CCCTL & 0x4)
      info.alpha = Vdp2Regs->CCRNB & 0x1F;

   info.coloroffset = Vdp2Regs->CRAOFA & 0x700;
   info.coordincx = info.coordincy = 1;

   info.priority = nbg2priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG2PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return false;

   //memset(nbg2buf,0,sizeof(nbg2buf));

   ReadMosaicData(&info, 0x4);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLB;
   info.isbitmap = 0;

   Vdp2DrawScroll(&info, NBG2, vdp2width, vdp2height);

    return true;
}

//////////////////////////////////////////////////////////////////////////////

static bool Vdp2DrawNBG3(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x8;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x800);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 6) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x20) >> 5;
	
   info.mapwh = 2;

   ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 6);
   info.x = Vdp2Regs->SCXN3 & 0x7FF;
   info.y = Vdp2Regs->SCYN3 & 0x7FF;
   ReadPatternData(&info, Vdp2Regs->PNCN3, Vdp2Regs->CHCTLB & 0x10);

   if (Vdp2Regs->CCCTL & 0x8)
      info.alpha = (Vdp2Regs->CCRNB >> 8) & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7000) >> 4;
   info.coordincx = info.coordincy = 1;

   info.priority = nbg3priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG3PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle))
      return false;

   //memset(nbg3buf,0,sizeof(nbg3buf));

   ReadMosaicData(&info, 0x8);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLB >> 8;
   info.isbitmap = 0;

   Vdp2DrawScroll(&info, NBG3, vdp2width, vdp2height);

   return true;
}

void generateRotationInfo(vdp2draw_struct &info, vdp2rotationparameterfp_struct parameter[]) {

   info.enable = Vdp2Regs->BGON & 0x10;
   info.priority = rbg0priority;

   info.transparencyenable = !(Vdp2Regs->BGON & 0x1000);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 8) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x7000) >> 12;

   Vdp2ReadRotationTableFP(info.rotatenum, &parameter[info.rotatenum]);

   if((info.isbitmap = Vdp2Regs->CHCTLB & 0x200) != 0)
   {
      // Bitmap Mode
      ReadBitmapSize(&info, Vdp2Regs->CHCTLB >> 10, 0x1);

      if (info.rotatenum == 0)
         // Parameter A
         info.charaddr = (Vdp2Regs->MPOFR & 0x7) * 0x20000;
      else
         // Parameter B
         info.charaddr = (Vdp2Regs->MPOFR & 0x70) * 0x2000;

      info.paladdr = (Vdp2Regs->BMPNB & 0x7) << 8;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      // Tile Mode
      info.mapwh = 4;

      if (info.rotatenum == 0)
         // Parameter A
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 8);
      else
         // Parameter B
         ReadPlaneSize(&info, Vdp2Regs->PLSZ >> 12);

      ReadPatternData(&info, Vdp2Regs->PNCR, Vdp2Regs->CHCTLB & 0x100);
   }

   if (Vdp2Regs->CCCTL & 0x10)
      info.alpha = Vdp2Regs->CCRR & 0x1F;

   info.coloroffset = (Vdp2Regs->CRAOFB & 0x7) << 8;

   info.coordincx = info.coordincy = 1;

   ReadMosaicData(&info, 0x10);
   info.islinescroll = 0;
   info.isverticalscroll = 0;
   info.wctl = Vdp2Regs->WCTLC;
}

//////////////////////////////////////////////////////////////////////////////

static bool Vdp2DrawRBG0(void)
{
   vdp2draw_struct infoA, infoB;
   vdp2rotationparameterfp_struct parameterA[2];
   vdp2rotationparameterfp_struct parameterB[2];

   if (!((Vdp2Regs->BGON & 0x10) & Vdp2External.disptoggle))
      return false;

   // Figure out which Rotation Parameter we're using
   switch (Vdp2Regs->RPMD & 0x3)
   {
      case 0:
         // Parameter A
         infoA.rotatenum = 0;
         infoA.rotatemode = 0;
         infoA.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
         break;
      case 1:
         // Parameter B
         infoA.rotatenum = 1;
         infoA.rotatemode = 0;
         infoA.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;
         break;
      case 2:
         // Parameter A+B switched via coefficients
		  //we are going to do A as normal and switch to B when necessary
         infoA.rotatenum = 0;
         infoA.rotatemode = 0;
         infoA.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;

         infoB.rotatenum = 1;
         infoB.rotatemode = 0;
         infoB.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterBPlaneAddr;

		 generateRotationInfo(infoB, parameterB);
		  break;
      case 3:
         // Parameter A+B switched via rotation parameter window
      default:
         infoA.rotatenum = 0;
         infoA.rotatemode = 1 + (Vdp2Regs->RPMD & 0x1);
         infoA.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2ParameterAPlaneAddr;
         break;
   }

   generateRotationInfo(infoA, parameterA);

   Vdp2DrawRotationFP<2>(&infoA, parameterA, &infoB, parameterB);

   return true;
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftInit(void)
{
   // Initialize output buffer
   if ((dispbuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   // Initialize VDP1 framebuffer 1
   if ((vdp1framebuffer[0] = (u8 *)calloc(sizeof(u8), 0x40000*9)) == NULL)
      return -1;

   // Initialize VDP1 framebuffer 2
   if ((vdp1framebuffer[1] = (u8 *)calloc(sizeof(u8), 0x40000*9)) == NULL)
      return -1;

   // Initialize VDP2 framebuffer
   if ((vdp2framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512*2)) == NULL)
      return -1;


   if ((nbg0framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg1framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg2framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((nbg3framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;
   if ((rbg0framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   vdp1backframebuffer = vdp1framebuffer[0];
   vdp1frontframebuffer = vdp1framebuffer[1];
   vdp2width = 320;
   vdp2height = 224;

#ifdef USE_OPENGL
   YuiSetVideoAttribute(DOUBLEBUFFER, 1);

   if (!YglScreenInit(8, 8, 8, 24))
   {
      if (!YglScreenInit(4, 4, 4, 24))
      {
         if (!YglScreenInit(5, 6, 5, 16))
         {
	    YuiErrorMsg("Couldn't set GL mode\n");
            return -1;
         }
      }
   }

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 320, 224, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-320, 320, -224, 224, 1, 0);
   outputwidth = 320;
   outputheight = 224;
   msglength = 0;
#endif

   VideoInitGlut();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftDeInit(void)
{
   if (dispbuffer)
   {
      free(dispbuffer);
      dispbuffer = NULL;
   }

   if (vdp1framebuffer[0])
      free(vdp1framebuffer[0]);

   if (vdp1framebuffer[1])
      free(vdp1framebuffer[1]);

   if (vdp2framebuffer)
      free(vdp2framebuffer);
}

//////////////////////////////////////////////////////////////////////////////

static int IsFullscreen = 0;

void VIDSoftResize(unsigned int w, unsigned int h, int on)
{
#ifdef USE_OPENGL
   IsFullscreen = on;

   if (on)
      YuiSetVideoMode(w, h, 32, 1);
   else
      YuiSetVideoMode(w, h, 32, 0);

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, h, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-w, w, -h, h, 1, 0);

   glViewport(0, 0, w, h);
   outputwidth = w;
   outputheight = h;
#endif
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftIsFullscreen(void) {
   return IsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp1Reset(void)
{
   vdp1clipxstart = 0;
   vdp1clipxend = 512;
   vdp1clipystart = 0;
   vdp1clipyend = 256;
   
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawStart(void)
{
	switch(Vdp1Regs->TVMR & 7) {
	//Normal
	case 0:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	//Hi resolution
	case 1:
		vdp1width = 1024;
		vdp1height = 256;
		vdp1pixelsize = 1;
		break;
	//Rotation 16
	case 2:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	//Rotation 8
	case 3:
		vdp1width = 512;
		vdp1height = 512;
		vdp1pixelsize = 1;
		break;
	//HDTV
	case 4:
		vdp1width = 512;
		vdp1height = 256;
		vdp1pixelsize = 2;
		break;
	}
	VIDSoftVdp1EraseFrameBuffer();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u16  Vdp1ReadPattern16( u32 base, u32 offset ) {

  u16 dot = T1ReadByte(Vdp1Ram, ( base + (offset>>1)) & 0x7FFFF);
  if ((offset & 0x1) == 0) dot >>= 4; // Even pixel
  else dot &= 0xF; // Odd pixel
  return dot;
}

static INLINE u16  Vdp1ReadPattern64( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0x3F;
}

static INLINE u16  Vdp1ReadPattern128( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0x7F;
}

static INLINE u16  Vdp1ReadPattern256( u32 base, u32 offset ) {

  return T1ReadByte(Vdp1Ram, ( base + offset ) & 0x7FFFF) & 0xFF;
}

static INLINE u16  Vdp1ReadPattern64k( u32 base, u32 offset ) {

  return T1ReadWord(Vdp1Ram, ( base + 2*offset) & 0x7FFFF);
}

////////////////////////////////////////////////////////////////////////////////

INLINE u32 alphablend16(u32 d, u32 s, u32 level)
{
	int r,g,b,sr,sg,sb,dr,dg,db;

	int invlevel = 256-level;
	sr = s & 0x001f; dr = d & 0x001f; 
	r = (sr*level + dr*invlevel)>>8; r&= 0x1f;
	sg = s & 0x03e0; dg = d & 0x03e0;
	g = (sg*level + dg*invlevel)>>8; g&= 0x03e0;
	sb = s & 0x7c00; db = d & 0x7c00;
	b = (sb*level + db*invlevel)>>8; b&= 0x7c00;
	return r|g|b;
}

typedef struct _COLOR_PARAMS
{
	double r,g,b;
} COLOR_PARAMS;

COLOR_PARAMS leftColumnColor;

vdp1cmd_struct cmd;

int currentPixel;
int currentPixelIsVisible;
int characterWidth;
int characterHeight;

int getpixel(int linenumber, int currentlineindex) {
	
	u32 characterAddress;
	u32 colorlut;
	u16 colorbank;
	u8 SPD;
	int endcode;
	int endcodesEnabled;
	int untexturedColor;
	int isTextured = 1;
	int currentShape = cmd.CMDCTRL & 0x7;
	int flip;

	characterAddress = cmd.CMDSRCA << 3;
	colorbank = cmd.CMDCOLR;
	colorlut = (u32)colorbank << 3;
	SPD = ((cmd.CMDPMOD & 0x40) != 0);//show the actual color of transparent pixels if 1 (they won't be drawn transparent)
	endcodesEnabled = (( cmd.CMDPMOD & 0x80) == 0 )?1:0;
	flip = (cmd.CMDCTRL & 0x30) >> 4;

	//4 polygon, 5 polyline or 6 line
	if(currentShape == 4 || currentShape == 5 || currentShape == 6) {
		isTextured = 0;
		untexturedColor = cmd.CMDCOLR;
	}

	switch( flip ) {
		case 1:
			// Horizontal flipping
			currentlineindex = characterWidth - currentlineindex-1;
			break;
		case 2:
			// Vertical flipping
			linenumber = characterHeight - linenumber-1;

			break;
		case 3:
			// Horizontal/Vertical flipping
			linenumber = characterHeight - linenumber-1;
			currentlineindex = characterWidth - currentlineindex-1;
			break;
	}

	switch ((cmd.CMDPMOD >> 3) & 0x7)
	{
		case 0x0: //4bpp bank
			endcode = 0xf;
			currentPixel = Vdp1ReadPattern16( characterAddress + (linenumber*(characterWidth>>1)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0xf;
			break;

		case 0x1://4bpp lut
			endcode = 0xf;
			currentPixel = Vdp1ReadPattern16( characterAddress + (linenumber*(characterWidth>>1)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!(currentPixel == 0 && !SPD))
				currentPixel = T1ReadWord(Vdp1Ram, (currentPixel * 2 + colorlut) & 0x7FFFF);
			currentPixelIsVisible = 0xffff;
			break;
		case 0x2://8pp bank (64 color)
			//is there a hardware bug with endcodes in this color mode?
			//there are white lines around some characters in scud
			//using an endcode of 63 eliminates the white lines
			//but also causes some dropout due to endcodes being triggered that aren't triggered on hardware
			//the closest thing i can do to match the hardware is make all pixels with color index 63 transparent
			//this needs more hardware testing

			endcode = 63;
			currentPixel = Vdp1ReadPattern64( characterAddress + (linenumber*(characterWidth)), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				currentPixel = 0;
		//		return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0x3f;
			break;
		case 0x3://128 color
			endcode = 0xff;
			currentPixel = Vdp1ReadPattern128( characterAddress + (linenumber*characterWidth), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			currentPixelIsVisible = 0x7f;
			break;
		case 0x4://256 color
			endcode = 0xff;
			currentPixel = Vdp1ReadPattern256( characterAddress + (linenumber*characterWidth), currentlineindex );
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			currentPixelIsVisible = 0xff;
			if (!((currentPixel == 0) && !SPD)) 
				currentPixel = colorbank | currentPixel;
			break;
		case 0x5://16bpp bank
			endcode = 0x7fff;
			currentPixel = Vdp1ReadPattern64k( characterAddress + (linenumber*characterWidth*2), currentlineindex );
			//sonic xtreme demonstrates this vdp1 quirk
			//and uses rgb values 0x1525, 0xA0, and 0x2800 as transparent pixels
			if(currentPixel > 0 && currentPixel <= 0x7ffe)
				currentPixel = 0;
			if(isTextured && endcodesEnabled && currentPixel == endcode)
				return 1;
			currentPixelIsVisible = 0xffff;
			break;
	}

	if(!isTextured)
		currentPixel = untexturedColor;

	return 0;
}

int gouraudAdjust( int color, int tableValue )
{
	color += (tableValue - 0x10);

	if ( color < 0 ) color = 0;
	if ( color > 0x1f ) color = 0x1f;

	return color;
}



void putpixel(int x, int y) {

	u16* iPix = &((u16 *)vdp1backframebuffer)[(y * vdp1width) + x];
	int mesh = cmd.CMDPMOD & 0x0100;
	int SPD = ((cmd.CMDPMOD & 0x40) != 0);//show the actual color of transparent pixels if 1 (they won't be drawn transparent)
	int currentShape = cmd.CMDCTRL & 0x7;
	int isTextured=1;

	if(mesh && (x^y)&1)
		return;

	if(currentShape == 4 || currentShape == 5 || currentShape == 6)
		isTextured = 0;

	if (cmd.CMDPMOD & 0x0400) PushUserClipping((cmd.CMDPMOD >> 9) & 0x1);

	if (x >= vdp1clipxstart &&
		x < vdp1clipxend &&
		y >= vdp1clipystart &&
		y < vdp1clipyend)
	{}
	else
		return;

	if (cmd.CMDPMOD & 0x0400) PopUserClipping();

	//see the corn boss in sexy parodius
	//taromaru demonstrates msb shadow without the overlapping
	//if MSB ON is set we just modify the msb of the framebuffer pixel
	if(cmd.CMDPMOD & (1 << 15)) {
		if(isTextured) {
			if (!((currentPixel == 0) && !SPD))
				*(iPix) |= (1 << 15);
		}
		else
			*(iPix) |= (1 << 15);

		return;
	}

	if ( SPD || (currentPixel & currentPixelIsVisible))
	{
		switch( cmd.CMDPMOD & 0x7 )//we want bits 0,1,2
		{
		case 0:	// replace
			if (!((currentPixel == 0) && !SPD))
				*(iPix) = currentPixel;
			break;
		case 1: // shadow, TODO
			*(iPix) = currentPixel;
			break;
		case 2: // half luminance
			*(iPix) = ((currentPixel & ~0x8421) >> 1) | (1 << 15);
			break;
		case 3: // half transparent
			if ( *(iPix) & (1 << 15) )//only if MSB of framebuffer data is set 
				*(iPix) = alphablend16( *(iPix), currentPixel, (1 << 7) ) | (1 << 15);
			else
				*(iPix) = currentPixel;
			break;
		case 4: //gouraud
			#define COLOR(r,g,b)    (((r)&0x1F)|(((g)&0x1F)<<5)|(((b)&0x1F)<<10) |0x8000 )

			//handle the special case demonstrated in the sgl chrome demo
			//if we are in a paletted bank mode and the other two colors are unused, adjust the index value instead of rgb
			if(
				(((cmd.CMDPMOD >> 3) & 0x7) != 5) &&
				(((cmd.CMDPMOD >> 3) & 0x7) != 1) && 
				(int)leftColumnColor.g == 16 && 
				(int)leftColumnColor.b == 16) 
			{
				int c = (int)(leftColumnColor.r-0x10);
				if(c < 0) c = 0;
				currentPixel = currentPixel+c;
				*(iPix) = currentPixel;
				break;
			}
			*(iPix) = COLOR(
				gouraudAdjust(
				currentPixel&0x001F,
				(int)leftColumnColor.r),

				gouraudAdjust(
				(currentPixel&0x03e0) >> 5,
				(int)leftColumnColor.g),

				gouraudAdjust(
				(currentPixel&0x7c00) >> 10,
				(int)leftColumnColor.b)
				);
			break;
		default:
			*(iPix) = alphablend16( COLOR((int)leftColumnColor.r,(int)leftColumnColor.g, (int)leftColumnColor.b), currentPixel, (1 << 7) ) | (1 << 15);
			break;
		}
	}
}

//TODO consolidate the following 3 functions
int bresenham( int x1, int y1, int x2, int y2, int x[], int y[])
{
	int dx, dy, xf, yf, a, b, c, i;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	//buffer protection
	if(dx > 4095 || dy > 4095)
		return INT_MAX;

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {
			x[i] = x1; y[i] = y1;
			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				y1 += yf;
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;
		for (i=0;i<=dy;i++) {
			x[i] = x1; y[i] = y1;
			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				x1 += xf;
			}
		}
		return dy+1;
	}
}

int DrawLine( int x1, int y1, int x2, int y2, double linenumber, double texturestep, double xredstep, double xgreenstep, double xbluestep)
{
	int dx, dy, xf, yf, a, b, c, i;
	int endcodesdetected=0;
	int previousStep = 123456789;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {
			leftColumnColor.r+=xredstep;
			leftColumnColor.g+=xgreenstep;
			leftColumnColor.b+=xbluestep;

			if(getpixel(linenumber,(int)i*texturestep)) {
				if(currentPixel != previousStep) {
					previousStep = (int)i*texturestep;
					endcodesdetected++;
				}
			}
			else
				putpixel(x1,y1);

			previousStep = currentPixel;

			if(endcodesdetected==2)
				break;

			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				getpixel(linenumber,(int)i*texturestep);
				putpixel(x1,y1);
				c += b;
				y1 += yf;
/*
				//same as sega's way, but just move the code down here instead
				//and use the pixel we already have instead of the next one
				if(xf>1&&yf>1) putpixel(x1,y1-1); //case 1
				if(xf<1&&yf<1) putpixel(x1,y1+1); //case 2
				if(xf<1&&yf>1) putpixel(x1+1,y1); //case 7
				if(xf>1&&yf<1) putpixel(x1-1,y1); //case 8*/
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;	
	for (i=0;i<=dy;i++) {
			leftColumnColor.r+=xredstep;
			leftColumnColor.g+=xgreenstep;
			leftColumnColor.b+=xbluestep;

			if(getpixel(linenumber,(int)i*texturestep)) {
				if(currentPixel != previousStep) {
					previousStep = (int)i*texturestep;
					endcodesdetected++;
				}
			}
			else
				putpixel(x1,y1);

			previousStep = currentPixel;

			if(endcodesdetected==2)
				break;

			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				getpixel(linenumber,(int)i*texturestep);
				putpixel(x1,y1);
				c += b;
				x1 += xf;
/*				
				if(xf>1&&yf>1) putpixel(x1,y1-1); //case 3
				if(xf<1&&yf<1) putpixel(x1,y1+1); //case 4
				if(xf<1&&yf>1) putpixel(x1+1,y1); //case 5
				if(xf>1&&yf<1) putpixel(x1-1,y1); //case 6*/

			}
		}
		return dy+1;
	}
}

int getlinelength(int x1, int y1, int x2, int y2) {
	int dx, dy, xf, yf, a, b, c, i;

	if (x2>x1) {
		dx = x2-x1;
		xf = 1;
	}
	else {
		dx = x1-x2;
		xf = -1;
	}

	if (y2>y1) {
		dy = y2-y1;
		yf = 1;
	}
	else {
		dy = y1-y2;
		yf = -1;
	}

	if (dx>dy) {
		a = dy+dy;
		c = a-dx;
		b = c-dx;
		for (i=0;i<=dx;i++) {

			x1 += xf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				y1 += yf;
			}
		}
		return dx+1;
	}
	else {
		a = dx+dx;
		c = a-dy;
		b = c-dy;	
	for (i=0;i<=dy;i++) {
			y1 += yf;
			if (c<0) {
				c += a;
			}
			else {
				c += b;
				x1 += xf;
			}
		}
		return dy+1;
	}
}

INLINE double interpolate(double start, double end, int numberofsteps) {

	double stepvalue = 0;

	if(numberofsteps == 0)
		return 1;

	stepvalue = (end - start) / numberofsteps;

	return stepvalue;
}

typedef union _COLOR { // xbgr x555
	struct {
#ifdef WORDS_BIGENDIAN
	u16 x:1;
	u16 b:5;
	u16 g:5;
	u16 r:5;
#else
     u16 r:5;
     u16 g:5;
     u16 b:5;
     u16 x:1;
#endif
	};
	u16 value;
} COLOR;


COLOR gouraudA;
COLOR gouraudB;
COLOR gouraudC;
COLOR gouraudD;

void gouraudTable()
{
	int gouraudTableAddress;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	gouraudTableAddress = (((unsigned int)cmd.CMDGRDA) << 3);

	gouraudA.value = T1ReadWord(Vdp1Ram,gouraudTableAddress);
	gouraudB.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+2);
	gouraudC.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+4);
	gouraudD.value = T1ReadWord(Vdp1Ram,gouraudTableAddress+6);
}

int xleft[4096];
int yleft[4096];
int xright[4096];
int yright[4096];

//a real vdp1 draws with arbitrary lines
//this is why endcodes are possible
//this is also the reason why half-transparent shading causes moire patterns
//and the reason why gouraud shading can be applied to a single line draw command
void drawQuad(s32 tl_x, s32 tl_y, s32 bl_x, s32 bl_y, s32 tr_x, s32 tr_y, s32 br_x, s32 br_y){

	int totalleft;
	int totalright;
	int total;
	int i;

	COLOR_PARAMS topLeftToBottomLeftColorStep, topRightToBottomRightColorStep;
		
	//how quickly we step through the line arrays
	double leftLineStep = 1;
	double rightLineStep = 1; 

	//a lookup table for the gouraud colors
	COLOR colors[4];

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
	characterWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	characterHeight = cmd.CMDSIZE & 0xFF;

	totalleft  = bresenham(tl_x,tl_y,bl_x,bl_y,xleft,yleft);
	totalright = bresenham(tr_x,tr_y,br_x,br_y,xright,yright);

	//just for now since burning rangers will freeze up trying to draw huge shapes
	if(totalleft == INT_MAX || totalright == INT_MAX)
		return;

	total = totalleft > totalright ? totalleft : totalright;


	if(cmd.CMDPMOD & (1 << 2)) {

		gouraudTable();

		{ colors[0] = gouraudA; colors[1] = gouraudD; colors[2] = gouraudB; colors[3] = gouraudC; }

		topLeftToBottomLeftColorStep.r = interpolate(colors[0].r,colors[1].r,total);
		topLeftToBottomLeftColorStep.g = interpolate(colors[0].g,colors[1].g,total);
		topLeftToBottomLeftColorStep.b = interpolate(colors[0].b,colors[1].b,total);

		topRightToBottomRightColorStep.r = interpolate(colors[2].r,colors[3].r,total);
		topRightToBottomRightColorStep.g = interpolate(colors[2].g,colors[3].g,total);
		topRightToBottomRightColorStep.b = interpolate(colors[2].b,colors[3].b,total);
	}

	//we have to step the equivalent of less than one pixel on the shorter side
	//to make sure textures stretch properly and the shape is correct
	if(total == totalleft && totalleft != totalright) {
		//left side is larger
		leftLineStep = 1;
		rightLineStep = (double)totalright / totalleft;
	}
	else if(totalleft != totalright){
		//right side is larger
		rightLineStep = 1;
		leftLineStep = (double)totalleft / totalright;
	}

	for(i = 0; i < total; i++) {

		int xlinelength;

		double xtexturestep;
		double ytexturestep;

		COLOR_PARAMS rightColumnColor;

		COLOR_PARAMS leftToRightStep = {0,0,0};

		//get the length of the line we are about to draw
		xlinelength = getlinelength(
			xleft[(int)(i*leftLineStep)],
			yleft[(int)(i*leftLineStep)],
			xright[(int)(i*rightLineStep)],
			yright[(int)(i*rightLineStep)]);

		//so from 0 to the width of the texture / the length of the line is how far we need to step
		xtexturestep=interpolate(0,characterWidth,xlinelength);

		//now we need to interpolate the y texture coordinate across multiple lines
		ytexturestep=interpolate(0,characterHeight,total);

		//gouraud interpolation
		if(cmd.CMDPMOD & (1 << 2)) {

			//for each new line we need to step once more through each column
			//and add the orignal color + the number of steps taken times the step value to the bottom of the shape
			//to get the current colors to use to interpolate across the line

			leftColumnColor.r = colors[0].r +(topLeftToBottomLeftColorStep.r*i);
			leftColumnColor.g = colors[0].g +(topLeftToBottomLeftColorStep.g*i);
			leftColumnColor.b = colors[0].b +(topLeftToBottomLeftColorStep.b*i);

			rightColumnColor.r = colors[2].r +(topRightToBottomRightColorStep.r*i);
			rightColumnColor.g = colors[2].g +(topRightToBottomRightColorStep.g*i);
			rightColumnColor.b = colors[2].b +(topRightToBottomRightColorStep.b*i);

			//interpolate colors across to get the right step values
			leftToRightStep.r = interpolate(leftColumnColor.r,rightColumnColor.r,xlinelength);
			leftToRightStep.g = interpolate(leftColumnColor.g,rightColumnColor.g,xlinelength);
			leftToRightStep.b = interpolate(leftColumnColor.b,rightColumnColor.b,xlinelength);
		}

		DrawLine(
			xleft[(int)(i*leftLineStep)],
			yleft[(int)(i*leftLineStep)],
			xright[(int)(i*rightLineStep)],
			yright[(int)(i*rightLineStep)],
			ytexturestep*i, 
			xtexturestep,
			leftToRightStep.r,
			leftToRightStep.g,
			leftToRightStep.b
			);
	}
}

void VIDSoftVdp1NormalSpriteDraw() {

	s16 topLeftx,topLefty,topRightx,topRighty,bottomRightx,bottomRighty,bottomLeftx,bottomLefty;
	int spriteWidth;
	int spriteHeight;
	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	topLeftx = cmd.CMDXA + Vdp1Regs->localX;
	topLefty = cmd.CMDYA + Vdp1Regs->localY;
	spriteWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	spriteHeight = cmd.CMDSIZE & 0xFF;

	topRightx = topLeftx+spriteWidth;
	topRighty = topLefty;
	bottomRightx = topLeftx+spriteWidth;
	bottomRighty = topLefty+spriteHeight;
	bottomLeftx = topLeftx;
	bottomLefty = topLefty+spriteHeight;

	drawQuad(topLeftx,topLefty,bottomLeftx,bottomLefty,topRightx,topRighty,bottomRightx,bottomRighty);
}

void VIDSoftVdp1ScaledSpriteDraw(){

	s32 topLeftx,topLefty,topRightx,topRighty,bottomRightx,bottomRighty,bottomLeftx,bottomLefty;
	int spriteWidth;
	int spriteHeight;
	int flip = 0;
	int x0,y0,x1,y1;
	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	x0 = cmd.CMDXA + Vdp1Regs->localX;
	y0 = cmd.CMDYA + Vdp1Regs->localY;

	switch ((cmd.CMDCTRL >> 8) & 0xF)
	{
	case 0x0: // Only two coordinates
	//TODO you can set 1-5, and it will display
	default:
		x1 = ((int)cmd.CMDXC) - x0 + Vdp1Regs->localX + 1;
		y1 = ((int)cmd.CMDYC) - y0 + Vdp1Regs->localY + 1;
		break;
	case 0x5: // Upper-left
		x1 = ((int)cmd.CMDXB) + 1;
		y1 = ((int)cmd.CMDYB) + 1;
		break;
	case 0x6: // Upper-Center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		x1++;
		y1++;
		break;
	case 0x7: // Upper-Right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		x1++;
		y1++;
		break;
	////////////////////////////////
	//you can set 8, and it will still display
	//this is an invalid setting though
	//this matches my hardware tests
	case 0x8:
		x1 = ((int)cmd.CMDXA);
		y1 = ((int)cmd.CMDYB);
		y0 = y0 - y1/2;
		x0 = 0 + Vdp1Regs->localX;
		x1++;
		y1++;
		break;
	///////////////////////////////
	case 0x9: // Center-left
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xA: // Center-center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xB: // Center-right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		y0 = y0 - y1/2;
		x1++;
		y1++;
		break;
	case 0xD: // Lower-left
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	case 0xE: // Lower-center
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1/2;
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	case 0xF: // Lower-right
		x1 = ((int)cmd.CMDXB);
		y1 = ((int)cmd.CMDYB);
		x0 = x0 - x1;
		y0 = y0 - y1;
		x1++;
		y1++;
		break;
	}

	spriteWidth = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
	spriteHeight = cmd.CMDSIZE & 0xFF;

	topLeftx = x0;
	topLefty = y0;

	topRightx = x1+x0;
	topRighty = topLefty;

	bottomRightx = x1+x0;
	bottomRighty = y1+y0;

	bottomLeftx = topLeftx;
	bottomLefty = y1+y0;

	drawQuad(topLeftx,topLefty,bottomLeftx,bottomLefty,topRightx,topRighty,bottomRightx,bottomRighty);
}
//the upper bits of the CMD__ parts are designated as code extension
//i would expect to need to ignore all of them but guardian heroes needs to draw sprites at -1840
//which is outside the -1024 to 1023 range suggested by the manual
//ignoring them all would be (((s16)((X)<<5))/32) but guardian heroes breaks so we will leave an additional bit
//burning rangers wants to draw polygons at 32769 without this
//more research should be done
#define TRUNC_10BIT(X) (((s16)((X)<<4))/16) //watch out for right shift arithmetic
void VIDSoftVdp1DistortedSpriteDraw() {

	s32 xa,ya,xb,yb,xc,yc,xd,yd;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    xa = (s32)(TRUNC_10BIT(cmd.CMDXA) + Vdp1Regs->localX);
	ya = (s32)(TRUNC_10BIT(cmd.CMDYA) + Vdp1Regs->localY);

    xb = (s32)(TRUNC_10BIT(cmd.CMDXB) + Vdp1Regs->localX);
	yb = (s32)(TRUNC_10BIT(cmd.CMDYB) + Vdp1Regs->localY);

    xc = (s32)(TRUNC_10BIT(cmd.CMDXC) + Vdp1Regs->localX);
	yc = (s32)(TRUNC_10BIT(cmd.CMDYC) + Vdp1Regs->localY);

    xd = (s32)(TRUNC_10BIT(cmd.CMDXD) + Vdp1Regs->localX);
	yd = (s32)(TRUNC_10BIT(cmd.CMDYD) + Vdp1Regs->localY);

	drawQuad(xa,ya,xd,yd,xb,yb,xc,yc);
}

void gouraudLineSetup(double * redstep, double * greenstep, double * bluestep, int length, COLOR table1, COLOR table2) {

	gouraudTable();

	*redstep =interpolate(table1.r,table2.r,length);
	*greenstep =interpolate(table1.g,table2.g,length);
	*bluestep =interpolate(table1.b,table2.b,length);

	leftColumnColor.r = table1.r;
	leftColumnColor.g = table1.g;
	leftColumnColor.b = table1.b;
}

void VIDSoftVdp1PolylineDraw(void)
{
	int X[4];
	int Y[4];
	double redstep = 0, greenstep = 0, bluestep = 0;
	int length;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	X[0] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
	Y[0] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
	X[1] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
	Y[1] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));
	X[2] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14));
	Y[2] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16));
	X[3] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18));
	Y[3] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A));

	length = getlinelength(X[0], Y[0], X[1], Y[1]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudA, gouraudB);
	DrawLine(X[0], Y[0], X[1], Y[1], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[1], Y[1], X[2], Y[2]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudB, gouraudC);
	DrawLine(X[1], Y[1], X[2], Y[2], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[2], Y[2], X[3], Y[3]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudD, gouraudC);
	DrawLine(X[3], Y[3], X[2], Y[2], 0,0,redstep,greenstep,bluestep);

	length = getlinelength(X[3], Y[3], X[0], Y[0]);
	gouraudLineSetup(&redstep,&greenstep,&bluestep,length, gouraudA,gouraudD);
	DrawLine(X[0], Y[0], X[3], Y[3], 0,0,redstep,greenstep,bluestep);
}

void VIDSoftVdp1LineDraw(void)
{
	int x1, y1, x2, y2;
	double redstep = 0, greenstep = 0, bluestep = 0;
	int length;

	Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

	x1 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
	y1 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
	x2 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
	y2 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));

	length = getlinelength(x1, y1, x2, y2);
	gouraudLineSetup(&redstep,&bluestep,&greenstep,length, gouraudA, gouraudB);
	DrawLine(x1, y1, x2, y2, 0,0,redstep,greenstep,bluestep);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1UserClipping(void)
{
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

#if 0
   vdp1clipxstart = Vdp1Regs->userclipX1;
   vdp1clipxend = Vdp1Regs->userclipX2;
   vdp1clipystart = Vdp1Regs->userclipY1;
   vdp1clipyend = Vdp1Regs->userclipY2;

   // This needs work
   if (vdp1clipxstart > Vdp1Regs->systemclipX1)
      vdp1clipxstart = Vdp1Regs->userclipX1;
   else
      vdp1clipxstart = Vdp1Regs->systemclipX1;

   if (vdp1clipxend < Vdp1Regs->systemclipX2)
      vdp1clipxend = Vdp1Regs->userclipX2;
   else
      vdp1clipxend = Vdp1Regs->systemclipX2;

   if (vdp1clipystart > Vdp1Regs->systemclipY1)
      vdp1clipystart = Vdp1Regs->userclipY1;
   else
      vdp1clipystart = Vdp1Regs->systemclipY1;

   if (vdp1clipyend < Vdp1Regs->systemclipY2)
      vdp1clipyend = Vdp1Regs->userclipY2;
   else
      vdp1clipyend = Vdp1Regs->systemclipY2;
#endif
}

//////////////////////////////////////////////////////////////////////////////

static void PushUserClipping(int mode)
{
   if (mode == 1)
   {
      VDP1LOG("User clipping mode 1 not implemented\n");
      return;
   }

   vdp1clipxstart = Vdp1Regs->userclipX1;
   vdp1clipxend = Vdp1Regs->userclipX2;
   vdp1clipystart = Vdp1Regs->userclipY1;
   vdp1clipyend = Vdp1Regs->userclipY2;

   // This needs work
   if (vdp1clipxstart > Vdp1Regs->systemclipX1)
      vdp1clipxstart = Vdp1Regs->userclipX1;
   else
      vdp1clipxstart = Vdp1Regs->systemclipX1;

   if (vdp1clipxend < Vdp1Regs->systemclipX2)
      vdp1clipxend = Vdp1Regs->userclipX2;
   else
      vdp1clipxend = Vdp1Regs->systemclipX2;

   if (vdp1clipystart > Vdp1Regs->systemclipY1)
      vdp1clipystart = Vdp1Regs->userclipY1;
   else
      vdp1clipystart = Vdp1Regs->systemclipY1;

   if (vdp1clipyend < Vdp1Regs->systemclipY2)
      vdp1clipyend = Vdp1Regs->userclipY2;
   else
      vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

static void PopUserClipping(void)
{
   vdp1clipxstart = Vdp1Regs->systemclipX1;
   vdp1clipxend = Vdp1Regs->systemclipX2;
   vdp1clipystart = Vdp1Regs->systemclipY1;
   vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1SystemClipping(void)
{
   Vdp1Regs->systemclipX1 = 0;
   Vdp1Regs->systemclipY1 = 0;
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

   vdp1clipxstart = Vdp1Regs->systemclipX1;
   vdp1clipxend = Vdp1Regs->systemclipX2;
   vdp1clipystart = Vdp1Regs->systemclipY1;
   vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1LocalCoordinate(void)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2DrawStart(void)
{
}
u32 alphablend(u32 toppixel, u32 bottompixel, u32 alpha)
{

   u8 oldr, oldg, oldb;
   u8 r, g, b;
   u32 oldpixel = bottompixel;

   static int topratio[32] = {
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
   };
   static int bottomratio[32] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
   };

   if(alpha > 31) {
	   alpha = 0;
	//   printf("alpha out of bounds, fix me");
   }

   // separate color components for top and second pixel
   r = (toppixel & 0xFF) * topratio[alpha] >> 5;
   g = ((toppixel >> 8) & 0xFF) * topratio[alpha] >> 5;
   b = ((toppixel >> 16) & 0xFF) * topratio[alpha] >> 5;

#ifdef WORDS_BIGENDIAN
   oldr = ((oldpixel >> 24) & 0xFF) * bottomratio[alpha] >> 5;
   oldg = ((oldpixel >> 16) & 0xFF) * bottomratio[alpha] >> 5;
   oldb = ((oldpixel >> 8) & 0xFF) * bottomratio[alpha] >> 5;
#else
   oldr = (oldpixel & 0xFF) * bottomratio[alpha] >> 5;
   oldg = ((oldpixel >> 8) & 0xFF) * bottomratio[alpha] >> 5;
   oldb = ((oldpixel >> 16) & 0xFF) * bottomratio[alpha] >> 5;
#endif

   // add color components and reform the pixel
   toppixel = ((b + oldb) << 16) | ((g + oldg) << 8) | (r + oldr);

   return toppixel;
}

u32 additive(u32 toppixel, u32 bottompixel)
{

	u16 oldr, oldg, oldb;
	u16 r, g, b;

	r = (toppixel & 0xFF);
	g = ((toppixel >> 8) & 0xFF);
	b = ((toppixel >> 16) & 0xFF);

#ifdef WORDS_BIGENDIAN
	oldr = ((bottompixel >> 24) & 0xFF);
	oldg = ((bottompixel >> 16) & 0xFF);
	oldb = ((bottompixel >> 8) & 0xFF);
#else
	oldr = (bottompixel & 0xFF);
	oldg = ((bottompixel >> 8) & 0xFF);
	oldb = ((bottompixel >> 16) & 0xFF);
#endif

	r = r + oldr;
	g = g + oldg;
	b = b + oldb;

	if(b > 255) b = 255;
	if(g > 255) g = 255;
	if(r > 255) r = 255;

	toppixel = (b << 16) | (g << 8) | r;

	return toppixel;
}

u32 ColorCalc(u32 toppixel, u32 bottompixel, u32 alpha) {
	if(Vdp2Regs->CCCTL & (1 << 8))
		return additive(toppixel, bottompixel);
	else
		return alphablend(toppixel,bottompixel,alpha);	
}

void VIDSoftVdp2DrawEnd(void)
{
	int i;

     // Render VDP2 only
      for (i = 0; i < (vdp2width*vdp2height); i++)
		   dispbuffer[i] = COLSATSTRIPPRIORITY(vdp2framebuffer[i]);

   VIDSoftVdp1SwapFrameBuffer();

#ifdef USE_OPENGL
   glRasterPos2i(0, 0);
   glPixelZoom((float)outputwidth / (float)vdp2width, 0 - ((float)outputheight / (float)vdp2height));
   glDrawPixels(vdp2width, vdp2height, GL_RGBA, GL_UNSIGNED_BYTE, dispbuffer);

#if HAVE_LIBGLUT
   if (msglength > 0) {
	   int LeftX=9;
	   int Width=500;
	   int TxtY=11;
	   int Height=13;

	 glBegin(GL_POLYGON);
        glColor3f(0, 0, 0);
        glVertex2i(LeftX, TxtY);
        glVertex2i(LeftX + Width, TxtY);
        glVertex2i(LeftX + Width, TxtY + Height);
        glVertex2i(LeftX, TxtY + Height);
    glEnd();

      glColor3f(1.0f, 1.0f, 1.0f);
      glRasterPos2i(10, 22);
      for (i = 0; i < msglength; i++) {
         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, message[i]);
      }
      glColor3f(1, 1, 1);
      msglength = 0;
   }
#endif

#endif
   YuiSwapBuffers();
}



void VIDSoftVdp2SetResolution(u16 TVMD)
{
   // This needs some work

   // Horizontal Resolution
   switch (TVMD & 0x7)
   {
      case 0://320
         vdp2width = 640;
         resxratio=2;
         break;
      case 1://352
         vdp2width = 704;
         resxratio=2;
         break;
      case 2: // 640
         vdp2width = 640;
         resxratio=1;
         break;
      case 3: // 704
         vdp2width = 704;
         resxratio=1;
         break;
      case 4://320
         vdp2width = 640;
         resxratio=2;
         break;
      case 5://352
         vdp2width = 704;
         resxratio=2;
         break;
      case 6: // 640
         vdp2width = 640;
         resxratio=1;
         break;
      case 7: // 704
         vdp2width = 704;
         resxratio=1;
         break;
   }

   resxratio_shift = resxratio-1;

   // Vertical Resolution
   switch ((TVMD >> 4) & 0x3)
   {
      case 0:
         vdp2height = 224;
         break;
      case 1:
         vdp2height = 240;
         break;
      case 2:
         vdp2height = 256;
         break;
      default: break;
   }
   resyratio=1;

   // Check for interlace
   switch ((TVMD >> 6) & 0x3)
   {
      case 3: // Double-density Interlace
         vdp2height *= 2;
         resyratio=2;
         break;
      case 2: // Single-density Interlace
      case 0: // Non-interlace
      default: break;
   }

   resyratio_shift = resyratio-1;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority)
{
   nbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority)
{
   nbg1priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority)
{
   nbg2priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority)
{
   nbg3priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority)
{
   rbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftOnScreenDebugMessage(char *string, ...)
{
   va_list arglist;

   va_start(arglist, string);
   vsprintf(message, string, arglist);
   va_end(arglist);
   msglength = strlen(message);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftGetScreenSize(int *width, int *height)
{
   *width = vdp2width;
   *height = vdp2height;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1SwapFrameBuffer(void)
{
   u8 *temp = vdp1frontframebuffer;
   vdp1frontframebuffer = vdp1backframebuffer;
   vdp1backframebuffer = temp;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1EraseFrameBuffer(void)
{   
   int i,i2;
   int w,h;
   int interlaced = 1;

   h = (Vdp1Regs->EWRR & 0x1FF) + 1;

   //doubled if we are double interlaced
   if(((Vdp2Regs->TVMD >> 6) & 0x3) == 3) {
	   h*=2;
	   interlaced = 2;
   }

   if (h > vdp1height*interlaced) h = vdp1height*interlaced;
   w = ((Vdp1Regs->EWRR >> 6) & 0x3F8) + 8;

   //width is in 16 pixel units for hi resolution, rotation 8
   if(vdp1pixelsize == 1)
	   w*=2;
   
   if (w > vdp1width) w = vdp1width;

   for (i2 = (Vdp1Regs->EWLR & 0x1FF); i2 < h; i2++)
   {
      for (i = ((Vdp1Regs->EWLR >> 6) & 0x1F8); i < w; i++)
         ((u16 *)vdp1backframebuffer)[(i2 * vdp1width) + i] = Vdp1Regs->EWDR;
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftGetGlSize(int *width, int *height)
{
#ifdef USE_OPENGL
   *width = outputwidth;
   *height = outputheight;
#else
   *width = vdp2width;
   *height = vdp2height;
#endif
}