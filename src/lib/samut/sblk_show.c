/*
 * sblk_show.c - Library functions for filesystem info display.
 *
 * 	See src/lib/samut/format.c for detailed formatting description.
 *
 *	See below for examples of format functions.
 *
 *	See src/fs/cmd/fstyp/fstyp.c for example of application usage.
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

#pragma ident "$Revision: 1.19 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <sys/inttypes.h>

#include <sam/lib.h>
#include <sam/param.h>
#include <sam/types.h>
#include <sam/format.h>
#include <sam/fs/samhost.h>
#include <sam/fs/ino.h>
#include <sam/fs/sblk.h>
#include <sam/sblk_show.h>
#include <sam/fs/sblk_mgmt.h>
#include <sam/sblk_nm.h>
#include <sam/devnm.h>
#include <sam/disk_show.h>
#include <pub/devstat.h>

/*
 * ----- sam_inodes_format - Format general i-node information
 * Returns errno status code.
 */
int
sam_inodes_format(
	struct sam_perm_inode *pip,	/* First i-node */
	sam_format_buf_t *bufp)		/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((pip == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%lld", pip->di.rm.size >> SAM_ISHIFT);
	(void) sam_format_element_append(&bp, "count", vp);

	sprintf(vp, "%d", pip->di.version);
	(void) sam_format_element_append(&bp, "version", vp);

	return (0);
}

/*
 * ----- sam_sbversion_to_str - SAM-FS/QFS superblock version -> string
 * Returns errno status code.
 */
int
sam_sbversion_to_str(
	unsigned long sbversion,	/* SAM-FS/QFS superblock version */
	char *strp,			/* String buffer */
	int len)			/* String buffer length (bytes) */
{
	sb_str_num_t *svp;

	if (strp == NULL) {	/* len is checked below. */
		return (EINVAL);
	}

	/* Search sb_version table for matching version. */
	for (svp = _sb_version; svp->nm != '\0'; svp++) {
		if (svp->val == sbversion) {
			if (len < (strlen(svp->nm)+1)) {
				return (EINVAL);
			}
			strcpy(strp, svp->nm);
			return (0);
		}
	}

	strcpy(strp, SAMFS_SBLK_UNKNOWN_STR);
	return (ENODEV);
}

/*
 * ----- sam_fsck_stat_to_str - Filesystem or slice state -> string
 * Returns errno status code.
 */
int
sam_fsck_stat_to_str(
	unsigned long state,	/* Filesystem or slice state */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	sb_str_num_t	*stp;

	if (strp == NULL) {	/* len is checked below. */
		return (EINVAL);
	}

	/* Search sb_version[] table for matching version. */
	for (stp = _fsck_status; stp->nm != '\0'; stp++) {
		if (stp->val == state) {
			if (len < (strlen(stp->nm)+1)) {
				return (EINVAL);
			}
			strcpy(strp, stp->nm);
			return (0);
		}
	}

	strcpy(strp, SB_UNKNOWN_STR);
	return (ENODEV);
}

/*
 * ----- sam_devtype_to_str - Device type -> string
 * Returns errno status code.
 */
int
sam_devtype_to_str(
	unsigned long devtype,	/* Device type */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	dev_nm_t *dnp;
	char *nm;

	if (strp == NULL) {
		return (EINVAL);
	}

	/* Look for matching type in dev_nm2dt table. */
	for (dnp = dev_nm2dt; dnp->nm != NULL; dnp++) {
		if (dnp->dt == devtype) {
			if (len < (strlen(dnp->nm)+1)) {
				return (EINVAL);
				/* NOTREACHED */
			}
			strcpy(strp, dnp->nm);
			break;
		}
	}

	if (dnp->nm == NULL) {   /* Not found in dev_nm2dt table. */
		nm = device_to_nm(devtype);
		strcpy(strp, nm);
	}

	return (0);
}

/*
 * ----- sam_seq_to_str - Character sequence -> string
 * Returns errno status code.
 */
int
sam_seq_to_str(
	char c[],		/* Character sequence */
	int len_src,		/* Character sequence length (bytes) */
	char *strp,		/* String buffer */
	int len_dest)		/* String buffer length (bytes) */
{
	if ((c == NULL) || (len_src < 0) || (strp == NULL) ||
	    (len_dest < (len_src + 1))) {
		return (EINVAL);
	}

	strncpy(strp, c, len_src);
	strp[len_src] = '\0';

	return (0);
}

/*
 * ----- sam_fstype_to_str - SAM-FS/QFS filesystem type -> string
 * Returns errno status code.
 */
int
sam_fstype_to_str(
	unsigned long fstype,	/* SAM-FS/QFS filesystem type */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	if (strp == NULL) {
		return (EINVAL);
	}

	switch (fstype) {
	case FSTYPE_SAM_QFS_SBV1:
		if (len < sizeof (FSNAME_SAM_QFS_SBV1)) {
			return (EINVAL);
		}
		strcpy(strp, FSNAME_SAM_QFS_SBV1);
		break;
	case FSTYPE_SAM_QFS_SBV2:
		if (len < sizeof (FSNAME_SAM_QFS_SBV2)) {
			return (EINVAL);
		}
		strcpy(strp, FSNAME_SAM_QFS_SBV2);
		break;
	case FSTYPE_SAM_QFS_SBV2A:
		if (len < sizeof (FSNAME_SAM_QFS_SBV2A)) {
			return (EINVAL);
		}
		strcpy(strp, FSNAME_SAM_QFS_SBV2A);
		break;
	default:
		if (len < sizeof (FSNAME_UNKNOWN)) {
			return (EINVAL);
		}
		strcpy(strp, FSNAME_UNKNOWN);
		return (ENODEV);
	}

	return (0);
}

/*
 * ----- sam_sbinfo_format - Format general superblock information
 * Returns errno status code.
 */
int
sam_sbinfo_format(
	struct sam_sbinfo *sbip,	/* Superblock info entry */
	sam_format_buf_t *bufp)		/* format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	time_t clock;
	sam_format_buf_t *bp = bufp;

	if ((sbip == NULL) || (bp == NULL)) {
		return (EINVAL);
	}
	if (sam_seq_to_str(sbip->name, SAMFS_SB_NAME_LEN, vp,
	    SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, SAMFS_SB_NAME_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "name", vp);

	sprintf(vp, "0x%x", sbip->magic);
	(void) sam_format_element_append(&bp, "magic", vp);

	sprintf(vp, "%d", sbip->gen);
	(void) sam_format_element_append(&bp, "gen", vp);

	sprintf(vp, "0x%x%x", sbip->fs_id.val[0], sbip->fs_id.val[1]);
	(void) sam_format_element_append(&bp, "id", vp);

	clock = (time_t)sbip->init;
	sprintf(vp, "%s", ctime(&clock));
	(void) sam_format_element_append(&bp, "init", vp);

	clock = (time_t)sbip->time;
	sprintf(vp, "%s", ctime(&clock));
	(void) sam_format_element_append(&bp, "update", vp);

	if (sam_fsck_stat_to_str(sbip->state, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, "unknown");
	}
	(void) sam_format_element_append(&bp, "state", vp);

	sprintf(vp, "%lld", sbip->offset[0]);
	(void) sam_format_element_append(&bp, "sb1_offset", vp);

	sprintf(vp, "%lld", sbip->offset[1]);
	(void) sam_format_element_append(&bp, "sb2_offset", vp);

	sprintf(vp, "%lld", sbip->hosts);
	(void) sam_format_element_append(&bp, "host_offset", vp);

	sprintf(vp, "%d", sbip->hosts_ord);
	(void) sam_format_element_append(&bp, "host_ord", vp);

	sprintf(vp, "%lld", sbip->inodes);
	(void) sam_format_element_append(&bp, "inode_offset", vp);

	sprintf(vp, "%d", sbip->min_usr_inum);
	(void) sam_format_element_append(&bp, "user_min_inode", vp);

	sprintf(vp, "%d", sbip->ext_bshift);
	(void) sam_format_element_append(&bp, "ext_shift", vp);

	/* Deal with 3.5.1 backwards compatibility. */
	if (sbip->mm_blks[1] == 0) { /* No large DAU metadata blocks */
		sprintf(vp, "%d", sbip->meta_blks);
		(void) sam_format_element_append(&bp, "sm_meta_blocks", vp);
		(void) sam_format_element_append(&bp, "lg_meta_blocks", vp);
	} else {
		sprintf(vp, "%d", sbip->mm_blks[0]);
		(void) sam_format_element_append(&bp, "sm_meta_blocks", vp);
		sprintf(vp, "%d", sbip->mm_blks[1]);
		(void) sam_format_element_append(&bp, "lg_meta_blocks", vp);
	}

	sprintf(vp, "%d", sbip->dau_blks[0]);
	(void) sam_format_element_append(&bp, "sm_data_blocks", vp);

	sprintf(vp, "%d", sbip->dau_blks[1]);
	(void) sam_format_element_append(&bp, "lg_data_blocks", vp);

	sprintf(vp, "%d", sbip->eq);
	(void) sam_format_element_append(&bp, "eq_id", vp);

	sprintf(vp, "%s", sbip->fs_name);
	(void) sam_format_element_append(&bp, "fset_name", vp);

	if (sam_devtype_to_str(sbip->fi_type, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, DT_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "fset_type", vp);

	sprintf(vp, "%d", sbip->ord);
	(void) sam_format_element_append(&bp, "fset_ord", vp);

	sprintf(vp, "%lld", sbip->space);
	(void) sam_format_element_append(&bp, "fset_blks_free", vp);

	sprintf(vp, "%lld", sbip->capacity);
	(void) sam_format_element_append(&bp, "fset_blks", vp);

	sprintf(vp, "%d", sbip->mm_count);
	(void) sam_format_element_append(&bp, "fset_meta_count", vp);

	sprintf(vp, "%d", sbip->da_count);
	(void) sam_format_element_append(&bp, "fset_data_count", vp);

	sprintf(vp, "%d", sbip->fs_count);
	(void) sam_format_element_append(&bp, "fset_count", vp);

	return (0);
}

/*
 * ----- sam_sbord_format - Format superblock ordinal information
 * Returns errno status code.
 */
int
sam_sbord_format(
	struct sam_sbord *sbop,		/* Superblock ordinal entry */
	sam_format_buf_t *bufp)		/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((sbop == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%d", sbop->ord);
	(void) sam_format_element_append(&bp, "ord", vp);

	sprintf(vp, "%d", sbop->eq);
	(void) sam_format_element_append(&bp, "eq_id", vp);

	if (sam_devtype_to_str(sbop->type, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, DT_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "dev_type", vp);

	if (sam_fsck_stat_to_str(sbop->state, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, "unknown");
	}
	(void) sam_format_element_append(&bp, "slice_state", vp);

	sprintf(vp, "%d", sbop->mm_ord);
	(void) sam_format_element_append(&bp, "meta_ord", vp);

	sprintf(vp, "%d", sbop->num_group);
	(void) sam_format_element_append(&bp, "stripe_count", vp);

	sprintf(vp, "%lld", sbop->space);
	(void) sam_format_element_append(&bp, "part_blocks_free", vp);

	sprintf(vp, "%lld", sbop->capacity);
	(void) sam_format_element_append(&bp, "part_blocks", vp);

	sprintf(vp, "%lld", sbop->allocmap);
	(void) sam_format_element_append(&bp, "alloc_map_offset", vp);

	/* sbop->fill2 ununsed */

	sprintf(vp, "%d", sbop->l_allocmap);
	(void) sam_format_element_append(&bp, "alloc_map_blocks", vp);

	sprintf(vp, "%lld", sbop->dau_next);
	(void) sam_format_element_append(&bp, "lg_dau_next", vp);

	sprintf(vp, "%lld", sbop->dau_size);
	(void) sam_format_element_append(&bp, "lg_dau_count", vp);

	sprintf(vp, "%d", sbop->system);
	(void) sam_format_element_append(&bp, "sys_blocks", vp);

	return (0);
}

/*
 * ----- sam_host_table_format - Format host table information
 * Returns errno status code.
 */
int
sam_host_table_format(
	struct sam_host_table *htp,	/* Host table buffer */
	sam_format_buf_t *bufp)		/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((htp == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%d", htp->version);
	(void) sam_format_element_append(&bp, "version", vp);

	sprintf(vp, "%d", htp->gen);
	(void) sam_format_element_append(&bp, "gen", vp);

	sprintf(vp, "%d", htp->count);
	(void) sam_format_element_append(&bp, "count", vp);

	sprintf(vp, "%d", htp->length);
	(void) sam_format_element_append(&bp, "size", vp);

	sprintf(vp, "%d", htp->pendsrv);
	(void) sam_format_element_append(&bp, "pend_serv_index", vp);

	sprintf(vp, "%d", htp->server);
	(void) sam_format_element_append(&bp, "serv_index", vp);

	return (0);
}
