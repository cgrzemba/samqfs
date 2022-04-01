/*
 * main.c - main routine for sam remote client
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.24 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define	MAIN

#include "sam/types.h"
#include "aml/shm.h"
#include "aml/remote.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "sam/exit.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "aml/dev_log.h"
#include "sam/sam_trace.h"
#include "sam/exit.h"
#include "client.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
shm_alloc_t master_shm, preview_shm;

char *program_name;

/* Function prototypes */
static int init_client(dev_ent_t *un, srvr_clnt_t *server);

int
main(int argc, char **argv)
{
	int what_signal;
	char logname[20];
	char *lc_mess;
	equ_t eq;
	sigset_t signal_set, full_block_set;
	dev_ent_t *un;
	srvr_clnt_t *server;
	struct sigaction sig_action;
	dev_ptr_tbl_t *dev_ptr_tbl;
	shm_ptr_tbl_t *shm_ptr_tbl;

	if (argc != 4) {
		fprintf(stderr, "Usage: rs_client mshmid pshmid equip\n");
		exit(EXIT_USAGE);
	}

	/* Crack arguments */

	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	/*
	 * Next argument is preview shared memory id.
	 * Skip it, not used.
	 */
	argv++;
	eq = atoi(*argv);
	sprintf(logname, "client-%d", eq);
	program_name = logname;

	(void) open("/dev/null", O_RDONLY);	/* stdin */
	(void) open("/dev/null", O_RDONLY);	/* stdout */
	(void) open("/dev/null", O_RDONLY);	/* stderr */

	/*
	 * Iniialize syslog.
	 */
	CustmsgInit(1, NULL);

	/*
	 * Prepare tracing.
	 */
	TraceInit(program_name, TI_rmtclient | TR_MPLOCK);
	Trace(TR_PROC, "Client proc started");

	master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774);
	if (master_shm.shared_memory == (void *) -1) {
		SysError(HERE, "Unable to attach master shared memory segment");
		exit(1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	if (CatalogInit("rmtclient") == -1) {
		LibFatal(CatalogInit, "");
	}

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[eq]);

	lc_mess = un->dis_mes[DIS_MES_CRIT];
	if (DBG_LVL(SAM_DBG_RBDELAY)) {
		int wait = 60;

		Trace(TR_MISC, "Waiting for 60 seconds.");
		while (wait > 0 && DBG_LVL(SAM_DBG_RBDELAY)) {
			sprintf(lc_mess, "waiting for %d seconds pid %ld",
			    wait, getpid());
			sleep(10);
			wait -= 10;
		}
		*lc_mess = '\0';
	}

	sigfillset(&full_block_set);	/* used to block all signals */

	sigemptyset(&signal_set);	/* signals to except. */
	sigaddset(&signal_set, SIGINT);	/* during sigwait */
	sigaddset(&signal_set, SIGHUP);
	sigaddset(&signal_set, SIGALRM);

	sig_action.sa_handler = SIG_DFL;  /* want to restart system calls */
	sigemptyset(&sig_action.sa_mask); /* on excepted signals. */
	sig_action.sa_flags = SA_RESTART;

	(void) sigaction(SIGINT, &sig_action, NULL);
	(void) sigaction(SIGHUP, &sig_action, NULL);
	(void) sigaction(SIGALRM, &sig_action, NULL);

	/* The default mode is to block everything */
	thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);

	server = (srvr_clnt_t *)SHM_REF_ADDR(un->dt.sc.server);
	(void) mutex_init(&(server->sc_mutex), USYNC_THREAD, (void *) NULL);

	if (init_client(un, server) != 0) {
		exit(EXIT_NORESTART);
	}

	/*
	 * The client should have no entries in the historian.
	 * Mark each entry unavail.  When the server sends over
	 * available media, the unavailable bit will be cleared.
	 */
	mark_catalog_unavail(un);

	if (thr_create(NULL, LG_THR_STK, rc_monitor_msg, (void *) un,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL)) {
		SysError(HERE, "Unable to start monitor thread");
		exit(EXIT_FATAL);
	}

	/* Start the connect thread */
	if (thr_create(NULL, LG_THR_STK, connect_server, (void *) un,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL)) {
		SysError(HERE, "Unable to start connection thread");
		exit(EXIT_FATAL);
	}

	thr_yield();
	for (;;) {
		alarm(20);
		what_signal = sigwait(&signal_set);	/* wait for a signal */
		switch (what_signal) {	/* process the signal */
		case SIGALRM:
			break;

		case SIGINT:
			Trace(TR_MISC, "Shutdown by signal %d", what_signal);
			exit(0);
			break;

		case SIGHUP:
			Trace(TR_MISC, "Shutdown by signal %d", what_signal);
			exit(0);
			break;

		default:
			break;
		}
	}
#ifdef lint
	/* NOTREACHED */
	return (0);
#endif
}

/*
 * Initialize client.  Process the configuration file.
 */
static int
init_client(
	dev_ent_t *un,
	srvr_clnt_t *server)
{
	char line[100];
	FILE *fd;
	struct hostent *h_ent;

	if ((fd = fopen(un->name, "r")) == NULL) {
		sam_syslog(LOG_INFO, "Unable to open %s for %d.", un->name,
		    un->eq);
		un->state = DEV_OFF;
		un->status.bits = 0;
		return (-1);
	}

	while (fgets(line, 100, fd) != NULL) {
		char *tmp;
		int error_num;

		line[99] = '\0';
		tmp = &line[strlen(line) - 1];

		while ((*tmp == ' ' || *tmp == '\n') && tmp != line) {
			*tmp = '\0';
			tmp--;
		}

		if (line[0] == '\n' || line[0] == '#' || line[0] == '\0') {
			continue;
		}

		if (h_ent = getipnodebyname(line, AF_INET6, AI_ADDRCONFIG,
		    &error_num)) {
			memcpy(&server->control_addr6, h_ent->h_addr,
			    h_ent->h_length);
			server->flags |= SRVR_CLNT_IPV6;
			Trace(TR_MISC, "Server is IPv6 '%s' for %s(%d) "
			    "addrlen %d.",
			    TrNullString(line), TrNullString(un->set),
			    un->eq, h_ent->h_length);
		} else if (h_ent = getipnodebyname(line, AF_INET, AI_ADDRCONFIG,
		    &error_num)) {
			memcpy(&server->control_addr, h_ent->h_addr,
			    h_ent->h_length);
			Trace(TR_MISC, "Server is IPv4 '%s' for %s(%d) "
			    "addrlen %d.",
			    TrNullString(line), TrNullString(un->set),
			    un->eq, h_ent->h_length);
		} else {
			Trace(TR_ERR, "Unable to find server '%s' for %s(%d).",
			    TrNullString(line), TrNullString(un->set), un->eq);
		}
		break;
	}
	fclose(fd);
	return (h_ent ? 0 : -1);
}
