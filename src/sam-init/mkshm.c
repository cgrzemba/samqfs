/*
 * mkshm.c - shared memory allocater for samfs.
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

#pragma ident "$Revision: 1.15 $"

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/shm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "aml/types.h"
#include "sam/mount.h"
#include "aml/shm.h"
#include "sam/resource.h"
#include "aml/mode_sense.h"
#include "aml/robots.h"
#include "sam/lib.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "aml/preview.h"
#include "sam/nl_samfs.h"
#include "sam/defaults.h"

/* Local headers. */
#include "amld.h"


/* Private data. */
static shm_ptr_tbl_t *shm_ptr_tbl;
/* The robot count starts at one since there is always an historian */
static int tape_count = 0, odisk_count = 0, robot_count = 1, sd_count = 0;
static int third_party_count = 0;
static int rs_devices = 0, rs_clients = 0, rs_servers = 0, rs_count = 0;

/* Private functions. */
static void initIo(sam_act_io_t *active_io);
static void layout_device_table(int high_eq, int count, dev_ent_t *devices);
static void layout_preview_table(shm_preview_tbl_t *memory,
		sam_defaults_t *defaults);


/*
 * alloc_shm_seg - allocate the main shared memory segment and build the
 * device table in that segment.
 */
void
alloc_shm_seg(
	void)
{
	dev_ent_t *device_list, *entry;
	sam_act_io_t *active_io;
	sam_defaults_t *defaults;
	shm_block_def_t *shm_block_def;
	shm_block_def_t *shm_preview_blk_def;
	struct shmid_ds shm_status;
	size_t	segment_size;
	void	*memory;
	char	*ent_pnt = "alloc_shm_seg";
	int		fs_count = 0;
	int		dev_count, high_eq = 0;
	int		shmid, page_size, fifo_path_len;
	int		round, actio_tab_len;

	fifo_path_len = strlen(SAM_FIFO_PATH);
	page_size = (int)sysconf(_SC_PAGESIZE);

	if ((dev_count = read_mcf(SAM_CONFIG_PATH "/" CONFIG,
	    &device_list, &high_eq)) == 0) {
		kill_off_threads();	/* no devices, why are we running */
		exit(EXIT_FAILURE);
	}
	defaults = (sam_defaults_t *)malloc_wait(sizeof (sam_defaults_t), 2, 0);
	get_defaults(defaults);

	/*
	 * Find the correct size of the shared memory segment.
	 * Find the number of scsi sense and mode sense blocks to allocate.
	 */
	for (entry = device_list; entry != NULL; entry = entry->next) {
		dtype_t tmp_type = entry->equ_type & DT_CLASS_MASK;

		if (tmp_type == DT_FAMILY_SET) {
			fs_count++;
		}
		if (((tmp_type == DT_OPTICAL) ? ++odisk_count : 0) ||
		    ((tmp_type == DT_TAPE) ? ++tape_count : 0) ||
		    ((tmp_type == DT_ROBOT) ? ++robot_count : 0) ||
		    ((tmp_type == DT_THIRD_PARTY) ? ++third_party_count : 0)) {
			sd_count++;
		}
		if (entry->equ_type == DT_PSEUDO_SS) {
			rs_servers++;
		}
		if (entry->equ_type == DT_PSEUDO_SC) {
			rs_clients++;
		}
		if (entry->equ_type == DT_PSEUDO_RD) {
			rs_devices++;
		}
	}

	rs_count = rs_servers + rs_clients + rs_devices;

	/* start with the length of the shm_ptr_tbl */
	segment_size = sizeof (shm_ptr_tbl_t) + SEGMENT_EXCESS + fifo_path_len;

	/* add defaults structure */
	segment_size += sizeof (sam_defaults_t);

	/* add Device tables */
	segment_size += (dev_count * (sizeof (dev_ent_t) + 0)) +
	    (high_eq * (sizeof (dev_ent_t *))) + sizeof (dev_ptr_tbl_t);

	/* add extended_sense_space */
	segment_size += (sd_count * sizeof (sam_extended_sense_t));

	/* add mode_sense_space */
	segment_size += (sd_count * (sizeof (mode_sense_t)));

	/*
	 * Add in the message areas (one for each robot, third party device,
	 * the scanner, archiver, recycler and each remote sam
	 * client/server/device.
	 */
	segment_size += ((robot_count + third_party_count + 3 + rs_count) *
	    sizeof (message_request_t));

	segment_size += (robot_count * ROBOT_PRIVATE_AREA);

	/*
	 * Add in the third party message area (for displaying messages.
	 */
	segment_size += (third_party_count * TP_DISP_MSG_LEN);

	/*
	 * Add in the structs for rmt sam, each server has one per client,
	 * each client has one.
	 */
	segment_size += ((rs_clients + (rs_servers * RMT_SAM_MAX_CLIENTS)) *
	    sizeof (srvr_clnt_t));

	/*
	 * Add in active io tables.
	 */
	actio_tab_len = ((odisk_count + tape_count + rs_devices) *
	    sizeof (sam_act_io_t));
	segment_size += (actio_tab_len + 8);	/* for int64_t rounding */

	/*
	 * Add in resource area for optical.
	 */
	segment_size += (odisk_count * sizeof (sam_resource_t));

	/*
	 * Round to even number of pages and add one extra page.
	 */
	segment_size = ((segment_size / page_size) + 2) * page_size;

	/*
	 * Create the segment.  If one already exists, remove it.
	 */
	if ((shmid = shmget(SHM_MASTER_KEY, segment_size,
	    0664 | IPC_EXCL | IPC_CREAT)) < 0) {
		if (errno == EINVAL) {
			/*
			 * Increase shared memory segment: for example, in
			 * /etc/system: set shmsys:shminfo_shmmax = 0x200000
			 */
			sam_syslog(LOG_ERR, catgets(catfd, SET, 20013,
			    "shmget: Invalid shared memory segment size %d.\n"),
			    segment_size);
			kill_off_threads();
			exit(EXIT_FAILURE);
		}
		if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) >= 0) {
			if ((memory = shmat(shmid, NULL, 0)) == (void *)-1) {
				sam_syslog(LOG_ERR, "%s: shmget: %m.", ent_pnt);
				kill_off_threads();
				exit(EXIT_FAILURE);
			}
			/* Invalidate old segment. */
			((shm_ptr_tbl_t *)memory)->valid = 0;
			if (shmctl(shmid, IPC_RMID, NULL) < 0) {
				sam_syslog(LOG_ERR, "%s: shmctl: %m.", ent_pnt);
				kill_off_threads();
				exit(EXIT_FAILURE);
			}
			shmid = shmget(SHM_MASTER_KEY, segment_size,
			    0664 | IPC_EXCL | IPC_CREAT);
		}
		if (shmid < 0) {
			sam_syslog(LOG_ERR, "%s: shmget: cannot create %m.",
			    ent_pnt);
			kill_off_threads();
			exit(EXIT_FAILURE);
		}
	}
	if ((memory = shmat(shmid, NULL, 0)) == (void *)-1) {
		sam_syslog(LOG_ERR, "%s: shmat: %m.", ent_pnt);
		kill_off_threads();	/* can't allocate shm segment */
		exit(EXIT_FAILURE);
	}
	(void) memset(memory, 0, segment_size);	/* zero the segment */

	/*
	 * Initialize and lock the main mutex.
	 */
	shm_ptr_tbl = (shm_ptr_tbl_t *)memory;
	shm_block_def = &shm_ptr_tbl->shm_block;

	if (mutex_init(&(shm_block_def->shm_mut), USYNC_PROCESS, NULL) < 0) {
		sam_syslog(LOG_ERR, "%s: mutex_init: %m.", ent_pnt);
		kill_off_threads();	/* can't create mutex */
		exit(EXIT_FAILURE);
	}
	while (mutex_trylock(&(shm_block_def->shm_mut)))
		sleep(2);		/* keep trying */

	/* initialize the shared memory block definition */
	(void) memmove(shm_block_def->segment_name, SAM_SEGMENT_NAME,
	    sizeof (SAM_SEGMENT_NAME));
	shm_block_def->size = segment_size;
	shm_block_def->left = segment_size - sizeof (shm_ptr_tbl_t);
	shm_block_def->next_p = sizeof (shm_ptr_tbl_t);

	/* put the samfs directory path name in the segment */
	shm_ptr_tbl->fifo_path = shm_block_def->next_p;
	(void) memmove(((char *)memory) + shm_ptr_tbl->fifo_path, SAM_FIFO_PATH,
	    fifo_path_len + 1);
	shm_block_def->next_p += (fifo_path_len + 1);
	shm_block_def->left -= (fifo_path_len + 1);

	/* record the number of robots */
	shm_ptr_tbl->robot_count = robot_count;
	/* Indicate if this is a remote sam client */
	if (rs_clients)
		shm_ptr_tbl->flags.bits |= SHM_FLAGS_RMTSAMCLNT;

	/* Indicate if this is a remote sam server */
	if (rs_servers)
		shm_ptr_tbl->flags.bits |= SHM_FLAGS_RMTSAMSRVR;

	/* we own the mutex, so let the world know where it is. */

	master_shm.shmid = shmid;
	master_shm.shared_memory = memory;

	/* Set the group to the seqment to oper */
	if (shmctl(shmid, IPC_STAT, &shm_status))
		sam_syslog(LOG_INFO, "%s: shmctl IPC_STAT: %m.", ent_pnt);
	else {
		shm_status.shm_perm.gid = defaults->operator.gid;
		shm_status.shm_perm.mode = 0664;
		if (shmctl(shmid, IPC_SET, &shm_status))
			sam_syslog(LOG_INFO, "%s: shmctl IPC_SET: %m.",
			    ent_pnt);
	}

	layout_device_table(high_eq, dev_count, device_list);

	/*
	 * layout the staging tables Since the staging struct contains a long
	 * long, it must be lined up on an 8 byte boundary.
	 */

	ALIGN(shm_block_def->next_p, long long, round);
	shm_block_def->left -= round;
	/*
	 * Layout the active io table Since the struct contains a long long,
	 * it must be lined up on an 8 byte boundary.
	 */
	ALIGN(shm_block_def->next_p, long long, round);
	shm_block_def->left -= round;
	active_io = (sam_act_io_t *)(long)shm_block_def->next_p;
	shm_block_def->left -= actio_tab_len;

	if (shm_block_def->left < 0) {
		sam_syslog(LOG_CRIT,
		    "Critical problem with shared memory segment length.");
		exit(10);
	}
	initIo(active_io);
	/* create preview table segment */

	/* start with the length of the shm_preview_tbl */
	segment_size = sizeof (shm_preview_tbl_t);
	segment_size += (defaults->previews * sizeof (preview_t));
	segment_size += fs_count * sizeof (prv_fs_ent_t);
	segment_size += ((defaults->stages + 10) * sizeof (stage_preview_t));
	/* round to even number of pages */
	segment_size = ((segment_size / page_size) + 1) * page_size;

	/* Create the segment.  If one already exists, remove it. */

	if ((shmid = shmget(SHM_PREVIEW_KEY, segment_size,
	    0664 | IPC_EXCL | IPC_CREAT)) < 0) {
		/* a segment already exists, get rid of it */

		if (((shmid = shmget(SHM_PREVIEW_KEY, 0, 0)) > 0) &&
		    !shmctl(shmid, IPC_RMID, NULL) &&
		    ((shmid = shmget(SHM_PREVIEW_KEY, segment_size,
		    0664 | IPC_EXCL | IPC_CREAT)) < 0)) {
			sam_syslog(LOG_ERR, "%s: shmget: %m.", ent_pnt);
			kill_off_threads();	/* can't allocate shm segment */
			exit(EXIT_FAILURE);
		}
	}
	if ((memory = shmat(shmid, NULL, 0777)) == (void *)-1) {
		sam_syslog(LOG_ERR, "%s: shmat: %m.", ent_pnt);
		kill_off_threads();	/* can't allocate shm segment */
		exit(EXIT_FAILURE);
	}
	(void) memset(memory, 0, segment_size);	/* zero the segment */

	/* Initialize and lock the main mutex */

	shm_preview_blk_def = (shm_block_def_t *)memory;

	if (mutex_init(&(shm_preview_blk_def->shm_mut), USYNC_PROCESS,
	    NULL) < 0) {
		sam_syslog(LOG_ERR, "%s: mutex_init: %m.", ent_pnt);
		kill_off_threads();	/* can't create mutex */
		exit(EXIT_FAILURE);
	}
	while (mutex_trylock(&(shm_preview_blk_def->shm_mut)))
		sleep(2);		/* keep trying */

	if (shmctl(shmid, IPC_STAT, &shm_status))
		sam_syslog(LOG_ERR, "%s: shmctl IPC_STAT: %m.", ent_pnt);
	else {
		shm_status.shm_perm.gid = defaults->operator.gid;
		shm_status.shm_perm.mode = 0664;
		if (shmctl(shmid, IPC_SET, &shm_status))
			sam_syslog(LOG_ERR, "%s: shmctl IPC_SET: %m.", ent_pnt);
	}

	/* initialize the shared memory block definition */
	preview_shm.shmid = shmid;
	shm_ptr_tbl->preview_shmid = shmid;
	preview_shm.shared_memory = memory;

	(void) memmove(shm_preview_blk_def->segment_name, SAM_PREVIEW_NAME,
	    sizeof (SAM_PREVIEW_NAME));
	shm_preview_blk_def->size = segment_size;
	shm_preview_blk_def->left = segment_size - sizeof (shm_ptr_tbl_t);
	shm_preview_blk_def->next_p = sizeof (shm_ptr_tbl_t);

	layout_preview_table((shm_preview_tbl_t *)memory, defaults);
	shm_ptr_tbl->debug |= SAM_DBG_LOGGING;
	mutex_unlock(&(shm_preview_blk_def->shm_mut));
	mutex_unlock(&(shm_block_def->shm_mut));	/* release the master */
							/* segment. */
}

