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
#pragma ident	"$Revision: 1.37 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>

#include "libgen.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/license.h"
#include "mgmt/log.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/report.h"
#include "pub/mgmt/monitor.h"

#include "com_sun_netstorage_samqfs_mgmt_adm_License.h"
#include "com_sun_netstorage_samqfs_mgmt_Job.h"
#include "com_sun_netstorage_samqfs_mgmt_Util.h"
#include "com_sun_netstorage_samqfs_mgmt_media_Media.h"
#include "jni_util.h"

extern int crtprio; // crt trace priority
extern jobject mdlic2MdLicense(JNIEnv *, void *);

/* C Structure -> Java class conversion function */

jobject
license2License(JNIEnv *env, void *v_license_info) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	license_info_t *li = (license_info_t *)v_license_info;

	PTRACE(2, "jni:license2License() entry");
	cls = (*env)->FindClass(env, BASEPKG"/adm/License");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(SJSJI[L"BASEPKG"/media/MdLicense;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jshort)li->type,
	    (jlong)li->expire,
	    (jshort)li->fsType,
	    (jlong)li->hostid,
	    (jint)li->feature_flags,
	    (lst2jarray(env, li->media_list,
		BASEPKG"/media/MdLicense", mdlic2MdLicense)));

	PTRACE(2, "jni:license2License() done");
	return (newObj);
}


/*
 * The individual methods to get license type and expiration are removed.
 * All the properties of license are obtainable from getLicenseInfo()
 *
 * the getFSType is retained for backward compatibility
 */

