/*
 * vsn_list.c - send vsns to the client
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

#pragma ident "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "server.h"

#include <sam/fs/bswap.h>

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
extern shm_alloc_t master_shm;

/* Local structs */
typedef struct {
	rmt_sam_client_t *client;
	rmt_vsn_equ_list_t *next_equ;
} d_list_t;

static void build_vsn_list(rmt_vsn_equ_list_t *next_equ,
	rmt_sam_client_t *client);
static void *send_list_thread(void *vclient);

/*
 * A thread routine to send the vsn list to the client.
 */
void *
send_vsn_list(
	void *vclient)
{
	rmt_vsn_equ_list_t *next_equ;
	rmt_sam_client_t *client;

	client = (rmt_sam_client_t *)vclient;
	Trace(TR_MISC, "[%s] Send VSN list", TrNullString(client->host_name));

	/*
	 * Go through each media changer and the historian and find media
	 * that this client can access.
	 * FIXME need to do something about media in the historian when the
	 * system is running in un-attended mode or the media is unavail.
	 */
	for (next_equ = client->first_equ;
	    next_equ != NULL; next_equ = next_equ->next) {

		dev_ent_t *this_un;

		this_un = next_equ->un;
		if (((this_un->status.bits & DVST_READY) == 0) &&
		    (IS_HISTORIAN(this_un) == 0)) {

			d_list_t *d_list;

			/*
			 * Device not ready.
			 */
			d_list = (d_list_t *)malloc_wait(
			    sizeof (*d_list), 2, 0);

			d_list->client = client;
			d_list->next_equ = next_equ;

			/*
			 * If thread creates works, continue with next device,
			 * else just do it.
			 */
			if (thr_create(NULL, DF_THR_STK, send_list_thread,
			    (void *) d_list, THR_DETACHED, NULL) == 0) {
				continue;
			}

			SysError(HERE, "Unable to create send list thread");
		}
		build_vsn_list(next_equ, client);
	}

	thr_exit(NULL);
	/* LINTED Function has no return statement */
}

int
add_vsn_entry(
	rmt_sam_client_t *client,
	rmt_entries_found_t *found,
	struct CatalogEntry *ce,
	uint_t flags)
{
	int rval;
	rmt_sam_vsn_entry_t *target;

	target = &found->entries[found->count++];

	memset(target, 0, sizeof (*target));
	memcpy(&target->ce, ce, sizeof (struct CatalogEntry));

	target->upd_flags = flags;

	Trace(TR_MISC, "[%s] Update vsn '%s' upd flags: %d",
	    TrNullString(client->host_name),
	    TrNullString(target->ce.CeVsn), target->upd_flags);
	Trace(TR_MISC, "\tstatus: 0x%x capacity: %lld ptoc: %lld",
	    target->ce.CeStatus, target->ce.CeCapacity, target->ce.m.CePtocFwa);

	Trace(TR_DEBUG, "\tmedia: '%s' slot: %d part: %d",
	    TrNullString(target->ce.CeMtype),
	    target->ce.CeSlot, target->ce.CePart);
	Trace(TR_DEBUG, "\tspace: %lld block size: %d",
	    target->ce.CeSpace, target->ce.CeBlockSize);
	Trace(TR_DEBUG, "\tlabel time: %d mod time: %d mount time: %d",
	    target->ce.CeLabelTime, target->ce.CeModTime,
	    target->ce.CeMountTime);
	Trace(TR_DEBUG, "\tbar code: '%s'",
	    TrNullString(target->ce.CeBarCode));

	rval = 0;
	if (found->count == RMT_SEND_VSN_COUNT) {
		rval = flush_list(client, found);
	}

	return (rval);
}

int
flush_list(
	rmt_sam_client_t *client,
	rmt_entries_found_t *found)
{
	int io_len;
	int write_size;
	struct iovec io_vector[2];
	rmt_sam_request_t req;
	rmt_sam_update_vsn_t *upd_vsn;
#if defined(__i386) || defined(__amd64)
	int i;
	rmt_sam_vsn_entry_t *ventry;
#endif

	upd_vsn = &req.request.update_vsn;
	memset(&req, 0, sizeof (req));
	req.command = RMT_SAM_UPDATE_VSN;
	req.version = RMT_SAM_VERSION;
	upd_vsn->count = found->count;
	memcpy(&upd_vsn->vsn_entry, &found->entries[0],
	    sizeof (rmt_sam_vsn_entry_t));

	io_vector[0].iov_base = (caddr_t)& req;
	write_size = io_vector[0].iov_len = sizeof (rmt_sam_request_t);
	io_vector[1].iov_base = (caddr_t)& found->entries[1];
	write_size += (io_vector[1].iov_len =
	    sizeof (rmt_sam_vsn_entry_t) * (found->count - 1));

#if defined(__i386) || defined(__amd64)
	if (sam_byte_swap(rmt_sam_request_swap_descriptor,
	    &req, sizeof (rmt_sam_request_t))) {
		SysError(HERE, "Byteswap error on update vsn req to client %s",
		    TrNullString(client->host_name));
	}

	/* swap header and first entry  */
	if (sam_byte_swap(rmt_sam_update_vsn_swap_descriptor,
	    upd_vsn, sizeof (rmt_sam_update_vsn_t))) {
		SysError(HERE, "Byteswap error on VSN update to client %s",
		    TrNullString(client->host_name));
	}

	/* swap rest of entries also */
	ventry = &found->entries[1];
	for (i = 1; i < found->count; i++, ventry++) {
		if (sam_byte_swap(rmt_sam_vsn_entry_swap_descriptor,
		    ventry, sizeof (rmt_sam_vsn_entry_t))) {
			SysError(HERE, "Byteswap error on VSN update");
		}
	}
#endif

	if ((io_len = writev(client->fd, io_vector, 2)) != write_size) {
		SysError(HERE, "Writev failed: request 0x%x, returned 0x%x",
		    write_size, io_len);
		return (1);
	}
	Trace(TR_MISC, "[%s] Sent update catalog command (UPDATE_VSN) vsns: %d",
	    TrNullString(client->host_name), found->count);

	found->count = 0;
	return (0);
}

