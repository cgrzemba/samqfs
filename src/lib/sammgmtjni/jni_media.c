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
 * or https://illumos.org/license/CDDL.
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
#pragma ident	"$Revision: 1.43 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/load.h"
#include "mgmt/log.h"
/* non-API header files */
#include "mgmt/config/media.h"
#include "aml/types.h"	// defines DIS_MES_TYPS
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_media_Media.h"
#include "com_sun_netstorage_samqfs_mgmt_media_MountJob.h"
#include "com_sun_netstorage_samqfs_mgmt_media_LabelJob.h"
#include "jni_util.h"

static char *
drv2str(const drive_t *drv) {
	static char str[80];
	if (!drv)
		return (NULL);
	sprintf(str, "%-15s %-10s %-10s",
	    Str(drv->serial_no),
	    Str(drv->vendor_id),
	    Str(drv->product_id));
	return (str);
}


jintArray lsmlst2jintArray(JNIEnv *env, sqm_lst_t *lsmlst);
jintArray panellst2jintArray(JNIEnv *env, sqm_lst_t *panellst);

/* used for debug */
void
displayDriveLst(sqm_lst_t *drvs) {
	drive_t *drv;
	node_t *n = drvs->head;
	while (n != NULL) {
		drv = (drive_t *)n->data;
		printf("drive: %s lib:%s %s", drv->base_info.name,
		    drv->library_name, drv->vendor_id);
		n = n->next;
	}
}

/* C structure <-> Java class conversion functions */

jobject
drive2DriveDev(JNIEnv *env, void *v_drv) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	drive_t *drv = (drive_t *)v_drv;

	PTRACE(2, "jni:drive2DriveDev(%x) entry", drv);
	if (NULL == drv)
		return (NULL);
	PTRACE(3, "jni:log_time=%ld drv=%s (%d paths) shared:%s",
	    drv->log_modtime,
	    drv2str(drv), (drv->alternate_paths_list != NULL) ?
	    drv->alternate_paths_list->length : -1,
	    drv->shared ? "T" : "F");
	// TRACE("jni:drvfw=%s", Str(drv->firmware_version));
	cls = (*env)->FindClass(env, BASEPKG"/media/DriveDev");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;ILjava/lang/String;"
	    "Ljava/lang/String;IILjava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;"
	    "[Ljava/lang/String;ZLjava/lang/String;"
	    "Ljava/lang/String;[Ljava/lang/String;"
	    "Ljava/lang/String;JJJ)V");
	if (NULL == mid) {
		PTRACE(2, "jni:mid not found");
		exit(-1);
	}
	newObj = (*env)->NewObject(env, cls, mid,
	    /* BaseDev properties */
	    JSTRING(drv->base_info.name),
	    (jint)drv->base_info.eq,
	    JSTRING(drv->base_info.equ_type),
	    JSTRING(drv->base_info.set),
	    (jint)drv->base_info.fseq,
	    (jint)drv->base_info.state,
	    JSTRING(drv->base_info.additional_params),
	    /* DriveDev-specific properties */
	    JSTRING(drv->serial_no),
	    JSTRING(drv->vendor_id),
	    JSTRING(drv->product_id),
	    JSTRING(drv->dev_status),
	    lst2jarray(env,
		drv->alternate_paths_list, "java/lang/String", charr2String),
	    JBOOL(drv->shared),
	    JSTRING(drv->loaded_vsn),
	    JSTRING(drv->firmware_version),
	    arr2jarray(env,
		drv->dis_mes,
		DIS_MES_LEN,
		DIS_MES_TYPS,
		"java/lang/String",
		charr2String),
	    JSTRING(drv->log_path),
	    (jlong)drv->log_modtime,
	    (jlong)drv->load_idletime,
	    (jlong)drv->tapealert_flags);
	PTRACE(2, "jni:drive2DriveDev() done");
	return (newObj);
}


void *
DriveDev2drive(JNIEnv *env, jobject drvObj) {

	jclass cls;
	int i;
	drive_t *drv = (drive_t *)malloc(sizeof (drive_t));

	PTRACE(2, "jni:DriveDev2drive() entry");
	cls = (*env)->GetObjectClass(env, drvObj);
	/* BaseDev properties */
	getStrFld(env, cls, drvObj, "devPath", drv->base_info.name);
	drv->base_info.eq = (equ_t)getJIntFld(env, cls, drvObj, "eq");
	getStrFld(env, cls, drvObj, "eqType", drv->base_info.equ_type);
	getStrFld(env, cls, drvObj, "fsetName", drv->base_info.set);
	drv->base_info.fseq = (equ_t)getJIntFld(env, cls, drvObj, "eqFSet");
	drv->base_info.state = (int)getJIntFld(env, cls, drvObj, "state");
	getStrFld(env, cls, drvObj, "paramFilePath",
	    drv->base_info.additional_params);
	/* DrivevDev-specific properties */
	getStrFld(env, cls, drvObj, "serialNum", drv->serial_no);
	strcpy(drv->library_name, drv->base_info.set);
	getStrFld(env, cls, drvObj, "vendor", drv->vendor_id);
	getStrFld(env, cls, drvObj, "product", drv->product_id);
	getStrFld(env, cls, drvObj, "devStatus", drv->dev_status);
	drv->alternate_paths_list = jarray2lst(env,
	    getJArrFld(env,
		cls, drvObj, "altPaths", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);
	drv->shared = getBoolFld(env, cls, drvObj, "shared");
	getStrFld(env, cls, drvObj, "loadedVSN", drv->loaded_vsn);
	getStrFld(env, cls, drvObj, "firmware", drv->firmware_version);
	for (i = 0; i < DIS_MES_TYPS; i++)
		drv->dis_mes[i][0] = '\0';
	/* used for discovery, not populated for use by GUI */
	drv->discover_state = 0;
	drv->scsi_version = 0;
	drv->scsi_path[0] = '\0';
	drv->scsi_2_info.lun_id = 0;
	drv->scsi_2_info.target_id = 0;
	drv->wwn[0] = '\0';
	drv->id_type = 0;
	for (i = 0; i < MAXIMUM_WWN; i++) {
		drv->wwn_id[i][0] = '\0';
		drv->wwn_id_type[i] = 0;
	}
	getStrFld(env, cls, drvObj, "logPath", drv->log_path);
	drv->log_modtime = (time_t)getJLongFld(env, cls, drvObj, "logModTime");
	drv->load_idletime =
		(time_t)getJLongFld(env, cls, drvObj, "loadIdleTime");
	drv->tapealert_flags =
		(uint64_t)getJLongFld(env, cls, drvObj, "tapeAlertFlags");
	PTRACE(2, "jni:DriveDev2drive(shared:%s) done",
	    drv->shared ? "T" : "F");
	return (drv);
}

void *
StkCapacity2stkcapacity(JNIEnv *env, jobject capObj) {

	jclass cls;
	stk_capacity_t *stk_capacity;

	PTRACE(2, "jni:StkCapacity2stkcapacity() entry");
	if (capObj == NULL) {
		PTRACE(2, "jni:capObj is null");
		return (NULL);
	}

	stk_capacity = (stk_capacity_t *)malloc(sizeof (stk_capacity_t));

	cls = (*env)->GetObjectClass(env, capObj); // StkCapacity
	stk_capacity->index = (int)getJIntFld(env, cls, capObj, "index");
	stk_capacity->value = (fsize_t)getJLongFld(env, cls, capObj, "value");

	PTRACE(2, "jni:stk_capacity(%ld,%d)",
		stk_capacity->value,
		stk_capacity->index);

	PTRACE(2, "jni:StkCapacity2stkcapcity() done");
	return (stk_capacity);

}

jobject
stkcapacity2StkCapacity(JNIEnv *env, void *v_stkcap) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_capacity_t *stkcapacity = (stk_capacity_t *)v_stkcap;

	PTRACE(2, "jni:stkcapacity2StkCapacity() entry");
	if (stkcapacity == NULL) {
		PTRACE(2, "jni:stkcapacity is null");
		return (NULL);
	}
	PTRACE(3, "jni: stkcapacity(%lld,%d)",
		stkcapacity->value,
		stkcapacity->index);

	cls = (*env)->FindClass(env, BASEPKG"/media/StkCapacity");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(IJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)stkcapacity->index,
	    (jlong)stkcapacity->value);

	PTRACE(2, "jni:stkcapacity2StkCapacity() done");
	return (newObj);
}

