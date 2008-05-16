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
#pragma ident "$Revision: 1.26 $"

/*
 *	log_info.c -  log and trace in SAM-FS/QFS configuration
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "sam/mount.h"
#include "sam/lib.h"
#define	TRACE_CONTROL
#include "sam/sam_trace.h"
#include "sam/defaults.h"
#include "aml/device.h"
#define	NEED_DL_NAMES
#include "aml/dev_log.h"
#include "aml/shm.h"
#include "mgmt/util.h"
#include "mgmt/log.h"
#include "mgmt/config/common.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/recycle.h"
#include "pub/mgmt/release.h"
#include "pub/mgmt/stage.h"
#include "pub/mgmt/license.h"
#include "pub/mgmt/restore.h"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

static char *ifacility2sfacility(int ifacility);
static void idevflags2sdevflags(uint_t iflags, char **sflags);
static char *devlog_params(equ_t eq, char **params);
static int get_filestat(char *filename, int captionId, char **info);

#define	KEY_PROCNAME	"procname"	/* used for daemon name */
#define	STATE_ON	"on"
#define	STATE_OFF	"off"

#define	SYSTEMLOG	"/var/adm/messages"

#define	GREP_CMD	"/usr/bin/grep"
#define	SYSLOG_CONF	"/etc/syslog.conf"
#define	ADD_DELIM(chars, buffer) \
	if (chars != 0) { \
		snprintf(buffer + chars, sizeof (buffer) - chars, ", "); \
		chars += 2; \
	}

/*
 * get the status of all log and trace files in the SAM-FS configuration
 * This includes the Solaris system log, SAM system log, archiver logs,
 * device logs, releaser logs, stager logs, recycler logs, and daemon trace
 * status
 *
 * Return a list of formatted strings
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_logntrace(
	ctx_t *ctx, /* ARGSUSED */
	sqm_lst_t **lst	/* return - list of formatted strings */
)
{
	char *info = NULL;
	sqm_lst_t *lst_log = NULL;

	if (ISNULL(lst) || ((*lst = lst_create()) == NULL)) {
		return (-1);
	}

	if (get_daemontrace_info(&lst_log) == 0) {
		lst_concat(*lst, lst_log);
	}
	if (get_samlog_lst(&lst_log) == 0) {
		lst_concat(*lst, lst_log);
	}
	if (get_filestat(
	    SYSTEMLOG, SE_SOLARIS_SYSTEM_LOG_DESC, &info) == 0) {
		lst_append(*lst, info);
	}

	/* If QFS only install, do not display archive related logs */
	if (get_samfs_type(NULL) != QFS) {

		if (get_devlog_info(&lst_log) == 0) {
			lst_concat(*lst, lst_log);
		}
		if (get_recyclelog_info(&info) == 0) {
			lst_append(*lst, info);
		}
		if (get_archivelog_info(&lst_log) == 0) {
			lst_concat(*lst, lst_log);
		}
		if (get_releaselog_info(&lst_log) == 0) {
			lst_concat(*lst, lst_log);
		}
		if (get_stagelog_info(&info) == 0) {
			lst_append(*lst, info);
		}
		/*
		 * For now api_version 1.3.2, the restore log is hardcoded to
		 * RESTORELOG = /var/opt/SUNWsamfs/restore.log
		 */
		if (get_filestat(
		    RESTORELOG, SE_RESTORE_LOG_DESC, &info) == 0) {
			lst_append(*lst, info);
		}
	}
	return (0);

}


