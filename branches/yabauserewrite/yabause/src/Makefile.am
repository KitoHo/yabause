bin_PROGRAMS = yabause
yabause_SOURCES = \
	c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c c68k/c68k_ini.inc \
	$(yabause_arch)/cd.h $(yabause_arch)/cd.c \
	cdbase.c cdbase.h core.h cs0.c cs0.h cs1.c cs1.h cs2.c cs2.h \
	debug.c debug.h memory.c memory.h peripheral.h peripheral.c \
	persdl.c persdl.h \
	scu.c scu.h sh2core.c sh2core.h sh2d.c sh2d.h sh2int.c sh2int.h \
	smpc.c smpc.h scsp.c scsp.h sndsdl.c sndsdl.h vdp1.c vdp1.h \
	vdp2.c vdp2.h vidsdlgl.c vidsdlgl.h \
	yabause.c yabause.h ygl.h ygl.c yui.h yui.c
BUILT_SOURCES = c68k/c68k_ini.inc
c68k/c68k_ini.inc: c68k/gen68k
	cd c68k && ./gen68k
c68k/gen68k: c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c
	$(COMPILE) -DC68K_GEN c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c -o c68k/gen68k
