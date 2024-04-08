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

/*
 *	stk.c -  APIs to do ACSLS network attached library ACSAPI calls.
 */
#pragma	ident	"$Revision: 1.46 $"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <procfs.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syslog.h>
#include <pthread.h>
#include <signal.h>
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/sam_trace.h"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
#include "pub/mgmt/device.h"
#include "mgmt/util.h"
#include "mgmt/hash.h"
#include "pub/mgmt/error.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "acssys.h"
#include "acsapi.h"
#include "cl_pub.h"

#include "parser_utils.h"

#include <libgen.h>

#include "mgmt/config/media.h"
#include "aml/proto.h"
#include "acs_media.h"


/*
 * This code represents a ACS client application and communicates with the ACS
 * library via the ACSAPI interface. ACSAPI procedures communicate via IPC
 * with the SSI process running on this same client machine. Each client app
 * can send multiple requests to the ACS Library Manager via this SSI. The SSI
 * receives requests from one or more clients, places them on a queue, and sends
 * the requests to the CSI to relay them to the ACS Library Manager. Multiple
 * heterogeneous clients can communicate and manage the ACSLS Library via the
 * same SSI. The SSI also relays the responses back to the appropriate client
 * application. The CSI and SSI talk to each other via RPC. The same RPC program
 * number is used for all instances of SSI and CSI connections. So there is a
 * limitation that a client cannot connect to multiple ACSLS.
 *
 * The ACSAPI resides on the client machine as a set of three C language library
 * object modules to be linked with a client application. These modules are  the
 * formal interface, and the functions that carry out the IPC for requests and
 * responses.
 *
 */

/* Display configuration and status */
static int acs_display_info(int query_type, char *cmdarg, sqm_lst_t **lst);

static int parse_fmtdata_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_cap_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_drive_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_lsm_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_pool_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_vol_resp(ALIGNED_BYTES buf, sqm_lst_t *fmt_lst,
    sqm_lst_t **lst);

static int parse_f(char *f, char *s);
static int parse_f_byte(char *f, char *s);
static int parse_f_int(char *f, int *i);
static int parse_fmt_resp(char *f, sqm_lst_t **l);
static int fmt_kv_lst(sqm_lst_t *fmt_lst, sqm_lst_t **kv_lst);

typedef enum {
	DISPLAY_CAP		= 0,
	DISPLAY_CELL		= 1,
	DISPLAY_DRIVE		= 2,
	DISPLAY_LOCK		= 3,
	DISPLAY_LSM		= 4,
	DISPLAY_PANEL		= 5,
	DISPLAY_POOL		= 6,
	DISPLAY_VOL		= 7,
	DISPLAY_VOL_BY_MEDIA	= 8,
	DISPLAY_VOL_CLEANING	= 9,
	DISPLAY_VOL_ACCESSED	= 10,
	DISPLAY_VOL_ENTERED	= 11,
	DISPLAY_UNSUPPORTED	= 12
} query_type_t;

#define	XMLREQ_CAP		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>cap</token><token>%s</token>" \
	"<token>-f</token><token>acs</token><token>lsm</token>" \
	"<token>cap</token>" \
	"</display></request>"

#define	XMLREQ_CELL		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>cell</token><token>%s</token>" \
	"<token>-f</token><token>status</token>" \
	"</display></request>"

#define	XMLREQ_DRIVE		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>drive</token><token>%s</token>" \
	"<token>-f</token><token>status</token><token>state</token>" \
	"<token>volume</token><token>type</token><token>lock</token>" \
	"<token>serial_num</token><token>condition</token>" \
	"</display></request>"

#define	XMLREQ_LOCK		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>lock</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_LSM		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>lsm</token><token>%s</token>" \
	"<token>-f</token><token>status</token><token>state</token>" \
	"<token>free_cells</token><token>type</token>" \
	"<token>serial_num</token>" \
	"<token>condition</token><token>door_status</token>" \
	"</display></request>"

#define	XMLREQ_PANEL		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>panel</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_POOL		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>pool</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_VOL		"<request type='DISPLAY'><display>" \
	"<token>display</token><token>volume</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_VOL_BY_MEDIA	"<request type='DISPLAY'><display>" \
	"<token>display</token><token>volume</token><token>*</token>" \
	"<token>-media</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_VOL_CLEANING	"<request type='DISPLAY'><display>" \
	"<token>display</token><token>volume</token><token>%s</token>" \
	"<token>-clean</token>" \
	"</display></request>"

#define	XMLREQ_VOL_ACCESSED	"<request type='DISPLAY'><display>" \
	"<token>display</token><token>volume</token><token>*</token>" \
	"<token>-access</token><token>%s</token>" \
	"</display></request>"

#define	XMLREQ_VOL_ENTERED	"<request type='DISPLAY'><display>" \
	"<token>display</token><token>volume</token><token>*</token>" \
	"<token>-entry</token><token>%s</token>" \
	"</display></request>"

typedef struct query_cmdresp_s {
	int query_type;
	char *xmlreq;
	int (*parse_resp)(ALIGNED_BYTES, sqm_lst_t *, sqm_lst_t **);
} query_cmdresp_t;

static query_cmdresp_t query_cmdresp_tbl[] = {
	{DISPLAY_CAP,		XMLREQ_CAP,		parse_cap_resp},
	{DISPLAY_CELL,		XMLREQ_CELL,		NULL}, // future
	{DISPLAY_DRIVE,		XMLREQ_DRIVE,		parse_drive_resp},
	{DISPLAY_LOCK,		XMLREQ_LOCK,		NULL}, // future
	{DISPLAY_LSM,		XMLREQ_LSM,		parse_lsm_resp},
	{DISPLAY_PANEL,		XMLREQ_PANEL,		parse_fmtdata_resp},
	{DISPLAY_POOL,		XMLREQ_POOL,		parse_pool_resp},
	{DISPLAY_VOL,		XMLREQ_VOL,		parse_vol_resp},
	{DISPLAY_VOL_BY_MEDIA,	XMLREQ_VOL_BY_MEDIA,	parse_vol_resp},
	{DISPLAY_VOL_CLEANING,	XMLREQ_VOL_CLEANING,	parse_vol_resp},
	{DISPLAY_VOL_ACCESSED,	XMLREQ_VOL_ACCESSED,	parse_vol_resp},
	{DISPLAY_VOL_ENTERED,	XMLREQ_VOL_ENTERED,	parse_vol_resp},
	{DISPLAY_UNSUPPORTED,	"",			NULL}
};


/*
 * Interfaces to control the lifecycle of the SSI process and its children
 *
 * This should be in the common library used by SAM, SMMS and fsmgmtd
 *
 * SAM or any other application (client of ACSLS) can start the SSI process. The
 * same process is to be used by all ACS clients on this machine to communicate
 * with the ACSLS library. Do not start multiple SSI process. To check if SSI is
 * running on the client machine, check for /opt/SUNWsamfs/sbin/ssi_so in the
 * process table. The environment variable ACSAPI_SSI_SOCKET is the IPC location
 * of the SSI
 *
 * duplicate start_ssi in src/robots/stk/init.c
 */

/*
 * start the stk daemon for the given stk configuration
 * The host and port number are given in stk_host_info
 */
int
start_stk_daemon(
stk_host_info_t *stk_host_info)
{

	char	stk_cmd[] = SCRIPT_DIR"/ssi.sh";
	sqm_lst_t	*proclist;

	/* envvar length = MAXHOSTNAMELEN + length of envvar + '=' + nul */
	char env_stk_hostname[MAXHOSTNAMELEN + (strlen(ACS_HOSTNAME)) + 2];
	char env_stk_port_num[128];
	char ssi_pid[129];
	pid_t pid_num;
	pid_t pid = -1;
	FILE *file_ptr;
	int fd;
	int checks;
	int status;

	char *env_hostname;
	char *env_portnum;

	Trace(TR_OPRMSG, "Entering stk ssi daemon startup");

	/*
	 * Check to see if the ssi_so daemon is already running. If so
	 * do not start another.
	 */
	if (find_process("ssi_so", &proclist) == 0) {
		if ((proclist != NULL) && (proclist->length > 0)) {

			lst_free_deep(proclist);
			Trace(TR_MISC, "ssi is already running");

			return (0);
		}
		lst_free_deep(proclist);
	}

	checks = 0;
	while (pid < 0) {
		if ((pid = fork1()) == 0) {

			char    shmid[12], equ[12], pshmid[12];

			/* Close open file descriptors > stderr */
			closefrom(3);

			/* Setup the needed environment variables */
			snprintf(env_stk_hostname, sizeof (env_stk_hostname),
			    "CSI_HOSTNAME=%s", stk_host_info->hostname);
			putenv(env_stk_hostname);
			snprintf(env_stk_port_num, sizeof (env_stk_port_num),
			    "ACSAPI_SSI_SOCKET=%s", stk_host_info->portnum);
			putenv(env_stk_port_num);

			strlcpy(shmid, "100", sizeof (shmid));
			strlcpy(pshmid, "200", sizeof (pshmid));
			pid_num = getpid();
			/*
			 *	Write pid id to a file so that
			 *	RPC daemon could cleanup if necessary.
			 */
			file_ptr = NULL;
			if ((fd = open(SSI_PID_FILE, O_WRONLY|O_CREAT|O_TRUNC,
			    0644)) != -1) {
				file_ptr = fdopen(fd, "w");
			}
			if (file_ptr == NULL) {
				samerrno = SE_FILE_APPEND_OPEN_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_FILE_APPEND_OPEN_FAILED),
				    ssi_pid);
				Trace(TR_ERR, "%s", samerrmsg);
				return (-1);
			}
			fprintf(file_ptr, "%d\n", pid_num);
			fprintf(file_ptr, "CSI_HOSTNAME=%s\n",
			    stk_host_info->hostname);
			fprintf(file_ptr, "ACSAPI_SSI_SOCKET=%s\n",
			    stk_host_info->portnum);
			fclose(file_ptr);
			snprintf(equ, sizeof (equ), "%d", pid_num);
			Trace(TR_OPRMSG, "Finished stk ssi daemon startup");
			execl(stk_cmd, "ssi.sh", "100", "200", equ, NULL);
			return (0);
		}

		/* Failure to launch. Don't hang around forever trying */
		if (pid < 0 && checks >= 5) {
			samerrno = SE_PID_FORK_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_PID_FORK_FAILED));
			Trace(TR_PROC, "%s", samerrmsg);
			return (-1);
		}
		checks ++;
		sleep(5);

	}

	/*
	 * Under normal conditions the ssi_so daemon will not exit.
	 * However, call waitpid several times in case there was an
	 * error on execution. This will avoid defunct process
	 * buildup if discovery is executed multiple times in the
	 * face of failures.
	 *
	 * In a non error case this code will leave a single defunct
	 * process hanging around if the ssi_so daemon goes down or
	 * is restarted by external means.
	 */
	checks = 0;
	while (waitpid(pid, &status, WNOHANG) == 0 && ++checks < 5) {
		sleep(1);
	}

	/*
	 * If the process terminated with non zero status then
	 * trace a message and return an error.
	 */
	if (WIFEXITED(status) && WEXITSTATUS(status)) {
		Trace(TR_ERR, "child ssi_so %d exited with status %d",
		    pid, WEXITSTATUS(status));
		samerrno = SE_ACS_START_CLIENT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}


	set_stk_env(stk_host_info);
	Trace(TR_OPRMSG, "Finished stk ssi daemon startup");
	return (0);
}


