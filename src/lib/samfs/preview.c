/*
 * preview.c -  common routines to preview table manipulations.
 *
 * Notes:
 *
 * 1) Nothing goes into the preview table with the generic device type of
 * DT_TAPE or DT_OPTICAL.  Before placing an entry in the table, the type
 * will be checked and if generic, it will be changed to the site default as
 * set in the shared memory segment defaults structure.
 *
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

#pragma ident "$Revision: 1.42 $"

static char    *_SrcFile = __FILE__;

#include <thread.h>
#include <synch.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/file.h>
#include <sys/types.h>
#include <values.h>
#include <stdlib.h>
#include <stdio.h>

#include "aml/shm.h"
#include "sam/names.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/fsd.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/preview.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"

/* function prototypes */

static void
preview_callback(sam_handle_t *, sam_resource_t *, dev_ent_t *,
    const int, enum callback);
static preview_t *find_entry(struct dev_ent *, int);

static int
handle_third_party(sam_handle_t *, sam_resource_t *, enum callback);

static int
add_preview_entry(sam_handle_t *, sam_resource_t *, float, enum callback);

static void prv_age_priority();
static void set_fswm_priority(int prv_id);


/* some globals */


extern shm_alloc_t master_shm, preview_shm;

/*
 * find_entry.
 *
 * Find a mounted vsn in the preview table.  If its there, return the address of
 * the entry.  Always returns old-est entry.
 *
 * returns - preview_t * to entry, NULL if none. busy bit in entry is set.
 */

static preview_t *
find_entry(dev_ent_t *un, int media)
{
	register preview_t	*preview, *old_preview = (preview_t *)NULL;
	preview_tbl_t		*preview_tbl;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	if (preview_tbl->ptbl_count > 0) {	/* if any entries */
		int    i, hits;
		float    highest = -MAXFLOAT;

		for (i = 0, hits = 0, preview = preview_tbl->p;
		    (i < preview_tbl->avail) &&
		    (hits < preview_tbl->ptbl_count); i++, preview++) {
			/*
			 * Since the device entry mutex (un->mutex) is being
			 * held, dont wait for the preview mutex, this could
			 * cause a deadlock.
			 */
			if (mutex_trylock(&preview->p_mutex))
				continue;
			/*
			 * If preview is in use and not busy or
			 * canceled(error)
			 */
			if ((preview->in_use ? ++hits : FALSE) &&
			    !preview->busy && !preview->p_error) {
				/*
				 * Check for vsn/media match, no write
				 * conflict and priority compared to another
				 * match
				 */
				if ((strcmp(preview->resource.archive.vsn,
				    un->vsn) == 0) &&
				    (preview->resource.archive.
				    rm_info.media == media) &&
				    !(preview->write & un->status.b.wr_lock) &&
				    (highest < preview->priority)) {
					/*
					 * All media are single access, don't
					 * start another
					 */
					if (un->active && preview->callback
					    == CB_NOTIFY_FS_LOAD) {
						mutex_unlock(&preview->p_mutex);
						continue;
					}
					/* If found another, release it */
					if (old_preview != NULL)
						mutex_unlock(
						    &old_preview->p_mutex);

					old_preview = preview;
					highest = preview->priority;
				} else
					/*
					 * Not same vsn/media, write conflict
					 * or lower priority than the last
					 * one
					 */
					mutex_unlock(&preview->p_mutex);
			} else	/* Its busy or not in use */
				mutex_unlock(&preview->p_mutex);
		}
	}
	if (old_preview != NULL) {
		old_preview->busy = TRUE;
		mutex_unlock(&old_preview->p_mutex);
	}
	return (old_preview);
}

/*
 * check_for_vsn.
 *
 * check if the specified vsn in the preview table.  Quick and dirty, uses no
 * mutex.
 *
 * returns - 1 - it's in there 0 - not there
 */

int
check_for_vsn(
	char *vsn,
	int media)
{
	register preview_t *preview, *pre_end;
	register preview_tbl_t *preview_tbl;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	preview = preview_tbl->p;
	pre_end = &preview_tbl->p[(preview_tbl->avail) - 1];
	if (preview_tbl->ptbl_count > 0)	/* if any entries */
		for (; preview <= pre_end; preview++)
			if (preview->in_use &&
			    (preview->resource.archive.rm_info.media
			    == media) &&
			    (strcmp(preview->resource.archive.vsn, vsn) == 0))
				return (1);
	return (0);
}

/*
 * check_preview - looks for preview entries waiting for the vsn mounted on
 * the specified device.  If found, the preview entry is removed and whoever
 * is waiting is notified.
 *
 */

void
check_preview(dev_ent_t *un)
{
	register preview_t *preview;

	if (un->status.b.unload)
		return;

	while ((preview = find_entry(un, un->type)) != (preview_t *)NULL) {
		un->mtime = time(NULL) + un->delay;
		remove_preview_ent(preview, un, PREVIEW_NOTIFY, 0);
	}
}

/*
 * check_preview_status - looks for a preview entry waiting for the vsn
 * mounted on the specified device.  If found, the preview entry is removed
 * and whoever is waiting is notified.  Returns 1 if an entry was found else
 * returns 0.
 *
 */

