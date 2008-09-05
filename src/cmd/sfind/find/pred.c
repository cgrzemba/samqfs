/* pred.c -- execute the expression tree.
   Copyright (C) 1987, 1990, 1991 Free Software Foundation, Inc.

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

#ifdef linux
#include <stdio.h>
#endif /* linux */
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <stdlib.h>
#include <string.h>
#ifdef sun
#include <utility.h>
#endif /* sun */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <pwd.h>
#include <grp.h>

#ifndef _POSIX_VERSION
struct passwd *getpwuid ();
struct group *getgrgid ();
#endif

#include "wait.h"

#if defined(DIRENT) || defined(_POSIX_VERSION)
#include <dirent.h>
#define NLENGTH(direct) (strlen((direct)->d_name))
#else /* not (DIRENT or _POSIX_VERSION) */
#define dirent direct
#define NLENGTH(direct) ((direct)->d_namlen)
#ifdef SYSNDIR
#include <sys/ndir.h>
#endif /* SYSNDIR */
#ifdef SYSDIR
#include <sys/dir.h>
#endif /* SYSDIR */
#ifdef NDIR
#include <ndir.h>
#endif /* NDIR */
#endif /* DIRENT or _POSIX_VERSION */

#ifdef VOID_CLOSEDIR
/* Fake a return value. */
#define CLOSEDIR(d) (closedir (d), 0)
#else
#define CLOSEDIR(d) closedir (d)
#endif

/* Extract or fake data from a `struct stat'.
   ST_NBLOCKS: Number of 512-byte blocks in the file
   (including indirect blocks). */
#ifdef _POSIX_SOURCE
#define ST_NBLOCKS(statp) (((statp)->st_size + 512 - 1) / 512)
#else /* !_POSIX_SOURCE */
#ifndef HAVE_ST_BLOCKS
#include "fileblocks.h"
#define ST_NBLOCKS(statp) (st_blocks ((statp)->st_size))
#else /* HAVE_ST_BLOCKS */
#if defined(hpux) || defined(__hpux__)
/* HP-UX, perhaps uniquely, counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and 4BSD filesystems, though. */
#define ST_NBLOCKS(statp) ((statp)->st_blocks * 2)
#else /* !hpux */
#define ST_NBLOCKS(statp) ((statp)->st_blocks)
#endif /* !hpux */
#endif /* HAVE_ST_BLOCKS */
#endif /* !_POSIX_SOURCE */

#include <sys/stat.h>
#include <pub/stat.h>
#include <pub/rminfo.h>
#include "defs.h"

typedef	struct	predicate	Predicate;
typedef	enum	comparison_type	Comparison_type;

boolean pred_amin ();
boolean pred_and ();
boolean pred_anewer ();
boolean pred_atime ();
boolean pred_close ();
boolean pred_cmin ();
boolean pred_cnewer ();
boolean pred_comma ();
boolean pred_ctime ();
/* no pred_daystart */
/* no pred_depth */
boolean pred_empty ();
boolean pred_exec ();
boolean pred_false ();
boolean pred_fprint ();
boolean pred_fprint0 ();
boolean pred_fprintf ();
boolean pred_fstype ();
boolean pred_gid ();
boolean pred_group ();
boolean pred_ilname ();
boolean pred_iname ();
boolean pred_inum ();
boolean pred_ipath ();
/* no pred_iregex */
boolean pred_links ();
boolean pred_lname ();
boolean pred_ls ();
boolean pred_mmin ();
boolean pred_mtime ();
boolean pred_name ();
boolean pred_negate ();
boolean pred_newer ();
/* no pred_noleaf */
boolean pred_nogroup ();
boolean pred_nouser ();
boolean pred_ok ();
boolean pred_open ();
boolean pred_or ();
boolean pred_path ();
boolean pred_perm ();
boolean pred_archpos ();
boolean pred_print ();
boolean pred_print0 ();
/* no pred_printf */
boolean pred_prune ();
boolean pred_regex ();
boolean pred_size ();
boolean pred_true ();
boolean pred_type ();
boolean pred_uid ();
boolean pred_used ();
boolean pred_user ();
/* no pred_version */
/* no pred_xdev */
boolean pred_xtype ();

boolean pred_aid ();			/* SUN */
boolean pred_admin_id ();		/* SUN */
boolean pred_any_copy_d ();		/* SUN */
boolean pred_any_copy_p ();		/* SUN */
boolean pred_any_copy_r ();		/* SUN */
boolean pred_any_copy_v ();		/* SUN */
boolean pred_archived ();		/* SUN */
boolean pred_archive_d();		/* SUN */
boolean pred_archive_n();		/* SUN */
boolean pred_archdone();		/* SUN */
boolean pred_copies ();			/* SUN */
boolean pred_copy ();			/* SUN */
boolean pred_copy_d ();			/* SUN */
boolean pred_copy_r ();			/* SUN */
boolean pred_copy_v ();			/* SUN */
boolean pred_damaged ();		/* SUN */
boolean	pred_number_of_segments (char *pathname,			/* SUN */
									struct sam_stat *sb,
									Predicate *pred_ptr);
boolean pred_offline ();		/* SUN */
boolean pred_online ();			/* SUN */
boolean pred_partial_on();		/* SUN */
boolean pred_partial_on_segmented_file(char *pathname,		/* SUN */
										struct sam_stat *sb,
										Predicate *pred_ptr);
boolean pred_release_d();		/* SUN */
boolean pred_release_a();		/* SUN */
boolean pred_release_n();		/* SUN */
boolean pred_release_p();		/* SUN */
boolean pred_release_p_segmented_file(char *pathname,		/* SUN */
										struct sam_stat *sb,
										Predicate *pred_ptr);

boolean pred_rmin ();			/* SUN */
boolean pred_rtime ();			/* SUN */
boolean pred_sections();		/* SUN */
boolean pred_segment_a(char *pathname, struct sam_stat *sb,	/* SUN */
						Predicate *pred_ptr);
boolean pred_segment_i(char *pathname, struct sam_stat *sb,	/* SUN */
						Predicate *pred_ptr);
boolean pred_segment_s(char *pathname, struct sam_stat *sb,	/* SUN */
						Predicate *pred_ptr);
boolean pred_segment_number(char *pathname, struct sam_stat *sb,	/* SUN */
						Predicate *pred_ptr);
boolean pred_segmented(char *pathname, struct sam_stat *sb,	/* SUN */
						Predicate *pred_ptr);
boolean pred_is_setfa_g();		/* SUN */
boolean pred_is_setfa_s();		/* SUN */
boolean pred_setfa_g();			/* SUN */
boolean pred_setfa_s();			/* SUN */
boolean pred_ssum_g();			/* SUN */
boolean pred_ssum_u();			/* SUN */
boolean pred_ssum_v();			/* SUN */
boolean pred_stage_d();			/* SUN */
boolean pred_stage_a();			/* SUN */
boolean pred_stage_n();			/* SUN */
boolean pred_safe();			/* SUN */
boolean pred_xmin ();			/* SUN */
boolean pred_xtime ();			/* SUN */
boolean pred_verify ();			/* SUN */
boolean pred_vsn ();			/* SUN */
boolean pred_mt ();				/* SUN */
boolean pred_worm ();			/* SUN */
boolean pred_expired ();		/* SUN */
boolean pred_retention ();		/* SUN */
boolean pred_remain ();			/* SUN */
boolean pred_after ();			/* SUN */
boolean pred_project ();		/* SUN */
boolean pred_xattr ();			/* SUN */


boolean							/* SUN */
aggregate_test_over_segments(char *pathname, struct sam_stat *stat_buf,
							struct predicate *pred_ptr,
							enum pred_aggregate_type pred_aggregate_type_arg,
							PFB pred_fct);

boolean	compare_time(Comparison_type, unsigned long, unsigned long, int);
boolean launch ();
char *basename ();
char *format_date ();
char *filesystem_type ();
char *stpcpy ();
void list_file ();

static boolean insert_lname ();

#ifdef	DEBUG
struct pred_assoc
{
  PFB pred_func;
  char *pred_name;
};

