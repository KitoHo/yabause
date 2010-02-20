project(yabause-windows)

yab_port_start()

if (NOT WIN32)
	return()
endif(NOT WIN32)

if ( NOT ntddcdrm_INCLUDE_DIR )
	return()
endif ( NOT ntddcdrm_INCLUDE_DIR )

if (MSVC)
        # Find DirectX SDK
        find_path(DirectX_INCLUDE_DIR dxerr9.h
		  "$ENV{DXSDK_DIR}/Include")
        if (NOT DirectX_INCLUDE_DIR)
		find_path(DirectX_INCLUDE_DIR "dxerr.h"
			  "$ENV{DXSDK_DIR}/Include")
		if (DirectX_INCLUDE_DIR)
			set(DXERRH_IS_BROKEN 1 CACHE INTERNAL "dxerr is broken")
                endif (DirectX_INCLUDE_DIR)
	endif(NOT DirectX_INCLUDE_DIR)

	find_path(DirectX_LIB_DIR d3d9.lib
                  "$ENV{DXSDK_DIR}/Lib/x86")
else (MSVC)
	add_definitions(-D_WIN32_IE=0x0500 -DUNICODE -D_UNICODE)
	find_path(DirectX_INCLUDE_DIR dxerr9.h)
	find_library(DirectX_LIB_DIR d3d9)
endif(MSVC)

if (MSVC)
        if (DirectX_INCLUDE_DIR AND DirectX_LIB_DIR)
        	set(DIRECTX_FOUND "found")
                include_directories(${DirectX_INCLUDE_DIR})
        endif (DirectX_INCLUDE_DIR AND DirectX_LIB_DIR)
endif(MSVC)

if (MINGW)
#	include(CheckIncludeFile)
#	check_include_file("dxerr9.h" DIRECTX_FOUND)
        	set(DIRECTX_FOUND "found")

	add_definitions(-D_WIN32_IE=0x0500 -D_UNICODE -DUNICODE)
endif(MINGW)

# DirectX
if (DIRECTX_FOUND)
	message(STATUS "Found DirectX SDK 9.0 or greater")
else (DIRECTX_FOUND)
	message(FATAL_ERROR "Could not find DirectX SDK 9.0 or greater")
endif (DIRECTX_FOUND)

if (DXERRH_IS_BROKEN)
	add_definitions(-DDXERRH_IS_BROKEN)
	message(STATUS "Using work-around for dxerr.h")
endif(DXERRH_IS_BROKEN)

set(yabause_windows_HEADERS
	aviout.h
	cd.h
	cheats.h
	custctl.h
	disasm.h
	dx.h
	hexedit.h
	hotkey.h
	perdx.h
	ramwatch.h
	ram_search.h
	resource.h
	snddx.h
	yuiwin.h
	cpudebug/yuidebug.h
	settings/settings.h)

set(yabause_windows_RESOURCES
	resource.rc)

set(yabause_windows_SOURCES
	aviout.c
	bup-manager.c
	cheats.c
	custctl.c
	disasm.c
	hexedit.c
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
add_executable(yabause-windows WIN32 ${yabause_windows_SOURCES} ${yabause_windows_RESOURCES} ${yabause_windows_HEADERS})
target_link_libraries(yabause-windows yabause ${YABAUSE_LIBRARIES} ${PORT_LIBRARIES} comctl32 dxguid dinput8 dsound dxerr8 vfw32)

if (DXERRH_IS_BROKEN)
	target_link_libraries(yabause-windows dxerr)
else(DXERRH_IS_BROKEN)
	if (MINGW)
		target_link_libraries(yabause-windows dxerr8)
	else (MINGW)
		target_link_libraries(yabause-windows dxerr9)
	endif(MINGW)
endif(DXERRH_IS_BROKEN)

yab_port_success(yabause-windows)
