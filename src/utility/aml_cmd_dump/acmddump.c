/*
 * sefreport.c
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
 * Use is subject to license terms.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "sam/types.h"
#include "sam/lib.h"
#include "aml/fifo.h"

char *cmd_table [] = {
	/* 0 */ "",
	/* 1 */ "mount",
	/* 2 */ "mount_s",
	/* 3 */ "unload",
	/* 4 */ "label",
	/* 5 */ "set state",
	/* 6 */ "audit",
	/* 7 */ "remove vsn",
	/* 8 */ "remove slot",
	/* 9 */ "remove eq",
	/* 10 */ "set tapealert",
	/* 11 */ "import",
	/* 12 */ "delete preview",
	/* 13 */ "clean drive",
	/* 14 */ "set cat flags",
	/* 15 */ "move media",
	/* 16 */ "add vsn",
	/* 17 */ "set sef state",
	/* 18 */ "load unavail"
};

/*
char flag_table [] = {".","e","r","l","b","","","",
		      "","","","","","","","",
		      "","","","","","","","",
		      "","","","","","","","",
		      "","","","b","","","",""};
*/

static int 
checkheader(sam_cmd_fifo_t *cmd, int len) 
{
	if (len == sizeof(sam_cmd_fifo_t) && cmd->magic == CMD_FIFO_MAGIC)
		return B_TRUE;
	return B_FALSE; 
}

static void
usage(char *s)
{
	printf("USAGE:  %s [-v]\n", s);
}

int
main( int argc, char *argv[])
{
	char		*amld_cmd_log = SAM_FIFO_PATH "/cmd_fifo_log";
	char 		c;
	int verbose = 0;
	int acmdfd = 0;
	sam_cmd_fifo_t cmdblk;
	int rec, readcount = -1;

	while ((c = getopt(argc, argv, "v")) != EOF) {
		switch (c) {
			case 'v':
				verbose = 1;
				break;
			case '?':
			default:
				usage(argv[0]);
				exit(1);
		} /* end switch */
	} /* end while */

	if ((acmdfd = open(amld_cmd_log, O_RDONLY)) < 0) {
		printf("Error opening %s, error %s.\n",
		    amld_cmd_log, strerror(errno));
		exit(1);
	}

	rec=1;
	printf("cmd                eq:slot      flags mtype blocksz state      vsn value\n");
	while (readcount != 0) {
		readcount = read(acmdfd, &cmdblk, sizeof (sam_cmd_fifo_t));
		if (!checkheader(&cmdblk, readcount)) {
			exit(1);
		}
		char slotbuf[7];
		sprintf(slotbuf, "%-6d", (int16_t) cmdblk.slot);
		printf("%16s %4d:%6s %8x %3x %6d %8x %8s %5ld\n", 
		    cmd_table[cmdblk.cmd & CMD_MASK],
		    cmdblk.eq,
		    slotbuf,
		    cmdblk.flags,
		    cmdblk.media,
		    cmdblk.block_size,
		    cmdblk.state,
		    cmdblk.vsn,
		    cmdblk.value);
		if (cmdblk.info[0] != 0)
			printf("%s\n", cmdblk.info);
		rec++;

	} /* end while */

	return (0);

} /* end main */
