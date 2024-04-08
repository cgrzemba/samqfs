/*
 * linklist.c - maintain the worklist linklist.
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

#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "librobots.h"
#include "aml/proto.h"

#pragma ident "$Revision: 1.16 $"


/*
 * init_list - malloc count worklist entries and build a linked list of
 * the entries.
 *
 */

robo_event_t *
init_list(int count)
{
	int i;
	register robo_event_t   *new, *next, *last;

	if (count == 0)
		return (NULL);

	new = (robo_event_t *)malloc_wait(sizeof (robo_event_t), 2, 0);
	memset(new, 0, sizeof (robo_event_t));
	new->status.bits = REST_FREEMEM;
	for (i = 1, last = new; i < count; i++) {
		next = (robo_event_t *)malloc_wait(sizeof (robo_event_t), 2, 0);
		memset(next, 0, sizeof (robo_event_t));
		next->status.bits = REST_FREEMEM;
		last->next = next;
		next->previous = last;
		last = next;
	}

	return (new);
}

/*
 * add_active_list - insert an entry before an entry.
 *
 */

robo_event_t *
add_active_list(
	robo_event_t *old,
	robo_event_t *new)
{
	new->next = old;		/* point new's next to current */
	new->previous = old->previous;	/* new's previous is old's */
	old->previous = new;		/* old's previous points to new */
	if (new->previous != (robo_event_t *)NULL)
		new->previous->next = new;
	return (new);
}

/*
 * append_list - insert an entry after the old entry.
 *
 *  Mutex on new should be held on entry.
 */

robo_event_t *
append_list(
	robo_event_t *old,
	robo_event_t *new)
{
	new->next = old->next;
	old->next = new;
	new->previous = old;
	if (new->next != (robo_event_t *)NULL)
		new->next->previous = new;
	return (new);
}

/*
 * unlink_list - remove the entry from the list.
 *
 * mutex for the entry to be removed must be held on entry.
 * returns pointer to entry after entry being removed.
 */

robo_event_t *
unlink_list(robo_event_t *entry)
{
	register robo_event_t *next = entry->next;

	if (entry->previous != (robo_event_t *)NULL)
		entry->previous->next = entry->next;

	if (entry->next != (robo_event_t *)NULL)
		entry->next->previous = entry->previous;

	entry->next = entry->previous = NULL;
	return (next);
}
