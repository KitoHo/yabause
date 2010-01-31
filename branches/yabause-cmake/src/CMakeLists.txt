project(yabause)

set(yabause_SOURCES
	bios.c
	cdbase.c cheat.c coffelf.c cs0.c cs1.c cs2.c
	debug.c
	error.c
	m68kcore.c m68kd.c m68kq68.c memory.c movie.c
	netlink.c
	peripheral.c profile.c
	scsp.c scu.c sh2core.c sh2d.c sh2idle.c sh2int.c sh2trace.c smpc.c
	vdp1.c vdp2.c vdp2debug.c vidogl.c vidshared.c vidsoft.c
	yabause.c ygl.c)

# Bigendian
include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
if (WORDS_BIGENDIAN)
	add_definitions(-DWORDS_BIGENDIAN=1)
endif (WORDS_BIGENDIAN)

# OpenGL
option(YAB_WANT_OPENGL "use OpenGL for video output (most ports require it)" ON)
if (YAB_WANT_OPENGL)
	include(FindOpenGL)
	if (OPENGL_FOUND)
		add_definitions(-DHAVE_LIBGL=1)
		set(YABAUSE_LIBRARIES ${YABAUSE_LIBRARIES} ${OPENGL_LIBRARY})
	endif(OPENGL_FOUND)
endif (YAB_WANT_OPENGL)

# SDL
option(YAB_WANT_SDL "use SDL cores if available" ON)
if (YAB_WANT_SDL)
	include(FindSDL)
	if (SDL_FOUND)
		add_definitions(-DHAVE_LIBSDL=1)
		include_directories(${SDL_INCLUDE_DIR})
		set(yabause_SOURCES ${yabause_SOURCES} persdljoy.c sndsdl.c)
		set(YABAUSE_LIBRARIES ${YABAUSE_LIBRARIES} ${SDL_LIBRARY})
	endif (SDL_FOUND)
endif (YAB_WANT_SDL)

# OpenAL
option(YAB_WANT_OPENAL "use OpenAL sound core if available" ON)
if (YAB_WANT_OPENAL)
	include(FindOpenAL)
	if (OPENAL_FOUND)
		add_definitions(-DHAVE_LIBAL=1)
		include_directories(${OPENAL_INCLUDE_DIR})
		set(yabause_SOURCES ${yabause_SOURCES} sndal.c)
		set(YABAUSE_LIBRARIES ${YABAUSE_LIBRARIES} ${OPENAL_LIBRARY})
	endif (OPENAL_FOUND)
endif (YAB_WANT_OPENAL)

# mini18n
find_path(MINI18N_INCLUDE_DIR mini18n.h)
find_library(MINI18N_LIBRARY mini18n)
if (NOT MINI18N_INCLUDE_DIR STREQUAL "MINI18N_INCLUDE_DIR-NOTFOUND" AND NOT MINI18N_LIBRARY STREQUAL "MINI18N_LIBRARY-NOTFOUND")
	set(MINI18N_FOUND TRUE)
	include_directories(${MINI18N_INCLUDE_DIR})
	add_definitions(-DHAVE_LIBMINI18N=1)
	set(YABAUSE_LIBRARIES ${YABAUSE_LIBRARIES} ${MINI18N_LIBRARY})
endif (NOT MINI18N_INCLUDE_DIR STREQUAL "MINI18N_INCLUDE_DIR-NOTFOUND" AND NOT MINI18N_LIBRARY STREQUAL "MINI18N_LIBRARY-NOTFOUND")

