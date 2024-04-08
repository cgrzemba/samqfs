/*
 * odblank.c - write labels on optical disk.
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

#pragma ident "$Revision: 1.18 $"

static char    *_SrcFile = __FILE__;

#include <stdlib.h>
#include <string.h>

#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/shm.h"
#include "aml/external_data.h"
#include "aml/odlabels.h"
#include "aml/proto.h"
#include "sam/devinfo.h"

/* globals */

extern shm_alloc_t master_shm, preview_shm;

/* Function prototypes */
int write_anchor(int, dev_ent_t *, int[], dkpri_label_t *, dkpart_label_t *);
int write_desc_ptr(int, dev_ent_t *, int[], dkpri_label_t *, dkpart_label_t *);
int write_vol_seq(int, dev_ent_t *, uint32_t[], dkpri_label_t *,
    dkpart_label_t *);
void build_tag(dklabel_tag_t *, int, int);

/*
 * blank_label_optic.
 *
 * Description: Write optical labels
 *
 * On entry:
 * open_fd - open file descriptor
 * un      - device entry(with locked io_mutex)
 * primary - primary label partition - partition label
 *
 * Returns: 0 - success !0 - failed
 */

int
blank_label_optic(int open_fd, dev_ent_t *un, dkpri_label_t *primary,
    dkpart_label_t *partition, int flags, int relabel)
{
	uint32_t	PtocFwa;
	int		err, len = un->sector_size;
	int		anchor[2];
	char		*buffer;

	anchor[0] = OD_ANCHOR_POS;	/* 1st anchor */
	anchor[1] = GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS;	/* 2nd anchor */

	buffer = (char *)malloc_wait(len, 5, 0);

	/*
	 * If erasable media.
	 */
	if (un->dt.od.medium_type == MEDIA_RW) {

		DevLog(DL_DETAIL(2001));

		/*
		 * Hitachi DVD-RAM drive does not support explict scsi erase
		 * command.  Erase operation is impicit with the write
		 * operation.
		 */
		if (un->model != M_HITACHI) {
			if ((flags & LABEL_ERASE) ?	/* how much to erase */
			    scsi_cmd(open_fd, un, SCMD_EXTENDED_ERASE,
			    (60 * 20), 0, GET_TOTAL_SECTORS(un)) :
			    (scsi_cmd(open_fd, un, SCMD_EXTENDED_ERASE, 0,
			    0, anchor[0] + 256) ||
			    scsi_cmd(open_fd, un, SCMD_EXTENDED_ERASE, 0,
			    anchor[1] - 256, 512))) {

				DevLog(DL_ERR(2002));
				DevLogSense(un);
				DevLogCdb(un);
				free(buffer);
				return (1);
			}
		}
		mutex_lock(&un->mutex);
		un->status.b.labeled = FALSE;	/* flag it unlabeled */
		mutex_unlock(&un->mutex);

		relabel = FALSE;
		PtocFwa = anchor[1] - 1;
		buffer[0] = PtocFwa >> 24;
		buffer[1] = PtocFwa >> 16;
		buffer[2] = PtocFwa >> 8;
		buffer[3] = PtocFwa;

		DevLog(DL_LABEL(2063), PtocFwa);
		if (scsi_cmd(open_fd, un, WRITE, 0, buffer, len, anchor[1] - 1,
		    (int *)NULL) != len) {

			DevLog(DL_ERR(2064));
			free(buffer);
			return (-1);
		}
	}
	free(buffer);
	if (!relabel)		/* if not relabeling */
		err = write_anchor(open_fd, un, anchor, primary, partition);
	else
		err = write_desc_ptr(open_fd, un, anchor, primary, partition);

	return (err);
}

/*
 * write_anchor
 *
 * Description: Write anchor, primary and partition labeles
 *
 * On entry:
 * open_fd - Open file decsriptor
 * un      - Pointer to the device entry(with locked io_mutex)
 * anchor  - Location of the 2 anchor blocks.
 * primary - dkpri_label_t *to primary label
 * partition - dkpart_label_t *to the partition label
 *
 * Returns: 0 - Success !0 - Failure
 */


