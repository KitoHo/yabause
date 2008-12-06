/*  Copyright 2004 Guillaume Duhamel
    Copyright 2004-2008 Theo Berkau
    Copyright 2005 Joost Peters

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
#include <windows.h>
#include <commctrl.h>
#include <GL/gl.h>
#undef FASTCALL
#include "../cs2.h"
#include "../vdp2.h"
#include "../yui.h"
#include "snddx.h"
#include "../vidogl.h"
#include "../vidsoft.h"
#include "../peripheral.h"
#include "perdx.h"
#include "../cs0.h"
#include "resource.h"
#include "settings.h"
#include "cd.h"
#include "../debug.h"
#include "cheats.h"
#include "../m68kcore.h"
#ifndef _MSC_VER
#include "../m68kc68k.h"
#endif
//#include "../m68khle.h"
#include "yuidebug.h"
#include "hexedit.h"

#define DONT_PROFILE
#include "../profile.h"

HANDLE emuthread=INVALID_HANDLE_VALUE;
int KillEmuThread=0;
int stop=1;
int stopped=1;
int paused=0;
int yabwinw;
int yabwinh;
int screenwidth;
int screenheight;

HINSTANCE y_hInstance;
HWND YabWin=NULL;
HMENU YabMenu=NULL;
HDC YabHDC=NULL;
HGLRC YabHRC=NULL;
BOOL isfullscreenset=FALSE;
int yabwinx = 0;
int yabwiny = 0;

static int redsize = 0;
static int greensize = 0;
static int bluesize = 0;
static int depthsize = 0;

char yssfilename[MAX_PATH] = "\0";
char ysspath[MAX_PATH] = "\0";
char netlinksetting[80];
char bmpfilename[MAX_PATH] = "\0";

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
void YuiReleaseVideo(void);

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
&PERDIRECTX,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&SPTICD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDDIRECTX,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDOGL,
&VIDSoft,
NULL
};

M68K_struct *M68KCoreList[] = {
&M68KDummy,
#ifndef _MSC_VER
&M68KC68K,
#endif
//&M68KHLE,
NULL
};

//////////////////////////////////////////////////////////////////////////////

void YuiSetBiosFilename(const char *bios)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetIsoFilename(const char *iso) {
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetCdromFilename(const char * cdromfilename)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetSoundEnable(int enablesound)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiHideShow(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiQuit(void)
{
        stop = 1;
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetVideoAttribute(int type, int val)
{
   switch (type)
   {
      case RED_SIZE:
      {
         redsize = val;
         break;
      }
      case GREEN_SIZE:
      {
         greensize = val;
         break;
      }
      case BLUE_SIZE:
      {
         bluesize = val;
         break;
      }
      case DEPTH_SIZE:
      {
         depthsize = val;
         break;
      }
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen)
{
   PIXELFORMATDESCRIPTOR pfd;
   DWORD style=0;
   DWORD exstyle=0;
   RECT rect;

   if (!isfullscreenset && fullscreen)
   {
      GetWindowRect(YabWin, &rect);
      yabwinx = rect.left;
      yabwiny = rect.top;
   }

   // Make sure any previously setup variables are released
   YuiReleaseVideo();

   if (fullscreen)
   {
       DEVMODE dmSettings;
       LONG ret;

       memset(&dmSettings, 0, sizeof(dmSettings));
       dmSettings.dmSize = sizeof(dmSettings);
       dmSettings.dmPelsWidth = width;
       dmSettings.dmPelsHeight = height;
       dmSettings.dmBitsPerPel = bpp;
       dmSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

       if ((ret = ChangeDisplaySettings(&dmSettings,CDS_FULLSCREEN)) != DISP_CHANGE_SUCCESSFUL)
       {
          // revert back to windowed mode
          ChangeDisplaySettings(NULL,0);
          ShowCursor(TRUE);
          fullscreen = FALSE;
          SetMenu(YabWin, YabMenu);
       }
       else
       {
          // Adjust window styles
          style = WS_POPUP;
          exstyle = WS_EX_APPWINDOW;
          SetMenu(YabWin, NULL);
          ShowCursor(FALSE);
       }
   }
   else
   {
       // Adjust window styles
       style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX;
       exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
       SetMenu(YabWin, YabMenu);
   }

   SetWindowLong(YabWin, GWL_STYLE, style);
   SetWindowLong(YabWin, GWL_EXSTYLE, exstyle);

   rect.left = 0;
   rect.right = width;
   rect.top = 0;
   rect.bottom = height;
   AdjustWindowRectEx(&rect, style, FALSE, exstyle);

   if (!fullscreen)
   {
      rect.right = rect.left + width + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
      rect.bottom = rect.top + height + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);  
   }

   // Get the Device Context for our window
   if ((YabHDC = GetDC(YabWin)) == NULL)
   {
      YuiReleaseVideo();
      return -1;
   }

   // Let's setup the Pixel format for the context
   memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = bpp;
   pfd.cRedBits = redsize;
   pfd.cGreenBits = greensize;
   pfd.cBlueBits = bluesize;
   pfd.cAlphaBits = 0;
   pfd.cAccumRedBits = 0;
   pfd.cAccumGreenBits = 0;
   pfd.cAccumBlueBits = 0;
   pfd.cAccumAlphaBits = 0;
   pfd.cAccumBits = pfd.cAccumRedBits + pfd.cAccumGreenBits +
                    pfd.cAccumBlueBits + pfd.cAccumAlphaBits;
   pfd.cDepthBits = depthsize;
   pfd.cStencilBits = 0;

   SetPixelFormat(YabHDC, ChoosePixelFormat(YabHDC, &pfd), &pfd);

   if ((YabHRC = wglCreateContext(YabHDC)) == NULL)
   {
      YuiReleaseVideo();
      return -1;
   }

   if(wglMakeCurrent(YabHDC,YabHRC) == FALSE)
   {
      YuiReleaseVideo();
      return -1;
   }

   ShowWindow(YabWin,SW_SHOW);
   SetForegroundWindow(YabWin);
   SetFocus(YabWin);

   if (fullscreen)
      SetWindowPos(YabWin, HWND_TOP, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOCOPYBITS | SWP_SHOWWINDOW);
   else
      SetWindowPos(YabWin, HWND_TOP, yabwinx, yabwiny, rect.right-rect.left, rect.bottom-rect.top, SWP_NOCOPYBITS | SWP_SHOWWINDOW);

   isfullscreenset = fullscreen;

   screenwidth = width;
   screenheight = height;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YuiReleaseVideo(void)
{
   if (isfullscreenset)
   {
      ChangeDisplaySettings(NULL,0);
      ShowCursor(TRUE);
   }

   if (YabHRC)
   {
       wglMakeCurrent(NULL,NULL);
       wglDeleteContext(YabHRC);
       YabHRC = NULL;
   }

   if (YabHDC)
   {
      ReleaseDC(YabWin,YabHDC);
      YabHDC = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////

void YuiSwapBuffers()
{
   SwapBuffers(YabHDC);
}

//////////////////////////////////////////////////////////////////////////////

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen)
{
}

//////////////////////////////////////////////////////////////////////////////

int YuiCaptureScreen(const char *filename)
{
   u8 *buf;
   FILE *fp;
   BITMAPFILEHEADER bmf;
   BITMAPINFOHEADER bmi;
   int totalsize=screenwidth * screenheight * sizeof(u32);
   int i;

   if ((fp = fopen(filename, "wb")) == NULL)
   {
      // error
      return -1;
   }

   if ((buf = (u8 *)malloc(totalsize)) == NULL)
   {
      // error
      fclose(fp);
      return -2;
   }

   SwapBuffers(YabHDC);
   glReadBuffer(GL_BACK);
   glReadPixels(0, 0, screenwidth, screenheight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   SwapBuffers(YabHDC);

   for (i = 0; i < (screenwidth * screenheight); i++)
   {
      u8 temp;

      temp = buf[i * 4];
      buf[i * 4] = buf[(i * 4) + 2];
      buf[(i * 4) + 2] = temp;
   }

   // Setup BMP header
   ZeroMemory(&bmf, sizeof(bmf));
   bmf.bfType = 'B' | ('M' << 8);
   bmf.bfSize = sizeof(bmf);
   bmf.bfOffBits = sizeof(bmf) + sizeof(bmi);

   ZeroMemory(&bmi, sizeof(bmi));

   bmi.biSize = sizeof(bmi);
   bmi.biWidth = screenwidth;
   bmi.biHeight = screenheight;
   bmi.biPlanes = 1;
   bmi.biBitCount = 32;
   bmi.biCompression = 0; // None
   bmi.biSizeImage = bmi.biWidth * bmi.biHeight * sizeof(u32);

   fwrite((void *)&bmf, 1, sizeof(bmf), fp);
   fwrite((void *)&bmi, 1, sizeof(bmi), fp);
   fwrite((void *)buf, 1, totalsize, fp);
   fclose(fp);
   free(buf);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YuiPause()
{
#ifdef USETHREADS
   stop = 1;
   while (!stopped) { Sleep(0); }
   ScspMuteAudio();
   paused = 1;
#endif
}

//////////////////////////////////////////////////////////////////////////////

void YuiUnPause()
{
#ifdef USETHREADS
   if (paused)
   {
      ScspUnMuteAudio();
      stop = 0;
      paused = 0;
   }
#endif
}

//////////////////////////////////////////////////////////////////////////////

void YuiTempPause()
{
#ifdef USETHREADS
   if (!paused)
   {
      stop = 1;
      while (!stopped) { Sleep(0); }
      ScspMuteAudio();
   }
#else
   ScspMuteAudio();
#endif
}

//////////////////////////////////////////////////////////////////////////////

void YuiTempUnPause()
{
#ifdef USETHREADS
   if (!paused)
   {
      ScspUnMuteAudio();
      stop = 0;
   }
#else
   ScspUnMuteAudio();
#endif
}


//////////////////////////////////////////////////////////////////////////////
#ifdef USETHREADS
DWORD WINAPI YabauseEmulate(LPVOID arg)
{
   yabauseinit_struct yinit;
   int ret;

//YabauseSetup:
   memset(&yinit, 0, sizeof(yabauseinit_struct));
   yinit.percoretype = percoretype;
   yinit.sh2coretype = sh2coretype;
   yinit.vidcoretype = vidcoretype;
   yinit.sndcoretype = sndcoretype;
   if (IsPathCdrom(cdrompath))
      yinit.cdcoretype = CDCORE_SPTI;
   else
      yinit.cdcoretype = CDCORE_ISO;
   //yinit.m68kcoretype = M68KCORE_HLE;
#ifdef _MSC_VER
   yinit.m68kcoretype = M68KCORE_DEFAULT;
#else
   yinit.m68kcoretype = M68KCORE_C68K;
#endif
   yinit.carttype = carttype;
   yinit.regionid = regionid;
   if (strcmp(biosfilename, "") == 0)
      yinit.biospath = NULL;
   else
      yinit.biospath = biosfilename;
   yinit.cdpath = cdrompath;
   yinit.buppath = backupramfilename;
   yinit.mpegpath = mpegromfilename;
   yinit.cartpath = cartfilename;
   yinit.netlinksetting = netlinksetting;
   yinit.flags = VIDEOFORMATTYPE_NTSC;

   if ((ret = YabauseInit(&yinit)) < 0)
   {
/*
      if (ret == -2)
      {
         nocorechange = 1;

         if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), NULL, (DLGPROC)SettingsDlgProc) != TRUE)
         {
            // exit program with error
            MessageBox (NULL, "yabause.ini must be properly setup before program can be used.", "Error",  MB_OK | MB_ICONINFORMATION);
            return -1;
         }

         YuiReleaseVideo();
         YabauseDeInit();

         goto YabauseSetup;
      }
*/
      return -1;
   }

   if (usefullscreenonstartup)
      VIDCore->Resize(fullscreenwidth, fullscreenheight, 1);
   else if (usecustomwindowsize)
      VIDCore->Resize(windowwidth, windowheight, 0);

   PERDXLoadDevices(inifilename);

   stop = 0;   

   ScspSetVolume(sndvolume);

   if (enableautofskip)
      EnableAutoFrameSkip();
   else
      DisableAutoFrameSkip();

