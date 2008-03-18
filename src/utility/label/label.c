/*
 * label.c - label removable media.
 *
 *	label causes a new label to be written to the specified removable media
 *	device.
 *
 *	Note:  label is an executable that is copied to the executables
 *	odlabel and tplabel.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	MAIN

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/param.h>
#include <libgen.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/device.h"
#include "sam/exit.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/exit_fifo.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/lint.h"

/* Private data. */
static dtype_t  type;		/* Requested media type */

/* Private functions. */
static int	is_ansi_tp_label(char *s, size_t size);
static void	pr_usage(void);

/* Public data. */
char   *program_name;	/* Program name: used by error */


int
main(int argc, char *argv[])
{
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	struct CatalogHdr *ch;
	struct VolId    vid;

	sam_cmd_fifo_t  cmd_block;	/* Command block */
	char    cmd_fifo[MAXPATHLEN];
	char   *new_vsn = NULL;		/* New VSN to be written */
	char   *old_vsn = NULL;		/* Old VSN for verification */
	char   *uinfo = NULL;		/* User information */
	int	erase = FALSE;		/* Erase media before labeling */
	int	fifo_fd;		/* Command FIFO descriptor */
	int	new_label = FALSE;	/* Blank media label flag */
	int	robot = FALSE;		/* Robotic slot label */
	int	verbose = 0;		/* Display labels */
	int	wait_response = 0;
	uint_t  block_size = 0;

	program_name = basename(argv[0]);
	if (strcmp(program_name, "tplabel") != 0) {
		type = DT_OPTICAL;
	} else {
		type = DT_TAPE;
	}
	CustmsgInit(0, NULL);
	if (geteuid() != 0) {
		printf(catgets(catfd, SET, 100,
		    "%s may be run only by super-user.\n"),
		    program_name);
		exit(1);
	}
	if (argc < 2) {
		pr_usage();
		exit(EXIT_USAGE);
	}
	if (CatalogInit(program_name) == -1) {
		error(EXIT_FAILURE, errno, catgets(catfd, SET, 18211,
		    "Catalog initialization failed!"));
	}
	/* Process arguments. */


	while (--argc > 0 && **++argv == '-') {
		char	   *endptr;

		if (strcmp(*argv, "-new") == 0) {
			new_label = TRUE;
			continue;
		}
		if (strcmp(*argv, "-vsn") == 0) {
			new_vsn = *++argv;
			--argc;
			continue;
		}
		if (strcmp(*argv, "-old") == 0) {
			old_vsn = *++argv;
			--argc;
			new_label = FALSE;
			continue;
		}
		if ((strcmp(*argv, "-V") == 0) || (strcmp(*argv, "-v") == 0)) {
			verbose = 1;
			continue;
		}
		if ((strcmp(*argv, "-W") == 0) || (strcmp(*argv, "-w") == 0)) {
			wait_response = 1;
			continue;
		}
		if (strcmp(*argv, "-erase") == 0) {
			erase = TRUE;
			continue;
		}
		if (type == DT_TAPE) {
			if (strcmp(*argv, "-b") == 0) {
				block_size =
				    (uint_t)strtol(*++argv, &endptr, 10);
				if (*endptr != '\0') {
					error(EXIT_USAGE, 0,
					    catgets(catfd, SET, 2798,
					    "Non-numeric blocksize %s."),
					    *argv);
				}
				--argc;
				continue;
			}
		}
		if (type == DT_OPTICAL) {
			if (strcmp(*argv, "-info") == 0) {
				uinfo = *++argv;
				--argc;
				continue;
			}
		} else {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 2799,
			    "Unrecognized argument %s."), *argv);
		}
	}

	/*
	 * Check arguments.
	 */
	if (argc > 1) {
		error(EXIT_USAGE, 0,
		    catgets(catfd, SET, 2799,
		    "Unrecognized argument %s."), *argv);
	}
	if (argc <= 0) {
		error(EXIT_USAGE, 0, catgets(catfd, SET, 18212,
		    "Volume specification missing."));
	}
	if (new_vsn == NULL) {
		error(EXIT_USAGE, 0, catgets(catfd, SET, 2885,
		    "VSN not specified."));
	}
	if (!new_label && old_vsn == NULL) {
		error(EXIT_USAGE, 0,
		    catgets(catfd, SET, 1085,
		    "Existing VSN not specified for relabel."));
	}
	/*
	 * Convert specifier to volume identifier.
	 */
	if (StrToVolId(*argv, &vid) != 0) {
		error(EXIT_FAILURE, errno, catgets(catfd, SET, 18207,
		    "Volume specification error %s"), *argv);
	}
	/*
	 * Use CatalogGetEntry() to validate the specifier.
	 */
	if ((ce = CatalogCheckVolId(&vid, &ced)) == NULL) {
		error(EXIT_FAILURE, errno, catgets(catfd, SET, 18207,
		    "Volume specification error %s"), *argv);
	}
	ch = CatalogGetHeader(ce->CeEq);
	if (ch == NULL) {
		error(EXIT_FAILURE, errno, catgets(catfd, SET, 18207,
		    "Volume specification error %s"), *argv);
	}
	if (ch->ChType == CH_historian) {
		error(EXIT_FAILURE, 0, catgets(catfd, SET, 13620,
		    "Cannot label volume in historian."));
	}
	switch (type) {
	case DT_TAPE: {
	/* N.B. Bad indentation to meet cstyle requirements */
	if (block_size != 0) {
	if (block_size != 16 && block_size != 32 && block_size != 64 &&
	    block_size != 128 && block_size != 256 && block_size != 512 &&
	    block_size != 1024 && block_size != 2048) {
		printf(catgets(catfd, SET, 533,
		    "Blocksize must be one of 16, 32, 64, 128, 256, "
		    "512, 1024 or 2048."));
		exit(1);
	}
	block_size <<= 10;
	}
	if ((int)strlen(new_vsn) > 6) {
		error(EXIT_USAGE, 0,
		    catgets(catfd, SET, 2882,
		    "VSN must be 6 characters or less."));
	}
	if (is_ansi_tp_label(new_vsn, 6) != 1) {
		error(EXIT_USAGE, 0,
		    catgets(catfd, SET, 1406,
		    "Invalid characters in VSN."));
	}
	if (old_vsn != NULL) {
		if ((int)strlen(old_vsn) > 6) {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 1844,
			    "Old VSN must be 6 characters or less."));
		}
		if (is_ansi_tp_label(old_vsn, 6) != 1) {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 1405,
			    "Invalid characters in old VSN."));
		}
	}
	}
	break;

	case DT_OPTICAL:
		if ((int)strlen(new_vsn) > 31) {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 2881,
			    "VSN must be 31 characters or less."));
		}
		if (old_vsn != NULL && (int)strlen(old_vsn) > 31) {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 1843,
			    "Old VSN must be 31 characters or less"));
		}
		if (uinfo != NULL && (int)strlen(uinfo) > 127) {
			error(EXIT_USAGE, 0,
			    catgets(catfd, SET, 2836,
			    "User information must be 127 characters "
			    "or less."));
		}
		break;

	default:
		error(EXIT_USAGE, 0, catgets(catfd, SET, 864,
		    "Device cannot be labeled."));
	}

	if (!new_label) {
		if (strcmp(old_vsn, ce->CeVsn) != 0) {
			error(EXIT_USAGE, 0, catgets(catfd, SET, 1840,
			    "Old VSN %s does not match media VSN %s."),
			    old_vsn, ce->CeVsn);
		}
	}
	/* Issue label command */

	memset(&cmd_block, 0, sizeof (sam_cmd_fifo_t));
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = CMD_FIFO_LABEL;
	cmd_block.block_size = block_size;
	cmd_block.flags = 0;
	if (old_vsn != NULL) {
		strcpy(cmd_block.old_vsn, old_vsn);
	}
	strcpy(cmd_block.vsn, new_vsn);
	cmd_block.eq = ce->CeEq;
	cmd_block.media = sam_atomedia(ce->CeMtype);
	if (erase) {
		cmd_block.flags |= CMD_LABEL_ERASE;
	}
	if (!new_label) {
		cmd_block.flags |= CMD_LABEL_RELABEL;
	}
	if (ch->ChType == CH_library)
		cmd_block.flags |= CMD_LABEL_SLOT;
	cmd_block.slot = ce->CeSlot;
	cmd_block.part = ce->CePart;
	if (uinfo != NULL) {
		strcpy(cmd_block.info, uinfo);
	}
	if (verbose) {
		printf("eq:   %d\n", cmd_block.eq);
		printf("vsn:  %s\n", cmd_block.vsn);
		if (*ce->CeVsn != '\0') {
			printf("was:  %s\n", ce->CeVsn);
		}
		if (uinfo != NULL) {
			printf("info: %s\n", cmd_block.info);
		}
		if (robot) {
			printf("MID: %d\n", cmd_block.slot);
		}
	}
	if (wait_response) {
		set_exit_id(0, &cmd_block.exit_id);
		if (create_exit_FIFO(&cmd_block.exit_id) < 0)
			error(EXIT_FAILURE, errno,
			    catgets(catfd, SET, 556,
			    "Cannot create response FIFO"));
	} else
		cmd_block.exit_id.pid = 0;

	strcpy(cmd_fifo, SAM_FIFO_PATH "/" CMD_FIFO_NAME);
	if ((fifo_fd = open(cmd_fifo, O_WRONLY)) < 0) {
		error(EXIT_FAILURE, errno, catgets(catfd, SET, 1113,
		    "Unable to open command fifo:"));
	}
	write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);

	if (wait_response) {
		char	    resp_string[256];
		int		completion;

		if (read_server_exit_string(&cmd_block.exit_id,
		    &completion, resp_string, 255, -1) < 0) {
			int		tmp = errno;
			(void) unlink_exit_FIFO(&cmd_block.exit_id);
			error(EXIT_FAILURE, tmp,
			    catgets(catfd, SET, 764,
			    "Could not retrieve command response"));
		}
		(void) unlink_exit_FIFO(&cmd_block.exit_id);
		if (completion != 0)
			error(EXIT_FAILURE, 0, resp_string);
	}
	return (0);
}


/*
 *	Validate ANSI tape label field.
 *	Check that a string is correct for an ANSI tape label field.
 */

static int		/* 1 if string is valid, 0 if not */
is_ansi_tp_label(
	char *s,	/* string to be validated */
	size_t size)	/* size of field */
{
	char	    c;

	if (strlen(s) > size)
		return (0);
	while ((c = *s++) != '\0') {
		if (isupper(c))
			continue;
		if (isdigit(c))
			continue;
		if (strchr("!\"%&'()*+,-./:;<=>?_", c) == NULL)
			return (0);
	}
	return (1);
}

static void
pr_usage(void)
{
	if (type == DT_TAPE) {
		printf(catgets(catfd, SET, 13001,
		    "Usage: %s %s\n"), program_name,
		"-vsn vv... -[new | old vv...] [-b blocksize] [-w] [-V]");
		printf("          [-erase] eq[:slot[:partition]]\n");
	} else {
		printf(catgets(catfd, SET, 13001,
		    "Usage: %s %s\n"), program_name,
		"-vsn vv... -[new | old vv...] [-info aa...] [-w] [-V]");
		printf("          [-erase] eq[:slot[:side]]\n");
	}
}
