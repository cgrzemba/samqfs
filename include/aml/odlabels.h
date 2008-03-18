/*
 * sam_odlabels.h - ANSI optical labels.
 *
 * Description:
 * Contains all of the structs and types for optical disk labels.
 *
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

#if !defined(_AML_ODLABELS_H)
#define	_AML_ODLABELS_H

#pragma ident "$Revision: 1.11 $"

#include "aml/labels.h"
#include "aml/opticals.h"

typedef struct {
	uint_t		length;
	uint_t		location;
} dklabel_extent_t;

typedef struct {
	ushort_t		identifier;
	ushort_t		version;
	uchar_t		checksum;
	uchar_t		reserved_1;
	ushort_t		serial_num;
	ushort_t		crc;
	ushort_t		crc_length;
	uint_t		location;
} dklabel_tag_t;

typedef struct {
	uchar_t		type;
	uchar_t		information[63];
} dklabel_charspec_t;

typedef struct {
	ushort_t		timezone;
	ushort_t		year;
	uchar_t		month;
	uchar_t		day;
	uchar_t		hour;
	uchar_t		minute;
	uchar_t		second;
	uchar_t		centisec;
	uchar_t		microsec_hundreds;
	uchar_t		microsec;
} dklabel_timestamp_t;

typedef struct {
	uchar_t		flags;
	uchar_t		identifier[23];
	uchar_t		id_suffix[8];
} dklabel_regid_t;

/*
 * dkpri - ANSI primary volume descriptor.n
 *
 *      Description:
 *      The ANSI primary volume descriptor contains the following:
 *      CP      LENGTH          Field Name
 *      1       16              Descriptor Tag
 *      17      4               Volume Descriptor Sequence Number
 *      21      4               Volume Descriptor Number
 *      25      32              Volume Identifier
 *      57      2               Volume Sequence Number
 *      59      2               Maximum Volume Sequence Number
 *      61      2               Interchange Level
 *      63      2               Maximum Interchange Level
 *      65      4               Character Set List
 *      69      4               Maximum Character Set List
 *      73      128             Volume Set Identifier
 *      201     64              Descriptor Character Set
 *      265     64              Explanatrory Character Set
 *      329     8               Volume Abstract
 *      337     8               Volume Copyright Notice
 *      345     32              Application Identifier
 *      377     12              Recording Data and Time
 *      389     32              Implementation Identifier
 *      421     64              Implementation Use
 *      485     4               Predecessor Volume Descriptor Sequence Location
 *      489     2               Unallocated Space Record extent
 *      491     22              Reserved
 *
 */

typedef struct {
	dklabel_tag_t		descriptor_tag;
	uint_t			descriptor_sequence_number;
	uint_t			volume_descriptor_number;
	uchar_t			volume_id[32];
	ushort_t			volume_sequence_number;
	ushort_t			max_volume_sequence_number;
	ushort_t			interchange_level;
	ushort_t			max_interchange_level;
	uint_t			character_set_list;
	uint_t			max_character_set_list;
	uchar_t			volume_set_id[128];
	dklabel_charspec_t	descriptor_char_set;
	dklabel_charspec_t	explanatory_char_set;
	dklabel_extent_t	abstract;
	dklabel_extent_t	copyright;
	dklabel_regid_t		application_identifier;
	dklabel_timestamp_t	recording_date_time;
	dklabel_regid_t		implementation_identifier;
	uchar_t			implementation_use[64];
	uint_t			predecessor_vol_seq_location;
	ushort_t			flags;
	uchar_t			reserved_1[22];
} dkpri_label_t;

/*
 * dkpart_label - The ANSI partition descriptor.
 *
 *      Description:
 *      The ANSI partition descriptor contains the following:
 *      CP      LENGTH          Field Name
 *      1       16              Descriptor Tag
 *      17      4               Volume Descriptor Sequence Number
 *      21      2               Partition Flags
 *      23      2               Partition Number
 *      25      32              Partition Contents
 *      57      128             Partition Contents Use
 *      185     4               Access Type
 *      189     4               Paration starting Location
 *      193     4               Paration Length
 *      197     32              Implementation Identifier
 *      229     128             Implementation Use
 *      357     156             Reserved
 */

typedef struct {
	dklabel_tag_t		descriptor_tag;
	uint_t			descriptor_sequence_number;
	ushort_t			flags;
	ushort_t			partition_number;
	dklabel_regid_t		partition_contents;
	uchar_t			partition_use[128];
	uint_t			access_type;
	uint_t			starting_location;
	uint_t			length;
	dklabel_regid_t		implementation_identifier;
	uchar_t			implementation_use[128];
	uchar_t			reserved_1[156];
} dkpart_label_t;


/* Some defines from the ANSI standard */

/* tag identifiers */

#define	PRIMARY_VOL_DES		1
#define	ANCHOR_VOL_DES_P	2
#define	VOLUME_DES_P		3
#define	IMPLEM_USE_VOL_DES	4
#define	PARTITION_DES		5
#define	LOGICAL_VOL_DES		6
#define	UNALLOCATED_SPACE_DES	7
#define	TERMINATING_DES		8