//   nocorechange = 1;

   while(!KillEmuThread)
   {
      while (!stop)
      {
         stopped = 0;

         if (PERCore->HandleEvents() != 0)
         {
            YuiReleaseVideo();
            if (YabMenu)
               DestroyMenu(YabMenu);
            return -1;
         }
         Sleep(0);
      }

      if (changecore)
      {
         // Update cores
         if (changecore & 0x1)
         {
            if (IsPathCdrom(cdrompath))
               Cs2ChangeCDCore(CDCORE_SPTI, cdrompath);
            else
               Cs2ChangeCDCore(CDCORE_ISO, cdrompath);
            MessageBox (NULL, "Changed cd core", "Notice",  MB_OK | MB_ICONINFORMATION);
         }
         else if (changecore & 0x2)
         {
            VideoChangeCore(vidcoretype);
            MessageBox (NULL, "Changed video core", "Notice",  MB_OK | MB_ICONINFORMATION);
         }
         else if (changecore & 0x4)
         {
            ScspChangeSoundCore(sndcoretype);
            MessageBox (NULL, "Changed sound core", "Notice",  MB_OK | MB_ICONINFORMATION);
         }

         changecore = 0;
         corechanged = 1;
      }

      stopped = 1;

      Sleep(300);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////

int YuiInit(void)
{
   MSG                         msg;
   DWORD inifilenamesize=0;
   char *pinifilename;
   char tempstr[MAX_PATH];
   yabauseinit_struct yinit;
   HACCEL hAccel;
   static char szAppName[128];
   WNDCLASS MyWndClass;
   int ret;
   int ip[4];
   INITCOMMONCONTROLSEX iccs;

   memset(&iccs, 0, sizeof(INITCOMMONCONTROLSEX));
   iccs.dwSize = sizeof(INITCOMMONCONTROLSEX);
   iccs.dwICC = ICC_INTERNET_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
   InitCommonControlsEx(&iccs);

   InitHexEdit();

   y_hInstance = GetModuleHandle(NULL);

   // get program pathname
   inifilenamesize = GetModuleFileName(y_hInstance, inifilename, MAX_PATH);

   // set pointer to start of extension

   if ((pinifilename = strrchr(inifilename, '.')))
      // replace .exe with .ini
      sprintf(pinifilename, ".ini");

   if (GetPrivateProfileString("General", "CDROMDrive", "", cdrompath, MAX_PATH, inifilename) == 0)
   {
      nocorechange = 1;

      // Startup Settings Configuration
      if (DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), NULL, (DLGPROC)SettingsDlgProc) != TRUE)
      {
         // exit program with error
         MessageBox (NULL, "yabause.ini must be properly setup before program can be used.", "Error",  MB_OK | MB_ICONINFORMATION);
         return -1;
      }
   }

   GetPrivateProfileString("General", "BiosPath", "", biosfilename, MAX_PATH, inifilename);
   GetPrivateProfileString("General", "BackupRamPath", "bkram.bin", backupramfilename, MAX_PATH, inifilename);
   GetPrivateProfileString("General", "MpegRomPath", "", mpegromfilename, MAX_PATH, inifilename);
   GetPrivateProfileString("General", "StatePath", "", ysspath, MAX_PATH, inifilename);
   if (strcmp(ysspath, "") == 0)
      GetCurrentDirectory(MAX_PATH, ysspath);

   GetPrivateProfileString("General", "CartType", "", tempstr, MAX_PATH, inifilename);

   // figure out cart type here, grab cartfilename if necessary
   carttype = atoi(tempstr);
   
   switch (carttype)
   {
      case CART_PAR:
      case CART_BACKUPRAM4MBIT:
      case CART_BACKUPRAM8MBIT:
      case CART_BACKUPRAM16MBIT:
      case CART_BACKUPRAM32MBIT:
      case CART_ROM16MBIT:
         GetPrivateProfileString("General", "CartPath", "", cartfilename, MAX_PATH, inifilename);
         break;
      default: break;
   }

   // Grab Bios Language Settings