int
check_preview_status(dev_ent_t *un)
{
	register preview_t *preview;

	if (un->status.b.unload)
		return (0);

	if ((preview = find_entry(un, un->type)) != (preview_t *)NULL) {
		un->mtime = time(NULL) + un->delay;
		remove_preview_ent(preview, un, PREVIEW_NOTIFY, 0);
		return (1);
	}
	return (0);
}

/*
 * remove_preview_ent deletes an entry from the preview table. Busy bit
 * should be set.
 *
 * This routine may be called with a dummy preview entry (an entry that is not
 * within the preview table).  This is to have a common file-system callback.
 *
 */

void
remove_preview_ent(
	preview_t *preview,	/* entry to remove */
	dev_ent_t *un,		/* device with mounted media */
	int flags,		/* flags */
	int err)
{
	int		errflg = err;
	char		*ent_pnt = "remove_preview_ent";
	preview_tbl_t	*preview_tbl;

	if (preview == (preview_t *)0) {
		/*
		 * If entry was set for removal, but we got here, entry would
		 * stick in preview forever
		 */
		sam_syslog(LOG_CRIT,
		    "remove_preview_ent() called with NULL preview");
#ifdef DEBUG
		abort();
#endif
		return;
	}
	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	switch (flags) {
	case PREVIEW_NOTIFY:	/* do notification using errflg */
		if (un)		/* set fs_active if needed */
			un->status.bits |= DVST_FS_ACTIVE;
		preview_callback(&preview->handle, &preview->resource, un,
		    errflg, preview->callback);
		mutex_lock(&preview->p_mutex);
		if (preview->count > 1) {
			preview_t new_preview;
			register uint_t next, last;
			register stage_preview_t *s_preview;

			new_preview.resource = preview->resource;
			next = preview->stages;
			mutex_lock(&preview_tbl->ptbl_mutex);
			while (preview->count > 1) {
				if (DBG_LVL(SAM_DBG_DEBUG) && (next == 0)) {
					sam_syslog(LOG_DEBUG,
					    "%s:stage link table:%s:%d.\n",
					    ent_pnt, __FILE__, __LINE__);
					break;
				}
				s_preview =
				    (stage_preview_t *)PRE_REF_ADDR(next);
				new_preview.resource.archive.rm_info
				    = s_preview->rm_info;
				new_preview.handle = s_preview->handle;
#if defined(CB_STAGE_FILE)
				new_preview.callback = CB_STAGE_FILE;
#endif
				preview_callback(&new_preview.handle,
				    &new_preview.resource, un,
				    errflg, new_preview.callback);
				preview->count--;
				last = next;
				preview->stages = s_preview->next;
				next = s_preview->next;
				(void) memset(s_preview, 0,
				    sizeof (stage_preview_t));
				s_preview->next = preview_tbl->stages;
				preview_tbl->stages = last;
			}
			mutex_unlock(&preview_tbl->ptbl_mutex);
		}
		mutex_unlock(&preview->p_mutex);
		/* FALLTHRU */

	case PREVIEW_NOTHING:	/* no notify needed */
		/* if in preview table */
		if ((preview >= preview_tbl->p) &&
		    (preview < &preview_tbl->p[preview_tbl->avail])) {

			mutex_lock(&preview->p_mutex);
			(void) memset(&preview->handle, 0,
			    sizeof (preview_t) - (uint_t) &
			    (((preview_t *)0)->handle));
			mutex_unlock(&preview->p_mutex);
			mutex_lock(&preview_tbl->ptbl_mutex);
			if (preview_tbl->ptbl_count > 0)
				preview_tbl->ptbl_count--;
			mutex_unlock(&preview_tbl->ptbl_mutex);

		} else	/* clear dummy preview */
			(void) memset(&preview->handle, 0,
			    sizeof (preview_t) - (uint_t) &
			    (((preview_t *)0)->handle));

		break;


	default:
		/* Code integrity is broken  */
		sam_syslog(LOG_CRIT, "%s: bad flags %#x\n", ent_pnt, flags);
	}
}

/*
 * add_preview_cmd - add an entry to the preview table where the request
 * originated on the command fifo.  Runs as a thread.
 *
 * entry - pointer to a command struct
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure to free it
 * before thr_exit.
 *
 */

void *
add_preview_cmd(
	void *vcommand)
{
	int		exit_status = 0;
	media_t		m_class;
	sam_cmd_fifo_t	*command = vcommand;
	sam_defaults_t	*defaults;
	sam_resource_t	resource;

	defaults = GetDefaults();

	m_class = command->media & DT_CLASS_MASK;

	/* only allow optical and tape in the preview */
	if ((m_class == DT_TAPE) || (m_class == DT_OPTICAL)) {
		/*
		 * if media type not specified, then set it to the site
		 * default
		 */
		if ((command->media & DT_MEDIA_MASK) == 0) {
			if (m_class == DT_TAPE)
				command->media = defaults->tape;
			else
				command->media = defaults->optical;
		}
		memset(&resource, '\0', sizeof (resource));
		resource.archive.rm_info.media = command->media;
		strncpy(resource.archive.vsn, command->vsn, sizeof (vsn_t) - 1);
		exit_status = add_preview_entry(NULL, &resource,
		    PRV_CMD_PRIO_DEFAULT, CB_NO_CALLBACK);
	}
	free(command);
	thr_exit(&exit_status);
	/* NOTREACHED */
	return ((void *) NULL);
}



