/*
 * scsi_cmd.c - generic interface for issuing scsi commands.
 *
 * Since the device entry is modified during the scsi command, the io_mutex must
 * be locked BEFORE calling scsi_cmd.  The cdb and possibly the sense fields
 * are modified.  The io_mutex is STILL held when the routine returns.
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

#pragma ident "$Revision: 1.53 $"

static char *_SrcFile = __FILE__;

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/scsi/scsi.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/types.h>

#define	SCSI_ERRCODES
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/tapes.h"
#include "aml/shm.h"
#include "aml/sefvals.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/lint.h"
#include "sam/types.h"
#include "aml/tapealert.h"
#include "aml/sef.h"
#include "sam/custmsg.h"
#include "sam/lib.h"

#define	 BYTES2SECTORS(x, y) ((x) / (y))
/* DEV_ENT - given an equipment ordinal, return the dev_ent */
#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))


static int cdb_fd = -1;
static mutex_t cdb_fd_mutex = {0};


extern shm_alloc_t master_shm, preview_shm;

#if	defined(ALLOW_SETTING_ERRORS)
int pfg_set = 0;
int pfg_command = SCMD_TEST_UNIT_READY, pfg_fails = 0, pfg_sense = 0,
			pfg_asc = 0, pfg_ascq = 0;
#endif

/*
 * scsi_cmd() - returns: For commands xfering data: number of bytes xfer'd or
 * -1; For commands not xfering : 0 (success) or -1 (error)
 */

/* VARARGS4 */
int
scsi_cmd(const int fd, dev_ent_t *un, const int command, const int timeit, ...)
{
	int	tmp1, tmp2, tmp3, tmp4, tmp5;
	int	count, req_xfer = 0;
	int	start_sector = -1, timeout;
	int	*resid = (int *)NULL;
	int	cur_mess = 0, retry;
	char	*d_mess, *dc_mess;
	char	dis_cdb[40];
	va_list args;
	sam_extended_sense_t *sp, *sp_ret = NULL;
	struct uscsi_cmd us;
	/* LINTED pointer cast may result in improper alignment */
	union scsi_cdb *cdb = (union scsi_cdb *)& un->cdb[0];
	struct {
		int	eq;
		int	what;
		time_t	now;
		int	fd;
		char	cdb[12];
		char	sense[20];
	} cdb_trace;

	if (timeit > SHRT_MAX)
		timeout = SHRT_MAX;
	else
		timeout = timeit;

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int    indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized))
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3140));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.scsi_cmd) {
			va_start(args, timeout);
			tmp1 = IO_table[indx].jmp_table.scsi_cmd(fd,
			    un, command, timeout, args);
			va_end(args);
			return (tmp1);
		}
	}
	d_mess = un->dis_mes[DIS_MES_NORM];
	dc_mess = un->dis_mes[DIS_MES_CRIT];

	if (!mutex_trylock(&un->io_mutex)) {
		/* don't care about door lock/unlock */
		if ((command & 0xff) != 0x1e)
			DevLog(DL_DEBUG(1021), command);
		mutex_unlock(&un->io_mutex);
	}
	while (un->io_active) {
		if (*dc_mess == '\0' && !cur_mess) {
			sprintf(dc_mess, "scsi_cmd(%#x) enter with io_active",
			    command);
			cur_mess = TRUE;
		}
		DevLog(DL_DEBUG(1022), command);
		sleep(2);
	}

	if (cur_mess)
		*dc_mess = '\0';

	if (DBG_LVL(SAM_DBG_DISSCSI))
		cur_mess = strlen(d_mess);
	else
		cur_mess = -1;

	un->io_active = TRUE;
	memset(&us, 0, sizeof (us));	/* clear the user scsi struct */
	memset(cdb, 0, CDB_GROUP5);	/* clear the cdb */
	us.uscsi_cdb = (caddr_t)cdb;	/* pointer to the cdb */
	/* sense data address */
	us.uscsi_rqbuf = (caddr_t)SHM_REF_ADDR(un->sense);
	/* length of sense data */
	us.uscsi_rqlen = sizeof (sam_extended_sense_t);
	us.uscsi_timeout = timeout ? timeout : SAM_SCSI_DEFAULT_TIMEOUT;
	us.uscsi_flags = USCSI_SILENT | USCSI_RQENABLE | USCSI_READ;
	cdb->scc_cmd = (command & 0xff);

	switch (command) {
	case SCMD_TEST_UNIT_READY:	/* 0x00 */
		us.uscsi_flags |= USCSI_DIAGNOSE;
		us.uscsi_cdblen = 6;
		break;

	case SCMD_REZERO_UNIT:	/* 0x01 */
		us.uscsi_cdblen = 6;
		break;

		/*
		 * MOVE_MEDIA takes two arguments, load flag and address and
		 * is used by the lf4500 rapid changer.
		 */

	case SCMD_MOVE_MEDIA:	/* 0x02 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		cdb->g0_addr2 = tmp1 << 1;
		count = va_arg(args, int);
		FORMG0COUNT(cdb, count);
		va_end(args);
		break;

		/*
		 * REQUEST_SENSE takes three parameters: (void *)address, int
		 * length, int *resid address - address to save the inquire
		 * length  - lenght of space at address resid   - residual
		 */
	case SCMD_REQUEST_SENSE:	/* 0x03 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		cdb->g0_count0 = us.uscsi_buflen;
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * READ_BLKLIM takes three parameters: (void *)address, int
		 * length, int *resid address - address to save the data
		 * length  - lenght of space at address resid   - residual
		 */
	case SCMD_READ_BLKLIM:	/* 0x05 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		req_xfer = us.uscsi_buflen;
		break;

	case SCMD_FORMAT:	/* 0x04 */
		us.uscsi_cdblen = 6;
		break;

	case SCMD_INIT_ELEMENT_STATUS:	/* 0x07 */
		us.uscsi_cdblen = 6;
		/* Exabyte needs to know if there is a barcode reader */
		if (un->equ_type == DT_EXB210 && !un->dt.rb.status.b.barcodes)
			un->cdb[5] |= (1 << 7);
		/* Plasmon G: test all elements in slow-scan mode */
		if (un->equ_type == DT_PLASMON_G) {
			un->cdb[5] |= (3 << 6);
		}
		break;

		/*
		 * Init a single element - Plasmon Enterprise
		 */
	case SCMD_INIT_SINGLE_ELEMENT_STATUS:	/* 0xC7 */

		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);	/* element */
		va_end(args);
		un->cdb[2] = tmp1 >> 8;
		un->cdb[3] = tmp1;
		break;

		/*
		 * ROTATE_MAILSLOT takes one parameter int action action  - 0
		 * = rotate in , !0 = rotate out
		 */
	case SCMD_ROTATE_MAILSLOT:	/* 0x0c */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		cdb->g0_count0 = tmp1 ? 1 : 0;
		break;

		/*
		 * Special Plasmon DVD-RAM library scsi command to open/close
		 * the I/E drawer.
		 */
	case 0xd:		/* 0xd */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		cdb->g0_count0 = tmp1 ? 1 : 0;
		break;

		/*
		 * Plasmon G command to open/close mailslot
		 */
	case SCMD_OPEN_CLOSE_MAILSLOT:
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		cdb->g0_count0 = tmp1 ? 1 : 0;
		break;

		/*
		 * WRITE_FILE_MARK takes one parameter: int count count -
		 * number of file marks to write.
		 */
	case SCMD_WRITE_FILE_MARK:	/* 0x10 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		count = va_arg(args, int);
		va_end(args);
		cdb->high_count = count >> 16;	/* set count */
		cdb->mid_count = ((count >> 8) & 0xff);
		cdb->low_count = (count & 0xff);
		break;

		/*
		 * SPACE takes two parameters: int count, int code, int
		 * *resid count - number of things to space over code - type
		 * of thing resid - residual
		 */
	case SCMD_SPACE:	/* 0x11 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		count = va_arg(args, int);
		cdb->t_code = va_arg(args, int);
		resid = va_arg(args, int *);
		va_end(args);
		cdb->high_count = count >> 16;	/* set count */
		cdb->mid_count = ((count >> 8) & 0xff);
		cdb->low_count = (count & 0xff);
		break;

		/*
		 * INQUIRY takes five parameters: (void *)address, int
		 * length, int vital, int page, int *resid address - address
		 * to save the inquire length  - lenght of space at address
		 * vital   - Vital product flag page    - page code if vital
		 * set resid   - residual stat    - extended sense buffer
		 */
	case SCMD_INQUIRY:	/* 0x12 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		tmp1 = va_arg(args, int);	/* vital */
		tmp2 = va_arg(args, int);	/* page */
		resid = va_arg(args, int *);
		sp_ret = va_arg(args, sam_extended_sense_t *);	/* sense data */
		va_end(args);
		cdb->g0_addr2 = (tmp1 != 0);
		cdb->g0_addr1 = (tmp1 != 0) ? tmp2 : 0;
		cdb->g0_count0 = us.uscsi_buflen;
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * MODE_SELECT takes four parameters: (void *)address, int
		 * length, int save_page, int *resid address - address of the
		 * mode select parameter block length - length of the block(
		 * Less then 256) save_page - bits 4 and 0 byte 1 of the cdb
		 * resid - residual
		 */
	case SCMD_MODE_SELECT:	/* 0x15 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		tmp1 = va_arg(args, int);	/* save page flags */
		resid = va_arg(args, int *);
		va_end(args);
#if USCSI_WRITE == 0
		us.uscsi_flags &= ~USCSI_READ;
