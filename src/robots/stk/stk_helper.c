/*
 *	stk_helper.c - program to issue commands for the stk acsapi.
 */

/*
 *
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

#include <synch.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sam/types.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "sam/devnm.h"
#include "stk.h"

#pragma ident "$Revision: 1.31 $"

/*	Function prototypes */
void process_signal(int);
void doit(int, char **);
int parse_drives(int *, DRIVEID drive_ids[MAX_ID], dev_ent_t *);

/* globals */
char *program_name;
shm_alloc_t master_shm;
dev_ptr_tbl_t *dev_ptr_tbl;
shm_ptr_tbl_t *shm_ptr_tbl;

/* These are needed by the signal handler */
dev_ent_t *un;
union {
	stk_resp_acs_t _stk_resp;
	full_stk_resp_acs_t full_stk_resp;
}	the_response;

#define	stk_resp (the_response._stk_resp)

#define	FIX_ARGS  5

static int	num_drives = 0;
static DRIVEID	drive_ids[MAX_ID];

/*
 *	stk_helper - issue requests to the acsapi.
 *
 * called:
 *		shmid	- shared memory id *
 *		eq		- equipment number of robot
 *
 *	  takes messages from the robot private message as a string of
 *	  comma seperated arguments to the helper.
 *
 *		command - command to issue
 *		shmid	- shared memory id
 *		sequ	- sequence number
 *		eq		- equipment number of robot
 *		event	- address of event(to pass back)
 *		varies by command.
 *
 * exits - with a 0, all ok response sent via message to robot
 *		   > zero acsapi status
 *		   > STATUS_LAST is a system error errno is added
 */
int
main(
	int argc,
	char **argv)
{
	equ_t 	equ;
	char 	logname[20];

	if (argc == 3 && strcmp(*argv, "sam-stk_helper") == 0) {
		/* main helper */
		char *l_mess;
		char *line;
		char *line2 = (char *)malloc(512);
		stk_priv_mess_t *priv_message;

		argv++;
		master_shm.shmid = atoi(*argv++);
		equ = atoi(*argv++);
		sprintf(logname, "stkhlp-%d", equ);
		program_name = logname;

		if ((master_shm.shared_memory =
		    shmat(master_shm.shmid, NULL, 0664)) == (void *)-1)
			exit(errno);

		shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
		dev_ptr_tbl = (dev_ptr_tbl_t *)
		    SHM_REF_ADDR(shm_ptr_tbl->dev_table);

		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "start helper daemon.");

		if (sizeof (sam_message_t) < sizeof (stk_resp_acs_t)) {
			sam_syslog(LOG_CRIT,
			    "sizeof message area(%d) is less than sizeof"
			    " stk_resp(%d).",
			    sizeof (sam_message_t),
			    sizeof (stk_resp_acs_t));
			exit(EINVAL);
		}

		/* LINTED pointer cast may result in improper alignment */
		un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[equ]);
		l_mess = un->dis_mes[DIS_MES_NORM];
		priv_message = (stk_priv_mess_t *)
		    SHM_REF_ADDR(un->dt.rb.private);
		sigignore(SIGCHLD);
		memset(line2, '\0', 512);

		mutex_lock(&priv_message->mutex);

		for (;;) {
			int l_argc = 0;
			int len;
			pid_t pid;
			char *l_args[FIX_ARGS + 6], *path;

			while (priv_message->mtype == STK_PRIV_VOID)
				cond_wait(&priv_message->cond_r,
				    &priv_message->mutex);

			memccpy(line2, priv_message->message, '\0',
			    (len = strlen(priv_message->message) + 2));

			line = priv_message->message;
			mutex_unlock(&priv_message->mutex);
			if (DBG_LVL(SAM_DBG_TMESG))
				sam_syslog(LOG_DEBUG, "ME:%s.", line);
			if ((path = strtok(line2, ", ")) == NULL) {
				sam_syslog(LOG_INFO, "Bad message %s.", line);
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sprintf(l_mess, "bad message ");
					memccpy(l_mess + 12, line, '\0',
					    DIS_MES_LEN - 13);
				}
			} else {
				for (l_argc = 0; l_argc < (FIX_ARGS + 6);
				    l_argc++) {
					if ((l_args[l_argc] =
					    strtok(NULL, ", ")) == NULL)
						break;
#if defined(DEBUG)
					else if (DBG_LVL_EQ(
					    SAM_DBG_DEBUG | SAM_DBG_TMESG))
						sam_syslog(LOG_INFO,
						"arg %d, %s", l_argc,
						    l_args[l_argc]);
#endif
				}

				if (l_argc == (FIX_ARGS + 6) ||
				    (l_argc < FIX_ARGS)) {
					sam_syslog(LOG_INFO,
					    "Bad message %s.", line);
					if (DBG_LVL(SAM_DBG_DEBUG)) {
						sprintf(l_mess, "bad message ");
						memccpy(l_mess + 12, line, '\0',
						    DIS_MES_LEN - 13);
					}
				} else {
					pid = -1;

					while (pid < 0) {
						int fd;

						/*
						 * Set non-standard files to
						 * close on exec.
						 */
						for (fd = STDERR_FILENO + 1;
						    fd < OPEN_MAX; fd++) {

							(void) fcntl(fd,
							    F_SETFD,
							    FD_CLOEXEC);
						}
						/* if we are the child */
						if ((pid = fork1()) == 0) {

#if defined(CALLDIRECT)
							doit(l_argc, &l_args);
#else
							execv(path, l_args);
#endif
							_exit(0);
						}
						if (pid < 0) {
							int l_err = errno;

							if (DBG_LVL(
							    SAM_DBG_DEBUG)) {
								sam_syslog(
								    LOG_DEBUG,
								"Helper start:"
								" %s:%m.",
								    line);
						memccpy(l_mess, "helper start"
						    " failed", '\0',
						    DIS_MES_LEN);
							} else
								sam_syslog(
								    LOG_DEBUG,
								"Helper start:"
								" %m.");

							if (l_err == EAGAIN ||
							    l_err == ENOMEM)
								sleep(4);
							else
								break;
						}
					}
				}
			}

			mutex_lock(&priv_message->mutex);
			memset(priv_message->message, '\0', len + 1);
			priv_message->mtype = STK_PRIV_VOID;
			cond_signal(&priv_message->cond_i);
		}
		/* NOTREACHED */
