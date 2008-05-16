/*
 * plist.c - priority-list package.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.16 $"


/*
 * This file implements a package for maintaining a priority-sorted
 * list of data.  Only the best "KEEP_HOW_MANY" elements need be kept.
 *
 * First, init() must be called to initialize the internal state of the
 * package.  Then(priority, data) pairs are passed to add_entry to add
 * the pairs to the list.  Once all entries have been given to add_entry,
 * finish() must be called to finalize the internal state.  Finally,
 * remove_entry is called to retrieve the best entries, highest priority
 * first.
 *
 * The internal data structure is three arrays, each list_size
 * long.  There are three variables which are used to subscript into
 * one each of the three arrays.  The subscripts are "working", "merge_in",
 * and "merge_out".
 *
 * Just after init()'s been called, calls to add_entry() simply append the
 * entry to the end of the working list.  Once that list's filled, we
 * sort it, and swap around the values of the subscript variables so that
 * the list which used to be indexed by "working" is now indexed by
 * "merge_in."  Then we proceed to fill up the new "working" list until it
 * too fills up.  Again, once the list has filled, we sort it.
 *
 * At this point, we have two full arrays, and one empty one.  We take
 * enough of the best entries from the two full arrays to fill up the
 * empty array, which is denoted by "merge_out".
 * (This can be done very efficiently because the two input
 * lists are sorted.)  Once that's been done, we can discard the contents
 * of the two input arrays, rename merge_out to merge_in, and continue
 * to fill up "working".  Then, when working's full, we again sort it,
 * merge it with merge_in, producing merge_out, swap merge_out and merge_in,
 * and repeat until finish() is called.
 *
 * finish() must make sure that the potentially partially-full working list
 * is sorted and merged in with the other entries.  The results are stored
 * in the working list.
 *
 * remove_entry simply grabs entries from the working list until exhausted.
 */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "sam/types.h"
#include "sam/exit.h"

#include "releaser.h"
#include "plist.h"
#include <sam/lint.h>

/* State variables */
static float worst_saved_priority;
static int finish_has_been_called = FALSE;
static int merge_in, merge_out, working;   /* the three subscripts */
static int merge_in_list_is_full;
static int remove_index;
static int working_length;
static int wsp_valid;

extern int list_size;

/*
 * An element in the priority list consists of a priority, and an
 * externally-defined data element.
 */
struct priority_list_element {
	float priority;
	struct data data;
};

/*  The priority lists */
static struct priority_list_element *list[3];

/*  macros */

/*
 *  add_to_working adds the current(priority, data) pair to the end
 *  of the current working list.  It also keeps track of the worst-priority
 *  element currently stored in all lists.
 */
#define	add_to_working() {					\
	list[working][working_length].priority = priority;	\
	list[working][working_length].data = *data;		\
	working_length++;					\
	if (!wsp_valid || worst_saved_priority > priority) {	\
		worst_saved_priority = priority;		\
		wsp_valid = TRUE;				\
	}							\
}

/* Returns true iff a has a better priority than b */
#define	better_than(a, b) ((a) > (b))

/*
 * Swaps the trailing two arguments which are of the type given by
 * the first arguments
 */
#define	swap(type, a, b) { type temp; temp = a; a = b; b = temp; }

/* prototypes */
static void merge(void);
static void sort_working(void);

/*  Initialize the data structures */
void
init()
{
	working = 0; merge_in = 1; merge_out = 2;
	working_length = 0;
	merge_in_list_is_full = FALSE;
	wsp_valid = FALSE;   /* worst_stored_priority is not valid */
	finish_has_been_called = FALSE;
}

/*  Initialize the data structures */
void
remake_lists(int size)
{
	int i;

	/*
	 *   Allocate the lists.
	 */
	for (i = 0; i < 3; i++) {
		if (list[i] != NULL) {
			free(list[i]);
		}
		list[i] = malloc(size * sizeof (struct priority_list_element));
		if (list[i] == NULL) {
			printf("Can't allocate memory for list size %d.",
			    size);
			fprintf(log, "Can't allocate memory for list size %d.",
			    size);
			exit(3);
		}
	}
}

/*
 *  Finish up.  We probably have a partially-full work list.  Sort it.
 *  And, we might have a merge_in list full of entries, too.  If we've
 *  got both, then we need to merge them together.  Finally, return
 *  the number of entries which are valid in the working list.
 */
int
finish()
{
	sort_working();
	if (merge_in_list_is_full) {
		merge();
		working_length = list_size;
		swap(int, merge_out, working);
	}
	finish_has_been_called = TRUE;
	remove_index = 0;
	return (working_length);
}

/*  Add an entry to the data structure */
void
add_entry(float priority, struct data *data)
{
	if (!wsp_valid) {
		worst_saved_priority = priority;
		wsp_valid = TRUE;
	}

	/* If we're just starting out, then we keep everything we can hold. */
	if (!merge_in_list_is_full && working_length < list_size) {
		add_to_working()
		    return;
	}

	/*
	 * We've got at least one full list.  We can ignore this call if
	 * the priority is worse than the worst one we've saved so far.
	 */
	if (!better_than(priority, worst_saved_priority)) {
		return;
	}

	/*
	 * Otherwise, we've got to save it.  See if we have room in the
	 * working list.
	 */
	if (working_length < list_size) {
		add_to_working()
		    return;
	}

	/* The working list just filled up.  Time to sort it. */
	sort_working();

	/*
	 * If we don't have a full merge_in list, then we just shuffle
	 * things around so that the current working list becomes the merge_in
	 * list, and we're done.
	 */
	if (!merge_in_list_is_full) {
		merge_in_list_is_full = TRUE;
		swap(int, working, merge_in)
		    working_length = 0;
		add_to_working()
		    return;
	}

	/*
	 * We now have two full lists(working and merge_in).  Merge them
	 * to the merge_out list.   It will also be a full list.
	 */
	merge();

	/*
	 * Now, swap merge_in and merge_out so that merge_in is again a full
	 * list.  Mark working as empty, because all the good entries from it
	 * are safely stored in the new merge_in.
	 */
	swap(int, merge_in, merge_out)
	    working_length = 0;

	/*
	 * Since we just threw away a lot of entries when we merged the lists,
	 * recalculate the worst_saved_priority as the priority of the last
	 * element of the merged list.
	 */
	worst_saved_priority = list[merge_in][list_size-1].priority;

	/* Finally, either toss this call's data or add to the working list. */
	if (better_than(priority, worst_saved_priority))
		add_to_working()
}

