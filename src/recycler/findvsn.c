/*
 * findvsn - vsn table lookup and insertion.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <limits.h>
#include <errno.h>
int errno;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <syslog.h>
#include <synch.h>
#include <thread.h>
#include <signal.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/lib.h"
#include "sam/resource.h"
#include "sam/uioctl.h"
#include "sam/param.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/odlabels.h"
#include "sam/exit.h"
#include "sam/lint.h"

/* Local headers. */
#include "recycler.h"

/* local protos */
static void *resize(void *ptr, size_t size, char *err_name);
static int is_valid_vsn(char *vsn, media_t media);
static int is_ansi_tp_label(char *s, size_t size);
static int is_valid_label(char *s, size_t size);
/*
 * Hash table
 *   The hash table is sized in the #define	below.  The idea is that the
 *   hash table should be bigger than the maximum size of the vsn_table.
 *   Experimentation with vsns of the form OPT01a, OPT01b, OPT02a, etc.,
 *   (similar to what many customers use) shows that the factor of 10 is
 *   necessary to reduce the number of hash colisions.  For example, 8
 *   performs much more poorly.  If you suspect that the hash table is
 *   causing problems, #define	DEBUG and the routine dump_hash_stats
 *   below will display the number of probes to the hash table.  If this
 *   number gets much more than 2 or 3 times the number of look ups, then
 *   you should think about expanding the size of the hash table and/or
 *   using a better (more randomizing) hash function.
 *
 *   Adding 13 gives a little extra slop.
 */

#define	HASH_MULTIPLIER		10
#define	HASH_SLOP		13
#define	HASH_INCREMENT		(VSN_INCREMENT * HASH_MULTIPLIER)
#define	INITIAL_HASH_SIZE	(HASH_INCREMENT + HASH_SLOP)
/* #define	HASH_size MAXVSNS*10+13 */
/* static int HASH_table[HASH_size]; */
static int *hash_table = NULL;

/* bool HASH_initialized = FALSE; */

#define	HASH_EMPTY -1

extern int VSNs_in_robot;

static struct {
	int lookups;
	int probes;
	int adds;
	int founds;
	int reallocs;
} hash_statistics = {0, 0, 0, 0, 0};

/*
 * ----- Find_VSN
 * Look up or insert the indicated media/VSN into the vsn_table.  A hash
 * table is used for speed.
 */

VSN_TABLE *			/* Location of media/VSN in vsn_table */
Find_VSN(
	media_t media,		/* Media type of VSN */
	char *vsn)		/* VSN to find */
{
	static int HASH_size = 0;
	int hash_l = strlen(vsn),	/* length of VSN to be hashed */
	    hash_i,			/* index into VSN being hashed */
	    hash_shift = 0; 	/* # of bits to shift the current char */
				/*    by during hashing */
	unsigned int hash_v = 0;	/* hashed value of VSN */

	if (is_valid_vsn(vsn, media) == 0) {
		return (NULL);
	}
	/* Allocate and fill hash table with all "empty" flags. */
	if (NULL == hash_table) {
		HASH_size = INITIAL_HASH_SIZE;
		hash_table = (int *)malloc(INITIAL_HASH_SIZE * sizeof (int));
		for (hash_i = 0; hash_i < HASH_size; hash_i++) {
			hash_table[hash_i] = HASH_EMPTY;
		}
	}

	hash_statistics.lookups++;

	/*
	 * Generate a hash key by summing C[i]<<i, for all i in the VSN,
	 * and taking that total modulo the hash table size.  Be careful
	 * to make the final result positive by declaring hash_v unsigned.
	 */
	hash_v = 0;
	for (hash_i = 0; hash_i < hash_l; hash_i++) {
		hash_v += vsn[hash_i]<<hash_shift;
		hash_shift = (hash_shift + 1) % 8;
	}
	hash_v %= HASH_size;

	/*
	 * Look through the hash table until we either get a HASH_EMPTY
	 * (which means that the VSN is new) or we find a match.
	 */
	while (hash_table[hash_v] != HASH_EMPTY) {
		VSN_TABLE *trial = &vsn_table[hash_table[hash_v]];
		hash_statistics.probes++;
		if ((trial->media == media) &&
		    (strcmp(trial->vsn, vsn) == 0)) {
			hash_statistics.founds++;
			return (trial);   /* Found a match */
		}
		hash_v++;
		if (hash_v >= HASH_size) {
			hash_v = 0;
		}
	}

	/* Need to add new entry */
	hash_statistics.adds++;
	if (table_used >= table_avail) {
		int i, new_size, new_HASH_size;

		/* allocate new sizes */
		new_size = table_avail + VSN_INCREMENT;
		new_HASH_size = HASH_size + (VSN_INCREMENT * HASH_MULTIPLIER);
		/* wow, lots o' casts.  is this so good? */
		vsn_table = (struct VSN_TABLE *)resize((void *) vsn_table,
		    (size_t)(new_size * sizeof (struct VSN_TABLE)),
		    "vsn_table");
		vsn_permute = (int *)resize((void *) vsn_permute,
		    (size_t)(new_size * sizeof (int)), "vsn_permute");
		hash_table = (int *)resize((void *) hash_table,
		    (size_t)(new_HASH_size * sizeof (int)), "hash_table");

		/* re-init the newly allocated data */
		memset(&vsn_table[table_avail], 0,
		    sizeof (struct VSN_TABLE) * VSN_INCREMENT);
		for (i = table_avail; i < new_size; i++)
			vsn_permute[i] = i;
		for (i = 0; i < new_HASH_size; i++)
			hash_table[i] = HASH_EMPTY;
		for (i = 0; i < new_size; i++) {
			VSN_TABLE *VSN = &vsn_table[i];
			int len = strlen(VSN->vsn);
			int j;
			int shift = 0;
			unsigned int ref = 0;

			for (j = 0; j < len; j++) {
				ref += VSN->vsn[j] << shift;
				shift = (shift + 1) % 8;
			}
			ref %= new_HASH_size;
			while (hash_table[ref] != HASH_EMPTY) {
				ref++;
				if (ref >= new_HASH_size) {
					ref = 0;
				}
			}
			hash_table[ref] = i;
		}

		/* reset the avail counter */
		table_avail += VSN_INCREMENT;
		HASH_size = new_HASH_size;
	}

	/* point hash table to this new vsn_table entry */
	hash_table[hash_v] = table_used;

	/* insert new VSN table entry */
	{
		VSN_TABLE *VSN = &vsn_table[table_used];

		VSN->media    = media;
		if (VSNs_in_robot) {
			VSN->in_robot = 1;
		} else {
			VSN->in_robot = 0;
		}
		VSN->size = 0;
		VSN->count = 0;
		strcpy(VSN->vsn, vsn);
		VSN->ce = NULL;
		VSN->slot = ROBOT_NO_SLOT;

		table_used++;
		return (VSN);
	}
}


