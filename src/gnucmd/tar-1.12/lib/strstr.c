/* strstr.c -- return the offset of one string within another
   Copyright (C) 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* For the avoidance of doubt, except that if any license choice other
   than GPL or LGPL is available it will apply instead, Sun elects to
   use only the General Public License version 2 (GPLv2) at this time
   for any software where a choice of GPL license versions is made
   available with the language indicating that GPLv2 or any later
   version may be used, or where a choice of which version of the GPL
   is applied is otherwise unspecified. */

/* Written by Mike Rendell <michael@cs.mun.ca>.  */

/* Return the starting address of string S2 in S1;
   return 0 if it is not found. */

char *
strstr(char *s1, char *s2)
{
  int i;
  char *p1;
  char *p2;
  char *s = s1;

  for (p2 = s2, i = 0; *s; p2 = s2, i++, s++)
    {
      for (p1 = s; *p1 && *p2 && *p1 == *p2; p1++, p2++)
	;
      if (!*p2)
	break;
    }
  if (!*p2)
    return s1 + i;

  return 0;
}
