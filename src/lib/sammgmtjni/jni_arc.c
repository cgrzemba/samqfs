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

#pragma ident	"$Revision: 1.37 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/directives.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/archive_sets.h"
#include "mgmt/log.h"
/* non-API header files */
#include "aml/archreq.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_arc_Archiver.h"
#include "com_sun_netstorage_samqfs_mgmt_arc_job_ArchReq.h"
#include "com_sun_netstorage_samqfs_mgmt_arc_job_ArFindJob.h"
#include "jni_util.h"


static const char KEYLEN = 16;

/* Defined in jni_vsn.c */
jobject map2VSNMap(JNIEnv *env, void *v_map);
void * VSNMap2map(JNIEnv *env, jobject mapObj);

/* used for debug */
void
displayCopy(ar_set_copy_cfg_t *cp) {
	if (NULL == cp) {
		printf("NULL\n");
		return;
	}
	printf("%d,%d %c,%c,%u,0x%x\n",
		cp->ar_copy.copy_seq, cp->ar_copy.ar_age,
		cp->release ? 'T' : 'F',
		cp->norelease ? 'T' : 'F', cp->change_flag);
}

/* used for debug */
void
displayCriteriaLst(sqm_lst_t *lst) {

	ar_set_criteria_t *crit;
	node_t *n;
	int copy;

	if (lst == NULL) {
		printf("criteria list is NULL");
		return;
	}
	printf("Criteria[%d] info:\n", lst->length);
	n = lst->head;
	while (NULL != n) {
		copy = 0;
		crit = (ar_set_criteria_t *)n->data;
		printf("%s,%s,%s,%llu,%llu,%s,%s,%s,%c,%c,%d,%x,%x,%x,%x,%x\n",
		    Str(crit->fs_name), Str(crit->set_name),
		    Str(crit->path),
		    crit->minsize, crit->maxsize,
		    Str(crit->name), Str(crit->user), Str(crit->group),
		    crit->release, crit->stage, crit->num_copies,
		    crit->arch_copy[0], crit->arch_copy[1],
		    crit->arch_copy[2], crit->arch_copy[3],
		    crit->change_flag);
		while (copy < MAX_COPY) {
			printf(" copy%d: \n", copy + 1);
			displayCopy(crit->arch_copy[copy++]);
		}
		n = n->next;
	}
}


/* C structure <-> Java class conversion functions */

extern jobject recparams2RecyclerParams(JNIEnv *, void *);
extern void * RecyclerParams2recparams(JNIEnv *, jobject);


jbyteArray
convertKeyC2J(JNIEnv *env, void *key) {

	jbyteArray byteArr  = (*env)->NewByteArray(env, KEYLEN);

	(*env)->SetByteArrayRegion(env, byteArr, 0, KEYLEN, key);
	return (byteArr);
}


void
convertKeyJ2C(JNIEnv *env, jbyteArray byteArr, struct_key_t *key) {

	if (NULL == byteArr) {
		PTRACE(2, "jni:Jkey not set (NULL)");
		memset(key, 0, KEYLEN);
	} else
		(*env)->GetByteArrayRegion(env,
			byteArr, 0, KEYLEN, (jbyte *)key);
}


jobject
copy2Copy(JNIEnv *env, void *v_arsetcopy) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_set_copy_cfg_t *copy = (ar_set_copy_cfg_t *)v_arsetcopy;

	PTRACE(2, "jni:copy2Copy() entry");
	if (NULL == v_arsetcopy) {
		PTRACE(2, "jni:copy2Copy() done (NULL)");
		return (NULL);
	}
	cls = (*env)->FindClass(env, BASEPKG"/arc/Copy");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(IJJZZJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)(copy->ar_copy.copy_seq),
	    (jlong)(copy->ar_copy.ar_age),
	    (copy->un_ar_age == uint_reset) ? -1 : (jlong)(copy->un_ar_age),
	    JBOOL(copy->release),
	    JBOOL(copy->norelease),
	    (jlong)(copy->change_flag));
	PTRACE(2, "jni:copy2Copy() done");
	return (newObj);
}


void *
Copy2copy(JNIEnv *env, jobject cpObj) {

	jclass cls;
	ar_set_copy_cfg_t *copy;

	PTRACE(2, "jni:Copy2copy() entry");
	if (NULL == cpObj) {
		PTRACE(2, "jni:Copy2copy() done (NULL)");
		return (NULL);
	}
	copy = (ar_set_copy_cfg_t *)malloc(sizeof (ar_set_copy_cfg_t));

	cls = (*env)->GetObjectClass(env, cpObj);

	copy->ar_copy.copy_seq = (int)getJIntFld(env, cls, cpObj, "copyNum");
	copy->ar_copy.ar_age = (uint_t)getJLongFld(env, cls, cpObj, "age");
	copy->un_ar_age = (uint_t)getJLongFld(env, cls, cpObj, "unage");
	copy->release = getBoolFld(env, cls, cpObj, "release");
	copy->norelease = getBoolFld(env, cls, cpObj, "norelease");
	copy->change_flag = (uint32_t)getJLongFld(env, cls, cpObj, "chgFlags");
	PTRACE(2, "jni:Copy2copy() done");
	return (copy);
}


jobject
crit2Criteria(JNIEnv *env, void *v_arcrit) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_set_criteria_t *crit = (ar_set_criteria_t *)v_arcrit;

	PTRACE(2, "jni:crit2Criteria() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/Criteria");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;"
	    "Ljava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
	    "CCI[L"BASEPKG"/arc/Copy;S[BZLjava/lang/String;"
	    "JII)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(crit->fs_name),
	    JSTRING(crit->set_name),
	    JSTRING(crit->path),
	    JSTRING(crit->name), /* regexp */
	    (crit->minsize == fsize_reset) ? NULL :
	    ull2jstr(env, crit->minsize),
	    (crit->maxsize == fsize_reset) ? NULL :
	    ull2jstr(env, crit->maxsize),
	    JSTRING(crit->user),
	    JSTRING(crit->group),
	    (jchar)(crit->release),
	    (jchar)(crit->stage),
	    (jint)crit->access,
	    arrOfPtrs2jarray(env, (void**)crit->arch_copy, MAX_COPY,
	    BASEPKG"/arc/Copy", copy2Copy),
	    (jshort)(crit->num_copies),
	    convertKeyC2J(env, crit->key),
	    JBOOL(crit->nftv),
	    JSTRING(crit->after),
	    (jlong)(crit->change_flag),
	    (jint)(crit->attr_flags),
	    (jint)(crit->partial_size));

	PTRACE(2, "jni:crit2Criteria() done");
	return (newObj);
}


