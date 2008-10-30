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
#pragma ident	"$Revision: 1.23 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/directives.h"
#include "pub/mgmt/stage.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_stg_Stager.h"
#include "com_sun_netstorage_samqfs_mgmt_stg_job_StagerJob.h"
#include "com_sun_netstorage_samqfs_mgmt_stg_job_StagerStream.h"
#include "jni_util.h"


/* C structure <-> Java class conversion functions */

extern jobject bufdir2BufDirective(JNIEnv *, void *);
extern void * BufDirective2bufdir(JNIEnv *, jobject);
extern jobject drvdir2DrvDirective(JNIEnv *, void *);
extern void * DrvDirective2drvdir(JNIEnv *, jobject);

jobject
stgcfg2StagerParams(JNIEnv *env, void *v_stgcfg) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stager_cfg_t *stg = (stager_cfg_t *)v_stgcfg;
	int	dkDrives = 0;
	fsize_t dkMaxSize = 0;
	int	dkMaxCount = 0;
	uint32_t dkChgFlags = 0;

	if (stg->dk_stream != NULL) {
		dkDrives = stg->dk_stream->drives;
		dkMaxSize = stg->dk_stream->max_size;
		dkMaxCount = stg->dk_stream->max_count;
		dkChgFlags = stg->dk_stream->change_flag;
	}


	PTRACE(2, "jni:stgcfg2StagerParams() entry");
	cls = (*env)->FindClass(env, BASEPKG"/stg/StagerParams");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;II[L"BASEPKG"/arc/BufDirective;"
	    "[L"BASEPKG"/arc/DrvDirective;JJIJIJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(stg->stage_log),
	    (jint)stg->max_active,
	    (jint)stg->max_retries,
	    lst2jarray(env, stg->stage_buf_list,
		BASEPKG"/arc/BufDirective", bufdir2BufDirective),
	    lst2jarray(env, stg->stage_drive_list,
		BASEPKG"/arc/DrvDirective", drvdir2DrvDirective),
	    (jlong)stg->options,
	    (jlong)stg->change_flag,
	    (jint)dkDrives,
	    (jlong)dkMaxSize,
	    (jint)dkMaxCount,
	    (jlong)dkChgFlags);

	PTRACE(2, "jni:stgcfg2StagerParams() done");
	return (newObj);
}

void *
StagerParams2stgcfg(JNIEnv *env, jobject stgObj) {

	jclass cls;
	stager_cfg_t *stg;
	stream_cfg_t *stream;

	stg = (stager_cfg_t *)malloc(sizeof (stager_cfg_t));
	if (stg == NULL) {
		return (NULL);
	}
	stream = (stream_cfg_t *)malloc(sizeof (stream_cfg_t));
	if (stream == NULL) {
		free(stg);
		return (NULL);
	}
	memset(stg, 0, sizeof (stager_cfg_t));
	memset(stream, 0, sizeof (stream_cfg_t));

	stg->dk_stream = stream;
	strlcpy(stg->dk_stream->media, "dk", 5);

	PTRACE(2, "jni:StagerParams2stgcfg() entry");
	cls = (*env)->GetObjectClass(env, stgObj);

	getStrFld(env, cls, stgObj, "logPath", stg->stage_log);
	stg->max_active = (int)getJIntFld(env, cls, stgObj, "maxActive");
	stg->max_retries = (int)getJIntFld(env, cls, stgObj, "maxRetries");
	stg->stage_buf_list = jarray2lst(env,
	    getJArrFld(env, cls,
		stgObj, "bufDirs", "[L"BASEPKG"/arc/BufDirective;"),
	    BASEPKG"/arc/BufDirective",
	    BufDirective2bufdir);
	stg->stage_drive_list = jarray2lst(env,
	    getJArrFld(env, cls,
		stgObj, "drvDirs", "[L"BASEPKG"/arc/DrvDirective;"),
	    BASEPKG"/arc/DrvDirective",
	    DrvDirective2drvdir);
	stg->change_flag =
	    (uint32_t)getJLongFld(env, cls, stgObj, "chgFlags");
	stg->options =
	    (uint32_t)getJLongFld(env, cls, stgObj, "options");
	stg->dk_stream->drives = (int)getJIntFld(env, cls,
	    stgObj, "dkDrives");
	stg->dk_stream->max_size = (fsize_t)getJLongFld(env, cls,
		stgObj, "dkMaxSize") * 1024;
	stg->dk_stream->max_count = (int)getJIntFld(env, cls,
	    stgObj, "dkMaxCount");
	stg->dk_stream->change_flag =
	    (uint32_t)getJLongFld(env, cls, stgObj, "dkChgFlags");

	PTRACE(2, "jni:StagerParams2stgcfg() done");
	return (stg);
}


