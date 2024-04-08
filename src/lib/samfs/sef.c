/*
 * sef.c - process sef requests
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

#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/scsi/impl/uscsi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <libsysevent.h>
#include <libnvpair.h>

#include "sam/types.h"
#include "sam/names.h"
#include "aml/device.h"
#include "aml/external_data.h"
#include "aml/dev_log.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/sef.h"
#include "aml/sefvals.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "sam/devnm.h"

/* global device shared memory */
extern shm_alloc_t master_shm, preview_shm;

static int sef_sysevent(dev_ent_t *un, uchar_t *wpage, int wlen,
	uchar_t *rpage, int rlen, sef_where_t where, uint64_t *seq_no);

static boolean_t	sef_once = B_FALSE;
static boolean_t	sef_on = B_FALSE;
static int		sef_file = -1;

boolean_t
sef_status(void)
{
	struct stat		statbuf;

	/* once at init, thereafter state sticks */
	if (sef_once == B_TRUE) {
		return (sef_on);
	}
	sef_once = B_TRUE;

	if (stat(SEFFILE, &statbuf) < 0) {
		/* file doesn't exist; sef is turned off */
		sef_file = open(SEFFILE, O_WRONLY | O_CREAT, 0644);
		if (sef_file < 0) {
			(void) sam_syslog(LOG_NOTICE,
			    "SEF: create of data file failed, errno %d;"
			    " sef turned off.", errno);
			sef_on = B_FALSE;
		} else {
			close(sef_file);
			sef_on = B_TRUE;
		}
	}

	return (sef_on);
}

void
sef_init(
dev_ent_t *un,
int	fd)
{
	ushort_t	count;
	struct sef_hdr	*sefhdr;
	struct sef_supp_pgs	sef_buf;
	struct		sef_pg_hdr	pghdr;
	int		resid, i, j;
	uchar_t		page;
	boolean_t	unlock_needed = B_FALSE;

	if (!sef_status()) {
		return;
	}

	/* re-init sef state info */
	un->sef_info.sef_inited = 0;

	/* Try to open sef file for writing */
	if ((sef_file = open(SEFFILE, O_WRONLY | O_APPEND | O_SYNC)) < 0) {
		/*
		 * Can't open the file, even though it exists.
		 * Turn off SEF in this process.
		 */
		DevLog(DL_MSG(11003), errno);
		sef_on = B_FALSE;
		return;
	}

	if (mutex_trylock(&un->io_mutex) == 0) {
		unlock_needed = B_TRUE;
	}

	/* issue scsi command to get log sense pages supported */
	(void) memset(&sef_buf, 0, sizeof (sef_buf));
	if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 30, &sef_buf, 1,
	    0, 0, sizeof (sef_buf), &resid) < 0) {
		DevLog(DL_MSG(11001), un->name);
		un->sef_info.sef_inited = SEF_NOT_SUPPORTED;

		/* remove lock */
		if (unlock_needed == B_TRUE) {
			mutex_unlock(&un->io_mutex);
		}
		return;
	}

	/*
	 * Build static area of sef header
	 */
	un->sef_info.sef_hdr = (struct sef_hdr *)
	    malloc_wait(sizeof (struct sef_hdr), 2, 0);
	(void) memset(un->sef_info.sef_hdr, 0, sizeof (struct sef_hdr));
	sefhdr = un->sef_info.sef_hdr;
	sefhdr->sef_magic = SEFMAGIC;
	sefhdr->sef_version = SEFVERSION;
	sefhdr->sef_eq = (uint16_t)un->eq;
	strncpy((char *)sefhdr->sef_devname, (char *)un->name, 128);
	strncpy((char *)sefhdr->sef_vendor_id, (char *)un->vendor_id, 9);
	strncpy((char *)sefhdr->sef_product_id, (char *)un->product_id, 17);
	strncpy((char *)sefhdr->sef_revision, (char *)un->revision, 5);
	sefhdr->sef_scsi_type = un->scsi_type;

	/*
	 * Figure out the intersection of the two sets here--the set of pages
	 * we're interested in and the set of pages the device supports.  For
	 * each page in the final set, get the real length of that log sense
	 * page from the device.
	 */
	BE16toH(&sef_buf.hdr.page_len, &count);
	i = 0; j = 0;
	while (i < count) {
		page = sef_buf.supp_pgs[i];
		if (SEFPAGE(page)) {
			/*
			 * Get the header of page i to find out its true length
			 * for this device. Squirrel that info away and add it
			 * to the total length of the sef record for this
			 * device.
			 */
			(void) memset(&pghdr, 0, sizeof (struct sef_pg_hdr));
			if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 30, &pghdr, 1,
			    page, 0, sizeof (struct sef_pg_hdr), &resid)
			    < 0) {
				/*
				 * Error getting this page; pretend this device
				 * doesn't support it, then.
				 */
				DevLog(DL_MSG(11002), page, un->name);
				i++;
				continue;
			}
			un->sef_info.sef_pgs[j].pgcode = page;
			BE16toH(&pghdr.page_len,
			    &un->sef_info.sef_pgs[j].pglen);
			un->sef_info.sef_pgs[j].pglen +=
			    sizeof (struct sef_pg_hdr);
			j++;
		} /* end if */
		i++;
	} /* end while */
	un->sef_info.sef_inited = SEF_INITED;

	if (unlock_needed == B_TRUE) {
		mutex_unlock(&un->io_mutex);
	}
} /* end sef_init */