//   GetPrivateProfileString("General", "BiosLanguage", "", tempstr, MAX_PATH, inifilename);

   // Grab SH2 Core Settings
   GetPrivateProfileString("General", "SH2Core", "", tempstr, MAX_PATH, inifilename);
   sh2coretype = atoi(tempstr);

   // Grab Region Settings
   GetPrivateProfileString("General", "Region", "", tempstr, MAX_PATH, inifilename);

   if (strlen(tempstr) == 1)
   {
      switch (tempstr[0])
      {
         case 'J':
            regionid = 1;
            break;
         case 'T':
            regionid = 2;
            break;
         case 'U':
            regionid = 4;
            break;
         case 'B':
            regionid = 5;
            break;
         case 'K':
            regionid = 6;
            break;
         case 'A':
            regionid = 0xA;
            break;
         case 'E':
            regionid = 0xC;
            break;
         case 'L':
            regionid = 0xD;
            break;
         default: break;
      }
   }
   else if (stricmp(tempstr, "AUTO") == 0)
      regionid = 0;

   // Grab Video Core Settings
   GetPrivateProfileString("Video", "VideoCore", "-1", tempstr, MAX_PATH, inifilename);
   vidcoretype = atoi(tempstr);
   if (vidcoretype == -1)
      vidcoretype = VIDCORE_OGL;

   // Grab Auto Frameskip Settings
   GetPrivateProfileString("Video", "AutoFrameSkip", "0", tempstr, MAX_PATH, inifilename);
   enableautofskip = atoi(tempstr);

   // Grab Full Screen Settings
   GetPrivateProfileString("Video", "UseFullScreenOnStartup", "0", tempstr, MAX_PATH, inifilename);
   usefullscreenonstartup = atoi(tempstr);

   GetPrivateProfileString("Video", "FullScreenWidth", "640", tempstr, MAX_PATH, inifilename);
   fullscreenwidth = atoi(tempstr);

   GetPrivateProfileString("Video", "FullScreenHeight", "480", tempstr, MAX_PATH, inifilename);
   fullscreenheight = atoi(tempstr);

   // Grab Window Settings
   GetPrivateProfileString("Video", "UseCustomWindowSize", "0", tempstr, MAX_PATH, inifilename);
   usecustomwindowsize = atoi(tempstr);

   GetPrivateProfileString("Video", "WindowWidth", "320", tempstr, MAX_PATH, inifilename);
   windowwidth = atoi(tempstr);

   GetPrivateProfileString("Video", "WindowHeight", "224", tempstr, MAX_PATH, inifilename);
   windowheight = atoi(tempstr);

   // Grab Sound Core Settings
   GetPrivateProfileString("Sound", "SoundCore", "-1", tempstr, MAX_PATH, inifilename);
   sndcoretype = atoi(tempstr);

   if (sndcoretype == -1)
      sndcoretype = SNDCORE_DIRECTX;

   // Grab Volume Settings
   GetPrivateProfileString("Sound", "Volume", "100", tempstr, MAX_PATH, inifilename);
   sndvolume = atoi(tempstr);

   GetPrivateProfileString("General", "CartType", "", tempstr, MAX_PATH, inifilename);

   // Grab Netlink Settings
   GetPrivateProfileString("Netlink", "LocalRemoteIP", "127.0.0.1", tempstr, MAX_PATH, inifilename);
   sscanf(tempstr, "%d.%d.%d.%d", ip, ip+1, ip+2, ip+3);
   netlinklocalremoteip = (DWORD)MAKEIPADDRESS(ip[0], ip[1], ip[2], ip[3]);

   GetPrivateProfileString("Netlink", "Port", "7845", tempstr, MAX_PATH, inifilename);
   netlinkport = atoi(tempstr);

   sprintf(netlinksetting, "%d.%d.%d.%d\n%d", (int)FIRST_IPADDRESS(netlinklocalremoteip), (int)SECOND_IPADDRESS(netlinklocalremoteip), (int)THIRD_IPADDRESS(netlinklocalremoteip), (int)FOURTH_IPADDRESS(netlinklocalremoteip), netlinkport);

