/*
 *  chmed.c - utility to set/clear various catalog flags, assign VSNs
 *	for migration toolkit api.
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

#pragma ident "$Revision: 1.31 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/shm.h>

/* Solaris headers. */
#include <libgen.h>
#include <dlfcn.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/remote.h"
#include "sam/lint.h"

/* Private functions. */
static void usage(void);
static void usage_help(void);

/* Public data. */
char *program_name = "chmed";
shm_alloc_t master_shm;
shm_ptr_tbl_t *master_shm_ptr;

int
main(int argc, char **argv)
{
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct VolId vid;
	char	buf[256];
	char	*string;
	uint64_t value;
	uint32_t mask;
	int		field;
	int		status;

	if (CatalogInit(program_name) == -1) {
		error(EXIT_FAILURE, errno, GetCustMsg(18211));
				/* Catalog initialization failed */
	}

	argv++;
	argc--;
	if (argc < 2 || argc > 3) {
		if (argc == 1 && strcmp(*argv, "-flags") == 0) {
			usage_help();
		} else {
			usage();
		}
	}

	if (strcmp(*argv, "-capacity") == 0) {
		argc--;
		argv++;
		field = CEF_Capacity;
		if (StrToFsize(*argv, &value) == -1) {
			error(EXIT_USAGE, 0, GetCustMsg(13612), *argv);
		}
		value /= 1024;
	} else if (strcmp(*argv, "-count") == 0) {
		char	*final_char;

		argc--;
		argv++;
		field = CEF_Access;
		value = strtoll(*argv, &final_char, 0);
		if (final_char != NULL && *final_char != '\0') {
			error(0, 0, GetCustMsg(13614), *argv);
			usage();
		}
	} else if (strcmp(*argv, "-mtype") == 0) {
		argc--;
		argv++;
		field = CEF_MediaType;
		string = *argv;
	} else if (strcmp(*argv, "-space") == 0) {
		argc--;
		argv++;
		field = CEF_Space;
		if (StrToFsize(*argv, &value) == -1) {
			error(EXIT_USAGE, 0, GetCustMsg(13613), *argv);
		}
		value /= 1024;
	} else if (strcmp(*argv, "-time") == 0) {
		argc--;
		argv++;
		field = CEF_MountTime;
		value = StrToTime(*argv);
		if ((int64_t)value <= 0) {
		/* N.B. bad indentation here to meet cstyle requirements */
		fprintf(stderr,
		    "%s: Cannot convert date \"%s\".  Valid formats are:\n"
		    "            yyyy-mm-dd [hh:mm[:ss]]\n"
		    "            Month-name Day [4-digit-year] [hh:mm]\n"
		    "      -or-  Day Month-name [4-digit-year] [hh:mm]\n",
		    program_name, *argv);
		exit(EXIT_FAILURE);
		}
	} else if (strcmp(*argv, "-vsn") == 0) {
		argc--;
		argv++;
		field = CEF_Vsn;
		string = *argv;
	} else if (strcmp(*argv, "-I") == 0) {
		argc--;
		argv++;
		field = CEF_VolInfo;
		string = *argv;
	} else if (**argv == '-' || **argv == '+') {
		char	*flags;
		/* User specified "+-flags specifier" */

		field = CEF_Status;
		flags = *argv + 1;
		mask = 0;
		while (*flags != '\0') {
			switch (*flags++) {

			case 'A':	mask |= CES_needs_audit;	break;
			case 'l':	mask |= CES_labeled;		break;
			case 'E':	mask |= CES_bad_media;		break;
			case 'o':	mask |= CES_occupied;		break;
			case 'C':	mask |= CES_cleaning;		break;
			case 'b':	mask |= CES_bar_code;		break;
			case 'W':	mask |= CES_writeprotect;	break;
			case 'R':	mask |= CES_read_only;		break;
			case 'c':	mask |= CES_recycle;		break;
			case 'U':	mask |= CES_unavail;		break;
			case 'N':	mask |= CES_non_sam;		break;
			case 'p':	mask |= CES_priority;		break;
			case 'X':	mask |= CES_export_slot;	break;
			case 'd':	mask |= CES_dupvsn;		break;
			case 'f':	mask |= CES_archfull;		break;
			default:
				error(0, 0, GetCustMsg(13615), *argv);
				usage();
				break;
			}
		}
		if (**argv == '-')  value = 0;
		else  value = mask;
	} else {
		error(0, 0, GetCustMsg(13611), *argv);
		usage();
	}

	argc--;
	argv++;

	/*
	 * There must be only one argument left - the volume identifier.
	 */
	if (*argv == NULL) {		/* not enough */
		usage();
	} else if (argc != 1) {		/* too many   */
		error(0, 0, GetCustMsg(13600), *argv);
		usage();
	}

	/*
	 * Convert argument to volume identifier.
	 */
	if (StrToVolId(*argv, &vid) != 0) {
		error(EXIT_FAILURE, errno, GetCustMsg(13600), *argv);
	}

	/*
	 * Use CatalogGetEntry() to validate the specifier.
	 */
	if ((ce = CatalogGetEntry(&vid, &ced)) == NULL) {
		error(EXIT_FAILURE, errno, GetCustMsg(13600), *argv);
	}

	/*
	 * Make the required change.
	 */
	if ((field != CEF_Vsn) && (field != CEF_MediaType) &&
	    (field != CEF_VolInfo)) {

		if ((field != CEF_Capacity) && (field != CEF_Space))
			status = CatalogSetField(&vid, field,
			    (uint32_t)value, mask);
		else {
			status = CatalogSetField(&vid, field, value, mask);
			if (field == CEF_Capacity) {
				field = CEF_Status;
				value = mask = CES_capacity_set;
				(void) CatalogSetField(&vid, field,
				    (uint32_t)value, mask);
			}
		}
	} else {
		status = CatalogSetString(&vid, field, string);
	}
	if (status != 0) {
		error(EXIT_FAILURE, errno, GetCustMsg(13616), *argv);
	}

	/*
	 * Display results.
	 */
	if (vid.ViFlags == VI_cart) {
		/*
		 * Volume was specified as eq:slot[:partition]
		 * If tape, list partition 0
		 * If m.o., list partitions 1 and 2
		 */
		vid.ViFlags |= VI_part;
		for (vid.ViPart = 0; /* Terminated inside. */; vid.ViPart++) {
			if ((ce = CatalogGetEntry(&vid, &ced)) == NULL)
				break;
			printf("%s\n",
			    CatalogStrFromEntry(ce, buf, sizeof (buf)));
			vid.ViPart = ce->CePart;
		}
	} else if ((ce = CatalogGetEntry(&vid, &ced)) != NULL)  {
		printf("%s\n", CatalogStrFromEntry(ce, buf, sizeof (buf)));
	}
	return (EXIT_SUCCESS);
}