jobject
stgfileinfo2StgFileInfo(JNIEnv *env, void *v_stgfileinfo) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	staging_file_info_t *stgfileinfo =
	    (staging_file_info_t *)v_stgfileinfo;

	PTRACE(2, "jni:stgfileinfo2StgFileInfo() entry");
	cls = (*env)->FindClass(env, BASEPKG"/stg/job/StgFileInfo");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(stgfileinfo->filename),
	    ull2jstr(env, stgfileinfo->len),
	    ull2jstr(env, stgfileinfo->position),
	    ull2jstr(env, stgfileinfo->offset),
	    JSTRING(stgfileinfo->vsn),
	    JSTRING(stgfileinfo->user));
	PTRACE(2, "jni:stgfileinfo2StgFileInfo() done");
	return (newObj);
}


jobject
stgstream2StagerStream(JNIEnv *env, void *v_stgstream) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stager_stream_t *stgstream = (stager_stream_t *)v_stgstream;
	staging_file_info_t crtfile;

	PTRACE(2, "jni:stgstream2StagerStream() entry");
	cls = (*env)->FindClass(env, BASEPKG"/stg/job/StagerStream");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ZLjava/lang/String;ILjava/lang/String;JJ"
	    "L"BASEPKG"/stg/job/StgFileInfo;I)V");

	// stgstream->current_file has type StagerStageDetail_t (!)
	if (NULL != stgstream->current_file) {
		strcpy(crtfile.filename, stgstream->current_file->name);
		crtfile.len = stgstream->current_file->len;
		crtfile.position = stgstream->current_file->position;
		crtfile.offset = stgstream->current_file->offset;
		strcpy(crtfile.vsn, stgstream->vsn);
		crtfile.user[0] = '\0'; // cant get this info from existing API

	}

	newObj = (*env)->NewObject(env, cls, mid,
	    JBOOL(stgstream->active),
	    JSTRING(stgstream->media),
	    (jint)stgstream->seqnum,
	    JSTRING(stgstream->vsn),
	    (jlong)stgstream->count,
	    (jlong)stgstream->create,
	    ((NULL == stgstream->current_file) ? (jobject)NULL :
		stgfileinfo2StgFileInfo(env, &crtfile)),
	    /* added in 4.6, state_flag to give state of stream if active */
	    /* 0 indicates that it the value is not available */
	    (jint)stgstream->state_flags);
	PTRACE(2, "jni:stgstream2StagerStream() done");
	return (newObj);
}


void *
StagerStream2stgstream(JNIEnv *env, jobject stgStream) {

	jclass cls;
	stager_stream_t *stream =
	    (stager_stream_t *)malloc(sizeof (stager_stream_t));

	PTRACE(2, "jni:StagerStream2stgstream() entry");
	cls = (*env)->GetObjectClass(env, stgStream);

	stream->active = getBoolFld(env, cls, stgStream, "active");
	getStrFld(env, cls, stgStream, "mediaType", stream->media);
	stream->seqnum = (int)getJIntFld(env, cls, stgStream, "seqNum");
	getStrFld(env, cls, stgStream, "vsn", stream->vsn);
	stream->count = (size_t)getJLongFld(env, cls, stgStream, "count");
	stream->create = (time_t)getJLongFld(env,
	    cls, stgStream, "creationTime");
	stream->age = 0;
	stream->msg[0] = '\0';
	stream->current_file = NULL;

	PTRACE(2, "jni:StagerStream2stgstream() done");
	return (stream);
}


