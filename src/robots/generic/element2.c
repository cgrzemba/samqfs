/*
 * element2.c - routines for element status.
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

#pragma ident "$Revision: 1.40 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "aml/external_data.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/mode_sense.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "element.h"
#include "aml/dev_log.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"

/*	function prototypes (shared with element.c) */
void
copy_xport_status(xport_state_t *, transport_element_t *,
		transport_element_ext_t *, element_status_page_t *);
void
copy_drive_status(drive_state_t *, data_transfer_element_t *,
		data_transfer_element_ext_t *, element_status_page_t *);
void
copy_import_status(iport_state_t *, import_export_element_t *,
		import_export_element_ext_t *, element_status_page_t *,
		int *);
void	   *lib_mode_sense(library_t *, uchar_t, uchar_t *, int);
static int	populate_drives(library_t *library, char *drv_tbl);

/*	Globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 *	init_elements - get status for all elements in the library.
 *
 * exit -
 */
int				/* 0 = all ok !0 = failure */
init_elements(
		library_t *library)
{
	uint16_t	count, start_element;
	uint16_t	avail_drives;
	int		i, err, conlevel = 5;
	size_t	  retry;
	dev_ent_t	*un;
	char	   *drv_tbl;
	mode_sense_t   *mode_sense;
	drive_state_t  *drive;
	xport_state_t  *xport;
	iport_state_t  *import;
	robot_ms_page1d_t *pg1d = NULL; /* Element Address Assignment */
	robot_ms_page1e_t *pg1e = NULL; /* Transport Geometry Parameters */
	robot_ms_page1f_t *pg1f = NULL; /* Device Capabilities */
	sam_extended_sense_t *sense;

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);

	/* Put mode sense data into shared memory. */

	/* LINTED pointer cast may result in improper alignment */
	mode_sense = (mode_sense_t *)SHM_REF_ADDR(un->mode_sense);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	SANITY_CHECK(mode_sense != (mode_sense_t *)0);
	SANITY_CHECK(sense != (sam_extended_sense_t *)0);
	(void) memset(mode_sense, 0, sizeof (mode_sense_t));

	mutex_lock(&un->io_mutex);
	pg1d = (robot_ms_page1d_t *)lib_mode_sense(library, 0x1d,
	    (uchar_t *)& mode_sense->u.robot_ms.pg1d,
	    sizeof (robot_ms_page1d_t));
	pg1f = (robot_ms_page1f_t *)lib_mode_sense(library, 0x1f,
	    (uchar_t *)& mode_sense->u.robot_ms.pg1f,
	    sizeof (robot_ms_page1f_t));
	pg1e = (robot_ms_page1e_t *)lib_mode_sense(library, 0x1e,
	    (uchar_t *)& mode_sense->u.robot_ms.pg1e,
	    sizeof (robot_ms_page1e_t));
	mutex_unlock(&un->io_mutex);

	if (pg1d == NULL || pg1f == NULL || pg1e == NULL) {
		DevLog(DL_ERR(5115));
		return (1);
	}
	library->status.b.two_sided = pg1e->transport_sets[0].rotate;
	if (un->type == DT_CYGNET)
		library->status.b.two_sided = 0;

	/* Allocate the drive tables. */
	BE16toH(&pg1d->first_drive, &start_element);
	BE16toH(&pg1d->num_drive, &count);
	library->range.drives_lower = start_element;
	library->range.drives_count = count;
	library->range.drives_upper = start_element + count - 1;

	/*
	 * This code is currently applied to IBM3584 only since the IBM3584
	 * returns a valid status if drive unit is not installed in a
	 * library. ASC/ASCQ:0x82/0x00. May need to add other library types
	 * to this check, check scsi docs.
	 *
	 * If drive is not fully populated and there is an empty slot for the
	 * drive, we don't need to create a redundant drive_thread.
	 */
	avail_drives = count;
	drv_tbl = malloc_wait(count, 2, 0);
	(void) memset(drv_tbl, TRUE, count);
	if (DT_IBM3584 == un->type)
		if ((avail_drives =
		    (uint16_t)populate_drives(library, drv_tbl)) == 0) {
			/*
			 * No drives installed, assum fully populated.
			 */
			DevLog(DL_ERR(5361));
			avail_drives = count;
			(void) memset(drv_tbl, TRUE, count);
		} else if (avail_drives > count) {
			avail_drives = count;
		}
	DevLog(DL_DETAIL(5362), avail_drives);

	/* one for the drive, one for stage and one for the stage helper */
	conlevel += (avail_drives * 3);

	library->drive = (drive_state_t *)malloc_wait(
	    sizeof (drive_state_t), 5, 0);
	library->index = library->drive;
	(void) memset(library->drive, 0, sizeof (drive_state_t));

	/*
	 * For each drive, build the drive state structure, put the init
	 * request on the list and start a thread with a new lwp.
	 */
	for (drive = library->drive, i = 0;
	    i < (int)count && avail_drives > 0; i++) {

		if (drv_tbl[i] == FALSE) {
			continue;
		}
		/* assign element number */
		drive->element = start_element + i;
		drive->library = library;
		/* hold the lock until ready */
		mutex_lock(&drive->mutex);
		drive->new_slot = ROBOT_NO_SLOT;
		drive->open_fd = -1;
		drive->active_count = 1;
		drive->first = (robo_event_t *)malloc_wait(
		    sizeof (robo_event_t), 5, 0);
		(void) memset(drive->first, 0, sizeof (robo_event_t));
		drive->first->type = EVENT_TYPE_INTERNAL;
		drive->first->status.bits = REST_FREEMEM;
		drive->first->request.internal.command = ROBOT_INTRL_INIT;
		if (thr_create(NULL, MD_THR_STK, &drive_thread, (void *) drive,
		    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
		    &drive->thread)) {
			DevLog(DL_SYSERR(5116));
			drive->status.b.offline = TRUE;
			drive->thread = (thread_t)- 1;
		}
		if (--avail_drives <= 0) {
			break;
		} else {
			/* Allocate next entry */
			drive->next = (drive_state_t *)malloc_wait(
			    sizeof (drive_state_t), 5, 0);
			(void) memset(drive->next, 0, sizeof (drive_state_t));
			drive->next->previous = drive;	/* set back link */
			drive = drive->next;
		}
	}

	drive->next = NULL;	/* no next drive */
	library->drive->previous = NULL;	/* no previous drive */
	free(drv_tbl);

	/* Allocate transport tables */

	BE16toH(&pg1d->first_tport, &start_element);
	BE16toH(&pg1d->num_tport, &count);
	library->range.transport_lower = start_element;
	library->range.transport_count = count;
	library->range.transport_upper = start_element + count - 1;
	library->range.default_transport = 0;
	library->page1f = pg1f;
	conlevel += count;
	library->transports =
	    (xport_state_t *)malloc_wait(sizeof (xport_state_t), 5, 0);
	(void) memset(library->transports, 0, sizeof (xport_state_t));

	for (xport = library->transports, i = 0; i < (int)count; i++) {
		/* assign element number */
		xport->element = start_element + i;
		xport->library = library;

		mutex_lock(&xport->mutex);
		/* start only one transport thread */
		if (i == 0) {
			xport->first =
			    (robo_event_t *)malloc_wait(
			    sizeof (robo_event_t), 5, 0);
			(void) memset(xport->first, 0, sizeof (robo_event_t));
			xport->first->type = EVENT_TYPE_INTERNAL;
			xport->first->status.bits = REST_FREEMEM;
			xport->first->request.internal.command =
			    ROBOT_INTRL_INIT;
			xport->active_count = 1;

			if (thr_create(NULL, SM_THR_STK,
			    &transport_thread, (void *) xport,
			    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
			    &xport->thread)) {
				DevLog(DL_SYSERR(5117));
				xport->thread = (thread_t)- 1;
			}
		}
		/* Allocate next entry */
		if (i != (count - 1)) {
			xport->next = (xport_state_t *)malloc_wait(
			    sizeof (xport_state_t), 5, 0);
			(void) memset(xport->next, 0, sizeof (xport_state_t));
			xport->next->previous = xport;	/* set back link */
			xport = xport->next;
		}
	}

	/* for the metrum d-360 the last transport is used with import export */
	xport->next = NULL;	/* no next transport */
	library->transports->previous = NULL;

	/* Allocate mailbox (import/export) tables */

	BE16toH(&pg1d->first_mail, &start_element);
	BE16toH(&pg1d->num_mail, &count);
	library->range.ie_lower = start_element;
	library->range.ie_count = count;
	if (count != 0)
		library->range.ie_upper = start_element + count - 1;
	else
		library->range.ie_upper = 0;

	conlevel += 1;		/* only one import/export thread */
	library->import = (iport_state_t *)malloc_wait(
	    sizeof (iport_state_t), 5, 0);
	(void) memset(library->import, 0, sizeof (iport_state_t));

	/* store the transport used in import/export for the metrum D-360 */
	if (un->type == DT_METD28)
		library->import->xport = xport;

	for (import = library->import, i = 0; i < (int)count; i++) {
		SANITY_CHECK(import != (iport_state_t *)0);
		/* assign element number */
		import->element = start_element + i;
		import->library = library;

		mutex_lock(&import->mutex);
		/* Create only one mailbox thread */
		if (i == 0) {
			import->active_count = 1;
			import->first = (robo_event_t *)malloc_wait(
			    sizeof (robo_event_t), 5, 0);
			(void) memset(import->first, 0, sizeof (robo_event_t));
			import->first->type = EVENT_TYPE_INTERNAL;
			import->first->status.bits = REST_FREEMEM;
			import->first->request.internal.command =
			    ROBOT_INTRL_INIT;
			if (thr_create(NULL, SM_THR_STK,
			    &import_thread, (void *) import,
			    (THR_DETACHED | THR_BOUND | THR_NEW_LWP),
			    &import->thread)) {
				DevLog(DL_SYSERR(5118));
				import->thread = (thread_t)- 1;
			}
		}
		if (i != (count - 1)) {	/* Allocate next entry */
			import->next = (iport_state_t *)malloc_wait(
			    sizeof (iport_state_t), 5, 0);
			(void) memset(import->next, 0, sizeof (iport_state_t));
			/* set back link */
			import->next->previous = import;
			import = import->next;
		}
	}

	import->next = NULL;	/* no next mailbox */
	SANITY_CHECK(library->import != (iport_state_t *)0);
	library->import->previous = NULL;

	/* allocate the audit table if needed */

	BE16toH(&pg1d->first_stor, &start_element);
	BE16toH(&pg1d->num_stor, &count);
	library->range.storage_lower = start_element;
	library->range.storage_count = count;
	library->range.storage_upper = start_element + count - 1;

	/* add for the import/export door slots */
	if (un->type == DT_ACL452)
		count += library->range.ie_count;

	DevLog(DL_DETAIL(5220), library->range.drives_count,
	    library->range.transport_count, library->range.storage_count,
	    library->range.ie_count);

	if (thr_setconcurrency(conlevel)) {
		DevLog(DL_SYSERR(5058));
	}
	/*
	 * If the audit table is the wrong length (based on the number of
	 * storage elements returned by mode-sense) or the audit bit is set,
	 * the set up for an audit.
	 */
	if ((library->audit_tab_len == 0) || un->status.b.audit) {
		int		added_more_time = FALSE;
		char	   *l_mess = un->dis_mes[DIS_MES_NORM];

		/*
		 * Audit table does not exist or is the wrong length.  This
		 * is generally a bad thing and  will force an initialize
		 * element scsi command and an audit. Both of these take a
		 * long time.
		 */
		/* tell the outside world */
		un->status.b.audit = TRUE;
		memccpy(l_mess, catgets(catfd, SET, 9022,
		    "initializing elements"),
		    '\0', DIS_MES_LEN);

		mutex_lock(&un->io_mutex);
		retry = 2;
		do {
			/*
			 * Allow 16 seconds for each storage element and 30
			 * seconds of slop.
			 */
			(void) memset(sense, 0, sizeof (sam_extended_sense_t));
			if ((err = scsi_cmd(library->open_fd, un,
			    SCMD_INIT_ELEMENT_STATUS,
			    (count << 4) + 30)) < 0) {
			TAPEALERT_SKEY(library->open_fd, un);
			GENERIC_SCSI_ERROR_PROCESSING(un,
			    library->scsi_err_tab, 0,
			    err, added_more_time, retry,
				/* code for DOWN_EQU */
			    down_library(library, SAM_STATE_CHANGE);
				mutex_unlock(&un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				/* code for ILLREQ */
				    mutex_unlock(&un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				    /* MACRO for cstyle */;
		/* MACRO for cstyle */)
			} else
				break;
		} while (--retry > 0);
		mutex_unlock(&un->io_mutex);

		if (retry <= 0) {
			DevLog(DL_ERR(5214));
			return (-1);
		}
		*l_mess = '\0';	/* clear the "initializing elements" message */
	}
	/*
	 * The adic can be really brain dead when it comes up.	Since it does
	 * not take much time, initialize all elements at every start up.
	 *
	 * The Exabyte X80 does not scan for bar codes on power up. The only way
	 * to get barcode information on a READ_ELEMENT_STATUS is to force an
	 * INIT_ELEMENT_STATUS.	This seemed the logical place to do this.
	 */
	else if (un->equ_type == DT_ADIC448 ||
	    un->equ_type == DT_EXBX80 ||
	    un->equ_type == DT_DOCSTOR ||
	    un->equ_type == DT_PLASMON_G) {
		int		added_more_time = FALSE;

		char	   *l_mess = un->dis_mes[DIS_MES_NORM];

		memccpy(l_mess, catgets(catfd, SET, 9022,
		    "initializing elements"),
		    '\0', DIS_MES_LEN);

		mutex_lock(&un->io_mutex);
		retry = 2;
		do {
			/*
			 * Allow 16 seconds for each storage element and 30
			 * seconds of slop.
			 */
			(void) memset(sense, 0, sizeof (sam_extended_sense_t));
			if ((err = scsi_cmd(library->open_fd, un,
			    SCMD_INIT_ELEMENT_STATUS,
			    (count << 4) + 30)) < 0) {
				TAPEALERT_SKEY(library->open_fd, un);
				GENERIC_SCSI_ERROR_PROCESSING(un,
				    library->scsi_err_tab, 0,
				    err, added_more_time, retry,
				/* code for DOWN_EQU */
				    down_library(library, SAM_STATE_CHANGE);
				mutex_unlock(&un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				/* code for ILLREQ */
				    mutex_unlock(&un->io_mutex);
				return (-1);
				/* MACRO for cstyle */,
				/* More codes */
				    /* MACRO cstyle */;
		/* MACRO for cstyle */)
			} else
				break;
		} while (--retry > 0);
		mutex_unlock(&un->io_mutex);

		if (retry <= 0) {
			DevLog(DL_ERR(5215));
			return (-1);
		}
		*l_mess = '\0';	/* clear the "initializing elements" message */
	}
	return (0);
}


/*
 *	update_element_status - update element status in local tables
 *
 * entry -
 *	  library - library_t *
 *	  type - element type to update
 *
 * exit -
 *	  0 = ok
 *	  1 = failure
 */
int
update_element_status(
			library_t *library,
			const int type)
{
	dev_ent_t	*un;
	uint16_t	count, ele_dest_len;
	int		buff_size, i, num_eles;
	char	   *buffer;
	uint_t	  current_element, last_element;
	xport_state_t  *transport;
	iport_state_t  *import;
	drive_state_t  *drive;
	element_status_page_t *status_page;
	element_status_data_t *status_data;

	/*
	 * buffer size = number of elements * (size of structure) plus (size
	 * of element status data) plus (size of element status page)
	 */

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);

	switch (type) {
	case TRANSPORT_ELEMENT:
		{
			transport_element_t *transport_descrip;

	redo_transport:
			buff_size =
			    (library->range.transport_count *
			    library->ele_dest_len) +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t) + 50;

			buffer = (char *)malloc_wait(buff_size, 2, 0);

			if (read_element_status(library, type,
			    library->range.transport_lower,
			    library->range.transport_count,
			    buffer, buff_size) < 0) {
				free(buffer);
				DevLog(DL_ERR(5059));
				return (1);
			}
			status_data = (element_status_data_t *)buffer;
			BE16toH(&status_data->numb_elements, &count);
			if (count != library->range.transport_count)
				DevLog(DL_ERR(5060),
				    library->range.transport_count, count);

			status_page = (element_status_page_t *)
			    (buffer + sizeof (element_status_data_t));

			/*
			 * Check the size of the element and if bigger than
			 * what we have set asside, change the size free the
			 * old buffer and try again.
			 */
			BE16toH(&status_page->ele_dest_len, &ele_dest_len);
			if (ele_dest_len > library->ele_dest_len) {
				library->ele_dest_len = ele_dest_len;
				free(buffer);
				goto redo_transport;
			}
			transport_descrip = (transport_element_t *)
			    ((char *)status_page +
			    sizeof (element_status_page_t));

			if (status_page->type_code != TRANSPORT_ELEMENT) {
				/* not correct element page */
				DevLog(DL_ERR(5061), status_page->type_code);
				free(buffer);
				return (1);
			}
			transport = library->transports;
			while (transport != NULL && count--) {
				transport_element_ext_t *extension;

				if (ele_dest_len > sizeof (transport_element_t))
					extension = (transport_element_ext_t *)
					    ((char *)transport_descrip +
					    sizeof (transport_element_t));
				else
					extension = NULL;

				copy_xport_status(transport, transport_descrip,
				    extension, status_page);
				transport = transport->next;
				transport_descrip = (transport_element_t *)
				    ((char *)transport_descrip + ele_dest_len);
			}
			free(buffer);
			break;
		}

	case STORAGE_ELEMENT:
		{
			storage_element_t *storage_descrip;
			storage_element_ext_t *extension;

			last_element = 0;
			current_element = library->range.storage_lower;

			while (current_element <=
			    library->range.storage_upper) {
				if ((num_eles = library->range.storage_count -
				    last_element) >
				    MAX_STORE_STATUS)
					num_eles = MAX_STORE_STATUS;

		redo_storage:
				buff_size = num_eles * library->ele_dest_len +
				    sizeof (element_status_data_t) +
				    sizeof (element_status_page_t) + 50;

				buffer = malloc_wait(buff_size, 2, 0);
				if (read_element_status(library, type,
				    current_element, num_eles,
				    buffer, buff_size) < 0) {
					DevLog(DL_ERR(5062));
					free(buffer);
					return (1);
				}
				status_data = (element_status_data_t *)buffer;
				BE16toH(&status_data->numb_elements, &count);

				if (count != num_eles) {
					DevLog(DL_ERR(5063), count, num_eles);
					num_eles = count;
				}
				status_page = (element_status_page_t *)
				    (buffer + sizeof (element_status_data_t));

				/*
				 * Check the size of the element and if
				 * bigger than what we have set asside,
				 * change the size free the old buffer and
				 * try again.
				 */
				BE16toH(&status_page->ele_dest_len,
				    &ele_dest_len);
				if (ele_dest_len > library->ele_dest_len) {
					library->ele_dest_len = ele_dest_len;
					free(buffer);
					goto redo_storage;
				}
				last_element += num_eles;

				storage_descrip = (storage_element_t *)
				    ((char *)status_page +
				    sizeof (element_status_page_t));

				if (status_page->type_code != STORAGE_ELEMENT) {
					/* not correct element page */
					DevLog(DL_ERR(5064),
					    status_page->type_code);
					free(buffer);
					return (1);
				}
				for (i = 0; i < num_eles;
				    i++, storage_descrip = (storage_element_t *)
				    ((char *)storage_descrip + ele_dest_len)) {
					struct VolId    vid;
					ushort_t	ele_addr;
					uint32_t	status;

					if (ele_dest_len >
					    sizeof (storage_element_t))
						extension =
						    (storage_element_ext_t *)
						    ((char *)storage_descrip +
						    sizeof (storage_element_t));
					else
						extension = NULL;

					BE16toH(&storage_descrip->ele_addr,
					    &ele_addr);

					status = 0;
					vid.ViFlags = VI_cart;
					vid.ViEq = library->un->eq;
					vid.ViSlot = SLOT_NUMBER(
					    library, ele_addr);
					vid.ViPart = 0;

					/*
					 * A Plasmon G library can have both
					 * mo and udo drives and mo and udo
					 * disks. If the media type on the
					 * storage element descriptor is
					 * unknown, set the media type from
					 * the library.
					 */
					if (storage_descrip->full &&
					    library->un->equ_type ==
					    DT_PLASMON_G) {

						int	media_offset;
						int media_type;

						if (status_page->PVol &&
						    status_page->AVol) {
							media_offset = 88;
						} else if (status_page->PVol &&
						    !status_page->AVol) {
							media_offset = 52;
						} else {
							media_offset = 16;
						}
			media_type = ((uint8_t *)storage_descrip)
			    [media_offset];

					if (media_type ==
					    PLASMON_MT_UNKNOWN) {
						sam_syslog(LOG_DEBUG,
						    "storage element 0x%4.4x,"
						    "media type unknown\n",
						    ele_addr);
					}
					sam_syslog(LOG_DEBUG,
					    "storage element 0x%4.4x, "
					    "media type 0x%2.2x\n",
					    ele_addr, media_type);

				if (media_type ==
				    PLASMON_MT_UDO) {
					memmove(vid.ViMtype,
					    sam_mediatoa(DT_PLASMON_UDO),
					    sizeof (vid.ViMtype));
					if (library->un->media !=
					    DT_PLASMON_UDO) {
						status |= CES_bad_media;
					}
				} else if (media_type ==
				    PLASMON_MT_MO) {
					memmove(vid.ViMtype,
					    sam_mediatoa(DT_ERASABLE),
					    sizeof (vid.ViMtype));
					if (library->un->media !=
					    DT_ERASABLE) {
						status |= CES_bad_media;
					}
				}
				} else {
					memmove(vid.ViMtype,
					    sam_mediatoa(library->un->media),
					    sizeof (vid.ViMtype));
					}

					if (ele_addr == current_element) {

						if (!storage_descrip->access) {
							status |= CES_unavail;
							/*
							 * If Plasmon DVD-RAM
							 * library, the tray
							 * for this storage
							 * element may be in
							 * a drive so we
							 * don't want to flag
							 * slot as
							 * unavailable.
							 */
				/* cstyle indentation */
				if (library->un->equ_type == DT_PLASMON_D ||
				    library->un->equ_type ==
				    DT_PLASMON_G) {
				drive_state_t *drive = library->drive;
				/*
				 * Check for media.
				 */
					while (drive != NULL) {
					/*
					 * Storage element
					 * in drive. Clear
					 * unavailable.
					 */
					if (drive->media_element == ele_addr) {
						status &= ~CES_unavail;
						break;
					}
					drive = drive->next;
					}
				}
				}
				/*
				 * The metrum library (D-28)
				 * returns a 83,2 for code if
				 * the 7 slot mag is not
				 * installed. This is not
				 * documented in the book.
				 */
				if (storage_descrip->except &&
				    extension != NULL) {
					switch (library->un->equ_type) {
					case DT_METD28:
					if (extension->add_sense_code == 0x83 &&
					    extension->add_sense_qual == 0x02)
						status |= CES_unavail;
					break;

					case DT_METD360:
					if (extension->add_sense_code != 0) {
						library->status.b.except = TRUE;
						DevLog(DL_ERR(5065),
						    extension->add_sense_code,
						    extension->add_sense_qual);
						return (0);
					}
					break;

					case DT_3570C:
						/*
						 * if the
						 * magazine
						 * is not
						 * available
						 */
					if (extension->add_sense_code == 0x3b &&
					    extension->add_sense_qual == 0x11)
						status |= CES_unavail;
					break;

					default:
						break;
					}
				}
				if (storage_descrip->full) {
				/*
				 * if occupied, it's
				 * inuse
				 */
					status |= CES_inuse | CES_occupied;
					if (status_page->PVol &&
					    extension != NULL) {
						dtb(&(extension->PVolTag[0]),
						    BARCODE_LEN);
					if (is_barcode(extension->PVolTag)) {
						status |= CES_bar_code;
					}
					if (status_page->AVol &&
					    (extension->AVolTag != '\0')) {
						dtb(&(extension->AVolTag[0]),
						    BARCODE_LEN);
					}
					}
					}
					}
				if ((storage_descrip->full) &&
				    (extension != NULL) &&
				    (status_page->PVol) &&
				    (extension->PVolTag != '\0')) {
					(void) CatalogSlotInit(&vid, status,
					    (library->status.b.two_sided) ?
					    2 : 0,
					    (char *)extension->PVolTag,
					    (char *)extension->AVolTag);
				} else {
					(void) CatalogSlotInit(&vid,
					    status,
					    (library->status.b.two_sided) ?
					    2 : 0, "", "");
				}
				if ((library->un->dt.rb.status.b.barcodes) &&
				    (status & CES_inuse)) {
					struct CatalogEntry ced;
					struct CatalogEntry *ce = &ced;
					int		set_default = 1;

						/*
						 * If there is a catalog
						 * entry, and the entry's
						 * capacity is zero, and not
						 * set to zero by the user,
						 * set the capacity to
						 * default.
						 *
						 * Cleaning tape capacity should
						 * be zero. This code will
						 * prevent a call to the
						 * catserver if cleaning tape
						 * capacity is zero, or set
						 * it to zero if not.
						 */
					if ((ce = CatalogGetEntry(
					    &vid, &ced)) != NULL) {
						if ((ce->CeStatus &
						    CES_capacity_set) ||
						    (ce->CeCapacity &&
						    !(ce->CeStatus &
						    CES_cleaning)) ||
						    (!ce->CeCapacity &&
						    (ce->CeStatus &
						    CES_cleaning))) {
							set_default = 0;
						}
					}

		/* indentation due to cstyle */
			if (set_default) {
				int		capacity;

				if (!(ce->CeStatus & CES_cleaning)) {
					capacity = DEFLT_CAPC(
					    library->un->media);
				} else {
					capacity = 0;
				}
				if (library->status.b.two_sided) {
					vid.ViPart = 1;
				}
				(void) CatalogSetFieldByLoc(library->un->eq,
				    vid.ViSlot, vid.ViPart, CEF_Capacity,
				    capacity, 0);
				(void) CatalogSetFieldByLoc(library->un->eq,
				    vid.ViSlot, vid.ViPart, CEF_Space,
				    capacity, 0);
				if (library->status.b.two_sided) {
					vid.ViPart = 2;
					(void) CatalogSetFieldByLoc(
					    library->un->eq,
					    vid.ViSlot, vid.ViPart,
					    CEF_Capacity,
					    capacity, 0);
					(void) CatalogSetFieldByLoc(
					    library->un->eq,
					    vid.ViSlot, vid.ViPart,
					    CEF_Space,
					    capacity, 0);
				}
			}
				}
				current_element++;
			}
			free(buffer);
		}

			break;
		}

	case IMPORT_EXPORT_ELEMENT:
		{
			import_export_element_t *import_descrip;

			/* there may not be a mailbox */
			if (library->range.ie_count > 0) {
				int		slot;
		redo_import:
				if (library->un->type == DT_ACL452) {
					slot = SLOT_NUMBER(library,
					    library->range.storage_count);
				} else {
					slot = ROBOT_NO_SLOT;
				}

				buff_size =
				    (library->range.ie_count *
				    library->ele_dest_len) +
				    sizeof (element_status_data_t) +
				    sizeof (element_status_page_t) + 50;

				buffer = malloc_wait(buff_size, 2, 0);
				if (read_element_status(library,
				    type, library->range.ie_lower,
				    library->range.ie_count,
				    buffer, buff_size) < 0) {
					DevLog(DL_ERR(5066));
					free(buffer);
					return (1);
				}
				status_data = (element_status_data_t *)buffer;
				BE16toH(&status_data->numb_elements, &count);
				if (count != library->range.ie_count) {
					DevLog(DL_DEBUG(5124), count,
					    library->range.ie_count);
				}
				status_page = (element_status_page_t *)
				    (buffer + sizeof (element_status_data_t));
				BE16toH(&status_page->ele_dest_len,
				    &ele_dest_len);
				if (ele_dest_len > library->ele_dest_len) {
					library->ele_dest_len = ele_dest_len;
					free(buffer);
					goto redo_import;
				}
				import_descrip = (import_export_element_t *)
				    ((char *)status_page +
				    sizeof (element_status_page_t));

				if (status_page->type_code !=
				    IMPORT_EXPORT_ELEMENT) {
					/* not correct element page */
					DevLog(DL_ERR(5067),
					    status_page->type_code);
					free(buffer);
					return (1);
				}
				import = library->import;

				while (import != NULL && count--) {
					import_export_element_ext_t *extension;
					uint16_t	stor_addr;

				if (ele_dest_len >
				    sizeof (import_export_element_t)) {
					extension =
					    (import_export_element_ext_t *)
					    ((char *)import_descrip +
					    sizeof (import_export_element_t *));
					BE16toH(&extension->stor_addr,
					    &stor_addr);
					slot = SLOT_NUMBER(library, stor_addr);
				} else {
					extension = NULL;
					slot = ROBOT_NO_SLOT;
				}

				copy_import_status(import,
				    import_descrip, extension,
				    status_page, &slot);
				import = import->next;
				import_descrip = (import_export_element_t *)
				    ((char *)import_descrip + ele_dest_len);
				}

				free(buffer);
			}
			break;
		}

	case DATA_TRANSFER_ELEMENT:
		{
			data_transfer_element_t *drive_descrip;

	redo_transfer:
			buff_size =
			    (library->range.drives_count *
			    library->ele_dest_len) +
			    sizeof (element_status_data_t) +
			    sizeof (element_status_page_t) + 50;

			buffer = malloc_wait(buff_size, 2, 0);
			if (read_element_status(library, type,
			    library->range.drives_lower,
			    library->range.drives_count,
			    buffer, buff_size) < 0) {
				DevLog(DL_ERR(5068));
				free(buffer);
				return (1);
			}
			status_data = (element_status_data_t *)buffer;
			BE16toH(&status_data->numb_elements, &count);
			if (count != library->range.drives_count) {
				DevLog(DL_ERR(5125), count,
				    library->range.drives_count);
			}
			status_page = (element_status_page_t *)
			    (buffer + sizeof (element_status_data_t));

			/*
			 * check the size of the element and if bigger than
			 * what we have set asside, change the size free the
			 * old buffer and try again.
			 */
			BE16toH(&status_page->ele_dest_len, &ele_dest_len);
			if (ele_dest_len > library->ele_dest_len) {
				library->ele_dest_len = ele_dest_len;
				free(buffer);
				goto redo_transfer;
			}
			drive_descrip = (data_transfer_element_t *)
			    ((char *)status_page +
			    sizeof (element_status_page_t));

			if (status_page->type_code != DATA_TRANSFER_ELEMENT) {
				/* not correct element page */
				DevLog(DL_ERR(5069), status_page->type_code);
				free(buffer);
				return (1);
			}
			drive = library->drive;

			while (drive != NULL && count--) {
				int		nodrv;
				data_transfer_element_ext_t *extension;

				if (ele_dest_len >
				    sizeof (data_transfer_element_t))
					extension =
					    (data_transfer_element_ext_t *)
					    ((char *)drive_descrip +
					    sizeof (data_transfer_element_t));
				else
					extension = NULL;

				if (drive_descrip->except &&
				    extension != NULL) {
					DevLog(DL_DETAIL(5363),
					    extension->add_sense_code,
					    extension->add_sense_qual);
				}
				nodrv = FALSE;
				switch (library->un->type) {
				case DT_IBM3584:
					if (drive_descrip->except &&
					    extension != NULL &&
					    extension->add_sense_code == 0x82 &&
					    extension->add_sense_qual == 0)
						nodrv = TRUE;
					break;

				default:
					break;
				}

				if (nodrv == FALSE) {
					copy_drive_status(drive, drive_descrip,
					    extension,
					    status_page);
					drive = drive->next;
				}
				drive_descrip = (data_transfer_element_t *)
				    ((char *)drive_descrip + ele_dest_len);
			}

			free(buffer);
			break;
		}

	default:
		return (1);
	}

	return (0);
}


