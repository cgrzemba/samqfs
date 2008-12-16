/*
 * optic.c - common routines for optical label management
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

#pragma ident "$Revision: 1.27 $"

static char    *_SrcFile = __FILE__;

#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <thread.h>
#include <synch.h>

#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/scsi.h"
#include "aml/external_data.h"
#include "aml/shm.h"
#include "aml/mode_sense.h"
#include "sam/devinfo.h"
#include "aml/odlabels.h"
#include "aml/historian.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "driver/samst_def.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* globals */


extern shm_alloc_t master_shm, preview_shm;

/* Function prototypes */

int close_file(int, dev_ent_t *, char *);
int get_mode_sense(int, dev_ent_t *);
time_t get_label_time(dklabel_timestamp_t *);
void find_ptoc(int, struct dev_ent *, int);
void get_ptoc(int, struct dev_ent *);
void ansi_label(int, dev_ent_t *, sam_extended_sense_t *, dkanchor_label_t *);


/*
 * bit_shift - calculate bit shift for a given block size.
 * Note: same function exists is fs/rm.c
 */
int
bit_shift(
	uint_t block_size)	/* block size to compute bit shift for */
{
	uint_t b_shift;

	/*
	 * if block size is 0, assuming its 1024
	 *
	 * Note:  This was done because older code didn't always set the mau
	 * for optical media because it was assumed to be 1024 sector size
	 */
	if (block_size == 0)
		return (10);

	/* work up the powers of 2 to find the bit shift to match block size */
	b_shift = 1;
	while (block_size ^ (1 << b_shift) && b_shift < 30) {
		b_shift++;
	}
	return (b_shift);
}



/*
 * - compute sectors from size
 *
 * Returns the number of sectors based on the media size and sector size. Note:
 * size is assumed to be in 1024 byte blocks
 *
 *
 * Entry -
 * un - *device_entry
 * size - media size (usually un->capacity)
 */

offset_t
compute_sectors_from_size(dev_ent_t *un, offset_t size)
{
	offset_t sector_cnt = size;

	if (un->sector_size != 1024) {
		/* size is always in 1024, so calc shift required from there */
		int bshift = 10 - bit_shift(un->sector_size);

		if (bshift < 0)
			sector_cnt = size >> (abs(bshift));
		else
			sector_cnt = size << bshift;
	}
	return (sector_cnt);
}

/*
 * - compute size from sectors
 *
 * Returns the media size based on the number of sectors and sector size. Note:
 * returned size will be in 1024 byte blocks
 *
 * Entry -
 * un - *device_entry
 * sector_cnt - number of sectors (usually ptoc fwa)
 *
 */
offset_t
compute_size_from_sectors(dev_ent_t *un, offset_t sector_cnt)
{
	offset_t size = sector_cnt;

	if (un->sector_size != 1024) {
		/* size is always in 1024, so calc shift required from there */
		int bshift = 10 - bit_shift(un->sector_size);

		if (bshift < 0)
			size = sector_cnt << (abs(bshift));
		else
			size = sector_cnt >> bshift;
	}
	return (size);
}



/*
 * get_capacity - get capacity and sector size of optical disk
 *
 * Sets the capacity of the inserted media in the capacity field of the
 * device_entry and the sector size in the sector_size field.
 *
 * Entry -
 * fd - open file descriptor
 * un - *device_entry
 *
 * Returns  0 - success,  -1 - error
 */

