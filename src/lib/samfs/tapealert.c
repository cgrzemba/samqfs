/*
 * tapealert.c - tapealert log sense page 0x2e processing
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
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
#pragma ident "$Revision: 1.27 $"

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/scsi/scsi.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

/* scsi device */
#include "aml/proto.h"
#include "aml/device.h"
#include "aml/dev_log.h"
#include "aml/tapes.h"
#include "aml/scsi.h"

/* shared memory */
#include "aml/shm.h"

/* tapealert */
#include "aml/tapealert.h"
#include "aml/tapealert_vals.h"

#include "sam/custmsg.h"

#include <libsysevent.h>
#include <libnvpair.h>

/* global device shared memory */
extern shm_alloc_t master_shm, preview_shm;

/* sense keys for unrecovered errors */
#define	UNRECOVERED_ERROR(key)\
	(key == 3 || key == 4 || key == 7 || key == 0xa || \
	key == 0xe || key == 0x10 || key == 0x16)

static void setup_tapealert(dev_ent_t *, int);
static int tapealert_sysevent(char *, int, dev_ent_t *, int, uint64_t,
	uint64_t *);

/*
 * --- get_supports_tapealert - determine if the robot or tape drive supports
 * TapeAlert.
 */
void
get_supports_tapealert(dev_ent_t *un, int fd)
{
	int			local_open = 0;
	int			open_fd;
	sam_extended_sense_t    *sense;
#define	PAGE_LEN 256
	char			page[PAGE_LEN], *page_ptr;
	int			i, page_len, resid;
	boolean_t		unlock_needed = B_FALSE;


	/* Feature check. */
	if ((un->tapealert & TAPEALERT_ENABLED) == 0) {
		return;
	}

	/* Clear previous success flag. */
	un->tapealert &= ~TAPEALERT_SUPPORTED;

	/* Only tape drives and robots. */
	if (un->scsi_type != 1 && un->scsi_type != 8) {
		return;
	}

	/* Local file descriptor open used by fifo command message. */
	if (fd < 0) {
		if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
			if (IS_TAPE(un)) {
					char	*open_name;
					if ((open_fd = open((open_name =
					    samst_devname(un)),
					    O_RDONLY | O_NONBLOCK)) < 0) {
						if (open_name != (char *)
						    un->dt.tp.samst_name)
							free(open_name);
						return;
					} else {
						INC_OPEN(un);
						if (open_name != (char *)
						    un->dt.tp.samst_name)
							free(open_name);
						local_open = TRUE;
					}
			} else {
				DevLog(DL_DEBUG(12014));
				return;
			}
		} else {
			INC_OPEN(un);
			local_open = TRUE;
		}
	} else {
		open_fd = fd;
	}

	/* Look for log sense tapealert page 0x2e */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	(void) memset(sense, 0, sizeof (sam_extended_sense_t));

	if (mutex_trylock(&un->io_mutex) == 0) {
		unlock_needed = B_TRUE;
	}

	memset(page, 0, PAGE_LEN);
	if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 0, page, 0, 0, 0,
	    PAGE_LEN, &resid) >= 0) {

		page_len = ((PAGE_LEN - resid) >= 4 ? (PAGE_LEN - resid) : 0);
		if (page_len >= 4) {
			page_len = (page[2] << 8) | page[3];
			page_ptr = page + 4;
		}

		for (i = 0; i < page_len; i++) {
			if (page_ptr[i] == 0x2e) {
				/* initialize tapealert data */
				un->tapealert |= TAPEALERT_SUPPORTED;
				un->tapealert_flags = 0;
				un->tapealert_vsn[0] = '\0';
				DevLog(DL_ALL(12001));
				setup_tapealert(un, open_fd);
				break;
			}
		}
	} else {
		if ( errno == EACCES ){
			DevLog(DL_DETAIL(1171), getpid(), SCMD_LOG_SENSE,
				un->name, open_fd,      strerror(errno) );
			/* set drive to state off */
			OffDevice(un, SAM_STATE_CHANGE);
			SendCustMsg(HERE, 9402);
		}
	}

	/* query, clear, and report active flags at setup */
	un->tapealert |= TAPEALERT_INIT_QUERY;
	TAPEALERT(open_fd, un);
	un->tapealert &= ~TAPEALERT_INIT_QUERY;

	if (unlock_needed == B_TRUE) {
		mutex_unlock(&un->io_mutex);
	}
	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}
}