int
write_anchor(int open_fd, dev_ent_t *un, int anchor[], dkpri_label_t *primary,
    dkpart_label_t *partition)
{
	uint32_t	location[2];
	uint32_t	h32;
	int		len, err, i;
	dkanchor_label_t *ap;

	len = un->sector_size;
	ap = (dkanchor_label_t *)malloc_wait(len, 5, 0);
	(void) memset(ap, 0, len);

	/* length of volume sequence */
	h32 = 256;
	HtoLE32(&h32, &ap->main_volume_descriptor.length);

	location[0] = 0;	/* 1st volume sequence */
	HtoLE32(&location[0], &ap->main_volume_descriptor.location);
	location[1] = anchor[1] + 1;	/* 2nd volume sequence */
	HtoLE32(&location[1], &ap->reserve_volume_descriptor.location);

	for (i = 0; i < 2; i++) {
		build_tag((dklabel_tag_t *)ap, ANCHOR_VOL_DES_P, anchor[i]);
		DevLog(DL_DETAIL(2004), i + 1, anchor[i]);
		if (scsi_cmd(open_fd, un, WRITE, 0, (char *)ap, len, anchor[i],
		    (int *)NULL) != len) {
			DevLog(DL_ERR(2005));
			free(ap);
			return (-1);
		}
	}
	free(ap);
	err = write_vol_seq(open_fd, un, location, primary, partition);
	return (err);
}

/*
 * write_desc_ptr.
 *
 * Description: Relabel by reading anchor and writing volume descriptor pointer
 *
 * On entry:
 * open_fd  - open file descriptor
 * un       - Pointer to the device entry(io_mutex locked)
 * anchor   - Location of the 2 anchor blocks
 * primary  - dkpri_label_t *
 * partition- dkpart_label_t *
 *
 * Returns: 0 - success !0 - failure
 */

int
write_desc_ptr(int open_fd, dev_ent_t *un, int anchor[], dkpri_label_t *primary,
    dkpart_label_t *partition)
{
	uint32_t	ext_len, loc[2], lw, seqno;
	int		vdl, len, err, i, ii;
	sam_extended_sense_t *sp =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	dkvdesc_label_t *vp;
	dkanchor_label_t *ap;

	DevLog(DL_LABEL(2097));
	/* length of volume sequence */
	/* compute new location */
	vdl = ((un->label_address > OD_ANCHOR_POS) ?
	    ((un->label_address -
	    (GET_TOTAL_SECTORS(un) - OD_ANCHOR_POS)) + 2) :
	    (un->label_address + 2));

	len = un->sector_size;
	ap = (dkanchor_label_t *)malloc_wait(len, 5, 0);
	vp = (dkvdesc_label_t *)ap;

	LE32toH(&vp->descriptor_sequence_number, &seqno);
	sp->es_key = 0;
	for (i = 0; i < 2; i++) {
		/* read an anchor record */
		DevLog(DL_DETAIL(2006), i + 1, anchor[i]);
		if (scsi_cmd(open_fd, un, READ, 0, ap, len, anchor[i],
		    (int *)NULL) != len) {

			if (sp->es_key != 1) {
				/* if error */
				DevLog(DL_ERR(2007), i + 1);
				if (i == 0)
					continue;	/* try other set */

				free(ap);
				un->status.b.scan_err = TRUE;
				DevLog(DL_ERR(2048));
				return (-1);
			}
		}
		if (vfyansi_label(ap, ANCHOR_VOL_DES_P) != 0) {
			free(ap);
			return (-1);
		}
		/*
		 * use the information from the anchor record about the
		 * location of the volume description
		 */
		LE32toH(&ap->main_volume_descriptor.length, &ext_len);
		LE32toH(&ap->main_volume_descriptor.location, &loc[0]);
		LE32toH(&ap->reserve_volume_descriptor.location, &loc[1]);
		break;
	}

	/* Write volume descriptor pointer block.    */

	for (ii = 0; ii < 2; ii++) {
		(void) memset(vp, 0, len);
		HtoLE32(&seqno, &vp->descriptor_sequence_number);
		/* calc new volume size */
		lw = ext_len - vdl;	/* len of vol sequence  */
		HtoLE32(&lw, &vp->next_vds_extent.length);
		/* calc new location for vol des */
		loc[ii] += vdl;	/* next vol sequence    */
		HtoLE32(&loc[ii], &vp->next_vds_extent.location);
		build_tag((dklabel_tag_t *)ap, VOLUME_DES_P, loc[ii]);
		/* save the new volume descriptors */
		DevLog(DL_DETAIL(2008), ii + 1, loc[ii]);
		if (scsi_cmd(open_fd, un, WRITE, 0, vp, len, loc[ii],
		    (int *)NULL) != len) {

			DevLog(DL_ERR(2009), ii + 1);
			DevLogSense(un);
			free(ap);
			return (-1);
		}
		/* prepare location for vol seq writes */
		loc[ii]++;
	}

	free(ap);
	err = write_vol_seq(open_fd, un, loc, primary, partition);
	return (err);
}