#endif
		cdb->g0_count0 = us.uscsi_buflen;
		cdb->t_code = tmp1 & 0x11;	/* PF and save page */
		req_xfer = us.uscsi_buflen;
		break;

	case SCMD_RESERVE:	/* 0x16 */
		us.uscsi_cdblen = 6;
		break;

	case SCMD_RELEASE:	/* 0x17 */
		us.uscsi_cdblen = 6;
		break;

		/*
		 * ERASE takes one parameter int  long long -  0 = short
		 * erase, !0 = long erase
		 */

	case SCMD_ERASE:	/* 0x19 */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);	/* long erase flag */
		va_end(args);
		un->cdb[1] |= (tmp1 ? 0x01 : 0);	/* long erase? */
		break;

		/*
		 * MODE_SENSE takes four parameters: (void *)address, int
		 * length, int page_code, int *resid address - address to put
		 * the mode sense data length  - length of the area page_code
		 * - page code for cdb resid   - residual
		 */
	case SCMD_MODE_SENSE:	/* 0x1a */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		cdb->g0_count0 = req_xfer = us.uscsi_buflen = va_arg(args, int);
		cdb->g0_addr1 = va_arg(args, int);
		resid = va_arg(args, int *);
		va_end(args);
		/*
		 * Some media changers will accept the mode_sense command
		 * without the "no block decsriptor" bit being set, and just
		 * assume it.  Others will report an error.  Since there is
		 * no block descriptor for a media changer, force this bit
		 * for all media changers.  Since we know this is a scsi
		 * device(we would not be here otherwise, the test for
		 * IS_ROBOT will work.
		 */
		if (IS_ROBOT(un))
			/* no blk desc for media changers */
			un->cdb[1] |= 0x08;
		break;

		/*
		 * START_STOP_UNIT/LOAD takes three parameter: int action,
		 * int eject action  - 0 = spin down , !0 = spin up eject   -
		 * 0 = no eject, !0 = eject
		 */
	case SCMD_START_STOP:	/* 0x1b */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		tmp2 = va_arg(args, int);
		va_end(args);
		/* On tape, the eject bit is retension so clear it. */
		if (IS_TAPE(un))
			tmp2 = 0;
		cdb->g0_count0 = ((tmp1 ? 1 : 0) | ((tmp2 ? 1 : 0) << 1));
#if	defined(ALLOW_SETTING_ERRORS)
		if (pfg_set && tmp1)
			pfg_fails = 1;
		else
			pfg_fails = 0;
