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
#ifndef _REPORT_H
#define	_REPORT_H

#pragma	ident	"$Revision: 1.17 $"

/*
 *	report.h --  SAM-FS APIs for report configuration and control
 */

#include "sqm_list.h"

#define	REPORT_DIR		VAR_DIR"/reports"
#define	FSMGMT_MEDIA_XML_REPORT	REPORT_DIR"/media"
#define	FSMGMT_FS_XML_REPORT	REPORT_DIR"/fs"
#define	FSMGMT_ACSLS_XML_REPORT	REPORT_DIR"/acsls"
#define	XML_VERSION_STRING	"<?xml version='1.0'?>"
#define	FS_EVENT_FILE	REPORT_DIR"/fs_event"

typedef struct acsls_report {
	time_t	log_time;
	char	log_date[128];		/* using format strftime */
	char	acsls_host[256];
	char	acsls_port[32];
	/*
	 *	various information lists.
	 */
	sqm_lst_t *drive_list;		/* acsls drive info list */
	sqm_lst_t *accessedvolume_list;	/* volume access list */
	sqm_lst_t *enteredvolume_list;	/* entered volume list */
	sqm_lst_t *lock_list;		/* acsls drive lock list */
	sqm_lst_t *pool_list;		/* current scratch pool list */

} acsls_report_t;

typedef enum report_type {
ALL_REPORT,
FS_REPORT,
MEDIA_REPORT,
ACSLS_REPORT
}report_type_t;


typedef struct report_requirement {
	sqm_lst_t	*email_names;
	uint32_t	section_flag;
	report_type_t report_type;
} report_requirement_t;


/*
 *	One API function to generate report.
 *
 * The reports will be stored in /var/opt/SUNWsamfs/reports
 * A subsequent call to list_directory with the appropriate
 * filter ('fs_*.xml', 'acsls_*.xml', 'media_*.xml') will
 * list the reports
 */
int32_t gen_report(ctx_t *ctx,
	report_requirement_t *report_req);

/*
 *	The following functions generate all report contents.
 *	report directory will be defined in the file.
 *	report name is given by customer.
 */
int32_t gen_fs_report(report_requirement_t *report_req);
int32_t gen_acsls_report(report_requirement_t *report_req);
int32_t gen_media_report(report_requirement_t *report_req);

/*
 * media report section flags
 * The flags are the same as the status bit definitions
 * See include/aml/catalog.h for status bit definitions
 *
 * CES_archfull		0x00000800
 * CES_partitioned	0x00001000	not supported in GUI
 * CES_reconcile	0x00002000	not supported in GUI
 * CES_dupvsn		0x00004000
 * CES_priority		0x00008000	not supported in GUI
 * CES_capacity_set	0x00010000	not supported in GUI
 * CES_accessed_today	0x00020000	only in GUI
 * CES_blank		0x00040000	only in GUI
 * CES_non_sam		0x00080000
 * CES_export_slot	0x00100000	not supported in GUI
 * CES_unavail		0x00200000
 * CES_recycle		0x00400000
 * CES_read_only	0x00800000
 * CES_writeprotect	0x01000000
 * CES_bar_code		0x02000000	not supported in GUI
 * CES_cleaning		0x04000000	not supported in GUI
 * CES_occupied		0x08000000	not supported in GUI
 * CES_bad_media	0x10000000
 * CES_labeled		0x20000000	not supported in GUI
 * CES_inuse		0x40000000
 * CES_needs_audit	0x80000000	not supported in GUI
 *
 */
#define	CES_blank		0x00040000
#define	CES_accessed_today	0x00020000

#define	INCLUDE_VSN_SUMMARY	0x00000001
#define	INCLUDE_POOL_SUMMARY	0x00000002
#define	INCLUDE_UTIL_SUMMARY	0x00000004

/* ACSLS report section flags */
#define	ACSLS_DRIVE_SUMMARY	0x00004000
#define	ACSLS_POOL		0x00008000
#define	ACSLS_ENTERED_VOLUME	0x00010000
#define	ACSLS_ACCESSED_VOLUME	0x00020000
#define	ACSLS_LOCK		0x00040000

#endif	/* _REPORT_H */
