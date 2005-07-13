/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// SH2 Interpreter Core

#include "sh2core.h"
#include "sh2int.h"

SH2Interface_struct SH2Interpreter = {
   SH2CORE_INTERPRETER,
   "SH2 Interpreter",
   SH2InterpreterInit,
   SH2InterpreterDeInit,
   SH2InterpreterReset,
   SH2InterpreterExec
};

//////////////////////////////////////////////////////////////////////////////

int SH2InterpreterInit() {
   // Initialize any internal variables here
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2InterpreterDeInit() {
   // DeInitialize any internal variables here
}

//////////////////////////////////////////////////////////////////////////////

int SH2InterpreterReset() {
   // Reset any internal variables here
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

FASTCALL u32 SH2InterpreterExec(SH2_struct *context, u32 cycles) {
   // Execute instructions here
   // onchip stuff is handled by sh2core
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
