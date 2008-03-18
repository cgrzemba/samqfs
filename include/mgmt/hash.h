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
#ifndef	HASH_H
#define	HASH_H

#pragma ident	"$Revision: 1.9 $"


/* default to a prime size */
#define	DEFAULT_TBL_SIZE 127

/*
 * hash.h
 * Interfaces for a chaining hashtable.
 * The current implementation uses libelf.h elf_hash() for hashing strings.
 */


typedef struct hnode {
	char	*key;
	void	*data;
	struct hnode *next;
} hnode_t;


typedef struct hashtable {
	int size;
	int count;
	hnode_t **items; /* ptr to ptr to allow possible rehashing. */
} hashtable_t;


typedef struct ht_iterator {
	hashtable_t *t;
	int visited;	/* how many elements have been returned */
	int next_index;
	hnode_t *next_item;
} ht_iterator_t;


/*
 * create and initialize a hash table
 */
hashtable_t *ht_create();


/*
 * associate the value with the key in the hash table.
 * If a value has already been associated with the key SE_DUPLICATE_ENTRY
 * will be returned.
 */
int
ht_put(hashtable_t *h, char *key, void *value);



/*
 * returns a pointer to the structure in the hash table if it exists.
 * If no entry is found *value will be set to NULL and 0 will be
 * returned.
 */
int
ht_get(hashtable_t *h, char *key, void **value);


/*
 * remove from the hashtable the value associated with the key.
 * return the value. If no entry is found *value will be set to
 * NULL and 0 will be returned.
 */
int
ht_remove(hashtable_t *h, char *key, void **value);



/*
 * create an iterator for the hashtable t. The underlying table should not
 * be modified after itteration has begun.
 */
int
ht_get_iterator(hashtable_t *t, ht_iterator_t **it);


int
ht_has_next(ht_iterator_t *it);


/*
 * get the next element from the iterator.
 */
int
ht_get_next(ht_iterator_t *itt, char **key, void **value);


/*
 * extension of hashtable put that uses lists of elements for duplicate keys
 * instead of returning an error. Subsequent gets will return a list of
 * elements that share the same key.
 * This can be only used to group items in a list if they share the same key
 */
int list_hash_put(hashtable_t *t, char *key, void *value);


/*
 * Free only the hashtable and hashtable nodes
 * external handles to the data must be preserved or all data must be
 * removed prior to calling this function.
 */
void
free_hashtable(hashtable_t **);


void
ht_free_deep(hashtable_t **t, void (*data_free)(void*));


void
free_ht_iterator(ht_iterator_t *);

#endif /* HASH_H */