/* the traceNames is positional, order from traceIdNames */
static struct {
	char *processName;
	int captionId;
} traceNames[] = {
	{ "", 0 },
	{ SAM_AMLD, SE_AMLD_DAEMON_NAME }, // Master automated library daemon
	{ SAM_ARCHIVER, SE_ACHIVER_DAEMON_NAME }, // Archiver daemon
	{ SAM_CATSERVER, SE_CATSERVER_DAEMON_NAME }, // Media catalog daemon
	{ SAM_FSD, SE_FSD_DAEMON_NAME }, // File system daemon
	{ SAM_RFT, SE_RFT_DAEMON_NAME }, // File transfer daemon
	{ SAM_RECYCLER, SE_RECYCLER_NAME }, // Recycler
	{ SAM_NRECYCLER, SE_NRECYCLER_NAME }, // Alternate recycler
	{ SAM_SHAREFSD, SE_SHARED_FS_DAEMON_NAME }, // Shared file system daemon
	{ SAM_STAGER, SE_STAGER_DAEMON_NAME }, // Stager daemon
	{ SAM_RMTSERVER, SE_REMOTE_SVR_DAEMON_NAME }, // SAM-Remote server
	{ SAM_RMTCLIENT, SE_REMOTE_CLNT_DAEMON_NAME }, // SAM-Remote client
	{ SAM_MGMTAPI, SE_MGMT_DAEMON_NAME }, // Management daemon
	{ "", 0 }
};

/*
 * get trace information of all SAM-daemon trace files
 *
 * Returns a list of formatted strings
 *
 * name=name,
 * procname=process name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 *
 */
int
get_daemontrace_info(sqm_lst_t **lst_trace) {

	char buffer[BUFSIZ] = {0};
	int tid = 0;
	int chars = 0;
	struct stat statbuf;
	struct TraceCtlBin *tb = NULL;

	/*
	 * 1) Trace files
	 * attach to TraceCtl.bin.
	 * get the state(on/off), path, size and last modified time
	 */

	*lst_trace = lst_create();

	tb = MapFileAttach(TRACECTL_BIN, TRACECTL_MAGIC, O_RDONLY);
	for (tid = 1 /* starts with 1 */; (tb != NULL && tid < TI_MAX); tid++) {
		struct TraceCtlEntry *tc;
		tc = &tb->entry[tid];

		chars = snprintf(buffer, sizeof (buffer),
			"%s=%s, %s=%s, %s=%s",
			KEY_NAME, GetCustMsg(traceNames[tid].captionId),
			KEY_PROCNAME, traceNames[tid].processName,
			KEY_TYPE, GetCustMsg(SE_TYPE_TRACE));

		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
			"%s=%s",
			KEY_STATE, (*tc->TrFname == '\0') ?
			STATE_OFF : STATE_ON);

		if (*tc->TrFname != '\0') {
			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
			sizeof (buffer) - chars,
				"%s=%s, %s=%s",
				KEY_PATH, tc->TrFname,
				KEY_FLAGS, TraceGetOptions(tc, NULL, 0));

			/* to get the modification time, stat the file */
			if (stat(tc->TrFname, &statbuf) == 0) {
				ADD_DELIM(chars, buffer);
				chars += snprintf(buffer + chars,
					sizeof (buffer) - chars,
					"%s=%ld, %s=%lu",
					KEY_SIZE, statbuf.st_size,
					KEY_MODTIME, statbuf.st_mtime);
			}
		}
		lst_append(*lst_trace, strdup(buffer));
	}
	return (0);
}