void *
Criteria2crit(JNIEnv *env, jobject critObj) {

	jclass cls;
	ar_set_copy_cfg_t **copiesin;
	ar_set_criteria_t *crit = NULL;

	PTRACE(2, "jni:Criteria2crit() entry");

	if (critObj == NULL)
		return (NULL);

	crit = (ar_set_criteria_t *)malloc(sizeof (ar_set_criteria_t));

	memset(crit, 0, sizeof (ar_set_criteria_t));

	PTRACE(2, "jni:Criteria2crit() entry");
	cls = (*env)->GetObjectClass(env, critObj);

	getStrFld(env, cls, critObj, "fsysName", crit->fs_name);
	getStrFld(env, cls, critObj, "setName", crit->set_name);
	PTRACE(3, "jni:   %s", crit->set_name);
	getStrFld(env, cls, critObj, "rootDir", crit->path);
	getStrFld(env, cls, critObj, "regExp", crit->name);
	crit->minsize = jstrFld2ull(env, cls, critObj, "minSize");
	crit->maxsize = jstrFld2ull(env, cls, critObj, "maxSize");
	getStrFld(env, cls, critObj, "user", crit->user);
	getStrFld(env, cls, critObj, "group", crit->group);
	crit->release = (char)getJCharFld(env, cls, critObj, "releaseAttr");
	crit->stage = (char)getJCharFld(env, cls, critObj, "stageAttr");
	crit->access = (int)getJIntFld(env, cls, critObj, "accessAge");
	copiesin = (ar_set_copy_cfg_t **)jarray2arrOfPtrs(env,
		getJArrFld(env, cls, critObj, "copies",
		"[L"BASEPKG"/arc/Copy;"), BASEPKG"/arc/Copy", Copy2copy);
	if (NULL != copiesin)
		memcpy(crit->arch_copy, copiesin, MAX_COPY * sizeof (void *));
	convertKeyJ2C(env, getObjFld(env, cls, critObj, "key", "[B"),
		&crit->key);
	crit->change_flag =
		(uint32_t)getJLongFld(env, cls, critObj, "chgFlags");
	crit->num_copies = -1; // this should be ignored by the API
	crit->nftv = getBoolFld(env, cls, critObj, "nftv");
	getStrFld(env, cls, critObj, "after", crit->after);
	crit->attr_flags =
	    (int32_t)getJIntFld(env, cls, critObj, "attrFlags");
	crit->partial_size =
	    (int32_t)getJIntFld(env, cls, critObj, "partialSize");

	PTRACE(2, "jni:Criteria2crit() done");
	return (crit);
}



jobject
bufdir2BufDirective(JNIEnv *env, void *v_bufdir) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	buffer_directive_t *bd = (buffer_directive_t *)v_bufdir;

	PTRACE(2, "jni:bufdir2BufDirective() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/BufDirective");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;Ljava/lang/String;ZJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(bd->media_type),
	    ull2jstr(env, bd->size),
	    JBOOL(bd->lock),
	    (jlong)(bd->change_flag));
	PTRACE(2, "jni:bufdir2BufDirective() done");
	return (newObj);
}


void *
BufDirective2bufdir(JNIEnv *env, jobject bufdObj) {

	jclass cls;
	buffer_directive_t *bd =
		(buffer_directive_t *)malloc(sizeof (buffer_directive_t));

	PTRACE(2, "jni:BufDirective2bufdir() entry");
	cls = (*env)->GetObjectClass(env, bufdObj);

	getStrFld(env, cls, bufdObj, "mediaType", bd->media_type);
	bd->size = jstrFld2ull(env, cls, bufdObj, "size");
	bd->lock = getBoolFld(env, cls, bufdObj, "lock");
	bd->change_flag =
	    (uint32_t)getJLongFld(env, cls, bufdObj, "chgFlags");
	PTRACE(2, "jni:BufDirective2bufdir() done");
	return (bd);
}


jobject
drvdir2DrvDirective(JNIEnv *env, void *v_drvdir) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	drive_directive_t *dd = (drive_directive_t *)v_drvdir;

	PTRACE(2, "jni:drvdir2DrvDirective() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/DrvDirective");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;IJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(dd->auto_lib),
	    (jint)dd->count,
	    (jlong)(dd->change_flag));
	PTRACE(2, "jni:drvdir2DrvDirective() done");
	return (newObj);
}


void *
DrvDirective2drvdir(JNIEnv *env, jobject drvdObj) {

	jclass cls;
	drive_directive_t *dd =
		(drive_directive_t *)malloc(sizeof (drive_directive_t));

	PTRACE(2, "jni:DrvDirective2drvdir() entry");
	cls = (*env)->GetObjectClass(env, drvdObj);

	getStrFld(env, cls, drvdObj, "autoLib", dd->auto_lib);
	dd->count = (int)getJIntFld(env, cls, drvdObj, "count");
	dd->change_flag =
	    (uint32_t)getJLongFld(env, cls, drvdObj, "chgFlags");
	PTRACE(2, "jni:DrvDirective2drvdir() done");
	return (dd);
}


jobject
arglobd2ArGlobalDirective(JNIEnv *env, void *v_arglobd) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_global_directive_t *argd = (ar_global_directive_t *)v_arglobd;

	PTRACE(2, "jni:arglobd2ArGlobalDirective entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/ArGlobalDirective");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "([L"BASEPKG"/arc/BufDirective;[L"BASEPKG"/arc/BufDirective;"
	    "[L"BASEPKG"/arc/BufDirective;[L"BASEPKG"/arc/DrvDirective;"
	    "JSLjava/lang/String;Ljava/lang/String;ZZ"
	    "[L"BASEPKG"/arc/Criteria;JI[Ljava/lang/String;JI)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    lst2jarray(env, argd->ar_bufs,
	    BASEPKG"/arc/BufDirective", bufdir2BufDirective),
	    lst2jarray(env, argd->ar_max,
	    BASEPKG"/arc/BufDirective", bufdir2BufDirective),
	    lst2jarray(env, argd->ar_overflow_lst,
	    BASEPKG"/arc/BufDirective", bufdir2BufDirective),
	    lst2jarray(env, argd->ar_drives,
	    BASEPKG"/arc/DrvDirective", drvdir2DrvDirective),
	    (jlong)(argd->ar_interval),
	    (jshort)argd->scan_method,
	    JSTRING(argd->log_path),
	    JSTRING(argd->notify_script),
	    JBOOL(argd->wait),
	    JBOOL(argd->archivemeta),
	    lst2jarray(env, argd->ar_set_lst,
		BASEPKG"/arc/Criteria", crit2Criteria),
	    (jlong)(argd->change_flag),
	    (jint)(argd->options),
	    lst2jarray(env, argd->timeouts, "java/lang/String",
		charr2String),
	    (jlong)(argd->bg_interval),
	    (jint)(argd->bg_time));

	PTRACE(2, "jni:arglobd2ArGlobalDirective() done");
	return (newObj);
}


