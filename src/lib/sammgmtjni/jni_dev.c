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
#pragma ident	"$Revision: 1.19 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/device.h"
#include "pub/mgmt/error.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_fs_AU.h"
#include "jni_util.h"

void *
SCSIDevInfo2scsiinfo(JNIEnv *env, jobject scObj) {
	jclass cls;
	scsi_info_t *scsi_info;
	jboolean isCopy;
	jstring js;
	char *cs;
	jfieldID fid;

	PTRACE(2, "jni:SCSIDevInfo2scsiinfo() entry");
	if (NULL == scObj) {
		PTRACE(2, "jni:SCSIDevInfo2scsiinfo() done. (NULL)");
		return (NULL);
	}
	scsi_info = (scsi_info_t *)malloc(sizeof (scsi_info_t));
	if (scsi_info == NULL) {
		PTRACE(2, "jni:SCSIDevInfo2scsiinfo() done. (no mem)");
		return (NULL);
	}
	memset(scsi_info, 0, sizeof (scsi_info_t));
	cls = (*env)->GetObjectClass(env, scObj);
	getStrFld(env, cls, scObj, "vendor", scsi_info->vendor);
	getStrFld(env, cls, scObj, "prodID", scsi_info->prod_id);
	getStrFld(env, cls, scObj, "version", scsi_info->rev_level);
	fid = (*env)->GetFieldID(env, cls, "devID", "Ljava/lang/String;");
	if (NULL == fid)
		(*env)->ExceptionDescribe(env);

	js = (jstring)(*env)->GetObjectField(env, scObj, fid);
	cs = GET_STR(js, isCopy);
	scsi_info->dev_id = strdup(cs);
	REL_STR(js, cs, isCopy);

	PTRACE(2, "jni:SCSIDevInfo2scsiinfo() done");
	return (scsi_info);
}

jobject
scsiinfo2SCSIDevInfo(JNIEnv *env, void *v_scsi) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	scsi_info_t *scsi = (scsi_info_t *)v_scsi;

	PTRACE(2, "jni:scsiinfo2SCSIDevInfo()");
	if (v_scsi == NULL) {
		return (NULL);
	}
	cls = (*env)->FindClass(env, BASEPKG"/fs/SCSIDevInfo");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(scsi->vendor),
	    JSTRING(scsi->prod_id),
	    JSTRING(scsi->rev_level),
	    JSTRING(scsi->dev_id));
	if (newObj == NULL)
	    PTRACE(1, "newObj (scsiinfo) NULL");
	PTRACE(2, "jni:scsiinfo2SCSIDevInfo done");
	return (newObj);
}


jobject
au2AU(JNIEnv *env, void *v_au) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	au_t *au = (au_t *)v_au;

	PTRACE(2, "jni:converting au(%d,%s,%lld,%s)",
	    au->type, au->path, au->capacity, Str(au->raid));
	if (v_au == NULL)
		PTRACE(1, "v_au is NULL");
	cls = (*env)->FindClass(env, BASEPKG"/fs/AU");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(IJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "L"BASEPKG"/fs/SCSIDevInfo;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)au->type,
	    (jlong)(au->capacity/1024),
	    JSTRING(au->path),
	    JSTRING(au->fsinfo),
	    JSTRING(au->raid),
	    scsiinfo2SCSIDevInfo(env, au->scsiinfo));
	if (newObj == NULL)
	    PTRACE(1, "newObj (au) NULL");
	PTRACE(2, "jni:au2AU() done");
	return (newObj);
}


void *
AU2au(JNIEnv *env, jobject auObj) {

	jclass cls;
	au_t *au = (au_t *)malloc(sizeof (au_t));

	PTRACE(2, "jni:AU2au() entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/AU");

	au->type = (au_type_t)getJIntFld(env, cls, auObj, "type");
	au->capacity = 1024 * (dsize_t)
	    getJLongFld(env, cls, auObj, "size");
	getStrFld(env, cls, auObj, "path", au->path);

	au->raid = strdup("x");
	au->scsiinfo = SCSIDevInfo2scsiinfo(env,
	    getObjFld(env, cls, auObj, "scsi", "L"BASEPKG"/fs/SCSIDevInfo;"));
	PTRACE(2, "jni:AU2au() done");
	return (au);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_AU_discoverAvailAUs(JNIEnv *env,
    jclass auCls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *aulst;
	jobjectArray jarr;

	PTRACE(1, "jni:AU_discoverAvailAUs() entry");
	if (-1 == discover_avail_aus(CTX, &aulst)) {
		ThrowEx(env);
		return (NULL);
	}
	jarr = lst2jarray(env, aulst, BASEPKG"/fs/AU", au2AU);
	free_au_list(aulst);
	PTRACE(1, "jni:AU_discoverAvailAUs() done");
	return (jarr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_AU_discoverAUs(JNIEnv *env,
    jclass auCls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *aulst;
	jobjectArray jarr;

	PTRACE(1, "jni:AU_discoverAUs() entry");
	if (-1 == discover_aus(CTX, &aulst)) {
		ThrowEx(env);
		return (NULL);
	}
	jarr = lst2jarray(env, aulst, BASEPKG"/fs/AU", au2AU);
	free_au_list(aulst);
	PTRACE(1, "jni:AU_discoverAUs() done");
	return (jarr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_AU_discoverHAAUs(JNIEnv *env,
    jclass auCls /*ARGSUSED*/, jobject ctx,
    jobjectArray hosts, jboolean availOnly) {

	sqm_lst_t *aulst;
	jobjectArray jarr;

	PTRACE(1, "jni:AU_discoverHAAUs() entry");
	if (-1 == discover_ha_aus(CTX,
	    jarray2lst(env, hosts, "java/lang/String", String2charr),
	    CBOOL(availOnly), &aulst)) {
		ThrowEx(env);
		return (NULL);
	}
	jarr = lst2jarray(env, aulst, BASEPKG"/fs/AU", au2AU);
	free_au_list(aulst);
	PTRACE(1, "jni:AU_discoverHAAUs() done");
	return (jarr);

}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_AU_checkSlicesForOverlaps(JNIEnv *env,
    jclass auCls /*ARGSUSED*/, jobject ctx, jobjectArray slices) {

	sqm_lst_t *slicesin, *slicesout;
	jobjectArray jarr;
	int res;

	PTRACE(1, "jni:AU_checkSlicesForOverlaps() entry");
	slicesin = jarray2lst(env, slices, "java/lang/String", String2charr);
	res = check_slices_for_overlaps(CTX, slicesin, &slicesout);
	lst_free_deep(slicesin);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	jarr = lst2jarray(env, slicesout, "java/lang/String", charr2String);
	lst_free_deep(slicesout);
	PTRACE(1, "jni:AU_checkSlicesForOverlaps() done");
	return (jarr);
}
