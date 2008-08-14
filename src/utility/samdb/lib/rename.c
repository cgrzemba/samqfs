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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sam/types.h>
#include <sam/lib.h>
#include <sam/fs/ino.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>

#include "util.h"

/*
 * Max no. of objects/directory
 * e.g., max no. identical hard
 * links/directory
 */
#define	MAX_OBJS	32

#define	L_CACHE		200		/* Id cache size and increment	*/

extern my_ulonglong update_path(sam_db_path_t *, char *, char *);
extern void free_path(sam_db_path_t *, int);
extern int idstat(unsigned int, unsigned int, struct sam_perm_inode *);
extern char *idfind(sam_id_t, sam_id_t);
extern int idfindall(sam_id_t, sam_id_t, char **, int);
extern char *find_id(sam_id_t, sam_id_t);
extern int find_all_ids(sam_id_t, sam_id_t, char **, int);
extern int verify_inode(struct sam_perm_inode *, sam_db_inode_t *);
extern sam_db_ftype sam_db_get_ftype(struct sam_perm_inode *);
extern char *sam_db_id_to_path(sam_id_t, char *);

typedef struct {
	sam_id_t id;		/* File inode/generation no. */
	sam_id_t parent; 	/* Parent inode/generation no. */
} id_cache_t;

static id_cache_t *ID_Cache; 	/* Id/parent cache table */
static int a_ids; 		/* Number of allocated ids */

/*
 * 	Rename - Process Rename.
 *
 *	Description:
 *	    Rename processes path/object name changes and updates the
 *	    database to reflect these changes.  The update accomodates
 *	    any number of hard links in the source or target directory.
 *
 *	Possiblities:
 *	    1. simple rename (same directory)
 *	    2. rename to new directory, must change path of object
 *	    3. object is directory, must rename all path references
 *	    4. object is regular file, posibility of multiple hardlinks in
 *	       source or target.
 *
 *
 *	On Entry:
 *	    id		Inode/generation number to update.
 *	    parent	Parent inode/generation number.
 *	    func	Not used at this time.
 *	    mount_point	SAM-FS mount point.
 *
 *	On Return:
 *	    Database updated.
 */