void *
ArGlobalDirective2arglobd(JNIEnv *env, jobject argdObj) {

	jclass cls;
	ar_global_directive_t *argd =
		(ar_global_directive_t *)malloc(sizeof (ar_global_directive_t));

	PTRACE(2, "jni:ArGlobalDirective2arglobd() entry");
	cls = (*env)->GetObjectClass(env, argdObj);

	argd->ar_bufs = jarray2lst(env,
		getJArrFld(env, cls,
		argdObj, "bufDirs", "[L"BASEPKG"/arc/BufDirective;"),
		BASEPKG"/arc/BufDirective",
		BufDirective2bufdir);
	argd->ar_max = jarray2lst(env,
		getJArrFld(env, cls,
		argdObj, "maxDirs", "[L"BASEPKG"/arc/BufDirective;"),
		BASEPKG"/arc/BufDirective",
		BufDirective2bufdir);
	argd->ar_overflow_lst = jarray2lst(env,
		getJArrFld(env, cls,
		argdObj, "overflowDirs", "[L"BASEPKG"/arc/BufDirective;"),
		BASEPKG"/arc/BufDirective",
		BufDirective2bufdir);
	argd->ar_drives =  jarray2lst(env,
		getJArrFld(env, cls,
		argdObj, "drvDirs", "[L"BASEPKG"/arc/DrvDirective;"),
		BASEPKG"/arc/DriveDirective",
		DrvDirective2drvdir);
	argd->ar_interval = (uint_t)getJLongFld(env, cls, argdObj, "interval");
	argd->scan_method =
		(ExamMethod_t)getJShortFld(env, cls, argdObj, "examMethod");
	getStrFld(env, cls, argdObj, "logFile", argd->log_path);
	getStrFld(env, cls, argdObj, "notifyScript", argd->notify_script);
	argd->wait = getBoolFld(env, cls, argdObj, "wait");
	argd->archivemeta = getBoolFld(env, cls, argdObj, "arcMeta");
	argd->ar_set_lst = jarray2lst(env,
		getJArrFld(env, cls,
		argdObj, "crit", "[L"BASEPKG"/arc/Criteria;"),
		BASEPKG"/arc/Criteria",
		Criteria2crit);
	argd->change_flag =
	    (uint32_t)getJLongFld(env, cls, argdObj, "chgFlags");
	argd->options = (int32_t)getJIntFld(env, cls, argdObj, "options");
	argd->timeouts = jarray2lst(env,
		getJArrFld(env, cls, argdObj,
		    "timeouts", "[Ljava/lang/String;"),
		    "java/lang/String",
		    String2charr);
	argd->bg_interval = (int32_t)getJLongFld(env, cls,
	    argdObj, "backgroundInterval");
	argd->bg_time = (int)getJIntFld(env, cls, argdObj, "backgroundTime");
	PTRACE(2, "jni:ArGlobalDirective2arglobd() done");
	return (argd);
}


jobject
arfsd2ArFSDirective(JNIEnv *env, void *v_arfsd) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_fs_directive_t *arfsd = (ar_fs_directive_t *)v_arfsd;

	PTRACE(2, "jni:arfsd2ArFSDirective entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/ArFSDirective");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;[L"BASEPKG"/arc/Criteria;Ljava/lang/String;"
		"SJZZI[L"BASEPKG"/arc/Copy;JIJI)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(arfsd->fs_name),
	    lst2jarray(env, arfsd->ar_set_criteria,
	    BASEPKG"/arc/Criteria", crit2Criteria),
	    JSTRING(arfsd->log_path),
	    (jshort)arfsd->scan_method,
	    (jlong)arfsd->fs_interval,
	    JBOOL(arfsd->wait),
	    JBOOL(arfsd->archivemeta),
	    (jint)arfsd->num_copies,
	    arrOfPtrs2jarray(env, (void**)arfsd->fs_copy, /* metadata copies */
		MAX_COPY, BASEPKG"/arc/Copy", copy2Copy),
	    (jlong)(arfsd->change_flag),
	    (jint)(arfsd->options),
	    (jlong)(arfsd->bg_interval),
	    (jint)(arfsd->bg_time));
	PTRACE(2, "jni:arfsd2ArFSDirective() done");
	return (newObj);
}


void *
ArFSDirective2arfsd(JNIEnv *env, jobject arfsdObj) {

	jclass cls;
	ar_fs_directive_t *arfsd =
		(ar_fs_directive_t *)malloc(sizeof (ar_fs_directive_t));
	ar_set_copy_cfg_t **metacopies;

	PTRACE(2, "jni:ArFSDirective2arfsd() entry");
	cls = (*env)->GetObjectClass(env, arfsdObj);

	getStrFld(env, cls, arfsdObj, "fsName", arfsd->fs_name);
	arfsd->ar_set_criteria = jarray2lst(env,
		getJArrFld(env, cls,
		arfsdObj, "crit", "[L"BASEPKG"/arc/Criteria;"),
		BASEPKG"/arc/Criteria",
		Criteria2crit);
	getStrFld(env, cls, arfsdObj, "logPath", arfsd->log_path);
	arfsd->fs_interval =
		(uint_t)getJLongFld(env, cls, arfsdObj, "interval");
	arfsd->scan_method =
		(ExamMethod_t)getJShortFld(env, cls, arfsdObj, "examMethod");
	arfsd->wait = getBoolFld(env, cls, arfsdObj, "wait");
	arfsd->archivemeta = getBoolFld(env, cls, arfsdObj, "arcMeta");
	arfsd->num_copies = (int)getJIntFld(env, cls, arfsdObj, "numCopies");

	metacopies = (ar_set_copy_cfg_t **)jarray2arrOfPtrs(env,
		getJArrFld(env, cls,
		arfsdObj, "metadataCopies", "[L"BASEPKG"/arc/Copy;"),
		BASEPKG"/arc/Copy",
		Copy2copy);
	if (NULL != metacopies)
		memcpy(arfsd->fs_copy, metacopies, MAX_COPY * sizeof (void *));

	arfsd->change_flag =
		(uint32_t)getJLongFld(env, cls, arfsdObj, "chgFlags");

	arfsd->options = (int32_t)getJIntFld(env, cls, arfsdObj, "options");

	arfsd->bg_interval = (int32_t)getJLongFld(env, cls,
	    arfsdObj, "backgroundInterval");

	arfsd->bg_time = (int)getJIntFld(env, cls, arfsdObj, "backgroundTime");

	PTRACE(2, "jni:ArFSDirective2arfsd() done");
	return (arfsd);
}


