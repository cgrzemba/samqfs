/*
 * odfile.c - support for logical files on optical devices
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.17 $"

static char *_SrcFile = __FILE__;

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <thread.h>
#include <synch.h>

#include "pub/devstat.h"
#include "sam/types.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/dev_log.h"
#include "aml/external_data.h"
#include "aml/historian.h"
#include "aml/shm.h"
#include "aml/odlabels.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"

/* Globals */

extern shm_alloc_t master_shm, preview_shm;

#define	FIND_FILE_LEN   8

/*
 * find_file - find the requested file on the specified device.
 *
 *   exit -
 *         0 - ok (mount_data filled in)
 *         !0 - error
 */

int
find_file(
	int open_fd,			/* open fd for the device */
	dev_ent_t *un,			/* device where media is mounted */
	sam_resource_t *resource)	/* resource record */
{
	uint16_t version;
	uint32_t h32;
	int err, resid, len, sector_cnt, trans_len, sectors_read, cur_ptoc;
	int match;
	char *buf;
	char scratch[40];
	sam_arch_rminfo_t *rm_info;
	sam_archive_t *archive;
	ls_bof1_label_t *bof;
	sam_extended_sense_t *sense;

	rm_info = &resource->archive.rm_info;
	archive = &resource->archive;
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* FIX - check dates */
	/* If reading and the file system thinks the data is good, return ok */
	if (rm_info->valid && resource->access != FWRITE) {
		return (0);
	}

	/* calc number of sectors to hold the BOF label */
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	len = sector_cnt * un->sector_size;
	/* read both BOF and EOF */
	trans_len = len * 2;

	buf = (char *)malloc_wait(trans_len, 3, 0);

	if (archive->mc.od.label_pda) {
		DevLog(DL_DETAIL(2094));
		cur_ptoc = archive->mc.od.label_pda;
	} else {
		cur_ptoc = un->dt.od.ptoc_fwa;
	}

	match = 0;

	while (cur_ptoc < un->dt.od.ptoc_lwa) {
		DevLog(DL_LABEL(2068), cur_ptoc, un->dt.od.ptoc_lwa);
		if ((cur_ptoc + FIND_FILE_LEN) > un->dt.od.ptoc_lwa) {
			int new_len = ((un->dt.od.ptoc_lwa - cur_ptoc) *
			    un->sector_size);

			if (new_len > trans_len) {
				free(buf);
				buf = (char *)malloc_wait(new_len, 3, 0);
			}
			trans_len = new_len;
		}
		sense->es_key = 0;
		/* Read the BOF/EOF data segment */
		DevLog(DL_LABEL(2084), cur_ptoc, trans_len);
		err = scsi_cmd(open_fd, un, READ, 0, buf, trans_len,
		    cur_ptoc, &resid);

		/* if blank and no transfer */
		if ((err == 0) && (sense->es_key == KEY_BLANK_CHECK)) {
			cur_ptoc++;
			continue;
		}
		/* If no data transfered, log error and try next sector */
		if (err <= 0) {
			if (sense->es_key == KEY_MEDIUM_ERROR) {
				DevLog(DL_ERR(2040));
			} else {
				DevLog(DL_ERR(2041));
			}
			DevLogSense(un);
			cur_ptoc++;
			continue;
		}
		if (err != trans_len) {
			DevLog(DL_DETAIL(2095), trans_len, err);
		}
		sectors_read = err / un->sector_size;
		if (sectors_read < sector_cnt) {
			DevLog(DL_DETAIL(2096), sectors_read, sector_cnt);
			break;
		}
		/* LINTED pointer cast may result in improper alignment */
		for (bof = (ls_bof1_label_t *)buf;
		    sectors_read > sector_cnt - 1;
		    sectors_read -= sector_cnt, cur_ptoc += sector_cnt,
		    bof = (ls_bof1_label_t *)(void *)(((char *)bof) + len)) {
			if (match == 0) {
				uint16_t h16;

				DevLog(DL_LABEL(2074), cur_ptoc);

				if (memcmp(bof->std_id, "WORM1", 5) != 0) {
					continue;
				}

				if (memcmp(bof->label_id, "BO", 2) != 0) {
					continue;
				}

				if (memcmp(bof->label_id, "ESA", 3) == 0) {
					break;
				}

				zfn(bof->file_id, scratch, 31);
				if (strncmp(scratch, archive->mc.od.file_id,
				    31) != 0) {
					continue;
				}

				BE16toH(&bof->version, &h16);
				if ((archive->mc.od.version != 0) &&
				    (h16 != archive->mc.od.version)) {
					continue;
				}

				zfn(bof->owner_id, scratch, 31);
				if (strncmp(scratch, resource->mc.od.owner_id,
				    31) != 0) {
					continue;
				}

				zfn(bof->group_id, scratch, 31);
				if (strncmp(scratch, resource->mc.od.group_id,
				    31) != 0) {
					continue;
				}

				DevLog(DL_LABEL(2075));
				match++;
				archive->mc.od.version = h16;
				archive->mc.od.label_pda = cur_ptoc;
				BE32toH(&bof->file_start, &h32);
				rm_info->position = h32;
				/* if no more labeles */
				if (sectors_read == sector_cnt)	{
					break;
				}
				continue;	/* jump to next buffer */
			}
			/* matching bof, check eof */

			DevLog(DL_LABEL(2072), cur_ptoc);
			err = 1;
			if (strncmp(bof->label_id, "EO", 2) == 0) {
				DevLog(DL_LABEL(2085));
				err = 0;
				zfn(bof->file_id, scratch, 31);
				if (strncmp(scratch, archive->mc.od.file_id,
				    31) != 0) {
					err++;
				}

				BE16toH(&bof->version, &version);
				if ((archive->mc.od.version != 0) &&
				    (version != archive->mc.od.version)) {
					err++;
				}

				zfn(bof->owner_id, scratch, 31);
				if (strncmp(scratch, resource->mc.od.owner_id,
				    31) != 0) {
					err++;
				}

				zfn(bof->group_id, scratch, 31);
				if (strncmp(scratch, resource->mc.od.group_id,
				    31) != 0) {
					err++;
				}
			}
			if (err == 0) {		/* found bof & matching eof */
				DevLog(DL_LABEL(2073));
				if (resource->access == FWRITE) {
					err = EEXIST;
					goto out;
				}
				BE32toH(&bof->byte_length, &h32);
				rm_info->size = h32;
			} else {		/* found bof, but no eof */
				DevLog(DL_ERR(2042), un->vsn,
				    archive->mc.od.file_id);
				err = EBADF;
			}
			goto out;
		}
	}

	err = ENOENT;

out:
	free(buf);

	/* Set up od info for new file, access mode = write. */

	if (resource->access == FWRITE) {	/* if write access */
		/* found previous version or no eof */
		if (err == EEXIST || err == EBADF) {
			archive->mc.od.version++;
			err = 0;
		} else if (err == ENOENT) {	/* no previous version  */
			archive->mc.od.version = 1;
			err = 0;
		}
		if (err == 0) {
			rm_info->position = un->dt.od.next_file_fwa;
			rm_info->mau = un->sector_size;
			rm_info->file_offset = 0;
			rm_info->size = 0;
			archive->mc.od.label_pda = 0;
		}
	}
	return (err);
}