/*
 *	lib_mode_sense - get a mode sense page for a media changer.
 *
 * io_mutex held on entry and exit
 */
void	   *
lib_mode_sense(
		library_t *library,
		uchar_t pg_code,
		uchar_t *buffp,
		int buff_size)
{
	int		resid;
	int		retry_count;
	void	   *retval = NULL;
	int		scsi_error = FALSE;
	enum sam_scsi_action scsi_action;
	sam_extended_sense_t *sense;
	uchar_t	*tmp_buffp;
	dev_ent_t	*un = library->un;
	int		xfer_size;

	/*
	 * We can't read the specified page directly into the caller's buffer
	 * because the MODE_SENSE command always prepends a header to the
	 * page. Malloc a temporary buffer to hold specified page and the
	 * mode sense header.
	 */
	xfer_size = buff_size + sizeof (robot_ms_hdr_t);
	tmp_buffp = malloc_wait(xfer_size, 2, 0);
	memset(tmp_buffp, 0, xfer_size);

	/* Issue the MODE_SENSE command */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	for (retry_count = 0; retry_count < 5; retry_count++) {
		(void) memset(sense, 0, sizeof (sam_extended_sense_t));
		if (scsi_cmd(library->open_fd, un, SCMD_MODE_SENSE,
		    30, tmp_buffp, xfer_size, pg_code, &resid) < 0) {
			sam_extended_sense_t *sense =
			    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

			TAPEALERT_SKEY(library->open_fd, un);

			/*
			 * If we cannot read a mode page, just return to
			 * caller without it.
			 */
			if (sense->es_add_code == 0x24 &&
			    sense->es_qual_code == 0x00 &&
			    (((uchar_t *)sense)[15] & 0x40) &&
			    ((uchar_t *)sense)[16] == 0 &&
			    ((uchar_t *)sense)[17] == 2) {
				free(tmp_buffp);
				return (NULL);
			}
			scsi_action = process_scsi_error(un,
			    library->scsi_err_tab, 0);
			if (scsi_action == LONG_WAIT_LOG) {
				sleep(WAIT_TIME_FOR_LONG_WAIT_LOG);
			} else if (scsi_action = WAIT_READY_LONG) {
				sleep(2 * WAIT_TIME_FOR_READY);
			} else if (scsi_action = WAIT_READY) {
				sleep(WAIT_TIME_FOR_READY);
			} else if (scsi_action = DOWN_EQU) {
				down_library(library, SAM_STATE_CHANGE);
				scsi_error = TRUE;
				break;
			} else {
				scsi_error = TRUE;
				break;
			}
		} else {
			break;
		}
	}

	/*
	 * We probably got some MODE_SENSE data. Do the following sanity
	 * checks to verify we got valid data: 1) Verify we didn't exceed the
	 * retry max 2) Verify there was no scsi error 3) Verify we got ALL
	 * of the possible MODE_SENSE data 4) Verify we got the correct page.
	 * If everything verified ok, copy the page to the user's buffer.
	 */
	if (retry_count >= 5) {
		DevLog(DL_ERR(5217));
	} else if (scsi_error == TRUE) {
		DevLog(DL_ERR(5221), pg_code);
	} else if (((robot_ms_hdr_t *)tmp_buffp)->sense_data_len > xfer_size) {
		DevLog(DL_ERR(5070));
		DevLog(DL_ERR(1068),
		    ((robot_ms_hdr_t *)tmp_buffp)->sense_data_len,
		    xfer_size,
		    pg_code);
	} else if ((*(tmp_buffp + sizeof (robot_ms_hdr_t)) & 0x3f) != pg_code) {
		DevLog(DL_ERR(5221), pg_code);
	} else {
		memcpy(buffp, tmp_buffp + sizeof (robot_ms_hdr_t), buff_size);
		retval = (void *) buffp;
	}
	free(tmp_buffp);
	return (retval);
}


