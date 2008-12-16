
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

#include <libelf.h>
#include <strings.h>
#include <stdlib.h>
#include "mgmt/hash.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"


static unsigned long get_hashcode(hashtable_t *t, char *key);

hashtable_t *
ht_create(void) {

	hashtable_t *t = NULL;

	t = (hashtable_t *)mallocer(sizeof (hashtable_t));
	if (t == NULL) {
		return (NULL);
	}

	memset(t, 0, sizeof (hashtable_t));


	t->items = (hnode_t **)calloc(DEFAULT_TBL_SIZE, sizeof (hnode_t *));
	if (t->items == NULL) {
		free(t);
		setsamerr(SE_NO_MEM);
		return (NULL);
	}

	t->size = DEFAULT_TBL_SIZE;

	return (t);
}


int
ht_put(
hashtable_t *t,
char *key,
void *value)
{

	unsigned long hashcode;
	hnode_t *new_node, *cur;
	void *tmpval = NULL;

	if (ht_get(t, key, &tmpval) != 0) {
		return (-1);
	} else if (tmpval != NULL) {
		samerrno = SE_DUPLICATE_ENTRY;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_DUPLICATE_ENTRY), key);
		return (-1);
	}

	hashcode = get_hashcode(t, key);

	new_node = (hnode_t *)mallocer(sizeof (hnode_t));
	if (new_node == NULL) {
		return (-1);
	}

	new_node->key = key;
	new_node->data = value;
	new_node->next = NULL;
	cur = t->items[hashcode];
	if (cur == NULL) {
		/* insert new */
		t->items[hashcode] = new_node;
		t->count++;
	} else {
		/* add to chain */
		new_node->next = cur;
		t->items[hashcode] = new_node;
		t->count++;
	}

	return (0);
}


static unsigned long
get_hashcode(hashtable_t *t, char *key) {
	unsigned long hashcode = 0;

	hashcode = elf_hash(key);
	hashcode %= t->size;
	return (hashcode);
}


int
ht_get(
hashtable_t *t,
char *key,
void **value)
{

	unsigned long hashcode;
	hnode_t *cur;


	/* calculate hash */
	hashcode = get_hashcode(t, key);

	/* see if table has entry and find node that has matching key */
	cur = t->items[hashcode];
	while (cur != NULL) {
		if (strcmp(cur->key, key) == 0) {
			*value = cur->data;
			return (0);
		}
		cur = cur->next;
	}

	/* key not found */
	*value = NULL;
	return (0);
}


int
ht_remove(
hashtable_t *t,
char *key,
void **value)
{

	unsigned long hashcode;
	hnode_t *cur, *prev;


	/* calculate hash */
	hashcode = get_hashcode(t, key);

	/* see if table has entry */
	if (t->items[hashcode] == NULL) {
		*value = NULL;
		return (0);
	}


	/* find a node with a matching key */
	cur = t->items[hashcode];
	prev = cur;
	while (cur != NULL) {

		if (strcmp(cur->key, key) == 0) {
			t->count--;

			/* setup the return and free the hash node and key */
			if (prev == cur) {
				t->items[hashcode] = cur->next;
			} else {
				prev->next = cur->next;
			}
			*value = cur->data;

			free(cur);
			return (0);
		}
		prev = cur;
		cur = cur->next;
	}

	/* key not found */
	*value = NULL;
	return (0);
}


int
print_hash_stats(hashtable_t *t) {

	hnode_t *cur;
	int count = 0;
	int full = 0;
	int empty = 0;
	int i;

	for (i = 0; i < t->size; i++) {
		if (t->items[i] != NULL) {
			full++;
			count = 0;
			cur = t->items[i];
			while (cur != NULL) {
				count++;
				cur = cur->next;
			}
		} else {
			empty++;
		}
	}

	return (0);
}


