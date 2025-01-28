/* parser.c -- convert the command line args into an expression tree.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#ifdef sun
#include <project.h>
#endif /* sun */

#ifndef isascii
#define isascii(c) 1
#endif

#define ISDIGIT(c) (isascii (c) && isdigit (c))
#define ISUPPER(c) (isascii (c) && isupper (c))

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _POSIX_VERSION
/* POSIX.1 header files should declare these.  */
struct group *getgrnam ();
struct passwd *getpwnam ();
#endif
#ifdef CACHE_IDS
/* These two aren't specified by POSIX.1.  */
struct group *getgrent ();
struct passwd *getpwent ();
#endif

#include "modechange.h"
#include <pub/lib.h>
#include <sam/lib.h>
#include <aml/diskvols.h>
#include <sys/stat.h>
#include <pub/stat.h>
#include "defs.h"

char *strstr ();
#ifndef atol /* for Linux */
long atol ();
#endif
struct tm *localtime ();
#ifdef _POSIX_SOURCE
#define endgrent()
#define endpwent()
#else
void endgrent ();
void endpwent ();
#endif

static boolean parse_amin ();
static boolean parse_and ();
static boolean parse_anewer ();
static boolean parse_atime ();
boolean parse_close ();
static boolean parse_cmin ();
static boolean parse_cnewer ();
static boolean parse_comma ();
static boolean parse_ctime ();
static boolean parse_daystart ();
static boolean parse_depth ();
static boolean parse_data_segments ();
static boolean parse_empty ();
static boolean parse_exec ();
static boolean parse_false ();
static boolean parse_follow ();
static boolean parse_fprint ();
static boolean parse_fprint0 ();
static boolean parse_fprintf ();
static boolean parse_fstype ();
static boolean parse_gid ();
static boolean parse_group ();
static boolean parse_ilname ();
static boolean parse_iname ();
static boolean parse_inum ();
static boolean parse_ipath ();
static boolean parse_iregex ();
static boolean parse_links ();
static boolean parse_lname ();
static boolean parse_ls ();
static boolean parse_maxdepth ();
static boolean parse_mindepth ();
static boolean parse_mmin ();
static boolean parse_mtime ();
static boolean parse_name ();
static boolean parse_negate ();
static boolean parse_newer ();
static boolean parse_noleaf ();
static boolean parse_nogroup ();
static boolean parse_nouser ();
static boolean parse_ok ();
boolean parse_open ();
static boolean parse_or ();
static boolean parse_path ();
static boolean parse_perm ();
boolean parse_print ();
static boolean parse_print0 ();
static boolean parse_printf ();
static boolean parse_prune ();
static boolean parse_regex ();
static boolean parse_size ();
static boolean parse_true ();
static boolean parse_type ();
static boolean parse_uid ();
static boolean parse_used ();
static boolean parse_user ();
static boolean parse_version ();
static boolean parse_xdev ();
static boolean parse_xtype ();

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
/* no pred_follow */
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
/* no pred_maxdepth */
/* no pred_mindepth */
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
boolean pred_print ();
boolean pred_print0 ();
/* no pred_printf */
boolean pred_prune ();
boolean pred_regex ();
boolean pred_sections();

boolean pred_size ();
boolean pred_true ();
boolean pred_type ();
boolean pred_uid ();
boolean pred_used ();
boolean pred_user ();
boolean pred_vsn ();
boolean pred_mt ();
/* no pred_version */
/* no pred_xdev */
boolean pred_xtype ();

boolean do_archpos();
boolean do_sections();
boolean do_ovfl();


boolean	parse_aid (), pred_aid ();			/* SUN */
boolean	parse_admin_id (), pred_admin_id ();		/* SUN */
boolean	parse_any_copy_d (), pred_any_copy_d ();	/* SUN */
boolean parse_any_copy_p (), pred_any_copy_p ();	/* SUN */
boolean	parse_any_copy_r (), pred_any_copy_r ();	/* SUN */
boolean parse_any_copy_v (), pred_any_copy_v ();	/* SUN */
boolean parse_archived (), pred_archived ();		/* SUN */
boolean parse_archive_d (), pred_archive_d();		/* SUN */
boolean parse_archive_n (), pred_archive_n();		/* SUN */
boolean parse_archdone (), pred_archdone();		/* SUN */
boolean	parse_copy (), pred_copy ();			/* SUN */
boolean	parse_copy_d (), pred_copy_d ();		/* SUN */
boolean	parse_copy_r (), pred_copy_r ();		/* SUN */
boolean parse_copy_v (), pred_copy_v ();		/* SUN */
boolean	parse_copies (), pred_copies ();		/* SUN */
boolean	parse_damaged (), pred_damaged ();		/* SUN */
boolean	parse_number_of_segments(), pred_number_of_segments ();	/* SUN */
boolean	parse_offline (), pred_offline ();		/* SUN */
boolean	parse_online (), pred_online ();		/* SUN */
boolean parse_ovfl();					/* SUN */
boolean parse_ovfl1();					/* SUN */
boolean parse_ovfl2();					/* SUN */
boolean parse_ovfl3();					/* SUN */
boolean parse_ovfl4();					/* SUN */
boolean parse_archpos(), pred_archpos();	
boolean parse_archpos1();
boolean parse_archpos2();
boolean parse_archpos3();
boolean parse_archpos4();
boolean parse_partial_on (), pred_partial_on();		/* SUN */
boolean parse_release_d (), pred_release_d();		/* SUN */
boolean parse_release_a (), pred_release_a();		/* SUN */
boolean parse_release_n (), pred_release_n();		/* SUN */
boolean parse_release_p (), pred_release_p();		/* SUN */
boolean	parse_rmin (), pred_rmin ();			/* SUN */
boolean	parse_rtime (), pred_rtime ();			/* SUN */
boolean parse_sections();				/* SUN */
boolean parse_sections1();				/* SUN */
boolean parse_sections2();				/* SUN */
boolean parse_sections3();				/* SUN */
boolean parse_sections4();				/* SUN */
boolean parse_segment_a (), pred_segment_a ();		/* SUN */
boolean parse_segment_i (), pred_segment_i ();		/* SUN */
boolean parse_segment_s (), pred_segment_s ();		/* SUN */
boolean parse_segment_number(), pred_segment_number ();	/* SUN */
boolean parse_segmented (), pred_segmented ();		/* SUN */
boolean parse_is_setfa_g (), pred_is_setfa_g ();	/* SUN */
boolean parse_is_setfa_s (), pred_is_setfa_s ();	/* SUN */
boolean parse_setfa_g (), pred_setfa_g ();		/* SUN */
boolean parse_setfa_s (), pred_setfa_s ();		/* SUN */
boolean parse_ssum_g (), pred_ssum_g ();		/* SUN */
boolean parse_ssum_u (), pred_ssum_u();			/* SUN */
boolean parse_ssum_v (), pred_ssum_v();			/* SUN */
boolean parse_stage_d (), pred_stage_d();		/* SUN */
boolean parse_stage_a (), pred_stage_a();		/* SUN */
boolean parse_stage_n (), pred_stage_n();		/* SUN */
boolean parse_verify (), pred_verify ();		/* SUN */
boolean parse_paxhdr (), pred_paxhdr ();		/* SUN */
boolean	parse_xmin (), pred_xmin ();			/* SUN */
boolean	parse_xtime (), pred_xtime ();			/* SUN */
boolean parse_vsn(), parse_mt ();			/* SUN */
boolean parse_vsn1(), parse_mt1();			/* SUN */
boolean parse_vsn2(), parse_mt2();			/* SUN */
boolean parse_vsn3(), parse_mt3();			/* SUN */
boolean parse_vsn4(), parse_mt4();			/* SUN */
boolean parse_worm(), pred_worm();			/* SUN */
boolean parse_expired(), pred_expired();		/* SUN */
boolean parse_retention(), pred_retention();		/* SUN */
boolean parse_after(), pred_after();			/* SUN */
boolean parse_remain(), pred_remain();			/* SUN */
boolean parse_longer(), pred_retention();		/* SUN */
boolean parse_permanent(), pred_retention();		/* SUN */
boolean parse_project(), pred_project();		/* SUN */
boolean parse_xattr(), pred_xattr();			/* SUN */

typedef	struct predicate	Predicate;
typedef	enum comparison_type	Comparison_type;

static boolean get_num ();
static boolean get_num_days ();
static boolean insert_exec_ok ();
static boolean insert_fprintf ();
static boolean insert_num ();
static boolean insert_regex ();
static boolean insert_time ();
static boolean insert_type ();
static FILE *open_output_file ();
static struct segment **make_segment ();

