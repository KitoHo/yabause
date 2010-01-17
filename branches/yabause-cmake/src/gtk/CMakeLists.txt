project(yabause)

include(FindPkgConfig)

pkg_check_modules(GTK REQUIRED gtk+-2.0)

include_directories(${GTK_INCLUDE_DIRS})
add_definitions(${GTK_CFLAGS})

set(yabause_gtk_SOURCES
	gtk-compat.c
	gtkglwidget.c
	main.c
	menu.c
	pergtk.c
	settings.c
	yuicheckbutton.c
	yuifileentry.c
	yuiinputentry.c
	yuim68k.c
	yuimem.c
	yuipage.c
	yuirange.c
	yuiresolution.c
	yuiscreenshot.c
	yuiscsp.c
	yuiscudsp.c
	yuish.c
	yuitransfer.c
	yuivdp1.c
	yuivdp2.c
	yuiviewer.c
	yuiwindow.c)

link_directories(..)
add_executable(yabause-gtk ${yabause_gtk_SOURCES})
set_target_properties(yabause-gtk PROPERTIES OUTPUT_NAME yabause)
target_link_libraries(yabause-gtk ${GTK_LIBRARIES})
target_link_libraries(yabause-gtk libyabause.a)