/*
 * Compare routine for qsort().   Note that qsort wants to sort in
 * ascending order, yet we want better priorities first(better
 * priorities are numerically larger).  So we are returning the opposite
 * of what "better_than" would.
 */
static int
compare_working(
	const void *i,
	const void *j
) {
	struct priority_list_element *pi = (struct priority_list_element *)i;
	struct priority_list_element *pj = (struct priority_list_element *)j;

	/*
	 * qsort takes excessive runtime if there are lots of equal keys.
	 * Tiebreak priority ties using the inode number.
	 */
	if (pi->priority == pj->priority) {
		if (pi->data.id.ino > pj->data.id.ino) {
			return (-1);
		} else if (pi->data.id.ino < pj->data.id.ino) {
			return (1);
		} else {
			return (0);
		}
	}

	if (better_than(pi->priority, pj->priority)) {
		return (-1);
	} else {
		return (1);
	}
}


#if MQSORT
/*
 * my version of qsort().  Used for debugging - replace calls to qsort with
 * calls to mqsort.
 */
static void
mqsort(void *base, size_t nel, size_t width,
    int (*compar) (const void *, const void *))
{
	int i, j;
	int swapped_any;
	char *tmp;
	char *cbase = (char *)base;

	tmp = malloc(width);
	if (tmp == NULL) {
		fprintf(stderr,
		    "Cannot malloc space for temp storage of %d bytes\n",
		    width);
		exit(EXIT_NOMEM);
	}

	for (i = 0; i < nel-1; i++) {
		swapped_any = FALSE;
		for (j = i+1; j < nel; j++) {
			/*
			 * If the entry with the larger index has a better
			 * priority than the one with the smaller index, then
			 * we need to swap them around.
			 */
			if (compar(cbase+i*width, cbase+j*width) > 0) {
				memcpy(tmp,	   cbase+i*width, width);
				memcpy(cbase+i*width, cbase+j*width, width);
				memcpy(cbase+j*width, tmp,	   width);
				swapped_any = TRUE;
			}
		}
		if (!swapped_any)
			return;
	}
}
#endif

/*
 *  Sort the working list by priority.  Note that the list may not be
 *  full.
 */
static void
sort_working(void)
{
	qsort(list[working], working_length,
	    sizeof (struct priority_list_element), &compare_working);
}

/*
 *  Merge merge_in(which is always list_size long) and working
 *  (which is working_length elements long) together into merge_out
 */
static void
merge(void)
{
	int index_in, index_out, index_working;

	index_working = 0;
	index_in = 0;

	for (index_out = 0; index_out < list_size; index_out++) {
		/*
		 * Pick from which list we grab the next entry.  If the
		 * working list is not exhausted and the working list has a
		 * better priority than the merge_in list, then take an entry
		 * from the working list.
		 */
		struct priority_list_element *po =
		    &list[merge_out][index_out];
		struct priority_list_element *pw =
		    &list[working][index_working];
		struct priority_list_element *pi =
		    &list[merge_in][index_in];

		if (index_working < working_length &&
		    better_than(pw->priority, pi->priority)) {

			*po = *pw;
			index_working++;

		} else {

			*po = *pi;
			index_in++;

		}
	}
}

/*
 *  Return the next entry from the working list.  Should only be called
 *  after finish() has been called.  Returns TRUE/FALSE if there are/aren't
 *  any more entries.
 */
int
remove_entry(float *priority, struct data *data)
{
	if (!finish_has_been_called || remove_index >= working_length) {
		return (FALSE);
	}

	*priority = list[working][remove_index].priority;
	*data = list[working][remove_index].data;

	remove_index++;
	return (TRUE);
}

#ifdef TEST_WRAPPER
If you compile with TEST_WRAPPER defined, you'll need to define
something like:

struct data {
	char string[32];
}

main()
{
	struct data d;
	int i, p;
	int valid;

	init();

	while (scanf("%d %s", &p, d.string) == 2) {
		add_entry(p, d);
	}

	valid = finish();
	printf("\nOnly %d entries are valid.\n", valid);

	while (remove_entry(&p, &d)) {
		printf("%d %s\n", p, d.string);
	}
}

/* Display the list (with a header message) to stdout */
static void
print_list(char *header, int list_num)
{
	int i;
	char *tag;

	if (list_num == working)
		tag = "working list";
	else if (list_num == merge_in)
		tag = "merge_in list";
	else if (list_num == merge_out)
		tag = "merge_out list";
	else
		tag = "??";

	printf("%s(%s)\n", header, tag);

	for (i = 0; i < list_size; i++) {
		printf("%d[%d]: %d %s\n", list_num, i,
		    list[list_num][i].priority,
		    list[list_num][i].data.string);
	}
}
#endif
