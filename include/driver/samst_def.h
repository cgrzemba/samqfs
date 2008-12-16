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

/*
 * Defines and structures used only within the driver, samst.c
 */

#ifndef _SAMST_DEF_H
#define	_SAMST_DEF_H

#pragma ident "$Revision: 1.17 $"

#ifdef  __cplusplus
extern "C" {
#endif

#include "sam/osversion.h"

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * This driver does not reset the SCSI bus. Instead we just give up
 * and complain if the target hangs. If the bus is really stuck, one
 * of the Sun drivers (e.g. sd) will reset it. If you decide that
 * your device is critical and you should in fact reset the bus in
 * this driver, turn on define SCSI_BUS_RESET.
 */

/*
 * Local definitions, for clarity of code
 */
#define	SAMST_SLICE(d) (getminor(d) & 0x7) /* dev_t -> partition */
#define	SAMST_MINOR(i, s) ((i) << 3 | (s)) /* instance/slice -> minor */
#define	SAMST_DEV(i) makedevice(0, SAMST_MINOR((i), 0)) /* instance -> dev_t */
#define	SAMST_INST(d) (getminor(d) >> 3) /* dev_t -> instance */

#define	SAMST_MUTEX(targ) (&(targ)->targ_devp->sd_mutex)
#define	ROUTE(targ) (&(targ)->targ_devp->sd_address)

#define	SAMST_UNLOCK	0	/* unlock media in drive */
#define	SAMST_LOCK	1	/* lock media in drive */
#define	SAMST_START	1	/* start disk */
#define	SAMST_EJECT	2	/* eject disk */

#define	SCBP(pkt)	((struct scsi_status *)(pkt)->pkt_scbp)
#define	SCBP_C(pkt)	((*(pkt)->pkt_scbp) & STATUS_MASK)
#define	CDBP(pkt)	((union scsi_cdb *)(pkt)->pkt_cdbp)
#define	BP_PKT(bp)	((struct scsi_pkt *)bp->av_back)
#define	ARQP(pktp)	((struct scsi_arq_status *)((pktp)->pkt_scbp))

#define	SAMST_REMOVABLE(targ)	(targ)->targ_devp->sd_inq->inq_rmb

#define	SAMST_CE_DEBUG1	((1 << 8) | CE_CONT)
#define	SAMST_CE_DEBUG2	((2 << 8) | CE_CONT)
#define	SAMST_CE_DEBUG3	((3 << 8) | CE_CONT)
#define	SAMST_CE_DEBUG4	((4 << 8) | CE_CONT)

#define	SAMST_LOG		if (samst_debug) samst_log
#define	SAMST_DEBUG_ENTER	if (samst_debug) debug_enter

typedef timeout_id_t    sam_timeout_id_t;

/*
 * Private info for scsi targets.
 *
 * Pointed to by the un_private pointer
 * of one of the scsi_device structures.
 */
struct samst_soft {
	struct scsi_pkt *targ_rqs;	/* ptr to request sense command pkt */
	struct scsi_pkt *targ_pkt;	/* ptr to current command pkt */
	struct diskhd   targ_iorq;	/* i/o request queue */
	struct buf	*targ_sbufp;	/* for use in special io */
	char		*targ_rqsbufp;	/* for sense data on special io */
	kcondvar_t	targ_sbuf_cv;	/* conditional variable for sbufp */
	kcondvar_t	targ_pkt_cv;	/* conditional variable for pkt */
	ksema_t		targ_semaphore;	/* open/cose serialization */
	daddr_t		targ_low_write;	/* lowest writable block */
	daddr_t		targ_high_write; /* highest writable block */
	ushort_t	targ_dev_bsize;	/* block size */
	ushort_t	targ_dev_bshift; /* block size shift count */
	ushort_t	targ_open_cnt;	/* count of opens, zeroed on close */
	ushort_t	targ_flags;	/* flags */
	uchar_t		targ_add_sense;	/* additional sense from unit attn */
	uchar_t		targ_sense_qual; /* sense qual from unit attn */
	uchar_t		targ_sbuf_busy;	/* Wait Variable */
	uchar_t		targ_excl;	/* exclusively opened */
	short		targ_retry_ct;	/* retry count */
	uchar_t		targ_state;	/* current state */
	uint_t		targ_arq;	/* ARQ mode on this tgt */
	sam_timeout_id_t targ_timeout_ret; /* timeout return value */
	struct buf	*targ_rqbp;	/* ARQ buf pointer */
	struct scsi_device *targ_devp;	/* back pointer to scsi_device */
	char		suspended;	/* system power management */
	char		pm_suspended;	/* device power management */
	kcondvar_t	disk_busy_cv;	/* cv for blocking while device busy */
	kcondvar_t	suspend_cv;	/* cv for blocking while suspended   */
};


#endif  /* defined(_KERNEL) || defined(_KMEMUSER) */

/*
 * Node Types
 *
 * Used by samst_doattach() to classify special files
 * (via ddi_create_minor_node). They are then referenced by the devfsadm(1M)
 * plug-ins in samst_link.c to create symbolic links in /dev/samst.
 */

#define	DDI_NT_SAMST_CHAN   "ddi_samst:channel"
#define	DDI_NT_SAMST_WWN    "ddi_samst:wwn"
#define	DDI_NT_SAMST_FABRIC "ddi_samst:fabric"

/*
 * Flags
 */
#define	SAMST_FLAGS_IDLE	0x01	/* device has been idled */
#define	SAMST_FLAGS_ATTN	0x02	/* unit attention received */
					/* cleared when ioctl is issued */
					/* to retrieve the flag. */
/*
 * Driver states
 */
#define	SAMST_STATE_NIL		0
#define	SAMST_STATE_RWAIT	1	/* waiting for resources */
#define	SAMST_STATE_NORESP	2	/* device is not responding */

/*
 * Parameters
 */

#define	SAMST_IO_TIME	30	    /* default command timeout, 30sec */

/*
 * 5 seconds is what we'll wait if we get a Busy Status back
 */
#define	SAMST_BSY_TIMEOUT	 (drv_usectohz(5 * 1000000))

/*
 * Number of times we'll retry a normal operation.
 * This includes retries due to transport failure
 */
#define	SAMST_RETRY_COUNT	 3

/*
 * samst_callback action codes
 */
#define	COMMAND_DONE		0
#define	COMMAND_DONE_ERROR	1
#define	QUE_COMMAND		2
#define	QUE_SENSE		3
#define	JUST_RETURN		4
#define	COMMAND_DONE_BLKCHK	5

/*
 * Ioctl commands
 */
#define	SAMSTIOC	('S' << 8)
#define	SAMSTIOC_READY	(SAMSTIOC|0)	/* Send a Test Unit Ready command */
#define	SAMSTIOC_ERRLEV	(SAMSTIOC|1)	/* Set Error Reporting level */
#define	SAMSTIOC_SETBLK	(SAMSTIOC|2)	/* set block size */
#define	SAMSTIOC_GETOPEN (SAMSTIOC|3)	/* get open count */
#define	SAMSTIOC_IDLE	(SAMSTIOC|4)	/* set or clear idle flag 0 = clear */
#define	SAMSTIOC_ATTN	(SAMSTIOC|5)	/* get attn status */
#define	SAMSTIOC_RANGE	(SAMSTIOC|6)	/* set write range  */
/* #define SAMSTIOC_GETBLK   (SAMSTIOC|7)   get block size */

/* Structure used by SAMSTIOC_RANGE to set the write range for optical */
typedef struct {
	offset_t	low_bn;		/* lowest block number */
	offset_t	high_bn;	/* highest block number */
}samst_range_t;

#define	SAMST_RANGE_NOWRITE  0x8fffffff
#define	BUFLEN  256			/* useful buffer length */

#ifdef  __cplusplus
}
#endif

#endif /* _SAMST_DEF_H */
