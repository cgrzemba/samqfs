/*
 * scsi_mode_sense.h - SAM-FS mode sense structures
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

#ifndef _AML_MODE_SENSE_H
#define	_AML_MODE_SENSE_H

#pragma ident "$Revision: 1.14 $"

/*
 * Mode Sense Header.
 *
 *	Description:
 *	  This command reports the device medium, logical unit,
 *	  or peripheral device parameters.
 */

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
	:		7;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		7,
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} generic_ms_hdr_t;

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
	:		6,
			EBC:1;		/* Enable block check */
#else	/* defined(_BIT_FIELDS_HTOL) */
			EBC:1,		/* Enable block check */
	:		6,
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} optical_ms_hdr_t;

/*  Generic scsi robot mode sense header */

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  res[3];
} robot_ms_hdr_t;

/* IBM 0632 Optical Disk drive */

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
	:		7;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		7,
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} multim_ms_hdr_t;

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
			BM:2,		/* Buffered mode bit */
			speed:5; 	/* Speed */
#else	/* defined(_BIT_FIELDS_HTOL) */
			speed:5, 	/* Speed */
			BM:2,		/* Buffered mode bit */
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} tape_ms_hdr_t;

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
			BM:3,		/* Buffered mode bit */
			speed:4; 	/* Speed */
#else	/* defined(_BIT_FIELDS_HTOL) */
			speed:4, 	/* Speed */
			BM:3,		/* Buffered mode bit */
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} video_ms_hdr_t;

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
			BM:3,		/* Buffered mode bit */
			speed:4; 	/* Speed */
#else	/* defined(_BIT_FIELDS_HTOL) */
			speed:4, 	/* Speed */
			BM:3,		/* Buffered mode bit */
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} dlt_ms_hdr_t;

typedef struct {
	uchar_t	  sense_data_len;	/* Sense data length */
	uchar_t	  medium_type;		/* Medium_type */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			WP:1,		/* Write protect bit */
			BM:3,		/* Buffered mode bit */
			speed:4; 	/* Speed */
#else	/* defined(_BIT_FIELDS_HTOL) */
			speed:4,	 /* Speed */
			BM:3,		/* Buffered mode bit */
			WP:1;		/* Write protect bit */
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  blk_desc_len;		/* Block descriptor len */
} exab_ms_hdr_t;

/* Mode Sense Block Descriptor. */

typedef struct {
	uchar_t	  density_code;
	uchar_t	  nob_high;		/* No. of blocks */
	uchar_t	  nob_mid;
	uchar_t	  nob_low;
	uchar_t	  res;
	uchar_t	  blk_len_high;		/* Block length */
	uchar_t	  blk_len_mid;
	uchar_t	  blk_len_low;
} generic_blk_desc_t;

/* video mode Sense Block Descriptor. */

typedef struct {
	uchar_t	  density_code;
	uchar_t	  nob_high;		/* No. of blocks */
	uchar_t	  nob_mid;
	uchar_t	  nob_low;
	uchar_t	  res;
	uchar_t	  blk_len_high;		/* Block length */
	uchar_t	  blk_len_mid;
	uchar_t	  blk_len_low;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		3,
			FTST:1,
			EJ:1,
			IN:1,
			HC:1,
			PL:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			PL:1,
			HC:1,
			IN:1,
			EJ:1,
			FTST:1,
	:		3;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  tape_length;
	uchar_t	  read_retry_count;
	uchar_t	  write_retur_count;
	uchar_t	  read_buffer_thresh;
	uchar_t	  write_buffer_thrsh;
} video_blk_desc_t;

/*
 * Mode Sense Page 1.
 *
 *	Description:
 *	  Error Recovery Page.
 */

typedef struct {
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
			awre:1,
			arre:1,
			tb:1,
			rc:1,
			ecc:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			ecc:1,
			rc:1,
			tb:1,
			arre:1,
			awre:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  retry_count;
	uchar_t	  correction_span;
	uchar_t	  head_offset;
	uchar_t	  strobe_offset;
	uchar_t	  recovery_time;
} sabre_ms_page1_t;


typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			awre:1,
			arre:1,
			tb:1,
			rc:1,
			ecc:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			ecc:1,
			rc:1,
			tb:1,
			arre:1,
			awre:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  read_retry_count;
	uchar_t	  correction_span;
	uchar_t	  head_offset;
	uchar_t	  strobe_offset;
	uchar_t	  res;
	uchar_t	  write_retry_count;
	uchar_t	  res1;
	uchar_t	  recovery_time_hi;
	uchar_t	  recovery_time_lo;
} elite_ms_page1_t;

typedef struct {
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
		:2,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
			auto_write_reallocation:1,
	:		1,
			transfer_block:1,
	:		1,
			enable_early_recovery:1,
			post_error_recovery:1,
			disable_transfer_error:1,
			disable_error_corr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			disable_error_corr:1,
			disable_transfer_error:1,
			post_error_recovery:1,
			enable_early_recovery:1,
	:		1,
			transfer_block:1,
	:		1,
			auto_write_reallocation:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  read_retry_count;
} ld4100_ms_page1_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		:2,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
	page_code:6,
	:2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			media_load:3;
