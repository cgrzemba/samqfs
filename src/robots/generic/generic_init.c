/*
 *  generic_init.c - initialize the library and all its elements
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

#pragma ident "$Revision: 1.38 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#include <stdio.h>
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
#include <sys/syslog.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/shm.h"
#include "aml/robots.h"
#include "aml/tapes.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "generic.h"
#include "aml/proto.h"
#include "sam/lint.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/tapealert.h"

	/* DEV_ENT - given an equipment ordinal, return the dev_ent */
#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
	((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))

/*	function prototypes */
void	    reconcile_storage(library_t *);
void	   *unload_a_drive_thread(void *);
void	    convert_slot_to_ea(library_t *);
int		init_library_configuration(library_t *);

/*	globals */
extern shm_alloc_t master_shm, preview_shm;
extern void	set_catalog_tape_media(library_t *library);
extern void	set_operator_panel(library_t *library, const int lock);


/*
 *	generic_initialize - initialize the library
 *
 * exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
generic_initialize(
		library_t *library)
{
	int		i, retry, fd, mcf_drives;
	int		init_done;
	char	   *err_code_name = NULL;
	char	   *lc_mess;
	dev_ent_t	*device_entry, *un;
	struct stat	err_stat;
	sam_model_t    *model;
	drive_state_t  *drive, *last_drive;
	iport_state_t  *iport;

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);

	lc_mess = un->dis_mes[DIS_MES_CRIT];
	for (model = sam_model; model->short_name != NULL; model++) {
		if (model->dt == un->type) {
			err_code_name = (char *)malloc_wait(MAXPATHLEN, 2, 0);
			sprintf(err_code_name, "%s/errcodes/%s",
			    SAM_VARIABLE_PATH, model->short_name);
			break;
		}
	}

	if ((err_code_name != NULL) &&
	    (fd = open(err_code_name, O_RDONLY)) >= 0) {
		if (fstat(fd, &err_stat) >= 0) {
			library->scsi_err_tab =
			    malloc_wait(err_stat.st_size, 2, 0);
			if (read(fd, library->scsi_err_tab, err_stat.st_size) !=
			    err_stat.st_size) {
				free(library->scsi_err_tab);
				library->scsi_err_tab = NULL;
			}
			close(fd);
		}
	} else {
		DevLog(DL_ERR(5033), model->short_name);
		library->scsi_err_tab = NULL;
	}

	if (err_code_name != NULL)
		free(err_code_name);

	if ((library->open_fd = open(un->name, O_RDWR | O_NDELAY)) < 0) {
		char *MES_20010 = catgets(catfd, SET, 20010,
		    "unable to open %s");
		char	   *mess = (char *)malloc_wait(strlen(MES_20010) +
		    strlen(un->name) + 4, 2, 0);

		sprintf(mess, MES_20010, un->name);
		memccpy(lc_mess, mess, '\0', DIS_MES_LEN);
		free(mess);
		DevLog(DL_SYSERR(5181), un->name);
		return (1);
	}
	i = 0664;
	mutex_lock(&library->un->mutex);
	library->un->status.b.present = TRUE;
	library->un->status.b.requested = TRUE;
	library->helper_pid = (pid_t)- 1;
	mutex_unlock(&library->un->mutex);

	if (CatalogInit("generic") == -1) {
		sam_syslog(LOG_ERR, "%s", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}
	/* wait for unit ready */
	if ((init_done = wait_library_ready(library)) < 0)
		exit(1);

	if (library->un->state == DEV_OFF || library->un->state == DEV_DOWN)
		exit(1);

	mutex_lock(&library->un->io_mutex);
	TAPEALERT(library->open_fd, library->un);
	mutex_unlock(&library->un->io_mutex);
	UNLOCK_MAILBOX(library);
	mutex_lock(&library->un->io_mutex);
	TAPEALERT(library->open_fd, library->un);
	mutex_unlock(&library->un->io_mutex);

	library->status.b.except = FALSE;
	if (IS_TAPELIB(library->un))
		unload_all_drives(library);	/* unload the tapes */

	set_catalog_tape_media(library);

	if (!(is_optical(library->un->media)) &&
	    !(is_tape(library->un->media))) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9357,
		"Unable to determine media type. Library daemon exiting."));
		exit(1);
	}
	/* determine hardware configuration */
	if (init_library_configuration(library)) {
		DevLog(DL_ERR(5346));
		exit(1);
	}
	/*
	 * When init_elements is called with audit_tab_len = 0, a complete
	 * element_initialize is done. Very expensive. This should be changed
	 * in init_elements, but for now, hardcode a value of 1.
	 */
	library->audit_tab_len = 1;
	if (init_elements(library))	/* initialize all elements */
		exit(1);
	library->audit_tab_len = library->range.storage_count;

	retry = 2;