/*
 * get device log information
 * Device-logging messages are written to individual log files
 * There is one log file for each library, tape drive etc. Log files are
 * located in /var/opt/SUNWsamfs/devlog. The name of each log file is the
 * same as the equipment ordinal specified in the mcf file
 * flags (events) is one or more of:
 * all, date, err, default, detail, module, label etc.
 *
 * Returns a list of formatted strings
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_devlog_info(
	sqm_lst_t **lst_devlog
)
{
	drive_t *drive = NULL;
	library_t *lib = NULL;
	sqm_lst_t *lst_lib = NULL;
	node_t *node_lib = NULL, *node_drive = NULL;

	if (get_all_libraries(NULL, &lst_lib) != 0) {
		return (-1);
	}
	*lst_devlog = lst_create();

	node_lib = lst_lib->head;
	while (node_lib != NULL) {
		int chars = 0;
		char buffer[BUFSIZ] = {0};
		char *params = NULL;


		lib = (library_t *)node_lib->data;

		chars = snprintf(buffer, sizeof (buffer),
		    "%s=%s %s %02d, %s=%s",
		    KEY_NAME, GetCustMsg(SE_DEVICE_LOG_DESC),
		    (strcmp(lib->base_info.equ_type, "hy") == 0) ?
		    GetCustMsg(SE_HISTORIAN_DESC) : lib->base_info.set,
		    lib->base_info.eq,
		    KEY_TYPE, GetCustMsg(SE_TYPE_LOG));

		if (devlog_params(lib->base_info.eq, &params) != NULL) {

			ADD_DELIM(chars, buffer);
			strlcat(buffer, params, sizeof (buffer));
			free(params); params = NULL;

			lst_append(*lst_devlog, strdup(buffer));
		}
		/* now the devlog for drives */
		node_drive = lib->drive_list->head;
		while (node_drive != NULL) {
			buffer[0] = '\0'; chars = 0;

			drive = (drive_t *)node_drive->data;
			chars = snprintf(buffer, sizeof (buffer),
			    "%s=%s %s %02d, %s=%s",
			    KEY_NAME, GetCustMsg(SE_DEVICE_LOG_DESC),
			    drive->base_info.set,
			    drive->base_info.eq,
			    KEY_TYPE, GetCustMsg(SE_TYPE_LOG));

			if (devlog_params(drive->base_info.eq, &params)
			    != NULL) {

				ADD_DELIM(chars, buffer);
				strlcat(buffer, params, sizeof (buffer));
				free(params); params = NULL;

				lst_append(*lst_devlog, strdup(buffer));
			}
			node_drive = node_drive->next;
		}
		buffer[0] = '\0'; chars = 0;
		node_lib = node_lib->next;
	}
	lst_free_deep_typed(lst_lib, FREEFUNCCAST(free_library));
	return (0);
}

/* DEV_ENT - given an equipment ordinal, return the dev_ent */
#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
	((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))


static char *
devlog_params(
	equ_t eq, char **params
)
{
	boolean_t is_sam_running = B_FALSE;
	char path[128] = {0};
	char buffer[512] = {0};
	char *sflags = NULL;
	dev_ent_t *dev = NULL;
	int chars = 0;
	struct stat statbuf;
	shm_alloc_t master_shm;


	/* set up pointer to the device table */
	if (((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) >= 0) &&
	    ((master_shm.shared_memory =
	    shmat(master_shm.shmid, (void *)NULL, SHM_RDONLY)) != (void *)-1)) {

			is_sam_running = B_TRUE;
	}

	*params = NULL; /* initialize buffer */

	snprintf(path, sizeof (path), "%s/%02d", DEVLOG_NAME, eq);
	chars = snprintf(buffer, sizeof (buffer), "%s=%s, %s=%s",
	    KEY_STATE, (is_sam_running == B_TRUE) ?
	    STATE_ON : STATE_OFF,
	    KEY_PATH, path);


	/* Get the flags from dev_ent_t */
	if ((is_sam_running == B_TRUE) && (dev = DEV_ENT(eq)) != NULL) {

		/* convert bit flags to strings */
		idevflags2sdevflags(dev->log.flags, &sflags);

		if (sflags != NULL) {
			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
			    sizeof (buffer) - chars,
			    "%s=%s", KEY_FLAGS, sflags);
			free(sflags);
		}

		if (stat(path, &statbuf) == 0) {
			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
			    sizeof (buffer) - chars,
			    "%s=%ld, %s=%lu",
			    KEY_SIZE, statbuf.st_size,
			    KEY_MODTIME, statbuf.st_mtime);
		}
		*params = (strdup(buffer));
	}
	return (*params);


}


static void
idevflags2sdevflags(
	uint_t iflags,
	char **sflags
)
{
	char buffer[BUFSIZ] = {0};
	int chars = 0;
	int j = 0;

	for (j = DL_detail; j < DL_MAX; j++) {
		if (iflags & (1 << j)) {
			chars += snprintf(buffer + chars,
			    sizeof (buffer) - chars, "%s ", DL_names[j]);
		}
	}
	/* remove the trailing space */
	buffer[chars - 1] = '\0';
	*sflags = strdup(buffer);

}


