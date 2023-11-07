/*
 * findvsn - vsn table lookup and insertion.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.18 $"

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
static int is_valid_vsn(char *vsn, media_t media);
static int is_ansi_tp_label(char *s, size_t size);
static int is_valid_label(char *s, size_t size);
static VSN_TABLE *insertIntoVsnTable(char *vsn, media_t media);
static VSN_TABLE *lookupVsnTable(char *vsn, media_t media);
static void extendTables(void);

extern int VSNs_in_robot;

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

	VSN_TABLE *entry = NULL;

	if (is_valid_vsn(vsn, media) == 0) {
		return (NULL);
	}

	if (hashTable == NULL) {
		hashTable = AllocateHashTable();
	}

	if ((entry = lookupVsnTable(vsn, media)) != NULL) {
		return (entry);
	}

	if (table_used >= table_avail) {
		extendTables();
	}

	entry = insertIntoVsnTable(vsn, media);

	return (entry);
}


/*
 * Tries to find the entry in the vsn table. Hash table is used
 * to speed up the operation.
 */
static VSN_TABLE *
lookupVsnTable(
	char *vsn,
	media_t media)
{
	VSN_TABLE *entry;
	int value;
	unsigned int key = GenerateHashKey(vsn, hashTable);

#ifdef DEBUG
	IncLookupsHashStats();
#endif

	value = hashTable->values[key];

	while (value != HASH_EMPTY) {
#ifdef DEBUG
		IncProbesHashStats();
#endif
		entry = &vsn_table[value];
		if ((entry->media == media) &&
		    (strcmp(entry->vsn, vsn) == 0)) {
#ifdef DEBUG
			IncFoundsHashStats();
#endif
			return (entry);	  /* Found a match */
		}

		key++;
		if (key >= hashTable->size) {
			key = 0;
		}

		value = hashTable->values[key];
	}

	return (NULL);
}


/*
 * Inserts the new media into the media table and returns
 * the pointer to that media.
 */
static VSN_TABLE *
insertIntoVsnTable(
	char *vsn,
	media_t media)
{

	VSN_TABLE *entry = &vsn_table[table_used];

#ifdef DEBUG
	IncAddsHashStats();
#endif

	entry->media = media;
	entry->size = 0;
	entry->count = 0;

	if (VSNs_in_robot) {
		entry->in_robot = 1;
	} else {
		entry->in_robot = 0;
	}

	strcpy(entry->vsn, vsn);
	entry->ce = NULL;
	entry->slot = ROBOT_NO_SLOT;

	InsertIntoHashTable(table_used, vsn, hashTable);

	table_used++;

	return (entry);
}


static void
extendTables(
	void)
{

	size_t new_size = table_avail + TABLE_INCREMENT;

#ifdef DEBUG
	IncReallocsHashStats();
#endif

	vsn_table = (struct VSN_TABLE *)Resize((void *) vsn_table,
	    (size_t)(new_size * sizeof (struct VSN_TABLE)),
	    "vsn_table");

	memset(&vsn_table[table_avail], 0,
	    sizeof (struct VSN_TABLE) * TABLE_INCREMENT);


	vsn_permute = (int *)Resize((void *) vsn_permute,
	    (size_t)(new_size * sizeof (int)), "vsn_permute");

	for (size_t i = table_avail; i < new_size; i++)
		vsn_permute[i] = (int)i;

	table_avail = new_size;

	ExtendHashTable(hashTable);

	for (int i = 0; i < table_used; i++) {
		InsertIntoHashTable(i, vsn_table[i].vsn, hashTable);
	}
}


static int			/* 1 if string is valid, 0 if not */
is_valid_vsn(char *vsn, media_t media)
{
	size_t slen;

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
