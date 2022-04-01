/*
 *  dump_log.c - Dump SAM-FS fifo & ioctl buffer.
 *
 *	Dump_log dumps the contents of the fifo & ioctl buffer for the
 *	mounted SAM-FS file system.
 *
 *	Syntax:
 *		dump_log [-f] [fifo_log] [ioctl_log]
 *
 *	where:
 *		fifo_log    The log buffer for fifo commands sent by the
 *                  SAM-FS filesystem to the sam-amld daemon.
 *
 *		ioctl_log   The log buffer for ioctl daemon commands send to
 *				the SAM-FS filesystem.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.27 $"


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>

#define	MAIN
#define	DEC_INIT

#include "sam/types.h"
#include "aml/device.h"
#include "aml/odlabels.h"
#include "aml/fifo.h"
#include "sam/resource.h"
#include "sam/fioctl.h"
#include "aml/logging.h"
#include "sam/devnm.h"
#include "sam/syscall.h"

char *ctime_r(const time_t *clock, char *buf, int buflen);
static void DumpArchive(sam_archive_t *);
static void DumpIoctl(ioctl_log_t *);
static void DumpResource(sam_resource_t *);
static void DumpRminfo(sam_arch_rminfo_t *);


static char *cmd_str[] = {
	"", "fs_load", "fs_stage", "fs_unload",			/* 0 - 3 */
	"fs_mount", "void", "fs_cancel", "void",		/* 4 - 7 */
	"void", "fs_unmount", "fs_release", "fs_start_archiver", /* 8 - 11 */
	"fs_resync", "void", "fs_syslog", "fs_wmstate"		/* 12 - 15 */
};

#if defined(USE_IOCTL_INTERFACE)
char *ioctl_dstr[] = {
	"", "fs_mount", "fs_unload", "fs_error",
	"fs_inval", "fs_stage", "fs_bioeof" "fs_direrr"
};

#endif

static char *ioctl_fstr[] = {
	"", "s_getdents", "s_write", "s_stsize", "mt_mount",
};

static char *progname = "dump_log";

