/*
 * disk_show.c - Library functions for formatting controller, geometry,
 *               VTOC information display.
 *
 *               See src/lib/samut/format.c for detailed formatting description.
 *
 *               See below for examples of format functions.
 *
 *               See src/fs/cmd/fstyp/fstyp.c for example of application usage.
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

/*
 * Multiple SAM applications, as well as logging and tracing facilities,
 * display information from the same SAM structures, but do so in a
 * variety of ways, using a variety of huuman-readable names and display
 * formats.  That displayed SAM information can be rather cryptic,
 * especially to someone not yet familiar with SAM.
 *
 * The purpose of these format functions is to promote uniform display
 * of external information by making available a single source for
 * defining human-readable element names and their corresponding
 * values for each element of a structure.  However, because each
 * application has its own structure element display requirements,
 * each set of standard human-readable names and values must be able
 * to be formatted flexibly.
 *
 * Not every SAM structure requires a human-readable format definition.
 * For the SAM structures that would benefit from such a definition,
 * a formatting function is recommended.  Library functions described
 * herein are available both to support writing your structure-specific
 * formatting function, as well as to support the formatting needs
 * that can be unique to each end-user application.
 *
 * The format function specific to each SAM structure accepts a
 * reference to the appropriate type of SAM structure and a reference to
 * a sam_format_buf_t buffer.  It then takes each element of the
 * referenced SAM structure, converts that element value to a string
 * (e.g., using sprintf), and uses the following library function to
 * append a display "line" to the supplied format buffer:
 *
 *	sam_format_element_append(sam_format_buf_t *bp, char *name, char *value)
 *
 * The above function updates the sam_format_buf_t buffer with contents
 * consisting of the following 5 constructs:
 *
 *	1) Line Prefix (default "")
 *	2) Element Name String
 *	3) Line Midfix (default " = ")
 *	4) Element value String
 *	5) Line Suffix (default "\n")
 *
 * To facilitate flexible formatting of display output, each of the above
 * constructs is a uniquely managed structure (but all are of the same type).
 * Library functions are used to manage each of those constructs within the
 * sam_format_buf_t buffer.  As such, neither those constructs nor the
 * contents of the sam_format_buf_t needs to be known by the caller.
 *
 * Your structure-specific formatting function should have a style similar
 * to the following:
 *
 *	#include <sam/format.h>
 *
 *	int
 *	sam_<struct_description>_format(
 *	struct <your_struct> *struct_ptr,
 *	sam_format_buf_t *bufp)
 *	{
 *		char str[SAM_FORMAT_WIDTH_MAX];
 *		sam_format_buf_t *bp = bufp;
 *
 *		if ((struct_ptr == NULL) || (bufp == NULL)) {
 *			return(EINVAL);
 *		}
 *
 *		sprintf(str, "%d", struct_ptr->elem_1);
 *		sam_format_element_append(&bp, "elem_1", str);
 *
 *		sprintf(str, "0x%x", struct_ptr->elem_3);
 *		sam_format_element_append(&bp, "elem_3", str);
 *
 *		....
 *
 *		return(0);
 *	}
 *
 * See src/lib/samut/format.c for a more detailed description.
 *
 * See below for examples of format functions.
 *
 * See src/fs/cmd/fstyp/fstyp.c for an example of application usage.
 *
 */

#pragma ident "$Revision: 1.10 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <strings.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <sys/inttypes.h>

#include <sam/param.h>
#include <sam/types.h>
#include <sam/format.h>
#include <sam/disk_show.h>
#include <sam/disk_nm.h>

/*
 * ----- sam_control_type_to_str - Device controller type -> string
 */
int				/* Errno status code */
sam_control_type_to_str(
	unsigned long ctype,	/* Device controller type */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	dk_str_num_t *ctp;

	if (strp == NULL) {	/* len is checked below. */
		return (EINVAL);
	}

	for (ctp = _dkc_type; ctp->nm != '\0'; ctp++) {
		if (ctp->val == ctype) {
			if (len < (strlen(ctp->nm)+1)) {
				return (EINVAL);
			}
			strcpy(strp, ctp->nm);
			return (0);
			/* NOTREACHED */
		}
	}
	if (len < (strlen(DKC_UNKNOWN_STR)+1)) {
		return (EINVAL);
	}
	strcpy(strp, DKC_UNKNOWN_STR);
	return (ENODEV);
}

