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
#if !defined(FSALOG_H)
#define	FSALOG_H

#include "sam/types.h"

#define	FSA_DEFAULT_LOG_PATH	"/var/opt/SUNWsamfs/fsalogd"
#define	FSA_LOGNAME_MAX		60
#define	FSA_EOF			0

typedef	enum	{
	fstat_none = 0,
	fstat_done,
	fstat_part,
	fstat_error,
	fstat_missing,
	fstat_MAX,
	fstat_MARK = 1<<15
}	sam_fsa_status_t;

typedef	struct {			/* FSA ondisk inventory entry	*/
	char		name[FSA_LOGNAME_MAX]; /* File name of log file	*/
	sam_time_t	init_time;	/* Initial log time		*/
	sam_time_t	last_time;	/* Last log time processed	*/
	off_t		offset;		/* Next entry to process offset	*/
	short		status;		/* File status			*/
}	sam_fsa_log_t;

typedef	struct {			/* FSA inventory table		*/
	uname_t		fs_name;	/* Family set name 		*/
	int		l_fsn;		/* Family set name string length */
	int		n_logs;		/* Number of log files		*/
	int		n_alloc;	/* Number of entries allocated	*/
	int		c_log;		/* Ordinal of current log file	*/
	int		fd_inv;		/* Inventory file descriptor	*/
	int		fd_log;		/* Current log file descriptor	*/
	char		*path_fsa;	/* FSA directory path		*/
	char 		*path_inv;	/* Inventory file path/name	*/
	char 		*path_log;	/* Log file path/name		*/
	sam_time_t	last_time;	/* Time of last event read	*/
	sam_fsa_log_t	logs[1];	/* Log file table		*/
}	sam_fsa_inv_t;

/* FSA management functions */
int sam_fsa_open_inv(sam_fsa_inv_t **inv, char *path,
    char *fs_name, char *appname);
int sam_fsa_read_event(sam_fsa_inv_t **inv, sam_event_t *event);
int sam_fsa_rollback(sam_fsa_inv_t **inv);
int sam_fsa_print_inv(sam_fsa_inv_t *inv, FILE *file);
int sam_fsa_close_inv(sam_fsa_inv_t **inv);

#endif /* FSALOG_H */