#if DEBUG
   // Grab Logging settings
   GetPrivateProfileString("Log", "Enable", "0", tempstr, MAX_PATH, inifilename);
   uselog = atoi(tempstr);

   GetPrivateProfileString("Log", "Type", "0", tempstr, MAX_PATH, inifilename);
   logtype = atoi(tempstr);

   GetPrivateProfileString("Log", "Filename", "", logfilename, MAX_PATH, inifilename);

   if (uselog)
   {
      switch (logtype)
      {
         case 0: // Log to file
            MainLog = DebugInit("main", DEBUG_STREAM, logfilename);
            break;
         case 1: // Log to Window
         {
            RECT rect;

            if ((logbuffer = (char *)malloc(logsize)) == NULL)
               break;
            LogWin = CreateDialog(y_hInstance,
                                  MAKEINTRESOURCE(IDD_LOG),
                                  NULL,
                                  (DLGPROC)LogDlgProc);
            GetWindowRect(LogWin, &rect);
            GetPrivateProfileString("Log", "WindowX", "0", tempstr, MAX_PATH, inifilename);
            ret = atoi(tempstr);
            GetPrivateProfileString("Log", "WindowY", "0", tempstr, MAX_PATH, inifilename);
            SetWindowPos(LogWin, HWND_TOP, ret, atoi(tempstr), rect.right-rect.left, rect.bottom-rect.top, SWP_NOCOPYBITS | SWP_SHOWWINDOW);
            MainLog = DebugInit("main", DEBUG_CALLBACK, (char *)&UpdateLogCallback);
            break;
         }
         default: break;
      }
   }