#endif
		break;

		/*
		 * DOORLOCK takes one parameter: int action action    - 0 =
		 * unlock, !0 = lock
		 */
	case SCMD_DOORLOCK:	/* 0x1e */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		cdb->g0_count0 = tmp1 ? 1 : 0;
		/*
		 * If Plasmon DVD-RAM library, there is another parameter
		 * specifying a value of one if locking/unlocking the I/E
		 * drawer, a value of zero if locking/unlocking the magazine
		 * access door.
		 */
		if (un->type == DT_PLASMON_D) {
			tmp2 = va_arg(args, int);
			cdb->g0_vu_0 = tmp2 ? 1 : 0;
		}
		break;

		/*
		 * READ_POSITION takes three parameters: (void *)address, int
		 * length, int *resid address - address to put the data
		 * length  - length of the area resid   - residual
		 */
	case SCMD_READ_POSITION:	/* 0x34 */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		if ((un->equ_type == DT_9490) ||
		    (un->equ_type == DT_D3) ||
		    (un->equ_type == DT_9840) ||
		    (un->equ_type == DT_9940))
			/* stk uses endor unique position */
			un->cdb[1] |= 0x01;
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * READ_CAPACITY takes three parameters: (void *)address, int
		 * length, int *resid address - address to put the data
		 * length  - length of the area resid   - residual
		 */
	case SCMD_READ_CAPACITY:	/* 0x25 */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * LOCATE takes one parameter int address - address to locate
		 */

	case SCMD_LOCATE:	/* 0x2b */
		cdb->scc_cmd = SCMD_SEEK_G1;
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		cdb->g1_addr2 = (tmp1 >> 24) & 0xFF;
		cdb->g1_addr1 = (tmp1 >> 16) & 0xFF;
		cdb->g1_addr0 = (tmp1 >> 8) & 0xFF;
		cdb->g1_rsvd0 = (tmp1 & 0xFF);
		if ((un->equ_type == DT_SQUARE_TAPE) ||
		    (un->equ_type == DT_9490) ||
		    (un->equ_type == DT_D3) ||
		    (un->equ_type == DT_9840) ||
		    (un->equ_type == DT_9940))
			/* stk uses vendor unique position */
			cdb->t_code |= (1 << 2);
		if (un->equ_type == DT_TITAN) {
			/*
			 * Titanium tape only accepts SCSI logical block
			 * number
			 */
			cdb->t_code = 0;
		}
		break;

		/*
		 * POSITION_TO_ELEMENT takes three parameters int xport, int
		 * dest, int invert xport - element address of transport to
		 * position dest  - element address to position to invert -
		 * invert transport
		 */

	case SCMD_POSITION_TO_ELEMENT:	/* 0x2b (0x012b) */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		tmp2 = va_arg(args, int);
		tmp3 = va_arg(args, int);
		va_end(args);
		cdb->g1_addr3 = (tmp1 >> 8) & 0xFF;
		cdb->g1_addr2 = tmp1 & 0xFF;
		cdb->g1_addr1 = (tmp2 >> 8) & 0xFF;
		cdb->g1_addr0 = tmp2 & 0xFF;
		cdb->g1_count0 = (tmp3 == 0) ? 0 : 1;
		break;

		/*
		 * VERIFY takes parameters: int start, int num, int flag, int
		 * *resid start   - starting sector address num    - number
		 * of sectors flag    - 0 no blank check, !0 = blank check
		 */

	case SCMD_VERIFY:	/* 0x2f */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		tmp2 = va_arg(args, int);
		tmp3 = va_arg(args, int);
		va_end(args);
		FORMG1ADDR(cdb, tmp1);
		FORMG1COUNT(cdb, tmp2);
		cdb->g1_reladdr = ((tmp3 ? 1 : 0) << 2);
		start_sector = tmp1;
		break;

		/*
		 * READ_BUFFER and WRITE_BUFFER take 6 parameters: (void *)
		 * address, int length, int offset, int mode, int, id, int
		 * *resid address - address of the data buffer length  -
		 * length of the buffer offset  - buffer offset mode    -
		 * mode id    - buffer id resid   - for return of residual
		 */
	case SCMD_WRITE_BUFFER:		/* 0x3b */
		us.uscsi_flags &= ~USCSI_READ;
		/* FALLTHRU */
	case SCMD_READ_BUFFER:	/* 0x3c */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		tmp1 = va_arg(args, int);	/* offset */
		tmp2 = va_arg(args, int);	/* mode */
		tmp3 = va_arg(args, int);	/* buffer id */
		resid = va_arg(args, int *);
		va_end(args);
		cdb->g1_rsvd0 = (us.uscsi_buflen >> 16) & 0xFF;
		cdb->g1_count1 = (us.uscsi_buflen >> 8) & 0xFF;
		cdb->g1_count0 = us.uscsi_buflen & 0xFF;
		FORMG1ADDR(cdb, tmp1);
		cdb->g1_addr3 = tmp3;
		cdb->g1_reladdr = tmp2 & 0x07;
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * DENSITY_SUPPORT takes 4 parameters: (void *)address, int
		 * media, int length, int &resid address - address for return
		 * of data media - 0 support by tape drive, 1 support by
		 * media length - length to return resid - for return of
		 * residual
		 */

	case SCMD_DENSITY_SUPPORT:	/* 0x44 */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		tmp1 = va_arg(args, int);	/* media */
		tmp2 = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		cdb->g1_reladdr = tmp1 & 1;	/* media */
		FORMG1COUNT(cdb, tmp2);
		us.uscsi_buflen = tmp2;
		break;

		/*
		 * LOG_SELECT takes 5 parameters: (void *)address, int len,
		 * int pcr, int sp, int pc address - address of parameter
		 * list len - length of parameter list pcr - parameter code
		 * reset sp - save parameter pc - page control
		 */

	case SCMD_LOG_SELECT:	/* 0x4c */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		tmp4 = va_arg(args, int);	/* length */
		tmp1 = va_arg(args, int);	/* pcr */
		tmp2 = va_arg(args, int);	/* sp */
		tmp3 = va_arg(args, int);	/* pc */
		va_end(args);
		cdb->cdb_opaque[1] = ((tmp1 ? 0x02 : 0) | (tmp2 & 0x1));
		cdb->cdb_opaque[2] = ((tmp3 & 0x03) << 6);
		if (tmp4 != 0 && us.uscsi_bufaddr != NULL) {
			FORMG1COUNT(cdb, tmp4);
			us.uscsi_buflen = tmp4;
#if USCSI_WRITE == 0
			us.uscsi_flags &= ~USCSI_READ;
#else
			fix this;
#endif
		}
		break;

		/*
		 * LOG_SENSE takes 6 parameters: (void *)address, int pc, int
		 * page_code, int para_point, int length, int &resid address
		 * - address for return of data pc - Page control page_code -
		 * page code para_point - parameter pointer(not a C pointer)
		 * length - lenght to return resid - for return of residual
		 */

	case SCMD_LOG_SENSE:	/* 0x4d */
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		tmp1 = va_arg(args, int);	/* pc */
		tmp2 = va_arg(args, int);	/* page_code */
		tmp3 = va_arg(args, int);	/* para_point */
		tmp4 = va_arg(args, int);	/* length */
		resid = va_arg(args, int *);
		va_end(args);
		cdb->g1_addr3 = ((tmp1 & 0x3) << 6) | (tmp2 & 0x3f);
		cdb->g1_addr0 = (tmp3 >> 8) & 0xff;
		cdb->g1_rsvd0 = tmp3 & 0xff;
		FORMG1COUNT(cdb, tmp4);
		us.uscsi_buflen = tmp4;
		break;

		/*
		 * MOVE_MEDIUM takes 4 parameters: int tps_addr, int
		 * src_addr, int dest_addr, int invert tps_addr - transport
		 * element address src_addr - source address dest_addr-
		 * destination address invert   - invert media 1 = invert it
		 * 0 don't if tape library, 1 = loading cleaning 0 = not
		 */

	case SCMD_MOVE_MEDIUM:	/* 0xa5 */
		us.uscsi_cdblen = 12;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);	/* tps_addr */
		tmp2 = va_arg(args, int);	/* src_addr */
		tmp3 = va_arg(args, int);	/* dest_addr */
		tmp4 = va_arg(args, int);	/* invert */
		va_end(args);
		/* move transport address */
		cdb->scc5_addr3 = (tmp1 >> 8) & 0xff;
		cdb->scc5_addr2 = tmp1 & 0xFF;
		/* move source address */
		cdb->scc5_addr1 = (tmp2 >> 8) & 0xFF;
		cdb->scc5_addr0 = tmp2 & 0xFF;
		/* move destination address */
		cdb->scc5_count3 = (tmp3 >> 8) & 0xff;
		cdb->scc5_count2 = tmp3 & 0xFF;

		/*
		 * Process any special moves for tape libraries
		 * (load cleaning)
		 */
		if (IS_TAPELIB(un)) {
			switch (un->equ_type) {
			case DT_SPECLOG:
				/*
				 * The spectralogic needs a flag for cleaning
				 */
				cdb->cdb_opaque[11] |= (tmp4 << 6);
				break;
			}
		} else		/* Set the invert status */
			cdb->cdb_un.sg.g5.rsvd1 = tmp4 ? 1 : 0;

		break;

		/*
		 * EXCHANGE_MEDIUM takes 6 parameters: int tps_addr, int
		 * src_addr, int dest_addr1, int dest_addr2, int invert1, int
		 * invert2 tps_addr  - transport element address src_addr  -
		 * source address dest_addr1- first destination address
		 * dest_addr2- second destination address invert1   - invert
		 * first media 1 = invert it 0 don't invert2   - invert
		 * second media 1 = invert it 0 don't
		 */
	case SCMD_EXCHANGE_MEDIUM:	/* 0xa6 */
		{
			int    tmp6;

			us.uscsi_cdblen = 12;
			va_start(args, timeout);
			tmp1 = va_arg(args, int);	/* tps_addr */
			tmp2 = va_arg(args, int);	/* src_addr */
			tmp3 = va_arg(args, int);	/* dest1_addr */
			tmp4 = va_arg(args, int);	/* dest2_addr */
			tmp5 = va_arg(args, int);	/* invert1 */
			tmp6 = va_arg(args, int);	/* invert2 */
			va_end(args);
			/* move transport address */
			cdb->scc5_addr3 = (tmp1 >> 8) & 0xff;
			cdb->scc5_addr2 = tmp1 & 0xFF;
			/* move source address */
			cdb->scc5_addr1 = (tmp2 >> 8) & 0xFF;
			cdb->scc5_addr0 = tmp2 & 0xFF;
			/* move 1st destination address */
			cdb->scc5_count3 = (tmp3 >> 8) & 0xff;
			cdb->scc5_count2 = tmp3 & 0xFF;
			/* move 2nd destination address */
			cdb->scc5_count1 = (tmp4 >> 8) & 0xff;
			cdb->scc5_count0 = tmp4 & 0xFF;

			cdb->cdb_un.sg.g5.rsvd1 =
			    (((tmp6 ? 1 : 0) << 1) | (tmp5 ? 1 : 0));
			break;
		}

		/*
		 * EXTENDED_ERASE takes 2 parameters: int start, int length
		 * start   - starting sector length  - number of sectors
		 */
	case SCMD_EXTENDED_ERASE:	/* 0xac */
		us.uscsi_cdblen = 12;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);	/* start */
		tmp2 = va_arg(args, int);	/* length */
		va_end(args);
		FORMG5ADDR(cdb, tmp1);
		cdb->scc5_count3 = tmp2 >> 24;
		cdb->scc5_count2 = (tmp2 >> 16) & 0xFF;
		cdb->scc5_count1 = (tmp2 >> 8) & 0xFF;
		cdb->scc5_count0 = tmp2 & 0xFF;
		break;

		/*
		 * READ_ELEMENT_STATUS takes 6 parameters: (void *)address,
		 * int length, int start, int number, int type, int *resid
		 * address - address to place data length  - length of data
		 * start   - starting element address number  - number of
		 * elements to report type    - type of element resid   -
		 * return of residual
		 */
	case SCMD_READ_ELEMENT_STATUS:	/* 0xb8 */
		us.uscsi_cdblen = 12;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);	/* address */
		us.uscsi_buflen = va_arg(args, int);	/* length */
		tmp1 = va_arg(args, int);	/* start */
		tmp2 = va_arg(args, int);	/* number */
		tmp3 = va_arg(args, int);	/* type */
		resid = va_arg(args, int *);
		va_end(args);
		(cdb)->scc5_addr3 = (tmp1 >> 8) & 0xFF;	/* move start */
		(cdb)->scc5_addr2 = tmp1 & 0xFF;

		(cdb)->scc5_addr1 = (tmp2 >> 8) & 0xFF;	/* move number */
		(cdb)->scc5_addr0 = tmp2 & 0xFF;

		cdb->scc5_count2 = (us.uscsi_buflen >> 16) & 0xFF;
		cdb->scc5_count1 = (us.uscsi_buflen >> 8) & 0xFF;
		cdb->scc5_count0 = us.uscsi_buflen & 0xFF;

		cdb->scc5_reladdr = tmp3;
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * READY_INPORT take one parameter int element element -
		 * element address of inport to ready
		 */

	case SCMD_READY_INPORT:		/* 0xde vendor unique */
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);
		va_end(args);
		tmp1 &= 0xffff;
		FORMG0ADDR(cdb, tmp1);
		break;

		/*
		 * READ and WRITE take four arguments: (void *)address, int
		 * length, int sector, int *resid address - address of data
		 * length  - length of data sector  - starting sector #  if
		 * tape device, unused resid   - pointer for return of
		 * residual
		 *
		 * For optical media, the reads and writes are done via the
		 * group 1 read and write.  Tape uses group 0
		 */

	case READ:
		/* FALLTHRU */
	case WRITE:
		if (command == READ) {
			cdb->scc_cmd = SCMD_READ;
		} else {
			cdb->scc_cmd = SCMD_WRITE;
#if USCSI_WRITE == 0
			us.uscsi_flags &= ~USCSI_READ;
#endif
		}
		us.uscsi_cdblen = 6;
		va_start(args, timeout);
		us.uscsi_bufaddr = va_arg(args, void *);
		us.uscsi_buflen = va_arg(args, int);
		tmp1 = va_arg(args, int);
		resid = va_arg(args, int *);
		va_end(args);

		if (IS_TAPE(un)) {
			if (un->dt.tp.status.b.fix_block_mode) {
				tmp2 = BYTES2SECTORS(us.uscsi_buflen,
				    un->sector_size);
				if (resid != (int *)NULL)
					*resid = us.uscsi_buflen -
					    (tmp2 * un->sector_size);
				us.uscsi_buflen = (tmp2 * un->sector_size);
				/* set fix block mode */
				cdb->t_code = 1;
			} else
				tmp2 = us.uscsi_buflen;

			cdb->high_count = tmp2 >> 16;	/* set length */
			cdb->mid_count = ((tmp2 >> 8) & 0xff);
			cdb->low_count = (tmp2 & 0xff);
		} else if (((un->equ_type & DT_CLASS_MASK) == DT_DISK) ||
		    IS_OPTICAL(un)) {

			us.uscsi_cdblen = 10;
			cdb->scc_cmd |= SCMD_GROUP1;	/* use extended */
			FORMG1COUNT(cdb, BYTES2SECTORS(us.uscsi_buflen,
			    un->sector_size));
			FORMG1ADDR(cdb, tmp1);
			/* need to know starting sector */
			start_sector = tmp1;
		}
		req_xfer = us.uscsi_buflen;
		break;

		/*
		 * Initialize range of elements takes 3 parameters int
		 * element, in count int *resid element - starting element
		 * count   - number of elements resid   - return of residual
		 */

	case SCMD_INIT_ELE_RANGE_37:
	case SCMD_INIT_ELE_RANGE:
		us.uscsi_cdblen = 10;
		va_start(args, timeout);
		tmp1 = va_arg(args, int);	/* element */
		tmp2 = va_arg(args, int);	/* count */
		resid = va_arg(args, int *);
		va_end(args);
		if ((un->equ_type == DT_ADIC448) ||
		    (un->equ_type == DT_ADIC100) ||
		    (un->equ_type == DT_ADIC1000) ||
		    (un->equ_type == DT_EXBX80) ||
		    (un->equ_type == DT_IBM3584) ||
		    (un->equ_type == DT_HPSLXX) ||
		    (un->equ_type == DT_ATLP3000)) {

			cdb->cdb_opaque[1] = 0x01;	/* Check within range */
		}
		cdb->cdb_opaque[2] = ((tmp1 >> 8) & 0xff);
		cdb->cdb_opaque[3] = (tmp1 & 0xff);
		cdb->cdb_opaque[6] = ((tmp2 >> 8) & 0xff);
		cdb->cdb_opaque[7] = (tmp2 & 0xff);
		break;

		/*
		 * Issue a command passing the cdb, takes 6  parameters -
		 * (void *)addr1, int len1, (void  *)addr2, int len2, int
		 * direct, int *resid addr1   - address of the cdb len1    -
		 * length of cdb addr2   - address to take/put data(can be
		 * NULL) len2    - length of data transfer direct  -
		 * USCSI_READ or USCSI_WRITE resid   - return of residual
		 */

	case SCMD_ISSUE_CDB:
		va_start(args, timeout);
		/* get the cdb */
		us.uscsi_bufaddr = va_arg(args, void *);
		us.uscsi_cdblen = va_arg(args, int);	/* its length */
		memcpy(cdb, us.uscsi_bufaddr, us.uscsi_cdblen);
		us.uscsi_bufaddr = va_arg(args, void *);
		us.uscsi_buflen = va_arg(args, int);
