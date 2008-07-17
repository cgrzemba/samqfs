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
/*
 * sqm_list.h
 * defines a generic linked list used in the SAM/QFS management software
 */
#ifndef _SQM_LIST_H
#define	_SQM_LIST_H

#pragma ident   "$Revision: 1.4 $"


#include <sys/types.h>

#define	FREEFUNCCAST(s) ((void (*)(void *))s)
#define	CMPFUNCCAST(s) ((int (*)(void *, void *))s)
#define	DUPFUNCCAST(s) ((void * (*)(void *))s)

typedef struct node {
	struct node *next;
	void *data;
} node_t;

typedef struct sqm_lst {
	node_t *head;
	node_t *tail;
	int length;
} sqm_lst_t;

typedef int (*lstsrch_t)(
	const void*	a,
	const void*	b
);

/*
 * linked list operations
 *
 * Each function sets errno and samerrmsg appropriately.
 * Possible errors: SE_NO_MEM, SE_NODE_NOT_FOUND, SE_NULL_PARAMETER
 * Note: NULL lists are valid arguments except for in lst_concat()
 */


/*
 * create an empty list.
 * return NULL if not enough memory
 */
sqm_lst_t *lst_create();

/*
 * Insert data into the list before the specified node. If the list is
 * empty and the node is NULL, data will simply be inserted into the
 * list.
 */
int lst_ins_before(sqm_lst_t *lst, const node_t *node, void *data);

/*
 * Insert data into the list after the specified node. If the
 * list is empty and the node is NULL, the data will simply
 * be inserted into the list.
 */
int lst_ins_after(sqm_lst_t *lst, const node_t *node, void *data);

/*
 * append an element at the end of a list
 * the next field is set to NULL
 */
int lst_append(sqm_lst_t *lst, void *data);

/*
 * remove an element from the list
 * it does NOT deallocate the actual data stored in the node
 */
int lst_remove(sqm_lst_t *lst, node_t *rm_node);

/* concatenate lst1 and lst2; lst2 is appended to lst1 */
int lst_concat(sqm_lst_t *lst1, sqm_lst_t *lst2);

/*
 * release all the memory previously allocated through lst_ functions
 * it does NOT deallocate the actual data stored in the nodes
 */
void lst_free(sqm_lst_t *lst);

/*
 * lst_free() and also call free() on each node's data field.
 */
void lst_free_deep(sqm_lst_t *lst);

/*
 * free a list of complex structs for which free() would not be
 * sufficient. Requires as an argument a function pointer to a
 * function that frees the type of data member that the list contains.
 */
void lst_free_deep_typed(sqm_lst_t *lst, void (*free_type)(void *));


/*
 * Function to make a copy of a typed list. The first function pointer
 * is a pointer to a function that makes a copy of the type of
 * elements contained in the list. The second function pointer is a
 * pointer to a free function for the type so that this function can
 * clean up in the face of errors.
 *
 * If lst is NULL this function will return 0 and set *ret_lst to NULL.
 * If lst is empty an empty list is returned.
 */
int lst_dup_typed(sqm_lst_t *lst, sqm_lst_t **ret_lst,
    void * (*dup) (void *), void (*free_type)(void *));


/*
 * sort the list according to the specified comparison function;
 * the comparison function compares the 'data' fields of the nodes;
 * it has two arguments (pointers to data fields) and returns:
 *   <0 if arg1 <  arg2
 *    0 if arg1 == arg2
 *   >0 if arg1 >  arg2   (in the spirit of strcmp())
 */
void
lst_sort(sqm_lst_t *lst,
    int (*comp)(void *, void *)); /* pointer to the comparison function */


/*
 * Use C library's qsort function to sort a list of strings.
 */
void
lst_qsort(sqm_lst_t *lst, int (*compar)(const void *, const void *));

/* Utility routine for above qsort routine. Does string compare */
int
node_cmp(const void *a, const void *b);


/*
 * return the node that contains 'data', or NULL if not found. since 4.5
 */
node_t *
lst_search(sqm_lst_t *lst, void *data, lstsrch_t comp);


/*
 * merges the two lists, by moving nodes from the second list to the first
 * one, if they don't already exist.
 * Note: this function changes both lists.
 */
void
lst_merge(sqm_lst_t *lst1, sqm_lst_t *lst2, int (*comp)(void *, void *));

/*
 * trims the list, the start and count are provided as input
 * the unwanted nodes are freed
 */
int
trim_list(sqm_lst_t *l, int start, int count);

/*
 * Helper function for lst_search.  Compares a char* to a char**.
 */
int cmp_str_2_str_ptr(const void *a, const void* b);

/*
 * create a list from the tokens contained in the specified string
 */
sqm_lst_t *
str2lst(char *str, const char *delims);

/*
 * return a string containing all the elements in the list
 * separated by the user-specified 'delim'
 *
 * The result string must be free()ed.
 */
char *
lst2str(
	sqm_lst_t *lst,	/* list to be converted */
	const char *delim);	/* data delimiter in the resulting string */

#endif	/* _SQM_LIST_H */
