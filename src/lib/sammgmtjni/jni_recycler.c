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
#pragma ident	"$Revision: 1.17 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/recycle.h"
#include "pub/mgmt/recyc_sh_wrap.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_rec_Recycler.h"
#include "jni_util.h"

/* C structure <-> Java class conversion functions */

jobject
recparams2RecyclerParams(JNIEnv *env, void *v_recprm) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	rc_param_t *rp = (rc_param_t *)v_recprm;

	PTRACE(2, "jni:recparams2RecyclerParams() entry");
	cls = (*env)->FindClass(env, BASEPKG"/rec/RecyclerParams");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ILjava/lang/String;ZLjava/lang/String;IIIJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)rp->hwm,
	    ull2jstr(env, rp->data_quantity),
	    JBOOL(rp->ignore),
	    JSTRING(rp->email_addr),
	    (jint)rp->mingain,
	    (jint)rp->vsncount,
	    (jint)rp->minobs,
	    (jlong)(rp->change_flag));
	PTRACE(2, "jni:recparams2RecyclerParams() done");
	return (newObj);
}


void *
RecyclerParams2recparams(JNIEnv *env, jobject rpObj) {

	jclass cls;
	rc_param_t *rp = NULL;

	PTRACE(2, "jni:RecyclerParams2recparams() entry");

	if (rpObj == NULL) {
		PTRACE(2, "jni:RecyclerParams2recparams() done (null)");
		return (NULL);
	}

	rp = (rc_param_t *)malloc(sizeof (rc_param_t));
	memset(rp, 0, sizeof (rc_param_t));

	cls = (*env)->GetObjectClass(env, rpObj);
	rp->hwm = (int)getJIntFld(env, cls, rpObj, "hwm");
	rp->data_quantity = (fsize_t)jstrFld2ull(env, cls, rpObj, "datasize");
	rp->ignore = getBoolFld(env, cls, rpObj, "ignore");
	getStrFld(env, cls, rpObj, "email", rp->email_addr);
	rp->mingain = (int)getJIntFld(env, cls, rpObj, "minGain");
	rp->vsncount = (int)getJIntFld(env, cls, rpObj, "vsnCount");
	rp->change_flag = (uint32_t)getJLongFld(env, cls, rpObj, "chgFlags");
	rp->mail = B_FALSE; /* possible pre 4.0 support, currently no support */
	/* minobs added in 1.3.4 */
	rp->minobs = (int)getJIntFld(env, cls, rpObj, "minObs");
	PTRACE(2, "jni:RecyclerParams2recparams() done");
	return (rp);
}


jobject
librecparams2LibRecParams(JNIEnv *env, void *v_librecprm) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	rc_robot_cfg_t *rr = (rc_robot_cfg_t *)v_librecprm;

	PTRACE(2, "jni:librecparams2LibRecParams() entry");
	cls = (*env)->FindClass(env, BASEPKG"/rec/LibRecParams");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ILjava/lang/String;ZLjava/lang/String;IIIJLjava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)rr->rc_params.hwm,
	    ull2jstr(env, rr->rc_params.data_quantity),
	    JBOOL(rr->rc_params.ignore),
	    JSTRING(rr->rc_params.email_addr),
	    (jint)rr->rc_params.mingain,
	    (jint)rr->rc_params.vsncount,
	    (jint)rr->rc_params.minobs,
	    (jlong)(rr->rc_params.change_flag),
	    JSTRING(rr->robot_name));
	PTRACE(2, "jni:librecparams2libRecParams() done");
	return (newObj);
}


void *
LibRecParams2recparams(JNIEnv *env, jobject lrpObj) {

	jclass cls;
	rc_robot_cfg_t *rr = (rc_robot_cfg_t *)malloc(sizeof (rc_robot_cfg_t));

	PTRACE(2, "jni:LibRecParams2librecparams() entry");
	cls = (*env)->FindClass(env, BASEPKG"/rec/LibRecParams");

	getStrFld(env, cls, lrpObj, "path", rr->robot_name);
	rr->rc_params.hwm = (int)getJIntFld(env, cls, lrpObj, "hwm");
	rr->rc_params.data_quantity =
	    jstrFld2ull(env, cls, lrpObj, "datasize");
	rr->rc_params.ignore = getBoolFld(env, cls, lrpObj, "ignore");
	getStrFld(env, cls, lrpObj, "email", rr->rc_params.email_addr);
	rr->rc_params.mingain = (int)getJIntFld(env, cls, lrpObj, "minGain");
	rr->rc_params.vsncount = (int)getJIntFld(env, cls, lrpObj, "vsnCount");
	rr->rc_params.mail = B_FALSE; /* currently not supported */
	/* minobs added in 1.3.4 */
	rr->rc_params.minobs = (int)getJIntFld(env, cls, lrpObj, "minObs");
	rr->rc_params.change_flag = (uint32_t)
	    getJLongFld(env, cls, lrpObj, "chgFlags");
	PTRACE(2, "jni:LibRecParams2librecparams() done");
	return (rr);
}


