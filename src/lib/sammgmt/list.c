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

#pragma ident   "$Revision: 1.31 $"

#include <stdlib.h>
#include <string.h>
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"

sqm_lst_t *
lst_create(void)
{

	sqm_lst_t *lstp = (sqm_lst_t *)mallocer(sizeof (sqm_lst_t));

	if (NULL != lstp) {
		lstp->length = 0;
		lstp->head = lstp->tail = NULL;
	}
	return (lstp);
}


/*
 * Insert data into the list before the specified node. If the list is
 * empty and the node is NULL, data will simply be inserted into the
 * list.
 */
int
lst_ins_before(
sqm_lst_t *lst,		/* insert in this list */
const node_t *node,	/* insert before this node */
void *data)		/* data stored in the new node */
{

	node_t *crt, *new_node;

	if (ISNULL(lst, data)) {
		return (-1);
	}

	if (lst->length == 0) {
		if (node != NULL) {
			setsamerr(SE_NODE_NOT_FOUND);
			return (-1);
		} else {
			return (lst_append(lst, data));
		}
	}
	if (ISNULL(node)) {
		return (-1);
	}
	if (NULL == (new_node = (node_t *)mallocer(sizeof (node_t))))
		return (-1);
	new_node->data = data;

	if (lst->head == node) {
		new_node->next = lst->head;
		lst->head = new_node;
		lst->length++;
		return (0);
	}

	crt = lst->head;
	while ((crt->next != node) && (crt->next != NULL))
		crt = crt->next;
	if (crt->next == NULL) {

		setsamerr(SE_NODE_NOT_FOUND);
		return (-1);
	}
	new_node ->next = crt->next;
	crt->next = new_node;
	if (lst->head == node)
		lst->head = new_node;
	lst->length++;
	return (0);
}

/*
 * Insert data into the list after the specified node. If the
 * list is empty and the node is NULL, the data will simply
 * be inserted into the list.
 */
int
lst_ins_after(
sqm_lst_t *lst,		/* insert in this list */
const node_t *node,	/* insert after this node */
void *data)		/* data stored in the new node */
{

	node_t *crt, *new_node;

	if (ISNULL(lst, data)) {
		return (-1);
	}

	if (lst->length == 0) {
		if (node != NULL) {
			setsamerr(SE_NODE_NOT_FOUND);
			return (-1);
		} else {
			return (lst_append(lst, data));
		}
	}
	if (ISNULL(node)) {
		return (-1);
	}

	if (NULL == (new_node = (node_t *)mallocer(sizeof (node_t))))
		return (-1);
	new_node->data = data;
	new_node->next = NULL;
	crt = lst->head;
	while ((crt != node) && (crt != NULL))
		crt = crt->next;
	if (crt == NULL) {
		setsamerr(SE_NODE_NOT_FOUND);
		return (-1);
	}
	new_node->next = crt->next;
	crt->next = new_node;
	if (lst->tail == node)
		lst->tail = new_node;
	lst->length++;
	return (0);
}


int
lst_append(
sqm_lst_t *lst,	/* append to this list */
void *data)	/* data stored in the new node */
{

	node_t *new_node;

	if (ISNULL(lst, data))
		return (-1);
	if (NULL == (new_node = (node_t *)mallocer(sizeof (node_t))))
		return (-1);
	new_node->data = (void *)data;
	new_node->next = NULL;
	if (lst->length == 0)
		lst->head = new_node;
	else
		lst->tail->next = new_node;
	lst->tail = new_node;
	lst->length++;
	return (0);
}


int
lst_remove(
sqm_lst_t *lst,		/* remove from this list */
node_t *rm_node)	/* pointer to the node to be removed */
{

	node_t *crt;

	if (ISNULL(lst, rm_node))
		return (-1);
	if (lst->length == 0) {
		setsamerr(SE_NODE_NOT_FOUND);
		return (-1);
	}
	crt = lst->head;
	if (crt == rm_node) {
		lst->head = crt->next;
		if (lst->head == NULL)
			lst->tail = NULL;
	} else {
		if (crt == NULL) {
			setsamerr(SE_NODE_NOT_FOUND);
			return (-1);
		}
		while ((crt->next != NULL) && (crt->next != rm_node))
			crt = crt->next;
		if (crt->next != rm_node) {
			setsamerr(SE_NODE_NOT_FOUND);
			return (-1);
		}
		crt->next = rm_node->next;
		if (crt->next == NULL)
			lst->tail = crt;
	}
	lst->length--;
	free(rm_node);
	return (0);
}