/*
 * get volume list by media type
 */
int
get_volume_list_by_media(
devtype_t equ_type,		/* INPUT */
stk_host_info_t *stk_host_info,	/* INPUT */
sqm_lst_t **stk_volume_list)	/* OUTPUT */
{

	sqm_lst_t *vollst		= NULL;
	stk_volume_t *vol		= NULL;
	node_t *node		= NULL;
	node_t *next		= NULL;
	mtype_t sam_mtype	= {0};

	Trace(TR_OPRMSG, "get volume list by media");

	if (ISNULL(stk_host_info, stk_volume_list)) {
		Trace(TR_ERR, "get volume list by media failed: %s", samerrmsg);
		return (-1);
	}

	if (acs_display_info(DISPLAY_VOL, NULL, &vollst) != 0) {

		Trace(TR_ERR, "get volume list by media failed: %s", samerrmsg);
		return (-1);
	}

	if (vollst != NULL) {
	node = vollst->head;
	while (node != NULL) {

		vol = (stk_volume_t *)node->data;
		next = node->next;

		if (ISNULL(vol)) {
			continue;
		}

		if (stkmtype2sammtype(vol->media_type, sam_mtype) != 0) {
			/*
			 * media type mapping failed. Remove this volume
			 * from the results.
			 */
			free(vol);
			if (lst_remove(vollst, node) != 0) {

				lst_free_deep(vollst);
				return (-1);
			}

			Trace(TR_ERR, "media type conversion failed: %s",
			    samerrmsg);
			sam_mtype[0] = '\0';
			node = next;
			continue;
		}

		if (strncmp(sam_mtype, equ_type, sizeof (sam_mtype)) != 0) {

			free(vol);
			if (lst_remove(vollst, node) != 0) {

				lst_free_deep(vollst);
				return (-1);
			}
		}
		sam_mtype[0] = '\0';
		node = next;
	}
	*stk_volume_list = vollst;
	}
	Trace(TR_OPRMSG, "get volume list by media success");
	return (0);
}

void
set_stk_env(
stk_host_info_t *stk_host_info)
{

	char env_stk_hostname[128];
	char env_stk_port_num[128];
	char env_stk_ssi_host[128];
	char env_stk_access[128];
	char env_stk_ssi_inet_port[128];
	char env_stk_csi_hostport[128];


	snprintf(env_stk_hostname, sizeof (env_stk_hostname),
	    "%s=%s", ACS_HOSTNAME, stk_host_info->hostname);
	putenv(env_stk_hostname);
	snprintf(env_stk_port_num, sizeof (env_stk_port_num),
	    "%s=%s", ACS_PORTNUM, stk_host_info->portnum);
	putenv(env_stk_port_num);
	if (strlen(stk_host_info->access) > 0) {
		snprintf(env_stk_access, sizeof (env_stk_access),
		    "%s=%s", ACS_ACCESS, stk_host_info->access);

		putenv(env_stk_access);
	}
	if (strlen(stk_host_info->ssi_host) > 0) {
		snprintf(env_stk_ssi_host, sizeof (env_stk_ssi_host),
		    "%s=%s", ACS_SSIHOST, stk_host_info->ssi_host);
		putenv(env_stk_ssi_host);
	}
	if (strlen(stk_host_info->ssi_inet_portnum) > 0) {
		snprintf(env_stk_ssi_inet_port, sizeof (env_stk_ssi_inet_port),
		    "%s=%s", ACS_SSI_INET_PORT,
		    stk_host_info->ssi_inet_portnum);
		putenv(env_stk_ssi_inet_port);
	}
	if (strlen(stk_host_info->csi_hostport) > 0) {
		snprintf(env_stk_csi_hostport, sizeof (env_stk_csi_hostport),
		    "%s=%s", ACS_CSI_HOSTPORT, stk_host_info->csi_hostport);
		putenv(env_stk_csi_hostport);
	}

}


/* modified to only return the stk_pool_list in stk_phyconf_info */
int
get_stk_phyconf_info(
ctx_t *ctx,				/* ARGSUSED */
stk_host_info_t *stk_host_info,		/* acsls server address */
devtype_t equ_type,			/* NOT USED */
stk_phyconf_info_t **info)
{

	sqm_lst_t *stkpool_lst = NULL;

	Trace(TR_MISC, "get the scratch pools in the acsls stk");

	*info = (stk_phyconf_info_t *)mallocer(sizeof (stk_phyconf_info_t));
	if (*info == NULL) {
		Trace(TR_ERR, "get scratch pools failed: %s", samerrmsg);
		return (-1);
	}
	memset(*info, 0, sizeof (stk_phyconf_info_t));

	if (start_stk_daemon(stk_host_info) != 0) {

		free(*info);

		Trace(TR_ERR, "get scratch pools failed: %s", samerrmsg);
		return (-1);
	}

	/* query scratch pools in the stk acsls library */
	if (acs_display_info(DISPLAY_POOL, NULL, &stkpool_lst) != 0) {

		free(*info);

		Trace(TR_ERR, "get scratch pools failed: %s", samerrmsg);
		return (-1);
	}

	(*info)->stk_pool_list = stkpool_lst;
	Trace(TR_MISC, "get scratch pools success");
	return (0);
}


/*
 *	get a list of vsns after filtering some vsns.
 *	sqm_lst_t **stk_volume_list -	a list of stk_volume_t,
 *					it must be freed by caller.
 *	This API function returns a volume list based on
 *	the filter options given by customer.  The current
 *	filter options include: none, physicaly location,
 *	scratch pool, VSN range, VSN regular expression.
 */
int
get_stk_filter_volume_list(
ctx_t *ctx,				/* ARGSUSED */
stk_host_info_t *stk_host_info,
char *in_str,
sqm_lst_t **stk_volume_list)
{

	int i;
	sqm_lst_t *vol_list;
	sqm_lst_t *range_list = NULL;
	node_t *node_volume, *node_c;
	stk_volume_t *print_volume;
	sqm_lst_t *namevalue_list;
	nw_param_pair_t *namevalue;
	vsn_filter_option_t *filter;

	Trace(TR_OPRMSG, "Entering get_stk_filter_volume_list");
	if (ISNULL(in_str)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "incoming string is %s", in_str);
	filter = (vsn_filter_option_t *)
	    mallocer(sizeof (vsn_filter_option_t));
	if (filter == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	memset(filter, 0,
	    sizeof (vsn_filter_option_t));

	filter->filter_type = NONE;

	i = get_string_namevalue(&namevalue_list, in_str);
	node_c = namevalue_list->head;
	while (node_c != NULL) {
		namevalue = (nw_param_pair_t *)node_c->data;
		if (strcmp(namevalue->name, EQU_TYPE) == 0) {
			strlcpy(filter->equ_type, namevalue->value,
			    sizeof (filter->equ_type));
		}
		if (strcmp(namevalue->name, ACCESS_DATE) == 0) {
			strlcpy(filter->access_date, namevalue->value,
			    sizeof (filter->access_date));
		}
		if (strcmp(namevalue->name, FILTER_TYPE) == 0) {
			filter->filter_type = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, SCRATCH_POOL_ID) == 0) {
			filter->scratch_pool_id = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, START_VSN) == 0) {
			strlcpy(filter->start_vsn, namevalue->value,
			    sizeof (filter->start_vsn));
		}
		if (strcmp(namevalue->name, END_VSN) == 0) {
			strlcpy(filter->end_vsn, namevalue->value,
			    sizeof (filter->end_vsn));
		}
		if (strcmp(namevalue->name, VSN_EXPRESSIONS) == 0) {
			strlcpy(filter->vsn_expression,
			    namevalue->value,
			    sizeof (filter->vsn_expression));
		}
		if (strcmp(namevalue->name, LSM) == 0) {
			filter->lsm = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, PANEL) == 0) {
			filter->panel = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, START_ROW) == 0) {
			filter->start_row = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, END_ROW) == 0) {
			filter->end_row = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, START_COL) == 0) {
			filter->start_col = atoi(namevalue->value);
		}
		if (strcmp(namevalue->name, END_COL) == 0) {
			filter->end_col = atoi(namevalue->value);
		}
		node_c = node_c->next;
	}
	if (start_stk_daemon(stk_host_info) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (get_volume_list_by_media(
	    filter->equ_type, stk_host_info, &vol_list) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}


	/* Apply the filters */
	node_volume = vol_list->head;
	while (node_volume != NULL && filter->filter_type != NONE) {

		print_volume = (stk_volume_t *)node_volume->data;

		if (filter->filter_type == SCRATCH_POOL) {
			if (print_volume->pool_id != filter->scratch_pool_id) {
				if (lst_remove(vol_list, node_volume) != 0) {
					Trace(TR_ERR, "%s", samerrmsg);
					free_list_of_stk_volume(vol_list);
					return (-1);
				}
				free(print_volume);
			}
		} else if (filter->filter_type == VSN_RANGE) {
			int beg_num, end_num;
			int count;
			int str_len, end_str_len;
			vsn_t new_vsn, temp_vsn;
			char *ch;

			/* Create the range list on the first time through */
			if (range_list == NULL) {
				range_list = lst_create();
				if (range_list == NULL) {
					free_list_of_stk_volume(vol_list);
					return (-1);
				}
			}
			beg_num = get_vsn_num(filter->start_vsn, &str_len);
			end_num = get_vsn_num(filter->end_vsn, &end_str_len);
			count = end_num - beg_num;
			strlcpy(new_vsn, filter->start_vsn, sizeof (new_vsn));

			for (i = 0; i <= count; i++) {
				strlcpy(temp_vsn, new_vsn,
				    sizeof (temp_vsn));
				if (strcmp(new_vsn,
				    print_volume->stk_vol) == 0) {


					/*
					 * move the volume to the range list
					 */
					node_volume->data = NULL;
					if (lst_append(range_list,
					    print_volume) != 0) {
						Trace(TR_ERR, "%s", samerrmsg);
						free_list_of_stk_volume(
						    vol_list);
						free_list_of_stk_volume(
						    range_list);
						return (-1);
					}
					break;
				}
				ch = gen_new_vsn(temp_vsn, str_len,
				    beg_num, new_vsn);
				if (ch == NULL) {
					samerrno = SE_VSN_GENERATION_FAILED;
					snprintf(samerrmsg, MAX_MSG_LEN,
					    GetCustMsg(samerrno));
					Trace(TR_ERR, "%s", samerrmsg);
					free_list_of_stk_volume(range_list);
					free_list_of_stk_volume(vol_list);
					return (-1);
				}
				beg_num ++;
			}

		} else if (filter->filter_type == VSN_EXPRESSION) {
			char *reg_ptr, *reg_rtn;
			reg_ptr = regcmp(filter->vsn_expression, NULL);
			if (reg_ptr == NULL) {
				samerrno = SE_GET_REGEXP_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_GET_REGEXP_FAILED),
				    filter->vsn_expression);
				Trace(TR_ERR, "%s", samerrmsg);
				free_list_of_stk_volume(vol_list);
				return (-1);
			}
			reg_rtn = regex(reg_ptr, print_volume->stk_vol);
			if (reg_rtn == NULL) {
				if (lst_remove(vol_list, node_volume) != 0) {
					Trace(TR_ERR, "%s", samerrmsg);
					free_list_of_stk_volume(vol_list);
					return (-1);
				}
				free(print_volume);
			}

		}
		node_volume = node_volume->next;
	}
	if (filter->filter_type == VSN_RANGE) {
		*stk_volume_list = range_list;
		free_list_of_stk_volume(vol_list);
	} else {
		*stk_volume_list = vol_list;
	}
	Trace(TR_OPRMSG, "Finished get_stk_filter_volume_list");
	return (0);

}