update_elements:

	if (update_element_status(library, TRANSPORT_ELEMENT)) {
		int		err;

		if (library->un->equ_type == DT_ADIC448)
			err = status_element_range(library, TRANSPORT_ELEMENT,
			    library->range.transport_lower,
			    library->range.transport_count);
		else
			err = update_element_status(library, TRANSPORT_ELEMENT);

		if (err)
			exit(1);
	}
	if (!library->status.b.except || init_done) {
		int		err;

		if (library->un->equ_type == DT_ADIC448)
			err = status_element_range(library,
			    IMPORT_EXPORT_ELEMENT,
			    library->range.ie_lower,
			    library->range.ie_count);
		else
			err = update_element_status(library,
			    IMPORT_EXPORT_ELEMENT);

		if (err)
			exit(1);
	}
	if (!library->status.b.except || init_done) {
		int		err;

		if (library->un->equ_type == DT_ADIC448)
			err = status_element_range(library,
			    DATA_TRANSFER_ELEMENT,
			    library->range.drives_lower,
			    library->range.drives_count);
		else
			err = update_element_status(library,
			    DATA_TRANSFER_ELEMENT);
		if (err)
			exit(1);
	}
	if (!library->status.b.except || init_done)
		if (update_element_status(library, STORAGE_ELEMENT))
			exit(1);

	if (library->status.b.except && !init_done) {
		int		err;
		int		added_more_time = FALSE;

		DevLog(DL_ERR(5037));
		mutex_lock(&library->un->io_mutex);
		retry = 2;
		do {
			if ((err = scsi_cmd(library->open_fd, library->un,
			    SCMD_INIT_ELEMENT_STATUS,
			    (library->range.storage_count * 45))) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				GENERIC_SCSI_ERROR_PROCESSING(library->un,
				    library->scsi_err_tab, 0,
				    err, added_more_time, retry,
				/* code for DOWN_EQU */
				    mutex_unlock(&library->un->io_mutex);
				return (1);
				/* MACRO for cstyle */,
				/* code for ILLREQ */
				    mutex_unlock(&library->un->io_mutex);
				return (1);
				/* MACRO for cstye */,
				    /* MACRO for cstyle */;
		/* MACRO for cstyle*/)
			} else
				break;
			DevLog(DL_RETRY(5038), 3 - retry);
		} while (--retry);

		mutex_unlock(&library->un->io_mutex);
		mutex_lock(&library->un->mutex);
		library->status.b.except = FALSE;
		mutex_unlock(&library->un->mutex);
		retry--;
		if (retry > 0)
			goto update_elements;
	} else {
		mutex_lock(&library->un->mutex);
		library->status.b.except = FALSE;
		mutex_unlock(&library->un->mutex);
	}

	/*
	 * reconcile_storge needs the dev_ent assigned for the drives, so
	 * assign device entries to each of the drive threads
	 */
	drive = library->drive;
	device_entry = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	mcf_drives = 0;
	for (i = 0; device_entry != NULL;
	    device_entry = (dev_ent_t *)SHM_REF_ADDR(device_entry->next)) {
		if (device_entry->fseq ==
		    library->eq && !IS_ROBOT(device_entry)) {
			mcf_drives++;
			if (drive == NULL) {
				DownDevice(device_entry, SAM_STATE_CHANGE);
				device_entry->status.bits = 0;
			} else {
				i++;
				mutex_lock(&device_entry->mutex);
				device_entry->status.b.requested = TRUE;
				mutex_unlock(&device_entry->mutex);
				/*
				 * Set full status for tape based on the
				 * unload bit. Optical uses the status from
				 * update_element_status.
				 */
				if (IS_TAPE(device_entry))
					drive->status.b.full |=
					    device_entry->status.b.unload;
				drive->un = device_entry;
				drive = drive->next;
			}
		}
	}

	if (library->range.drives_count != i)
		DevLog(DL_ERR(5184), library->range.drives_count, i);
	else if (library->range.drives_count != mcf_drives)
		DevLog(DL_ERR(5184), library->range.drives_count, mcf_drives);

	reconcile_storage(library);	/* update in_use fields */

	/*
	 * Since we can't do anything without the transports, free the mutex
	 * and let them clear before going on.
	 */
	cond_init(&library->transports->condit, USYNC_THREAD, NULL);
	cond_wait(&library->transports->condit, &library->transports->mutex);
	mutex_unlock(&library->transports->mutex);
	cond_destroy(&library->transports->condit);

	/* free all but the first import/export structure */
	for (iport = library->import->next; iport != NULL; iport = iport->next)
		mutex_unlock(&iport->mutex);

	/* Start the mailbox thread */
	mutex_unlock(&library->import->mutex);
	/*
	 * no import thread running	 for the exabyte 210, so lock the
	 * door here.
	 */
	if (library->un->type == DT_EXB210)
		(void) LOCK_MAILBOX(library);

	/* Start each drive thread */
	for (i = 0, drive = last_drive = library->drive;
	    drive != NULL;
	    drive = drive->next) {
		if (drive->un == NULL) {
			/*
			 * Kill the list at this point so we do not find the
			 * other drive entries, since there are no un entries
			 * for the extra drives.
			 */
			if (last_drive != NULL) {
				library->range.drives_count = i;
				last_drive->next = NULL;
				last_drive = NULL;
			}
		} else {
			int retries = 24;
			/*
			 * If no access to the drive, sleep a bit and try
			 * again -- the drive may be in a temporary state of
			 * no access due to some action it is performing
			 * (like unloading the media)
			 */
			while (retries && !drive->status.b.access) {
				sleep(5);
				update_element_status(library,
				    DATA_TRANSFER_ELEMENT);
				retries--;
			}

			/*
			 * if still no access, down the drive if there is no
			 * media in it
			 */
			if (!drive->status.b.access) {
				DevLog(DL_ERR(5186), drive->element);
				memccpy(drive->un->dis_mes[DIS_MES_CRIT],
				    catgets(catfd, SET, 9064,
				"media changer reports no access"), '\0',
				    DIS_MES_LEN);
				if (!drive->status.b.full) {
					DownDevice(drive->un, SAM_STATE_CHANGE);
					drive->status.b.offline = TRUE;
				}
			}
			mutex_unlock(&drive->mutex);
			i++;
		}
		if (last_drive != NULL)
			last_drive = drive;
	}
	if (library->range.drives_count != i)
		DevLog(DL_ERR(5185), library->range.drives_count, i);

	DevLog(DL_DETAIL(5039), library->range.drives_count);
	set_operator_panel(library, LOCK);

	return (0);
}


