/*
 * damage.c - stager's damage file processing
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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/syscall.h"
#include "aml/tar.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "copy_defs.h"
#include "file_defs.h"
#include "rmedia.h"
#include "stream.h"

#include "stage_reqs.h"
#include "schedule.h"

static boolean_t isDamageError(int error);
static void damageFile(char *name, int	copy);

static char pathBuffer[PATHBUF_SIZE];

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/*
 * Damage current archive copy.
 */
boolean_t
DamageArcopy(
	FileInfo_t *file)
{
	int segment_ord;
	boolean_t damaged = B_FALSE;

	if (isDamageError(file->error)) {
		GetFileName(file, &pathBuffer[0], PATHBUF_SIZE, &segment_ord);
		/*
		 * If segment file, remove ordinal from file name.
		 */
		if (segment_ord > 0) {
			char *p;

			p = strrchr(&pathBuffer[0], '/');
			if (p != NULL) {
				*p = '\0';
			}
		}
		damageFile(pathBuffer, file->copy);
		SendCustMsg(HERE, 19029,
		    pathBuffer, file->id.ino, file->id.gen, file->copy + 1);
		damaged = B_TRUE;
	}
	/*
	 * May not have actually damaged the copy in the inode but
	 * set our damaged flag so we don't try to use this copy
	 * again.
	 */
	file->ar[file->copy].flags |= STAGE_COPY_DAMAGED;
	return (damaged);
}

/*
 * Return TRUE if error should damage copy.
 */
static boolean_t
isDamageError(
	int error)
{
	boolean_t damage;

	switch (error) {
		case ESRCH:
		case ETIME:
		case ECOMM:
		case EBUSY:
		case EAGAIN:
		case ECONNRESET:
		case ENODEV:
		case EINTR:
			damage = B_FALSE;
			break;
		default:
			damage = B_TRUE;
	}
	return (damage);
}

/*
 * Damage file's archive copy.
 */
static void
damageFile(
	char *name,
	int copy)
{
	int syserr;
	struct sam_archive_copy_arg arg;

	Trace(TR_MISC, "Damage file: '%s' copy %d", name, copy + 1);

	memset(&arg, 0, sizeof (arg));
	arg.operation = OP_damage;
	arg.path.ptr = name;
	arg.copies = 1 << copy;

	syserr = sam_syscall(SC_archive_copy, &arg, sizeof (arg));
	if (syserr == -1) {
		WarnSyscallError(HERE, "SC_archive_copy", name);
	}
}