#else	/* defined(_BIT_FIELDS_HTOL) */
			media_load:3,
	:		5;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
			door_open:1,
	:		2,
			storage_addr:5;
#else	/* defined(_BIT_FIELDS_HTOL) */
			storage_addr:5,
	:		2,
			door_open:1;
#endif	/* _BIT_FIELDS_HTOL */
} ld4500_ms_page21_t;

typedef struct {
	uchar_t	  save_parms:1, :1, page_code:6;
	uchar_t	  page_length;
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
			awre:1,
			arre:1,
			tb:1,
			rc:1,
			ecc:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			ecc:1,
			rc:1,
			tb:1,
			arre:1,
			awre:1;
#endif	/* _BIT_FIELDS_HTOL */
} multim_ms_page1_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			awre:1,
	:		1,
			tb:1,
	:		2,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
	:		2,
			tb:1,
	:		1,
			awre:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  read_retry_cnt;
	uchar_t	  res3[4];
	uchar_t	  write_retry_cnt;
	uchar_t	  res4[3];
} hp1716_ms_page1_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		:2,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			err_recovery_cntl:3;
#else				/* defined(_BIT_FIELDS_HTOL) */
			err_recovery_cntl:3,
	:		5;
#endif				/* _BIT_FIELDS_HTOL */
	uchar_t	  res2[6];
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			post_log:1,
			attach_log:1,
	:		6;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			attach_log:1,
			post_log:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res4[2];
} tape_ms_page1_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else				/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif				/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		2,
			tb:1,
	:		1,
			eer:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			eer:1,
	:		1,
			tb:1,
	:		2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  read_retry_count;
	uchar_t	  res[4];
	uchar_t	  write_retry_count;
} exab_ms_page1_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		2,
			tb:1,
	:		1,
			eer:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			eer:1,
	:		1,
			tb:1,
	:		2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  read_retry_count;
	uchar_t	  res3[4];
	uchar_t	  write_retry_count;
	uchar_t	  res4[3];
} dlt_ms_page1_t;

/*
 *  Mode Sense Page 2.
 *
 *	Description:
 *	  Disconnect/Reconnect control page.
 */


typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  buffer_full_ratio;
	uchar_t	  buffer_empty_ratio;
	uchar_t	  bus_inactivity_limit_msb;
	uchar_t	  bus_inactivity_limit_lsb;
	uchar_t	  disconnect_time_limit_msb;
	uchar_t	  disconnect_time_limit_lsb;
	uchar_t	  connection_time_limit_msb;
	uchar_t	  connection_time_limit_lsb;
	uchar_t	  maximum_burst_size_msb;
	uchar_t	  maximum_burst_size_lsb;
}		sabre_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  buffer_full_ratio;
	uchar_t	  buffer_empty_ratio;
	uchar_t	  bus_inactivity_limit_msb;
	uchar_t	  bus_inactivity_limit_lsb;
	uchar_t	  disconnect_time_limit_msb;
	uchar_t	  disconnect_time_limit_lsb;
	uchar_t	  connection_time_limit_msb;
	uchar_t	  connection_time_limit_lsb;
	uchar_t	  maximum_burst_size_msb;
	uchar_t	  maximum_burst_size_lsb;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		6,
			dtdc:2;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dtdc:2,
	:		6;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res2[3];
} elite_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res[8];
	uchar_t	  maximum_burst_size_msb;
	uchar_t	  maximum_burst_size_lsb;
} ld4100_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  read_buffer_full_ratio;
	uchar_t	  write_buffer_full_ratio;
} multim_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  buffer_full_ratio;
	uchar_t	  res1[7];
	uchar_t	  max_burst_size_msb;
	uchar_t	  max_burst_size_lsb;
	uchar_t	  res2[4];
} hp1716_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  buffer_full_ratio;
	uchar_t	  buffer_empty_ratio;
	uchar_t	  bus_inactivity_limit_msb;
	uchar_t	  bus_inactivity_limit_lsb;
	uchar_t	  disconnect_time_limit_msb;
	uchar_t	  disconnect_time_limit_lsb;
	uchar_t	  connection_time_limit_msb;
	uchar_t	  connection_time_limit_lsb;
	uchar_t	  maximum_burst_size_msb;
	uchar_t	  maximum_burst_size_lsb;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		6,
			dtdc:2;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dtdc:2,
	:		6;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res2[3];
} dlt_ms_page2_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  buffer_full_ratio;
	uchar_t	  buffer_empty_ratio;
	uchar_t	  bus_inactivity_limit_msb;
	uchar_t	  bus_inactivity_limit_lsb;
	uchar_t	  disconnect_time_limit_msb;
	uchar_t	  disconnect_time_limit_lsb;
	uchar_t	  connection_time_limit_msb;
	uchar_t	  connection_time_limit_lsb;
	uchar_t	  maximum_burst_size_msb;
	uchar_t	  maximum_burst_size_lsb;
} exab_ms_page2_t;

