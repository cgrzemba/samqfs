/*
 * sam_chmed.c - Change media catalog entry  API
 *
 *	sam_chmed() modifies the catalog entry for the media specified
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

#pragma ident "$Revision: 1.26 $"


#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/defaults.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "aml/shm.h"
#include "aml/sam_utils.h"

#include "aml/samapi.h"
#include "pub/sam_errno.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Private data defintions */
#define	CATALOG_FLAGS	CMD_CATALOG_NEEDS_AUDIT | CMD_CATALOG_SLOT_INUSE | \
			CMD_CATALOG_LABELED | CMD_CATALOG_BAD_MEDIA | \
			CMD_CATALOG_SLOT_OCCUPIED | CMD_CATALOG_CLEANING | \
			CMD_CATALOG_BAR_CODE | CMD_CATALOG_WRITE_PROTECT | \
			CMD_CATALOG_READ_ONLY | CMD_CATALOG_RECYCLE | \
			CMD_CATALOG_UNAVAIL | CMD_CATALOG_EXPORT_SLOT | \
			CMD_CATALOG_VSN | CMD_CATALOG_STRANGE | \
			CMD_CATALOG_CAPACITY | CMD_CATALOG_SPACE | \
			CMD_CATALOG_TIME | CMD_CATALOG_COUNT | \
			CMD_CATALOG_PRIORITY | CMD_CATALOG_INFO


void sam_chmed_doit(struct CatalogEntry *ce, uint_t flags, int value);


/*
 *	sam_chmed() - API function to chmed flags for media catalog
 *
 *	Input parameters --
 *		eq_ord			Equipment ordinal
 *		ea			Element address in robot to be changed
 *					or ROBOT_NO_SLOT
 *		modifier		Side of optical
 *		media			Type of media with VSN
 *		vsn			Volume serial number to be changed
 *					or(char *) NULL
 *		flags			Bit fields representing flags to set
 *					or clear
 *		on_off			If 1, set the bits specified in flags
 *					If 0, clear the bits specified in flags
 *		wait_response		Wait for command response if nonzero
 *					-1	Wait forever
 *					> zero	Wait this many seconds
 *						for response
 *
 *	Output parameters --
 *		errno			Set to error number if error encountered
 *
 *	Return value --
 *		0			If successful in chmeding the media
 *		-1			If error encountered,
 *					'errno' set to error number
 *
 */

int
sam_chmed(
	ushort_t eq_ord,	/* Equipment ordinal */
	uint_t	ea,		/* Element address */
	int	modifier,	/* Side of optical */
	char	*media,		/* Type of media with VSN */
	char	*vsn,		/* Volume Serial Number */
	int	flags,		/* Flags representing fields */
				/* to be set/cleared */
	int	on_off,		/* Set flag bits if 1; */
				/* clear flag bits if zero */
	/* LINTED argument unused in function */
	int	wait_response)	/* Nonzero value to wait for response */
{

	struct CatalogEntry ced;
	struct CatalogEntry *ce;

	/*
	 *  Verify input arguments
	 */

	if (!((0 == on_off) || (1 == on_off))) {
		errno = ER_ON_OFF_BAD_VALUE;
		return (-1);
	}

	if (ea != ROBOT_NO_SLOT) {
		ce = CatalogGetCeByLoc(eq_ord, ea, modifier, &ced);
	}
	else
	{
		ce = CatalogGetCeByMedia(media, vsn, &ced);
	}

	if (ce == NULL) {
		errno = ER_SLOT_OR_VSN_REQUIRED;
		return (-1);
	}

	return (sam_chmed_value(ce, flags, (long long) on_off));
}


/*
 *  sam_chmed_value() - API function to chmed media catalog
 *
 *  Input parameters --
 *	ce		Catalog entry to be changed
 *	flags		Bit fields representing flags to set or clear
 *	value		If 1, set the bits specified in flags
 *			If 0, clear the bits specified in flags
 *			if value, pass along to robot process
 *	wait_response   Wait for command response if nonzero
 *			  -1  Wait forever
 *			> zero  Wait this many seconds for response
 *
 *  Output parameters --
 *	errno		Set to error number if error encountered
 *
 *  Return value --
 *	0		If successful in chmeding the media
 *	-1		If error encountered, 'errno' set to error number
 *
 */