#define	VOL_SEQ_LEN	8
#define	VERIFY_LEN	8

/* Character set types */

#define	CSET_0    0
#define	CSET_1    1
#define	CSET_2    2
#define	CSET_3    3
#define	CSET_4    4
#define	CSET_5    5
#define	CSET_6    6
#define	CSET_7    7
#define	CSET_8    8

/* Access type for partition */

#define	ACCESS_NS    0
#define	ACCESS_RO    1
#define	ACCESS_WO    2
#define	ACCESS_RW    3

/*
 * dkanchor_label - The ANSI anchor descriptor.
 *
 *      Description:
 *      The ANSI anchor descriptor contains the following:
 *      CP      LENGTH          Field Name
 *      1       16              Descriptor Tag
 *      17      8               Main Volume Descriptor Sequence Extent
 *      25      8               Reserve Volume Descriptor Sequence Extent
 *      33      480             Reserved
 */

typedef struct {
	dklabel_tag_t		descriptor_tag;
	dklabel_extent_t	main_volume_descriptor;
	dklabel_extent_t	reserve_volume_descriptor;
	uchar_t			reserved_1[480];
} dkanchor_label_t;

/*
 * dkvdesc_label - The ANSI volume descriptor label.
 *
 *      Description:
 *      The ANSI volume descriptor label contains the following:
 *      CP      LENGTH          Field Name
 *      1       16              Descriptor Tag
 *      17      4               Volume Descriptor Sequence Number
 *      21      8               Next volume descriptor sequence extent
 *      29      484             Reserved
 */

typedef struct {
	dklabel_tag_t		descriptor_tag;
	uint_t			descriptor_sequence_number;
	dklabel_extent_t	next_vds_extent;
	uchar_t			reserved_1[484];
} dkvdesc_label_t;


/* ls_time - label time format. */

typedef struct {
	ushort_t	lt_year;	/* Number of years    (since 0) */
	uchar_t		lt_mon;		/* Month of the year  (1 - 12)  */
	uchar_t		lt_day;		/* Day of the month   (1 - 31)  */
	uchar_t		lt_hour;	/* Hour of the day    (0 - 23)  */
	uchar_t		lt_min;		/* Munite of the hour (0 - 59)  */
	uchar_t		lt_sec;		/* Second of the min. (0 - 59)  */
	uchar_t		lt_gmtoff;	/* Offset from GMT in 30 minute */
					/* intervals   (-24 W - +24 E)  */
} ls_time_t;

/* ls_bof1_label - Beginning/end-of-file label. */

typedef struct {
	char		resa;		/* Reserved */
	char		std_id[5];	/* Standard identifier  "WORM1" */
	char		resb[2];	/* Reserved */
	char		label_id[3];	/* Label identifier "BOF" */
	uchar_t		label_version;	/* ANSI version number */
	char		file_id[31];	/* File identifier */
	char		resc;		/* Reserved */
	ushort_t		version;	/* File version number */
	uchar_t		security_level;	/* Security access level */
	uchar_t		append_iteration; /* Append iteration */
	char		password[31];	/* File password */
	char		resd;		/* Reserved */
	char		owner_id[31];	/* Owner identifier */
	char		rese;		/* Reserved */
	char		group_id[31];	/* Group identifier */
	char		resf;		/* Reserved */
	char		system_id[31];	/* System identifier */
	char		resg;		/* Reserved */
	char		equipment_id[31]; /* Equipment identifier */
	char		resh;		/* Reserved */
	char		resi[40];	/* Reserved */
	ls_time_t	creation_date;	/* Creation date and time */
	uint_t		file_start_u;	/* File section start address */
	uint_t		file_start;
	uint_t		file_size_u;	/* File section size */
	uint_t		file_size;
	ushort_t		fsn;		/* File sequence number */
	uchar_t		recording_method; /* File recording method */
	uchar_t		file_contents;	/* File contents */
	uchar_t		record_delimiting_char;	/* Record delimiting char */
	char		resj;		/* Reserved */
	uchar_t		block_type;	/* Block type */
	uchar_t		record_type;	/* Record type */
	int		resk;		/* Reserved */
	int		block_length;	/* Block length */
	int		resl;		/* Reserved */
	int		record_length;	/* Record length */
	ls_time_t	expiration_date; /* Expiration date and time */
	uint_t		byte_length_u;	/* Reserved */
	uint_t		byte_length;	/* File section byte length */
	int		resm[2];	/* Reserved */
	char		next_vsn[32];	/* Back/forw volume identifier */
	char		user_information[160]; /* File user information */
	char		reso[512];	/* Reserved */
} ls_bof1_label_t;


/* function prototypes */
offset_t compute_sectors_from_size(dev_ent_t *un, offset_t size);
offset_t compute_size_from_sectors(dev_ent_t *un, offset_t sector_cnt);

#define	SECTORS_FOR_SIZE(un, x) ((x+(un->sector_size-1))/un->sector_size)
#define	GET_TOTAL_SECTORS(un) (compute_sectors_from_size(un, un->capacity) \
	+ OD_SPACE_UNAVAIL)

#endif				/* !defined(_AML_ODLABELS_H) */
