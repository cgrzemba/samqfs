/*
 * malloc_wait.c - misc routines.
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

#pragma ident "$Revision: 1.14 $"

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sam/types.h"
#include "sam/lib.h"

static void * mem_alloc_wait(const size_t size, const uint_t delay,
	const int attempts, int pg_aligned);

/*
 * valloc_wait - wait for memory.
 *
 * returns - pointer to malloced memory
 */
void *valloc_wait(
	const size_t size,	/* number of bytes. */
	const uint_t delay,	/* time delay between attempts. */
	const int attempts)	/* number of attempts (0) = forever. */
{
	return (mem_alloc_wait(size, delay, attempts, 1));
}

void *malloc_wait(
	const size_t size,	/* number of bytes. */
	const uint_t delay,	/* time delay between attempts. */
	const int attempts)	/* number of attempts (0) = forever. */
{
	return (mem_alloc_wait(size, delay, attempts, 0));
}


/*
 * mem_alloc_wait - wait for memory.
 *
 * returns - pointer to malloced memory
 */

static void *
mem_alloc_wait(
	const size_t size,	/* number of bytes. */
	const uint_t delay,	/* time delay between attempts. */
	const int attempts,	/* number of attempts (0) = forever. */
	int pg_aligned)
{
	int    trys = 1;
	void    *tmp;

	if (size < 1) {
		sam_syslog(LOG_ERR, "Bad call to malloc, size %d", size);
		abort();
	}

	for (;;) {
		if (pg_aligned) {
			tmp = valloc(size);
		} else {
			tmp = malloc(size);
		}
		if (tmp != NULL) {
			break;
		}
		sam_syslog(LOG_WARNING,
		    "Unable to malloc %d bytes of memory.", size);
		if (trys++ == attempts) {
			sam_syslog(LOG_ERR,
			    "Unable to malloc %d bytes of memory, aborting",
			    size);
			abort();
		}
		(void) sleep(delay);
	}
	return (tmp);
}
