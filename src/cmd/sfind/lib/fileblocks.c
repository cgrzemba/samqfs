/* Convert file size to number of blocks on System V-like machines.
   Copyright (C) 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* For the avoidance of doubt, except that if any license choice other
   than GPL or LGPL is available it will apply instead, Sun elects to
   use only the General Public License version 2 (GPLv2) at this time
   for any software where a choice of GPL license versions is made
   available with the language indicating that GPLv2 or any later
   version may be used, or where a choice of which version of the GPL
   is applied is otherwise unspecified. */

/* Written by Brian L. Matthews, blm@6sceng.UUCP. */

#if !defined (HAVE_ST_BLOCKS) && !defined(_POSIX_SOURCE)
#include <sys/types.h>
#include <sys/param.h>
#include "fileblocks.h"
#ifdef linux
#include <sam/linux_types.h>	/* u_longlong_t */
#endif /* linux */

#ifndef NINDIR
/* Some SysV's, like Irix, seem to lack these.  Hope they're correct. */
/* Size of a indirect block, in bytes. */
#define BSIZE 1024

/* Number of inode pointers per indirect block. */
#define NINDIR (BSIZE/sizeof(daddr_t))
#endif /* !NINDIR */

/* Number of direct block addresses in an inode. */
#define NDIR	10

/* Return the number of 512-byte blocks in a file of SIZE bytes. */

long
st_blocks (size)
     u_longlong_t size;
{
  long datablks = (size + 512 - 1) / 512;
  long indrblks = 0;

  if (datablks > NDIR)
    {
      indrblks = (datablks - NDIR - 1) / NINDIR + 1;

      if (datablks > NDIR + NINDIR)
	{
	  indrblks += (datablks - NDIR - NINDIR - 1) / (NINDIR * NINDIR) + 1;

	  if (datablks > NDIR + NINDIR + NINDIR * NINDIR)
	    indrblks++;
	}
    }

  return datablks + indrblks;
}
#endif
