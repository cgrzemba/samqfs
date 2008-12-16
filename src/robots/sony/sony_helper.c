/*
 * sony_helper.c - program to issue commands for the sony acsapi.
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

#include <synch.h>
#include <stdlib.h>
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

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#define	DEV_NM_HERE
#include "sam/devnm.h"
#undef DEV_NM_HERE
#include "sony.h"
#include "psc.h"

#pragma ident "$Revision: 1.24 $"

/* Function prototypes */
void doit(int, char **);

/* globals */
shm_alloc_t	master_shm;
dev_ptr_tbl_t	*dev_ptr_tbl;
shm_ptr_tbl_t	*shm_ptr_tbl;
sam_defaults_t	*defaults;
dev_ent_t	*un;

#define	FIX_ARGS  5

/*
 * sony_helper - issue requests to the Sony API
 * called:
 *    shmid   - shared memory id *
 *    eq      - equipment number of robot
 *
 *    takes messages from the robot private message as a string of
 *    comma seperated arguments to the helper.
 *
 *    command - command to issue
 *    shmid   - shared memory id
 *    sequ    - sequence number
 *    eq      - equipment number of robot
 *    event   - address of event(to pass back)
 *    varies by command.
 *
 * exits - with a 0, all ok response sent via message to robot
 */

int
main(int argc, char **argv)
{
	equ_t	equ;
	char	logname[20];


	if (argc == 3 && strcmp(*argv, "sam-sony_helper") == 0) {
		char	*l_mess;
		char	*line;
		char	*line2 = (char *)malloc(512);
		sony_priv_mess_t  *priv_message;

		argv++;
		master_shm.shmid = atoi(*argv++);
		equ = atoi(*argv++);

		if ((master_shm.shared_memory =
		    shmat(master_shm.shmid, NULL, 0664)) == (void *)-1)
			exit(errno);

		shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
		defaults = GetDefaults();
		dev_ptr_tbl = (dev_ptr_tbl_t *)
		    SHM_REF_ADDR(shm_ptr_tbl->dev_table);

		sprintf(logname, "sonyhlp-%d", equ);
		openlog(logname, LOG_PID | LOG_NOWAIT, defaults->log_facility);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "start helper daemon.");

		if (sizeof (sam_message_t) < sizeof (sony_resp_api_t)) {
			sam_syslog(LOG_CRIT, "sizeof message area(%d) is less"
			    " than sizeof sony_resp(%d).",
			    sizeof (sam_message_t),
			    sizeof (sony_resp_api_t));
			exit(EINVAL);
		}

		/* LINTED pointer cast may result in improper alignment */
		un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[equ]);
		l_mess = un->dis_mes[DIS_MES_NORM];
		priv_message = (sony_priv_mess_t *)
		    SHM_REF_ADDR(un->dt.rb.private);
		sigignore(SIGCHLD);
		memset(line2, '\0', 512);
		mutex_lock(&priv_message->mutex);

		for (;;) {
			int	l_argc = 0;
			int	len;
			pid_t	pid;
			char	*l_args[FIX_ARGS + 6], *path;

			while (priv_message->mtype == SONY_PRIV_VOID)
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
					    DIS_MES_LEN-13);
				}
			} else {
				for (l_argc = 0; l_argc < (FIX_ARGS + 6);
				    l_argc++) {

					if ((l_args[l_argc] =
					    strtok(NULL, ", ")) == NULL)
						break;
#if defined(DEBUG)
					else if (DBG_LVL_EQ(SAM_DBG_DEBUG |
					    SAM_DBG_TMESG))

						sam_syslog(LOG_INFO,
						    "arg %d, %s", l_argc,
						    l_args[l_argc]);
#endif
				}

				if (l_argc == (FIX_ARGS + 6) ||
				    (l_argc < FIX_ARGS)) {

					sam_syslog(LOG_INFO, "Bad message %s.",
					    line);
					if (DBG_LVL(SAM_DBG_DEBUG)) {
						sprintf(l_mess, "bad message ");
						memccpy(l_mess + 12, line,
						    '\0', DIS_MES_LEN-13);
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
							/*
							 * N.B. Poor indenta-
							 * ion to meet cstyle
							 */
						if (DBG_LVL(SAM_DBG_DEBUG)) {
							sam_syslog(LOG_DEBUG,
							"Helper start:%s:%m.",
							    line);

							memccpy(l_mess,
							"helper start failed",
							    '\0', DIS_MES_LEN);
						} else
							sam_syslog(LOG_DEBUG,
							"Helper start:%m.");

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
			priv_message->mtype = SONY_PRIV_VOID;
			cond_signal(&priv_message->cond_i);
		}
	/* NOTREACHED */
	/* exit(0); */
	}
	doit(argc, argv);
	_exit(0);
}

