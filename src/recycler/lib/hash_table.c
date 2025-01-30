/*
 * hash_table.c
 *
 * Hash table implementation
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

#pragma ident "$Revision$"

static char *_SrcFile = __FILE__;

/* Common recycler headers */
#include "recycler_c.h"

/* SAM-FS headers */
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"

/* Library related headers */
#include "recycler_lib.h"

#ifdef DEBUG
static HashStatistics_t hashStatistics = {0, 0, 0, 0, 0};
#endif

/*
 * Returns allocated and empty hash table of default size.
 */
HashTable_t *
AllocateHashTable()
{
	HashTable_t *hashTable;

	SamMalloc(hashTable, sizeof (HashTable_t));

#ifdef HASH_DEBUG
	hashTable->size = HASH_DEBUG_INCREMENT;
#else
	hashTable->size = HASH_INITIAL_SIZE;
#endif

	SamMalloc(hashTable->values, hashTable->size * sizeof (int));

	for (int i = 0; i < hashTable->size; i++) {
		hashTable->values[i] = HASH_EMPTY;
	}

	return (hashTable);
}


/*
 * Generates a hash key using the djb2 algorithm and taking that total modulo
 * the hash table size. The final result is always positive, so if storing the
 * result of this function into a variable, make sure its type is unsigned int.
 */
unsigned int
GenerateHashKey(
	char *vsn,
	HashTable_t *hashTable)
{
	int vsnLength = strlen(vsn);
	int c;
	unsigned int key = 5381;

	for (int i = 0; i < vsnLength; i++) {
		c = vsn[i];
		key = ((key << 5) + key) + c; /* key * 33 + c */
	}

	key = key % hashTable->size;

	return (key);
}


/*
 * Inserts the index into the hash table. Indices corresponding to table entries
 * with empty vsns are ignored.
 */
void
InsertIntoHashTable(
	int index,
	char *vsn,
	HashTable_t *hashTable)
{
	unsigned int key;

	if (*vsn != '\0') {
		key = GenerateHashKey(vsn, hashTable);

		while (hashTable->values[key] != HASH_EMPTY) {
			key++;
			if (key > hashTable->size) {
				key = 0;
			}
		}

		hashTable->values[key] = index;
	}
}


/*
 * Extends the hash table. The table is than populated with rehashed media table
 * indices.
 *
 * The first paremeter designates the index of the first empty entry in the
 * media table. Only those indices that are lesser than this value will be
 * rehashed.
 */
void
ExtendHashTable(
	HashTable_t *hashTable)
{

#ifdef HASH_DEBUG
	hashTable->size += HASH_DEBUG_INCREMENT;
#else
	hashTable->size += (TABLE_INCREMENT * HASH_MULTIPLIER);
#endif
	hashTable->values = (int *)Resize((void *)hashTable->values,
	    (size_t)(hashTable->size * sizeof (int)), "hash table");

	for (int i = 0; i < hashTable->size; i++) {
		hashTable->values[i] = HASH_EMPTY;
	}
}


/*
 * Resizes a table. Not only used for resizing the hash table, but also for
 * other recycler's and nrecycler's tables.
 */
void *
Resize(
	void *ptr,
	size_t size,
	char *err_name)
{
	SamRealloc(ptr, size);
	if (ptr == NULL) {
		Trace(TR_MISC, "Error: SamRealloc failed '%s'", err_name);
		abort();
	}

	return (ptr);
}

#ifdef DEBUG
/*
 * Hash table statistics support code
 */
void
DumpHashStats(
	void)
{
	Trace(TR_DEBUG, "Lookups:  %d",  hashStatistics.lookups);
	Trace(TR_DEBUG, "Reallocs: %d", hashStatistics.reallocs);
	Trace(TR_DEBUG, "Probes: %d (%.2f%%)", hashStatistics.probes,
	    pct(hashStatistics.probes));
	Trace(TR_DEBUG, "Founds: %d (%.2f%%)", hashStatistics.founds,
	    pct(hashStatistics.founds));
	Trace(TR_DEBUG, "Adds: %d (%.2f%%)", hashStatistics.adds,
	    pct(hashStatistics.adds));
}

void
PrintHashStats(
	void)
{
	printf("Lookups:  %d\n",  hashStatistics.lookups);
	printf("Reallocs: %d\n", hashStatistics.reallocs);
	printf("Probes: %d (%.2f%%)\n", hashStatistics.probes,
	    pct(hashStatistics.probes));
	printf("Founds: %d (%.2f%%)\n", hashStatistics.founds,
	    pct(hashStatistics.founds));
	printf("Adds: %d (%.2f%%)\n", hashStatistics.adds,
	    pct(hashStatistics.adds));
}


void
IncLookupsHashStats(
	void)
{
	hashStatistics.lookups++;
}


void IncProbesHashStats(
	void)
{
	hashStatistics.probes++;
}


void IncAddsHashStats(
	void)
{
	hashStatistics.adds++;
}


void IncFoundsHashStats(
	void)
{
	hashStatistics.founds++;
}


void IncReallocsHashStats(
	void)
{
	hashStatistics.reallocs++;
}
#endif

#ifdef HASH_DEBUG
/*
 * Dumps the whole hash table
 */
void
DumpHashTable(
	HashTable_t *hashTable)
{
	for (int i = 0; i < hashTable->size; i += 10) {
		printf("%6d: %6d %6d %6d %6d %6d %6d %6d %6d %6d %6d\n", i,
		    hashTable->values[i], hashTable->values[i+1],
		    hashTable->values[i+2], hashTable->values[i+3],
		    hashTable->values[i+4], hashTable->values[i+5],
		    hashTable->values[i+6], hashTable->values[i+7],
		    hashTable->values[i+8], hashTable->values[i+9]);
	}
}
#endif