int
main(int argc, char **argv)
{
	int file_fd, ioctl_fd;
	int fifo_len, ioctl_len;
	int f_opt = 0;
	int fout, iout;
	int new_fifo = 1, new_ioctl = 1;
	char line[150];
	char c;
	char *fifo_path = FS_FIFO_LOG;
	char *ioctl_path = FS_IOCTL_LOG;
	fifo_log_t *f_blk;
	ioctl_log_t *i_blk;
	fifo_log_t dummy_fifo;
	ioctl_log_t dummy_ioctl;
	ioctl_fet_t *ioctl_fet;
	fifo_fet_t *fifo_fet;
	struct stat	ioctl_stat;
	struct stat	fifo_stat;

	while ((c = getopt(argc, argv, "fi:p:")) != -1) {
		switch (c) {
		case 'f':
			f_opt = 1;
			break;
		case 'i':
			ioctl_path = optarg;
			break;
		case 'p':
			fifo_path = optarg;
			break;
		default:
			fprintf(stderr, "usage: %s [-f] [-i ioctl] "
			    "[-p fifo]\n", progname);
			break;
		}
	}

	dummy_fifo.time.tv_sec = INT32_MAX;
	dummy_ioctl.time.tv_sec = INT32_MAX;

	if (stat(fifo_path, &fifo_stat) < 0) {
		printf("Unable to stat fifo log - %s\n", strerror(errno));
		exit(1);
	}
	fifo_len = sizeof (fifo_fet_t) + (1000 * sizeof (fifo_log_t));
	if (fifo_stat.st_size != fifo_len) {
		printf("fifo log incorrect size - %d, should be %d\n",
		    (int)fifo_stat.st_size, fifo_len);
		exit(1);
	}
	if (stat(ioctl_path, &ioctl_stat) < 0) {
		printf("Unable to stat ioctl log - %s\n", strerror(errno));
		exit(1);
	}
	ioctl_len = sizeof (ioctl_fet_t) + (1000 * sizeof (ioctl_log_t));
	if (ioctl_stat.st_size != ioctl_len) {
		printf("ioctl log incorrect size - %d, should be %d\n",
		    (int)ioctl_stat.st_size, ioctl_len);
		exit(1);
	}

	file_fd = open(fifo_path, O_RDWR);
	if (file_fd < 0) {
		printf("Unable to open fifo log - %s\n", strerror(errno));
		exit(1);
	}
	ioctl_fd = open(ioctl_path, O_RDWR);
	if (ioctl_fd < 0) {
		printf("Unable to open ioctl log - %s\n", strerror(errno));
		exit(1);
	}
	if ((fifo_fet = (fifo_fet_t *)mmap((caddr_t)NULL, fifo_len,
	    (PROT_READ | PROT_WRITE),
	    MAP_SHARED, file_fd, (off_t)0)) ==
	    (fifo_fet_t *)MAP_FAILED) {
		printf("Unable to map fifo log - %s\n", strerror(errno));
		exit(1);
	}
	if ((ioctl_fet = (ioctl_fet_t *)mmap((caddr_t)NULL, ioctl_len,
	    (PROT_READ | PROT_WRITE), MAP_SHARED,
	    ioctl_fd, (off_t)0)) == (ioctl_fet_t *)MAP_FAILED) {
		printf("Unable to map ioctl log - %s\n", strerror(errno));
		exit(1);
	}
	fout = fifo_fet->out;
	iout = ioctl_fet->out;

	for (;;) {
		if (new_fifo) {
			if (fout == fifo_fet->in) {
				f_blk = &dummy_fifo;
			} else {
				if (fout > (fifo_fet->limit - 1)) {
					fout = fifo_fet->first;
				}
				f_blk = (fifo_log_t *)(fout +
				    (char *)fifo_fet);
				fout += sizeof (fifo_log_t);
				if (f_opt) {
					fifo_fet->out = fout;
				}
			}
			new_fifo = 0;
		}
		if (new_ioctl) {
			if (iout == ioctl_fet->in) {
				i_blk = &dummy_ioctl;
			} else {
				if (iout > (ioctl_fet->limit - 1)) {
					iout = ioctl_fet->first;
				}
				i_blk = (ioctl_log_t *)(iout +
				    (char *)ioctl_fet);
				iout += sizeof (ioctl_log_t);
				if (f_opt) {
					ioctl_fet->out = iout;
				}
			}
			new_ioctl = 0;
		}
		if (f_blk == &dummy_fifo && i_blk == &dummy_ioctl) {
			fflush(stdout);
			if (!f_opt) {
				exit(0);
			}
			sleep(1);
			new_fifo = new_ioctl = 1;
			continue;
		}
		if (f_blk->time.tv_sec < i_blk->time.tv_sec ||
		    (f_blk->time.tv_sec == i_blk->time.tv_sec &&
		    f_blk->time.tv_usec <= i_blk->time.tv_usec)) {
			new_fifo++;
			ctime_r((time_t *)&f_blk->time.tv_sec, &line[0], 150);
			sprintf(&line[19], ".%.3ld",
			    f_blk->time.tv_usec / 1000);
			line[23] = '\0';
			printf(
			    "%s fifo command \"%s\" (%d) "
			    "handle(%lu.%lu,%d,%u,%llx) %s\n",
			    &line[11],
			    cmd_str[f_blk->fifo_cmd.cmd],
			    f_blk->fifo_cmd.cmd,
			    f_blk->fifo_cmd.handle.id.ino,
			    f_blk->fifo_cmd.handle.id.gen,
			    f_blk->fifo_cmd.handle.fseq,
			    f_blk->fifo_cmd.handle.seqno,
			    (f_blk->fifo_cmd.handle.fifo_cmd.p64),
			    f_blk->fifo_cmd.handle.flags.b.eagain ?
			    "EAGAIN" : "");

			switch (f_blk->fifo_cmd.cmd) {
			case FS_FIFO_LOAD:
		/* N.B. Bad indentation here to meet cstyle requirements */
		DumpRminfo(
		    &f_blk->fifo_cmd.param.fs_load.resource.archive.rm_info);
		DumpArchive(&f_blk->fifo_cmd.param.fs_load.resource.archive);
		DumpResource(&f_blk->fifo_cmd.param.fs_load.resource);
		break;

			case FS_FIFO_UNLOAD:
		/* N.B. Bad indentation here to meet cstyle requirements */
		printf("\trdev %#lx, mt_handle %#x\n",
		    f_blk->fifo_cmd.param.fs_unload.rdev,
		    (int)(f_blk->fifo_cmd.param.fs_unload.mt_handle.p32));
		DumpRminfo(
		    &f_blk->fifo_cmd.param.fs_unload.resource.archive.rm_info);
		DumpArchive(&f_blk->fifo_cmd.param.fs_unload.resource.archive);
		DumpResource(&f_blk->fifo_cmd.param.fs_unload.resource);
		break;

			case FS_FIFO_CANCEL:
		/* N.B. Bad indentation here to meet cstyle requirements */
		printf("\tcommand to cancel \"%s\" (%d)\n",
		    cmd_str[f_blk->fifo_cmd.param.fs_cancel.cmd],
		    f_blk->fifo_cmd.param.fs_cancel.cmd);
		DumpRminfo(
		    &f_blk->fifo_cmd.param.fs_cancel.resource.archive.rm_info);
		DumpArchive(&f_blk->fifo_cmd.param.fs_cancel.resource.archive);
		DumpResource(&f_blk->fifo_cmd.param.fs_cancel.resource);
		break;

			}
			printf("\n");
		} else {
			DumpIoctl(i_blk);
			new_ioctl++;
		}
	}
	/* NOTREACHED */
}