/*
 *	support function
 */
static int
vol_in_vol_access_list(
char *member,		/* list member */
sqm_lst_t *list)		/* a list of stk_volume pointer */
{
	node_t	*node;
	stk_volume_t	*temp_volume;

	if (list == NULL) {
		return (-1);
	}
	node = list->head;
	while (node != NULL) {
		temp_volume = (stk_volume_t *)node->data;
		if (strcmp(temp_volume->stk_vol, member) == 0) {
			return (0);
		}
		node = node->next;
	}
	return (-1);
}


/*
 * discover_stk()
 * communicate with the input acsls hostname to get all the drives hosted by
 * the library, then check if the drives have physical access to the client
 * exclude the libraries and drives that are already under the control of SAM
 */
int
discover_stk(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t *stk_host_list,		/* TBD: change from list to a struct */
				/* only a single entry is expected */
sqm_lst_t **stk_library_list)	/* a list of structure library_t */
{
	stk_host_info_t	*stk_host;
	node_t		*node;
	sqm_lst_t	*mcf_paths;

	Trace(TR_MISC, "discovering acsls configuration");

	if (ISNULL(stk_library_list, stk_host_list)) {
		Trace(TR_ERR, "discover stk failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * get the libraries (direct-attached, network-attached) and drives
	 * that have been added to SAM's master config file
	 */
	if (get_paths_in_mcf(PATH_LIBRARY, &mcf_paths) == -1) {

		Trace(TR_ERR, "discover stk failed: %s", samerrmsg);
		return (-1);
	}

	/* ACSLS limitation: client cannot connect to multiple ACSLS */
	node = stk_host_list->head;
	if (node != NULL) {
		stk_host = (stk_host_info_t *)node->data;
	}

	if (get_acs_library_cfg(stk_host, mcf_paths, stk_library_list) != 0) {

		lst_free_deep(mcf_paths);
		Trace(TR_ERR, "discover stk failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "discover stk success");
	return (0);
}


/*
 * Get the configuration of the acs libraries given the name of the
 * acsls host and port.  If no libraries are found and no errors
 * encountered an empty list will be returned.
 */
int
get_acs_library_cfg(
stk_host_info_t *stk_host,
sqm_lst_t *mcf_paths,	/* a list of device paths listed in mcf */
sqm_lst_t **res_lst)	/* RETURN - a list of structure library_t, */
{
	uname_t lib_serial_no = {0}; /* 32 chars */
	sqm_lst_t *stk_lsm_serial_list	= NULL;
	sqm_lst_t *stk_cap_list		= NULL;
	node_t *stk_cap_node;
	stk_cap_t *stk_cap		= NULL;
	hashtable_t *ht_drives		= NULL;
	hashtable_t *ht_stk_devpaths	= NULL;
	stk_param_t *stk_param		= NULL;
	ht_iterator_t *it_drives	= NULL;
	library_t *lib;

	if (ISNULL(res_lst, stk_host, mcf_paths)) {
		Trace(TR_ERR, "get acs library cfg failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get acs library cfg for %s", stk_host->hostname);

	/* set the results to null in case an error is encountered */
	*res_lst = NULL;


	/* check if the acsls host is accessible */
	if (nw_down((char *)stk_host->hostname) != 0) {
		samerrno = SE_RPC_PING_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    (char *)stk_host->hostname);

		Trace(TR_ERR, "get acs library cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* start the stk daemon and set env params */
	if (start_stk_daemon(stk_host) != 0) {
		Trace(TR_ERR, "get acs library cfg failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * SAM interacts with the library via a parameter file, this consists
	 * of the host communication parameters, the CAP info, capacity of the
	 * media supported, and the device path names
	 */

	/* get device path entries */
	if (get_devpath_entries(
	    stk_host, mcf_paths, &ht_stk_devpaths, &ht_drives) != 0) {

		goto error;
	}

	/*
	 * TBD: get the serial number of the library, in 4.6, the serial number
	 * of the LSM was used, there can be multiple LSMs and hence it is not
	 * clear how the LSM serial number can be mapped to a library
	 *
	 * REVISIT > 4.6: what is serial number used for in FSM?
	 */

	/*
	 * query CAP (Cartridge Access Port) to be used for exporting of volumes
	 * The CAPid entry is only used for the -f option on the export command
	 * to designate where the user wants the exported volume to go.
	 *
	 * TBD: Should this be user input, or should the code just select the
	 * first CAPid?
	 */
	if (acs_display_info(DISPLAY_CAP, NULL, &stk_cap_list) != 0) {
		goto error;
	}

	/*
	 * The STK library can be configured with multiple CAPs. However SAM
	 * handles only one CAPid via the parameter file.
	 */
	stk_cap_node = stk_cap_list->head;
	if (stk_cap_node != NULL) {
		stk_cap = (stk_cap_t *)stk_cap_node->data;
		stk_cap_node->data = NULL;
	}
	lst_free_deep(stk_cap_list);

	/*
	 * The capacity entry in stk the parameter file is optional and only
	 * required if the ACS is not returning the correct media type or if
	 * new media types have been added. Also the mechanism of evaluating
	 * capacities from the 'query volume' output in 4.6 is slightly flawed.
	 * Ideally we should get the type of media that can be supported by the
	 * drive types hosted by the library and then get the corresponding
	 * capacity values.
	 */

	*res_lst = lst_create();
	if (*res_lst == NULL) {
		goto error;
	}

	/*
	 * Mixed-media, If a library has mixed media, then it
	 * should be added as multiple parameter file, one for
	 * each media type.
	 * Iterate through the hashtable to get the unique media
	 * types, the length of the ht_drives and h_stk_devpaths
	 * should be equal
	 */
	if ((ht_get_iterator(ht_drives, &it_drives) != 0)) {

		goto error;
	}

	/*
	 * Iterate over the drives building library objects. Remove or
	 * detach data in the hashtables as it is included in the results so
	 * that nothing is referenced from two places and all data can be
	 * safely freed.
	 */
	while (ht_has_next(it_drives)) {
		char *key;
		void *drives;
		void *devpaths;

		/* ht_drives key=equ_type, data=list of drive_t */
		if (ht_detach_next(it_drives, &key, &drives) != 0) {
			goto error;
		}

		if (ISNULL(key)) {
			lst_free_deep_typed(drives, FREEFUNCCAST(free_drive));
			goto error;
		}

		/*
		 * Get the devpaths for this key from ht_stk_devpaths
		 * h_stk_devpaths key=equ_typ,data=list stk_device_t
		 */
		if ((ht_remove(ht_stk_devpaths, key, &devpaths) != 0) ||
		    devpaths == NULL) {
			lst_free_deep_typed(drives, FREEFUNCCAST(free_drive));
			goto error;
		}

		stk_param = (stk_param_t *)mallocer(sizeof (stk_param_t));
		if (ISNULL(stk_param)) {
			lst_free_deep(devpaths);
			lst_free_deep_typed(drives, FREEFUNCCAST(free_drive));
			goto error;
		}
		memset(stk_param, 0, sizeof (stk_param_t));

		stk_param->stk_device_list = (sqm_lst_t *)devpaths;

		/* stk_param->stk_capacity_list = stk_capacity_list; */
		strlcpy(stk_param->hostname,
		    stk_host->hostname, sizeof (uname_t));
		stk_param->portnum = atoi(stk_host->portnum);


		/* Create a library and attach the drives */
		lib = (library_t *)mallocer(sizeof (library_t));
		if (ISNULL(lib)) {
			free_stk_param(stk_param);
			lst_free_deep_typed(drives, FREEFUNCCAST(free_drive));
			goto error;
		}
		memset(lib, 0, sizeof (library_t));
		lib->drive_list = (sqm_lst_t *)drives;
		lib->storage_tek_parameter = stk_param;
		strlcpy(lib->serial_no, lib_serial_no, sizeof (uname_t));
		strlcpy(lib->base_info.equ_type, "sk", sizeof (devtype_t));

		if (lst_append(*res_lst, lib) != 0) {
			free_library(lib);
			goto error;
		}
	}

	free(it_drives);

	/* Deep free the hasttables and any remaining elements */
	ht_list_hash_free_deep(&ht_drives, FREEFUNCCAST(free_drive));
	ht_list_hash_free_deep(&ht_stk_devpaths, FREEFUNCCAST(free));

	Trace(TR_OPRMSG, "finished getting stk_only media information");
	return (0);
error:
	Trace(TR_ERR, "get stk media information failed: %d[%s]",
	    samerrno, samerrmsg);

	/* Free the results and set the return to NULL */
	free_list_of_libraries(*res_lst);
	*res_lst = NULL;

	free(stk_cap);
	free(it_drives);

	/* Deep free the hasttables and any remaining elements */
	ht_list_hash_free_deep(&ht_drives, FREEFUNCCAST(free_drive));
	ht_list_hash_free_deep(&ht_stk_devpaths, FREEFUNCCAST(free));
	return (-1);
}

/*
 * get devpath entries
 *
 * Discover the standalone tape drives not in SAM's control, then
 * query drives from the ACSLS library, match them to return a list
 * of stk_devpaths and (respective) drive_t grouped by media type.
 *
 * A library with mixed-media configuration is supported by creating
 * multiple stk parameter files, one for each media type. The drives
 * of that particular type are only added to the respective parameter
 * file.
 *
 * RETURN hashtable h_drives:-
 * caller will iterate through the hashtable to get the list of drive_t
 * for a media type, the media type (euq_type) is used as the key.
 * Then iterate through the list to get drive_t, if needed
 * RETURN hashtable h_stk_devpaths:-
 * caller will iterate through the hashtable to get the list of stk_device_t
 * for a media type, the media type (euq_type) is used as the key.
 * Then iterate through the list to get stk_device_t, if needed
 *
 * If the library is not confugured with mixed media, then the hashtable
 * will have a single key and a list of all the drive_t/stk_device_t as value
 */
static int
get_devpath_entries(
stk_host_info_t *stk_host, /* stk host configuration information */
sqm_lst_t *mcf_paths,
	/* list of drive paths already under sam control */
hashtable_t **h_stk_devpaths,
	/* OUTPUT - hashtable with list of stk_devpaths, for each media type */
hashtable_t **h_drives)
	/* OUTPUT - hashtable with list of drive_t, for each media type */
{

	sqm_lst_t	*tdrive_list	= NULL;
	node_t		*tdrive_node;
	drive_t		*tdrive		= NULL;
	sqm_lst_t	*stkdrive_list	= NULL;
	node_t		*stkdrive_node;
	acs_drive_t	*stkdrive;
	stk_device_t	*stk_devpath;
	sqm_lst_t	*paths;

	if (ISNULL(stk_host, mcf_paths, h_stk_devpaths, h_drives)) {
		return (-1);
	}

	*h_drives = ht_create();
	if (ISNULL(*h_drives)) {
		Trace(TR_ERR, "get stk dev paths failed: %d[%s]",
		    samerrno, samerrmsg);
		return (-1);
	}

	*h_stk_devpaths = ht_create();
	if (ISNULL(*h_stk_devpaths)) {
		Trace(TR_ERR, "get stk dev paths failed: %d[%s]",
		    samerrno, samerrmsg);
		free_hashtable(h_drives);
		return (-1);
	}

	/* query drives in the stk acsls library */
	if (acs_display_info(DISPLAY_DRIVE, NULL, &stkdrive_list) != 0) {

		goto error;
	}

	for (stkdrive_node = stkdrive_list->head; stkdrive_node != NULL;
	    stkdrive_node = stkdrive_node->next) {

		stkdrive = (acs_drive_t *)stkdrive_node->data;

		/*
		 * Find the drive on this host by searching for a drive
		 * that matches the serial number returned from acsls.
		 */
		if (discover_tape_drive(
		    stkdrive->serial_num, mcf_paths, &tdrive_list) == -1) {
			goto error;
		}
		/*
		 * If nothing discovered continue.
		 */
		if (tdrive_list == NULL) {
			continue;
		}
		/*
		 * Free the list if it is non-NULL but empty
		 */
		if (tdrive_list->length == 0) {
			lst_free(tdrive_list);
			tdrive_list = NULL;
			continue;
		}

		/* A match is found from tape discovery on this host. */
		tdrive_node = tdrive_list->head;
		if (tdrive_node != NULL) {
			tdrive = (drive_t *)tdrive_node->data;
		}
		if (tdrive != NULL) {

			/* create a devpath entry for parameter file */
			stk_devpath =
			    (stk_device_t *)mallocer(sizeof (stk_device_t));
			if (ISNULL(stk_devpath)) {
				goto error;
			}

			memset(stk_devpath, 0, sizeof (stk_device_t));
			stk_devpath->acs_num = stkdrive->id.panel_id.lsm_id.acs;
			stk_devpath->lsm_num = stkdrive->id.panel_id.lsm_id.lsm;
			stk_devpath->panel_num = stkdrive->id.panel_id.panel;
			stk_devpath->drive_num = stkdrive->id.drive;
			stk_devpath->shared = B_FALSE; /* ALWAYS */

			paths = tdrive->alternate_paths_list;
			if (paths != NULL && paths->head != NULL &&
			    paths->head->data != NULL) {

				strlcpy(stk_devpath->pathname,
				    (char *)paths->head->data,
				    sizeof (upath_t));

				/* update the drive's device name */
				strlcpy(tdrive->base_info.name,
				    (char *)paths->head->data,
				    sizeof (upath_t));
			}

			if (list_hash_put(*h_drives,
			    tdrive->base_info.equ_type,
			    tdrive) != 0) {
				free(stk_devpath);
				goto error;
			}

			/*
			 * set the tdrives data to NULL so it does not
			 * get freed in the the error case's lst_free_deep.
			 */
			tdrive_node->data = NULL;

			if (list_hash_put(*h_stk_devpaths,
			    strdup(tdrive->base_info.equ_type),
			    stk_devpath) != 0) {

				free(stk_devpath);
				goto error;
			}

			/*
			 * free tdrive_list but not the elements since
			 * these have been included in the hashtable.
			 */
			lst_free(tdrive_list);
			tdrive_list = NULL;
		}
	}

	lst_free_deep(stkdrive_list);
	return (0);
error:
	Trace(TR_ERR, "get stk dev paths failed: %d[%s]", samerrno, samerrmsg);

	/*
	 * Deep free all of the lists and hash tables created here.
	 */
	lst_free_deep_typed(tdrive_list, FREEFUNCCAST(free_drive));
	lst_free_deep(stkdrive_list);
	ht_list_hash_free_deep(h_drives, FREEFUNCCAST(free_drive));
	ht_list_hash_free_deep(h_stk_devpaths, FREEFUNCCAST(free));
	return (-1);
}

/*
 *	free_list_of_stk_volume().
 *	This function will free the
 *	list of md_license structure.
 */
static void
free_list_of_stk_volume(sqm_lst_t *stk_volume_list)
{
	node_t *node;
	Trace(TR_ALLOC, "freeing stk_volume_list");
	if (stk_volume_list == NULL)
		return;
	node = stk_volume_list->head;
	while (node != NULL) {
		free((stk_volume_t *)node->data);
		node = node->next;
	}
	lst_free(stk_volume_list);
	Trace(TR_ALLOC, "finished freeing stk_volume_list");
}


/*
 *	free_stk_phyconf_info().
 *	This function will free the
 *	list of md_license structure.
 */
static void
free_stk_phyconf_info(stk_phyconf_info_t *stk_phyconf_info)
{
	Trace(TR_ALLOC, "freeing stk_phyconf_info_t");
	if (stk_phyconf_info == NULL) {
		return;
	}

	if (stk_phyconf_info ->stk_pool_list != NULL) {
		lst_free_deep(stk_phyconf_info ->stk_pool_list);
	}
	if (stk_phyconf_info ->stk_panel_list != NULL) {
		lst_free_deep(stk_phyconf_info ->stk_panel_list);
	}
	if (stk_phyconf_info ->stk_lsm_list != NULL) {
		lst_free_deep(stk_phyconf_info ->stk_lsm_list);
	}
	free(stk_phyconf_info);
	Trace(TR_ALLOC, "finished free stk_phyconf_info_t");
}




/*
 * Map the stk media type from the ACSLS_MAP table
 * and return the corresponding sam media type
 *
 * sam_mtype should be malloced by the caller
 * and should be atleast of length mtype_t (5)
 *
 */
static int
stkmtype2sammtype(char *stk_mtype, char *sam_mtype) {

	int j = 0;

	if (ISNULL(stk_mtype, sam_mtype)) {
		return (-1);
	}
	for (j = 0; j < ACSLS_MAP_ENTRY_COUNT; j++) {

		if (strcmp(ACSLS_MAP[j].stk_mtype, stk_mtype) == 0) {

			strlcpy(sam_mtype,
				ACSLS_MAP[j].sam_mtype,
				sizeof (mtype_t));
			return (0);
		}
	}
	/* Cannot map the stk media type to sam media type */
	samerrno = SE_ACSLS_MEDIA_MAP_FAILED;
	snprintf(samerrmsg, MAX_MSG_LEN,
		GetCustMsg(SE_ACSLS_MEDIA_MAP_FAILED));
	return (-1);
}

/*
 * Remove the volumes that have duplicate media type
 *
 *
 * The caller of this function should use lst_free to free the
 * return lst since the same data will be in the input list as
 * well and will get freed when the lst_duplicates gets freed
 * in the callers function
 */
static int
get_vol_with_uniq_mtype(
sqm_lst_t *lst_duplicates,	/* input list - list with duplicate mtype */
sqm_lst_t **lst)		/* return list - with no duplicates */
{

	node_t *n1 = NULL;
	stk_volume_t *v1 = NULL;
	node_t *n2 = NULL;
	stk_volume_t *v2 = NULL;
	int found = 0;

	if (ISNULL(lst_duplicates, lst)) {
		return (-1);
	}

	*lst = lst_create();
	if (ISNULL(*lst)) {
		return (-1);
	}

	n1 = lst_duplicates->head;
	while (n1 != NULL) {
		v1 = (stk_volume_t *)n1->data;

		n2 = (*lst)->head;

		while (n2 != NULL) {
			v2 = (stk_volume_t *)n2->data;

			if (ISNULL(v1, v2)) {
				goto error;
			}
			if (strcmp(v1->media_type, v2->media_type) == 0) {
				found = 1;
				break;
			} else {
				found = 0;
			}
			n2 = n2->next;
		}

		if (found == 0) {
			if (lst_append(*lst, v1) != 0) {
				goto error;
			}
		}
		n1 = n1->next;
	}
	return (0);
error:
	Trace(TR_ERR, "get volumes with uniq media type failed: %s",
	    samerrmsg);

	if (*lst != NULL) {
		lst_free_deep(*lst);
	}
	return (-1);
}

/*
 *	parse a string.
 *	get a list of name value pairs.
 */
int
get_string_namevalue(
sqm_lst_t **namevalue_list,	/* a list of nw_param_pair_t, */
				/* need to be freed by caller */
char *str_buf)			/* given network attached library */
				/* parameter file path */
{
	char buf[BUFSIZ];
	char *p, *q;
	int ii, i;
	int leng;
	nw_param_pair_t *nw_namevalue;
	char abc[20][100];
	sqm_lst_t *temp_list;

	Trace(TR_OPRMSG, "entering getting incoming name value pair");
	temp_list = lst_create();
	if (temp_list == NULL) {

		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	*namevalue_list = NULL;
	*namevalue_list = lst_create();
	if (*namevalue_list == NULL) {

		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	ii = 0;
	strcpy(buf, str_buf);

	q = strtok(buf, ",");
	while (q != NULL) {
		strcpy(abc[ii], q);
		ii++;
		q = strtok(NULL, ",");
	}

	for (i = 0; i < ii; i++) {
		p = strstr(abc[i], "=");

		leng = Ptrdiff(p, &abc[i][0]);

		abc[i][leng] = '\0';
			nw_namevalue = (nw_param_pair_t *)
			mallocer(sizeof (nw_param_pair_t));
			if (nw_namevalue == NULL) {
				Trace(TR_OPRMSG, "%s", samerrmsg);
				return (-1);
			}


		strcpy(nw_namevalue->name, abc[i]);
		strcpy(nw_namevalue->value, p + 1);
		lst_append(*namevalue_list, nw_namevalue);
	}

	Trace(TR_OPRMSG, "finished incoming name value pair");
	return (0);
}

/*
 *	THE PRINT UTILITY FUNCTIONS START HERE
 */

/*
 * Name:
 *
 *	st_show_ack_res()
 *
 * Description:
 *
 *	This function prints acknowledge response data from the server
 *	for every request made by main. It has two modes of operation.
 *	- a checking mode when interacting with the sample server t_acslm.
 *	- a reporting mode when interacting with a native server.
 *
 *
 * Return Values:
 *
 *		None
 *
 */
static void
st_show_ack_res(
SEQ_NO sn,		/* ARGSUSED */
unsigned char mopts,
unsigned char eopts,
LOCKID lid,
ACS_RESPONSE_TYPE type,
SEQ_NO seq_nmbr,
STATUS status,
ALIGNED_BYTES rbuf[MAX_MESSAGE_SIZE / sizeof (ALIGNED_BYTES)])
{

	MESSAGE_HEADER  *msg_hdr_ptr;
	char buf[80];
	char self[50];
	BOOLEAN	check_mode;
	ACKNOWLEDGE_RESPONSE *ap;
	char *rtto_str;

	Trace(TR_OPRMSG, "Entering st_show_ack_res");
	strlcpy(self, "st_show_ack_res", sizeof (self));
	ap = (ACKNOWLEDGE_RESPONSE *) rbuf;
	msg_hdr_ptr = &(ap->request_header.message_header);
	if (!check_mode) {
		switch (type) {
			case RT_ACKNOWLEDGE:
				Trace(TR_OPRMSG, "\n\t%s: "
				    "ACKNOWLEDGE RESPONSE received.\n",
				    self);
				Trace(TR_OPRMSG, "\t%s: Status = %s\n",
				    self, acs_status(status));
				Trace(TR_OPRMSG, "\t%s: Sequence "
				    "Number = %d\n", self,
				    seq_nmbr);
				Trace(TR_OPRMSG, "\t%s: Msg Opts "
				    "= %s (0x%x)\n", self,
				    decode_mopts(
				    msg_hdr_ptr->message_options, buf),
				    msg_hdr_ptr->message_options);
				Trace(TR_OPRMSG, "\t%s: Version = %s (%d)\n",
				    self,
				    decode_vers((long)msg_hdr_ptr->version,
				    buf), msg_hdr_ptr->version);
				Trace(TR_OPRMSG, "\t%s: extended_options "
				    "%s = 0x%x\n", self,
				    decode_eopts((unsigned char)
				    msg_hdr_ptr->extended_options, buf),
				    msg_hdr_ptr->extended_options);
				Trace(TR_OPRMSG, "\t%s: lock_id = 0x%x\n", self,
				    msg_hdr_ptr->lock_id);
				Trace(TR_OPRMSG, "\t%s: status = %s\n", self,
				    acs_status(ap->message_status.status));
				Trace(TR_OPRMSG, "\t%s: type = %s\n", self,
				    acs_type(ap->message_status.type));
				Trace(TR_OPRMSG, "\t%s: message_id = %d \n",
				    self, ap->message_id);
				break;
			default:
				rtto_str = st_rttostr(type);
				Trace(TR_OPRMSG,
				    "\n\t%s: Got %s RESPONSE "
				    "when expecting ACK RESPONSE\n",
				    self, rtto_str);
				free(rtto_str);
				break;
		}
	} else {
		Trace(TR_OPRMSG, "\n\t%s: ACKNOWLEDGE "
		    "RESPONSE received.\n", self);
		if (msg_hdr_ptr->message_options != mopts) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "message_options failure.\n", self);
		}

		if (msg_hdr_ptr->version != VERSION_LAST - 1) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "version failure.\n", self);
		}

		if (msg_hdr_ptr->extended_options != eopts) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "extended_options failure.\n", self);
		}

		if (msg_hdr_ptr->lock_id != lid) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "lock_id failure.\n", self);
		}
		if (ap->message_status.status != STATUS_VALID) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "status failure.\n", self);
		}
		if (ap->message_status.type != TYPE_NONE) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "type failure.\n", self);
		}
		if (ap->message_id != 1) {
			Trace(TR_OPRMSG, "\t%s: ACK RESPONSE "
			    "message_id failure.\n", self);
		}
	}
	Trace(TR_OPRMSG, "Finished st_show_ack_res");
}