struct pred_assoc pred_table[] =
{
  {pred_amin, "amin    "},
  {pred_and, "and     "},
  {pred_anewer, "anewer  "},
  {pred_atime, "atime   "},
  {pred_close, ")       "},
  {pred_cmin, "cmin    "},
  {pred_cnewer, "cnewer  "},
  {pred_comma, ",       "},
  {pred_ctime, "ctime   "},
  {pred_empty, "empty   "},
  {pred_exec, "exec    "},
  {pred_false, "false   "},
  {pred_fprint, "fprint  "},
  {pred_fprint0, "fprint0 "},
  {pred_fprintf, "fprintf "},
  {pred_fstype, "fstype  "},
  {pred_gid, "gid     "},
  {pred_group, "group   "},
  {pred_ilname, "ilname  "},
  {pred_iname, "iname   "},
  {pred_inum, "inum    "},
  {pred_ipath, "ipath   "},
  {pred_links, "links   "},
  {pred_lname, "lname   "},
  {pred_ls, "ls      "},
  {pred_mmin, "mmin    "},
  {pred_mtime, "mtime   "},
  {pred_name, "name    "},
  {pred_negate, "not     "},
  {pred_newer, "newer   "},
  {pred_nogroup, "nogroup "},
  {pred_nouser, "nouser  "},
  {pred_ok, "ok      "},
  {pred_open, "(       "},
  {pred_or, "or      "},
  {pred_path, "path    "},
  {pred_perm, "perm    "},
  {pred_archpos, "archpos "},
  {pred_print, "print   "},
  {pred_print0, "print0  "},
  {pred_prune, "prune   "},
  {pred_regex, "regex   "},
  {pred_size, "size    "},
  {pred_true, "true    "},
  {pred_type, "type    "},
  {pred_uid, "uid     "},
  {pred_used, "used    "},
  {pred_user, "user    "},
  {pred_verify, "verify  "},
  {pred_xtype, "xtype   "},

  {pred_aid, "aid     "},
  {pred_admin_id,   "admin_id  "},
  {pred_any_copy_d, "any_copy_d"},
  {pred_any_copy_p, "any_copy_p"},
  {pred_any_copy_r, "any_copy_r"},
  {pred_any_copy_v, "any_copy_v"},
  {pred_archived, "archived"},
  {pred_archive_d, "archive_d   "},
  {pred_archive_n, "archive_n   "},
  {pred_archdone, "archdone   "},
  {pred_copies, "copies"},
  {pred_copy, "copy"},
  {pred_copy_d, "copy_d"},
  {pred_copy_r, "copy_r"},
  {pred_copy_v,	"copy_v"},
  {pred_damaged, "damaged "},
  {pred_number_of_segments, "segments    "},
  {pred_offline, "offline "},
  {pred_online,	"online  "},
  {pred_partial_on, "partial_on   "},
  {pred_release_d, "release_d   "},
  {pred_release_a, "release_a   "},
  {pred_release_n, "release_n   "},
  {pred_release_p, "release_p   "},
  {pred_rmin, "rmin    "},
  {pred_rtime,"rtime   "},
  {pred_sections,"sections    " },
  {pred_segment_a, "segment_a   "},
  {pred_segment_s, "segment_s   "},
  {pred_segment_i, "segment_i   "},
  {pred_segment_number, "segment     "},
  {pred_segmented, "segmented   "},
  {pred_is_setfa_g, "is_setfa_g  "},
  {pred_is_setfa_s, "is_setfa_s  "},
  {pred_setfa_g, "setfa_g     "},
  {pred_setfa_s, "setfa_s     "},
  {pred_ssum_g, "ssum_g   "},
  {pred_ssum_u, "ssum_u   "},
  {pred_ssum_v, "ssum_v   "},
  {pred_stage_d, "stage_d   "},
  {pred_stage_a, "stage_a   "},
  {pred_stage_n, "stage_n   "},
  {pred_xmin, "xmin    "},
  {pred_xtime,"xtime   "},
  {pred_vsn,"vsn   "},
  {pred_mt,"mt   "},
  {pred_worm, "ractive"},
  {pred_expired, "rover"},
  {pred_retention, "retention"},
  {pred_remain, "rremain"},
  {pred_after, "rafter"},
  {pred_project, "project"},
  {pred_xattr, "xattr"},
  {0, "none    "}
};

struct op_assoc
{
  short type;
  char *type_name;
};

struct op_assoc type_table[] =
{
  {NO_TYPE, "no          "},
  {VICTIM_TYPE, "victim      "},
  {UNI_OP, "uni_op      "},
  {BI_OP, "bi_op       "},
  {OPEN_PAREN, "open_paren  "},
  {CLOSE_PAREN, "close_paren "},
  {-1, "unknown     "}
};

struct prec_assoc
{
  short prec;
  char *prec_name;
};

struct prec_assoc prec_table[] =
{
  {NO_PREC, "no      "},
  {COMMA_PREC, "comma   "},
  {OR_PREC, "or      "},
  {AND_PREC, "and     "},
  {NEGATE_PREC, "negate  "},
  {MAX_PREC, "max     "},
  {-1, "unknown "}
};

#endif	/* DEBUG */

/* Predicate processing routines.
 
   PATHNAME is the full pathname of the file being checked.
   *STAT_BUF contains information about PATHNAME.
   *PRED_PTR contains information for applying the predicate.
 
   Return true if the file passes this predicate, false if not. */

boolean
pred_amin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_atime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_atime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_and (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left == NULL
      || (*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
					    pred_ptr->pred_left))
    {
      /* Check whether we need a stat here. */
      if (pred_ptr->need_stat)
	{
	  if (!have_stat && (*StatFunc)(pathname, stat_buf, sizeof(stat_buf)) != 0)
	    {
	      (void) fflush (stdout);
	      error (0, errno, "StatFunc(%s)", pathname);
	      exit_status = 1;
	      return (false);
	    }
	  have_stat = true;
	}
      return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
						  pred_ptr->pred_right));
    }
  else
    return (false);
}

boolean
pred_anewer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_atime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_atime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_atime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_atime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_atime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_close (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_cmin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ctime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_ctime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_cnewer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_ctime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_comma (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left != NULL)
    (*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
				       pred_ptr->pred_left);
  /* Check whether we need a stat here. */
  if (pred_ptr->need_stat)
    {
      if (!have_stat && (*StatFunc)(pathname, stat_buf, sizeof(stat_buf)) != 0)
	{
	  (void) fflush (stdout);
	  error (0, errno, "StatFunc(%s)", pathname);
	  exit_status = 1;
	  return (false);
	}
      have_stat = true;
    }
  return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
					      pred_ptr->pred_right));
}

boolean
pred_ctime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ctime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_ctime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_ctime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_empty (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (S_ISDIR (stat_buf->st_mode))
    {
      DIR *d;
      struct dirent *dp;
      boolean empty = true;

      errno = 0;
      d = opendir (pathname);
      if (d == NULL)
	{
	  (void) fflush (stdout);
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      for (dp = readdir (d); dp; dp = readdir (d))
	{
	  if (dp->d_name[0] != '.'
	      || (dp->d_name[1] != '\0'
		  && (dp->d_name[1] != '.' || dp->d_name[2] != '\0')))
	    {
	      empty = false;
	      break;
	    }
	}
      if (CLOSEDIR (d))
	{
	  (void) fflush (stdout);
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	  return (false);
	}
      return (empty);
    }
  else if (S_ISREG (stat_buf->st_mode))
    return (stat_buf->st_size == 0);
  else
    return (false);
}

boolean
pred_exec (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  int i;
  int path_pos;
  struct exec_val *execp;	/* Pointer for efficiency. */

  execp = &pred_ptr->args.exec_vec;

  /* Replace "{}" with the real path in each affected arg. */
  for (path_pos = 0; execp->paths[path_pos].offset >= 0; path_pos++)
    {
      register char *from, *to;

      i = execp->paths[path_pos].offset;
      execp->vec[i] =
	xmalloc (strlen (execp->paths[path_pos].origarg) + 1
		 + (strlen (pathname) - 2) * execp->paths[path_pos].count);
      for (from = execp->paths[path_pos].origarg, to = execp->vec[i]; *from; )
	if (from[0] == '{' && from[1] == '}')
	  {
	    to = stpcpy (to, pathname);
	    from += 2;
	  }
	else
	  *to++ = *from++;
      *to = *from;		/* Copy null. */
    }

  i = launch (pred_ptr);

  /* Free the temporary args. */
  for (path_pos = 0; execp->paths[path_pos].offset >= 0; path_pos++)
    free (execp->vec[execp->paths[path_pos].offset]);

  return (i);
}

boolean
pred_false (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (false);
}

boolean
pred_fprint (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, pred_ptr->args.stream);
  putc ('\n', pred_ptr->args.stream);
  return (true);
}

