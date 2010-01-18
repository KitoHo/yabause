project(yabause)

include(FindOpenGL)
if (OPENGL_FOUND)
	add_definitions(-DHAVE_LIBGL=1)
endif(OPENGL_FOUND)

include(CheckCSourceCompiles)

set( unix_not_apple_not_cygwin "UNIX and NOT APPLE and NOT CYGWIN" )

# Linux
#if (unix_not_apple_not_cygwin) # This may need revising
#	check_c_source_compiles("
#	#include <linux/cdrom.h>
#	int main(int argc, char ** argv) { int i = CDSL_CURRENT; }
#	", LINUX_CDROM_H_OK)
#	if(NOT LINUX_CDROM_H_OK)
		add_definitions(-DLINUX_CDROM_H_IS_BROKEN)
#	endif(NOT LINUX_CDROM_H_OK)
#endif(unix_not_apple_not_cygwin)

set(yabause_SOURCES
	bios.c
	cdbase.c cheat.c coffelf.c cs0.c cs1.c cs2.c
	debug.c
	error.c
	m68kcore.c m68kd.c m68kq68.c memory.c movie.c
	netlink.c
	peripheral.c profile.c
	scsp.c scu.c sh2core.c sh2d.c sh2idle.c sh2int.c sh2trace.c smpc.c sndal.c sndsdl.c
	vdp1.c vdp2.c vdp2debug.c vidogl.c vidshared.c vidsoft.c
	yabause.c ygl.c)

# Linux
if (unix_not_apple_not_cygwin) # This may need revising
	set(yabause_SOURCES ${yabause_SOURCES} cd-linux.c perlinuxjoy.c persdljoy.c)
endif(unix_not_apple_not_cygwin)

if (MSVC)
	add_definitions(-DHAVE_C99_VARIADIC_MACROS)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DC68K_NO_JUMP_TABLE)
endif (MSVC)

if (WIN32)
	set(yabause_SOURCES ${yabause_SOURCES} cd-windows.c)
endif (WIN32)

option(YAB_WANT_C68K "enable c68k compilation" ON)
if (YAB_WANT_C68K)
	add_definitions(-DHAVE_C68K=1)
	add_subdirectory(c68k)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/c68k)
	set(yabause_SOURCES ${yabause_SOURCES} c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c m68kc68k.c)
endif(YAB_WANT_C68K)

add_definitions(-DPACKAGE=\"Yabause\")
add_definitions(-DVERSION=\"0.9.10\")

add_library(yabause ${yabause_SOURCES})

if(OPENGL_FOUND)
	link_libraries(yabause ${OPENGL_gl_LIBRARY})
endif(OPENGL_FOUND)

if (YAB_WANT_C68K)
	add_dependencies(yabause c68kinc)
endif(YAB_WANT_C68K)

FIND_PACKAGE(Qt4)

if (UNIX)
	#if (QT4_INSTALLED)
		subdirs(qt)
	#else (QT4_INSTALLED)
		#subdirs(gtk)
	#endif (QT4_INSTALLED)
endif(UNIX)

if (WIN32)
	subdirs(windows)
endif(WIN32)