void *
StkDev2stkdev(JNIEnv *env, jobject devObj) {

	jclass cls;
	stk_device_t *stkdev;

	PTRACE(2, "jni:StkDev2stkdev() entry");
	if (devObj == NULL) {
		PTRACE(2, "jni:devObj is null");
		return (NULL);
	}

	stkdev = (stk_device_t *)malloc(sizeof (stk_device_t));

	cls = (*env)->GetObjectClass(env, devObj); // StkDevice
	getStrFld(env, cls, devObj, "pathName", stkdev->pathname);
	stkdev->acs_num = (int)getJIntFld(env, cls, devObj, "acsNum");
	stkdev->lsm_num = (int)getJIntFld(env, cls, devObj, "lsmNum");
	stkdev->panel_num = (int)getJIntFld(env, cls, devObj, "panelNum");
	stkdev->drive_num = (int)getJIntFld(env, cls, devObj, "driveNum");
	stkdev->shared = getBoolFld(env, cls, devObj, "shared");

	PTRACE(2, "jni:stk_device_t(%d,%d,%d)",
		stkdev->acs_num,
		stkdev->lsm_num,
		stkdev->panel_num);

	PTRACE(2, "jni:StkDev2stkdev() done");
	return (stkdev);

}

jobject
stkdev2StkDev(JNIEnv *env, void *v_stkdev) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_device_t *stkdev = (stk_device_t *)v_stkdev;

	PTRACE(2, "jni:stkdev2StkDev() entry");
	if (stkdev == NULL) {
		PTRACE(2, "jni:stkdev2StkDev is null");
		return (NULL);
	}
	PTRACE(3, "jni: stkdev(%s,%d,%d,%d,%d,%s)",
		stkdev->pathname,
		stkdev->acs_num,
		stkdev->lsm_num,
		stkdev->panel_num,
		stkdev->drive_num,
		stkdev->shared ? "T" : "F");
	cls = (*env)->FindClass(env, BASEPKG"/media/StkDevice");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IIIIZ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(stkdev->pathname),
	    (jint)stkdev->acs_num,
	    (jint)stkdev->lsm_num,
	    (jint)stkdev->panel_num,
	    (jint)stkdev->drive_num,
	    JBOOL(stkdev->shared));

	PTRACE(2, "jni:stkdev2StkDev() done");
	return (newObj);
}

void *
StkCap2stkcap(JNIEnv *env, jobject capObj) {

	jclass cls;
	stk_cap_t *stk_cap;

	PTRACE(2, "jni:StkCap2stkcap() entry");
	if (capObj == NULL) {
		PTRACE(2, "jni:capObj is null");
		return (NULL);
	}

	stk_cap = (stk_cap_t *)malloc(sizeof (stk_cap_t));

	cls = (*env)->GetObjectClass(env, capObj); // StkCap
	stk_cap->acs_num = (int)getJIntFld(env, cls, capObj, "acsNum");
	stk_cap->lsm_num = (int)getJIntFld(env, cls, capObj, "lsmNum");
	stk_cap->cap_num = (int)getJIntFld(env, cls, capObj, "capNum");

	PTRACE(2, "jni:stk_cap(%d,%d,%d)",
		stk_cap->acs_num,
		stk_cap->lsm_num,
		stk_cap->cap_num);

	PTRACE(2, "jni:StkCap2stkcap() done");
	return (stk_cap);

}

jobject
stkcap2StkCap(JNIEnv *env, void *v_stkcap) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_cap_t *stkcap = (stk_cap_t *)v_stkcap;

	PTRACE(2, "jni:stkcap2StkCap() entry");
	if (stkcap == NULL) {
		PTRACE(2, "jni:stkcap2StkCap is null");
		return (NULL);
	}
	PTRACE(3, "jni: stkcap(%d,%d,%d)",
		stkcap->acs_num,
		stkcap->lsm_num,
		stkcap->cap_num);

	cls = (*env)->FindClass(env, BASEPKG"/media/StkCap");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(III)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)stkcap->acs_num,
	    (jint)stkcap->lsm_num,
	    (jint)stkcap->cap_num);

	PTRACE(2, "jni:stkcap2StkCap() done");
	return (newObj);
}

void *
StkParam2stkparam(JNIEnv *env, jobject paramObj) {

	jclass cls;
	stk_param_t *stk_param;
	void *temp = NULL;

	PTRACE(2, "jni:StkParam2stkparam() entry");
	if (paramObj == NULL) {
		PTRACE(2, "jni:paramObj is null");
		return (NULL);
	}

	stk_param = (stk_param_t *)malloc(sizeof (stk_param_t));

	cls = (*env)->GetObjectClass(env, paramObj); // StkNetLibParam
	getStrFld(env, cls, paramObj, "path", stk_param->param_path);
	getStrFld(env, cls, paramObj, "acsServerName", stk_param->hostname);
	stk_param->portnum = (int)getJIntFld(env, cls, paramObj, "acsPort");
	getStrFld(env, cls, paramObj, "access", stk_param->access);
	getStrFld(env, cls, paramObj, "samServerName", stk_param->ssi_host);
	stk_param->ssi_inet_portnum = (int)getJIntFld(env, cls, paramObj,
	    "samRecvPort");
	stk_param->csi_hostport = (int)getJIntFld(env, cls, paramObj,
	    "samSendPort");

	/* not used by the gui */
	temp = StkCap2stkcap(env,
	    getObjFld(env, cls, paramObj, "stkCap",
	    "L"BASEPKG"/media/StkCap;"));
	if (temp != NULL) {
	    stk_param->stk_cap = *(stk_cap_t *)temp;
	} else {
	    (void) memset((char *)&stk_param->stk_cap, 0,
		sizeof (stk_param->stk_cap));
	}

	stk_param->stk_capacity_list = jarray2lst(env,
	    getJArrFld(env,
		cls, paramObj, "stkCapacities",
		"[L"BASEPKG"/media/StkCapacity;"),
	    BASEPKG"/media/StkCapacity",
	    StkCapacity2stkcapacity);

	stk_param->stk_device_list = jarray2lst(env,
	    getJArrFld(env,
		cls, paramObj, "stkDevices",
		"[L"BASEPKG"/media/StkDevice;"),
	    BASEPKG"/media/StkDevice",
	    StkDev2stkdev);

	PTRACE(2, "jni:stk_param(%s,%s,%s,%d,%s,%d, %d)",
		stk_param->param_path,
		stk_param->hostname,
		stk_param->access,
		stk_param->portnum,
		stk_param->ssi_host,
		stk_param->ssi_inet_portnum,
		stk_param->csi_hostport);

	PTRACE(2, "jni:StkClntConn2stkhostinfo() done");
	return (stk_param);

}