/*
 * -- setup_tapealert - Setup TapeAlert in polling mode.  Disable
 * recovered error check condition mode.  Disable test mode. Set
 * all other flags to the default values listed in the TapeAlert
 * Specification v3.
 */
static void
setup_tapealert(dev_ent_t *un, int fd)
{
	int		resid;		/* bytes not xfered */
#define	LEN 255				/* max page len */
	uchar_t		page[LEN];	/* current page */
	int		i, len, offset; /* page variables */
#define	PERF 0x80			/* performance */
#define	EBF 0x20			/* enable background funcs */
#define	EWASC 0x10			/* excpt warn chk cond */
#define	DEXCPT 0x08			/* polling mode */
#define	TEST 0x04			/* test mode */
#define	LOGERR 0x01			/* logging of errors */
#define	MIRE 0x03			/* tapealert mode */
#define	MIRE_MASK 0x0f			/* mire field */
	sam_extended_sense_t    *sense; /* device sense data ptr */
	uchar_t		 pagechg[LEN];	/* changable page */
	int		 offsetchg;	/* chg page variables */
	uchar_t		 cdb[SAM_CDB_LENGTH];    /* custom cdb */


	/*
	 * Setup TapeAlert mode page in polling mode.
	 */


	memset(page, 0, LEN);
	memset(pagechg, 0, LEN);
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* get current page */
	if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 30, page, LEN, 0x1c,
	    &resid) < 0) {
		DevLog(DL_DEBUG(12009), sense->es_key, sense->es_add_code,
		    sense->es_qual_code);
		goto done;
	}
	len = page[0]+1;
	page[0] = 0;
	offset = page[3];
	page[4+offset] &= 0x7f;


	/* get changeable page */
	memset(cdb, 0, SAM_CDB_LENGTH);
	cdb[0] = SCMD_MODE_SENSE;
	if (IS_ROBOT(un)) {
		cdb[1] = 0x08;
	}
	cdb[2] = 0x40 | 0x1c;
	cdb[4] = LEN;
	if (scsi_cmd(fd, un, SCMD_ISSUE_CDB, 30, cdb, 6, pagechg, LEN,
	    USCSI_READ, &resid) < 0) {
		DevLog(DL_DEBUG(12010), sense->es_key, sense->es_add_code,
		    sense->es_qual_code);
		goto done;
	}
	offsetchg = pagechg[3];


	/*
	 * Change current TapeAlert page settings to polling mode.
	 */


	/* performance */
	if ((pagechg[6+offsetchg] & PERF) == PERF) {
		page[6+offset] &= ~PERF;
	}

	/* enable background functions */
	if ((pagechg[6+offsetchg] & EBF) == EBF) {
		page[6+offset] &= ~EBF;
	}

	/* exception warning reporting */
	if ((pagechg[6+offsetchg] & EWASC) == EWASC) {
		page[6+offset] &= ~EWASC;
	}

	/* disable exception control */
	if ((pagechg[6+offsetchg] & DEXCPT) == DEXCPT) {
		page[6+offset] |= DEXCPT;
	}

	/* test mode */
	if ((pagechg[6+offsetchg] & TEST) == TEST) {
		page[6+offset] &= ~TEST;
	}

	/* log err */
	if ((pagechg[6+offsetchg] & LOGERR) == LOGERR) {
		page[6+offset] &= ~LOGERR;
	}

	/* MIRE - field ignored if DEXCPT is set */
	if ((pagechg[7+offsetchg] & MIRE_MASK) != 0) {
		page[7+offset] &= ~(pagechg[7+offsetchg] & MIRE_MASK);
		page[7+offset] |= (MIRE & pagechg[7+offsetchg]);
		if ((page[7+offset] & MIRE_MASK) != MIRE) {
			page[7+offset] &= ~MIRE_MASK;
		}
	}

	/* interval timer */
	for (i = 0; i < 4; i++) {
		if (pagechg[i+8+offset] != 0) {
			page[i+8+offset] &= ~(pagechg[i+8+offset]);
		}
	}

	/* report count / test flag number */
	for (i = 0; i < 4; i++) {
		if (pagechg[i+12+offset] != 0) {
			page[i+12+offset] &= ~(pagechg[i+12+offset]);
		}
	}

	/* set polling mode and defaults */
	memset(cdb, 0, SAM_CDB_LENGTH);
	cdb[0] = SCMD_MODE_SELECT;
	cdb[1] = 0x10; /* pf bit */
	cdb[4] = len;
	if (scsi_cmd(fd, un, SCMD_ISSUE_CDB, 30, cdb, 6, page, len,
	    USCSI_WRITE, &resid) < 0) {
		DevLog(DL_DEBUG(12011), sense->es_key, sense->es_add_code,
		    sense->es_qual_code);
		goto done;
	}

