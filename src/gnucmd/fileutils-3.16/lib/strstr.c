/* Copyright (C) 1994 Free Software Foundation, Inc.
This file is part of the GNU C Library.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* For the avoidance of doubt, except that if any license choice other
than GPL or LGPL is available it will apply instead, Sun elects to
use only the General Public License version 2 (GPLv2) at this time
for any software where a choice of GPL license versions is made
available with the language indicating that GPLv2 or any later
version may be used, or where a choice of which version of the GPL
is applied is otherwise unspecified. */

/*
 * My personal strstr() implementation that beats most other algorithms.
 * Until someone tells me otherwise, I assume that this is the
 * fastest implementation of strstr() in C.
 * I deliberately chose not to comment it.  You should have at least
 * as much fun trying to understand it, as I had to write it :-).
 *
 * Stephen R. van den Berg, berg@pool.informatik.rwth-aachen.de	*/

#include <string.h>
#include <sys/types.h>

typedef unsigned chartype;

char *
strstr(const char *phaystack, const char *pneedle)
{
  register const unsigned char *haystack, *needle;
  register chartype b, c;

  haystack = (const unsigned char *) phaystack;
  needle = (const unsigned char *) pneedle;

  b = *needle;
  if (b != '\0')
    {
      haystack--;				/* possible ANSI violation */
      do
	{
	  c = *++haystack;
	  if (c == '\0')
	    goto ret0;
	}
      while (c != b);

      c = *++needle;
      if (c == '\0')
	goto foundneedle;
      ++needle;
      goto jin;

      for (;;)
        {
          register chartype a;
	  register const unsigned char *rhaystack, *rneedle;

	  do
	    {
	      a = *++haystack;
	      if (a == '\0')
		goto ret0;
	      if (a == b)
		break;
	      a = *++haystack;
	      if (a == '\0')
		goto ret0;
shloop:	    }
          while (a != b);

jin:	  a = *++haystack;
	  if (a == '\0')
	    goto ret0;

	  if (a != c)
	    goto shloop;

	  rhaystack = haystack-- + 1;
	  rneedle = needle;
	  a = *rneedle;

	  if (*rhaystack == a)
	    do
	      {
		if (a == '\0')
		  goto foundneedle;
		++rhaystack;
		a = *++needle;
		if (*rhaystack != a)
		  break;
		if (a == '\0')
		  goto foundneedle;
		++rhaystack;
		a = *++needle;
	      }
	    while (*rhaystack == a);

	  needle = rneedle;		   /* took the register-poor aproach */

	  if (a == '\0')
	    break;
        }
    }
foundneedle:
  return (char*) haystack;
ret0:
  return 0;
}
