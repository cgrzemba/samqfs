/* grau_helper.c - program to issue commands for the grau api */

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
#include <stdio.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/mman.h>

#define	MAIN
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "sam/devinfo.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/defaults.h"
#include "sam/devnm.h"
#include "api_errors.h"
#include "generic.h"

#pragma ident "$Revision: 1.28 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#define	DO_LOG_DEBUG


/* For version 1.30 of DAS */
#pragma weak aci_qvolsrange
void	    doit(int, char **);

/*	globals */
char	   *dbg_tty;
char	   *program_name;
shm_alloc_t	master_shm;
dev_ent_t	*un;
dev_ptr_tbl_t  *dev_ptr_tbl;
shm_ptr_tbl_t  *shm_ptr_tbl;
sam_defaults_t *defaults;


/*
 *	grau_helper - issue requests to the grau api.
 *
 * called:
 *		shmid	- shared memory id *
 *		eq		- equipment number of robot
 *
 *	  takes messages from the robot private message as a string of
 *	  comma seperated arguments to the helper.
 *
 *		command - command to issue
 *		debug	- pathname to debug tty "/dev/null" if none
 *		shmid	- shared memory id
 *		sequ	- sequence number
 *		eq		- equipment number of robot
 *		event	- address of event(to pass back)
 *		varies by command.
 *
 */
#if defined(TRACE_ACTIVES)
int
InC_AcTiVe(char *name, int line, dev_ent_t *un)
{
	if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG, "inc_active from %d
		    for un->eq % d(%s:%d) \ n ",
		    un->active, un->eq, name, line);
	return (un->active++);
}
#endif				/* TRACE_ACTIVES */

#if defined(DO_LOG_DEBUG)
int		Helper_log_fd = -1;
char	    Log_message[500];
#define	LOGIT(_s_, _t_) if (Helper_log_fd >= 0) { \
	sprintf(Log_message, _s_, _t_); \
	Log_message[499] = (char)0; \
	write(Helper_log_fd, Log_message, strlen(Log_message)); \
	}
#else
#define	LOGIT(_s_, _t_);
#endif				/* DO_LOG_DEBUG */


#define	FIX_ARGS	  6
#define	NUMBER_OF_HELPER_ARGS (FIX_ARGS + 6)

int
main(
	int argc,
	char **argv)
{
	char	   *ent_pnt = "main";
	char	    logname[20];
	equ_t	   equ;
#if defined(DO_LOG_DEBUG)
	char	    name[40];
#endif

	if (argc == 3 && strcmp(*argv, "sam-grau_helper") == 0) {
		/* main helper */
		char	   *l_mess;
		char	   *line;
		char	   *line2 = (char *)malloc(512);
		api_priv_mess_t *priv_message;

		argv++;
		master_shm.shmid = atoi(*argv++);
		equ = (equ_t)atoi(*argv++);


		if ((master_shm.shared_memory
		    = shmat(master_shm.shmid, NULL, 0664)) == (void *) -1)
			exit(errno);

		shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
		defaults = GetDefaults();
		dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
		    shm_ptr_tbl->dev_table);

		sprintf(logname, "grauhlp-%d", (int)equ);
		program_name = logname;
#if defined(DEBUG)
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG, "start helper daemon.");
#endif
#if defined(DO_LOG_DEBUG)
		sprintf(name, SAM_VARIABLE_PATH "/.grau/graulog-%d", (int)equ);
		Helper_log_fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0744);
		{
			time_t	  xtime;
			char	    buff[100];
			time(&xtime);
			LOGIT("\n\n%sStart ++++++++++++++++++\n",
			    asctime_r(localtime(&xtime), buff, 100))
		}
