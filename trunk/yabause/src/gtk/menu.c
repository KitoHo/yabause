#include "settings.h"
#include "yuiwindow.h"
#include "yuivdp1.h"
#include "yuivdp2.h"
#include "yuish.h"
#include "yuitransfer.h"
#include "yuim68k.h"
#include "yuiscudsp.h"
#include "yuiscsp.h"
#include "yuimem.h"

void openAboutDialog(GtkWidget * w, gpointer data) {
	gtk_show_about_dialog(data,
		"name", "Yabause",
		"version", VERSION,
		"website", "http://yabause.sourceforge.net",
		NULL);
}

GtkWidget* create_menu(YuiWindow * window1) {
  GtkWidget *menubar1;
  GtkWidget *menuitem1;
  GtkWidget *menuitem1_menu;
  GtkWidget *new1;
  GtkWidget *separatormenuitem1;
  GtkWidget *quit1;
  GtkWidget *view1;
  GtkWidget *view1_menu;
  GtkWidget *fps1;
  GtkWidget *layer1;
  GtkWidget *layer1_menu;
  GtkWidget *vdp3;
  GtkWidget *nbg1;
  GtkWidget *nbg2;
  GtkWidget *nbg3;
  GtkWidget *nbg4;
  GtkWidget *rbg1;
  GtkWidget *log;
  GtkWidget *menuitem3;
  GtkWidget *menuitem3_menu;
  GtkWidget *msh;
  GtkWidget *ssh;
  GtkWidget *vdp2;
  GtkWidget *vdp1;
  GtkWidget *m68k;
  GtkWidget *scudsp;
  GtkWidget *scsp;
  GtkWidget *menuitem4;
  GtkWidget *menuitem4_menu;
  GtkWidget *about1;
  GtkAccelGroup *accel_group;
  GtkWidget *transfer;
  GtkWidget *memory;

  accel_group = gtk_accel_group_new ();

  menubar1 = gtk_menu_bar_new ();

  menuitem1 = gtk_menu_item_new_with_mnemonic ("_Yabause");
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem1);

  menuitem1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem1), menuitem1_menu);

  new1 = gtk_image_menu_item_new_from_stock ("gtk-preferences", NULL);
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), new1);

  gtk_container_add(GTK_CONTAINER(menuitem1_menu), gtk_action_create_menu_item(gtk_action_group_get_action(window1->action_group, "run")));
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), gtk_action_create_menu_item(gtk_action_group_get_action(window1->action_group, "pause")));
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), gtk_action_create_menu_item(gtk_action_group_get_action(window1->action_group, "reset")));

  transfer = gtk_menu_item_new_with_mnemonic ("Transfer");
  gtk_container_add (GTK_CONTAINER (menuitem1_menu), transfer);
  g_signal_connect_swapped(transfer, "activate", G_CALLBACK(yui_transfer_new), window1);

  gtk_container_add (GTK_CONTAINER (menuitem1_menu), gtk_separator_menu_item_new ());

  gtk_container_add(GTK_CONTAINER(menuitem1_menu), gtk_action_create_menu_item(gtk_action_group_get_action(window1->action_group, "quit")));

  view1 = gtk_menu_item_new_with_mnemonic ("_View");
  gtk_container_add (GTK_CONTAINER (menubar1), view1);

  view1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (view1), view1_menu);

  fps1 = gtk_check_menu_item_new_with_mnemonic ("FPS");
  gtk_container_add (GTK_CONTAINER (view1_menu), fps1);

  layer1 = gtk_menu_item_new_with_mnemonic ("Layer");
  gtk_container_add (GTK_CONTAINER (view1_menu), layer1);

  layer1_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (layer1), layer1_menu);

  vdp3 = gtk_check_menu_item_new_with_mnemonic ("Vdp1");
  gtk_container_add (GTK_CONTAINER (layer1_menu), vdp3);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (vdp3), TRUE);
  g_signal_connect(vdp3, "activate", G_CALLBACK(ToggleVDP1), 0);

  nbg1 = gtk_check_menu_item_new_with_mnemonic ("NBG0");
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg1);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg1), TRUE);
  g_signal_connect(nbg1, "activate", G_CALLBACK(ToggleNBG0), 0);

  nbg2 = gtk_check_menu_item_new_with_mnemonic ("NBG1");
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg2);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg2), TRUE);
  g_signal_connect(nbg2, "activate", G_CALLBACK(ToggleNBG1), 0);

  nbg3 = gtk_check_menu_item_new_with_mnemonic ("NBG2");
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg3);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg3), TRUE);
  g_signal_connect(nbg3, "activate", G_CALLBACK(ToggleNBG2), 0);

  nbg4 = gtk_check_menu_item_new_with_mnemonic ("NBG3");
  gtk_container_add (GTK_CONTAINER (layer1_menu), nbg4);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (nbg4), TRUE);
  g_signal_connect(nbg4, "activate", G_CALLBACK(ToggleNBG3), 0);

  rbg1 = gtk_check_menu_item_new_with_mnemonic ("RBG1");
  gtk_container_add (GTK_CONTAINER (layer1_menu), rbg1);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (rbg1), TRUE);
  g_signal_connect(rbg1, "activate", G_CALLBACK(ToggleRBG0), 0);

  gtk_container_add(GTK_CONTAINER(view1_menu), gtk_action_create_menu_item(gtk_action_group_get_action(window1->action_group, "fullscreen")));

  log = gtk_menu_item_new_with_mnemonic ("Log");
  g_signal_connect_swapped(log, "activate", G_CALLBACK(yui_window_show_log), window1);
  gtk_container_add(GTK_CONTAINER(view1_menu), log);

  menuitem3 = gtk_menu_item_new_with_mnemonic ("_Debug");
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem3);

  menuitem3_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem3), menuitem3_menu);

  msh = gtk_menu_item_new_with_mnemonic ("MSH2");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), msh);
  g_signal_connect_swapped(msh, "activate", G_CALLBACK(yui_msh_new), window1);

  ssh = gtk_menu_item_new_with_mnemonic ("SSH2");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), ssh);
  g_signal_connect_swapped(ssh, "activate", G_CALLBACK(yui_ssh_new), window1);

  vdp2 = gtk_menu_item_new_with_mnemonic ("Vdp1");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), vdp2);
  g_signal_connect_swapped(vdp2, "activate", G_CALLBACK(yui_vdp1_new), window1);

  vdp1 = gtk_menu_item_new_with_mnemonic ("Vdp2");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), vdp1);
  g_signal_connect_swapped(vdp1, "activate", G_CALLBACK(yui_vdp2_new), window1);

  m68k = gtk_menu_item_new_with_mnemonic ("M68K");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), m68k);
  g_signal_connect_swapped(m68k, "activate", G_CALLBACK(yui_m68k_new), window1);

  scudsp = gtk_menu_item_new_with_mnemonic ("SCU-DSP");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), scudsp);
  g_signal_connect_swapped(scudsp, "activate", G_CALLBACK(yui_scudsp_new), window1);

  scsp = gtk_menu_item_new_with_mnemonic ("SCSP");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), scsp);
  g_signal_connect_swapped(scsp, "activate", G_CALLBACK(yui_scsp_new), window1);

  gtk_container_add (GTK_CONTAINER (menuitem3_menu), gtk_separator_menu_item_new ());

  memory = gtk_menu_item_new_with_mnemonic ("Memory dump");
  gtk_container_add (GTK_CONTAINER (menuitem3_menu), memory);
  g_signal_connect_swapped(memory, "activate", G_CALLBACK(yui_mem_new), window1);

  menuitem4 = gtk_menu_item_new_with_mnemonic ("_Help");
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem4);

  menuitem4_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4), menuitem4_menu);

  about1 = gtk_image_menu_item_new_from_stock ("gtk-about", NULL);
  gtk_container_add (GTK_CONTAINER (menuitem4_menu), about1);
  g_signal_connect(about1, "activate", G_CALLBACK(openAboutDialog), window1);

  //gtk_window_add_accel_group (GTK_WINDOW (window1), accel_group);

  g_signal_connect(new1, "activate", yui_conf, 0);

  return menubar1;
}