int
sef_data(
dev_ent_t	*un,
int	fd
)
{
	struct sef_hdr	*sh;
	struct sef_devinfo *sd;
	int	i, resid, iovcnt = 0;
	struct iovec iov[SEF_MAX_PAGES];
	boolean_t unlock_needed = B_FALSE;

	(void) sef_data_sample(fd, un, SEF_UNLOAD);

	if (sef_status()) {

		/*
		 * Initialization.
		 */
		if (un->sef_info.sef_inited == SEF_NOT_SUPPORTED) {
			return (0);
		}


		if (un->sef_info.sef_inited != SEF_INITED) {
			sef_init(un, fd);

			if (un->sef_info.sef_inited != SEF_INITED) {
				return (0);
			}
		}

		if (mutex_trylock(&un->io_mutex) == 0) {
			unlock_needed = B_TRUE;
		}

		sd = &(un->sef_info);
		sh = sd->sef_hdr;

		/*
		 * Fill in timestamp and vsn in sef record header
		 */
		(void) time(&sh->sef_timestamp);
		memcpy(sh->sef_vsn, un->vsn, 32);

		/*
		 * Initialize record size to 0.
		 */
		sh->sef_size = 0;

		/* we'll write header out later with writev */
		iov[0].iov_base = (caddr_t)sh;
		iov[0].iov_len = sizeof (struct sef_hdr);
		iovcnt++;

		/*
		 * Log sense page codes that are both supported by this device
		 * and desired to see are stored in the sef_pgs array for this
		 * device.  The array is terminated with a page code of 0.
		 *
		 * Get each page and set up iov fields for writing later with
		 * writev.
		 */

		for (i = 0; (sd->sef_pgs[i].pgcode != 0) &&
		    (i < SEF_MAX_PAGES); i++) {
			/* malloc buffer for this log sense page */
			if ((iov[iovcnt].iov_base =
			    (caddr_t)malloc_wait(sd->sef_pgs[i].pglen,
			    1, 5)) == NULL) {
				/* couldn't malloc memory; skip this page */
				DevLog(DL_MSG(11004), sd->sef_pgs[i].pgcode,
				    un->name);
				continue;
			}

			/* issue scsi command to get this log sense page */
			if (scsi_cmd(fd, un, SCMD_LOG_SENSE, 30,
			    iov[iovcnt].iov_base, 1, sd->sef_pgs[i].pgcode,
			    0, sd->sef_pgs[i].pglen, &resid) < 0) {
				/*
				 * Error getting this page; just don't write it
				 * this time, then.
				 */
				DevLog(DL_MSG(11005), sd->sef_pgs[i].pgcode,
				    un->name);
				free(iov[iovcnt].iov_base);
				continue;
			} /* end if scsi_cmd */


			/*
			 * set up iov fields, increment iovcnt for writev,
			 * and increment sef_size (size of this sef record)
			 */
			iov[iovcnt].iov_len = sd->sef_pgs[i].pglen;
			iovcnt++;
			sh->sef_size += sd->sef_pgs[i].pglen;
		}

		/*
		 * Write the data.
		 */
		if (writev(sef_file, iov, iovcnt) < 0) {
			DevLog(DL_MSG(11006), un->name, errno);
			sef_on = B_FALSE;
		} /* end if writev */

		/* free malloc'ed buffers (but don't free iov[0].iov_base!) */
		for (i = 1; i < iovcnt; i++) {
			free(iov[i].iov_base);
		}

		if (unlock_needed == B_TRUE) {
			mutex_unlock(&un->io_mutex);
		}

	} /* end if sef_on */

	return (0);
} /* end function sef_data */


