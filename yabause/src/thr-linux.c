/*  src/thr-linux.c: Thread functions for Linux
    Copyright 2010 Andrew Church

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

#include "core.h"
#include "threads.h"

#include <pthread.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////////////

// Thread handles for each Yabause subthread
static pthread_t thread_handle[YAB_NUM_THREADS];

//////////////////////////////////////////////////////////////////////////////

int YabThreadStart(unsigned int id, void (*func)(void))
{
   if (thread_handle[id])
   {
      fprintf(stderr, "YabThreadStart: thread %u is already started!\n", id);
      return -1;
   }

   if ((errno = pthread_create(&thread_handle[id], NULL, (void *)func, NULL)) != 0)
   {
      perror("pthread_create");
      return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadWait(unsigned int id)
{
   if (!thread_handle[id])
      return;  // Thread wasn't running in the first place

   pthread_join(thread_handle[id], NULL);

   thread_handle[id] = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadYield(void)
{
   sched_yield();
}

//////////////////////////////////////////////////////////////////////////////