/*
 * create_bof - Write the bof sector
 *
 * io_mutex should be aquired before calling create_bof.
 *
 * exit -
 *      0 - ok
 *     !0 - failed
 */

int
create_bof(
	int open_fd,			/* open fd for the device */
	dev_ent_t *un,			/* device entry (mutex held) */
	sam_resource_t *resource)	/* preview entry */
{
	uint16_t h16;
	uint32_t h32;
	int resid, len, attempts, cur_ptoc, half_len, sector_cnt;
	char *buf;
	time_t now;
	struct tm tm_time;
	ls_bof1_label_t *bof;
	sam_arch_rminfo_t *rm_info;
	sam_archive_t *archive;

	rm_info = &resource->archive.rm_info;
	archive = &resource->archive;

	/* calc number of sectors to hold the BOF label */
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	len = sector_cnt * un->sector_size;
	/* calc mid point of the BOF label (second half is empty) */
	half_len = sizeof (ls_bof1_label_t) / 2;

	buf = (char *)malloc_wait(len, 4, 0);

	/* carve out the space needed for the BOF and EOF */
	cur_ptoc = un->dt.od.ptoc_fwa - sector_cnt * 2;

	(void) memset(buf, ' ', half_len);
	(void) memset(buf + half_len, 0, half_len);
	/* LINTED pointer cast may result in improper alignment */
	bof = (ls_bof1_label_t *)buf;
	(void) memcpy(bof->std_id, "WORM1", 5);
	if (rm_info->process_eox) {
		(void) memcpy(bof->label_id, "BOV", 3);
		(void) strncpy(bof->next_vsn,
		    resource->next_vsn, sizeof (vsn_t) - 1);
	} else {
		(void) memcpy(bof->label_id, "BOF", 3);
	}
	bof->label_version = 1;
	(void) strncpy(bof->file_id, archive->mc.od.file_id, 31);
	h16 = archive->mc.od.version;
	HtoBE16(&h16, &bof->version);
	bof->security_level = 0;
	bof->append_iteration = 0;
	(void) strncpy(bof->owner_id, resource->mc.od.owner_id, 31);
	(void) strncpy(bof->group_id, resource->mc.od.group_id, 31);
	(void) memcpy(bof->system_id, "IDS", 3);
	(void) strncpy(bof->equipment_id,
	    sam_model[un->model_index].long_name, 31);

	/* Clear the number fields from creation date to byte 316.    */

	(void) memset(&bof->creation_date, 0, (316 - 249) + 1);

	now = time((time_t *)NULL);
	(void) localtime_r(&now, &tm_time);
	h16 = tm_time.tm_year;
	HtoBE16(&h16, &bof->creation_date.lt_year);
	bof->creation_date.lt_mon = tm_time.tm_mon + 1;
	bof->creation_date.lt_day = tm_time.tm_mday;
	bof->creation_date.lt_hour = tm_time.tm_hour;
	bof->creation_date.lt_min = tm_time.tm_min;
	bof->creation_date.lt_sec = tm_time.tm_sec;
	bof->creation_date.lt_gmtoff = timezone / 3600;
	h32 = un->dt.od.next_file_fwa;
	DevLog(DL_DETAIL(2091), un->dt.od.next_file_fwa);
	HtoBE32(&h32, &bof->file_start);
	h16 = 1;
	HtoBE16(&h16, &bof->fsn);
	bof->recording_method = 1;

	attempts = 0;
	DevLog(DL_LABEL(2082), cur_ptoc);
	while (un->dt.od.next_file_fwa < cur_ptoc &&
	    scsi_cmd(open_fd, un, WRITE, 0,
	    buf, len, cur_ptoc, &resid) != len && attempts != 6) {
		DevLog(DL_LABEL(2083), cur_ptoc);
		/* set ptoc fwa to last write */
		mutex_lock(&un->mutex);
		un->dt.od.ptoc_fwa = cur_ptoc;
		mutex_unlock(&un->mutex);
		/* move ahead and try the write again */
		cur_ptoc -= sector_cnt * 2;
		DevLog(DL_LABEL(2082), cur_ptoc);
		attempts++;
	}

	free(buf);
	if (attempts == 6 || cur_ptoc <= un->dt.od.next_file_fwa) {
		if (cur_ptoc < un->dt.od.next_file_fwa) {
			DevLog(DL_ERR(2092), un->dt.od.next_file_fwa, cur_ptoc);
		}
		DevLog(DL_ERR(2083));
		return (EIO);
	}

	/* set ptoc fwa to last write */
	mutex_lock(&un->mutex);
	un->dt.od.ptoc_fwa = cur_ptoc;
	mutex_unlock(&un->mutex);
	rm_info->media = un->type;
	rm_info->bof_written = 1;
	rm_info->size = 0;
	rm_info->mau = un->sector_size;
	archive->mc.od.label_pda = cur_ptoc;
	return (0);
}