/*
 * --- get_supports_sef - determine if the robot or tape drive supports SEF.
 */
void
get_supports_sef(
dev_ent_t *un,		/* device */
int fd)			/* device file descriptor */
{
	int			local_open = 0;
	int			open_fd;
	sam_extended_sense_t    *sense;
#define	PAGE_LEN	256
	char			page[PAGE_LEN], *page_ptr;
	int			i, page_len, resid;
	boolean_t		unlock_needed = B_FALSE;
	sef_state_t		sef_mask;


	/* Feature check. */
	if ((un->sef_sample.state & SEF_ENABLED) == 0) {
		return;
	}

	/* Clear previous success flag. */
	un->sef_sample.state &= ~SEF_SUPPORTED;

	/* Only tape drives. */
	if (un->scsi_type != 1) {
		return;
	}

	/* Local file descriptor open. */
	if (fd < 0) {
		if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
			char    *open_name;

			if ((open_fd = open((open_name =
			    (char *)samst_devname(un)),
			    O_RDONLY | O_NONBLOCK)) < 0) {

				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
				return;
			} else {
				INC_OPEN(un);
				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
				local_open = TRUE;
			}
		} else {
			INC_OPEN(un);
			local_open = TRUE;
		}
	} else {
		open_fd = fd;
	}

	/* Look for log sense sef pages 2 and 3. */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);
	(void) memset(sense, 0, sizeof (sam_extended_sense_t));

	if (mutex_trylock(&un->io_mutex) == 0) {
		unlock_needed = B_TRUE;
	}

	(void) memset(page, 0, PAGE_LEN);
	if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 30, page, 0, 0, 0,
	    PAGE_LEN, &resid) >= 0) {

		page_len = ((PAGE_LEN - resid) >= 4 ? (PAGE_LEN - resid) : 0);
		if (page_len >= 4) {
			page_len = (page[2] << 8) | page[3];
			page_ptr = page + 4;
		}

		for (i = 0; i < page_len; i++) {
			if (page_ptr[i] == 2) {
				un->sef_sample.state |= SEF_WRT_ERR_COUNTERS;
			} else if (page_ptr[i] == 3) {
				un->sef_sample.state |= SEF_RD_ERR_COUNTERS;
			}
		}
	}

	un->sef_sample.state |= SEF_SYSEVENT;

	/* if write, read and sysevent then sef is supported */
	sef_mask = (SEF_WRT_ERR_COUNTERS | SEF_RD_ERR_COUNTERS | SEF_SYSEVENT);
	if ((un->sef_sample.state & sef_mask) == sef_mask) {
		un->sef_sample.state |= SEF_SUPPORTED;
		DevLog(DL_ALL(11100));
	}

	if (unlock_needed == B_TRUE) {
		mutex_unlock(&un->io_mutex);
	}
	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}
}

