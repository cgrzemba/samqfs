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
#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/vfs.h>

#include <sam/custmsg.h>
#include <sam/mount.h>
#include <sam/lib.h>
#include <sam/sam_trace.h>
#include <sam/sam_malloc.h>
#include <sam/fs/sblk.h>

#include "util.h"

extern char *program_name;

#define	INODE_BUF_SIZE (INO_BLK_FACTOR * INO_BLK_SIZE)

/*
 * init_trace - initializes sam_trace.h depending on whether
 * caller is a Daemon or command line process.
 */
void
init_trace(int is_daemon, int trace_id)
{
	CustmsgInit(is_daemon, NULL);

	/* Set up tracing.  */
	if (is_daemon) {
		/* Provide file descriptors for the standard files.  */
		(void) close(STDIN_FILENO);
		if (open("/dev/null", O_RDONLY) != STDIN_FILENO) {
			LibFatal(open, "stdin /dev/null");
		}
		(void) close(STDOUT_FILENO);
		if (open("/dev/null", O_RDWR) != STDOUT_FILENO) {
			LibFatal(open, "stdout /dev/null");
		}
		(void) close(STDERR_FILENO);
		if (open("/dev/null", O_RDWR) != STDERR_FILENO) {
			LibFatal(open, "stderr /dev/null");
		}

		TraceInit(program_name, trace_id | TR_MPLOCK | TR_MPMASTER);
	} else {
		/* Redirect trace messages to stdout/stderr.  */
		TraceInit(NULL, TI_none);
		*TraceFlags = (1 << TR_date) - 1;
		*TraceFlags &= ~(1 << TR_alloc);
		*TraceFlags &= ~(1 << TR_dbfile);
		*TraceFlags &= ~(1 << TR_files);
		*TraceFlags &= ~(1 << TR_oprmsg);
		*TraceFlags &= ~(1 << TR_sig);
	}
}

/*
 * Utility for reading the .inodes file.  Reads the inodes
 * using buffered input, using a callback function to process each
 * inode read.
 *
 * mp - mount point of sam filesystem to read .inodes
 * callback - callback function to call for each inode
 * arg - passthrough argument for callback function
 *
 * Returns 0 on success (all callbacks successful), -1 on failure
 */
int
read_inodes(char *mp, readinode_cb_f callback, void *arg) {
	int inode_fd = -1;
	int err = 0;
	int nbytes;
	sam_perm_inode_t *inode_buf = NULL;

	if ((inode_fd = OpenInodesFile(mp)) < 0) {
		Trace(TR_ERR, "Could not open .inodes file for %s\n", mp);
		return (-1);
	}

	SamMalloc(inode_buf, INODE_BUF_SIZE);
	while ((nbytes = read(inode_fd, inode_buf, INODE_BUF_SIZE)) > 0) {
		int ninodes = nbytes / sizeof (sam_perm_inode_t);
		int i;

		for (i = 0; i < ninodes; i++) {
			if (inode_buf[i].di.id.ino == 0) {
				continue;
			}
			if (callback(&inode_buf[i], arg) < 0) {
				err = -1;
				goto out;
			}
		}
	}

out:
	SamFree(inode_buf);
	return (err);
}