int
get_capacity(int fd, dev_ent_t *un)
{
	int 		resid, count = 0;
	int		retry;
	char		*d_mess = un->dis_mes[DIS_MES_NORM];
	generic_blk_desc_t	*bdp;
	sam_scsi_capacity_t	cap;
	sam_extended_sense_t	*sp =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	union {
		ld4100_mode_sense_t *ld4100_ms;
		ld4500_mode_sense_t *ld4500_ms;
		multim_mode_sense_t *ibm0632_ms;
		hp1716_mode_sense_t *hp1716_ms;
		mode_sense_t   *msp;
	} msu;

	msu.msp = (mode_sense_t *)SHM_REF_ADDR(un->mode_sense);
	retry = 0;
	DevLog(DL_DETAIL(2012));
again:
	(void) memccpy(d_mess,
	    catgets(catfd, SET, 851, "determine capacity"), '\0', DIS_MES_LEN);
	(void) memset(&cap, 0, sizeof (sam_scsi_capacity_t));
	sp->es_key = 0;
	if (scsi_cmd(fd, un, SCMD_READ_CAPACITY, 0, &cap, sizeof (cap),
	    &resid) != sizeof (cap) || sp->es_key != 0) {

		if (sp->es_key == KEY_MEDIUM_ERROR && retry < 3) {
			DevLog(DL_RETRY(2013), retry);
			retry++;
			(void) sleep(4);
			goto again;
		}
		DevLog(DL_ERR(2014));
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 2007, "Read capacity failed"),
		    '\0', DIS_MES_LEN);
		DevLogSense(un);
		return (-1);
	}
	un->capacity = ((cap.blk_addr_3 << 24) + (cap.blk_addr_2 << 16)
	    + (cap.blk_addr_1 << 8) + cap.blk_addr_0);

	if (get_mode_sense(fd, un)) {
		DevLog(DL_ERR(2015));
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 1673, "Mode sense failed"),
		    '\0', DIS_MES_LEN);
		return (-1);
	}
	bdp = &msu.msp->u.generic_ms.blk_desc;
	un->sector_size = ((bdp->blk_len_high << 16) +
	    (bdp->blk_len_mid << 8) + bdp->blk_len_low);

	switch (un->model) {
	case M_LMS4100:
		/* FALLTHRU */
	case M_LMS4500:
		un->status.b.write_protect = msu.ld4100_ms->hdr.WP;
		un->dt.od.medium_type = MEDIA_WORM;
		break;

	case M_IBM0632:
		un->status.b.write_protect = msu.ibm0632_ms->hdr.WP;
		if ((un->dt.od.medium_type = msu.ibm0632_ms->hdr.medium_type)
		    == 0) {

			if (count++)
				break;
			DevLog(DL_RETRY(2016));
			goto again;
		}
		if (un->dt.od.medium_type == MEDIA_INVALID) {
			DevLog(DL_ERR(2017));
			(void) memccpy(d_mess,
			    catgets(catfd, SET, 1416, "Invalid media"),
			    '\0', DIS_MES_LEN);
			return (-1);
		}
		/*
		 * make sure sector size is reasonable -- must be between 512
		 * and 16K
		 */
		if (un->sector_size < MIN_OPTICAL_SECTOR_SIZE ||
		    un->sector_size > MAX_OPTICAL_SECTOR_SIZE) {
			DevLog(DL_ERR(2018), un->sector_size);
			return (-1);
		}
		/* finally, set the samst block size for this device to match */
		DevLog(DL_DETAIL(2080), un->sector_size);
		if (ioctl(fd, SAMSTIOC_SETBLK, &un->sector_size)) {
			DevLog(DL_SYSERR(2081));
			return (-1);
		}
		if (un->dt.od.medium_type == MEDIA_WORM)
			un->type = DT_WORM_OPTICAL;
		else if (un->dt.od.medium_type == MEDIA_RW)
			un->type = DT_ERASABLE;
		break;

	case M_HITACHI:
	case M_HPC1716:
	case M_PLASMON_UDO:

		un->status.b.write_protect = msu.hp1716_ms->hdr.WP;
		un->dt.od.medium_type = msu.hp1716_ms->hdr.medium_type;
		if (un->dt.od.medium_type == 0 ||
		    (un->model == M_HITACHI && un->dt.od.medium_type == 0x41)) {
			if (count++) {
				un->dt.od.medium_type = MEDIA_RW;
			} else {
				DevLog(DL_RETRY(2016));
				goto again;
			}
		}
		if (un->dt.od.medium_type == MEDIA_INVALID) {
			DevLog(DL_ERR(2017));
			(void) memccpy(d_mess,
			    catgets(catfd, SET, 1416, "Invalid media"),
			    '\0', DIS_MES_LEN);
			return (-1);
		}
		/*
		 * make sure sector size is reasonable -- must be between 512
		 * and 16K
		 */
		if (un->sector_size < MIN_OPTICAL_SECTOR_SIZE ||
		    un->sector_size > MAX_OPTICAL_SECTOR_SIZE) {
			DevLog(DL_ERR(2018), un->sector_size);
			return (-1);
		}
		/* finally, set the samst block size for this device to match */
		DevLog(DL_DETAIL(2080), un->sector_size);
		if (ioctl(fd, SAMSTIOC_SETBLK, &un->sector_size)) {
			DevLog(DL_SYSERR(2081));
			return (-1);
		}
		break;


	default:
		break;
	}

	/* recalc capacity based on sector size (minus 512 labeling sectors) */
	{
		offset_t tmp;
		tmp = un->capacity - OD_SPACE_UNAVAIL;
		tmp = compute_size_from_sectors(un, tmp);
		un->capacity = tmp;
	}

	DevLog(DL_DETAIL(2019), un->dt.od.medium_type,
	    (long long) 1024 * un->capacity, un->sector_size,
	    un->status.b.write_protect);
	un->dt.od.fs_alloc = PAGESIZE / un->sector_size;
	return (0);
}


/*
 * process_optic_labels - Process scan optical labels.
 *
 * Description: Process the scanned optical disk unit for labels.
 * On entry:
 * fd - open file descriptor
 * un - dev_ent pointer
 *
 */