int
Rename(
	sam_id_t id,		/* Inode to update */
	sam_id_t parent,	/* Parent inode */
	int func,		/* Rename function type */
	char *mount_point)	/* SAM-FS mount point */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen;	/* Generation number for query */
	sam_id_t source;	/* Inode for original parent */
	sam_id_t target; 	/* Inode for destination parent */

	sam_db_path_t *T_path, *p; 	/* Path table */
	sam_db_inode_t *T_inode, *t; 	/* Inode table */
	my_ulonglong n_path; 		/* No. of items in path table */
	my_ulonglong n_inode; 		/* No. of items in path table */
	long long n_rows; 		/* No. of rows returned */

	char **Tobjs;	/* Object (file) name list */
	int n_tobj; 	/* Number of objects in list */
	char **Sobjs; 	/* Object (file) name list */
	int n_sobj; 	/* Number of objects in list */
	my_ulonglong n;
	struct sam_perm_inode inode; /* Inode image */
	char *Spath;	/* File path */
	char *Tpath;	/* File path */
	int rst;
	int i, k;

	ino = id.ino;
	gen = id.gen;
	T_inode = NULL;
	T_path = NULL;
	n_path = n_sobj = n_tobj = 0;
	Sobjs = NULL;
	Tobjs = NULL;
	Spath = NULL;
	Tpath = NULL;
	rst = -1;

	switch (func) {
	case -1: /* Initialize */
		a_ids = L_CACHE;
		SamMalloc(ID_Cache, a_ids * sizeof (id_cache_t));
		memset((char *)ID_Cache, 0, a_ids * sizeof (id_cache_t));
		return (0);

	case 0: /* Rename within same parent */
		source.ino = 0;
		source.gen = 0;
		target = parent;
		break;

	case 1: /* Rename to new parent part 1 */
		for (i = 0; i < a_ids; i++) {
			if (ID_Cache[i].id.ino == 0) {
				ID_Cache[i].id = id;
				ID_Cache[i].parent = parent;
				goto insert;
			}
		}
		a_ids += L_CACHE;

		SamRealloc(ID_Cache, a_ids * sizeof (id_cache_t));

		ID_Cache[i].id = id;
		ID_Cache[i].parent = parent;

		for (i++; i < a_ids; i++) {
			ID_Cache[i].id.ino = 0;
		}

insert:
		if (SAMDB_Debug) {
			Trace(TR_ERR, "id cache insert %d ino=%u:%u "
			    "parent=%u.%u\n", i, id.ino, id.gen,
			    parent.ino, parent.gen);
		}
		return (0);

	/* Rename to new parent part 2	*/
	case 2:
		for (i = 0; i < a_ids; i++) { /* Search cache	*/
			if (ID_Cache[i].id.ino == id.ino &&
			    ID_Cache[i].id.gen == id.gen) {
				break;
			}
		}

		if (i == a_ids) {
			Trace(TR_ERR, "inode not found in id cache, "
			    "ino=%u.%u", id.ino, id.gen);
			return (-1);
		}
		source = ID_Cache[i].parent;
		target = parent;
		ID_Cache[i].id.ino = 0;
		break; /* Finish processing rename	*/

	default:
		Trace(TR_ERR, "Unknown rename function, func=%d", func);
		return (-1);
	}

	n_inode = sam_db_query_inode(&T_inode, ino, gen);

	if (n_inode == 0) {
		Trace(TR_ERR, "activity lost for ino=%u.%u", ino, gen);
	}

	if (n_inode > 1) {
		Trace(TR_ERR, "more than 1 inode entry for ino=%u.%u",
		    ino, gen);
	}

	for (n = 0; n < n_inode; n++) {
		t = T_inode + n;
		if (!t->deleted) {
			break;
		}
	}

	/* If all entries deleted */
	if (n == n_inode) {
		Trace(TR_ERR, "no active entry for ino=%u.%u", ino, gen);
		t = T_inode; /* Use deleted entry	*/
	}

	n_path = sam_db_query_path(&T_path, ino, gen);

	if (t->type != FTYPE_REG && n_path > 1) {
		Trace(TR_ERR, "more than 1 path entry for "
		    "non-regular file, ino=%u.%u", ino, gen);
	}

	i = idstat(ino, gen, &inode);

	if (i < 0) {
		if (errno == ENOENT) {
			Trace(TR_ERR, "rename lost for ino=%u.%u", ino, gen);
		} else {
			Trace(TR_ERR, "can't stat ino=%u.%u", ino, gen);
		}
		goto exit;
	}

	if (verify_inode(&inode, t)) {
		Trace(TR_ERR, "inode error ino=%u.%u", ino, gen);
		goto exit;
	}

	Tpath = sam_db_id_to_path(target, mount_point);

	SamMalloc(Tobjs, MAX_OBJS * sizeof (char *));
	n_tobj = find_all_ids(target, id, Tobjs, MAX_OBJS);

	if (n_tobj == 0) {
		Trace(TR_ERR, "can't find obj ino=%u.%u in "
		    "target ino=%u.%u", id.ino, id.gen,
		    target.ino, target.gen);
		goto exit;
	}

	if (source.ino != 0) { /* If move between directories */
		Spath = sam_db_id_to_path(source, mount_point);

		if (Spath == NULL) {
			Trace(TR_ERR, "can't resolve path for ino=%u.%u",
			    ino, gen);
			goto exit;
		}

		if (Spath == NULL) {
			Trace(TR_ERR, "can't resolve path for ino=%u.%u",
			    ino, gen);
			goto exit;
		}

		SamMalloc(Sobjs, MAX_OBJS * sizeof (char *));
		n_sobj = find_all_ids(source, id, Sobjs, MAX_OBJS);
	}

	/*
	 * 	This complex block of code addresses the situation where
	 *	it is possible to have a multiple hard links to the same
	 *	file (i.e., inode) with in the same directory.
	 */
	for (k = 0; k < n_path; k++) {
		p = T_path + k;
		p->flag = 0;
		if (strcmp(p->path, Tpath) == 0) {
			p->flag = 1;
			for (i = 0; i < n_tobj; i++) {
				if (Tobjs[n] == NULL ||
				    strcmp(p->obj, Tobjs[i]) != 0) {
					continue;
				}
				SamFree(Tobjs[i]);
				Tobjs[i] = NULL;
				p->flag = 0;
				break;
			}
			continue;
		}
		if (Spath != NULL && strcmp(p->path, Spath) == 0) {
			p->flag = 2;
			for (i = 0; i < n_sobj; i++) {
				if (Sobjs[n] == NULL ||
				    strcmp(p->obj, Sobjs[i]) != 0) {
					continue;
				}

				SamFree(Sobjs[i]);
				Sobjs[i] = NULL;
				p->flag = 0;
				break;
			}
			continue;
		}
	}

	/*
	 * 	We are left with the following after the above elimination
	 *	loop:
	 *
	 *	T_path	Should have unmarked, the path entry that changed.
	 *	Tobjs	Should have at least on remaning entry which is
	 *		the new name for the moved object.  It is possible
	 *		that multiple remain and we shall simply claim the
	 *		the first one. The others will be claimed by following
	 *		events.
	 *	Sobjs	Should have no remaing entries.  It is possible that
	 *		multiple may remain, the result of hard links being
	 *		established after the move.  And in the odd case where
	 *		a newly link file has the same object name as the file
	 *		moved, we have an intersting situation.
	 *
	 *	Lets count up the results.
	 */
	i = -1;
	for (k = n = 0; k < n_path; k++) {
		p = T_path + k;
		if (p->flag == 0) {
			continue;
		}
		if (Spath != NULL && p->flag != 2) {
			continue;
		}
		if (i < 0) {
			i = k;
		}
		n++;
	}

	p = i < 0 ? NULL : T_path+i;

	if (n == 0 && SAMDB_Verbose) {
		Trace(TR_ERR, "no path found after elimination");
	}

	if (n > 1 && SAMDB_Verbose) {
		Trace(TR_ERR, "multiple paths found after elimination,"
		    "n=%d", n);
	}

	for (k = n = 0; k < n_sobj; k++) {
		if (Sobjs[k] == NULL) {
			continue;
		}
		SamFree(Sobjs[k]);
		n++;
	}

	if (n > 0) {
		Trace(TR_ERR, "source entries remain after elimination,"
		    "n=%d", n);
	}

	i = -1;
	for (k = n = 0; k < n_tobj; k++) {
		if (Tobjs[k] == NULL) {
			continue;
		}
		if (i < 0) {
			i = k;
		}
		n++;
	}
	if (n == 0 && SAMDB_Verbose) {
		Trace(TR_ERR, "no target entry found after elimination");
	}

	if (n > 1 && SAMDB_Verbose) {
		Trace(TR_ERR, "multiple target entries found "
		    "after elimination, n=%d", n);
	}

	if (p == NULL || i < 0) {
		goto exit;
	}

	if (t->type == FTYPE_DIR) {
		n_rows = sam_db_rename(p->path, p->obj, Tpath, Tobjs[i]);
		if (n_rows < 0) {
			goto exit;
		}
	}

	(void) update_path(p, Spath == NULL ? NULL : Tpath, Tobjs[i]);
	rst = 0;