#ifdef	DEBUG
char *find_pred_name ();
#endif	/* DEBUG */

struct parser_table_t
{
  char *parser_name;
  PFB parser_func;
};

/* GNU find predicates that are not mentioned in POSIX.2 are marked `GNU'.
   If they are in some Unix versions of find, they are marked `Unix'. */

static struct parser_table_t const parse_table[] =
{
  {"!", parse_negate},
  {"not", parse_negate},	/* GNU */
  {"(", parse_open},
  {")", parse_close},
  {",", parse_comma},		/* GNU */
  {"a", parse_and},
  {"amin", parse_amin},		/* GNU */
  {"and", parse_and},		/* GNU */
  {"anewer", parse_anewer},	/* GNU */
  {"atime", parse_atime},
  {"cmin", parse_cmin},		/* GNU */
  {"cnewer", parse_cnewer},	/* GNU */
#ifdef UNIMPLEMENTED_UNIX
  /* It's pretty ugly for find to know about archive formats.
     Plus what it could do with cpio archives is very limited.
     Better to leave it out.  */
  {"cpio", parse_cpio},		/* Unix */
#endif
  {"ctime", parse_ctime},
  {"daystart", parse_daystart},	/* GNU */
  {"depth", parse_depth},
  {"empty", parse_empty},	/* GNU */
  {"exec", parse_exec},
  {"false", parse_false},	/* GNU */
  {"follow", parse_follow},	/* GNU, Unix */
  {"fprint", parse_fprint},	/* GNU */
  {"fprint0", parse_fprint0},	/* GNU */
  {"fprintf", parse_fprintf},	/* GNU */
  {"fstype", parse_fstype},	/* GNU, Unix */
  {"gid", parse_gid},		/* GNU */
  {"group", parse_group},
  {"ilname", parse_ilname},	/* GNU */
  {"iname", parse_iname},	/* GNU */
  {"inum", parse_inum},		/* GNU, Unix */
  {"ipath", parse_ipath},	/* GNU */
  {"iregex", parse_iregex},	/* GNU */
  {"links", parse_links},
  {"lname", parse_lname},	/* GNU */
  {"ls", parse_ls},		/* GNU, Unix */
  {"maxdepth", parse_maxdepth},	/* GNU */
  {"mindepth", parse_mindepth},	/* GNU */
  {"mmin", parse_mmin},		/* GNU */
  {"mtime", parse_mtime},
  {"name", parse_name},
#ifdef UNIMPLEMENTED_UNIX
  {"ncpio", parse_ncpio},	/* Unix */
#endif
  {"newer", parse_newer},
  {"noleaf", parse_noleaf},	/* GNU */
  {"nogroup", parse_nogroup},
  {"nouser", parse_nouser},
  {"o", parse_or},
  {"or", parse_or},		/* GNU */
  {"ok", parse_ok},
  {"path", parse_path},		/* GNU, HP-UX */
  {"perm", parse_perm},
  {"print", parse_print},
  {"print0", parse_print0},	/* GNU */
  {"printf", parse_printf},	/* GNU */
  {"prune", parse_prune},
  {"regex", parse_regex},	/* GNU */
  {"size", parse_size},
  {"true", parse_true},		/* GNU */
  {"type", parse_type},
  {"uid", parse_uid},		/* GNU */
  {"used", parse_used},		/* GNU */
  {"user", parse_user},
  {"version", parse_version},	/* GNU */
  {"xdev", parse_xdev},
  {"xtype", parse_xtype},	/* GNU */
  {"aid", parse_aid},				/* SUN */
  {"admin_id", parse_admin_id},			/* SUN */
  {"any_copy_d", parse_any_copy_d}, 		/* SUN */
  {"any_copy_p", parse_any_copy_p}, 		/* SUN */
  {"any_copy_r", parse_any_copy_r}, 		/* SUN */
  {"any_copy_v", parse_any_copy_v}, 		/* SUN */
  {"archived", parse_archived},			/* SUN */
  {"archive_d", parse_archive_d},		/* SUN */
  {"archive_n", parse_archive_n},		/* SUN */
  {"archdone", parse_archdone},			/* SUN */
  {"copy", parse_copy},				/* SUN */
  {"copy_d", parse_copy_d},			/* SUN */
  {"copy_r", parse_copy_r},			/* SUN */
  {"copy_v", parse_copy_v},			/* SUN */
  {"copies", parse_copies},			/* SUN */
  {"damaged", parse_damaged},			/* SUN */
  {"offline", parse_offline},			/* SUN */
  {"online", parse_online},			/* SUN */
  {"ovfl", parse_ovfl},				/* SUN */
  {"ovfl1", parse_ovfl1},			/* SUN */
  {"ovfl2", parse_ovfl2},			/* SUN */
  {"ovfl3", parse_ovfl3},			/* SUN */
  {"ovfl4", parse_ovfl4},			/* SUN */
  {"archpos", parse_archpos},
  {"archpos1", parse_archpos1},
  {"archpos2", parse_archpos2},
  {"archpos3", parse_archpos3},
  {"archpos4", parse_archpos4},
  {"partial_on", parse_partial_on},	/* SUN */
  {"release_d", parse_release_d},	/* SUN */
  {"release_a", parse_release_a},	/* SUN */
  {"release_n", parse_release_n},	/* SUN */
  {"release_p", parse_release_p},	/* SUN */
  {"rmin", parse_rmin},			/* SUN */
  {"rtime", parse_rtime},		/* SUN */
  {"sections", parse_sections},		/* SUN */
  {"sections1", parse_sections1},	/* SUN */
  {"sections2", parse_sections2},	/* SUN */
  {"sections3", parse_sections3},	/* SUN */
  {"sections4", parse_sections4},	/* SUN */
  {"segment_a", parse_segment_a},	/* SUN */
  {"segment_i", parse_segment_i},	/* SUN */
  {"segment_s", parse_segment_s},	/* SUN */
  {"segment", parse_segment_number},	/* SUN */
  {"segments", parse_number_of_segments},	/* SUN */
  {"segmented", parse_segmented},	/* SUN */
  {"is_setfa_g", parse_is_setfa_g},	/* SUN */
  {"is_setfa_s", parse_is_setfa_s}, 	/* SUN */
  {"setfa_g", parse_setfa_g},		/* SUN */
  {"setfa_s", parse_setfa_s},		/* SUN */
  {"ssum_g", parse_ssum_g},		/* SUN */
  {"ssum_u", parse_ssum_u},		/* SUN */
  {"ssum_v", parse_ssum_v},		/* SUN */
  {"stage_d", parse_stage_d},		/* SUN */
  {"stage_a", parse_stage_a},		/* SUN */
  {"stage_n", parse_stage_n},		/* SUN */
  {"test_segments", parse_data_segments},	/* SUN */
  {"verify", parse_verify},		/* SUN */
  {"xmin", parse_xmin},			/* SUN */
  {"xtime", parse_xtime},		/* SUN */
  {"vsn", parse_vsn},			/* SUN */
  {"vsn1", parse_vsn1},			/* SUN */
  {"vsn2", parse_vsn2},			/* SUN */
  {"vsn3", parse_vsn3},			/* SUN */
  {"vsn4", parse_vsn4},			/* SUN */
  {"mt", parse_mt},			/* SUN */
  {"mt1", parse_mt1},			/* SUN */
  {"mt2", parse_mt2},			/* SUN */
  {"mt3", parse_mt3},			/* SUN */
  {"mt4", parse_mt4},			/* SUN */
  {"ractive", parse_worm},		/* SUN */
  {"rover", parse_expired},		/* SUN */
  {"retention", parse_retention},	/* SUN */
  {"rafter", parse_after},		/* SUN */
  {"rremain", parse_remain},		/* SUN */
  {"rlonger", parse_longer},		/* SUN */
  {"rpermanent", parse_permanent},	/* SUN */
  {"project", parse_project},
  {"xattr", parse_xattr},		/* SUN */
  {0, 0}
};

/* Return a pointer to the parser function to invoke for predicate
   SEARCH_NAME.
   Return NULL if SEARCH_NAME is not a valid predicate name. */

PFB
find_parser (search_name)
     char *search_name;
{
  int i;

  if (*search_name == '-')
    search_name++;
  for (i = 0; parse_table[i].parser_name != 0; i++)
    if (strcmp (parse_table[i].parser_name, search_name) == 0)
      return (parse_table[i].parser_func);
  return (NULL);
}