/*
 * add_preview_fs - add an entry to the preview table where the request
 * originated on the file system fifo.
 *
 * exit - 0 - ok !0 - error
 */

int
add_preview_fs(
	sam_handle_t *handle,		/* handle for file sys callback */
	sam_resource_t *resource,	/* resource record */
	float priority,			/* request's base priority */
	enum callback callback)		/* call back function */
{
	int			have_dev = FALSE;
	media_t			m_class;
	shm_ptr_tbl_t		*shm_ptr_tbl;
	sam_defaults_t		*defaults;
	register dev_ent_t	*un;

	if (is_third_party(resource->archive.rm_info.media))
		return (handle_third_party(handle, resource, callback));

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	/*
	 * If this is a remote sam client, make it look like all device types
	 * (media types) are covered.
	 *
	 * LDK.  Would be better to have a shm resident table of all media types
	 * covered by remote sam.
	 */
	if (shm_ptr_tbl->flags.bits & SHM_FLAGS_RMTSAMCLNT)
		have_dev = TRUE;

	defaults = GetDefaults();

	/*
	 * if the resource record specifies generic media, then change it to
	 * the site default.
	 */
	m_class = (resource->archive.rm_info.media & DT_CLASS_MASK);
	if ((resource->archive.rm_info.media & DT_MEDIA_MASK) == 0) {
		if (m_class == DT_TAPE)
			resource->archive.rm_info.media = defaults->tape;
		else if (m_class == DT_OPTICAL)
			resource->archive.rm_info.media = defaults->optical;
	}
	un = (dev_ent_t *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);

	/* See if the media is already mounted */
	for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		if (!have_dev && ((IS_TAPE(un) || IS_OPTICAL(un)) &&
		    un->state < DEV_IDLE)) {

			if ((resource->archive.rm_info.media == un->equ_type) ||
			    (un->equ_type == DT_MULTIFUNCTION &&
			    (resource->archive.rm_info.media ==
			    DT_WORM_OPTICAL ||
			    resource->archive.rm_info.media ==
			    DT_ERASABLE ||
			    resource->archive.rm_info.media ==
			    DT_PLASMON_UDO)))
				have_dev = TRUE;
		}
		/*
		 * If it's tape or optical, see if the requested media is
		 * already mounted.
		 */
		/* Do quick vsn check without the mutex */

		if ((IS_TAPE(un) || IS_OPTICAL(un)) &&
		    (un->type == (resource->archive.rm_info.media)) &&
		    (un->status.b.labeled ||
		    (un->status.b.strange && un->status.b.write_protect) ||
		    (un->status.b.strange && callback ==
		    CB_NOTIFY_TP_LOAD)) &&
		    !(un->status.bits & (DVST_REQUESTED | DVST_UNLOAD)) &&
		    (strcmp(resource->archive.vsn, un->vsn) == 0)) {

			/* Wait for mutex, it should not be held long. */
			mutex_lock(&un->mutex);

			/*
			 * if ready and labeled (or strange) and not
			 * requested and vsn match and write mode does not
			 * conflict.
			 */
			if (un->state < DEV_IDLE && un->status.b.ready &&
			    (un->status.b.labeled ||
			    (un->status.b.strange &&
			    un->status.b.write_protect) ||
			    (un->status.b.strange &&
			    callback == CB_NOTIFY_TP_LOAD)) &&
			    !(un->status.bits &
			    (DVST_REQUESTED | DVST_UNLOAD)) &&
			    (strcmp(resource->archive.vsn, un->vsn) == 0) &&
			    (!(resource->access == FWRITE &&
			    un->status.b.wr_lock))) {
				int	err;

				/*
				 * Only start mounts (via todo) if nothing
				 * else is going on. Only start a stage if
				 * nothing is going on and it is not already
				 * staging.
				 */
				if (un->active &&
				    (callback == CB_NOTIFY_FS_LOAD ||
				    callback == CB_NOTIFY_TP_LOAD)) {

					mutex_unlock(&un->mutex);
					un = NULL;
					break;
				}
				/*
				 * the todo routines will unlock the
				 * un->mutex after incrementing the active
				 * count.
				 */
				if (un->fseq == 0)
					err = send_scanner_todo(TODO_ADD, un,
					    callback, handle, resource, NULL);
				else
					err = send_robot_todo(TODO_ADD,
					    un->fseq, un->eq,
					    callback, handle, resource, NULL);
				if (err)
					mutex_unlock(&un->mutex);
				return (err);
			}
			mutex_unlock(&un->mutex);
		}
	}

	/* device exist for this media? */
	if (!have_dev) {
		sam_syslog(LOG_ERR, catgets(catfd, SET, 9300,
		    "No device for media type: %s."),
		    dt_to_nm((int)resource->archive.rm_info.media));
		return (ESRCH);
	}
	/* Must put it in the table */
	return (add_preview_entry(handle, resource, priority, callback));
}