#endif

		if (sizeof (sam_message_t) < sizeof (api_resp_api_t)) {
	sam_syslog(LOG_CRIT,
	    "size of message area(%d) is less than size of grau_resp(%d).",
	    sizeof (sam_message_t), sizeof (api_resp_api_t));
		exit(EINVAL);
		}
		/* LINTED pointer cast may result in improper alignment */
		un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[(int)equ]);

		{
			char	    aciver[200], dasver[200];

			memset(aciver, 0, 200);
			memset(dasver, 0, 200);
			aci_qversion(aciver, dasver);
			sam_syslog(LOG_DEBUG,
			    "GRAU ACI Version '%s' DAS Version '%s' \n ",
			    aciver, dasver);
		}

		l_mess = un->dis_mes[DIS_MES_NORM];
		priv_message = (api_priv_mess_t *)SHM_REF_ADDR(
		    un->dt.rb.private);
		sigignore(SIGCHLD);
		memset(line2, '\0', 512);

		mutex_lock(&priv_message->mutex);
		for (;;) {
			int		l_argc = 0;
			int		len;
			pid_t	   pid;
			char	   *l_args[NUMBER_OF_HELPER_ARGS], *path;

			while (priv_message->mtype == API_PRIV_VOID)
				cond_wait(&priv_message->cond_r,
				    &priv_message->mutex);

			memccpy(line2, priv_message->message, '\0',
			    (len = strlen(priv_message->message) + 2));

			line = priv_message->message;
			if (DBG_LVL(SAM_DBG_TMESG))
				sam_syslog(LOG_DEBUG,
				    "GRAU Helper Command:'%s'.", line);
				LOGIT("%s\n", line)
				if ((path = strtok(line2, ", ")) == NULL) {
				sam_syslog(LOG_ERR, "%s: Bad message %s.",
				    ent_pnt, line);
					sprintf(l_mess, "bad message ");
					memccpy(l_mess + 12, line, '\0',
					    DIS_MES_LEN - 13);
				} else {
				for (l_argc = 0; l_argc < NUMBER_OF_HELPER_ARGS;
				    l_argc++) {
					if ((l_args[l_argc] =
					    strtok(NULL, ", ")) == NULL)
						break;
#if defined(DEBUG)
					else if (DBG_LVL_EQ(SAM_DBG_DEBUG |
					    SAM_DBG_TMESG))
						sam_syslog(LOG_INFO,
						    "arg % d, %s ", l_argc,
						    l_args[l_argc]);
#endif
					if (DBG_LVL(SAM_DBG_TMESG))
						sam_syslog(LOG_DEBUG,
						    "%s : arg %d, %s",
						    ent_pnt, l_argc,
						    l_args[l_argc]);
				}

				if (l_argc == NUMBER_OF_HELPER_ARGS ||
				    (l_argc < FIX_ARGS)) {
					sam_syslog(LOG_ERR,
				    "%s: Bad message %s.",
					    ent_pnt, line);
					sprintf(l_mess, "bad message ");
					memccpy(l_mess + 12, line, '\0',
					    DIS_MES_LEN - 13);
				} else {
#if defined(CALLDIRECT)
					doit(l_argc, l_args);
#else
					pid = -1;

		while (pid < 0) {
			int		fd;

			if (DBG_LVL(SAM_DBG_TMESG))
				sam_syslog(LOG_DEBUG,
			    "sam-grau_helper : fork %s\n", path);
					/*
					 * Set non-standard
					 * files to close on
					 * exec.
					 */
				for (fd = STDERR_FILENO + 1;
				    fd < OPEN_MAX; fd++) {
					(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
				}
				if ((pid = fork1()) == 0) {
					execv(path, l_args);
					_exit(0);
				}
				if (pid < 0) {
					int	l_err = errno;
					sam_syslog(LOG_DEBUG,
					    "%s: fork: %s:%m.",
					    ent_pnt, line);
#if defined(DEBUG)
					memccpy(l_mess,
					    "helper start failed",
					    '\0', DIS_MES_LEN);
#endif
					if (l_err == EAGAIN || l_err == ENOMEM)
						sleep(4);
					else
						break;
				}
			}
#endif				/* CALLDIRECT */
			}
		}

			memset(priv_message->message, '\0', len + 1);
			priv_message->mtype = API_PRIV_VOID;
			cond_signal(&priv_message->cond_i);
		}
		/* NOTREACHED */
	}
	doit(argc, argv);
	_exit(0);
}


