/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon
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

#include "gtkglwidget.h"
#include "yuiwindow.h"

#include "../yabause.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../vidogl.h"
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../persdljoy.h"
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "pergtk.h"

#include "settings.h"

#ifdef USEM68KCORE
M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_C68K
&M68KC68K,
#endif
NULL
};
#endif

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
&PERGTK,
#ifdef HAVE_LIBSDL
&PERSDLJoy,
#endif
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
#ifdef HAVE_LIBGL
&VIDOGL,
#endif
&VIDSoft,
NULL
};

GtkWidget * yui;
GKeyFile * keyfile;
yabauseinit_struct yinit;

const char * key_names[] = { "Up", "Right", "Down", "Left", "Right trigger", "Left trigger",
	"Start", "A", "B", "C", "X", "Y", "Z", NULL };

int yui_main(gpointer data) {
	PERCore->HandleEvents();
	return TRUE;
}

GtkWidget * yui_new() {
#ifdef USENEWPERINTERFACE
	yui = yui_window_new(NULL, G_CALLBACK(YabauseInit), &yinit, yui_main, G_CALLBACK(YabauseReset));
#else
	yui = yui_window_new(NULL, G_CALLBACK(YabauseInit), &yinit, yui_main, G_CALLBACK(YabauseReset));
#endif

	gtk_widget_show(yui);

	return yui;
}

void yui_settings_init(void) {
	yinit.m68kcoretype = M68KCORE_C68K;
	yinit.percoretype = PERCORE_GTK;
	yinit.sh2coretype = SH2CORE_DEFAULT;
#ifdef HAVE_LIBGL
	yinit.vidcoretype = VIDCORE_OGL;
#else
	yinit.vidcoretype = VIDCORE_SOFT;
#endif
	yinit.sndcoretype = SNDCORE_DUMMY;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_NONE;
	yinit.regionid = 0;
	yinit.biospath = 0;
	yinit.cdpath = 0;
	yinit.buppath = 0;
	yinit.mpegpath = 0;
	yinit.cartpath = 0;
        yinit.flags = VIDEOFORMATTYPE_NTSC;
}

gchar * inifile;

void yui_settings_load(void) {
	int i;
	gchar *biosPath;
	g_key_file_load_from_file(keyfile, inifile, G_KEY_FILE_NONE, 0);
	if (yinit.biospath)
	  g_free(yinit.biospath);
	biosPath = g_key_file_get_value(keyfile, "General", "BiosPath", 0);
	if ( !biosPath || (!*biosPath) ) yinit.biospath = NULL; 
	else yinit.biospath = g_strdup(biosPath);
	if (yinit.cdpath)
		g_free(yinit.cdpath);
	yinit.cdpath = g_strdup(g_key_file_get_value(keyfile, "General", "CDROMDrive", 0));
	yinit.cdcoretype = g_key_file_get_integer(keyfile, "General", "CDROMCore", 0);
	{
		char * region = g_key_file_get_value(keyfile, "General", "Region", 0);
		if ((region == 0) || !strcmp(region, "Auto")) {
			yinit.regionid = 0;
		} else {
			switch(region[0]) {
				case 'J': yinit.regionid = 1; break;
				case 'T': yinit.regionid = 2; break;
				case 'U': yinit.regionid = 4; break;
				case 'B': yinit.regionid = 5; break;
				case 'K': yinit.regionid = 6; break;
				case 'A': yinit.regionid = 0xA; break;
				case 'E': yinit.regionid = 0xC; break;
				case 'L': yinit.regionid = 0xD; break;
			}
		}
	}
	if (yinit.cartpath)
		g_free(yinit.cartpath);
	yinit.cartpath = g_strdup(g_key_file_get_value(keyfile, "General", "CartPath", 0));
	if (yinit.buppath)
		g_free(yinit.buppath);
	yinit.buppath = g_strdup(g_key_file_get_value(keyfile, "General", "BackupRamPath", 0));
	if (yinit.mpegpath)
		g_free(yinit.mpegpath);
	yinit.sh2coretype = g_key_file_get_integer(keyfile, "General", "SH2Int", 0);
	yinit.mpegpath = g_strdup(g_key_file_get_value(keyfile, "General", "MpegRomPath", 0));
	yinit.carttype = g_key_file_get_integer(keyfile, "General", "CartType", 0);
	yinit.vidcoretype = g_key_file_get_integer(keyfile, "General", "VideoCore", 0);
	yinit.sndcoretype = g_key_file_get_integer(keyfile, "General", "SoundCore", 0);

	i = 0;

	while(key_names[i]) {
	  u32 key = g_key_file_get_integer(keyfile, "Input", key_names[i], 0);
	  PerSetKey(key, key_names[i]);
	  i++;
	}

	yui_resize(g_key_file_get_integer(keyfile, "General", "Width", 0),
			g_key_file_get_integer(keyfile, "General", "Height", 0),
			g_key_file_get_integer(keyfile, "General", "Fullscreen", 0));

        yinit.flags = g_key_file_get_integer(keyfile, "General", "VideoFormat", 0);
}