/*
 * write_vol_seq.
 *
 * Description: Write volume and partition labels at main extent and reserve
 * extents.
 *
 * On entry:
 * open_fd  - open file descriptor
 * un       - pointer to the device entry(io_mutex locked)
 * location - Location of the 2 volume blocks
 * primary  - dkpri_label_t *
 * partition- dkpart_label_t *
 *
 * Returns: 0 - success !0 - failure
 */


int
write_vol_seq(int open_fd, dev_ent_t *un, uint32_t location[],
    dkpri_label_t *primary, dkpart_label_t *partition)
{
	int	len, i, ploc;
	int	err = 0;
	dkpri_label_t	*vp;
	dkpart_label_t	*pp;

	len = un->sector_size << 1;	/* allocate 2 sectors */

	vp = (dkpri_label_t *)malloc_wait(len, 5, 0);
	/* LINTED pointer cast may result in improper alignment */
	pp = (dkpart_label_t *)((char *)vp + un->sector_size);
	(void) (void) memset(vp, 0, len);
	(void) memcpy(vp, primary, 512);
	(void) memcpy(pp, partition, 512);
	for (i = 0; i < 2; i++) {	/* write each set of labels */
		build_tag((dklabel_tag_t *)vp, PRIMARY_VOL_DES, location[i]);
		ploc = location[i] + 1;
		build_tag((dklabel_tag_t *)pp, PARTITION_DES, ploc);
		DevLog(DL_DETAIL(2010), i + 1, location[i]);
		if (scsi_cmd(open_fd, un, WRITE, 0, vp, len, location[i],
		    (int *)NULL) != len) {
			DevLog(DL_ERR(2011), i + 1);
			DevLogSense(un);
			err = -1;	/* flag an error */
			break;
		}
	}
	free(vp);
	return (err);
}

/*
 * build_tag.
 *
 * Description: Build tag structure in label
 *
 * On entry:
 * lp         - dklabel_tap_t *
 * label_type - type of label
 * location   - sector location of this label
 */

void
build_tag(dklabel_tag_t *lp, int label_type, int location)
{
	uint16_t	h16;
	int		ii;
	ushort_t	cksum;
	char		*cp;

	(void) memset(lp, 0, sizeof (dklabel_tag_t));
	h16 = label_type;
	HtoLE16(&h16, &lp->identifier);
	h16 = 1;
	HtoLE16(&h16, &lp->version);
	HtoLE32((uint32_t *)& location, &lp->location);
	cp = (char *)lp;
	for (ii = 1, cksum = 0; ii <= 16; cp++, ii++)
		if (ii < 5 || ii > 6)
			cksum += *cp;

	cksum &= 0xff;		/* mask to byte	 */
	lp->checksum = cksum;
}