/*
 * -- sef - log sense write and read page processing
 * Process media changer or tape drive request for sef log sense
 * pages 2 and 3.  SEF data is sent via sysevent.
 */
int			/* 0 successful */
sef_data_sample(
int fd,			/* device file descriptor */
dev_ent_t *un,		/* device */
sef_where_t where)    /* sef location */
{
	int			rtn = 1;
	int			resid;
	sam_extended_sense_t	*sense;
	sam_extended_sense_t	saved_sense;
	uchar_t			saved_cdb[16];
	int			save_errno = errno;
	uint64_t		seq_no = 0;
#define		PAGE_LEN 256
	uchar_t			wpage[PAGE_LEN];
	uchar_t			rpage[PAGE_LEN];
	int			wlen;
	int			rlen;
	int			open_fd;
	int			local_open = 0;
	boolean_t		unlock_needed = B_FALSE;
	const int		sef_mask = SEF_SUPPORTED|SEF_ENABLED;


	if ((where == SEF_SAMPLE && (un->sef_sample.state & SEF_POLL) == 0) ||
	    un->scsi_type != 1 || (un->sef_sample.state & sef_mask)
	    != sef_mask) {
		return (0);
	}

	/* Local file descriptor open. */
	if (fd < 0) {
		if ((open_fd = open(un->name, O_RDONLY | O_NONBLOCK)) < 0) {
			char    *open_name;

			if ((open_fd = open((open_name =
			    (char *)samst_devname(un)),
			    O_RDONLY | O_NONBLOCK)) < 0) {

				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
				return (1);
			} else {
				INC_OPEN(un);
				if (open_name != (char *)un->dt.tp.samst_name)
					free(open_name);
				local_open = TRUE;
			}
		} else {
			INC_OPEN(un);
			local_open = TRUE;
		}
	} else {
		open_fd = fd;
	}

	if (mutex_trylock(&un->io_mutex) == 0) {
		unlock_needed = B_TRUE;
	}


	/* get device sense pointer */
	sense = (sam_extended_sense_t *)SHM_REF_ADDR(un->sense);

	/* save callers previous cdb and sense */
	memcpy(saved_cdb, un->cdb, SAM_CDB_LENGTH);
	memcpy(&saved_sense, sense, sizeof (sam_extended_sense_t));

	/*
	 * get sef pages.
	 */
	if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 30, wpage, 1, 2, 0,
	    PAGE_LEN, &resid) < 0) {
		DevLog(DL_DEBUG(11101), 2,
		    sense->es_key, sense->es_add_code, sense->es_qual_code);
		goto error;
	}
	wlen = PAGE_LEN - resid;

	if (scsi_cmd(open_fd, un, SCMD_LOG_SENSE, 30, rpage, 1, 3, 0,
	    PAGE_LEN, &resid) < 0) {
		DevLog(DL_DEBUG(11101), 3,
		    sense->es_key, sense->es_add_code, sense->es_qual_code);
		goto error;
	}
	rlen = PAGE_LEN - resid;

	/*
	 * send sef sysevent
	 */
	if ((rtn = sef_sysevent(un, wpage, wlen, rpage, rlen, where, &seq_no))
	    == 0) {
		un->sef_sample.state &= ~SEF_POLL;
	}

error:
	/* restore callers previous cdb and sense */
	memcpy(un->cdb, saved_cdb, SAM_CDB_LENGTH);
	memcpy(sense, &saved_sense, sizeof (sam_extended_sense_t));

	if (unlock_needed == B_TRUE) {
		mutex_unlock(&un->io_mutex);
	}

	if (local_open) {
		(void) close(open_fd);
		DEC_OPEN(un);
	}

	/*
	 * restore callers errno state
	 */
	errno = save_errno;
	return (rtn);
}