void YuiErrorMsg(const char * string) {
	yui_window_log(YUI_WINDOW(yui), string);
}

int main(int argc, char *argv[]) {
#ifndef NO_CLI
	int i;
#endif
	LogStart();
	LogChangeOutput( DEBUG_CALLBACK, YuiErrorMsg );
	inifile = g_build_filename(g_get_home_dir(), ".yabause.ini", NULL);
	
	keyfile = g_key_file_new();

	gtk_init(&argc, &argv);
#ifdef HAVE_LIBGL
	gtk_gl_init(&argc, &argv);
#endif

	yui_settings_init();

	yui = yui_new();

	yui_settings_load();

#ifndef NO_CLI
   //handle command line arguments
   for (i = 1; i < argc; ++i) {
      if (argv[i]) {
         //show usage
         if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
         }
			
         //set bios
         if (0 == strcmp(argv[i], "-b") && argv[i + 1]) {
	    if (yinit.biospath)
	       g_free(yinit.biospath);
	    yinit.biospath = g_strdup(argv[i + 1]);
	 } else if (strstr(argv[i], "--bios=")) {
	    if (yinit.biospath)
	       g_free(yinit.biospath);
	    yinit.biospath =  g_strdup(argv[i] + strlen("--bios="));
	 }
         //set iso
         else if (0 == strcmp(argv[i], "-i") && argv[i + 1]) {
	    if (yinit.cdpath)
		g_free(yinit.cdpath);
	    yinit.cdpath = g_strdup(argv[i + 1]);
	    yinit.cdcoretype = 1;
	 } else if (strstr(argv[i], "--iso=")) {
	    if (yinit.cdpath)
		g_free(yinit.cdpath);
	    yinit.cdpath = g_strdup(argv[i] + strlen("--iso="));
	    yinit.cdcoretype = 1;
	 }
         //set cdrom
	 else if (0 == strcmp(argv[i], "-c") && argv[i + 1]) {
	    if (yinit.cdpath)
		g_free(yinit.cdpath);
	    yinit.cdpath = g_strdup(argv[i + 1]);
	    yinit.cdcoretype = 2;
	 } else if (strstr(argv[i], "--cdrom=")) {
	    if (yinit.cdpath)
		g_free(yinit.cdpath);
	    yinit.cdpath = g_strdup(argv[i] + strlen("--cdrom="));
	    yinit.cdcoretype = 2;
	 }
         // Set sound
         else if (strcmp(argv[i], "-ns") == 0 || strcmp(argv[i], "--nosound") == 0) {
	    yinit.sndcoretype = 0;
	 }
	 // Autostart
	 else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--autostart") == 0) {
            yui_window_run(NULL, YUI_WINDOW(yui));
	 }
	 // Fullscreen
	 else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0) {
            yui_window_set_fullscreen(YUI_WINDOW(yui), TRUE);
	 } else if (strstr(argv[i], "--binary=")) {
            yui_window_run(NULL, YUI_WINDOW(yui));
	    MappedMemoryLoadExec(argv[i] + strlen("--binary="), 0x06004000);
	 }
      }
   }
#endif

	gtk_main ();

	YabauseDeInit();
	LogStop();

	return 0;
}

void YuiQuit(void) {
}


void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {
}

void YuiSetVideoAttribute(int type, int val) {
}

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) {
	return 0;
}

void YuiSwapBuffers(void) {
	yui_window_update(YUI_WINDOW(yui));
}

void yui_conf(void) {
	gint result;
	GtkWidget * dialog;

	dialog = create_dialog1(YUI_WINDOW(yui));

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	switch(result) {
		case GTK_RESPONSE_OK:
                {
                        GtkWidget* warningDlg = gtk_message_dialog_new (GTK_WINDOW(yui),
                                                                        GTK_DIALOG_MODAL,
                                                                        GTK_MESSAGE_WARNING,
                                                                        GTK_BUTTONS_OK,
                                                                        "You must restart Yabause before the changes take effect.",
                                                                        NULL);

                        gtk_dialog_run (warningDlg);
                        gtk_widget_destroy (warningDlg); 
			g_file_set_contents(inifile, g_key_file_to_data(keyfile, 0, 0), -1, 0);
			yui_settings_load();
			break;
                }
		case GTK_RESPONSE_CANCEL:
			g_key_file_load_from_file(keyfile, inifile, G_KEY_FILE_NONE, 0);
			break;
	}
}

void yui_resize(guint width, guint height, gboolean fullscreen) {
	gtk_widget_set_size_request(YUI_WINDOW(yui)->area, width, height);
	yui_window_set_fullscreen(yui, fullscreen);
}
