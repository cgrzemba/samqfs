/*
 * tplabels.h - ANSI tape labels.
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

#if !defined(_AM_AMLLABELS_H)
#define	_AML_TPLABELS_H

#pragma ident "$Revision: 1.9 $"

#include "aml/labels.h"

/*
 * vol1_label.
 *
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'VOL'
 *    4               Label Number                    '1'
 *    5 - 10          Volume Serial Number            alpha-numeric
 *    11              Accessibility                   alpha-numeric
 *    12 - 24         Reserved to ANSI                '       '
 *    25 - 37         Implementation Identifier       alpha-numeric
 *    38 - 51         Owner Identifier                alpha-numeric
 *    52 - 79         Reserved to ANSI                '       '
 *    80              Label Standard Level            numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	volume_serial_number[6];
	char	accessibility[1];
	char	reserved_to_ansi1[13];
	char	implementation_identifier[13];
	char	owner_identifier[14];
	char	reserved_to_ansi2[28];
	char	label_standard_version[1];
} vol1_label_t;


/*
 * voln_label.
 *
 *    Description:
 *    The ANSI VOL2 thru VOL9 labels contain the following fields:
 *
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'VOL'
 *    4               Label Number                    '2' - '9'
 *    5 - 80          Reserved to Implementors        alpha-numeric
 */


typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_implementors[76];
} voln_label_t;

/*
 * eov1_label
 *
 *    Description:
 *
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'EOV'
 *    4               Label Number                    '1'
 *    5 - 21          File Identifier                 alpha-numeric
 *    22 - 27         File-set Identifier             alpha-numeric
 *    28 - 31         File Section Number             numeric
 *    32 - 35         File Sequence Number            numeric
 *    36 - 39         Generation Number               numeric
 *    40 - 41         Generation Version Number numeric
 *    42 - 47         Creation Date                   ' yyddd'
 *    48 - 53         Expiration Date                 ' yyddd'
 *    54              Accessibility                   alpha-numeric
 *    55 - 60         Block Count                     numeric
 *    61 - 73         System Code                     alpha-numeric
 *    74 - 80         Reserved to ANSI                '       '
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	file_identifier[17];
	char	file_set_identifier[6];
	char	file_section_number[4];
	char	file_sequence_number[4];
	char	generation_number[4];
	char	generation_version_number[2];
	char	creation_date[6];
	char	expiration_date[6];
	char	accessibility[1];
	char	block_count[6];
	char	system_code[13];
	char	reserved_to_ansi[7];
} eov1_label_t;


/*
 * eov2_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 ..  3         Label Identifier                'EOV'
 *    4               Label Number                    '2'
 *    5               Record Format                   'F' 'D' or 'S'
 *    6 .. 10         Block Length                    numeric
 *    11 .. 15        Record Length                   numeric
 *    16 .. 50        Reserved to Implementors
 *    16 .. 17        SYSTEM Block Type               'US' or 'SS'
 *    18              SYSTEM Record Type              'F' 'U' or 'V'
 *    19 .. 21        SYSTEM Block Length Ext.        numeric
 *    (Most Significant Digits of Block Length)
 *    22 .. 24        SYSTEM Record Length Ext.       numeric
 *    (Most Significant Digits of Record Length)
 *    25              SYSTEM Padding Character        alpha-numeric
 *    26              SYSTEM Character Set            'A' or 'E'
 *    ('A' for ASCII, 'E' for EBCDIC)
 *    27              SYSTEM Character Conversion     'T' or 'F'
 *    ('T' for TRUE, 'F' for FALSE)
 *    28 .. 50        Reserved                        '       '
 *    51 .. 52        Buffer Offset Length            numeric
 *    53 .. 80        Reserved to ANSI                '       '
 */

typedef struct {
	union {
		char	label_string[80];
		struct {
			char	label_identifier[3];
			char	label_number[1];
			char	record_format[1];
			char	block_length[5];
			char	record_length[5];
			/* Reserved to implementors */
			char	block_type[2];
			char	record_type[1];
			char	block_length_ext[3];
			char	record_length_ext[3];
			char	padding_character[1];
			char	character_set[1];
			char	character_conversion[1];
			char	reserved[23];
			char	buffer_offset_length[2];
			char	reserved_to_ansi[28];
		} EOV2;
	} F;
} eov2_label_t;