boolean
pred_fprint0 (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, pred_ptr->args.stream);
  putc (0, pred_ptr->args.stream);
  return (true);
}

boolean
pred_fprintf (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  FILE *fp = pred_ptr->args.printf_vec.stream;
  struct segment *segment;
  static char out_buf[512];
  char *cp;
  char *path_for_fprintf;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_fprintf = current_pathname;
  } else {
    path_for_fprintf = pathname;
  }

  for (segment = pred_ptr->args.printf_vec.segment; segment;
       segment = segment->next)
    {
      if (segment->kind & 0xff00) /* Component of date. */
	{
	  time_t t;

	  switch (segment->kind & 0xff)
	    {
	    case 'A':
	      t = stat_buf->st_atime;
	      break;
	    case 'C':
	      t = stat_buf->st_ctime;
	      break;
	    case 'T':
	      t = stat_buf->st_mtime;
	      break;
	    }
	  fprintf (fp, segment->text,
		   format_date (t, (segment->kind >> 8) & 0xff));
	  continue;
	}

      switch (segment->kind)
	{
	case KIND_PLAIN:	/* Plain text string (no % conversion). */
	  fwrite (segment->text, 1, segment->text_len, fp);
	  break;
	case KIND_STOP:		/* Terminate argument (no newline). */
	  fwrite (segment->text, 1, segment->text_len, fp);
	  return (true);
	case 'a':		/* atime in `ctime' format. */
	  cp = ctime ((time_t *) &stat_buf->st_atime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'b':		/* size in 512-byte blocks */
	  fprintf (fp, segment->text, ((stat_buf->st_size + 511) / 512));
	  break;
	case 'Z':		/* Segment length setting */
	  {
	    if (SS_ISSEGMENT_F(stat_buf->attr) || SS_ISSEGMENT_S(stat_buf->attr) ||
	            (SS_ISSEGMENT_A(stat_buf->attr))) {
		  sprintf(out_buf,  "%u", stat_buf->segment_size);
	    } else {
	      out_buf[0] = '-';
	      out_buf[1] = '\0';
	    }
	    fprintf (fp, segment->text, out_buf);
	  }
	  break;
	case 'c':		/* ctime in `ctime' format */
	  cp = ctime ((time_t *) &stat_buf->st_ctime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'd':		/* depth in search tree */
	  fprintf (fp, segment->text, curdepth);
	  break;
	case 'r':
	  if (SS_ISSAMFS(stat_buf->attr) && SS_ISSETFA_G(stat_buf->attr)) {
	    sprintf(out_buf, "%u", (unsigned) stat_buf->stripe_group);
	  } else {
	    out_buf[0] = '-';
	    out_buf[1] = '\0';
	  }

	  fprintf (fp, segment->text, out_buf);
	  break;
	case 'w':
	  if (SS_ISSAMFS(stat_buf->attr) && SS_ISSETFA_S(stat_buf->attr)) {
	    sprintf(out_buf, "%u", (unsigned) stat_buf->stripe_width);
	  } else {
	    out_buf[0] = '-';
	    out_buf[1] = '\0';
	  }

	  fprintf (fp, segment->text, out_buf);
	  break;
	case 'f':		/* basename of path */
	  cp = rindex (path_for_fprintf, '/');
	  if (cp)
	    cp++;
	  else
	    cp = path_for_fprintf;
	  fprintf (fp, segment->text, cp);
	  break;
	case 'F':		/* filesystem type */
	  fprintf (fp, segment->text, filesystem_type (path_for_fprintf, stat_buf));
	  break;
	case 'g':		/* group name */
	  {
	    struct group *g;

	    g = getgrgid (stat_buf->st_gid);
	    if (g)
	      {
		segment->text[segment->text_len] = 's';
		fprintf (fp, segment->text, g->gr_name);
		break;
	      }
	    /* else fallthru */
	  }
	case 'G':		/* GID number */
	  segment->text[segment->text_len] = 'u';
	  fprintf (fp, segment->text, stat_buf->st_gid);
	  break;
	case 'h':		/* leading directories part of path */
	  {
	    char cc;

	    cp = rindex (path_for_fprintf, '/');
	    if (cp == NULL)	/* No leading directories. */
	      break;
	    cc = *cp;
	    *cp = '\0';
	    fprintf (fp, segment->text, path_for_fprintf);
	    *cp = cc;
	    break;
	  }
	case 'H':		/* ARGV element file was found under */
	  {
	    char cc = path_for_fprintf[path_length];

	    path_for_fprintf[path_length] = '\0';
	    fprintf (fp, segment->text, path_for_fprintf);
	    path_for_fprintf[path_length] = cc;
	    break;
	  }
	case 'i':		/* inode number */
	  fprintf (fp, segment->text, stat_buf->st_ino);
	  break;
	case 'K':		/* segment number */
	  /*
	   * If seg_num >= 0, then result of find that is being fprintf'ed is
	   * an index inode or a data segment, output the segment number in place
	   * of the user's %K argument.
	   *
	   * If seg_num < 0, then result of find that is being fprintf'ed is not
	   * an index inode nor a data segment, output a '-' in place of the
	   * of the user's %K argument.
	   */
	  {
	    if (SS_ISSEGMENT_S(stat_buf->attr) && stat_buf->segment_number > 0) {
	      /* Item is a data segment, output its segment number: */
	      sprintf (out_buf ,"%d", stat_buf->segment_number);
	    } else if (SS_ISSEGMENT_F(stat_buf->attr)) {
	      /* Item is an index inode, output 0 for its segment number: */
	      out_buf[0] = '0';
	      out_buf[1] = '\0';
	    } else {
	      /* Item is not a data segment and not an index inode, output a dash
	       * (-)
	       */
	      out_buf[0] = '-';
	      out_buf[1] = '\0';
	    }

	    fprintf (fp, segment->text, out_buf);
	  }
	  break;
	case 'Q':		/* Quantity of segments */
	  {
	    if (SS_ISSEGMENT_F(stat_buf->attr)) {
	      sprintf (out_buf, "%d", NUM_SEGS(stat_buf));
	    } else {
	      out_buf[0] = '-';
	      out_buf[1] = '\0';
	    }

	    fprintf (fp, segment->text, out_buf);
	  }
	  break;
	case 'k':		/* size in 1K blocks */
	  fprintf (fp, segment->text,
			((stat_buf->st_size + 1023) / 1024));
	  break;
	case 'l':		/* object of symlink */
#ifdef S_ISLNK
	  {
	    int linklen;
	    char *linkname;

	    if (!S_ISLNK (stat_buf->st_mode))
	      break;
#ifdef _AIX
#define LINK_BUF PATH_MAX
#else
#define LINK_BUF stat_buf->st_size
#endif
	    linkname = (char *) xmalloc (LINK_BUF + 1);
	    linklen = readlink (path_for_fprintf, linkname, LINK_BUF);
	    if (linklen < 0)
	      {
		(void) fflush (stdout);
		error (0, errno, "%s", path_for_fprintf);
		exit_status = 1;
		free (linkname);
		break;
	      }
	    linkname[linklen] = '\0';
	    fprintf (fp, segment->text, linkname);
	    free (linkname);
	  }
#endif				/* S_ISLNK */
	  break;
	case 'm':		/* mode as octal number (perms only) */
	  fprintf (fp, segment->text, stat_buf->st_mode & 07777);
	  break;
	case 'n':		/* number of links */
	  fprintf (fp, segment->text, stat_buf->st_nlink);
	  break;
	case 'p':		/* path_for_fprintf */
	  fprintf (fp, segment->text, path_for_fprintf);
	  break;
	case 'P':		/* path_for_fprintf with ARGV element stripped */
	  if (curdepth)
	    {
	      cp = path_for_fprintf + path_length;
	      if (*cp == '/')
		/* Move past the slash between the ARGV element
		   and the rest of the path_for_fprintf.  But if the ARGV element
		   ends in a slash, we didn't add another, so we've
		   already skipped past it.  */
		cp++;
	      fprintf (fp, segment->text, cp);
	    }
	  break;
	case 's':		/* size in bytes */
	  fprintf (fp, segment->text, stat_buf->st_size);
	  break;
	case 't':		/* mtime in `ctime' format */
	  cp = ctime ((time_t *) &stat_buf->st_mtime);
	  cp[24] = '\0';
	  fprintf (fp, segment->text, cp);
	  break;
	case 'u':		/* user name */
	  {
	    struct passwd *p;

	    p = getpwuid (stat_buf->st_uid);
	    if (p)
	      {
		segment->text[segment->text_len] = 's';
		fprintf (fp, segment->text, p->pw_name);
		break;
	      }
	    /* else fallthru */
	  }
	case 'U':		/* UID number */
	  segment->text[segment->text_len] = 'u';
	  fprintf (fp, segment->text, stat_buf->st_uid);
	  break;
	}
    }
  return (true);
}

boolean
pred_fstype (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (strcmp (filesystem_type (pathname, stat_buf), pred_ptr->args.str) == 0)
    return (true);
  return (false);
}

boolean
pred_gid (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_gid > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_gid < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_gid == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_group (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.gid == stat_buf->st_gid)
    return (true);
  else
    return (false);
}

boolean
pred_ilname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *path_for_name_test;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_name_test = current_pathname;
  } else {
    path_for_name_test = pathname;
  }

  return insert_lname (path_for_name_test, stat_buf, pred_ptr, true);
}

