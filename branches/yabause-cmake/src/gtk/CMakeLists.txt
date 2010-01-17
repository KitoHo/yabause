project(yabause)

include(FindPkgConfig)

if (NOT OPENGL_FOUND)
	pkg_check_modules(GTK REQUIRED gtk+-2.0)
	set(PORT_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
	set(PORT_CFLAGS ${GTK_CFLAGS})
	set(PORT_LIBRARIES ${GTK_LIBRARIES})
else (NOT OPENGL_FOUND)
	pkg_check_modules(GTKGLEXT gtkglext-1.0)
	if (NOT GTKGLEXT_FOUND)
		message(FATAL_ERROR "Found OpenGL and Gtk+ but not libgtkglext.")
	endif (NOT GTKGLEXT_FOUND)
	set(PORT_INCLUDE_DIRS ${GTKGLEXT_INCLUDE_DIRS})
	set(PORT_CFLAGS -DHAVE_LIBGTKGLEXT=1 ${GTKGLEXT_CFLAGS})
	set(PORT_LIBRARIES ${GTKGLEXT_LIBRARIES})
endif (NOT OPENGL_FOUND)
include_directories(${PORT_INCLUDE_DIRS})
add_definitions(${PORT_CFLAGS})

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
target_link_libraries(yabause-gtk ${PORT_LIBRARIES})
target_link_libraries(yabause-gtk libyabause.a)