/*
 * eovn_label
 *
 *    Description:
 *    The ANSI EOV3 through EOV9 labels contains the following fields:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'EOV'
 *    4               Label Number                    '3' .. '9'
 *    5 - 80          Reserved to Implementors        alpha-numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_implementors[76];
} eovn_label_t;

/*
 * eof1_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 ..  3         Label Identifier                'EOF'
 *    4               Label Number                    '1'
 *    5 .. 21         File Identifier                 alpha-numeric
 *    22 .. 27        File-set Identifier             alpha-numeric
 *    28 .. 31        File Section Number             numeric
 *    32 .. 35        File Sequence Number            numeric
 *    36 .. 39        Generation Number               numeric
 *    40 .. 41        Generation Version Number       numeric
 *    42 .. 47        Creation Date                   ' yyddd'
 *    48 .. 53        Expiration Date                 ' yyddd'
 *    54              Accessibility                   alpha-numeric
 *    55 .. 60        Block Count                     numeric
 *    61 .. 73        System Code                     alpha-numeric
 *    74 .. 80        Reserved to ANSI                '       '
 */


typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	file_identifier[17];
	char	file_set_identifier[6];
	char	file_section_number[4];
	char	file_sequence_number[4];
	char	generation_number[4];
	char	generation_version_number[2];
	char	creation_date[6];
	char	expiration_date[6];
	char	accessibility[1];
	char	block_count[6];
	char	system_code[13];
	char	reserved_to_ansi[7];
} eof1_label_t;


/*
 * eof2_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 ..  3         Label Identifier                'EOF'
 *    4               Label Number                    '2'
 *    5               Record Format                   'F' 'D' or 'S'
 *    6 - 10          Block Length                    numeric
 *    11 - 15         Record Length                   numeric
 *
 *    16 - 50         Reserved to Implementors
 *    16 - 17         SYSTEM Block Type               'US' or 'SS'
 *    18              SYSTEM Record Type              'F' 'U' or 'V'
 *    19 - 21         SYSTEM Block Length Ext.        numeric
 *    (Most Significant Digits of Block Length)
 *    22 - 24         SYSTEM Record Length Ext.       numeric
 *    (Most Significant Digits of Record Length)
 *    25              SYSTEM Padding Character        alpha-numeric
 *    26              SYSTEM Character Set            'A' or 'E'
 *    ('A' for ASCII, 'E' for EBCDIC)
 *    27              SYSTEM Character Conversion 'T' or 'F'
 *    ('T' for TRUE, 'F' for FALSE)
 *    28 - 50         Reserved for SYSTEM             '       '
 *
 *    51 - 52         Buffer Offset Length            numeric
 *    53 - 80         Reserved to ANSI                '       '
 */

typedef struct {
	union {
		char	label_string[80];
		struct {
			char	label_identifier[3];
			char	label_number[1];
			char	record_format[1];
			char	block_length[5];
			char	record_length[5];
			/* Reserved to implementors */
			char	block_type[2];
			char	record_type[1];
			char	block_length_ext[3];
			char	record_length_ext[3];
			char	padding_character[1];
			char	character_set[1];
			char	character_conversion[1];
			char	reserved[23];

			char	buffer_offset_length[2];
			char	reserved_to_ansi[28];
		} EOF2;
	} F;
} eof2_label_t;

