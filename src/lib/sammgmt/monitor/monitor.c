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
#pragma	ident	"$Revision: 1.22 $"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "sam/mount.h"
#include "sam/sam_trace.h"
#include "aml/id_to_path.h"
#include "mgmt/util.h"
#include "mgmt/log.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/monitor.h"
#include "pub/mgmt/file_util.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/load.h"
#include "pub/mgmt/stage.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/diskvols.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
#define	ADD_DELIM(chars, buffer) \
	if (chars != 0) { \
		snprintf(buffer + chars, sizeof (buffer) - chars, ", "); \
		chars += 2; \
	}

#define	STATUS_OK	0
#define	STATUS_WARNING	1
#define	STATUS_ERR	2
#define	STATUS_FAILURE	3

#define	Q_THRESHOLD_WAITTIME_DEFAULT 		1800	/* 30 minutes */
#define	Q_THRESHOLD_LENGTH_DEFAULT		100	/* items in queue */

/* helper function to add to status list */
#define	ADD2LIST() \
	dup_str = strdup(status_str); \
	if (ISNULL(dup_str)) { \
		goto error; \
	} \
	if (lst_append(*status_lst, dup_str) == -1) { \
		goto error; \
	}

/* init vars in get_component_status_summary */
#define	INIT_VARS() \
	lst = NULL; \
	status_flag = STATUS_FAILURE; \
	status_str[0] = '\0';

static parsekv_t arcopy_tokens[] = {
	{"name",	offsetof(arcopy_t, name),	parsekv_string_32},
	{"type",	offsetof(arcopy_t, mtype),	parsekv_string_3},
	{"capacity",	offsetof(arcopy_t, capacity),	parsekv_llu},
	{"free",	offsetof(arcopy_t, free),	parsekv_llu},
	{"usage",	offsetof(arcopy_t, usage),	parsekv_int},
	{"",		0,				NULL}
};

static int _is_DVOL(char *mntpoint, boolean_t *dv_flag);

/*
 * Summarize the state of the system, return list of formatted strings
 * with the component name and status (OK, WARNING, ERR, FAILURE)
 * In future, the error or warning message may also be returned
 *
 * Returned list of strings:-
 *	name=daemons,status=0
 *	name=fs,status=0
 *	name=copyUtil,status=0
 *	name=libraries,status=0
 *	name=drives,status=0
 *	name=loadQ,status=0
 *	name=unusableVsn,status=0
 *	name=arcopyQ,status=0
 *	name=stageQ,status=0
 */
/*
 * Performance:- TBD:
 * Use multiple threads to collect the status for different components
 * and cache it ?
 */