jobject
stkparam2StkParam(JNIEnv *env, void *v_stkparam) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_param_t *stkparam = (stk_param_t *)v_stkparam;

	PTRACE(2, "jni:stkparam2StkParam() entry");
	if (stkparam == NULL) {
		PTRACE(2, "jni:stkparam2StkParam is null");
		return (NULL);
	}
	PTRACE(3, "jni: stkparam(%s,%s,%s,%d,%s,%d,%d)",
		stkparam->param_path,
		stkparam->access,
		stkparam->hostname,
		stkparam->portnum,
		stkparam->ssi_host,
		stkparam->ssi_inet_portnum,
		stkparam->csi_hostport);

	cls = (*env)->FindClass(env, BASEPKG"/media/StkNetLibParam");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "ILjava/lang/String;IIL"BASEPKG"/media/StkCap;"
	    "[L"BASEPKG"/media/StkCapacity;[L"
BASEPKG"/media/StkDevice;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(stkparam->param_path),
	    JSTRING(stkparam->access),
	    JSTRING(stkparam->hostname),
	    (jint)stkparam->portnum,
	    JSTRING(stkparam->ssi_host),
	    (jint)stkparam->ssi_inet_portnum,
	    (jint)stkparam->csi_hostport,
	    stkcap2StkCap(env, &stkparam->stk_cap),
	    lst2jarray(env, stkparam->stk_capacity_list,
		BASEPKG"/media/StkCapacity", stkcapacity2StkCapacity),
	    lst2jarray(env, stkparam->stk_device_list,
		BASEPKG"/media/StkDevice", stkdev2StkDev));

	PTRACE(2, "jni:stkparam2StkParam() done");
	return (newObj);
}

jobject
mdlic2MdLicense(JNIEnv *env, void *v_mdlic) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	md_license_t *mdlic = (md_license_t *)v_mdlic;

	PTRACE(2, "jni:mdlic2MdLicense() entry");
	cls = (*env)->FindClass(env, BASEPKG"/media/MdLicense");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;II)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(mdlic->media_type),
	    (jint)mdlic->max_licensed_slots,
	    (jint)mdlic->robot_type);
	PTRACE(2, "jni:mdlic2MdLicense() done");
	return (newObj);
}

/* not sure if this is needed. license ignored by API-s other then get()? */
void *
MdLicense2mdlic(JNIEnv *env, jobject licObj) {

	jclass cls;
	md_license_t *lic;

	PTRACE(2, "jni:MdLicense2mdlic() entry");
	lic = (md_license_t *)malloc(sizeof (md_license_t));
	cls = (*env)->GetObjectClass(env, licObj);
	getStrFld(env, cls, licObj, "mediaType", lic->media_type);
	lic->max_licensed_slots =  getJIntFld(env, cls, licObj, "maxSlots");
	lic->robot_type = getJIntFld(env, cls, licObj, "robotType");
	PTRACE(2, "jni:MdLicense2mdlic() done");
	return (lic);
}


jobject
lib2LibDev(JNIEnv *env, void *v_lib) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	library_t *lib = (library_t *)v_lib;

	PTRACE(2, "jni:lib2LibDev(%ld) added stkparam entry", lib->log_modtime);

	cls = (*env)->FindClass(env, BASEPKG"/media/LibDev");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    /* BaseDev arguments */
	    "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;"
	    "IILjava/lang/String;"
	    /* LibDev-specific arguments */
	    "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;"
	    "[L"BASEPKG"/media/DriveDev;[L"BASEPKG"/media/MdLicense;"
	    "Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;"
	    "[Ljava/lang/String;L"BASEPKG"/media/StkNetLibParam;"
	    "Ljava/lang/String;J)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    /* BaseDev properties */
	    JSTRING(lib->base_info.name),
	    (jint)lib->base_info.eq,
	    JSTRING(lib->base_info.equ_type),
	    JSTRING(lib->base_info.set),
	    (jint)lib->base_info.fseq,
	    (jint)lib->base_info.state,
	    JSTRING(lib->base_info.additional_params),
	    /* LibDev-specific properties */
	    JSTRING(lib->serial_no),
	    JSTRING(lib->vendor_id),
	    JSTRING(lib->product_id),
	    JSTRING(lib->firmware_version),
	    lst2jarray(env, lib->drive_list,
		BASEPKG"/media/DriveDev", drive2DriveDev),
	    lst2jarray(env, lib->media_license_list,
		BASEPKG"/media/MdLicense", mdlic2MdLicense),
	    JSTRING(lib->dev_status),
	    lst2jarray(env,
		lib->alternate_paths_list, "java/lang/String", charr2String),
	    JSTRING(lib->catalog_path),
	    arr2jarray(env,
		lib->dis_mes,
		DIS_MES_LEN,
		DIS_MES_TYPS,
		"java/lang/String",
		charr2String),
	    stkparam2StkParam(env, lib->storage_tek_parameter),
	    JSTRING(lib->log_path),
	    (jlong)lib->log_modtime);

	PTRACE(2, "jni:lib2LibDev() done");
	return (newObj);
}


