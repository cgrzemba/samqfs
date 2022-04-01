/* find -- search for files in a directory hierarchy
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

/* GNU find was written by Eric Decker (cire@cisco.com),
   with enhancements by David MacKenzie (djm@gnu.ai.mit.edu),
   Jay Plett (jay@silence.princeton.nj.us),
   and Tim Wood (axolotl!tim@toad.com).
   The idea for -print0 and xargs -0 came from
   Dan Bernstein (brnstnd@kramden.acf.nyu.edu).  */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef sun
#include <utility.h>
#endif /* sun */
#include <pub/lib.h>
#include <pub/stat.h>

#include "defs.h"
#include "savedir.h"

/* SAM-FS headers. */
#define DEC_INIT
#include <sam/lib.h>

#if !defined(FALSE)
#define FALSE (0)
#endif
#if !defined(TRUE)
#define TRUE (1)
#endif

#define apply_predicate_to_one_item(pathname, stat_buf_ptr, node)	\
  (*(node)->pred_func)((pathname), (stat_buf_ptr), (node))

static int integer_length (int value);
static int setup_pathname_for_seg_number(char *file_pathname, int n_segs,
											char **dest, int *len_pathname_ptr,
											int *dest_size_ptr);
void free_memory_used_for_testing_segments();
void apply_predicate(char *pathname, struct sam_stat *stat_buf_ptr,
					struct predicate *eval_tree_ptr);
boolean mark_stat ();
boolean opt_expr ();
boolean parse_open ();
boolean parse_close ();

static void scan_directory ();
static int process_path ();

/* Name this program was run with. */
char *program_name;

/* All predicates for each path to process. */
struct predicate *predicates;

/* The last predicate allocated. */
struct predicate *last_pred;

/* The root of the evaluation tree. */
static struct predicate *eval_tree;

/* If true, process directory before contents.  True unless -depth given. */
boolean do_dir_first;

/* If >=0, don't descend more than this many levels of subdirectories. */
int maxdepth;

/* If >=0, don't process files above this level. */
int mindepth;

/* Current depth; 0 means current path is a command line arg. */
int curdepth;

/* Seconds between 00:00 1/1/70 and either one day before now
   (the default), or the start of today (if -daystart is given). */
time_t cur_day_start;

/* If true, cur_day_start has been adjusted to the start of the day. */
boolean full_days;

/* If true, do not assume that files in directories with nlink == 2
   are non-directories. */
boolean no_leaf_check;

/* If true, don't cross filesystem boundaries. */
boolean stay_on_filesystem;

/* If true, don't descend past current directory.
   Can be set by -prune, -maxdepth, and -xdev. */
boolean stop_at_current_level;

/* If true, we have called stat on the current path. */
boolean have_stat;

/* Pointer to stat buffer for data segment stat */
struct sam_stat *segment_stat_ptr;

/* Number of segs for which *segment_stat_ptr can hold stat information. */
int seg_buf_capacity = 0;

/* True if sam_segment_vsn stat has been performed on the current path. */
boolean have_seg_stat = false;

/* True if use selects the segments option to apply sfind to segmented file's
 * individual data segments.
 *
 * If false, then sfind aggregates the tests over all of the data segments.
 */
boolean output_data_segments = false;

/* Status value to return to system. */
int exit_status;

/* Length of current path. */
int path_length;

/* Pointer to the function used to stat files. */
int (*StatFunc)(const char *path, struct sam_stat *buf, size_t bufsize);

/* Pointer for storing the pathname of the file followed by /segment_number */
char *seg_pathname = (char *) NULL;

/* Pointer to the current pathname being tested by sfind. */
char *current_pathname = (char *) NULL;

/* Length of the buffer seg_pathname */
int seg_pathname_buf_size = 0;