void
process_optic_labels(int fd, dev_ent_t *un)
{
	int	err, resid, ii, anchor_loc[2], looks_ansi = 0;
	int	len;
	char	*d_mess = un->dis_mes[DIS_MES_NORM];
	dkanchor_label_t	*ap;
	sam_extended_sense_t	*sp;

	(void) memccpy(d_mess, catgets(catfd, SET, 1972, "process labels"),
	    '\0', DIS_MES_LEN);
	len = un->sector_size;
	ap = (dkanchor_label_t *)malloc_wait(len, 5, 0);
	sp = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* Read anchor sector */

	anchor_loc[0] = GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS;
	anchor_loc[1] = OD_ANCHOR_POS;
	for (ii = 0; ii < 2; ii++) {
		int retry = 3;

		sp->es_key = 0;
		/* locations are flipped on the read */
		DevLog(DL_DETAIL(2006), (ii == 0) ? 2 : 1, anchor_loc[ii]);
		err = scsi_cmd(fd, un, READ, 0, ap, len, anchor_loc[ii],
		    &resid);
		while (sp->es_key == KEY_MEDIUM_ERROR && --retry) {
			DevLog(DL_RETRY(2020), retry);
			(void) sleep(4);
			sp->es_key = 0;
			err = scsi_cmd(fd, un, READ, 0, ap, len, anchor_loc[ii],
			    &resid);
		}
		if (err <= 0)
			DevLog(DL_DETAIL(2007), (ii == 0) ? 2 : 1);

		if (sp->es_key == KEY_BLANK_CHECK)	/* if empty */
			continue;

		if (err > 0) {
			if (vfyansi_label((void *) ap, ANCHOR_VOL_DES_P) == 0) {
				looks_ansi = 1;
				break;	/* looks ansi to me */
			}
		/* if not empty */
		} else if (sp->es_key != KEY_BLANK_CHECK) {
			DevLog(DL_ERR(2021));
			un->status.b.scan_err = TRUE;
			(void) memccpy(d_mess, catgets(catfd, SET, 1033,
			    "Error during label scan"), '\0', DIS_MES_LEN);
			DevLogSense(un);
			free(ap);
			return;
		}
	}

	if (looks_ansi)
		ansi_label(fd, un, sp, ap);	/* process the labels */

	if (un->status.b.labeled) {
		samst_range_t	ranges;
		char		*msg_buf, *msg1;
		int		reserve_sectors = 12 *
		    SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));

		ranges.low_bn = un->dt.od.next_file_fwa;
		ranges.high_bn = un->dt.od.ptoc_fwa - reserve_sectors;
		if (ranges.high_bn < ranges.low_bn)
			ranges.high_bn = ranges.low_bn = SAMST_RANGE_NOWRITE;
		msg1 = catgets(catfd, SET, 1487, "labeled - range %#llx-%llx");
		msg_buf = malloc_wait(strlen(msg1) + 24, 2, 0);
		(void) sprintf(msg_buf, msg1, ranges.low_bn, ranges.high_bn);
		(void) memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
		free(msg_buf);
		DevLog(DL_DETAIL(2022), ranges.low_bn, ranges.high_bn);
		if (ioctl(fd, SAMSTIOC_RANGE, &ranges))
			DevLog(DL_SYSERR(2049));
	}
	free(ap);
}

/*
 * vfyansi_label - Verify ansi label.
 *
 * Description: Process the ansi volume sequence labels.
 *
 * On entry:
 * lp - The generic label pointer
 * label_type - type of label.
 *
 * Returns: 0 = successful, 1 = failed
 */
int
vfyansi_label(void *ilp, int label_type)
{
	char		*cp;
	uchar_t		len = 0;
	ushort_t	ii, id;
	dklabel_tag_t 	*lp = (dklabel_tag_t *)ilp;

	cp = (char *)lp;
	for (ii = 1; ii <= 16; cp++, ii++)
		if (ii < 5 || ii > 6)
			len += *cp;

	LE16toH(&lp->identifier, &id);

	if ((lp->checksum == len) && (id == label_type))
		return (0);

	return (1);
}

/*
 * ansi_label - Process ansi volume sequence.
 *
 * Description: Process the ansi volume sequence labels.
 *
 * On entry:
 * un = The unit table pointer
 * sp = The scsi sense pointer
 * ap = The anchor descriptor pointer.
 *
 * Returns:
 */