/*
 * scan_preview - check each entry in the preview and see if a vsn is in the
 * catalog. Func is called for each entry found passing preview_t *, mid,
 * *func_arg, 0 with the busy flag set. Func must not clear the busy bit.
 *
 * return - preview_t * - pointer to the oldest preview entry NULL if no matches
 * Before exit,  func is called with the chosen entry preview_t *, mid
 * *func_arg, 1 with the busy flag set. Func must not clear the busy bit.
 *
 */
preview_t *
scan_preview(
	int equ,			/* equip. ordinal * */
	struct CatalogEntry *ced,	/* Catalog Entry */
	const int flags,		/* flags (see sam_preview.h) */
	void *func_arg,			/* Third argument for func */
	int func())			/* Function to call if found. */
{
	int			i, hits;
	uint_t			tmpu, old_slot;
	float			highest = -MAXFLOAT;
	preview_t		*old_preview = NULL;
	preview_tbl_t		*preview_tbl;
	sam_defaults_t		*defaults;
	register preview_t	*preview;
	int			hold_mid;
	struct CatalogEntry	*ce;

	defaults = GetDefaults();

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	if (preview_tbl->ptbl_count <= 0)	/* if no entries */
		return ((preview_t *)NULL);

	for (i = 0, preview = preview_tbl->p, hits = 0;
	    i < defaults->previews && hits < preview_tbl->ptbl_count;
	    i++, preview++) {
		mutex_lock(&preview->p_mutex);
		if ((preview->in_use ? ++hits : FALSE) &&
		    !preview->busy && !preview->p_error) {
			tmpu = ROBOT_NO_SLOT;
			/* is this claimed by an AL yet */
			if (preview->robot_equ == 0) {
				ce = CatalogGetCeByMedia((char *)
				    sam_mediatoa(preview->
				    resource.archive.rm_info.media),
				    (char *)preview->resource.archive.vsn, ced);
				if (ce == NULL || ce->CeEq != equ) {
					mutex_unlock(&preview->p_mutex);
					continue;
				}
				/* historian is calling */
				if (flags & PREVIEW_HISTORIAN)
					hold_mid = ce->CeMid;
				else {
					hold_mid = ce->CeMid;
					if (hold_mid != ROBOT_NO_SLOT &&
					    (ce->CeStatus & CES_unavail))
						hold_mid = ROBOT_NO_SLOT;
				}
				if (hold_mid != ROBOT_NO_SLOT) {
					tmpu = ce->CeSlot;
				} else {
					tmpu = ROBOT_NO_SLOT;
				}
			} else {
				ce = CatalogGetCeByMedia((char *)
				    sam_mediatoa(preview->
				    resource.archive.rm_info.media),
				    (char *)preview->resource.archive.vsn, ced);
				if (ce == NULL ||
				    ce->CeEq != preview->robot_equ) {

					mutex_unlock(&preview->p_mutex);
					continue;
				}
				hold_mid = ce->CeMid;
				if (hold_mid != ROBOT_NO_SLOT) {
					tmpu = ce->CeSlot;
				} else {
					tmpu = ROBOT_NO_SLOT;
				}
			}

			if (tmpu == ROBOT_NO_SLOT) {
				mutex_unlock(&preview->p_mutex);
				/* not in this robot, go to the next */
				continue;
			} else {
				/*
				 * If its a cleaning tape or the vsn has
				 * changed
				 */

				if ((ce->CeStatus & CES_cleaning) ||
				    (strcmp((char *)preview->
				    resource.archive.vsn, ce->CeVsn)) != 0 ||
				    preview->resource.archive.rm_info.media
				    != sam_atomedia(ce->CeMtype)) {
					preview->slot = ROBOT_NO_SLOT;
					preview->robot_equ = 0;
					preview->element = 0;
					mutex_unlock(&preview->p_mutex);
					continue;
				}
				/* flag it busy and release */
				preview->busy = TRUE;
				mutex_unlock(&preview->p_mutex);
			}

			/*
			 * Call the supplied function to do what its gotta do
			 * for any entry found in the preview thats in this
			 * catalog. Function returns 0 if vsn is in the
			 * library, 1 if not in this library, -1 if preview
			 * entry has been removed.
			 */

			if (func != NULL && (flags & PREVIEW_SET_FOUND)) {
				int	rc;
				if ((rc = func(preview, ce, func_arg,
				    tmpu, 0)) != 0) {

					if (rc != -1) {
						/*
						 * preview entry has not been
						 * already removed
						 */
						mutex_lock(&preview->p_mutex);
						preview->busy = FALSE;
						mutex_unlock(&preview->p_mutex);
					}
					continue;
				}
			}
			if ((highest < preview->priority) &&
			    (!preview->flip_side) &&
			    (strlen(preview->resource.archive.vsn) != 0)) {

				/* This one has higher priority */
				if (old_preview != NULL) {
					mutex_lock(&old_preview->p_mutex);
					old_preview->busy = FALSE;
					mutex_unlock(&old_preview->p_mutex);
				}
				old_preview = preview;
				highest = preview->priority;
				old_slot = tmpu;
			} else {
				mutex_lock(&preview->p_mutex);
				preview->busy = FALSE;
				mutex_unlock(&preview->p_mutex);
			}
		} else
			mutex_unlock(&preview->p_mutex);
	}

	if (old_preview != (preview_t *)NULL) {
		/*
		 * Get the catalog entry again. It may be that the last entry
		 * on the preview queue was not for this library but *ce will
		 * still be pointing to it.
		 */
		ce = CatalogGetCeByMedia((char *)sam_mediatoa(
		    old_preview->resource.archive.rm_info.media),
		    (char *)old_preview->resource.archive.vsn, ced);
		if (ce == NULL) {
			old_preview = NULL;
		}
		/* call the function with the entry about to be returned */
		if (func != NULL) {
			/*
			 * Shouldn't happen at this point but best to be
			 * prepared.
			 */
			func(old_preview, ce, func_arg, old_slot, 1);
		}
	}
	return (old_preview);
}