boolean
pred_iname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *base;
  char *path_for_name_test;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_name_test = current_pathname;
  } else {
    path_for_name_test = pathname;
  }

  base = basename (path_for_name_test);
  if (fnmatch (pred_ptr->args.str, base, FNM_PERIOD | FNM_CASEFOLD) == 0)
    return (true);
  return (false);
}

boolean
pred_inum (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ino > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ino < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_ino == pred_ptr->args.info.l_val)
	return (true);
      break;
    }

  if (!SS_ISSEGMENT_F(stat_buf->attr) || output_data_segments)
  {
    return (false);
  }
  else
  {
    return aggregate_test_over_segments(pathname, stat_buf, pred_ptr,
										TRUE_if_true_for_one, pred_inum);
  }
}

boolean
pred_ipath (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (fnmatch (pred_ptr->args.str, pathname, FNM_CASEFOLD) == 0)
    return (true);
  return (false);
}

boolean
pred_links (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_nlink > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_nlink < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_nlink == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_lname (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *path_for_name_test;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_name_test = current_pathname;
  } else {
    path_for_name_test = pathname;
  }

  return insert_lname (path_for_name_test, stat_buf, pred_ptr, false);
}

static boolean
insert_lname (pathname, stat_buf, pred_ptr, ignore_case)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
     boolean ignore_case;
{
  boolean ret = false;
#ifdef S_ISLNK
  int linklen;
  char *linkname;

  if (S_ISLNK (stat_buf->st_mode))
    {
      linkname = (char *) xmalloc (LINK_BUF + 1);
      linklen = readlink (pathname, linkname, LINK_BUF);
      if (linklen < 0)
	{
	  (void) fflush (stdout);
	  error (0, errno, "can't read link %s", pathname);
	}
      else
	{
	  linkname[linklen] = '\0';
	  if (fnmatch (pred_ptr->args.str, linkname,
		       ignore_case ? FNM_CASEFOLD : 0) == 0)
	    ret = true;
	}
      free (linkname);
    }
#endif /* S_ISLNK */
  return (ret);
}

boolean
pred_ls (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  list_file (pathname, stat_buf);
  return (true);
}

boolean
pred_mmin (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_mtime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_mtime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val + 60))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_mtime (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_mtime > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((stat_buf->st_mtime >= (time_t) pred_ptr->args.info.l_val)
	  && (stat_buf->st_mtime < (time_t) pred_ptr->args.info.l_val
	      + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_name (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *base;
  char *path_for_name_test;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_name_test = current_pathname;
  } else {
    path_for_name_test = pathname;
  }

  base = basename (path_for_name_test);
  if (fnmatch (pred_ptr->args.str, base, FNM_PERIOD) == 0)
    return (true);
  return (false);
}

boolean
pred_negate (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  /* Check whether we need a stat here. */
  if (pred_ptr->need_stat)
    {
      if (!have_stat && (*StatFunc)(pathname, stat_buf, sizeof(stat_buf)) != 0)
	{
	  (void) fflush (stdout);
	  error (0, errno, "StatFunc(%s)", pathname);
	  exit_status = 1;
	  return (false);
	}
      have_stat = true;
    }
  return (!(*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
					      pred_ptr->pred_right));
}

boolean
pred_newer (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (stat_buf->st_mtime > pred_ptr->args.time)
    return (true);
  return (false);
}

boolean
pred_nogroup (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
#ifdef CACHE_IDS
  extern char *gid_unused;

  return gid_unused[(unsigned) stat_buf->st_gid];
#else
  return getgrgid (stat_buf->st_gid) == NULL;
#endif
}

boolean
pred_nouser (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
#ifdef CACHE_IDS
  extern char *uid_unused;

  return uid_unused[(unsigned) stat_buf->st_uid];
#else
  return getpwuid (stat_buf->st_uid) == NULL;
#endif
}

boolean
pred_ok (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  int i, yes;
  
  (void) fflush (stdout);
  fprintf (stderr, "< %s ... %s > ? ",
	   pred_ptr->args.exec_vec.vec[0], pathname);
  (void) fflush (stderr);
  i = getchar ();
  yes = (i == 'y' || i == 'Y');
  while (i != EOF && i != '\n')
    i = getchar ();
  if (!yes)
    return (false);
  return pred_exec (pathname, stat_buf, pred_ptr);
}

boolean
pred_open (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_or (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->pred_left == NULL
      || !(*pred_ptr->pred_left->pred_func) (pathname, stat_buf,
					     pred_ptr->pred_left))
    {
      /* Check whether we need a stat here. */
      if (pred_ptr->need_stat)
	{
	  if (!have_stat && (*StatFunc)(pathname, stat_buf, sizeof(stat_buf)) != 0)
	    {
	      (void) fflush (stdout);
	      error (0, errno, "StatFunc(%s)", pathname);
	      exit_status = 1;
	      return (false);
	    }
	  have_stat = true;
	}
      return ((*pred_ptr->pred_right->pred_func) (pathname, stat_buf,
						  pred_ptr->pred_right));
    }
  else
    return (true);
}

boolean
pred_path (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  char *path_for_path_test;

  if (current_pathname && 
      (SS_ISSEGMENT_S(stat_buf->attr) ||
        (SS_ISSEGMENT_F(stat_buf->attr) && output_data_segments))) {
    path_for_path_test = current_pathname;
  } else {
    path_for_path_test = pathname;
  }

  if (fnmatch (pred_ptr->args.str, path_for_path_test, 0) == 0)
    return (true);
  return (false);
}

boolean
pred_perm (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.perm & 010000)
    {
      /* Magic flag set in parse_perm:
	 true if at least the given bits are set. */
      if ((stat_buf->st_mode & 07777 & pred_ptr->args.perm)
	  == (pred_ptr->args.perm & 07777))
	return (true);
    }
  else if (pred_ptr->args.perm & 020000)
    {
      /* Magic flag set in parse_perm:
	 true if any of the given bits are set. */
      if ((stat_buf->st_mode & 07777) & pred_ptr->args.perm)
	return (true);
    }
  else
    {
      /* True if exactly the given bits are set. */
      if ((stat_buf->st_mode & 07777) == pred_ptr->args.perm)
	return (true);
    }
  return (false);
}

boolean
pred_archpos(
char *pathname,
struct sam_stat *sb,
struct predicate *pred_ptr)
{
	enum comparison_type comp_type;
	offset_t arg_pos;
	offset_t f_val;
	int c;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	comp_type = pred_ptr->args.size.kind;
	arg_pos = pred_ptr->args.size.size;
	
	for (c = 0; c < MAX_ARCHIVE; c++) {
		if (!(sb->copy[c].flags & CF_ARCHIVED)) {
			continue;
		}
		if (pred_ptr->args.vsn_mt.copy & (1 << c)) {
			f_val = sb->copy[c].position;

			switch (comp_type) {
			case COMP_GT:
				if (f_val > arg_pos) {
					return (true);
				}
				break;
			case COMP_LT:
				if (f_val < arg_pos) {
					return (true);
				}
				break;
			case COMP_EQ:
				if (f_val == arg_pos) {
					return (true);
				}
				break;
			}	/* end switch */
		}	/* end if */
	}	/* end for copy */

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return (false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
			TRUE_if_true_for_one, pred_archpos);
	}
}

boolean
pred_print (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  puts (pathname);
  return (true);
}