#if USCSI_WRITE == 0
		if (va_arg(args, int) == USCSI_WRITE)
			us.uscsi_flags &= ~USCSI_READ;
#endif
		resid = va_arg(args, int *);
		va_end(args);
		break;

	default:
		un->io_active = FALSE;
		return (-1);	/* Fix.  Need to set errno??? */
	}

	if (DBG_LVL(SAM_DBG_TSCSI)) {
		memcpy(&cdb_trace.cdb, (void *) cdb, 12);
		cdb_trace.eq = un->eq;
		cdb_trace.what = 0;
		cdb_trace.fd = fd;
		(void) time(&cdb_trace.now);
		if (cdb_fd < 0) {
			mutex_lock(&cdb_fd_mutex);
			if (cdb_fd < 0) {
				char	f_name[32];
				sprintf(f_name, "/tmp/sam_scsi_trace_%d",
				    un->fseq ? un->fseq : un->eq);
				sam_syslog(LOG_DEBUG, "Open cdb trace file %s",
				    f_name);
				if ((cdb_fd = open(f_name, O_WRONLY | O_CREAT |
				    O_APPEND, 0644)) < 0)
					sam_syslog(LOG_DEBUG,
					    "cdb_trace open: %s: error: %m.",
					    f_name);
				else
					memccpy(((shm_ptr_tbl_t *)
					    master_shm.shared_memory)->
					    dis_mes[DIS_MES_CRIT],
					    "S C S I   T R A C E   F I L E"
					    " ( S )   O P E N",
					    '\0', DIS_MES_LEN);
			}
			mutex_unlock(&cdb_fd_mutex);
		}
		if (write(cdb_fd, &cdb_trace, sizeof (cdb_trace)) !=
		    sizeof (cdb_trace))
			sam_syslog(LOG_DEBUG, "cdb_trace write error %m.");
	} else if (cdb_fd >= 0) {	/* need to close it if opened */
		mutex_lock(&cdb_fd_mutex);
		if (cdb_fd >= 0) {
			sam_syslog(LOG_DEBUG, "Closing cdb trace file.");
			if (memcmp(((shm_ptr_tbl_t *)master_shm.shared_memory)->
			    dis_mes[DIS_MES_CRIT], "S C S I   T R", 13) == 0) {
				*((shm_ptr_tbl_t *)master_shm.shared_memory)->
				    dis_mes[DIS_MES_CRIT] = '\0';
			}
			(void) close(cdb_fd);
			cdb_fd = -1;
		}
		mutex_unlock(&cdb_fd_mutex);
	}
	if ((un->equ_type == DT_SQUARE_TAPE) && (command == SCMD_DOORLOCK)) {
		un->io_active = FALSE;
		return (-1);
	}
	if ((DBG_LVL(SAM_DBG_DISSCSI) && (cur_mess >= 0)) ||
	    DBG_LVL(SAM_DBG_DEBUG)) {
		int	thislen = 0;
		uint_t	i;

		memset(dis_cdb, ' ', 40);
		for (i = 0; i < us.uscsi_cdblen; i++) {
			sprintf(&dis_cdb[thislen], " %2.2x",
			    cdb->cdb_opaque[i]);
			thislen += 3;
		}
		dis_cdb[0] = '|';
		dis_cdb[thislen] = '\0';
		if (DBG_LVL(SAM_DBG_DISSCSI) && (cur_mess >= 0)) {
			if ((thislen + cur_mess) < DIS_MES_LEN)
				memcpy(d_mess + cur_mess, dis_cdb, thislen + 1);
		}
	}
	retry = 3;

	if (sp_ret)
		memset(sp_ret, 0, sizeof (sam_extended_sense_t));
redo:


#if	0
	/*
	 * Print out the scsi cdb.
	 */
	if (DBG_LVL(SAM_DBG_DEBUG)) {
		sam_syslog(LOG_DEBUG, "issued scsi cmd %s", dis_cdb);
	}
#endif

	if (tmp5 = ioctl(fd, USCSICMD, &us)) {
		/* Command failed, return the proper error */

		int    hold_errno = errno;

#if	defined(ALLOW_SETTING_ERRORS)
		if (pfg_fails && (command == pfg_command)) {
			unsigned char  *xyz = (unsigned char *) us.uscsi_rqbuf;
			memset(xyz, 0, 20);
			xyz[0] = 0x70;
			xyz[2] = pfg_sense;
			xyz[12] = pfg_asc;
			xyz[13] = pfg_ascq;
			un->io_active = FALSE;
			if (DBG_LVL(SAM_DBG_TSCSI)) {
				(void) time(&cdb_trace.now);
				cdb_trace.what = 1;
				memcpy(cdb_trace.sense, us.uscsi_rqbuf, 20);
				(void) write(cdb_fd, &cdb_trace,
				    sizeof (cdb_trace));
			}
			return (-1);
		}
#endif
		/*
		 * Attempt recovery from scsi resource problem
		 */
		if (hold_errno == EAGAIN && --retry) {
			DevLog(DL_DEBUG(1023));
			sleep(3);
			goto redo;
		}
		un->io_active = FALSE;
		if (DBG_LVL(SAM_DBG_TSCSI)) {
			(void) time(&cdb_trace.now);
			memcpy(cdb_trace.sense, us.uscsi_rqbuf, 20);
			cdb_trace.what = 1;
			(void) write(cdb_fd, &cdb_trace, sizeof (cdb_trace));
		}
		if (DBG_LVL(SAM_DBG_DISSCSI) || (cur_mess >= 0))
			*(d_mess + cur_mess) = '\0';

		sp = (sam_extended_sense_t *)us.uscsi_rqbuf;
		if (sp_ret) {
			memcpy(sp_ret, sp, sizeof (sam_extended_sense_t));
		}
		/* find resid if needed */
		if (sp->es_valid && (start_sector >= 0) &&
		    (((un->equ_type & DT_CLASS_MASK) == DT_DISK) ||
		    IS_OPTICAL(un))) {

			/* See if resid returned from the driver */
			if (req_xfer && (req_xfer != us.uscsi_resid)) {
				if (resid != (int *)NULL)
					*resid = req_xfer - us.uscsi_resid;

				errno = hold_errno;
				/* return bytes transfered */
				return (req_xfer - us.uscsi_resid);
			}
			/*
			 * set resid based on the info bytes in the sense
			 * data
			 */
			tmp1 = ((sp->es_info_1 << 24) +
			    (sp->es_info_2 << 16) + (sp->es_info_3 << 8) +
			    sp->es_info_4);

			/*
			 * resid = req_xfer - ((sector with err -
			 * starting_sector) times sector_size)
			 */
			tmp2 = req_xfer -
			    ((tmp1 - start_sector) * un->sector_size);
			if (resid != (int *)NULL)
				*resid = tmp2;

			errno = hold_errno;
			return (req_xfer - tmp2);
		} else if (sp->es_valid && IS_TAPE(un) && req_xfer &&
		    (req_xfer != us.uscsi_resid)) {

			if (resid != (int *)NULL)
				*resid = req_xfer - us.uscsi_resid;

			errno = hold_errno;
			/* return bytes transfered */
			return (req_xfer - us.uscsi_resid);

		} else {
			errno = hold_errno;
			return (-1);	/* else command must have failed */
		}
	}
#if	defined(ALLOW_SETTING_ERRORS)
	if (pfg_fails && (command == pfg_command)) {
		unsigned char  *xyz = (unsigned char *) us.uscsi_rqbuf;
		memset(xyz, 0, 20);
		xyz[0] = 0x70;
		xyz[2] = pfg_sense;
		xyz[12] = pfg_asc;
		xyz[13] = pfg_ascq;
		un->io_active = FALSE;
		if (DBG_LVL(SAM_DBG_TSCSI)) {
			(void) time(&cdb_trace.now);
			memcpy(cdb_trace.sense, us.uscsi_rqbuf, 20);
			cdb_trace.what = 1;
			(void) write(cdb_fd, &cdb_trace, sizeof (cdb_trace));
		}
		return (-1);
	}