int
main (argc, argv)
     int argc;
     char *argv[];
{
  int i;
  PFB parse_function;		/* Pointer to who is to do the parsing. */
  struct predicate *cur_pred;
  char *predicate_name;		/* Name of predicate being parsed. */

  program_name = argv[0];

  predicates = NULL;
  last_pred = NULL;
  do_dir_first = true;
  maxdepth = mindepth = -1;
  cur_day_start = time ((time_t *) 0) - DAYSECS;
  full_days = false;
  no_leaf_check = false;
  stay_on_filesystem = false;
  exit_status = 0;
  StatFunc = sam_lstat;
  segment_stat_ptr          = (struct sam_stat *) NULL;
  current_pathname          = (char *) NULL;
  seg_pathname              = (char *) NULL;
  seg_pathname_buf_size		= 0;
  have_seg_stat             = false;
  seg_buf_capacity          = 0;
  output_data_segments      = false;

#ifdef DEBUG
  printf ("cur_day_start = %s", ctime (&cur_day_start));
#endif /* DEBUG */

  /* Find where in ARGV the predicates begin. */
  for (i = 1; i < argc && strchr ("-!(),", argv[i][0]) == NULL; i++)
    /* Do nothing. */ ;

  /* Enclose the expression in `( ... )' so a default -print will
     apply to the whole expression. */
  parse_open (argv, &argc);
  /* Build the input order list. */
  while (i < argc)
    {
      if (strchr ("-!(),", argv[i][0]) == NULL)
	usage ("paths must precede expression");
      predicate_name = argv[i];
      parse_function = find_parser (predicate_name);
      if (parse_function == NULL)
	error (1, 0, "invalid predicate `%s'", predicate_name);
      i++;
      if (!(*parse_function) (argv, &i))
	{
	  if (argv[i] == NULL)
	    error (1, 0, "missing argument to `%s'", predicate_name);
	  else
	    error (1, 0, "invalid argument to `%s'", predicate_name);
	}
    }
  if (predicates->pred_next == NULL)
    {
      /* No predicates that do something other than set a global variable
	 were given; remove the unneeded initial `(' and add `-print'. */
      cur_pred = predicates;
      predicates = last_pred = predicates->pred_next;
      free ((char *) cur_pred);
      parse_print (argv, &argc);
    }
  else if (!no_side_effects (predicates->pred_next))
    {
      /* One or more predicates that produce output were given;
	 remove the unneeded initial `('. */
      cur_pred = predicates;
      predicates = predicates->pred_next;
      free ((char *) cur_pred);
    }
  else
    {
      /* `( user-supplied-expression ) -print'. */
      parse_close (argv, &argc);
      parse_print (argv, &argc);
    }

#ifdef	DEBUG
  printf ("Predicate List:\n");
  print_list (predicates);
#endif /* DEBUG */

  /* Done parsing the predicates.  Build the evaluation tree. */
  cur_pred = predicates;
  eval_tree = get_expr (&cur_pred, NO_PREC);
#ifdef	DEBUG
  printf ("Eval Tree:\n");
  print_tree (eval_tree, 0);
#endif /* DEBUG */

  /* Rearrange the eval tree in optimal-predicate order. */
  opt_expr (&eval_tree);

  /* Determine the point, if any, at which to stat the file. */
  mark_stat (eval_tree);

#ifdef DEBUG
  printf ("Optimized Eval Tree:\n");
  print_tree (eval_tree, 0);
#endif /* DEBUG */

  /* If no paths given, default to ".". */
  for (i = 1; i < argc && strchr ("-!(),", argv[i][0]) == NULL; i++)
    {
      curdepth = 0;
      path_length = strlen (argv[i]);
      process_path (argv[i], false);
    }
  if (i == 1)
    {
      curdepth = 0;
      path_length = 1;
      process_path (".", false);
    }

  free_memory_used_for_testing_segments();

  exit (exit_status);
}

/* Recursively descend path PATHNAME, applying the predicates.
   LEAF is nonzero if PATHNAME is in a directory that has no
   unexamined subdirectories, and therefore it is not a directory.
   This allows us to avoid stat as long as possible for leaf files.

   Return nonzero iff PATHNAME is a directory. */

