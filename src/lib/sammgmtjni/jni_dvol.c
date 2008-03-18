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
#pragma ident	"$Revision: 1.13 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/diskvols.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_arc_DiskVol.h"
#include "jni_util.h"

jobject
diskvol2DiskVol(JNIEnv *env, void *v_dskvol) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	disk_vol_t *dvol = (disk_vol_t *)v_dskvol;

	PTRACE(2, "jni:diskvol2DiskVol(%x)", dvol);
	cls = (*env)->FindClass(env, BASEPKG"/arc/DiskVol");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;JJI)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(dvol->vol_name),
	    JSTRING(dvol->host),
	    JSTRING(dvol->path),
	    (jlong)(dvol->capacity/1024),
	    (jlong)(dvol->free_space/1024),
	    (jint)(dvol->status_flags));
	PTRACE(2, "jni:diskvol2DiskVol() done");
	return (newObj);
}


void *
DiskVol2diskvol(JNIEnv *env, jobject diskVol) {

	jclass cls;
	disk_vol_t *dvol;

	PTRACE(2, "jni:DiskVol2diskvol() entry");
	cls = (*env)->GetObjectClass(env, diskVol);
	dvol = (disk_vol_t *)malloc(sizeof (disk_vol_t));
	getStrFld(env, cls, diskVol, "volName", dvol->vol_name);
	getStrFld(env, cls, diskVol, "host", dvol->host);
	getStrFld(env, cls, diskVol, "path", dvol->path);
	dvol->set_flags = 0; /* not used */
	dvol->capacity = 1024 * getJLongFld(env, cls, diskVol, "kbytesTotal");
	dvol->free_space = 1024 * getJLongFld(env, cls, diskVol, "kbytesAvail");
	dvol->status_flags = (uint32_t)
	    getJIntFld(env, cls, diskVol, "statusFlags");

	PTRACE(2, "jni:DiskVol2diskvol() done");
	return (dvol);
}

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_get(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring volName) {

	jobject newObj;
	jboolean isCopy;
	disk_vol_t *dskvol;
	char *cstr = GET_STR(volName, isCopy);
	int res;

	PTRACE(1, "jni:DiskVol_get(...,%s)", cstr);
	res = get_disk_vol(CTX, cstr, &dskvol);
	REL_STR(volName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = diskvol2DiskVol(env, dskvol);
	REL_STR(volName, cstr, isCopy);
	free(dskvol);
	PTRACE(1, "jni:DiskVol_get() done");
	return (newObj);
}



JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *dvollst;
	int res;

	PTRACE(1, "jni:DiskVol_getAll() entry");
	res = get_all_disk_vols(CTX, &dvollst);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    dvollst, BASEPKG"/arc/DiskVol", diskvol2DiskVol);
	lst_free_deep(dvollst);
	PTRACE(1, "jni:DiskVol_getAll() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_add(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject diskVol) {

	disk_vol_t *dvol;
	int res;

	PTRACE(1, "jni:DiskVol_add() entry");
	dvol = (disk_vol_t *)DiskVol2diskvol(env, diskVol);
	res = add_disk_vol(CTX, dvol);
	free(dvol);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:DiskVol_add() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_remove(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring volName) {

	jboolean isCopy;
	int res;
	char *cstr = GET_STR(volName, isCopy);

	PTRACE(1, "jni:DiskVol_remove(...,%s) entry", Str(cstr));
	res = remove_disk_vol(CTX, cstr);
	REL_STR(volName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:DiskVol_remove(...,%s) done", Str(cstr));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_getClients(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *clients;

	PTRACE(1, "jni:DiskVol_getClients() entry");
	if (-1 == get_all_clients(CTX, &clients)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
	    clients, "java/lang/String", charr2String);
	lst_free_deep(clients);
	PTRACE(1, "jni:DiskVol_getClients() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_addClient(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring hostName) {

	jboolean isCopy;
	int res;
	char *cstr = GET_STR(hostName, isCopy);

	PTRACE(1, "jni:DiskVol_addClient(...,%s) entry", Str(cstr));
	res = add_client(CTX, cstr);
	REL_STR(hostName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:DiskVol_addClient(...,%s) done", Str(cstr));
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_removeClient(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring hostName) {

	jboolean isCopy;
	int res;
	char *cstr = GET_STR(hostName, isCopy);

	PTRACE(1, "jni:DiskVol_removeClient(...,%s) entry", Str(cstr));
	res = remove_client(CTX, cstr);
	REL_STR(hostName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:DiskVol_removeClient(...,%s) done", Str(cstr));
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_DiskVol_setFlags(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring volName, jint flags) {

	jboolean isCopy;
	int res;
	char *cstr = GET_STR(volName, isCopy);

	PTRACE(1, "jni:DiskVol_setFlags(...,%s) entry", Str(cstr));
	res = set_disk_vol_flags(CTX, cstr, (uint32_t)flags);
	REL_STR(volName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:DiskVol_setFlags(...) done");
}