/*
 * remove_preview_handle - remove entries from preview if they are waiting
 * for the handle in the fifo_command.  Used by cancel command processing.
 *
 */
void
remove_preview_handle(
	sam_fs_fifo_t *command,		/* file system cancel command */
	enum callback callback)		/* call back function */
{
	int		i;
	char		*ent_pnt = "remove_preview_handle";
	preview_t	*preview;
	sam_handle_t	*handle = &command->handle;
	preview_tbl_t	*preview_tbl;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	if (DBG_LVL(SAM_DBG_CANCEL))
		sam_syslog(LOG_DEBUG, "%s:(%#x) entered.",
		    ent_pnt, handle->id.ino);

	for (i = 0, preview = preview_tbl->p;
	    i < preview_tbl->avail && preview_tbl->ptbl_count > 0;
	    i++, preview++) {

		mutex_lock(&preview->p_mutex);

		/* If empty preview or its been canceled */
		if (!preview->in_use || (preview->in_use && preview->p_error)) {
			mutex_unlock(&preview->p_mutex);
			continue;
		}
		switch (callback) {
		case CB_NOTIFY_FS_LOAD:
			if (handles_match(&preview->handle, handle)) {
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "%s: index %d, vsn %s\n", ent_pnt,
					    i, preview->resource.archive.vsn);
				preview_callback(&preview->handle,
				    &preview->resource, (dev_ent_t *)NULL,
				    ECANCELED, callback);
				preview->p_error = TRUE;
				if (preview->busy)
					mutex_unlock(&preview->p_mutex);
				else {
					preview->busy = TRUE;
					mutex_unlock(&preview->p_mutex);
					remove_preview_ent(preview, NULL,
					    PREVIEW_NOTHING, 0);
				}
				return;
			}
			mutex_unlock(&preview->p_mutex);
			continue;

		default:
			mutex_unlock(&preview->p_mutex);
			continue;

		}
	}

	/*
	 * Not in preview.  Look for a device with the media mounted and send
	 * a cancel to the device/robot.
	 */
	{
		dev_ent_t	*un;
		sam_resource_t	*resource;

		resource = &command->param.fs_cancel.resource;
		un = (dev_ent_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
#if defined(DEBUG)
		if (DBG_LVL(SAM_DBG_CANCEL))
			sam_syslog(LOG_DEBUG, "%s:(%#x): Cancel !preview.",
			    ent_pnt, handle->id.ino);
#endif
		if (resource->archive.vsn[0] == '\0' &&
		    !is_third_party(resource->archive.rm_info.media)) {
#if defined(DEBUG)
			if (DBG_LVL(SAM_DBG_CANCEL))
				sam_syslog(LOG_DEBUG,
				    "%s:(%#x): Vsn not supplied.",
				    ent_pnt, handle->id.ino);
#endif
			return;
		}
		for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
			/*
			 * If third party match the device media type and
			 * send todo
			 */
			if (is_third_party(resource->archive.rm_info.media)) {
				if (un->type ==
				    resource->archive.rm_info.media) {

					mutex_lock(&un->mutex);
					if (send_tp_todo(TODO_CANCEL, un,
					    handle, resource, callback))
						mutex_unlock(&un->mutex);
					return;
				}
				continue;
			}
			/*
			 * If it's tape or optical, see if the requested
			 * media is already mounted.
			 */
			if ((IS_TAPE(un) || IS_OPTICAL(un) || IS_RSD(un)) &&
			    (un->type == resource->archive.rm_info.media)) {
				mutex_lock(&un->mutex);
				/*
				 * if ready and labeled and not requested and
				 * vsn match and write mode does not
				 * conflict.
				 */
				if (un->state < DEV_IDLE &&
				    un->status.b.ready &&
				    (un->status.b.labeled || IS_RSD(un)) &&
				    (strcmp(resource->archive.vsn, un->vsn)
				    == 0)) {

					int err = 0;
					/*
					 * the todo routines will unlock the
					 * un->mutex after incrementing the
					 * active count.
					 */
					if (un->fseq == 0)
						err = send_scanner_todo(
						    TODO_CANCEL, un,
						    callback, handle,
						    resource, NULL);
					else if (IS_RSD(un))
						send_sp_todo(TODO_CANCEL, un,
						    handle, resource,
						    callback);
					else
						err = send_robot_todo(
						    TODO_CANCEL, un->fseq,
						    un->eq, callback,
						    handle, resource, NULL);
					if (err)
						mutex_unlock(&un->mutex);
					return;
				}
				mutex_unlock(&un->mutex);
			}
		}
	}
}

/*
 * preview_callback - invoke callback routine when removing preview entries.
 */