#define	pct(x) ((((float)x)/((float)hash_statistics.lookups))*100.0)
void
dump_hash_stats(void)
{
	Trace(TR_DEBUG, "Lookups:  %d",  hash_statistics.lookups);
	Trace(TR_DEBUG, "Reallocs: %d", hash_statistics.reallocs);
	Trace(TR_DEBUG, "Probes: %d (%.2f%%)",
	    hash_statistics.probes,  pct(hash_statistics.probes));
	Trace(TR_DEBUG, "Founds: %d (%.2f%%)",
	    hash_statistics.founds,  pct(hash_statistics.founds));
	Trace(TR_DEBUG, "Adds: %d (%.2f%%)",
	    hash_statistics.adds,  pct(hash_statistics.adds));
}

static void *
resize(void *ptr, size_t size, char *err_name)
{
	void *new_ptr;

	if ((new_ptr = realloc(ptr, size)) == NULL) {
		emit(TO_ALL, LOG_ERR, 20294, size, err_name);
		exit(EXIT_NOMEM);
	}

	return (new_ptr);
}

static int			/* 1 if string is valid, 0 if not */
is_valid_vsn(char *vsn, media_t media)
{
	int slen;

	if (vsn[0] == '\0') {
		return (0);
	}

	switch (media & DT_CLASS_MASK) {
	case DT_TAPE:
		return (is_ansi_tp_label(vsn, 6));
	case DT_DISK:
	case DT_OPTICAL:
		slen = strlen(vsn);
		if (slen > 31) {
			return (0);
		}
		return (is_valid_label(vsn, slen));
	default:
		return (0);
	}
}

static int			/* 1 if string is valid, 0 if not */
is_ansi_tp_label(char *s, size_t size)
{
	char c;

	if (strlen(s) > size) {
		return (0);
	}
	while ((c = *s++) != '\0') {
		if (isupper(c)) {
			continue;
		}
		if (isdigit(c)) {
			continue;
		}
		if (strchr("!\"%&'()*+,-./:;<=>?_", c) == NULL) {
			return (0);
		}
	}
	return (1);
}

static int			/* 1 if string is valid, 0 if not */
is_valid_label(char *s, size_t size)
{
	char c;

	if (strlen(s) > size) {
		return (0);
	}
	while ((c = *s++) != '\0') {
		if (isalnum(c)) {
			continue;
		}
		if (strchr("!\"%&'()*+,-./:;<>?@[]^`_{|}~", c) == NULL) {
			return (0);
		}
	}
	return (1);
}
