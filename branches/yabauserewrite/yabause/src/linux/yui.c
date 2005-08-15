#include "../yui.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../persdl.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "yabause_logo.xpm"

#include "SDL.h"

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERSDL,
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
&SNDSDL,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSDLGL,
NULL
};

void GtkYuiRun(void);

static GtkItemFactoryEntry entries[] = {
	{ "/_Yabause",			NULL,		NULL,			0, "<Branch>" },
	{ "/Yabause/tear1",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Yabause/_Open BIOS...",	"<CTRL>B",	NULL,			1, "<Item>" },
	{ "/Yabause/_Open Binary...",	NULL,		NULL,			1, "<Item>" },
	{ "/Yabause/_Open CDROM...",	"<CTRL>C",	NULL,			1, "<Item>" },
	{ "/Yabause/_Open ISO...",	"<CTRL>I",	NULL,			1, "<Item>" },
	{ "/Yabause/sep1",		NULL,		NULL,			0, "<Separator>" },
	{ "/Yabause/_Quit",		"<CTRL>Q",	YuiQuit,		0, "<StockItem>", GTK_STOCK_QUIT },
	{ "/Emulation",			NULL,		NULL,			0, "<Branch>" },
	{ "/Emulation/tear2",		NULL,		NULL,			0, "<Tearoff>" },
	{ "/Emulation/_Run",		"r",		GtkYuiRun,		1, "<Item>" },
	{"/_Help",NULL,NULL,0,"<Branch>"},
	{"/Help/tear3",NULL,NULL,0,"<Tearoff>"},
	{"/Help/About",NULL,NULL,1,"<Item>"}
};

struct {
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *event_box;
	GtkWidget *menu_bar;
	gboolean hide;
} GtkYui;

char SDL_windowhack[32];
int cdcore = CDCORE_DEFAULT;
const char * bios = "jap.rom";
const char * iso_or_cd = 0;

int GtkWorkaround(void) {
	return PERCore->HandleEvents() + 1;
}

static gboolean GtkYuiKeyPress( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYDOWN;
	eve.key.state = SDL_PRESSED;
	switch(event->key.keyval) {
		case GDK_Up:
			eve.key.keysym.sym = SDLK_UP;
			break;
		case GDK_Right:
			eve.key.keysym.sym = SDLK_RIGHT;
			break;
		case GDK_Down:
			eve.key.keysym.sym = SDLK_DOWN;
			break;
		case GDK_Left:
			eve.key.keysym.sym = SDLK_LEFT;
			break;
		case GDK_F1:
			eve.key.keysym.sym = SDLK_F1;
			break;
		default:
			eve.key.keysym.sym = (SDLKey) event->key.keyval;
			break;
	}
	SDL_PushEvent(&eve);
}

static gboolean GtkYuiKeyRelease( GtkWidget *widget, GdkEvent *event, gpointer data) {
	SDL_Event eve;
	eve.type = eve.key.type = SDL_KEYUP;
	eve.key.state = SDL_RELEASED;
	switch(event->key.keyval) {
		case GDK_Up:
			eve.key.keysym.sym = SDLK_UP;
			break;
		case GDK_Right:
			eve.key.keysym.sym = SDLK_RIGHT;
			break;
		case GDK_Down:
			eve.key.keysym.sym = SDLK_DOWN;
			break;
		case GDK_Left:
			eve.key.keysym.sym = SDLK_LEFT;
			break;
		case GDK_F1:
			eve.key.keysym.sym = SDLK_F1;
			break;
		default:
			eve.key.keysym.sym = (SDLKey) event->key.keyval;
			break;
	}
	SDL_PushEvent(&eve);
}

void GtkYuiRun(void) {
   YabauseInit(PERCORE_SDL, SH2CORE_DEFAULT, VIDCORE_SDLGL, SNDCORE_SDL,
                   cdcore, REGION_AUTODETECT, bios, iso_or_cd,
                   NULL, NULL);

   g_idle_add((gboolean (*)(void*)) GtkWorkaround, NULL);
}

int GtkYuiInit(void) {
	int fake_argc = 0;
	char ** fake_argv = NULL;
	char * text;
	GtkItemFactory *gif;

	gtk_init (&fake_argc, &fake_argv);

	GtkYui.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT (GtkYui.window), "key_press_event", G_CALLBACK (GtkYuiKeyPress), NULL);
	g_signal_connect(G_OBJECT (GtkYui.window), "key_release_event", G_CALLBACK (GtkYuiKeyRelease), NULL);
	g_signal_connect(G_OBJECT (GtkYui.window), "delete_event", GTK_SIGNAL_FUNC(YuiQuit), NULL);

	GtkYui.vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (GtkYui.window), GtkYui.vbox);
	gtk_widget_show (GtkYui.vbox);

	text = g_strdup_printf ("Yabause %s", VERSION);
	gtk_window_set_title(GTK_WINDOW (GtkYui.window), text);
	g_free(text);

	gif = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<YabauseMain>", NULL);
	gtk_item_factory_create_items(gif, sizeof(entries)/sizeof(entries[0]), entries, NULL);

	GtkYui.menu_bar = gtk_item_factory_get_widget(gif, "<YabauseMain>");
	gtk_box_pack_start(GTK_BOX (GtkYui.vbox), GtkYui.menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (GtkYui.menu_bar);

	GtkYui.event_box = gtk_event_box_new ();
	
	gtk_widget_set_usize(GtkYui.event_box, 320, 224);
	gtk_box_pack_start (GTK_BOX (GtkYui.vbox), GtkYui.event_box, FALSE, FALSE, 0);
	gtk_widget_show (GtkYui.event_box);
	gtk_widget_realize(GtkYui.event_box);

	gtk_window_set_resizable( GTK_WINDOW(GtkYui.window), FALSE);
	gtk_widget_show (GtkYui.window);

	GtkYui.hide = FALSE;
	sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", GDK_WINDOW_XWINDOW(GTK_WIDGET(GtkYui.event_box)->window));
	putenv(SDL_windowhack);
}

