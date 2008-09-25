/*
 * stager_defs.h - stager daemon internal definitions
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

#ifndef _AML_STAGER_DEFS_H
#define	_AML_STAGER_DEFS_H

#pragma ident "$Revision: 1.27 $"

#include "sam/types.h"
#include "aml/device.h"
#include "sam/resource.h"
#include "pub/mig.h"

/*
 * Volume section.
 * Each archive copy contains a volume information.
 */
typedef struct SectionInfo {
	u_longlong_t	position;	/* position of archived file */
	ulong_t		offset;		/* offset from beginning of */
					/*    archived file */
	u_longlong_t	length;		/* length of section on this VSN */
	vsn_t		vsn;
} SectionInfo_t;

/*
 * Archive copy information.
 */
typedef struct ArcopyInfo {
	media_t		media;		/* media type */
	int		n_vsns;		/* num of VSNs in use for this copy */
	ushort_t	flags;		/* copy flags */
	int		ext_ord;	/* extended multivolume ordinal */
	struct sam_vsn_section section;	/* only one vsn record saved here */
} ArcopyInfo_t;

/*
 * File information.
 * Information about file that is to be staged.
 * Its important that this structure use the minimal amount of
 * memory as possible.
 */

#define	STAGER_REQUEST_MAGIC	0x53746752	/* "StgR" */

typedef struct FileInfo {
	sam_id_t	id;		/* file identification */
	equ_t		fseq;		/* family set equipment number */

	ArcopyInfo_t	ar[MAX_ARCHIVE];

	u_longlong_t	len;		/* length of staging request */
	u_longlong_t	offset;		/* offset of staging request */
	u_longlong_t	stage_size;	/* size of staged bytes */
	int		copy;		/* stage from this archive copy */
	int		directio;	/* directio = 1, pagedio = 0 */

	uint_t		flags;
	int		error;		/* errno if staging error */
	short		retry;		/* retry counter for error recovery */
	short		namelen;	/* length of file name */
	int		se_ord;		/* section ordinal */

	uchar_t		csum_algo;	/* checksum algorithm */

	pid_t		pid;		/* pid of requestor */
	uid_t		user;		/* uid of requestor */
	u_longlong_t	read;		/* amount of data staged */
	equ_t		eq;		/* staged drive number */

	uid_t		owner;		/* owner's user id */
	gid_t		group;		/* owner's group id */

	sam_stage_filesys_t	fs;	/* persistent filesystem data */

	/*
	 * Data maintained for an opened disk cache file across
	 * mounts during multivolume staging or copy retry error
	 * recovery.
	 */
	int 		dcache;		/* open file desc for disk cache file */
	offset_t	write_off;	/* write offset for disk cache file */
	int		vsn_cnt;	/* count of VSNs read */
	pid_t		context;	/* copy process with open disk */
					/*    cache file */
	csum_t		csum_val;	/* checksum value accumulator */

	tp_stage_t	*migfile;	/* pointer to thirdparty request */
	pthread_mutex_t	mutex;
	int		sort;		/* own index for resolving post-sort */
	int		next;		/* index to next file in stage stream */
	int32_t		magic;		/* magic for validation "StgR" */
} FileInfo_t;

/*
 * Information about a VSN.
 * Each VSN is referenced by this structure.
 */
typedef struct VsnInfo {
	ushort_t	flags;
	media_t		media;		/* type of media */
	vsn_t		vsn;		/* VSN label */
	int		lib;		/* library table index */
	struct CatalogEntry *ce;	/* catalog entry for VSN */
	int		disk_idx; 	/* index for disk volume entry */
	int		drive;		/* if loaded, drive table index */
} VsnInfo_t;

/*
 * Stream priority flags.
 */
enum StreamPriority {
	SP_noresources,		/* no resources available */
	SP_nofilesys,		/* filesystem not mounted */
	SP_busy,		/* resources busy */
	SP_start,		/* start a copy stream */
	SP_idle			/* staging idled by operator */
};

/*
 * Stream status flags.
 */
enum {
	SR_ACTIVE =		1 << 0,		/* staging is active */
	SR_DONE =		1 << 1,		/* staging has completed */
	SR_LOADING =		1 << 2,		/* loading vsn */
	SR_WAIT =		1 << 3,		/* waiting load by operator */
	SR_WAITDONE =		1 << 4,		/* load by operator done */
	SR_ERROR =		1 << 5,		/* unable to mount vsn */
	SR_CLEAR =		1 << 6,		/* clearing stage requests */
	SR_UNAVAIL =		1 << 7,		/* media unavailable */
	SR_DCACHE_CLOSE	=	1 << 8,		/* close file descriptor */
	SR_full	=		1 << 9		/* met full stream params */
};

/*
 * Stream parameter flags.
 */
enum {
	SP_maxsize =		1 << 0,		/* use max size */
	SP_maxcount	=	1 << 1,		/* use max count */
};

/*
 * Stream information.
 * Information about a group of files to be staged together.
 */
typedef struct StreamInfo {
	size_t		count;		/* number of request files in stream */
	offset_t	size;		/* size of stream */
	time_t		create;		/* time stream req was created */
	VsnInfo_t	vi;		/* VSN information */
	vsn_t		vsn;		/* VSN label */
	int		lib;		/* library identifier */
	media_t		media;		/* type of media */
	boolean_t	diskarch;	/* disk archiving */
	boolean_t	thirdparty;	/* third party media */
	int		seqnum;		/* sequence number */
	pid_t		context;	/* copy process with open disk */
					/*    cache file */
	pid_t		pid;		/* if active, pid of copy proc */
	ushort_t	flags;
	pthread_mutex_t	mutex;		/* protect access to data */
	enum StreamPriority priority;	/* scheduling priority for stream */
	struct StreamInfo *next;	/* link to next stream entry in workq */
	int		first;		/* start of request file indices */
	int		last;		/* last of request file indices */
	int		error;		/* error for all files in stream */
	/*
	 * Stream parameters.
	 */
	struct {
		ushort_t sr_flags;
		size_t   sr_maxCount;	/* max files in stream */
		offset_t sr_maxSize;	/* max size of stream */
	} sr_params;
	/*
	 * Fields used for canceling load exported vol.
	 */
	pthread_t	ldtid;		/* thread id of loading exported vol. */
	int		ldfd;		/* fd for loading exported vol. */
	void		*ldarg;		/* argument for loadExportedVol */
	char		*rmPath;
} StreamInfo_t;

/*
 * Migration file information.
 * Information about file that is to be staged from third party media.
 * IMPORTANT: the stage request (tp_stage_t) MUST be declared as
 * the first entry in this structure.
 */
typedef struct MigFileInfo {
	tp_stage_t	req;		/* request for migration toolkit */
					/*    library */
	StreamInfo_t	*stream;	/* stream to which this file belongs */
	FileInfo_t	*file;		/* information about file to be */
					/*    staged */
	void		*dev;	/* device entry */
} MigFileInfo_t;

#endif /* _AML_STAGER_DEFS_H */
