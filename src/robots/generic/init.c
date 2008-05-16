/*
 * init.c - initialize the library and all its elements
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

#pragma ident "$Revision: 1.23 $"

/* Using __FILE__ makes duplicate strings */
static char *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <syslog.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/dev_log.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "generic.h"
#include "sam/lint.h"

/*	function prototypes */
int generic_initialize(library_t *library);
int api_initialize(library_t *library, dev_ptr_tbl_t *dev_ptr_tbl);
int generic_re_init_library(library_t *library);
int api_re_init_library(library_t *library);


/*	globals */
extern void api_init_drive(drive_state_t *drive);
extern void generic_init_drive(drive_state_t *drive);


/*
 *  initialize - initialize the library
 *  exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
initialize(
library_t *library,
dev_ptr_tbl_t *dev_ptr_tbl)
{
	if (IS_GENERIC_API(library->un->type))
		return (api_initialize(library, dev_ptr_tbl));
	else
		return (generic_initialize(library));
}


/*
 *  re_init_library - reinitialize library.
 *
 *  exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
re_init_library(
library_t *library)
{
	if (IS_GENERIC_API(library->un->type))
		return (api_re_init_library(library));
	else
		return (generic_re_init_library(library));
}


/*
 *  init_drive - initialize drive.
 */
void
init_drive(
drive_state_t *drive)
{
	if (IS_GENERIC_API(drive->library->un->type))
		api_init_drive(drive);
	else
		generic_init_drive(drive);
}


/*
 * reconcile_storage - set final status of each slot in the catalog
 * go through each element and update the status of the storage element
 * that in contained in the element.
 */
void
reconcile_storage(
library_t *library)
{
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId vid;
	dev_ent_t 		*un;
	drive_state_t 	*drive;
	xport_state_t 	*transport;
	iport_state_t 	*import;
	uint32_t 		CatStatus;

	SANITY_CHECK(library != NULL);
	SANITY_CHECK(library->un != NULL);
	un = library->un;

	/* For each element with a valid address, update the catalog entry. */
	memset(&vid, 0, sizeof (vid));
	vid.ViFlags = VI_cart;
	vid.ViEq = library->un->eq;
	memmove(vid.ViMtype, sam_mediatoa(library->un->media),
	    sizeof (vid.ViMtype));
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		struct CatalogEntry ced;
		struct CatalogEntry *ce = &ced;
		int set_default = 1;

		if ((drive->status.b.valid &&
		    (IS_STORAGE(library, drive->media_element)))) {
			vid.ViSlot = SLOT_NUMBER(library, drive->media_element);
			ce = CatalogGetEntry(&vid, &ced);
			if (ce != NULL && (ce->CeStatus & CES_occupied)) {
				if (drive->un != NULL) {
					DevLog(DL_ALL(5339), drive->un->eq,
					    vid.ViSlot);
				} else {
					/* mcf config errors can cause */
					/* drive->un to be NULL. */
					DevLog(DL_ALL(5357));
				}
				drive->status.b.valid = FALSE;
				continue;
			}
			CatStatus = CES_inuse;
			if (drive->status.b.bar_code) {
				CatStatus |= CES_bar_code;
				(void) CatalogSlotInit(&vid, CatStatus,
				    (library->status.b.two_sided) ? 2 : 0,
				    (char *)drive->bar_code, "");
			} else
				(void) CatalogSlotInit(&vid, CatStatus,
				    (library->status.b.two_sided) ?
				    2 : 0, "", "");

			if ((ce = CatalogGetEntry(&vid, &ced)) != NULL) {
				if ((ce->CeStatus & CES_capacity_set) ||
				    (ce->CeCapacity))
					set_default = 0;
			}
			if (set_default) {
				(void) CatalogSetFieldByLoc(library->un->eq,
				    vid.ViSlot, vid.ViPart, CEF_Capacity,
				    DEFLT_CAPC(library->un->media), 0);
				(void) CatalogSetFieldByLoc(library->un->eq,
				    vid.ViSlot, vid.ViPart, CEF_Space,
				    DEFLT_CAPC(library->un->media), 0);
			}
		}
	}

	for (transport = library->transports; transport != NULL;
	    transport = transport->next) {
		if ((transport->status.b.valid &&
		    (IS_STORAGE(library, transport->media_element)))) {
			vid.ViSlot = SLOT_NUMBER(library,
			    transport->media_element);
			ce = CatalogGetEntry(&vid, &ced);
			if (ce != NULL && (ce->CeStatus & CES_occupied)) {
				DevLog(DL_ALL(5340), vid.ViSlot);
				transport->status.b.valid = FALSE;
				continue;
			}
			CatStatus = CES_inuse;
			if (transport->status.b.bar_code) {
				CatStatus |= CES_bar_code;
				(void) CatalogSlotInit(&vid, CatStatus,
				    (library->status.b.two_sided) ? 2 : 0,
				    (char *)transport->bar_code, "");
			} else {
				(void) CatalogSlotInit(&vid, CatStatus,
				    (library->status.b.two_sided) ?
				    2 : 0, "", "");
			}
		}
	}

	if (library->un->type == DT_ACL452 ||
	    IS_GENERIC_API(library->un->type))
		return;

	for (import = library->import;
	    import != NULL;
	    import = import->next) {
		if ((import->status.b.valid &&
		    (IS_STORAGE(library, import->media_element)))) {
			vid.ViSlot = SLOT_NUMBER(library,
			    import->media_element);
			ce = CatalogGetEntry(&vid, &ced);
			if (ce != NULL && (ce->CeStatus & CES_occupied)) {
				DevLog(DL_ALL(5341), vid.ViSlot);
				import->status.b.valid = FALSE;
				continue;
			}
		}
	}

	/*
	 * All cartridges should be back in their appropriate slots now.
	 * Call CatalogReconcileCatalog to free any
	 * catalog entries that no longer have occupied slots.
	 */
	CatalogReconcileCatalog(library->un->eq);
}