/*
 * get SAM system log information
 *
 * Information is a list of formatted strings
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_samlog_lst(
	sqm_lst_t **lst_log
)
{
	sam_defaults_t *sam_defaults = NULL;
	char *log_keyword = NULL;
	char *flags = NULL;
	FILE *fp = NULL;
	char linebuf[BUFSIZ];

	char buffer[4096];

	/* log facility from defaults is an int, convert to syslog facility */
	if ((sam_defaults = GetDefaults()) == NULL) {
		Trace(TR_ERR, "Could not get sam log config");
		return (-1);
	}
	log_keyword = ifacility2sfacility(sam_defaults->log_facility);
	if (log_keyword == NULL) {
		Trace(TR_ERR, "Unrecognized system facility for sam log");
		return (-1);
	}

	if ((fp = fopen(SYSLOG_CONF, "r")) == NULL) {
		return (-1);
	}
	*lst_log = lst_create();

	/*
	 * res_stream format:facility.level [ ; facility.level ]<tab>action
	 *
	 * there can be multiple lines with this facility
	 * e.g.
	 * local7.debug	/var/adm/sam-debug
	 * local7.warn	/var/adm/sam-warn
	 * local7.crit	/var/adm/sam-crit
	 */
	while (fgets(linebuf, BUFSIZ, fp)) {

		linebuf[BUFSIZ - 1] = '\0';

		char *lptr = linebuf;

		/* ignore whitespaces, empty lines and comments */
		lptr += strspn(lptr, WHITESPACE);
		if (lptr[0] == CHAR_POUND) {
			continue;
		}
		if (strstr(lptr, log_keyword) == NULL) {
			continue;
		}
		char *facility = NULL, *action = NULL, *lasts = NULL;
		if ((facility = strtok_r(lptr, WHITESPACE, &lasts)) != NULL) {
			action = strtok_r(NULL, WHITESPACE, &lasts);
		}
		if (facility == NULL || action == NULL) { /* ignore */
			continue;
		}
		Trace(TR_MISC, "facility=%s, action=%s", facility, action);
		free(log_keyword);
		log_keyword = NULL;
		flags = NULL;
		lasts = NULL;
		/* tokenize facility to get log_keyword and flag */
		if ((log_keyword = strtok_r(facility, COLON, &lasts)) != NULL) {
			flags = strtok_r(NULL, ":", &lasts);
			flags = (flags != NULL) ? flags : "";
		}

		struct stat statbuf;
		off_t size = 0; long age = 0;

		if (stat(action, &statbuf) == 0) {
			size = statbuf.st_size;
			age = statbuf.st_mtime;
		}
		snprintf(buffer, sizeof (buffer),
		    "%s=%s, %s=%s, %s=%s, %s=%s, %s=%s, %s=%ld, %s=%lu",
		    KEY_NAME,
		    GetCustMsg(SE_SYSTEM_LOG_DESC),
		    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
		    KEY_STATE, STATE_ON,
		    KEY_PATH, action,
		    KEY_FLAGS, flags,
		    KEY_SIZE, size,
		    KEY_MODTIME, age);

		lst_append(*lst_log, strdup(buffer));
		buffer[0] = '\0';
	}
	fclose(fp);
	Trace(TR_MISC, "SAM system log info obtained");
	return (0);

}

/*
 * DEPRECATED: get_samlog_info
 * SAM can be configured to log to multiple files, so use get_samlog_lst instead
 * to be removed after monitor.c is modified to use get_samlog_lst
 */
int
get_samlog_info(
	char **info
)
{
	sqm_lst_t *lst = NULL;
	if (get_samlog_lst(&lst) == 0) {

		node_t *node = lst->head;
		if (node != NULL) {

			*info = strdup((char *)node->data);
			return (0);
		}
	}
	return (-1);
}


/*
 * table containing the facility as an int and its corresponding string
 * as used in syslog.conf
 */
#define	MAX_LOCAL_LOG_FACILITY	8

static struct tab_facility {
	int ifacility;	/* e.g. LOG_LOCAL7 */
	char sfacility[16]; /* e.g. local7 */
} facility[] = {
	{LOG_LOCAL0,	"local0"},
	{LOG_LOCAL1,	"local1"},
	{LOG_LOCAL2,	"local2"},
	{LOG_LOCAL3,	"local3"},
	{LOG_LOCAL4,	"local4"},
	{LOG_LOCAL5,	"local5"},
	{LOG_LOCAL6,	"local6"},
	{LOG_LOCAL7,	"local7"}
	};