/*
 *  Mode Sense Page 3.
 *
 *	Description:
 *	  Format Parameters.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  trk_per_zone_hi;	/* Track per zone */
	uchar_t	  trk_per_zone_lo;
	uchar_t	  alt_sctr_per_zone_hi;	/* Alternate sectors per zone */
	uchar_t	  alt_sctr_per_zone_lo;
	uchar_t	  alt_trk_per_zone_hi;	/* Alternate tracks per zone */
	uchar_t	  alt_trk_per_zone_lo;
	uchar_t	  alt_trk_per_vol_hi;	/* Alternate tracks per vol */
	uchar_t	  alt_trk_per_vol_lo;
	uchar_t	  sctr_per_trk_hi;	/* Sectors per track */
	uchar_t	  sctr_per_trk_lo;
	uchar_t	  byte_per_phy_sctr_hi;	/* Bytes per physical sector */
	uchar_t	  byte_per_phy_sctr_lo;
	uchar_t	  interleave_value_hi;	/* Interleave_value */
	uchar_t	  interleave_value_lo;
	uchar_t	  res1[8];
} sabre_ms_page3_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  trk_per_zone_hi;	/* Track per zone */
	uchar_t	  trk_per_zone_lo;
	uchar_t	  alt_sctr_per_zone_hi;	/* Alternate sectors per zone */
	uchar_t	  alt_sctr_per_zone_lo;
	uchar_t	  alt_trk_per_zone_hi;	/* Alternate tracks per zone */
	uchar_t	  alt_trk_per_zone_lo;
	uchar_t	  alt_trk_per_vol_hi;	/* Alternate tracks per vol */
	uchar_t	  alt_trk_per_vol_lo;
	uchar_t	  sctr_per_trk_hi;	/* Sectors per track */
	uchar_t	  sctr_per_trk_lo;
	uchar_t	  byte_per_phy_sctr_hi;	/* Bytes per physical sector */
	uchar_t	  byte_per_phy_sctr_lo;
	uchar_t	  interleave_value_hi;	/* Interleave_value */
	uchar_t	  interleave_value_lo;
	uchar_t	  track_skew_factor_hi;	/* Track skew factor */
	uchar_t	  track_skew_factor_lo;
	uchar_t	  cyl_skew_factor_hi;	/* Cylinder skew factor */
	uchar_t	  cyl_skew_factor_lo;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			ssec:1,
			hsec:1,
			rmb:1,
			surf:1,
			res1:4;
#else	/* defined(_BIT_FIELDS_HTOL) */
			res1:4,
			surf:1,
			rmb:1,
			hsec:1,
			ssec:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res2[3];
} elite_ms_page3_t;



/*
 *  Mode Sense Page 4.
 *
 *	Description:
 *	  Rigid disk drive geometry page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  max_no_cyl_hi;	/* Max no of cylinders */
	uchar_t	  max_no_cyl_mid;
	uchar_t	  max_no_cyl_lo;
	uchar_t	  max_no_heads;		/* Max no of heads */
	uchar_t	  res1[14];
} sabre_ms_page4_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  max_no_cyl_hi;	/* Max no of cylinders */
	uchar_t	  max_no_cyl_mid;
	uchar_t	  max_no_cyl_lo;
	uchar_t	  max_no_heads;		/* Max no of heads */
	uchar_t	  write_precomp_hi;	/* Starting cylinder */
	uchar_t	  write_precomp_mid;
	uchar_t	  write_precomp_lo;
	uchar_t	  reduced_wr_current_hi;	/* Starting cylinder */
	uchar_t	  reduced_wr_current_mid;
	uchar_t	  reduced_wr_current_lo;
	uchar_t	  drive_step_rate_hi;
	uchar_t	  drive_step_rate_lo;
	uchar_t	  landing_zone_cyl_hi;
	uchar_t	  landing_zone_cyl_mid;
	uchar_t	  landing_zone_cyl_lo;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		6,
			rpl:2;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rpl:2,
	:		6;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  rotational_offset;
	uchar_t	  res2;
	uchar_t	  rotation_rate_hi;
	uchar_t	  rotation_rate_lo;
	uchar_t	  res3[2];
} elite_ms_page4_t;



/*
 *  Mode Sense Page 6.
 *
 *	Description:
 *	  Optical memory  page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res1;
	uchar_t	  res2;
} hp1716_ms_page6_t;


/*
 *  Mode Sense Page 7.
 *
 *	Description:
 *	  Verify error recovery page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			err:1,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
			err:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  verify_retry_count;
	uchar_t	  verify_correction_span;
	uchar_t	  res2[5];
	uchar_t	  verify_recovery_time_hi;
	uchar_t	  verify_recovery_time_lo;
} elite_ms_page7_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			per:1,
			dte:1,
			dcr:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dcr:1,
			dte:1,
			per:1,
	:		5;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  verify_retry_count;
	uchar_t	  res3[8];
} hp1716_ms_page7_t;


/*
 *  Mode Sense Page 8.
 *
 *	Description:
 *	  Caching parameters.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			wce:1,
	:		1,
			rcd:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rcd:1,
	:		1,
			wce:1,
	:		5;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			read_priority:4,
			write_priority:4;
#else	/* defined(_BIT_FIELDS_HTOL) */
			write_priority:4,
			read_priority:4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  disable_pre_fetch_hi;
	uchar_t	  disable_pre_fetch_lo;
	uchar_t	  min_prefetch_hi;
	uchar_t	  min_prefetch_lo;
	uchar_t	  max_prefetch_hi;
	uchar_t	  max_prefetch_lo;
	uchar_t	  max_prefetch_ceiling_hi;
	uchar_t	  max_prefetch_ceiling_lo;
} elite_ms_page8_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			wce:1,
			mf:1,
			rcd:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rcd:1,
			mf:1,
			wce:1,
	:		5;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			read_priority:4,
			write_priority:4;
