/*
 *	main.c - main routine for sam remote server
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

#pragma ident "$Revision: 1.27 $"

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
#include <regex.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <ctype.h>

/* regexp definitions. */
#define	INIT char *sp = instring;
#define	GETC(void) (*sp++)
#define	PEEKC(void) (*sp)
#define	UNGETC(void) (--sp)
#define	RETURN(ptr) return (ptr)
#define	ERROR(val) (val)

#include <regexp.h>

#define	MAIN

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "sam/devnm.h"
#include "server.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "aml/logging.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
#include "sam/exit.h"
#include "sam/signals.h"

#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Globals */
shm_alloc_t master_shm;
char *program_name;

/* Function prototypes */
int sigwait(sigset_t *);
static int compile_reg_ex(rmt_vsn_equ_list_t *current_equ,
	char *string, char *errmsg, int mes_len);
static void build_media_list(FILE *fd, char *fname, int *line_count,
	rmt_sam_client_t *clnt);
static void init_server(dev_ent_t *un);

int
main(
	int argc,
	char **argv)
{
	int what_signal;
	char logname[20];
	char *lc_mess;
	equ_t eq;
	sigset_t signal_set, full_block_set;
	dev_ent_t *un;
	struct sigaction sig_action;
	dev_ptr_tbl_t *dev_ptr_tbl;
	shm_ptr_tbl_t *shm_ptr_tbl;
	int i;
	srvr_clnt_t *srvr_clnt;
	rmt_sam_client_t *clnt;


	if (debug_init != NULL) {
		debug_init();
	}

	if (argc != 4) {
		fprintf(stderr, "Usage: rs_server mshmid pshmid equip\n");
		exit(1);
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
	sprintf(logname, "server-%d", eq);
	program_name = logname;

	(void) open("/dev/null", O_RDONLY);	/* stdin */
	(void) open("/dev/null", O_RDONLY);	/* stdout */
	(void) open("/dev/null", O_RDONLY);	/* stderr */

	/*
	 * Initialize syslog.
	 */
	CustmsgInit(1, NULL);

	/*
	 * Prepare tracing.
	 */
	TraceInit(program_name, TI_rmtserver | TR_MPLOCK);
	Trace(TR_PROC, "Server proc started");

	master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774);
	if (master_shm.shared_memory == (void *) -1) {
		SysError(HERE, "Unable to attach master shared memory segment");
		exit(1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;

	if (CatalogInit("rmtserver") == -1) {
		LibFatal(CatalogInit, "");
	}

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[eq]);

	lc_mess = un->dis_mes[DIS_MES_CRIT];
	if (DBG_LVL(SAM_DBG_RBDELAY)) {
		int wait = 60;

		Trace(TR_MISC, "Waiting for 60 seconds");
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

	sig_action.sa_handler = SIG_DFL; /* want to restart system calls */
	sigemptyset(&sig_action.sa_mask); /* on excepted signals. */
	sig_action.sa_flags = SA_RESTART;

	(void) sigaction(SIGINT, &sig_action, NULL);
	(void) sigaction(SIGHUP, &sig_action, NULL);
	(void) sigaction(SIGALRM, &sig_action, NULL);

	/* The default mode is to block everything */
	thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);

	un->dt.ss.private = (void *)
	    malloc_wait(sizeof (rmt_sam_client_t) * RMT_SAM_MAX_CLIENTS, 4, 0);

	/*
	 * Initialize the client structs in the shared memory area and
	 * the private area.
	 */
	srvr_clnt = (srvr_clnt_t *)SHM_REF_ADDR(un->dt.ss.clients);
	clnt = (rmt_sam_client_t *)un->dt.ss.private;

	memset(srvr_clnt, 0, sizeof (srvr_clnt_t) * RMT_SAM_MAX_CLIENTS);
	memset(clnt, 0, sizeof (rmt_sam_client_t) * RMT_SAM_MAX_CLIENTS);
	for (i = 0; i < RMT_SAM_MAX_CLIENTS; i++, srvr_clnt++, clnt++) {
		(void) mutex_init(&(srvr_clnt->sc_mutex), USYNC_THREAD,
		    (void *) NULL);
		srvr_clnt->index = i;
		memset(clnt, 0, sizeof (*clnt));
		clnt->fd = -1;
		clnt->un = un;
	}

	init_server(un);

	/*
	 * Start the connect thread.  This thread will watch
	 * for connections from clients.
	 */
	if (thr_create(NULL, LG_THR_STK, watch_connects, (void *) un,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL)) {
		SysError(HERE, "Unable to start connection thread");
		exit(EXIT_FATAL);
	}

	if (thr_create(NULL, LG_THR_STK, rs_monitor_msg, (void *) un,
	    (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL)) {
		SysError(HERE, "Unable to start monitor_msg thread");
		exit(EXIT_FATAL);
	}

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
 * Initialize server.  Process the configuration file.
 */
static void
init_server(
	dev_ent_t *un)
{
	int client_cnt = 0, ln = 0, herr;
	char *c_buffer, *d_buffer;
	char line[100];
	FILE *fd;
	struct hostent *h_ent = NULL;
	srvr_clnt_t *srvr_clnt =
	    (srvr_clnt_t *)SHM_REF_ADDR(un->dt.ss.clients);
	rmt_sam_client_t *clnt = (rmt_sam_client_t *)un->dt.ss.private;

	if ((fd = fopen(un->name, "r")) == NULL) {
		/*
		 * Unable to open configuration file.
		 */
		SendCustMsg(HERE, 22300, un->name);
		un->state = DEV_OFF;
		un->status.bits = 0;
		return;
	}

	c_buffer = malloc_wait(2048, 2, 0);
	d_buffer = malloc_wait(2048, 2, 0);
	while (fgets(line, 100, fd) != NULL) {
		char *tmp;

		ln++;
		line[99] = '\0';
		tmp = &line[strlen(line) - 1];

		while ((*tmp == ' ' || *tmp == '\t' || *tmp == '\n') &&
		    tmp != line) {
			*tmp = '\0';
			tmp--;
		}

		if (line[0] == '\n' || line[0] == '#' || line[0] == '\0')
			continue;

		/*
		 * If line starts with white space, then it contains
		 * parameters for the last client found.
		 */
		if (isspace(line[0])) {
			char *key;

			key = strtok(line, "\t= , ");
			while ((key != NULL)) {
				/* Media has no = */
				if (strcmp(key, "media") == 0) {
					build_media_list(fd, un->name, &ln,
					    clnt);
					break;
				}

				if (strtok(NULL, "\t= , ") == NULL) {
					/*
					 * Syntax error in configuration file.
					 */
					SendCustMsg(HERE, 22303, un->name, ln);
					break;
				} else {
					/*
					 * Invalid keyword.
					 */
					SendCustMsg(HERE, 22304, key,
					    un->name, ln);
				}
				key = strtok(NULL, "\t= , ");
			}
			continue;
		} else if (h_ent != NULL) {
			clnt++;
			srvr_clnt++;
			client_cnt++;
		}
		if (client_cnt == RMT_SAM_MAX_CLIENTS) {
			/*
			 * Too many clients, connection not accepted.
			 */
			SendCustMsg(HERE, 22305, un->set, line);
			continue;
		}

		if (h_ent = getipnodebyname(line, AF_INET6, AI_ADDRCONFIG,
		    &herr)) {

			Trace(TR_MISC, "Client is IPv6 '%s' for %s(%d).",
			    TrNullString(line), TrNullString(un->set), un->eq);
			memcpy(&srvr_clnt->control_addr6, h_ent->h_addr,
			    h_ent->h_length);
			srvr_clnt->flags |= SRVR_CLNT_IPV6;
			srvr_clnt->flags |= SRVR_CLNT_PRESENT;
			/*
			 * IPv6 Client authorized.
			 */
			SendCustMsg(HERE, 22306, line, un->set);

		} else if (h_ent = getipnodebyname(line, AF_INET, AI_ADDRCONFIG,
		    &herr)) {
			Trace(TR_MISC, "Client is IPv4 '%s' for %s(%d).",
			    TrNullString(line), TrNullString(un->set), un->eq);
			memcpy(&srvr_clnt->control_addr, h_ent->h_addr,
			    h_ent->h_length);
			srvr_clnt->flags |= SRVR_CLNT_PRESENT;
			/*
			 * IPv4 Client authorized.
			 */
			SendCustMsg(HERE, 22306, line, un->set);
		} else {
			/*
			 * Unable to find client.
			 */
			Trace(TR_ERR, "Unable to find client '%s' for %s(%d).",
			    TrNullString(line), TrNullString(un->set), un->eq);
			SendCustMsg(HERE, 22307, line, un->set);
		}
	}
	fclose(fd);
	free(c_buffer);
	free(d_buffer);
}

/*
 * Build list of searchable devices for this client.
 */
static void
build_media_list(
	FILE *fd,			/* fd of configuration file */
	char *fname,			/* name of configuration file */
	int *line_count,
	rmt_sam_client_t *clnt)
{
	int got_historian = FALSE;
	char line[100];
	dev_ptr_tbl_t *dev_ptr_tbl;
	sam_defaults_t *defaults;
	rmt_vsn_equ_list_t *current_equ;
	rmt_vsn_equ_list_t *last_equ = NULL;
	rmt_vsn_equ_list_t *first_equ;
	dev_nm_t *nm_ent;

	dev_ptr_tbl = (dev_ptr_tbl_t *)
	    SHM_REF_ADDR(((shm_ptr_tbl_t *)master_shm.shared_memory)->
	    dev_table);

	defaults = GetDefaults();
	/* Always free current_equ before return */
	current_equ =
	    (rmt_vsn_equ_list_t *)malloc_wait(sizeof (*current_equ), 2, 0);
	memset(current_equ, 0, sizeof (*current_equ));
	clnt->first_equ = first_equ = current_equ;

	while (fgets(line, 100, fd) != NULL) {
		uint_t eq;
		media_t media;
		char *tmp, *key, *last_char;
		char comp_err[180];

		(*line_count)++;
		line[99] = '\0';
		tmp = &line[strlen(line) - 1];

		while ((*tmp == ' ' || *tmp == '\n') && tmp != line) {
			*tmp = '\0';
			tmp--;
		}

		if (line[0] == '\n' || line[0] == '#' || line[0] == '\0') {
			continue;
		}

		last_char = tmp;
		if ((key = strtok(line, "\t ")) == NULL) {
			SendCustMsg(HERE, 22303, fname, *line_count);
			continue;
		}

		if (strcmp(key, "endmedia") == 0) {
			break;
		}

		eq = strtoul(key, NULL, 10);
		if (eq == 0 || eq > dev_ptr_tbl->max_devices ||
		    dev_ptr_tbl->d_ent[eq] == NULL) {
			SendCustMsg(HERE, 22308, eq, fname, *line_count);
			continue;
		}

		current_equ->un =
		    (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[eq]);

		if (!IS_ROBOT(current_equ->un)) {
			SendCustMsg(HERE, 22309, eq, fname, *line_count);
			continue;
		}

		if (current_equ->un->equ_type == DT_HISTORIAN) {
			got_historian = TRUE;
		}

		if ((key = strtok(NULL, "\t ")) == NULL) {
			SendCustMsg(HERE, 22303, fname, *line_count);
			continue;
		}

		nm_ent = dev_nm2mt;

		media = 0;
		for (; nm_ent->nm != NULL; nm_ent++) {
			if (strcmp(nm_ent->nm, key) == 0) {
				media = nm_ent->dt;
				break;
			}
		}

		if (media == 0) {
			SendCustMsg(HERE, 22310, key, fname, *line_count);
			continue;
		}

		/* If generic type, get default */
		if (media == DT_TAPE) {
			media = defaults->tape;
		} else if (media == DT_OPTICAL) {
			media = defaults->optical;
		}

		current_equ->media = media;

		/*
		 * Find the first non white space after the media type string.
		 * that should be the start of the pattern for matching the
		 * vsns.
		 */
		tmp = key + 3;
		while (isspace(*tmp) && tmp < last_char)
			tmp++;
		if (tmp == last_char) {
			if (isspace(*tmp)) {
				SendCustMsg(HERE, 22303, fname, *line_count);
				continue;
			}
		}

		strcpy(&comp_err[0], "unknown error");
		/* tmp points to the start of the pattern */
		if (compile_reg_ex(current_equ, tmp, &comp_err[0], 180)) {
			SendCustMsg(HERE, 22311, &comp_err[0],
			    fname, *line_count);
			free(current_equ->comp_exp);
			continue;
		}

		current_equ->next = (rmt_vsn_equ_list_t *)malloc_wait(
		    sizeof (*current_equ), 2, 0);
		last_equ = current_equ;
		current_equ = current_equ->next;
		memset(current_equ, 0, sizeof (*current_equ));
	}

	/*
	 * If the historian is not supplied then build entries for the
	 * historian for each equipment/expression supplied
	 */
	if (!got_historian) {
		dev_ent_t *hist_dev = find_historian();
		rmt_vsn_equ_list_t *stop_at = current_equ;

		for (; first_equ != stop_at; first_equ = first_equ->next) {
			current_equ->un = hist_dev;
			current_equ->media = first_equ->media;
			current_equ->comp_exp = first_equ->comp_exp;
			current_equ->comp_type = first_equ->comp_type;
			current_equ->next = (rmt_vsn_equ_list_t *)malloc_wait(
			    sizeof (*current_equ), 2, 0);
			last_equ = current_equ;
			current_equ = current_equ->next;
			memset(current_equ, 0, sizeof (*current_equ));
		}
	}

	if (last_equ != NULL) {
		last_equ->next = NULL;
	}

	if (clnt->first_equ == current_equ) {
		clnt->first_equ = NULL;
	}

	free(current_equ);
}

static int
compile_reg_ex(
	rmt_vsn_equ_list_t *current_equ,
	char *string,
	char *errmsg,
	int mes_len)
{
	int cmp_err;

	current_equ->comp_type = RMT_COMP_TYPE_REGCOMP;
	current_equ->comp_exp = malloc_wait(sizeof (regex_t), 2, 0);
	if ((cmp_err = regcomp((regex_t *)current_equ->comp_exp, string,
	    (REG_EXTENDED | REG_NOSUB))) != 0) {
		if (cmp_err != REG_ENOSYS) {
			regerror(cmp_err, (regex_t *)current_equ->comp_exp,
			    errmsg, mes_len);
			regfree((regex_t *)current_equ->comp_exp);
			free(current_equ->comp_exp);
			return (1);
		}

		free(current_equ->comp_exp);
		current_equ->comp_type = RMT_COMP_TYPE_COMPILE;
		if ((current_equ->comp_exp =
		    compile(string, NULL, NULL, '\0')) == NULL) {
			extern int regerrno;
			sprintf(errmsg, "Error number %d.", regerrno);
			return (1);
		}
	}

	return (0);
}