static void
preview_callback(
	sam_handle_t *handle,
	sam_resource_t *resource,
	dev_ent_t *un,
	int errflg,
	enum callback callback)
{
	switch (callback) {
		case CB_NO_CALLBACK:
		break;

	case CB_NOTIFY_TP_LOAD:
		notify_tp_mount(handle, un, errflg);
		break;

	case CB_NOTIFY_FS_LOAD:
		notify_fs_mount(handle, resource, un, errflg);
		break;

	default:
		break;
	}
}


/*
 * delete_preview_cmd - deletes an entry to the preview table where the
 * request originated on the command fifo.  Runs as a thread.
 *
 * entry - pointer to a command struct
 *
 * Programming note:  The parameter passed in was malloc'ed, be sure to free it
 * before thr_exit.
 *
 */
void *
delete_preview_cmd(
	void *vcommand)
{
	int		i, exit_status = 0;
	preview_t	*preview;
	sam_archive_t	*archive;
	preview_tbl_t	*preview_tbl;
	sam_cmd_fifo_t	*command = vcommand;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	if (command->slot >= 0 && command->slot < preview_tbl->avail) {
		preview = &preview_tbl->p[command->slot];

		mutex_lock(&preview->p_mutex);
		if (preview->in_use && !preview->busy) {
			preview->busy = TRUE;
			mutex_unlock(&preview->p_mutex);
			remove_preview_ent(preview, NULL,
			    PREVIEW_NOTIFY, ECANCELED);
		} else {
			preview->p_error = TRUE;
			mutex_unlock(&preview->p_mutex);
		}
		free(command);
		thr_exit(&exit_status);
		return ((void *) NULL);
	} else {
		if (preview_tbl->ptbl_count > 0) {	/* if any entries */
			preview = preview_tbl->p;
			for (i = 0; i < preview_tbl->avail; i++, preview++) {

				mutex_lock(&preview->p_mutex);

				if (preview->in_use && !preview->busy) {
					archive = &preview->resource.archive;
					if ((strcmp(archive->vsn, command->vsn)
					    == 0) &&
					    (archive->rm_info.media ==
					    command->media)) {

						if (!preview->busy) {
							preview->busy = TRUE;
							mutex_unlock(
							    &preview->
							    p_mutex);
							remove_preview_ent(
							    preview, NULL,
							    PREVIEW_NOTIFY,
							    ECANCELED);
						} else {
							preview->p_error = TRUE;
							mutex_unlock(
							    &preview->p_mutex);
						}
						free(command);
						thr_exit(&exit_status);
						return ((void *) NULL);
					}
				}
				mutex_unlock(&preview->p_mutex);
			}
		}
	}
	free(command);
	thr_exit(&exit_status);
	/* NOTREACHED */
}

/*
 * remove_stale_preview - remove entries in the preview table that are over
 * stale_time minutes old or no slot for the VSN. If stale_time is zero,
 * entries are never removed.
 */
/* ARGSUSED0 */
void *
remove_stale_preview(
	void *vparam)
{
	int		 i;
	time_t		now;
	preview_t	*preview;
	preview_tbl_t	*preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;
	sam_defaults_t	*defaults;
	dev_ptr_tbl_t	*dev_ptr_tbl = (dev_ptr_tbl_t *)
	    SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->dev_table);

	defaults = GetDefaults();
	now = time(NULL) - defaults->stale_time;
	/* if entries */
	if ((preview_tbl->ptbl_count > 0) && (defaults->stale_time > 0)) {
		preview = preview_tbl->p;
		for (i = 0; i < preview_tbl->avail; i++, preview++) {
			dev_ent_t *un = (dev_ent_t *)SHM_REF_ADDR
			    (dev_ptr_tbl->d_ent[preview->robot_equ]);

			mutex_lock(&preview->p_mutex);
			if ((preview->in_use && !preview->busy &&
			    (preview->ptime < now)) || (un && (IS_ROBOT(un)) &&
			    (un->state != DEV_ON))) {

				preview->busy = TRUE;
				mutex_unlock(&preview->p_mutex);
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 2096,
				    "Remove stale preview %s"),
				    preview->resource.archive.vsn);
				remove_preview_ent(preview, NULL,
				    PREVIEW_NOTIFY, ETIME);
			} else {
				mutex_unlock(&preview->p_mutex);
			}
		}
	}
	thr_exit(NULL);
	/* NOTREACHED */
	return ((void *) NULL);
}


/*
 * handle_third_party - Assign this to the third party device. exit - 0 - ok
 * !0 - error
 */
static int
handle_third_party(
	sam_handle_t *handle,
	sam_resource_t *resource,
	enum callback callback)
{
	char			*ent_pnt = "handle_third_party";
	register dev_ent_t	*un;

	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	/* find the device for this media type */
	for (; un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next))
		if (un->equ_type == resource->archive.rm_info.media)
			break;

	if (un == NULL) {
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "%s: Unknown device for media type z%c (%#x).",
			    ent_pnt,
			    *(((char *)&resource->archive.rm_info.media) +
			    1), resource->archive.rm_info.media);

		return (EINVAL);
	}
	mutex_lock(&un->mutex);
	if (send_tp_todo(TODO_ADD, un, handle, resource, callback))
		mutex_unlock(&un->mutex);
	return (0);
}


