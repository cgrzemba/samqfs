/* path-concat.c -- concatenate two arbitrary pathnames
   Copyright (C) 1996 Free Software Foundation, Inc.

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

/* Written by Jim Meyering.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
char *malloc ();
#endif
#include <string.h>

char *stpcpy ();

/* Concatenate two pathname components, DIR and BASE, in newly-allocated
   storage and return the result.  Return 0 if out of memory.  Add a slash
   between DIR and BASE in the result if neither would contribute one.
   If each would contribute at least one, elide one from the end of DIR.
   Otherwise, simply concatenate DIR and BASE.  In any case, if
   BASE_IN_RESULT is non-NULL, set *BASE_IN_RESULT to point to the copy of
   BASE in the returned concatenation.  */

char *
path_concat (dir, base, base_in_result)
     const char *dir;
     const char *base;
     char **base_in_result;
{
  char *p;
  char *p_concat;

  p_concat = malloc (strlen (dir) + strlen (base) + 2);
  if (!p_concat)
    return 0;

  p = stpcpy (p_concat, dir);

  if (*(p - 1) == '/' && *base == '/')
    --p;
  else if (*(p - 1) != '/' && *base != '/')
    p = stpcpy (p, "/");

  if (base_in_result)
    *base_in_result = p;

  stpcpy (p, base);

  return p_concat;
}