/* return 1 if matches, else return 0 */
int
run_regex(
	rmt_vsn_equ_list_t *current_equ,
	char *string)
{
	int rval;

	rval = 0;

	switch (current_equ->comp_type) {

	case RMT_COMP_TYPE_REGCOMP:
		rval = regexec((regex_t *)current_equ->comp_exp,
		    string, 0, NULL, 0);
		if (rval != 0) {
#if defined(DEBUG)
			char err_buf[120];

			regerror(rval, (regex_t *)current_equ->comp_exp,
			    &err_buf[0], 120);
			Trace(TR_DEBUG, "Regular expression exec: %s: %s",
			    TrNullString(&err_buf[0]), TrNullString(string));
#endif
			rval = 0;
		} else {
			rval = 1;
		}
		break;

	case RMT_COMP_TYPE_COMPILE:
		rval = step(string, (char *)current_equ->comp_exp);
		break;

	}

	return (rval);
}

static void
build_vsn_list(
	rmt_vsn_equ_list_t *next_equ,
	rmt_sam_client_t *client)
{
	int i;
	int n_entries;
	struct CatalogEntry *list;
	rmt_entries_found_t *found;
	dev_ent_t *this_un;

	this_un = next_equ->un;
	Trace(TR_MISC, "[%s] Build VSN list eq: %d",
	    TrNullString(client->host_name), this_un->eq);

	found = (rmt_entries_found_t *)malloc_wait(sizeof (*found), 2, 0);
	found->count = 0;

	(void) CatalogSync();
	list = CatalogGetEntriesByLibrary(this_un->eq, &n_entries);

	for (i = 0; i < n_entries; i++) {
		struct CatalogEntry *ce;

		ce = &list[i];

		/*
		 * If any of these bits are set or the media type does
		 * not match, ignore the entry
		 */
		if ((ce->CeStatus & CES_NOT_ON) ||
		    (sam_atomedia(ce->CeMtype) != next_equ->media)) {
			continue;
		}

		/*
		 * CES_MATCH bits must all be on and expression must match.
		 */
		if (((ce->CeStatus & CES_MATCH) == CES_MATCH) &&
		    run_regex(next_equ, (char *)ce->CeVsn) &&
		    add_vsn_entry(client, found, ce, 0)) {

			/*
			 * Only failure of add_vsn_entry will get up here, only
			 * way it fails is if the connection is broken.
			 */
			free(found);
			client->flags.bits &= ~CLIENT_VSN_LIST;
			free(list);
			return;
		}
	}
	free(list);

	if (found->count > 0) {
		(void) flush_list(client, found);
	}

	free(found);
	client->flags.bits &= ~CLIENT_VSN_LIST;
}

/*
 * The device was not ready, wait for it to ready then send the vsn list.
 */
static void *
send_list_thread(
	void *vd_list)
{
	d_list_t *d_list;
	rmt_vsn_equ_list_t *next_equ;
	rmt_sam_client_t *client;
	dev_ent_t *this_un;

	d_list = (d_list_t *)vd_list;
	next_equ = d_list->next_equ;
	client = d_list->client;
	this_un = next_equ->un;

	Trace(TR_MISC, "[%s] Send list thread started eq: %d",
	    TrNullString(client->host_name), this_un->eq);

	free(vd_list);

	/* Wait forever for the device to come ready */
	while (!(this_un->status.bits & DVST_READY)) {
		Trace(TR_MISC, "[%s] Device not ready eq: %d",
		    client->host_name, this_un->eq);
		sleep(10);
	}

	build_vsn_list(next_equ, client);

	thr_exit(NULL);
	/* LINTED Function has no return statement */
}
	/* keep lint happy */
#if	defined(lint)
void
swapdummy_v(void *buf)
{
	sam_bswap2(buf, 1);
	sam_bswap4(buf, 1);
	sam_bswap8(buf, 1);
}
#endif /* defined(lint) */