void
doit(int argc, char **argv)
{
	int	i, sequence;
	int	ConnectId, DriveBinNo;
	int	Count = 1, ResponseRecvd = FALSE;
	long	UserId;
	unsigned long	LastError;
	equ_t	equ;
	char	lg_mess[200];
	char	*cmd;
	char	*ServerName;
	char	logname[20];
	time_t	start;
	char	CassetteId[PSCCASSIDLEN+1];
	sony_resp_api_t		response;
	struct _PSCCASSINFO	CassInfo;

	memset(lg_mess, '\0', 200);
	sprintf(lg_mess, "start:");
	for (i = 0; i < argc; i++) {
		if (*(argv + i) != NULL)
			sprintf(&lg_mess[strlen(lg_mess)], ", %s", *(argv + i));
		else
			sprintf(&lg_mess[strlen(lg_mess)], ", missing");
	}

	memset(&response, 0, sizeof (sony_resp_api_t));
	if (argc < FIX_ARGS)
		exit(EINVAL);
	cmd = *argv++;
	master_shm.shmid = atoi(*argv++);
	sequence = atoi(*argv++);
	equ = atoi(*argv++);
	response.event = (robo_event_t *)strtoul(*argv++, NULL, 16);

#if !defined(CALLDIRECT)
	if ((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774)) ==
	    (void *)-1)
		exit(errno);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	defaults = GetDefaults();
	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	/* LINTED pointer cast may result in improper alignment */
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[equ]);
	sprintf(logname, "sonyhlp-%d", equ);
	openlog(logname, LOG_PID | LOG_NOWAIT, defaults->log_facility);