/* exit(0); */
	}
	doit(argc, argv);
	_exit(0);
}


void
doit(
	int argc,
	char **argv)
{
	int 	i, no_resp = FALSE;
	uint_t 	driveid;
	DRIVEID drive_id;
	equ_t 	equ;
	char 	lg_mess[200];
	char 	*cmd, *seqn;
	char 	logname[20];
	time_t 	start;
	struct 	sigaction s_act;
	sigset_t block_set;
	uint_t 	capid;
	CAPID 	cap_id;
	LOCKID 	lockid;
	VOLID 	vol_ids[MAX_ID];
	SEQ_NO 	sequence, resp_seq;
	REQ_ID 	resp_req;
	ACS_RESPONSE_TYPE type = RT_LAST;
	int	force_cmd_count;
	struct	stat buf;
	boolean_t hasam_running;

	s_act.sa_handler = process_signal;
	sigfillset(&s_act.sa_mask);	/* block all other signals while */

	/* signal handler active. */
	sigfillset(&block_set);		/* block all signals */
	sigdelset(&block_set, SIGALRM);	/* except these */
	sigdelset(&block_set, SIGINT);
	sigprocmask(SIG_SETMASK, &block_set, NULL);
	s_act.sa_flags = 0;

	if (sigaction(SIGALRM, &s_act, NULL))
		sam_syslog(LOG_DEBUG, "doit: sigaction: ALRM: %m");

	if (sigaction(SIGINT, &s_act, NULL))
		sam_syslog(LOG_DEBUG, "doit: sigaction: INT: %m");

	memset(lg_mess, '\0', 200);
	sprintf(lg_mess, "start:");

	hasam_running = FALSE;

	if (stat(HASAM_RUN_FILE, &buf) == 0) {
		hasam_running = TRUE;
	}

	for (i = 0; i < argc; i++) {
		if (*(argv + i) != NULL)
			sprintf(&lg_mess[strlen(lg_mess)], ", %s", *(argv + i));
		else
			sprintf(&lg_mess[strlen(lg_mess)], ", missing");
	}

	memset(&the_response, 0, sizeof (full_stk_resp_acs_t));
	memset((void *)vol_ids, 0, sizeof (VOLID) * MAX_ID);
	if (argc < FIX_ARGS)
		exit(STATUS_LAST + EINVAL);
	cmd = *argv++;
	master_shm.shmid = atoi(*argv++);
	seqn = *argv++;
	equ = atoi(*argv++);
	stk_resp.hdr.event = (robo_event_t *)strtoul(*argv++, NULL, 16);
#if !defined(CALLDIRECT)
	if ((master_shm.shared_memory =
	    shmat(master_shm.shmid, NULL, 0774)) == (void *)-1)
		exit(STATUS_LAST + errno);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	/* LINTED pointer cast may result in improper alignment */
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[equ]);
	sprintf(logname, "stkhlp-%d", equ);
	program_name = logname;