/* The parsers are responsible to continue scanning ARGV for
   their arguments.  Each parser knows what is and isn't
   allowed for itself.
   
   ARGV is the argument array.
   *ARG_PTR is the index to start at in ARGV,
   updated to point beyond the last element consumed.
 
   The predicate structure is updated with the new information. */

static boolean
parse_amin (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  unsigned long num;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return (false);
  our_pred = insert_victim (pred_amin);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = cur_day_start + DAYSECS - num * 60;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_and (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred ();
  our_pred->pred_func = pred_and;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_and);
#endif	/* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = AND_PREC;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_anewer (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  struct sam_stat stat_newer;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if ((*StatFunc)(argv[*arg_ptr], &stat_newer, sizeof(stat_newer)))
    error (1, errno, "%s", argv[*arg_ptr]);
  our_pred = insert_victim (pred_anewer);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_atime (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_time (argv, arg_ptr, pred_atime));
}

boolean
parse_close (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred ();
  our_pred->pred_func = pred_close;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_close);
#endif	/* DEBUG */
  our_pred->p_type = CLOSE_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_cmin (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  unsigned long num;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return (false);
  our_pred = insert_victim (pred_cmin);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = cur_day_start + DAYSECS - num * 60;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_cnewer (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  struct sam_stat stat_newer;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if ((*StatFunc)(argv[*arg_ptr], &stat_newer, sizeof(stat_newer)))
    error (1, errno, "%s", argv[*arg_ptr]);
  our_pred = insert_victim (pred_cnewer);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_comma (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred ();
  our_pred->pred_func = pred_comma;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_comma);
#endif /* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = COMMA_PREC;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_ctime (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_time (argv, arg_ptr, pred_ctime));
}

static boolean
parse_daystart (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct tm *local;

  if (full_days == false)
    {
      cur_day_start += DAYSECS;
      local = localtime (&cur_day_start);
      cur_day_start -= local->tm_sec + local->tm_min * 60
	+ local->tm_hour * 3600;
      full_days = true;
    }
  return (true);
}

static boolean
parse_depth (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  do_dir_first = false;
  return (true);
}
 
static boolean
parse_empty (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  insert_victim (pred_empty);
  return (true);
}

static boolean
parse_exec (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_exec_ok (pred_exec, argv, arg_ptr));
}

static boolean
parse_false (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_false);
  our_pred->need_stat = false;
  return (true);
}

static boolean 
parse_fprintf (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  FILE *fp;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (argv[*arg_ptr + 1] == NULL)
    {
      /* Ensure we get "missing arg" message, not "invalid arg".  */
      (*arg_ptr)++;
      return (false);
    }
  fp = open_output_file (argv[*arg_ptr]);
  (*arg_ptr)++;
  return (insert_fprintf (fp, pred_fprintf, argv, arg_ptr));
}

static boolean
parse_follow (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  StatFunc = sam_stat;
  no_leaf_check = true;
  return (true);
}

static boolean
parse_fprint (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_fprint);
  our_pred->args.stream = open_output_file (argv[*arg_ptr]);
  our_pred->side_effects = true;
  our_pred->need_stat = false;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_fprint0 (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_fprint0);
  our_pred->args.stream = open_output_file (argv[*arg_ptr]);
  our_pred->side_effects = true;
  our_pred->need_stat = false;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_fstype (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_fstype);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_gid (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_num (argv, arg_ptr, pred_gid));
}

static boolean
parse_group (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct group *cur_gr;
  struct predicate *our_pred;
  uint_t gid;
  int gid_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL)) {
    return (false);
  }

  cur_gr = getgrnam (argv[*arg_ptr]);
  endgrent ();

  if (cur_gr != NULL) {
    gid = (uint_t) cur_gr->gr_gid;
  } else {
      gid_len = strspn (argv[*arg_ptr], "0123456789");

      if ((gid_len == 0) || (argv[*arg_ptr][gid_len] != '\0')) {
         return (false);
      }

      gid = (uint_t) atoi (argv[*arg_ptr]);
  }

  our_pred = insert_victim (pred_group);
  our_pred->args.gid = gid;

  (*arg_ptr)++;

  return (true);
}

static boolean
parse_ilname (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_ilname);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_iname (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_iname);
  our_pred->need_stat = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_inum (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_num (argv, arg_ptr, pred_inum));
}

static boolean
parse_ipath (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_ipath);
  our_pred->need_stat = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_iregex (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return insert_regex (argv, arg_ptr, true);
}

static boolean
parse_links (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_num (argv, arg_ptr, pred_links));
}

static boolean
parse_lname (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_lname);
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_ls (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_ls);
  our_pred->side_effects = true;
  return (true);
}

static boolean
parse_maxdepth (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  int depth_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  depth_len = strspn (argv[*arg_ptr], "0123456789");
  if ((depth_len == 0) || (argv[*arg_ptr][depth_len] != '\0'))
    return (false);
  maxdepth = atoi (argv[*arg_ptr]);
  if (maxdepth < 0)
    return (false);
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_mindepth (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  int depth_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  depth_len = strspn (argv[*arg_ptr], "0123456789");
  if ((depth_len == 0) || (argv[*arg_ptr][depth_len] != '\0'))
    return (false);
  mindepth = atoi (argv[*arg_ptr]);
  if (mindepth < 0)
    return (false);
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_mmin (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  unsigned long num;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num_days (argv[*arg_ptr], &num, &c_type))
    return (false);
  our_pred = insert_victim (pred_mmin);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = cur_day_start + DAYSECS - num * 60;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_mtime (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_time (argv, arg_ptr, pred_mtime));
}

static boolean
parse_name (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_name);
  our_pred->need_stat = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_negate (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred_chk_op ();
  our_pred->pred_func = pred_negate;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_negate);
#endif	/* DEBUG */
  our_pred->p_type = UNI_OP;
  our_pred->p_prec = NEGATE_PREC;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_newer (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  struct sam_stat stat_newer;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if ((*StatFunc)(argv[*arg_ptr], &stat_newer, sizeof(stat_newer)))
    error (1, errno, "StatFunc(%s)", argv[*arg_ptr]);
  our_pred = insert_victim (pred_newer);
  our_pred->args.time = stat_newer.st_mtime;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_noleaf (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  no_leaf_check = true;
  return true;
}

#ifdef CACHE_IDS
/* Arbitrary amount by which to increase size
   of `uid_unused' and `gid_unused'. */
#define ALLOC_STEP 2048

/* Boolean: if uid_unused[n] is nonzero, then UID n has no passwd entry. */
char *uid_unused = NULL;

/* Number of elements in `uid_unused'. */
unsigned uid_allocated;

/* Similar for GIDs and group entries. */
char *gid_unused = NULL;
unsigned gid_allocated;
#endif

static boolean
parse_nogroup (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_nogroup);
#ifdef CACHE_IDS
  if (gid_unused == NULL)
    {
      struct group *gr;

      gid_allocated = ALLOC_STEP;
      gid_unused = xmalloc (gid_allocated);
      memset (gid_unused, 1, gid_allocated);
      setgrent ();
      while ((gr = getgrent ()) != NULL)
	{
	  if ((unsigned) gr->gr_gid >= gid_allocated)
	    {
	      unsigned new_allocated = gr->gr_gid + ALLOC_STEP;
	      gid_unused = xrealloc (gid_unused, new_allocated);
	      memset (gid_unused + gid_allocated, 1,
		      new_allocated - gid_allocated);
	      gid_allocated = new_allocated;
	    }
	  gid_unused[(unsigned) gr->gr_gid] = 0;
	}
      endgrent ();
    }
#endif
  return (true);
}

static boolean
parse_nouser (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_nouser);
#ifdef CACHE_IDS
  if (uid_unused == NULL)
    {
      struct passwd *pw;

      uid_allocated = ALLOC_STEP;
      uid_unused = xmalloc (uid_allocated);
      memset (uid_unused, 1, uid_allocated);
      setpwent ();
      while ((pw = getpwent ()) != NULL)
	{
	  if ((unsigned) pw->pw_uid >= uid_allocated)
	    {
	      unsigned new_allocated = pw->pw_uid + ALLOC_STEP;
	      uid_unused = xrealloc (uid_unused, new_allocated);
	      memset (uid_unused + uid_allocated, 1,
		      new_allocated - uid_allocated);
	      uid_allocated = new_allocated;
	    }
	  uid_unused[(unsigned) pw->pw_uid] = 0;
	}
      endpwent ();
    }
#endif
  return (true);
}