#endif

   // Get Window Position(if saved)
   GetPrivateProfileString("General", "WindowX", "0", tempstr, MAX_PATH, inifilename);
   yabwinx = atoi(tempstr);
   GetPrivateProfileString("General", "WindowY", "0", tempstr, MAX_PATH, inifilename);
   yabwiny = atoi(tempstr);

   // Figure out how much of the screen is useable
   if (usecustomwindowsize)
   {
      // Since we can't retrieve it, use default values
      yabwinw = windowwidth + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
      yabwinh = windowheight + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);
   }
   else
   {
      yabwinw = 320 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
      yabwinh = 224 + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);
   }

   hAccel = LoadAccelerators(y_hInstance, MAKEINTRESOURCE(IDR_MAIN_ACCEL));

   // Set up and register window class
   MyWndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
   MyWndClass.lpfnWndProc = (WNDPROC) WindowProc;
   MyWndClass.cbClsExtra = 0;
   MyWndClass.cbWndExtra = sizeof(DWORD);
   MyWndClass.hInstance = y_hInstance;
   MyWndClass.hIcon = LoadIcon(y_hInstance, MAKEINTRESOURCE(IDI_ICON));
   MyWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
   MyWndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
   MyWndClass.lpszClassName = "Yabause";
   MyWndClass.lpszMenuName = NULL;

   YabMenu = LoadMenu(y_hInstance, MAKEINTRESOURCE(IDR_MENU));

   if (!RegisterClass(&MyWndClass))
      return -1;

   sprintf(szAppName, "Yabause %s", VERSION);

   // Create new window
   YabWin = CreateWindow("Yabause",            // class
                         szAppName,            // caption
                         WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |                                        
                         WS_THICKFRAME | WS_MINIMIZEBOX |   // style
                         WS_CLIPCHILDREN,
                         yabwinx,              // x pos
                         yabwiny,              // y pos
                         yabwinw,              // width
                         yabwinh,              // height
                         HWND_DESKTOP,         // parent window
                         NULL,                 // menu
                         y_hInstance,          // instance
                         NULL);                // parms