void *
LibDev2lib(JNIEnv *env, jobject libObj) {

	jclass cls;
	int i;
	library_t *lib = (library_t *)malloc(sizeof (library_t));
	memset(lib, 0, sizeof (library_t));

	PTRACE(2, "jni:LibDev2lib() entry");
	cls = (*env)->GetObjectClass(env, libObj);
	/* BaseDev properties */
	getStrFld(env, cls, libObj, "devPath", lib->base_info.name);
	lib->base_info.eq = (equ_t)getJIntFld(env, cls, libObj, "eq");
	getStrFld(env, cls, libObj, "eqType", lib->base_info.equ_type);
	getStrFld(env, cls, libObj, "fsetName", lib->base_info.set);
	lib->base_info.fseq = (equ_t)getJIntFld(env, cls, libObj, "eqFSet");
	lib->base_info.state = (int)getJIntFld(env, cls, libObj, "state");
	getStrFld(env, cls, libObj, "paramFilePath",
	    lib->base_info.additional_params);
	/* LibDev-specific properties */
	getStrFld(env, cls, libObj, "serialNum", lib->serial_no);
	getStrFld(env, cls, libObj, "vendor", lib->vendor_id);
	getStrFld(env, cls, libObj, "product", lib->product_id);
	getStrFld(env, cls, libObj, "firmware", lib->firmware_version);
	lib->drive_list = jarray2lst(env,
	    getJArrFld(env,
		cls, libObj, "drives", "[L"BASEPKG"/media/DriveDev;"),
	    BASEPKG"/media/DriveDev",
	    DriveDev2drive);
	// TRACE("jni:lib->drv_list[%d]", lib->drive_list->length);
	lib->media_license_list = jarray2lst(env,
	    getJArrFld(env,
		cls, libObj, "mlicenses", "[L"BASEPKG"/media/MdLicense;"),
	    BASEPKG"/media/MdLicense",
	    MdLicense2mdlic);
	getStrFld(env, cls, libObj, "devStatus", lib->dev_status);
	lib->alternate_paths_list = jarray2lst(env,
	    getJArrFld(env,
		cls, libObj, "altPaths", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);
	getStrFld(env, cls, libObj, "catalogPath", lib->catalog_path);
	// displayDriveLst(lib->drive_list);
	lib->no_of_drives = (NULL != lib->drive_list) ? lib->drive_list->length
	    : 0;
	for (i = 0; i < DIS_MES_TYPS; i++)
		lib->dis_mes[i][0] = '\0';
	/* used for discovery, not populated for use by GUI */
	lib->discover_state = 0;
	lib->scsi_version = 0;
	lib->scsi_path[0] = '\0';
	lib->id_type = 0;
	lib->storage_tek_parameter = StkParam2stkparam(env,
	    getObjFld(env, cls, libObj, "stkParam",
	    "L"BASEPKG"/media/StkNetLibParam;"));
	getStrFld(env, cls, libObj, "logPath", lib->log_path);
	lib->log_modtime = (time_t)getJLongFld(env, cls, libObj, "logModTime");
	PTRACE(2, "jni:LibDev2lib() done");
	return (lib);
}


void *
NetAttachLibInfo2netattlib(JNIEnv *env, jobject libObj) {

	jclass cls;
	nwlib_req_info_t *nalib = (nwlib_req_info_t *)
	    malloc(sizeof (nwlib_req_info_t));

	PTRACE(2, "jni:NetAttachLibInfo2netattlib() entry");
	cls = (*env)->GetObjectClass(env, libObj); // NetAttachLibInfo
	getStrFld(env, cls, libObj, "name", nalib->nwlib_name);
	nalib->nw_lib_type = (nw_lib_type_t)
	    getJIntFld(env, cls, libObj, "type");
	nalib->eq = (equ_t)getJIntFld(env, cls, libObj, "eq");
	getStrFld(env, cls, libObj, "catalogPath", nalib->catalog_loc);
	getStrFld(env, cls, libObj,
	    "paramFilePath", nalib->parameter_file_loc);
	PTRACE(2, "jni:NetAttachLibInfo2netattlib() done");
	return (nalib);
}


void *
ImportOpts2impopt(JNIEnv *env, jobject ioptObj) {

	jclass cls;
	import_option_t *iopt;

	PTRACE(2, "jni:ImportOpts2impopt() entry");
	if (NULL == ioptObj) {
		PTRACE(2, "jni:no import options");
		return (NULL);
	}
	iopt = (import_option_t *)malloc(sizeof (import_option_t));
	cls = (*env)->GetObjectClass(env, ioptObj); // ImportOpts
	getStrFld(env, cls, ioptObj, "vsn", iopt->vsn);
	getStrFld(env, cls, ioptObj, "barcode", iopt->barcode);
	getStrFld(env, cls, ioptObj, "mediaType", iopt->mtype);
	iopt->audit = getBoolFld(env, cls, ioptObj, "audit");
	iopt->foreign_tape = getBoolFld(env, cls, ioptObj, "foreignTape");
	/* network-attached StorageTek libraries */
	iopt->vol_count = (int)getJIntFld(env, cls, ioptObj, "volCount");
	iopt->pool = (long)getJLongFld(env, cls, ioptObj, "pool");
	PTRACE(2, "jni:ImportOpts2impopt() done");
	return (iopt);
}


jobject
catentry2CatEntry(JNIEnv *env, void *v_catentry) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	struct CatalogEntry *catentry = (struct CatalogEntry *)v_catentry;

	PTRACE(2, "jni:catentry2CatEntry() entry");
	cls = (*env)->FindClass(env, BASEPKG"/media/CatEntry");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ILjava/lang/String;Ljava/lang/String;IIJJJJJJJLjava/lang/String;"
	    "JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)catentry->CeStatus,
	    JSTRING(catentry->CeMtype),
	    JSTRING(catentry->CeVsn),
	    (jint)catentry->CeSlot,
	    (jint)catentry->CePart,
	    (jlong)catentry->CeAccess,
	    (jlong)(catentry->CeCapacity / 1024), // kbytes
	    (jlong)(catentry->CeSpace / 1024),    // kbytes
	    (jlong)catentry->CeBlockSize,
	    (jlong)catentry->CeLabelTime,
	    (jlong)catentry->CeModTime,
	    (jlong)catentry->CeMountTime,
	    JSTRING(catentry->CeBarCode),
	    // archive reservation
	    (jlong)catentry->r.CerTime,
	    JSTRING(catentry->r.CerAsname),
	    JSTRING(catentry->r.CerOwner),
	    JSTRING(catentry->r.CerFsname),
	    // library eq
	    (jint)catentry->CeEq);
	PTRACE(2, "jni:catentry2CatEntry() done");
	return (newObj);
}


void *
ReservInfo2reserve(JNIEnv *env, jobject resObj) {

	jclass cls;
	reserve_option_t *resrv = (reserve_option_t *)
	    malloc(sizeof (reserve_option_t));

	PTRACE(2, "jni:ReservInfo2reserve() entry");
	cls = (*env)->GetObjectClass(env, resObj); // ReservInfo
	resrv->CerTime = (time32_t)getJLongFld(env, cls, resObj, "resTime");
	getStrFld(env, cls, resObj, "resCopyName", resrv->CerAsname);
	getStrFld(env, cls, resObj, "resOwner", resrv->CerOwner);
	getStrFld(env, cls, resObj, "resFS", resrv->CerFsname);
	PTRACE(2, "jni:ReservInfo2reserve() done");
	return (resrv);
}


jobject
pendload2MountJob(JNIEnv *env, void *v_pendload) {
	jclass cls;
	jmethodID mid;
	jobject newObj;
	pending_load_info_t *pendload = (pending_load_info_t *)v_pendload;

	PTRACE(2, "jni:pendload2MountJob() entry");
	cls = (*env)->FindClass(env, BASEPKG"/media/MountJob");

	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ISILjava/lang/String;J"
	    "Ljava/lang/String;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)pendload->id,	/* preview id (formerly slot) */
	    (jshort)pendload->flags,
	    (jint)pendload->robot_equ,
	    JSTRING(pendload->media),
	    (jlong)pendload->pid,
	    JSTRING(pendload->user),
	    JSTRING(pendload->vsn));
	PTRACE(2, "jni:pendload2MountJob() done");
	return (newObj);
}



void *
StkClntConn2stkhostinfo(JNIEnv *env, jobject libObj) {

	jclass cls;
	stk_host_info_t *stk_host_info = (stk_host_info_t *)
	    malloc(sizeof (stk_host_info_t));
	memset(stk_host_info, 0, sizeof (stk_host_info_t));

	PTRACE(2, "jni:StkClntConn2stkhostinfo() entry");
	cls = (*env)->GetObjectClass(env, libObj); // StkClntConn
	getStrFld(env, cls, libObj, "acsServerName", stk_host_info->hostname);
	getStrFld(env, cls, libObj, "acsPort", stk_host_info->portnum);
	getStrFld(env, cls, libObj, "access", stk_host_info->access);
	getStrFld(env, cls, libObj, "samServerName", stk_host_info->ssi_host);
	getStrFld(env, cls, libObj, "samRecvPort",
	    stk_host_info->ssi_inet_portnum);
	getStrFld(env, cls, libObj, "samSendPort", stk_host_info->csi_hostport);

	PTRACE(2, "jni:stk_host_info(%s,%s,%s,%s,%s,%s)",
		stk_host_info->hostname,
		stk_host_info->portnum,
		stk_host_info->access,
		stk_host_info->ssi_host,
		stk_host_info->ssi_inet_portnum,
		stk_host_info->csi_hostport);

	PTRACE(2, "jni:StkClntConn2stkhostinfo() done");
	return (stk_host_info);
}

