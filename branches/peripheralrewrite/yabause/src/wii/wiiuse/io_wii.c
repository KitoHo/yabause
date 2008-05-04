/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header: /home/guillaume/projects/soupcon/rrmerge/cvsdump/yabause/src/wii/wiiuse/io_wii.c,v 1.1.2.1 2008-05-04 00:45:15 cyberwarriorx Exp $
 *
 */

/**
 *	@file
 *	@brief Handles device I/O for wii.
 */

#ifdef GEKKO

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "definitions.h"
#include "wiiuse_internal.h"
#include "events.h"
#include "io.h"

static int wiiuse_connect_single(struct wiimote_t* wm, char* address);

/**
 *	@brief Find a wiimote or wiimotes.
 *
 *	@param wm			An array of wiimote_t structures.
 *	@param max_wiimotes	The number of wiimote structures in \a wm.
 *	@param timeout		The number of seconds before the search times out.
 *
 *	@return The number of wiimotes found.
 *
 *	@see wiimote_connect()
 *
 *	This function will only look for wiimote devices.						\n
 *	When a device is found the address in the structures will be set.		\n
 *	You can then call wiimote_connect() to connect to the found				\n
 *	devices.
 */
int wiiuse_find(struct wiimote_t** wm, int max_wiimotes, int timeout) {
	int found_devices;
	int found_wiimotes;

	/* reset all wiimote bluetooth device addresses */
	for (found_wiimotes = 0; found_wiimotes < max_wiimotes; ++found_wiimotes) {
		int i = 0;
		for(; i < BD_ADDR_LEN; i++)
			wm[found_wiimotes]->bdaddr.addr[i] = 0;
	}
	found_wiimotes = 0;

	struct inquiry_info scan_info_arr[128];
	struct inquiry_info* scan_info = scan_info_arr;
	memset(&scan_info_arr, 0, sizeof(scan_info_arr));

	/* scan for bluetooth devices */
	found_devices = bte_inquiry(scan_info, 128, 1);
	if (found_devices < 0) {
		perror("bte_inquiry");
		return 0;
	}

	WIIUSE_INFO("Found %i bluetooth device(s).", found_devices);

	int i = 0;

	/* display discovered devices */
	for (; (i < found_devices) && (found_wiimotes < max_wiimotes); ++i) {
		if ((scan_info[i].cod[0] == WM_DEV_CLASS_0) &&
			(scan_info[i].cod[1] == WM_DEV_CLASS_1) &&
			(scan_info[i].cod[2] == WM_DEV_CLASS_2))
		{
			/* found a device */
			sprintf(wm[i]->bdaddr_str, "%x:%x:%x:%x:%x:%x",
				scan_info[i].bdaddr.addr[0],
				scan_info[i].bdaddr.addr[1],
				scan_info[i].bdaddr.addr[2],
				scan_info[i].bdaddr.addr[3],
				scan_info[i].bdaddr.addr[4],
				scan_info[i].bdaddr.addr[5]);

			WIIUSE_INFO("Found wiimote (%s) [id %i].", wm[found_wiimotes]->bdaddr_str, wm[found_wiimotes]->unid);

			wm[found_wiimotes]->bdaddr = scan_info[i].bdaddr;
			WIIMOTE_ENABLE_STATE(wm[found_wiimotes], WIIMOTE_STATE_DEV_FOUND);
			++found_wiimotes;
		}
	}

	return found_wiimotes;
}


/**
 *	@brief Connect to a wiimote or wiimotes once an address is known.
 *
 *	@param wm			An array of wiimote_t structures.
 *	@param wiimotes		The number of wiimote structures in \a wm.
 *
 *	@return The number of wiimotes that successfully connected.
 *
 *	@see wiiuse_find()
 *	@see wiiuse_connect_single()
 *	@see wiiuse_disconnect()
 *
 *	Connect to a number of wiimotes when the address is already set
 *	in the wiimote_t structures.  These addresses are normally set
 *	by the wiiuse_find() function, but can also be set manually.
 */
int wiiuse_connect(struct wiimote_t** wm, int wiimotes) {
	int connected = 0;
	int i = 0;

	for (; i < wiimotes; ++i) {
		if (!WIIMOTE_IS_SET(wm[i], WIIMOTE_STATE_DEV_FOUND))
			/* if the device address is not set, skip it */
			continue;

		if (wiiuse_connect_single(wm[i], NULL))
			++connected;
	}

	return connected;
}


/**
 *	@brief Connect to a wiimote with a known address.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param address	The address of the device to connect to.
 *					If NULL, use the address in the struct set by wiiuse_find().
 *
 *	@return 1 on success, 0 on failure
 */
static int wiiuse_connect_single(struct wiimote_t* wm, char* address) {
	struct bd_addr addr;

	if (!wm || WIIMOTE_IS_CONNECTED(wm))
		return 0;

	if (address) {
		/* use provided address */
		/* todo: this? */
	} else
		/* use address of device discovered */
		addr = wm->bdaddr;

	/*
	 *	OUTPUT CHANNEL
	 */
	wm->out_sock = bte_new();
	if (wm->out_sock == NULL)
		return 0;

	bte_arg(wm->out_sock, wm);

	/* connect to wiimote */
	if (bte_connect(wm->out_sock, &addr, htons(WM_OUTPUT_CHANNEL), NULL) < 0) {
		perror("bte_connect() output sock");
		wm->out_sock = NULL;
		return 0;
	}

	/*
	 *	INPUT CHANNEL
	 */
	wm->in_sock = bte_new();
	if (wm->in_sock == NULL)
		return 0;

	bte_arg(wm->in_sock, wm);

	/* connect to wiimote */
	if (bte_connect(wm->in_sock, &addr, htons(WM_INPUT_CHANNEL), wiiuse_recv) < 0) {
		perror("bte_connect() input sock");
		wm->out_sock = NULL;
		wm->in_sock = NULL;
		return 0;
	}

	WIIUSE_INFO("Connected to wiimote [id %i].", wm->unid);

	/* do the handshake */
	WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_CONNECTED);
	wiiuse_handshake(wm, NULL, 0);

	wiiuse_set_report_type(wm);

	return 1;
}


/**
 *	@brief Disconnect a wiimote.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	@see wiiuse_connect()
 *
 *	Note that this will not free the wiimote structure.
 */
void wiiuse_disconnect(struct wiimote_t* wm) {
	if (!wm || WIIMOTE_IS_CONNECTED(wm))
		return;

	/* disconnect!? */

	wm->out_sock = NULL;
	wm->in_sock = NULL;
	wm->event = WIIUSE_NONE;

	WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_CONNECTED);
	WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_HANDSHAKE);
}


int wiiuse_io_read(struct wiimote_t* wm) {
	/* not used */
	return 0;
}


int wiiuse_io_write(struct wiimote_t* wm, byte* buf, int len) {
	if(wm->out_sock)
		return bte_sendmessage(wm->out_sock, buf, len);
	else
		return ERR_CONN;
}

#endif /* ifndef WIN32 */
