/*
 * sam_move.c
 *
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

#pragma ident "$Revision: 1.21 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/exit.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "aml/samapi.h"
#include "aml/sam_utils.h"

/* Private functions. */
static int MoveCartridge(struct VolId *vid, int DestSlot, int wait_response,
		void (*MsgFunc)(int code, char *msg));
static void NoMsgFunc(int code, char *msg);

#ifdef MAIN
int main(int argc, char **argv);
#endif

#ifdef MAIN
int
main(int argc, char **argv)
{
	ushort_t	eq_ord;
	int		src_slot;
	int		dest_slot;
	int		wait_response;
	int		error;

	if (argc != 4 && argc != 5) {
		fprintf(stderr, "Usage: %s eq_ord src_slot "
		    "dest_slot [ wait_response ].\n",
		    argv[0]);
		exit(-70);
	}

	eq_ord		= (ushort_t)atoi(argv[1]);
	src_slot	= atoi(argv[2]);
	dest_slot	= atoi(argv[3]);

	if (argc > 4) {
		wait_response = atoi(argv[4]);
	} else {
		wait_response = 0;
	}

	error = sam_move(eq_ord, src_slot, dest_slot, wait_response);

	fprintf(stderr, "Called sam_move to move %d:%d to %d with", (int)eq_ord,
	    src_slot, dest_slot);

	if (wait_response) {
		fprintf(stderr, " wait-response enabled.\n");
	} else {
		fprintf(stderr, "out wait-response enabled.\n");
	}

	fprintf(stderr, "sam_move returned the value %d.  "
	    "errno has the value %d.\n",
	    error, errno);

	if (errno == 0) {
		fprintf(stderr, "The errno %d implies success.\n", (int)0);
	} else {
		char *err_msg;

		err_msg = StrFromErrno(errno, (char *)NULL, 0);

		fprintf(stderr,
		    "The errno %d corresponds to the error message: %s\n",
		    errno, err_msg);
	}

	return (errno);
}
#endif

/*
 *	SamMoveCartridge - Move cartridge in a library.
 *
 */
int				/* 0 if success, -1 if failure */
SamMoveCartridge(
	char *volspec,		/* Volume specification eq:slot or mt.vsn */
	int DestSlot,		/* Destination slot */
	/* LINTED argument unused in function */
	int WaitResponse,	/* Wait for response */
	void (*MsgFunc)(int code, char *msg))
{
	struct VolId vid;

	/*
	 *	 Convert specifier to volume identifier.
	 *	 Validate specifier.
	 */
	if (StrToVolId(volspec, &vid) != 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18207, volspec);
				/* "Volume specification error %s" */
		return (-1);
	}
	return (MoveCartridge(&vid, DestSlot, 0, MsgFunc));
}


/*
 *	sam_move() - deprecated API function to move
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		src_slot	Source slot number
 *		dest_slot	Destination slot number
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in moveing the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_move(
	ushort_t eq_ord,	/*	Equipment ordinal */
	int	 src_slot,	/*	Source slot number */
	int	 dest_slot,	/*	Destination slot number */
	int wait_response)	/*	Set nonzero to wait for response */
{
	struct VolId vid;

	vid.ViFlags = VI_cart;
	vid.ViEq = eq_ord;
	vid.ViSlot = src_slot;
	return (MoveCartridge(&vid, dest_slot, wait_response, NoMsgFunc));
}


/*
 * Message function for sam_move().
 * No messages are processed.
 */
/*ARGSUSED0*/
static void
NoMsgFunc(int code, char *msg)
{
}


/*
 *	Common move cartridge.
 */