static char *
ifacility2sfacility(
	int ifacility	/* from defaults.conf, e.g. LOG_LOCAL7 */
)
{
	int pos;

	for (pos = 0; pos < MAX_LOCAL_LOG_FACILITY; pos++) {
		if (ifacility == facility[pos].ifacility) {
			return (strdup(facility[pos].sfacility));
		}
	}
	return (NULL);
}

/*
 * get recycler log information
 *
 * Returns a formatted string
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_recyclelog_info(
	char **info
)
{

	char buffer[BUFSIZ] = {0};
	int chars = 0;
	struct stat statbuf;
	upath_t rc_log = {0};

	*info = NULL; /* initialize */

	get_rc_log(NULL, rc_log);

	chars = snprintf(buffer, sizeof (buffer),
	    "%s=%s, %s=%s, %s=%s",
	    KEY_NAME, GetCustMsg(SE_RECYCLE_LOG_DESC),
	    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
	    KEY_STATE, (rc_log[0] != '\0') ?
	    STATE_ON : STATE_OFF);

	if (rc_log[0] != '\0') {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%s", KEY_PATH, rc_log);
	}
	/* no flags ? rc_log_opts_t */
	if (stat(rc_log, &statbuf) == 0) {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%ld, %s=%lu",
		    KEY_SIZE, statbuf.st_size,
		    KEY_MODTIME, statbuf.st_mtime);
	}

	*info = strdup(buffer);
	return (0);
}

/*
 * get archiver log info, there is one archive log per file system
 *
 * Returns a formatted string
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_archivelog_info(
	sqm_lst_t **lst_archivelog
)
{

	ar_fs_directive_t *ar_fs_dir = NULL;
	ar_global_directive_t *ar_global = NULL;
	sqm_lst_t *lst_ar_fs_dir = NULL;
	node_t *node_ar_fs_dir = NULL;

	char buffer[BUFSIZ] = {0};
	int chars = 0;
	struct stat statbuf;

	/* get all file system directives for archiving */
	if (get_all_ar_fs_directives(NULL, &lst_ar_fs_dir) == -1) {
		return (-1);
	}

	/* get archiver log global directive */
	if (get_ar_global_directive(NULL, &ar_global) == -1) {

		lst_free_deep_typed(lst_ar_fs_dir,
		    FREEFUNCCAST(free_ar_fs_directive));
		return (-1);
	}

	*lst_archivelog = lst_create();

	/* add the global archive directive */
	chars = snprintf(buffer, sizeof (buffer),
	    "%s=%s, %s=%s, %s=%s",
	    KEY_NAME, GetCustMsg(SE_ARCHIVE_GLOBAL_LOG_DESC),
	    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
	    KEY_STATE, (ar_global->log_path[0] != '\0') ?
	    STATE_ON : STATE_OFF);

	if (ar_global->log_path[0] != '\0') {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%s", KEY_PATH, ar_global->log_path);

		if (stat(ar_global->log_path, &statbuf) == 0) {
			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
			    sizeof (buffer) - chars,
			    "%s=%ld, %s=%lu",
			    KEY_SIZE, statbuf.st_size,
			    KEY_MODTIME, statbuf.st_mtime);
		}
	}
	lst_append(*lst_archivelog, strdup(buffer));

	/* now check if any filesystems have overloaded the global log */
	node_ar_fs_dir = lst_ar_fs_dir->head;
	while (node_ar_fs_dir != NULL) {
		ar_fs_dir = (ar_fs_directive_t *)node_ar_fs_dir->data;

		buffer[0] = '\0'; chars = 0;
		if ((ar_fs_dir != NULL) &&
		    strcmp(ar_global->log_path, ar_fs_dir->log_path))  {

			/* Get the fsname and the logpath */
			chars = snprintf(buffer, sizeof (buffer),
			    "%s=%s %s, %s=%s, %s=%s",
			    KEY_NAME, GetCustMsg(SE_ARCHIVE_LOG_DESC),
			    ar_fs_dir->fs_name,
			    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
			    KEY_STATE, (ar_fs_dir->log_path[0] != '\0') ?
			    STATE_ON : STATE_OFF);


			if (ar_fs_dir->log_path[0] != '\0') {
				ADD_DELIM(chars, buffer);
				chars += snprintf(buffer + chars,
				    sizeof (buffer) - chars,
				    "%s=%s", KEY_PATH, ar_fs_dir->log_path);

				if (stat(ar_fs_dir->log_path, &statbuf) == 0) {
					ADD_DELIM(chars, buffer);
					chars += snprintf(buffer + chars,
					    sizeof (buffer) - chars,
					    "%s=%ld, %s=%lu",
					    KEY_SIZE, statbuf.st_size,
					    KEY_MODTIME, statbuf.st_mtime);
				}
			}
			lst_append(*lst_archivelog, strdup(buffer));
		}
		buffer[0] = '\0'; chars = 0;
		node_ar_fs_dir = node_ar_fs_dir->next;
	}
	lst_free_deep_typed(lst_ar_fs_dir, FREEFUNCCAST(free_ar_fs_directive));
	free_ar_global_directive(ar_global);
	return (0);
}