/* --- get_uints - get array of error counters from log sense page. */
static int		/* 0 successful */
get_uints(
uchar_t *page,		/* write or read error counters page */
int page_len,		/* page length */
uint64_t *uints,	/* array of counters */
int num)		/* array of counters length */
{
	char *endptr;
	int len;
	int param_code;
	int param_code_len;
	char *param;
	int i, j;
	uchar_t lp;
	uchar_t lbin;
	uint64_t value;


	memset(uints, 0, sizeof (uint64_t) * num);
	if (page == NULL || page_len < 8) {
		return (1);
	}
	len = (page[2] << 8) | page[3];

	for (i = 0; i < len && i+4 < page_len; i += (param_code_len+4)) {
		param_code = (page[4+i] << 8) | page[5+i];
		lp = page[6+i] & 0x1;
		lbin = page[6+i] & 0x2;
		param_code_len = page[7+i];

		/*
		 * stop processig after log parameter 6 or
		 * at first vendor unique log parameter
		 */
		if (param_code_len <= 0 || param_code >= num ||
		    !(param_code >= 0 && param_code <= 6)) {
			return (0);
		}

		value = 0;
		if (lp != 0 && lbin == 0) { /* ascii */
			if (param_code_len > 16) {
				return (1);
			}

			param = (char *)malloc(param_code_len+1);
			memcpy(param, &page[8+i], param_code_len);
			param[param_code_len] = '\0';
			value = strtoull(param, &endptr, 16);
			free(param);

			if (*endptr != '\0' ||
			    (value == 0 && errno == EINVAL) ||
			    (value == ULONG_MAX && errno == ERANGE)) {
				return (1);
			}
		} else { /* binary */
			if (param_code_len > 8) {
				return (1);
			}
			for (j = 0; j < param_code_len; j++) {
				value <<= 8;
				value |= page[8+i+j];
			}
		}
		uints[param_code] = value;

		if (param_code == 6 || param_code == num-1) {
			break; /* Counters 0-6 processed. */
		}
	}
	return (0);
}