static boolean
parse_ok (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_exec_ok (pred_ok, argv, arg_ptr));
}

boolean
parse_open (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred_chk_op ();
  our_pred->pred_func = pred_open;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_open);
#endif	/* DEBUG */
  our_pred->p_type = OPEN_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_or (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = get_new_pred ();
  our_pred->pred_func = pred_or;
#ifdef	DEBUG
  our_pred->p_name = find_pred_name (pred_or);
#endif	/* DEBUG */
  our_pred->p_type = BI_OP;
  our_pred->p_prec = OR_PREC;
  our_pred->need_stat = false;
  return (true);
}

boolean do_ovfl (argv, arg_ptr, copy)
	char	*argv[];
	int		*arg_ptr;
	int		copy;
{
	Predicate *our_pred;
	int len;
	enum comparison_type kind;
	offset_t sections;

	our_pred = insert_victim (pred_sections);
	our_pred->args.vsn_mt.copy = copy;
	our_pred->args.size.kind = COMP_GT;
	our_pred->args.size.size = (offset_t) 1;

	return (true);
}

boolean parse_ovfl	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_ovfl(argv, arg_ptr, 0xf));
}

boolean parse_ovfl1	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_ovfl(argv, arg_ptr, 0x1));
}

boolean parse_ovfl2	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_ovfl(argv, arg_ptr, 0x2));
}

boolean parse_ovfl3	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_ovfl(argv, arg_ptr, 0x4));
}

boolean parse_ovfl4	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_ovfl(argv, arg_ptr, 0x8));
}

static boolean
parse_path (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_path);
  our_pred->need_stat = false;
  our_pred->args.str = argv[*arg_ptr];
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_perm (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  unsigned long perm_val;
  int mode_start = 0;
  struct mode_change *change;
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);

  switch (argv[*arg_ptr][0])
    {
    case '-':
    case '+':
      mode_start = 1;
      break;
    default:
      /* empty */
      break;
    }

  change = mode_compile (argv[*arg_ptr] + mode_start, MODE_MASK_PLUS);
  if (change == MODE_INVALID)
    error (1, 0, "invalid mode `%s'", argv[*arg_ptr]);
  else if (change == MODE_MEMORY_EXHAUSTED)
    error (1, 0, "virtual memory exhausted");
  perm_val = mode_adjust (0, change);
  mode_free (change);

  our_pred = insert_victim (pred_perm);

  switch (argv[*arg_ptr][0])
    {
    case '-':
      /* Set magic flag to indicate true if at least the given bits are set. */
      our_pred->args.perm = (perm_val & 07777) | 010000;
      break;
    case '+':
      /* Set magic flag to indicate true if any of the given bits are set. */
      our_pred->args.perm = (perm_val & 07777) | 020000;
      break;
    default:
      /* True if exactly the given bits are set. */
      our_pred->args.perm = (perm_val & 07777);
      break;
    }
  (*arg_ptr)++;
  return (true);
}

boolean
do_archpos(
char *argv[],
int *arg_ptr,
int copies)
{
	struct predicate *our_pred;
	enum comparison_type kind;
	offset_t position;
	int len;

	if ((argv == NULL) || (argv[*arg_ptr] == NULL)) {
		return(false);
	}

	len = strlen(argv[*arg_ptr]);
	if (len == 0) {
		error(1, 0, "invalid null argument to -archpos");
		return(false);
	}
	if (!get_num(argv[*arg_ptr], &position, &kind)) {
		kind  = COMP_EQ;
		if ((position = DiskVolsGenSequence(argv[*arg_ptr])) == -1) {
			return(false);
		}
	}

	our_pred = insert_victim(pred_archpos);
	our_pred->args.vsn_mt.copy = copies;
	our_pred->args.size.kind = kind;
	our_pred->args.size.size = position;

	(*arg_ptr)++;
	return(true);
}

boolean
parse_archpos(
char	*argv[],
int		*arg_ptr)
{
	return(do_archpos(argv, arg_ptr, 0xf));
}

boolean
parse_archpos1(
char	*argv[],
int		*arg_ptr)
{
	return(do_archpos(argv, arg_ptr, 0x1));
}

boolean
parse_archpos2(
char	*argv[],
int		*arg_ptr)
{
	return(do_archpos(argv, arg_ptr, 0x2));
}

boolean
parse_archpos3(
char	*argv[],
int		*arg_ptr)
{
	return(do_archpos(argv, arg_ptr, 0x4));
}

boolean
parse_archpos4(
char	*argv[],
int		*arg_ptr)
{
	return(do_archpos(argv, arg_ptr, 0x8));
}

boolean
parse_print (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_print);
  /* -print has the side effect of printing.  This prevents us
     from doing undesired multiple printing when the user has
     already specified -print. */
  our_pred->side_effects = true;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_print0 (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_print0);
  /* -print0 has the side effect of printing.  This prevents us
     from doing undesired multiple printing when the user has
     already specified -print0. */
  our_pred->side_effects = true;
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_printf (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  return (insert_fprintf (stdout, pred_fprintf, argv, arg_ptr));
}

static boolean
parse_prune (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_prune);
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_regex (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return insert_regex (argv, arg_ptr, false);
}

static boolean
insert_regex (argv, arg_ptr, ignore_case)
     char *argv[];
     int *arg_ptr;
     boolean ignore_case;
{
  struct predicate *our_pred;
  struct re_pattern_buffer *re;
  const char *error_message;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  our_pred = insert_victim (pred_regex);
  our_pred->need_stat = false;
  re = (struct re_pattern_buffer *)
    xmalloc (sizeof (struct re_pattern_buffer));
  our_pred->args.regex = re;
  re->allocated = 100;
  re->buffer = (unsigned char *) xmalloc (re->allocated);
  re->fastmap = NULL;

  if (ignore_case)
    {
      unsigned i;
      
      re->translate = xmalloc (256);
      /* Map uppercase characters to corresponding lowercase ones.  */
      for (i = 0; i < 256; i++)
        re->translate[i] = ISUPPER (i) ? tolower (i) : i;
    }
  else
    re->translate = NULL;

  error_message = xre_compile_pattern (argv[*arg_ptr], strlen (argv[*arg_ptr]),
				      re);
  if (error_message)
    error (1, 0, "%s", error_message);
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_size (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;
  offset_t num;
  offset_t nscale = 1LL;
  enum comparison_type c_type;
  offset_t blksize = 512;
  int len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  len = strlen (argv[*arg_ptr]);
  if (len == 0)
    error (1, 0, "invalid null argument to -size");
  switch (argv[*arg_ptr][len - 1])
    {
    case 'b':
    case 'c':
      blksize = 1;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'k':
      blksize = 1;
      nscale = 1024LL;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'm':
      blksize = 1;
      nscale = 1024*1024LL;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'g':
      blksize = 1;
      nscale = 1024*1024*1024LL;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 't':
      blksize = 1;
      nscale = 1024*1024*1024*1024LL;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;

    default:
      error (1, 0, "invalid -size type `%c'", argv[*arg_ptr][len - 1]);
    }
  if (!get_num (argv[*arg_ptr], &num, &c_type))
    return (false);
  our_pred = insert_victim (pred_size);
  our_pred->args.size.kind = c_type;
  our_pred->args.size.blocksize = blksize;
  our_pred->args.size.size = num * nscale;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_true (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct predicate *our_pred;

  our_pred = insert_victim (pred_true);
  our_pred->need_stat = false;
  return (true);
}

static boolean
parse_type (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return insert_type (argv, arg_ptr, pred_type);
}

static boolean
parse_uid (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return (insert_num (argv, arg_ptr, pred_uid));
}

static boolean
parse_used (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;

{
  struct predicate *our_pred;
  offset_t num_days;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num (argv[*arg_ptr], &num_days, &c_type))
    return (false);
  our_pred = insert_victim (pred_used);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = num_days * DAYSECS;
  (*arg_ptr)++;
  return (true);
}

static boolean
parse_user (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  struct passwd *cur_pwd;
  struct predicate *our_pred;
  uint_t uid;
  int uid_len;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL)) {
    return (false);
  }

  cur_pwd = getpwnam (argv[*arg_ptr]);
  endpwent ();

  if (cur_pwd != NULL) {
    uid = (uint_t) cur_pwd->pw_uid;
  } else {
      uid_len = strspn (argv[*arg_ptr], "0123456789");
      if ((uid_len == 0) || (argv[*arg_ptr][uid_len] != '\0')) {
        return (false);
      }
      uid = (uint_t) atoi (argv[*arg_ptr]);
  }

  our_pred = insert_victim (pred_user);
  our_pred->args.uid = uid;

  (*arg_ptr)++;

  return (true);
}

