/*
 * element.h
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

#ifndef ELEMENT_H
#define	ELEMENT_H

#pragma ident "$Revision: 1.17 $"

/* Element types */

#define	ALL_ELEMENTS	    0
#define	TRANSPORT_ELEMENT	1
#define	STORAGE_ELEMENT	 2
#define	IMPORT_EXPORT_ELEMENT   3
#define	DATA_TRANSFER_ELEMENT   4

/* Some upper limits */
#define	MAX_STORE_STATUS    512	/* maximum number of elements for */
	/* a read_element_status for storage */
/* Structures */

/* Element data for read_element_status command. */

typedef struct {
	uint8_t	 first_ele_addr[2];	/* first element reported */
/* number of elements reported */
	uint8_t	 numb_elements[2];
	uint8_t	 res;
/* byte count of returned data - 7 (header) */
	uint8_t	 count[3];
}		element_status_data_t;	/* Element status page */

typedef struct {
	uint8_t	 type_code;	/* Element type of this page */
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			PVol:1,	/* Primary volume info */
			AVol:1,	/* Alternate volume info */
	:		6;
#else				/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			AVol:1,	/* Alternate volume info */
			PVol:1;	/* Primary volume info */
#endif				/* _BIT_FIELDS_HTOL */
/* length of element descriptor */
	uint8_t	 ele_dest_len[2];
	uint8_t	 res;
/* Byte count of data for this - 7 (header) */
	uint8_t	 count[3];
}		element_status_page_t;


/*
 * Element descriptors have a required part and an extension.  The extension
 * can vary but if it there, the extensions defined after the element type
 * are about all this code is interested in.
 *
 * ELEMENT_DESCRIPTOR_LENGTH is the length of the fixed + ext part of
 * the descriptor. This value is stored in the library structure and used by
 * read-element-status.  If any element status page returns a larger
 * value than this, the library struct will be updated to the larger
 * value and the command re-executed.  This larger value will be left in
 * the struct for any other read-element-status commands.
 *
 */

#define	  ELEMENT_DESCRIPTOR_LENGTH   (48 + 4)

/* Medium transport element descriptor */
typedef struct {
	uint8_t	 ele_addr[2];	/* element address */
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			except:1,	/* 1 = abnormal state */
	:		1,
			full:1;	/* 1 = holds a unit of media */
#else				/* defined(_BIT_FIELDS_HTOL) */
			full:1,	/* 1 = holds a unit of media */
	:		1,
			except:1,	/* 1 = abnormal state */
	:		5;
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 res;
}		transport_element_t;

typedef struct {
	uint8_t	 add_sense_code;
	uint8_t	 add_sense_qual;
	uint8_t	 res[3];
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			svalid:1,	/* 1 = valid data */
			invert:1,	/* 1 if media was inverted */
	:		6;
#else				/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			invert:1,	/* 1 if media was inverted */
			svalid:1;	/* 1 = valid data */
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 stor_addr[2];	/* last address where this media */
	/* was moved from. */
	uint8_t	 PVolTag[BARCODE_LEN];	/* Primary volume tag */
	uint8_t	 AVolTag[BARCODE_LEN];	/* Alternate volume tag */
	uint8_t	 resv[4];
}		transport_element_ext_t;

/* Storage element descriptor */
typedef struct {
	uint8_t	 ele_addr[2];	/* element address */
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			access:1,	/* 1 = access enabled */
			except:1,	/* 1 = abnormal state */
	:		1,
			full:1;	/* 1 = holds a unit of media */
#else				/* defined(_BIT_FIELDS_HTOL) */
			full:1,	/* 1 = holds a unit of media */
	:		1,
			except:1,	/* 1 = abnormal state */
			access:1,	/* 1 = access enabled */
	:		4;
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 res;
}		storage_element_t;


#define	PLASMON_MT_UDO				0x01
#define	PLASMON_MT_MO				0x00
#define	PLASMON_MT_UNKNOWN			0xff