/*
 *	generic_re_init_library - reinitialize library.
 *
 * exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
generic_re_init_library(
			library_t *library)
{
	int		retries, need_rotate = FALSE;
	int		init_done;
	dev_ent_t	*un;
	sam_extended_sense_t *sense;
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(library->un->sense);

	SANITY_CHECK(library != (library_t *)0);
	un = library->un;
	SANITY_CHECK(un != (dev_ent_t *)0);
	SANITY_CHECK(sense != (sam_extended_sense_t *)0);

	mutex_lock(&library->un->mutex);
	library->un->status.b.ready = FALSE;
	library->un->status.b.present = TRUE;
	library->un->status.b.requested = TRUE;
	if (library->un->status.b.i_e_port) {
		library->un->status.b.attention = FALSE;
		library->un->status.b.i_e_port = FALSE;
		if (library->un->type == DT_METD28)
			library->status.b.except = TRUE;
		need_rotate = TRUE;
	}
	mutex_unlock(&library->un->mutex);

	DevLog(DL_DETAIL(5040));

	if (IS_TAPELIB(library->un))
		unload_all_drives(library);

	if ((init_done = wait_library_ready(library)) < 0)
		exit(1);

	DevLog(DL_DETAIL(5041));

	if (library->un->state == DEV_OFF || library->un->state == DEV_DOWN)
		exit(1);

	if (library->un->type == DT_EXB210)
		(void) LOCK_MAILBOX(library);
	else if (need_rotate) {
		if (library->un->equ_type == DT_PLASMON_G) {
			scsi_cmd(library->open_fd, un,
			    SCMD_OPEN_CLOSE_MAILSLOT, 0,
			    PLASMON_G_CLOSE);
		}
		UNLOCK_MAILBOX(library);
	}
	mutex_lock(&library->un->io_mutex);
	retries = 2;
	if (!init_done) {
		int		added_more_time = FALSE;

		do {
			int		err, count;

			count = library->range.drives_count +
			    library->range.storage_count;
			/*
			 * allow 16 second for each element an 30 seconds
			 * additional.
			 */
			(void) memset(sense, 0, sizeof (sam_extended_sense_t));

			if ((err = scsi_cmd(library->open_fd, library->un,
			    SCMD_INIT_ELEMENT_STATUS, (count * 45))) < 0) {
				TAPEALERT_SKEY(library->open_fd, library->un);
				GENERIC_SCSI_ERROR_PROCESSING(library->un,
				    library->scsi_err_tab, 0,
				    err, added_more_time, retries,
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
		} while (--retries > 0);

		if (retries <= 0)
			DevLog(DL_ERR(5213));
	}
	mutex_unlock(&library->un->io_mutex);

	if (update_element_status(library, TRANSPORT_ELEMENT))
		exit(1);

	if (update_element_status(library, IMPORT_EXPORT_ELEMENT))
		exit(1);

	if (update_element_status(library, DATA_TRANSFER_ELEMENT))
		exit(1);

	if (update_element_status(library, STORAGE_ELEMENT))
		exit(1);

	mutex_lock(&library->un->mutex);
	library->status.b.except = FALSE;
	mutex_unlock(&library->un->mutex);

	reconcile_storage(library);	/* update in_use fields */

	mutex_lock(&library->un->mutex);
	library->un->status.b.present = TRUE;
	library->un->status.b.ready = TRUE;
	library->un->status.b.requested = FALSE;
	library->un->status.b.attention = FALSE;
	mutex_unlock(&library->un->mutex);
	set_operator_panel(library, LOCK);
	return (0);
}