#endif				/* ALLOW_SETTING_ERRORS */

	un->io_active = FALSE;
	if (DBG_LVL(SAM_DBG_TSCSI)) {
		(void) time(&cdb_trace.now);
		memset(cdb_trace.sense, 0, 20);
		cdb_trace.what = 1;
		(void) write(cdb_fd, &cdb_trace, sizeof (cdb_trace));
	}
	if (DBG_LVL(SAM_DBG_DISSCSI) || (cur_mess >= 0))
		*(d_mess + cur_mess) = '\0';

	if (resid != (int *)NULL)	/* return resid if asked */
		*resid = us.uscsi_resid;

	if (req_xfer)		/* if transfer asked for */
		/* return bytes transfered */
		return (req_xfer - us.uscsi_resid);

	return (0);		/* return ok */
}

int
scsi_reset(const int fd, dev_ent_t *un)
{
	struct uscsi_cmd us;
	/* LINTED pointer cast may result in improper alignment */
	union scsi_cdb *cdb = (union scsi_cdb *)& un->cdb[0];

	memset(&us, 0, sizeof (us));	/* clear the user scsi struct */
	memset(cdb, 0, CDB_GROUP5);	/* clear the cdb */
	us.uscsi_cdb = (caddr_t)cdb;	/* pointer to the cdb */
	/* sense data address */
	us.uscsi_rqbuf = (caddr_t)SHM_REF_ADDR(un->sense);
	/* length of sense data */
	us.uscsi_rqlen = sizeof (sam_extended_sense_t);
	us.uscsi_timeout = SAM_SCSI_DEFAULT_TIMEOUT;
	us.uscsi_flags = USCSI_SILENT | USCSI_RQENABLE | USCSI_RESET;
	return (ioctl(fd, USCSICMD, &us));
}

/* These are the default mappings for SCSI check condition errors */

struct scsi_mappings {
	uchar_t sense_key;
	uchar_t add_sense;
	uchar_t add_sense_qual;
	enum sam_scsi_action action;	/* recovery action */
} default_scsi_mapping[] =
{
	KEY_NO_SENSE, 0xff, 0xff, IGNORE,
	KEY_RECOVERABLE_ERROR, 0xff, 0xff, IGNORE,
	/* Cleaning cart is in drive */
	KEY_NOT_READY, 0x30, 0x03, CLEANING_CART,
	/* Media not present */
	KEY_NOT_READY, 0x3a, 0x00, NO_MEDIA,
	KEY_NOT_READY, 0xff, 0xff, IGNORE,
	/* STK 9840 will do this */
	KEY_MEDIUM_ERROR, 0x3a, 0x00, NO_MEDIA,
	KEY_MEDIUM_ERROR, 0xff, 0xff, BAD_MEDIA,
	KEY_HARDWARE_ERROR, 0xff, 0xff, DOWN_EQU,
	/* Media not present */
	KEY_ILLEGAL_REQUEST, 0x3a, 0x00, NO_MEDIA,
	KEY_ILLEGAL_REQUEST, 0x3b, 0x0d, D_ELE_FULL,
	KEY_ILLEGAL_REQUEST, 0x3b, 0x0e, S_ELE_EMPTY,
	KEY_ILLEGAL_REQUEST, 0xff, 0xff, ILLREQ,
	/* Power on, Reset, or Bus Device Reset occurred */
	KEY_UNIT_ATTENTION, 0x29, 0x00, IGNORE,
	KEY_UNIT_ATTENTION, 0xff, 0xff, WAIT_READY,
	KEY_WRITE_PROTECT, 0xff, 0xff, WRITE_PROTECT,
	KEY_BLANK_CHECK, 0xff, 0xff, BLANK_CHECK,
	KEY_VENDOR_UNIQUE, 0xff, 0xff, DOWN_EQU,
	KEY_COPY_ABORTED, 0xff, 0xff, DOWN_EQU,
	KEY_ABORTED_COMMAND, 0xff, 0xff, WAIT_READY,
	KEY_EQUAL, 0xff, 0xff, DOWN_EQU,
	KEY_VOLUME_OVERFLOW, 0xff, 0xff, DOWN_EQU,
	KEY_MISCOMPARE, 0xff, 0xff, DOWN_EQU,
	KEY_RESERVED, 0xff, 0xff, DOWN_EQU,
	0, 0, 0, END_OF_LIST
};

/*
 * process_scsi_error() - process the given sense data and return an action
 */
enum sam_scsi_action
process_scsi_error(
	dev_ent_t *un,	/* dev_ent_t with error */
	sam_scsi_err_t *scsi_err_tab,
	int log)	/* 1 = log sense , 0 = don't log */
{
	sam_extended_sense_t *sense =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	if (scsi_err_tab != NULL) {
		sam_scsi_err_t *err_tab = scsi_err_tab;
		for (; err_tab->action != END_OF_LIST; err_tab++) {
			if (((sense->es_key == err_tab->sense_key) ||
			    (err_tab->sense_key == 0xff)) &&
			    ((sense->es_add_code == err_tab->add_sense) ||
			    (err_tab->add_sense == 0xff)) &&
			    ((sense->es_qual_code == err_tab->add_sense_qual) ||
			    (err_tab->add_sense_qual == 0xff))) {
				if (err_tab->message_code != 0x00)
					DevLog(DL_ERR(1010),
					    scsi_errcode[
					    err_tab->message_code]);
				if (err_tab->action > LOG_ABOVE_THIS) {
					DevLogCdb(un);
					DevLogSense(un);
				}
				return ((int)err_tab->action);
			}
		}
		DevLogCdb(un);
		DevLogSense(un);
		return (DOWN_EQU);
	}
	if (log) {
		DevLogCdb(un);
		DevLogSense(un);
	}
	if ((sense->es_key == KEY_UNIT_ATTENTION) &&
	    ((un->equ_type == DT_VIDEO_TAPE) ||
	    (un->equ_type == DT_3570C)))
		return (NOT_READY);

	{
		struct scsi_mappings *err_tab = default_scsi_mapping;

		for (; err_tab->action != END_OF_LIST; err_tab++) {
			if (((sense->es_key == err_tab->sense_key) ||
			    (err_tab->sense_key == 0xff)) &&
			    ((sense->es_add_code == err_tab->add_sense) ||
			    (err_tab->add_sense == 0xff)) &&
			    ((sense->es_qual_code == err_tab->add_sense_qual) ||
			    (err_tab->add_sense_qual == 0xff))) {
					if ((err_tab->action >
					    LOG_ABOVE_THIS) && (!log)) {
						DevLogCdb(un);
						DevLogSense(un);
				}
				return ((int)err_tab->action);
			}
		}
	}
	if (!log) {
		DevLogCdb(un);
		DevLogSense(un);
	}
	return (DOWN_EQU);
}

/*
 * spin_unit - spinup/down a device
 *
 * on entry un->status.b.requested should be set.
 *
 * exit !0 = failure. -1 -> generic error else enum sam_scsi_action
 */