boolean
pred_print0 (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  fputs (pathname, stdout);
  putc (0, stdout);
  return (true);
}

boolean
pred_prune (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  stop_at_current_level = true;
  return (do_dir_first);	/* This is what SunOS find seems to do. */
}

boolean
pred_regex (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (re_match (pred_ptr->args.regex, pathname, strlen (pathname), 0,
		(struct re_registers *) NULL) != -1)
    return (true);
  return (false);
}

boolean
pred_rmin(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) &&
			compare_time(pred_ptr->args.info.kind,
			     sb->residence_time,
			     pred_ptr->args.info.l_val, 60));
}

boolean
pred_rtime(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) &&
			compare_time(pred_ptr->args.info.kind,
			     sb->residence_time,
			     pred_ptr->args.info.l_val, DAYSECS));
}

boolean pred_is_setfa_g(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr) || !SS_ISSETFA_G(sb->attr)) {
		return (false);
	} else {
		return (true);
	}
}

boolean pred_is_setfa_s(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr) || !SS_ISSETFA_S(sb->attr)) {
		return (false);
	} else {
		return (true);
	}
}

boolean pred_setfa_g(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	enum comparison_type comp_type;
	offset_t arg_group_num;

	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	}

	comp_type      = pred_ptr->args.size.kind;
	arg_group_num  = pred_ptr->args.size.size;

	if (SS_ISSETFA_G(sb->attr)) {
		offset_t group_num;

		group_num = (offset_t) sb->stripe_group;

		switch (comp_type) {
		case COMP_GT:
			if (group_num > arg_group_num) {
			  return (true);
			}
			break;
		case COMP_LT:
			if (group_num < arg_group_num) {
			  return (true);
			}
			break;
		case COMP_EQ:
			if (group_num == arg_group_num) {
				return (true);
			}
		}
	}

	return (false);
}

boolean pred_setfa_s(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	enum comparison_type comp_type;
	offset_t arg_width_num;

	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	}

	comp_type      = pred_ptr->args.size.kind;
	arg_width_num  = pred_ptr->args.size.size;

	if (SS_ISSETFA_S(sb->attr)) {
		offset_t width_num;

		width_num = (offset_t) sb->stripe_width;

		switch (comp_type) {
		case COMP_GT:
			if (width_num > arg_width_num) {
			  return (true);
			}
			break;
		case COMP_LT:
			if (width_num < arg_width_num) {
			  return (true);
			}
			break;
		case COMP_EQ:
			if (width_num == arg_width_num) {
				return (true);
			}
		}
	}

	return (false);
}

boolean
pred_size (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  offset_t f_val;

  f_val = (stat_buf->st_size + pred_ptr->args.size.blocksize - 1)
    / pred_ptr->args.size.blocksize;
  switch (pred_ptr->args.size.kind)
    {
    case COMP_GT:
      if (f_val > pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_LT:
      if (f_val < pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_EQ:
      if (f_val == pred_ptr->args.size.size)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_true (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  return (true);
}

boolean
pred_type (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  unsigned long mode = stat_buf->st_mode;
  unsigned long type = pred_ptr->args.type;

#ifndef S_IFMT
  /* POSIX system; check `mode' the slow way. */
  if ((S_ISBLK (mode) && type == S_IFBLK)
      || (S_ISCHR (mode) && type == S_IFCHR)
      || (S_ISDIR (mode) && type == S_IFDIR)
      || (S_ISREG (mode) && type == S_IFREG)
#ifdef S_IFLNK
      || (S_ISLNK (mode) && type == S_IFLNK)
#endif
#ifdef S_IFIFO
      || (S_ISFIFO (mode) && type == S_IFIFO)
#endif
#ifdef S_IFSOCK
      || (S_ISSOCK (mode) && type == S_IFSOCK)
#endif
      )
#else /* S_IFMT */
  /* Unix system; check `mode' the fast way. */
  if ((mode & S_IFMT) == type)
#endif /* S_IFMT */
    return (true);
  else
    return (false);
}

boolean
pred_uid (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->st_uid > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_uid < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_uid == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_used (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  time_t delta;

  delta = stat_buf->st_atime - stat_buf->st_ctime; /* Use difftime? */
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (delta > (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (delta < (time_t) pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if ((delta >= (time_t) pred_ptr->args.info.l_val)
	  && (delta < (time_t) pred_ptr->args.info.l_val + DAYSECS))
	return (true);
      break;
    }
  return (false);
}

boolean
pred_user (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.uid == stat_buf->st_uid)
    return (true);
  else
    return (false);
}

boolean
pred_verify(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		if (SS_ISDATAV(sb->attr)) {
			return (true);
		} else {
			return (false);
		}
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_verify);
	}
}

boolean
pred_xtype (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  struct sam_stat sbuf;
  int (*ystat) ();

  ystat = (StatFunc == sam_lstat) ? sam_stat : sam_lstat;
  if ((*ystat) (pathname, &sbuf, sizeof(sbuf)) != 0)
    {
      if (ystat == stat && errno == ENOENT)
	/* Mimic behavior of ls -lL. */
	return (pred_type (pathname, stat_buf, pred_ptr));
      (void) fflush (stdout);
      error (0, errno, "StatFunc(%s)", pathname);
      exit_status = 1;
      return (false);
    }
  return (pred_type (pathname, &sbuf, pred_ptr));
}

boolean	compare_time	(kind, lhs, rhs, secs)
	Comparison_type	kind;
	unsigned long	lhs;
	unsigned long	rhs;
	int		secs;
{
	switch (kind) {

	case	COMP_GT:
		return (lhs >  rhs );

	case	COMP_LT:
		return (lhs <  rhs );

	case	COMP_EQ:
		return (lhs >= rhs && lhs < rhs+secs );
	}
}

boolean	compare_value	(kind, lhs, rhs)
	Comparison_type	kind;
	unsigned long	lhs;
	unsigned long	rhs;
{
	switch (kind) {

	case	COMP_GT:
		return (lhs >  rhs );

	case	COMP_LT:
		return (lhs <  rhs );

	case	COMP_EQ:
		return (lhs == rhs );
	}
}


boolean
pred_aid (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  switch (pred_ptr->args.info.kind)
    {
    case COMP_GT:
      if (stat_buf->admin_id > pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->admin_id < pred_ptr->args.info.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->admin_id == pred_ptr->args.info.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_admin_id (pathname, stat_buf, pred_ptr)
     char *pathname;
     struct sam_stat *stat_buf;
     struct predicate *pred_ptr;
{
  if (pred_ptr->args.gid == stat_buf->admin_id)
    return (true);
  else
    return (false);
}

boolean
pred_archived(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		if (SS_ISARCHIVED(sb->attr)) {
			return (true);
		} else {
			return (false);
		}
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_archived);
	}
}


boolean
pred_archive_d(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && !SS_ISARCHIVE_N(sb->attr));
}

boolean
pred_archive_n(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISARCHIVE_N(sb->attr));
}


boolean
pred_archdone(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		if (SS_ISARCHDONE(sb->attr)) {
			return (true);
		} else {
			return (false);
		}
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_archdone);
	}
}

boolean	pred_copies(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int	c;
	int	f_val = 0;

	if (!SS_ISSAMFS(sb->attr)) {
	  return (false);
	}

	if (SS_ISSEGMENT_F(sb->attr) && !output_data_segments) {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_all, pred_copies);
	}

	for (c = 0; c < MAX_ARCHIVE; c++) {
		if (sb->copy[c].flags & CF_ARCHIVED) f_val++;
	}
	switch (pred_ptr->args.size.kind) {
	case COMP_GT:
		if (f_val > pred_ptr->args.size.size)
		return (true);
		break;
	case COMP_LT:
		if (f_val < pred_ptr->args.size.size)
		return (true);
		break;
	case COMP_EQ:
		if (f_val == pred_ptr->args.size.size)
		return (true);
		break;
	}
	return (false);
}

boolean	pred_copy(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	if (SS_ISSEGMENT_F(sb->attr) && !output_data_segments) {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_all, pred_copy);
	}

	return(sb->copy[pred_ptr->args.type].flags & CF_ARCHIVED);
}


boolean
pred_damaged(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (SS_ISDAMAGED(sb->attr)) {
		return (true);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return (false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_damaged);
	}
}