/*
 * Routine to initialize library configuration data other than element
 * data.
 *
 * exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
init_library_configuration(
			library_t *library)
{
	dev_ent_t	*un;
	int		retval = 0;

	un = library->un;
	switch (un->type) {
	case DT_HP_C7200:
		{
			void *lib_mode_sense(library_t *, uchar_t,
			    uchar_t *, int);
			hp_c7200_ms_page21_t page21;

			mutex_lock(&library->un->io_mutex);
			if (lib_mode_sense(library, 0x21, (uchar_t *)& page21,
			    sizeof (page21))) {
				un->dt.rb.status.b.barcodes =
				    page21.barcode_reader;
				if (page21.stacker_mode != 2 &&
				    page21.stacker_mode != 3) {
					DevLog(DL_ERR(5131));
					retval = 1;
				}
			} else {
				un->dt.rb.status.b.barcodes = 0;
			}
			mutex_unlock(&library->un->io_mutex);
		}
		break;

	case DT_DOCSTOR:
		{
			void *lib_mode_sense(library_t *, uchar_t,
			    uchar_t *, int);
			plasmo_page21_t page21;

			mutex_lock(&library->un->io_mutex);
			if (lib_mode_sense(library, 0x21, (uchar_t *)& page21,
			    sizeof (page21))) {
				un->dt.rb.status.b.barcodes = page21.enbcr;
			} else {
				un->dt.rb.status.b.barcodes = 0;
			}
			mutex_unlock(&library->un->io_mutex);
		}
		break;

	case DT_PLASMON_G:
		{
			/*
			 * Verify that the library is configured the way we
			 * can deal with.
			 */
			void *lib_mode_sense(library_t *, uchar_t,
			    uchar_t *, int);
			plasmo_page21_t page21;

			mutex_lock(&library->un->io_mutex);
			if (!lib_mode_sense(library, 0x21, (uchar_t *)& page21,
			    sizeof (page21))) {
				mutex_unlock(&library->un->io_mutex);
				retval = 1;
				break;
			}
			mutex_unlock(&library->un->io_mutex);

			/*
			 * Library must not be in emulation mode.
			 */
			if (page21.libemulacd != 0) {
				DevLog(DL_ERR(13001));
				retval = 1;
			}
			/*
			 * Library must use address scheme 1
			 */
			if (page21.eleaddrsch != 1) {
				DevLog(DL_ERR(13002));
				retval = 1;
			}
			/*
			 * Library must use barcode type 2 or type 3
			 */
			if (page21.barcodetype != 2 &&
			    page21.barcodetype != 3) {
				DevLog(DL_ERR(13003));
				retval = 1;
			}
			if (retval)
				break;
			un->dt.rb.status.b.barcodes = page21.enbcr;
		}
		break;
	default:
		break;
	}
	return (retval);
}


