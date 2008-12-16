/*
 * misc.c - misc routines.
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

#include <thread.h>
#include <synch.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/defaults.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/historian.h"
#include "sam/nl_samfs.h"
#include "../generic/generic.h"
#include "aml/trace.h"
#include "aml/proto.h"
#include "driver/samst_def.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.24 $"


/* globals */
extern shm_alloc_t master_shm, preview_shm;


/*
 * common_init - everyone calls this at startup right after disabeling
 * signals
 */
void
common_init(
	dev_ent_t *un)
{
	ld_devices(un->fseq);
}


/*
 * free_allocated - free memory allocated to a library.
 *
 * entry -
 *    library - library_t *
 *
 */
void
free_allocated(
	library_t *library)
{
	drive_state_t   *drive;		/* pointer to first drive states */
	xport_state_t   *transports;	/* pointer to first transport */
	iport_state_t   *import;	/* pointer to import/export status */
	void *tmp;

	drive = library->drive;		/* free drive tables */

	while (drive != NULL) {
		if (drive->thread >= 0)
			thr_kill(drive->thread, SIGINT);
		tmp = drive->next;
		free(drive);
		drive = (drive_state_t *)tmp;
	}
	library->drive = NULL;

	transports = library->transports;	/* free transport tables */
	while (transports != NULL) {
		if (transports->thread >= 0)
			thr_kill(transports->thread, SIGINT);
		tmp = transports->next;
		free(transports);
		transports = (xport_state_t *)tmp;
	}
	library->transports = NULL;

	import = library->import;		/* free import/export tables */
	while (import != NULL) {
		if (import->thread >= 0)
			thr_kill(import->thread, SIGINT);
		tmp = import->next;
		free(import);
		import = (iport_state_t *)tmp;
	}
	library->import = NULL;

	return;

}


/*
 * disp_of_event - dispose of event
 *
 * clean up event
 */
void
disp_of_event(
	library_t *library,
	robo_event_t *event,
	int completion)
{
	char    tos;
	char   *ent_pnt = "disp_of_event";

	/* send condition signal if requested. */
	if (event->status.b.sig_cond) {
		if (event->status.b.free_mem && DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "%s: signal and free.", ent_pnt);

		ETRACE((LOG_NOTICE, "EV:DsEv:%#x:Signal", event));

		mutex_lock(&event->mutex);

		/* if not changed, then it must be ok */
		if (event->completion == REQUEST_NOT_COMPLETE)
			event->completion = completion;
		else
			write_event_exit(event, completion, NULL);

		cond_signal(&event->condit);	/* signal the waiting thread */
		mutex_unlock(&event->mutex);
		return;			   /* can't free signaled event */
	}

	/* ensure an exit response is sent */
	write_event_exit(event, completion, NULL);

	/* disp_of_event no longer puts stuff back on the free list */
	if (event->status.b.free_mem && ((void *)event > (void *)&tos)) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_INFO, "%s: free in stack %#x, %#x.",
			    ent_pnt, event, &tos);
		return;
	}

	/*
	 * If the event was malloc'ed and the caller wants us to free it
	 * destroy mutex and condition before freeing the memory
	 */
	if (event->status.b.free_mem) {
		mutex_destroy(&event->mutex);
		cond_destroy(&event->condit);
		free(event);
		ETRACE((LOG_NOTICE, "DsEv %#x Free", event));
	}
}


/*
 * clear_driver_idle
 *
 * clears the driver idle flag if needed.
 */
void
clear_driver_idle(
	drive_state_t *drive,
	int open_fd)
{
	int i = 0;

	if (IS_OPTICAL(drive->un) && (open_fd >= 0))
		ioctl(open_fd, SAMSTIOC_IDLE, &i);
}


/*
 * set_driver_idle
 *
 * sets the driver idle flag if needed.
 */
int
set_driver_idle(
	drive_state_t *drive,
	int open_fd)
{
	int i = 1;

	if (IS_OPTICAL(drive->un) && (open_fd >= 0))
		ioctl(open_fd, SAMSTIOC_IDLE, &i);
	return (i);
}


/*
 * add the event to the end of the link list.
 */
void
add_to_end(
	library_t *library,
	robo_event_t *current)
{

	mutex_lock(&library->list_mutex);

	if (library->active_count == 0) {
		ETRACE((LOG_NOTICE, "LbAf %#x %#x", current, current->timeout));
		current->next = current->previous = NULL;
		library->first = current;
	} else {
		register robo_event_t *end;

		LISTEND(library, end);
		ETRACE((LOG_NOTICE,
		    "LbAe e %#x c %#x t %#x", end, current, current->timeout));
		(void) append_list(end, current);
	}

	library->active_count++;
	cond_signal(&library->list_condit);
	mutex_unlock(&library->list_mutex);
}


/*
 * is_barcode - is the barcode good
 *
 * return TRUE if the barcode looks good, return FALSE is it matches
 * known "bad" values.
 */
int
is_barcode(
	void *bar_code)
{
	int	len;

	if (memcmp(bar_code, METRUM_BC_UNREAD, METRUM_BC_SIZE) &&
	    memcmp(bar_code, METRUM_BC_NOCHK, METRUM_BC_SIZE) &&
	    memcmp(bar_code, METRUM_BC_NOBARC, METRUM_BC_SIZE) &&
	    ((len = (int)strlen(bar_code)) > 0)) {
		char *tmp = (char *)bar_code;

		for (; *tmp != '\0'; tmp++)
			if (!isprint(*tmp))
				return (FALSE);
			return (TRUE);
	}

	return (FALSE);
}