int
lst_concat(
sqm_lst_t *lst1,	/* first list */
sqm_lst_t *lst2)	/* list appended to the first list, then destroyed */
{

	if (ISNULL(lst1, lst2)) {
		return (-1);
	}
	if (lst2->length == 0) {
		free(lst2);
		return (0);
	}
	lst1->length += lst2->length;
	if (lst1->tail != NULL)
		lst1->tail->next = lst2->head;
	else
		lst1->head = lst2->head;
	lst1->tail = lst2->tail;
	free(lst2);
	return (0);
}


void
lst_free(
sqm_lst_t *lst)	/* free this list (but not the data) */
{

	node_t *node, *rmnode;

	if (lst == NULL)
		return;
	node = lst->head;
	while (node != NULL) {
		rmnode = node;
		node = node->next;
		free(rmnode);
	}
	free(lst);
}


void
lst_free_deep(
sqm_lst_t *lst)	/* free this list and its data */
{

	node_t *node;

	if (lst == NULL)
		return;
	node = lst->head;
	while (node != NULL) {
		if (node->data != NULL) {
			free(node->data);
		}
		node = node->next;
	}
	lst_free(lst);
}


/* swap node1 with node2 */
static void
swap_consec_nodes(
sqm_lst_t *lst,
node_t *n0)
{

	node_t *n1, *n2;

	if (n0 == NULL) {
		n1 = lst->head;
		n2 = n1->next;
		lst->head = n2;
	} else {
		n1 = n0->next;
		n2 = n1->next;
	}
	if (n2 == lst->tail)
		lst->tail = n1;
	if (n2 != NULL) {
		if (n0 != NULL)
			n0->next = n2;
		n1->next = n2->next;
		n2->next = n1;
	}
}


/* bubble sort */
void
lst_sort(
sqm_lst_t *lst,	/* list to be sorted */
int (*comp)(void *, void *))	/* pointer to a comparison function */
{

	int done = 0;
	int n = 0;	 /* current number of traversals */
	node_t *n0, *n1, *n2;

	/* can't sort a list that doesn't have at least 2 entries */
	if ((lst == NULL) || (lst->length < 2)) {
		return;
	}

	while (!done) {
		n0 = NULL;
		n1 = lst->head;
		n2 = n1->next;
		done = 1;
		do {
			if ((*comp)(n1->data, n2->data) > 0) {
				swap_consec_nodes(lst, n0);
				done = 0;
			}
			if (n0 == NULL)
				n0 = lst->head;
			else
				n0 = n0->next;
			n1 = n0->next;
		}	while ((n2 = n1->next) != NULL);
		if (++n == lst->length)
			done = 1;
	}
}



int
node_cmp(const void *a, const void *b)
{
	return (strcmp((*(node_t **)a)->data, (*(node_t **)b)->data));
}

/*
 * Sort list using C-library's quick-sort. If the list is more than
 * a couple dozen entries long, this is probably faster than bubble sort.
 */

void
lst_qsort(sqm_lst_t *lst, int (*compar)(const void *, const void *))
{
	node_t **sortdb;
	node_t *nodep;
	int i;
	int n;

	if (ISNULL(lst, compar)) {
		return;
	}

	n = lst->length;

	if (n < 2) {
		/* nothing to sort */
		return;
	}

	/* Allocate and fill sort structure - n_child pointers */
	sortdb = malloc((n+1) * sizeof (node_t *));
	nodep = lst->head;	/* Start at front of chain */
	for (i = 0; i < n; i++) {
		sortdb[i] = nodep;
		nodep = nodep->next;
	}
	sortdb[n] = NULL;	/* End-of-list */

	/* Sort. */
	qsort(sortdb, n, sizeof (node_t *), compar);

	/* Re-arrange chain in new order */
	lst->head = sortdb[0];
	for (i = 0; i < n; i++)
		sortdb[i]->next = sortdb[i+1];
	lst->tail = sortdb[n-1];

	free(sortdb);

}