/*
 * Name:
 *
 *	st_show_int_resp_hdr()
 *
 * Description:
 *
 *	This function prints the intermediate response header
 *	data returned from acs_response().
 *	It has two modes of operation.
 *	- a checking mode when interacting with the sample server t_acslm.
 *	- a reporting mode when interacting with a native server.
 *
 *
 *
 * Implicit Inputs:
 *
 *	status - Global variable that holds the status returned from
 *		acs_response.
 *	type - Global variable that holds the value of the response
 *		type returned by acs_response.
 *	seq_number - Global variable that holds the user supplied sequence
 *		number that uniquely identifies the response and which
 *		is returned by acs_response.
 *	self - string that names the calling routine.
 *
 *
 * Return Values:
 *
 *		None.
 */
static void
st_show_int_resp_hdr(
ACS_RESPONSE_TYPE type,
SEQ_NO seq_nmbr,
SEQ_NO s,
STATUS status)
{

	BOOLEAN	check_mode;
	char *rtto_str;
	check_mode = FALSE;
	Trace(TR_OPRMSG, "Entering st_show_int_resp_hdr");
	Trace(TR_OPRMSG, "INTERMEDIATE RESPONSE received.\n");
	if (status != STATUS_SUCCESS) {
		Trace(TR_OPRMSG, "acs_response() "
		    "(int RESPONSE) failed:%s\n",
		    acs_status(status));
		return;
	}
	if (seq_nmbr != s) {
		Trace(TR_OPRMSG, "%s sequence "
		    "mismatch got:%d expected:%d\n",
		    "int RESPONSE", seq_nmbr, s);
		return;
	}
	if (!check_mode) {
		Trace(TR_OPRMSG, "\tIntermediate Status: %s\n",
		    acs_status(status));
		Trace(TR_OPRMSG, "\tSequence Number: %d\n", seq_nmbr);
		rtto_str = st_rttostr(type);
		Trace(TR_OPRMSG, "\tResponse Type: %s\n", rtto_str);
		free(rtto_str);
	}
	Trace(TR_OPRMSG, "Finished st_show_int_resp_hdr");
}


