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

#define	SAM_FSA_LOG_PATH	"/var/opt/SUNWsamfs/fsalogd"

typedef	enum	{
	fstat_none = 0,
	fstat_done,
	fstat_part,
	fstat_error,
	fstat_missing,
	fstat_removed
}	fstatus_t;

typedef	struct	{			/* FSA log file table entry:	*/
	char		file_name[60];	/* File name of log file	*/
	sam_time_t	init_time;	/* Initial log time		*/
	sam_time_t	last_time;	/* Last log time processed	*/
	off_t		offset;		/* Next entry to process offset	*/
	short		status;		/* File status			*/
}	fsalog_file_t;

typedef	struct	{			/* FSA log file inventory table	*/
	char		fs_name[40];	/* Family set name 		*/
	int		l_fsn;		/* Family set name string length */
	int		n_logs;		/* Number of log files		*/
	int		n_alloc;	/* Number of entries allocated	*/
	int		c_log;		/* Ordinal of current log file	*/
	fsalog_file_t	logs[1];	/* Log file table		*/
}	fsalog_inv_t;

void FSA_log_path(char *);
int FSA_load_inventory(fsalog_inv_t **, char *, char *);
int FSA_update_inventory(fsalog_inv_t **);
int FSA_save_inventory(fsalog_inv_t  *, int);
int FSA_print_inventory(fsalog_inv_t  *, FILE *);

int FSA_next_log_file(fsalog_inv_t  *, int);
int FSA_open_log_file(fsalog_inv_t  *, int, int);
int FSA_close_log_file(fsalog_inv_t  *, int);
int FSA_read_next_event(fsalog_inv_t  *, sam_event_t *);