jobject
priority2ArPriority(JNIEnv *env, void *v_prio) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	priority_t *prio = (priority_t *)v_prio;

	PTRACE(2, "jni:priority2ArPriority() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/ArPriority");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;FI)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(prio->priority_name),
	    (jfloat)prio->value,
	    (jint)prio->change_flag);
	PTRACE(2, "jni:priority2ArPriority() done");

	return (newObj);
}


void *
ArPriority2priority(JNIEnv *env, jobject prioObj) {

	jclass cls;
	priority_t *prio =
		(priority_t *)malloc(sizeof (priority_t));

	PTRACE(2, "jni:ArPriority2priority() entry");
	cls = (*env)->GetObjectClass(env, prioObj);

	getStrFld(env, cls, prioObj, "name", prio->priority_name);
	prio->value = (float)getJFloatFld(env, cls, prioObj, "prio");
	prio->change_flag =
	    (uint32_t)getJIntFld(env, cls, prioObj, "chgFlags");
	PTRACE(2, "jni:ArPriority2priority() done");
	return (prio);
}


jobject
cparams2CopyParams(JNIEnv *env, void *v_cparams) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_set_copy_params_t *cp = (ar_set_copy_params_t *)v_cparams;

	PTRACE(2, "jni:cparams2CopyParams entry");

	if (cp == NULL) {
		PTRACE(2, "jni: cparams2CopyParams called with NULL v_cparams");
		return (NULL);
	}

	cls = (*env)->FindClass(env, BASEPKG"/arc/CopyParams");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IZLjava/lang/String;ILjava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;ZZ[L"BASEPKG"/arc/ArPriority;"
	    "ZIIIISJILjava/lang/String;Ljava/lang/String;"
	    "L"BASEPKG"/rec/RecyclerParams;IZZSJJJ)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(cp->ar_set_copy_name),
	    (jint)cp->bufsize,
	    JBOOL(cp->buflock),
	    ull2jstr(env, cp->archmax),
	    (jint)cp->drives,
	    ull2jstr(env, cp->drivemax),
	    ull2jstr(env, cp->drivemin),
	    JSTRING(cp->disk_volume),
	    JBOOL(cp->fillvsns),
	    JBOOL(cp->tapenonstop),
	    lst2jarray(env, cp->priority_lst,
	    BASEPKG"/arc/ArPriority", priority2ArPriority),
	    JBOOL(cp->unarchage),
	    (jint)cp->join,
	    (jint)cp->rsort,
	    (jint)cp->sort,
	    (jint)cp->offline_copy,
	    (jshort)cp->reserve,
	    uint2jlong(cp->startage),
	    (jint)cp->startcount,
	    ull2jstr(env, cp->startsize),
	    ull2jstr(env, cp->ovflmin),
	    recparams2RecyclerParams(env, &cp->recycle),
	    // DEBUG params
	    (jint)cp->simdelay,
	    JBOOL(cp->tstovfl),
	    JBOOL(cp->directio),
	    (jshort)(cp->rearch_stage_copy),
	    (jlong)(cp->change_flag),
	    (jlong)(cp->queue_time_limit),
	    (jlong)(cp->fillvsns_min));
	PTRACE(2, "jni:cparams2CopyParams() done");
	return (newObj);
}

void *
CopyParams2cparams(JNIEnv *env, jobject cpObj) {

	jclass cls;
	ar_set_copy_params_t *cp = NULL;
	void *temp = NULL;

	PTRACE(2, "jni:CopyParams2cparams() entry");
	if (cpObj == NULL)
		return (NULL);

	cp = (ar_set_copy_params_t *)malloc(sizeof (ar_set_copy_params_t));
	cls = (*env)->GetObjectClass(env, cpObj);

	getStrFld(env, cls, cpObj, "copyName", cp->ar_set_copy_name);
	cp->bufsize = (int)getJIntFld(env, cls, cpObj, "bufSize");
	cp->buflock = getBoolFld(env, cls, cpObj, "bufLocked");
	cp->archmax = (fsize_t)jstrFld2ull(env, cls, cpObj, "archMax");
	cp->drives = (int)getJIntFld(env, cls, cpObj, "drives");
	cp->drivemax = (fsize_t)jstrFld2ull(env, cls, cpObj, "drvMax");
	cp->drivemin = (fsize_t)jstrFld2ull(env, cls, cpObj, "drvMin");
	getStrFld(env, cls, cpObj, "dskVol", cp->disk_volume);
	cp->fillvsns = getBoolFld(env, cls, cpObj, "fillVSNs");
	cp->tapenonstop = getBoolFld(env, cls, cpObj, "tapeNonStop");

	cp->priority_lst = jarray2lst(env,
	    getJArrFld(env, cls,
	    cpObj, "arPrios", "[L"BASEPKG"/arc/ArPriority;"),
	    BASEPKG"/arc/ArPriority",
	    ArPriority2priority);

	cp->unarchage = getBoolFld(env, cls, cpObj, "unarchAge");
	cp->join = (join_method_t)getJIntFld(env, cls, cpObj, "join");
	cp->rsort = (sort_method_t)getJIntFld(env, cls, cpObj, "rsort");
	cp->sort = (sort_method_t)getJIntFld(env, cls, cpObj, "sort");
	cp->offline_copy = (offline_copy_method_t)
	    getJIntFld(env, cls, cpObj, "offlineCp");
	cp->reserve = (short)getJShortFld(env, cls, cpObj, "reserve");
	cp->startage = (uint_t)getJLongFld(env, cls, cpObj, "age");
	cp->startcount = (int)getJIntFld(env, cls, cpObj, "count");
	cp->startsize = (fsize_t)jstrFld2ull(env, cls, cpObj, "startSize");
	cp->ovflmin = (fsize_t)jstrFld2ull(env, cls, cpObj, "overflowMinSz");


	temp = RecyclerParams2recparams(env,
	    getObjFld(env, cls,
	    cpObj, "recParams", "L"BASEPKG"/rec/RecyclerParams;"));
	if (temp != NULL) {
	    cp->recycle = *(rc_param_t *)temp;
	} else {
	    (void) memset((char *)&cp->recycle, 0, sizeof (cp->recycle));
	}
	cp->priority_lst = NULL; // the change flag will never be set by GUI
	// DEBUG params
	cp->simdelay = (int)getJIntFld(env, cls, cpObj, "simdelay");
	cp->tstovfl = getBoolFld(env, cls, cpObj, "tstovfl");
	cp->directio = getBoolFld(env, cls, cpObj, "directio");
	cp->change_flag =
	    (uint32_t)getJLongFld(env, cls, cpObj, "chgFlags");
	cp->rearch_stage_copy = (short)getJShortFld(env, cls, cpObj,
	    "rearch_stage_copy");
	cp->queue_time_limit =
	    (uint32_t)getJLongFld(env, cls, cpObj, "queue_time_limit");

	cp->fillvsns_min =
	    (uint32_t)getJLongFld(env, cls, cpObj, "fillvsnsMin");

	PTRACE(2, "jni:CopyParams2cparams() done");
	return (cp);
}


