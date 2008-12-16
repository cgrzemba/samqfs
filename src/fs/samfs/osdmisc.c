/*
 * ----- osdmisc.c - Process the object storage miscellaneous work.
 *
 * Processes the OSD error codes returned by the sosd device driver.
 * Processes the scsi sense.
 * Process the map of the object_id.
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

#pragma ident "$Revision: 1.3 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/flock.h>
#include <sys/fs_subr.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <vm/pvn.h>
#include <sys/ddi.h>
#include <sys/byteorder.h>
#include <sys/modctl.h>
#include <sys/scsi/generic/sense.h>
#include <sys/scsi/impl/sense.h>
#include <sys/scsi/scsi.h>
#include "scsi_osd.h"
#include "osd.h"


/* ----- SAMFS Includes */

#include <sam/types.h>
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "quota.h"
#include "ino_ext.h"
#include "rwio.h"
#include "indirect.h"
#include "extern.h"
#include "trace.h"
#include "debug.h"
#include "object.h"

/*
 * This routine depends on the general scsi support routines
 */
#ifndef lint
char	_depends_on[] = "misc/scsi";
#endif

#define	SCSI_VEND_SPEC_ASC		0x80
#define	SCSI_VEND_SPEC_ASCQ		0x80
#define	OSD_ROOT_QUOTA_ATTRIBUTE	0x9000000300010001ULL

extern uint8_t scsi_sense_key(uint8_t *sense_buffer);
extern uint8_t scsi_sense_asc(uint8_t *sense_buffer);
extern uint8_t scsi_sense_ascq(uint8_t *sense_buffer);

/*
 * SCSI values from SPC-3; need to find the real deal
 */
#define	SPC_ASC_INVALID_CDB		0x24
#define	SPC_ASCQ_INVALID_CDB		0
#define	SPC_SEND_DIAG_SELFTEST		4
#define	SPC_ASC_PWR_ON			0x21
#define	SPC_ASCQ_PWR_ON			1
#define	MODE_SENSE_CONTROL		0xA

static const int sosd_errcodes2errno[] = {
	0,		/* OSD_SUCCESS */
	EIO,		/* OSD_FAILURE */
	0,		/* OSD_RECOVERED_ERROR */
	EIO,		/* OSD_CHECK_CONDITION */
	EINVAL,		/* OSD_INVALID */
	E2BIG,		/* OSD_TOOBIG */
	EINVAL,		/* OSD_BADREQUEST */
	EIO,		/* OSD_LUN_ERROR */
	EIO,		/* OSD_LUN_FAILURE */
	EBUSY,		/* OSD_BUSY */
	EACCES,		/* OSD_RESERVATION_CONFLICT */
	EIO,		/* OSD_RESIDUAL */
	EAGAIN		/* OSD_NORESOURCES */
};


/*
 * ----- sam_osd_map_errno - Map sosd device driver error codes to generic
 * errno in /usr/include/sys/errno.h. This routine should only be called to
 * map sychronous error codes(rc) returned when calling sosd routines, e.g.
 * rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
 */
int
sam_osd_map_errno(int sosd_errcode)
{
	if ((sosd_errcode >= OSD_SUCCESS) &&
	    (sosd_errcode <= OSD_NORESOURCES)) {
		return (sosd_errcodes2errno[sosd_errcode]);
	} else {
		dcmn_err((CE_WARN, "SAM-QFS: Unknown SOSD ERROR sosd_errcode "
		    "=%d", sosd_errcode));
		return (EIO);
	}
}


/*
 * ----- sam_osd_sense_data - Given the OSD result struct, crack the
 * SCSI sense buffer to transalte SCSI/OSD/COMSTAR/LU/OSN FS errors
 * into errno type error code as defined in /usr/include/sys/errno.h.
 * void **bidi_info - Pointer to BIDI info area in sense data buffer (unused)
 * len - Length of BIDI info (unused)
 */