static boolean
parse_version (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  extern char *version_string;

  fflush (stdout);
  fprintf (stderr, "%s", version_string);
  fflush (stderr);
  return true;
}

static boolean
parse_xdev (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  stay_on_filesystem = true;
  return true;
}

static boolean
parse_xtype (argv, arg_ptr)
     char *argv[];
     int *arg_ptr;
{
  return insert_type (argv, arg_ptr, pred_xtype);
}

static boolean
insert_type (argv, arg_ptr, which_pred)
     char *argv[];
     int *arg_ptr;
     boolean (*which_pred) ();
{
  unsigned long type_cell;
  struct predicate *our_pred;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL)
      || (strlen (argv[*arg_ptr]) != 1))
    return (false);
  switch (argv[*arg_ptr][0])
    {
    case 'b':			/* block special */
      type_cell = S_IFBLK;
      break;
    case 'c':			/* character special */
      type_cell = S_IFCHR;
      break;
    case 'd':			/* directory */
      type_cell = S_IFDIR;
      break;
    case 'f':			/* regular file */
      type_cell = S_IFREG;
      break;
#ifdef S_IFLNK
    case 'l':			/* symbolic link */
      type_cell = S_IFLNK;
      break;
#endif
#ifdef S_IFIFO
    case 'p':			/* pipe */
      type_cell = S_IFIFO;
      break;
#endif
#ifdef S_IFSOCK
    case 's':			/* socket */
      type_cell = S_IFSOCK;
      break;
#endif
	case 'R':			/* removable media */
		type_cell = 0xe000/*S_IFREQ*/;
		break;

    default:			/* None of the above ... nuke 'em. */
      return (false);
    }
  our_pred = insert_victim (which_pred);
  our_pred->args.type = type_cell;
  (*arg_ptr)++;			/* Move on to next argument. */
  return (true);
}

/* If true, we've determined that the current fprintf predicate
   uses stat information. */
static boolean fprintf_stat_needed;

static boolean
insert_fprintf (fp, func, argv, arg_ptr)
     FILE *fp;
     boolean (*func) ();
     char *argv[];
     int *arg_ptr;
{
  char *format;			/* Beginning of unprocessed format string. */
  register char *scan;		/* Current address in scanning `format'. */
  register char *scan2;		/* Address inside of element being scanned. */
  struct segment **segmentp;	/* Address of current segment. */
  struct predicate *our_pred;

  format = argv[(*arg_ptr)++];

  fprintf_stat_needed = false;	/* Might be overridden later. */
  our_pred = insert_victim (func);
  our_pred->side_effects = true;
  our_pred->args.printf_vec.stream = fp;
  segmentp = &our_pred->args.printf_vec.segment;
  *segmentp = NULL;

  for (scan = format; *scan; scan++)
    {
      if (*scan == '\\')
	{
	  scan2 = scan + 1;
	  if (*scan2 >= '0' && *scan2 <= '7')
	    {
	      register int n, i;

	      for (i = n = 0; i < 3 && (*scan2 >= '0' && *scan2 <= '7');
		   i++, scan2++)
		n = 8 * n + *scan2 - '0';
	      scan2--;
	      *scan = n;
	    }
	  else
	    {
	      switch (*scan2)
		{
		case 'a':
		  *scan = 7;
		  break;
		case 'b':
		  *scan = '\b';
		  break;
		case 'c':
		  make_segment (segmentp, format, scan - format, KIND_STOP);
		  return (true);
		case 'f':
		  *scan = '\f';
		  break;
		case 'n':
		  *scan = '\n';
		  break;
		case 'r':
		  *scan = '\r';
		  break;
		case 't':
		  *scan = '\t';
		  break;
		case 'v':
		  *scan = '\v';
		  break;
		case '\\':
		  /* *scan = '\\'; * it already is */
		  break;
		default:
		  scan++;
		  continue;
		}
	    }
	  segmentp = make_segment (segmentp, format, scan - format + 1,
				   KIND_PLAIN);
	  format = scan2 + 1;	/* Move past the escape. */
	  scan = scan2;		/* Incremented immediately by `for'. */
	}
      else if (*scan == '%')
	{
	  if (scan[1] == '%')
	    {
	      segmentp = make_segment (segmentp, format, scan - format + 1,
				       KIND_PLAIN);
	      scan++;
	      format = scan + 1;
	      continue;
	    }
	  /* Scan past flags, width and precision, to verify kind. */
	  for (scan2 = scan; *++scan2 && strchr ("-+ #", *scan2);)
	    /* Do nothing. */ ;
	  while (ISDIGIT (*scan2))
	    scan2++;
	  if (*scan2 == '.')
	    for (scan2++; ISDIGIT (*scan2); scan2++)
	      /* Do nothing. */ ;
	  if (strchr ("abcdfFgGhHikKlmnpPQrstuUwZ", *scan2))
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       (int) *scan2);
	      scan = scan2;
	      format = scan + 1;
	    }
	  else if (strchr ("ACT", *scan2) && scan2[1])
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       *scan2 | (scan2[1] << 8));
	      scan = scan2 + 1;
	      format = scan + 1;
	      continue;
	    }
	  else
	    {
	      /* An unrecognized % escape.  Print the char after the %. */
	      segmentp = make_segment (segmentp, format, scan - format,
				       KIND_PLAIN);
	      format = scan + 1;
	      continue;
	    }
	}
    }

  if (scan > format)
    make_segment (segmentp, format, scan - format, KIND_PLAIN);
  our_pred->need_stat = fprintf_stat_needed;
  return (true);
}

/* Create a new fprintf segment in *SEGMENT, with type KIND,
   from the text in FORMAT, which has length LEN.
   Return the address of the `next' pointer of the new segment. */

static struct segment **
make_segment (segment, format, len, kind)
     struct segment **segment;
     char *format;
     int len, kind;
{
  char *fmt;

  *segment = (struct segment *) xmalloc (sizeof (struct segment));

  (*segment)->kind = kind;
  (*segment)->next = NULL;
  (*segment)->text_len = len;

  fmt = (*segment)->text = xmalloc (len + 4);	/* room for "lld\0" */
  strncpy (fmt, format, len);
  fmt += len;

  switch (kind & 0xff)
    {
    case KIND_PLAIN:		/* Plain text string, no % conversion. */
    case KIND_STOP:		/* Terminate argument, no newline. */
      break;

    case 'a':			/* atime in `ctime' format */
    case 'c':			/* ctime in `ctime' format */
    case 'F':			/* filesystem type */
    case 'g':			/* group name */
    case 'l':			/* object of symlink */
    case 't':			/* mtime in `ctime' format */
    case 'u':			/* user name */
    case 'A':			/* atime in user-specified strftime format */
    case 'C':			/* ctime in user-specified strftime format */
    case 'T':			/* mtime in user-specified strftime format */
      fprintf_stat_needed = true;
      /* FALLTHROUGH */
    case 'f':			/* basename of path */
    case 'h':			/* leading directories part of path */
    case 'H':			/* ARGV element file was found under */
    case 'p':			/* pathname */
    case 'P':			/* pathname with ARGV element stripped */
      *fmt++ = 's';
      break;

    case 'b':			/* size in 512-byte blocks */
    case 'k':			/* size in 1K blocks */
    case 's':			/* size in bytes */
      *fmt++ = 'l';
      *fmt++ = 'l';
      /*FALLTHROUGH*/
    case 'n':			/* number of links */
      fprintf_stat_needed = true;
      /* FALLTHROUGH */
    case 'd':			/* depth in search tree (0 = ARGV element) */
      *fmt++ = 'd';
      break;

    case 'K':			/* segment number */
      *fmt++ = 's';
      break;

    case 'Q':			/* Quantity of segments */
      *fmt++ = 's';
      break;

    case 'Z':			/* Segment length setting */
      *fmt++ = 's';
      break;

    case 'r':			/* setfa -g stripe group setting */
      *fmt++ = 's';
      break;

    case 'w':			/* setfa -s stripe width setting */
      *fmt++ = 's';
      break;

    case 'i':			/* inode number */
      *fmt++ = 'l';
      /*FALLTHROUGH*/
    case 'G':			/* GID number */
    case 'U':			/* UID number */
      *fmt++ = 'u';
      fprintf_stat_needed = true;
      break;

    case 'm':			/* mode as octal number (perms only) */
      *fmt++ = 'o';
      fprintf_stat_needed = true;
      break;
    }
  *fmt = '\0';

  return (&(*segment)->next);
}