int
sam_chmed_value(
	struct CatalogEntry *ce,
				/* Catalog entry to be changed */
	int flags,		/* Flags representing fields */
				/* to be set/cleared */
	long long value)	/* Value to be used by flag bits */
{
	operator_t operator;		/* Operator data */

	/*
	 *	Verify input arguments
	 */

	if (flags & ~(CATALOG_FLAGS)) {
		errno = ER_INVALID_FLAG_SET;
		return (-1);
	}

	/*
	 *	Check if operator has privilege for label
	 */

	SET_SAM_OPER_LEVEL(operator);

	if (!SAM_OPER_LABEL(operator)) {
		errno = ER_OPERATOR_NOT_PRIV;
		return (-1);
	}

	if (ce == NULL) {
		errno = ER_NO_DEVICE_FOUND;
		return (-1);
	}

	sam_chmed_doit(ce, flags, value);

	return (0);
}

void
sam_chmed_doit(struct CatalogEntry *ce, uint_t flags, int value)
{
	int state, field, mask;
	extern shm_alloc_t master_shm;	/* Master shared memory structure */

	mask = 0;
	if (flags & CMD_CATALOG_NEEDS_AUDIT) {
		mask |= CES_needs_audit;
	}

	if (flags & CMD_CATALOG_SLOT_INUSE) {
		mask |= CES_inuse;
	}

	if (flags & CMD_CATALOG_LABELED) {
		mask |= CES_labeled;
	}

	if (flags & CMD_CATALOG_BAD_MEDIA) {
		mask |= CES_bad_media;
	}

	if (flags & CMD_CATALOG_SLOT_OCCUPIED) {
		mask |= CES_occupied;
	}

	if (flags & CMD_CATALOG_CLEANING) {
		mask |= CES_cleaning;
	}

	if (flags & CMD_CATALOG_BAR_CODE) {
		mask |= CES_bar_code;
	}

	if (flags & CMD_CATALOG_WRITE_PROTECT) {
		mask |= CES_writeprotect;
	}

	if (flags & CMD_CATALOG_READ_ONLY) {
		mask |= CES_read_only;
	}

	if (flags & CMD_CATALOG_RECYCLE) {
		mask |= CES_recycle;
	}

	if (flags & CMD_CATALOG_UNAVAIL) {
		mask |= CES_unavail;
	}

	if (flags & CMD_CATALOG_EXPORT_SLOT) {
		mask |= CES_export_slot;
	}

	if (flags & CMD_CATALOG_STRANGE) {
		mask |= CES_non_sam;
	}

	if (flags & CMD_CATALOG_PRIORITY) {
		mask |= CES_priority;
	}

	/*
	 * At this point all the CEF_Status fields have been captured
	 * in 'mask' so set the status fields.
	 */
	field = CEF_Status;
	if (value) {
		state = flags;
	} else {
		state = 0;
	}
	(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
	    field, mask, state);

	/*
	 * Now handle other catalog fields not covered by CEF_Status.
	 */
	if (flags & CMD_CATALOG_CAPACITY) {
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, value, 0);
	}

	if (flags & CMD_CATALOG_SPACE) {
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, value, 0);
	}

	if (flags & CMD_CATALOG_TIME) {
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, value, 0);
	}

	if (flags & CMD_CATALOG_COUNT) {
		(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, value, 0);
	}

	if (flags & CMD_CATALOG_VSN) {
		(void) CatalogSetStringByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, (char *)value);
	}

	if (flags & CMD_CATALOG_INFO) {
		(void) CatalogSetStringByLoc(ce->CeEq, ce->CeSlot, ce->CePart,
		    flags, (char *)value);
	}

	shmdt((char *)master_shm.shared_memory);
}