/*
 * eofn_label
 *
 *    Description:
 *    The ANSI EOF3 through EOF9 labels contains the following fields:
 *    CP              Field Name                      Content
 *    1 ..  3         Label Identifier                'EOF'
 *    4               Label Number                    '3' .. '9'
 *    5 .. 80         Reserved to Implementors        alpha-numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_implementors[76];
} eofn_label_t;


/*
 * hdr1_label.
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 - 3           Label Identifier                'HDR'
 *    4               Label Number                    '1'
 *    5 - 21          File-identifier                 alpha-numeric
 *    22 - 27         File-set Identifier             alpha-numeric
 *    28 - 31         File Section Number             numeric
 *    32 - 35         File Sequence Number            numeric
 *    36 - 39         Generation Number               numeric
 *    40 - 41         Generation Version Number       numeric
 *    42 - 47         Creation Date                   ' yyddd'
 *    48 - 53         Expiration Date                 ' yyddd'
 *    54              Accessibility                   alpha-numeric
 *    55 - 60         Block Count                     '000000'
 *    61 - 73         System Code                     alpha-numeric
 *    74 - 80         Reserved to ANSI                '       '
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	file_identifier[17];
	char	file_set_identifier[6];
	char	file_section_number[4];
	char	file_sequence_number[4];
	char	generation_number[4];
	char	generation_version_number[2];
	char	creation_date[6];
	char	expiration_date[6];
	char	accessibility[1];
	char	block_count[6];
	char	system_code[13];
	char	reserved_to_ansi[7];
} hdr1_label_t;

/*
 * hdr2_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'HDR'
 *    4               Label Number                    '2'
 *    5               Record Format                   'F' 'D' or 'S'
 *    6 - 10          Block Length                    numeric
 *    11 - 15         Record Length                   numeric
 *    16 - 50         Reserved to Implementors
 *    16 - 17         SYSTEM Block Type               'US' or 'SS'
 *    18              SYSTEM Record Type              'F' 'U' or 'V'
 *    19 - 21         SYSTEM Block Length Ext.        numeric
 *    (Most Significant Digits of Block Length)
 *    22 - 24         SYSTEM Record Length Ext.       numeric
 *    (Most Significant Digits of Record Length)
 *    25              SYSTEM Padding Character        alpha-numeric
 *    26              SYSTEM Character Set            'A' or 'E'
 *    ('A' for ASCII, 'E' for EBCDIC)
 *    27              SYSTEM Character Conversion 'T' or 'F'
 *    ('T' for TRUE, 'F' for FALSE)
 *    28 - 50         Reserved for SYSTEM             '       '
 *    51 - 52         Buffer Offset Length            numeric
 *    53 - 80         Reserved to ANSI                '       '
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	record_format[1];
	char	block_length[5];
	char	record_length[5];

	/*  Reserved to implementors: */
	char	block_type[2];
	char	record_type[1];
	char	block_length_ext[3];
	char	record_length_ext[3];
	char	padding_character[1];
	char	character_set[1];
	char	character_conversion[1];
	union {		/* Unique to LSC */
		char		reserved_for_os[23];
		sam_time_t	label_time;
	} lsc_uniq;
	char	buffer_offset[2];
	char	reserved_to_ansi[28];
} hdr2_label_t;

/*
 * hdrn_label
 *
 *    Description:
 *    The ANSI HDR3 label contains the following fields:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'HDR'
 *    4               Label Number                    '3' .. '9'
 *    5 .. 80         Reserved to Implementors        alpha-numeric
 */

typedef struct hdrn_label {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_implementors[76];
} HDRN_LABEL;

typedef struct {
	char		file_identifier[17];
	char		file_set_identifier[6];
	char		implementation_identifier[13];
	ushort_t		file_section_number;		/* 0 .. 9999 */
	ushort_t		file_sequence_number;		/* 0 .. 9999 */
	ushort_t		generation_number;		/* 0 .. 9999 */
	char		generation_version_number;	/* 0 .. 99 */
	char		accessibility[1];
	uint_t		block_count;			/* 0 .. 999999 */
#ifdef  HELP
	struct time_ds	creation_date;
	struct time_ds	expiration_date;
#endif

	/* HDR2 label information */

	uint_t		block_length;			/* 0 .. 99999 */
	uint_t		record_length;			/* 0 .. 99999 */
	char		buffer_offset_length;		/* 0 .. 99 */
	char		record_format[1];
} label_info_t;


/*
 * uhla_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'UHL'
 *    4               Label Number                    alpha-numeric
 *    5 .. 80         Reserved to Implementors        alpha-numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_applications[76];
} uhla_label_t;

/*
 * utla_label
 *
 *    Description:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'UTL'
 *    4               Label Number                    alpha-numeric
 *    5 .. 80         Reserved to Implementors        alpha-numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_applications[76];
} utla_label_t;

/*
 * uvla_label
 *
 *    Description:
 *    The ANSI UVL1 thru UVL9 labels contain the following fields:
 *    CP              Field Name                      Content
 *    1 -  3          Label Identifier                'UVL'
 *    4               Label Number                    '1' .. '9'
 *    5 - 80          Reserved to Installations       alpha-numeric
 */

typedef struct {
	char	label_identifier[3];
	char	label_number[1];
	char	reserved_to_installations[76];
} uvln_label_t;


#endif /* !defined(_AML_TPLABELS_H) */