int
spin_unit(
	dev_ent_t *un,		/* the device */
	char *samst_name,	/* samst path name */
	int *open_fd,		/* place for open fd */
	int updown,		/* SPINUP or SPINDOWN */
	int eject)		/* EJECT_MEDIA or NOEJECT */
{
	int	err = 0, local_open = 0, tmp_err;
	int	timeout, local_eject, need_rewind = TRUE, spin_drive;
	time_t	start, stop;
	time_t	timeout_start, timeout_end;
	char	*d_mess = un->dis_mes[DIS_MES_NORM];
	char	*dc_mess = un->dis_mes[DIS_MES_CRIT];
	char	*msg_buf, *msg1, *msg2;
	sam_extended_sense_t *sense =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	if (IS_TAPE(un) && un->dt.tp.drive_index != SAM_TAPE_DVR_DEFAULT) {
		register int	indx = un->dt.tp.drive_index;
		if (!(IO_table[indx].initialized)) {
			mutex_lock(&IO_table[indx].mutex);
			if (!(IO_table[indx].initialized))
				if (load_tape_io_lib(&tape_IO_entries[indx],
				    &(IO_table[indx].jmp_table))) {
					memccpy(un->dis_mes[DIS_MES_CRIT],
					    catgets(catfd, SET, 2162,
					    "Runtime interface not found."),
					    '\0', DIS_MES_LEN);
					DevLog(DL_ERR(3141));
					DownDevice(un, SAM_STATE_CHANGE);
					return (-1);
				} else
					IO_table[indx].initialized = TRUE;
			mutex_unlock(&IO_table[indx].mutex);
		}
		if (IO_table[indx].jmp_table.spin_unit)
			return (IO_table[indx].jmp_table.spin_unit(un,
			    samst_name, open_fd, updown, eject));
	}
	if (!IS_TAPE(un) && updown == SPINDOWN &&
	    un->status.b.ready && un->status.b.fs_active) {
		notify_fs_invalid_cache(un);
		mutex_lock(&un->mutex);
		un->status.bits &= ~DVST_FS_ACTIVE;
		mutex_unlock(&un->mutex);
	}
	/* get an open file descriptor if needed */
	if (*open_fd < 0) {
		mutex_lock(&un->mutex);
		/*
		 * If a tape device, open and wait ready using the samst
		 * devce driver.  This should fix the problem with the 2.5 st
		 * driver allowing open with conflicting modes when using
		 * O_NDELAY.  The correct device is opened before return.
		 */
		if (IS_TAPE(un)) {
			int	retry;

			if (samst_name == NULL)
				samst_name = samst_devname(un);

			if (updown == SPINDOWN)
				retry = 6;
			else
				retry = 2;

			*open_fd = open_unit(un, samst_name, retry);
			local_open = -1;
			if (samst_name != (char *)un->dt.tp.samst_name)
				free(samst_name);
		} else if ((*open_fd = open_unit(un, un->name, 2)) >= 0)
			local_open = 1;

		if (*open_fd < 0) {
			mutex_unlock(&un->mutex);
			if (updown == SPINUP) {
				memccpy(dc_mess, catgets(catfd, SET, 2659,
				    "Unable to open for spinup"),
				    '\0', DIS_MES_LEN);
			}
#if	defined(FUJITSU_SIMULATOR_XX)
			return ((int)0);
#else
			return ((int)DOWN_EQU);
#endif
		}
		mutex_unlock(&un->mutex);
	}
	tapeclean(un, *open_fd);

	timeout = ((un->equ_type == DT_LINEAR_TAPE) &&
	    (updown == SPINUP)) ? SPINUP_DLT_TUR_TIMEOUT : INITIAL_TUR_TIMEOUT;

	mutex_lock(&un->io_mutex);

	/* initial wait for 3590 drives - otherwise strange results on TUR */
	if (un->equ_type == DT_3590)
		sleep(30);

	(void) time(&start);
	if (updown == SPINUP) {
		memccpy(d_mess, catgets(catfd, SET, 2897,
		    "waiting for drive to become ready"), '\0', DIS_MES_LEN);
	}
	/*
	 * Test Unit Ready
	 */

	if ((updown == SPINUP) && IS_TAPE(un)) {
		int    local_retry;

		for (local_retry = 2; ; local_retry--) {
			TAPEALERT(*open_fd, un);
			err = do_tur(un, *open_fd, timeout);
			if (TUR_ERROR(err) || (local_retry <= 0)) {
				mutex_unlock(&un->io_mutex);
				memccpy(dc_mess, catgets(catfd, SET, 899,
				    "Did not become ready, check log"),
				    '\0', DIS_MES_LEN);
				if (local_open != 0) {
					mutex_lock(&un->mutex);
					/* close_unit closes an open_fd */
					(void) close_unit(un, open_fd);
					mutex_unlock(&un->mutex);
					local_open = 0;
				}
				return (err);
			} else if ((err == WAIT_READY) ||
			    (err == WAIT_READY_LONG) ||
			    (err == LONG_WAIT_LOG)) {
				sleep((err == WAIT_READY) ?
				    WAIT_TIME_FOR_READY :
				    ((err == WAIT_READY_LONG) ?
				    WAIT_TIME_FOR_READY_LONG :
				    WAIT_TIME_FOR_LONG_WAIT_LOG));
				continue;
			} else
				break;	/* done */
		}
	}
	if (updown == SPINDOWN) {
		if (!(IS_OPTICAL(un))) {
			(void) sef_data(un, *open_fd);
		}
	}
	time(&timeout_start);
	timeout_end = timeout_start + (5 * 60);	/* 5 minutes */

	/*
	 * Load Media
	 */

	for (;;) {
		int		is_DLTx700 = FALSE;
		dev_ent_t	*l_un = NULL;

		if (time((time_t *)0) >= timeout_end) {
			/*
			 * If we are spinning up - this is an error. If we
			 * are spinning down we'll log the error and try to
			 * continue
			 */
			DevLog(DL_ERR(3221), updown == SPINUP ?
			    "load" : "unload");
			DevLogCdb(un);
			DevLogSense(un);
			if (updown == SPINUP) {
				memccpy(dc_mess, catgets(catfd, SET, 899,
				    "Did not become ready, check log"),
				    '\0', DIS_MES_LEN);
				err = NOT_READY;
			} else {
				err = 0;
				memccpy(dc_mess, catgets(catfd, SET, 1086,
				    "Did not unload, check log"),
				    '\0', DIS_MES_LEN);
			}
			break;
		}
		/* if media changer, need it */
		if (un->fseq != 0) {
			l_un = DEV_ENT(un->fseq);
			is_DLTx700 = l_un->equ_type == DT_DLT2700;
		}
		(void) time(&start);
		/*
		 * rewind devices that cannot unload in the middle. If the
		 * site wishes, they can configure the device to not rewind
		 * at unload.  All devices not listed here must be rewound
		 * before unloading.  We need to do the rewind because some
		 * devices return from the unload command while a rewind is
		 * in progress.
		 */
		if ((IS_TAPE(un)) && (updown == SPINDOWN) && need_rewind &&
		    !(un->equ_type == DT_VIDEO_TAPE)) {
			memccpy(d_mess, catgets(catfd, SET, 2128, "Rewinding"),
			    '\0', DIS_MES_LEN);
			(void) time(&start);
			(void) scsi_cmd(*open_fd, un, SCMD_REZERO_UNIT, 300);
			(void) time(&stop);
			DevLog(DL_TIME(3031), stop - start);
			need_rewind = FALSE;
			(void) time(&start);
			TAPEALERT_SKEY(*open_fd, un);
		}
		/* If its a spindown and optical and GRAU , do eject */
		local_eject = (IS_OPTICAL(un) && (updown == SPINDOWN) &&
		    l_un && (l_un->type == DT_GRAUACI)) ? EJECT_MEDIA : eject;

		memset(sense, 0, sizeof (sam_extended_sense_t));
		if (IS_TAPE(un)) {
			if (updown == SPINDOWN) {
				msg1 = catgets(catfd, SET, 2790,
				    "Unloading %s");
				msg_buf = malloc_wait(strlen(msg1) +
				    strlen(un->vsn) + 4, 2, 0);
				sprintf(msg_buf, msg1, un->vsn);
				memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
				free(msg_buf);
			} else
				memccpy(d_mess,
				    catgets(catfd, SET, 1573, "Loading"),
				    '\0', DIS_MES_LEN);
		} else {
			if (updown == SPINDOWN) {
				msg1 = catgets(catfd, SET, 2386,
				    "Spin down  %s");
				msg_buf = malloc_wait(strlen(msg1) +
				    strlen(un->vsn) + 4, 2, 0);
				sprintf(msg_buf, msg1, un->vsn);
				memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
				free(msg_buf);
			} else
				memccpy(d_mess,
				    catgets(catfd, SET, 2387, "Spin up"),
				    '\0', DIS_MES_LEN);
		}

		/*
		 * If this device is in a 2700 type loader, don't issue the
		 * unload.  If a reset has been issued or this is after a
		 * power on, then the loader is in "sequential" mode and the
		 * unload will force the next cartridge to be loaded. This
		 * condition should be fixed with the code to mode select to
		 * not go into sequential mode, but lets not do it anyway...
		 *
		 * Also if its a tape and just passed TUR then don't spin up.
		 */

		spin_drive = FALSE;
		if (!((IS_TAPE(un)) && (un->status.bits & DVST_READY) &&
		    (updown == SPINUP)) &&
		    !(is_DLTx700 && (updown == SPINDOWN))) {

			spin_drive = TRUE;
			TAPEALERT(*open_fd, un);
			tmp_err = scsi_cmd(*open_fd, un, SCMD_START_STOP, 300,
			    updown, local_eject);
			TAPEALERT(*open_fd, un);
		}
		if (spin_drive == TRUE && tmp_err < 0) {
			enum sam_scsi_action scsi_err;
			int no_retry = 0;

			scsi_err = process_scsi_error(un, NULL, ERR_SCSI_NOLOG);
			switch ((int)scsi_err) {
				/* ignore errors */
			case BLANK_CHECK:
				/*
				 * This is here because
				 * someday the Sony will
				 * return it
				 */
			case WRITE_PROTECT:
				/*
				 * This is here because
				 * someday the Sony will
				 * return it
				 */
			case NEEDS_FORMATTING:
				/*
				 * This is here because
				 * someday the Sony will
				 * return it
				 */
			case IGNORE:
			case NO_MEDIA:
			case CLEANING_CART:
				no_retry = 1;
				if ((scsi_err == NO_MEDIA) &&
				    (updown == SPINUP))
					/* This is an error */
					err = (int)scsi_err;
				else
					err = 0;
				break;

				/* Fatal errors */
			case DOWN_EQU:
			case DOWN_SUB_EQU:
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(3032), un->slot);
				} else {
					DevLog(DL_ERR(3253));
				}

				memccpy(dc_mess, catgets(catfd, SET, 899,
				    "Did not become ready, check log"),
				    '\0', DIS_MES_LEN);
				err = (int)scsi_err;
				no_retry = 1;

				break;

				/* Bad media is a fatal */
			case BAD_MEDIA:
				DevLog(DL_ERR(3231), un->slot);
				memccpy(dc_mess, catgets(catfd, SET, 1087,
				    "Drive contains bad media - check log"),
				    '\0', DIS_MES_LEN);
				err = (int)scsi_err;
				no_retry = 1;
				break;

				/* No point in retrying illegal requests */
			case ILLREQ:
				DevLog(DL_ERR(3232));
				if (updown == SPINUP) {
					memccpy(dc_mess,
					    catgets(catfd, SET, 899,
					    "Did not become ready,"
					    " check log"),
					    '\0', DIS_MES_LEN);
				} else {
					memccpy(dc_mess,
					    catgets(catfd, SET, 1086,
					    "Did not unload, check log"),
					    '\0', DIS_MES_LEN);
				}
				err = (int)scsi_err;
				no_retry = 1;
				break;

			case NOT_READY:
				/* if already spundown - ignore */
				if ((updown == SPINDOWN) &&
				    ((sense->es_add_code == 4) &&
				    (sense->es_qual_code == 2))) {
					/*
					 * We close here because the close
					 * lower will only close on error
					 * (err !=0)
					 */
					if (local_open != 0) {
						mutex_lock(&un->mutex);
						/*
						 * close_unit closes an open
						 * fd
						 */
						(void) close_unit(un, open_fd);
						mutex_unlock(&un->mutex);
						local_open = 0;
					}
					err = 0;
					no_retry = 1;
					break;
				}
				/* If becoming ready */
				if (updown == SPINUP) {
					if (un->type == DT_VIDEO_TAPE) {
						memccpy(d_mess,
						    catgets(catfd, SET, 525,
						    "Becoming ready"),
						    '\0', DIS_MES_LEN);
					} else if (sense->es_add_code == 4 &&
					    sense->es_qual_code == 1) {
						memccpy(d_mess,
						    catgets(catfd, SET, 525,
						    "Becoming ready"),
						    '\0', DIS_MES_LEN);

						DevLog(DL_DETAIL(1014));
					}
				}
				break;

			case WAIT_READY:
			case WAIT_READY_LONG:
			case LONG_WAIT_LOG:
			sleep((err == WAIT_READY) ? WAIT_TIME_FOR_READY :
			    ((err == WAIT_READY_LONG) ?
			    WAIT_TIME_FOR_READY_LONG :
			    WAIT_TIME_FOR_LONG_WAIT_LOG));
				break;
			}
			if (no_retry)
				break;

			/* At this point retry */
			sleep(5);
		} else {
			err = 0;
			/*
			 * If this is a tape and we opened using the samst
			 * device then we need to close and reopen the rmt
			 * device for spinup. After a spinup the caller will
			 * most likely be doing some I/0 which samst does not
			 * support for tape
			 */
			if ((local_open < 0) && (updown == SPINUP)) {
				mutex_lock(&un->mutex);
				close_unit(un, open_fd);
				/* Open the real thing now */
				if ((*open_fd =
				    open_unit(un, un->name, 3)) < 0) {

					DevLog(DL_SYSERR(3033), un->name);
					mutex_unlock(&un->mutex);
					memccpy(dc_mess, GetCustMsg(2643),
					    '\0', DIS_MES_LEN);
					SendCustMsg(HERE, 2643);
					err = -1;
				} else
					mutex_unlock(&un->mutex);
			}
			break;
		}
	}	/* end of for() loop */

	mutex_unlock(&un->io_mutex);
	(void) time(&stop);
	if (!(IS_TAPE(un))) {
		if (updown == SPINUP) {
			DevLog(DL_TIME(2045), stop - start);
		} else {
			DevLog(DL_TIME(2046), stop - start);
		}
	}
	/* if there is an unload delay, do it now */
	if (updown == SPINDOWN && un->unload_delay) {
		DevLog(DL_DETAIL(1015), un->unload_delay);
		memccpy(d_mess, catgets(catfd, SET, 836,
		    "Delaying for media changer"), '\0', DIS_MES_LEN);
		sleep(un->unload_delay);
	}
	msg1 = catgets(catfd, SET, 871, "Device has %s");
	msg2 = updown == SPINDOWN ? (IS_TAPE(un) ?
	    catgets(catfd, SET, 2787, "unloaded") :
	    catgets(catfd, SET, 2388, "spun down")) :
	    catgets(catfd, SET, 524, "become ready");
	msg_buf = malloc_wait(strlen(msg1) + strlen(msg2) + 4, 2, 0);
	sprintf(msg_buf, msg1, msg2);
	memccpy(d_mess, msg_buf, '\0', DIS_MES_LEN);
	free(msg_buf);
	if (err != 0) {
		if (un->slot != ROBOT_NO_SLOT) {
			DevLog(DL_ERR(1119), updown, un->slot, err);
		} else {
			DevLog(DL_ERR(1156), updown, err);
		}

		/* close on errors */
		if (local_open != 0) {
			/* close_unit sets open_fd to -1 */
			mutex_lock(&un->mutex);
			(void) close_unit(un, open_fd);
			mutex_unlock(&un->mutex);
		}
	}
	DevLog(DL_DETAIL(1016));
	return (err);
}