/*
 * Name:
 *
 *		st_show_final_resp_hdr()
 *
 * Description:
 *
 *	This function checks and traces the response data returned from
 *	acs_response(). It has two modes of operation.
 *	- a checking mode when interacting with the sample server t_acslm.
 *	- a reporting mode when interacting with a native server.
 *
 * Inputs:
 *	status - variable that holds the status returned from
 *		 acs_response.
 *	type -  variable that holds the value of the response
 *		type returned by acs_response.
 *	seq_number -  variable that holds the user supplied sequence
 *		 number that uniquely identifies the response and which
 *		 is returned by acs_response.
 *	self - string that names the calling routine.
 *
 * Return Values:
 *		-1 if an error is detected.
 */
static int
st_show_final_resp_hdr(
ACS_RESPONSE_TYPE type,
SEQ_NO seq_nmbr,
SEQ_NO s,
STATUS status)
{
	char	self[50];
	char *rtto_str;
	BOOLEAN	check_mode;

	check_mode = FALSE;
	strlcpy(self, "st_show_final_resp_hdr", sizeof (self));

	Trace(TR_OPRMSG, "Entering st_show_final_resp_hdr");
	Trace(TR_OPRMSG, "\n\t%s: FINAL RESPONSE received.\n", self);
	if ((status != STATUS_SUCCESS) &&
	    (status != STATUS_DONE) &&
	    (status != STATUS_RECOVERY_COMPLETE) &&
	    (status != STATUS_NORMAL) &&
	    (status != STATUS_INVALID_CAP) &&
	    (status != STATUS_MULTI_ACS_AUDIT) &&
	    (status != STATUS_VALID)) {
		Trace(TR_OPRMSG, "\t%s: acs_response() "
		    "(FINAL RESPONSE) failed:%s\n",
		    self, acs_status(status));
		return (-1);
	}
	if (seq_nmbr != s) {
		Trace(TR_OPRMSG, "\t%s: %s sequence "
		    "mismatch got:%d expected:%d\n",
		    self, "FINAL RESPONSE", seq_nmbr, s);
		return (-1);
	}
	if (type != RT_FINAL) {
		Trace(TR_OPRMSG, "\t%s: FINAL RESPONSE type failure.\n", self);
		Trace(TR_OPRMSG, "\t%s: expected RT_FINAL, got %s.\n", self,
		    acs_type_response(type));
		return (-1);
	}
	if (!check_mode) {
		Trace(TR_OPRMSG, "\tFinal Status: %s\n", acs_status(status));
		Trace(TR_OPRMSG, "\tSequence Number: %d\n", seq_nmbr);
		rtto_str = st_rttostr(type);
		Trace(TR_OPRMSG, "\tResponse Type: %s\n", rtto_str);
		free(rtto_str);
	}
	Trace(TR_OPRMSG, "Finished st_show_final_resp_hdr");
	return (0);
}