#else	/* defined(_BIT_FIELDS_HTOL) */
			write_priority:4,
			read_priority:4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  disable_pre_fetch_hi;
	uchar_t	  disable_pre_fetch_lo;
	uchar_t	  min_prefetch_hi;
	uchar_t	  min_prefetch_lo;
	uchar_t	  max_prefetch_hi;
	uchar_t	  max_prefetch_lo;
	uchar_t	  max_prefetch_ceiling_hi;
	uchar_t	  max_prefetch_ceiling_lo;
} multim_ms_page8_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		5,
			wce:1,
	:		1,
			rcd:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rcd:1,
	:		1,
			wce:1,
	:		5;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res3;
	uchar_t	  disable_pre_fetch_hi;
	uchar_t	  disable_pre_fetch_lo;
	uchar_t	  min_prefetch_hi;
	uchar_t	  min_prefetch_lo;
	uchar_t	  max_prefetch_hi;
	uchar_t	  max_prefetch_lo;
	uchar_t	  res4[2];
} hp1716_ms_page8_t;



/*
 *  Mode Sense Page A.
 *
 *	Description:
 *	  Control mode page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
			uchar_t
#if	defined(_BIT_FIELDS_HTOL)
	:		7,
			rlec:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rlec:1,
	:		7;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			queue_a_m:4,
	:		2,
			qerr:1,
			dque:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dque:1,
			qerr:1,
	:		2,
			queue_a_m:4;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			eeca:1,
	:		4,
			raenp:1,
			uaaenp:1,
			eaenp:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			eaenp:1,
			uaaenp:1,
			raenp:1,
	:		4,
			eeca:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res4;
	uchar_t	  ready_aen_holdoff_hi;
	uchar_t	  ready_aen_holdoff_lo;
} elite_ms_pagea_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		7,
			rlec:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rlec:1,
	:		7;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			queue_a_m:4,
	:		2,
			qerr:1,
			dque:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dque:1,
			qerr:1,
	:		2,
			queue_a_m:4;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			eeca:1,
	:		4,
			raenp:1,
			uaaenp:1,
			eaenp:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			eaenp:1,
			uaaenp:1,
			raenp:1,
	:		4,
			eeca:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res4;
	uchar_t	  ready_aen_holdoff_hi;
	uchar_t	  ready_aen_holdoff_lo;
} dlt_ms_pagea_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		7,
			rlec:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rlec:1,
	:		7;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			queue_a_m:4,
	:		2,
			qerr:1,
			dque:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dque:1,
			qerr:1,
	:		2,
			queue_a_m:4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			eeca:1,
	:		4,
			raenp:1,
			uaaenp:1,
			eaenp:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			eaenp:1,
			uaaenp:1,
			raenp:1,
	:		4,
			eeca:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res4;
	uchar_t	  ready_aen_holdoff_hi;
	uchar_t	  ready_aen_holdoff_lo;
} exab_ms_pagea_t;

/*
 *  Mode Sense Page B.
 *
 *	Description:
 *	  Medium types supported page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
	page_code:6,
	:1,
	save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res1;
	uchar_t	  res2;
	uchar_t	  type_one;
	uchar_t	  type_two;
	uchar_t	  res3[2];
} hp1716_ms_pageb_t;

/*
 *  Mode Sense Page F.
 *
 *	Description:
 *	  Data Compression Page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			DCE:1,
			DCC:1,
	:		6;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			DCC:1,
			DCE:1;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			DDE:1,
			RED:2,
	:		5;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		5,
			RED:2,
			DDE:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  comp_alg[4];
	uchar_t	  decomp_alg[4];
	uchar_t	  res3[4];
} exab_ms_pagef_t;

/*
 *  Mode Sense Page 10.
 *
 *	Description:
 *	  Control mode page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		1,
			cap:1,
			caf:1,
			active_format:5;
#else	/* defined(_BIT_FIELDS_HTOL) */
			active_format:5,
			caf:1,
			cap:1,
	:		1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  active_partition;
	uchar_t	  write_buffer_full_ratio;
	uchar_t	  read_buffer_empty_ratio;
	uchar_t	  write_delay_time_hi;
	uchar_t	  write_delay_time_lo;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			dbr:1,
			bis:1,
			rsmk:1,
			avc:1,
			socf:2,
			rbo:1,
			rew:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rew:1,
			rbo:1,
			socf:2,
			avc:1,
			rsmk:1,
			bis:1,
			dbr:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  gap_size;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			eod_defined:3,
			eeg:1,
			sew:1,
	:		3;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		3,
			sew:1,
			eeg:1,
			eod_defined:3;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  buffer_size_early_warn_hi;
	uchar_t	  buffer_size_early_warn_mid;
	uchar_t	  buffer_size_early_warn_lo;
	uchar_t	  data_compression;
	uchar_t	  res3;
} tape_ms_page10_t;