/*
 * create_optic_eof - create eof label.
 *
 * io_mutex should be aquired before calling create_optic_eof.
 *
 * exit -
 *      0 - ok
 *     !0 - not ok
 */

int
create_optic_eof(
	int open_fd,			/* open fd for the device */
	dev_ent_t *un,			/* device entry (mutex held) */
	sam_resource_t *resource)	/* resource record */
{
	uint16_t h16;
	uint32_t h32, file_size;
	int err, resid, len, attempts, cur_ptoc, fs_alloc_bytes;
	int daddr, sector_cnt;
	uint_t new_next_file_fwa;
	char *buf;
	char scratch[40];
	time_t now;
	struct tm tm_time;
	sam_arch_rminfo_t *rm_info;
	sam_archive_t *archive;
	ls_bof1_label_t *bof, *eof;

	rm_info = &resource->archive.rm_info;
	archive = &resource->archive;

	/* calc number of sectors to hold the BOF label */
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	/* length to store a BOF/EOF */
	len = sector_cnt * un->sector_size;

	/* Number of bytes per file system allocation unit */
	fs_alloc_bytes = un->sector_size * un->dt.od.fs_alloc;

	/* alloc memory for both BOF and EOF */
	buf = (char *)malloc_wait(len * 2, 4, 0);
	cur_ptoc = archive->mc.od.label_pda;

	/* Read the BOF for this segment */
	DevLog(DL_LABEL(2074), cur_ptoc);
	if (scsi_cmd(open_fd, un, READ, 0, buf, len, cur_ptoc, &resid) != len) {
		DevLogSense(un);
		DevLog(DL_ERR(2077));
		set_bad_media(un);
		un->space = 0;
		free(buf);
		return (EIO);
	}
	/* LINTED pointer cast may result in improper alignment */
	bof = (ls_bof1_label_t *)buf;
	/* LINTED pointer cast may result in improper alignment */
	eof = (ls_bof1_label_t *)(buf + len);
	/* now copy the BOF data into the EOF to initialize it */
	(void) memcpy(eof, bof, len);
	if (rm_info->process_eox) {
		(void) memcpy(eof->label_id, "EOV", 3);
		(void) strncpy(eof->next_vsn,
		    resource->prev_vsn, sizeof (vsn_t) - 1);
	} else {
		(void) memcpy(eof->label_id, "EOF", 3);
	}

	(void) time(&now);
	(void) localtime_r(&now, &tm_time);
	h16 = tm_time.tm_year;
	HtoBE16(&h16, &eof->creation_date.lt_year);
	eof->creation_date.lt_mon = tm_time.tm_mon + 1;
	eof->creation_date.lt_day = tm_time.tm_mday;
	eof->creation_date.lt_hour = tm_time.tm_hour;
	eof->creation_date.lt_min = tm_time.tm_min;
	eof->creation_date.lt_sec = tm_time.tm_sec;
	eof->creation_date.lt_gmtoff = timezone / 3600;


	/* Do a sanity check on the BOF we just read */
	err = 1;
	if (strncmp(bof->label_id, "BO", 2) == 0) {
		err = 0;
		zfn(bof->file_id, scratch, 31);
		if (strncmp(scratch, archive->mc.od.file_id, 31) != 0) {
			err++;
		}

		BE16toH(&bof->version, &h16);
		if ((archive->mc.od.version != 0) &&
		    (h16 != archive->mc.od.version)) {
			err++;
		}

		zfn(bof->owner_id, scratch, 31);
		if (strncmp(scratch, resource->mc.od.owner_id, 31) != 0) {
			err++;
		}

		zfn(bof->group_id, scratch, 31);
		if (strncmp(scratch, resource->mc.od.group_id, 31) != 0) {
			err++;
		}
	}
	if (err) {			/* something is very wrong  */
		DevLog(DL_ERR(2077));
		set_bad_media(un);
		un->space = 0;
		free(buf);
		return (EBADF);
	}
	/* BOF was valid */
	DevLog(DL_LABEL(2075));

	h32 = rm_info->size;
	HtoBE32(&h32, &eof->byte_length);
	/*
	 * size of the file rounded to full next allocation units in pdus.
	 */
	file_size = ((rm_info->size + fs_alloc_bytes - 1) / fs_alloc_bytes) *
	    un->dt.od.fs_alloc;
	HtoBE32(&file_size, &eof->file_size);
	new_next_file_fwa = rm_info->position + file_size + 1;

	attempts = 0;
	/* Write an EOF at position BOF+1 (units in sectors) */
	DevLog(DL_LABEL(2070), cur_ptoc + sector_cnt);
	if (scsi_cmd(open_fd, un, WRITE, 0, eof,
	    len, cur_ptoc + sector_cnt, &resid) != len) {
		attempts = 1;
		DevLog(DL_LABEL(2071));
		cur_ptoc -= 2 * sector_cnt;
		DevLog(DL_LABEL(2070), cur_ptoc);
		/* attempt to write both pairs again */
		while (new_next_file_fwa < cur_ptoc &&
		    scsi_cmd(open_fd, un, WRITE, 0,
		    buf, len * 2, cur_ptoc, &resid) != len * 2 &&
		    attempts != 6) {
			DevLog(DL_LABEL(2071));
			/* set ptoc fwa to last write */
			mutex_lock(&un->mutex);
			un->dt.od.ptoc_fwa = cur_ptoc;
			mutex_unlock(&un->mutex);
			cur_ptoc -= 2 * sector_cnt;
			attempts++;
			DevLog(DL_LABEL(2070), cur_ptoc);
		}
	}
	if (attempts == 6 || cur_ptoc < new_next_file_fwa) {
		free(buf);
		un->space = 0;
		if (cur_ptoc < new_next_file_fwa) {
			DevLog(DL_ERR(2092), new_next_file_fwa, cur_ptoc);
		}
		DevLog(DL_ERR(2071));
		return (EIO);
	}
	/* EOF was successfully written */

	/* set ptoc fwa to last write  */

	archive->mc.od.label_pda = cur_ptoc;

	mutex_lock(&un->mutex);
	un->dt.od.ptoc_fwa = cur_ptoc;
	un->dt.od.next_file_fwa = new_next_file_fwa;
	DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
	un->space = (cur_ptoc > un->dt.od.next_file_fwa) ?
	    compute_size_from_sectors(un,
	    cur_ptoc - un->dt.od.next_file_fwa) : 0;
	mutex_unlock(&un->mutex);

	/*
	 * update ptoc fwa on read write media and blank the sector
	 * after the file.
	 *
	 * Hitachi DVD-RAM drive does not support explict scsi erase command.
	 */
	if (un->dt.od.medium_type == MEDIA_RW && un->model != M_HITACHI) {
		(void) scsi_cmd(open_fd, un, SCMD_EXTENDED_ERASE, 0,
		    un->dt.od.next_file_fwa - 1, 1);
		(void) memset(buf, 0, un->sector_size);
		h32 = un->dt.od.ptoc_fwa;
		buf[0] = (h32 >> 24);
		buf[1] = (h32 >> 16);
		buf[2] = (h32 >> 8);
		buf[3] = h32;

		/* also used in scan.c */
		daddr = GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS - 1;

		if (scsi_cmd(open_fd, un, WRITE, 0,
		    buf, un->sector_size, daddr, (int *)NULL) < 0) {

			mutex_lock(&un->mutex);
			un->space = 0;
			mutex_unlock(&un->mutex);
			DevLog(DL_ERR(2043));
		}
	}
	free(buf);
	return (0);
}