boolean	pred_any_copy_d(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int copy;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	for( copy = 0; copy < MAX_ARCHIVE; copy ++ ) {
		if((sb->copy[copy].flags & (CF_ARCHIVED|CF_DAMAGED))
				== (CF_ARCHIVED|CF_DAMAGED) ) {
			return(true);
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_any_copy_d);
	}
}

boolean	pred_copy_d(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	} else if ((sb->copy[pred_ptr->args.type].flags & (CF_ARCHIVED|CF_DAMAGED))
		== (CF_ARCHIVED|CF_DAMAGED)) {
	  return (true);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_one, pred_copy_d);
	}
}

boolean	pred_any_copy_p(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int copy;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	for (copy = 0; copy < MAX_ARCHIVE; copy ++) {
		if (sb->copy[copy].flags & CF_PAX_ARCH_FMT) {
			return(true);
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_any_copy_p);
	}
}

boolean	pred_any_copy_r(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int copy;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	for( copy = 0; copy < MAX_ARCHIVE; copy ++ ) {
		if(sb->copy[copy].flags & CF_REARCH) {
			return(true);
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_any_copy_r);
	}
}

boolean	pred_copy_r(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	} else if (sb->copy[pred_ptr->args.type].flags & CF_REARCH) {
		return (true);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_copy_r);
	}
}

boolean	pred_any_copy_v(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int copy;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	for( copy = 0; copy < MAX_ARCHIVE; copy ++ ) {
		if(sb->copy[copy].flags & CF_VERIFIED) {
			return(true);
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_any_copy_v);
	}
}

boolean	pred_copy_v(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	} else if (sb->copy[pred_ptr->args.type].flags & CF_VERIFIED) {
		return (true);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_copy_v);
	}
}

boolean	pred_number_of_segments (
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int file_seg_num;

	if (!SS_ISSAMFS(sb->attr) || !SS_ISSEGMENT_F(sb->attr)) {
		return(false);
	}

	file_seg_num = NUM_SEGS(sb);

	switch (pred_ptr->args.size.kind) {
	case COMP_GT:
		if (file_seg_num > pred_ptr->args.size.size)
		return (true);
		break;
	case COMP_LT:
		if (file_seg_num < pred_ptr->args.size.size)
		return (true);
		break;
	case COMP_EQ:
		if (file_seg_num == pred_ptr->args.size.size)
		return (true);
		break;
	}

	return (false);
}

boolean
pred_offline(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (SS_ISOFFLINE(sb->attr)) {
		/*
		 * If the file is segmented and it's index inode is offline, then the
		 * file is effectively offline, until its index inode is back online
		 * and is available.
		 */
		return (true);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return (false);
	} else {
	  return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_all, pred_offline);

	}
}


boolean
pred_online(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (SS_ISOFFLINE(sb->attr)) {
		/*
		 * If the file is segmented and it's index inode is offline, then the
		 * file is effectively offline, until its index inode is back online
		 * and is available.
		 */
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return (true);
	} else {
	  return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_all, pred_online);
	}
}

boolean pred_partial_on(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false); /* Not a SAM-FS file, so partial-on not applicable. */
	} else if (SS_ISSEGMENT_S(sb->attr)) {
		if (sb->segment_number != 1) {
			/* Data segment, but not 1st one, partial-on not applicable. */
			return (false);
		} else {
			return SS_ISPARTIAL(sb->attr);
		}
	} else if (!SS_ISSEGMENT_F(sb->attr)) {
		return SS_ISPARTIAL(sb->attr);
	} else if (output_data_segments) {
		/* output_data_segments selected, but this is the index inode,
		 * not the first data segment, partial-on not applicable.
		 */
		return (false);
	} else {
		return pred_partial_on_segmented_file(pathname, sb, pred_ptr);
	}
}

boolean pred_partial_on_segmented_file(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int stat_result;
	int file_seg_num;

	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr)) {
		return (false);
	}

	/* Stat the file's data segments. */
	stat_result = seg_stat_file(pathname, sb);

	if (stat_result != 0) { /* stat'ing of file's data segs. failed. */
		return (false);
	}

	file_seg_num = NUM_SEGS(sb);

	if (file_seg_num <= 0) {
		/* File has no data segs. so no data segs. have part. on */
		return (false);
	}

	/* Partial rel. stored in first data segment inode. */
	return (SS_ISPARTIAL(segment_stat_ptr[0].attr));
}

boolean pred_release_d(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
  return (SS_ISSAMFS(sb->attr) &&
			!pred_release_a(pathname, sb, pred_ptr) &&
			!pred_release_n(pathname, sb, pred_ptr) &&
			!pred_release_p(pathname, sb, pred_ptr));
}

boolean pred_release_a(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISRELEASE_A(sb->attr));
}


boolean pred_release_n(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISRELEASE_N(sb->attr));
}


boolean pred_release_p(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	}

	if (SS_ISSEGMENT_S(sb->attr)) {
		/* Item is a data segment */
		if (sb->segment_number != 1) {
			/* Data segment, but not 1st one, release-partial not app. */
			return (false);
		} else {
			return SS_ISRELEASE_P(sb->attr);
		}
	} else if (!SS_ISSEGMENT_F(sb->attr)) {
		/* Item is not a data segment and not a segmented file */
		return SS_ISRELEASE_P(sb->attr);
	} else if (output_data_segments) {
		/* output_data_segments selected, but item is the index inode,
		 * release_p not applicable.
		 */
		return (false);
	} else {
		return pred_release_p_segmented_file(pathname, sb, pred_ptr);
	}
}

boolean pred_release_p_segmented_file(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int stat_result;
	int file_seg_num;

	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr)) {
		return (false);
	}

	/* Stat the file's data segments. */
	stat_result = seg_stat_file(pathname, sb);

	if (stat_result != 0) { /* stat'ing of file's data segs. failed. */
		return (false);
	}

	file_seg_num = NUM_SEGS(sb);

	if (file_seg_num <= 0) {
		/* File has no data segs. so no data segs. have part. on */
		return (false);
	}

	/* Partial rel. stored in first data segment inode. */
	return (SS_ISRELEASE_P(segment_stat_ptr[0].attr));
}

boolean pred_sections(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	enum comparison_type comp_type;
	offset_t arg_sections;
	int c;

	if (!SS_ISSAMFS(sb->attr)) {
		return(false);
	}

	comp_type     = pred_ptr->args.size.kind;
	arg_sections  = pred_ptr->args.size.size;

	for (c = 0; c < MAX_ARCHIVE; c++) {	
		if (!(sb->copy[c].flags & CF_ARCHIVED)) {
			continue;
		}

		if (pred_ptr->args.vsn_mt.copy & (1 << c)) {
			offset_t num_sections;

			num_sections = (offset_t) sb->copy[c].n_vsns;

			switch (comp_type) {
			case COMP_GT:

				if (num_sections > arg_sections) {
					return (true);
				}

				break;
			case COMP_LT:

				if (num_sections < arg_sections) {
					return (true);
				}

				break;
			case COMP_EQ:
				if (num_sections == arg_sections) {
					return (true);
				}

			break;
			}
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return (false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
										TRUE_if_true_for_one, pred_sections);
	}
}

boolean pred_segment_a(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr) ||
		(!SS_ISSEGMENT_S(sb->attr) && !SS_ISSEGMENT_F(sb->attr) &&
			!SS_ISSEGMENT_A(sb->attr))) {
		return (false);
	} else {
		return (true);
	}
}

boolean pred_segment_i(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr) || !SS_ISSEGMENT_F(sb->attr)) {
		return (false);
	} else {
		return (true);
	}
}

boolean pred_segment_s(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr) || !SS_ISSEGMENT_S(sb->attr)) {
		return (false);
	} else {
		return (true);
	}
}

boolean	pred_segment_number (
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int seg_num;

	if (!SS_ISSAMFS(sb->attr) || (!SS_ISSEGMENT_F(sb->attr) &&
			!SS_ISSEGMENT_S(sb->attr))) {
		/*
		 * 
		 */
		return (false);
	} else if (SS_ISSEGMENT_S(sb->attr)) {
		seg_num = sb->segment_number;
	} else {
		seg_num = 0;
	}

	switch (pred_ptr->args.size.kind) {
	case COMP_GT:
		if (seg_num > pred_ptr->args.size.size)
			return (true);
		break;
	case COMP_LT:
		if (seg_num < pred_ptr->args.size.size)
			return (true);
		break;
	case COMP_EQ:
		if (seg_num == pred_ptr->args.size.size)
			return (true);
		break;
	}

	return (false);
}

