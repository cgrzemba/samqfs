/*
 * auditslot.c
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

#pragma ident "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <libgen.h>

#define	DEC_INIT

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/exit.h"

#define	USAGE_STRING	"[-e] eq:slot[:partition] [eq:slot[:partition]...]"

static void Usage(char *);

int
main(int argc, char **argv)
{
	int	shmid, fifo_fd;
	int	eodflag = FALSE;
	int	errors = 0;
	void	*memory;
	char	fifo_file[MAXPATHLEN];
	char	buff[256];
	char 	*my_name;

	shm_ptr_tbl_t	*shm_ptr_tbl;
	sam_cmd_fifo_t    cmd_block;

	CustmsgInit(0, NULL);

#if !defined(DEBUG)
	if (geteuid() != 0) {
		fprintf(stderr, catgets(catfd, SET, 100,
		    "%s may be run only by super-user.\n"),
		    basename(argv[0]));
		exit(EXIT_FAILURE);
	}
#endif

	my_name = argv[0];
	argv++;
	argc--;

	if (argc > 0 && strcmp (*argv, "-e") == 0) {
		eodflag = TRUE;
		argv++;
		argc--;
	} else if (argc > 0 && **argv == '-') {
		Usage(my_name);
	}

	if (argc < 1)
		Usage(my_name);

	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		fprintf(stderr, catgets(catfd, SET, 272,
		    "SAM_FS is not running.\n"));
		exit(EXIT_FAILURE);
	}

	if ((memory = shmat(shmid, (void *)NULL, 0444)) == (void *)-1) {
		fprintf(stderr,
		    catgets(catfd, SET, 568,
		    "Cannot attach shared memory segment.\n"));
		exit(EXIT_FAILURE);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)memory;
	sprintf(fifo_file, "%s" "/" CMD_FIFO_NAME,
	    ((char *)memory + shm_ptr_tbl->fifo_path));

	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
		perror(catgets(catfd, SET, 1113,
		    "Unable to open command fifo:"));
		exit(EXIT_FAILURE);
	}

	if (CatalogInit("auditslot") == -1) {
		fprintf(stderr, "%s\n", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(EXIT_FAILURE);
	}


	while (argc-- > 0) {
		struct CatalogEntry ced;
		struct CatalogEntry *ce = &ced;
		struct VolId vid;

		if ((StrToVolId(*argv, &vid) != 0) ||
		    ((vid.ViFlags != VI_cart) && (vid.ViFlags != VI_onepart))) {
			fprintf(stderr,
			    catgets(catfd, SET, 18207,
			    "Volume specification error %s:"),
			    *argv);
			fprintf(stderr, "\n");
			errors++;
			argv++;
			continue;
		}

		if ((ce = CatalogGetEntry(&vid, &ced)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 18208,
			    "Volume specification error %s: %s"),
			    *argv, StrFromErrno(errno, buff, sizeof (buff)));
			fprintf(stderr, "\n");
			errors++;
			argv++;
			continue;
		} else {
			(void) CatalogSetFieldByLoc(ce->CeEq, ce->CeSlot,
			    ce->CePart, CEF_Status, CES_needs_audit, 0);
		}

		memset(&cmd_block, 0, sizeof (cmd_block));

		cmd_block.magic = CMD_FIFO_MAGIC;
		cmd_block.cmd = CMD_FIFO_AUDIT;
		if (eodflag)
			cmd_block.flags = CMD_AUDIT_EOD;
		cmd_block.eq = vid.ViEq;
		cmd_block.slot = ce->CeSlot;
		cmd_block.part = ce->CePart;

		write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
		argv++;
	}

	close(fifo_fd);
	exit(EXIT_SUCCESS);
	/* LINTED function has no return statement */
}

static void
Usage(char *name)
{
	fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
	    basename(name), USAGE_STRING);
	exit(EXIT_USAGE);
}
