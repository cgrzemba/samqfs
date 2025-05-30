/* Parse dates for touch.
   Copyright (C) 1989, 1990, 1991 Free Software Foundation Inc.

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

/* Written by Jim Kingdon and David MacKenzie. */
%{

#ifdef HAVE_CONFIG_H
#if defined (CONFIG_BROKETS)
/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because it found this file in $srcdir).  */
#include <config.h>
#else
#include "config.h"
#endif
#endif

/* The following block of alloca-related preprocessor directives is here
   solely to allow compilation by non GNU-C compilers of the C parser
   produced from this file by old versions of bison.  Newer versions of
   bison include a block similar to this one in bison.simple.  */
   
#ifdef __GNUC__
#undef alloca
#define alloca __builtin_alloca
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#ifdef _AIX
 #pragma alloca
#else
void *alloca ();
#endif
#endif
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

/* Some old versions of bison generate parsers that use bcopy.
   That loses on systems that don't provide the function, so we have
   to redefine it here.  */
#if !defined (HAVE_BCOPY) && defined (HAVE_MEMCPY) && !defined (bcopy)
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif

#define YYDEBUG 1

/* Lexical analyzer's current scan position in the input string. */
static char *curpos;

/* The return value. */
static struct tm t;

time_t mktime ();

#define yyparse posixtime_yyparse
int yyparse();
static int yylex ();
static int yyerror ();
%}

%token DIGIT

%%
date :
       digitpair /* month */
       digitpair /* day */
       digitpair /* hours */
       digitpair /* minutes */
       year
       seconds {
	         if ($1 >= 1 && $1 <= 12)
		   t.tm_mon = $1 - 1;
		 else {
		   YYABORT;
		 }
		 if ($2 >= 1 && $2 <= 31)
		   t.tm_mday = $2;
		 else {
		   YYABORT;
		 }
		 if ($3 >= 0 && $3 <= 23)
		   t.tm_hour = $3;
		 else {
		   YYABORT;
		 }
		 if ($4 >= 0 && $4 <= 59)
		   t.tm_min = $4;
		 else {
		   YYABORT;
		 }
	       }

year : digitpair {
                   t.tm_year = $1;
		   /* Deduce the century based on the year.
		      See POSIX.2 section 4.63.3.  */
		   if ($1 <= 68)
		     t.tm_year += 100;
		 }
    | digitpair digitpair {
                            t.tm_year = $1 * 100 + $2;
			    if (t.tm_year < 1900) {
			      YYABORT;
			    } else
			      t.tm_year -= 1900;
			  }
    | /* empty */ {
                    time_t now;
		    struct tm *tmp;

                    /* Use current year.  */
                    time (&now);
		    tmp = localtime (&now);
		    t.tm_year = tmp->tm_year;
		  }
    ;

seconds : /* empty */ {
                        t.tm_sec = 0;
		      }
        | '.' digitpair {
	                  if ($2 >= 0 && $2 <= 61)
			    t.tm_sec = $2;
			  else {
			    YYABORT;
			  }
			}
        ;

digitpair : DIGIT DIGIT {
                          $$ = $1 * 10 + $2;
			}
          ;
%%
static int
yylex ()
{
  char ch = *curpos++;

  if (ch >= '0' && ch <= '9')
    {
      yylval = ch - '0';
      return DIGIT;
    }
  else if (ch == '.' || ch == 0)
    return ch;
  else
    return '?';			/* Cause an error.  */
}

static int
yyerror ()
{
  return 0;
}

/* Parse a POSIX-style date and return it, or (time_t)-1 for an error.  */

time_t
posixtime (s)
     char *s;
{
  curpos = s;
  /* Let mktime decide whether it is daylight savings time.  */
  t.tm_isdst = -1;
  if (yyparse ())
    return (time_t)-1;
  else
    return mktime (&t);
}

/* Parse a POSIX-style date and return it, or NULL for an error.  */

struct tm *
posixtm (s)
     char *s;
{
  if (posixtime (s) == -1)
    return NULL;
  return &t;
}