/* native functions implementation */


/* Part1/2: recycler.cmd related functions */

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getDefaultParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	rc_param_t *rcparam;

	PTRACE(1, "jni:Recycler_getDefaultParams() entry");
	if (-1 == get_default_rc_params(CTX, &rcparam)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = recparams2RecyclerParams(env, rcparam);
	free(rcparam);
	PTRACE(1, "jni:Recycler_getDefaultParams() done");
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getAllLibRecParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *recparamslst;

	PTRACE(1, "jni:Recycler_getAllLibRecParams() entry");
	if (-1 == get_all_rc_robot_cfg(CTX, &recparamslst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, recparamslst,
	    BASEPKG"/rec/LibRecParams", librecparams2LibRecParams);
	lst_free_deep(recparamslst);
	PTRACE(1, "jni:Recycler_getAllLibRecParams() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getLibRecParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring libName) {

	jboolean isCopy;
	jobject newObj;
	int res;
	char *cstr = GET_STR(libName, isCopy);
	rc_robot_cfg_t *rc;

	PTRACE(1, "jni:Recycler_getLibRecParams() entry");
	res = get_rc_robot_cfg(CTX, cstr, &rc);
	REL_STR(libName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = librecparams2LibRecParams(env, rc);
	free(rc);
	PTRACE(1, "jni:Recycler_getLibRecParams() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_setLibRecParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject libRecParams) {

	rc_robot_cfg_t *rc;
	int res;

	PTRACE(1, "jni:Recycler_setLibRecParams() entry");
	rc = LibRecParams2recparams(env, libRecParams);
	res = set_rc_robot_cfg(CTX, rc);
	free(rc);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Recycler_setLibRecParams() done");
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getLogPath(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jstring logPath;
	upath_t rc_log;

	PTRACE(1, "jni:Recycler_getLogPath() entry");
	if (-1 == get_rc_log(CTX, rc_log)) {
		ThrowEx(env);
		return (NULL);
	}
	logPath = JSTRING(rc_log);
	PTRACE(1, "jni:Recycler_getLogPath() done");
	return (logPath);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_setLogPath(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring logPath) {

	jboolean isCopy;
	int res;
	char *cstr;

	PTRACE(1, "jni:Recycler_setLogPath() entry");
	cstr = GET_STR(logPath, isCopy);
	res = set_rc_log(CTX, cstr);
	REL_STR(logPath, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Recycler_setLogPath() done");
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getDefaultLogPath(JNIEnv *env,
    jobject ctx, jclass cls /*ARGSUSED*/) {

	jstring logPath;
	upath_t rc_log;

	PTRACE(1, "jni:Recycler_getDefaultLogPath() entry");
	if (-1 == get_default_rc_log(CTX, rc_log)) {
		ThrowEx(env);
		return (NULL);
	}
	logPath = JSTRING(rc_log);
	PTRACE(1, "jni:Recycler_getDefaultLogPath() done");
	return (logPath);
}


/* Part2/2: recycle.sh related functions */

JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_getActions(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	int res;

	PTRACE(1, "jni:Recycler_getAction() entry");
	if (-1 == (res = get_recycl_sh_action_status(CTX))) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:Recycler_getAction() done");
	return (res);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_addActionLabel(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Recycler_addActionLabel() entry");
	if (-1 == add_recycle_sh_action_label(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Recycler_addActionLabel() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_addActionExport(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring emailAddr) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:Recycler_addActionExport() entry");
	cstr = GET_STR(emailAddr, isCopy);
	res = add_recycle_sh_action_export(CTX, cstr);
	REL_STR(emailAddr, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Recycler_addActionExport() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rec_Recycler_delAction(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Recycler_delAction() entry");
	if (-1 == del_recycle_sh_action(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Recycler_delAction() done");
}