static int
add_preview_entry(
	sam_handle_t *handle,	/* handle for file sys callback */
	sam_resource_t *resource,	/* resource record */
	float priority,		/* request's base priority */
	enum callback callback)	/* call back function */
{
	int		i, exit_status = EAGAIN;
	uint_t		sequence;
	preview_tbl_t	*preview_tbl;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	sam_defaults_t	*defaults;
	static char	*notify_dir = SAM_SCRIPT_PATH "/load_notify.sh";
	static char	notify_args[128];
	preview_t	*preview;
	float		vsn_priority = 0;
	int		eq = 0;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	dev_ptr_tbl_t	*dev_tbl;	/* Device pointer table */
	dev_ent_t	*dev;

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;
	defaults = GetDefaults();
	dev_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);

	/*
	 * check if it should be added to the preview.  If it's set unavail
	 * or there's no operator and it's in a historian, return an error.
	 * If it's in a robot, preset the equipment number for the preview
	 * table for the benefit of samu etc. If it's in the historian, set
	 * the equipment number to 0 and notify the operator that a VSN must
	 * be loaded manually.
	 */

	ce = CatalogGetCeByMedia(sam_mediatoa(resource->archive.rm_info.media),
	    resource->archive.vsn, &ced);
	if (ce == NULL) {
		if (!(defaults->flags & DF_ATTENDED)) {
			return (ESRCH);
		}
	} else {
		eq = (int)ce->CeEq;
		dev = (dev_ent_t *)SHM_REF_ADDR(dev_tbl->d_ent[eq]);
		if (dev == NULL) {
			return (EINVAL);	/* log it? */
		}
		if ((ce->CeStatus & CES_unavail) ||
		    (ce->CeStatus & CES_dupvsn) ||
		    (!(defaults->flags & DF_ATTENDED) &&
		    dev->equ_type == DT_HISTORIAN)) {
			return (ESRCH);
		}
		if (dev->equ_type == DT_HISTORIAN) {
			eq = 0;	/* manually mounted */
			snprintf(notify_args, 128, "%s", resource->archive.vsn);
			(void) FsdNotify(notify_dir, LOG_NOTICE, 0,
			    notify_args);
			(void) SendCustMsg(HERE, 9400, resource->archive.vsn);
		}
	}

	mutex_lock(&preview_tbl->ptbl_mutex);
	if (preview_tbl->ptbl_count == preview_tbl->avail) {
		sam_syslog(LOG_ERR,
		    catgets(catfd, SET, 1957, "Preview table full."));
		mutex_unlock(&preview_tbl->ptbl_mutex);
		goto out;
	}
	preview_tbl->ptbl_count++;
	sequence = preview_tbl->sequence++;
	mutex_unlock(&preview_tbl->ptbl_mutex);

	preview = preview_tbl->p;
	for (i = 0; i < preview_tbl->avail; preview++, i++) {

		mutex_lock(&preview->p_mutex);

		if (!preview->in_use) {
			/*
			 * Get VSN priority from the catalog. For now VSN
			 * priority is just a bit with PRV_VSN_PRIORITY being
			 * applied to VSNs having priority bit set. Could be
			 * the field by itself in future implementation.
			 */
			if ((ce != NULL) &&
			    ((ce->CeStatus & CES_priority) != 0)) {
				vsn_priority = preview_tbl->prv_vsn_factor;
			}
#ifdef DEBUG_PREVIEW
			sam_syslog(LOG_DEBUG,
			    "Adding new entry %d: new count %d, seq %d,"
			    " prio %f vsnpri %f", preview->prv_id,
			    preview_tbl->ptbl_count, preview_tbl->sequence,
			    priority, vsn_priority);
#endif

			(void) memset(&preview->handle, 0, sizeof (preview_t) -
			    (uint_t)&(((preview_t *)0)->handle));
			preview->in_use = preview->busy = TRUE;
			if (handle)
				preview->handle = *handle;
			preview->callback = callback;
			preview->stage = FALSE;
			preview->write = (resource->access == FWRITE);
			preview->fs_req = TRUE;
			preview->slot = ROBOT_NO_SLOT;
			preview->robot_equ = eq;
			preview->sequence = sequence;
			preview->resource = *resource;
			(void) time(&preview->ptime);
			preview->count = 1;
			preview->stat_priority = priority + vsn_priority;
			set_fswm_priority(i);
			preview->busy = FALSE;
			mutex_unlock(&preview->p_mutex);

			prv_age_priority();

			exit_status = 0;
			break;
		}
		mutex_unlock(&preview->p_mutex);
	}

	if (i == preview_tbl->avail) {
		mutex_lock(&preview_tbl->ptbl_mutex);
		preview_tbl->ptbl_count--;
		mutex_unlock(&preview_tbl->ptbl_mutex);
		sam_syslog(LOG_ERR, "Preview table is inconsistent.\n");
		exit_status = EAGAIN;
	}
out:
	signal_preview();	/* send preview event to robots */
	if (shm_ptr_tbl->scanner > 0) {
		/* Signal the scanner */
		if (DBG_LVL(SAM_DBG_DEBUG))
			if (kill(shm_ptr_tbl->scanner, SIGALRM)) {
				sam_syslog(LOG_DEBUG,
				    "unable to signal scanner:%s:%d.\n",
				    __FILE__, __LINE__);
			} else
				(void) kill(shm_ptr_tbl->scanner, SIGALRM);
	}
	return (exit_status);

}