/*
 * Routine to start a bound thread to read labels then unload every
 * drive in a library.
 *
 * This routine may be called BEFORE all library structs are complete.
 * The only requirement is that the device entries and the eq
 * field of the library be initialized.
 */
void
unload_all_drives(library_t *library)
{
	int		drive_count = 0;
	thread_t	*threads;
	dev_ent_t	*un;
	boolean_t	unload_issued;
	int		err;

	SANITY_CHECK(library != (library_t *)0);

	/* allow for 128 devices, should be more than enough */
	threads = (thread_t *)malloc_wait(128 * sizeof (thread_t), 2, 0);

	un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	/* find all drives */
	for (; un != NULL;
	    un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		if (un->fseq == library->eq && !IS_ROBOT(un) &&
		    un->state < DEV_IDLE) {
			/*
			 * We know nothing about these drives, clear out the
			 * information pertaining to the catalog.
			 */
			mutex_lock(&un->mutex);
			un->mid = un->flip_mid = ROBOT_NO_SLOT;
			un->slot = ROBOT_NO_SLOT;
			mutex_unlock(&un->mutex);
			if (thr_create(NULL,
			    MD_THR_STK, &unload_a_drive_thread, (void *) un,
			    (THR_BOUND | THR_NEW_LWP), &threads[drive_count])) {
				/* could not create thread, do it serially */
				int		open_fd;

				DevLog(DL_SYSERR(5043));
				mutex_lock(&un->mutex);
				INC_ACTIVE(un);
				mutex_lock(&un->io_mutex);
				/*
				 * use no speical open flags.  If the open
				 * fails, then assume there is no tape in the
				 * drive.
				 */
				if ((open_fd = open(un->name, O_RDONLY)) >= 0) {
					INC_OPEN(un);
					un->status.b.scanning = TRUE;
					process_tape_labels(open_fd, un);
					un->status.b.scanning = FALSE;
					un->status.bits |= (DVST_SCANNED |
					    DVST_UNLOAD);
					if (IS_TAPE(un))
						un->dt.tp.position = 0;
					mutex_unlock(&un->mutex);

					/*
					 * If this device is in a 2700 type
					 * loader, don't issue the unload. If
					 * a reset has been issued or this is
					 * after a power on, then the loader
					 * is in "sequential" mode and the
					 * unload will force the next
					 * cartridge to be loaded.
					 */
					unload_issued = B_FALSE;
					if (!(DEV_ENT(un->fseq)->equ_type ==
					    DT_DLT2700)) {
						unload_issued = B_TRUE;
						TAPEALERT(open_fd, un);
						err = scsi_cmd(open_fd, un,
						    SCMD_START_STOP, 300,
						    SPINDOWN, 0);
						TAPEALERT(open_fd, un);
					}
					if (unload_issued ==
					    B_TRUE && err != 0) {
						DevLog(DL_ERR(5187));
						SendCustMsg(HERE, 9346);
						DownDevice(library->un,
						    SAM_STATE_CHANGE);
						close(open_fd);
						mutex_lock(&un->mutex);
						DEC_ACTIVE(un);
						DEC_OPEN(un);
						mutex_unlock(&un->mutex);
						mutex_unlock(&un->io_mutex);
						if (threads)
							free(threads);
						return;
					}
					close(open_fd);
					mutex_lock(&un->mutex);
					DEC_OPEN(un);
				}
				DEC_ACTIVE(un);
				mutex_unlock(&un->io_mutex);
				mutex_unlock(&un->mutex);
			} else
				drive_count++;
		}
	}

	/* wait for all threads to exit */
	while (drive_count > 0) {
		drive_count--;
		thr_join(threads[drive_count], (void *) NULL, (void *) NULL);
	}
	free(threads);
}