/*
 *  Mode Sense Page 11.
 *
 *	Description:
 *	  Control mode page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  max_additional_partitions;
	uchar_t	  additional_partitions_defined;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			fdp:1,
			sdp:1,
			idp:1,
			psum:2,
	:		3;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		3,
			psum:2,
			idp:1,
			sdp:1,
			fdp:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  medium_format_recongnition;
	uchar_t	  res2[2];
} dlt_ms_page11_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  max_additional_partitions;
	uchar_t	  additional_partitions_defined;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			fdp:1,
			sdp:1,
			idp:1,
			psum:2,
	:		3;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		3,
			psum:2,
			idp:1,
			sdp:1,
			fdp:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  medium_format_recongnition;
	uchar_t	  res2[2];
	uchar_t	  partition_size_hi;
	uchar_t	  partition_size_lo;
} exab_ms_page11_t;


/*
 *  Mode Sense Page 1D.
 *
 *	Description:
 *	Element Address Assignment Page, scsi robots.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t		page_length;
	ushort_t	first_tport;
	ushort_t	num_tport;
	ushort_t	first_stor;
	ushort_t	num_stor;
	ushort_t	first_mail;
	ushort_t	num_mail;
	ushort_t	first_drive;
	ushort_t	num_drive;
	uchar_t		res1;
	uchar_t		res2;
} robot_ms_page1d_t;

/*
 *  Mode Sense Page 1E.
 *
 *	Description:
 *	Transport Geometry Page, generic scsi robots with 16 transports.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	struct {
		uchar_t
#if defined(_BIT_FIELDS_HTOL)
			:7,
			rotate:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			rotate:1,
			:7;
#endif	/* _BIT_FIELDS_HTOL */
		uchar_t	set_member;
	} transport_sets[16];
} robot_ms_page1e_t;


/*
 *  Mode Sense Page 1FE.
 *
 *	Description:
 *	Device Capabilities Page, generic scsi robots.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;	/* 0x0e */
	uchar_t	/* which elements provide storage for media */
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			dt_store:1,
			ie_store:1,
			st_store:1,
			mt_store:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			mt_store:1,
			st_store:1,
			ie_store:1,
			dt_store:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res3;
	uchar_t	/* support for move medium */
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			mt_move_dt:1,
			mt_move_ie:1,
			mt_move_st:1,
			mt_move_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			mt_move_mt:1,
			mt_move_st:1,
			mt_move_ie:1,
			mt_move_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			st_move_dt:1,
			st_move_ie:1,
			st_move_st:1,
			st_move_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			st_move_mt:1,
			st_move_st:1,
			st_move_ie:1,
			st_move_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			ie_move_dt:1,
			ie_move_ie:1,
			ie_move_st:1,
			ie_move_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			ie_move_mt:1,
			ie_move_st:1,
			ie_move_ie:1,
			ie_move_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			dt_move_dt:1,
			dt_move_ie:1,
			dt_move_st:1,
			dt_move_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dt_move_mt:1,
			dt_move_st:1,
			dt_move_ie:1,
			dt_move_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res8;
	uchar_t	  res9;
	uchar_t	  res10;
	uchar_t	  res11;
	uchar_t	/* support for exchange medium */
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			mt_xchg_dt:1,
			mt_xchg_ie:1,
			mt_xchg_st:1,
			mt_xchg_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			mt_xchg_mt:1,
			mt_xchg_st:1,
			mt_xchg_ie:1,
			mt_xchg_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			st_xchg_dt:1,
			st_xchg_ie:1,
			st_xchg_st:1,
			st_xchg_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			st_xchg_mt:1,
			st_xchg_st:1,
			st_xchg_ie:1,
			st_xchg_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			ie_xchg_dt:1,
			ie_xchg_ie:1,
			ie_xchg_st:1,
			ie_xchg_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			ie_xchg_mt:1,
			ie_xchg_st:1,
			ie_xchg_ie:1,
			ie_xchg_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		4,
			dt_xchg_dt:1,
			dt_xchg_ie:1,
			dt_xchg_st:1,
			dt_xchg_mt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dt_xchg_mt:1,
			dt_xchg_st:1,
			dt_xchg_ie:1,
			dt_xchg_dt:1,
	:		4;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res16;
	uchar_t	  res17;
	uchar_t	  res18;
	uchar_t	  res19;
} robot_ms_page1f_t;


