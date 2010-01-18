project(yabause)

#include(FindPkgConfig)

#if (NOT OPENGL_FOUND)
#	pkg_check_modules(GTK REQUIRED gtk+-2.0)
#	set(PORT_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
#	set(PORT_CFLAGS ${GTK_CFLAGS})
#	set(PORT_LIBRARIES ${GTK_LIBRARIES})
#else (NOT OPENGL_FOUND)
#	pkg_check_modules(GTKGLEXT gtkglext-1.0)
#	if (NOT GTKGLEXT_FOUND)
#		message(FATAL_ERROR "Found OpenGL and Gtk+ but not libgtkglext.")
#	endif (NOT GTKGLEXT_FOUND)
#	set(PORT_INCLUDE_DIRS ${GTKGLEXT_INCLUDE_DIRS})
#	set(PORT_CFLAGS -DHAVE_LIBGTKGLEXT=1 ${GTKGLEXT_CFLAGS})
#	set(PORT_LIBRARIES ${GTKGLEXT_LIBRARIES})
#endif (NOT OPENGL_FOUND)
#include_directories(${PORT_INCLUDE_DIRS})
#add_definitions(${PORT_CFLAGS})



set(yabause_windows_SOURCES
	aviout.c
	bup-manager.c
	cheats.c
	custctl.c
	disasm.c
	hotkey.cpp
	perdx.c
	ramwatch.cpp
	ram_search.cpp
	snddx.c
	yui.cpp
	cpudebug/debug-68k.c
	cpudebug/debug-scsp.c
	cpudebug/debug-scu.c
	cpudebug/debug-sh2.c
	cpudebug/debug-smpc.c
	cpudebug/debug-vdp1.c
	cpudebug/debug-vdp2.c
	cpudebug/yuidebug.c
	settings/settings.c
	settings/settings-basic.c
	settings/settings-input.c
	settings/settings-log.c
	settings/settings-netlink.c
	settings/settings-sound.c
	settings/settings-video.c)

link_directories(..)
add_executable(yabause-windows ${yabause_windows_SOURCES})
set_target_properties(yabause-windows PROPERTIES OUTPUT_NAME yabause)
target_link_libraries(yabause-windows ${PORT_LIBRARIES})
target_link_libraries(yabause-windows libyabause.a)
