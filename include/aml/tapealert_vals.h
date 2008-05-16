/*
 * tapealert_vals.h - defines for tapealert sysevent name value pairs
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
#ifndef _TAPEALERT_VALS_H
#define	_TAPEALERT_VALS_H

#pragma ident "$Revision: 1.10 $"

/* sysevent class, subclass, vendor, publisher */
#define	TAPEALERT_SE_CLASS "Device"    /* class */
#define	TAPEALERT_SE_SUBCLASS "TapeAlert"  /* subclass */
#define	TAPEALERT_SE_VENDOR "SUNW"    /* vendor */
#define	TAPEALERT_SE_PUBLISHER "SUNWsamfs" /* publisher */

/* sysevent attributes for log sense tapealert page 0x2e */
#ifdef DEBUG
#define	TAPEALERT_SRC_FILE "SRC_FILE"	/* string - source filename */
#define	TAPEALERT_SRC_LINE "SRC_LINE"	/* int32 - source linenumber */
#endif
#define	TAPEALERT_VENDOR "VENDOR"	/* string - inquiry vendor */
#define	TAPEALERT_PRODUCT "PRODUCT"	/* string - inquiry product */
#define	TAPEALERT_REV "REV"		/* string - inquiry revision */
#define	TAPEALERT_USN "USN"		/* string - inquiry unit serial num */
#define	TAPEALERT_TOD "TOD"		/* int32 - time of day */
#define	TAPEALERT_EQ_ORD "EQ_ORD"	/* int16 - mcf eq ordinal */
#define	TAPEALERT_NAME "NAME"		/* string - device name */
#define	TAPEALERT_VERSION "VERSION"	/* byte - inquiry version */
#define	TAPEALERT_INQ_TYPE "INQ_TYPE"	/* byte - inquiry peripheral dev type */
#define	TAPEALERT_VSN "VSN"		/* string - volume serial number */
#define	TAPEALERT_FLAGS_LEN "FLAGS_LEN"	/* int16 - tapealert flags count */
#define	TAPEALERT_FLAGS "FLAGS"		/* uint64 - 64 tapealert flags */
#define	TAPEALERT_SET "SET"		/* string - mcf family set name */
#define	TAPEALERT_FSEQ "FSEQ"		/* int16 - family set eq ord */

/* vsn or barcode empty string */
#define	TAPEALERT_EMPTY_STR "\"\""

#endif
