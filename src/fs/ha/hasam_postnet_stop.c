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

/*
 * hasam_postnet_stop.c - Postnet_stop method for SUNW.hasam
 * Run samd hastop first and then samd stop to make sure archiver
 * and stager daemons are stopped before stopping sam-amld and its
 * children.
 */

#include <stdlib.h>
#include <unistd.h>
#include <procfs.h>

#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "mgmt/util.h"

#include "hasam.h"

void kill_proc_if_alive(char *);

int
main(int argc, char *argv[])
{
	struct RgInfo_4hasam rg;
	boolean_t amld_status;
	boolean_t archiver_status;
	boolean_t stager_status;
	boolean_t stageall_status;
	boolean_t cats_status;
	boolean_t robot_status;
	int i;

	/*
	 * Set debug to print messages to stderr
	 */
	hasam_debug = 0;

	/*
	 * Process arguments passed by RGM and initialize syslog.
	 */
	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo_4hasam(argc, argv, &rg) < 0) {
		return (1);
	}

	/*
	 * Check is archiver or stager daemons are running,
	 * if yes make about 3 attempts to stop them.
	 */
	for (i = 0; i < 3; i++) {
		archiver_status = check_sam_daemon("sam-archiverd");
		stager_status = check_sam_daemon("sam-stagerd");
		stageall_status = check_sam_daemon("sam-stagealld");

		if ((archiver_status) || (stager_status) ||
		    (stageall_status)) {
			dprintf(stderr, "Arch=%d, Stg=%d, Stgall=%d",
			    archiver_status, stager_status,
			    stageall_status);
			samd_hastop();
		} else {
			break;
		}
		sleep(samd_timer);
	}

	/*
	 * Check if sam-amld or its children continues to run, make 3 attempts
	 * to stop it.
	 */
	for (i = 0; i < 3; i++) {
		amld_status = check_sam_daemon("sam-amld");
		cats_status = check_sam_daemon("sam-catserverd");
		robot_status = check_sam_daemon("sam-robotsd");

		if ((amld_status) || (cats_status) || (robot_status)) {
			dprintf(stderr, "Amld=%d, Cats=%d, Robot=%d",
			    amld_status, cats_status, robot_status);
			samd_stop();
		} else {
			break;
		}
		sleep(samd_timer);
	}

	/*
	 * There have been instances in which sam-stkd children
	 * sam-stk_helper or ssi_so have not exited even after
	 * executing samd stop command. So check if these process
	 * have exited, if not kill it.
	 */
	kill_proc_if_alive("ssi_so");
	kill_proc_if_alive("sam-stk_helper");

	delete_run_file();

	/*
	 * Close the open ds/cluster/rg handles we have open.
	 */
	RelRgInfo_4hasam(&rg);
	return (0);
}

void
kill_proc_if_alive(char *pname)
{
	sqm_lst_t	*procs = NULL;
	psinfo_t	*infop;
	node_t		*node;
	int			rval;

	if (pname == NULL) {
		return;
	}

	rval = find_process(pname, &procs);
	if (rval == -1) {
		return;
	}

	node = procs->head;
	while (node != NULL) {
		infop = node->data;
		dprintf(stderr, "Killing %s process with pid %ld\n",
		    pname, infop->pr_pid);

		sigsend(P_PID, infop->pr_pid, SIGKILL);
		node = node->next;
	}

	lst_free_deep(procs);
}
