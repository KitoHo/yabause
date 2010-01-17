set(gen68k_SOURCES c68kexec.c c68k.c gen68k.c)

add_definitions(-DC68K_GEN)

add_executable(gen68k ${gen68k_SOURCES})

execute_process(COMMAND gen68k)

set(GEN68K_INC c68k_ini.inc c68k_op0.inc c68k_op1.inc c68k_op2.inc c68k_op3.inc c68k_op4.inc c68k_op5.inc c68k_op6.inc c68k_op7.inc c68k_op8.inc c68k_op9.inc c68k_opA.inc c68k_opB.inc c68k_opC.inc c68k_opD.inc c68k_opE.inc c68k_opF.inc)

add_custom_command(OUTPUT ${GEN68K_INC} COMMAND gen68k DEPENDS gen68k)

add_custom_target(c68kinc ALL DEPENDS ${GEN68K_INC})
