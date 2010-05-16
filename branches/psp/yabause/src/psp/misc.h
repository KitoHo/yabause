/*  src/psp/misc.h: PSP support routine header
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

#ifndef PSP_MISC_H
#define PSP_MISC_H

/*************************************************************************/

/**
 * save_backup_ram:  Save the contents of backup RAM to the configured
 * file.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero on success, zero on failure
 */
extern int save_backup_ram(void);

/**
 * psp_writeback_cache_for_scsp:  Write back all dirty data from the SC's
 * cache for an ScspExec() call, depending on the writeback frequency
 * selected by the user.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero if writeback was skipped, zero if writeback was executed
 */
extern int psp_writeback_cache_for_scsp(void);

/*************************************************************************/

#endif  // PSP_MISC_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