/*
 * get log information for stager
 *
 * Returns a formatted string
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_stagelog_info(
	char **info
)
{

	char buffer[BUFSIZ] = {0};
	int chars = 0;
	struct stat statbuf;
	stager_cfg_t *cfg = NULL;

	*info = NULL; /* initialize */

	/* get stager configuration */
	if (get_stager_cfg(NULL, &cfg) == -1) {
		return (-1);
	}

	chars = snprintf(buffer, sizeof (buffer),
	    "%s=%s, %s=%s, %s=%s",
	    KEY_NAME, GetCustMsg(SE_STAGE_LOG_DESC),
	    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
	    KEY_STATE, (cfg->stage_log[0] != '\0') ?
	    STATE_ON : STATE_OFF);

	if (cfg->stage_log[0] != '\0') {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%s", KEY_PATH, cfg->stage_log);
		/* no flags ? */
		if (stat(cfg->stage_log, &statbuf) == 0) {
			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
			    sizeof (buffer) - chars,
			    "%s=%ld, %s=%lu",
			    KEY_SIZE, statbuf.st_size,
			    KEY_MODTIME, statbuf.st_mtime);
		}
	}
	free_stager_cfg(cfg);
	*info = strdup(buffer);
	return (0);
}

/*
 * get log information for releaser (one log per file system)
 *
 * Returns a formatted string
 *
 * name=name,
 * type=Log/Trace,
 * state=on/off,
 * path=filename,
 * flags=flags,
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
int
get_releaselog_info(
	sqm_lst_t **lst_releaselog
)
{

	char buffer[BUFSIZ] = {0};
	char global_log[128] = {0};
	int chars = 0;
	sqm_lst_t *lst_rel_fs_dir = NULL;
	node_t *node_rel_fs_dir = NULL;
	rl_fs_directive_t *rel_fs_dir = NULL;
	struct stat statbuf;

	/* get releaser configuration */
	if (get_all_rl_fs_directives(NULL, &lst_rel_fs_dir) == -1) {
		return (-1);
	}

	*lst_releaselog = lst_create(); /* initialize */

	node_rel_fs_dir = lst_rel_fs_dir->head;
	for (node_rel_fs_dir = lst_rel_fs_dir->head;
	    node_rel_fs_dir != NULL;
	    node_rel_fs_dir = node_rel_fs_dir->next) {

		char releaser_log[128]; releaser_log[0] = '\0';
		buffer[0] = '\0'; chars = 0;

		rel_fs_dir = (rl_fs_directive_t *)node_rel_fs_dir->data;

		/*
		 * Get the global properties logfile
		 * assume it would always be at the head of the list
		 */
		if (strcmp(rel_fs_dir->fs, GLOBAL) == 0) {
			strlcpy(global_log, rel_fs_dir->releaser_log,
			    sizeof (global_log));

			chars = snprintf(buffer, sizeof (buffer),
			    "%s=%s, %s=%s, %s=%s",
			    KEY_NAME,
			    GetCustMsg(SE_RELEASE_GLOBAL_LOG_DESC),
			    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
			    KEY_STATE, (global_log[0] != '\0') ?
			    STATE_ON : STATE_OFF);

			if (global_log[0] != '\0') {
				ADD_DELIM(chars, buffer);
				chars += snprintf(buffer + chars,
				    sizeof (buffer) - chars,
				    "%s=%s",
				    KEY_PATH, global_log);
				/* no flags ? */
				if (stat(global_log, &statbuf) == 0) {
					ADD_DELIM(chars, buffer);
					chars += snprintf(buffer + chars,
					    sizeof (buffer) - chars,
					    "%s=%ld, %s=%lu",
					    KEY_SIZE, statbuf.st_size,
					    KEY_MODTIME, statbuf.st_mtime);
				}
			}
			lst_append(*lst_releaselog, strdup(buffer));
			continue;

		} else {
			/* not a global property logfile */

			/*
			 * If the file system does not have a release logfile
			 * associated with it, skip
			 */
			if ((rel_fs_dir->releaser_log[0] != '\0') &&
			    (global_log[0] != '\0') &&
			    (strcmp(rel_fs_dir->releaser_log,
			    global_log) != 0)) {

				/* logfile specific to file system */
				chars = snprintf(buffer, sizeof (buffer),
				    "%s=%s %s, %s=%s, %s=%s",
				    KEY_NAME,
				    GetCustMsg(SE_RELEASE_LOG_DESC),
				    rel_fs_dir->fs,
				    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
				    KEY_STATE,
				    (rel_fs_dir->releaser_log[0] != '\0') ?
				    STATE_ON : STATE_OFF);

				if (rel_fs_dir->releaser_log[0] != '\0') {
					ADD_DELIM(chars, buffer);
					chars += snprintf(buffer + chars,
					    sizeof (buffer) - chars,
					    "%s=%s",
					    KEY_PATH,
					    rel_fs_dir->releaser_log);
					/* no flags ? */
					if (stat(rel_fs_dir->releaser_log,
					    &statbuf) == 0) {
						ADD_DELIM(chars, buffer);
						chars +=
						    snprintf(buffer + chars,
						    sizeof (buffer) - chars,
						    "%s=%ld, %s=%lu",
						    KEY_SIZE,
						    statbuf.st_size,
						    KEY_MODTIME,
						    statbuf.st_mtime);
					}
				}
				lst_append(*lst_releaselog, strdup(buffer));
			}
		}
	}
	lst_free_deep(lst_rel_fs_dir);

	return (0);
}


