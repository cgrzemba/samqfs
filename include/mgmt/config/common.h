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
 * common.h
 * Common structures used in other header files.
 */


#ifndef _CFG_COMMON_H
#define	_CFG_COMMON_H

#pragma ident   "$Revision: 1.18 $"

/* move to parser_utils.h once notify is updated */
#define	MAX_LINE 1024

/* define commonly-used paths */
#define	SAMPKGNAM	"SUNWsamfs"
#define	PKGNAM		SAMPKGNAM

#ifdef SAM_DIR
#undef SAM_DIR
#endif
#define	SAM_DIR		"/opt/"PKGNAM

#define	OPT_DIR		SAM_DIR
#define	VAR_DIR		"/var"OPT_DIR
#define	ETC_DIR		"/etc"OPT_DIR
#define	CFG_DIR		ETC_DIR
#define	SBIN_DIR	OPT_DIR"/sbin"
#define	BIN_DIR		OPT_DIR"/bin"
#define	SCRIPT_DIR	ETC_DIR"/scripts"
#define	TMPFILE_DIR	VAR_DIR"/tmpfiles"
#define	DEVLOG_DIR	VAR_DIR"/devlog"

#define	SAMSTDIR	"/dev/samst"
#define	TAPEDEVDIR	"/dev/rmt"
#define	SAMST_CFG	"/kernel/drv/samst.conf"

#define	MCF_CFG		CFG_DIR"/mcf"
#define	DEFAULTS_CFG	CFG_DIR"/defaults.conf"
#define	SAMFS_CFG	CFG_DIR"/samfs.cmd"
#define	ARCHIVER_CFG	CFG_DIR"/archiver.cmd"
#define	DISKVOL_CFG	CFG_DIR"/diskvols.conf"
#define	RELEASE_CFG	CFG_DIR"/releaser.cmd"
#define	STAGE_CFG	CFG_DIR"/stager.cmd"
#define	PREVIEW_CFG	CFG_DIR"/preview.cmd"
#define	RECYCLE_CFG	CFG_DIR"/recycler.cmd"
#define	CFG_BACKUP_DIR	CFG_DIR"/cfg_backups"
#define	INQUIRY_PATH	CFG_DIR"/inquiry.conf"
#define	CLASS_FILE	CFG_DIR"/data_class.cmd"

#define	HWM_RECORDLOG	VAR_DIR"/faults/recordhwm.log"
#define	EMAIL_RECORDLOG	VAR_DIR"/faults/email.log"

/* define commonly used sizes */
#define	KILO	(uint64_t)(1024LL)
#define	MEGA	(uint64_t)(1024LL * 1024)
#define	GIGA	(uint64_t)(1024LL * 1024 * 1024)
#define	TERA	(uint64_t)(1024LL * 1024 * 1024 * 1024)
#define	PETA	(uint64_t)(1024LL * 1024 * 1024 * 1024 * 1024)
#define	EXA	(uint64_t)(1024LL * 1024 * 1024 * 1024 * 1024 * 1024)

/* define common keys */
#define	KEY_NAME	"name"
#define	KEY_TYPE	"type"
#define	KEY_STATE	"state"
#define	KEY_PATH	"path"
#define	KEY_FLAGS	"flags"
#define	KEY_SIZE	"size"
#define	KEY_CAPACITY	"capacity"
#define	KEY_FREE	"free"
#define	KEY_USAGE	"usage"
#define	KEY_MODTIME	"modtime"
#define	KEY_STATUS	"status"

#endif /* _CFG_COMMON_H */