/*
 * open_unit - Open the device
 *
 * un->mutex aquired before calling.
 */
int
open_unit(dev_ent_t *un, char *path, int retry)
{
	int	flags;
	int	hold_err;
	int	open_fd;
	int	rd_only;
	int	retries;
	struct stat	stat_buf;

	/* Initialize local variables */
	flags = O_NDELAY;
	open_fd = -1;

	/*
	 * If tape device and the path is the "st" path name don't open with
	 * O_NDELAY.
	 */

	if (IS_TAPE(un) &&
	    (strcmp(path, un->name) == 0)) {
		flags &= ~O_NDELAY;
	}
	for (retries = retry; (retries > 0) && (open_fd < 0); retries--) {
		rd_only = (un->status.bits) &
		    (DVST_READ_ONLY | DVST_WRITE_PROTECT);

		DevLog(DL_DETAIL(1017), path, flags);

		if ((open_fd = open(path, (rd_only ? O_RDONLY : O_RDWR) |
		    flags)) >= 0) {
			INC_OPEN(un);

			if (un->scsi_type == '\0') {
				ident_dev(un, open_fd);
			}
			/*
			 * if it's a tape device, and if we're supposed to
			 * log sef data, figure out the intersection of the
			 * sets {log sense pages we're interested in} and
			 * {log sense pages supported by this device}.
			 */

			if (IS_TAPE(un)) {
				if (un->st_rdev == 0 &&
				    strcmp(path, un->name) == 0) {
					if (fstat(open_fd, &stat_buf)) {
						DevLog(DL_SYSERR(1018),
						    un->name);
					} else {
						un->st_rdev = stat_buf.st_rdev;
					}
				}
				if (sef_status()) {
					DevLog(DL_MSG(11010), un->name);

					if (!un->sef_info.sef_inited) {
						(void) sef_init(un, open_fd);
					}
				} else {
					DevLog(DL_MSG(11011));
				}
			}
			break;
		} else {
			hold_err = errno;

			if (hold_err == EACCES) {
				/*
				 * Slot switched to write protected since
				 * last mounted.
				 *
				 * Log a message, switch status bits to indicate
				 * the tape is write protected.
				 */
				DevLog(DL_ERR(1150), un->slot);
				un->status.bits |= DVST_WRITE_PROTECT;
				continue;
			}
			DevLog(DL_SYSERR(1012), path, hold_err);
			sleep(10);
		}
	}

	if (open_fd < 0) {
		/*
		 * Attempt to open the path did not succeed; log an error
		 * message:
		 */

		DevLog(DL_SYSERR(1151), path);
	} else if (rd_only) {
		/*
		 * Successfully opened path in read-only mode; log a message:
		 */
		DevLog(DL_DETAIL(1153), open_fd, path, un->open_count);
	} else {
		/*
		 * Successfully opened the path in read-write mode; log a
		 * message:
		 */

		DevLog(DL_DETAIL(1152), open_fd, path, un->open_count);
	}

	return (open_fd);
}

/*
 * close_unit - Close the device for this process.  Others may still have it
 * opened. un->mutex aquired before calling
 */
void
close_unit(dev_ent_t *un, int *open_fd)
{
	DEC_OPEN(un);
	DevLog(DL_DETAIL(1020), *open_fd, un->open_count);
	if (*open_fd >= 0)
		(void) close(*open_fd);

	*open_fd = -1;
}

/*
 * do_tur() - Perform Test Unit Ready. Returns : 0 No error -1 generic error
 * (int)enum sam_scsi_action
 */

