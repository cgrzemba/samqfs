/*
 * tapes.h - defines and stuff for tape processing
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

#if !defined(_AML_TAPES_H)
#define	_AML_TAPES_H

#pragma ident "$Revision: 1.39 $"

#include "aml/labels.h"
#include "sam/resource.h"

/* Define the default tape device permissions when SAM owns the device  */

#define	SAM_TAPE_MODE  0660

/* See defaults.h for definitions of the default tape block sizes */

/* Define the min. and max sector (block) sizes */

#define	TAPE_SECTOR_SIZE    (16 * 1024)
#define	MAX_TAPE_SECTOR_SIZE    (2048 * 1024) /* 2 MB */


/*
 * default capacity of tape media(without looking at the media)
 * Usually always wrong, but less than the actual capacity.
 * Expressed in units of 1024
 *
 * The stk 9490 and D3 have different size cartridges, but no
 * software way to tell what cartridge is installed.  These limits
 * define the size of the larger cartridge.  The archiver will just
 * hit logical EOT and start over on the next tape...
 */
#define	DLT_SIZE	(1024 * 1024 * 800)	/* 800 gb(dlt-s4) */
#define	EXB_SIZE	(1024 * 1024 * 2)	/*   2 gb */
#define	DAT_SIZE	((1024 * 1024 * 13)/10)	/* 1.3 gb */
#define	MO_SIZE		(1024 * 1024 * 9)	/* 650 mb */
#define	PLASMON_UDO_SIZE (1024 * 1024 * 15)	/*  15 gb */
#define	STK_SIZE	(1024 * 200)		/* 200 mb */
#define	STK_9490_SZ	(1024 * 800)		/* 800 mb */
#define	STK_9490_EX	(1024 * 1024 * 2)	/*   2 gb */
#define	IBM_3590_SZ	(1024 * 1024 * 60)	/*  60 gb */
#define	IBM_3592_SZ	(1024 * 1024 * 700)	/* 700 gb */
#define	IBM_3570_SZ	(1024 * 1024 * 5)	/*   5 gb */
#define	IBM_3580_SZ	(1024 * 1024 * 800)	/* 800 gb */
#define	STK_D3_SZ	(1024 * 1024 * 10)	/*  10 gb */
#define	STK_9840_SZ	(1024 * 1024 * 75)	/*  75 gb */
#define	STK_9940_SZ	(1024 * 1024 * 200)	/* 200 gb */
#define	STK_TITAN_SZ	(1024 * 1024 * 1000)	/* 1000 gb */
#define	SONYDTF_DFT_SZ	(1024 * 1024 * 200)	/* 200 gb */
#define	SONYAIT_DFT_SZ	(1024 * 1024 * 100)	/* 100 gb */
#define	SONYSAIT_DFT_SZ	(1024 * 1024 * 800)	/* 800 gb */
#define	FUJITSU_128_SZ	(1024 * 1024 * 10)	/*  10 gb */

/* physical tracks are 16k not 32k as the documentation states */
#define	VHS343_SIZE	(1144800 << 4)
#define	VHS367_SIZE	(1293129 << 4)
#define	VHS258_SIZE	(897360 << 4)
#define	DEFLT_SIZE	(1000)		/* 1 mb default */

#define	EOT_MARKS	2		/* Number of tape marks to write */

/* A macro to return the default capacity for given media type */