jobject
arcopyinst2ArCopyProc(JNIEnv *env, void *v_arcopyinst) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	struct ArcopyInstance *cpi = (struct ArcopyInstance *)v_arcopyinst;

	PTRACE(2, "jni:arcopyinst2ArCopyProc(%llu,%llu,%d,%s,%s)",
	    cpi->CiBytesWritten, cpi->CiSpace, cpi->CiFiles,
	    Str(cpi->CiMtype), Str(cpi->CiVsn));
	cls = (*env)->FindClass(env, BASEPKG"/arc/job/ArCopyProc");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(JJJIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    (jlong)(cpi->CiBytesWritten / 1024), // KB to handle fsize_t
	    (jlong)(cpi->CiSpace / 1024), // KB required to archive all files
	    (jlong)cpi->CiPid,
	    (jint)cpi->CiFiles,
	    (jint)cpi->CiFilesWritten,
	    JSTRING(cpi->CiMtype),
	    JSTRING(cpi->CiVsn),
	    JSTRING(cpi->CiOprmsg));
	PTRACE(2, "jni:arcopyinst2ArCopyProc() done");
	return (newObj);
}


jobject
archreq2ArchReq(JNIEnv *env, void *v_archreq) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	struct ArchReq *areq = (struct ArchReq *)v_archreq;
	int file_count;

	PTRACE(2, "jni:archreq2ArchReq entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/job/ArchReq");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;JIJIIJS"
	    "[L"BASEPKG"/arc/job/ArCopyProc;)V");

	/* Showqueue uses ArCount if ArFiles is 0 so we will too. */
	file_count = (areq->ArFiles != 0) ? areq->ArFiles : areq->ArCount;

	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(areq->ArFsname),
	    JSTRING(areq->ArAsname),
	    (jlong)areq->ArTime,
	    (jint)areq->ArState,
	    (jlong)areq->ArSeqnum,
	    (jint)areq->ArDrivesUsed,
	    (jint)file_count,
	    (jlong)(areq->ArSpace / 1024),
	    (jshort)areq->ArFlags,
	    arr2jarray(env,
		areq->ArCpi,
		sizeof (struct ArcopyInstance),
		areq->ArDrives,
		BASEPKG"/arc/job/ArCopyProc",
		arcopyinst2ArCopyProc));
	PTRACE(2, "jni:archreq2ArchReq() done");
	return (newObj);
}


jobject
stats2Stats(JNIEnv *env, void *v_stats) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	struct stats *stats = (struct stats *)v_stats;

	PTRACE(2, "jni:stats2Stats entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/job/Stats");
	mid = (*env)->GetMethodID(env, cls, "<init>", "(IJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    (jint)stats->numof,
	    (jlong)stats->size);
	PTRACE(2, "jni:stats2Stats() done");
	return (newObj);
}


jobject
fsstats2ArFindFsStats(JNIEnv *env, void *v_fstats) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	struct FsStats *fstats = (struct FsStats *)v_fstats;

	PTRACE(2, "jni:fsstats2ArFindFsStats entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/job/ArFindFsStats");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(L"BASEPKG"/arc/job/Stats;"
		"L"BASEPKG"/arc/job/Stats;L"BASEPKG"/arc/job/Stats;"
		"L"BASEPKG"/arc/job/Stats;L"BASEPKG"/arc/job/Stats;"
		"[L"BASEPKG"/arc/job/Stats;)V");

	newObj = (*env)->NewObject(env, cls, mid,
		stats2Stats(env, &fstats->total),
		stats2Stats(env, &fstats->regular),
		stats2Stats(env, &fstats->offline),
		stats2Stats(env, &fstats->archdone),
		stats2Stats(env, &fstats->dirs),
		arr2jarray(env,
		fstats->copies,
		sizeof (struct stats),
		MAX_ARCHIVE,
		BASEPKG"/arc/job/Stats",
		stats2Stats));
	PTRACE(2, "jni:fsstats2ArFindFsStats() done");
	return (newObj);
}


jobject
arfind2ArFindJob(JNIEnv *env, void *v_arfind) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	ar_find_state_t *arfind = (ar_find_state_t *)v_arfind;

	PTRACE(2, "jni:arfind2ArFindJob entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/job/ArFindJob");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;J"
		"L"BASEPKG"/arc/job/ArFindFsStats;"
		"L"BASEPKG"/arc/job/ArFindFsStats;)V");

	newObj = (*env)->NewObject(env, cls, mid,
		JSTRING(arfind->fs_name),
		(jlong)arfind->state.AfPid,
		fsstats2ArFindFsStats(env, &arfind->state.AfStats),
		fsstats2ArFindFsStats(env, &arfind->state.AfStatsScan));
	PTRACE(2, "jni:arfind2ArFindJob() done");
	return (newObj);
}


