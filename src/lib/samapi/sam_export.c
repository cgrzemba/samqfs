/*
 * sam_export.c - export removable media API
 *
 *	sam_export() causes the media specified to be output from the removable
 *	media device.
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

#pragma ident "$Revision: 1.29 $"

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
static int ExportCartridge(struct VolId *vid, int wait_response, int OneStep,
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
	int			slot;
	int			wait_response;
	int			one_step;
	int			error;
	char		*vsn;

	if (argc < 3 || argc > 6) {
		fprintf(stderr, "Usage: %s eq_ord slot "
		    "[ wait_response ] [ one_step ] [ vsn ].\n",
		    argv[0]);
		exit(-70);
	}

	eq_ord		= (ushort_t)atoi(argv[1]);
	slot		= atoi(argv[2]);

	if (argc > 3) {
		wait_response = atoi(argv[3]);
	} else {
		wait_response = 0;
	}

	if (argc > 4) {
		one_step =  atoi(argv[4]);
	} else {
		one_step = 0;
	}

	if (argc > 5) {
		vsn = argv[5];
	} else {
		vsn = (char *)NULL;
	}

	error = sam_export(eq_ord, vsn, slot, wait_response, one_step);

	fprintf(stderr,
	    "Called sam_export to export %d:%d ", (int)eq_ord, slot);

	if (vsn) {
		fprintf(stderr, "vsn %s ", vsn);
	}

	if (wait_response) {
		fprintf(stderr, "wait-response enabled");
	} else {
		fprintf(stderr, "wait-response not enabled");
	}

	if (one_step) {
		fprintf(stderr, " one-step export");
	} else {
		fprintf(stderr, " export-to-historian");
	}

	fprintf(stderr, ".\n");

	fprintf(stderr, "sam_export returned the value %d.  "
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
 * SamExportCartridge - Export cartridge from a library.
 * Returns  0 if success, -1 if failure.
 *
 */
int
SamExportCartridge(
	char *volspec,		/* Volume specification eq:slot or mt.vsn */
	int WaitResponse,	/* Wait for response */
	int OneStep,		/* ACSLS one step export */
	void (*MsgFunc)(int code, char *msg))
{
	struct VolId vid;

	/*
	 * Convert specifier to volume identifier.
	 * Validate specifier.
	 */
	if (StrToVolId(volspec, &vid) != 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18207, volspec);
				/* "Volume specification error %s" */
		return (-1);
	}
	return (ExportCartridge(&vid, WaitResponse, OneStep, MsgFunc));
}


/*
 *	sam_export() - API function to export media
 *
 *	Input parameters --
 *		eq_ord		Equipment ordinal
 *		vsn		Volume serial number to be exported
 *				or(char *) NULL
 *		slot		Slot in robot to be exported
 *		wait_response	Wait for command response if nonzero
 *				-1	Wait forever
 *				> zero	Wait this many seconds for response
 *		OneStep		ACSLS one step export
 *				1 = one step export
 *				0 = export to historian only
 *
 *	Output parameters --
 *		errno		Set to error number if error encountered
 *
 *	Return value --
 *		0		If successful in exporting the media
 *		-1		If error encountered,
 *				'errno' set to error number
 *
 */

int
sam_export(
	ushort_t eq_ord,	/* Equipment ordinal */
	char	*vsn,		/* New Volume Serial Number */
	int 	slot,
	int	wait_response,
int one_step)
{
	struct VolId vid;

	if (vsn == NULL) {
		vid.ViFlags = VI_cart;
		vid.ViEq    = eq_ord;
		vid.ViSlot  = slot;
	} else {
		/*
		 * Convert specifier to volume identifier.
		 * Validate specifier.
		 */
		if (StrToVolId(vsn, &vid) != 0) {
			LibError(NoMsgFunc, EXIT_FAILURE, 18207, vsn);
					/* "Volume specification error %s" */
			return (-1);
		}
	}
	return (ExportCartridge(&vid, wait_response, one_step, NoMsgFunc));
}


/*
 * Message function for sam_export().
 * No messages are processed.
 */
/*ARGSUSED0*/
static void
NoMsgFunc(int code, char *msg)
{
}


/*
 * Common export cartridge.
 */
static int
ExportCartridge(
struct VolId *vid,
int WaitResponse,	/* Wait for response */
int OneStep,		/* ACSLS one step export */
void (*MsgFunc)(int code, char *msg))
{
	struct CatalogHdr *ch;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	dev_ent_t	*dev;
	operator_t	operator;	/* Data on operator */
	sam_cmd_fifo_t	cmd_block;
	char		volspec[STR_FROM_VOLID_BUF_SIZE];
	char		*fifo_path;
				/* Path to FIFO pipe, from shared memory */

	if (CatalogInit("SamApi") == -1) {
		LibError(MsgFunc, EXIT_FAILURE, 18211);
				/* "Catalog initialization failed." */
		return (-1);
	}
	if ((ce = CatalogCheckSlot(vid, &ced)) == NULL) {
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
	if (ch->ChType == CH_manual) {
		LibError(MsgFunc, EXIT_FAILURE, 18207, "Manual");
				/* "Volume specification error %s" */
		return (-1);
	}

	/*
	 * Get device entry and FIFO path from master shared memory segment
	 */
	if (sam_get_dev(ce->CeEq, &dev, &fifo_path, &operator) < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18213);
		return (-1);
	}

	/*
	 * If operator does not have privilege to move media in robot, return
	 */
	if (!SAM_OPER_SLOT(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		LibError(MsgFunc, EXIT_FAILURE, 18213);
		return (-1);
	}

	if (dev->state > DEV_IDLE) {
		if (CatalogExport(vid) == -1) {
			LibError(MsgFunc, EXIT_FAILURE, 18213);
			return (-1);
		}
	}

	/*
	 * Fail if this robot doesn't have a mailbox.
	 * (probably missing some types - Ampex, 3570, etc.)
	 */
	if (dev->equ_type == DT_DLT2700 || dev->equ_type == DT_METD28) {
		LibError(MsgFunc, EXIT_FAILURE, 18213);
		if (!errno) {
			errno = ER_ROBOT_NO_EXPORT_SUPPORT;
		}
		return (-1);
	}

	memset(&cmd_block, 0, sizeof (cmd_block));

	/*
	 * Must define cap to accept exported
	 * volume in the stk params file.
	 */
	if (OneStep) {
		if (dev->dt.rb.capid == ROBOT_NO_SLOT) {
			LibError(MsgFunc, 0, 16077);
			errno = ER_CAPID_NOT_DEFINED;
			return (-1);
		} else {
			cmd_block.flags = CMD_EXPORT_ONESTEP;
		}
	} else {
		cmd_block.flags = 0;
	}

	/*
	 * Format and send commmand to sam-amld
	 */
	cmd_block.magic =  CMD_FIFO_MAGIC;
	cmd_block.cmd =    CMD_FIFO_REMOVE_S;
	cmd_block.eq =	ce->CeEq;
	cmd_block.slot =   ce->CeSlot;

	if (sam_send_cmd(&cmd_block, WaitResponse, fifo_path) < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 16075);
		if (!errno) {
			errno = ER_DEVICE_OP_FAILED;
		}
		return (-1);
	}
	return (0);
}