static int
process_path (pathname, leaf)
     char *pathname;
     boolean leaf;
{
  struct sam_stat stat_buf;
  int pathlen;			/* Length of PATHNAME. */
  static dev_t root_dev;	/* Device ID of current argument pathname. */

  pathlen = strlen (pathname);

  /* Assume non-directory initially. */
  stat_buf.st_mode = 0;

  have_seg_stat = false;

  if (leaf)
    have_stat = false;
  else
    {
      if ((*StatFunc)(pathname, &stat_buf, sizeof(stat_buf)) != 0)
	{
	  fflush (stdout);
	  error (0, errno, "StatFunc(%s)", pathname);
	  exit_status = 1;
	  return 0;
	}
      have_stat = true;
    }

  if (!S_ISDIR (stat_buf.st_mode))
    {
      if (curdepth >= mindepth)
        {
          if (!have_stat)
            {
              if ((*StatFunc)(pathname, &stat_buf, sizeof(stat_buf)) != 0)
                {
                  fflush (stdout);
                  error (0, errno, "StatFunc(%s)", pathname);
                  exit_status = 1;
                  return 0;
                }
              have_stat = true;
            }

          apply_predicate (pathname, &stat_buf, eval_tree);
          return 0;
        }
      
    } 

  stop_at_current_level = maxdepth >= 0 && curdepth >= maxdepth;

  if (stay_on_filesystem)
    {
      if (curdepth == 0)
	root_dev = stat_buf.st_dev;
      else if (stat_buf.st_dev != root_dev)
	stop_at_current_level = true;
    }

  if (do_dir_first && curdepth >= mindepth)
    apply_predicate (pathname, &stat_buf, eval_tree);

  if (stop_at_current_level == false)
    /* Scan directory on disk. */
    scan_directory (pathname, pathlen, &stat_buf);

  if (do_dir_first == false && curdepth >= mindepth)
    apply_predicate (pathname, &stat_buf, eval_tree);

  return 1;
}

/* Scan directory PATHNAME and recurse through process_path for each entry.
   PATHLEN is the length of PATHNAME.
   STATP is the results of *StatFunc on it. */

static void
scan_directory (pathname, pathlen, statp)
     char *pathname;
     int pathlen;
     struct sam_stat *statp;
{
  char *name_space;		/* Names of files in PATHNAME. */
  int subdirs_left;		/* Number of unexamined subdirs in PATHNAME. */

  subdirs_left = statp->st_nlink - 2; /* Account for name and ".". */

  errno = 0;
  /* On some systems (VAX 4.3BSD+NFS), NFS mount points have st_size < 0.  */
  name_space = savedir (pathname, (unsigned) (statp->st_size > 0 ? statp->st_size : 512));
  if (name_space == NULL)
    {
      if (errno)
	{
	  fflush (stdout);
	  error (0, errno, "%s", pathname);
	  exit_status = 1;
	}
      else
	{
	  fflush (stdout);
	  error (1, 0, "virtual memory exhausted");
	}
    }
  else
    {
      register char *namep;	/* Current point in `name_space'. */
      char *cur_path;		/* Full path of each file to process. */
      unsigned cur_path_size;	/* Bytes allocated for `cur_path'. */
      register unsigned file_len; /* Length of each path to process. */
      register unsigned pathname_len; /* PATHLEN plus trailing '/'. */

      if (pathname[pathlen - 1] == '/')
	pathname_len = pathlen + 1; /* For '\0'; already have '/'. */
      else
	pathname_len = pathlen + 2; /* For '/' and '\0'. */
      cur_path_size = 0;
      cur_path = NULL;

      for (namep = name_space; *namep; namep += file_len - pathname_len + 1)
	{
	  /* Append this directory entry's name to the path being searched. */
	  file_len = pathname_len + strlen (namep);
	  if (file_len > cur_path_size)
	    {
	      while (file_len > cur_path_size)
		cur_path_size += 1024;
	      if (cur_path)
		free (cur_path);
	      cur_path = xmalloc (cur_path_size);
	      strcpy (cur_path, pathname);
	      cur_path[pathname_len - 2] = '/';
	    }
	  strcpy (cur_path + pathname_len - 1, namep);

	  curdepth++;
	  if (!no_leaf_check)
	    /* Normal case optimization.
	       On normal Unix filesystems, a directory that has no
	       subdirectories has two links: its name, and ".".  Any
	       additional links are to the ".." entries of its
	       subdirectories.  Once we have processed as many
	       subdirectories as there are additional links, we know
	       that the rest of the entries are non-directories --
	       in other words, leaf files. */
	    subdirs_left -= process_path (cur_path, subdirs_left == 0);
	  else
	    /* There might be weird (NFS?) filesystems mounted,
	       which don't have Unix-like directory link counts. */
	    process_path (cur_path, false);
	  curdepth--;
	}
      if (cur_path)
	free (cur_path);
      free (name_space);
    }
}

/* Return true if there are no side effects in any of the predicates in
   predicate list PRED, false if there are any. */

boolean
no_side_effects (pred)
     struct predicate *pred;
{
  while (pred != NULL)
    {
      if (pred->side_effects)
	return (false);
      pred = pred->pred_next;
    }
  return (true);
}

static int integer_length (int value)
{
	int length;
	int is_neg;

	if (value < 0) {
		is_neg = 1;
		value = value * -1;
	} else if (value == 0) {
		return 1;
	} else {
		is_neg = 0;
	}

	length = 0;

	while (value > 0) {
		length++;
		value = value / 10;
	}

	length = length + is_neg;

	return length;
}