/*
 *  Mode Sense Page 20.
 *
 *	Description:
 *	  Host support of modify data pointer messages.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res1;
	uchar_t	  res2;
/*	This byte is odd and is defined by page 38 to make C work. */
/*	u_char   res3	    : 7; */
/*	u_char   host_mdp	: 1; */
} elite_ms_page20_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  language;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			write_verify:1,
			suppress_attn:1,
			read_ahead:1,
	:		2,
			auto_spin_up:1,
			parity_disable:1,
			disable_start_stop:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			disable_start_stop:1,
			parity_disable:1,
			auto_spin_up:1,
	:		2,
			read_ahead:1,
			suppress_attn:1,
			write_verify:1;
#endif	/* _BIT_FIELDS_HTOL */
} ld4100_ms_page20_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  number_groups_hi;
	uchar_t	  number_groups_lo;
	uchar_t	  block_group_hi;	/* following 6 bytes are for */
	uchar_t	  block_group_mid;	/* the 595 and 652 mega byte */
	uchar_t	  block_group_lo;	/* versions and are reserved */
	uchar_t	  spare_block_group_hi;	/* on the 1.2giga byte versions. */
	uchar_t	  spare_block_group_mid;
	uchar_t	  spare_block_group_lo;
} multim_ms_page20_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res1;
	uchar_t	  number_groups_hi;
	uchar_t	  number_groups_lo;
	uchar_t	  block_group_hi;
	uchar_t	  block_group_mid;
	uchar_t	  block_group_lo;
	uchar_t	  spare_block_group_hi;
	uchar_t	  spare_block_group_mid;
	uchar_t	  spare_block_group_lo;
	uchar_t	  sectores_in_track_0;
	uchar_t	  res2[2];
} hp1716_ms_page20_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			CT:1,
	:		1,
			ND:1,
	:		1,
			NBE:1,
			EBD:1,
			PE:1,
			NAL:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			NAL:1,
			PE:1,
			EBD:1,
			NBE:1,
	:		1,
			ND:1,
	:		1,
			CT:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			RTF:3,
			WTF:3,
	:		1,
			m112:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			m112:1,
	:		1,
			WTF:3,
			RTF:3;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  motion_threshold;
	uchar_t	  gap_threshold;
} exab_ms_page20_t;

/*  Mode Sense Page 21. */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  laser_on_interval;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			vu:1,
			appl:1,
	:		6;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		6,
			appl:1,
			vu:1;
#endif	/* _BIT_FIELDS_HTOL */
} multim_ms_page21_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			err:1,
			dsp_log:1,
			dm_log:1,
			cm_log:1,
			reset:1,
			das:1,
			dtis:1,
			dair:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			dair:1,
			dtis:1,
			das:1,
			reset:1,
			cm_log:1,
			dm_log:1,
			dsp_log:1,
			err:1;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			dwr:1,
			quick_disconn:1,
			memory_log:1,
			force_verify:1,
			dltw:1,
			q_log:1,
			task_log:1,
			time_stamp:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			time_stamp:1,
			task_log:1,
			q_log:1,
			dltw:1,
			force_verify:1,
			memory_log:1,
			quick_disconn:1,
			dwr:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  max_buffer_lat_hi;
	uchar_t	  max_buffer_lat_mid1;
	uchar_t	  max_buffer_lat_mid2;
	uchar_t	  max_buffer_lat_lo;
	uchar_t	  drive_retry_count;
	uchar_t	  autochanger_eject_dist;
	uchar_t	  phase_retry_count;
	uchar_t	  res1;
} hp1716_ms_page21_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		6,
			LPART:1,
			WWR:2;
#else	/* defined(_BIT_FIELDS_HTOL) */
			WWR:2,
			LPART:1,
	:		6;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res1[2];
} exab_ms_page21_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  mailslot_size;
	uchar_t	  error_recovery;
	uchar_t	  barcode_reader;
	uchar_t	  scsi_logging;
	uchar_t	  reserved_0;
	uchar_t	  power_secure;
	uchar_t	  report_recoveries;
	uchar_t	  hp_product_id;
	uchar_t	  stacker_mode;
	uchar_t	  scsi_id;
	uchar_t	  reserved_1;
	uchar_t	  dhcp;
	uchar_t	  rmc_ip_addr[4];
	uchar_t	  rmc_subnet_mask[4];
	uchar_t	  rmc_gateway[4];
} hp_c7200_ms_page21_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		ps:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		ps:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			light_off:1,
	:		5,
			noscanst:1,
			noscandt:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		7,
			light_off:1;
#endif	/* _BIT_FIELDS_HTOL */
			uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setopt3:1,
	:		1,
			loginfo:1,
	:		3,
			oexprt:1,
			NoPwCyc:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			NoPwCyc:1,
			oexprt:1,
	:		3,
			loginfo:1,
	:		1,
			setopt3:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setopt4:1,
	:		1,
			mixed_media:1,
			enabmag:1,
			enmlslt:1,
			enbcr:1,
			enabams:1,
	:		1;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		1,
			enabams:1,
			enbcr:1,
			enmlslt:1,
			enabmag:1,
			mixed_media:1,
	:		1,
			setopt4:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setemu:1,
			libemulacd:7;