jobject
stkpool2StkPool(JNIEnv *env, void *v_stkpool) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_pool_t *stkpool = (stk_pool_t *)v_stkpool;

	PTRACE(2, "jni:stkpool2StkPool() entry");
	PTRACE(3, "jni: stkpool(%d,%d,%ld,%s)",
		stkpool->pool_id, stkpool->low_water_mark,
		stkpool->high_water_mark, stkpool->over_flow);

	cls = (*env)->FindClass(env, BASEPKG"/media/StkPool");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(IJILjava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)stkpool->pool_id,
	    (jlong)stkpool->high_water_mark,
	    (jint)stkpool->low_water_mark,
	    JSTRING(stkpool->over_flow));

	PTRACE(2, "jni:stkpool2StkPool() done");
	return (newObj);
}

jobject
stkcell2StkCell(JNIEnv *env, void *v_stkcell) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_cell_t *stkcell = (stk_cell_t *)v_stkcell;

	PTRACE(2, "jni:stkcell2StkCell() entry");
	PTRACE(3, "jni:stkcell(%d,%d,%d,%d)",
		stkcell->min_row,
		stkcell->max_row,
		stkcell->min_column,
		stkcell->max_column);

	cls = (*env)->FindClass(env, BASEPKG"/media/StkCell");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(IIII)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)stkcell->min_row,
	    (jint)stkcell->max_row,
	    (jint)stkcell->min_column,
	    (jint)stkcell->max_column);

	PTRACE(2, "jni:stkcell2StkCell() done");
	return (newObj);
}

jobject
stkvol2StkVSN(JNIEnv *env, void *v_stkvol) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_volume_t *stkvol = (stk_volume_t *)v_stkvol;

	PTRACE(2, "jni:stkvol2StkVSN() entry");
	cls = (*env)->FindClass(env, BASEPKG"/media/StkVSN");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IIIIIILjava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(stkvol->stk_vol),
	    (jint)stkvol->acs_num,
	    (jint)stkvol->lsm_num,
	    (jint)stkvol->panel_num,
	    (jint)stkvol->row_id,
	    (jint)stkvol->col_id,
	    (jint)stkvol->pool_id,
	    JSTRING(stkvol->status),
	    JSTRING(stkvol->media_type),
	    JSTRING(stkvol->volume_type));

	PTRACE(2, "jni:stkvol2StkVSN() done");
	return (newObj);
}