/*
 * is_cleaning - check the barcode for cleaning tape.
 *
 * return TRUE if the barcode matches cleaning tape barcode
 * else return FALSE.
 */
int
is_cleaning(
	void *bar_code)
{
	register int  match;

	match = !memcmp(bar_code, CLEANING_BAR_CODE, CLEANING_BAR_LEN) ||
	    !memcmp(bar_code, CLEANING_FULL_CODE, CLEANING_FULL_LEN);
	return (match);
}


/*
 * clear_busy_bit - clear the busy bit in preview entry
 *
 */
void
clear_busy_bit(
	robo_event_t *event,
	drive_state_t *drive,
	const uint_t slot)
{
	char *ent_pnt = "clear_busy_bit";
	register preview_t  *preview;

	if (event->type == EVENT_TYPE_INTERNAL &&
	    event->request.internal.address != NULL) {

		preview = (preview_t *)(event->request.internal.address);

		/* Make sure its the same entry */
		if (preview->sequence == event->request.internal.sequence) {
			mutex_lock(&preview->p_mutex);
			if (preview->p_error) {
				mutex_unlock(&preview->p_mutex);
				remove_preview_ent(
				    preview, NULL, PREVIEW_NOTHING, 0);
			} else {
				preview->busy = FALSE;
				mutex_unlock(&preview->p_mutex);
			}
		} else if (DBG_LVL(SAM_DBG_DEBUG)) {
			sam_syslog(LOG_DEBUG,
			    "%s(%d): No match, (%#x, %#x), (%#x, %#x).",
			    ent_pnt, drive->un->eq, preview->slot, slot,
			    preview->robot_equ, drive->library->un->eq);
		}
	}
}


/*
 * set_media_default - set the media type to the default if needed
 */
void
set_media_default(
	media_t *media)
{
	media_t			m_class;
	sam_defaults_t 	*defaults;

	defaults = GetDefaults();

	if (*media != 0) {
		m_class = (*media & DT_CLASS_MASK);
		if ((*media & DT_MEDIA_MASK) == 0) {
			if (m_class == DT_TAPE)
				*media = defaults->tape;
			else if (m_class == DT_OPTICAL)
				*media = defaults->optical;
		}
	}
}


/*
 * set_catalog_tape_media - set the media type from the first active drive
 */
void
set_catalog_tape_media(
	library_t *library)
{
	char   		*ent_pnt = "set_catalog_tape_media";
	dev_ent_t 	*un;

	/*
	 * Set catalog media type to the media type of the
	 * first device found belonging to the this family set
	 */
	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next))
		if (un->fseq == library->eq && !IS_ROBOT(un) &&
		    un->state < DEV_IDLE) {
			library->un->media = un->equ_type;
			break;
		}

	if (un == NULL) {
		library->un->media = DT_UNKNOWN;
	}

	if (un == NULL && DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "%s: Unable to determine media type.",
		    ent_pnt);
	}
}

/*
 * using_st_driver - verify the drives in a library use the st driver
 */

int
using_supported_tape_driver(
	library_t *library)
{
	struct	stat	statbuf;
	int		dev_major;
	FILE		*fd;
	char		driver[20] = "";
	char		readbuf[80];
	char		*path;
	drive_state_t	*drive;

	/*
	 * Get the solaris tapedriver major device number
	 */
	if ((fd = fopen(NAME_TO_MAJOR, "r")) == NULL) {
		sam_syslog(LOG_ERR, "Can't open %s: %s",
		    NAME_TO_MAJOR, strerror(errno));
		return (-1);
	}

	while (1) {
		readbuf[0] = '\0';
		if (fgets(readbuf, sizeof (readbuf), fd) == NULL) {
			sam_syslog(LOG_ERR,
			    "Can't find a driver for for drives in library %d",
			    library->un->eq);
			fclose(fd);
			return (-1);
		}
		if (readbuf[0] == '\0') {
			continue;
		}
		if (sscanf(readbuf, "%s %d", driver, &dev_major) != 2) {
			sam_syslog(LOG_ERR, "Invalid record format in %s",
			    NAME_TO_MAJOR);
			fclose(fd);
			return (-1);
		}
		if (strcmp(driver, SUPPORTED_TAPE_DRIVER) == 0) {
			fclose(fd);
			break;
		}
	}

	/*
	 * Check the major device number of each drive to see if
	 * the drive belongs to the solaris tape driver.
	 */
	for (drive = library->drive; drive != NULL; drive = drive->next) {
		path = library->drive->un->name;
		if (stat(path, &statbuf)) {
			sam_syslog(LOG_ERR,
			    "Can't stat %s: %s", path, strerror(errno));
			return (-1);
		}
		if (dev_major != major(statbuf.st_rdev)) {
			sam_syslog(LOG_ERR,
			    "device %s does not use the %s driver", path,
			    SUPPORTED_TAPE_DRIVER);
			return (-1);
		}
	}
	/*
	 * All drives use the solaris tape driver
	 */
	return (0);
}