/*
 * get file status (wrapper around stat)
 * Return a formated string using key-value pairs
 *
 * name=name, -  user friendly descriptive name
 * type=Log,
 * state=on/off,
 * path=filename,
 * flags=flags, - not applicable
 * size=size,
 * modtime=last modified time (num of seconds since 1970)
 */
static int
get_filestat(
	char *filename, /* input filename to get status on */
	int captionId, /* id to retrieve user friendly name from catalog.msg */
	char **info	/* return key-value pairs */
)
{
	char buffer[BUFSIZ] = {0};
	int chars = 0;
	struct stat statbuf;

	*info = NULL; /* initialize */

	chars = snprintf(buffer, sizeof (buffer),
	    "%s=%s, %s=%s, %s=%s",
	    KEY_NAME, GetCustMsg(captionId),
	    KEY_TYPE, GetCustMsg(SE_TYPE_LOG),
	    KEY_PATH, filename);


	/* no flags */
	if (stat(filename, &statbuf) == 0) {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%s, %s=%ld, %s=%lu",
		    KEY_STATE, STATE_ON,
		    KEY_SIZE, statbuf.st_size,
		    KEY_MODTIME, statbuf.st_mtime);
	} else {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		    "%s=%s", KEY_STATE, STATE_OFF);
	}

	*info = strdup(buffer);
	return (0);
}
