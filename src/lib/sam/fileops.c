/*
 * fileops.c - Perform operations on a SAMFS file.
 *
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

#pragma ident "$Revision: 1.22 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
	/* None. */

/* POSIX headers. */
#include <stropts.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* Solaris headers. */
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <errno.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "pub/lib.h"
#include "sam/fioctl.h"
#include "sam/syscall.h"
#include "pub/sam_errno.h"
#include "pub/stat.h"
#include "pub/listio.h"
#include "sam/checksum.h"
#include "sam/checksumf.h"
#include "sam/samioc.h"

/* Local headers. */
	/* None. */

/* Macros. */
	/* None. */

/* Types. */
	/* None. */

/* Structures. */
	/* None. */

/* Private data. */
	/* None. */

/* Private functions. */
static int compute_checksum(const char *path, csum_t *csum);
static int check_args_consistency(struct sam_archive_copy_arg *arg);
static int parse_args(struct sam_archive_copy_arg *arg,
			int num_opts, va_list opts_list);
static int check_permissions();

/* Public data. */
#pragma weak cs_simple

/* Function macros. */
	/* None. */

/* Signal catching functions. */
	/* None. */


/*
 *	Archive operations.
 *	d - default, i - immediate, n - never, C - concurrent
 */
int
sam_archive(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	return (sam_syscall(SC_archive, &arg, sizeof (arg)));
}

/*
 *	Remove archive copy/copies.
 */
int
sam_unarchive(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}
	if (check_args_consistency(&arg) != 0) {
		errno = EINVAL;
		return (-1);
	}

	arg.operation = OP_unarchive;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Rearchive archive copy.
 */
int
sam_rearch(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}
	if (check_args_consistency(&arg) != 0) {
		errno = EINVAL;
		return (-1);
	}

	arg.operation = OP_rearch;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Remove rearchive attributes.
 */
int
sam_unrearch(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}
	if (check_args_consistency(&arg) != 0) {
		errno = EINVAL;
		return (-1);
	}

	arg.operation = OP_unrearch;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Exchange archive copies.
 */
int
sam_exarchive(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}

	if (arg.ncopies != 2) {
		fprintf(stderr, "Source copy and Destination copy"
		    "must be specified\n");
		errno = EINVAL;
		return (-1);
	}

	arg.copies &= ~(1 << arg.dcopy); /* Remove destination copy */

	arg.operation = OP_exarchive;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Set status as damaged.
 */
int
sam_damage(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}
	if (check_args_consistency(&arg) != 0) {
		errno = EINVAL;
		return (-1);
	}

	arg.operation = OP_damage;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Clear damaged and stale status.
 */
int
sam_undamage(
	const char *path,	/* Pathname */
	int num_opts,		/* Number of operations */
	...)			/* Operations */
{
	va_list 	ops_list;
	struct 		sam_archive_copy_arg arg;

	if (check_permissions() != 0) {
		return (-1);
	}

	memset(&arg, 0, sizeof (arg));
	arg.path.ptr = path;

	va_start(ops_list, num_opts);
	if (parse_args(&arg, num_opts, ops_list) != 0) {
		return (-1);
	}
	if (check_args_consistency(&arg) != 0) {
		errno = EINVAL;
		return (-1);
	}

	arg.operation = OP_undamage;
	return (sam_syscall(SC_archive_copy, &arg, sizeof (arg)));
}

/*
 *	Cancel stage.
 */
int
sam_cancelstage(
	const	char *path)	/* Pathname */
{
	struct sam_fileops_arg	arg;

	arg.path.ptr = path;
	arg.ops.ptr = "\0";
	return (sam_syscall(SC_cancelstage, &arg, sizeof (arg)));
}


/*
 *	Release operations.
 *	d - default, a - after archive, i - immediate, n - never
 */
int
sam_release(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	return (sam_syscall(SC_release, &arg, sizeof (arg)));
}


/*
 *	Stage operations.
 *	d - default, a - associative, i - immediate, n - never, p - partial
 */
int
sam_stage(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	return (sam_syscall(SC_stage, &arg, sizeof (arg)));
}


/*
 *	Verify data file.
 *	d - default
 *	e - verify copies after archive
 *	g - generate [checksum]
 *	n - specify [checksum algorithm (where n is a digit)]
 *	u - use [checksum for staging]
 */
int
sam_ssum(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;
	int error;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	if (strchr(ops, 'G')) {
		csum_t csum;

		if ((error = compute_checksum(path, &csum)) == 0) {
			struct sam_set_csum_arg arg;
			int error;

			arg.path.ptr = path;
			arg.csum = csum;
			if ((error = sam_syscall(SC_setcsum, &arg,
			    sizeof (arg))) == 0) {
				return (error);
			}
		} else {
			errno = error;
			return (-1);
		}
	}
	return (sam_syscall(SC_ssum, &arg, sizeof (arg)));
}


/*
 *	Set file attributes
 *	d - default, b - directio, l - preallocate length, s - stride
 */
int
sam_setfa(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	return (sam_syscall(SC_setfa, &arg, sizeof (arg)));
}


/*
 *	Set segment access
 *	d - default, l - segment length, s - segment stage width
 */
int
sam_segment(
	const char *path,	/* Pathname */
	const char *ops)	/* Operations */
{
	struct sam_fileops_arg arg;

	arg.path.ptr = path;
	arg.ops.ptr = ops;
	return (sam_syscall(SC_segment, &arg, sizeof (arg)));
}


/*
 *	Advise file operating attributes
 *	d - default, b - directio, p - buffers locked, s - stride
 */
