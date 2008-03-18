/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident	"$Revision: 1.14 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/faults.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_adm_Fault.h"
#include "jni_util.h"

/* C structure <-> Java class conversion functions */

jobject
faultattr2FaultAttr(JNIEnv *env, void *v_fltattr) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	fault_attr_t *fa = (fault_attr_t *)v_fltattr;

	PTRACE(2, "jni:faultattr2FaultAttr() entry");
	cls = (*env)->FindClass(env, BASEPKG"/adm/FaultAttr");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(JLjava/lang/String;BJLjava/lang/String;Ljava/lang/String;B"
	    "Ljava/lang/String;I)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jlong)fa->errorID,
	    JSTRING(fa->compID),
	    (jbyte)fa->errorType,
	    (jlong)fa->timestamp,
	    JSTRING(fa->hostname),
	    JSTRING(fa->msg),
	    (jbyte)fa->state,
	    JSTRING(fa->library),
	    (jint)fa->eq);
	PTRACE(2, "jni:faultattr2FaultAttr() done");
	return (newObj);
}


void *
FaultAttr2faultattr(JNIEnv *env, jobject faObj) {

	jclass cls;
	fault_attr_t *fa = (fault_attr_t *)malloc(sizeof (fault_attr_t));

	PTRACE(2, "jni:FaultAttr2faultattr() entry");
	cls = (*env)->FindClass(env, BASEPKG"/adm/FaultAttr");
	fa->errorID = (long)getJLongFld(env, cls, faObj, "errID");
	getStrFld(env, cls, faObj, "compID", fa->compID);
	fa->errorType = (fault_sev_t)getJByteFld(env, cls, faObj, "severity");
	fa->timestamp = (time_t)getJLongFld(env, cls, faObj, "timestamp");
	getStrFld(env, cls, faObj, "hostname", fa->hostname);
	getStrFld(env, cls, faObj, "message", fa->msg);
	fa->state = (fault_state_t)getJByteFld(env, cls, faObj, "state");
	PTRACE(2, "jni:FaultAttr2faultattr() done");
	return (fa);
}


jobject
faultsum2FaultSummary(JNIEnv *env, void *v_fltsum) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	fault_summary_t *flts = (fault_summary_t *)v_fltsum;

	if (NULL == flts)
		return (NULL);
	cls = (*env)->FindClass(env, BASEPKG"/adm/FaultSummary");
	mid = (*env)->GetMethodID(env, cls, "<init>", "(III)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)flts->num_critical_faults,
	    (jint)flts->num_major_faults,
	    (jint)flts->num_minor_faults);
	PTRACE(2, "jni:faultsum(%d,%d,%d)", flts->num_critical_faults,
	    flts->num_major_faults, flts->num_minor_faults);
	return (newObj);
}


/* native functions implementation */

// Client always sends -1 -1 -1 -1 as input params
// Fault_get: deprecated in API_VERISON !.5.0, use Fault_getAll instead

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_get(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint numFaults,
    jbyte sev, jbyte state, jlong errID) {

	jobjectArray newArr;
	sqm_lst_t *fltlst;
	flt_req_t fltReq;

	fltReq.numFaults = (int)numFaults;
	fltReq.sev = (fault_sev_t)sev;
	fltReq.state = (fault_state_t)state;
	fltReq.errorID = (long)errID;
	PTRACE(1, "jni:Fault_get(%d,%d,%d,%d)",
	    fltReq.numFaults, fltReq.sev, fltReq.state, fltReq.errorID);
	if (-1 == get_faults(CTX, fltReq, &fltlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    fltlst, BASEPKG"/adm/FaultAttr", faultattr2FaultAttr);
	lst_free_deep(fltlst);
	PTRACE(1, "jni:Fault_get() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *fltlst;

	PTRACE(1, "jni:Fault_getAll");
	if (-1 == get_all_faults(CTX, &fltlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    fltlst, BASEPKG"/adm/FaultAttr", faultattr2FaultAttr);
	lst_free_deep(fltlst);
	PTRACE(1, "jni:Fault_get() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_getByLibName(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring libName) {

	jobjectArray newArr;
	sqm_lst_t *fltlst;
	jboolean isCopy;
	char *cstr = GET_STR(libName, isCopy);
	int res;

	PTRACE(1, "jni:Fault_getByLibName(%s)", Str(cstr));
	res = get_faults_by_lib(CTX, cstr, &fltlst);
	REL_STR(libName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    fltlst, BASEPKG"/adm/FaultAttr", faultattr2FaultAttr);
	lst_free_deep(fltlst);
	PTRACE(1, "jni:Fault_getByLibName() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_getByLibEq(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint libEq) {
	jobjectArray newArr;
	sqm_lst_t *fltlst;

	PTRACE(1, "jni:Fault_getByLibEq(%d)", (equ_t)libEq);
	if (-1 == get_faults_by_eq(CTX, (equ_t)libEq, &fltlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    fltlst, BASEPKG"/adm/FaultAttr", faultattr2FaultAttr);
	lst_free_deep(fltlst);
	PTRACE(1, "jni:Fault_getByLibEq() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_ack(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jlongArray errIDs) {

	long *err_ids;
	int len, res;

	PTRACE(1, "jni:Fault_ack() entry");
	len = jlongArray2newarr(env, errIDs, &err_ids);
	res = ack_faults(CTX, len, err_ids);
	free(err_ids);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Fault_ack([%d]) done", len);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_delete(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jlongArray errIDs) {

	long *err_ids;
	int len, res;

	PTRACE(1, "jni:Fault_delete() entry");
	len = jlongArray2newarr(env, errIDs, &err_ids);
	res = delete_faults(CTX, len, err_ids);
	free(err_ids);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Fault_delete([%d]) done", len);
}


JNIEXPORT jboolean JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_isOn(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	boolean_t status;

	PTRACE(1, "jni:Fault_isOn() entry");
	if (-1 == is_faults_gen_status_on(CTX, &status)) {
		ThrowEx(env);
		return (JNI_FALSE);
	}
	PTRACE(1, "jni:Fault_isOn() done");
	return (JBOOL(status));
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_Fault_getSummary(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	fault_summary_t flts;

	PTRACE(1, "jni:Fault_getSummary");
	if (-1 == get_fault_summary(CTX, &flts)) {
		ThrowEx(env);
		return (JNI_FALSE);
	}
	newObj = faultsum2FaultSummary(env, &flts);
	return (newObj);
}
