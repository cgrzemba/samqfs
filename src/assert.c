/*
 * assert.c - Perform assertions on SAM-fs definitions.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.25 $"


/* ----- UNIX Includes */

#include <stdio.h>
#include <sys/types.h>
#include <sys/flock.h>

/* ----- SAMFS Includes */

/* The following lines need to stay in this order. */
#include "pub/stat.h"
#include "pub/rminfo.h"
#define	pubMAX_ARCHIVE MAX_ARCHIVE
#undef MAX_ARCHIVE

/* These are duplicated in sys/stat.h */
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISFIFO
#undef S_ISGID
#undef S_ISREG
#undef S_ISUID
#undef S_ISLNK
#undef S_ISSOCK
#include <sys/stat.h>
#undef st_atime
#undef st_mtime
#undef st_ctime

#include "sam/types.h"
#include "sam/fioctl.h"
#include "sam/resource.h"
#include "sam/syscall.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/robots.h"

#include "fs/include/samfs.h"
#include "sam/fs/ino.h"
#include "sam/fs/amld.h"
#include "sam/fs/ino_ext.h"
#include "fs/include/sblk.h"
#include "fs/include/inode.h"

/* Private functions. */
static void Check_dev_status_t(void);

static int errors = 0;

#define	assert(a) \
	if (!(a)) {			\
		errors++;		\
		fprintf(stderr, "assertion on line %d (\"%s\") failed.\n", \
			__LINE__, #a);	\
	}
#define	PR(v)  fprintf(stderr, #v " = %d\n", (int)v)


int
main(int argc, char *argv[])
{
	assert(MAX_ARCHIVE == pubMAX_ARCHIVE);
	assert(MAX_ARCHIVE == 4);
	assert(SAM_ISIZE >= (sizeof (struct sam_inode_ext)));
	assert((AR_arch_i >> 2) == 1);
	assert(sizeof (vsn_t) == 32);
	assert((sizeof (struct sam_sblk) % SAM_DEV_BSIZE) == 0);
	assert(sizeof (struct sam_sblk) == (16 * SAM_DEV_BSIZE));
	assert(sizeof (sam_operation_t) == sizeof (void *));
	assert(sizeof (struct sam_perm_inode) ==
	    sizeof (struct sam_perm_inode_v1));
	/*
	 * Make sure sam_stat attributes don't overlap inode status.
	 * See include/sam/fs/ino.h and include/pub/stat.h
	 */
	assert((SAM_ATTR_MASK &
	    (SS_SAMFS | SS_ARCHIVE_R | SS_ARCHIVED | SS_ARCHIVE_A)) == 0);


#if defined(_BIT_FIELDS_HTOL)
	printf("bitfields assigned high to low\n");
#elif defined(_BIT_FIELDS_LTOH)
	printf("bitfields assigned low to high\n");
#else
#error  One of _BIT_FIELDS_HTOL or _BIT_FIELDS_LTOH must be defined
#endif  /* _BIT_FIELDS_HTOL */

	Check_dev_status_t();
	if (errors != 0)
		return (1);
	return (0);
}



/*
 * Check dev_status_t bits.
 */
static void
Check_dev_status_t(void)
{
	dev_status_t s;
	uint_t	*sp;

	sp = (uint_t *)&s;

	*sp = 0; s.maint = 1; assert(*sp == DVST_MAINT);
	*sp = 0; s.scan_err = 1; assert(*sp == DVST_SCAN_ERR);
	*sp = 0; s.audit = 1; assert(*sp == DVST_AUDIT);
	*sp = 0; s.attention = 1; assert(*sp == DVST_ATTENTION);

	*sp = 0; s.scanning = 1; assert(*sp == DVST_SCANNING);
	*sp = 0; s.mounted = 1; assert(*sp == DVST_MOUNTED);
	*sp = 0; s.scanned = 1; assert(*sp == DVST_SCANNED);
	*sp = 0; s.read_only = 1; assert(*sp == DVST_READ_ONLY);

	*sp = 0; s.labeled = 1; assert(*sp == DVST_LABELED);
	*sp = 0; s.wr_lock = 1; assert(*sp == DVST_WR_LOCK);
	*sp = 0; s.unload = 1; assert(*sp == DVST_UNLOAD);
	*sp = 0; s.requested = 1; assert(*sp == DVST_REQUESTED);

	*sp = 0; s.opened = 1; assert(*sp == DVST_OPENED);
	*sp = 0; s.ready = 1; assert(*sp == DVST_READY);
	*sp = 0; s.present = 1; assert(*sp == DVST_PRESENT);
	*sp = 0; s.bad_media = 1; assert(*sp == DVST_BAD_MEDIA);

	*sp = 0; s.stor_full = 1; assert(*sp == DVST_STOR_FULL);
	*sp = 0; s.i_e_port = 1; assert(*sp == DVST_I_E_PORT);

	*sp = 0; s.cleaning = 1; assert(*sp == DVST_CLEANING);

	*sp = 0; s.positioning = 1; assert(*sp == DVST_POSITION);
	*sp = 0; s.forward = 1; assert(*sp == DVST_FORWARD);
	*sp = 0; s.wait_idle = 1; assert(*sp == DVST_WAIT_IDLE);
	*sp = 0; s.fs_active = 1; assert(*sp == DVST_FS_ACTIVE);

	*sp = 0; s.write_protect = 1; assert(*sp == DVST_WRITE_PROTECT);
	*sp = 0; s.strange = 1; assert(*sp == DVST_STRANGE);
}