/*
 * return a string containing all the elements in the list
 * separated by the user-specified 'delim'
 *
 * The result string must be free()ed.
 */
char *
lst2str(
sqm_lst_t *lst,		/* list to be converted */
const char *delim)	/* node data delimiter in the resulting string */
{

	node_t *n;
	char *outstr = NULL;
	size_t len = 0;

	if (ISNULL(lst, delim)) {
		return (NULL);
	}

	/*
	 * loop through the string twice.  First to get the string
	 * length, second time to actually generate the string
	 */

	n = lst->head;
	while (n) {
		if (n->data != NULL) {
			len += strlen((char *)n->data);
		}
		n = n->next;
	}

	/* increase len to include delimiter + trailing nul */
	len += (lst->length * sizeof (delim)) + 1;

	outstr = calloc(len, 1);
	if (outstr == NULL) {
		return (NULL);
	}

	n = lst->head;
	while (n) {
		if (n->data != NULL) {
			strlcat(outstr, (char *)n->data, len);
		}
		n = n->next;
		if (n != NULL) {
			strlcat(outstr, delim, len);
		}
	}

	return (outstr);
}


/*
 * create a list from the tokens contained in the specified string
 */
sqm_lst_t *
str2lst(
char *str,	/* string to be converted to a list */
const char *delims)	/* delimiter for elements within the string */
{

	char *strcp =  strdup(str);
	char *token = NULL;
	char *last = NULL;

	sqm_lst_t *lst = lst_create();

	if (lst == NULL) {
		free(strcp);
		return (NULL);
	}

	token = strtok_r(strcp, delims, &last);
	while (token != NULL) {
		/* strip leading whitespace if any */
		token += strspn(token, WHITESPACE);
		if (token != NULL) {
			lst_append(lst, strdup(token));
			token = strtok_r(NULL, delims, &last);
		}
	}
	free(strcp);
	return (lst);
}


node_t *
lst_search(
sqm_lst_t *lst,
void *data,	/* data to be searched */
int (*comp)(const void *, const void*))	/* comparison function */
{
	node_t *node;
	if (ISNULL(lst, data, comp))
		return (NULL);
	node = lst->head;
	while (node != NULL) {
		if (0 == (comp(data, node->data)))
			return (node);
		node = node->next;
	}
	return (NULL);
}


void
lst_merge(
sqm_lst_t *lst1,	/* result will go here */
sqm_lst_t *lst2,	/* non-duplicates are moved from here */
int (*comp)(void *, void *))
{
	node_t *n1, *n2, *initial_tail, *saved_nxt;
	int found = -1;

	if (lst1 == NULL || lst2 == NULL)
		return;
	if (lst1->length == 0) {
		lst_concat(lst1, lst2);
		return;
	}
	initial_tail = lst1->tail;
	n2 = lst2->head;
	while (n2 != NULL) {
		n1 = lst1->head;
		found = 0;
		do {
			if (comp(n1->data, n2->data) == 0) {
				found = 1;
				break;
			}
			n1 = n1->next;
		} while (n1 != initial_tail->next);

		saved_nxt = n2->next;
		if (!found) {
			lst_append(lst1, n2->data);
			lst_remove(lst2, n2);
		}
		n2 = saved_nxt;
	}
}


void
lst_free_deep_typed(
sqm_lst_t *lst,
void (*free_type)(void *))	/* function to free the data members */
{

	node_t *node;

	if (lst == NULL)
		return;
	node = lst->head;
	while (node != NULL) {
		if (node->data != NULL) {
			free_type(node->data);
		}
		node = node->next;
	}
	lst_free(lst);
}