done:
	/* log run-time settings from current mode page query */
	memset(page, 0, LEN);
	if (scsi_cmd(fd, un, SCMD_MODE_SENSE, 30, page, LEN, 0x1c,
	    &resid) < 0) {
		DevLog(DL_DEBUG(12012), sense->es_key, sense->es_add_code,
		    sense->es_qual_code);
		return;
	}
	offset = page[3];
	if (page[6+offset] != 8 || page[7+offset] != 3) {
		DevLog(DL_DEBUG(12013), page[6+offset], page[7+offset]);
	}
}

/*
 * -- tapealert - tapealert log sense page 0x2e processing
 * Process media changer or tape drive request for tapealert log sense
 * page 0x2e.  An active tapealert never interferes with data
 * transfers.  Active tapealert flags are written to the device log and
 * real-time notification is by a posted tapealert sysevent.  See
 * www.t10.org SSC-2 and SMC-2 for additional tapealert information.
 */
int			/* 0 successful */
tapealert(
	char *src_fn,		/* source filename */
	int src_ln,		/* source file line number */
	int fd,			/* device file descriptor */
	dev_ent_t *un,		/* device */
	uchar_t *logpage,	/* existing tapealert log sense page */
	int logpage_len)	/* existing tapealert log sense page length */
{
#define	CLEAN_NOW		0x80000
#define	CLEAN_PERIODIC		0x100000
#define	EXPIRED_CLEANING_MEDIA	0x200000
#define	INVALID_CLEANING_MEDIA	0x400000
#define	STK_CLEAN_REQUESTED	0x800000
	int			rtn = 1;
	uchar_t			*page;
	uchar_t			*log_param;
#define		PAGE_LEN 256
	uchar_t			tmp_page [PAGE_LEN];
	int 			resid;
	int 			i;
	int			j;
	int			page_len;
	int			param_len;
	int			param_code;
	int 			add_page_len;
	sam_extended_sense_t	*sense;
	sam_extended_sense_t	saved_sense;
	uchar_t			saved_cdb [16];
#define		FLAGS_LEN 64
	int			flags_len;
	uint64_t		flags;
	uchar_t			val;
	int			save_errno = errno;
	uint64_t		seq_no = 0;
	int			supported;
	int			enabled;
	int			init_query;
	int			required;
	int 			requested;
	int			expired;
	int			invalid;


	supported = un->tapealert & TAPEALERT_SUPPORTED;
	enabled = un->tapealert & TAPEALERT_ENABLED;
	init_query = un->tapealert & TAPEALERT_INIT_QUERY;
	if (!(supported && (enabled || init_query))) {
		return (0);
	}

	/*
	 * get device sense pointer.
	 */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/*
	 * Second, process tapealert log sense page.
	 */
	if (logpage == NULL || logpage_len < 4) {
		/* save callers previous cdb and sense */
		memcpy(saved_cdb, un->cdb, SAM_CDB_LENGTH);
		memcpy(&saved_sense, sense, sizeof (sam_extended_sense_t));
		if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 0, tmp_page, 0,
		    0x2e, 0, PAGE_LEN, &resid) < 0) {
			DevLog(DL_DEBUG(12002), sense->es_key,
			    sense->es_add_code, sense->es_qual_code);
			memcpy(un->cdb, saved_cdb, SAM_CDB_LENGTH);
			memcpy(sense, &saved_sense,
			    sizeof (sam_extended_sense_t));
			goto cleanup;
		}
		/* restore callers previous cdb and sense */
		memcpy(un->cdb, saved_cdb, SAM_CDB_LENGTH);
		memcpy(sense, &saved_sense, sizeof (sam_extended_sense_t));

		page = tmp_page;
		page_len = PAGE_LEN - resid;
	} else {
		page = logpage;
		page_len = logpage_len;
	}

	if (page_len < 4) {
		DevLog(DL_DEBUG(12003), page_len);
		goto cleanup;
	}

	if (page [0] != 0x2e) {
		DevLog(DL_DEBUG(12004), page [0]);
		goto cleanup;
	}

	add_page_len = (page [2] << 8) | page [3];
	add_page_len &= 0xffff;
	log_param = page + 4;

	flags = 0;
	flags_len = 0;
	for (i = 0, j = 0; i < FLAGS_LEN && j < add_page_len; i++, j += 5) {

		param_code = (log_param [j] << 8) | log_param [j + 1];
		if (param_code != (i + 1)) {
			if (flags != 0) break;
			goto cleanup;
		}

		param_len = log_param [j + 3];
		param_len &= 0xff;
		if (param_len == 1) {
			val = log_param [j + 4];
			if (i < 64 && val != 0) {
				flags |= ((uint64_t)1 << i);
			}
		} else {
			/*
			 * vendor unique flag length value, quit
			 * processing flags
			 */
			break;
		}

		/*
		 * increment number of valid TapeAlert flags
		 * contained in the 64 bit wide flags variable
		 */
		flags_len++;
	}

	/* check for in-active or already seen */
	if (flags == 0 || (flags == un->tapealert_flags &&
	    strcmp(un->tapealert_vsn, un->vsn) == 0)) {
		rtn = 0;
		goto cleanup;
	}

	/* build active flags filter by vsn */
	if (strcmp(un->tapealert_vsn, un->vsn) == 0) {
		/* check for already seen */
		if ((flags & ~un->tapealert_flags) == 0) {
			rtn = 0;
			goto cleanup;
		}
		/* current vsn, add active flag(s) to filter */
		un->tapealert_flags |= flags;
	} else {
		/* new vsn, initialize flags filter */
		un->tapealert_flags = flags;
	}
	strcpy(un->tapealert_vsn, un->vsn);

	/*
	 * send tapealert sysevent
	 */
	rtn = tapealert_sysevent(src_fn, src_ln, un, flags_len, flags, &seq_no);

	/*
	 * log active flags
	 *
	 * seq_no is zero when sysevent not sent.
	 */
	if (strlen(un->vsn) > 0) {
		DevLog(DL_ALL(12007), un->version, un->eq, un->scsi_type,
		    seq_no, flags_len, flags, un->vsn);
	} else {
		DevLog(DL_ALL(12006), un->version, un->eq, un->scsi_type,
		    seq_no, flags_len, flags);
	}

	if ((un->tapeclean & TAPECLEAN_AUTOCLEAN) &&
	    (un->tapeclean & TAPECLEAN_LOGSENSE)) {
		required = (flags & CLEAN_NOW) ? 1 : 0;
		requested = (flags & CLEAN_PERIODIC) ? 1 : 0;
		expired = (flags & EXPIRED_CLEANING_MEDIA) ? 1 : 0;
		invalid = (flags & INVALID_CLEANING_MEDIA) ? 1 : 0;
		if (un->equ_type == DT_9840 ||
		    un->equ_type == DT_9940 || un->equ_type == DT_TITAN) {
			requested = (flags & STK_CLEAN_REQUESTED) ? 1 : 0;
		}

		/* Map active TapeAlert flags onto SAM-FS status. */
		tapeclean_active(un, required, requested, expired, invalid);
	}