void
ansi_label(int fd, dev_ent_t *un, sam_extended_sense_t *sp,
	dkanchor_label_t *ap)
{
	uint32_t	location[2], lvar;
	int		i, ii, len, buffer_offset;
	int		err, resid;
	int		labeled = 0;
	char		*buf, *buf_fwa, *cp;
	time_t		label_time;
	dkpri_label_t	*priv;
	dkpart_label_t	*parp;

	LE32toH(&ap->reserve_volume_descriptor.location, &location[0]);
	LE32toH(&ap->main_volume_descriptor.location, &location[1]);

	cp = (char *)un->vsn;
	if (un->type == DT_ERASABLE || un->type == DT_PLASMON_UDO)
		len = un->sector_size * 2;
	else
		len = un->sector_size * VOL_SEQ_LEN;

	buf = (char *)malloc_wait(len, 5, 0);
	buf_fwa = buf;

	for (i = 0; i < 2; i++) {
		sp->es_key = 0;
		if (location[i] > GET_TOTAL_SECTORS(un))
			continue;

		(void) memset(buf_fwa, 0, 256);
		/* locations are flipped on the read */
		DevLog(DL_LABEL(2060), (i == 0) ? 2 : 1, location[i]);
		err = scsi_cmd(fd, un, READ, 0, buf_fwa, len,
		    location[i], &resid);
		/* if error or first empty sector */
		if ((err < 0) || (resid == len)) {
			DevLog(DL_LABEL(2061), (i == 0) ? 2 : 1);
			if (sp->es_key != 0) {
				DevLog(DL_ERR(2023));
				DevLogSense(un);
			}
			if (i == 0)
				continue;

			un->status.b.scan_err = TRUE;
			free(buf_fwa);
			return;
		}
		ii = err / un->sector_size;
		buffer_offset = 0;
		buf = buf_fwa;
		while (ii) {
			priv = (void *)buf;
			parp = (void *)buf;
			if (vfyansi_label((void *)priv, PRIMARY_VOL_DES) == 0) {
				(void) memcpy(cp, priv->volume_id,
				    sizeof (vsn_t) - 1);
				cp[sizeof (vsn_t) - 1] = '\0';
				label_time = get_label_time(
				    &priv->recording_date_time);
				un->label_address = location[i] + buffer_offset;
			} else if (vfyansi_label((void *) parp, PARTITION_DES)
			    == 0) {

				un->space = 0;
				LE32toH(&parp->starting_location,
				    &un->dt.od.next_file_fwa);
				DevLog(DL_DETAIL(2091),
				    un->dt.od.next_file_fwa);
				LE32toH(&parp->length, &lvar);
				un->dt.od.ptoc_lwa = un->dt.od.next_file_fwa
				    + lvar;
				labeled = 1;
			}
			buf += un->sector_size;
			ii--;
			buffer_offset++;
		}
		if (labeled)
			break;
	}

	free(buf_fwa);
	if (labeled) {	/* find the ptoc fwa */
		if (un->dt.od.medium_type == MEDIA_RW)
			get_ptoc(fd, un);	/* Erasable media */
		else {		/* WORM media */
			un->status.b.scan_err = TRUE;
			if (un->fseq) {	/* if robot device */
				un->status.b.scan_err = FALSE;
				/* look in the catalog */
				find_ptoc(fd, un, 0);
			}
			if (un->status.b.scan_err) {
				un->status.b.scan_err = FALSE;
				/* find the hard way */
				find_ptoc(fd, un, 1);
			}
		}
		if (!un->status.b.scan_err && un->status.b.ready) {
			char timestr[24];
			struct tm local_time;

			un->status.b.labeled = TRUE;
			un->label_time = label_time;
			(void) localtime_r(&un->label_time, &local_time);
			(void) strftime(timestr, sizeof (timestr) - 1,
			    "%Y/%m/%d %H:%M:%S", &local_time);
			DevLog(DL_ALL(2024), un->vsn, timestr);
		}
	} else {
		/*
		 * May have thought we found a valid primary volume
		 * descriptor and filled in a VSN but didn't find a partition
		 * descriptor so media does not have a valid ANSI label.
		 * Remove the VSN that was saved.  This case occurs when
		 * reading DVD-RAM media that contain a UDS format label.
		 */
		un->label_address = 0;
	}
}


/*
 * get_ptoc - Get partition table of contents.
 *
 * Description: Get the partition table of contents from high water mark sector
 * (Erasable media).
 *
 * On entry:
 * fd - open file descriptor
 * un - pointer to the device entry
 *
 */
void
get_ptoc(int fd, dev_ent_t *un)
{
	uint8_t		*buf;
	uint32_t	h32;
	int		len, sector_cnt;
	int		retry;
	int		daddr, resid;
	char		*d_mess = un->dis_mes[DIS_MES_NORM];
	ls_bof1_label_t *bof;
	ls_bof1_label_t *eof;
	sam_extended_sense_t *sp =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* calc number of sectors to hold the BOF label */
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	len = sector_cnt * un->sector_size;
	/* storage for both BOF and EOF */
	buf = (uint8_t *)malloc_wait(len * 2, 5, 0);
	daddr = GET_TOTAL_SECTORS(un) - 257;	/* also used in odfile.c */
	retry = 3;
	sp->es_key = 0;
	DevLog(DL_LABEL(2065), (long)daddr);
	while (scsi_cmd(fd, un, READ, 0, buf, len, daddr, &resid) < 0 ||
	    (sp->es_key != 0 && --retry)) {

		DevLog(DL_LABEL(2066));
		if (sp->es_key == KEY_MEDIUM_ERROR) {
			DevLog(DL_ERR(2025), retry);
			(void) sleep(4);
			sp->es_key = 0;
			continue;
		}
		un->status.b.scan_err = TRUE;
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 1033, "Error during label scan"),
		    '\0', DIS_MES_LEN);
		DevLog(DL_ERR(2044));
		DevLogSense(un);
		free(buf);
		return;
	}

	/* LINTED pointer cast may result in improper alignment */
	BE32toH((uint32_t *)buf, &h32);
	DevLog(DL_LABEL(2067), h32);
	if ((h32 <= OD_ANCHOR_POS) ||
	    (h32 >= (GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS))) {

		un->status.b.scan_err = TRUE;
		DevLog(DL_ERR(2026), h32,
		    GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS);
		(void) memccpy(d_mess,
		    catgets(catfd, SET, 1033, "Error during label scan"),
		    '\0', DIS_MES_LEN);
		free(buf);
		return;
	}
	un->dt.od.ptoc_fwa = h32;
	retry = 3;
	while (un->dt.od.ptoc_fwa < un->dt.od.ptoc_lwa) {
		DevLog(DL_LABEL(2068), un->dt.od.ptoc_fwa, un->dt.od.ptoc_lwa);
		sp->es_key = 0;
		(void) memset(buf, 0, 16);
		(void) memset(buf + len, 0, 16);
		/* read both BOF and EOF */
		if (scsi_cmd(fd, un, READ, 0, buf, len * 2, un->dt.od.ptoc_fwa,
		    &resid) < len) {
			/* if we didn't even read one label, bail out */
			if (sp->es_key != KEY_RECOVERABLE_ERROR &&
			    sp->es_key != KEY_BLANK_CHECK && --retry) {
				if (sp->es_key == KEY_MEDIUM_ERROR) {
					DevLog(DL_RETRY(2027), retry);
					(void) sleep(4);
					continue;
				}
				un->status.b.scan_err = TRUE;
				(void) memccpy(d_mess, catgets(catfd, SET, 1033,
				    "Error during label scan"),
				    '\0', DIS_MES_LEN);
				DevLogSense(un);
				DevLog(DL_ERR(2028));
				goto out;
			}
		}
		bof = (void *)buf;
		if (memcmp(&bof->label_id, "BO", 2) == 0) {	/* If label */
			BE32toH(&bof->file_start, &h32);
			un->dt.od.next_file_fwa = h32;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
			eof = (void *)(buf + len);

			/* If file is not closed, skip it */
			if (memcmp(&eof->label_id, "EO", 2) != 0) {
				un->dt.od.ptoc_fwa += 2 * sector_cnt;
				DevLog(DL_DETAIL(2093));
				continue;
			}
			BE32toH(&eof->file_start, &h32);
			un->dt.od.next_file_fwa = h32;
			BE32toH(&eof->file_size, &h32);
			un->dt.od.next_file_fwa += h32 + 1;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
			break;
		} else {	/* not a label */
			un->status.b.scan_err = TRUE;
			(void) memccpy(d_mess,
			    catgets(catfd, SET, 1033,
			    "Error during label scan"), '\0', DIS_MES_LEN);
			DevLogSense(un);
			DevLog(DL_ERR(2029));
			goto out;
		}
	}
	un->space = compute_size_from_sectors(un,
	    un->dt.od.ptoc_fwa - un->dt.od.next_file_fwa);

