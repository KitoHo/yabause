EXTRA_DIST = icache-funcs-2450.patch psp-logo.png sh2-core.i
CLEANFILES = yabause.prx yabause.sfo EBOOT.PBP

# yabause.{elf,prx} are only intermediate targets; the final target is
# EBOOT.PBP (but we keep the ELF/PRX files around for testing purposes).
all-local: EBOOT.PBP me.prx
.SECONDARY: yabause.elf yabause.prx me.elf

########

noinst_PROGRAMS = yabause.elf me.elf

yabause_elf_SOURCES = config.c control.c display.c filesel.c font.c gu.c \
                      init.c localtime.c main.c menu.c misc.c profile.c \
                      psp-cd.c psp-per.c psp-sh2.c psp-sound.c psp-video.c \
                      rtl.c rtl-mips.c rtlexec.c rtlinsn.c rtlopt.c rtlunit.c \
                      sh2.c sh2-interpret.c sh2-opcodeinfo.c sh2-optimize.c \
                      sys.c texcache.c yui.c \
                      melib.S \
                      \
                      common.h config.h control.h display.h filesel.h font.h \
                      gu.h init.h localtime.h menu.h misc.h profile.h \
                      psp-cd.h psp-per.h psp-sh2.h psp-sound.h psp-video.h \
                      rtl.h rtl-internal.h rtl-mips.h \
                      sh2.h sh2-internal.h \
                      sys.h texcache.h
yabause_elf_CFLAGS = $(YAB_CFLAGS) -Wpointer-arith -Wshadow
yabause_elf_LDADD = ../libyabause.a $(YAB_LIBS)

me_elf_SOURCES = me.c me.h me-exp.c
me_elf_CFLAGS = $(YAB_CFLAGS) -Wpointer-arith -Wshadow
me_elf_LDFLAGS = -nostartfiles
me_elf_LIBS = -lpspkernel
me.elf: $(me_elf_OBJECTS)
	$(me_elf_LINK) $(me_elf_OBJECTS) $(me_elf_LDADD) $(me_elf_LIBS)

########

.elf.prx:
	psp-fixup-imports $<
	psp-prxgen $< $@

me-exp.c: me.exp
	psp-build-exports -b $< >$@
melib.S: me.exp
	psp-build-exports -s $<

yabause.sfo:
	mksfoex -d MEMSIZE=1 "Yabause $(PACKAGE_VERSION)" $@

EBOOT.PBP: psp-logo.png yabause.prx yabause.sfo
	pack-pbp $@ yabause.sfo $< NULL NULL NULL NULL yabause.prx NULL
