/*  src/psp/psp-per.c: PSP peripheral interface module
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

#include "common.h"

#include "../peripheral.h"

#include "control.h"
#include "psp-per.h"

/*************************************************************************/
/************************* Interface definition **************************/
/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_per_init(void);
static void psp_per_deinit(void);
static int psp_per_handle_events(void);
#ifdef PERKEYNAME
static void psp_per_key_name(u32 key, char *name, int size);
#endif

/*-----------------------------------------------------------------------*/

/* Module interface definition */

PerInterface_struct PERPSP = {
    .id           = PERCORE_PSP,
    .Name         = "PSP Peripheral Interface",
    .Init         = psp_per_init,
    .DeInit       = psp_per_deinit,
    .HandleEvents = psp_per_handle_events,
    .canScan      = 0,
#ifdef PERKEYNAME
    .KeyName      = psp_per_key_name,
#endif
};

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * psp_per_init:  Initialize the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_per_init(void)
{
    /* Nothing to do */
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_per_deinit:  Shut down the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_per_deinit(void)
{
    /* Nothing to do */
}

/*************************************************************************/

/**
 * psp_per_handle_events:  Process pending peripheral events, and run one
 * iteration of the emulation.
 *
 * For the PSP, the main loop is located in main.c; we only update the
 * current button status here.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_per_handle_events(void)
{
    static uint32_t last_buttons;

#if 1
static int frame;
if(frame==140)PerKeyDown(PSP_CTRL_START);
if(frame==170)PerKeyUp(PSP_CTRL_START);
if(frame==580)PerKeyDown(PSP_CTRL_START);
if(frame==583)PerKeyUp(PSP_CTRL_START);
if(frame==730)PerKeyDown(PSP_CTRL_START);
if(frame==733)PerKeyUp(PSP_CTRL_START);
if(frame==740)PerKeyDown(PSP_CTRL_START);
if(frame==743)PerKeyUp(PSP_CTRL_START);
if(frame==840)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==843)PerKeyUp(PSP_CTRL_CIRCLE);
// 1st file load
/*if(frame==850)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==853)PerKeyUp(PSP_CTRL_CIRCLE);*/
// 2nd file load
if(frame==850)PerKeyDown(PSP_CTRL_DOWN);
if(frame==853)PerKeyUp(PSP_CTRL_DOWN);
if(frame==860)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==863)PerKeyUp(PSP_CTRL_CIRCLE);
// back to water and to zako battle
/*if(frame==1050)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==1155)PerKeyDown(PSP_CTRL_CROSS);
if(frame==1160)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==1223)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1235)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1287)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1298)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1325)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1345)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1525)PerKeyUp(PSP_CTRL_CROSS);
if(frame==1835)PerKeyDown(PSP_CTRL_CROSS);
if(frame==1893)PerKeyDown(PSP_CTRL_DOWN);
if(frame==1906)PerKeyUp(PSP_CTRL_DOWN);
if(frame==2131)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2139)PerKeyUp(PSP_CTRL_LEFT);
if(frame==2167)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2175)PerKeyUp(PSP_CTRL_LEFT);
if(frame==2383)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2400)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2427)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2443)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2559)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2575)PerKeyUp(PSP_CTRL_LEFT);
if(frame==2638)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2643)PerKeyUp(PSP_CTRL_LEFT);
if(frame==2743)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2750)PerKeyUp(PSP_CTRL_LEFT);
if(frame==3063)PerKeyDown(PSP_CTRL_LEFT);
if(frame==3079)PerKeyUp(PSP_CTRL_LEFT);
if(frame==3260)PerKeyUp(PSP_CTRL_CROSS);
if(frame==3825)PerKeyDown(PSP_CTRL_LEFT);
if(frame==3847)PerKeyUp(PSP_CTRL_LEFT);*/
// through 1st boss battle
/*if(frame==1050)PerKeyDown(PSP_CTRL_CROSS);
if(frame==1147)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1156)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1178)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1185)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1206)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1213)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1224)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1232)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1241)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1248)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1264)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1267)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1284)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1292)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1306)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1315)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1333)PerKeyDown(PSP_CTRL_LEFT);
if(frame==1343)PerKeyUp(PSP_CTRL_LEFT);
if(frame==1488)PerKeyUp(PSP_CTRL_CROSS);
if(frame==1677)PerKeyDown(PSP_CTRL_START);
if(frame==1694)PerKeyUp(PSP_CTRL_START);
if(frame==2288)PerKeyDown(PSP_CTRL_LEFT);
if(frame==2305)PerKeyUp(PSP_CTRL_LEFT);
if(frame==2535)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2613)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2760)PerKeyDown(PSP_CTRL_START);
if(frame==2766)PerKeyUp(PSP_CTRL_START);
if(frame==2805)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==2810)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==2825)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2829)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2836)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2841)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2845)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==2849)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==2855)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==2860)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==3509)PerKeyDown(PSP_CTRL_LEFT);
if(frame==3603)PerKeyUp(PSP_CTRL_LEFT);
if(frame==3835)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==3927)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==4023)PerKeyDown(PSP_CTRL_START);
if(frame==4028)PerKeyUp(PSP_CTRL_START);
if(frame==4069)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==4075)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==4090)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==4095)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==4099)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==4103)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==4107)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==4111)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==4117)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==4122)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==4822)PerKeyDown(PSP_CTRL_LEFT);
if(frame==4929)PerKeyUp(PSP_CTRL_LEFT);
if(frame==5153)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==5259)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==5274)PerKeyDown(PSP_CTRL_START);
if(frame==5329)PerKeyUp(PSP_CTRL_START);
if(frame==5345)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==5353)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==5358)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==5363)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==5367)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==5371)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==5376)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==5379)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==5384)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==5389)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==6588)PerKeyDown(PSP_CTRL_CROSS);
if(frame==8499)PerKeyUp(PSP_CTRL_CROSS);
if(frame==8896)PerKeyDown(PSP_CTRL_CROSS);
if(frame==10201)PerKeyUp(PSP_CTRL_CROSS);
// and to the campfire
if(frame==12511)PerKeyDown(PSP_CTRL_START);
if(frame==12528)PerKeyUp(PSP_CTRL_START);
if(frame==12829)PerKeyDown(PSP_CTRL_RIGHT);
if(frame==12832)PerKeyUp(PSP_CTRL_RIGHT);
if(frame==12886)PerKeyDown(PSP_CTRL_CIRCLE);
if(frame==12889)PerKeyUp(PSP_CTRL_CIRCLE);
if(frame==13211)PerKeyDown(PSP_CTRL_START);
if(frame==13215)PerKeyUp(PSP_CTRL_START);*/
frame++;
#endif
    const uint32_t buttons = control_state();
    const uint32_t changed_buttons = buttons ^ last_buttons;
//if(changed_buttons)printf("frame %d: +%04X -%04X\n",frame-1,changed_buttons&buttons,changed_buttons&~buttons);
    last_buttons = buttons;

    int i;
    for (i = 0; i < 16; i++) {
        const uint32_t button = 1 << i;
        if (changed_buttons & button) {
            if (buttons & button) {
                PerKeyDown(button);
            } else {
                PerKeyUp(button);
            }
        }
    }

    YabauseExec();
    return 0;
}

/*************************************************************************/

#ifdef PERKEYNAME

/**
 * psp_per_key_name:  Return the name corresponding to a system-dependent
 * key value.
 *
 * [Parameters]
 *      key: Key value to return name for
 *     name: Buffer into which name is to be stored
 *     size: Size of buffer in bytes
 * [Return value]
 *     None
 */
static void psp_per_key_name(u32 key, char *name, int size)
{
    /* Not supported on PSP */
    *name = 0;
}

#endif  // PERKEYNAME

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