static void
DumpRminfo(sam_arch_rminfo_t *rm_info)
{
	char stat[24];
	char *p;
	int n;

	stat[0] = rm_info->valid ? 'v' : '-';
	stat[1] = rm_info->stranger ? 'S' : '-';
	stat[2] = rm_info->bof_written ? 'b' : '-';
	stat[3] = rm_info->file_written ? 'f' : '-';

	stat[4] = rm_info->process_wtm ? 't' : '-';
	stat[5] = rm_info->block_io ? 'B' : '-';
	stat[6] = rm_info->partial_pdu ? 'p' : '-';
	stat[7] = rm_info->process_eox ? 'V' : '-';

	stat[8] = '-';
	stat[9] = '-';
	stat[10] = '-';
	stat[11] = rm_info->volumes ? 'O' : '-';

	stat[12] = rm_info->buffered_io ? 'B' : '-';
	stat[13] = '-';
	stat[14] = '-';
	stat[15] = rm_info->filemark ? 'f' : '-';
	stat[16] = '\0';

	printf("\t===rm_info===\n");
	n = rm_info->media & DT_MEDIA_MASK;
	if ((rm_info->media & DT_CLASS_MASK) == DT_OPTICAL) {
		p = dev_nmod[(n < OD_CNT ? n : 0)];
	} else if ((rm_info->media & DT_CLASS_MASK) == DT_TAPE) {
		p = dev_nmtp[(n < MT_CNT ? n : 0)];
	} else if ((rm_info->media & DT_CLASS_MASK) == DT_THIRD_PARTY) {
		p = "  ";
		sprintf(p, "z%c", rm_info->media & 0xff);
	} else {
		p = "??";
	}
	printf("\t size %lld, media type %s, %s\n",
	    rm_info->size, p,
	    stat);
	printf("\t file_offset %#x, pos %#x, alloc unit %#x\n",
	    rm_info->file_offset, rm_info->position, rm_info->mau);
}

static void
DumpResource(sam_resource_t *res)
{
	printf("\t===resource struct===\n");
	printf("\t revision %d, access %d, protect %d, \n\t next vsn \"%s\","
	    "prev vsn \"%s\"\n",
	    res->revision, res->access, res->protect, res->next_vsn,
	    res->prev_vsn);
	printf("\t group_id \"%s\", owner id \"%s\", req size %ld\n\t "
	    "info \"%s\"\n",
	    res->mc.od.group_id, res->mc.od.owner_id, res->required_size,
	    res->mc.od.info);
}