exit:
	if (Spath != NULL) {
		SamFree(Spath);
	}

	if (Tpath != NULL) {
		SamFree(Tpath);
	}

	for (k = 0; k < n_sobj; k++) {
		if (Sobjs[k] != NULL) {
			SamFree(Sobjs[k]);
		}
	}

	for (k = 0; k < n_tobj; k++) {
		if (Tobjs[k] != NULL) {
			SamFree(Tobjs[k]);
		}
	}

	if (Sobjs != NULL) {
		SamFree(Sobjs);
	}
	if (Tobjs != NULL) {
		SamFree(Tobjs);
	}

	if (T_inode != NULL) {
		SamFree(T_inode);
	}

	if (T_path != NULL) {
		free_path(T_path, n_path);
	}

	return (rst);
}

/*
 * 	check_id_cache - Check cache for remaining entries.
 *
 *	Description:
 *	    Check ID Cache for remaining entries.
 *
 *	Returns:
 *	    Number of entries found in cache.
 */
int check_id_cache(void) {
	int i, n;

	for (i = n = 0; i < a_ids; i++) {
		if (ID_Cache[i].id.ino == 0)
			continue;
		if (SAMDB_Verbose)
			Trace(TR_ERR, "remaining cache entry "
			    "ino=%u.%u, parent=%u.%u",
			    ID_Cache[i].id.ino, ID_Cache[i].id.gen,
			    ID_Cache[i].parent.ino, ID_Cache[i].parent.gen);
		n++;
	}

	return (n);
}