/*
 * ----- sam_control_info_format - Format device controller information
 */
int				/* Errno status code */
sam_control_info_format(
	struct dk_cinfo *cip,	/* Device controller info buffer */
	sam_format_buf_t *bufp)	/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((cip == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%s", cip->dki_cname);
	(void) sam_format_element_append(&bp, "name", vp);

	if (sam_control_type_to_str(cip->dki_ctype, vp,
	    SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, "unknown");
	}
	(void) sam_format_element_append(&bp, "type", vp);

	sprintf(vp, "0x%x", cip->dki_flags);
	(void) sam_format_element_append(&bp, "flags", vp);

	sprintf(vp, "%d", cip->dki_cnum);
	(void) sam_format_element_append(&bp, "number", vp);

	sprintf(vp, "0x%x", cip->dki_addr);
	(void) sam_format_element_append(&bp, "address", vp);

	sprintf(vp, "0x%x", cip->dki_space);
	(void) sam_format_element_append(&bp, "bus", vp);

	sprintf(vp, "%d", cip->dki_prio);
	(void) sam_format_element_append(&bp, "intr_pri", vp);

	sprintf(vp, "0x%x", cip->dki_vec);
	(void) sam_format_element_append(&bp, "intr_vec", vp);

	sprintf(vp, "%s", cip->dki_dname);
	(void) sam_format_element_append(&bp, "drive_name", vp);

	sprintf(vp, "%d", cip->dki_unit);
	(void) sam_format_element_append(&bp, "unit_num", vp);

	sprintf(vp, "%d", cip->dki_slave);
	(void) sam_format_element_append(&bp, "slave_num", vp);

	sprintf(vp, "%d", cip->dki_partition);
	(void) sam_format_element_append(&bp, "part_num", vp);

	sprintf(vp, "%d", cip->dki_maxtransfer);
	(void) sam_format_element_append(&bp, "max_trans", vp);

	return (0);
}

/*
 * ----- sam_control_geometry_format - Format device geometry
 */
int					/* Errno status code */
sam_control_geometry_format(
	struct dk_geom *cgp,		/* Device geometry buffer */
	sam_format_buf_t *bufp)		/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((cgp == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%d", cgp->dkg_ncyl);
	(void) sam_format_element_append(&bp, "data_cyl", vp);

	sprintf(vp, "%d", cgp->dkg_acyl);
	(void) sam_format_element_append(&bp, "alt_cyl", vp);

	sprintf(vp, "%d", cgp->dkg_bcyl);
	(void) sam_format_element_append(&bp, "cyl_offset", vp);

	sprintf(vp, "%d", cgp->dkg_nhead);
	(void) sam_format_element_append(&bp, "heads", vp);

	/* cgp->dkg_obs1 obsolete */

	sprintf(vp, "%d", cgp->dkg_nsect);
	(void) sam_format_element_append(&bp, "track_sect", vp);

	sprintf(vp, "%d", cgp->dkg_intrlv);
	(void) sam_format_element_append(&bp, "interleave", vp);

	/* cgp->dkg_obs2 obsolete */
	/* cgp->dkg_obs3 obsolete */

	sprintf(vp, "%d", cgp->dkg_apc);
	(void) sam_format_element_append(&bp, "cyl_alt", vp);

	sprintf(vp, "%d", cgp->dkg_rpm);
	(void) sam_format_element_append(&bp, "rpm", vp);

	sprintf(vp, "%d", cgp->dkg_pcyl);
	(void) sam_format_element_append(&bp, "phys_cyl", vp);

	sprintf(vp, "%d", cgp->dkg_read_reinstruct);
	(void) sam_format_element_append(&bp, "sect_read_skip", vp);

	sprintf(vp, "%d", cgp->dkg_write_reinstruct);
	(void) sam_format_element_append(&bp, "sect_write_skip", vp);

	/* cgp->dkg_extra[0] expansion */
	/* cgp->dkg_extra[1] expansion */
	/* cgp->dkg_extra[2] expansion */
	/* cgp->dkg_extra[3] expansion */
	/* cgp->dkg_extra[4] expansion */
	/* cgp->dkg_extra[5] expansion */
	/* cgp->dkg_extra[6] expansion */

	return (0);
}

/*
 * ----- sam_vpart_id_to_str - VTOC partition ID -> string
 */
int				/* Errno status code */
sam_vpart_id_to_str(
	unsigned long id,	/* VTOC partition ID */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	dk_str_num_t	*vpp;

	if (strp == NULL) {
		return (EINVAL);
	}

	for (vpp = _vpart_id; vpp->nm != '\0'; vpp++) {
		if (vpp->val == id) {
			if (len < (strlen(vpp->nm)+1)) {
				return (EINVAL);
			}
			strcpy(strp, vpp->nm);
			return (0);
			/* NOTREACHED */
		}
	}

	if (len < (strlen(V_UNKNOWN_STR)+1)) {
		return (EINVAL);
	}
	strcpy(strp, V_UNKNOWN_STR);
	return (ENODEV);
}

/*
 * ----- sam_vpart_pflags_to_str - VTOC partition permissions -> string
 */
int				/* Errno status code */
sam_vpart_pflags_to_str(
	unsigned long pflags,	/* VTOC partition permissions */
	char *strp,		/* String buffer */
	int len)		/* String buffer length */
{
	dk_str_num_t	*vfp;

	if (strp == NULL) {
		return (EINVAL);
	}

	for (vfp = _vpart_pflags; vfp->nm != '\0'; vfp++) {
		if (vfp->val == pflags) {
			if (len < (strlen(vfp->nm)+1)) {
				return (EINVAL);
			}
			strcpy(strp, vfp->nm);
			return (0);
			/* NOTREACHED */
		}
	}

	if (len < strlen(V_UNKNOWN_STR)) {
		return (EINVAL);
	}
	strcpy(strp, V_UNKNOWN_STR);
	return (ENODEV);
}

/*
 * ----- sam_vtoc_format - Format device VTOC information
 */
int				/* Errno status code */
sam_vtoc_format(
	struct vtoc *vtp,	/* Device VTOC */
	sam_format_buf_t *bufp)	/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((vtp == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%s", vtp->v_asciilabel);
	(void) sam_format_element_append(&bp, "label", vp);

	sprintf(vp, "0x%lx/0x%lx/0x%lx", vtp->v_bootinfo[0], vtp->v_bootinfo[1],
	    vtp->v_bootinfo[2]);
	(void) sam_format_element_append(&bp, "boot", vp);

	sprintf(vp, "0x%lx", vtp->v_sanity);
	(void) sam_format_element_append(&bp, "sanity", vp);

	sprintf(vp, "%ld", vtp->v_version);
	(void) sam_format_element_append(&bp, "layout", vp);

	sprintf(vp, "'%s'", vtp->v_volume);
	(void) sam_format_element_append(&bp, "name", vp);

	sprintf(vp, "%d", vtp->v_sectorsz);
	(void) sam_format_element_append(&bp, "sector_size", vp);

	sprintf(vp, "%d", vtp->v_nparts);
	(void) sam_format_element_append(&bp, "part_count", vp);

	return (0);
}

/*
 * ----- sam_vtoc_partition_format - Format device VTOC partition information
 */
int				/* Errno status code */
sam_vtoc_partition_format(
	struct vtoc *vtp,	/* Device VTOC buffer */
	int index,		/* Index of partition to format */
	sam_format_buf_t *bufp)	/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;
	struct partition *vpp;

	if ((vtp == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	if ((index < 0) || (index > V_NUMPAR)) {
		return (EINVAL);
	}

	vpp = &vtp->v_part[index];

	if (sam_vpart_id_to_str(vpp->p_tag, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, "unknown");
	}
	(void) sam_format_element_append(&bp, "id", vp);

	if (sam_vpart_pflags_to_str(vpp->p_flag, vp,
	    SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, "unknown");
	}
	(void) sam_format_element_append(&bp, "permissions", vp);

	sprintf(vp, "%ld", vpp->p_start);
	(void) sam_format_element_append(&bp, "first_sector", vp);

	sprintf(vp, "%ld", vpp->p_size);
	(void) sam_format_element_append(&bp, "blocks", vp);

	return (0);
}


/*
 * ----- sam_efi_format - Format device EFI VTOC information
 */
int				/* Errno status code */
sam_efi_format(
	struct dk_gpt *eip,	/* Device EFI VTOC buffer */
	sam_format_buf_t *bufp)	/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;

	if ((eip == NULL) || (bp == NULL)) {
		return (EINVAL);
	}

	sprintf(vp, "%d", eip->efi_version);
	(void) sam_format_element_append(&bp, "version", vp);

	sprintf(vp, "%d", eip->efi_nparts);
	(void) sam_format_element_append(&bp, "part_count", vp);

	/* eip->efi_part_size unused */

	sprintf(vp, "%d", eip->efi_lbasize);
	(void) sam_format_element_append(&bp, "block_size", vp);

	sprintf(vp, "%lld", eip->efi_last_lba);
	(void) sam_format_element_append(&bp, "last_dsk_blk", vp);

	sprintf(vp, "%lld", eip->efi_last_u_lba);
	(void) sam_format_element_append(&bp, "blk_before_labels", vp);

	sprintf(vp, "%lld", eip->efi_first_u_lba);
	(void) sam_format_element_append(&bp, "blk_after_labels", vp);

	if (sam_uuid_to_str(&eip->efi_disk_uguid, vp,
	    SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, UUID_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "guid", vp);

	/* eip->efi_reserved[0] unused */
	/* eip->efi_reserved[1] unused */
	/* eip->efi_reserved[2] unused */
	/* eip->efi_reserved[3] unused */
	/* eip->efi_reserved[4] unused */
	/* eip->efi_reserved[5] unused */
	/* eip->efi_reserved[6] unused */
	/* eip->efi_reserved[7] unused */
	/* eip->efi_reserved[8] unused */
	/* eip->efi_reserved[9] unused */
	/* eip->efi_reserved[10] unused */
	/* eip->efi_reserved[11] unused */
	/* eip->efi_reserved[12] unused */
	/* eip->efi_reserved[13] unused */
	/* eip->efi_reserved[14] unused */
	/* eip->efi_reserved[15] unused */

	return (0);
}

/*
 * ----- sam_uuid_to_str - UUID -> string
 */
int				/* Errno status code */
sam_uuid_to_str(
	struct uuid *uup,	/* UUID buffer */
	char *strp,		/* String buffer */
	int len)		/* String buffer length (bytes) */
{
	int i;
	char sp[3];

	if ((uup == NULL) || (strp == NULL) || (len < ((UUID_LEN * 2) + 3))) {
		return (EINVAL);
	}

	strcpy(strp, "0x");
	for (i = 0; i < UUID_LEN; i++) {
		sprintf(sp, "%02x", ((uchar_t *)uup)[i]);
		strcat(strp, sp);
	}

	return (0);
}

/*
 * ----- sam_efi_partition_format - Format device EFI VTOC partition info
 */
int				/* Errno status code */
sam_efi_partition_format(
	struct dk_gpt *eip,	/* Device EFI VTOC buffer */
	int index,		/* index of partition to format */
	sam_format_buf_t *bufp)	/* Format buffer */
{
	char vp[SAM_FORMAT_WIDTH_MAX];
	sam_format_buf_t *bp = bufp;
	struct dk_part *epp;

	if ((eip == NULL) || (bp == NULL)) {
		return (EINVAL);
	}
	if ((index < 0) || (index > (eip->efi_nparts - 1))) {
		return (EINVAL);
	}
	epp = &eip->efi_parts[index];

	sprintf(vp, "%s", epp->p_name);
	(void) sam_format_element_append(&bp, "name", vp);

	sprintf(vp, "%lld", epp->p_start);
	(void) sam_format_element_append(&bp, "start_lba", vp);

	sprintf(vp, "%lld", epp->p_size);
	(void) sam_format_element_append(&bp, "block_size", vp);

	if (sam_uuid_to_str(&epp->p_guid, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, UUID_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "type_guid", vp);

	sprintf(vp, "%d", epp->p_tag);
	(void) sam_format_element_append(&bp, "tag", vp);

	sprintf(vp, "0x%x", epp->p_flag);
	(void) sam_format_element_append(&bp, "flags", vp);

	if (sam_uuid_to_str(&epp->p_uguid, vp, SAM_FORMAT_WIDTH_MAX) != 0) {
		strcpy(vp, UUID_UNKNOWN_STR);
	}
	(void) sam_format_element_append(&bp, "unique_guid", vp);

	return (0);
}