out:
	free(buf);
}

/*
 * find_ptoc - Find partition table of contents.
 *
 * Description: Find the partition table of contents.
 *
 * On entry:
 * fd - Open file descriptor
 * un - pointer to the device entry
 * flag - 0 if use catalog ptoc_fwa, 1 if full search.
 *
 */
void
find_ptoc(int fd, dev_ent_t *un, int flag)
{
	uint32_t	h32;
	int		resid, ii;
	int		sector_cnt, label_len, current, cur_lwa;
	char		*buf, *d_mess = un->dis_mes[DIS_MES_NORM];
	ls_bof1_label_t *bof;
	ls_bof1_label_t *eof;
	sam_extended_sense_t *sp =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* Find the start of the partition table of contents. */

	/* calc number of sectors to hold the BOF label */
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	label_len = sector_cnt * un->sector_size;
	/* storage for both BOF and EOF */
	buf = (char *)malloc_wait(label_len * 2, 5, 0);

	DevLog(DL_DETAIL(2028));
	/* Get PTOC from catalog entry */

	if ((flag == 0) && (un->slot != ROBOT_NO_SLOT) && un->fseq) {
		struct CatalogEntry ced;
		struct CatalogEntry *ce;

		ce = CatalogGetCeByLoc(un->fseq, un->slot, un->i.ViPart, &ced);
		if (ce != NULL) {
			if ((ce->m.CePtocFwa != 0) &&
			    (un->dt.od.ptoc_lwa != ce->m.CePtocFwa)) {

				un->dt.od.ptoc_fwa = ce->m.CePtocFwa - 5;
				goto verify;
			}
		}
	}
	/* read the PTOC value first */
	sp->es_key = 0;
	DevLog(DL_DETAIL(2065), un->dt.od.ptoc_lwa - 2);
	if (scsi_cmd(fd, un, READ, 0, buf, un->sector_size,
	    un->dt.od.ptoc_lwa - 2, &resid) != un->sector_size &&
	    sp->es_key != KEY_BLANK_CHECK) {

		DevLog(DL_ERR(2030));
		goto scan_error;
	}
	if (sp->es_key == KEY_BLANK_CHECK) {	/* if no files */
		free(buf);
		un->dt.od.ptoc_fwa = un->dt.od.ptoc_lwa;
		un->space = compute_size_from_sectors(un,
		    un->dt.od.ptoc_fwa - un->dt.od.next_file_fwa);
		return;
	}
	/* Find the lower bound by skipping back 1024 sectors and */
	/* reading 1 sector until an empty or file data sector is found */

	for (un->dt.od.ptoc_fwa = un->dt.od.ptoc_lwa - 1024;
	    un->dt.od.ptoc_fwa >= un->dt.od.next_file_fwa;
	    un->dt.od.ptoc_fwa -= 1024) {

		(void) memset(buf, 0, 6);
		sp->es_key = 0;
		DevLog(DL_DETAIL(2065), un->dt.od.ptoc_fwa);
		if ((scsi_cmd(fd, un, READ, 0, buf, un->sector_size,
		    un->dt.od.ptoc_fwa, &resid) == un->sector_size) ||
		    (sp->es_key == KEY_BLANK_CHECK)) {

			/* if empty then we have upper */
			if ((sp->es_key == KEY_BLANK_CHECK) ||
				/* if file data */
			    (memcmp(buf + 1, "WORM1", 5) != 0)) {
				break;	/* exit for loop */
			}
			continue;	/* try next -1024 */
		}
		DevLog(DL_ERR(2031));
		goto scan_error;	/* if any other error, bail.. */
	}

	/*
	 * Use a binary search to locate the start of the PTOC to
	 * within 32 sectors.
	 */

	cur_lwa = un->dt.od.ptoc_fwa + 1024;
	/* Keep halving the position -- doing a binary search */
	for (ii = 512; ii >= 32; ii = (ii >> 1)) {
		current = cur_lwa - ii;
		(void) memset(buf, 0, 6);
		sp->es_key = 0;
		DevLog(DL_DETAIL(2065), current);
		if ((scsi_cmd(fd, un, READ, 0, buf, un->sector_size,
		    current, &resid) == un->sector_size) ||
		    (sp->es_key == KEY_BLANK_CHECK)) {
			/* if label */
			if (memcmp(buf + 1, "WORM1", 5) == 0)
				cur_lwa = current;
			else
				un->dt.od.ptoc_fwa = current;
			continue;
		}
		DevLog(DL_ERR(2032));
		goto scan_error;
	}

	/*
	 * Read last 32 sectors 1 at a time to find first label sector. If in
	 * a robot, read 5 sectors.
	 */
verify:
	for (ii = 0; ii < 32; un->dt.od.ptoc_fwa++, ii++) {
		(void) memset(buf, 0, 6);
		sp->es_key = 0;
		/* attempt to read the BOF */
		DevLog(DL_LABEL(2074), un->dt.od.ptoc_fwa);
		if (scsi_cmd(fd, un, READ, 0, buf, label_len,
		    un->dt.od.ptoc_fwa, &resid)
		    != label_len && sp->es_key != KEY_BLANK_CHECK) {

			if (sp->es_key > 1) {	/* if not recoverable error */
				DevLog(DL_ERR(2033));
				goto scan_error;
			}
		}
		if (sp->es_key == KEY_BLANK_CHECK || resid != 0)
			continue;	/* no data */

		/* LINTED pointer cast may result in improper alignment */
		bof = (ls_bof1_label_t *)buf;
		if (memcmp(&bof->label_id, "BOF", 3) == 0) {	/* if label */
			BE32toH(&bof->file_start, &h32);
			un->dt.od.next_file_fwa = h32;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
			eof = (void *)(buf + label_len);
			sp->es_key = 0;
			DevLog(DL_LABEL(2075));
			/* found BOF, now read EOF */
			DevLog(DL_LABEL(2072), un->dt.od.ptoc_fwa + sector_cnt);
			if (scsi_cmd(fd, un, READ, 0, (void *) eof, label_len,
			    un->dt.od.ptoc_fwa + sector_cnt, &resid)
			    != label_len && sp->es_key != KEY_BLANK_CHECK) {
				/* if not recoverable error */
				if (sp->es_key > 1) {
					DevLog(DL_ERR(2034));
					goto scan_error;
				}
			}
			/* if file is not closed (EOF or EOV), close it */
			if (memcmp(&eof->label_id, "EO", 2) != 0) {
				if (close_file(fd, un, buf) != 0) {
					DevLog(DL_SYSERR(2035), fd);
					goto scan_error;
				}
			} else
				DevLog(DL_LABEL(2073));

			BE32toH(&eof->file_start, &h32);
			un->dt.od.next_file_fwa = h32;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
			BE32toH(&eof->file_size, &h32);
			un->dt.od.next_file_fwa += h32 + 1;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
			un->space = compute_size_from_sectors(un,
			    un->dt.od.ptoc_fwa - un->dt.od.next_file_fwa);
			break;
		}
	}

	free(buf);
	return;

scan_error:
	un->status.b.scan_err = TRUE;
	(void) memccpy(d_mess,
	    catgets(catfd, SET, 1033, "Error during label scan"),
	    '\0', DIS_MES_LEN);
	DevLog(DL_ERR(2050));
	free(buf);
}

