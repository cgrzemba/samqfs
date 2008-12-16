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
#pragma	ident	"       $Revision: 1.34 $ "

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <dlfcn.h>
#include "sam/custmsg.h"
#include "pub/devstat.h"
#include "sam/mount.h"
#include "sam/types.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/report.h"
#include "pub/mgmt/monitor.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/diskvols.h"
#include "pub/mgmt/file_util.h"

#include "sam/sam_trace.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	DAY_SECONDS	87600	/* 24 * 60 *60 */
#define	STKAPILIB	OPT_DIR"/lib/libstkapi.so"


static int write_vsn_errors_in_xml(FILE *ptr, uint32_t flags);
static int write_copy_util_in_xml(FILE *ptr);
static int write_vsn_summary_in_xml(FILE *ptr);
static int write_pool_summary_in_xml(FILE *ptr);
static int write_vsn_blank_in_xml(FILE *ptr);
static int write_vsn_accessed_in_xml(FILE *ptr, int32_t accessTime);
static int write_acsls_scratchpool_in_xml(FILE *ptr, sqm_lst_t *pools);
static int write_acsls_driveinfo_in_xml(FILE *ptr, sqm_lst_t *drives);
static int write_acsls_volume_in_xml(FILE *ptr, sqm_lst_t *volumes);
static int write_acsls_lock_in_xml(FILE *ptr, sqm_lst_t *locks);

static int
get_hwm_exceed_count(char *fsname, time_t since, int32_t *count);

static void
free_acsls_report(acsls_report_t *acsls_report_info_p);

static int
get_acsls_summary(acsls_report_t **acsls_report,
	char *hostname, int port,
	report_requirement_t *report_req);

/*
 * generate report
 * the requirements are provided in report_arg
 * Currently the requirement option to send email is ignored.
 * (working on the framework to convert XML to email/text format)
 */
int
gen_report(
ctx_t *c /* ARGSUSED */,
report_requirement_t *report_req)
{
	int32_t i;

	if (ISNULL(report_req)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (report_req->report_type == FS_REPORT) {
		i = gen_fs_report(report_req);

	}
	if (report_req->report_type == MEDIA_REPORT) {
		i = gen_media_report(report_req);
	}
	if (report_req->report_type == ACSLS_REPORT) {
		i = gen_acsls_report(report_req);

	}
	return (i);
}


/*
 * Generate File System report.
 *
 * The current file system summary is converted to XML and stored in the
 * reports directory. A subsequent call to get_all_report() will get the
 * list of generated reports
 *
 * Errors while creating the XML file, opening or writing to it are
 * reported immediately and the report generation FAILS. Errors encountered
 * while iterating through the list of filesystems or converting to XML
 * format are reported in the XML file itself, and the report generation
 * is marked as SUCCESS
 *
 * The generated report is as follows:-
 * <?xml version='1.0'?>
 * <report name="fs" time="1148749201">
 * <filesystems>
 * <fs name="%s" mountPnt="%s" mounted="%s" capacity="%llu" free="%llu"
 *	lwm="%d" hwm="%d" timesHwmExceeded="%d"/>
 * </filesystems>
 * </report>
 *
 * For QFS filesystems, lwm, hwm and timesHwmExceeded are not supported
 *
 * If errors encountered when getting the filesystems summary,
 * <error>samerrmsg</error>
 */
int
gen_fs_report(
report_requirement_t *report_req)
{

	char filename[MAXPATHLEN]	= {0};
	FILE *ptr			= NULL;
	int32_t fd			= -1;
	mode_t mode			= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	sqm_lst_t *fs_list 		= NULL;
	node_t *fs_node 		= NULL;
	fs_t *fs			= NULL;
	int count			= 0; /* num of times hwm exceeded */
	time_t yesterday		= time(NULL) - 86400;


	if (ISNULL(report_req)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (create_dir(NULL, REPORT_DIR) != 0) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	snprintf(filename, MAXPATHLEN, "%s/fs-%ld.xml",
	    REPORT_DIR, time(NULL));

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd < 0) {
		samerrno = SE_CANT_OPEN;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), filename);
		return (-1);
	}
	if ((ptr = fdopen(fd, "w")) == NULL) {
		samerrno = SE_FILE_WRITTEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_WRITTEN_FAILED), filename);
		close(fd);
		return (-1);
	}

	/* xml header and root element */
	fprintf(ptr, "%s\n<report name=\"fs\" time=\"%lu\">\n<filesystems>\n",
	    XML_VERSION_STRING,
	    time(NULL));

	/* write error to XML file */
	if (get_all_fs(NULL, &fs_list) == -1) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	// handle return of 0 and -2 (partial success)
	} else {
		for (fs_node = fs_list->head; fs_node != NULL;
		    fs_node = fs_node->next) {

			fs = (fs_t *)fs_node->data;
			/* don't fail if unable to get one fs detail */
			if (ISNULL(fs)) {
				continue;
			}

			fprintf(ptr,
			    "<fs name=\"%s\" mountPnt=\"%s\" mounted=\"%s\""
			    " capacity=\"%llu\" free=\"%llu\" ",
			    fs->fi_name,
			    fs->fi_mnt_point,
			    ((fs->fi_status & FS_MOUNTED) == 0) ? "No" : "Yes",
			    fs->fi_capacity,
			    fs->fi_space);

			/* If SAM-QFS then include hwm information */
			if (fs->fi_archiving == B_TRUE &&
			    fs->mount_options != NULL) {

				fprintf(ptr, "hwm=\"%d\" lwm=\"%d\" ",
				    fs->mount_options->sam_opts.high,
				    fs->mount_options->sam_opts.low);

				/*
				 * Best effort: get the number of times
				 * hwm was exceeded in the last 24 hours
				 */
				if (get_hwm_exceed_count(
				    fs->fi_name, yesterday, &count) == 0) {

					fprintf(ptr, "timesHwmExceeded=\"%d\"",
					    count);
				} else {
					fprintf(ptr, "timesHwmExceeded=\"-1\"");
				}
			}
			/* end the fs tag */
			fprintf(ptr, "/>\n");
		}
		lst_free_deep_typed(fs_list, FREEFUNCCAST(free_fs));

	}

	/* xml closing tags */
	fprintf(ptr, "</filesystems>\n</report>\n");

	fclose(ptr);

	Trace(TR_MISC, "finished generating file system report");
	return (0);
}




