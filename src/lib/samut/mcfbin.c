/*
 * mcfbin.c - write and read mcf binary file.
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

#pragma ident "$Revision: 1.12 $"

/* ANSI C headers. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/device.h"
#include "sam/lib.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Private data. */
static char *fname = SAM_VARIABLE_PATH"/mcf.bin";
static dev_ent_t dev;

/*
 * Read device file.
 * This function replaces the original read_mcf.  It has the same interface,
 * but reads the mcf binary file produced by WriteMcfbin(), which is
 * called by sam-fsd.
 */
int
read_mcf(
/* LINTED argument unused in function */
	char *dummy,		/* Was name of mcf */
	dev_ent_t **devlist,	/* Returned device list */
	int *high_eq)		/* Returned highest equipment ordinal */
{
	dev_ent_t *lp;
	size_t  size;
	int		fd;
	int		n;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		sam_syslog(LOG_ERR, "Cannot open %s %m", fname);
		exit(EXIT_FAILURE);
	}
	size = sizeof (dev_ent_t);
	lp = NULL;
	*high_eq = 0;
	for (n = -1; /* Terminated inside */; n++) {
		dev_ent_t *dp;

		if (read(fd, &dev, size) != size) {
			sam_syslog(LOG_ERR, "Cannot read %s %m", fname);
			exit(EXIT_FAILURE);
		}
		if (n == -1) {
			/*
			 * First record is a validation.
			 */
			if (strcmp(dev.name, fname) != 0) {
				sam_syslog(LOG_ERR, "mcf.bin read failure");
				exit(EXIT_FAILURE);
			}
			continue;
		}
		/*
		 * File is terminated with an empty name.
		 */
		if (*dev.name == '\0') {
			break;
		}

		/*
		 * Link entry.
		 */
		dp = (dev_ent_t *)malloc(size);
		memmove(dp, &dev, size);
		if (lp == NULL) {
			*devlist = dp;
		} else {
			lp->next = dp;
		}
		lp = dp;
		dp->next = NULL;
		if (dp->eq > *high_eq) {
			*high_eq = dp->eq;
		}
	}
	(void) close(fd);
	return (n);
}


/*
 * Write mcf binary file.
 */
void
WriteMcfbin(
	int DeviceNumof,
	dev_ent_t *DeviceTable)
{
	size_t	size;
	int		fd;
	int		i;

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		sam_syslog(LOG_ERR, "Cannot create %s %m", fname);
		exit(EXIT_FAILURE);
	}
	size = sizeof (dev_ent_t);

	/*
	 * Write Identification record.
	 */
	memset(&dev, 0, sizeof (dev));
	strncpy(dev.name, fname, sizeof (dev.name));
	if (write(fd, &dev, size) != size) {
		sam_syslog(LOG_ERR, "Cannot write %s %m", fname);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < DeviceNumof; i++) {
		dev_ent_t *dp;

		dp = &DeviceTable[i];
		if (write(fd, dp, size) != size) {
			sam_syslog(LOG_ERR, "Cannot write %s %m", fname);
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * Write terminating record.
	 */
	memset(&dev, 0, sizeof (dev));
	if (write(fd, &dev, size) != size) {
		sam_syslog(LOG_ERR, "Cannot write %s %m", fname);
		exit(EXIT_FAILURE);
	}
	if (close(fd) == -1) {
		sam_syslog(LOG_ERR, "Cannot close %s %m", fname);
		exit(EXIT_FAILURE);
	}
}