#endif

	if (DBG_LVL_EQ(SAM_DBG_DEBUG | SAM_DBG_EVENT | SAM_DBG_TMESG))
		sam_syslog(LOG_DEBUG, "start:%s(%d)-%d, %s.",
		    cmd, sequence, argc, lg_mess);

	ServerName = *argv++;
	UserId = strtoul(*argv++, NULL, 10);

	if (DBG_LVL_EQ(SAM_DBG_DEBUG | SAM_DBG_EVENT | SAM_DBG_TMESG)) {
		sam_syslog(LOG_DEBUG, "start:%s,%s(%d)-%d, %s.",
		    ServerName, cmd, sequence, argc, lg_mess);
	}

	ConnectId = PscInitialize(ServerName, UserId,
	    PSC_RECVMODE_WAIT, &LastError);

	if (ConnectId < 0) {
		sam_syslog(LOG_DEBUG, "PscInitialize: [%08x]%s", LastError,
		    PscGetErrorMessage(LastError));
		response.api_status = LastError;
		ResponseRecvd = TRUE;
		goto done;
	}

	response.api_status = 0;
	time(&start);
	if (strcmp(cmd, "mount") == 0) {
		if (argc != (FIX_ARGS + 4)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			response.api_status = EINVAL;
			ResponseRecvd = TRUE;
		} else {
			memset(&CassInfo, 0, sizeof (struct _PSCCASSINFO));
			strcpy(CassetteId, *argv++);
			/*
			 * Change any '_' back to ' '
			 */
			for (i = 0; i <= PSCCASSIDLEN; i++) {
				if (CassetteId[i] == '_') CassetteId[i] = ' ';
			}

			DriveBinNo = strtoul(*argv++, NULL, 10);
			if (PscGetCassetteInfo(ConnectId, CassetteId,
			    &CassInfo) < 0) {

				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscGetLastError: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			}
			if (PscMoveCassette(ConnectId, CassInfo.wBinNo,
			    DriveBinNo, PSC_EXEC_DEFAULT) < 0) {
				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscMoveCassette: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			} else {
				ResponseRecvd = TRUE;
				response.api_status = PSCERR_NO_ERROR;
			}
		}
	} else if (strcmp(cmd, "dismount") == 0) {
		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			response.api_status = EINVAL;
			ResponseRecvd = TRUE;
		} else {
			DriveBinNo = strtoul(*argv, NULL, 10);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "dismount(%d), drive(%d):",
				    sequence, DriveBinNo);

			if (PscMoveCassette(ConnectId, DriveBinNo,
			    PSC_AUTO_HOME, PSC_EXEC_DEFAULT) < 0) {

				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscMoveCassette: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			} else {
				ResponseRecvd = TRUE;
				response.api_status = PSCERR_NO_ERROR;
			}
		}
	} else if (strcmp(cmd, "force") == 0) {
		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			response.api_status = LastError;
			ResponseRecvd = TRUE;
		} else {
			DriveBinNo = strtoul(*argv, NULL, 10);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "force(%d)-isu:%d.",
				    sequence, DriveBinNo);
			if (PscMoveCassette(ConnectId, DriveBinNo,
			    PSC_AUTO_HOME, PSC_EXEC_DEFAULT) < 0) {

				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscMoveCassette: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			} else {
				ResponseRecvd = TRUE;
				response.api_status = PSCERR_NO_ERROR;
			}
		}
	} else if (strcmp(cmd, "view") == 0) {
		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			response.api_status = EINVAL;
			ResponseRecvd = TRUE;
		} else {
			memset(&CassInfo, 0, sizeof (struct _PSCCASSINFO));
			strcpy(CassetteId, *argv++);
			/*
			 * Change any '_' back to ' '
			 */
			for (i = 0; i <= PSCCASSIDLEN; i++) {
				if (CassetteId[i] == '_') CassetteId[i] = ' ';
			}

			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "view:vol(%s)",
				    CassetteId);

			if (PscGetCassetteInfo(ConnectId, CassetteId,
			    &CassInfo) < 0) {

				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscGetLastError: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			} else {
				ResponseRecvd = TRUE;
				response.api_status = PSCERR_NO_ERROR;
			}
		}
	} else if (strcmp(cmd, "query_drv") == 0) {
		struct	_PSCBINSTATUS BinStatus;
		int	Condition;
		BYTE	BinType;

		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).", cmd,
				    sequence, argc);
			response.api_status = EINVAL;
			ResponseRecvd = TRUE;
		} else {
			BinStatus.lBinCount = 1;
			BinType = PSC_BINTYPE_DRIVE;
			Condition = PSC_BINCOND_ALL;

			DriveBinNo = strtoul(*argv, NULL, 10);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "query_drv: drive(%d)",
				    DriveBinNo);
			if (PscGetBinStatus(ConnectId, BinType, DriveBinNo,
			    Count, Condition, &BinStatus) < 0) {
				LastError = PscGetLastError(ConnectId);
				sam_syslog(LOG_DEBUG,
				    "PscGetBinStatus: [%08x]%s", LastError,
				    PscGetErrorMessage(LastError));
				response.api_status = LastError;
				ResponseRecvd = TRUE;
			} else {
				ResponseRecvd = TRUE;
				response.api_status = PSCERR_NO_ERROR;
				memcpy((void *)&(response.data.BinInfo),
				    (void *)&(BinStatus.BinInfo[0]),
				    sizeof (PscBinInfo_t));
			}
		}
	} else {
		ResponseRecvd = TRUE;
		response.api_status = EINVAL;
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_ERR, "%s(%d): error on sony_helper call,"
			    " wrong arguments(%d).",
			    cmd, sequence, argc);
	}

	PscUninitialize(ConnectId);

	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "%s(%d): Returns:  %x.", cmd, sequence,
		    response.api_status);

done:

	if ((un->state < DEV_IDLE) && un->status.b.present) {
		message_request_t  *message;

		mutex_lock(&un->mutex);
		un->active++;
		message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		if (DBG_LVL(SAM_DBG_TMESG))
			sam_syslog(LOG_DEBUG, "PM: %s(%d) for %#x.",
			    cmd, sequence, response.event);
		message->message.magic = MESSAGE_MAGIC;
		/* a harmless command */
		message->message.command = MESS_CMD_ACK;
		message->mtype = MESS_MT_APIHELP;
		memcpy((void *)&message->message.param.start_of_request,
		    (void *)&response, sizeof (sony_resp_api_t));
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
}