#define	MEDIA_TYPE_DISK	disk(dk)
/*
 * Generate generic media report.
 *
 * Errors while creating the XML file, opening or writing to it are
 * reported immediately and the report generation FAILS. Errors encountered
 * while iterating through the list of libraies or catalog etc. or
 * converting to XML format are reported in the XML file itself, and the
 * report generation returns SUCCESS
 *
 * The generated report is as follows:-
 * <?xml version='1.0'?>
 * <report name="media" time="1160773789" desc="SAMPLE">
 *   <media>
 *     <vsnSummary>
 *       <vsn type="%s" count="%d" capacity="%llu" free="%llu"/>
 *     </vsnSummary>
 *     <pools>
 *	 <pool name="%s" type="%s" count="%d" free="%llu"/>
 *     </pools>
 *     <copyUtilization>
 *       <copy name="%s" type="%s" capacity="%llu" free="%llu" usage="%d"/>
 *     </copyUtilization>
 *     <errorVsns>
 *       <vsn name="%s" type="%s" status="%ld"/>
 *     </errorVsns>
 *     <accessedVsns>
 *     </accessedVsns>
 *     <blankVsns>
 *       <vsn name="%s" type="%s"/>
 *     </blankVsns>
 *   </media>
 * </report>
 *
 * type is mtype_t, i.e. li, tp etc.
 *
 * If errors encountered when getting writing to the XML file
 * <error>samerrmsg</error>
 */
int
gen_media_report(
report_requirement_t *report_req)
{
	char filename[MAXPATHLEN]	= {0};
	FILE *ptr		= NULL;
	int32_t fd		= -1;
	mode_t mode		= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (ISNULL(report_req)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (create_dir(NULL, REPORT_DIR) != 0) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	snprintf(filename, MAXPATHLEN, "%s/media-%ld.xml",
	    REPORT_DIR, time(NULL));

	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd < 0) {
		samerrno = SE_CANT_OPEN;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), filename);
		return (-1);
	}
	if ((ptr = fdopen(fd, "w")) == NULL) {
		samerrno = SE_FILE_WRITTEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FILE_WRITTEN_FAILED), filename);
		close(fd);
		return (-1);
	}

	/* xml header and root element */
	fprintf(ptr, "%s\n<report name=\"media\" time=\"%lu\">\n<media>\n",
	    XML_VERSION_STRING,
	    time(NULL));

	/* the write_XXX_in_xml functions write the error also to the file */

	if (report_req->section_flag & INCLUDE_VSN_SUMMARY) {
		write_vsn_summary_in_xml(ptr);
	}

	if (report_req->section_flag & INCLUDE_POOL_SUMMARY) {
		write_pool_summary_in_xml(ptr);
	}

	if (report_req->section_flag & INCLUDE_UTIL_SUMMARY) {
		write_copy_util_in_xml(ptr);
	}

	write_vsn_errors_in_xml(ptr, report_req->section_flag);

	if (report_req->section_flag & CES_accessed_today) {
		write_vsn_accessed_in_xml(ptr, 86400); /* last 24 hours */
	}

	if (report_req->section_flag & CES_blank) {
		write_vsn_blank_in_xml(ptr);
	}

	/* xml closing tags */
	fprintf(ptr, "</media>\n</report>\n");

	fclose(ptr);

	Trace(TR_MISC, "finished generating media report");

	return (0);
}