/*
 * Name:
 *
 *		st_show_media_type()
 *
 * Description:
 *
 *	This function prints media type data from the server for QUERY
 *	CLEAN, QUERY VOLUME, and QUERY SCRATCH requests made by main.
 *	It is also called by st_show_media_info.
 *	It only reports the values, it does no checking.
 *
 *
 *
 * Explicit Inputs:
 *
 *	i - indicates the nth acs, lsm, or drive being reported.
 *	mtype - integer that specifies the type of the tape cartridge.
 *
 * Implicit Inputs:
 *		self - string that names the calling routine.
 *
 * Return Values:
 *
 *		None.
 */
static void
st_show_media_type(
unsigned short i,
MEDIA_TYPE mtype)
{

	char		mstr[80];

	Trace(TR_OPRMSG, "Entering st_show_media_type");
	switch (mtype) {
		case ANY_MEDIA_TYPE:
			snprintf(mstr, sizeof (mstr), "ANY_MEDIA_TYPE");
			break;
		case UNKNOWN_MEDIA_TYPE:
			snprintf(mstr, sizeof (mstr), "UNKNOWN_MEDIA_TYPE");
			break;
		case MEDIA_TYPE_3480:
			snprintf(mstr, sizeof (mstr), "MEDIA_TYPE_3480");
			break;
		default:
			snprintf(mstr, sizeof (mstr), "MEDIA_TYPE_%d", mtype);
	}
	Trace(TR_OPRMSG, "media type[%d] is %s.\n", i, mstr);
	Trace(TR_OPRMSG, "Finished st_show_media_type");
}


/*
 * Name:
 *
 *        st_show_usage()
 *
 * Description:
 *
 *        This function prints cleaning cartridge usage data from the server
 *        for QUERY CLEAN requests made by main. It only reports the values,
 *        it does no checking.
 *
 * Explicit Inputs:
 *
 *        cstat - pointer to a cleaning cartridge information structure.
 *
 * Implicit Inputs:
 *        self - string that names the calling routine.
 *
 * Return Values:
 *
 *        None.
 */
void
st_show_usage(QU_CLN_STATUS *cstat)
{
	Trace(TR_OPRMSG, "max use = %d, current use = %d.",
	    cstat->max_use, cstat->current_use);
}


/*
 * RT Type to string ... local function
 */
static char *
st_rttostr(k)
ACS_RESPONSE_TYPE k;
{
	char	 tbuf[80];

	Trace(TR_OPRMSG, "Entering st_rttostr");
	switch (k) {
		case RT_ACKNOWLEDGE:
			strlcpy(tbuf, "RT_ACKNOWLEDGE",
				sizeof (tbuf));
			break;
		case RT_FINAL:
			strlcpy(tbuf, "RT_FINAL",
				sizeof (tbuf));
			break;
		case RT_INTERMEDIATE:
			strlcpy(tbuf, "RT_INTERMEDIATE",
				sizeof (tbuf));
			break;
		default:
			snprintf(tbuf, sizeof (tbuf),
				"RT_Unknown: %d", k);
	}
	Trace(TR_OPRMSG, "Finished st_rttostr");
	return (strdup(tbuf));
}


/*
 *	Create the message option string.
 *	Pass in str_buffer has size at least 256
 */
static char *decode_mopts(
unsigned char msgopt,
char *str_buffer)
{

	strlcpy(str_buffer, "<", BUFSIZE);	  /* initialize */

	if (msgopt == '\0') {
		strlcpy(str_buffer, "<NONE>", BUFSIZE);
		return (str_buffer);
	}
	if (msgopt & FORCE) {
		strlcat(str_buffer, "FORCE|", BUFSIZE);
	}
	if (msgopt & INTERMEDIATE) {
		strlcat(str_buffer, "INTERMEDIATE|", BUFSIZE);
	}
	if (msgopt & ACKNOWLEDGE) {
		strlcat(str_buffer, "ACKNOWLEDGE|", BUFSIZE);
	}

	if (msgopt & READONLY) {
		strlcat(str_buffer, "READONLY|", BUFSIZE);
	}

	if (msgopt & EXTENDED) {
		strlcat(str_buffer, "EXTENDED|", BUFSIZE);
	}

	/* Check for bad value (none of the above) */
	if (strlen(str_buffer) == 1) {
		strlcat(str_buffer, "BAD ", BUFSIZE);
	}

	/* terminate the string */
	str_buffer[strlen(str_buffer) - 1] = '>';
	return (str_buffer);
}


/*
 *	Create a version string.
 *	pass in buffer has size at least 256.
 */
static char *decode_vers(
long vers,
char *buffer)
{

	/* buffer to hold string */

	switch (vers) {
		case VERSION0:
			strlcpy(buffer, "<VERSION0>", BUFSIZE);
			break;

		case VERSION1:
			strlcpy(buffer, "<VERSION1>", BUFSIZE);
			break;
		case VERSION2:
			strlcpy(buffer, "<VERSION2>", BUFSIZE);
			break;
		case VERSION3:
			strlcpy(buffer, "<VERSION3>", BUFSIZE);
			break;
		case VERSION4:
			strlcpy(buffer, "<VERSION4>", BUFSIZE);
			break;
		default:
			strlcpy(buffer, "<BAD>", BUFSIZE);
			break;
	}
	return (buffer);
}


/*
 *	Create the extended option string
 *	pass in str_buffer has size at least 256.
 */
static char *decode_eopts(
	unsigned char extopt,
	char *str_buffer)
{

	strlcpy(str_buffer, "<", BUFSIZE);	  /* initialize */

	if (extopt == '\0') {
		strlcpy(str_buffer, "<NONE>", BUFSIZE);
		return (str_buffer);
	}
	if (extopt & WAIT) {
		strlcat(str_buffer, "WAIT|", BUFSIZE);
	}
	if (extopt & RESET) {
		strlcat(str_buffer, "RESET|", BUFSIZE);
	}
	if (extopt & VIRTUAL) {
		strlcat(str_buffer, "VIRTUAL|", BUFSIZE);
	}
	if (extopt & CONTINUOUS) {
		strlcat(str_buffer, "CONTINUOUS|", BUFSIZE);
	}
	if (extopt & RANGE) {
		strlcat(str_buffer, "RANGE|", BUFSIZE);
	}
	if (extopt & CLEAN_DRIVE) {
		strlcat(str_buffer, "CLEAN_DRIVE|", BUFSIZE);
	}
	/* Check for bad value (none of the above) */
	if (strlen(str_buffer) == 1) {
		strlcat(str_buffer, "BAD ", BUFSIZE);
	}
	/* terminate the string */
	str_buffer[strlen(str_buffer) - 1] = '>';
	return (str_buffer);
}


/*
 *
 * Test the status in the final response header to see if there is a
 * variable part to the response packet.
 */