int
sam_advise(
	const int fildes,
	const char *ops)	/* Operations */
{
	return (ioctl(fildes, F_ADVISE, ops));
}


/*
 *	listio read
 */
int
qfs_lio_read(int fd, int mem_list_count, void **mem_addr, size_t *mem_count,
	int file_list_count, offset_t *file_off, offset_t *file_len,
	qfs_lio_handle_t *hdl)
{
	sam_listio_t lio;

	lio.mem_list_count = mem_list_count;
	lio.mem_addr = mem_addr;
	lio.mem_count = mem_count;
	lio.file_list_count = file_list_count;
	lio.file_off = file_off;
	lio.file_len = file_len;
	lio.wait_handle = hdl;
	if (hdl)
		*hdl = (*hdl & 0xffffffff) |  ((qfs_lio_handle_t)fd << 32);

	return (ioctl(fd, F_LISTIO_RD, &lio));
}

/*
 *	listio write
 */
int
qfs_lio_write(int fd, int mem_list_count, void **mem_addr, size_t *mem_count,
	int file_list_count, offset_t *file_off, offset_t *file_len,
	qfs_lio_handle_t *hdl)
{
	sam_listio_t lio;

	lio.mem_list_count = mem_list_count;
	lio.mem_addr = mem_addr;
	lio.mem_count = mem_count;
	lio.file_list_count = file_list_count;
	lio.file_off = file_off;
	lio.file_len = file_len;
	lio.wait_handle = hdl;
	if (hdl)
		*hdl = (*hdl & 0xffffffff) |  ((qfs_lio_handle_t)fd << 32);

	return (ioctl(fd, F_LISTIO_WR, &lio));
}


/*
 *	listio wait
 */
int
qfs_lio_wait(qfs_lio_handle_t *hdl)
{
	return (ioctl(*hdl >> 32, F_LISTIO_WAIT, hdl));
}


/*
 * listio init
 */
int
qfs_lio_init(qfs_lio_handle_t *hdl)
{
	*hdl = 0;
	return (0);
}


static unsigned char buf[64*1024];

static int
compute_checksum(const char *path, csum_t *csum)
{
	time_t mtime;
	struct sam_stat sbuf;
	int fsize;
	int fd;
	int read_size;

	if (sam_stat(path, &sbuf, sizeof (sbuf)) < 0) {
		return (errno);
	}
	if (SS_ISOFFLINE(sbuf.attr)) {
		return (ER_FILE_IS_OFFLINE);
	}
	mtime = sbuf.st_mtime;
	cs_simple(&sbuf.st_size, 0, 0, csum);
	fsize = (int)sbuf.st_size;
	if ((fd = open(path, O_RDONLY)) < 0) {
		return (errno);
	}
	while (fsize > 0) {
		if ((read_size = read(fd, buf, 64*1024)) < 0) {
			return (errno);
		}
		cs_simple(NULL, buf, read_size, csum);
		fsize -= read_size;
	}
	close(fd);
	if (sam_stat(path, &sbuf, sizeof (sbuf)) < 0) {
		return (errno);
	}
	if (sbuf.st_mtime != mtime) {
		return (EARCHMD);
	}
	return (0);
}

/*
 *	Ascertain super-user privilege level to execute command
 */
static int
check_permissions()
{
	if (geteuid() != 0) {
		errno = EPERM;
		return (-1);
	}

	return (0);
}

/*
 *	Parse options
 */
static int
parse_args(struct sam_archive_copy_arg *arg, int num_args, va_list opts_list)
{
	long 	copy_num;
	char 	*opt, *media, *vsn;


	while (num_args--) {
		opt = va_arg(opts_list, char *);
		switch (*opt) {
		case 'M':	/* Metadata */
			arg->flags |= SU_meta;
			break;

		case 'o':	/* online */
			arg->flags |= SU_online;
			break;

		case 'a':	/* archive */
			arg->flags |= SU_archive;
			break;

		case 'c':	/* copy number(s) */
			copy_num = strtol(++opt, NULL, 10) - 1;
			if (copy_num >= 0 && copy_num < MAX_ARCHIVE) {
				arg->copies |= 1 << copy_num;
				arg->ncopies++;
			if (arg->ncopies == 2) {
				arg->dcopy = (uchar_t)copy_num;
			}
			} else {
				fprintf(stderr,
				    "copy must be : 1 << c <= %d\n",
				    MAX_ARCHIVE);
				errno = EINVAL;
				return (-1);
			}
			break;

		case 'm':	/* media */
			media = opt+1;
			arg->media = sam_atomedia(media);
			if (arg->media == 0) {
				fprintf(stderr, "Unknown media\n");
				errno = EINVAL;
				return (-1);
			}
			break;

		case 'v':	/* and vsn */
			vsn = opt+1;
			strncpy(arg->vsn, vsn, sizeof (vsn_t));
			break;

		default :
			errno = EINVAL;
			return (-1);
		}
	}

	va_end(opts_list);

	return (0);
}

/*
 *	Consistency checking
 */
static int
check_args_consistency(struct sam_archive_copy_arg *arg)
{
	if (*arg->vsn != '\0' && arg->media == 0) {
		fprintf(stderr, "-m must be specified if using -v\n");
		return (-1);
	}

	if (arg->copies == 0 && arg->media == 0) {
		fprintf(stderr, "Either copy or VSN/media must be specified");
		return (-1);
	}

	if (arg->copies == 0)  arg->copies = (1 << MAX_ARCHIVE) - 1;

	return (0);
}