/*
 * Generate ACSLS report.
 * A report is generated for every ACSLS STK library connected to
 * this host, the ACSLS host and port tuple are obtained from
 * get_all_libraries()
 *
 */
int
gen_acsls_report(
report_requirement_t *report_req)
{
	char filename[MAXPATHLEN]	= {0};
	FILE *ptr			= NULL;
	int32_t fd			= -1;
	mode_t mode			= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	sqm_lst_t *lib_list		= NULL;
	node_t *lib_node		= NULL;
	library_t *lib			= NULL;
	acsls_report_t *report		= NULL;

	if (ISNULL(report_req)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	if (create_dir(NULL, REPORT_DIR) != 0) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	if (get_all_libraries(NULL, &lib_list) != 0) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	/*
	 * Iterate over the list of acsls libraries found, a site
	 * could have more than one acsls library in its configuration
	 */
	for (lib_node = lib_list->head; lib_node != NULL;
	    lib_node = lib_node->next) {

		lib = (library_t *)lib_node->data;

		/* find stk libraries */
		if (strcmp(lib->base_info.equ_type, "sk") != 0) {
			continue;
		}
		if (lib->storage_tek_parameter == NULL) {
			/*
			 * get_all_libraries return 0, but
			 * perhaps library information was
			 * obtained from mcf if catserverd
			 * is not running
			 */
			samerrno = SE_CANNOT_GET_ACSLS_HOSTNAME;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));

			free_list_of_libraries(lib_list);
			return (-1);
		}
		snprintf(filename, MAXPATHLEN, "%s/acsls-%s-%ld.xml",
		    REPORT_DIR,
		    lib->storage_tek_parameter->hostname,
		    time(NULL));

		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
		if (fd < 0) {
			samerrno = SE_CANT_OPEN;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), filename);
			free_list_of_libraries(lib_list);
			return (-1);
		}
		if ((ptr = fdopen(fd, "w")) == NULL) {
			samerrno = SE_FILE_WRITTEN_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_FILE_WRITTEN_FAILED), filename);
			free_list_of_libraries(lib_list);
			close(fd);
			return (-1);
		}

		/* xml header and root element */
		fprintf(ptr, "%s\n<report name=\"acsls\" time=\"%lu\">\n"
		    "<acsls>\n",
		    XML_VERSION_STRING,
		    time(NULL));

		/*
		 * Here all the information is obtained and stored in
		 * report irrespective of whether it is requested or not ?
		 * TBD: Change the impl of get_acsls_summary
		 */
		if (get_acsls_summary(&report,
		    lib->storage_tek_parameter->hostname,
		    lib->storage_tek_parameter->portnum,
		    report_req) != 0) {
			fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
		} else {

			if (report_req->section_flag & ACSLS_DRIVE_SUMMARY) {

				write_acsls_driveinfo_in_xml(ptr,
				    report->drive_list);
			}

			if (report_req->section_flag & ACSLS_ACCESSED_VOLUME) {
				fprintf(ptr, "<accessedVolumes>\n");
				write_acsls_volume_in_xml(ptr,
				    report->accessedvolume_list);
				fprintf(ptr, "</accessedVolumes>\n");
			}

			if (report_req->section_flag & ACSLS_ENTERED_VOLUME) {
				fprintf(ptr, "<enteredVolumes>\n");
				write_acsls_volume_in_xml(ptr,
				    report->enteredvolume_list);
				fprintf(ptr, "</enteredVolumes>\n");
			}

			if (report_req->section_flag & ACSLS_POOL) {
				write_acsls_scratchpool_in_xml(ptr,
				    report->pool_list);
			}

			if (report_req->section_flag & ACSLS_LOCK) {
				write_acsls_lock_in_xml(ptr,
				    report->lock_list);
			}
		}

		/* closing tag */
		fprintf(ptr, "</acsls>\n</report>\n");
		fclose(ptr);

		if (report != NULL) {
			free_acsls_report(report);
		}

	}
	if (lib_list != NULL) {
		free_list_of_libraries(lib_list);
	}
	Trace(TR_MISC, "finished acsls report generation");
	return (0);
}


