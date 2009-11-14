/*  src/psp/me.h: PSP Media Engine library interface
    Copyright 2009 Andrew Church

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <pspkernel.h>
extern int sceSysregAvcResetEnable(void);  // Missing from pspsysreg.h

#include "me.h"

/*************************************************************************/
/************************ PSP module information *************************/
/*************************************************************************/

#define MODULE_FLAGS \
    (PSP_MODULE_KERNEL | PSP_MODULE_SINGLE_LOAD | PSP_MODULE_SINGLE_START)
#define MODULE_VERSION   1
#define MODULE_REVISION  0

PSP_MODULE_INFO("melib", MODULE_FLAGS, MODULE_VERSION, MODULE_REVISION);

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/* Has the ME been started? */

static int me_started;

/* Message block for signaling between the main CPU and Media Engine */

typedef struct MEMessageBlock_ {
    /* ME idle flag; nonzero indicates that the Media Engine is idle and
     * ready to accept a new execute request.  Set only by the ME; cleared
     * only by the main CPU. */
    int idle;

    /* Execute request flag; nonzero indicates that the execute request
     * fields below have been filled in and the ME should start executing
     * the given function.  Set only by the main CPU; cleared only by the
     * ME (except when cleared by the main CPU before starting the ME). */
    int request;

    /* Function to be executed.  Written only by the main CPU. */
    int (*function)(void *);

    /* Argument to be passed to function.  Written only by the main CPU. */
    void *argument;

    /* Return value of last function executed.  Written only by the ME. */
    int result;
} MEMessageBlock;

static volatile __attribute__((aligned(64)))
    MEMessageBlock message_block_buffer;
static volatile MEMessageBlock *message_block;
    // = &message_block_buffer | 0xA0000000

/*************************************************************************/

/* Local function/pointer declarations */

extern void me_init, me_init_end;
__attribute__((noreturn)) void me_loop(void);

/*************************************************************************/
/************************** Module entry points **************************/
/*************************************************************************/

/**
 * module_start:  Entry point called when the module is started.  We have
 * no initialization code to perform, so we just return success.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, otherwise an error code (negative)
 */
extern int module_start(void);
int module_start(void)
{
    return 0;
}

/*************************************************************************/

/**
 * module_stop:  Entry point called when the module is stopped.  We have
 * no cleanup code to perform, so we just return success.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, otherwise an error code (negative)
 */