/* native functions implementation */


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_getParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	stager_cfg_t *stg;

	PTRACE(1, "jni:Stager_getParams() entry");
	if (-1 == get_stager_cfg(CTX, &stg)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = stgcfg2StagerParams(env, stg);
	free_stager_cfg(stg);
	PTRACE(1, "jni:Stager_getParams() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_getDefaultParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	stager_cfg_t *stg;

	PTRACE(1, "jni:Stager_getDefaultParams() entry");
	if (-1 == get_default_stager_cfg(CTX, &stg)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = stgcfg2StagerParams(env, stg);
	free_stager_cfg(stg);
	PTRACE(1, "jni:Stager_getDefaultParams() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_setParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject stgObj) {

	stager_cfg_t *stg;
	int res;

	PTRACE(1, "jni:Stager_setParams() entry");
	stg = StagerParams2stgcfg(env, stgObj);
	if (stg == NULL) {
		ThrowEx(env);
		return;
	}
	res = set_stager_cfg(CTX, stg);
	free_stager_cfg(stg);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stager_setParams() done");

}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_resetBufDirective(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject bufdirObj) {

	buffer_directive_t *bufdir;
	int res;

	PTRACE(1, "jni:Stager_resetBufDirective() entry");
	bufdir = BufDirective2bufdir(env, bufdirObj);
	res = reset_buffer_directive(CTX, bufdir);
	free(bufdir);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stager_resetBufDirective() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_resetDrvDirective(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject drvdirObj) {

	drive_directive_t *drvdir;
	int res;

	PTRACE(1, "jni:Stager_resetDrvDirective() entry");
	drvdir = DrvDirective2drvdir(env, drvdirObj);
	res = reset_drive_directive(CTX, drvdir);
	free(drvdir);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stager_resetDrvDirective() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_idle(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Stager_idle() entry");
	if (-1 == st_idle(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stager_idle() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_run(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Stager_run() entry");
	if (-1 == st_run(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stager_run() done");
}


// --------------- Job-related functions

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_job_StagerStream_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newArr;
	stager_info_t *stginfo;

	PTRACE(1, "jni:StagerStream_getAll() entry");
	if (-1 == get_stager_info(CTX, &stginfo)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, stginfo->stager_streams,
	    BASEPKG"/stg/job/StagerStream", stgstream2StagerStream);
	free_stager_info(stginfo);
	PTRACE(1, "jni:StagerStream_getAll() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_job_StagerJob_getFiles(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx,
    jobject stgStream, jint start, jint size, jshort sortMet, jboolean asc) {

	jobjectArray newArr;
	sqm_lst_t *fileinfos;
	stager_stream_t *ss;
	int res;

	PTRACE(1, "jni:StagerJob_getFiles() entry");

	ss = (stager_stream_t *)StagerStream2stgstream(env, stgStream);
	res = get_staging_files_in_stream(CTX, ss,
	    (int)start, (int)size, (st_sort_key_t)sortMet, CBOOL(asc),
	    &fileinfos);
	free_stager_stream(ss);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, fileinfos,
	    BASEPKG"/stg/job/StgFileInfo", stgfileinfo2StgFileInfo);
	lst_free_deep(fileinfos);
	PTRACE(1, "jni:Stagerjob_getFiles() done");
	return (newArr);
}


JNIEXPORT jlong JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_job_StagerJob_getNumOfFiles(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jobject stgStream) {

	stager_stream_t *ss;
	size_t num;
	int res;

	PTRACE(1, "jni:StagerJob_getNumOfFiles() entry");
	ss = (stager_stream_t *)StagerStream2stgstream(env, stgStream);
	res = get_numof_staging_files_in_stream(CTX, ss, &num);
	free_stager_stream(ss);
	if (-1 == res) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:StagerJob_getNumOfFiles() done");
	return ((jlong)num);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_job_StagerJob_cancelStgForFiles(
    JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx,
    jobjectArray fileNames, jboolean recursive) {

	// not needed by GUI
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_job_StagerJob_clearRequest(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring mType, jstring vsn) {

	jboolean isCopy, isCopy2;
	char *mtypestr, *vsnstr;
	int res;

	PTRACE(1, "jni:StagerJob_clearRequest() entry");
	mtypestr = GET_STR(mType, isCopy);
	vsnstr = GET_STR(vsn, isCopy2);
	res = clear_stage_request(CTX, mtypestr, vsnstr);
	REL_STR(mType, mtypestr, isCopy);
	REL_STR(vsn, vsnstr, isCopy2);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Stagerjob_clearRequest() done");
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_stg_Stager_stageFiles(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx,
	jobjectArray filepaths, jint options) {

	int ret;
	char *job_id;

	PTRACE(1, "jni:Stager_stageFiles() entry");
	if (-1 == stage_files(CTX, jarray2lst(env, filepaths,
		"java/lang/String", String2charr), (int32_t)options,
		&job_id)) {

		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:Stager_stageFiles() exit");
	return (JSTRING(job_id));
}