typedef struct {
	uint8_t	 add_sense_code;
	uint8_t	 add_sense_qual;
	uint8_t	 res[3];
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			svalid:1,	/* 1 = valid data */
			invert:1,	/* 1 if media was inverted */
	:		6;
#else				/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			invert:1,	/* 1 if media was inverted */
			svalid:1;	/* 1 = valid data */
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 stor_addr[2];	/* last address where this media */
	/* was moved from. */
	uint8_t	 PVolTag[BARCODE_LEN];	/* Primary volume tag */
	uint8_t	 AVolTag[BARCODE_LEN];	/* Alternate volume tag */
	uint8_t	 resv[4];
}		storage_element_ext_t;

/* Import/Export element descriptor */
typedef struct {
	uint8_t	 ele_addr[2];	/* element address */
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
	:		2,
			imp_enable:1,	/* 1 = import enabled */
			exp_enable:1,	/* 1 = export enabled */
			access:1,	/* 1 = access enabled */
			except:1,	/* 1 = abnormal state */
			impexp:1,
			full:1;	/* 1 = holds a unit of media */
#else				/* defined(_BIT_FIELDS_HTOL) */
			full:1,	/* 1 = holds a unit of media */
			impexp:1,
			except:1,	/* 1 = abnormal state */
			access:1,	/* 1 = access enabled */
			exp_enable:1,	/* 1 = export enabled */
			imp_enable:1,	/* 1 = import enabled */
	:		2;
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 res;
}		import_export_element_t;

typedef struct {
	uint8_t	 add_sense_code;
	uint8_t	 add_sense_qual;
	uint8_t	 res[3];
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			svalid:1,	/* 1 = valid data */
			invert:1,	/* 1 if media was inverted */
	:		6;
#else				/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			invert:1,	/* 1 if media was inverted */
			svalid:1;	/* 1 = valid data */
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 stor_addr[2];	/* last address where this media */
	/* was moved from. */
	uint8_t	 PVolTag[BARCODE_LEN];	/* Primary volume tag */
	uint8_t	 AVolTag[BARCODE_LEN];	/* Alternate volume tag */
	uint8_t	 resv[4];
}		import_export_element_ext_t;

/* Data transfer element descriptor */
typedef struct {
	uint8_t	 ele_addr[2];	/* element address */
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			access:1,	/* 1 = access enabled */
			except:1,	/* 1 = abnormal state */
	:		1,
			full:1;	/* 1 = holds a unit of media */
#else				/* defined(_BIT_FIELDS_HTOL) */
			full:1,	/* 1 = holds a unit of media */
	:		1,
			except:1,	/* 1 = abnormal state */
			access:1,	/* 1 = access enabled */
	:		4;
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 res;
}		data_transfer_element_t;

typedef struct {
	uint8_t	 add_sense_code;
	uint8_t	 add_sense_qual;
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			not_bus:1,	/* not on same bus as robot */
	:		1,
			id_valid:1,	/* 1 = scsi bus address is valid */
			lu_valid:1,	/* 1 = logical unit is valid */
	:		1,
			log_unit:3;	/* logical unit for this drive */
#else				/* defined(_BIT_FIELDS_HTOL) */
			log_unit:3,	/* logical unit for this drive */
	:		1,
			lu_valid:1,	/* 1 = logical unit is valid */
			id_valid:1,	/* 1 = scsi bus address is valid */
	:		1,
			not_bus:1;	/* not on same bus as robot */
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 scsi_bus_addr;
	uint8_t	 res;
			uint8_t
#if defined(_BIT_FIELDS_HTOL)
			svalid:1,	/* 1 = valid data */
			invert:1,	/* 1 if media was inverted */
	:		6;
#else				/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			invert:1,	/* 1 if media was inverted */
			svalid:1;	/* 1 = valid data */
#endif				/* _BIT_FIELDS_HTOL */
	uint8_t	 stor_addr[2];	/* last address where this media */
	/* was moved from. */
	uint8_t	 PVolTag[BARCODE_LEN];	/* Primary volume tag */
	uint8_t	 AVolTag[BARCODE_LEN];	/* Alternate volume tag */
	uint8_t	 resv[4];
}		data_transfer_element_ext_t;

#endif	/* ELEMENT_H */