jobject
stkphyconf2StkPhyConf(JNIEnv *env, void *v_stkphyconf) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	stk_phyconf_info_t *stkphyconf = (stk_phyconf_info_t *)v_stkphyconf;
	sqm_lst_t *ilsm_lst = lst_create();
	sqm_lst_t *ipanel_lst = lst_create();

	PTRACE(2, "jni:stkphyconf2StkPhyConf() entry");

	/*
	 * from 5.0. only the stk scratch pool is required, the physical
	 * location such as panel, lsm and cell are not required
	 */

	cls = (*env)->FindClass(env, BASEPKG"/media/StkPool");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(IIII[I[L"BASEPKG"/media/StkPool;[I)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    stkphyconf->stk_cell_info.min_row,
	    stkphyconf->stk_cell_info.max_row,
	    stkphyconf->stk_cell_info.min_column,
	    stkphyconf->stk_cell_info.max_column,
	    lst2jintArray(env, ipanel_lst),
	    lst2jarray(env, stkphyconf->stk_pool_list,
		BASEPKG"/media/StkPool", stkpool2StkPool),
	    lst2jintArray(env, ilsm_lst));

	PTRACE(2, "jni:stkphyconf2StkPhyConf() done");
	return (newObj);
}

/* native functions implementation */


/* Part1/3: library configuration functions */

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_discoverUnused(JNIEnv *env,
    jclass unused /* ARGSUSED */, jobject ctx) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	sqm_lst_t *liblst, *drvlst = NULL, *netattlibs;
	int res;
	sqm_lst_t *lst = lst_create();

	PTRACE(1, "jni:Media_discoverUnused() entry");

	res = discover_media_unused_in_mcf(CTX, &liblst);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	// TRACE("jni:API call discover_media() done");
	if (NULL == liblst) {
		PTRACE(1, "jni:liblst is NULL");
	}
	if (NULL == drvlst) {
		PTRACE(1, "jni:drvlst is NULL");
	}

	cls = (*env)->FindClass(env, BASEPKG"/media/Discovered");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "([L"BASEPKG"/media/LibDev;[L"BASEPKG"/media/DriveDev;)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    lst2jarray(env, liblst, BASEPKG"/media/LibDev", lib2LibDev),
	    lst2jarray(env, lst, BASEPKG"/media/DriveDev", drive2DriveDev));
	free_list_of_libraries(liblst);
	free_list_of_drives(lst);
	PTRACE(1, "jni:Media_discoverUnused() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getNetAttachLib(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject netAttLibInfo) {

	jobject newObj;
	library_t *nwlib;
	nwlib_req_info_t *inf;
	int res;

	PTRACE(1, "jni:Media_getNetAttachLib() entry");
	inf = NetAttachLibInfo2netattlib(env, netAttLibInfo);
	res = get_nw_library(CTX, inf, &nwlib);
	free(inf);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = lib2LibDev(env, nwlib);
	free_library(nwlib);
	PTRACE(1, "jni:Media_getNetAttachLib() done");
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibraries(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *liblst;

	PTRACE(1, "jni:Media_getLibraries() entry");
	if (-1 == get_all_libraries(CTX, &liblst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, liblst, BASEPKG"/media/LibDev", lib2LibDev);
	free_list_of_libraries(liblst);
	PTRACE(1, "jni:Media_getLibraries() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibraryByPath(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jstring pathStr) {

	jboolean isCopy;
	jobject newObj;
	library_t *lib;
	char *cstr = GET_STR(pathStr, isCopy);
	int res;

	PTRACE(1, "jni:Media_getLibraryByPath(%s) entry", cstr);
	res = get_library_by_path(CTX, cstr, &lib);
	REL_STR(pathStr, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = lib2LibDev(env, lib);
	free_library(lib);
	PTRACE(1, "jni:Media_getLibraryByPath() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibraryByFSet(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jstring fsetStr) {

	jboolean isCopy;
	jobject newObj;
	library_t *lib;
	char *cstr = GET_STR(fsetStr, isCopy);
	int res;

	PTRACE(1, "jni:Media_getLibraryByFSet(%s) entry", cstr);
	res = get_library_by_family_set(CTX, cstr, &lib);
	REL_STR(fsetStr, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = lib2LibDev(env, lib);
	free_library(lib);
	PTRACE(1, "jni:Media_getLibraryByFSet() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibraryByEq(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq) {

	jobject newObj;
	library_t *lib;

	PTRACE(1, "jni:Media_getLibraryByEq(%d) entry", (equ_t)eq);
	if (-1 == get_library_by_equ(CTX, (equ_t)eq, &lib)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = lib2LibDev(env, lib);
	free_library(lib);
	PTRACE(1, "jni:Media_getLibraryByEq() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_addLibrary(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jobject libObj) {

	library_t *lib;
	int res;

	PTRACE(1, "jni:Media_addLibrary() entry");
	lib = (library_t *)LibDev2lib(env, libObj);
	res = add_library(CTX, lib);
	free_library(lib);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_addLibrary() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_removeLibrary(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jboolean unload) {

	PTRACE(1, "jni:Media_removeLibrary(...,%d,%d)",
	    (equ_t)eq, CBOOL(unload));
	if (-1 == remove_library(CTX, (equ_t)eq, CBOOL(unload))) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_removeLibrary() done");
}


/* Part2/3: standalone drive condfiguration functions */

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getStdDrives(JNIEnv *env,
jclass cls /* ARGSUSED */, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *drvlst;

	PTRACE(1, "jni:Media_getStdDrives() entry");
	if (-1 == get_all_standalone_drives(CTX, &drvlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, drvlst,
	    BASEPKG"/media/DriveDev", drive2DriveDev);
	free_list_of_drives(drvlst);
	PTRACE(1, "jni:Media_getStdDrives() done");
	return (newArr);
}

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getStdDriveByPath(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jstring pathStr) {

	jboolean isCopy;
	jobject newObj;
	drive_t *drv;
	char *cstr = GET_STR(pathStr, isCopy);
	int res;

	PTRACE(1, "jni:Media_getStdDriveByPath(%s) entry", cstr);
	res = get_standalone_drive_by_path(CTX, cstr, &drv);
	REL_STR(pathStr, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = drive2DriveDev(env, drv);
	free_drive(drv);
	PTRACE(1, "jni:Media_getStdDriveByPath() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getStdDriveByEq(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq) {

	jobject newObj;
	drive_t *drv;

	PTRACE(1, "jni:Media_getStdDriveByEq(%d) entry", (equ_t)eq);
	if (-1 == get_standalone_drive_by_equ(CTX, (equ_t)eq, &drv)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = drive2DriveDev(env, drv);
	free_drive(drv);
	PTRACE(1, "jni:Media_getStdDriveByEq() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_addStdDrive(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jobject drvObj) {

	drive_t *drv;
	int res;

	PTRACE(1, "jni:Media_addStdDrive() entry");
	drv = (drive_t *)DriveDev2drive(env, drvObj);
	res = add_standalone_drive(CTX, drv);
	free_drive(drv);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_addStdDrive() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_removeStdDrive(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq) {

	PTRACE(1, "jni:Media_removeStdDrive(...,%d)", (equ_t)eq);
	if (-1 == remove_standalone_drive(CTX, (equ_t)eq)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_removeStdDrive() done");
}


/* Part3/3: media control functions */

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_auditSlot(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jint slot, jint part,
    jboolean skipToEOD) {

	PTRACE(1, "jni:Media_auditSlot() entry");
	if (-1 == rb_auditslot_from_eq(CTX,
	    (equ_t)libEq,
	    (int)slot,
	    (int)part,
	    CBOOL(skipToEOD))) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_auditSlot() done");
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getNumberOfCatEntries(
	JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jint libEq) {

	int cat_entries;

	PTRACE(1, "jni:Media_getNumberOfCatEntries() entry");
	if (-1 == get_no_of_catalog_entries(CTX, (int)libEq, &cat_entries)) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:Media_getNumberOfCatEntries() done");
	return ((jint)cat_entries);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_chgMediaStatus(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jint slot,
    jboolean set, jint flags) {

	PTRACE(1, "jni:Media_chgMediaStatus() entry");
	if (-1 == rb_chmed_flags_from_eq(CTX, (equ_t)libEq, (int)slot,
	    CBOOL(set), (uint32_t)flags)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_chgMediaStatus() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_cleanDrive(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq) {

	PTRACE(1, "jni:Media_cleanDrive() entry");
	if (-1 == rb_clean_drive(CTX, (int)eq)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_cleanDrive() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_importCartridge(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jobject impOpts) {

	import_option_t *impopt;
	int res;

	PTRACE(1, "jni:Media_importCartridge() entry");
	impopt = ImportOpts2impopt(env, impOpts);
	res = rb_import(CTX, (int)libEq, impopt);
	free(impopt);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_importCartridge() done");
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_importCartridges(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jstring startVSN,
    jstring endVSN, jobject impOpts) {

	jboolean isCopy, isCopy2;
	char *start_vsn, *end_vsn;
	int res;
	int imported = -1; // # of VSN-s successfully imported
	import_option_t *impopt;

	PTRACE(1, "jni:Media_importCartridges() entry");
	start_vsn = GET_STR(startVSN, isCopy);
	end_vsn   = GET_STR(endVSN,   isCopy2);
	impopt = (import_option_t *)ImportOpts2impopt(env, impOpts);
	res = import_all(CTX,
	    start_vsn, end_vsn, &imported, (equ_t)libEq, impopt);
	REL_STR(startVSN, start_vsn, isCopy);
	REL_STR(endVSN, end_vsn, isCopy2);
	free(impopt);
	if (-1 == res) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:Media_importCartridges() done");
	return (imported);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_exportCartridge(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jint slot,
    jboolean stkOneStep) {

	PTRACE(1, "jni:Media_exportCartridge() entry");
	if (-1 == rb_export_from_eq(CTX, (equ_t)libEq, (int)slot,
	    CBOOL(stkOneStep))) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_exportCartridge() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_moveCartridge(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq,
    jint srcSlot, jint destSlot) {

	PTRACE(1, "jni:Media_moveCartridge() entry");
	if (-1 == rb_move_from_eq(CTX, (equ_t)libEq, (int)srcSlot,
	    (int)destSlot)) {
		ThrowEx(env);
	}
	PTRACE(1, "jni:Media_moveCartridge() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_tapeLabel(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jint slot, jint part,
    jstring newVSN, jstring oldVSN, jlong blkSzKb,
    jboolean wait, jboolean erase) {

	jboolean isCopy, isCopy2;
	char *oldvsn, *newvsn;
	int res;

	PTRACE(1, "jni:Media_tapeLabel() entry");
	oldvsn = GET_STR(oldVSN, isCopy);
	newvsn = GET_STR(newVSN, isCopy2);
	res = rb_tplabel_from_eq(CTX, (equ_t)eq, (int)slot, (int)part,
	    newvsn, oldvsn, (uint_t)blkSzKb, CBOOL(wait), CBOOL(erase));
	REL_STR(oldVSN, oldvsn, isCopy);
	REL_STR(newVSN, newvsn, isCopy2);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_tapeLabel() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_load(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jint slot, jint part,
    jboolean wait) {

	PTRACE(1, "jni:Media_load() entry");
	if (-1 == rb_load_from_eq(CTX, (equ_t)libEq, (int)slot, (int)part,
	    CBOOL(wait))) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_load() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_unload(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jboolean wait) {

	PTRACE(1, "jni:Media_unload() entry");
	if (-1 == rb_unload(CTX, (equ_t)eq, CBOOL(wait))) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_unload() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_reserve(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jint slot, jint part,
    jobject resObj) {

	int res;
	reserve_option_t *resrv;

	PTRACE(1, "jni:Media_reserve() entry");
	resrv = ReservInfo2reserve(env, resObj);
	res = rb_reserve_from_eq(CTX, (equ_t)eq, (int)slot,
	    (int)part, resrv);
	free(resrv);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_reserve() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_unreserve(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jint slot, jint part) {

	PTRACE(1, "jni:Media_unreserve() entry");
	if (-1 == rb_unreserve_from_eq(CTX, (equ_t)eq, (int)slot,
	    (int)part)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_unreserve() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_changeDriveState(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint eq, jint newState) {

	PTRACE(1, "jni:Media_chageDriveState() entry");
	if (-1 == change_state(CTX, (equ_t)eq, (dstate_t)newState)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_changeDriveState() done");

}


/* return the loaded drived or NULL if VSN not loaded */
JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_isVSNLoaded(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jstring vsn) {

	jobject newObj;
	jboolean isCopy;
	char *cstr;
	drive_t *loadeddrv = NULL;
	int res;

	PTRACE(1, "jni:Media_isVSNLoaded() entry");
	cstr = GET_STR(vsn, isCopy);
	res = is_vsn_loaded(CTX, cstr, &loadeddrv);
	REL_STR(vsn, cstr, isCopy);
	if (-1 == res) {
		free_drive(loadeddrv);
		ThrowEx(env);
		return (NULL);
	}
	newObj = drive2DriveDev(env, loadeddrv);
	if (loadeddrv->base_info.eq <= 0) // no VSN loaded
		newObj = NULL;
	free_drive(loadeddrv);
	PTRACE(1, "jni:Media_isVNSLoaded() done");
	return (newObj);
}


/* return library capacity in KBytes */
JNIEXPORT jlong JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibCapacity(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq) {

	fsize_t capacity; // bytes

	PTRACE(1, "jni:Media_getLibCapacity() entry");
	if (-1 == get_total_capacity_of_library(CTX, (equ_t)libEq,
	    &capacity)) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:Media_getLibCapacity() done");
	return ((jlong) (capacity / 1024));
}


/* return library free space in KBytes */
JNIEXPORT jlong JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getLibFreeSpace(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq) {

	fsize_t freespace; // bytes

	PTRACE(1, "jni:Media_getLibFreeSpace() entry");
	if (-1 == get_free_space_of_library(CTX, (equ_t)libEq,
	    &freespace)) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:Media_getLibFreeSpace() done");
	return ((jlong) (freespace / 1024));
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getCatEntryForSlot(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx,
    jint libEq, jint slot, jint partition) {

	jobject newObj;
	struct CatalogEntry *catentry;

	PTRACE(1, "jni:Media_getCatEntryForSlot()");
	if (-1 == get_catalog_entry_from_lib(CTX,
	    (int)libEq, (int)slot, (int)partition, &catentry)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = catentry2CatEntry(env, catentry);
	free(catentry);
	PTRACE(1, "jni:Media_getCatEntrysForSlot() done");
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getCatEntriesForRegexp(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx,
    jstring vsnRegexp, jint start, jint size, jshort sortMet, jboolean asc) {

	jobjectArray newArr;
	jboolean isCopy;
	char *cstr;
	sqm_lst_t *catentrylst;
	int res;

	PTRACE(1, "jni:Media_getCatEntriesForRegexp() entry");
	cstr = GET_STR(vsnRegexp, isCopy);
	res = get_vsn_list(CTX, cstr, (int)start, (int)size,
	    (vsn_sort_key_t)sortMet, CBOOL(asc), &catentrylst);
	REL_STR(vsnRegexp, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, catentrylst, BASEPKG"/media/CatEntry",
	    catentry2CatEntry);
	lst_free_deep(catentrylst);
	PTRACE(1, "jni:Media_getCatEntriesForRegexp() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getCatEntriesForVSN(
	JNIEnv *env, jclass cls /* ARGSUSED */,
	jobject ctx, jstring vsnName) {

	jobjectArray newArr;
	jboolean isCopy;
	char *cstr;
	sqm_lst_t *catentrylst;
	int res;

	PTRACE(1, "jni:Media_getCatEntriesForVSN() entry");
	cstr = GET_STR(vsnName, isCopy);
	res = get_catalog_entry(CTX, cstr, &catentrylst);
	REL_STR(vsnName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, catentrylst, BASEPKG"/media/CatEntry",
	    catentry2CatEntry);
	lst_free_deep(catentrylst);
	PTRACE(1, "jni:Media_getCatEntriesForVSN() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getCatEntriesForLib(
    JNIEnv *env, jclass cls /* ARGSUSED */,
    jobject ctx, jint libEq, jint startSlot, jint endSlot,
    jshort sortMet, jboolean asc) {

	jobjectArray newArr;
	sqm_lst_t *catentrylst;

	PTRACE(1, "jni:Media_getCatEntriesForLib() entry");
	if (-1 == get_catalog_entries(CTX, (equ_t)libEq, (int)startSlot,
	    (int)endSlot, (vsn_sort_key_t)sortMet, CBOOL(asc), &catentrylst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, catentrylst, BASEPKG"/media/CatEntry",
	    catentry2CatEntry);
	lst_free_deep(catentrylst);
	PTRACE(1, "jni:Media_getCatEntriesForLib() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getAllCatEntriesForLib(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jint libEq,
    jint start, jint size, short sortMet, jboolean asc) {

	jobjectArray newArr;
	sqm_lst_t *catentrylst;

	PTRACE(1, "jni:Media_getAllCatEntriesForLib() entry");
	if (-1 == get_all_catalog_entries(CTX, (equ_t)libEq, (int)start,
	    (int)size, (vsn_sort_key_t)sortMet, CBOOL(asc), &catentrylst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, catentrylst, BASEPKG"/media/CatEntry",
	    catentry2CatEntry);
	lst_free_deep(catentrylst);
	PTRACE(1, "jni:Media_getAllCatEntriesForLib() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getAvailMediaTypes(
	JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *mtypelst;

	PTRACE(1, "jni:Media_getAvailMediaTypes() entry");
	if (-1 == get_all_available_media_type(CTX, &mtypelst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, mtypelst, "java/lang/String", charr2String);
	lst_free_deep(mtypelst);
	PTRACE(1, "jni:Media_getAvailMediaTypes() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_LabelJob_getDrives(
	JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *drvlst;

	PTRACE(1, "jni:LabelJob_getDrives() entry");
	if (-1 == get_tape_label_running_list(CTX, &drvlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, drvlst, BASEPKG"/media/DriveDev",
	    drive2DriveDev);
	free_list_of_drives(drvlst);
	PTRACE(1, "jni:LabelJob_getDrives() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_MountJob_getAll(
	JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *pendloadlst;

	PTRACE(1, "jni:MountJob_getAll() entry");
	if (-1 == get_pending_load_info(CTX, &pendloadlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, pendloadlst, BASEPKG"/media/MountJob",
	    pendload2MountJob);
	free_list_of_pending_load_info(pendloadlst);
	PTRACE(1, "jni:MountJob_getAll() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_MountJob_cancel(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jstring vsn, jint idx) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:MountJob_cancel() entry");
	cstr = GET_STR(vsn, isCopy);
	res = clear_load_request(CTX, cstr, (int)idx);
	REL_STR(vsn, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:MountJob_cancel() done");
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_discoverStk(
    JNIEnv *env,
    jclass unused /* ARGSUSED */, jobject ctx, jobjectArray stkConn) {

	jclass cls;
	jmethodID mid;
	jobjectArray newArr;
	sqm_lst_t *liblst, *stk_host_list;
	int res;
	node_t *node;

	PTRACE(1, "jni:Media_discoverStk() entry");
	stk_host_list = jarray2lst(env, stkConn,
	    BASEPKG"/media/StkClntConn", StkClntConn2stkhostinfo);
	node = stk_host_list->head;
	while (node != NULL) {
		stk_host_info_t *stkhost = (stk_host_info_t *)node->data;
		PTRACE(3, "stkhost: %s %s",
			stkhost->hostname, stkhost->portnum);
		node = node->next;
	}
	res = discover_stk(CTX, stk_host_list, &liblst);
	lst_free_deep(stk_host_list);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	// TRACE("jni:API call discover_stk() done");
	if (NULL == liblst) {
		PTRACE(1, "jni:liblst is NULL");
	}
	PTRACE(3, "Discovered %d libraries", liblst->length);
	newArr = lst2jarray(env, liblst, BASEPKG"/media/LibDev", lib2LibDev);
	free_list_of_libraries(liblst);
	PTRACE(1, "jni:Media_discoverStk() done");
	return (newArr);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_addLibraries(
    JNIEnv *env,
    jclass unused /* ARGSUSED */, jobject ctx, jobjectArray libs) {


	jclass cls;
	jmethodID mid;
	sqm_lst_t *liblst;
	int res;

	PTRACE(1, "jni:Media_AddLibraries() entry");
	liblst = jarray2lst(env, libs,
	    BASEPKG"/media/LibDev", LibDev2lib);

	res = add_list_libraries(CTX, liblst);
	lst_free_deep(liblst);
	if (-1 == res) {
		ThrowEx(env);
	}
	TRACE("jni:Media_AddLibraries() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_importStkCartridges(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jint libEq, jobject impOpts,
    jobjectArray vsns) {

	import_option_t *impopt;
	sqm_lst_t *vsnlst;
	int res;

	PTRACE(1, "jni:Media_importStkCartridges() entry");
	impopt = ImportOpts2impopt(env, impOpts);
	vsnlst = jarray2lst(env, vsns, "java/lang/String", String2charr);

	res = import_stk_vsns(CTX, (int)libEq, impopt, vsnlst);
	free(impopt);
	lst_free_deep(vsnlst);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Media_importStkCartridges() done");
}


/*
 * Temporary conversions of stk_lsm_t list to just a intArray with just the
 * lsm number
 * The consumer (GUI) does not require the other fields from the stk_lsm_t
 */

jintArray
lsmlst2jintArray(JNIEnv *env,
    sqm_lst_t *lsmlst) {

	jintArray jarr;
	node_t *node = NULL;
	int idx = 0;
	jint *p = NULL;

	if (NULL == lsmlst) {
		PTRACE(1, "jni:lsmlst2jintArray(null). return.");
		return (NULL);
	} else {
		PTRACE(2, "jni:lsmlst2jintArray(lst[%d])", lsmlst->length);
		if (lsmlst->length > 0) {
			p = (jint *) malloc(lsmlst->length * sizeof (jint));
		}
	}

	jarr = (*env)->NewIntArray(env, lsmlst->length);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create jintArray");
		return (NULL);
	}

	node = lsmlst->head;
	while (node) {
		// TRACE("jni:lsmlst2jintArray:building node %d", idx);
		stk_lsm_t *stk_lsm = (stk_lsm_t *)node->data;
		p[idx] = stk_lsm->lsm_num;
		PTRACE(2, "lsm: %d\n", p[idx]);
		node = node->next;
		idx++;
	}
	(*env)->SetIntArrayRegion(env, jarr, 0, lsmlst->length, p);

	if (p != NULL) {
		free(p);
	}
	PTRACE(2, "jni:lsmlst2jintArray() done");
	return (jarr);
}


jintArray
panellst2jintArray(JNIEnv *env,
    sqm_lst_t *panellst) {

	jintArray jarr;
	node_t *node = NULL;
	int idx = 0;
	jint *p = NULL;

	if (NULL == panellst) {
		PTRACE(1, "jni:panellst2jintArray(null). return.");
		return (NULL);
	} else {
		PTRACE(2, "jni:panellst2jintArray(lst[%d])", panellst->length);
		if (panellst->length > 0) {
			p = (jint *) malloc(panellst->length * sizeof (jint));
		}
	}

	jarr = (*env)->NewIntArray(env, panellst->length);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create jintArray");
		return (NULL);
	}

	node = panellst->head;
	while (node) {
		// TRACE("jni:panellst2jintArray:building node %d", idx);
		stk_panel_t *stk_panel = (stk_panel_t *)node->data;
		p[idx] = stk_panel->panel_num;
		PTRACE(2, "panel: %d\n", p[idx]);
		node = node->next;
		idx++;
	}
	(*env)->SetIntArrayRegion(env, jarr, 0, panellst->length, p);

	if (p != NULL) {
		free(p);
	}
	PTRACE(2, "jni:panellst2jintArray() done");
	return (jarr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getVSNsForStkLib(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jobject stkConn,
    jstring filter) {

	stk_host_info_t *stkhostinfo;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(filter, isCopy);
	jobjectArray newArr;
	sqm_lst_t *vsnlst;
	int res;

	PTRACE(1, "jni:Media_getVSNsForStkLib() entry");
	stkhostinfo = StkClntConn2stkhostinfo(env, stkConn);
	res = get_stk_filter_volume_list(CTX, stkhostinfo, cstr, &vsnlst);
	free(stkhostinfo);
	REL_STR(filter, cstr, isCopy);

	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, vsnlst,
		BASEPKG"/media/StkVSN", stkvol2StkVSN);

	lst_free(vsnlst);

	PTRACE(1, "jni:Media_getVSNsForStkLib() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getVSNNamesForStkLib(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jstring mediaType) {

	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(mediaType, isCopy);
	jobjectArray newArr;
	sqm_lst_t *vsnlst;
	int res;

	PTRACE(1, "jni:Media_getVSNNamesForStkLib() entry");
	res = get_stk_vsn_names(CTX, cstr, &vsnlst);
	REL_STR(mediaType, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, vsnlst, "java/lang/String", charr2String);

	lst_free_deep(vsnlst);

	PTRACE(1, "jni:Media_getVSNNamesForStkLib() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getStkScratchPools(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jstring mediaType,
    jobject stkConn) {

	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(mediaType, isCopy);
	stk_host_info_t *stkhostinfo = NULL;
	stk_phyconf_info_t *stkphyconf = NULL;
	int res;
	jobjectArray newArr;

	PTRACE(1, "jni:Media_getPhyConfForStkLib() entry");
	stkhostinfo = StkClntConn2stkhostinfo(env, stkConn);
	res = get_stk_phyconf_info(CTX, stkhostinfo, cstr, &stkphyconf);
	REL_STR(mediaType, cstr, isCopy);
	free(stkhostinfo);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}

	/*
	 * From 5.0, only the stk scratch pool is to be considered
	 */
	newArr = lst2jarray(env, stkphyconf->stk_pool_list,
	    BASEPKG"/media/StkPool", stkpool2StkPool);

	free(stkphyconf);

	PTRACE(1, "jni:Media_getScratchPools() done");
	return (newArr);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_changeStkDriveShareStatus(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jint libEq,
    jint driveEq, jboolean shared) {

	int res;

	PTRACE(1, "jni:Media_changeStkDriveShareStatus() entry");
	if (-1 == modify_stkdrive_share_status(CTX, (equ_t)libEq,
		(equ_t)driveEq, CBOOL(shared))) {
		ThrowEx(env);
		return;
	}

	PTRACE(1, "jni:Media_changeStkDriveShareStatus() done");
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_media_Media_getVSNs(
    JNIEnv *env, jclass cls /* ARGSUSED */, jobject ctx, jint libEq,
    jint flags) {

	jobjectArray newArr;
	sqm_lst_t *vsnlst;

	PTRACE(1, "jni:Media_getVSNs() entry");
	if (-1 == get_vsns(
			CTX,
			(equ_t)libEq,
			(uint32_t)flags,
			&vsnlst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(
		env, vsnlst, "java/lang/String", charr2String);

	lst_free_deep(vsnlst);

	PTRACE(1, "jni:Media_getVSNs() done");
	return (newArr);
}
