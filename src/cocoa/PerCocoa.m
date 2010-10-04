/*  Copyright 2010 Lawrence Sebald

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

#include "PerCocoa.h"
#include "yabause.h"

#include <AppKit/NSEvent.h>

/* Forward Declarations. */
static int PERCocoaInit(void);
static void PERCocoaDeInit(void);
static int PERCocoaHandleEvents(void);
static void PERCocoaNothing(void);

static u32 PERCocoaScan(void);
static void PERCocoaFlush(void);

PerInterface_struct PERCocoa = {
    PERCORE_COCOA,
    "Cocoa Keyboard Input Interface",
    &PERCocoaInit,
    &PERCocoaDeInit,
    &PERCocoaHandleEvents,
    &PERCocoaNothing,
    &PERCocoaScan,
    0,
    &PERCocoaFlush
};

static int PERCocoaInit(void) {
    PerPad_struct *p = PerPadAdd(&PORTDATA1);

    PerSetKey((u32)NSUpArrowFunctionKey, PERPAD_UP, p);
    PerSetKey((u32)NSDownArrowFunctionKey, PERPAD_DOWN, p);
    PerSetKey((u32)NSLeftArrowFunctionKey, PERPAD_LEFT, p);
    PerSetKey((u32)NSRightArrowFunctionKey, PERPAD_RIGHT, p);
    PerSetKey((u32)'q', PERPAD_LEFT_TRIGGER, p);
    PerSetKey((u32)'p', PERPAD_RIGHT_TRIGGER, p);
    PerSetKey((u32)'\r', PERPAD_START, p);
    PerSetKey((u32)'a', PERPAD_A, p);
    PerSetKey((u32)'s', PERPAD_B, p);
    PerSetKey((u32)'d', PERPAD_C, p);
    PerSetKey((u32)'q', PERPAD_X, p);
    PerSetKey((u32)'w', PERPAD_Y, p);
    PerSetKey((u32)'e', PERPAD_Z, p);
    return 0;
}

static void PERCocoaDeInit(void) {
}

static int PERCocoaHandleEvents(void) {
    return YabauseExec();
}

static void PERCocoaNothing(void) {
}

static u32 PERCocoaScan(void) {
    return 0;
}

static void PERCocoaFlush(void) {
}