static int
MoveCartridge(
	struct VolId *vid,
	int DestSlot,		/* Destination slot */
	int WaitResponse,	/* Wait for response */
	void (*MsgFunc)(int code, char *msg))
{
	struct CatalogEntry ced_s;
	struct CatalogEntry *ce = &ced_s;
	struct CatalogHdr *ch;
	dev_ent_t *dev;
	operator_t operator;	/* Data on operator */
	sam_cmd_fifo_t cmd_block;
	char volspec[STR_FROM_VOLID_BUF_SIZE];
	char *fifo_path;	/* Path to FIFO pipe, from shared memory */

	if (CatalogInit("SamApi") == -1) {
		LibError(MsgFunc, EXIT_FAILURE, 18211);
				/* "Catalog initialization failed." */
		return (-1);
	}
	if ((ce = CatalogCheckSlot(vid, &ced_s)) == NULL) {
		LibError(MsgFunc, EXIT_FAILURE, 18207,
		    StrFromVolId(vid, volspec, sizeof (volspec)));
				/* "Volume specification error %s" */
		return (-1);
	}
	ch = CatalogGetHeader(ce->CeEq);
	if (ch == NULL) {
		LibError(MsgFunc, EXIT_FAILURE, 18207,
		    StrFromVolId(vid, volspec, sizeof (volspec)));
				/* "Volume specification error %s" */
		return (-1);
	}
	if (ch->ChType != CH_library) {
		char dev_eq_no[21];

		(void) sprintf(dev_eq_no, "%u", (unsigned int) ce->CeEq);

		LibError(MsgFunc, EXIT_FAILURE, 874, dev_eq_no);
		/* "Device not a robot (%s)." */

		if (!errno) {
			errno = ER_ROBOT_DEVICE_REQUIRED;
		}

		return (-1);
	}

	/*
	 *	Check destination slot.
	 */
	if (DestSlot != vid->ViSlot) {
		int save_errno;
		struct CatalogEntry ced_d;
		struct CatalogEntry *ced = &ced_d;
		struct VolId vi_dst;

		vi_dst.ViFlags	= VI_cart;
		vi_dst.ViEq	= ce->CeEq;
		vi_dst.ViSlot	= DestSlot;

		/*
		 *	If destination slot is empty, then CatalogCheckSlot
		 *	will set errno to a non-zero value. However, the dest.
		 *	slot being empty is the DESIRED condition,
		 *	so if ced == NULL OR the slot is not in-use,
		 *	then restore errno.
		 */

		save_errno = errno;

		ced = CatalogCheckSlot(&vi_dst, &ced_d);
		if (ced != NULL && (ced->CeStatus & CES_inuse)) {
			LibError(MsgFunc, EXIT_FAILURE, 13631, DestSlot);
					/* Destination slot %d is occupied. */

			if (!errno) {
				errno = ER_DST_SLOT_IS_OCCUPIED;
			}

			return (-1);
		} else {
			errno = save_errno;
		}
	}

	/*
	 *	Get device entry and FIFO path from master shared memory segment
	 */
	if (sam_get_dev(ce->CeEq, &dev, &fifo_path, &operator) < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 13632, (unsigned int) ce->CeEq);
		return (-1);
	}

	/*
	 *	If operator does not have privilege to move media in robot,
	 *	return
	 */
	if (!SAM_OPER_SLOT(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		LibError(MsgFunc, EXIT_FAILURE, 16013);
		return (-1);
	}

	if (dev->type == DT_DLT2700) {
		errno = ER_ROBOT_NO_MOVE_SUPPORT;
		LibError(MsgFunc, EXIT_FAILURE, 16045);
		return (-1);
	}

	/*
	 *	Format and send commmand to sam-amld
	 */
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.magic =  CMD_FIFO_MAGIC;
	cmd_block.cmd =    CMD_FIFO_MOVE;
	cmd_block.eq =	ce->CeEq;
	cmd_block.slot =   ce->CeSlot;
	cmd_block.d_slot = DestSlot;
	if (sam_send_cmd(&cmd_block, WaitResponse, fifo_path) < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 16075);
		if (!errno) {
			errno = ER_DEVICE_OP_FAILED;
		}
		return (-1);
	}
	return (0);
}