if (UNIX AND MINI18N_FOUND)
	add_definitions(-DYTSDIR=\"${CMAKE_INSTALL_PREFIX}/share/${PACKAGE}/yts\")
endif (UNIX AND MINI18N_FOUND)

include(CheckCSourceCompiles)

# Linux
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	check_c_source_compiles("
		#include <linux/cdrom.h>
		int main(int argc, char ** argv) { int i = CDSL_CURRENT; }
	" LINUX_CDROM_H_OK)
	if (NOT LINUX_CDROM_H_OK)
       		add_definitions(-DLINUX_CDROM_H_IS_BROKEN)
	endif (NOT LINUX_CDROM_H_OK)

	set(yabause_SOURCES ${yabause_SOURCES} cd-linux.c perlinuxjoy.c)
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

# APPLE // not necessary mac os x, but i don't care ;)
if (APPLE)
	FIND_LIBRARY(COREFOUNDATION_LIBRARY NAMES CoreFoundation )
	FIND_LIBRARY(IOKIT_LIBRARY NAMES IOKit )
	set(yabause_SOURCES ${yabause_SOURCES} macjoy.c permacjoy.c cd-macosx.c)
	set(YABAUSE_LIBRARIES ${YABAUSE_LIBRARIES} ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
endif (APPLE)

# Visual Studio
if (MSVC)
        # Find stdint.h
        include(CheckIncludeFile)
        check_include_file("stdint.h" STDINT_H_FOUND)

        # Find DDK
        if (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")
        elseif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/6000/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/6000/")
        elseif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/7600.16385.0/")
		set(DDK_DIR "$ENV{SYSTEMDRIVE}/WINDDK/7600.16385.0/")
        endif (EXISTS "$ENV{SYSTEMDRIVE}/WINDDK/3790.1830/")

	add_definitions(-DHAVE_C99_VARIADIC_MACROS -D_CRT_SECURE_NO_WARNINGS -DC68K_NO_JUMP_TABLE 
			-D_UNICODE -DUNICODE)
endif (MSVC)

# Windows
if (WIN32)
        # Find ntddcdrm.h
	find_path(ntddcdrm_INCLUDE_DIR ddk/ntddcdrm.h)
	if (ntddcdrm_INCLUDE_DIR)
		set (ntddcdrm_INCLUDE_DIR "${ntddcdrm_INCLUDE_DIR}/ddk")
	else (ntddcdrm_INCLUDE_DIR)
        	find_path(ntddcdrm_INCLUDE_DIR ntddcdrm.h
                	  "${DDK_DIR}"
	                  "${DDK_DIR}/ddk"
        	          "${DDK_DIR}/inc"
                	  "${DDK_DIR}/inc/api"
	                  "${DDK_DIR}/inc/ddk")
	endif (ntddcdrm_INCLUDE_DIR)

        if (ntddcdrm_INCLUDE_DIR)
                include_directories(${ntddcdrm_INCLUDE_DIR})
		message(STATUS "Found ntddcdrm.h: ${ntddcdrm_INCLUDE_DIR}")
        else (ntddcdrm_INCLUDE_DIR)
		message(FATAL_ERROR "Could not find ntddcdrm.h")
        endif (ntddcdrm_INCLUDE_DIR)

	set(yabause_SOURCES ${yabause_SOURCES} cd-windows.c)
endif (WIN32)

# c68k
option(YAB_WANT_C68K "enable c68k compilation" ON)
if (YAB_WANT_C68K)
	include(ExternalProject)
	ExternalProject_Add(c68kinc
		DOWNLOAD_COMMAND ""
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/c68k
		CONFIGURE_COMMAND cmake <SOURCE_DIR>
		INSTALL_COMMAND ""
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/c68k
	)

	add_definitions(-DHAVE_C68K=1)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/c68k)
	set(yabause_SOURCES ${yabause_SOURCES} c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c m68kc68k.c)
endif(YAB_WANT_C68K)

# *DEBUG
set(YAB_DEBUG "" CACHE STRING "List of enabled debug information") 
foreach(DEBUG IN LISTS YAB_DEBUG)
	if (${DEBUG} STREQUAL "main")
		add_definitions(-DDEBUG=1)
	elseif (${DEBUG} STREQUAL "cd")
		add_definitions(-DCDDEBUG=1)
	elseif (${DEBUG} STREQUAL "idle")
		add_definitions(-DIDLEDETECTVERBOSE=1)
	else (${DEBUG} STREQUAL "main")
		string(TOUPPER ${DEBUG} UPDEBUG)
		add_definitions(-D${UPDEBUG}_DEBUG=1)
	endif (${DEBUG} STREQUAL "main")
endforeach(DEBUG)

# Yabause Arch
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(-DARCH_IS_MACOSX=1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	add_definitions(-DARCH_IS_FREEBSD=1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_definitions(-DARCH_IS_LINUX=1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
	add_definitions(-DARCH_IS_NETBSD=1)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_definitions(-DARCH_IS_Windows=1)
else (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(-DUNKNOWN_ARCH=1)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(YAB_VERSION 0.9.10)
add_definitions(-DPACKAGE=\"yabause\")
add_definitions(-DVERSION=\"${YAB_VERSION}\")

add_library(yabause ${yabause_SOURCES})

if (YAB_WANT_C68K)
	add_dependencies(yabause c68kinc)
endif(YAB_WANT_C68K)

subdirs(gtk qt windows dreamcast)
