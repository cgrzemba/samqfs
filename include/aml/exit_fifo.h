/*
 * exit_fifo.h - declares/macros for using exit FIFOs w/ command responses.
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


#if !defined(_AML_EXIT_FIFO_H)
#define	_AML_EXIT_FIFO_H

#pragma ident "$Revision: 1.9 $"

#define	EXIT_FIFO_PATH "/tmp/.EXIT-FIFO-p%dt%d.%d"

#define	EXIT_FAILED 	-1
#define	EXIT_OK		0
/* anything > 0 is errno */

/* short-hand functions name */
#define	write_client_exit(id, code) \
	write_client_exit_string(id, code, NULL)

#define	get_exit_fifo_path(s, id) \
	(void) sprintf(s, EXIT_FIFO_PATH, (int)id->pid, id->thr_id, id->seqnum)

#define	EEID(event) (&event->request.message.exit_id)

#define	write_event_exit(event, code, string) \
	write_client_exit_string(&event->request.message.exit_id, code, string)

typedef struct {
	long	pid;
	int	thr_id;
	int	seqnum;
} exit_FIFO_id;

typedef struct {
	int	code;
	int	length;
} exit_FIFO_header;

int create_exit_FIFO(exit_FIFO_id *);
int unlink_exit_FIFO(exit_FIFO_id *);
void write_client_exit_string(exit_FIFO_id *, int, char *);
int read_server_exit_string(exit_FIFO_id *, int *, char *, int, int);
void set_exit_id(int, exit_FIFO_id *);
int timeout_factor(equ_t);

extern char *exit_completed;
extern char *exit_unknown;

#endif /* !defined(_AML_EXIT_FIFO_H) */