#else	/* defined(_BIT_FIELDS_HTOL) */
			libemulacd:7,
			setemu:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setaddr:1,
			eleaddrsch:7;
#else	/* defined(_BIT_FIELDS_HTOL) */
			eleaddrsch:7,
			setaddr:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setbctype:1,
			barcodetype:7;
#else	/* defined(_BIT_FIELDS_HTOL) */
			barcodetype:7,
			setbctype:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			setopt8:1,
	:		6,
			enallua:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
			enallua:1,
	:		6,
			setopt8:1;
#endif
	uchar_t	  res2;
} plasmo_page21_t;

#define	PLASMON_G_NO_EMULATION	0	/* no emulation */




/*
 *  Mode sense page 22h.
 *  Data Compression Status Page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif				/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t	  res;
	uchar_t	  bytes_recieved[5];
	uchar_t	  bytes_written[5];
} exab_ms_page22_t;

/*
 *  Mode Sense Page 38.
 *
 *	Description:
 *	  Cache control page.
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			cache_res1:1,
			cache_wie:1,
			cache_res2:1,
			cache_ce:1,
			cache_tbl_size:4;
#else	/* defined(_BIT_FIELDS_HTOL) */
			cache_tbl_size:4,
			cache_ce:1,
			cache_res2:1,
			cache_wie:1,
			cache_res1:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  prefetch_threshold;
	uchar_t	  max_prefetch;
	uchar_t	  max_prefetch_multiplier;
	uchar_t	  min_prefetch;
	uchar_t	  min_prefetch_multipler;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		1,
			ascu_enable:1,
			elog:1,
	:		5;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		5,
			elog:1,
			ascu_enable:1,
	:		1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res3[7];
} sabre_ms_page38_t;

typedef struct {
	uchar_t			/* Last byte of elite pg20 */
#if defined(_BIT_FIELDS_HTOL)
		pg20_resv:7,
		pg20_host_mdp:1;
#else	/* defined(_BIT_FIELDS_HTOL) */
		pg20_host_mdp:1,
		pg20_resv:7;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			cache_res1:1,
			cache_wie:1,
			cache_res2:1,
			cache_ce:1,
			cache_tbl_size:4;
#else	/* defined(_BIT_FIELDS_HTOL) */
			cache_tbl_size:4,
			cache_ce:1,
			cache_res2:1,
			cache_wie:1,
			cache_res1:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  prefetch_threshold;
	uchar_t	  max_prefetch;
	uchar_t	  max_prefetch_multiplier;
	uchar_t	  min_prefetch;
	uchar_t	  min_prefetch_multipler;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
	:		1,
			ascu_enable:1,
			elog:1,
	:		5;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		5,
			elog:1,
			ascu_enable:1,
	:		1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res3[6];
} elite_ms_page38_t;


/*
 *  Mode Sense Page 00.
 *
 *	Description:
 *	  Unit attention parameters.
 */

typedef struct {
	uchar_t	  pg30_resv;	/* Last byte of elite pg38 */
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			save_parms:1,
	:		1,
			page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
			page_code:6,
	:		1,
			save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if	defined(_BIT_FIELDS_HTOL)
	:		2,
			deid:1,
			unit_attention:1,
	:		4;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		4,
			unit_attention:1,
			deid:1,
	:		2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res3;
} elite_ms_page00_t;

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		save_parms:1,
		:1,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:1,
		save_parms:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			qpe:1,
	:		1,
			iwv:1,
	:		5;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		5,
			iwv:1,
	:		1,
			qpe:1;
#endif	/* _BIT_FIELDS_HTOL */
} multim_ms_page0_t;


/*
 *  Sabre Mode Sense.
 *
 *	Description:
 *	  Seagate Sabre Disk Mode Sense Data Block.
 */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	sabre_ms_page1_t pg1;
	sabre_ms_page2_t pg2;
	sabre_ms_page3_t pg3;
	sabre_ms_page4_t pg4;
	sabre_ms_page38_t pg38;
} sabre_mode_sense_t;

/*
 *  Elite Mode Sense.
 *
 *	Description:
 *	  Seagate Elite Disk Mode Sense Data Block.
 */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	elite_ms_page1_t pg1;
	elite_ms_page2_t pg2;
	elite_ms_page3_t pg3;
	elite_ms_page4_t pg4;
	elite_ms_page7_t pg7;
	elite_ms_page8_t pg8;
	elite_ms_pagea_t pga;
	elite_ms_page20_t pg20;
	elite_ms_page38_t pg38;
	elite_ms_page00_t pg00;
} elite_mode_sense_t;

/*
 *  Tape Mode Sense Page 0
 *
 *	Description:
 */