/*
 * close_file - Process find file eoi.
 *
 * Description: Use a binary search to find eoi and close the file by writing
 * the EOF label.
 *
 * On entry:
 * fd    - open file descriptor
 * un    - pointer to device entry
 * lab_buf - label buffer
 *
 * Returns: error   = 0 if found ending sector.
 */
int
close_file(int fd, dev_ent_t *un, char *lab_buf)
{
	uint_t		h16;
	uint32_t	h32;
	int		cur_lwa, current, cur_ptoc;
	int		errcnt, attempts, len, sector_cnt, label_len, ii,
	    err = 0, resid;
	char		*buf, *d_mess = un->dis_mes[DIS_MES_NORM];
	time_t		utime;
	struct tm	tm_time;
	ls_bof1_label_t *bof;
	ls_bof1_label_t *eof;
	sam_extended_sense_t *sp =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	DevLog(DL_DETAIL(2099));
	(void) memccpy(d_mess,
	    catgets(catfd, SET, 705, "closing opened optical file"),
	    '\0', DIS_MES_LEN);
	sector_cnt = SECTORS_FOR_SIZE(un, sizeof (ls_bof1_label_t));
	label_len = un->sector_size * sector_cnt;

	/* Use binary search to find file data */

	cur_lwa = un->dt.od.ptoc_fwa;
	ii = cur_lwa - un->dt.od.next_file_fwa;
	if (ii <= 0) {
		return (-1);
	}
	len = VERIFY_LEN * label_len;
	buf = (char *)malloc_wait(len, 5, 0);
	while (ii >= VERIFY_LEN) {
		ii = ii >> 1;
		current = cur_lwa - ii;
		(void) memset(buf, 0, 6);
		sp->es_key = 0;
		errcnt = 0;
		DevLog(DL_LABEL(2074), current);
		if (scsi_cmd(fd, un, READ, 0, buf, un->sector_size,
		    current, &resid) != un->sector_size &&
		    sp->es_key != KEY_BLANK_CHECK) {

			if (sp->es_key > 1) {
				for (;;) {
					/*
					 * move up a sector at a time until
					 * something is read
					 */
					current++;
					sp->es_key = 0;
					DevLog(DL_LABEL(2074), current);
					if (scsi_cmd(fd, un, READ, 0, buf,
					    un->sector_size, current,
					    &resid) != un->sector_size) {
						if (sp->es_key > 1) {
							if (++errcnt >
							    VERIFY_LEN) {
								err = -1;
								goto out;
							}
						}
						continue;
					}
					break;
				}
				un->dt.od.next_file_fwa = current;
				DevLog(DL_DETAIL(2090),
				    un->dt.od.next_file_fwa);
				break;
			}
		}
		if (sp->es_key == KEY_BLANK_CHECK) {	/* if empty */
			cur_lwa = current;
		} else if (memcmp(buf + 1, "WORM1", 5) == 0) {	/* label */
			/* ooops, found a label better not write an EOF ? */
			err = -1;
			goto out;
		} else {	/* if not label */
			un->dt.od.next_file_fwa = current;
			DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
		}
	}

	sp->es_key = 0;
	DevLog(DL_DETAIL(2100), un->dt.od.next_file_fwa, VERIFY_LEN);
	if (scsi_cmd(fd, un, SCMD_VERIFY, 0, un->dt.od.next_file_fwa,
	    VERIFY_LEN, 0) && sp->es_key != KEY_BLANK_CHECK) {
		/* if not recoverable error */
		if (sp->es_key > 1) {
			un->status.b.scan_err = TRUE;
			DevLog(DL_ERR(2036));
			(void) memccpy(d_mess,
			    catgets(catfd, SET, 1033,
			    "Error during label scan"), '\0', DIS_MES_LEN);
			goto out;
		}
	}
	if (sp->es_key == KEY_BLANK_CHECK) {	/* if empty */
		/* should check valid bit here Fix... */
		ii = ((sp->es_info_1 << 24) +	/* what sector did we stop on */
		    (sp->es_info_2 << 16) + (sp->es_info_3 << 8) +
		    sp->es_info_4);

		un->dt.od.next_file_fwa = ii;
		DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);
	}
	/* guess we are ready to write an EOF now, set up the EOF data */

	/* LINTED pointer cast may result in improper alignment */
	bof = (ls_bof1_label_t *)lab_buf;
	/* LINTED pointer cast may result in improper alignment */
	eof = (ls_bof1_label_t *)(lab_buf + label_len);
	(void) memcpy(eof, bof, label_len);
	(void) memcpy(&eof->label_id, "EOF", 3);
	(void) time(&utime);
	(void) localtime_r(&utime, &tm_time);
	HtoBE16(&tm_time.tm_year, &h16);
	eof->creation_date.lt_year = h16;
	eof->creation_date.lt_mon = tm_time.tm_mon + 1;
	eof->creation_date.lt_day = tm_time.tm_mday;
	eof->creation_date.lt_hour = tm_time.tm_hour;
	eof->creation_date.lt_min = tm_time.tm_min;
	eof->creation_date.lt_sec = tm_time.tm_sec;
	eof->creation_date.lt_gmtoff = 0x00;	/* GMT offset */

	/* Enter file size. */
	BE32toH(&bof->file_start, &h32);
	DevLog(DL_DETAIL(2091), bof->file_start);
	h32 = un->dt.od.next_file_fwa - h32;
	HtoBE32(&h32, &eof->file_size);
	h32 *= un->sector_size;
	HtoBE32(&h32, &eof->byte_length);
	un->dt.od.next_file_fwa += 1;
	DevLog(DL_DETAIL(2090), un->dt.od.next_file_fwa);


	attempts = 0;
	cur_ptoc = un->dt.od.ptoc_fwa;
	DevLog(DL_LABEL(2070), cur_ptoc + sector_cnt);
	/*
	 * if it's WORM media and the EOF sectors aren't blank    OR the
	 * write doesn't complete
	 *
	 * Note: we should probably do write verifies when writing critical
	 * data
	 */
	if ((un->dt.od.medium_type == MEDIA_WORM &&
	    scsi_cmd(fd, un, SCMD_VERIFY, 0, cur_ptoc + sector_cnt,
	    sector_cnt, TRUE) != 0) ||
	    scsi_cmd(fd, un, WRITE, 0, (void *)eof, label_len,
	    cur_ptoc + sector_cnt, &resid) != label_len) {
		/*
		 * could not write eof, so back off two and try to write both
		 * bof and eof again.
		 */
		DevLog(DL_LABEL(2071));
		cur_ptoc -= sector_cnt * 2;
		DevLog(DL_LABEL(2087), cur_ptoc);
		while ((scsi_cmd(fd, un, WRITE, 0, lab_buf, label_len * 2,
		    cur_ptoc, &resid) != label_len * 2) && attempts != 6) {
			DevLog(DL_LABEL(2088), cur_ptoc);
			/* set ptoc fwa to last write */
			un->dt.od.ptoc_fwa = cur_ptoc;
			cur_ptoc -= sector_cnt * 2;
			DevLog(DL_LABEL(2087), cur_ptoc);
			attempts++;
		}
	}
	if (attempts == 6) {
		DevLog(DL_ERR(2071));
		err = -1;
	}