#endif

	sequence = strtoul(seqn, NULL, 16);
	if (DBG_LVL_EQ(SAM_DBG_DEBUG | SAM_DBG_EVENT | SAM_DBG_TMESG))
		sam_syslog(LOG_DEBUG,
		    "start:%s(%d)-%d, %s.",
		    cmd, sequence, argc, lg_mess);

	if (un->vsn[0] != '\0')
		acs_set_access(&un->vsn[0]);

	stk_resp.hdr.api_status = 0;
	time(&start);
	if (strcmp(cmd, "mount") == 0) {
		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_MOUNT;
			driveid = strtoul(*argv++, NULL, 16);
			memcpy((void *)&vol_ids[0].external_label[0],
			    *argv, EXTERNAL_LABEL_SIZE);
			memcpy((void *)&drive_id, &driveid, sizeof (driveid));
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "mount: drive(%d,%d,%d,%d), vol(%s)",
				    DRIVE_LOC(drive_id),
				    vol_ids[0].external_label);

			if ((stk_resp.hdr.api_status =
			    acs_mount(sequence, 0, vol_ids[0],
			    drive_id, FALSE, FALSE))) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_ERR,
					    "error on mount call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
			}
		}
	} else if (strcmp(cmd, "dismount") == 0) {
		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_DISMOUNT;
			driveid = strtoul(*argv++, NULL, 16);
			memcpy((void *)&vol_ids[0].external_label[0],
			    *argv, EXTERNAL_LABEL_SIZE);
			memcpy((void *)&drive_id, &driveid, sizeof (driveid));

			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "dismount: drive(%d,%d,%d,%d), vol(%s)",
				    DRIVE_LOC(drive_id),
				    vol_ids[0].external_label);

			if ((stk_resp.hdr.api_status =
			    acs_dismount(sequence, 0, vol_ids[0],
			    drive_id, FALSE))) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_ERR,
					"error on dismount call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
			}
		}
	} else if (strcmp(cmd, "force") == 0) {
		if ((hasam_running) && (argc == (FIX_ARGS + 2))) {
			force_cmd_count = (FIX_ARGS + 2);
		} else {
			force_cmd_count = (FIX_ARGS + 1);
		}
		if (argc != force_cmd_count) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_DISMOUNT;
			if ((hasam_running) && (argc == (FIX_ARGS + 2))) {
				driveid = strtoul(*argv++, NULL, 16);
				memcpy((void *)&vol_ids[0].external_label[0],
				    *argv, EXTERNAL_LABEL_SIZE);
			} else {
				driveid = strtoul(*argv, NULL, 16);
				memcpy((void *)&vol_ids[0].external_label[0],
				    "UNKNWN", EXTERNAL_LABEL_SIZE);
			}
			memcpy((void *)&drive_id, &driveid, sizeof (driveid));
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "force: drive(%d,%d,%d,%d), vol(%s)",
				    DRIVE_LOC(drive_id),
				    vol_ids[0].external_label);

			if ((stk_resp.hdr.api_status =
			    acs_dismount(sequence, 0, vol_ids[0],
			    drive_id, TRUE))) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_ERR,
					"error on dismount call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
			}
		}
	} else if (strcmp(cmd, "view") == 0) {
		if (argc != (FIX_ARGS + 1)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);

			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_QUERY;
			memcpy((void *)&vol_ids[0].external_label[0],
			    *argv, sizeof (VOLID));
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"view:vol(%s)", vol_ids[0].external_label);

			if ((stk_resp.hdr.api_status =
			    acs_query_volume(sequence, vol_ids, 1))) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_ERR,
					"error on query call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
			}
		}
	} else if (strcmp(cmd, "query_drv") == 0) {
		uint_t 	driveid;
		DRIVEID drive_ids[MAX_ID];	/* need to allocate after int */

		if (argc != (FIX_ARGS + 1)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_QUERY;
			driveid = strtoul(*argv, NULL, 16);
			memcpy((void *)&
			    drive_ids[0], &driveid, sizeof (driveid));
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "qury_drv: drive(%#x)", driveid);

			if ((stk_resp.hdr.api_status =
			    acs_query_drive(sequence, drive_ids, 1))) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_ERR,
					"error on query call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
			}
		}
	} else if (strcmp(cmd, "query_all_drvs") == 0) {
		uint_t	libraryid;

		if (argc != FIX_ARGS + 1) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			if (parse_drives(&num_drives, &drive_ids[0], un)) {
				no_resp = TRUE;
			} else {
				stk_resp.hdr.acs_command = COMMAND_QUERY;
				libraryid = strtoul(*argv, NULL, 9);
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG, "query_all_drvs");

				if ((stk_resp.hdr.api_status =
				    acs_query_drive(sequence, drive_ids,
				    num_drives))) {
					no_resp = TRUE;
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_ERR,
						"error on query call(%d) %#x.",
						    sequence,
						    stk_resp.hdr.api_status);
				}
			}
		}
	} else if (strcmp(cmd, "query_mnt_stat") == 0) {
		if (argc != (FIX_ARGS + 1)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_QUERY;
			memcpy((void *)&vol_ids[0].external_label[0],
			    *argv, EXTERNAL_LABEL_SIZE);
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG, "query_mount: vol(%s)",
				    vol_ids[0].external_label);
			}
			if (stk_resp.hdr.api_status =
			    acs_query_mount(sequence, vol_ids, 1)) {

				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sam_syslog(LOG_ERR,
					"error on query mount call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
				}
			}
		}
	} else if (strcmp(cmd, "eject_volume") == 0) {
		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			no_resp = TRUE;
			stk_resp.hdr.api_status = STATUS_LAST + EINVAL;
		} else {
			stk_resp.hdr.acs_command = COMMAND_EJECT;
			memcpy((void *)&vol_ids[0].external_label[0],
			    *argv, EXTERNAL_LABEL_SIZE);
			argv++;
			capid = strtoul(*argv, NULL, 16);
			memcpy((void *)&cap_id, &capid, sizeof (capid));
			lockid = NO_LOCK_ID;
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG, "eject_volume: vol(%s)",
				    vol_ids[0].external_label);
			}
			if ((stk_resp.hdr.api_status =
			    acs_eject(sequence, lockid, cap_id, 1, vol_ids))) {
				no_resp = TRUE;
				if (DBG_LVL(SAM_DBG_DEBUG)) {
					sam_syslog(LOG_ERR,
					"error on eject call(%d) %#x.",
					    sequence, stk_resp.hdr.api_status);
				}
			}
		}
	} else {
		no_resp = TRUE;
		stk_resp.hdr.api_status = STATUS_LAST + EBADF;
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_ERR, "error on query call(%d) %#x.",
			    sequence, stk_resp.hdr.api_status);
	}
	if (!no_resp)
		do {
			/*
			 * If query command, drive or volume queries,
			 * wait 2 minutes
			 * for a reply. Everything else will wait 15
			 * minutes for a response from ACSLS.
			 */
			if (stk_resp.hdr.acs_command == COMMAND_QUERY) {
				alarm(2 * 60);
			} else {
				alarm(15 * 60);
			}
			if ((stk_resp.hdr.acs_status =
			    acs_response(-1, &resp_seq, &resp_req, &type,
			    (void *)&the_response.full_stk_resp.data)) !=
			    STATUS_SUCCESS) {

				sam_syslog(LOG_ERR,
				    "acs_response returned failure %s for %d.",
				    acs_status(stk_resp.hdr.acs_status),
				    sequence);
			}
			if (resp_seq != sequence) {
				sam_syslog(LOG_ERR,
				    "acs_response returned wrong sequence %d,"
				    " expecting %d.", resp_seq, sequence);
			}
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"acs_response(%d): type %d.", resp_seq, type);

			if (type == RT_NONE && stk_resp.hdr.acs_status
			    != STATUS_SUCCESS)
				break;

		} while (type != RT_FINAL);

	if (DBG_LVL(SAM_DBG_TIME))
		sam_syslog(LOG_INFO, "TIME: %s(%d): %d seconds.", cmd, sequence,
		    (int)(time(NULL) - start));

	sigfillset(&block_set);		/* block all signals */
	sigprocmask(SIG_SETMASK, &block_set, NULL);
	alarm(0);
	stk_resp.hdr.type = type;
	if (un->status.b.present) {
		message_request_t *message;

		mutex_lock(&un->mutex);
		un->active++;
		message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = MESSAGE_MAGIC;
		/* a harmless command */
		message->message.command = MESS_CMD_ACK;
		message->mtype = MESS_MT_APIHELP;
		memcpy((void *)&message->message.param.start_of_request,
		    (void *)&stk_resp, sizeof (stk_resp_acs_t));
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
}