BOOLEAN no_variable_part(STATUS status) {
	if ((status != STATUS_SUCCESS) &&
	    (status != STATUS_DONE) &&
	    (status != STATUS_RECOVERY_COMPLETE) &&
	    (status != STATUS_NORMAL) &&
	    (status != STATUS_VALID)) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

static int
wait_for_response(
	int	seq,
	int	(*parse_acs_resp)(ALIGNED_BYTES, sqm_lst_t *, sqm_lst_t **),
	sqm_lst_t **lst)
{

	STATUS			st;
	SEQ_NO			rseq;
	REQ_ID			reqid;
	int			ret;
	ALIGNED_BYTES		rbuf[MAX_MESSAGE_SIZE / sizeof (ALIGNED_BYTES)];
	ACS_RESPONSE_TYPE	type;
	sqm_lst_t			*fmt_lst = NULL;

	/*
	 * call acs_response() repeatedly until the FINAL packet for this
	 * request has been received
	 */
	do {
		rbuf[0] = '\0';

		/*
		 * This used to block indefinitely which caused problems
		 * in error cases. Like when the wrong server was specified.
		 */
		st = acs_response(
		    300,
		    &rseq,
		    &reqid,
		    &type,
		    rbuf);

		if (st == STATUS_IPC_FAILURE) {
			samerrno = SE_ACS_RESPONSE_ERR;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			return (-1);
		} else if (st == STATUS_PROCESS_FAILURE) {
			samerrno = SE_ACS_RESPONSE_ERR;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			return (-1);
		}

		if (type == RT_ACKNOWLEDGE) {

			/* TBD: validate acknowledgement */
			st_show_ack_res(
			    seq,
			    EXTENDED | ACKNOWLEDGE,
			    0,
			    NO_LOCK_ID,
			    type,
			    rseq,
			    st,
			    rbuf);

		} else if (type == RT_INTERMEDIATE) {

			/* TBD: validate response */
			st_show_int_resp_hdr(type, rseq, seq, st);

			ret = parse_acs_resp(rbuf, fmt_lst, lst);

		} else { // type == RT_FINAL

			/* TBD: validate response */
			if (st_show_final_resp_hdr(type, rseq, seq, st) != 0) {
				samerrno = SE_ACS_RESPONSE_ERR;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				return (-1);
			}

			if (no_variable_part(st) == TRUE) {
				break;
			}

			ret = parse_acs_resp(rbuf, fmt_lst, lst);

		}
	} while (type != RT_FINAL);

	return (ret);
}


/*
 * To get the configuration of the components in an ACSLS library, or their
 * status, use the 'display' command to create complex or detailed queries
 * using XML as the Query language. The XML request is then sent to the SSI
 * using the acs_display() ACSAPI and the responses are awaited and parsed.
 *
 * The SSI process must be running before this API can be used.
 */
int
acs_display_info(
	int	query_type,	/* type of query */
	char	*cmdarg,	/* arguments for the XML request */
	sqm_lst_t	**lst)		/* response parsed as a list */
{

	STATUS			st;
	SEQ_NO			seq;
	DISPLAY_XML_DATA	cmd;
	char			xmlreq[512] = {0};
	char			s[32] = {0};
	boolean_t		all_items;


	if (cmdarg == NULL || cmdarg[0] == '\0') {
		/* get info about all the items of a requested type */
		all_items = B_TRUE;
	}

	if (all_items == B_TRUE) {
		strcpy(s, "*");
	} else {
		/* TBD: parse the cmdarg to generate the id string */
	}

	snprintf(xmlreq, sizeof (xmlreq),
	    query_cmdresp_tbl[query_type].xmlreq, s);

	cmd.length = strlen(xmlreq);
	strlcpy(cmd.xml_data, xmlreq, sizeof (cmd.xml_data));

	/*
	 * TBD
	 * generate a sequence number, this uniquely identifies the response
	 * with the request.
	 */
	seq = (SEQ_NO)509;

	if ((st = acs_display(seq, TYPE_DISPLAY, cmd)) != STATUS_SUCCESS) {

		samerrno = SE_ACS_REQUEST_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    Str(acs_status(st)));

		Trace(TR_ERR, "get acs display info failed: %s", samerrmsg);
		return (-1);
	}

	if (wait_for_response(
	    seq,
	    (int (*)(ALIGNED_BYTES, sqm_lst_t *, sqm_lst_t **))
	    query_cmdresp_tbl[query_type].parse_resp,
	    lst) != 0) {

		Trace(TR_ERR, "get acs display info failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get acs display info success");
	return (0);
}


/*
 * The display command response contains a format response and a data response
 * When this function is called with the display response format, the format
 * list is filled with a list of keys. When the function is called with a
 * display response data, the format list is passed as input and the function
 * uses the keys in the format list to generate a list of key=value pairs.
 *
 * if the response contains the format of the data fields, parse the
 * format to retrieve the keys
 * <format>
 *     <fields>
 *	   <field name="fieldname" format="fmttype" maxlen="nn"/>
 *     </fields>
 * </format>
 *
 * else if the response contains the data,
 * <data>
 *     <r>
 *         <f maxlen=n>fieldvalue</f>...
 *     </r>
 *     <r>...
 * </data>
 *
 * The return parameter is a list of keyword = value pairs
 */
static int
parse_fmtdata_resp(
	ALIGNED_BYTES	buf,		/* INPUT - */
	sqm_lst_t		*fmt_lst,	/* INPUT/OUTPUT */
	sqm_lst_t		**lst)		/* OUTPUT - list of strings */
{
	size_t			l;
	int			st;
	char			*ptr1, *ptr2;
	char			*key, val[32];
	ACS_DISPLAY_RESPONSE	*res;

	if (ISNULL(buf, lst)) {
		Trace(TR_ERR, "parse format and data response failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "parse format and data response");

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse format and data response failed: %s",
		    samerrmsg);
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "display format and data response: %s", ptr1);

	if ((ptr2 = strstr(ptr1, "<format>")) != NULL) {
		st = parse_fmt_resp(ptr2, &fmt_lst);
	} else if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {
		/* parse data into key value pairs */
		st = fmt_kv_lst(fmt_lst, lst);
	}
	if (st != 0) {
		Trace(TR_ERR, "parse format and data response failed: %s",
		    samerrmsg);
	}
	Trace(TR_MISC, "parse format and data response success");
	return (0);
}


/*
 * parse_cap_response assumes the format of the data response, the
 * following information is expected in the cap response data:
 * acs, lsm, cap
 */
static int
parse_cap_resp(
	ALIGNED_BYTES   buf,
	sqm_lst_t		*fmt_lst, /* NOT USED */
	sqm_lst_t		**lst)
{

	stk_cap_t		*cap;
	char			*ptr1, *ptr2;
	ACS_DISPLAY_RESPONSE	*res;
	size_t			l;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display cap resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	/* Terminate the response */
	ptr2 = strstr(ptr1, "</response>");
	if (ptr2 == NULL) {
		/* invalid response string */
		return (-1);
	}

	ptr2 += 11;
	*ptr2 = '\0';

	Trace(TR_DEBUG, "Display response: %s", ptr1);

	if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {

		if (*lst == NULL) {
			*lst = lst_create();
		}

		while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

			cap = (stk_cap_t *)mallocer(sizeof (stk_cap_t));
			if (cap == NULL) {
				return (-1);
			}
			memset(cap, 0, sizeof (stk_cap_t));

			ptr2 += 3; /* skip past <r> */
			l = parse_f_int(ptr2, &cap->acs_num);
			ptr2 += l;
			l = parse_f_int(ptr2, &cap->lsm_num);
			ptr2 += l;
			l = parse_f_int(ptr2, &cap->cap_num);
			ptr2 += l;

			if (lst_append(*lst, cap) != 0) {
				free(cap);
				return (-1);
			}

			ptr2 += 4; /* advance to the start of the next drive */
			ptr1 = ptr2;
		}
	}
	return (0);
}


/*
 * parse_drive_resp() assumes the format of the data response, the
 * following information is expected in the drive data:
 * acs, lsm, panel, drive, type, status, state and serial number
 */
static int
parse_drive_resp(
	ALIGNED_BYTES	buf,
	sqm_lst_t 		*fmt_lst, /* NOT USED */
	sqm_lst_t		**lst)
{

	acs_drive_t		*drive;
	char			*ptr1, *ptr2;
	ACS_DISPLAY_RESPONSE	*res;
	size_t l;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display drive resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "Display response: %s", ptr1);
	/*
	 * <r> marks the start of a drive entry, <f> marks the start of a field
	 *
	 * <r>
	 * <f maxlen="3">acs</f>
	 * <f maxlen="3">lsm</f>
	 * <f maxlen="5">panel</f>
	 * <f maxlen="5">drive</f>
	 * <f maxlen="9">status</f>
	 * <f maxlen="10">state</f>
	 * <f maxlen="6">volume</f>
	 * <f maxlen="9">type</f>
	 * <f maxlen="5">lock</f>
	 * <f maxlen="32">serial_num</f>
	 * <f maxlen="14">condition</f>
	 * </r>
	 */

	if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {

		if (*lst == NULL) {
			*lst = lst_create();
		}

		while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

			drive = (acs_drive_t *)mallocer(sizeof (acs_drive_t));
			if (drive == NULL) {
				return (-1);
			}
			memset(drive, 0, sizeof (acs_drive_t));

			/* extract string from <f ....>..</f> */
			ptr2 += 3; /* skip past <r> */
			l = parse_f_byte(ptr2, &drive->id.panel_id.lsm_id.acs);
			ptr2 += l;
			l = parse_f_byte(ptr2, &drive->id.panel_id.lsm_id.lsm);
			ptr2 += l;
			l = parse_f_byte(ptr2, &drive->id.panel_id.panel);
			ptr2 += l;
			l = parse_f_byte(ptr2, &drive->id.drive);
			ptr2 += l;
			l = parse_f(ptr2, drive->status);
			ptr2 += l;
			l = parse_f(ptr2, drive->state);
			ptr2 += l;
			l = parse_f(ptr2, drive->volume);
			ptr2 += l;
			l = parse_f(ptr2, drive->type);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&drive->lock);
			ptr2 += l;
			l = parse_f(ptr2, drive->serial_num);
			ptr2 += l;
			l = parse_f(ptr2, drive->condition);
			ptr2 += l;

			if (lst_append(*lst, drive) != 0) {
				free(drive);
				return (-1);
			}

			ptr2 += 4; /* advance to the start of the next drive */
			ptr1 = ptr2;
		}
	}
	return (0);
}


/*
 * parse_lsm_resp() parses the response data assuming a particular
 * format for the data. The following information is expected in the
 * response:- acs, lsm, serial number, status, state and free cells
 */
static int
parse_lsm_resp(
	ALIGNED_BYTES	buf,
	sqm_lst_t 		*fmt_lst, /* NOT USED */
	sqm_lst_t		**lst)
{
	size_t			l;
	acs_lsm_t		*lsm;
	char			*ptr1, *ptr2, *ptr3;
	ACS_DISPLAY_RESPONSE	*res;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display drive resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "Display response: %s", ptr1);

	if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {
		if (*lst == NULL) {
			*lst = lst_create();
		}

		while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

			lsm = (acs_lsm_t *)mallocer(sizeof (acs_lsm_t));
			if (lsm == NULL) {
				return (-1);
			}
			memset(lsm, 0, sizeof (acs_lsm_t));

			ptr2 += 3; /* skip past <r> */
			l = parse_f_byte(ptr2, &lsm->id.acs);
			ptr2 += l;
			l = parse_f_byte(ptr2, &lsm->id.lsm);
			ptr2 += l;
			l = parse_f(ptr2, lsm->serial_num);
			ptr2 += l;
			l = parse_f(ptr2, lsm->status);
			ptr2 += l;
			l = parse_f(ptr2, lsm->state);
			ptr2 += l;
			l = parse_f_int(ptr2, &lsm->free_cells);
			ptr2 += l;

			if (lst_append(*lst, lsm) != 0) {
				free(lsm);
				return (-1);
			}

			ptr2 += 4; /* advance to the start of the next drive */
			ptr1 = ptr2;
		}
	}
	return (0);
}