void
free_hashtable(hashtable_t **t) {
	int i;
	hnode_t *cur, *next;

	if (t == NULL || *t == NULL)
		return;

	for (i = 0; i < (*t)->size; i++) {

		cur = (*t)->items[i];
		while (cur != NULL) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	free((*t)->items);
	free((*t));
	*t = NULL;
}


void
ht_free_deep(hashtable_t **t, void (*data_free)(void*)) {
	int i;
	hnode_t *cur, *next;

	if (t == NULL || *t == NULL)
		return;

	for (i = 0; i < (*t)->size; i++) {

		cur = (*t)->items[i];
		while (cur != NULL) {
			next = cur->next;
			if (cur->data != NULL) {
				(*data_free)(cur->data);
			}
			free(cur);
			cur = next;
		}
	}
	free((*t)->items);
	free((*t));
	*t = NULL;
}


void
ht_list_hash_free_deep(hashtable_t **t, void (*data_free)(void *)) {
	int i;
	hnode_t *cur, *next;

	if (t == NULL || *t == NULL)
		return;

	for (i = 0; i < (*t)->size; i++) {

		cur = (*t)->items[i];
		while (cur != NULL) {
			next = cur->next;
			if (cur->data != NULL) {
				lst_free_deep_typed((sqm_lst_t *)cur->data,
				    FREEFUNCCAST(*data_free));
			}
			free(cur);
			cur = next;
		}
	}
	free((*t)->items);
	free((*t));
	*t = NULL;
}


static int
ht_next(
ht_iterator_t *it,
char **key,
void **value,
boolean_t detach)
{

	/* traverse in two dimensions, along table and down chains */

	if (it->visited >= it->t->count) {
		/* done */
		samerrno = SE_NO_MORE_ELEMENTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_MORE_ELEMENTS));
		return (-1);
	}



	/* if next item is null find the next chain in the array */
	if (it->next_item == NULL) {
		while (it->next_index < it->t->size &&
		    it->t->items[it->next_index] == NULL) {

			it->next_index++;
		}
		if (it->next_index >= it->t->size) {
			samerrno = SE_NO_MORE_ELEMENTS;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NO_MORE_ELEMENTS));
			return (-1);
		}
		it->next_item = it->t->items[it->next_index];
		it->next_index++; /* increment now or you loop on the chain */
	}


	if (it->next_item == NULL) {
		samerrno = SE_NO_MORE_ELEMENTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_MORE_ELEMENTS));

		return (-1);
	}

	/*
	 * Set the key and value.
	 */
	*key = it->next_item->key;
	*value = it->next_item->data;

	if (detach) {
		it->next_item->key = NULL;
		it->next_item->data = NULL;
	}

	it->next_item = it->next_item->next;
	it->visited++;

	return (0);
}


int
ht_detach_next(
ht_iterator_t *it,
char **key,
void **value)
{

	return (ht_next(it, key, value, B_TRUE));
}

int
ht_get_next(
ht_iterator_t *it,
char **key,
void **value)
{
	return (ht_next(it, key, value, B_FALSE));
}

int
ht_has_next(ht_iterator_t *it) {

	if (it->visited < it->t->count) {
		return (B_TRUE);
	}

	return (B_FALSE);
}


int
ht_get_iterator(hashtable_t *t, ht_iterator_t **it) {
	ht_iterator_t *tmp_it;

	if (ISNULL(t, it)) {
		return (-1);
	}

	tmp_it = (ht_iterator_t *)mallocer(sizeof (ht_iterator_t));
	if (tmp_it == NULL) {
		return (-1);
	}

	memset(tmp_it, 0, sizeof (ht_iterator_t));

	tmp_it->t = t;
	*it = tmp_it;
	return (0);
}


/*
 * This is based on elf_hash but works on byte arrays or structs that can
 * have null bytes. Not currently used.
 */
unsigned long
hash_n_bytes(const char *key, int n) {


	unsigned long h = 0;
	unsigned long g = 0;
	int i;

	for (i = 0; i < n; i++) {
		h = (h << 4) + key[i];
	/* LINTED:assignment operator "=" found where "==" was expected */
		if (g = h & 0XF0000000) {
			h ^= g >> 24;
		}

		h &= ~g;
	}

	return (h);
}

/*
 * extension of hashtable put that uses lists of elements for duplicate keys
 * instead of returning an error. Subsequent gets will return a list of
 * elements that share the same key.
 * This can be only used to group items in a list, if they have the same key.
 */
int
list_hash_put(
hashtable_t *t,
char *key,
void *value)
{

	void *tmp;
	sqm_lst_t *l;


	if (ht_get(t, key, &tmp) != 0) {
		return (-1);
	} else if (tmp == NULL) {
		l = lst_create();
		if (l == NULL) {
			return (-1);
		}
		if (lst_append(l, value) != 0) {
			return (-1);
		}
		if (ht_put(t, key, l) != 0) {
			return (-1);
		}
	} else {
		if (lst_append((sqm_lst_t *)tmp, value) != 0) {
			return (-1);
		}
	}

	return (0);
}
