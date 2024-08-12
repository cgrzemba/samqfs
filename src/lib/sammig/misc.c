/*
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

#pragma ident "$Revision: 1.8 $"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <pthread.h>

#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/proto.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/lib.h"
#include "aml/sam_utils.h"

#if defined(lint)
#include "sam/lint.h"
#endif	/* defined(lint) */

static char *libname = "libsammig";

/* needed by macro DBG_LVL */
extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;

#define	SAM_MIG_TMP_DIR		"/usr/tmp"
#define	SAM_MIG_TMP_DIR_LEN	(sizeof (SAM_MIG_TMP_DIR) + 20)
#define	SAM_MIG_PREFIX_LEN	10

int
sam_mig_open_device(
	const char *path,
	int oflag)
{
	static char *funcname = "open_device";

	upath_t device;
	int fd;

	if (path == NULL) {
		return (-1);
	}

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG, "%s(%s) [t@%d] %s",
		    libname, funcname, pthread_self(), path);
	}

	memset(device, 0, sizeof (device));

	if (readlink(path, device, sizeof (device)) < 0) {
		sam_syslog(LOG_ERR, "%s(%s) readlink(%s) failed errno= %d",
		    libname, funcname, path, errno);
		return (-1);
	}

	fd = open(path, oflag);

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG,
		    "%s(%s) [t@%d] Open complete '%s' device= '%s' fd= %d",
		    libname, funcname, pthread_self(), path, device, fd);
	}

	return (fd);
}

int
sam_mig_close_device(
	int fd)
{
	int ret;
	static char *funcname = "close_device";

	if (fd < 0) {
		return (-1);
	}

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG,
		    "%s(%s) [t@%d] fd= %d",
		    libname, funcname, pthread_self(), fd);
	}

	ret = close(fd);

	if (ret < 0) {
		sam_syslog(LOG_ERR,
		    "%s(%s) [t@%d] Close failed '%s' fd= %d errno= %d",
		    libname, funcname, pthread_self(), fd, errno);
	}

	return (ret);
}
