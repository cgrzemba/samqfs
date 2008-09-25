/*
 * ----- sam_attributes.h - SAM-FS file system file attribute definitions.
 *
 *	Description:
 *	    Defines file attributes and structure for the SAM file
 *	    system. This structures resides in the file and directory
 *	    inode. It is also in the .user and .group attributes files.
 *	    There is 1 copy defined for global attributes.
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
#ifndef	_SAM_FS_ATTRIBUTES_H
#define	_SAM_FS_ATTRIBUTES_H

#ifdef sun
#pragma ident "$Revision: 1.15 $"
#endif

#include	<sam/fs/ino.h>

typedef struct	sam_file_attr	{
	ushort_t				/* Change flags: */
#if defined(_BIT_FIELDS_HTOL)
		c_damaged	: 1,	/* Set/clear damaged flag */
		c_direct	: 1,	/* Enable/disable direct access */
		c_nodrop	: 1,	/* Enable/disable nodrop status */
		c_noarch	: 1,	/* Enable/disable noarchive status */

		c_stageall	: 1,	/* Enable/disable stageall */
		c_migrate	: 1,	/* Enable/disable immediate archive */
		c_inherit	: 1,	/* Change directory inherit attr mode */
		c_release	: 1,	/* Change directory release attr mode */

		c_bof_online	: 1,	/* Change beg. of file left on-line */
					/* status */
		c_cs_use	: 1,	/* Change whether checksum is used */
		c_cs_gen	: 1,	/* Change whether checksum is */
					/* generated */
		c_cs_val	: 1,	/* Change whether checksum is valid */

		unused		: 4;
#else	/* defined(_BIT_FIELDS_HTOL) */
		unused		: 4,

		c_cs_val	: 1,	/* Change whether checksum is valid */
		c_cs_gen	: 1,	/* Change whether checksum is */
					/* generated */
		c_cs_use	: 1,	/* Change whether checksum is used */
		c_bof_online	: 1,	/* Change beg. of file left on-line */
					/* status */

		c_release	: 1,	/* Change directory release attr mode */
		c_inherit	: 1,	/* Change directory inherit attr mode */
		c_migrate	: 1,	/* Enable/disable immediate archive */
		c_stageall	: 1,	/* Enable/disable stageall */

		c_noarch	: 1,	/* Enable/disable noarchive status */
		c_nodrop	: 1,	/* Enable/disable nodrop status */
		c_direct	: 1,	/* Enable/disable direct access */
		c_damaged	: 1;	/* Set/clear damaged flag */
#endif	/* defined(_BIT_FIELDS_HTOL) */

	uchar_t		attr;		/* Attribute mode */
	ino_st_t	status;		/*  File status flags--see fs/ino.h */
} sam_file_attr_t;

#endif	/* _SAM_FS_ATTRIBUTES_H */