/*
 * populate_drives - populate installed drives
 *
 * entry -
 *    library - library_t *
 *    drv_tbl - drive map table to be filled
 *
 * exit -
 *    returns # of installed drives
 */
int
populate_drives(
		library_t *library,
		char *drv_tbl)
{
	dev_ent_t	*un;
	uint16_t	count, ele_dest_len;
	int		buff_size, i, drives;
	char	   *buffer;
	element_status_page_t *status_page;
	element_status_data_t *status_data;
	data_transfer_element_t *drive_descrip;

	/*
	 * buffer size = number of elements * (size of structure) plus (size
	 * of element status data) plus (size of element status page)
	 */

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);

redo_transfer:
	buff_size =
	    (library->range.drives_count * library->ele_dest_len) +
	    sizeof (element_status_data_t) +
	    sizeof (element_status_page_t) + 50;

	buffer = malloc_wait(buff_size, 2, 0);

	if (read_element_status(library, DATA_TRANSFER_ELEMENT,
	    library->range.drives_lower,
	    library->range.drives_count,
	    buffer, buff_size) < 0) {
		DevLog(DL_ERR(5068));
		free(buffer);
		return (0);
	}
	status_data = (element_status_data_t *)buffer;
	BE16toH(&status_data->numb_elements, &count);
	if (count != library->range.drives_count)
		DevLog(DL_ERR(5125), count, library->range.drives_count);

	status_page = (element_status_page_t *)
	    (buffer + sizeof (element_status_data_t));

	/*
	 * Check the size of the element and if bigger than what we have set
	 * asside, change the size free the old buffer and try again.
	 */
	BE16toH(&status_page->ele_dest_len, &ele_dest_len);
	if (ele_dest_len > library->ele_dest_len) {
		library->ele_dest_len = ele_dest_len;
		free(buffer);

		goto redo_transfer;

	}
	drive_descrip = (data_transfer_element_t *)
	    ((char *)status_page + sizeof (element_status_page_t));

	if (status_page->type_code != DATA_TRANSFER_ELEMENT) {
		/* not correct element page */
		DevLog(DL_ERR(5069), status_page->type_code);
		free(buffer);
		return (0);
	}
	drives = 0;

	/*
	 * Skip if no extension returned.
	 */
	if (ele_dest_len > sizeof (data_transfer_element_t)) {
		data_transfer_element_ext_t *extension;

		for (i = 0; i < library->range.drives_count && i < count; i++) {
			extension = (data_transfer_element_ext_t *)
			    ((char *)drive_descrip +
			    sizeof (data_transfer_element_t));

			switch (un->type) {
			case DT_IBM3584:	/* IBM3584 */
				/*
				 * If except = 1, ASC = 0x82, ASCQ = 0x00,
				 * drive is not installed. See IBM3584 SCSI
				 * manual.
				 */
				if (drive_descrip->except &&
				    extension->add_sense_code == 0x82 &&
				    extension->add_sense_qual == 0) {
					DevLog(DL_DETAIL(5363),
					    extension->add_sense_code,
					    extension->add_sense_qual);
					drv_tbl[i] = FALSE;
				} else {
					drives++;
				}
				break;

			default:
				drives++;
			}

			drive_descrip = (data_transfer_element_t *)
			    ((char *)drive_descrip + ele_dest_len);
		}
	}
	free(buffer);
	return (drives);
}