#define	DEFLT_CAPC(m)  ((m) == DT_VIDEO_TAPE ? VHS258_SIZE : \
			(m) == DT_SQUARE_TAPE ? STK_SIZE : \
			(m) == DT_9490 ? STK_9490_SZ : \
			(m) == DT_EXABYTE_TAPE ? EXB_SIZE : \
			(m) == DT_LINEAR_TAPE ? DLT_SIZE : \
			(m) == DT_3590 ? IBM_3590_SZ : \
			(m) == DT_3592 ? IBM_3592_SZ : \
			(m) == DT_3570 ? IBM_3570_SZ : \
			(m) == DT_IBM3580 ? IBM_3580_SZ : \
			(m) == DT_SONYDTF ? SONYDTF_DFT_SZ : \
			(m) == DT_SONYAIT ? SONYAIT_DFT_SZ : \
			(m) == DT_SONYSAIT ? SONYSAIT_DFT_SZ : \
			(m) == DT_D3 ? STK_D3_SZ : \
			(m) == DT_9840 ? STK_9840_SZ : \
			(m) == DT_9940 ? STK_9940_SZ : \
			(m) == DT_TITAN ? STK_TITAN_SZ : \
			(m) == DT_FUJITSU_128 ? FUJITSU_128_SZ : \
			(m) == DT_OPTICAL ? MO_SIZE : \
			(m) == DT_WORM_OPTICAL_12 ? MO_SIZE : \
			(m) == DT_WORM_OPTICAL ? MO_SIZE : \
			(m) == DT_ERASABLE ? MO_SIZE : \
			(m) == DT_PLASMON_UDO ? PLASMON_UDO_SIZE : \
			(m) == DT_MULTIFUNCTION ? MO_SIZE : \
			(m) == DT_DAT ? DAT_SIZE : (DEFLT_SIZE))

/* Define the default position time values */

#define	TP_PT_DEFAULT		(30 * 60)		/* 'tp' */

/*
 * The "high performance" STKs can take up to 45 minutes to
 * position.  Give them an hour.
 */
#define	TP_PT_STK		(60 * 60)		/* 'sg'/'se'/'d3' */

/*
 * DLTs can take forever to "fast position" if they have lost
 * the directory information ....
 */
#define	TP_PT_LINEAR_TAPE	(5 * 60 * 60)		/* 'lt' */


/* structs used by the libsamfs tape routines */
typedef struct read_position {		/* Read position data */
#if defined(_BIT_FIELDS_HTOL)
	uchar_t
			BOP	:1,
			EOP	:1,
				:3,
			BPU	:1,
				:2;
#else	/* defined(_BIT_FIELDS_HTOL) */
	uchar_t
				:2,
			BPU	:1,
				:3,
			EOP	:1,
			BOP	:1;
#endif	/* defined(_BIT_FIELDS_HTOL) */
	uchar_t		pn;		/* Partition number */
	uchar_t		rsvd1[2];
	uchar_t		fbl[4];		/* First block location */
	uchar_t		lbl[4];		/* Last block location */
	uchar_t		rsvd2;
	uchar_t		nbkb[3];	/* Number of blocks in buffer */
	uchar_t		nbyb[4];	/* Number of bytes in buffer */
} read_position_t;

/*
 * Since sam supports a number of different tape drives and some of
 * them do not use the standard device drives, the following structs
 * are used to access the routines.  It is up to the first caller
 * of the routine to fill in the table.
 */

/*
 * These definitions were left here during removal of all
 * Ampex support because the jump table could be useful when
 * moving to new device apis.
 */
#define	SAM_TAPE_DRIVERS	2	/* Number of drivers */
#define	SAM_TAPE_N_ENTRIES	15	/* Number of entry points */
#define	SAM_TAPE_DVR_DEFAULT	0	/* The INDEX into the default (st) */
#define	SAM_TAPE_D2_INDEX	1	/* Index for the D2 */

/*
 * This struct is a list of entry point names to map into the jump table
 * The first entry is the name of the library to load.  A empty string
 * indicates a missing(not needed ) routine.  There must be one-to-one
 * between entry point name and the address.  All entry points must
 * have the same number of arguments and types.  If a  entry point is not
 * found in the named library, an attempt will be made to map it into
 * the currently running objects.
 */

typedef struct tape_IO_entry {
	char	*lib_name;
	char	*create_tape_eof;
	char	*find_tape_file;
	char	*position_tape;
	char	*position_tape_offset;
	char	*read_position;
	char	*scsi_cmd;
	char	*spin_drive;
	char	*tape_append;
	char	*read_tape_capacity;
	char	*process_tape_labels;
	char	*update_block_limits;
	char	*write_tape_labels;
	char	*format_tape;
	char	*get_n_partitions;
} tape_IO_entry_t;

