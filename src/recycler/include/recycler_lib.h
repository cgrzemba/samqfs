/*
 * recycler_lib.h
 *
 * Declaration of functions which serve for work with disk archive
 * sequence numbers that are in use by currently running arcopy
 * processes.
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
 * or https://illumos.org/license/CDDL.
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


#ifndef	_RECYCLER_LIB_H
#define	_RECYCLER_LIB_H

#pragma	ident	"$Revision$"

/*
 * Common code used in recycler and nrecycler
 */
#define	TABLE_INCREMENT 1000

void *Resize(void *ptr, size_t size, char *err_name);

/*
 * Representation of sequence numbers in use by currently running arcopies and
 * related functions.
 */
typedef struct SeqNumsInUse {
	int count; /* Sequence numbers array count */
	DiskVolumeSeqnum_t *seqnums; /* Array of sequence numbers */
} SeqNumsInUse_t;

SeqNumsInUse_t *GetSeqNumsInUse(char *vsn, char *fsname, SeqNumsInUse_t *inuse);
boolean_t IsSeqNumInUse(DiskVolumeSeqnum_t seqnum,
    SeqNumsInUse_t *seqNumsInUse);


/*
 * Hash table related data structure and related functions.
 *
 * Contains hashed indexes into recycler's vsn table or nrecycler's
 * media table. Since both data structures are different, user needs to
 * implement the custom lookup functionality for each table.
 */
typedef struct HashTable {
	int size;
	int *values;
} HashTable_t;

#define	HASH_MULTIPLIER 13
#define	HASH_SLOP 13
#define	HASH_INCREMENT (TABLE_INCREMENT * HASH_MULTIPLIER)
#define	HASH_INITIAL_SIZE (HASH_INCREMENT + HASH_SLOP)
#define	HASH_EMPTY -1

#ifdef DEBUG
/* Uncomment the lines below for hash table debugging */
/*
 * #define HASH_DEBUG
 * #define HASH_DEBUG_INCREMENT 5
 */

/*
 * Structure and function for hash table performance measurement
 */
typedef struct HashStatistics {
	int lookups;
	int probes;
	int adds;
	int founds;
	int reallocs;
} HashStatistics_t;

#define	pct(x)	((((float)x)/((float)hashStatistics.lookups))*100.0)
void DumpHashStats();
void PrintHashStats();
void IncLookupsHashStats();
void IncProbesHashStats();
void IncAddsHashStats();
void IncFoundsHashStats();
void IncReallocsHashStats();
#endif

HashTable_t *AllocateHashTable();
unsigned int GenerateHashKey(char *vsn, HashTable_t *hashTable);
void InsertIntoHashTable(int index, char *vsn, HashTable_t *hashTable);
void ExtendHashTable(HashTable_t *hashTable);
#ifdef HASH_DEBUG
void DumpHashTable(HashTable_t *hashTable);
#endif

#endif /* _RECYCLER_LIB_H */
