/*
 * sefstructs.h - SAM-FS system error facility(_AML_SEF)  information.
 *
 *    Description:
 *    Definitions for SAM-FS system error facility
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#if !defined(_AML_SEFSTRUCTS_H)
#define	_AML_SEFSTRUCTS_H

#pragma ident "$Revision: 1.10 $"

/*
 * In the SCSI spec, the possible page codes returned in the "list of
 * supported pages" are 0 - 0x3F.
 */
#define	SEF_MAX_PAGES	(0x3F + 1)

struct sef_hdr {

	/* The following fields will be filled in once at open */
	uint_t		sef_magic;	/* magic # for app to sync file posn */
	uint_t		sef_version;	/* version number */
	uint_t		sef_size;	/* size of this record, excl. header */
	uint16_t	sef_eq;		/* equipment number of this device */

	char		sef_devname[128];	/* pathname of device */
	uchar_t		sef_vendor_id[9];	/* vendor id from inquiry */
	uchar_t		sef_product_id[17];	/* product id from inquiry */
	uchar_t		sef_revision[5];	/* revision from inquiry */
	uchar_t		sef_scsi_type;		/* device type from inquiry */

	/*
	 * The following fields must be filled in before each
	 * record is written.
	 */
	char		sef_vsn[32];	/* vsn of media that was mounted */
	time_t		sef_timestamp;	/* timestamp of this record */

};


struct sef_pginfo {
	ushort_t	pgcode;
	ushort_t	pglen;
};

/*
 * struct sef_devinfo contains the fields of sef information that are put in
 * the dev_ent_t structure--in the device table.
 */
struct sef_devinfo {
	struct sef_pginfo sef_pgs[SEF_MAX_PAGES];
	struct sef_hdr	*sef_hdr;	/* ptr to sef header for this device */
	uint8_t 	sef_inited;	/* indicates sef has been initialized */
					/* for this dev */
};


/* Structure of a scsi-standard log sense page header */
struct sef_pg_hdr {
	uchar_t		page_code;
	uchar_t		reserved;
	ushort_t	page_len;
};

/* Structure of "supported pages" log sense page */
struct sef_supp_pgs {
	struct sef_pg_hdr	hdr;
	uchar_t			supp_pgs[SEF_MAX_PAGES];
};

#endif /* !defined(_AML_SEFSTRUCTS_H) */
