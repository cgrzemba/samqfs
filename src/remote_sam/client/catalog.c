/*
 *	catalog.c - maintain client catalog
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

#pragma ident "$Revision: 1.29 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stropts.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "aml/remote.h"
#include "client.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/proto.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"

#include <sam/fs/bswap.h>

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Function prototypes */
static void rs_upd_catalog(dev_ent_t *un, rmt_sam_vsn_entry_t *entry);


/*
 * Ask server to send a list of usable VSNs.
 */
void
request_vsn_list(
	dev_ent_t *un)
{
	rmt_sam_request_t req;

	memset(&req, 0, sizeof (req));
	req.command = RMT_SAM_SEND_VSNS;
	req.version = RMT_SAM_VERSION;

#if defined(__i386) || defined(__amd64)
	if (sam_byte_swap(rmt_sam_request_swap_descriptor,
	    &req, sizeof (rmt_sam_request_t))) {
	SysError(HERE, "Byteswap error sending VSN request");
	}
#endif
	if (write(un->space, &req, sizeof (req)) != sizeof (req)) {
		SysError(HERE, "Write failed (SEND_VSNS)");
	} else {
		Trace(TR_MISC, "Sent VSN request (SEND_VSNS) eq: %d", un->eq);
	}
}


/*
 * Update catalog.
 */
void
update_catalog_entries(
	dev_ent_t *un,
	rmt_sam_request_t *req)
{
	int size;
	void *v_list;
	rmt_sam_update_vsn_t *upd_vsn = &req->request.update_vsn;
	rmt_sam_vsn_entry_t *next_entry;

	size = upd_vsn->count * sizeof (rmt_sam_vsn_entry_t);
	v_list = malloc_wait(size + sizeof (*req), 2, 0);
	/* Copy first one over */
	memcpy(v_list, &upd_vsn->vsn_entry, sizeof (rmt_sam_vsn_entry_t));

	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_AUDIT;
	mutex_unlock(&un->mutex);
	if (upd_vsn->count > 1) {
		int needed = size - sizeof (rmt_sam_vsn_entry_t);
#if defined(__i386) || defined(__amd64)
		rmt_sam_vsn_entry_t *ventry;
#endif
		char *where = ((char *)v_list) + sizeof (rmt_sam_vsn_entry_t);

#if defined(__i386) || defined(__amd64)
		/*LINTED pointer cast may result in improper alignment */
		ventry = (rmt_sam_vsn_entry_t *)where;
#endif
		while (needed > 0) {
			int io_len = read(un->space, where, needed);

			if (io_len == 0) {	/* disconnect */
				free(v_list);
				return;
			}

			if (io_len < 0) {
				SysError(HERE, "Read failed (UPDATE_VSN)");
				/* close will cause disconnect in client */
				(void) close(un->space);
				free(v_list);
				return;
			}
			needed -= io_len;
			where += io_len;
		}
#if defined(__i386) || defined(__amd64)
		for (int i = 1; i < upd_vsn->count; i++, ventry++) {
			if (sam_byte_swap(rmt_sam_vsn_entry_swap_descriptor,
			    ventry, sizeof (rmt_sam_vsn_entry_t))) {
				SysError(HERE, "Byteswap error on VSN update");
			}
		}
#endif
	}

	next_entry = (rmt_sam_vsn_entry_t *)v_list;
	while (upd_vsn->count--) {
		rs_upd_catalog(un, next_entry);
		next_entry++;
	}

	free(v_list);
}


/*
 * Mark each entry in the catalog as unavailable.  Should only be
 * called during start or if the server goes down.
 */
void
mark_catalog_unavail(
	dev_ent_t *un)
{
	int i;
	struct CatalogEntry *list;
	int n_entries;

	(void) CatalogSync();
	list = CatalogGetEntriesByLibrary(un->eq, &n_entries);

	mutex_lock(&un->mutex);
	un->status.bits |= DVST_AUDIT;
	mutex_unlock(&un->mutex);

	for (i = 0; i < n_entries; i++) {
		struct CatalogEntry *ce;

		ce = &list[i];

		if (ce->CeStatus & CES_inuse) {
			uint32_t status;
			int slot, partition;

			status = 0;
			slot = ce->CeSlot;
			partition = ce->CePart;
			status |= CES_unavail;
			(void) CatalogSetFieldByLoc(un->eq, slot, partition,
			    CEF_Status, status, 0);
		}
	}
	free(list);
}


/*
 * Update VSN.
 */
static void
rs_upd_catalog(
	dev_ent_t *un,
	rmt_sam_vsn_entry_t *entry)
{
	struct CatalogEntry ce;
	uint16_t flags = 0;

	Trace(TR_MISC, "Update vsn '%s' upd flags: %d",
	    TrNullString(entry->ce.CeVsn), entry->upd_flags);
	Trace(TR_MISC, "\tstatus: 0x%x capacity: %lld ptoc: %lld",
	    entry->ce.CeStatus, entry->ce.CeCapacity,
	    entry->ce.m.CePtocFwa);

	Trace(TR_DEBUG, "\tmedia: '%s' slot: %d part: %d",
	    TrNullString(entry->ce.CeMtype), entry->ce.CeSlot,
	    entry->ce.CePart);
	Trace(TR_DEBUG, "\tspace: %lld block size: %d",
	    entry->ce.CeSpace, entry->ce.CeBlockSize);
	Trace(TR_DEBUG, "\tlabel time: %d mod time: %d mount time: %d",
	    entry->ce.CeLabelTime, entry->ce.CeModTime,
	    entry->ce.CeMountTime);
	Trace(TR_DEBUG, "\tbar code: '%s'",
	    TrNullString(entry->ce.CeBarCode));

	/*
	 * We don't deal with unlabeled media.
	 */
	if ((entry->ce.CeStatus & CES_labeled) == 0) {
		return;
	}

	/*
	 * Make up a catalog entry from the information we have.
	 * Call the catalog server to update this client's catalog.
	 */
	memset(&ce, 0, sizeof (ce));

	memcpy(&ce, &entry->ce, sizeof (struct CatalogEntry));
	ce.CeEq = un->eq;

	/*
	 * Always set the occupy bit.
	 */
	ce.CeStatus |= CES_occupied;

	/*
	 * Media is being exported, set to unavailable.
	 */
	if (entry->upd_flags == RMT_CAT_CHG_FLGS_EXP) {
		ce.CeStatus |= CES_unavail;
	}

	/*
	 * Media is being relabeled, set relabel flag for catalog server.
	 */
	if (entry->upd_flags == RMT_CAT_CHG_FLGS_LBL) {
		flags = 1;
	}

	(void) CatalogRemoteSamUpdate(&ce, flags);
}
	/* keep lint happy */
#if	defined(lint)
void
swapdummy_C(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