static boolean
insert_exec_ok (func, argv, arg_ptr)
     boolean (*func) ();
     char *argv[];
     int *arg_ptr;
{
  int start, end;		/* Indexes in ARGV of start & end of cmd. */
  int num_paths;		/* Number of args with path replacements. */
  int path_pos;			/* Index in array of path replacements. */
  int vec_pos;			/* Index in array of args. */
  struct predicate *our_pred;
  struct exec_val *execp;	/* Pointer for efficiency. */

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);

  /* Count the number of args with path replacements, up until the ';'. */
  start = *arg_ptr;
  for (end = start, num_paths = 0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    if (strstr (argv[end], "{}"))
      num_paths++;
  /* Fail if no command given or no semicolon found. */
  if ((end == start) || (argv[end] == NULL))
    {
      *arg_ptr = end;
      return (false);
    }

  our_pred = insert_victim (func);
  our_pred->side_effects = true;
  execp = &our_pred->args.exec_vec;
  execp->paths =
    (struct path_arg *) xmalloc (sizeof (struct path_arg) * (num_paths + 1));
  execp->vec = (char **) xmalloc (sizeof (char *) * (end - start + 1));
  /* Record the positions of all args, and the args with path replacements. */
  for (end = start, path_pos = vec_pos = 0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    {
      register char *p;
      
      execp->paths[path_pos].count = 0;
      for (p = argv[end]; *p; ++p)
	if (p[0] == '{' && p[1] == '}')
	  {
	    execp->paths[path_pos].count++;
	    ++p;
	  }
      if (execp->paths[path_pos].count)
	{
	  execp->paths[path_pos].offset = vec_pos;
	  execp->paths[path_pos].origarg = argv[end];
	  path_pos++;
	}
      execp->vec[vec_pos++] = argv[end];
    }
  execp->paths[path_pos].offset = -1;
  execp->vec[vec_pos] = NULL;

  if (argv[end] == NULL)
    *arg_ptr = end;
  else
    *arg_ptr = end + 1;
  return (true);
}

/* Get a number of days and comparison type.
   STR is the ASCII representation.
   Set *NUM_DAYS to the number of days, taken as being from
   the current moment (or possibly midnight).  Thus the sense of the
   comparison type appears to be reversed.
   Set *COMP_TYPE to the kind of comparison that is requested.

   Return true if all okay, false if input error.

   Used by -atime, -ctime and -mtime (parsers) to
   get the appropriate information for a time predicate processor. */

static boolean
get_num_days (str, num_days, comp_type)
     char *str;
     unsigned long *num_days;
     enum comparison_type *comp_type;
{
  int len_days;			/* length of field */

  if (str == NULL)
    return (false);
  switch (str[0])
    {
    case '+':
      *comp_type = COMP_LT;
      str++;
      break;
    case '-':
      *comp_type = COMP_GT;
      str++;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      *comp_type = COMP_EQ;
      break;
    default:
      return (false);
    }

  /* We know the first char has been reasonable.  Find the
     number of days to play with. */
  len_days = strspn (str, "0123456789");
  if ((len_days == 0) || (str[len_days] != '\0'))
    return (false);
  *num_days = (unsigned long) atol (str);
  return (true);
}

/* Insert a time predicate PRED.
   ARGV is a pointer to the argument array.
   ARG_PTR is a pointer to an index into the array, incremented if
   all went well.

   Return true if input is valid, false if not.

   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -atime, -ctime, and -mtime parsers. */

static boolean
insert_time (argv, arg_ptr, pred)
     char *argv[];
     int *arg_ptr;
     PFB pred;
{
  struct predicate *our_pred;
  unsigned long num_days;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num_days (argv[*arg_ptr], &num_days, &c_type))
    return (false);
  our_pred = insert_victim (pred);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = cur_day_start - num_days * DAYSECS
    + ((c_type == COMP_GT) ? DAYSECS - 1 : 0);
  (*arg_ptr)++;
#ifdef	DEBUG
  printf ("inserting %s\n", our_pred->p_name);
  printf ("    type: %s    %s  ",
	  (c_type == COMP_GT) ? "gt" :
	  ((c_type == COMP_LT) ? "lt" : ((c_type == COMP_EQ) ? "eq" : "?")),
	  (c_type == COMP_GT) ? " >" :
	  ((c_type == COMP_LT) ? " <" : ((c_type == COMP_EQ) ? ">=" : " ?")));
  printf ("%ld %s", our_pred->args.info.l_val,
	  ctime ((time_t *)&our_pred->args.info.l_val));
  if (c_type == COMP_EQ)
    {
      our_pred->args.info.l_val += DAYSECS;
      printf ("                 <  %ld %s", our_pred->args.info.l_val,
	      ctime ((time_t *)&our_pred->args.info.l_val));
      our_pred->args.info.l_val -= DAYSECS;
    }
#endif	/* DEBUG */
  return (true);
}

/* Get a number with comparision information.
   The sense of the comparision information is 'normal'; that is,
   '+' looks for inums or links > than the number and '-' less than.
   
   STR is the ASCII representation of the number.
   Set *NUM to the number.
   Set *COMP_TYPE to the kind of comparison that is requested.
 
   Return true if all okay, false if input error.

   Used by the -inum and -links predicate parsers. */

static boolean
get_num (str, num, comp_type)
     char *str;
     offset_t *num;
     enum comparison_type *comp_type;
{
  int len_num;			/* Length of field. */
  char *endptr;

  if (str == NULL)
    return (false);
  switch (str[0])
    {
    case '+':
      *comp_type = COMP_GT;
      str++;
      break;
    case '-':
      *comp_type = COMP_LT;
      str++;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      *comp_type = COMP_EQ;
      break;
    default:
      return (false);
    }

	*num = (offset_t) strtoll(str, &endptr, 0);
	if ((endptr == str) || (*endptr != '\0')) {
		return (false);
	}
	return (true);
}

/* Insert a number predicate.
   ARGV is a pointer to the argument array.
   *ARG_PTR is an index into ARGV, incremented if all went well.
   *PRED is the predicate processor to insert.

   Return true if input is valid, false if error.
   
   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -inum and -links parsers. */

static boolean
insert_num (argv, arg_ptr, pred)
     char *argv[];
     int *arg_ptr;
     PFB pred;
{
  struct predicate *our_pred;
  offset_t num;
  enum comparison_type c_type;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return (false);
  if (!get_num (argv[*arg_ptr], &num, &c_type))
    return (false);
  our_pred = insert_victim (pred);
  our_pred->args.info.kind = c_type;
  our_pred->args.info.l_val = num;
  (*arg_ptr)++;
#ifdef	DEBUG
  printf ("inserting %s\n", our_pred->p_name);
  printf ("    type: %s    %s  ",
	  (c_type == COMP_GT) ? "gt" :
	  ((c_type == COMP_LT) ? "lt" : ((c_type == COMP_EQ) ? "eq" : "?")),
	  (c_type == COMP_GT) ? " >" :
	  ((c_type == COMP_LT) ? " <" : ((c_type == COMP_EQ) ? " =" : " ?")));
  printf ("%ld\n", our_pred->args.info.l_val);
#endif	/* DEBUG */
  return (true);
}

static FILE *
open_output_file (path)
     char *path;
{
  FILE *f;

  if (!strcmp (path, "/dev/stderr"))
    return (stderr);
  else if (!strcmp (path, "/dev/stdout"))
    return (stdout);
  f = fopen (path, "w");
  if (f == NULL)
    error (1, errno, "%s", path);
  return (f);
}

boolean	parse_aid(
char *argv[],
int *arg_ptr)
{
	return (insert_num (argv, arg_ptr, pred_aid));
}


boolean	parse_admin_id(
char *argv[],
int *arg_ptr)
{
	/*
	 * Someday, if we support an /etc/admins file, then
	 * we'd need to map names to numbers here.
	 */
	return (insert_num (argv, arg_ptr, pred_aid));
}

boolean	parse_archived	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_archived);
	return (true);
}

boolean	parse_archive_d	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_archive_d);
	return (true);
}

boolean	parse_archive_n	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_archive_n);
	return (true);
}

boolean	parse_archdone	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_archdone);
	return (true);
}