int
lst_dup_typed(sqm_lst_t *lst, sqm_lst_t **ret_lst,
    void * (*dup) (void *), void (*free_type) (void *)) {

	node_t *n;

	if (ISNULL(ret_lst, dup)) {
		return (-1);
	}

	if (lst == NULL) {
		*ret_lst = NULL;
		return (0);
	}
	*ret_lst = lst_create();
	if (*ret_lst == NULL) {
		return (-1);
	}

	for (n = lst->head; n != NULL; n = n->next) {
		void *copy;

		if (n->data == NULL) {
			lst_free_deep_typed(*ret_lst, free_type);
			*ret_lst = NULL;
			return (-1);
		}
		copy = dup(n->data);
		if (copy == NULL) {
			lst_free_deep_typed(*ret_lst, free_type);
			*ret_lst = NULL;
			return (-1);
		}
		if (lst_append(*ret_lst, copy) != 0) {
			free_type(copy);
			lst_free_deep_typed(*ret_lst, free_type);
			*ret_lst = NULL;
			return (-1);
		}
	}
	return (0);
}


/*
 * trim the list (start and count are provided)
 */
int
trim_list(
sqm_lst_t *l,
int start,
int count) {

	int i;
	node_t *n;
	node_t *tmp;
	int length = 0;

	for (i = 0, n = l->head; i < l->length; i++) {

		if (i < start) {
			tmp = n;
			free(n->data);
			n = n->next;
			free(tmp);
			continue;
		} else if (i == start) {
			l->head = n;
			n = n->next;
			length++;
			if (count == 1) {
				l->head->next = NULL;
				l->tail = l->head;
			}

		} else if (i == (start + count - 1)) {

			/*
			 * set the tail to the last requested node but make
			 * sure to update the n so we can continue and free
			 * unneeded trailing nodes and data
			 */
			l->tail = n;
			tmp = n->next;
			n->next = NULL;
			n = tmp;
			length++;

		} else if (i >= start + count) {
			/*
			 * free the unneeded elements and the nodes
			 * that extend past the requested portion of the list
			 */
			free(n->data);
			tmp = n;
			n = n->next;
			free(tmp);
			continue;
		} else {
			length++;
			n = n->next;
		}
	}

	l->length = length;
	return (0);
}

/*
 * The interface conforms to that expected by list.h lst_search. But
 * this function takes as arguments a char pointer (a), and a pointer to a
 * pointer (b). Can be used to compare a the string referenced by the
 * char ptr (a) to a string that is referenced by a char ptr that is the
 * first element of a struct pointed to by b.
 */
int
cmp_str_2_str_ptr(const void *a, const void* b) {
	if (a == NULL || b == NULL) {
		return (1);
	}
	return (strcmp((const char *)a, (*(char **)b)));
}



#ifdef TEST
#include <string.h>
int
mycmp(void *i1, void *i2)
{
	return (strcmp((char *)i1, (char *)i2));
}
void
prt(sqm_lst_t *lst) {
	node_t *node = lst->head;
	int i;
	printf("len:%d head:%d tail:%d\n", lst->length, lst->head, lst->tail);
	for (i = 0; i < lst->length; i++) {
		printf("el.%d=%s\tnext=%d\n", i, (char *)node->data,
		    node->next);
		node = node->next;
	}
}
void
tst1()
{
	sqm_lst_t *lst = lst_create();
	node_t *rm;

	lst_append(lst, "b");
	lst_append(lst, "c2");
	lst_append(lst, "a");
	lst_append(lst, "f");
	prt(lst);
	lst_ins_after(lst, lst->head, "z");
	lst_ins_after(lst, lst->tail, "q");
	lst_ins_after(lst, lst->head->next, "3rd");
	prt(lst);
	printf("sorting then removing head\n");
	lst_sort(lst, mycmp);
	rm = lst->head;
	lst_remove(lst, lst->rm);
	prt(lst);
	printf("trying to remove non-existent node...");
	if (-1 == lst_remove(lst, rm))
		printf("failed\n");
	else
		printf("success\n");
	printf("test1 done\n");
}
void
tst2()
{
	sqm_lst_t *l = str2lst("str1 str2 str3", " ");
	prt(l);
	printf("now back to string:");
	printf("%s.\n", lst2str(l, " "));
	lst_free_deep(l);
	printf("test2 done\n");
}
void
main()
{
	tst1();
	tst2();
}
#endif