void	   *
unload_a_drive_thread(
			void *vun)
{
	int		open_fd;
	dev_ent_t	*un = (dev_ent_t *)vun;
	sam_extended_sense_t *sp;
	boolean_t	unload_issued;
	int		err;

	SANITY_CHECK(un != (dev_ent_t *)0);
	sp = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	SANITY_CHECK(sp != (sam_extended_sense_t *)0);

	mutex_lock(&un->mutex);
	INC_ACTIVE(un);
	un->status.b.requested = TRUE;
	mutex_lock(&un->io_mutex);

	/*
	 * If this device is in a 2700 type loader, issue the a mode select
	 * to disable sequential mode. This will prevent the changer from
	 * loading media when an unload is issued. This is 2.5+ code.
	 * Solaris releases less than 2.5 do not support the O_NDELAY option
	 * to open without a tape in the drive.
	 */
	if (DEV_ENT(un->fseq)->equ_type == DT_DLT2700)
		if ((open_fd = open(un->name, O_RDONLY | O_NDELAY)) >= 0) {
			char	   *p_name = "ENALDRAUTOLD 0";
			int		len = strlen(p_name) + 1;
			int		md_len = len + 4 + 2;
			char *md_select = (char *)malloc_wait(
			    md_len, 2, 0);

			memset(md_select, 0, md_len);
			md_select[2] = 0x10;	/* set buffered mode */
			md_select[4] = 0x3e;	/* vendor unique eerom set */
			md_select[5] = len;
			memcpy(&md_select[6], p_name, len);
			memset(sp, 0, sizeof (sam_extended_sense_t));
			if (scsi_cmd(open_fd, un, SCMD_MODE_SELECT,
			    10, md_select,
			    md_len, 0x10, NULL) < 0) {
				TAPEALERT_SKEY(open_fd, un);
				DevLog(DL_RETRY(5044));
				DevLogSense(un);
				DevLogCdb(un);
				memset(sp, 0, sizeof (sam_extended_sense_t));
				if (scsi_cmd(open_fd, un, SCMD_MODE_SELECT,
				    10, md_select,
				    md_len, 0x10, NULL) < 0) {
					TAPEALERT_SKEY(open_fd, un);
					DevLog(DL_ERR(5045));
					DevLogSense(un);
					DevLogCdb(un);
				}
			}
			close(open_fd);
			free(md_select);
		}
	if ((open_fd = open(un->name, O_RDONLY)) >= 0) {
		INC_OPEN(un);
		un->status.b.scanning = TRUE;
		process_tape_labels(open_fd, un);
		un->status.b.scanning = FALSE;
		un->status.bits |= (DVST_SCANNED | DVST_UNLOAD);
		if (IS_TAPE(un))
			un->dt.tp.position = 0;
		mutex_unlock(&un->mutex);

		/*
		 * If this is a 2700, avoid the unload since a bus reset can
		 * set the device to sequential mode.
		 */
		unload_issued = B_FALSE;
		if (!(DEV_ENT(un->fseq)->equ_type == DT_DLT2700)) {
			unload_issued = B_TRUE;
			TAPEALERT(open_fd, un);
			err = scsi_cmd(open_fd, un, SCMD_START_STOP,
			    300, SPINDOWN, 0);
			TAPEALERT(open_fd, un);
		}
		if (unload_issued == B_TRUE && err != 0) {
			DevLog(DL_ERR(5046));
			DownDevice(un, SAM_STATE_CHANGE);
			close(open_fd);
			mutex_lock(&un->mutex);
			DEC_ACTIVE(un);
			DEC_OPEN(un);
			mutex_unlock(&un->mutex);
			mutex_unlock(&un->io_mutex);
			thr_exit((void *) NULL);
		}
		close(open_fd);
		mutex_lock(&un->mutex);
		DEC_OPEN(un);
	}
	un->status.b.requested = FALSE;
	DEC_ACTIVE(un);
	mutex_unlock(&un->io_mutex);
	mutex_unlock(&un->mutex);
	thr_exit((void *) NULL);
}