/*
 * prv_age_priority - recalculate priorities for all preview entries with new
 * age after adding new request
 */
static void
prv_age_priority()
{
	preview_tbl_t	*preview_tbl;
	preview_t	*preview;
	time_t		now = time(NULL);
	int		i;
	int		hits;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	/* Recalculate all priorities with new time */
	for (i = 0, hits = 0, preview = preview_tbl->p;
	    (i < preview_tbl->avail) && (hits < preview_tbl->ptbl_count);
	    i++, preview++) {

		mutex_lock(&preview->p_mutex);
		/* If preview is in use and not canceled(error) */
		if ((preview->in_use ? ++hits : FALSE) && !preview->p_error) {
		preview->priority = preview->stat_priority +
		    preview->fswm_priority + (now - preview->ptime) *
		    preview_tbl->prv_age_factor;

#ifdef DEBUG_PREVIEW
			sam_syslog(LOG_DEBUG,
			    "\tentry %d: s_prio %f, f_prio %f, prio %f",
			    preview->prv_id, preview->stat_priority,
			    preview->fswm_priority, preview->priority);
#endif
		}
		mutex_unlock(&preview->p_mutex);
	}
}


/*
 * prv_fswm_priority - recalculate priorities for requested filesystem based
 * on new filesystem water mark state
 */
void
prv_fswm_priority(equ_t fseq, fs_wmstate_e fswm_state)
{
	preview_tbl_t	*preview_tbl;
	preview_t	*preview;
	int		i, j;
	prv_fs_ent_t	*prv_fs_ent;
	int		hits;

	preview_tbl = &((shm_preview_tbl_t *)
	    (preview_shm.shared_memory))->preview_table;

	prv_fs_ent = (prv_fs_ent_t *)PRE_REF_ADDR(preview_tbl->prv_fs_table);

	/* Recalculate priorities for requested filesystem based */
	/* on new filesystem water mark state    */

	for (j = 0; j < preview_tbl->fs_count; j++, prv_fs_ent++) {
		if (prv_fs_ent->prv_fseq == fseq) {
			prv_fs_ent->prv_fswm_state = fswm_state;

			/* Find all preview requests for this filesystem */
			for (i = 0, hits = 0, preview = preview_tbl->p;
			    (i < preview_tbl->avail) &&
			    (hits < preview_tbl->ptbl_count); i++, preview++) {
				mutex_lock(&preview->p_mutex);

		/* N.B. Bad indentation here to meet cstyle requirements */

		/*
		 * If preview is in use and not
		 * canceled(error)
		 */
		if ((preview->in_use ? ++hits : FALSE) && !preview->p_error) {
			if ((preview->resource.access == FWRITE ||
			/*
			 * stage requested by archiver
			 * treated as archive request
			 */
			    (preview->resource.access == FREAD &&
			    preview->handle.flags.b.arstage)) &&
			    preview->handle.fseq == fseq) {
				if (preview->fswm_priority !=
				    prv_fs_ent->prv_fswm_factor[fswm_state]) {
					/*
					 * Replace
					 * fswm_priority with
					 * the new one
					 * leaving all other
					 * priorities intact
					 */
					preview->priority -=
					    preview->fswm_priority;
					preview->fswm_priority =
					    prv_fs_ent->
					    prv_fswm_factor[fswm_state];
					preview->priority +=
					    preview->fswm_priority;
				}
#ifdef DEBUG_PREVIEW
				sam_syslog(LOG_DEBUG,
				    "\tentry %d: f_prio %f, prio %f",
				    preview->prv_id, preview->fswm_priority,
				    preview->priority);
#endif
			}
		}
				mutex_unlock(&preview->p_mutex);
			}
			break;
		}
	}
}


static void
set_fswm_priority(int prv_id)
{
	preview_tbl_t	*preview_tbl;
	preview_t	*preview;
	int		j;
	prv_fs_ent_t	*prv_fs_ent;

	preview_tbl = &((shm_preview_tbl_t *)(preview_shm.shared_memory))->
	    preview_table;
	preview = &preview_tbl->p[prv_id];

	if (preview->resource.access == FWRITE ||
		/* stage requested by archiver treated as archive request */
	    (preview->resource.access == FREAD &&
	    preview->handle.flags.b.arstage)) {

		/* Search for fs in preview filesystem table */
		prv_fs_ent =
		    (prv_fs_ent_t *)PRE_REF_ADDR(preview_tbl->prv_fs_table);
		for (j = 0; j < preview_tbl->fs_count; j++, prv_fs_ent++)
			if (prv_fs_ent->prv_fseq == preview->handle.fseq) {
				preview->fswm_priority =
				    prv_fs_ent->prv_fswm_factor
				    [prv_fs_ent->prv_fswm_state];
				break;
			}
		if (j == preview_tbl->fs_count) {
			sam_syslog(LOG_ERR,
			    "Didn't find preview request's fseq %d in"
			    " preview fs table", preview->handle.fseq);
#ifdef DEBUG
			abort();
#endif
		}
	}
}