typedef struct {
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
		:2,
		page_code:6;
#else	/* defined(_BIT_FIELDS_HTOL) */
		page_code:6,
		:2;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  page_length;
	uchar_t
#if defined(_BIT_FIELDS_HTOL)
			buf_tp_mark:1,
	:		7;
#else	/* defined(_BIT_FIELDS_HTOL) */
	:		7,
			buf_tp_mark:1;
#endif	/* _BIT_FIELDS_HTOL */
	uchar_t	  res2;
	uchar_t	  stop_count;
} tape_ms_page0_t;

/*
 *  Optical Mode Sense.
 *
 *	Description:
 */

typedef struct {
	optical_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	ld4100_ms_page1_t pg1;
	ld4100_ms_page2_t pg2;
	ld4100_ms_page20_t pg20;
} ld4100_mode_sense_t;

typedef struct {
	optical_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	ld4100_ms_page1_t pg1;
	ld4100_ms_page2_t pg2;
	ld4100_ms_page20_t pg20;
	ld4500_ms_page21_t pg21;
} ld4500_mode_sense_t;

/*
 *  IBM Multi-media optical Mode Sense.
 *
 *	Description:
 *	  IBM Multi-media optical Mode Sense
 */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	multim_ms_page0_t pg0;
	multim_ms_page1_t pg1;
	multim_ms_page2_t pg2;
	multim_ms_page8_t pg8;
	multim_ms_page20_t pg20;
	multim_ms_page21_t pg21;
} multim_mode_sense_t;

/*
 *  HP 1716 optical Mode Sense.
 *
 *	Description:
 *	  HP 1716optical Mode Sense
 */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
	hp1716_ms_page1_t pg1;
	hp1716_ms_page2_t pg2;
	hp1716_ms_page6_t pg6;
	hp1716_ms_page7_t pg7;
	hp1716_ms_page8_t pg8;
	hp1716_ms_pageb_t pgb;
	hp1716_ms_page20_t pg20;
	hp1716_ms_page21_t pg21;
} hp1716_mode_sense_t;

/*  One transport generic scsi robot Mode Sense */

typedef struct {
	robot_ms_hdr_t  hdr;
	robot_ms_page1d_t pg1d;
	robot_ms_page1e_t pg1e;
	robot_ms_page1f_t pg1f;
} robot_mode_sense_t;

/*
 *  Tape Mode Sense.
 *
 *	Description:
 */

typedef struct {
	tape_ms_hdr_t   hdr;
	generic_blk_desc_t blk_desc;
	tape_ms_page1_t pg1;
	tape_ms_page0_t pg0;
} tape_mode_sense_t;


/*
 *  Video Mode Sense.
 *
 *	Description:
 */

typedef struct {
	video_ms_hdr_t  hdr;
	video_blk_desc_t blk_desc;
} video_mode_sense_t;

/*
 *  STK Mode Sense.
 *
 *	Description:
 */

typedef struct {
	video_ms_hdr_t  hdr;
	generic_blk_desc_t blk_desc;
	tape_ms_page1_t pg1;
	tape_ms_page10_t pg10;
	tape_ms_page0_t pg0;
} stk_mode_sense_t;


/*
 *  DLT tape mode sense.
 *
 *	Description:
 *	  DLT tape mode sense
 */

typedef struct {
	dlt_ms_hdr_t    hdr;
	generic_blk_desc_t blk_desc;
	dlt_ms_page1_t  pg1;
	dlt_ms_page2_t  pg2;
	dlt_ms_pagea_t  pga;
	tape_ms_page10_t pg10;
	dlt_ms_page11_t pg11;
} dlt_mode_sense_t;

/*
 *  exabyte tape mode sense.
 *
 *	Description:
 *	  exabyte tape mode sense
 */

typedef struct {
	exab_ms_hdr_t   hdr;
	generic_blk_desc_t blk_desc;
	exab_ms_page1_t pg1;
	exab_ms_page2_t pg2;
	exab_ms_pagea_t pga;
	exab_ms_pagef_t pgf;
	tape_ms_page10_t pg10;
	exab_ms_page11_t pg11;
	exab_ms_page20_t pg20;
	exab_ms_page21_t pg21;
	exab_ms_page22_t pg22;
} exab_mode_sense_t;


/*  Generic Mode Sense. --- */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
} generic_mode_sense_t;

typedef struct mode_sense {
	union {
		generic_mode_sense_t generic_ms;
		elite_mode_sense_t elite_ms;
		sabre_mode_sense_t sabre_ms;
		tape_mode_sense_t tape_ms;
		video_mode_sense_t video_ms;
		ld4100_mode_sense_t ld4100_ms;
		ld4500_mode_sense_t ld4500_ms;
		robot_mode_sense_t robot_ms;
		multim_mode_sense_t multim_ms;
		dlt_mode_sense_t dlt_ms;
		exab_mode_sense_t exab_ms;
		hp1716_mode_sense_t hp1716_ms;
		stk_mode_sense_t stk_ms;
	} u;
} mode_sense_t;

/*
 *  Mode Select.
 *
 *	Description:
 *	 The mode select for the FORMAT command.
 */

typedef struct {
	generic_ms_hdr_t hdr;
	generic_blk_desc_t blk_desc;
} fmt_mode_select_t;


#endif	/* _AML_MODE_SENSE_H */
