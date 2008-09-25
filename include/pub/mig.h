/*
 *  mig.h - Migration Toolkit
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
#ifndef _SAM_MIG_H
#define	_SAM_MIG_H

#ifdef sun
#pragma ident "$Revision: 1.17 $"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include "stat.h"

#ifdef  __cplusplus
extern "C" {
#endif

#if !defined(_SAM_TYPES_H)
typedef unsigned short media_t;
typedef unsigned short equ_t;
typedef char  vsn_t[32];
#endif /* !defined _SAM_TYPES_H */

/*
 * Media type is made up of the constant 0x8000 inclusively or'ed with
 * the ascii bit representation of the second character of the
 * acsii media type.  If the acsii media type is "za", then the
 * internal media type is 0x8041.  If its "z5", then the internal
 * media type is 0x8035.
 */

typedef struct {
	offset_t    offset;		   /* offset from beginning of file */
	offset_t    size;			/* size of file to stage in */
	long long   position;		 /* position from meta data */
	ino_t	inode;		    /* file system inode number */
	vsn_t	vsn;			/* vsn from meta data */
	equ_t	fseq;			/* sam-fs file system equment numner */
	media_t	media_type;		/* sam-fs internal media type */
	void	*tp_data;		 /* generic pointer for tp use */
}tp_stage_t;

#if !defined(_AML_DEVICE_H)
#define	DT_THIRD_PARTY  0x8000
#define	IS_THIRD_PARTY (t) ((t & 0xFF00) == DT_THIRD_PARTY)
#endif /* !defined _AML_DEVICE_H */


/* Function prototypes */

/*
 * The following are functions that must be provided by the third party
 * developer.
 */
int usam_mig_initialize(int);
int usam_mig_stage_file_req(tp_stage_t *);
int usam_mig_cancel_stage_req(tp_stage_t *);

/*
 * The following functions are provided by the SAM-FS third party
 * interface.
 */
int sam_mig_stage_error(tp_stage_t *, int);
int sam_mig_stage_file(tp_stage_t *);
int sam_mig_stage_write(tp_stage_t *, char *, int, offset_t);
int sam_mig_stage_end(tp_stage_t *, int);
char *sam_mig_mount_media(char *, char *);
int sam_mig_release_device(char *);
int sam_mig_open_device(const char *, int);
int sam_mig_close_device(int);

/*
 * The following functions are used to build the name space for the
 * third party files.
 */
int sam_mig_create_file(char *, struct sam_stat *);

/*
 * The following functions are used to set the rearchive flag
 */
int sam_mig_rearchive(char *, char **, char *);

#ifdef  __cplusplus
}
#endif

#endif /* _SAM_MIG_H */