/* --- sef_sysevent - send a sef sysevent to n event handlers */
static int		/* 0 successful */
sef_sysevent(
dev_ent_t *un,		/* device */
uchar_t *wpage,		/* log sense page */
int wlen,		/* log sense page length */
uchar_t *rpage,		/* log sense page */
int rlen,		/* log sense page length */
sef_where_t where,	/* sef location */
uint64_t *seq_no)	/* sysevent sequence number */
{
	int			rtn = 1;
	time_t			tm;
	nvlist_t		*attr_list = NULL;
	sysevent_id_t		eid;
	boolean_t		free_attr_list = B_FALSE;
	char			*str;
#define	UINTS_NUM	7	/* params 0-6 */
	uint64_t		uints[UINTS_NUM];
	int			i;


	*seq_no = 0;

	/*
	 * build and send sysevent with active sef flags
	 */
	if (nvlist_alloc(&attr_list, 0, 0)) {
		DevLog(DL_DEBUG(11102), "alloc", strerror(errno));
		goto done;
	}
	free_attr_list = B_TRUE;

	if (nvlist_add_string(attr_list, SEF_VENDOR, (char *)un->vendor_id)) {
		DevLog(DL_DEBUG(11102), SEF_VENDOR, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, SEF_PRODUCT, (char *)un->product_id)) {
		DevLog(DL_DEBUG(11102), SEF_PRODUCT, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, SEF_USN, (char *)un->serial)) {
		DevLog(DL_DEBUG(11102), SEF_USN, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, SEF_REV, (char *)un->revision)) {
		DevLog(DL_DEBUG(11102), SEF_REV, strerror(errno));
		goto done;
	}
	time(&tm);
	if (nvlist_add_int32(attr_list, SEF_TOD, tm)) {
		DevLog(DL_DEBUG(11102), SEF_TOD, strerror(errno));
		goto done;
	}
	if (nvlist_add_int16(attr_list, SEF_EQ_ORD, un->eq)) {
		DevLog(DL_DEBUG(11102), SEF_EQ_ORD, strerror(errno));
		goto done;
	}
	if (nvlist_add_string(attr_list, SEF_NAME, un->name)) {
		DevLog(DL_DEBUG(11102), SEF_NAME, strerror(errno));
		goto done;
	}
	if (nvlist_add_byte(attr_list, SEF_VERSION, un->version)) {
		DevLog(DL_DEBUG(11102), SEF_VERSION, strerror(errno));
		goto done;
	}
	if (nvlist_add_byte(attr_list, SEF_INQ_TYPE, un->scsi_type)) {
		DevLog(DL_DEBUG(11102), SEF_INQ_TYPE, strerror(errno));
		goto done;
	}
	if (strlen(str = (char *)un->set) < 1) {
		str = SEF_EMPTY_STR;
	}
	if (nvlist_add_string(attr_list, SEF_SET, str)) {
		DevLog(DL_DEBUG(11102), SEF_SET, strerror(errno));
		goto done;
	}
	if (nvlist_add_int16(attr_list, SEF_FSEQ, un->fseq)) {
		DevLog(DL_DEBUG(11102), SEF_FSEQ, strerror(errno));
		goto done;
	}
	str = SEF_EMPTY_STR;
	for (i = 0; dev_nmtp[i] != NULL; i++) {
		if (i == (un->type & DT_MEDIA_MASK)) {
			str = dev_nmtp[i];
			break;
		}
	}
	if (nvlist_add_string(attr_list, SEF_MEDIA_TYPE, (char *)str)) {
		DevLog(DL_DEBUG(11102), SEF_MEDIA_TYPE, strerror(errno));
		goto done;
	}
	if (strlen(str = (char *)un->vsn) < 1) {
		str = SEF_EMPTY_STR;
	}
	if (nvlist_add_string(attr_list, SEF_VSN, (char *)str)) {
		DevLog(DL_DEBUG(11102), SEF_VSN, strerror(errno));
		goto done;
	}
	if (nvlist_add_int32(attr_list, SEF_LABEL_TIME, un->label_time)) {
		DevLog(DL_DEBUG(11102), SEF_LABEL_TIME, strerror(errno));
		goto done;
	}
	if (nvlist_add_byte(attr_list, SEF_WHERE, where)) {
		DevLog(DL_DEBUG(11102), SEF_WHERE, strerror(errno));
		goto done;
	}
	if (get_uints(wpage, wlen, uints, UINTS_NUM)) {
		DevLog(DL_DEBUG(11103));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC0, uints[0])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC0, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC1, uints[1])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC1, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC2, uints[2])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC2, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC3, uints[3])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC3, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC4, uints[4])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC4, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint64(attr_list, SEF_LP2_PC5, uints[5])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC5, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP2_PC6, uints[6])) {
		DevLog(DL_DEBUG(11102), SEF_LP2_PC6, strerror(errno));
		goto done;
	}
	if (get_uints(rpage, rlen, uints, UINTS_NUM)) {
		DevLog(DL_DEBUG(11103));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC0, uints[0])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC0, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC1, uints[1])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC1, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC2, uints[2])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC2, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC3, uints[3])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC3, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC4, uints[4])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC4, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint64(attr_list, SEF_LP3_PC5, uints[5])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC5, strerror(errno));
		goto done;
	}
	if (nvlist_add_uint32(attr_list, SEF_LP3_PC6, uints[6])) {
		DevLog(DL_DEBUG(11102), SEF_LP3_PC6, strerror(errno));
		goto done;
	}

	/* class, subclass, vendor, publisher, attribute list, event id */
	if (sysevent_post_event(SEF_SE_CLASS, SEF_SE_SUBCLASS,
	    SEF_SE_VENDOR, SEF_SE_PUBLISHER, attr_list, &eid) != 0) {
		DevLog(DL_DEBUG(11102), SEF_SE_PUBLISHER, strerror(errno));
		goto done;
	}
	*seq_no = eid.eid_seq;

	/*
	 * sef sysevent successfully sent.
	 */
	rtn = 0;

done:
	if (free_attr_list == B_TRUE) {
		nvlist_free(attr_list);
	}

	return (rtn);
}