boolean	parse_copies(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	offset_t copies;
	int len;
	enum comparison_type c_type;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return(false);
	}
	len = strlen(argv[*arg_ptr]);
	if (len < 1 ||
		(len == 2 && argv[*arg_ptr][0] != '+' && argv[*arg_ptr][0] != '-' ) ||
		len > 2) {
		error (1, 0, "invalid argument to -copies: %s", argv[*arg_ptr]);
	}

	switch (argv[*arg_ptr][len - 1]) {

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
		break;
	default:
		return(false);
	}
	if (!get_num (argv[*arg_ptr], &copies, &c_type))
		return (false);
	pred_ptr = insert_victim(pred_copies);
	pred_ptr->args.size.kind = c_type;
	pred_ptr->args.size.size = copies;
	(*arg_ptr)++;
	return(true);
}

boolean	parse_copy(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	char ch;
	int copy;

	if (argv == NULL || argv[*arg_ptr] == NULL || strlen(argv[*arg_ptr]) != 1) {
    	return(false);
	}
	ch = argv[*arg_ptr][0];
	(*arg_ptr)++;
	switch (ch) {
	case '1':		
	case '2':		
	case '3':		
	case '4':		
		copy = ch - '0';
		break;
	default:
		return(false);
	}
	pred_ptr = insert_victim(pred_copy);
	pred_ptr->args.type = copy - 1;
	return(true);
}

boolean	parse_any_copy_d(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_any_copy_d);
	return (true);
}

boolean	parse_copy_d(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	char ch;
	int copy;

	if (argv == NULL || argv[*arg_ptr] == NULL || strlen(argv[*arg_ptr]) != 1) {
    	return(false);
	}
	ch = argv[*arg_ptr][0];
	(*arg_ptr)++;
	switch (ch) {
	case '1':		
	case '2':		
	case '3':		
	case '4':		
		copy = ch - '0';
		break;
	default:
		return(false);
	}
	pred_ptr = insert_victim(pred_copy_d);
	pred_ptr->args.type = copy - 1;
	return(true);
}

boolean	parse_any_copy_p(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_any_copy_p);
	return (true);
}

boolean	parse_any_copy_r(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_any_copy_r);
	return (true);
}

boolean	parse_copy_r(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	char ch;
	int copy;

	if (argv == NULL || argv[*arg_ptr] == NULL || strlen(argv[*arg_ptr]) != 1) {
    	return(false);
	}
	ch = argv[*arg_ptr][0];
	(*arg_ptr)++;
	switch (ch) {
	case '1':		
	case '2':		
	case '3':		
	case '4':		
		copy = ch - '0';
		break;
	default:
		return(false);
	}
	pred_ptr = insert_victim (pred_copy_r);
	pred_ptr->args.type = copy - 1;
	return(true);
}

boolean	parse_any_copy_v(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_any_copy_v);
	return (true);
}

boolean parse_copy_v(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	char ch;
	int copy;

	if (argv == NULL || argv[*arg_ptr] == NULL || strlen(argv[*arg_ptr]) != 1) {
    	return(false);
	}
	ch = argv[*arg_ptr][0];
	(*arg_ptr)++;
	switch (ch) {
	case '1':		
	case '2':		
	case '3':		
	case '4':		
		copy = ch - '0';
		break;
	default:
		return(false);
	}
	pred_ptr = insert_victim (pred_copy_v);
	pred_ptr->args.type = copy - 1;
	return(true);
}

static boolean
parse_data_segments (argv, arg_ptr)
	 char *argv[];
	 int *arg_ptr;
{
	output_data_segments = true;
	return (true);
}

boolean	parse_damaged	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_damaged);
	return (true);
}

boolean	parse_number_of_segments(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	offset_t segments;
	int len;
	enum comparison_type c_type;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return(false);
	}
	len = strlen(argv[*arg_ptr]);	

	if (len == 0) {
		error (1, 0, "invalid null argument to -segments");
		return (false);
	}

	if (!get_num (argv[*arg_ptr], &segments, &c_type)) {
		return (false);
	}

	pred_ptr = insert_victim(pred_number_of_segments);

	pred_ptr->args.size.kind = c_type;
	pred_ptr->args.size.size = segments;
	(*arg_ptr)++;

	return(true);
}


boolean	parse_offline	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_offline);
	return (true);
}
 
boolean	parse_online	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_online);
	return (true);
}

boolean	parse_partial_on(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_partial_on);
	return (true);
}

boolean	parse_release_d	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_release_d);
	return (true);
}

boolean	parse_release_a	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_release_a);
	return (true);
}

boolean	parse_release_n	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_release_n);
	return (true);
}

boolean	parse_release_p	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_release_p);
	return (true);
}

boolean	parse_rmin	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	Predicate	*our_pred;
	unsigned long	num;
	Comparison_type	c_type;

	if  ((argv == NULL) || (argv[*arg_ptr] == NULL)) return (false);
	if  (!get_num_days (argv[*arg_ptr], &num, &c_type)) return (false);
	our_pred = insert_victim (pred_rmin);
	our_pred->args.info.kind  = c_type;
	our_pred->args.info.l_val = cur_day_start + DAYSECS - num * 60;
	(*arg_ptr)++;
	return (true);
}

boolean	parse_rtime	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return (insert_time (argv, arg_ptr, pred_rtime));
}

boolean do_sections (argv, arg_ptr, test_name, copy)
	char	*argv[];
	int		*arg_ptr;
	char	*test_name;
	int		copy;
{
	Predicate *our_pred;
	int len;
	enum comparison_type kind;
	offset_t sections;

	if ((argv == NULL) || (argv[*arg_ptr] == NULL)) {
		return (false);
	}

	len = strlen(argv[*arg_ptr]);	
	if (len == 0) {
		error (1, 0, "invalid null argument to %s", test_name);
		return (false);
	}

	if (!get_num (argv[*arg_ptr], &sections, &kind)) {
		return (false);
	}

	our_pred = insert_victim (pred_sections);
	our_pred->args.vsn_mt.copy = copy;
	our_pred->args.size.kind = kind;
	our_pred->args.size.size = sections;

	(*arg_ptr)++;
	return (true);
}

boolean parse_sections	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_sections(argv, arg_ptr, "-sections", 0xf));
}

boolean parse_sections1	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_sections(argv, arg_ptr, "-sections1", 0x1));
}

boolean parse_sections2	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_sections(argv, arg_ptr, "-sections2", 0x2));
}

boolean parse_sections3	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(do_sections(argv, arg_ptr, "-sections3", 0x4));
}

boolean parse_sections4	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_sections(argv, arg_ptr, "-sections4", 0x8));
}

boolean parse_segment_a (argv, arg_ptr)
	char		*argv[];
	int			*arg_ptr;


{
	insert_victim (pred_segment_a);

	return (true);
}

boolean parse_segment_i (argv, arg_ptr)
	char		*argv[];
	int			*arg_ptr;
{
	parse_data_segments(argv, arg_ptr);

	insert_victim (pred_segment_i);

	return (true);
}

boolean parse_segment_s (argv, arg_ptr)
	char		*argv[];
	int			*arg_ptr;
{
	parse_data_segments(argv, arg_ptr);


	insert_victim (pred_segment_s);

	return (true);
}

boolean	parse_segment_number (
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	offset_t segment_number;
	int len;
	enum comparison_type c_type;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return(false);
	}

	len = strlen(argv[*arg_ptr]);	

	if (len == 0) {
		error (1, 0, "invalid null argument to -segment");
	}

	if (!get_num (argv[*arg_ptr], &segment_number, &c_type)) {
		return (false);
	}

	(void) parse_data_segments();

	pred_ptr = insert_victim(pred_segment_number);
	pred_ptr->args.size.kind = c_type;
	pred_ptr->args.size.size = segment_number;
	(*arg_ptr)++;

	return(true);
}

boolean parse_segmented	(argv, arg_ptr)
	char		*argv[];
	int			*arg_ptr;
{
	insert_victim (pred_segmented);
	return (true);
}

boolean parse_is_setfa_g(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_is_setfa_g);
	return (true);
}

boolean parse_is_setfa_s(
char *argv[],
int *arg_ptr)
{
	insert_victim (pred_is_setfa_s);
	return (true);
}

boolean parse_setfa_g(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	offset_t group_num;
	int len;
	enum comparison_type c_type;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return(false);
	}
	len = strlen(argv[*arg_ptr]);	
	if (len == 0) {
		error (1, 0, "invalid null argument to -setfa_g");
		return (false);
	}

	if (!get_num (argv[*arg_ptr], &group_num, &c_type)) {
		return (false);
	}

	pred_ptr = insert_victim(pred_setfa_g);
	pred_ptr->args.size.kind = c_type;
	pred_ptr->args.size.size = group_num;
	(*arg_ptr)++;

	return(true);
}

