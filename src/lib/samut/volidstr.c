/*
 * volidstr.c - VolId structure string conversion functions.
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

#pragma ident "$Revision: 1.16 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "sam/lib.h"
#include "sam/lint.h"


/*
 * Parse string to get VolId elements.
 *
 * Return to user provided VolId structure.
 * The string is either:
 *
 * 1) equipment:slot:partition
 * OR
 * 2) mediatype.VSN
 *
 * mediatype must be non-numeric, equipment must be numeric.
 * "partition" is either optical side or tape partition, both numeric.
 *
 * Possibilities for 1) are:
 * equipment:slot:partition
 * equipment:slot
 *
 * returns -1 if error;  errno is set to an error code from sam_errno.h
 */
int
StrToVolId(
	char *arg,			/* String to convert */
	struct VolId *vid)		/* Resulting VolID structure */
{
	char *endptr;
	char *p;
	int eq;
	int slot;
	uint16_t part;

	if (arg == NULL || vid == NULL) {
		/* error - bad argument */
		errno = EINVAL;
		return (-1);
	}

	memset(vid, 0, sizeof (struct VolId));
	p = arg;
	eq = strtol(p, &endptr, 10);
	if (p == endptr) {

		/*
		 * Not numeric, assume mt.vsn.
		 */
		strncpy(vid->ViMtype, p, 2);
		vid->ViFlags |= VI_mtype;
		p += 2;
		if (*p++ != '.' || *p == '\0') {
			/* error - badly formed "mt::vsn" */
			errno = EINVAL;
			return (-1);
		}
		if (strlen(p) >= sizeof (vid->ViVsn)) {
			/* error - vsn too long */
			errno = EINVAL;
			return (-1);
		}
		strncpy(vid->ViVsn, p, sizeof (vid->ViVsn));
		vid->ViFlags |= VI_vsn;
		return (0);
	}

	if (eq < 0 || eq > MAX_DEVICES) {
		/* error - eq out of range */
		errno = EINVAL;
		return (-1);
	}
	vid->ViEq = (uint_t)eq;
	vid->ViFlags |= VI_eq;

	if (*endptr == ':') {
		p = endptr + 1;
		slot = strtol(p, &endptr, 10);
		if (p == endptr) {
			/* error - non-numeric */
			errno = EINVAL;
			return (-1);
		}
		if (slot < 0 || slot > MAX_SLOTS) {
			/* error - slot out of range */
			errno = ER_NOT_VALID_SLOT_NUMBER;
			return (-1);
		}
		vid->ViSlot = (uint_t)slot;
		vid->ViFlags |= VI_slot;
	}

	if (*endptr == ':') {
		p = endptr + 1;
		part = strtol(p, &endptr, 10);
		if (p == endptr) {
			/* error - non-numeric */
			errno = EINVAL;
			return (-1);
		}
		if ((int)part <= 0 || part > MAX_PARTITIONS) {
			/* error - partition out of range */
			errno = ER_NOT_VALID_PARTITION;
			return (-1);
		}
		vid->ViPart = part;
		vid->ViFlags |= VI_part;
	}
	if (*endptr != '\0') {
		/* error - non-numeric */
		errno = EINVAL;
		return (-1);
	}
	return (0);
}


/*
 * Construct string from media identifier(s).
 * Returns start of result string.
 *
 */
char *
StrFromVolId(
	struct VolId *vid,	/* Structure containing vid */
	char *buf,		/* pointer to user buffer */
	int buf_size)		/* Size of buffer */
{
	char	*p, *pe;

	if (buf_size == 0 || buf == NULL || vid == NULL)
		return (NULL);

	if (vid->ViFlags == VI_NONE) {
		*buf = '\0';
		return (buf);
	}

	/*
	 * Logical (media type/vsn) takes precedence.
	 */
	if (vid->ViFlags == VI_logical) {
		snprintf(buf, buf_size, "%s.%s", vid->ViMtype, vid->ViVsn);
		return (buf);
	}

	p = buf;
	pe = buf + buf_size;
	if (vid->ViFlags & VI_eq) {
		snprintf(p, Ptrdiff(pe, p), "%d", vid->ViEq);
		p += strlen(p);
	}
	if (vid->ViFlags & VI_slot) {
		snprintf(p, Ptrdiff(pe, p), ":%d", vid->ViSlot);
		p += strlen(p);
	}
	if (vid->ViFlags & VI_part) {
		snprintf(p, Ptrdiff(pe, p), ":%d", vid->ViPart);
	}
	return (buf);
}


#if defined(TEST)

int
main(
	int argc)
{
	char *test_strings[] = {
		"0:0:1",
		"0:0:0",
		"10:10:10",
		"9999:65000:37777",
		"1111:22",
		"600",
		":500:2",
		":33:0",
		":44:277",
		"9999:99:99",
		"lt.TAPE01",
		"sg.BANANA",
		"mo.Thisverylongopticallabelisthere",
		"d3.SKTAPE",
		"6666:666:66",
		".BADMED",
		"666.666",
		"-42:123:456",
		"100:foo:shouldnotbehere",
		"at.spaces in string",
		"mo.:",
		"wo.",
		"od. ",
		NULL
	};
	struct VolId vid;
	char buf[STR_FROM_ERRNO_BUF_SIZE];
	int i = 0;
	int ret;

	while (test_strings[i] != NULL) {
		memset(&vid, 0, sizeof (vid));
		printf("test case %d: %s\n", i, test_strings[i]);
		ret = StrToVolId(test_strings[i], &vid);
		if (ret != 0)
			printf("error in case %d: \"%s\" %s\n",
			    i, test_strings[i],
			    StrFromErrno(errno, buf, sizeof (buf)));
		printf("    f %x eq %d s %d q %d mt %s vsn %s\n",
		    vid.ViFlags, vid.ViEq, vid.ViSlot, vid.ViPart,
		    vid.ViMtype, vid.ViVsn);
		if (StrFromVolId(&vid, buf, sizeof (buf)) == NULL) {
			printf("    case %d conversion error\n", i);
		} else {
			printf("    result from case %d: %s\n", i, buf);
		}
		printf("\n");
		i++;
	}
}
#endif