#ifndef USETHREADS
YabauseSetup:
   memset(&yinit, 0, sizeof(yabauseinit_struct));
   yinit.percoretype = percoretype;
   yinit.sh2coretype = sh2coretype;
   yinit.vidcoretype = vidcoretype;
   yinit.sndcoretype = sndcoretype;
   if (IsPathCdrom(cdrompath))
      yinit.cdcoretype = CDCORE_SPTI;
   else
      yinit.cdcoretype = CDCORE_ISO;
   //yinit.m68kcoretype = M68KCORE_HLE;
#ifdef _MSC_VER
   yinit.m68kcoretype = M68KCORE_DEFAULT;
#else
   yinit.m68kcoretype = M68KCORE_C68K;
#endif
   yinit.carttype = carttype;
   yinit.regionid = regionid;
   if (strcmp(biosfilename, "") == 0)
      yinit.biospath = NULL;
   else
      yinit.biospath = biosfilename;
   yinit.cdpath = cdrompath;
   yinit.buppath = backupramfilename;
   yinit.mpegpath = mpegromfilename;
   yinit.cartpath = cartfilename;
   yinit.netlinksetting = netlinksetting;
   yinit.flags = VIDEOFORMATTYPE_NTSC;

   if ((ret = YabauseInit(&yinit)) < 0)
   {
      if (ret == -2)
      {
         nocorechange = 1;

         if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), NULL, (DLGPROC)SettingsDlgProc) != TRUE)
         {
            // exit program with error
            MessageBox (NULL, "yabause.ini must be properly setup before program can be used.", "Error",  MB_OK | MB_ICONINFORMATION);
            return -1;
         }

         YuiReleaseVideo();
         YabauseDeInit();

         goto YabauseSetup;
      }
      return -1;
   }

   if (usefullscreenonstartup)
      VIDCore->Resize(fullscreenwidth, fullscreenheight, 1);
   else if (usecustomwindowsize)
      VIDCore->Resize(windowwidth, windowheight, 0);

   PERDXLoadDevices(inifilename);

   stop = 0;

   ScspSetVolume(sndvolume);

   if (enableautofskip)
      EnableAutoFrameSkip();
   else
      DisableAutoFrameSkip();


   while (!stop)
   {
      if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
      {
         if (TranslateAccelerator(YabWin, hAccel, &msg) == 0)
         {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         }
      }

      if (PERCore->HandleEvents() != 0)
      {
         YuiReleaseVideo();
         if (YabMenu)
            DestroyMenu(YabMenu);
         return -1;
      }
   }