boolean pred_segmented(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (SS_ISSEGMENT_F(sb->attr) ||
				(SS_ISSEGMENT_S(sb->attr) && output_data_segments)) {
		return (true);
	} else {
		return (false);
	}
}


boolean pred_ssum_g(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISCSGEN(sb->attr));
}


boolean pred_ssum_u(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISCSUSE(sb->attr));
}


boolean pred_ssum_v(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (!SS_ISSAMFS(sb->attr)) {
		return (false);
	} else if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
	  return SS_ISCSVAL(sb->attr);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_all, pred_ssum_v);
	}
}


boolean pred_stage_d(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && !SS_ISSTAGE_A(sb->attr) &&
			!SS_ISSTAGE_N(sb->attr));
}


boolean pred_stage_a(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISSTAGE_A(sb->attr));
}


boolean pred_stage_n(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) && SS_ISSTAGE_N(sb->attr));
}


boolean
pred_xmin(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) &&
			compare_time(pred_ptr->args.info.kind, sb->creation_time,
			     pred_ptr->args.info.l_val, 60));
}


boolean
pred_xtime(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	return(SS_ISSAMFS(sb->attr) &&
			compare_time(pred_ptr->args.info.kind, sb->creation_time,
			     pred_ptr->args.info.l_val, DAYSECS));
}


boolean pred_vsn(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int c;
	int n_vsns;
	static struct sam_section vsns[MAX_VOLUMES];
	
	if (!SS_ISSAMFS(sb->attr))  return(false);
	for (c = 0; c < MAX_ARCHIVE; c++) {	
		if (!(sb->copy[c].flags & CF_ARCHIVED))  continue;
		if (pred_ptr->args.vsn_mt.copy & (1 << c)) {
			if (sb->copy[c].n_vsns > 1) {
				char *path_to_vsn_stat;
				int v;

				if (current_pathname &&
						(SS_ISSEGMENT_S(sb->attr) ||
						(SS_ISSEGMENT_F(sb->attr) && output_data_segments))) {
					path_to_vsn_stat = current_pathname;
				} else {
					path_to_vsn_stat = pathname;
				}

				/*
				 * The archive copy is overflowed which means that
				 * it spans multiple volumes: we need to get the
				 * vsn stat for each volume associated with this archive copy.
				 */

				n_vsns = (int) MIN( sb->copy[c].n_vsns, MAX_VOLUMES );

				(void) memset(vsns, 0, n_vsns * sizeof(struct sam_section));

				/*
				 * Call sam_vsn_stat to get the information for all of the
				 * multiple sections (volume sections) which make up this
				 * archive copy:
				 */

				if (!SS_ISSEGMENT_S(sb->attr)) {
					/* Item is not a data segment, call sam_vsn_stat to get
					 * the VSN information for the archive copy that overflows
					 * VSNs.
					 */
					if (sam_vsn_stat(path_to_vsn_stat, c, vsns,
							MAX_VOLUMES * sizeof(struct sam_section)) < 0) {
						(void) fflush (stdout);
						error (0, errno, "sam_vsn_stat(%s, %d)",
								path_to_vsn_stat, c + 1);
						exit_status = 1;

						return (false);
					}
				} else {
					/* Item is a data segment, call sam_segment_vsn_stat to get
					 * the VSN information for the archive copy the data
					 * segment that overflows VSNs.
					 */
					if (sam_segment_vsn_stat(path_to_vsn_stat, c,
							sb->segment_number - 1, vsns,
							MAX_VOLUMES * sizeof(struct sam_section)) < 0) {
						(void) fflush (stdout);
						error (0, errno,
								"sam_segment_vsn_stat(%s, %d, %d, ...)",
								path_to_vsn_stat, c + 1,
								(int) sb->segment_number);
						exit_status = 1;

						return (false);
					}
				}

				for (v = 0; v < n_vsns; v++) {
					if (fnmatch(pred_ptr->args.vsn_mt.key, vsns[v].vsn, 0) == 0) {
						return(true);
					}
				}
			} else {	/* not volume overflowed */
				if (fnmatch(pred_ptr->args.vsn_mt.key, sb->copy[c].vsn, 0) == 0) {
					return(true);
				}
			}
		}
	}

	if (!SS_ISSEGMENT_F(sb->attr) || output_data_segments) {
		return(false);
	} else {
		return aggregate_test_over_segments(pathname, sb, pred_ptr,
											TRUE_if_true_for_one, pred_vsn);
	}
}


boolean pred_mt(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	int c;

	if (!SS_ISSAMFS(sb->attr))  return(false);
	for (c = 0; c < MAX_ARCHIVE; c++) {
		if (!(sb->copy[c].flags & CF_ARCHIVED))  continue;
		if ((pred_ptr->args.vsn_mt.copy & (1 << c)) &&
				strcmp(pred_ptr->args.vsn_mt.key, sb->copy[c].media) == 0) {
			return(true);
		}
	}
	return(false);
}


boolean
pred_worm(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (SS_ISSAMFS(sb->attr) && SS_ISREADONLY(sb->attr) &&
			!S_ISDIR(sb->st_mode)) {
		if ((sb->rperiod_duration == 0) ||
				((sb->rperiod_start_time/60 +
				sb->rperiod_duration) >
				(time((time_t *)NULL)/60))) {
			return (true);
		}
	}
	return (false);
}


boolean
pred_expired(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if (SS_ISSAMFS(sb->attr) && SS_ISREADONLY(sb->attr) &&
			!S_ISDIR(sb->st_mode)) {
		if (sb->rperiod_duration == 0) {
			return (false);
		} else if ((sb->rperiod_start_time/60 + sb->rperiod_duration) <
				(time((time_t *)NULL))/60) {
			return (true);
		}
	}
	return (false);
}


boolean
pred_retention(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	enum comparison_type comp_type;
	long num_mins = 0;
	time_t curr_time = time((time_t *)NULL);
	boolean ret = false;


	comp_type = pred_ptr->args.info.kind;
	num_mins = pred_ptr->args.info.l_val;

	if (SS_ISSAMFS(sb->attr) && SS_ISREADONLY(sb->attr) &&
			!S_ISDIR(sb->st_mode)) {
		switch (comp_type) {
			/*
			 * The granularity of time is minutes.
			 */
			case COMP_GT:
				if (((sb->rperiod_start_time/60) +
				    sb->rperiod_duration) >
						((curr_time/60) + num_mins) ||
						(sb->rperiod_duration == 0)) {
					ret = true;
				}
				break;

			case COMP_LT:
				if (((sb->rperiod_start_time/60) +
				    sb->rperiod_duration) <
						((curr_time/60) + num_mins)) {
					ret = true;
				}
				break;

			case COMP_EQ:
				if (sb->rperiod_duration == num_mins) {
					ret = true;
				}
				break;
		}
	}

	return (ret);
}


boolean
pred_remain(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	long num_mins = 0;
	time_t curr_time = time((time_t *)NULL);


	num_mins = pred_ptr->args.info.l_val;

	if (SS_ISSAMFS(sb->attr) && SS_ISREADONLY(sb->attr) &&
	    !S_ISDIR(sb->st_mode)) {
		if ((((sb->rperiod_start_time/60) +
		    sb->rperiod_duration) >=
		    ((curr_time/60) + num_mins)) ||
		    (sb->rperiod_duration == 0)) {
			return (true);
		}
	}
	return (false);
}


boolean
pred_after(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	long num_mins = 0;
	num_mins = pred_ptr->args.info.l_val;

	if (SS_ISSAMFS(sb->attr) && SS_ISREADONLY(sb->attr) &&
			!S_ISDIR(sb->st_mode)) {
		if ((sb->rperiod_duration == 0) ||
				((sb->rperiod_start_time/60) +
				sb->rperiod_start_time) >
				num_mins) {
			return (true);
		}
	}
	return (false);
}


boolean
pred_project(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
	if ((SS_ISSAMFS(sb->attr)) &&
	    (pred_ptr->args.projid == sb->projid)) {
		return (true);
	}

	return (false);
}


boolean
pred_xattr(
char *pathname,
struct sam_stat *sb,
Predicate *pred_ptr)
{
#if sun
	return (pathconf(pathname, _PC_XATTR_EXISTS) == 1);
#else /* sun */
	return (false);
#endif /* sun */
}


