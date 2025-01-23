/*
 * shm.h - constants and structs for the shared memory segment.
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


#ifndef _AML_SHM_H
#define	_AML_SHM_H

#pragma ident "$Revision: 1.14 $"

#include <semaphore.h>
#include "aml/device.h"
#include "aml/preview.h"
#include "sam/types.h"
#include "license/license.h"

extern	void *sam_mastershm_attach(int shmget_priv, int shmat_priv);

#define	SHM_MASTER_KEY  22494	/* master shm key */
#define	SHM_PREVIEW_KEY 33194	/* preview shm key */
#define	SHM_CATALOG_KEY	55994	/* catalog shm key */

/*
 * because structs must be lined up on specific word boundaries,
 * leave some space at the end of the segment.
 */
#define	SEGMENT_EXCESS  (512 * 1)	/* excess bytes in segment */
#define	SAM_SEGMENT_NAME "samfs - shared memory segment"
#define	SAM_PREVIEW_NAME "samfs - preview memory segment"

/*
 * Macros for access "pointers" in the segment to other places in the
 * segment.  Since having a pointer to somewhere in the segment makes
 * no sense(since every process has a different base address) the
 * pointers in the segment are actually offsets from the beginning of
 * the segment.
 */

/* find the address pointed to by x in the shared memory segement a */
#define	SHM_ADDRESS(a, x) (((a) == NULL) ? NULL : ((char *)a + (long)(x)))

/* find the address pointed to by x if shm_alloc_t structure in use */
#define	SHM_REF_ADDR(x) (((x) == 0) ? \
	NULL : ((void *)((char *)master_shm.shared_memory + (size_t)(x))))

/* get address as an offset from the segment */
#define	SHM_GET_OFFS(x) (Ptrdiff((x), master_shm.shared_memory))

/*
 * Shared memory descriptor.  This structure should be created(global) by
 * every PROCESS that is going to access the main segment.  It needs to be
 * filled in with the id and the address of the segment for THIS PROCESS.
 * Any threads running on behalf of the process should have an extern
 * referencing this structure.  Use of the preceeding macros requires
 * that this structure be setup and that it be called master_shm.
 */
typedef struct {
	int	shmid;		/* shared memory id (-1 until */
				/* segment is created and locked) */
	void	*shared_memory;	/* pointer to the memory */
} shm_alloc_t;

/*
 * This struct is allocated at the beginning of the segment.  It contains
 * the master lock, size and allocation information of the segment.  The
 * mutex is initialize by sam-amld when the segment is created.  ANYONE
 * changing information in the struct WILL obtain the mutex before
 * and release the mutex after the change.  The lock will only be held
 * for the absolute minimum time.
 */
typedef struct {
	mutex_t		shm_mut;	/* mutex for locking the segment */
	uname_t		segment_name;	/* will contain the segment name */
					/* used for sanity checking */
	int		size;		/* allocated size of segment */
					/* set by sam-amld only. */
	int		left;		/* bytes left in segment */
	int		next_p;		/* offset to next unused area */
					/* this is relative to the start of */
					/* the segment.  So if the segment */
					/* was empty, then next_p would be */
					/* zero. */
} shm_block_def_t;


/*
 * The shm_ptr_tbl is allocated at the beginning of the segment.
 * It has the shm_block_definitation and offsets to the start
 * of the device table, and any other tables allocated in the segment.
 * These offsets are added to the baseaddress of the segment to find
 * the required table.
 *
 * Note: lic_time is updated every time the license thread in
 * sam-amld runs(about once every 60 seconds).  It is updated
 * WITHOUT any mutex locks.
 *
 * The debug field is updated and read WITHOUT any mutex locks.
 */
typedef struct {
	shm_block_def_t	shm_block;	/* shared memory control */
	pid_t		sam_amld;	/* process id of the sam-amld that */
					/* created this segment. */
	pid_t		scanner;	/* process id of the scanner */
	int		valid;		/* flag set when sam-amld initializes */
	time_t		lic_time;	/* last time license scan ran */
	sam_lic_value_33 license;	/* license value */
	sam_debug_t	debug;		/* debug flags */
	sem_t		notify_sem;	/* semaphore for notify program */
	union {				/* use the shm_mut mutex for setting */
		struct {		/* the bits in the flags area */
			uint_t
#if defined(_BIT_FIELDS_HTOL)
				rmtsamclnt: 1,	/* remote sam client */
				rmtsamsrvr: 1,	/* remote sam server */
				lic_init  : 1,	/* license manager */
						/* has initialized */
				unused	  :29;
#else	/* defined(_BIT_FIELDS_HTOL) */
				unused	  :29,
				lic_init  : 1,	/* license manager */
						/* has initialized */
				rmtsamsrvr: 1,	/* remote sam server */
				rmtsamclnt: 1;	/* remote sam client */
#endif	/* defined(_BIT_FIELDS_HTOL) */
			} b;
			uint_t	bits;
	} flags;
	int		fifo_path;	/* path to the fifo directory */
	int		dev_table;	/* device pointer table */
	int		first_dev;	/* first dev_ent entry */
	int		scan_mess;	/* message area for scanner */
	int		preview_shmid;	/* preview table shared memory id */
	int		robot_count;	/* number of robots */
	char		dis_mes[DIS_MES_TYPS][DIS_MES_LEN]; /* system message */
} shm_ptr_tbl_t;

/* For the flags.bits above */

#define	SHM_FLAGS_RMTSAMCLNT 0x80000000
#define	SHM_FLAGS_RMTSAMSRVR 0x40000000
#define	SHM_FLAGS_LICINIT    0x20000000

typedef struct {
	shm_block_def_t	shm_block;		/* shared memory control */
	preview_tbl_t	preview_table;		/* the preview table itself */
}shm_preview_tbl_t;

#endif /* _AML_SHM_H */