/*
 * Looks like the acsls is not talking, fake a communications failure
 * and signal the requester.
 */
void
process_signal(int sig_no)
{
	sam_syslog(LOG_DEBUG, "ACSLS communication failure: signal %d", sig_no);

	if (un->status.b.present) {
		message_request_t *message;

		stk_resp.hdr.type = RT_NONE;
		stk_resp.hdr.acs_status = STATUS_COMMUNICATION_FAILED;
		mutex_lock(&un->mutex);
		un->active++;
		message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.magic = MESSAGE_MAGIC;
		/* a harmless command */
		message->message.command = MESS_CMD_PREVIEW;
		message->mtype = MESS_MT_APIHELP;
		memcpy((void *)&message->message.param.start_of_request,
		    (void *)&stk_resp, sizeof (stk_resp_acs_t));
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
	exit(1);
}


/*
 * Parse the parameters file to get the drive ids
 */
int
parse_drives(
	int *num_drive,
	DRIVEID *drive_ids,
	dev_ent_t *un)
{
	int		parts, value;
	char 	*line, *tmp;
	char	*ent_ptr = "parse_drives";
	FILE	*open_str;
	DRIVE	acs_drive;
	PANEL	acs_panel;
	LSM		acs_lsm;
	ACS		acs_acs;

	if ((open_str = fopen(un->name, "r")) == NULL) {
		sam_syslog(LOG_CRIT, catgets(catfd, SET, 9117,
		    "%s: Unable to open"
		    " configuration file(%s): %m."), ent_ptr, un->name);
		return (1);
	}

	line = malloc(200);
	mutex_lock(&un->mutex);

	while (fgets(memset(line, 0, 200), 200, open_str) != NULL) {
		char *open_paren;

		if (*line == '#' || strlen(line) == 0 || *line == '\n')
			continue;
		if ((tmp = strchr(line, '#')) != NULL)
			memset(tmp, 0, (200 - (tmp - line)));
		if ((open_paren = strchr(line, '(')) != NULL)
			*open_paren = '\0';
		if ((tmp = strtok(line, "= \t\n")) == NULL)
			continue;

		if (*tmp != '/') {
			continue;
		} else {
			open_paren++;
			parts = 0;
			while (parts < 4) {
				char *param;

				if (parts == 0) {
					tmp = strtok(open_paren, " ,=:\t)");
				} else {
					tmp = strtok(NULL, " ,=:\t)");
				}

				param = strtok(NULL, " ,=:\t)");
				value = atoi(param);

				if (strcasecmp(tmp, "acs") == 0) {
					acs_acs = value;
				} else if (strcasecmp(tmp, "lsm") == 0) {
					acs_lsm = value;
				} else if (strcasecmp(tmp, "panel") == 0) {
					acs_panel = value;
				} else if (strcasecmp(tmp, "drive") == 0) {
					acs_drive = value;
				}
				parts++;
			}

			drive_ids[num_drives].drive = acs_drive;
			drive_ids[num_drives].panel_id.panel = acs_panel;
			drive_ids[num_drives].panel_id.lsm_id.lsm = acs_lsm;
			drive_ids[num_drives].panel_id.lsm_id.acs = acs_acs;
			num_drives++;
		}
	}
	mutex_unlock(&un->mutex);
	free(line);
	return (0);
}