/*  1) fork to get a child; parent remembers the child pid
    2) child execs the command requested
    3) parent waits for child; checks for proper pid of child

    Possible returns:

    ret		errno	status(h)   status(l)

    pid		x	signal#	    0177	stopped
    pid		x	exit arg    0		term by _exit
    pid		x	0	    signal #	term by signal
    -1		EINTR				parent got signal
    -1		other				some other kind of error

    Return true only if the pid matches, status(l) is
    zero, and the exit arg (status high) is 0.
    Otherwise return false, possibly printing an error message. */

boolean
launch (pred_ptr)
     struct predicate *pred_ptr;
{
  int status, wait_ret, child_pid;
  struct exec_val *execp;	/* Pointer for efficiency. */

  execp = &pred_ptr->args.exec_vec;

  /* Make sure output of command doesn't get mixed with find output. */
  (void) fflush (stdout);
  (void) fflush (stderr);

  child_pid = fork ();
  if (child_pid == -1)
    error (1, errno, "cannot fork");
  if (child_pid == 0)
    {
      /* We be the child. */
      execvp (execp->vec[0], execp->vec);
      error (0, errno, "%s", execp->vec[0]);
      _exit (1);
    }

  wait_ret = wait (&status);
  if (wait_ret == -1)
    {
      (void) fflush (stdout);
      error (0, errno, "error waiting for %s", execp->vec[0]);
      exit_status = 1;
      return (false);
    }
  if (wait_ret != child_pid)
    {
      (void) fflush (stdout);
      error (0, 0, "wait got pid %d, expected pid %d", wait_ret, child_pid);
      exit_status = 1;
      return (false);
    }
  if (WIFSTOPPED (status))
    {
      (void) fflush (stdout);
      error (0, 0, "%s stopped by signal %d", 
	     execp->vec[0], WSTOPSIG (status));
      exit_status = 1;
      return (false);
    }
  if (WIFSIGNALED (status))
    {
      (void) fflush (stdout);
      error (0, 0, "%s terminated by signal %d",
	     execp->vec[0], WTERMSIG (status));
      exit_status = 1;
      return (false);
    }
  return (!WEXITSTATUS (status));
}

/* Return a static string formatting the time WHEN according to the
   strftime format character KIND.  */

char *
format_date (when, kind)
     time_t when;
     int kind;
{
  static char fmt[3];
  static char buf[64];		/* More than enough space. */

  if (kind == '@')
    {
      sprintf (buf, "%ld", when);
      return (buf);
    }
  else
    {
      fmt[0] = '%';
      fmt[1] = kind;
      fmt[2] = '\0';
      if (strftime (buf, sizeof (buf), fmt, localtime (&when)))
	return (buf);
    }
  return "";
}

boolean 
aggregate_test_over_segments(
							char *pathname,
							struct sam_stat *stat_buf,
							struct predicate *pred_ptr,
							enum pred_aggregate_type pred_aggregate_type_arg,
							PFB pred_fct)
{
	int file_seg_num;
	int seg_no;
	int stat_result;

	if (!SS_ISSAMFS(stat_buf->attr) || !SS_ISSEGMENT_F(stat_buf->attr)) {
		return (false);
	}

	stat_result = seg_stat_file(pathname, stat_buf);

	if (stat_result != 0) {
		return (false);
	}

	file_seg_num = NUM_SEGS(stat_buf);

	if (file_seg_num <= 0) {
		/* File is segmented, but has no data segments yet, can not compare
		 * data segment inums to argument inum as no data segments implies
		 * no data segment inums.
		 */
		return (false);
	}

	if (pred_aggregate_type_arg == TRUE_if_true_for_all) {
		for (seg_no = 0; seg_no < file_seg_num; seg_no++) {
			if (!((*pred_fct)(pathname, &segment_stat_ptr[seg_no], pred_ptr)))
			{
			  return (false);
			}
		}

		return (true);
	} else {
		for (seg_no = 0; seg_no < file_seg_num; seg_no++) {
			if ((*pred_fct)(pathname, &segment_stat_ptr[seg_no], pred_ptr)) {
			  return (true);
			}
		}

		return (false);
	}
}

int                         /* 0 upon success, -1 upon failure anderrno set  */
seg_stat_file(
const char *path,           /* path of file whose segments are being stat'ed */
struct sam_stat *file_stat_buf) { /* path's sam_lstat stats                  */
  int file_seg_num;
  int stat_result;

  if (have_seg_stat) { /* seg. stat'ed path already. */
    return 0;
  } else if (!SS_ISSEGMENT_F(file_stat_buf->attr)) {
    /* path is not a segmented file, can not seg. stat path. */
    error(0, EINVAL, "Error: %s is not a segmented file; can not perform sam_segment_lstat on it.",
          path);
    if (!errno) {
      errno = EINVAL;
    }

    return -1;
  }

  /*
   * Check, does segment_stat_ptr point to a sufficiently big buffer?
   */
  file_seg_num = NUM_SEGS(file_stat_buf);

  if (file_seg_num > seg_buf_capacity) {
    if (segment_stat_ptr != (struct sam_stat *)NULL) {
      free(segment_stat_ptr);
    }

    segment_stat_ptr = (struct sam_stat *) malloc(sizeof(struct sam_stat)*file_seg_num);

    if (segment_stat_ptr == (struct sam_stat *)NULL) {
		  /* Allocation operation failed. */
      if (!errno) {
        errno = EAGAIN;
      }

      error(0, errno, "Error: Failed to allocate memory to store segment stat information for %s.", path);
      exit_status = 1;

      return -1;
    }

    seg_buf_capacity = file_seg_num;
  }
  else if (file_seg_num < seg_buf_capacity) {
    /* Set unused entries to zero to avoid confusion */
    memset((void *) (segment_stat_ptr + file_seg_num), 0,
           (size_t) (sizeof(struct sam_stat) * (seg_buf_capacity - file_seg_num)));
  }

  stat_result = sam_segment_stat(path, segment_stat_ptr,
                                 sizeof(struct sam_stat)*file_seg_num);

  if (stat_result != 0) {
    /* Stat operation failed */
    if (!errno) {
      error(0, errno, "Error: sam_segment_lstat failed to perform segment stat operation on file %s.",
            path);
      exit_status = 1;

      return -1;
    }
  }

  have_seg_stat = true;

  return 0;
}

#ifdef	DEBUG
/* Return a pointer to the string representation of 
   the predicate function PRED_FUNC. */

char *
find_pred_name (pred_func)
     PFB pred_func;
{
  int i;

  for (i = 0; pred_table[i].pred_func != 0; i++)
    if (pred_table[i].pred_func == pred_func)
      break;
  return (pred_table[i].pred_name);
}

char *
type_name (type)
     short type;
{
  int i;

  for (i = 0; type_table[i].type != (short) -1; i++)
    if (type_table[i].type == type)
      break;
  return (type_table[i].type_name);
}

char *
prec_name (prec)
     short prec;
{
  int i;

  for (i = 0; prec_table[i].prec != (short) -1; i++)
    if (prec_table[i].prec == prec)
      break;
  return (prec_table[i].prec_name);
}

/* Walk the expression tree NODE to stdout.
   INDENT is the number of levels to indent the left margin. */

void
print_tree (node, indent)
     struct predicate *node;
     int indent;
{
  int i;

  if (node == NULL)
    return;
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("pred = %s type = %s prec = %s addr = %x\n",
	  find_pred_name (node->pred_func),
	  type_name (node->p_type), prec_name (node->p_prec), node);
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("left:\n");
  print_tree (node->pred_left, indent + 1);
  for (i = 0; i < indent; i++)
    printf ("    ");
  printf ("right:\n");
  print_tree (node->pred_right, indent + 1);
}

/* Copy STR into BUF and trim blanks from the end of BUF.
   Return BUF. */

char *
blank_rtrim (str, buf)
     char *str;
     char *buf;
{
  int i;

  if (str == NULL)
    return (NULL);
  strcpy (buf, str);
  i = strlen (buf) - 1;
  while ((i >= 0) && ((buf[i] == ' ') || buf[i] == '\t'))
    i--;
  buf[++i] = '\0';
  return (buf);
}

/* Print out the predicate list starting at NODE. */

void
print_list (node)
     struct predicate *node;
{
  struct predicate *cur;
  char name[256];

  cur = node;
  while (cur != NULL)
    {
      printf ("%s ", blank_rtrim (find_pred_name (cur->pred_func), name));
      cur = cur->pred_next;
    }
  printf ("\n");
}
#endif	/* DEBUG */