JNIEXPORT jshort JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_License_getFSType(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	int res;

	PTRACE(1, "jni:License_getFSType");
	if (-1 == (res = get_samfs_type(CTX))) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:license information obtained");
	return ((jshort)res);

}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_License_getLicense(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	license_info_t *info;

	PTRACE(1, "jni:License_getInfo");
	if (-1 == get_license_info(CTX, &info)) {
		ThrowEx(env);
		return (NULL);
	}

	newObj = license2License(env, info);
	free_license_info(info);

	PTRACE(1, "jni:license information obtained");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Job_terminateProcess(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jlong pid, jshort ptype) {

	PTRACE(1, "jni:terminateProcess(%ld,%d)",
	    (pid_t)pid, (int)ptype);
	if (-1 == destroy_process(CTX, (pid_t)pid, (int)ptype)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:process termination cmd issued succesfully");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Util_writeToSyslog(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jstring msg) {

	jboolean isCopy;
	char *cstr;

	if (msg == NULL)
		return;
	cstr = GET_STR(msg, isCopy);
	TRACE(Str(cstr));
	REL_STR(msg, cstr, isCopy);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Util_setNativeTraceLevel(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jint prio) {

	PTRACE(3, "jni:Util_setNativeTraceLevel(%d)", prio);
	if (crtprio != (int)prio) {
	    TRACE("jni:trace level changed to %d", prio);
	}
	crtprio = (int)prio;
}


JNIEXPORT jboolean JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Util_isValidRegExp(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jstring regExp) {

	jboolean isCopy, ret;
	char *name = NULL;
	char *regexp = GET_STR(regExp, isCopy);

	name = regcmp(regexp, (char *)0);
	ret = (NULL == name) ? JNI_FALSE : JNI_TRUE;
	REL_STR(regExp, regexp, isCopy);
	free(name);
	return (ret);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getCapacity(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	char *str = NULL;

	PTRACE(3, "jni:SysInfo getCapacity() entry");
	if (-1 == get_server_capacities(CTX, &str)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(3, "jni:SysInfo getCapacity() done");
	return (JSTRING(str));
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getOSInfo(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	char *str = NULL;

	PTRACE(3, "jni:SysInfo getOSInfo() entry");
	if (-1 == get_system_info(CTX, &str)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(3, "jni:SysInfo getOSInfo() done");
	return (JSTRING(str));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getLogInfo(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:SysInfo getLogInfo() entry");
	if (-1 == get_logntrace(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getLogInfo() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Job_getAllActivities(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, int maxEntries, jstring filter) {

	sqm_lst_t  *lst = NULL;
	char *cstr = NULL;
	jboolean isCopy;
	jobjectArray newArr;

	PTRACE(3, "jni:Job getAllActivites() entry");

	cstr = GET_STR(filter, isCopy);
	if (-1 == list_activities(CTX, (int)maxEntries, cstr, &lst)) {
		REL_STR(filter, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	REL_STR(filter, cstr, isCopy);

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:Job getAllActivities() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_Job_cancelActivity(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring id, jstring type) {

	char *cstr = NULL, *tstr = NULL;
	jboolean isCopy, isCopy2;

	cstr = GET_STR(id, isCopy);
	tstr = GET_STR(type, isCopy2);

	PTRACE(3, "jni:Job cancelActivity() entry");
	if (-1 == kill_activity(CTX, cstr, tstr)) {
		REL_STR(id, cstr, isCopy);
		REL_STR(type, tstr, isCopy2);
		ThrowEx(env);
		return;
	}
	REL_STR(id, cstr, isCopy);
	REL_STR(type, tstr, isCopy2);
	PTRACE(3, "jni:Job cancel activity() done");
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getPackageInfo(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring pkgname) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;
	char *cstr = NULL;
	jboolean isCopy;

	cstr = GET_STR(pkgname, isCopy);

	PTRACE(3, "jni:Sysinfo getPackageInfo() entry");
	if (-1 == get_package_info(CTX, cstr, &lst)) {
		REL_STR(pkgname, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	REL_STR(pkgname, cstr, isCopy);

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getPackageInfo() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getConfigStatus(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:Sysinfo getConfigStatus() entry");
	if (-1 == get_configuration_status(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getConfigStatus() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_listSamExplorerOutputs(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:Sysinfo listExplorerOutputs() entry");
	if (-1 == list_explorer_outputs(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo listExplorerOutputs() done");
	return (newArr);
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_runSamExplorer(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring location, jint loglines) {

	jboolean isCopy;
	char *cstr = NULL;
	int ret;
	char idbuf[11]; /* long enough for INT_MAX + '\0' */

	cstr = GET_STR(location, isCopy);

	PTRACE(1, "jni:runSamExplorer(%s,%d)", Str(cstr), loglines);
	ret == run_sam_explorer(CTX, cstr, loglines);

	PTRACE(1, "jni:runSamExplorer(%s,%d) returned %d",
	    Str(cstr), loglines, ret);

	REL_STR(location, cstr, isCopy);
	if (ret == 0 || ret == -2) {
		return (NULL);
	} else if (ret == -1) {
		ThrowEx(env);
		return (NULL);
	} else {
		/*
		 * function went asynch and a job id was
		 * returned. Convert the id to a string and return
		 * it.
		 */
		snprintf(idbuf, sizeof (idbuf), "%d", ret);
		return (JSTRING(idbuf));
	}
}


// SC related methods. since 4.5

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getSCVersion(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	char *str = NULL;

	PTRACE(3, "jni:SysInfo getSCVersion() entry");
	if (-1 == get_sc_version(CTX, &str)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(3, "jni:SysInfo getSCVersion() done");
	return (JSTRING(str));
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getSCName(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	char *str = NULL;

	PTRACE(3, "jni:SysInfo getSCName() entry");
	if (-1 == get_sc_name(CTX, &str)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(3, "jni:SysInfo getSCName() done");
	return (JSTRING(str));
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getSCNodes(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:SysInfo getSCNodes() entry");
	if (-1 == get_sc_nodes(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getSCNodes() done");
	return (newArr);
}

JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getSCUIState(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	int res;
	PTRACE(3, "jni:SysInfo getSCUIState() entry");
	if (-1 == (res = get_sc_ui_state(CTX))) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(3, "jni:SysInfo getSCUIState() done");
	return ((jint)res);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Report_generate(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint type, jint includeFlags,
    jobjectArray emails, jobject stkconn) {

	PTRACE(1, "jni:Report generate(%d,%d)", type, includeFlags);
	report_requirement_t *req;
	req = (report_requirement_t *)malloc(sizeof (report_requirement_t));
	memset(req, 0, sizeof (report_requirement_t));
	req->section_flag = includeFlags;
	req->report_type = type;

	if (-1 == gen_report(CTX, req)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Report generate() done");
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getProcessStatus(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:SysInfo getProcessStatus() entry");
	if (-1 == get_status_processes(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getProcessStatus() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getComponentStatusSummary(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:SysInfo getComponentStatusSummary() entry");
	if (-1 == get_component_status_summary(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:SysInfo getComponentStatusSummary() done");
	return (newArr);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_SysInfo_getConfigurationSummary(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	char *str = NULL;

	PTRACE(3, "jni:SysInfo getConfigurationSummary() entry");
	if (-1 == get_config_summary(CTX, &str)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(3, "jni:SysInfo getConfigurationSummary() done");
	return (JSTRING(str));
}
