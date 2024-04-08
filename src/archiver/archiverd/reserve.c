/*
 * reserve.c - Process reserving VSNs.
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

#pragma ident "$Revision: 1.22 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#define	MAX_TRIALS 10

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Posix headers. */
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "common.h"
#include "archset.h"

#if defined(lint)
#undef fputs
#endif /* defined(lint) */

/* Macros. */
#define	LINE_LENGTH 256

/* Private data. */
static FILE *old_st;
static char	msgarg[LINE_LENGTH];
static int	msgnum;

/* Private functions. */
static int processLine(char *lineBuf);


/*
 * Read the reserved VSNs file.
 */
void
ReadReservedVsns(void)
{
	char	bakName[sizeof (ScrPath)];
	char	fname[sizeof (ScrPath)];
	char	lineBuf[LINE_LENGTH];
	int	lineno;
	int	trials;
	int	warnings;

	/*
	 * Open the file to read.
	 */
	snprintf(fname, sizeof (fname), ARCHIVER_DIR"/"RESERVED_VSNS);
	if ((old_st = fopen(fname, "r")) == NULL) {
		return;
	}
	Trace(TR_MISC, "Reading Reserved VSNs file %s", fname);

	/*
	 * Read the line and prepare it for translation.
	 */
	lineno = 0;
	warnings = 0;
	while (fgets(lineBuf, sizeof (lineBuf)-1, old_st) != NULL) {
		lineno++;
		lineBuf[strlen(lineBuf)-1] = '\0';
		msgnum = 0;
		*msgarg = '\0';
		if (processLine(lineBuf) != 0) {
			warnings++;
			Trace(TR_MISC, GetCustMsg(4034), lineno,
			    GetCustMsg(msgnum), msgarg);
		}
	}

	/*
	 * End of reading file.
	 */
	(void) fclose(old_st);
	if (warnings == 0) {
		return;
	}

	/*
	 * Rename the ReservedVSNs file.
	 * Start with the name with 351bak appended.
	 * If this file exists, then try a series with name.351bak.nums
	 */
	snprintf(bakName, sizeof (bakName), "%s.351bak", fname);
	for (trials = 1; trials < MAX_TRIALS; trials++) {
		struct stat st;
		int	ret;

		ret = stat(bakName, &st);
		if (ret == -1 && errno == ENOENT) {
			break;
		}
		snprintf(bakName, sizeof (bakName), "%s.351bak.%d",
		    fname, trials);
	}
	if (trials >= MAX_TRIALS) {
		Trace(TR_ERR, "Cannot create backup for ReservedVSNs file %s",
		    fname);
		warnings++;
	} else {
		if (rename(fname, bakName) == -1) {
			Trace(TR_ERR, "Cannot rename %s to %s", fname, bakName);
		} else {
			SendCustMsg(HERE, 4041, bakName);
		}
	}


	/*
	 * Log the warning summary.
	 */
	if (warnings != 1) {
		SendCustMsg(HERE, 4035, warnings, fname);
	} else {
		SendCustMsg(HERE, 4036, fname);
	}
	/*
	 * Tell how to find the warning messages.
	 */
	SendCustMsg(HERE, 4037, "TraceFile");
}



/* Private functions. */


/*
 * Process line from reserved VSNs file.
 * Ignore blank lines and comments.
 * Return status to keep, discard, or comment out line.
 */
static int
processLine(
	char *lineBuf)
{
	struct VolId vid;
	struct tm tm;
	time_t	rtime;
	char	tokBuf[256];
	char	timestr[32];
	char	*asname;
	char	*owner;
	char	*fsname;
	char	*mtype;
	char	*token;
	char	*vsn;

	strncpy(tokBuf, lineBuf, sizeof (tokBuf)-1);
	mtype = strtok(tokBuf, " ");
	if (mtype == NULL || *mtype == '#') {
		return (0);
	}

	/*
	 * Look up the media.
	 */
	if (MediaParamsGetEntry(mtype) == NULL) {
		msgnum = 4431;
		return (-1);
	}

	/*
	 * Isolate the VSN.
	 */
	vsn = strtok(NULL, " ");
	if (vsn == NULL) {
		msgnum = 4453;
		return (-1);
	}

	/*
	 * Isolate the archive set name, owner, and file system.
	 */
	asname = strtok(NULL, " ");
	if (asname == NULL) {
		msgnum = 4486;
		return (-1);
	}
	if ((owner = strchr(asname, '/')) == NULL) {
		msgnum = 4495;
		return (-1);
	}
	*owner++ = '\0';
	if ((fsname = strchr(owner, '/')) == NULL) {
		msgnum = 4496;
		return (-1);
	}
	*fsname++ = '\0';

	/*
	 * Extract time.
	 */
	token = strtok(NULL, " ");
	if (token == NULL) {
		msgnum = 4514;
		return (-1);
	}
	strncpy(timestr, token, 11);
	strncat(timestr, " ", sizeof (timestr)-1);
	token = strtok(NULL, " ");
	if (token == NULL) {
		msgnum = 4514;
		return (-1);
	}
	(void) strncat(timestr, token, sizeof (timestr));
	rtime = 0;
	if (strptime(timestr, "%Y/%m/%d %H:%M:%S", &tm) != NULL) {
		tm.tm_isdst = -1;	/* Let mktime() determine DST */
		rtime = mktime(&tm);
	}
	if (rtime <= 0) {
		/*
		 * Invalid time.
		 */
		msgnum = 4408;
		strncpy(msgarg, timestr, sizeof (msgarg)-1);
		return (-1);
	}

	/*
	 * Make reservation information.
	 */
	strncpy(vid.ViMtype, mtype, sizeof (vid.ViMtype));
	strncpy(vid.ViVsn, vsn, sizeof (vid.ViVsn));
	vid.ViFlags = VI_logical;
	if (CatalogReserveVolume(&vid, 0, asname, owner, fsname) == -1) {
		if (errno == ER_VOLUME_ALREADY_RESERVED) {
			/*
			 * Volume already reserved.
			 */
			msgnum = 4407;
			strncpy(msgarg, asname, sizeof (msgarg)-1);
			return (-1);
		} else {
			Trace(TR_ERR, "ReserveVolume failed(%s.%s)",
			    mtype, vsn);
			return (0);
		}
	}
	Trace(TR_MISC, "Volume %s.%s reserved to %s/%s/%s",
	    vid.ViMtype, vid.ViVsn, asname, owner, fsname);
	return (0);
}
