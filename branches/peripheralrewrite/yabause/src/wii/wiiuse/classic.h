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
 *	$Header: /home/guillaume/projects/soupcon/rrmerge/cvsdump/yabause/src/wii/wiiuse/classic.h,v 1.1.2.1 2008-05-04 00:45:14 cyberwarriorx Exp $
 *
 */

/**
 *	@file
 *	@brief Classic controller expansion device.
 */

#ifndef CLASSIC_H_INCLUDED
#define CLASSIC_H_INCLUDED

#include "wiiuse_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int classic_ctrl_handshake(struct wiimote_t* wm, struct classic_ctrl_t* cc, byte* data, unsigned short len);

void classic_ctrl_disconnected(struct classic_ctrl_t* cc);

void classic_ctrl_event(struct classic_ctrl_t* cc, byte* msg);

#ifdef __cplusplus
}
#endif

#endif // CLASSIC_H_INCLUDED