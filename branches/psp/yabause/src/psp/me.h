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

#ifndef ME_H
#define ME_H

/*************************************************************************/

/* Error codes specific to Media Engine routines */

enum {
    /* Media Engine has not been successfully started with meStart() */
    ME_ERROR_NOT_STARTED = 0x90000001,
    /* Media Engine is currently executing a function */
    ME_ERROR_BUSY = 0x90000002,
};

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
extern int meStart(void);

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
extern void meStop(void);

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
extern int meCall(int (*function)(void *), void *argument);

/**
 * mePoll:  Check whether the Media Engine is idle.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero if the Media Engine is idle, otherwise an error code (negative)
 */
extern int mePoll(void);

/**
 * meWait:  Wait for the Media Engine to become idle.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero if the Media Engine has become (or was already) idle,
 *     otherwise an error code (negative)
 */
extern int meWait(void);

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
extern int meResult(void);

/*-----------------------------------------------------------------------*/

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
extern void meIcacheInvalidateAll(void);

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
extern void meDcacheInvalidateAll(void);

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
extern void meDcacheWritebackInvalidateAll(void);

/*************************************************************************/

#endif  // ME_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