/*
 *	generic_init_drive - initialize drive.
 */
void
generic_init_drive(
		drive_state_t *drive)
{
	char	   *d_mess;
	dev_ent_t	*un;

	SANITY_CHECK(drive != (drive_state_t *)0);
	un = drive->un;
	SANITY_CHECK(un != (dev_ent_t *)0);

	d_mess = un->dis_mes[DIS_MES_NORM];

	mutex_lock(&drive->mutex);
	mutex_lock(&un->mutex);
	un->scan_tid = thr_self();
	if (un->state < DEV_IDLE) {
		DevLog(DL_DETAIL(5047));
		memccpy(d_mess, catgets(catfd, SET, 9065, "initializing"),
		    '\0', DIS_MES_LEN);
		un->status.b.requested = TRUE;
		if (IS_TAPE(un))
			ChangeMode(un->name, SAM_TAPE_MODE);
		if (un->st_rdev == 0 && IS_OPTICAL(un)) {
			int		local_open;
			struct stat	stat_buf;

			local_open = open_unit(un, un->name, 1);
			clear_driver_idle(drive, local_open);
			if (fstat(local_open, &stat_buf))
				DevLog(DL_SYSERR(5188), un->name);
			else
				un->st_rdev = stat_buf.st_rdev;

			close_unit(un, &local_open);
			mutex_unlock(&un->mutex);
		} else
			mutex_unlock(&un->mutex);

		if (drive->status.b.full && clear_drive(drive))
			down_drive(drive, SAM_STATE_CHANGE);

		mutex_lock(&un->mutex);
		if (drive->open_fd >= 0)
			close_unit(un, &drive->open_fd);
	}
	un->status.b.requested = FALSE;
	mutex_unlock(&un->mutex);
	mutex_unlock(&drive->mutex);
}