int				/* Returned errno */
sam_osd_sense_data(
	osd_result_t *resp,	/* Result struct from sosd - done routine */
	sam_osd_req_priv_t *iorp, /* Pointer to private struct */
	uint64_t *bytes_xfer)	/* Bytes transferred, 0 means all */
{
	sam_mount_t 	*mp = iorp->mp;
	sam_node_t	*ip = iorp->ip;
	char		warn_msg[128]	= " ";
	int		flags;
	int		rc;
	uint8_t		key, asc, ascq;
	uint8_t		cdb_offset = 0;
	uint8_t		*descr = NULL;
	uint64_t	descr64 = 0ULL;
	int		osd_errno;	/* Error returned by OSN FS component */
	int		error;

	error = 0;
	osd_errno = 0;
	*bytes_xfer = 0;

	/*
	 * If no sense buffer, just map sosd err_code to system errno.
	 */
	if (resp->sense_data == NULL) {
		dcmn_err((CE_WARN, "SAM-QFS %s: Sense_data buffer is NULL "
		    "SA = %x OID %d.%d",
		    mp ? mp->mt.fi_name: " ", resp->service_action,
		    (uint32_t)(iorp->object_id & 0xffffffff),
		    (uint32_t)(iorp->object_id >> 32) & 0xffffffff));
		error = sam_osd_map_errno(resp->err_code);
		return (error);
	}

	key  = scsi_sense_key(resp->sense_data);
	asc  = scsi_sense_asc(resp->sense_data);
	ascq = scsi_sense_ascq(resp->sense_data);

	sprintf(warn_msg, "SAM-QFS: %s: OSD cmd %x OID %d.%d: err_code %d "
	    "Sense Buffer key 0x%x asc 0x%x ascq 0x%x",
	    mp ? mp->mt.fi_name: " ", resp->service_action,
	    (uint32_t)(iorp->object_id & 0xffffffff),
	    (uint32_t)((iorp->object_id >> 32) & 0xffffffff),
	    resp->err_code, key, asc, ascq);

	switch (key) {

	case KEY_ILLEGAL_REQUEST:
		/*
		 * Invalid Parameter in CDB. If offset is beyond eoo
		 * this is a system error, but return EIO to the user.
		 */
		error = EIO;
		if ((asc == 0x20) && (ascq == 0)) {
			cmn_err(CE_WARN, "%s: Unsupported Function errno %d",
			    warn_msg, error);
			break;
		}

		if ((asc == SPC_ASC_INVALID_CDB) &&
		    (ascq == SPC_ASCQ_INVALID_CDB)) {
			/*
			 * Sense key specific value is offset into cdb
			 */
			rc = scsi_validate_sense(resp->sense_data,
			    resp->sense_data_len, &flags);
			if (rc == SENSE_DESCR_FORMAT) {
				descr = scsi_find_sense_descr(
				    resp->sense_data,
				    resp->sense_data_len,
				    DESCR_SENSE_KEY_SPECIFIC);
			}

			/*
			 * If this is a READ command and the offset is
			 * beyond End Of Object, sosd sets err_code =
			 * OSD_RESIDUAL and a resid will be returned in the
			 * result struct. The cdb_offset points to the
			 * starting byte of the offset field in the CDB.
			 * Caller doing READ should check for these.
			 */
			if (descr != NULL) {
				cdb_offset = *(descr + 6);
			}

			if (cdb_offset == 40) {
				/*
				 * This is a special case when offset is beyond
				 * End Of Object.
				 */
				cmn_err(CE_WARN, "%s: CDB error byte number "
				    "%d errno %d: Offset Beyond End of Object",
				    warn_msg, cdb_offset, error);
			} else {
				cmn_err(CE_WARN, "%s: CDB error byte number "
				    "%d errno %d",
				    warn_msg, cdb_offset, error);
			}
			break;
		}
		cmn_err(CE_WARN, "%s: Unsupported asc ascq codes errno %d",
		    warn_msg, error);
		break;

	case KEY_ABORTED_COMMAND:
		error = ECANCELED;
		if ((asc == 0xe) && (ascq == 2)) {
			/*
			 * Internal resource shortage in OSN
			 * Caller should retry the request again
			 */
			error = EAGAIN;
		}

		if ((asc == 0xc) && (ascq == 0xc)) {
			/*
			 * SCSI Unknown Error.  We capture the errno set by
			 * the Backing Stote File System on the OSN.
			 * This is returned in osd_errno.
			 *
			 * 0x80 is vss descriptor type
			 */
			rc = scsi_validate_sense(resp->sense_data,
			    resp->sense_data_len, &flags);
			if (rc == SENSE_DESCR_FORMAT) {
				descr = scsi_find_sense_descr(resp->sense_data,
				    resp->sense_data_len, 0x80);
			}
			if (descr != NULL) {
				memcpy(&osd_errno, descr + 2,
				    sizeof (osd_errno));
				osd_errno = BE_32(osd_errno);
			}
		}
		dcmn_err((CE_WARN, "%s: errno %d osd_errno %d", warn_msg, error,
		    osd_errno));
		break;

	case KEY_DATA_PROTECT:
		/*
		 * Check for an ENOSPC error from the OSD.
		 */
		error = EIO;
		if ((asc == 0x55) && (ascq == 7)) {
			/*
			 * quota violation
			 */
			rc = scsi_validate_sense(resp->sense_data,
			    resp->sense_data_len, &flags);
			if (rc == SENSE_DESCR_FORMAT) {
				descr = scsi_find_sense_descr(
				    resp->sense_data, resp->sense_data_len,
				    DESCR_OSD_ATTR_ID);
			}
			error = ENOSPC;
			dcmn_err((CE_WARN, "%s: No Space errno %d",
			    warn_msg, error));
			break;
#if 0
			if (descr != NULL) {
				memcpy(&descr64, descr + 2, 8);
				if (BE_64(descr64) ==
				    OSD_ROOT_QUOTA_ATTRIBUTE) {
					/*
					 * No space left on root -- ENOSPC
					 */
					error = ENOSPC;
				}
				dcmn_err((CE_WARN, "%s: No Space errno %d",
				    warn_msg, error));
				break;
			}
#endif
		}
		cmn_err(CE_WARN, "%s: Unknown KEY_DATA_PROTECT asc ascq "
		    "errno %d", warn_msg, error);
		break;

	case KEY_RECOVERABLE_ERROR:
		/*
		 * For partial read data transfer, get the amount of data that
		 * was transfered. Offset is less than eoo, but length of
		 * request exceeds the eoo.
		 */
		if ((asc == 0x3b) && (ascq == 0x17)) { /* Partial read */
			rc = scsi_validate_sense(resp->sense_data,
			    resp->sense_data_len, &flags);
			if (rc == SENSE_DESCR_FORMAT) {
				descr = scsi_find_sense_descr(resp->sense_data,
				    resp->sense_data_len,
				    DESCR_COMMAND_SPECIFIC);
			}

			if (descr != NULL) {
				/*
				 * descr64 is the number of bytes transferred
				 */
				memcpy(&descr64, descr + 2, 8);
				*bytes_xfer = BE_64(descr64);
			}
		}

		/*
		 * Get the Mini BiDi or OSN FS errno if available.
		 */
		if ((asc == SCSI_VEND_SPEC_ASC) &&
		    (ascq == SCSI_VEND_SPEC_ASCQ)) {
			rc = scsi_validate_sense(resp->sense_data,
			    resp->sense_data_len, &flags);
			if (rc == SENSE_DESCR_FORMAT) {
				descr = scsi_find_sense_descr(resp->sense_data,
				    resp->sense_data_len, 0x80);
			}
			if (descr != NULL) {
				if (*(descr + 1) <= 4) {
					/*
					 * Generic fs error
					 */
					memcpy(&osd_errno, descr+2,
					    sizeof (osd_errno));
					osd_errno = BE_32(osd_errno);
				}
			}
		}
		dcmn_err((CE_WARN, "%s: Number bytes xferred %lld osd error %d",
		    warn_msg, *bytes_xfer, osd_errno));
		break;

	default:
		/* Unusable sense */
		dcmn_err((CE_WARN, "%s: Unprocessed sense key", warn_msg));
		break;
	}

	/*
	 * If no error from scsi sense, map error from sosi to system errno.
	 */
	if (error == 0) {
		error = sam_osd_map_errno(resp->err_code);
	}
	return (error);
}