jobject
archset2ArSet(JNIEnv *env, void *v_arch_set) {
	jclass cls;
	jmethodID mid;
	jobject newObj;
	arch_set_t *as = (arch_set_t *)v_arch_set;

	PTRACE(2, "jni:archset2ArSet() entry");

	if (as == NULL)
		return (NULL);

	cls = (*env)->FindClass(env, BASEPKG"/arc/ArSet");
	mid = (*env)->GetMethodID(env, cls, "<init>",
		"(Ljava/lang/String;"
		"Ljava/lang/String;"
		"S[L"BASEPKG"/arc/Criteria;"
		"[L"BASEPKG"/arc/CopyParams;"
		"[L"BASEPKG"/arc/CopyParams;"
		"[L"BASEPKG"/arc/VSNMap;"
		"[L"BASEPKG"/arc/VSNMap;)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(as->name),
	    JSTRING(as->description),
	    (jshort)(as->type),
	    lst2jarray(env, as->criteria, BASEPKG"/arc/Criteria",
		crit2Criteria),
	    arrOfPtrs2jarray(env, (void *) as->copy_params,
		5, BASEPKG"/arc/CopyParams", cparams2CopyParams),
	    arrOfPtrs2jarray(env, (void *) as->rearch_copy_params,
		5, BASEPKG"/arc/CopyParams", cparams2CopyParams),
	    arrOfPtrs2jarray(env, (void *) as->vsn_maps, 5,
		BASEPKG"/arc/VSNMap", map2VSNMap),
	    arrOfPtrs2jarray(env, (void *) as->rearch_vsn_maps, 5,
		BASEPKG"/arc/VSNMap", map2VSNMap));
	PTRACE(2, "jni:archset2ArSet() done");
	return (newObj);
}


void *
ArSet2archset(JNIEnv *env, jobject asObj) {

	jclass cls;
	jobjectArray j_array;
	void ** arr;
	arch_set_t *as = NULL;

	if (asObj == NULL)
		return (NULL);

	as = (arch_set_t *)malloc(sizeof (arch_set_t));

	PTRACE(2, "jni:ArSet2archset() entry");
	cls = (*env)->GetObjectClass(env, asObj);
	getStrFld(env, cls, asObj, "setName", as->name);
	as->type = getJShortFld(env, cls, asObj, "setType");
	j_array = getJArrFld(env, cls, asObj, "crits",
		"[L"BASEPKG"/arc/Criteria;");
	as->criteria = jarray2lst(env, j_array, BASEPKG"/arc/Criteria",
		Criteria2crit);
	j_array = getJArrFld(env, cls, asObj, "copies",
		"[L"BASEPKG"/arc/CopyParams;");
	arr = jarray2arrOfPtrs(env, j_array, BASEPKG"/arc/CopyParams",
		CopyParams2cparams);
	if (NULL != arr) {
		memcpy(as->copy_params, arr, 5 * sizeof (void *));
		free(arr);
	}
	j_array = getJArrFld(env, cls, asObj, "rearchCopies",
		"[L"BASEPKG"/arc/CopyParams;");
	arr = jarray2arrOfPtrs(env, j_array, BASEPKG"/arc/CopyParams",
		CopyParams2cparams);
	if (NULL != arr) {
		memcpy(as->rearch_copy_params, arr, 5 * sizeof (void *));
		free(arr);
	}
	j_array = getJArrFld(env, cls, asObj, "maps",
		"[L"BASEPKG"/arc/VSNMap;");
	arr = jarray2arrOfPtrs(env, j_array, BASEPKG"/arc/VSNMap",
		VSNMap2map);
	if (NULL != arr) {
		memcpy(as->vsn_maps, arr, 5 * sizeof (void *));
		free(arr);
	}
	j_array = getJArrFld(env, cls, asObj, "rearchMaps",
		"[L"BASEPKG"/arc/VSNMap;");
	arr = jarray2arrOfPtrs(env, j_array, BASEPKG"/arc/VSNMap",
		VSNMap2map);
	if (NULL != arr) {
		memcpy(as->rearch_vsn_maps, arr, 5 * sizeof (void *));
		free(arr);
	}
	getCharStarFld(env, cls, asObj, "description", &(as->description));
	PTRACE(2, "jni:ArSet2archset() done");
	return (as);
}