extern int module_stop(void);
int module_stop(void)
{
    return 0;
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

/**
 * meStart:  Start up the Media Engine.  This function must be called
 * before any other Media Engine operations are performed.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, otherwise an error code (negative)
 */
int meStart(void)
{
    uint32_t old_k1;
    asm("move %0, $k1; move $k1, $zero" : "=r" (old_k1));

    if (me_started) {
        asm volatile("move $k1, %0" : : "r" (old_k1));
        return 0;
    }

    message_block =
        (volatile MEMessageBlock *)((uintptr_t)&message_block_buffer
                                    | 0xA0000000);
    message_block->idle = 0;
    message_block->request = 0;

    /* Roll our own memcpy() to avoid an unneeded libc reference. */
    const uint32_t *src = (const uint32_t *)&me_init;
    const uint32_t *src_top = (const uint32_t *)&me_init_end;
    uint32_t *dest = (uint32_t *)0xBFC00040;
    for (; src < src_top; src++, dest++) {
        *dest = *src;
    }

    /* Flush the data cache, just to be safe. */
    sceKernelDcacheWritebackInvalidateAll();

    int res;
    if ((res = sceSysregMeResetEnable()) < 0
     || (res = sceSysregMeBusClockEnable()) < 0
     || (res = sceSysregMeResetDisable()) < 0
    ) {
        asm volatile("move $k1, %0" : : "r" (old_k1));
        return res;
    }

    me_started = 1;

    asm volatile("move $k1, %0" : : "r" (old_k1));
    return 0;
}

/*************************************************************************/

/**
 * meStop:  Stop the Media Engine.  No other Media Engine functions except
 * meStart() may be called after this function returns.
 *
 * If code is currently executing on the Media Engine when this function is
 * called, the effect on system state is undefined.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void meStop(void)
{
    sceSysregVmeResetEnable();
    sceSysregAvcResetEnable();
    sceSysregMeResetEnable();
    sceSysregMeBusClockDisable();
}

/*************************************************************************/

/**
 * meCall:  Begin executing the given function on the Media Engine.  The
 * function must not call any firmware functions, whether directly or
 * indirectly.  This routine may only be called when the Media Engine is
 * idle (i.e., when mePoll() returns zero).
 *
 * [Parameters]
 *     function: Function pointer
 *     argument: Optional argument to function
 * [Return value]
 *     Zero on success, otherwise an error code (negative)
 */
int meCall(int (*function)(void *), void *argument)
{
    if (!me_started) {
        return ME_ERROR_NOT_STARTED;
    }
    if (!message_block->idle || message_block->request) {
        return ME_ERROR_BUSY;
    }

    message_block->function = function;
    message_block->argument = argument;
    message_block->request = 1;
    return 0;
}

/*************************************************************************/

/**
 * mePoll:  Check whether the Media Engine is idle.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero if the Media Engine is idle, otherwise an error code (negative)
 */
int mePoll(void)
{
    if (!me_started) {
        return ME_ERROR_NOT_STARTED;
    }
    return message_block->idle && !message_block->request ? 0 : ME_ERROR_BUSY;
}

/*************************************************************************/

/**
 * meWait:  Wait for the Media Engine to become idle.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero if the Media Engine has become (or was already) idle,
 *     otherwise an error code (negative)
 */
int meWait(void)
{
    if (!me_started) {
        return ME_ERROR_NOT_STARTED;
    }
    while (!message_block->idle || message_block->request) { /*spin*/ }
    return 0;
}

/*************************************************************************/

/**
 * meResult:  Return the result (return value) of the most recently
 * executed function.  The result is undefined if the Media Engine has not
 * been started or is not idle, or if the most recently executed function
 * did not return a result (i.e. had a return type of void).
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Result of the most recently executed function if the Media Engine
 *     is currently idle, otherwise undefined
 */
int meResult(void)
{
    if (!me_started) {
        return ME_ERROR_NOT_STARTED;
    }
    return message_block->result;
}

/*************************************************************************/
/********************* Media Engine utility routines *********************/
/*************************************************************************/

/**
 * meIcacheInvalidateAll:  Invalidate all entries in the Media Engine's
 * instruction cache.
 *
 * This routine may only be called from code executing on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void meIcacheInvalidateAll(void)
{
    unsigned int cachesize_bits;
    asm("mfc0 %0, $16; ext %0, %0, 9, 3" : "=r" (cachesize_bits));
    const unsigned int cachesize = 4096 << cachesize_bits;

    asm("mtc0 $zero, $28");  // TagLo
    asm("mtc0 $zero, $29");  // TagHi
    unsigned int i;
    for (i = 0; i < cachesize; i += 64) {
        asm volatile("cache 0x1, 0(%0)" : : "r" (i));
    }
}

/*************************************************************************/

/**
 * meDcacheInvalidateAll:  Invalidate all entries in the Media Engine's
 * data cache.
 *
 * This routine may only be called from code executing on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void meDcacheInvalidateAll(void)
{
    unsigned int cachesize_bits;
    asm("mfc0 %0, $16; ext %0, %0, 6, 3" : "=r" (cachesize_bits));
    const unsigned int cachesize = 4096 << cachesize_bits;

    asm("mtc0 $zero, $28");  // TagLo
    asm("mtc0 $zero, $29");  // TagHi
    unsigned int i;
    for (i = 0; i < cachesize; i += 64) {
        asm volatile("cache 0x11, 0(%0)" : : "r" (i));
    }
}

/*************************************************************************/

/**
 * meDcacheWritebackInvalidateAll:  Write back and then invalidate all
 * entries in the Media Engine's data cache.
 *
 * This routine may only be called from code executing on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void meDcacheWritebackInvalidateAll(void)
{
    unsigned int cachesize_bits;
    asm("mfc0 %0, $16; ext %0, %0, 6, 3" : "=r" (cachesize_bits));
    const unsigned int cachesize_half = 2048 << cachesize_bits;

    unsigned int i;
    for (i = 0; i < cachesize_half; i += 64) {
        asm volatile("cache 0x14, 0(%0)" : : "r" (i));
        asm volatile("cache 0x14, 0(%0)" : : "r" (i));
    }
    asm volatile("sync");
}

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * me_init:  Initialization code for the Media Engine.  Copied to
 * 0xBFC00040 in shared memory space.
 */
asm("\
        .set push; .set noreorder                                       \n\
                                                                        \n\
        .globl me_init                                                  \n\
me_init:                                                                \n\
                                                                        \n\
        # Set up hardware registers.                                    \n\
        li $k0, 0xBC100000                                              \n\
        li $v0, 7                                                       \n\
        sw $v0, 0x50($k0)  # Enable bus clock                           \n\
        li $v0, -1                                                      \n\
        sw $v0, 0x04($k0)  # Clear pending interrupts                   \n\
        li $v0, 2          # Assume 64MB for now (watch your pointers!) \n\
        sw $v0, 0x40($k0)  # Set memory size                            \n\
                                                                        \n\
        # Clear the caches                                              \n\
        mtc0 $zero, $28    # TagLo                                      \n\
        mtc0 $zero, $29    # TagLo                                      \n\
        mfc0 $v1, $16      # Config                                     \n\
        ext $v0, $v1, 9, 3 # Instruction cache size                     \n\
        li $a0, 2048                                                    \n\
        sllv $a0, $a0, $v0                                              \n\
        ext $v0, $v1, 6, 3 # Data cache size                            \n\
        li $a1, 2048                                                    \n\
        sllv $a1, $a1, $v0                                              \n\
1:      addiu $a0, $a0, -64                                             \n\
        bnez $a0, 1b                                                    \n\
        cache 0x01, 0($a0) # Shouldn't this be 0x00?                    \n\
1:      addiu $a1, $a1, -64                                             \n\
        bnez $a1, 1b                                                    \n\
        cache 0x11, 0($a1)                                              \n\
                                                                        \n\
        # Enable the FPU (COP1) and clear the interrupt-pending flag.   \n\
        li $v0, 0x20000000                                              \n\
        mtc0 $v0, $12      # Status                                     \n\
        mtc0 $zero, $13    # Cause                                      \n\
                                                                        \n\
        # Wait for the hardware to become ready.                        \n\
        li $k0, 0xBCC00000                                              \n\
        li $v0, 1                                                       \n\
        sw $v0, 0x10($k0)                                               \n\
1:      lw $v0, 0x10($k0)                                               \n\
        andi $v0, $v0, 1                                                \n\
        bnez $v0, 1b                                                    \n\
        nop                                                             \n\
                                                                        \n\
        # Set up more hardware registers.                               \n\
        li $v0, 1                                                       \n\
        sw $v0, 0x70($k0)                                               \n\
        li $v0, 8                                                       \n\
        sw $v0, 0x30($k0)                                               \n\
        li $v0, 2  # Assume 64MB for now (watch your pointers!)         \n\
        sw $v0, 0x40($k0)                                               \n\
        sync                                                            \n\
                                                                        \n\
        # Initialize the stack pointer and call the main loop.          \n\
        li $sp, 0x80200000                                              \n\
        lui $ra, %hi(me_loop)                                           \n\
        addiu $ra, %lo(me_loop)                                         \n\
        jr $ra                                                          \n\
        nop                                                             \n\
                                                                        \n\
        .globl me_init_end                                              \n\
me_init_end:                                                            \n\
                                                                        \n\
        .set pop                                                        \n\
");

/*************************************************************************/

/**
 * me_loop:  Media Engine main loop.  Infinitely repeats the cycle of
 * waiting for an execute request from the main CPU, then executing the
 * requested function.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Does not return
 * [Notes]
 *     This is not declared "static" because the address must be referenced
 *     by the assembly code in me_init().
 */
__attribute__((noreturn)) void me_loop(void)
{
    for (;;) {
        message_block->idle = 1;
        while (!message_block->request) { /*spin*/ }
        message_block->idle = 0;
        int (*function)(void *) = message_block->function;
        void *argument = message_block->argument;
        message_block->request = 0;
        message_block->result = (*function)(argument);
    }
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