static int
fmt_kv_lst(sqm_lst_t *fmt_lst, sqm_lst_t **kv_lst) {

	char *ptr1, *ptr2, *str, *key;
	node_t *n;
	char val[32], kv[1024];
	size_t l;

	if (ISNULL(fmt_lst, kv_lst)) {
		Trace(TR_ERR, "create key-value from fmt and data failed: %s",
		    samerrmsg);
		return (-1);
	}

	*kv_lst = lst_create();
	if (*kv_lst == NULL) {
		Trace(TR_ERR, "create key-value from fmt and data failed: %s",
		    samerrmsg);
		return (-1);
	}
	while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

		/* extract string from <f ....>..</f> */
		ptr2 += 3; /* skip past <r> */

		for (n = fmt_lst->head; n != NULL; n = n->next) {
			val[0] = '\0';

			char *key = (char *)n->data;
			l = parse_f(ptr2, val);

			if (key == NULL || l <= 0) {

				lst_free_deep(*kv_lst);

				/* samerrno = SE_ACS_PARSE_ERR; */
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));

				Trace(TR_ERR, "create kv failed: %s",
				    samerrmsg);
				return (-1);
			}

			sprintf(kv, "%s%s=%s ", kv, key, val);
			ptr2 += l;
		}

		str = copystr(kv);
		if (str == NULL) {
			lst_free_deep(*kv_lst);

			Trace(TR_ERR, "create kv failed: %s", samerrmsg);
			return (-1);
		}

		if (lst_append(*kv_lst, str) != 0) {
			free(str);
			lst_free_deep(*kv_lst);

			Trace(TR_ERR, "create kv failed: %s", samerrmsg);
			return (-1);
		}

		ptr2 += 4; /* advance to the start of the next <r> */
		ptr1 = ptr2;
	}
}

static int
parse_fmt_resp(char *f, sqm_lst_t **l) {

	size_t blanks;
	char *ptr1, *ptr2, *ptr3, *ptr4;
	char *val;

	if (l == NULL || f == NULL || strlen(f) == 0) {
		/* samerrno = SE_ACS_PARSE_ERR; */
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "parse response format failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "parse response format: %s", f);

	*l = lst_create();
	if (*l == NULL) {
		Trace(TR_ERR, "parse response format failed: %s", samerrmsg);
		return (-1);
	}
	while (ptr1 = strstr(f, "<field ")) {
		ptr1 += 7; /* skip past <field  */

		/* skip whitespace */
		blanks = strspn(ptr1, WHITESPACE);
		ptr1 += blanks;

		/* read till the next whitespace (delimiter) */
		ptr2 = strstr(ptr1, WHITESPACE);
		if (ptr2 != NULL) {
			*ptr2 = '\0';
			ptr2++;
		}

		/* separate the key and value */
		ptr3 = strchr(ptr1, '=');
		if (ptr3 != NULL) {
			*ptr3 = '\0';
			ptr3++;
			ptr3 += strspn(ptr3, WHITESPACE);
			ptr4 = strrspn(ptr3, WHITESPACE);
			if (ptr4 != NULL) {
				*ptr4 = '\0';
			}
		}

		ptr4 = strrspn(ptr1, WHITESPACE);
		if (ptr4 != NULL) {
			*ptr4 = '\0';
		}
		/* ptr1 = key, ptr3 = value, for now, just use the name value */
		if (strcasecmp(ptr1, "name") != 0) {
			continue;
		}

		val = copystr(ptr3);
		if (val == NULL) {

			lst_free_deep(*l);
			Trace(TR_ERR, "parse response format failed: %s",
			    samerrmsg);
			return (-1);
		}

		if (lst_append(*l, val) != 0) {

			free(val);
			lst_free_deep(*l);

			Trace(TR_ERR, "parse response format failed: %s",
			    samerrmsg);
			return (-1);
		}

	}
	Trace(TR_MISC, "parse response format success");
	return (0);
}


/*
 * The display volume command response contains the following information:
 * vol_id, acs, lsm, panel, row, column, pool, status, media, type
 *
 * <r>
 * <f maxlen=n>fieldvalue</f>...
 * </r>
 * <r>....</r>...
 *
 */
static int
parse_vol_resp(
	ALIGNED_BYTES	buf,
	sqm_lst_t 		*fmt_lst,
	sqm_lst_t		**lst)
{

	stk_volume_t		*vol;
	ACS_DISPLAY_RESPONSE	*res;
	char			*ptr1, *ptr2;
	size_t l;
	int st;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display vol resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		Trace(TR_ERR, "parse display vol resp failed: %s", samerrmsg);
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "Display response: %s", ptr1);

	/*
	 * <r> marks the start of a drive entry, <f> marks the start of a field
	 *
	 * </r>
	 */

	if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {

		if (*lst == NULL) {
			*lst = lst_create();
		}

		while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

			vol = (stk_volume_t *)mallocer(sizeof (stk_volume_t));
			if (vol == NULL) {
				return (-1);
			}
			memset(vol, 0, sizeof (stk_volume_t));

			/* extract string from <f ....>..</f> */
			ptr2 += 3; /* skip past <r> */
			l = parse_f(ptr2, vol->stk_vol);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->acs_num);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->lsm_num);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->panel_num);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->row_id);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->col_id);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&vol->pool_id);
			ptr2 += l;
			l = parse_f(ptr2, vol->status);
			ptr2 += l;
			l = parse_f(ptr2, vol->media_type);
			ptr2 += l;
			l = parse_f(ptr2, vol->volume_type);
			ptr2 += l;

			Trace(TR_OPRMSG, "%s %s %s", Str(vol->stk_vol),
			    Str(vol->media_type), Str(vol->volume_type));

			if (lst_append(*lst, vol) != 0) {
				free(vol);
				return (-1);
			}

			ptr2 += 4; /* advance to the start of the next drive */
			ptr1 = ptr2;
		}
	}
	return (0);
}


/* generic function to parse the response into key-value pairs */
static int
parse_resp_to_kv(
	ALIGNED_BYTES	buf,
	sqm_lst_t 		*fmt_lst,
	sqm_lst_t		**lst)
{

	stk_volume_t		*vol;
	char			*ptr1, *ptr2;
	ACS_DISPLAY_RESPONSE	*res;
	size_t l;
	int st;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display vol resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		Trace(TR_ERR, "parse display vol resp failed: %s", samerrmsg);
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "Display response: %s", ptr1);

	if ((ptr2 = strstr(ptr1, "<format>")) != NULL) {
		st = parse_fmt_resp(ptr2, &fmt_lst);
	} else if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {
		/* parse data into fmt key value pairs */
		st = fmt_kv_lst(fmt_lst, lst);
	}
	Trace(TR_MISC, "parse display volume response success");
}


static int
parse_pool_resp(
	ALIGNED_BYTES	buf,
	sqm_lst_t 		*fmt_lst,
	sqm_lst_t		**lst)
{

	stk_pool_t		*pool;
	char			*ptr1, *ptr2;
	ACS_DISPLAY_RESPONSE	*res;
	size_t l;

	res = (ACS_DISPLAY_RESPONSE *)buf;
	if (res->display_status != STATUS_SUCCESS) {

		samerrno = SE_ACS_RESPONSE_ERR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    acs_status(res->display_status));

		Trace(TR_ERR, "parse display pool resp failed: %s", samerrmsg);
		return (-1);
	}

	if (ISNULL(lst)) {
		return (-1);
	}

	ptr1 = &res->display_xml_data.xml_data[0];
	Trace(TR_DEBUG, "Display response: %s", ptr1);
	/*
	 * <r> marks the start of a drive entry, <f> marks the start of a field
	 *
	 * </r>
	 */

	if ((ptr2 = strstr(ptr1, "<data>")) != NULL) {

		if (*lst == NULL) {
			*lst = lst_create();
		}

		while ((ptr2 = strstr(ptr1, "<r>")) != NULL) {

			pool = (stk_pool_t *)mallocer(sizeof (stk_pool_t));
			if (pool == NULL) {
				return (-1);
			}
			memset(pool, 0, sizeof (stk_pool_t));

			/* extract string from <f ....>..</f> */
			ptr2 += 3; /* skip past <r> */
			l = parse_f_int(ptr2, (int *)&pool->pool_id);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&pool->low_water_mark);
			ptr2 += l;
			l = parse_f_int(ptr2, (int *)&pool->high_water_mark);
			ptr2 += l;
			l = parse_f(ptr2, pool->over_flow);
			ptr2 += l;

			if (lst_append(*lst, pool) != 0) {
				free(pool);
				return (-1);
			}

			ptr2 += 4; /* advance to the start of the next drive */
			ptr1 = ptr2;
		}
	}
	return (0);
}


static int /* return number of characters parsed */
parse_f_int(char *f, int *i) {

	size_t n;
	char *ptr;

	if (f == NULL || strlen(f) == 0) {
		return (0);
	}

	ptr = strstr(f, "</f>");
	if (ptr != NULL) {
		*ptr = '\0';
	}
	n = strlen(f);

	for (; f != NULL && *f != '>'; f++);
	++f;

	*i = (int)strtol(f, (char **)NULL, 10);

	return (n + 4);
}

static int /* return number of characters parsed */
parse_f(char *f, char *s) {

	size_t n;
	char *ptr;

	if (f == NULL || strlen(f) == 0) {
		return (0);
	}

	ptr = strstr(f, "</f>");
	if (ptr != NULL) {
		*ptr = '\0';
	}
	n = strlen(f);

	for (; f != NULL && *f != '>'; f++);
	++f;

	strlcpy(s, f, 64);

	return (n + 4);
}

static int /* return number of characters parsed */
parse_f_byte(char *f, char *b) {

	size_t n;
	char *ptr;
	long l;

	if (f == NULL || strlen(f) == 0) {
		return (0);
	}

	ptr = strstr(f, "</f>");
	if (ptr != NULL) {
		*ptr = '\0';
	}
	n = strlen(f);

	/* walk past the opening <f> */
	for (; f != NULL && *f != '>'; f++);
	++f;

	errno = 0;
	l = (int)strtol(f, (char **)NULL, 10);
	if (errno != 0) {
		Trace(TR_ERR, "Unexpected byte value returned by ACSLS %s", f);
		*b = -3;
	}

	/*
	 * Check for range overflow and trace an error if it falls outside
	 * the range.
	 */
	if (l < CHAR_MIN || l >= CHAR_MAX) {
		Trace(TR_ERR, "Unexpected size returned by ACSLS %d", l);
		*b = -3;
	}
	*b = (char)l;

	return (n + 4);
}
