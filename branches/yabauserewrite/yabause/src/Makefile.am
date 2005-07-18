bin_PROGRAMS = yabause
yabause_SOURCES = cdbase.c memory.c scu.c smpc.c sh2core.c vdp1.c vdp1.h memory.h smpc.h scu.h sh2core.h sh2int.c sh2int.h yabause.c yabause.h scsp.h yui.c cs2.c cs2.h vdp2.c vdp2.h scsp.c scsp.h c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c c68k/c68k_ini.inc
BUILT_SOURCES = c68k/c68k_ini.inc
c68k/c68k_ini.inc: c68k/gen68k
	cd c68k && ./gen68k
c68k/gen68k: c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c
	$(COMPILE) -DC68K_GEN c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c -o c68k/gen68k
