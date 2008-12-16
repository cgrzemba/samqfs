/*
 * disk_nm.h - Disk Value <-> String Conversion Tables
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

#ifndef _SAM_DISK_NM_H
#define	_SAM_DISK_NM_H

#ifdef sun
#pragma ident "$Revision: 1.11 $"
#endif

/* String definitions for disk type */

#define	DKC_CDROM_STR			"cdrom"
#define	DKC_WDC2880_STR			"wdc-2880"
#define	DKC_XXX_0_STR			"unassigned"
#define	DKC_XXX_1_STR			"unassigned"
#define	DKC_DSD5215_STR			"dsd-5215"
#define	DKC_ACB4000_STR			"acb-4000"
#define	DKC_MD21_STR			"md-21"
#define	DKC_XXX_2_STR			"unassigned"
#define	DKC_NCRFLOPPY_STR		"ncr-floppy"
#define	DKC_SMSFLOPPY_STR		"sms-floppy"
#define	DKC_SCSI_CCS_STR		"scsi-ccs"
#define	DKC_INTEL82072_STR		"intel-82072"
#define	DKC_MD_STR				"metadisk"
#define	DKC_INTEL82077_STR		"intel-82077"
#define	DKC_DIRECT_STR			"direct"
#define	DKC_PCMCIA_MEM_STR		"pcmcia-mem"
#define	DKC_PCMCIA_ATA_STR		"pcmcia-ata"

/* String definitions for VTOC partition ID tags */
#define	V_UNASSIGNED_STR		"unassigned"
#define	V_BOOT_STR				"boot"
#define	V_ROOT_STR				"root"
#define	V_SWAP_STR				"swap"
#define	V_USR_STR				"usr"
#define	V_BACKUP_STR			"backup"
#define	V_STAND_STR				"stand"
#define	V_VAR_STR				"var"
#define	V_HOME_STR				"home"
#define	V_ALTSCTR_STR			"alternate-sector"
#define	V_CACHE_STR				"cache"
#define	V_RESERVED_STR			"reserved"

/* String definitions for VTOC partition permission flags */
#define	V_NONE_STR		"(none)"
#define	V_UNMNT_STR		"unmountable"
#define	V_RONLY_STR		"read-only"
#define	V_RO_UNMNT_STR	"read-only,unmountable"

/* Number <-> string conversion tables */

typedef struct {
	char *nm;		/* Character string */
	unsigned long val;	/* Number */
} dk_str_num_t;

/* Controller Type */
dk_str_num_t	_dkc_type[] =
	{{DKC_CDROM_STR, DKC_CDROM},
	{DKC_WDC2880_STR, DKC_WDC2880},
	{DKC_XXX_0_STR, DKC_XXX_0},
	{DKC_XXX_1_STR, DKC_XXX_1},
	{DKC_DSD5215_STR, DKC_DSD5215},
	{DKC_ACB4000_STR, DKC_ACB4000},
	{DKC_MD21_STR, DKC_MD21},
	{DKC_XXX_2_STR, DKC_XXX_2},
	{DKC_NCRFLOPPY_STR, DKC_NCRFLOPPY},
	{DKC_SMSFLOPPY_STR, DKC_SMSFLOPPY},
	{DKC_SCSI_CCS_STR, DKC_SCSI_CCS},
	{DKC_INTEL82072_STR, DKC_INTEL82072},
	{DKC_MD_STR, DKC_MD},
	{DKC_INTEL82077_STR, DKC_INTEL82077},
	{DKC_DIRECT_STR, DKC_DIRECT},
	{DKC_PCMCIA_MEM_STR, DKC_PCMCIA_MEM},
	{DKC_PCMCIA_ATA_STR, DKC_PCMCIA_ATA},
	{"", 0}
};

/* VTOC Partition ID */

dk_str_num_t	_vpart_id[] =
	{{V_UNASSIGNED_STR, V_UNASSIGNED},
	{V_BOOT_STR, V_BOOT},
	{V_ROOT_STR, V_ROOT},
	{V_SWAP_STR, V_SWAP},
	{V_USR_STR, V_USR},
	{V_BACKUP_STR, V_BACKUP},
	{V_STAND_STR, V_STAND},
	{V_VAR_STR, V_VAR},
	{V_HOME_STR, V_HOME},
	{V_ALTSCTR_STR, V_ALTSCTR},
	{V_CACHE_STR, V_CACHE},
	{V_RESERVED_STR, V_RESERVED},
	{"", 0}
};

/* VTOC Partition Permissions */

dk_str_num_t	_vpart_pflags[] =
	{{V_NONE_STR, V_NONE},
	{V_UNMNT_STR, V_UNMNT},
	{V_RONLY_STR, V_RONLY},
	{V_RO_UNMNT_STR, V_RO_UNMNT},
	{"", 0}
};

#endif /* _SAM_DISK_NM_H */