void
doit(int argc,
	char **argv)
{
	int		sequence, i;
	char	   *cmd, *drive_name, *vol_ser;
	char	    lg_mess[200];
	equ_t	   equ;
	aci_media_t	media;
	api_resp_api_t  response;
	char	    logname[20];
#if defined(DO_LOG_DEBUG)
	char	    name[40];
#endif

	/*
	 * build a debug string, don't know if its needed yet, but this is a
	 * handy place to do it
	 */
	memset(&response, 0, sizeof (api_resp_api_t));
	memset(lg_mess, '\0', 200);
	sprintf(lg_mess, "start:");
	for (i = 0; i < argc; i++) {
		if (*(argv + i) != NULL)
			sprintf(&lg_mess[strlen(lg_mess)], ", %s", *(argv + i));
		else
			sprintf(&lg_mess[strlen(lg_mess)], ", missing");
	}

	/* pull off the fixed part of the argument list */
	cmd = *argv++;
	dbg_tty = *argv++;
	master_shm.shmid = atoi(*argv++);
	sequence = atoi(*argv++);
	equ = atoi(*argv++);
	response.event = (robo_event_t *)strtoul(*argv++, NULL, 16);
	response.sequence = sequence;


#if !defined(CALLDIRECT)
	if ((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0664)) ==
	    (void *) -1)
		exit(errno);

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	defaults = GetDefaults();
	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);

	sprintf(logname, "grauhlp-%d", equ);
	program_name = logname;
	un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[equ]);
#endif
#if defined(DO_LOG_DEBUG)
	sprintf(name, SAM_VARIABLE_PATH "/.grau/graulog-%d", (int)equ);
	Helper_log_fd = open(name, O_WRONLY | O_CREAT | O_APPEND, 0744);