boolean parse_setfa_s(
char *argv[],
int *arg_ptr)
{
	Predicate *pred_ptr;
	offset_t group_width;
	int len;
	enum comparison_type c_type;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return(false);
	}
	len = strlen(argv[*arg_ptr]);	
	if (len == 0) {
		error (1, 0, "invalid null argument to -setfa_s");
		return (false);
	}

	if (!get_num (argv[*arg_ptr], &group_width, &c_type)) {
		return (false);
	}

	pred_ptr = insert_victim(pred_setfa_s);
	pred_ptr->args.size.kind = c_type;
	pred_ptr->args.size.size = group_width;
	(*arg_ptr)++;

	return(true);
}

boolean	parse_ssum_g	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_ssum_g);
	return (true);
}

boolean	parse_ssum_u	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_ssum_u);
	return (true);
}

boolean	parse_ssum_v	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_ssum_v);
	return (true);
}

boolean	parse_stage_d	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_stage_d);
	return (true);
}

boolean	parse_stage_a	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_stage_a);
	return (true);
}

boolean	parse_stage_n	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_stage_n);
	return (true);
}

boolean parse_verify	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_verify);
	return (true);
}

boolean	parse_xmin	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	Predicate	*our_pred;
	unsigned long	num;
	Comparison_type	c_type;

	if  ((argv == NULL) || (argv[*arg_ptr] == NULL)) return (false);
	if  (!get_num_days (argv[*arg_ptr], &num, &c_type)) return (false);
	our_pred = insert_victim (pred_xmin);
	our_pred->args.info.kind  = c_type;
	our_pred->args.info.l_val = cur_day_start + DAYSECS - num * 60;
	(*arg_ptr)++;
	return (true);
}

boolean	parse_xtime	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return (insert_time (argv, arg_ptr, pred_xtime));
}

boolean	do_vsn		(argv, arg_ptr, copy )
	char		*argv[];
	int		*arg_ptr;
	int		copy;
{
	Predicate	*our_pred;
	char		*s;

	if ((argv == NULL) || (argv[*arg_ptr] == NULL))
		return (false);
	our_pred = insert_victim (pred_vsn);
	our_pred->args.vsn_mt.copy = copy;
	our_pred->args.vsn_mt.key = argv[*arg_ptr];
		
	(*arg_ptr)++;
	return (true);
}

boolean parse_vsn	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_vsn( argv, arg_ptr, 0xf ));
}

boolean parse_vsn1	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_vsn( argv, arg_ptr, 0x1 ));
}

boolean parse_vsn2	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_vsn( argv, arg_ptr, 0x2 ));
}

boolean parse_vsn3	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_vsn( argv, arg_ptr, 0x4 ));
}

boolean parse_vsn4	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return( do_vsn( argv, arg_ptr, 0x8 ));
}

boolean do_mt		(argv, arg_ptr, copy)
	char		*argv[];
	int		*arg_ptr;
	int		copy;
{
	Predicate	*our_pred;
	int 		i;
	char		*s;

	if (argv == NULL || argv[*arg_ptr] == NULL)  return (false);
	our_pred = insert_victim (pred_mt);
	our_pred->args.vsn_mt.copy = copy;
	our_pred->args.vsn_mt.key = argv[*arg_ptr];
	(*arg_ptr)++;
	return(true);
}

boolean parse_mt	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(	do_mt( argv, arg_ptr, 0xf ));
}

boolean parse_mt1	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(	do_mt( argv, arg_ptr, 0x1 ));
}

boolean parse_mt2	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(	do_mt( argv, arg_ptr, 0x2 ));
}

boolean parse_mt3	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(	do_mt( argv, arg_ptr, 0x4 ));
}

boolean parse_mt4	(argv, arg_ptr)
	char		*argv[];
	int		*arg_ptr;
{
	return(	do_mt( argv, arg_ptr, 0x8 ));
}

boolean   parse_worm  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_worm);
	return (true);
}

boolean   parse_expired  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	insert_victim (pred_expired);
	return (true);
}

boolean   parse_retention  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	struct predicate *our_pred;
	long num;
	enum comparison_type c_type;
	char remain[256];

	if (argv[*arg_ptr][0] == '+') {
		c_type = COMP_GT;
		strcpy(remain, &argv[*arg_ptr][1]);
	} else if (argv[*arg_ptr][0] == '-') {
		c_type = COMP_LT;
		strcpy(remain, &argv[*arg_ptr][1]);
	} else if (strspn(argv[*arg_ptr], "0123456789") != 0) {
		c_type = COMP_EQ;
		strcpy(remain, &argv[*arg_ptr][0]);
	} else {
		return (false);
	}

	if (argv == NULL)
		return (false);
	if (argv[*arg_ptr] != NULL) {
		if (StrToMinutes(remain, &num) == 0) {
			our_pred = insert_victim (pred_retention);
			our_pred->args.info.kind = c_type;
			our_pred->args.info.l_val = num;
			(*arg_ptr)++;
			return (true);
		}
	}
	return (false);
}

boolean   parse_remain  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	struct predicate *our_pred;
	long num;

	if (argv == NULL)
		return (false);
	if (argv[*arg_ptr] != NULL) {
		if (StrToMinutes(argv[*arg_ptr], &num) == 0) {
			our_pred = insert_victim (pred_remain);
			our_pred->args.info.l_val = num;
			(*arg_ptr)++;
			return (true);
		}
	}
	return (false);
}

boolean   parse_longer  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	struct predicate *our_pred;
	long num;

	if (argv == NULL)
		return (false);
	if (argv[*arg_ptr] != NULL) {
		if (StrToMinutes(argv[*arg_ptr], &num) == 0) {
			our_pred = insert_victim (pred_retention);
			our_pred->args.info.kind = COMP_GT;
			our_pred->args.info.l_val = num;
			(*arg_ptr)++;
			return (true);
		}
	}
	return (false);
}

boolean   parse_permanent  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	struct predicate *our_pred;

	if (argv == NULL)
		return (false);

	our_pred = insert_victim (pred_retention);
	our_pred->args.info.kind = COMP_EQ;
	our_pred->args.info.l_val = 0;
	(*arg_ptr)++;
	return (true);
}

boolean   parse_after  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	struct predicate *our_pred;
	long num;

	if (argv == NULL)
		return (false);
	if (argv[*arg_ptr] != NULL) {
		if (DateToMinutes(argv[*arg_ptr], &num) == 0) {
			our_pred = insert_victim (pred_after);
			our_pred->args.info.l_val = num;
			(*arg_ptr)++;
			return (true);
		}
	}
	return (false);
}

#ifdef sun
boolean   parse_project  (argv, arg_ptr)
	char	*argv[];
	int	*arg_ptr;
{
	char *pname;
	Predicate *pred_ptr;
	projid_t projid;
	int projid_len;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return (false);
	}

	pname = strdup(argv[*arg_ptr]);
	projid = getprojidbyname(argv[*arg_ptr]);
	if (projid < 0) {
		projid_len = strspn(argv[*arg_ptr], "0123456789");

		if ((projid_len == 0) || (argv[*arg_ptr][projid_len] != '\0')) {
			return (false);
		}

		projid = (projid_t)atoi(argv[*arg_ptr]);
	}

	pred_ptr = insert_victim(pred_project);
	pred_ptr->args.projid = projid;
	(*arg_ptr)++;

	return (true);
}
#endif /* sun */

boolean    parse_xattr   (argv, arg_ptr)
    char    *argv[];
    int     *arg_ptr;
{
    insert_victim (pred_xattr);
    return (true);
}

#ifdef linux
	/*
	 * Can only accept numeric project IDs on Linux
	 */
boolean   parse_project  (argv, arg_ptr)
	char	*argv[];
	int		*arg_ptr;
{
	Predicate *pred_ptr;
	projid_t projid;
	int projid_len;

	if (argv == NULL || argv[*arg_ptr] == NULL) {
		return (false);
	}

	projid_len = strspn(argv[*arg_ptr], "0123456789");

	if ((projid_len == 0) || (argv[*arg_ptr][projid_len] != '\0')) {
		return (false);
	}

	projid = (projid_t)atoi(argv[*arg_ptr]);

	pred_ptr = insert_victim(pred_project);
	pred_ptr->args.projid = projid;
	(*arg_ptr)++;

	return (true);

}
#endif /* linux */