void
rel_shm_seg(void)
{
	/* Invalidate master shm. */
	((shm_ptr_tbl_t *)master_shm.shared_memory)->valid = 0;
	(void) shmctl(master_shm.shmid, IPC_RMID, NULL);
	(void) shmctl(preview_shm.shmid, IPC_RMID, NULL);
}


/*
 * Initialize I/O controls.
 */
static void
initIo(sam_act_io_t *active_io)
{
	dev_ptr_tbl_t *dev_tab;
	sam_act_io_t *active_data;
	int i;

	active_data = (sam_act_io_t *)SHM_REF_ADDR(active_io);
	dev_tab = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);

	for (i = 0; i <= dev_tab->max_devices; i++) {
		dev_ent_t *un;

		if (dev_tab->d_ent[i] == 0) {
			continue;
		}
		un = (dev_ent_t *)SHM_REF_ADDR(dev_tab->d_ent[i]);
		if (IS_TAPE(un) || IS_OPTICAL(un) || IS_RSD(un)) {
			un->active_io = active_io;
			(void) mutex_init(&active_data->mutex, USYNC_PROCESS,
			    NULL);
			cond_init(&active_data->cond, USYNC_PROCESS, NULL);
			active_io++;
			active_data++;
		}
	}
}


void ReadPreviewCmd(preview_tbl_t *, prv_fs_ent_t *);