/*
 * Print usage message and exit.
 */
static void
usage(void)
{
	fprintf(stderr, GetCustMsg(13610), program_name, program_name);
	fprintf(stderr, "\n");
	exit(EXIT_USAGE);
}


/*
 * Print help and exit.
 */
static void
usage_help(void)
{
	fprintf(stderr, "Usage: %s +flags specifier    (set flags)\n",
	    program_name);
	fprintf(stderr, "       %s -flags specifier    (clear flags)\n",
	    program_name);
	fprintf(stderr, "       %s -capacity capacity specifier\n",
	    program_name);
	fprintf(stderr, "       %s -space space specifier\n",
	    program_name);
	fprintf(stderr, "       %s -count mount-count specifier\n",
	    program_name);
	fprintf(stderr, "       %s -time mount-time specifier\n",
	    program_name);
	fprintf(stderr, "       %s -vsn vsn specifier\n",
	    program_name);
	fprintf(stderr, "       %s -mtype media specifier\n",
	    program_name);
	fprintf(stderr,
	    "  where flags is one or more of the"
	    " (case-sensitive) \"flags\" as shown in the\n"
	    "  robot VSN catalog entries:\n\n"
	    "    A - slot needs audit\n"
	    "    C - slot contains cleaning cartridge\n"
	    "    E - volume is bad\n"
	    "    N - volume is not for SAM usage\n"
	    "    R - volume is read-only (software flag)\n"
	    "    U - volume is unavailable (historian only)\n"
	    "    W - volume is physically write-protected\n"
	    "    X - slot is an export slot\n"
	    "    b - volume has a bar code\n"
	    "    c - volume is scheduled for recycling\n"
	    "    d - volume has a duplicate vsn\n"
	    "    f - volume found full by archiver\n"
	    "    l - volume is labeled\n"
	    "    o - slot is occupied\n"
	    "    p - high priority medium\n"
	    "\n");

	fprintf(stderr, "  where specifier is one of:\n\n"
	    "    media.vsn   -or-\n"
	    "    eq:slot[:partition]\n");
	fprintf(stderr, "\n  Examples: %s -RW lt.TAPE0\n", program_name);
	fprintf(stderr, "\n            %s -space 102300 lt.TAPE0\n",
	    program_name);
	fprintf(stderr, "\n            %s -space 102300 30:12\n",
	    program_name);
	fprintf(stderr, "\n            %s -time \"Mar 23 10:15\" lt.TAPE0\n",
	    program_name);
	fprintf(stderr,
	    "\n            %s -time \"Nov 28 1991 10:15\" lt.TAPE0\n",
	    program_name);
	fprintf(stderr, "\n            %s -vsn mo OPT123 30:12:1\n",
	    program_name);
	exit(EXIT_SUCCESS);
}