static void
DumpArchive(sam_archive_t *archive)
{
	time_t time;

	time = (time_t)archive->creation_time;
	printf("\t===archive struct===\n");
	printf("\t create_time %s", ctime(&time));
	printf("\t vsn \"%s\"\n", archive->vsn);
	printf("\t od\n\t\t label_pda %#lx, version %d, file_id \"%s\"\n",
	    archive->mc.od.label_pda, archive->mc.od.version,
	    archive->mc.od.file_id);
}

static void
DumpIoctl(ioctl_log_t *i_blk)
{
	uint_t lower;
	int upper;
	char line[50];

	ctime_r((time_t *)& i_blk->time.tv_sec, &line[0], 150);
	sprintf(&line[19], ".%.3ld", i_blk->time.tv_usec / 1000);
	line[23] = '\0';

	switch (i_blk->ioctl_system) {
#if defined(USE_IOCTL_INTERFACE)
	case 'd':
	case 'D':
	{
		printf("%s ioctl \"%s\" (%d)\n",
		    &line[11], ioctl_dstr[i_blk->ioctl_type],
		    i_blk->ioctl_type);

		switch (i_blk->ioctl_type) {
		case C_FSMOUNT:
	/* N.B. Bad indentation here to meet cstyle requirements */
	{
		lower = i_blk->ioctl_data.fsmount.space;
		upper = i_blk->ioctl_data.fsmount.space >> 32;
		printf("  mount - handle(%d.%d,%d,%u,%x), err %d, "
		    "space %#x%8.8x, rdev %#x\n",
		    i_blk->ioctl_data.fsmount.handle.id.ino,
		    i_blk->ioctl_data.fsmount.handle.id.gen,
		    i_blk->ioctl_data.fsmount.handle.fseq,
		    i_blk->ioctl_data.fsmount.handle.segno,
		    i_blk->ioctl_data.fsmount.handle.fifo_cmd,
		    i_blk->ioctl_data.fsmount.ret_err,
		    upper, lower,
		    i_blk->ioctl_data.fsmount.rdev);
		printf("\tmt_handle %#x.\n",
		    i_blk->ioctl_data.fsmount.mt_handle);
		DumpRminfo(
		    &i_blk->ioctl_data.fsmount.resource.archive.rm_info);
		DumpArchive(&i_blk->ioctl_data.fsmount.resource.archive);
		DumpResource(&i_blk->ioctl_data.fsmount.resource);
	}
	break;

		case C_FSUNLOAD:
			printf("  unload -   handle(%d.%d,%d,%u,%x), "
			    "err %d, pos %#llx\n",
			    i_blk->ioctl_data.fsmount.handle.id.ino,
			    i_blk->ioctl_data.fsmount.handle.id.gen,
			    i_blk->ioctl_data.fsmount.handle.fseq,
			    i_blk->ioctl_data.fsmount.handle.seqno,
			    i_blk->ioctl_data.fsmount.handle.fifo_cmd,
			    i_blk->ioctl_data.fsunload.ret_err,
			    i_blk->ioctl_data.fsunload.position);
			break;

		case C_FSERROR:
			printf("  error -  handle(%d.%d,%d,%u,%x), err %d\n",
			    i_blk->ioctl_data.fsmount.handle.id.ino,
			    i_blk->ioctl_data.fsmount.handle.id.gen,
			    i_blk->ioctl_data.fsmount.handle.fseq,
			    i_blk->ioctl_data.fsmount.handle.seqno,
			    i_blk->ioctl_data.fsmount.handle.fifo_cmd,
			    i_blk->ioctl_data.fserror.ret_err);
			break;

		case C_FSSTAGE:
			printf("  stage -  handle(%d.%d,%d,%u,%x), err %d\n",
			    i_blk->ioctl_data.fsmount.handle.id.ino,
			    i_blk->ioctl_data.fsmount.handle.id.gen,
			    i_blk->ioctl_data.fsmount.handle.fseq,
			    i_blk->ioctl_data.fsmount.handle.seqno,
			    i_blk->ioctl_data.fsmount.handle.fifo_cmd,
			    i_blk->ioctl_data.fsstage.ret_err);
			break;

		case C_FSINVAL:
			printf("  invalidate -  rdev %#x\n",
			    i_blk->ioctl_data.fsinval.rdev);
			break;

		case C_FSCANCEL:
			printf("  cancel -  handle(%d.%d,%d,%u,%x), err %d\n",
			    i_blk->ioctl_data.fsmount.handle.id.ino,
			    i_blk->ioctl_data.fsmount.handle.id.gen,
			    i_blk->ioctl_data.fsmount.handle.fseq,
			    i_blk->ioctl_data.fsmount.handle.seqno,
			    i_blk->ioctl_data.fsmount.handle.fifo_cmd,
			    i_blk->ioctl_data.fserror.ret_err);
			break;

		default:
			printf("  unknown -  handle(%d.%d,%d,%u,%x)\n",
			    i_blk->ioctl_data.fsmount.handle.id.ino,
			    i_blk->ioctl_data.fsmount.handle.id.gen,
			    i_blk->ioctl_data.fsmount.handle.fseq,
			    i_blk->ioctl_data.fsmount.handle.seqno,
			    i_blk->ioctl_data.fsmount.handle.fifo_cmd);
			break;

		}
	}
	break;
#endif				/* USE_IOCTL_INTERFACE */
	case 's':
	{
		switch (i_blk->ioctl_type) {
		case SC_fsmount:
	/* N.B. Bad indentation here to meet cstyle requirements */
	{
		printf("%s syscall SC_fsmount\n", &line[11]);
		lower = i_blk->ioctl_data.fsmount.space;
		upper = i_blk->ioctl_data.fsmount.space >> 32;
		printf("  mount - handle(%ld.%ld,%d,%u,%llx), "
		    "err %d, space %#x%8.8x, rdev %#lx\n",
		    i_blk->ioctl_data.fsmount.handle.id.ino,
		    i_blk->ioctl_data.fsmount.handle.id.gen,
		    i_blk->ioctl_data.fsmount.handle.fseq,
		    i_blk->ioctl_data.fsmount.handle.seqno,
		    (i_blk->ioctl_data.fsmount.handle.fifo_cmd.p64),
		    i_blk->ioctl_data.fsmount.ret_err,
		    upper, lower,
		    i_blk->ioctl_data.fsmount.rdev);
		printf("\tmt_handle %#x.\n",
		    (int)(i_blk->ioctl_data.fsmount.mt_handle));
		DumpRminfo(&i_blk->ioctl_data.fsmount.resource.archive.rm_info);
		DumpArchive(&i_blk->ioctl_data.fsmount.resource.archive);
		DumpResource(&i_blk->ioctl_data.fsmount.resource);
	}
	break;

		case SC_fsunload:
			printf("%s syscall SC_fsunload\n", &line[11]);
			printf("  unload -   handle(%ld.%ld,%d,%u,%llx), "
			    "err %d\n",
			    i_blk->ioctl_data.sc_fsunload.handle.id.ino,
			    i_blk->ioctl_data.sc_fsunload.handle.id.gen,
			    i_blk->ioctl_data.sc_fsunload.handle.fseq,
			    i_blk->ioctl_data.sc_fsunload.handle.seqno,
			    (i_blk->ioctl_data.sc_fsunload.handle.fifo_cmd.p64),
			    i_blk->ioctl_data.sc_fsunload.ret_err);
			break;

		case SC_fserror:
			printf("%s syscall SC_fserror\n", &line[11]);
			printf("  error -  handle(%ld.%ld,%d,%u,%llx), "
			    "err %d\n",
			    i_blk->ioctl_data.sc_fserror.handle.id.ino,
			    i_blk->ioctl_data.sc_fserror.handle.id.gen,
			    i_blk->ioctl_data.sc_fserror.handle.fseq,
			    i_blk->ioctl_data.sc_fserror.handle.seqno,
			    (i_blk->ioctl_data.sc_fserror.handle.fifo_cmd.p64),
			    i_blk->ioctl_data.sc_fserror.ret_err);
			break;

		case SC_fsstage:
			printf("%s syscall SC_fsstage\n", &line[11]);
			printf("  stage -  handle(%ld.%ld,%d,%u,%llx), "
			    "err %d\n",
			    i_blk->ioctl_data.sc_fsstage.handle.id.ino,
			    i_blk->ioctl_data.sc_fsstage.handle.id.gen,
			    i_blk->ioctl_data.sc_fsstage.handle.fseq,
			    i_blk->ioctl_data.sc_fsstage.handle.seqno,
			    (i_blk->ioctl_data.sc_fsstage.handle.fifo_cmd.p64),
			    i_blk->ioctl_data.sc_fsstage.ret_err);
			break;

		case SC_fsinval:
			printf("%s syscall SC_fsinval\n", &line[11]);
			printf("  invalidate -  rdev %#lx\n",
			    i_blk->ioctl_data.sc_fsinval.rdev);
			break;

		case SC_fscancel:
			printf("%s syscall SC_fscancel\n", &line[11]);
			printf("  cancel -  handle(%ld.%ld,%d,%u,%llx), "
			    "err %d\n",
			    i_blk->ioctl_data.sc_fserror.handle.id.ino,
			    i_blk->ioctl_data.sc_fserror.handle.id.gen,
			    i_blk->ioctl_data.sc_fserror.handle.fseq,
			    i_blk->ioctl_data.sc_fserror.handle.seqno,
			    (i_blk->ioctl_data.sc_fserror.handle.fifo_cmd.p64),
			    i_blk->ioctl_data.sc_fserror.ret_err);
			break;

		case SC_fsiocount:
		/* N.B. Bad indentation here to meet cstyle requirements */
		printf("%s syscall SC_fsiocount\n", &line[11]);
		printf("  iocount -  handle(%ld.%ld,%d,%u,%llx)\n",
		    i_blk->ioctl_data.sc_fsiocount.handle.id.ino,
		    i_blk->ioctl_data.sc_fsiocount.handle.id.gen,
		    i_blk->ioctl_data.sc_fsiocount.handle.fseq,
		    i_blk->ioctl_data.sc_fsiocount.handle.seqno,
		    (i_blk->ioctl_data.sc_fsiocount.handle.fifo_cmd.p64));
		break;

		default:
			printf("%s syscall SC_unknown\n", &line[11]);
			printf("  unknown -  handle(%ld.%ld,%d,%u,%llx)\n",
			    i_blk->ioctl_data.sc_fsbeof.handle.id.ino,
			    i_blk->ioctl_data.sc_fsbeof.handle.id.gen,
			    i_blk->ioctl_data.sc_fsbeof.handle.fseq,
			    i_blk->ioctl_data.sc_fsbeof.handle.seqno,
			    (i_blk->ioctl_data.sc_fsbeof.handle.fifo_cmd.p64));
			break;

		}
		break;
	}
	break;

	case 'f':
		printf("%s ioctl \"%s\" (%d)\n",
		    &line[11], ioctl_fstr[i_blk->ioctl_type],
		    i_blk->ioctl_type);
		switch (i_blk->ioctl_type) {
		case C_SWRITE:
			{
			uint_t *upper, *lower;

			upper = (uint_t *)&(i_blk->ioctl_data.swrite.offset);
			lower = upper + 1;
			printf("  swrite - offset %#x%8.8x, buffer %#x, "
			    "nbytes %d\n",
			    *upper, *lower,
			    (int)(i_blk->ioctl_data.swrite.buf.ptr),
			    i_blk->ioctl_data.swrite.nbyte);
			break;
			}
		case C_STSIZE:
			{
			uint_t *upper, *lower;

			upper = (uint_t *)&(i_blk->ioctl_data.stsize.size);
			lower = upper + 1;
			printf("  stsize - size %#x%#8.8x\n",
			    *upper, *lower);
			break;
			}

		default:
			break;
		}
		break;
	}

	printf("\n");
}