static void
layout_device_table(int high_eq, int count, dev_ent_t *devices)
{
	int length, round, offset;
	int ms_len, sen_len;
	int srservers = 0;
	char *robot_private, *base_address, *tp_messages;
	char *ent_pnt = "layout_device_table";
	dev_ent_t *next_device, *last_device;
	dev_ent_t *next_shm_device, *last_shm_device;
	srvr_clnt_t *srvr_clnts;
	mode_sense_t *mode_sense_location;
	dev_ptr_tbl_t *dev_table;
	shm_ptr_tbl_t *shm_ptr_tbl;
	shm_block_def_t *shm_block_def;
	message_request_t *message_location, *tmp_message;
	sam_extended_sense_t *sense_location;

	base_address = master_shm.shared_memory;
	/* LINTED pointer cast may result in improper alignment */
	shm_ptr_tbl = (shm_ptr_tbl_t *)base_address;

	shm_block_def = &(shm_ptr_tbl->shm_block);

	/*
	 * Force correct memory alignment for pointers for the start of the
	 * dev_table.
	 */

	ALIGN(shm_block_def->next_p, void *, length);
	shm_block_def->left -= length;
	shm_ptr_tbl->dev_table = shm_block_def->next_p;

	length = sizeof (dev_ptr_tbl_t) + (count * sizeof (dev_ent_t)) +
	    (high_eq * (sizeof (dev_ent_t *)));

	/*
	 * The first dev_ent_t entry is located at the end of the d_ent[]
	 * array.  Set next_shm_device to the offset relitive the memory seg.
	 * To get correct memory alignment round the starting offset up to a
	 * multiple of sizeof(long long).
	 */

	offset = sizeof (dev_ptr_tbl_t) + shm_ptr_tbl->dev_table +
	    (high_eq * sizeof (dev_ent_t *));

	ALIGN(offset, long long, round);
	/* LINTED pointer cast may result in improper alignment */
	next_shm_device = (dev_ent_t *)(base_address + offset);
	shm_ptr_tbl->first_dev = offset;
	length += round;

	/*
	 * Layout the messages areas - one for each robot, third party device
	 * the scanner, the archiver, each remote server/client/device and
	 * the recycler.
	 */
	last_shm_device = NULL;
	message_location =
	    (message_request_t *)(long)(shm_block_def->next_p + length);
	length += ((robot_count + third_party_count + rs_count + 3) *
	    sizeof (message_request_t));

	/*
	 * Initialize the scanners message request area.
	 */
	tmp_message = (message_request_t *)(long)(base_address +
	    (long)message_location);
	(void) mutex_init(&tmp_message->mutex, USYNC_PROCESS, NULL);
	cond_init(&tmp_message->cond_i, USYNC_PROCESS, NULL);
	cond_init(&tmp_message->cond_r, USYNC_PROCESS, NULL);
	tmp_message->mtype = MESS_MT_VOID;
	shm_ptr_tbl->scan_mess = (long)message_location++;

	/*
	 * Set the size of sense and mode sense to multiples of 4 bytes.
	 */
	ms_len = sizeof (mode_sense_t) + (4 - (sizeof (mode_sense_t) & 0x3));
	sen_len = sizeof (sam_extended_sense_t) +
	    (4 - (sizeof (sam_extended_sense_t) & 0x3));

	sense_location = (sam_extended_sense_t *)(long)(shm_block_def->next_p +
	    length);
	length += (sd_count * sen_len);

	mode_sense_location = (mode_sense_t *)(long)(shm_block_def->next_p + length);
	length += (sd_count * ms_len);

	shm_block_def->next_p += length; /* next unused area */
	shm_block_def->left -= length;

	length = (rs_clients + (rs_servers * RMT_SAM_MAX_CLIENTS)) *
	    sizeof (srvr_clnt_t);
	srvr_clnts = (srvr_clnt_t *)(long)shm_block_def->next_p;
	shm_block_def->next_p += length; /* next unused area */
	shm_block_def->left -= length;

	/* Set the location of the robot privare area to multiples of 8 bytes */
	ALIGN(shm_block_def->next_p, long long, round);
	shm_block_def->left -= round;
	robot_private = (char *)(long)shm_block_def->next_p;
	shm_block_def->next_p += (robot_count * ROBOT_PRIVATE_AREA);
	tp_messages = (char *)(long)shm_block_def->next_p;
	shm_block_def->next_p += (third_party_count * TP_DISP_MSG_LEN);

	memmove(shm_block_def->segment_name, SAM_SEGMENT_NAME,
	    strlen(SAM_SEGMENT_NAME));

	/*
	 * Get a nice pointer to the device table in the segment.
	 */
	/* LINTED pointer cast may result in improper alignment */
	dev_table = (dev_ptr_tbl_t *)(base_address + shm_ptr_tbl->dev_table);

	if (mutex_init(&(dev_table->mutex), USYNC_PROCESS, NULL) < 0) {
		sam_syslog(LOG_ERR, "%s: mutex_init:(%s:%d) %m", ent_pnt,
		    __FILE__, __LINE__);
		kill_off_threads();
		exit(EXIT_FAILURE);
	}
	dev_table->max_devices = high_eq;
	last_shm_device = NULL;
	last_device = NULL;

	/*
	 * Loop through the device list and move the entries to the shared
	 * memory segment, release the memory as you go.
	 */
	for (next_device = devices;
	    next_device != NULL;
	    next_device = next_device->next) {
		memmove(next_shm_device, next_device, sizeof (dev_ent_t));
		next_shm_device->mid = ROBOT_NO_SLOT;
		if (last_shm_device != NULL) {
			last_shm_device->next =
			    (void *)Ptrdiff(next_shm_device, base_address);
		}
		last_shm_device = next_shm_device;
		dev_table->d_ent[next_device->eq] =
		    (void *)Ptrdiff(next_shm_device, base_address);

		if (last_device != NULL) {
			free(last_device);	/* release memory */
		}
		last_device = next_device;

		/*
		 * Initialize the mutex for the device entry.
		 */
		if (mutex_init(&(next_shm_device->mutex), USYNC_PROCESS, NULL) <
		    0) {
			sam_syslog(LOG_ERR, "%s: mutex_init:(%s:%d) %m",
			    ent_pnt, __FILE__, __LINE__);
			kill_off_threads();
			exit(EXIT_FAILURE);
		}
		/*
		 * Initialize any fields that need it here, because of
		 * generic typing, the real device type may not be known at
		 * this time.
		 */
		{
			next_shm_device->status.bits = 0;
			next_shm_device->sef_info.sef_inited = 0;

			if (mutex_init(&(next_shm_device->io_mutex),
			    USYNC_PROCESS, NULL)) {
				sam_syslog(LOG_ERR, "%s: mutex_init:(%s:%d) %m",
				    ent_pnt, __FILE__, __LINE__);
				kill_off_threads();
				exit(EXIT_FAILURE);
			}
			/*
			 * If tape or optical clear out stuff affecting
			 * catalog entries.
			 */
			if ((IS_TAPE(next_shm_device) ||
			    IS_OPTICAL(next_shm_device))) {
				next_shm_device->mid = ROBOT_NO_SLOT;
				next_shm_device->flip_mid = ROBOT_NO_SLOT;
				next_shm_device->slot = (ushort_t)ROBOT_NO_SLOT;
			}
			if (next_shm_device->equ_type == DT_PSEUDO_SS) {
				next_shm_device->dt.ss.clients = srvr_clnts;
				next_shm_device->dt.ss.ordinal = srservers++;
				srvr_clnts += RMT_SAM_MAX_CLIENTS;
			}
			if (next_shm_device->equ_type == DT_PSEUDO_SC) {
				next_shm_device->dt.sc.server = srvr_clnts++;
			}

			/* If its the historian, set it up like a family set */
			if (next_shm_device->equ_type == DT_HISTORIAN) {
				next_shm_device->fseq = next_shm_device->eq;
			}

			/* does it need robot stuff */
			if (IS_ROBOT(next_shm_device)) {
				next_shm_device->dt.rb.message =
				    (long)message_location;
				next_shm_device->dt.rb.private =
				    (long)robot_private;
				tmp_message = (message_request_t *)(void *)
				    (base_address + (long)message_location);
				(void) mutex_init(&tmp_message->mutex,
				    USYNC_PROCESS, NULL);
				cond_init(&tmp_message->cond_i, USYNC_PROCESS,
				    NULL);
				cond_init(&tmp_message->cond_r, USYNC_PROCESS,
				    NULL);
				tmp_message->mtype = MESS_MT_VOID;
				message_location++;
				robot_private += ROBOT_PRIVATE_AREA;
			}
			/* does it need third party stuff */
			if (IS_THIRD_PARTY(next_shm_device)) {
				next_shm_device->dt.tr.message =
				    (long)message_location;
				tmp_message = (message_request_t *)(void *)
				    (base_address + (long)message_location);
				next_shm_device->dt.tr.disp_msg =
				    (long)tp_messages;
				tp_messages += TP_DISP_MSG_LEN;
				(void) mutex_init(&tmp_message->mutex,
				    USYNC_PROCESS, NULL);
				cond_init(&tmp_message->cond_i, USYNC_PROCESS,
				    NULL);
				cond_init(&tmp_message->cond_r, USYNC_PROCESS,
				    NULL);
				tmp_message->mtype = MESS_MT_VOID;
				message_location++;
			}
			/* does it need remote server stuff */
			if (IS_RSS(next_shm_device)) {
				next_shm_device->dt.ss.message =
				    (long)message_location;
				tmp_message = (message_request_t *)(void *)
				    (base_address + (long)message_location);
				(void) mutex_init(&tmp_message->mutex,
				    USYNC_PROCESS, NULL);
				cond_init(&tmp_message->cond_i, USYNC_PROCESS,
				    NULL);
				cond_init(&tmp_message->cond_r, USYNC_PROCESS,
				    NULL);
				tmp_message->mtype = MESS_MT_VOID;
				message_location++;
			}
			/* does it need remote client stuff */
			if (IS_RSC(next_shm_device)) {
				next_shm_device->dt.sc.message =
				    (long)message_location;
				tmp_message = (message_request_t *)(void *)
				    (base_address + (long)message_location);
				(void) mutex_init(&tmp_message->mutex,
				    USYNC_PROCESS, NULL);
				cond_init(&tmp_message->cond_i, USYNC_PROCESS,
				    NULL);
				cond_init(&tmp_message->cond_r, USYNC_PROCESS,
				    NULL);
				tmp_message->mtype = MESS_MT_VOID;
				message_location++;
			}
			/* does it need remote device stuff */
			if (IS_RSD(next_shm_device)) {
				next_shm_device->dt.sc.message =
				    (long)message_location;
				tmp_message = (message_request_t *)(void *)
				    (base_address + (long)message_location);
				(void) mutex_init(&tmp_message->mutex,
				    USYNC_PROCESS, NULL);
				cond_init(&tmp_message->cond_i, USYNC_PROCESS,
				    NULL);
				cond_init(&tmp_message->cond_r, USYNC_PROCESS,
				    NULL);
				tmp_message->mtype = MESS_MT_VOID;
				message_location++;
			}
			/* If its a device needing mode_sense data */
			if (is_scsi(next_shm_device) ||
			    (next_shm_device->type == DT_ROBOT)) {
				next_shm_device->sense = sense_location;
				sense_location = (sam_extended_sense_t *)
				    ((char *)sense_location + sen_len);
				next_shm_device->mode_sense =
				    mode_sense_location;
				mode_sense_location = (mode_sense_t *)(void *)
				    ((char *)mode_sense_location + ms_len);
			} else {
				next_shm_device->sense =
				    (sam_extended_sense_t *)0;
				next_shm_device->mode_sense = (mode_sense_t *)0;
			}

			/* assume it supports all pages */
			next_shm_device->pages = 0x3f;

		}

		next_shm_device++;
	}
	last_shm_device->next = NULL;
	free(last_device);		/* free last bit of memory */
}