cleanup:

	/*
	 * restore callers errno state
	 */
	errno = save_errno;
	return (rtn);
}

/*
 * -- tapealert_skey - tapealert sense key
 * If a scsi command produced an unrecovered check condition, then
 * request the log sense tapealert page 0x2e for processing.
 */
int			/* 0 successful */
tapealert_skey(
	char *fn,		/* source filename */
	int ln,			/* source file line number */
	int fd,			/* device file descriptor */
	dev_ent_t *un)		/* device */
{
	sam_extended_sense_t	*sense;

	if (un == NULL) {
		return (1);
	}

	/* inspect sense data for unrecovered check condition */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	if (UNRECOVERED_ERROR(sense->es_key)) {
		return (tapealert(fn, ln, fd, un, NULL, 0));
	}
	return (0);
}

/*
 * -- tapealert_mts - tapealert magnetic tape
 * If an error was produced on the general magnetic tape interface,
 * then request the log sense tapealert page 0x2e for processing.
 */
int			/* 0 successful */
tapealert_mts(
	char *fn,		/* source filename */
	int ln,			/* source file line number */
	int fd,			/* device file descriptor */
	dev_ent_t *un,		/* device */
	short mt_erreg)		/* mtio error register */
{
	if (un == NULL) {
		return (1);
	}

	/* inspect mtio for unrecovered check condition */
	if (UNRECOVERED_ERROR(mt_erreg)) {
		return (tapealert(fn, ln, fd, un, NULL, 0));
	}
	return (0);
}