/*
 *	----	sam_check_osd_daus
 * Verify that all object pool members have matching DAUs.
 */
int			/* errno if error */
sam_check_osd_daus(
	sam_mount_t *mp)
{
	struct sam_fs_part	*fsp, *fspp;
	int			i, j;
	int			r = 0;

	/*
	 * Scan over all object devices.
	 */
	for (i = 0; i < mp->mt.fs_count; i++) {
		fsp = &mp->mi.m_fs[i].part;
		if (!is_osd_group(fsp->pt_type)) {
			continue;
		}
		/*
		 * Check all following devices of same object pool for DAU match
		 */
		for (j = i+1; j < mp->mt.fs_count; j++) {
			fspp = &mp->mi.m_fs[j].part;
			if (fsp->pt_type != fspp->pt_type) {
				continue;
			}
			if (fsp->pt_sm_dau != fspp->pt_sm_dau) {
				cmn_err(CE_WARN, "SAM-QFS, %s: Error eq %d and "
				    "eq %d, same object pool, but small DAUs "
				    "mismatch (%lld vs. %lld)", mp->mt.fi_name,
				    fsp->pt_eq, fspp->pt_eq, fsp->pt_sm_dau,
				    fspp->pt_sm_dau);
				r = EINVAL;
			}
			if (fsp->pt_lg_dau != fspp->pt_lg_dau) {
				cmn_err(CE_WARN, "SAM-QFS, %s: Error eq %d and "
				    "eq %d, same object pool, but large DAUs "
				    "mismatch (%lld vs. %lld)", mp->mt.fi_name,
				    fsp->pt_eq, fspp->pt_eq, fsp->pt_lg_dau,
				    fspp->pt_lg_dau);
				r = EINVAL;
			}
		}
	}
	return (r);
}