/*
 * TBD: Revisit the implementation, Should this function be aware of
 * dynamic loading of libraries?
 *
 *	Get all ACSLS report needed information.
 *	There are two ways to handle this function:
 *	1> get all required information once.  It is
 *	efficient because less overhead.
 *	2> table driven implementation, acsls function
 *	Got called only it is set.
 *	Method 1 is used.
 */
static int
get_acsls_summary(
acsls_report_t **acsls_report,
char *hostname,
int port,
report_requirement_t *report_req)
{

	int32_t i;
	time_t log_time;
	struct tm tm;
	char report_time[128];
	char time_format[128];

	sqm_lst_t *volume_accessed_list = NULL;
	sqm_lst_t *volume_entered_list = NULL;
	sqm_lst_t *drive_list = NULL;
	sqm_lst_t *drive_lock_list = NULL;
	sqm_lst_t *volume_lock_list = NULL;
	sqm_lst_t *pool_list = NULL;
	sqm_lst_t *volume_info_list = NULL;

	void *lib_handle;
	int32_t (*lib_func) ();
	void (*lib_func1) ();
	stk_host_info_t *stk_host_info;

	if (ISNULL(acsls_report, hostname, report_req)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	log_time = time(NULL);
	*acsls_report = (acsls_report_t *)
	    mallocer(sizeof (acsls_report_t));
	if (*acsls_report == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	memset(*acsls_report, 0, sizeof (acsls_report_t));

	stk_host_info = (stk_host_info_t *)
	    mallocer(sizeof (stk_host_info_t));
	if (stk_host_info == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		goto error;
	}

	strlcpy(stk_host_info->hostname, hostname,
	    sizeof (stk_host_info->hostname));
	sprintf(stk_host_info->portnum, "%d", port);

	lib_handle = dlopen(STKAPILIB, RTLD_LAZY);
	if (!lib_handle) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	Trace(TR_MISC, "host info %s and %s\n",
	    stk_host_info->hostname, stk_host_info->portnum);
	lib_func1 = (void (*)())dlsym(lib_handle, "set_stk_env");
	if (lib_func1 == NULL) {  /* need error number */
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	(*lib_func1) (stk_host_info);

	lib_func1 = (void (*)())dlsym(lib_handle, "start_stk_daemon");
	if (lib_func1 == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	(*lib_func1) (stk_host_info);

	lib_func = (int32_t (*)())dlsym(lib_handle, "get_stk_info_list");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	i = (*lib_func) (NULL, stk_host_info, DRIVE_INFO,
	    NULL, NULL, &drive_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}


	/*
	 *	query stk volume list of accessed volume.
	 */
	log_time = time(NULL) - DAY_SECONDS;
	localtime_r(&log_time, &tm);
	strftime(report_time, sizeof (report_time), "%Y-%m-%d:%H:%M:%S", &tm);
	snprintf(time_format, sizeof (time_format), "> %s", report_time);
	lib_func = (int32_t (*)())dlsym(lib_handle, "get_stk_info_list");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	i = (*lib_func) (NULL, stk_host_info, VOLUME_WITH_DATE_INFO,
	    time_format, "-access",
	    &volume_accessed_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	/*
	 *	query stk volume list of entered volume.
	 */
	lib_func = (int32_t (*)())dlsym(lib_handle, "get_stk_info_list");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	i = (*lib_func) (NULL, stk_host_info, VOLUME_WITH_DATE_INFO,
	    time_format, "-entry",
	    &volume_entered_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	/*
	 *	query stk lock of drive.
	 */
	lib_func = (int32_t (*)())dlsym(lib_handle, "query_stk_lock_drive");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	i = (*lib_func) (stk_host_info, drive_list, &drive_lock_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	/*
	 *	query stk all volume.  The volume information will be used
	 *	to query locked volume.
	 */
	lib_func = (int32_t (*)())dlsym(lib_handle, "get_stk_info_list");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	i = (*lib_func) (NULL, stk_host_info, VOLUME_INFO,
	    NULL, NULL, &volume_info_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	/*
	 *	query stk lock volume using above volume list.
	 */
	lib_func = (int32_t (*)())dlsym(lib_handle, "query_stk_lock_volume");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	i = (*lib_func) (stk_host_info, volume_info_list, &volume_lock_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}


	/*
	 *	query stk pool information.
	 */
	lib_func = (int32_t (*)())dlsym(lib_handle, "get_stk_info_list");
	if (lib_func == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}
	i = (*lib_func) (NULL, stk_host_info, POOL_INFO,
	    NULL, NULL, &pool_list);
	if (i != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}


	(*acsls_report)->drive_list = drive_list;
	(*acsls_report)->accessedvolume_list = volume_accessed_list;
	(*acsls_report)->enteredvolume_list = volume_entered_list;
	/* concat the drive_lock_list and volume_lock_list */
	(*acsls_report)->lock_list = lst_create();
	lst_concat((*acsls_report)->lock_list, drive_lock_list);
	lst_concat((*acsls_report)->lock_list, volume_lock_list);
	(*acsls_report)->pool_list = pool_list;

	if (volume_info_list != NULL) {
		lst_free_deep(volume_info_list);
	}
	if (stk_host_info != NULL) {
		free(stk_host_info);
	}
	return (0);
error:
	if (volume_info_list != NULL) {
		lst_free_deep(volume_info_list);
	}
	if (drive_list != NULL) {
		lst_free_deep(drive_list);
	}
	if (volume_accessed_list != NULL) {
		lst_free_deep(volume_accessed_list);
	}
	if (volume_entered_list != NULL) {
		lst_free_deep(volume_entered_list);
	}
	if (drive_lock_list != NULL) {
		lst_free_deep(drive_lock_list);
	}
	if (volume_lock_list != NULL) {
		lst_free_deep(volume_lock_list);
	}
	if (volume_info_list != NULL) {
		lst_free_deep(volume_info_list);
	}
	if (pool_list != NULL) {
		lst_free_deep(pool_list);
	}
	if (stk_host_info != NULL) {
		free(stk_host_info);
	}
	free_acsls_report(*acsls_report);
	*acsls_report = NULL;
	return (-1);
}

/* Count the number of times the hwm crossed from the given time */
static int
get_hwm_exceed_count(
char *fsname,	/* fsname to look for */
time_t since,	/* hwm exceeded since this time */
int32_t *count) /* return - count */
{

	FILE *ptr 		= NULL;
	char buf[BUFSIZ]	= {0};
	char *rest		= NULL;
	char *tokp		= NULL;
	char *name		= NULL;
	long hwm_exceeded_at	= 0; /* time at which hwm exceeded */
	char *timeStr		= NULL;

	if (ISNULL(fsname, count)) {
		return (-1);
	}

	*count = 0;

	/*
	 * A sysevent is generated when hwm is exceeded, a listener to this
	 * event writes the fsname and the time at which the hwm exceeded
	 * to a HWM_RECORDLOG. (samfs1	1160773789)
	 *
	 * Parse this file, if the fsname matches the input and the time
	 * at which the hwm exceeded is after the given 'since' time,
	 * increment the count
	 */
	ptr = fopen(HWM_RECORDLOG, "r");
	if (ptr == NULL) {
		samerrno = SE_FILE_READ_FAILED;
		/* do not use GetCustMsg and strerror in the same call */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno),
		    HWM_RECORDLOG, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		return (-1);
	}
	while (fgets(buf, BUFSIZ, ptr) != NULL) {

		tokp = strtok_r(buf, WHITESPACE, &rest);
		if (tokp != NULL) {
			name = tokp;

			tokp = strtok_r(NULL, WHITESPACE, &rest);

			if (tokp != NULL) {
				/* this is the time_t when hwm exceeded */
				hwm_exceeded_at = strtol(tokp, &timeStr, 10);
				if (tokp == timeStr) {
					/* non-numeric, incorrect format */
					samerrno = SE_UNABLE_TO_GET_HWMEXCEED;
					snprintf(samerrmsg, MAX_MSG_LEN,
					    GetCustMsg(samerrno));

					fclose(ptr);
					return (-1);
				}
			}

			if (name != NULL &&
			    (strncmp(fsname, name, strlen(fsname)) == 0) &&
			    hwm_exceeded_at > since) {

				(*count)++;
			}
		}
	}
	fclose(ptr);
	return (0);
}

/*
 *	Handle ACSLS volume information xml format report.
 */
static int
write_acsls_volume_in_xml(FILE *ptr, sqm_lst_t *volumes) {

	node_t *n		= NULL;
	stk_volume_t *vol	= NULL;

	if (ISNULL(ptr, volumes)) {
		return (-1);
	}

	/* parent element accessedVolumes/enteredVolumes is written by caller */

	for (n = volumes->head; n != NULL; n = n->next) {

		vol = (stk_volume_t *)n->data;

		if (ISNULL(vol)) {
			continue;
		}
		fprintf(ptr, "<volume name=\"%s\" acsId=\"%d\""
		    " lsmId=\"%d\" panelId=\"%d\" row=\"%d\""
		    " col=\"%d\" poolId=\"%d\" status=\"%s\""
		    " mtype=\"%s\" volType=\"%s\"/>\n",
		    vol->stk_vol,
		    vol->acs_num,
		    vol->lsm_num,
		    vol->panel_num,
		    vol->row_id,
		    vol->col_id,
		    vol->pool_id,
		    vol->status,
		    vol->media_type,
		    vol->volume_type);
	}
	/* closing parent tag added by caller */
	return (0);
}


/*
 *	Handle ACSLS lock information report.
 */
static int
write_acsls_lock_in_xml(FILE *ptr, sqm_lst_t *locks) {

	node_t *n		= NULL;
	acsls_lock_t *lock	= NULL;

	if (ISNULL(ptr)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	fprintf(ptr, "<locks>\n");
	if (locks != NULL) {
		for (n = locks->head; n != NULL; n = n->next) {

			lock = (acsls_lock_t *)n->data;

			if (ISNULL(locks)) {
				continue;
			}
			fprintf(ptr, "<lock identify=\"%s\""
			    " type=\"%s\" lockId=\"%d\""
			    " duration=\"%ld\" pending=\"%d\""
			    " status=\"%s\" userId=\"%s\"/>\n",
			    lock->identifier,
			    lock->type,
			    lock->lock_id,
			    lock->duration,
			    lock->pending,
			    lock->status,
			    lock->user);
		}
	}

	fprintf(ptr, "</locks>\n");
	return (0);
}


/* write vsn summary (disk and tape) in XML format to file */
static int
write_vsn_summary_in_xml(FILE *ptr) {

	sqm_lst_t *vsn_list	= NULL;
	sqm_lst_t *mtype_list	= NULL;
	node_t *node_v		= NULL;
	node_t *node_c		= NULL;
	disk_vol_t *dvol	= NULL;
	struct CatalogEntry *ce = NULL;

	int32_t vsn_count	= 0;
	fsize_t capacity	= 0;
	fsize_t free		= 0;

	if (ISNULL(ptr)) {
		return (-1);
	}

	/* VSN Summary begin */
	fprintf(ptr, "<vsnSummary>\n");

	/* Get all disk vsns */
	if (get_all_disk_vols(NULL, &vsn_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
		/* well-formed xml */

	} else {

		/* Calculate the summary from the list of diskvsns */
		node_v = vsn_list->head;
		while (node_v != NULL) {
			dvol = (disk_vol_t *)node_v->data;

			if (dvol != NULL) {

				vsn_count ++;
				capacity += dvol->capacity;
				free += dvol->free_space;
			}
			node_v = node_v->next;
		}
		fprintf(ptr,
		    "<vsn type=\"dk\" count=\"%d\""
		    " capacity=\"%llu\" free=\"%llu\"/>\n",
		    vsn_count,
		    capacity,
		    free);
		lst_free_deep(vsn_list);
		vsn_list = NULL;
	}

	/* Get all tape vsns by media type */
	if (get_all_available_media_type(NULL, &mtype_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {
		/* for each media type, get the vsns */
		node_c = mtype_list->head;
		while (node_c != NULL) {
			char *mtype = (char *)node_c->data;

			vsn_count = 0;
			capacity = 0;
			free = 0;

			if (get_catalog_entry_list_by_media_type(
			    NULL, mtype, &vsn_list) == 0) {

				node_v = vsn_list->head;
				while (node_v != NULL) {
					ce = (struct CatalogEntry *)
					    node_v->data;

					vsn_count++;
					capacity += ce->CeCapacity;
					free += ce->CeSpace;

					node_v = node_v->next;
				}

				lst_free_deep(vsn_list);

				fprintf(ptr,
				    "<vsn type=\"%s\" count=\"%d\""
				    " capacity=\"%llu\""
				    " free=\"%llu\"/>\n",
				    mtype,
				    vsn_count,
				    capacity,
				    free);
			}
			node_c = node_c->next;
		}
		lst_free_deep(mtype_list);
	}
	fprintf(ptr, "</vsnSummary>\n");
	/* VSN Summary end */

	return (0);
}

/* write archive pool summary in XML format to file */
static int
write_pool_summary_in_xml(FILE *ptr) {

	sqm_lst_t *pool_list	= NULL;
	node_t *node		= NULL;
	vsn_pool_t *pool	= NULL;
	vsnpool_property_t *pp	= NULL;

	if (ISNULL(ptr)) {
		return (-1);
	}

	/* Pool Summary start */
	fprintf(ptr, "<pools>\n");
	if (get_all_vsn_pools(NULL, &pool_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {

		node = pool_list->head;
		while (node != NULL) {

			pool = (vsn_pool_t *)node->data;

			if (get_vsn_pool_properties(NULL, pool,
				0, -1 /* ALL */, VSN_NO_SORT,
				B_TRUE /* ascending */, &pp) == 0) {


				fprintf(ptr,
				    "<pool name=\"%s\" type=\"%s\""
				    " count=\"%d\" free=\"%llu\"/>\n",
				    pp->name,
				    pp->media_type,
				    pp->number_of_vsn,
				    pp->free_space);

				free_vsnpool_property(pp);
			}
			node = node->next;
		}
		lst_free_deep_typed(pool_list,
			FREEFUNCCAST(free_vsn_pool));
	}
	fprintf(ptr, "</pools>\n");
	/* Pool Summary end */

	return (0);
}
static parsekv_t arcopy_tokens[] = {
	{"name",	offsetof(arcopy_t, name),	parsekv_string_32},
	{"type",	offsetof(arcopy_t, mtype),	parsekv_string_3},
	{"capacity",	offsetof(arcopy_t, capacity),	parsekv_llu},
	{"free",	offsetof(arcopy_t, free),	parsekv_llu},
	{"usage",	offsetof(arcopy_t, usage),	parsekv_int},
	{"",		0,				NULL}
};

/* write archive media copy utilization in XML format to file */
static int
write_copy_util_in_xml(FILE *ptr) {

	sqm_lst_t *util_list = NULL;
	node_t *node = NULL;
	arcopy_t acopy;

	fprintf(ptr, "<copyUtilization>\n");
	if (get_copy_utilization(NULL, 10, &util_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {

		/*
		 * string obtained from the above call is:
		 * name=samfs1.1,type=li,capacity=0,free=0,usage=0
		 *
		 * Required format is:
		 * name="samfs1.1" type="li" capacity="0" free="0" usage="0"
		 */

		node = util_list->head;
		while (node != NULL) {
			if (parse_kv((char *)node->data,
			    &arcopy_tokens[0],
			    (void *)&acopy) == 0) {

				fprintf(ptr, "<copy name=\"%s\" type=\"%s\""
				    " capacity=\"%llu\" free=\"%llu\""
				    " usage=\"%d\"/>\n",
				    acopy.name,
				    acopy.mtype,
				    acopy.capacity,
				    acopy.free,
				    acopy.usage);
			}
			node = node->next;
		}
		lst_free_deep(util_list);
	}
	fprintf(ptr, "</copyUtilization>\n");

	return (0);
}

/* write tape vsn errors in XML format to file */
/* filter with the error flags provided */
static int
write_vsn_errors_in_xml(FILE *ptr, uint32_t flags) {

	sqm_lst_t *vsn_list = NULL;
	node_t *node = NULL;
	struct CatalogEntry *ce = NULL;

	fprintf(ptr, "<errorVsns>\n");
	if (get_all_catalog(&vsn_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {
		node = vsn_list->head;
		while (node != NULL) {
			ce = (struct CatalogEntry *)node->data;

			if (ce != NULL && ce->CeStatus & flags) {

				fprintf(ptr,
				    "<vsn name=\"%s\" type=\"%s\""
				    " status=\"%u\"/>\n",
				    ce->CeVsn,
				    ce->CeMtype,
				    /* don't over-report */
				    ce->CeStatus & flags);
			}

			node = node->next;
		}

		lst_free_deep(vsn_list);
	}
	fprintf(ptr, "</errorVsns>\n");

	return (0);
}

/* e.g. get vsns accessed in the last 24 hours */
static int
write_vsn_accessed_in_xml(
	FILE *ptr,	/* write to FILE */
	int32_t accessTime) /* accessed in the last x seconds */
{

	sqm_lst_t *vsn_list = NULL;
	node_t *node = NULL;
	struct CatalogEntry *ce = NULL;

	time_t now = time(NULL);

	fprintf(ptr, "<accessedVsns>\n");
	if (get_all_catalog(&vsn_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {
		node = vsn_list->head;
		while (node != NULL) {
			ce = (struct CatalogEntry *)node->data;

			if (ce != NULL &&
			    (now - ce->CeMountTime) < accessTime) {

				fprintf(ptr,
				    "<vsn name=\"%s\" type=\"%s\"/>\n",
				    ce->CeVsn,
				    ce->CeMtype);
			}

			node = node->next;
		}

		lst_free_deep(vsn_list);
	}
	fprintf(ptr, "</accessedVsns>\n");

	return (0);
}

/* get all blank vsns, output is written to file in XML format */
static int
write_vsn_blank_in_xml(
	FILE *ptr)	/* write to FILE */
{

	sqm_lst_t *vsn_list = NULL;
	node_t *node = NULL;
	struct CatalogEntry *ce = NULL;

	fprintf(ptr, "<blankVsns>\n");
	if (get_all_catalog(&vsn_list) != 0) {
		fprintf(ptr, "<error>\"%s\"</error>\n", samerrmsg);
	} else {
		node = vsn_list->head;
		while (node != NULL) {
			ce = (struct CatalogEntry *)node->data;
			/*
			 * TBD: Check if this logic reports all
			 * blank vsns
			 * Cannot reproduce now, but Umang found
			 * a case where in all blank vsns were not
			 * being reported
			 */
			if (ce != NULL && (ce->CeCapacity == ce->CeSpace)) {

				fprintf(ptr,
				    "<vsn name=\"%s\" type=\"%s\"/>\n",
				    ce->CeVsn,
				    ce->CeMtype);
			}

			node = node->next;
		}

		lst_free_deep(vsn_list);
	}
	fprintf(ptr, "</blankVsns>\n");

	return (0);
}

static int
write_acsls_driveinfo_in_xml(FILE *ptr, sqm_lst_t *drives) {

	node_t *n = NULL;
	acsls_drive_info_t *drive = NULL;

	if (ISNULL(ptr)) {
		return (-1);
	}

	fprintf(ptr, "<drives>\n");

	if (drives != NULL) {
		for (n = drives->head; n != NULL; n = n->next) {

			drive = (acsls_drive_info_t *)n->data;

			if (drive == NULL) {
				continue; /* ignore the error */
			}
			fprintf(ptr, "<drive acsId=\"%d\""
			    " lsmId=\"%d\" panelId=\"%d\""
			    " driveId=\"%d\" serial=\"%s\""
			    " type=\"%s\" state=\"%s\"/>\n",
			    drive->acs_num,
			    drive->lsm_num,
			    drive->panel_num,
			    drive->drive_num,
			    drive->serial_num,
			    drive->type,
			    drive->state);
		}
	}
	fprintf(ptr, "</drives>\n");

	return (0);
}

static int
write_acsls_scratchpool_in_xml(FILE *ptr, sqm_lst_t *pools) {

	node_t *n = NULL;
	stk_pool_t *pool = NULL;

	if (ISNULL(ptr)) {
		return (-1);
	}
	fprintf(ptr, "<scratchpools>\n");
	if (pools != NULL) {

		for (n = pools->head; n != NULL; n = n->next) {
			pool = (stk_pool_t *)n->data;

			if (pool == NULL) {
				continue;
			}
			fprintf(ptr,
			    "<pool id=\"%d\" lwm=\"%d\" hwm=\"%ld\""
			    " overflow=\"%s\"/>\n",
			    pool->pool_id,
			    pool->low_water_mark,
			    pool->high_water_mark,
			    pool->over_flow);
		}
	}
	fprintf(ptr, "</scratchpools>\n");
	return (0);
}

static void
free_acsls_report(
acsls_report_t *acsls_report_info_p)
{
	Trace(TR_ALLOC, "freeing acsls_report_t");
	if (acsls_report_info_p == NULL)
		return;
	if (acsls_report_info_p->drive_list != NULL)
		lst_free_deep(acsls_report_info_p->drive_list);
	if (acsls_report_info_p->accessedvolume_list != NULL)
		lst_free_deep(acsls_report_info_p->accessedvolume_list);
	if (acsls_report_info_p->enteredvolume_list != NULL)
		lst_free_deep(acsls_report_info_p->enteredvolume_list);
	if (acsls_report_info_p->lock_list != NULL)
		lst_free_deep(acsls_report_info_p->lock_list);
	if (acsls_report_info_p->pool_list != NULL)
		lst_free_deep(acsls_report_info_p->pool_list);
	free(acsls_report_info_p);
	Trace(TR_ALLOC, "finished freeing acsls_report_t");
}