/* --- tapealert_sysevent - send a tapealert sysevent to n event handlers */
static int		/* 0 successful */
tapealert_sysevent(
	char *src_fn,		/* source filename */
	int src_ln,		/* source file line number */
	dev_ent_t *un,		/* device */
	int flags_len,		/* num valid tapealert flags */
	uint64_t flags,		/* tapealert flags */
	uint64_t *seq_no)	/* sysevent sequence number */
{
	int			rtn = 1;
	time_t			tm;
	nvlist_t		*attr_list = NULL;
	sysevent_id_t		eid;
	boolean_t		free_attr_list = B_FALSE;
	char			*str;

	*seq_no = 0;


	/*
	 * build and send sysevent with active tapealert flags
	 */
	if (nvlist_alloc(&attr_list, 0, 0)) {
		DevLog(DL_DEBUG(12008), "alloc", strerror(errno));
		goto done;
	}
	free_attr_list = B_TRUE;

#ifdef DEBUG
	if (nvlist_add_string(attr_list, TAPEALERT_SRC_FILE, src_fn)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_SRC_FILE, strerror(errno));
		goto done;
	}
	if (nvlist_add_int32(attr_list, TAPEALERT_SRC_LINE, src_ln)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_SRC_LINE, strerror(errno));
		goto done;
	}
#endif /* DEBUG */
	if (nvlist_add_string(attr_list, TAPEALERT_VENDOR,
	    (char *)un->vendor_id)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_VENDOR, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_PRODUCT,
	    (char *)un->product_id)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_PRODUCT, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_USN, (char *)un->serial)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_USN, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_REV, (char *)un->revision)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_REV, strerror(errno));
		goto done;
	}
	time(&tm);
	if (nvlist_add_int32(attr_list, TAPEALERT_TOD, tm)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_TOD, strerror(errno));
		goto done;
	}
	if (nvlist_add_int16(attr_list, TAPEALERT_EQ_ORD, un->eq)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_EQ_ORD, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_NAME, un->name)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_NAME, strerror(errno));
		goto done;
	}
	if (nvlist_add_byte(attr_list, TAPEALERT_VERSION, un->version)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_VERSION, strerror(errno));
		goto done;
	}
	if (nvlist_add_byte(attr_list, TAPEALERT_INQ_TYPE, un->scsi_type)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_INQ_TYPE, strerror(errno));
		goto done;
	}
	if (strlen(str = (char *)un->set) < 1) {
		str = TAPEALERT_EMPTY_STR;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_SET, str)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_SET, strerror(errno));
		goto done;
	}
	if (nvlist_add_int16(attr_list, TAPEALERT_FSEQ, un->fseq)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_FSEQ, strerror(errno));
		goto done;
	}
	if (strlen(str = (char *)un->vsn) < 1) {
		str = TAPEALERT_EMPTY_STR;
	}
	if (nvlist_add_string(attr_list, TAPEALERT_VSN, str)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_VSN, strerror(errno));
		goto done;
	}
	if (nvlist_add_int16(attr_list, TAPEALERT_FLAGS_LEN, flags_len)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_FLAGS_LEN, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint64(attr_list, TAPEALERT_FLAGS, flags)) {
		DevLog(DL_DEBUG(12008), TAPEALERT_FLAGS,
		    strerror(errno));
		goto done;
	}

	/* class, subclass, vendor, publisher, attribute list, event id */
	if (sysevent_post_event(TAPEALERT_SE_CLASS, TAPEALERT_SE_SUBCLASS,
	    TAPEALERT_SE_VENDOR, TAPEALERT_SE_PUBLISHER, attr_list, &eid)
	    != 0) {
		DevLog(DL_DEBUG(12008), TAPEALERT_SE_PUBLISHER,
		    strerror(errno));
		goto done;
	}
	*seq_no = eid.eid_seq;

	/*
	 * tapealert sysevent successfully sent.
	 */
	rtn = 0;

done:
	if (free_attr_list == B_TRUE) {
		nvlist_free(attr_list);
	}

	return (rtn);
}