#else

   KillEmuThread=0;
   emuthread = CreateThread(NULL, 0, YabauseEmulate, &KillEmuThread, 0, NULL);

   while (GetMessage(&msg,NULL,0,0))
   {
      if (TranslateAccelerator(YabWin, hAccel, &msg) == 0)
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   if (emuthread != INVALID_HANDLE_VALUE)
   {
      // If the playback thread is going
      KillEmuThread=1;     // Set the flag telling it to stop
      if (WaitForSingleObject(emuthread,INFINITE) == WAIT_TIMEOUT)
      {
         // Couldn't close thread cleanly
         TerminateThread(emuthread,0);
      }
      CloseHandle(emuthread);
      emuthread = INVALID_HANDLE_VALUE;
   }
#endif

   YuiReleaseVideo();
   if (YabMenu)
      DestroyMenu(YabMenu);

   sprintf(tempstr, "%d", yabwinx);
   WritePrivateProfileString("General", "WindowX", tempstr, inifilename);
   sprintf(tempstr, "%d", yabwiny);
   WritePrivateProfileString("General", "WindowY", tempstr, inifilename);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDM_MEMTRANSFER:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_MEMTRANSFER), hWnd, (DLGPROC)MemTransferDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_RUN:
            {
               YuiUnPause();
               EnableMenuItem(YabMenu, IDM_RUN, MF_GRAYED);
               EnableMenuItem(YabMenu, IDM_PAUSE, MF_ENABLED);
               break;
            }
            case IDM_PAUSE:
            {
               YuiPause();
               EnableMenuItem(YabMenu, IDM_PAUSE, MF_GRAYED);
               EnableMenuItem(YabMenu, IDM_RUN, MF_ENABLED);
               break;
            }
            case IDM_RESET:
            {
               YuiTempPause();
               YabauseReset();
               YuiTempUnPause();
               break;
            }
            case IDM_CHEATSEARCH:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_CHEATSEARCH), hWnd, (DLGPROC)CheatSearchDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_CHEATLIST:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_CHEATLIST), hWnd, (DLGPROC)CheatListDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_SETTINGS:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, (DLGPROC)SettingsDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_BACKUPRAMMANAGER:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_BACKUPRAM), hWnd, (DLGPROC)BackupRamDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_MSH2DEBUG:
            {
               YuiTempPause();
               debugsh = MSH2;
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SH2DEBUG), hWnd, (DLGPROC)SH2DebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_SSH2DEBUG:
            {
               YuiTempPause();
               debugsh = SSH2;
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SH2DEBUG), hWnd, (DLGPROC)SH2DebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_VDP1DEBUG:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_VDP1DEBUG), hWnd, (DLGPROC)VDP1DebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_VDP2DEBUG:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_VDP2DEBUG), hWnd, (DLGPROC)VDP2DebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_M68KDEBUG:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_M68KDEBUG), hWnd, (DLGPROC)M68KDebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_SCUDSPDEBUG:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SCUDSPDEBUG), hWnd, (DLGPROC)SCUDSPDebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_SCSPDEBUG:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_SCSPDEBUG), hWnd, (DLGPROC)SCSPDebugDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_MEMORYEDITOR:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_MEMORYEDITOR), hWnd, (DLGPROC)MemoryEditorDlgProc);
               YuiTempUnPause();
               break;
            }
            case IDM_TOGGLEFULLSCREEN:
            {
               // Normally I should be using the function provided in vdp2.c,
               // but it doesn't support odd custom resolutions.
               if (isfullscreenset)
                  VIDCore->Resize(windowwidth, windowheight, 0);
               else
                  VIDCore->Resize(fullscreenwidth, fullscreenheight, 1);

               break;
            }
            case IDM_TOGGLENBG0:
            {
               ToggleNBG0();
               break;
            }
            case IDM_TOGGLENBG1:
            {
               ToggleNBG1();
               break;
            }
            case IDM_TOGGLENBG2:
            {
               ToggleNBG2();
               break;
            }
            case IDM_TOGGLENBG3:
            {
               ToggleNBG3();
               break;
            }
            case IDM_TOGGLERBG0:
            {
               ToggleRBG0();
               break;
            }
            case IDM_TOGGLEVDP1:
            {
               ToggleVDP1();
               break;
            }
            case IDM_TOGGLEFPS:
            {
               ToggleFPS();
               break;
            }
            case IDM_SAVESTATEAS:
            {
               OPENFILENAME ofn;

               SetupOFN(&ofn, OFN_DEFAULTSAVE, hWnd,
                        "Yabause Save State files\0*.YSS\0All Files\0*.*\0",
                        yssfilename, sizeof(yssfilename));

               if (GetSaveFileName(&ofn))
               {
                  if (YabSaveState(yssfilename) != 0)
                     MessageBox (hWnd, "Couldn't save state file", "Error",  MB_OK | MB_ICONINFORMATION);
               }
               break;
            }
            case IDM_LOADSTATEAS:
            {
               OPENFILENAME ofn;

               SetupOFN(&ofn, OFN_DEFAULTLOAD, hWnd,
                        "Yabause Save State files\0*.YSS\0All Files\0*.*\0",
                        yssfilename, sizeof(yssfilename));

               if (GetOpenFileName(&ofn))
               {
                  if (YabLoadState(yssfilename) != 0)
                     MessageBox (hWnd, "Couldn't load state file", "Error",  MB_OK | MB_ICONINFORMATION);
               }

               break;
            }
            case IDM_SAVESTATE_F2:
            case IDM_SAVESTATE_F3:
            case IDM_SAVESTATE_F4:
            case IDM_SAVESTATE_F5:
            case IDM_SAVESTATE_F6:
            case IDM_SAVESTATE_F7:
            case IDM_SAVESTATE_F8:
            case IDM_SAVESTATE_F9:
            case IDM_SAVESTATE_F10:
               if (YabSaveStateSlot(ysspath, LOWORD(wParam)-IDM_SAVESTATE_F2) != 0)
                  MessageBox (hWnd, "Couldn't load state file", "Error",  MB_OK | MB_ICONINFORMATION);
               break;
            case IDM_LOADSTATE_F2:
            case IDM_LOADSTATE_F3:
            case IDM_LOADSTATE_F4:
            case IDM_LOADSTATE_F5:
            case IDM_LOADSTATE_F6:
            case IDM_LOADSTATE_F7:
            case IDM_LOADSTATE_F8:
            case IDM_LOADSTATE_F9:
            case IDM_LOADSTATE_F10:
               if (YabLoadStateSlot(ysspath, LOWORD(wParam)-IDM_LOADSTATE_F2) != 0)
                  MessageBox (hWnd, "Couldn't load state file", "Error",  MB_OK | MB_ICONINFORMATION);
               break;
            case IDM_CAPTURESCREEN:
            {
               OPENFILENAME ofn;
               
               YuiTempPause();
               SetupOFN(&ofn, OFN_DEFAULTSAVE, hWnd,
                       "All Files\0*.*\0Bitmap Files\0*.BMP\0",
                       bmpfilename, sizeof(bmpfilename));

               if (GetSaveFileName(&ofn))
               {
                  if (YuiCaptureScreen(bmpfilename))
                     MessageBox (hWnd, "Couldn't save capture file", "Error",  MB_OK | MB_ICONINFORMATION);
               }
               YuiTempUnPause();
               break;
            }
            case IDM_EXIT:
            {
               ScspMuteAudio();
               PostMessage(hWnd, WM_CLOSE, 0, 0);
               break;
            }
            case IDM_WEBSITE:
            {
               ShellExecute(NULL, "open", "http://yabause.sourceforge.net", NULL, NULL, SW_SHOWNORMAL);
               break;
            }
            case IDM_FORUM:
            {
               ShellExecute(NULL, "open", "http://yabause.sourceforge.net/forums/", NULL, NULL, SW_SHOWNORMAL);
               break;
            }
            case IDM_SUBMITBUGREPORT:
            {
               ShellExecute(NULL, "open", "http://sourceforge.net/tracker/?func=add&group_id=89991&atid=592126", NULL, NULL, SW_SHOWNORMAL);
               break;
            }
            case IDM_DONATE:
            {
               ShellExecute(NULL, "open", "https://sourceforge.net/donate/index.php?group_id=89991", NULL, NULL, SW_SHOWNORMAL);
               break;
            }
            case IDM_COMPATLIST:
            {
               ShellExecute(NULL, "open", "http://www.emu-compatibility.com/yabause/index.php?lang=uk", NULL, NULL, SW_SHOWNORMAL);
               break;
            }
            case IDM_ABOUT:
            {
               YuiTempPause();
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)AboutDlgProc);
               YuiTempUnPause();
               break;
            }
         }

         return 0L;
      }
      case WM_KEYDOWN:
      {
         if(wParam == VK_OEM_3) // ~ key
            SpeedThrottleEnable();

         return 0L;
      }
      case WM_KEYUP:
      {
         if(wParam == VK_OEM_3) // ~ key
             SpeedThrottleDisable();
         return 0L;
      }