#endif

	if (DBG_LVL_EQ(SAM_DBG_DEBUG | SAM_DBG_EVENT | SAM_DBG_TMESG))
		sam_syslog(LOG_DEBUG, "start:%s(%d)-%d, %s.",
		    cmd, sequence, argc, lg_mess);

	if (sizeof (sam_message_t) < sizeof (api_resp_api_t)) {
		sam_syslog(LOG_CRIT,
		"sizeof message area(%d) is less than sizeof grau_resp(%d).",
		    sizeof (sam_message_t), sizeof (api_resp_api_t));
		LOGIT("sizeof message area %d is too small\n",
		    sizeof (sam_message_t))
			response.api_status = EINVAL;
		goto done;
	}
	/* LINTED pointer cast may result in improper alignment */
	response.d_errno = 0;
	if (strcmp(cmd, "mount") == 0) {
		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("mount wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			vol_ser = *argv++;
			media = (aci_media_t)atoi(*argv++);
			drive_name = *argv;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "mount(%d):%s, %s, %d.",
				    sequence, drive_name, vol_ser, media);

			if ((response.api_status = aci_mount(
			    vol_ser, media, drive_name))) {
				response.d_errno = d_errno;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "mount(%d): error, %d, d_errno %d.",
					    sequence, response.api_status,
					    d_errno);
			}
		}
	} else if (strcmp(cmd, "dismount") == 0) {
		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("dismount wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			vol_ser = *argv++;
			media = (aci_media_t)atoi(*argv++);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "dismount(%d)-isu:%s, %d",
				    sequence, vol_ser, media);

		if ((response.api_status = aci_dismount(
		    vol_ser, media))) {
			response.d_errno = d_errno;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "dismount(%d): error, %d, d_errno %d.",
				    sequence, response.api_status, d_errno);
		}
	}
	} else if (strcmp(cmd, "force") == 0) {
		if (argc != (FIX_ARGS + 1)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("force wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			drive_name = *argv;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				"force(%d)-isu:%s.", sequence, drive_name);

			if ((response.api_status = aci_force(drive_name))) {
				response.d_errno = d_errno;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "force(%d): error, %d, d_errno %d.",
					    sequence, response.api_status,
					    d_errno);
			}
		}
	} else if (strcmp(cmd, "view") == 0) {
		if (argc != (FIX_ARGS + 3)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("view wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			char *api_client;
			char save_vol_ser[ACI_VOLSER_LEN + 2];

			/*
			 * the aci_view function is broken at das release
			 * 1.30C emass says to use the qvolsrange function
			 * (new to release 1.3). Attempt to make this
			 * transparent to the site. qvolsrange() clears this
			 * on error
			 */
			vol_ser = *argv++;
			strncpy(save_vol_ser, vol_ser, ACI_VOLSER_LEN);
			save_vol_ser[ACI_VOLSER_LEN] = (char)0;
			media = (aci_media_t)atoi(*argv++);
			api_client = *argv;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "argv client = '%s'\n", api_client);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG, "view(%d)-isu:%s, %d.",
				    sequence, vol_ser, media);

			d_errno = 0;
			response.api_status = aci_view(vol_ser, media,
			    &response.data.vol_des);
			if (response.api_status || d_errno ||
			    response.data.vol_des.volser[0] == '\0') {
				response.api_status = -1;
				response.d_errno = d_errno;
				if (DBG_LVL(SAM_DBG_DEBUG))
					sam_syslog(LOG_DEBUG,
					    "view(%d): error, %d, d_errno %d.",
					    sequence, response.api_status,
					    d_errno);
			}
		}
	} else if (strcmp(cmd, "driveaccess") == 0) {
		char	   *api_client;

		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("driveaccess wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			api_client = *argv++;
			drive_name = *argv++;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "driveaccess(%d)-isu:%s, %s.",
				    sequence, drive_name, api_client);

			if ((response.api_status =
			    aci_driveaccess(api_client, drive_name,
			    ACI_DRIVE_FUP))) {
				response.d_errno = d_errno;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "driveaccess(%d): error, %d, d_errno %d.",
				    sequence, response.api_status, d_errno);
			}
		}
	} else if (strcmp(cmd, "getside") == 0) {
		char	   *p;
		double	  conv;
		char	    aciver[200], dasver[200];

		memset(aciver, 0, 200);
		memset(dasver, 0, 200);
		aci_qversion(aciver, dasver);
		conv = strtod(aciver, &p);
		if (conv < 3) {
	syslog(LOG_CRIT,
	    "ACI Version does not support query of 2nd side of imported media");
			response.api_status = EINVAL;
			goto done;
		}
		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("getside wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			char	   *api_client;
			struct aci_sideinfo (sideinfo)[2];

			api_client = *argv++;
			vol_ser = *argv;
			if (DBG_LVL(SAM_DBG_DEBUG))
				syslog(LOG_DEBUG,
				    "getside(%d)-isu:%s.",
				    sequence, vol_ser);

			memset(sideinfo[0].szVolser, 0, ACI_VOLSER_LEN);
			memset(sideinfo[1].szVolser, 0, ACI_VOLSER_LEN);
			if ((response.api_status =
			    aci_getvolsertoside(vol_ser, &sideinfo))) {
				response.d_errno = d_errno;
				if (DBG_LVL(SAM_DBG_DEBUG))
				syslog(LOG_DEBUG,
				    "getside(%d): error, %d, d_errno %d.",
				    sequence, response.api_status,
				    d_errno);
			} else {
				if (strcmp(vol_ser, sideinfo[0].szVolser) != 0)
					memcpy(&response.data.vol_des.volser[0],
					    sideinfo[0].szVolser,
					    ACI_VOLSER_LEN);
				else
					memcpy(&response.data.vol_des.volser[0],
					    sideinfo[1].szVolser,
					    ACI_VOLSER_LEN);
				response.d_errno = d_errno;
			}
		}
	} else if (strcmp(cmd, "querydrive") == 0) {
		char	   *api_client;
		struct aci_drive_entry *drive_entry[ACI_MAX_DRIVE_ENTRIES];

		if (argc != (FIX_ARGS + 2)) {
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "%s(%d): wrong arguments(%d).",
				    cmd, sequence, argc);
			LOGIT("querydrive wrong args %d\n", argc)
				response.api_status = EINVAL;
		} else {
			api_client = *argv++;
			drive_name = *argv++;
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "querydrive(%d):%s, %s.", sequence,
				    drive_name, api_client);

			if ((response.api_status =
			    aci_drivestatus(api_client, drive_entry)) == 0) {
				response.d_errno = d_errno;
				for (i = 0; i < ACI_MAX_DRIVE_ENTRIES; i++) {
					/* out of entries */
					if ('\0' == *drive_entry[i]->drive_name)
						break;
					if (strcmp(drive_name,
					    drive_entry[i]->drive_name) == 0) {
						/*
						 * For some reason,
						 * response.data.drive_ent
						 * and drive_entry[i] are
						 * laid out differently.
						 * Other than using the
						 * following hardcodes, one
						 * could create a fake
						 * drive_entry[i] structure,
						 * but that would still need
						 * to track
						 * response.data.drive_ent.
						 */
				/* Indentation for cstyle requirements */
				memcpy(response.data.drive_ent.drive_name,
				    drive_entry[i]->drive_name, 10);
				memcpy(response.data.drive_ent.amu_drive_name,
				    &drive_entry[i]->drive_name[10], 3);
				memcpy(&response.data.drive_ent.drive_state,
				    &drive_entry[i]->drive_name[13], 4);
				memcpy(&response.data.drive_ent.type,
				    &drive_entry[i]->drive_name[17], 1);
				memcpy(&response.data.drive_ent.system_id,
				    &drive_entry[i]->drive_name[21], 65);
				memcpy(response.data.drive_ent.clientname,
				    &drive_entry[i]->drive_name[86], 65);
				memcpy(response.data.drive_ent.volser,
				    &drive_entry[i]->drive_name[151], 17);
				memcpy(&response.data.drive_ent.cleaning,
				    &drive_entry[i]->drive_name[168], 4);
				memcpy(&response.data.drive_ent.clean_count,
				    &drive_entry[i]->drive_name[172], 2);
				break;
					}
				}
			}
		}
	} else {
		LOGIT("Unknown command %d\n", __LINE__)
			response.api_status = EINVAL;
	}

done:

	LOGIT("%s ", cmd)
		LOGIT("api_status %d, ", response.api_status)
		LOGIT("d_errno %d\n", d_errno)
		if (DBG_LVL(SAM_DBG_DEBUG))
		sam_syslog(LOG_DEBUG,
		    "%s(%d): Returns:	 %d, %d.",
		    cmd, sequence,
		    response.api_status, d_errno);

	if ((un->state < DEV_IDLE) && un->status.b.present) {
		message_request_t *message;

		mutex_lock(&un->mutex);
		INC_ACTIVE(un);
		message = (message_request_t *)SHM_REF_ADDR(un->dt.rb.message);
		mutex_unlock(&un->mutex);
		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		if (DBG_LVL(SAM_DBG_TMESG))
			sam_syslog(LOG_DEBUG,
			    "PM: %s(%d) for %#x.", cmd, sequence,
			    response.event);
		message->message.magic = MESSAGE_MAGIC;
		/* a harmless command */
		message->message.command = MESS_CMD_ACK;
		message->mtype = MESS_MT_APIHELP;
		memcpy((void *) &message->message.param.start_of_request,
		    (void *) &response, sizeof (api_resp_api_t));
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}
}