/* native functions implementation */

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getArGlobalDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	ar_global_directive_t *argd;
	PTRACE(1, "jni:Archiver_getArGlobalDirective() entry");
	if (-1 == get_ar_global_directive(CTX, &argd)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = arglobd2ArGlobalDirective(env, argd);
	// displayCriteriaLst(argd->ar_set_lst);
	free_ar_global_directive(argd);
	PTRACE(1, "jni:Archiver_getArGlobalDirective() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getDefaultArGlobalDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	ar_global_directive_t *argd;

	PTRACE(1, "jni:Archiver_getDefaultArGlobalDirective() entry");

	if (-1 == get_default_ar_global_directive(CTX, &argd)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = arglobd2ArGlobalDirective(env, argd);
	free_ar_global_directive(argd);
	PTRACE(1, "jni:Archiver_getDefaultArGlobalDirective() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_setArGlobalDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jobject gdObj) {

	ar_global_directive_t *argd;
	int res;

	PTRACE(1, "jni:Archiver_setArGlobalDirective() entry");
	argd = ArGlobalDirective2arglobd(env, gdObj);
	res = set_ar_global_directive(CTX, argd);
	free_ar_global_directive(argd);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_setArGlobalDirective() done");
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getArFSDirectives(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newArr;
	sqm_lst_t *arfsdlst;

	PTRACE(1, "jni:Archiver_getArFSDirectives() entry");
	if (-1 == get_all_ar_fs_directives(CTX, &arfsdlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		arfsdlst, BASEPKG"/arc/ArFSDirective", arfsd2ArFSDirective);
	free_ar_fs_directive_list(arfsdlst);
	PTRACE(1, "jni:Archiver_getArFSDirectives() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getArFSDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	jobject newObj;
	ar_fs_directive_t *arfsd;
	int res;
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Archiver_getArFSDirective(%s) entry", Str(cstr));
	res = get_ar_fs_directive(CTX, cstr, &arfsd);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = arfsd2ArFSDirective(env, arfsd);
	free_ar_fs_directive(arfsd);
	PTRACE(1, "jni:Archiver_getArFSDirective() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getDefaultArFSDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	ar_fs_directive_t *arfsd;

	PTRACE(1, "jni:Archiver_getDefaultArFSDirective() entry");
	if (-1 == get_default_ar_fs_directive(CTX, &arfsd)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = arfsd2ArFSDirective(env, arfsd);
	free_ar_fs_directive(arfsd);
	PTRACE(1, "jni:Archiver_getDefaultArFSDirective() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_setArFSDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jobject fsdObj) {

	ar_fs_directive_t *arfsd;
	int res;

	PTRACE(1, "jni:Archiver_setArFSDirective() entry");
	arfsd = ArFSDirective2arfsd(env, fsdObj);
	res = set_ar_fs_directive(CTX, arfsd);
	free_ar_fs_directive(arfsd);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_settArFSDirective() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_resetArFSDirective(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	int res;
	char *cstr;

	PTRACE(1, "jni:Archiver_resetArFSDirective() entry");
	cstr = GET_STR(fsName, isCopy);
	res = reset_ar_fs_directive(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_resettArFSDirective() done");
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCriteriaNames(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *critnames;

	PTRACE(1, "jni:Archiver_getCriteriaNames() entry");
	if (-1 == get_ar_set_criteria_names(CTX, &critnames)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		critnames, "java/lang/String", charr2String);
	lst_free_deep(critnames);
	PTRACE(1, "jni:Archiver_getCriteriaNames() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCriteria(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *critlst;

	PTRACE(1, "jni:Archiver_getCriteria() entry");
	if (-1 == get_all_ar_set_criteria(CTX, &critlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		critlst, BASEPKG"/arc/Criteria", crit2Criteria);
	free_ar_set_criteria_list(critlst);
	PTRACE(1, "jni:Archiver_getCriteria() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCriteriaForFS(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	jobjectArray newArr;
	sqm_lst_t *critlst;
	int res;
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Archiver_getCriteriaForFS() entry");
	res = get_ar_set_criteria_list(CTX, cstr, &critlst);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		critlst, BASEPKG"/arc/Criteria", crit2Criteria);
	free_ar_set_criteria_list(critlst);
	PTRACE(1, "jni:Archiver_getCriteriaForFS() done");
	return (newArr);
}


JNIEXPORT jboolean JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_isValidGroup(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring grpName) {

	jboolean isCopy;
	int res;
	boolean_t isValid;
	char *cstr = GET_STR(grpName, isCopy);

	PTRACE(1, "jni:Archiver_isValidGroup");
	res = is_valid_group(CTX, cstr, &isValid);
	REL_STR(grpName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:done checking group");
	return (JBOOL(isValid));
}


JNIEXPORT jboolean JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_isValidUser(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring usrName) {

	jboolean isCopy;
	int res;
	boolean_t isValid;
	char *cstr = GET_STR(usrName, isCopy);

	PTRACE(1, "jni:Archiver_isValidUser");
	res = is_valid_user(CTX, cstr, &isValid);
	REL_STR(usrName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:done checking user");
	return (JBOOL(isValid));
}

// ---------------------- copy parameters functions


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCopyParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *cplst;

	PTRACE(1, "jni:Archiver_getCopyParams() entry");
	if (-1 == get_all_ar_set_copy_params(CTX, &cplst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		cplst, BASEPKG"/arc/CopyParams", cparams2CopyParams);
	free_ar_set_copy_params_list(cplst);
	PTRACE(1, "jni:Archiver_getCopyParams() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCopyParamsNames(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *cpnames;

	PTRACE(1, "jni:Archiver_getCopyParamsNames() entry");
	if (-1 == get_ar_set_copy_params_names(CTX, &cpnames)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		cpnames, "java/lang/String", charr2String);
	lst_free_deep(cpnames);
	PTRACE(1, "jni:Archiver_getCopyParamsNames() done");
	return (newArr);

}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCopyParamsForCopy(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring copyName) {

	jboolean isCopy;
	jobject newObj;
	ar_set_copy_params_t *cp;
	int res;
	char *cstr;

	PTRACE(1, "jni:Archiver_getCopyParamsForCopy() entry");
	cstr = GET_STR(copyName, isCopy);
	res = get_ar_set_copy_params(CTX, cstr, &cp);
	REL_STR(copyName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = cparams2CopyParams(env, cp);
	free(cp);
	PTRACE(1, "jni:Archiver_getCopyParamsForCopy() done");
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCopyParamsForSet(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring setName) {

	jboolean isCopy;
	jobjectArray newArr;
	sqm_lst_t *cplst;
	int res;
	char *cstr;

	PTRACE(1, "jni:Archiver_getCopyParamsForSet() entry");
	cstr = GET_STR(setName, isCopy);
	res = get_ar_set_copy_params_for_ar_set(CTX, cstr, &cplst);
	REL_STR(setName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		cplst, BASEPKG"/arc/CopyParams", cparams2CopyParams);
	free_ar_set_copy_params_list(cplst);
	PTRACE(1, "jni:Archiver_getCopyParamsForSet() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getDefaultCopyParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newObj;
	ar_set_copy_params_t *cp;

	PTRACE(1, "jni:Archiver_getDefaultCopyParams() entry");
	if (-1 == get_default_ar_set_copy_params(CTX, &cp)) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = cparams2CopyParams(env, cp);
	free(cp);
	PTRACE(1, "jni:Archiver_getDefaultCopyParams() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_setCopyParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jobject cprmObj) {

	ar_set_copy_params_t *cp;
	int res;

	PTRACE(1, "jni:Archiver_setCopyParams() entry");
	cp = CopyParams2cparams(env, cprmObj);
	res = set_ar_set_copy_params(CTX, cp);
	free(cp);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_setCopyParams() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_resetCopyParams(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring cpName) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:Archiver_resetCopyParams() entry");
	cstr = GET_STR(cpName, isCopy);
	res = reset_ar_set_copy_params(CTX, cstr);
	REL_STR(cpName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_resetCopyParams() done");
}


/* ------------------- Archive Set functions */


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getArSets(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	int res;
	sqm_lst_t *listArchiveSets = NULL;
	jobjectArray newArr;

	PTRACE(1, "jni:Archiver_getArSets() entry");
	res = get_all_arch_sets(CTX, &listArchiveSets);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	if (listArchiveSets == NULL) {
		return (NULL);
	}
	newArr = lst2jarray(env,
		listArchiveSets, BASEPKG"/arc/ArSet", archset2ArSet);
	free_arch_set_list(listArchiveSets);

	PTRACE(1, "jni:Archiver_getArSets() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getArSet(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring jname) {

	arch_set_t *set;
	int res;
	jobject ret;
	jboolean isCopy;
	char *name;

	PTRACE(1, "jni:Archiver_getArSet() entry");
	name = GET_STR(jname, isCopy);
	res = get_arch_set(CTX, name, &set);
	REL_STR(jname, name, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	ret = archset2ArSet(env, set);
	PTRACE(1, "jni:Archiver_getArSet() done");
	return (ret);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_createArSet(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jobject jset) {

	arch_set_t *set;
	int res = 0;

	PTRACE(1, "jni:Archiver_createArSet() entry");
	set = ArSet2archset(env, jset);
	res = create_arch_set(CTX, set);
	free_arch_set(set);

	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_createArSet() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_modifyArSet(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jobject jset) {

	arch_set_t *set;
	int res;
	PTRACE(1, "jni:Archiver_modifyArSet() entry");
	set = ArSet2archset(env, jset);
	res = modify_arch_set(CTX, set);

	free_arch_set(set);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_modifyArSet() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_deleteArSet(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring jname) {

	int res;
	jboolean isCopy;
	char *name;

	PTRACE(1, "jni:Archiver_deleteArSet() entry");
	name = GET_STR(jname, isCopy);
	res = delete_arch_set(CTX, name);
	REL_STR(jname, name, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_deleteArSet() done");
}


// ---------------- archiver control functions


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_runForFS(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:Archiver_runForFS() entry");
	cstr = GET_STR(fsName, isCopy);
	res = ar_run(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_runForFS() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_run(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Archiver_run() entry");
	if (-1 == ar_run_all(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_run() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_idleForFS(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:Archiver_idleForFS() entry");
	cstr = GET_STR(fsName, isCopy);
	res = ar_idle(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_idleForFS() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_idle(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Archiver_idle() entry");
	if (-1 == ar_idle_all(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_idle() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_stopForFS(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr;
	int res;

	PTRACE(1, "jni:Archiver_stopForFS() entry");
	cstr = GET_STR(fsName, isCopy);
	res = ar_stop(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_stopForFS() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_stop(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Archiver_stop() entry");
	if (-1 == ar_stop_all(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_stop() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_restart(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Archiver_restart() entry");
	if (-1 == ar_restart_all(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_restart() done");
}

/* Archiver rerun is available in 4.4 (samfsversion 1.3.2) and above */
JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_rerun(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	PTRACE(1, "jni:Archiver_rerun() entry");
	if (-1 == ar_rerun_all(CTX)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Archiver_rerun() done");
}

JNIEXPORT jobjectArray
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_activateCfg(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *err_warn_lst;
	jobjectArray warnArr, errArr;
	int res;

	PTRACE(1, "jni:Archiver_activateCfg() entry");
	res = activate_archiver_cfg(CTX, &err_warn_lst);
	PTRACE(1, "jni:activateCfg returned %d, lst[%d]", res,
	    (res == -2 || res == -3) ? err_warn_lst->length : -1);
	switch (res) {
	case -1:
		/* internal error */
		ThrowEx(env);
		return (NULL);
	case -2:
		/* archiver.cmd errors */
		errArr = lst2jarray(env,
			err_warn_lst, "java/lang/String", charr2String);
		lst_free_deep(err_warn_lst);
		ThrowMultiMsgEx(env, errArr);
		return (NULL);
	case -3:
		/* archiver.cmd warnings */
		warnArr = lst2jarray(env,
			err_warn_lst, "java/lang/String", charr2String);
		lst_free_deep(err_warn_lst);
		break;
	default:
		/* success */
		lst_free(err_warn_lst);
		warnArr = NULL;
	}
	PTRACE(1, "jni:Archiver_activateCfg() done");
	return (warnArr);
}


JNIEXPORT void
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_activateCfgThrowWarnings(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *err_warn_lst;
	jobjectArray warnArr, errArr;
	int res;

	PTRACE(1, "jni:Archiver_activateCfgThrowWarnings() entry");
	res = activate_archiver_cfg(CTX, &err_warn_lst);
	PTRACE(1, "jni:activateCfg returned %d, lst[%d]", res,
	    (res == -2 || res == -3) ? err_warn_lst->length : -1);
	switch (res) {
	case -1:
		/* internal error */
		ThrowEx(env);
		return;
	case -2:
		/* archiver.cmd errors */
		errArr = lst2jarray(env,
			err_warn_lst, "java/lang/String", charr2String);
		lst_free_deep(err_warn_lst);
		ThrowMultiMsgEx(env, errArr);
		return;
	case -3:
		/* archiver.cmd warnings */
		warnArr = lst2jarray(env,
			err_warn_lst, "java/lang/String", charr2String);
		lst_free_deep(err_warn_lst);
		ThrowWarnings(env, warnArr);
		return;
	default:
		/* success */
		lst_free(err_warn_lst);
	}
	PTRACE(1, "jni:Archiver_activateCfgThrowWarnings() done");
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_archiveFiles(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jobjectArray files, jint options) {
	char *jobID;


	PTRACE(1, "jni:Releaser_releaseFiles() entry");
	if (-1 == archive_files(CTX,
	    jarray2lst(env, files, "java/lang/String", String2charr),
	    (int32_t)options, &jobID)) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:Archiver_archiveFiles() exit");
	return (JSTRING(jobID));
}


// ---------------- archiver jobs functions

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_job_ArchReq_getAll(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *arcreqlst;

	PTRACE(1, "jni:ArchReq_getAll() entry");
	if (-1 == get_all_archreqs(CTX, &arcreqlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		arcreqlst, BASEPKG"/arc/job/ArchReq", archreq2ArchReq);
	lst_free_deep(arcreqlst);
	PTRACE(1, "jni:ArchReq_getAll() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_job_ArFindJob_getAll(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *arfindlst; // list of ar_find_state_t

	PTRACE(1, "jni:ArFindJob_getAll() entry");
	if (-1 == get_all_arfind_state(CTX, &arfindlst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env,
		arfindlst, BASEPKG"/arc/job/ArFindJob", arfind2ArFindJob);
	lst_free_deep(arfindlst);
	PTRACE(1, "jni:ArFindJob_getAll() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_Archiver_getCopyUtil(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jint topCount) {

	jobjectArray newArr;
	sqm_lst_t *lst;

	PTRACE(1, "jni:getCopyUtil() entry");
	if (-1 == get_copy_utilization(CTX, (int)topCount, &lst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst,  "java/lang/String", charr2String);
	lst_free_deep(lst);
	PTRACE(1, "jni:getCopyUtil() done");
	return (newArr);
}