#ifndef USETHREADS
      case WM_ENTERMENULOOP:
      {
         ScspMuteAudio();
         return 0L;
      }
      case WM_EXITMENULOOP:
      {
         ScspUnMuteAudio();
         return 0L;
      }
#endif
      case WM_MOVE:
      {
         RECT rect;

         GetWindowRect(hWnd, &rect);
         yabwinx = rect.left;
         yabwiny = rect.top;
         return 0L;
      }
      case WM_CLOSE:
      {
         stop = 1;
         PostQuitMessage(0);
         return 0L;
      }
      case WM_SIZE:
      {
         return 0L;
      }
      case WM_PAINT:
      {
         PAINTSTRUCT ps;

         BeginPaint(hWnd, &ps);
         EndPaint(hWnd, &ps);
         return 0L;
      }
      case WM_DESTROY:
         return 0L;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                              LPARAM lParam)
{
   char tempstr[256];

   switch (uMsg)
   {
      case WM_INITDIALOG:
         sprintf(tempstr, "Yabause v%s", VERSION);
         SetDlgItemText(hDlg, IDC_VERSIONTEXT, tempstr);
         return TRUE;
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               EndDialog(hDlg, TRUE);

               return TRUE;
            }
            default: break;
         }
         break;
      }
      case WM_CLOSE:
      {
         EndDialog(hDlg, TRUE);

         return TRUE;
      }
      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
   if (YuiInit() != 0)
      fprintf(stderr, "Error running Yabause\n");

   YabauseDeInit();
   PROFILE_PRINT();
#if DEBUG
   LogStop();
   if (LogWin)
   {
      RECT rect;
      char text[10];

      // Remember log window position
      GetWindowRect(LogWin, &rect);
      sprintf(text, "%ld", rect.left);
      WritePrivateProfileString("Log", "WindowX", text, inifilename);
      sprintf(text, "%ld", rect.top);
      WritePrivateProfileString("Log", "WindowY", text, inifilename);

      DestroyWindow(LogWin);
   }
#endif
   return 0;
}

//////////////////////////////////////////////////////////////////////////////