typedef struct tape_IO {
	int (*create_tape_eof)(int *, dev_ent_t *, sam_resource_t *);
	int (*find_tape_file)(int, dev_ent_t *, sam_resource_t *);
	int (*position_tape)(dev_ent_t *, int, uint_t);
	int (*position_tape_offset)(dev_ent_t *, int, uint_t, uint_t);
	int (*read_position)(dev_ent_t *, int, uint_t *);
	int (*scsi_cmd)(int, dev_ent_t *, int, int, ...);
	int (*spin_unit)(void *, char *, int *, int, int);
	int (*tape_append)(int, dev_ent_t *, sam_resource_t *);
	int (*read_tape_capacity)(dev_ent_t *, int);
	void (*process_tape_labels)(int, dev_ent_t *);
	void (*update_block_limits)(dev_ent_t *, int);
	int  (*write_tape_labels)(int *, dev_ent_t *, label_req_t *);
	int  (*format_tape)(int *, dev_ent_t *, format_req_t *);
	int  (*get_n_partitions)(dev_ent_t *, int);
} tape_IO_t;

typedef struct tape_IO_table {
	mutex_t		mutex;
	int		initialized;
	tape_IO_t	jmp_table;
} tape_IO_table_t;

#if defined(MAIN) && !defined(lint)
tape_IO_entry_t tape_IO_entries[SAM_TAPE_DRIVERS] = {
	{	/* Standard stuff */
		"libsamfs.so",
		"create_tape_eof",
		"find_tape_file",
		"position_tape",
		"position_tape_offset",
		"read_position",
		"scsi_cmd",
		"spin_unit",
		"tape_append",
		"read_tape_capacity",
		"process_tape_labels",
		"update_block_limits",
		"write_tape_labels",
		"format_tape",
		"get_n_partitons",
	},

	{	/* The dst310 (d2) */
		"libsamdstio.so",
		"dst_create_tape_eof",
		"dst_find_tape_file",
		"",	/* position_tape not used */
		"dst_position_tape_offset",
		"dst_read_position",
		"dst_scsi_cmd",
		"dst_spin_unit",
		"dst_tape_append",
		"dst_read_tape_capacity",
		"dst_process_tape_labels",
		"dst_update_block_limits",
		"dst_write_tape_labels",
		"dst_format_tape",
		"dst_get_n_partitions",
	}
};
tape_IO_table_t	IO_table[SAM_TAPE_DRIVERS];
#endif

extern tape_IO_entry_t	tape_IO_entries[];
extern tape_IO_table_t	IO_table[];

/* Function prototypes used by libsamfs tape routines */
int backspace_record(dev_ent_t *, int, int, int *);
int backspace_file(dev_ent_t *, int, int, int *);
int forwardspace_record(dev_ent_t *, int, int, int *);
int skip_block_forward(dev_ent_t *, int, int, int *);
int skip_block_backward(dev_ent_t *, int, int, int *);
int repair_at_eom(struct dev_ent *, int);
int rewind_skipeom(dev_ent_t *, int);
int rewind_tape(dev_ent_t *, int);
int check_cleaning_error(dev_ent_t *, int, void *);
int load_tape_io_lib(tape_IO_entry_t *, tape_IO_t *);
void update_block_limits(dev_ent_t *, int);
uchar_t get_media_type(dev_ent_t *un);
int tape_properties(dev_ent_t *un, int fd);
void position_rmedia(dev_ent_t *un);
void tapeclean(dev_ent_t *un, int fd);
void tapeclean_active(dev_ent_t *un, int required, int requested,
	int expired, int invalid);
void tapeclean_media(dev_ent_t *un);
int tapeclean_drive(dev_ent_t *un);

#endif /* !defined(_AML_TAPES_H) */
