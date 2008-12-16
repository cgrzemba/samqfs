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
#pragma ident	"$Revision: 1.15 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/notify_summary.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_adm_NotifSummary.h"
#include "jni_util.h"

/* C structure <-> Java class conversion functions */

jobject
notfsum2NotifSummary(JNIEnv *env, void *v_notfsum) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	notf_summary_t *ns = (notf_summary_t *)v_notfsum;

	PTRACE(2, "jni:notfsum2NotifSummary() entry");
	cls = (*env)->FindClass(env, BASEPKG"/adm/NotifSummary");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;[Z)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(ns->admin_name),
	    JSTRING(ns->emailaddr),
	    arr2jboolArray(env, ns->notf_subj_arr, SUBJ_MAX));
	PTRACE(2, "jni:notfsum2NotifSummary() done");
	return (newObj);
}


void *
NotifSummary2notfsum(JNIEnv *env, jobject notifObj) {

	jclass cls;
	jobject boolArrFld;
	notf_summary_t *ns = (notf_summary_t *)malloc(sizeof (notf_summary_t));

	PTRACE(2, "jni:NotifSummary2notfsum() entry");
	cls = (*env)->GetObjectClass(env, notifObj);

	getStrFld(env, cls, notifObj, "adminName", ns->admin_name);
	getStrFld(env, cls, notifObj, "emailAddr", ns->emailaddr);
	boolArrFld = getObjFld(env, cls, notifObj, "subj", "[Z");
	jboolArray2arr(env, boolArrFld, ns->notf_subj_arr);

	PTRACE(2, "jni:NotifSummary2notfsum() done");
	return (ns);
}


/* native functions implementations */

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_NotifSummary_get(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *notiflst;

	PTRACE(1, "jni:NotifSummary_get() entry");
	if (-1 == get_notify_summary(CTX, &notiflst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    notiflst, BASEPKG"/adm/NotifSummary", notfsum2NotifSummary);
	lst_free_deep(notiflst);
	PTRACE(1, "jni:NotifSummary_get() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_NotifSummary_delete(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject notifSum) {

	notf_summary_t *ns;
	int res;

	PTRACE(1, "jni:NotifSummary_delete() entry");
	ns = NotifSummary2notfsum(env, notifSum);
	res = del_notify_summary(CTX, ns);
	free(ns);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:NotifSummary_delete() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_NotifSummary_modify(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring oldEmail, jobject notifSum) {

	jboolean isCopy;
	char *cstr;
	notf_summary_t *ns;
	int res;

	PTRACE(1, "jni:NotifSummary_modify() entry");
	cstr = GET_STR(oldEmail, isCopy);
	ns = NotifSummary2notfsum(env, notifSum);
	res = mod_notify_summary(CTX, cstr, ns);
	REL_STR(oldEmail, cstr, isCopy);
	free(ns);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:NotifSummary_modify() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_NotifSummary_add(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject notifSum) {

	notf_summary_t *ns;
	int res;

	PTRACE(1, "jni:NotifSummary_add() entry");
	ns = NotifSummary2notfsum(env, notifSum);
	res = add_notify_summary(CTX, ns);
	free(ns);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:NotifSummary_add() done");
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_NotifSummary_getEmailAddrsForSubject
(JNIEnv *env, jclass cls, /*ARGSUSED*/ jobject ctx, jint notifSubj) {

	PTRACE(1, "jni:NotifSummary_getEmailAddrsForSubject() entry");

	char *retVal = NULL;

	if (-1 == get_email_addrs_by_subj(CTX, (int)notifSubj, &retVal)) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:NotifSummary_getEmailAddrsForSubject() done");
	return (JSTRING(retVal));

}