static void
layout_preview_table(
	shm_preview_tbl_t *memory,
	sam_defaults_t *defaults)
{
	preview_t *entry;
	preview_tbl_t *preview;
	stage_preview_t *stages;
	dev_ent_t *un;
	prv_fs_ent_t *preview_fs;
	char	*ent_pnt = "layout_preview_table";
	uint_t	stage_start;
	int		fs_count = 0;
	int		i;

	preview = &memory->preview_table;

	/* show the preview segment full */
	memory->shm_block.left = 0;
	memory->shm_block.next_p = memory->shm_block.size - 1;

	/* initialize the mutex */
	if (mutex_init(&(preview->ptbl_mutex), USYNC_PROCESS, NULL) < 0) {
		sam_syslog(LOG_ERR, "%s: mutex_init:(%s:%d) %m", ent_pnt,
		    __FILE__, __LINE__);
		kill_off_threads();
		exit(EXIT_FAILURE);
	}
	preview->avail = defaults->previews;
	preview->ptbl_count = 0;
	for (entry = &preview->p[0], i = 0; i < preview->avail; entry++, i++) {
		if (mutex_init(&(entry->p_mutex), USYNC_PROCESS, NULL) < 0) {
			sam_syslog(LOG_ERR, "%s: mutex_init:(%s:%d) %m",
			    ent_pnt, __FILE__, __LINE__);
			kill_off_threads();
			exit(EXIT_FAILURE);
		}
		entry->prv_id = i;
	}

	/*
	 * Set up hwm_factor for all filesystem. For now the factor is
	 * hardcoded to PRV_[HL]WM_FACTOR_DEFAULT.
	 */
	preview->prv_age_factor = PRV_AGE_FACTOR_DEFAULT;
	preview->prv_fs_table = PRE_GET_OFFS(&preview->p[0]) +
	    defaults->previews * sizeof (preview_t);
	preview_fs = (prv_fs_ent_t *)PRE_REF_ADDR(preview->prv_fs_table);

	for (un = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
	    un != NULL; un = (dev_ent_t *)SHM_REF_ADDR(un->next)) {
		if (IS_FS(un)) {
			preview_fs->prv_fseq = un->eq;
			strcpy(preview_fs->prv_fsname, un->set);
			preview_fs++;
			fs_count++;
		}
	}
	preview->fs_count = fs_count;

	preview_fs = (prv_fs_ent_t *)PRE_REF_ADDR(preview->prv_fs_table);
	ReadPreviewCmd(preview, preview_fs);

	stage_start = preview->prv_fs_table + fs_count * sizeof (prv_fs_ent_t);
	if ((stage_start & 0x3) != 0) {
		/* force it to align on int */
		stage_start = stage_start + (4 - (stage_start & 0x3));
	}

	preview->stages = stage_start;
	stages = (stage_preview_t *)(void *)((char *)memory + stage_start);
	for (i = 0; i < (defaults->stages + 5); i++, stages++) {
		stage_start += sizeof (stage_preview_t);
		stages->next = stage_start;
	}
}