int
get_component_status_summary(
	/* LINTED E_FUNC_ARG_UNUSED */
	ctx_t *ctx,
	sqm_lst_t **status_lst) 	/* list of formatted strings */
{
	node_t *node			= NULL;
	sqm_lst_t *lst			= NULL;

	int32_t status_flag		= STATUS_FAILURE;
	int32_t drive_flag		= STATUS_FAILURE;
	int32_t lib_flag		= STATUS_FAILURE;
	int timeout			= 0;
	char status_str[BUFSIZ]		= {0};
	char *dup_str			= NULL;

	char *str			= NULL;
	fs_t *fs			= NULL;
	arcopy_t acopy;
	pending_load_info_t *load	= NULL;
	struct ArchReq *archreq		= NULL;
	uint32_t total_stagefiles	= 0;
	library_t *lib;
	node_t *node_drive;
	drive_t *drive;
	boolean_t dv_flag;

	if (ISNULL(status_lst)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	*status_lst = lst_create();
	if (*status_lst == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/* Application daemons - status is calculated on daemon status */
	if (get_status_processes(NULL, &lst) == 0) {
		status_flag = STATUS_OK;
		node = lst->head;
		while (node != NULL) {
			str = (char *)node->data;

			if (ISNULL(str)) {
				goto error;
			}

			/*
			 * If activityid=pid is absent, i.e. daemon is not
			 * running, set daemon status = ERR
			 */
			if (strstr(str, "activityid=") == NULL) {
				status_flag = STATUS_ERR;
				break;
			}

			node = node->next;
		}
		lst_free_deep(lst);
	}
	snprintf(status_str, sizeof (status_str),
	    "name=daemons,status=%d", status_flag);
	ADD2LIST();

	/* File Systems - status is calculated on utilization */
	INIT_VARS();

	/*
	 * An fs that is over the HWM is a condition that users should be aware
	 * of. Over HWM is indicated as a warning because it will occur in a
	 * healthy active system. An archiving file system that is full or is
	 * almost full (halfway between HWM and FULL) is flagged as ERROR
	 */
	if (get_all_fs(NULL, &lst) == 0) {
		status_flag = STATUS_OK;
		node = lst->head;
		for (node = lst->head; node != NULL; node = node->next) {
			fs = (fs_t *)node->data;

			if (ISNULL(fs)) {
				goto error;
			}

			fsize_t used = (fs->fi_capacity - fs->fi_space);
			int usage = 0;
			if (fs->fi_capacity > 0 && used > 0) {
				usage = (used / fs->fi_capacity) * 100;
			}

			/*
			 * SAM-FS: If usage > hwm, STATUS = WARNING
			 * SAM-FS: If usage is halfway between hwm and full,
			 * STATUS = ERROR
			 */
			if (fs->fi_archiving && fs->mount_options != NULL) {
				int hwm = fs->mount_options->sam_opts.high;

				if ((fs->fi_status & FS_MOUNTED) &&
				    (usage > hwm)) {

					int threshold = hwm + ((100 - hwm) / 2);
					if (usage > threshold) {
						status_flag = STATUS_ERR;
					} else {
						status_flag = STATUS_WARNING;
					}
					break;
				}
			} else {
			/*
			 * no-archiving fs:- If the fs is used as disk archives,
			 * it is not an error or warning if the usage is high;
			 * this is the expected behavior. For other
			 * non-archiving fs, if usage > 80%, STATUS = WARNING
			 *
			 */
				if ((_is_DVOL(fs->fi_mnt_point, &dv_flag)
				    == 0) && (dv_flag == B_TRUE)) {
					continue;
				}
				if (usage > 80) {
					status_flag = STATUS_WARNING;
					break;
				}
			}

		}
		lst_free_deep_typed(lst, FREEFUNCCAST(free_fs));
	}
	snprintf(status_str, sizeof (status_str),
	    "name=fs,status=%d", status_flag);
	ADD2LIST();

	/* Archive Media Utilization - if util > 80%, err */
	INIT_VARS();

	if (get_copy_utilization(NULL, 10 /* top 10 */, &lst) == 0) {
		status_flag = STATUS_OK;
		node = lst->head;
		while (node != NULL) {
			if (parse_kv((char *)node->data,
			    &arcopy_tokens[0],
			    (void *)&acopy) == 0) {

				if (acopy.usage > 80) {
					status_flag = STATUS_WARNING;
					break;
				}
			}

			node = node->next;
		}
		lst_free_deep(lst);
	}
	snprintf(status_str, sizeof (status_str),
	    "name=copyUtil,status=%d", status_flag);
	ADD2LIST();

	/*
	 * Tape Libraries and drives -
	 * if unable to get status, set status flag to 'Err'
	 * If state of drive or library is DEV_OFF or DEV_DOWN,
	 * set status flag to warning
	 */
	INIT_VARS();

	if (get_all_libraries(NULL, &lst) == 0) {
		lib_flag = STATUS_OK;
		drive_flag = STATUS_OK;

		node = lst->head;
		while (node != NULL) {
			lib = (library_t *)node->data;

			if (lib->base_info.state == DEV_OFF ||
			    lib->base_info.state == DEV_DOWN) {

				lib_flag = STATUS_WARNING;
			}
			node_drive = lib->drive_list->head;
			while (node_drive != NULL) {
				drive = (drive_t *)node_drive->data;

				if (drive->base_info.state == DEV_OFF ||
				    drive->base_info.state == DEV_DOWN) {

					drive_flag = STATUS_WARNING;
					break;
				}
				node_drive = node_drive->next;
			}

			node =  node->next;
		}

		lst_free_deep_typed(lst, FREEFUNCCAST(free_library));
	}

	snprintf(status_str, sizeof (status_str),
	    "name=libraries,status=%d", lib_flag);
	ADD2LIST();
	status_str[0] = '\0';
	snprintf(status_str, sizeof (status_str),
	    "name=drives,status=%d", drive_flag);
	ADD2LIST();

	/* Pending load requests - if wait time > 30 min, err */
	INIT_VARS();
	status_flag = STATUS_OK;
	if (get_pending_load_info(NULL, &lst) == 0) {
		node = lst->head;
		while (node != NULL) {
			load = (pending_load_info_t *)node->data;

			if (ISNULL(load)) {
				goto error;
			}
			/* if wait time is > than 30 minutes */
			if ((time(NULL) - load->ptime) >
			    Q_THRESHOLD_WAITTIME_DEFAULT) {

				status_flag = STATUS_WARNING;
				break;
			}
			node = node->next;
		}
		lst_free_deep(lst);
	}
	snprintf(status_str, sizeof (status_str),
	    "name=loadQ,status=%d", status_flag);
	ADD2LIST();

	/*
	 * Unusable VSNS -
	 * if vsn is unavailable, duplicate, foreign media or damaged
	 * set status to STATUS_WARNING
	 */
	INIT_VARS();
	status_flag = STATUS_OK;
	if (get_vsns(
	    NULL,
	    EQU_MAX,
	    CES_unavail | CES_dupvsn | CES_non_sam | CES_bad_media,
	    &lst) == 0) {

		status_flag = lst->length > 0 ? STATUS_WARNING : STATUS_OK;

		lst_free_deep(lst);
	}
	snprintf(status_str, sizeof (status_str),
	    "name=unusableVsns,status=%d", status_flag);
	ADD2LIST();

	/* Archiving copy queue */
	INIT_VARS();
	status_flag = STATUS_OK;

	/*
	 * Archive requests are considered to be operations that write to the
	 * archive media. The queue timeout is optinally provided as input via
	 * the archiver configuration. Get the timeout and use it if is set to
	 * a value greater than Q_THRESHOLD_WAITTIME_DEFAULT (30 minutes)
	 *
	 * If unable to determine, use default threshold value
	 */
	get_archreq_qtimeout(&timeout); /* ignore errors */
	timeout = (timeout > Q_THRESHOLD_WAITTIME_DEFAULT) ?
	    timeout : Q_THRESHOLD_WAITTIME_DEFAULT;

	if (get_all_archreqs(NULL, &lst) == 0) {
		node = lst->head;

		while (node != NULL) {
			archreq = (struct ArchReq *)node->data;

			if (ISNULL(archreq)) {
				goto error;
			}
			if ((time(NULL) - archreq->ArTime) > timeout) {

				status_flag = STATUS_WARNING;
				break;
			}
			node = node->next;

		}
		lst_free_deep(lst);
	}
	snprintf(status_str, sizeof (status_str),
	    "name=arcopyQ,status=%d", status_flag);
	ADD2LIST();

	/* Staging queue */
	INIT_VARS();
	status_flag = STATUS_OK;

	if (get_total_staging_files(NULL, &total_stagefiles) == 0) {
		status_flag = (total_stagefiles > Q_THRESHOLD_LENGTH_DEFAULT) ?
		    STATUS_WARNING : STATUS_OK;
	}
	snprintf(status_str, sizeof (status_str),
	    "name=stageQ,status=%d", status_flag);
	ADD2LIST();

	return (0);
error:
	Trace(TR_ERR, "get monitor status summary failed: %d [%s]",
	    samerrno, samerrmsg);
	if (dup_str != NULL) {
		free(dup_str);
	}
	lst_free_deep(lst);
	lst_free_deep(*status_lst);
	return (-1);
}


/*
 * get_status_processes
 * Returns a list of formatted strings that represent daemons and process
 * (related to SAM activity) that are running or are expected to run.
 *
 * activityid=pid (if absent, then it is not running)
 * details=daemon or process name
 * type=SAMDXXXX
 * description=long user friendly name
 * starttime=secs
 * parentid=ppid
 * modtime=secs
 * path=/var/opt/SUNWsamfs/devlog/21
 */
int
get_status_processes(
	/* LINTED E_FUNC_ARG_UNUSED */
	ctx_t *ctx,
	sqm_lst_t **proclst
) {
	char buffer[BUFSIZ] = {0};
	char *keyvalue = NULL; /* format str from list_activities() */
	sqm_lst_t *loglst = NULL; /* get log information for daemons */
	char *loginfo = NULL; /* single log */
	sqm_lst_t *lst = NULL;
	node_t *node = NULL;
	node_t *lognode = NULL;

	/* Check for daemons that are expected to be running */
	sqm_lst_t *liblst = NULL;
	sqm_lst_t *drivelst = NULL;
	boolean_t is_sam_running = B_FALSE;
	boolean_t is_catd_running = B_FALSE;
	boolean_t is_fsmgmt_running = B_FALSE;

	if (ISNULL(proclst)) {
		goto error;
	}

	*proclst = lst_create();
	if (*proclst == NULL) {
		goto error;
	}

	/* get all daemons/activities currently in progress */
	if (list_activities(NULL, 100, "type=SAMD", &lst) != 0) {
		goto error;
	}

	for (node = lst->head; node != NULL; node = node->next) {
		keyvalue = (char *)node->data;
		if (strstr(keyvalue, "type=SAMDAMLD") != NULL) {
			is_sam_running = B_TRUE;
			get_samlog_info(&loginfo);
		} else if (strstr(keyvalue, "type=SAMDCATSERVERD") != NULL) {
			is_catd_running = B_TRUE;

			if (get_devlog_info(&loglst) == 0) {
				/* get the first log */
				lognode = loglst->head;
				if (lognode != NULL) {
					loginfo =
						strdup(lognode->data);
				}
				lst_free(loglst);
			}
		} else if (strstr(keyvalue, "type=SAMDARCHIVERD") != NULL) {
			if (get_archivelog_info(&loglst) == 0) {
				/* get the first log */
				lognode = loglst->head;
				if (lognode != NULL) {
					loginfo =
						strdup(lognode->data);
				}
				lst_free(loglst);
			}
		} else if (strstr(keyvalue, "type=SAMDMGMTD") != NULL) {
			is_fsmgmt_running = B_TRUE;
		} else if (strstr(keyvalue, "type=SAMDSTAGERD") != NULL) {
			get_stagelog_info(&loginfo);
		} /* else ignore */

		/* append log information to keyvalue */
		if (loginfo != NULL) {
			snprintf(buffer, sizeof (buffer), "%s,%s",
				keyvalue, loginfo);
		} else {
			strlcpy(buffer, keyvalue, sizeof (buffer));
		}
		/* populate daemon list with process and log info */
		if (lst_append(*proclst, strdup(buffer)) != 0) {
			goto error;
		}
		loginfo = NULL;
		buffer[0] = '\0';
	}
	lst_free_deep(lst);

	/* TBD: nfsd, cifsd, smbd */

	buffer[0] = '\0';
	/* fsmgmtd should always be running */
	if (is_fsmgmt_running == B_FALSE) {
		/* flag as warning */
		snprintf(buffer, sizeof (buffer),
			"details=fsmgmtd,type=SAMDMGMTD,description=%s",
			GetCustMsg(SE_MGMT_DAEMON_NAME));
		if (lst_append(*proclst, strdup(buffer)) != 0) {
			goto error;
		}
	}

	if (is_sam_running == B_TRUE && is_catd_running == B_TRUE) {
		return (0); /* processing complete */
	}

	/*
	 * If file systems are mounted and library is configured,
	 * SAMDAMLD and SAMDCATSERVERD should be running
	 */
	if (((get_all_libraries(NULL, &liblst) == 0) &&
		liblst->length > 0) ||
		((get_all_standalone_drives(NULL, &drivelst) == 0) &&
		drivelst->length > 0)) {

		buffer[0] = '\0';
		if (is_sam_running == B_FALSE) {
			snprintf(buffer, sizeof (buffer),
				"details=sam-amld,type=SAMDAMLD,description=%s",
				GetCustMsg(SE_AMLD_DAEMON_NAME));
			if (lst_append(*proclst, strdup(buffer)) != 0) {
				goto error;
			}
		}
		buffer[0] = '\0';
		if (is_catd_running == B_FALSE) {
			snprintf(buffer, sizeof (buffer),
				"details=sam-catserverd,type=SAMDCATSERVERD,"
				"description=%s",
				GetCustMsg(SE_CATSERVER_DAEMON_NAME));
			if (lst_append(*proclst, strdup(buffer)) != 0) {
				goto error;
			}
		}
	}

	/* TBD: Stager info, should stager be running, does cfg have errors? */

	if (liblst != NULL) {
		lst_free_deep(liblst);
	}
	if (drivelst != NULL) {
		lst_free_deep(drivelst);
	}
	return (0);
error:
	if (liblst != NULL) {
		lst_free_deep(liblst);
	}
	if (drivelst != NULL) {
		lst_free_deep(drivelst);
	}
	if (*proclst != NULL) {
		lst_free_deep(*proclst);
	}
	return (-1);
}

/*
 * Identify if the filesytem specified by its mount point is used as a disk
 * archive.
 * Best Effort: Check if any disk volumes are mounted from the given mount point
 *
 * Returns 0 if able to determine that mount point is used for disk archiving
 * diskarchive is set to B_TRUE if mount point is used for disk archiving
 * diskarchive is set to B_FALSE if mount point is not used for disk archiving
 *
 * returns -1 if unable to make the determination
 */
static int
_is_DVOL(char *mntpoint, boolean_t *dv_flag)
{

	size_t len;
	sqm_lst_t *dvols;
	node_t *node;
	char dpath[MAXPATHLEN] = {0};
	boolean_t match_flag = B_FALSE;

	if (mntpoint == NULL) {
		Trace(TR_ERR, "unable to check if mntpoint is a disk archive");
		return (-1);
	}

	len = strlen(mntpoint);

	Trace(TR_OPRMSG, "check if mntpoint[%s] is a disk archive", mntpoint);

	if (get_all_disk_vols(NULL, &dvols) != 0) {
		Trace(TR_ERR, "unable to check if mntpoint is a disk archive");
		return (-1);
	}

	for (node = dvols->head; node != NULL; node = node->next) {

		disk_vol_t *dvol = (disk_vol_t *)node->data;

		if (dvol == NULL) {
			continue;
		}

		/* get its realpath */
		if (realpath(dvol->path, dpath) == NULL) {
			continue;
		}

		if (strncmp(dpath, mntpoint, len) == 0) {

			if ((strlen(dpath) == len) ||
			    (strlen(dpath) > len && dpath[len] == '/')) {

				/* exact match or is a subdir in the mntpoint */
				match_flag = B_TRUE;
				break;
			}
		}

	}
	lst_free_deep(dvols);

	*dv_flag = match_flag;

	Trace(TR_OPRMSG, "check mntpoint is a disk archive complete");
	return (0);
}