void YuiSetBiosFilename(const char * biosfilename) {
        bios = biosfilename;
}

void YuiSetIsoFilename(const char * isofilename) {
	cdcore = CDCORE_DEFAULT;
	iso_or_cd = isofilename;
}

void YuiSetCdromFilename(const char * cdromfilename) {
	cdcore = CDCORE_ARCH;
	iso_or_cd = cdromfilename;
}

void YuiHideShow(void) {
}

void YuiQuit(void) {
   gtk_main_quit();
}

int YuiInit(void) {
   GtkYuiInit();

   gtk_main();

   return 0;
}

void YuiErrorMsg(const char *error_text, SH2_struct *sh2opcodes) {
   fprintf(stderr, "Error: %s\n", error_text);

   fprintf(stderr, "R0 = %08X\tR12 = %08X\n", sh2opcodes->regs.R[0], sh2opcodes->regs.R[12]);
   fprintf(stderr, "R1 = %08X\tR13 = %08X\n", sh2opcodes->regs.R[1], sh2opcodes->regs.R[13]);
   fprintf(stderr, "R2 = %08X\tR14 = %08X\n", sh2opcodes->regs.R[2], sh2opcodes->regs.R[14]);
   fprintf(stderr, "R3 = %08X\tR15 = %08X\n", sh2opcodes->regs.R[3], sh2opcodes->regs.R[15]);
   fprintf(stderr, "R4 = %08X\tSR = %08X\n", sh2opcodes->regs.R[4], sh2opcodes->regs.SR.all);
   fprintf(stderr, "R5 = %08X\tGBR = %08X\n", sh2opcodes->regs.R[5], sh2opcodes->regs.GBR);
   fprintf(stderr, "R6 = %08X\tVBR = %08X\n", sh2opcodes->regs.R[6], sh2opcodes->regs.VBR);
   fprintf(stderr, "R7 = %08X\tMACH = %08X\n", sh2opcodes->regs.R[7], sh2opcodes->regs.MACH);
   fprintf(stderr, "R8 = %08X\tMACL = %08X\n", sh2opcodes->regs.R[8], sh2opcodes->regs.MACL);
   fprintf(stderr, "R9 = %08X\tPR = %08X\n", sh2opcodes->regs.R[9], sh2opcodes->regs.PR);
   fprintf(stderr, "R10 = %08X\tPC = %08X\n", sh2opcodes->regs.R[10], sh2opcodes->regs.PC);
   fprintf(stderr, "R11 = %08X\n", sh2opcodes->regs.R[11]);
}

