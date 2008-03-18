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
#include "pub/mgmt/release.h"
#include "pub/mgmt/error.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_rel_Releaser.h"
#include "jni_util.h"


jobject
reldir2ReleaserDirective(JNIEnv *env, void *v_reldir) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	float sprio, aprio, mprio, rprio;
	rl_fs_directive_t *rl = (rl_fs_directive_t *)v_reldir;

	PTRACE(2, "jni:reldir2ReleaserDirective(%x,%x)", env, rl);

	sprio = aprio = mprio = rprio = -1;
	if (SIMPLE_AGE_PRIO == rl->type)
		sprio = rl->age_priority.simple;
	if (DETAILED_AGE_PRIO == rl->type) {
		aprio = rl->age_priority.detailed.access_weight;
		mprio = rl->age_priority.detailed.modify_weight;
		rprio = rl->age_priority.detailed.residence_weight;
	}

	cls = (*env)->FindClass(env, BASEPKG"/rel/ReleaserDirective");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls,
	    "<init>", "(Ljava/lang/String;JZZZZIFSFFFFI)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(rl->releaser_log),
	    (jlong)rl->min_residence_age,
	    JBOOL(rl->no_release),
	    JBOOL(rl->rearch_no_release),
	    JBOOL(rl->display_all_candidates),
	    JBOOL(rl->debug_partial),
	    (jint)rl->list_size,
	    (jfloat)rl->size_priority,
	    (jshort)rl->type,
	    (jfloat)sprio,
	    (jfloat)aprio,
	    (jfloat)mprio,
	    (jfloat)rprio,
	    (jint)rl->change_flag);
	PTRACE(2, "jni:reldir2ReleaserDirective done", env, rl);
	return (newObj);
}


rl_fs_directive_t *
ReleaserDirective2reldir(JNIEnv *env, jobject rlObj) {

	jclass cls;
	rl_fs_directive_t *rl = (rl_fs_directive_t *)
	    malloc(sizeof (rl_fs_directive_t));

	PTRACE(2, "jni:ReleaserDirective2reldir() entry");
	cls = (*env)->GetObjectClass(env, rlObj);
	getStrFld(env, cls, rlObj, "logFileName", rl->releaser_log);
	rl->min_residence_age = (long)getJLongFld(env, cls, rlObj, "minAge");
	rl->no_release = getBoolFld(env, cls, rlObj, "noRelease");
	rl->rearch_no_release = getBoolFld(env, cls, rlObj, "rearchNoRelease");
	rl->display_all_candidates =
	    getBoolFld(env, cls, rlObj, "logCandidates");
	rl->debug_partial = getBoolFld(env, cls, rlObj, "debugPartial");
	rl->list_size = (int)getJIntFld(env, cls, rlObj, "files");
	rl->size_priority = (float)getJFloatFld(env, cls, rlObj, "sizePrio");
	rl->type = (age_prio_type)getJShortFld(env, cls, rlObj, "agePrioType");
	if (SIMPLE_AGE_PRIO == rl->type)
		rl->age_priority.simple =
		    (float)getJFloatFld(env, cls, rlObj, "agePrioSimple");
	if (DETAILED_AGE_PRIO == rl->type) {
		rl->age_priority.detailed.access_weight =
		    (float)getJFloatFld(env, cls, rlObj, "agePrioAccess");
		rl->age_priority.detailed.modify_weight =
		    (float)getJFloatFld(env, cls, rlObj, "agePrioModify");
		rl->age_priority.detailed.residence_weight =
		    (float)getJFloatFld(env, cls, rlObj, "agePrioResidence");
	}
	rl->change_flag = (uint32_t)getJIntFld(env, cls, rlObj, "chgFlags");
	PTRACE(2, "jni:ReleaserDirective2reldir() done");
	return (rl);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rel_Releaser_getDefaultDirective(
    JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject relDirObj;
	rl_fs_directive_t *rl;

	PTRACE(1, "jni:Releaser_getDefaultDirective() entry");
	if (-1 == get_default_rl_fs_directive(CTX, &rl)) {
		ThrowEx(env);
		return (NULL);
	}

	relDirObj = reldir2ReleaserDirective(env, rl);
	free(rl);
	PTRACE(1, "jni:Releaser_getDefaultDirective() done");
	return (relDirObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rel_Releaser_getGlobalDirective(
    JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject relDirObj;
	rl_fs_directive_t *rl;

	PTRACE(1, "jni:Releaser_getGlobalDirective() entry");
	if (-1 == get_rl_fs_directive(CTX, GLOBAL, &rl)) {
		ThrowEx(env);
		return (NULL);
	}

	relDirObj = reldir2ReleaserDirective(env, rl);
	free(rl);
	PTRACE(1, "jni:Releaser_getGlobalDirective() done");
	return (relDirObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rel_Releaser_setGlobalDirective(
    JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jobject relDirObj) {

	rl_fs_directive_t *rl;
	int res;

	PTRACE(1, "jni:Releaser_setGlobalDirective() entry");
	rl = ReleaserDirective2reldir(env, relDirObj);
	if (NULL != rl)
		strcpy(rl->fs, GLOBAL);
	else
		PTRACE(1, "jni:rl is NULL");
	res = set_rl_fs_directive(CTX, rl);
	free(rl);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:Releaser_setGlobalDirective() done");
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rel_Releaser_releaseFiles(JNIEnv *env,
    jclass cls /* ARGSUSED */, jobject ctx, jobjectArray files,
    jint options, jint partial_sz) {

	char *jobID;

	PTRACE(1, "jni:Releaser_releaseFiles() entry");
	if (-1 == release_files(CTX,
	    jarray2lst(env, files, "java/lang/String", String2charr),
	    (int32_t)options, (int32_t)partial_sz, &jobID)) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:Releaser_releaseFiles() exit");
	return (JSTRING(jobID));
}




// ----------------- job-related functions

jobject
relfs2ReleaserJob(JNIEnv *env, void *v_relfs) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	release_fs_t *relfs = (release_fs_t *)v_relfs;

	PTRACE(1, "jni:relfs2ReleaserJob() entry");
	cls = (*env)->FindClass(env, BASEPKG"/rel/ReleaserJob");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;SS)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(relfs->fi_name),
	    (jshort)relfs->fi_low,
	    (jshort)relfs->used_pct);
	PTRACE(1, "jni:relfs2ReleaserJob() done");
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_rel_ReleaserJob_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobject newArr;
	sqm_lst_t *relfslst;

	PTRACE(1, "jni:ReleaserJob_getAll() entry");
	if (-1 == get_releasing_fs_list(CTX, &relfslst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, relfslst,
	    BASEPKG"/rel/ReleaserJob", relfs2ReleaserJob);
	lst_free_deep(relfslst);
	PTRACE(1, "jni:ReleaserJob_getAll() done");
	return (newArr);
}