out:
	free(buf);
	return (err);
}

int
get_mode_sense(int fd, dev_ent_t *un)
{
	int		len;
	void		*dummy;
	mode_sense_t	mode_sense;

	union {
		ld4100_mode_sense_t *ld4100_ms;
		ld4500_mode_sense_t *ld4500_ms;
		multim_mode_sense_t *ibm0632_ms;
		hp1716_mode_sense_t *hp1716_ms;
		mode_sense_t   *msp;
	} msu;

	struct pg_codes {
		uchar_t	code;
		uchar_t	len;
	} *page_codes;

	msu.msp = (mode_sense_t *)SHM_REF_ADDR(un->mode_sense);

	if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 0,
	    &mode_sense, sizeof (mode_sense_t), 0x3f, (int *)NULL) < 0)
		return (-1);

	(void) memcpy(msu.msp, &mode_sense, sizeof (generic_mode_sense_t));
	/* Find and copy specfic mode sense pages */

	page_codes = (struct pg_codes *)
	    ((char *)&mode_sense + (msu.msp->u.generic_ms.hdr.blk_desc_len
	    + sizeof (generic_ms_hdr_t)));

	while ((char *)page_codes < ((char *)&mode_sense +
	    mode_sense.u.generic_ms.hdr.sense_data_len)) {

		switch (page_codes->code & 0x3f) {
		case 0x01:
			switch (un->model) {
			case M_LMS4100:
				dummy = &msu.ld4100_ms->pg1;
				len = sizeof (ld4100_ms_page1_t);
				break;

			case M_LMS4500:
				dummy = &msu.ld4500_ms->pg1;
				len = sizeof (ld4100_ms_page1_t);
				break;

			case M_IBM0632:
				dummy = &msu.ibm0632_ms->pg1;
				len = sizeof (multim_ms_page1_t);
				break;

			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg1;
				len = sizeof (hp1716_ms_page1_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x02:
			switch (un->model) {
			case M_LMS4100:
				dummy = &msu.ld4100_ms->pg2;
				len = sizeof (ld4100_ms_page2_t);
				break;

			case M_LMS4500:
				dummy = &msu.ld4500_ms->pg2;
				len = sizeof (ld4100_ms_page2_t);
				break;

			case M_IBM0632:
				dummy = &msu.ibm0632_ms->pg2;
				len = sizeof (multim_ms_page2_t);
				break;

			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg2;
				len = sizeof (hp1716_ms_page2_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x06:
			switch (un->model) {
			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg6;
				len = sizeof (hp1716_ms_page6_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x08:
			switch (un->model) {
			case M_IBM0632:
				dummy = &msu.ibm0632_ms->pg8;
				len = sizeof (multim_ms_page8_t);
				break;

			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg8;
				len = sizeof (hp1716_ms_page8_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x0b:
			switch (un->model) {
			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pgb;
				len = sizeof (hp1716_ms_pageb_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x20:
			switch (un->model) {
			case M_LMS4100:
				dummy = &msu.ld4100_ms->pg20;
				len = sizeof (ld4100_ms_page20_t);
				break;

			case M_LMS4500:
				dummy = &msu.ld4500_ms->pg20;
				len = sizeof (ld4100_ms_page20_t);
				break;

			case M_IBM0632:
				dummy = &msu.ibm0632_ms->pg20;
				len = sizeof (multim_ms_page20_t);
				break;

			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg20;
				len = sizeof (hp1716_ms_page20_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		case 0x21:
			switch (un->model) {
			case M_LMS4500:
				dummy = &msu.ld4500_ms->pg21;
				len = sizeof (ld4500_ms_page21_t);
				break;

			case M_IBM0632:
				dummy = &msu.ibm0632_ms->pg21;
				len = sizeof (multim_ms_page21_t);
				break;

			case M_HITACHI:
			case M_HPC1716:
				dummy = &msu.hp1716_ms->pg21;
				len = sizeof (hp1716_ms_page21_t);
				break;

			default:
				dummy = NULL;
			}
			break;

		default:
			dummy = NULL;
			break;
		}

		if (dummy != NULL)
			(void) memcpy(dummy, page_codes, len);

		page_codes = (struct pg_codes *)((char *)
		    page_codes + (page_codes->len + 2));
	}
	return (0);
}

time_t
get_label_time(dklabel_timestamp_t *pl)
{
	struct tm	tm;
	ushort_t	my_year;

	LE16toH(&pl->year, &my_year);

	tm.tm_year = my_year - 1900;
	tm.tm_mon = pl->month - 1;
	tm.tm_mday = pl->day;
	tm.tm_hour = pl->hour;
	tm.tm_min = pl->minute;
	tm.tm_sec = pl->second;
	tm.tm_isdst = -1;

	return (thread_mktime(&tm));
}