static int setup_pathname_for_seg_number(
		char *file_pathname,	/* IN: File name to copy into dest */
		int n_segs,				/* IN: Total number of segments that
								 *     comprise the file
								 */
		char **dest,			/* IN/OUT: Pointer to pointer for the filepath
								 *         copy
								 */
		int *len_pathname_ptr,	/* OUT: Pointer to int in which to store length
								 *      of file_pathname
								 */
		int *dest_size_ptr		/* IN/OUT: Pointer to length of buffer used for
								 *         storing pathname.
								 */
		)
{
	int seg_num_len;

	/*
	 * 
	 * Calculate the number of digits in n_segs:
	 */
	seg_num_len = integer_length(n_segs);

	*len_pathname_ptr = strlen(file_pathname);

	/*
	 * Is the destination buffer big enough?
	 */

	if (*len_pathname_ptr + seg_num_len + 1 >= *dest_size_ptr) {
		/*
		 * The destination buffer is not big enough, reallocate it:
		 */

		if (*dest) {
			free(*dest);
		}

		*dest =
			(char *) malloc(sizeof(char)*(*len_pathname_ptr+seg_num_len+2));

		if (*dest == (char *)NULL) {
			/*
			 *
			 */
			if (!errno) {
				errno = EAGAIN;
			}

			error(0, errno,
				"Error: Failed to allocate memory to store segment file path information for %s.",
				file_pathname);

			exit_status = 1;

			return -1;
		} else {
			*dest_size_ptr = *len_pathname_ptr + seg_num_len + 2;
		}
	}

	(void) strncpy(*dest, file_pathname, MIN(*len_pathname_ptr,
												*dest_size_ptr));

	(*dest)[*len_pathname_ptr] = '/';
	(*dest)[*len_pathname_ptr+1] = '\0';

	return 0;
}

void apply_predicate(char *pathname, struct sam_stat *stat_buf_ptr,
					struct predicate *eval_tree_ptr) {
	char	*end_of_seg_pathname;
	int		len_pathname;
	int		n_segs;
	int		seg_num;
	int		seg_stat_err;
	int		setup_path_err;

	current_pathname = pathname;

	if (!SS_ISSEGMENT_F(stat_buf_ptr->attr) || !output_data_segments) {
		/*
		 * Item is not a segmented file or output_data_segments has not
		 * been selected so that the predicate tests are being aggregated over
		 * the file's data segments.
		 */
		apply_predicate_to_one_item(pathname, stat_buf_ptr, eval_tree_ptr);

		current_pathname = NULL;

		return;
	}

	/* File is segmented and output_data_segments has been selected,
	 * apply the predicate tests to each individual data segment and the
	 * index inode, one at a time.
	 */

	n_segs = NUM_SEGS(stat_buf_ptr);

	setup_path_err = setup_pathname_for_seg_number(pathname,
													n_segs,
													&seg_pathname,
													&len_pathname,
													&seg_pathname_buf_size);

	if (setup_path_err != 0) {
		current_pathname = NULL;

		return;
	}

	end_of_seg_pathname = seg_pathname + len_pathname + 1;
	sprintf(end_of_seg_pathname, "0");

	/* First apply the predicates to the index inode */
	apply_predicate_to_one_item(seg_pathname, stat_buf_ptr, eval_tree_ptr);

	/* Next, apply predicates to each of the data segments, one at a time. */

	seg_stat_err = seg_stat_file(pathname, stat_buf_ptr);

	if (seg_stat_err != 0) {
		current_pathname = NULL;

		exit_status = 1;

		return;
	}

	for (seg_num = 1; seg_num <= n_segs; seg_num++) {
		sprintf(end_of_seg_pathname, "%d", seg_num);

		apply_predicate_to_one_item(seg_pathname,
									&segment_stat_ptr[seg_num - 1],
									eval_tree_ptr);
	}

	current_pathname = NULL;

	return;
}

void free_memory_used_for_testing_segments()
{
	if (segment_stat_ptr) {
		free(segment_stat_ptr);
		segment_stat_ptr = (struct sam_stat *) NULL;
		seg_buf_capacity = 0;
		have_seg_stat    = false;
	}

	if (seg_pathname) {
		free(seg_pathname);
		seg_pathname = (char *) NULL;
		seg_pathname_buf_size = 0;
	}

	return;
}