int
do_tur(dev_ent_t *un, int open_fd, int timeout)
{
	char	*d_mess = un->dis_mes[DIS_MES_NORM];
	char	*dc_mess = un->dis_mes[DIS_MES_CRIT];
	sam_extended_sense_t *sense =
	    (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	int	added_more_time = FALSE, wait_time_in_secs, ret,
	    no_media_counter = 0;
	time_t	start, end;
	int	load_cmd_issued = FALSE;


	switch (un->equ_type) {
	case DT_TITAN:
		wait_time_in_secs = 20;
		break;

	case DT_LINEAR_TAPE:
		wait_time_in_secs = 30;
		break;

	default:
		wait_time_in_secs = 5;
		break;
	}

	mutex_lock(&un->mutex);
	un->status.bits &= ~DVST_READY;
	mutex_unlock(&un->mutex);
	start = time(NULL);
	end = start + (time_t)timeout;

	/*
	 * We are about to start the Test Unit Ready (TUR), log a detail
	 * message to the dev log:
	 */

	if (un->slot != ROBOT_NO_SLOT) {
		DevLog(DL_DETAIL(1154), un->slot);
	} else {
		DevLog(DL_DETAIL(1157));
	}

	for (;;) {
		memset(sense, 0, sizeof (sam_extended_sense_t));
		if ((ret = scsi_cmd(open_fd, un, SCMD_TEST_UNIT_READY, 20))
		    < 0 || sense->es_key != KEY_NO_SENSE) {

			TAPEALERT_SKEY(open_fd, un);

			/*
			 * Not Ready
			 */

			if (sense->es_key == KEY_NOT_READY) {
				if (sense->es_add_code == 0x04) {
					/* Unit Not Ready */
					memccpy(d_mess,
					    catgets(catfd, SET, 525,
					    "becoming ready"),
					    '\0', DIS_MES_LEN);
					if (un->equ_type == DT_LINEAR_TAPE) {
						switch (sense->es_qual_code) {
						case 0x01:
						/*
						 * Unit not ready, calibration
						 * in progress
						 */
							memccpy(d_mess,
							    catgets(catfd, SET,
							    550,
							    "calibrating"),
							    '\0', DIS_MES_LEN);

							if (added_more_time ==
							    FALSE) {
								end += 180;
								added_more_time
								    = TRUE;
							}
							break;
						case 0x03:
						/*
						 * Unit not ready, manual
						 * intervention needed
						 */
							if (un->slot !=
							    ROBOT_NO_SLOT) {
							DevLog(DL_DETAIL(3219),
							    un->slot,
							    sense->es_key,
							    sense->es_add_code,
							    sense->
							    es_qual_code);

							} else {
							DevLog(DL_DETAIL(3254),
							    sense->es_key,
							    sense->es_add_code,
							    sense->
							    es_qual_code);
							}
							DevLogCdb(un);
							DevLogSense(un);
							return ((int)DOWN_EQU);
						default:
							break;
						}
					}
					/*
					 * UNIT NOT READY: load command
					 * needed
					 */
					if (sense->es_qual_code == 0x02 &&
					    load_cmd_issued == FALSE) {
						DevLog(DL_DETAIL(3267));
						if (scsi_cmd(open_fd, un,
						    SCMD_START_STOP, 300,
						    1, 0) < 0) {

							DevLogCdb(un);
							DevLogSense(un);
						}
						TAPEALERT(open_fd, un);
						load_cmd_issued = TRUE;
						end += 180;
						sleep(180);
						continue;
					}
					sleep(wait_time_in_secs);
					wait_time_in_secs = 5;
				} else if (un->equ_type == DT_SONYDTF &&
				    sense->es_key == 0x02 &&
				    sense->es_add_code == 0x4c) {
					/*
					 * Check for sony dtf and a no load
					 * condition
					 */
					char	tim_scr[40];
					uchar_t	recov_cdb[10];
					time_t	now;

					(void) time(&now);
					(void) ctime_r(&now, tim_scr, 40);
					tim_scr[24] = '\0';
					sprintf(d_mess, "recovery started %s",
					    tim_scr);

					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DETAIL(3028),
						    un->slot);
					} else {
						DevLog(DL_DETAIL(3250));
					}

					memset(recov_cdb, 0, 10);
					recov_cdb[0] = 0xe9;

					memset(sense, 0,
					    sizeof (sam_extended_sense_t));
					if (scsi_cmd(open_fd, un,
					    SCMD_ISSUE_CDB, (3600 * 4),
					    recov_cdb, 10, NULL, 0,
					    USCSI_WRITE, NULL) ||
					    sense->es_key != 0) {
						sprintf(d_mess,
						    "recovery failed. %s",
						    tim_scr);

						if (un->slot != ROBOT_NO_SLOT) {
							DevLog(DL_ERR(3029),
							    un->slot);
						} else {
							DevLog(DL_ERR(3251));
						}

						DevLogCdb(un);
						DevLogSense(un);
						TAPEALERT_SKEY(open_fd, un);
						/* return error */
						return ((int)NOT_READY);
					} else {

						if (un->slot != ROBOT_NO_SLOT) {
							DevLog(DL_TIME(3030),
							    un->slot,
							    time(NULL) - now);
						} else {
							DevLog(DL_TIME(3252),
							    time(NULL) - now);
						}
						/* One more try at test ready */
						end = time((time_t *)0) + 5;
					}
				} else if ((sense->es_add_code == 0x3a) &&
				    (sense->es_qual_code == 0)) {
					/* No media present */
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DETAIL(5301),
						    un->slot);
					} else {
						DevLog(DL_DETAIL(5347));
					}

					/*
					 * Some drives will return NO_MEDIA
					 * if we TUR them too quickly after
					 * loading the tape. So we'll give
					 * them 5 seconds.
					 */
					if (no_media_counter == 0) {
						no_media_counter++;
						sleep(wait_time_in_secs);
						wait_time_in_secs = 5;
					} else {
						/* Sony's can't be trusted */
						if (un->equ_type ==
						    DT_SONYDTF) {
							sleep(1);
						} else {
							if (un->slot !=
							    ROBOT_NO_SLOT) {
					DevLog(DL_DETAIL(5301), un->slot);
							} else {
							DevLog(DL_DETAIL(5347));
							}

							DevLogCdb(un);
							DevLogSense(un);
							return ((int)NO_MEDIA);
						}
					}
				} else {
					/* wait and retry */
					sleep(wait_time_in_secs);
					wait_time_in_secs = 5;
				}
			}
			/*
			 * Unit Attention
			 */

			else if (sense->es_key == KEY_UNIT_ATTENTION) {
				if ((sense->es_add_code == 0x28) ||
				    (sense->es_add_code == 0x29)) {
					/* Not Ready or Reset occured */
					sleep(1);
				} else if ((un->equ_type == DT_SONYDTF) &&
				    (sense->es_add_code == 0x5a) &&
				    (sense->es_qual_code == 0x00)) {
					sleep(1);
				} else {
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DETAIL(5303),
						    un->slot);
					} else {
						DevLog(DL_DETAIL(5349));
					}

					sleep(1);
				}
			}
			/*
			 * Retry all thsee
			 */
			else if ((sense->es_key == KEY_ABORTED_COMMAND) ||
			    (sense->es_key == KEY_VENDOR_UNIQUE) ||
			    (sense->es_key == KEY_EQUAL) ||
			    (sense->es_key == KEY_VOLUME_OVERFLOW) ||
			    (sense->es_key == KEY_MISCOMPARE) ||
			    (sense->es_key == KEY_COPY_ABORTED)) {
				/* Retry */
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_DETAIL(5308), un->slot);
				} else {
					DevLog(DL_DETAIL(5354));
				}
			}
			/*
			 * Hardware Error
			 */
			else if (sense->es_key == KEY_HARDWARE_ERROR) {
				/* Drive errors */
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(5304), un->slot);
				} else {
					DevLog(DL_ERR(5350));
				}

				DevLogCdb(un);
				DevLogSense(un);
				return ((int)DOWN_EQU);
			}
			/*
			 * Medium Error
			 */

			else if (sense->es_key == KEY_MEDIUM_ERROR) {
				/* media errors */
				if ((sense->es_add_code == 0x3a) &&
				    (sense->es_qual_code == 0)) {
					/*
					 * No media present - stk 9840 will
					 * do this
					 */
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DETAIL(5301),
						    un->slot);
					} else {
						DevLog(DL_DETAIL(5347));
					}

					DevLogCdb(un);
					DevLogSense(un);
					return ((int)NO_MEDIA);
				} else {
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_ERR(5305), un->slot);
					} else {
						DevLog(DL_ERR(5351));
					}

					DevLogCdb(un);
					DevLogSense(un);

					return ((int)BAD_MEDIA);
				}
			}
			/*
			 * Blank Check, Write Protect, Recoverable
			 */

			else if ((sense->es_key == KEY_BLANK_CHECK) ||
			    (sense->es_key == KEY_WRITE_PROTECT) ||
			    (sense->es_key == KEY_RECOVERABLE_ERROR)) {
				/* Done */
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_DETAIL(5309), un->slot,
					    sense->es_key,
					    sense->es_add_code,
					    sense->es_qual_code);
				} else {
					DevLog(DL_DETAIL(5355), sense->es_key,
					    sense->es_add_code,
					    sense->es_qual_code);
				}

				break;
			}
			/*
			 * Illegal Request
			 */

			else if (sense->es_key == KEY_ILLEGAL_REQUEST) {
				if ((sense->es_add_code == 0x3a) &&
				    (sense->es_qual_code == 0)) {
					/* No media present */
					if (un->slot != ROBOT_NO_SLOT) {
						DevLog(DL_DETAIL(5301),
						    un->slot);
					} else {
						DevLog(DL_DETAIL(5347));
					}

					DevLogCdb(un);
					DevLogSense(un);
					return ((int)NO_MEDIA);
				}
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(5306), un->slot);
				} else {
					DevLog(DL_ERR(5352));
				}

				DevLogCdb(un);
				DevLogSense(un);
				return ((int)ILLREQ);
			}
			/*
			 * No Sense
			 */

			else if (sense->es_key != KEY_NO_SENSE) {
				/* Everything else */
				if (un->slot != ROBOT_NO_SLOT) {
					DevLog(DL_ERR(5307), un->slot,
					    errno, ret);
				} else {
					DevLog(DL_ERR(5353), errno, ret);
				}

				DevLogCdb(un);
				DevLogSense(un);
				return ((int)NOT_READY);
			}
		} else {
			/* Command succeeds - no error */
			if (un->slot != ROBOT_NO_SLOT) {
				DevLog(DL_TIME(1013), un->slot,
				    time(NULL) - start);
			} else {
				DevLog(DL_TIME(1155), time(NULL) - start);
			}

			mutex_lock(&un->mutex);
			un->status.bits |= DVST_READY;
			mutex_unlock(&un->mutex);
			break;
		}

		if (time((time_t *)0) >= end) {
			/* Retries exceeded */
			if (un->slot != ROBOT_NO_SLOT) {
				DevLog(DL_ERR(5310), un->slot);
			} else {
				DevLog(DL_ERR(5356));
			}

			DevLogCdb(un);
			DevLogSense(un);
			memccpy(dc_mess, catgets(catfd, SET, 899,
			    "Did not become ready, check log"),
			    '\0', DIS_MES_LEN);
			return ((int)RETRIES_EXCEEDED);
		}
	}
	return (0);
}
