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
#pragma ident	"$Revision: 1.50 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/device.h"
#include "mgmt/log.h"
#include "pub/mgmt/file_metrics_report.h"
/* SAM-FS/QFS core header files */
#include "sam/param.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_fs_FS.h"
#include "com_sun_netstorage_samqfs_mgmt_fs_SamfsckJob.h"
#include "jni_util.h"

#define	SMALLDAU 4096


/* C structure <-> Java class conversion functions */

extern jobject au2AU(JNIEnv *env, void *v_au);
extern void *AU2au(JNIEnv *env, jobject auObj);
extern jobject host2Host(JNIEnv *env, void *v_host);
extern void *Host2host(JNIEnv *env, jobject hostObj);
extern void * Copy2copy(JNIEnv *env, jobject cpObj);
extern void * VSNMap2map(JNIEnv *env, jobject mapObj);

jobject
dsk2DiskDev(JNIEnv *env, void *v_dsk) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	disk_t *dsk = (disk_t *)v_dsk;

	PTRACE(2, "jni:dsk2DiskDev entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/DiskDev");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;"
	    "IILjava/lang/String;"
	    "L"BASEPKG"/fs/AU;JJ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    // BaseDev properties
	    JSTRING(dsk->base_info.name),
	    (jint)dsk->base_info.eq,
	    JSTRING(dsk->base_info.equ_type),
	    JSTRING(dsk->base_info.set),
	    (jint)dsk->base_info.fseq,
	    (jint)dsk->base_info.state,
	    JSTRING(dsk->base_info.additional_params),
	    // DiskDev-specific properties
	    au2AU(env, &dsk->au_info),
	    (jlong)(dsk->freespace/1024),
	    (jlong)(dsk->freespace/SAM_ISIZE));
	PTRACE(2, "jni:dsk2DiskDev() done");
	return (newObj);
}


void *
DiskDev2dsk(JNIEnv *env, jobject ddevObj) {

	jclass cls;
	disk_t *dsk;
	base_dev_t binf;

	PTRACE(2, "jni:DiskDev2dsk() entry");
	if (ddevObj == NULL) {
		PTRACE(2, "jni:DiskDev2dsk():NULL arg");
		return (NULL);
	}
	dsk = (disk_t *)malloc(sizeof (disk_t));
	cls = (*env)->GetObjectClass(env, ddevObj);

	/* initialize base_dev fields */
	getStrFld(env, cls, ddevObj, "devPath", binf.name);
	binf.eq = (equ_t)getJIntFld(env, cls, ddevObj, "eq");
	getStrFld(env, cls, ddevObj, "eqType", binf.equ_type);
	getStrFld(env, cls, ddevObj, "fsetName", binf.set);
	binf.fseq = (equ_t)getJIntFld(env, cls, ddevObj, "eqFSet");
	binf.state = (dstate_t)getJIntFld(env, cls, ddevObj, "state");
	getStrFld(env, cls, ddevObj, "paramFilePath",
	    binf.additional_params);
	dsk->base_info = binf;
	/* initialize the rest of the disk_t fields */
	dsk->au_info = *(au_t *)AU2au(env,
	    getObjFld(env, cls, ddevObj, "au", "L"BASEPKG"/fs/AU;"));
	dsk->freespace = 1024 * (fsize_t)
	    getJLongFld(env, cls, ddevObj, "kbytesFree");
//	TRACE("jni:%s|%s|%s|%s|%lld.",
//	    binf.name, binf.equ_type, binf.set, binf.additional_params,
//	    dsk->freespace);
	PTRACE(2, "jni:DiskDev2dsk() done");
	return (dsk);
}


jobject
sgrp2StripedGrp(JNIEnv *env, void *v_grp) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	striped_group_t *grp = (striped_group_t *)v_grp;

	PTRACE(2, "jni:sgrp2StripedGrp() entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/StripedGrp");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;[L"BASEPKG"/fs/DiskDev;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(grp->name),
	    lst2jarray(env, grp->disk_list,
		BASEPKG"/fs/DiskDev", dsk2DiskDev));
	PTRACE(2, "jni:sgrp2StripedGrp() done");
	return (newObj);
}


void *
StripedGrp2sgrp(JNIEnv *env, jobject sgrpObj) {

	jclass cls;
	striped_group_t *sgrp =
	    (striped_group_t *)malloc(sizeof (striped_group_t));

	PTRACE(2, "jni:StripedGrp2sgrp() entry");
	cls = (*env)->GetObjectClass(env, sgrpObj);
	getStrFld(env, cls, sgrpObj, "name", sgrp->name);
	sgrp->disk_list = jarray2lst(env,
	    getJArrFld(env,
		cls, sgrpObj, "devs", "[L"BASEPKG"/fs/DiskDev;"),
	    BASEPKG"/fs/DiskDev",
	    DiskDev2dsk);
	PTRACE(2, "jni:StripedGrp2sgrp() done");
	return (sgrp);
}


jobject
mntopts2MountOptions(JNIEnv *env, void *v_mntopts) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	mount_options_t *opts = (mount_options_t *)v_mntopts;

	PTRACE(2, "jni:mntopts2MountOptions() entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/MountOptions");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(ZSZSZZIIISSIIJJIIZZZIZSJJIIIZIIIIZZIZIZSIIIIIIIZZIJJJZIIIZ"
	    "ZZZIZZZZZZZSIIIJIIZZZ)V");

	newObj = (*env)->NewObject(env, cls, mid,
	    // general options
	    JBOOL(opts->readonly),
	    (jshort)opts->sync_meta,
	    JBOOL(opts->no_suid),
	    (jshort)opts->stripe,
	    JBOOL(opts->trace),
	    JBOOL(opts->quota),
	    (jint)opts->rd_ino_buf_size,
	    (jint)opts->wr_ino_buf_size,
	    (jint)opts->change_flag,
	    // SAM options
	    (jshort)opts->sam_opts.high,
	    (jshort)opts->sam_opts.low,
	    (jint)opts->sam_opts.partial,
	    (jint)opts->sam_opts.maxpartial,
	    (jlong)opts->sam_opts.partial_stage,
	    (jlong)opts->sam_opts.stage_n_window,
	    (jint)opts->sam_opts.stage_retries,
	    (jint)opts->sam_opts.stage_flush_behind,
	    JBOOL(opts->sam_opts.hwm_archive),
	    JBOOL(opts->sam_opts.archive),
	    JBOOL(opts->sam_opts.arscan),
	    (jint)opts->sam_opts.change_flag,
	    // shared FS options
	    // opts->sharedfs_opts.shared ??
	    JBOOL(opts->sharedfs_opts.bg),
	    (jshort)opts->sharedfs_opts.retry,
	    (jlong)opts->sharedfs_opts.minallocsz,
	    (jlong)opts->sharedfs_opts.maxallocsz,
	    (jint)opts->sharedfs_opts.rdlease,
	    (jint)opts->sharedfs_opts.wrlease,
	    (jint)opts->sharedfs_opts.aplease,
	    JBOOL(opts->sharedfs_opts.mh_write),
	    (jint)opts->sharedfs_opts.nstreams,
	    (jint)opts->sharedfs_opts.meta_timeo,
	    (jint)opts->sharedfs_opts.lease_timeo,
	    (jint)opts->sharedfs_opts.change_flag,
	    // multireader options
	    JBOOL(opts->multireader_opts.writer),
	    JBOOL(opts->multireader_opts.reader),
	    (jint)opts->multireader_opts.invalid,
	    JBOOL(opts->multireader_opts.refresh_at_eof),
	    (jint)opts->multireader_opts.change_flag,
	    // qfs options
	    JBOOL(opts->qfs_opts.qwrite),
	    (jshort)opts->qfs_opts.mm_stripe,
	    (jint)opts->qfs_opts.change_flag,
	    // I/O options
	    (jint)opts->io_opts.dio_rd_consec,
	    (jint)opts->io_opts.dio_rd_form_min,
	    (jint)opts->io_opts.dio_rd_ill_min,
	    (jint)opts->io_opts.dio_wr_consec,
	    (jint)opts->io_opts.dio_wr_form_min,
	    (jint)opts->io_opts.dio_wr_ill_min,
	    JBOOL(opts->io_opts.forcedirectio),
	    JBOOL(opts->io_opts.sw_raid),
	    (jint)opts->io_opts.flush_behind,
	    (jlong)opts->io_opts.readahead,
	    (jlong)opts->io_opts.writebehind,
	    (jlong)opts->io_opts.wr_throttle,
	    JBOOL(opts->io_opts.forcenfsasync),
	    (jint)opts->io_opts.change_flag,
	    // Post 4.2 option
	    (jint)opts->post_4_2_opts.change_flag,
	    (jint)opts->post_4_2_opts.def_retention,
	    JBOOL(opts->post_4_2_opts.abr),
	    JBOOL(opts->post_4_2_opts.dmr),
	    JBOOL(opts->post_4_2_opts.dio_szero),
	    JBOOL(opts->post_4_2_opts.cattr),
	    // release 4.6 options
	    (jint)opts->rel_4_6_opts.change_flag,
	    JBOOL(opts->rel_4_6_opts.worm_emul),
	    JBOOL(opts->rel_4_6_opts.worm_lite),
	    JBOOL(opts->rel_4_6_opts.emul_lite),
	    JBOOL(opts->rel_4_6_opts.cdevid),
	    JBOOL(opts->rel_4_6_opts.clustermgmt),
	    JBOOL(opts->rel_4_6_opts.clusterfastsw),
	    JBOOL(opts->rel_4_6_opts.noatime),
	    (jshort)opts->rel_4_6_opts.atime,
	    (jint)opts->rel_4_6_opts.min_pool,
	    // release 5.0 options
	    (jint)opts->rel_5_0_opts.change_flag,
	    (jint)opts->rel_5_0_opts.obj_width,
	    (jlong)opts->rel_5_0_opts.obj_depth,
	    (jint)opts->rel_5_0_opts.obj_pool,
	    (jint)opts->rel_5_0_opts.obj_sync_data,
	    JBOOL(opts->rel_5_0_opts.logging),
	    JBOOL(opts->rel_5_0_opts.sam_db),
	    JBOOL(opts->rel_5_0_opts.xattr));

	PTRACE(2, "jni:mntopts2MountOptions() done");
	return (newObj);
}


void *
MountOptions2mntopts(JNIEnv *env, jobject moObj) {

	jclass cls;
	mount_options_t *opts;

	PTRACE(2, "jni:MountOptions2mntopts() entry");
	if (NULL == moObj) {
		PTRACE(2, "jni:MountOptions2mntopts() done. (NULL)");
		return (NULL);
	}
	opts = (mount_options_t *)malloc(sizeof (mount_options_t));
	memset(opts, 0, sizeof (mount_options_t));
	cls = (*env)->GetObjectClass(env, moObj);

	// general options
	opts->readonly = getBoolFld(env, cls, moObj, "readOnly");
	opts->sync_meta = (int16_t)getJShortFld(env, cls, moObj, "syncMeta");
	opts->no_suid = getBoolFld(env, cls, moObj, "noSetUID");
	opts->stripe = (int16_t)getJShortFld(env, cls, moObj, "stripeWidth");
	opts->trace = getBoolFld(env, cls, moObj, "trace");
	opts->quota = getBoolFld(env, cls, moObj, "quota");
	opts->rd_ino_buf_size = (int)getJIntFld(env, cls,
	    moObj, "rd_ino_buf_size");
	opts->wr_ino_buf_size = (int)getJIntFld(env, cls,
	    moObj, "wr_ino_buf_size");

	opts->change_flag = (uint32_t)
	    getJIntFld(env, cls, moObj, "chgFlags");
	// SAM options
	opts->sam_opts.high = (int)getJShortFld(env, cls, moObj, "high");
	opts->sam_opts.low = (int)getJShortFld(env, cls, moObj, "low");
	opts->sam_opts.partial = (int)getJIntFld(env, cls,
	    moObj, "partialRelKb");
	opts->sam_opts.maxpartial = (int)getJIntFld(env, cls,
	    moObj, "maxPartialRelKb");
	opts->sam_opts.partial_stage = (uint32_t)getJLongFld(env, cls,
	    moObj, "partialStageKb");
	opts->sam_opts.stage_n_window = (uint32_t)getJLongFld(env, cls,
	    moObj, "stageWinKb");
	opts->sam_opts.stage_retries = (int)getJIntFld(env, cls,
	    moObj, "stageRetries");
	opts->sam_opts.stage_flush_behind = (int)getJIntFld(env, cls,
	    moObj, "stageFlushBehindKb");
	opts->sam_opts.hwm_archive = getBoolFld(env, cls, moObj, "arcAutorun");
	opts->sam_opts.archive = getBoolFld(env, cls, moObj, "archive");
	opts->sam_opts.arscan = getBoolFld(env, cls, moObj, "arscan");
	opts->sam_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "samChgFlags");
	// shared FS options
	opts->sharedfs_opts.shared = 0;
	opts->sharedfs_opts.bg = getBoolFld(env, cls, moObj, "backgr");
	opts->sharedfs_opts.retry = (int16_t)getJShortFld(env, cls,
	    moObj, "retry");
	opts->sharedfs_opts.minallocsz = (long long)getJLongFld(env, cls,
	    moObj, "minBlocks");
	opts->sharedfs_opts.maxallocsz = (long long)getJLongFld(env, cls,
	    moObj, "maxBlocks");
	opts->sharedfs_opts.rdlease = (int)getJIntFld(env, cls,
	    moObj, "rdLease");
	opts->sharedfs_opts.wrlease = (int)getJIntFld(env, cls,
	    moObj, "wrLease");
	opts->sharedfs_opts.aplease = (int)getJIntFld(env, cls,
	    moObj, "appLease");
	opts->sharedfs_opts.mh_write = getBoolFld(env, cls,
	    moObj, "multiWrite");
	opts->sharedfs_opts.nstreams = (int)getJIntFld(env, cls,
	    moObj, "nStreams");
	opts->sharedfs_opts.meta_timeo = (int)getJIntFld(env, cls,
	    moObj, "metaTimeout");
	opts->sharedfs_opts.lease_timeo = (int)getJIntFld(env, cls,
	    moObj, "leaseTimeout");
	opts->sharedfs_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "sharedfsChgFlags");
	// multireader options
	opts->multireader_opts.writer = getBoolFld(env, cls, moObj, "writer");
	opts->multireader_opts.reader = getBoolFld(env, cls, moObj, "reader");
	opts->multireader_opts.invalid = (int)getJIntFld(env, cls,
	    moObj, "invalid");
	opts->multireader_opts.refresh_at_eof = getBoolFld(env, cls,
	    moObj, "refresh_at_eof");
	opts->multireader_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "multirdChgFlags");
	// qfs options
	opts->qfs_opts.qwrite = getBoolFld(env, cls, moObj, "quickWrite");
	opts->qfs_opts.mm_stripe = (uint16_t)getJShortFld(env, cls,
	    moObj, "metaStripeWidth");
	opts->qfs_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "qfsChgFlags");
	// I/O options
	opts->io_opts.dio_rd_consec = (int)getJIntFld(env, cls,
	    moObj, "dioRdConsec");
	opts->io_opts.dio_rd_form_min = (int)getJIntFld(env, cls,
	    moObj, "dioRdFormMinKb");
	opts->io_opts.dio_rd_ill_min = (int)getJIntFld(env, cls,
	    moObj, "dioRdIllMinKb");
	opts->io_opts.dio_wr_consec = (int)getJIntFld(env, cls,
	    moObj, "dioWrConsec");
	opts->io_opts.dio_wr_form_min = (int)getJIntFld(env, cls,
	    moObj, "dioWrFormMinKb");
	opts->io_opts.dio_wr_ill_min = (int)getJIntFld(env, cls,
	    moObj, "dioWrIllMinKb");
	opts->io_opts.forcedirectio = getBoolFld(env, cls, moObj, "forceDIO");
	opts->io_opts.sw_raid = getBoolFld(env, cls, moObj, "softRAID");
	opts->io_opts.flush_behind = (int)getJIntFld(env, cls,
	    moObj, "flushBehindKb");
	opts->io_opts.readahead = (long long)getJLongFld(env, cls,
	    moObj, "rdAheadKb");
	opts->io_opts.writebehind = (long long)getJLongFld(env, cls,
	    moObj, "wrBehindKb");
	opts->io_opts.wr_throttle = (long long)getJLongFld(env, cls,
	    moObj, "wrThrottleKb");
	opts->io_opts.forcenfsasync = getBoolFld(env, cls,
		moObj, "forceNFSAsync");
	opts->io_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "ioChgFlags");

	// Post 4.2 options
	opts->post_4_2_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "post42ChgFlags");
	opts->post_4_2_opts.def_retention = (int)getJIntFld(env, cls,
	    moObj, "defRetention");
	opts->post_4_2_opts.abr = getBoolFld(env, cls,
		moObj, "appBasedRecovery");
	opts->post_4_2_opts.dmr = getBoolFld(env, cls,
		moObj, "directedMirrorReads");
	opts->post_4_2_opts.dio_szero = getBoolFld(env, cls,
		moObj, "directIOZeroing");
	opts->post_4_2_opts.cattr = getBoolFld(env, cls,
		moObj, "consistencyChecking");

	// Release 4.6 options
	opts->rel_4_6_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "rel46ChgFlags");
	opts->rel_4_6_opts.worm_emul = getBoolFld(env, cls,
		moObj, "worm_emul");
	opts->rel_4_6_opts.worm_lite = getBoolFld(env, cls,
		moObj, "worm_lite");
	opts->rel_4_6_opts.emul_lite = getBoolFld(env, cls,
		moObj, "emul_lite");
	opts->rel_4_6_opts.cdevid = getBoolFld(env, cls,
		moObj, "cdevid");
	opts->rel_4_6_opts.clustermgmt = getBoolFld(env, cls,
		moObj, "clustermgmt");
	opts->rel_4_6_opts.clusterfastsw = getBoolFld(env, cls,
		moObj, "clusterfastsw");
	opts->rel_4_6_opts.noatime = getBoolFld(env, cls,
		moObj, "noatime");
	opts->rel_4_6_opts.atime = (int)getJShortFld(env, cls,
		moObj, "atime");
	opts->rel_4_6_opts.min_pool = (int)getJIntFld(env, cls,
		moObj, "min_pool");

	// Release 5.0 options
	opts->rel_5_0_opts.change_flag = (uint32_t)getJIntFld(env, cls,
	    moObj, "rel50ChgFlags");
	opts->rel_5_0_opts.obj_width = (int16_t)getJIntFld(env, cls,
		moObj, "objWidth");
	opts->rel_5_0_opts.obj_depth = (int64_t)getJLongFld(env, cls,
		moObj, "objDepth");
	opts->rel_5_0_opts.obj_pool = (int16_t)getJIntFld(env, cls,
		moObj, "objPool");
	opts->rel_5_0_opts.obj_sync_data = (int16_t)getJIntFld(env, cls,
		moObj, "objSyncData");
	opts->rel_5_0_opts.logging = getBoolFld(env, cls,
		moObj, "logging");
	opts->rel_5_0_opts.sam_db = getBoolFld(env, cls,
		moObj, "samDB");
	opts->rel_5_0_opts.xattr = getBoolFld(env, cls,
		moObj, "xattr");

	PTRACE(2, "jni:MountOptions2mntopts() done");
	return (opts);
}


jobject
fs2FSInfo(JNIEnv *env, void *v_fs) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	fs_t *fs = (fs_t *)v_fs;

	PTRACE(2, "jni:fs2FSInfo(%x,%x)", env, fs);
	cls = (*env)->FindClass(env, BASEPKG"/fs/FSInfo");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IIILjava/lang/String;JZZ"
	    "[L"BASEPKG"/fs/DiskDev;[L"BASEPKG"/fs/DiskDev;"
	    "[L"BASEPKG"/fs/StripedGrp;L"BASEPKG"/fs/MountOptions;"
	    "Ljava/lang/String;JJLjava/lang/String;[L"BASEPKG"/fs/Host;I"
	    "Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(fs->fi_name),
	    (jint)fs->fi_eq,
	    (jint)SMALLDAU,
	    (jint)fs->dau,
	    JSTRING(fs->equ_type),
	    (jlong)fs->ctime,
	    JBOOL(fs->fi_archiving),
	    JBOOL(fs->fi_shared_fs),
	    lst2jarray(env, fs->meta_data_disk_list,
		BASEPKG"/fs/DiskDev", dsk2DiskDev),
	    lst2jarray(env, fs->data_disk_list,
		BASEPKG"/fs/DiskDev", dsk2DiskDev),
	    lst2jarray(env, fs->striped_group_list,
		BASEPKG"/fs/StripedGrp", sgrp2StripedGrp),
	    mntopts2MountOptions(env, fs->mount_options),
	    JSTRING(fs->fi_mnt_point),
	    (jlong)(fs->fi_capacity/1024),
	    (jlong)(fs->fi_space/1024),
	    JSTRING(fs->fi_server),
	    lst2jarray(env, fs->hosts_config,
		BASEPKG"/fs/Host", host2Host),
	    (jint)(fs->fi_status),
	    JSTRING(fs->nfs));
	PTRACE(2, "jni:fs2FSInfo() done");
	return (newObj);
}


fs_t *
FSInfo2fs(JNIEnv *env, jobject fsObj) {

	jclass cls;
	fs_t *fs = (fs_t *)malloc(sizeof (fs_t));

	PTRACE(2, "jni:FSInfo2fs() entry");
	cls = (*env)->GetObjectClass(env, fsObj);
	getStrFld(env, cls, fsObj, "name", fs->fi_name);
	fs->fi_eq = (int)getJIntFld(env, cls, fsObj, "equ");
	fs->dau = (ushort_t)getJIntFld(env, cls, fsObj, "dau");
	getStrFld(env, cls, fsObj, "eqType", fs->equ_type);
	fs->ctime = (time_t)getJLongFld(env, cls, fsObj, "time");
	fs->fi_archiving = getBoolFld(env, cls, fsObj, "archiving");
	fs->fi_shared_fs = B_FALSE;	// cannot currently by set by GUI
	fs->meta_data_disk_list = jarray2lst(env,
	    getJArrFld(env,
		cls, fsObj, "metadataDevs", "[L"BASEPKG"/fs/DiskDev;"),
	    BASEPKG"/fs/DiskDev",
	    DiskDev2dsk);
	fs->data_disk_list = jarray2lst(env,
	    getJArrFld(env,
		cls, fsObj, "dataDevs", "[L"BASEPKG"/fs/DiskDev;"),
	    BASEPKG"/fs/DiskDev",
	    DiskDev2dsk);
	fs->striped_group_list = jarray2lst(env,
	    getJArrFld(env,
		cls, fsObj, "stripedGrps", "[L"BASEPKG"/fs/StripedGrp;"),
	    BASEPKG"/fs/StripedGrp",
	    StripedGrp2sgrp);
	fs->mount_options = MountOptions2mntopts(env,
	    getObjFld(env, cls, fsObj, "opts", "L"BASEPKG"/fs/MountOptions;"));
	getStrFld(env, cls, fsObj, "mntPoint", fs->fi_mnt_point);
	fs->fi_capacity = 1024 * getJLongFld(env, cls, fsObj, "kbytesTotal");
	fs->fi_space = 1024 * getJLongFld(env, cls, fsObj, "kbytesAvail");
	getStrFld(env, cls, fsObj, "sharedFsServerName", fs->fi_server);
	fs->hosts_config = jarray2lst(env,
	    getJArrFld(env,
		cls, fsObj, "hosts", "[L"BASEPKG"/fs/Host;"),
	    BASEPKG"/fs/Host",
	    Host2host);
	fs->fi_status = getJIntFld(env, cls, fsObj, "statusFlags");
	getStrFld(env, cls, fsObj, "nfsShareState", fs->nfs);

	PTRACE(3, "jni:%s|%s|%d|%d|%s|%s|%s|mdev[%d]|ddev[%d].",
	    Str(fs->fi_name), Str(fs->nfs), fs->fi_eq, fs->dau,
	    Str(fs->equ_type), Str(fs->fi_mnt_point), Str(fs->fi_server),
	    (NULL == fs->meta_data_disk_list) ? -1 :
		fs->meta_data_disk_list->length,
	    (NULL == fs->data_disk_list) ? -1 : fs->data_disk_list->length);
	PTRACE(2, "jni:FSInfo2fs() done");
	return (fs);
}

void *
FSArchCFG2fs_arch_cfg(JNIEnv *env, jobject facObj) {

	jclass cls;
	fs_arch_cfg_t *a = (fs_arch_cfg_t *)malloc(sizeof (fs_arch_cfg_t));

	PTRACE(2, "jni:FSArchCFG2fs_arch_cfg() entry");
	cls = (*env)->GetObjectClass(env, facObj);
	getStrFld(env, cls, facObj, "setName", a->set_name);
	getStrFld(env, cls, facObj, "logPath", a->log_path);
	a->copies = jarray2lst(env, getJArrFld(env,
		cls, facObj, "copies", "[L"BASEPKG"/arc/Copy;"),
	    BASEPKG"/arc/Copy",
	    Copy2copy);
	a->vsn_maps = jarray2lst(env,
	    getJArrFld(env,
		cls, facObj, "maps", "[L"BASEPKG"/arc/VSNMap;"),
	    BASEPKG"/arc/VSNMap",
	    VSNMap2map);

	PTRACE(3, "jni:%s|%s|vsn_maps[%d]|copies[%d]|",
	    Str(a->set_name), Str(a->log_path),
	    (NULL == a->vsn_maps) ? -1 :
		a->vsn_maps->length,
	    (NULL == a->copies) ? -1 : a->copies->length);
	PTRACE(2, "jni:FSArchCFG2fs_arch_cfg() done");
	return (a);

}



jobject
samfsckinfo2SamfsckJob(JNIEnv *env, void *v_fsckinfo) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	samfsck_info_t *fsckinfo = (samfsck_info_t *)v_fsckinfo;

	PTRACE(2, "jni:samfsckinfo2SamfsckJob() entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/SamfsckJob");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;CLjava/lang/String;JJZ)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(fsckinfo->fsname),
	    (jchar)fsckinfo->state,
	    JSTRING(fsckinfo->user),
	    (jlong)fsckinfo->pid,
	    (jlong)fsckinfo->stime,
	    JBOOL(fsckinfo->repair));
	PTRACE(2, "jni:samfsckinfo2SamfsckJob() done");
	return (newObj);
}


/* native functions implementation */


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_get(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jobject newObj;
	jboolean isCopy; /* used by string macros */
	fs_t *fs;
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:FS_get(...,%s)", Str(cstr));
	if (-1 == get_fs(CTX, cstr, &fs)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	newObj = fs2FSInfo(env, fs);

	REL_STR(fsName, cstr, isCopy);
	free_fs(fs);
	PTRACE(1, "jni:FS_get(...,%s) done", Str(cstr));
	return (newObj);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *fslst;
	int res;

	PTRACE(1, "jni:FS_getAll() entry");
	if (-1 == (res = get_all_fs(CTX, &fslst))) {
		ThrowEx(env);
		return (NULL);
	}
	if (-2 == res) {
	    PTRACE(1, "jni:get_all_fs: -2");
	}

	newArr = lst2jarray(env, fslst, BASEPKG"/fs/FSInfo", fs2FSInfo);
	free_list_of_fs(fslst);
	PTRACE(1, "jni:FS_getAll() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getNames(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *fsnameslst;

	PTRACE(1, "jni:FS_getNames()");
	if (-1 == get_fs_names(CTX, &fsnameslst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, fsnameslst, "java/lang/String", charr2String);
	lst_free_deep(fsnameslst);
	PTRACE(1, "jni:FS_getNames() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getNamesAllTypes(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *fsnameslst;

	PTRACE(1, "jni:FS_getNamesAllTypes()");
	if (-1 == get_fs_names_all_types(CTX, &fsnameslst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, fsnameslst, "java/lang/String", charr2String);
	lst_free_deep(fsnameslst);
	PTRACE(1, "jni:FS_getNamesAllTypes() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getGenericFilesystems(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring filter) {

	jobjectArray newArr;
	sqm_lst_t *fslst;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(filter, isCopy);


	PTRACE(1, "jni:FS_getGenericFilesystems()");
	if (-1 == get_generic_filesystems(CTX, cstr, &fslst)) {
		REL_STR(filter, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(filter, cstr, isCopy);
	newArr = lst2jarray(env, fslst, "java/lang/String", charr2String);
	lst_free_deep(fslst);
	PTRACE(1, "jni:FS_getGenericFilesystems() done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_create(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject fsInfo,
    jboolean mntAtBoot) {

	fs_t *fs;

	PTRACE(1, "jni:FS_create() entry");
	fs = (fs_t *)FSInfo2fs(env, fsInfo);
	if (-1 == create_fs(CTX, fs, CBOOL(mntAtBoot))) {
		ThrowEx(env);
		return;
	}
	free_fs(fs);
	PTRACE(1, "jni:FS_create() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_createAndMount(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject fsInfo,
    jboolean mntAtBoot, jboolean createMntPt, jboolean mount) {

	fs_t *fs;
	int res;

	PTRACE(1, "jni:FS_createAndMount() entry");
	fs = (fs_t *)FSInfo2fs(env, fsInfo);
	res = create_fs_and_mount(CTX, fs,
	    CBOOL(mntAtBoot), CBOOL(createMntPt), CBOOL(mount));
	free_fs(fs);
	switch (res) {
	case -1:
	    ThrowEx(env);
	    return;
	case 0:
	    // success. do nothing
	    break;
	default:
	    ThrowMultiStepOpEx(env, res);
	    return;
	}
	PTRACE(1, "jni:FS_createAndMount() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_createArchFS(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject fsInfo, jboolean mntAtBoot,
    jboolean createMntPt, jboolean mount, jobject FSArchCfg) {

	fs_t *fs;
	fs_arch_cfg_t *arch_cfg;
	int res;

	PTRACE(1, "jni:FS_createAndMount() entry");
	fs = (fs_t *)FSInfo2fs(env, fsInfo);
	arch_cfg = (fs_arch_cfg_t *)FSArchCFG2fs_arch_cfg(env, FSArchCfg);

	res = create_arch_fs(CTX, fs,
	    CBOOL(mntAtBoot), CBOOL(createMntPt), CBOOL(mount), arch_cfg);
	free_fs(fs);
	switch (res) {
	case -1:
	    ThrowEx(env);
	    return;
	case 0:
	    // success. do nothing
	    break;
	default:
	    ThrowMultiStepOpEx(env, res);
	    return;
	}
	PTRACE(1, "jni:FS_createAndMount() done");
}




JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_remove(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	int res;

	PTRACE(1, "jni:FS_remove(...,%s)", cstr);
	res = remove_fs(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_remove() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_mount(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	int res;

	PTRACE(1, "jni:FS_mount(...,%s) entry", Str(cstr));
	res = mount_fs(CTX, cstr);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_mount(...,%s) done", Str(cstr));
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_umount(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	int res;

	PTRACE(1, "jni:FS_umount(...,%s) entry", Str(cstr));
	res = umount_fs(CTX, cstr, B_FALSE);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_umount(...,%s) done", Str(cstr));
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_grow(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject fsObj,
    jobjectArray mdDsks, jobjectArray dDsks, jobjectArray Grps) {

	int ret;
	PTRACE(1, "jni:FS_grow() entry");
	ret == grow_fs(CTX,
	    FSInfo2fs(env, fsObj),
	    jarray2lst(env, mdDsks, BASEPKG"/fs/DiskDev", DiskDev2dsk),
	    jarray2lst(env, dDsks, BASEPKG"/fs/DiskDev", DiskDev2dsk),
	    jarray2lst(env, Grps, BASEPKG"/fs/StripedGrp", StripedGrp2sgrp));

	if (ret == -1) {
		ThrowEx(env);
		return (-1);
	}

	PTRACE(1, "jni:FS_grow() done");
	return (ret);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_fsck(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jstring log,
    jboolean repair) {

	jboolean isCopy, isCopy2;
	char *fsname = GET_STR(fsName, isCopy),
	    *logname = GET_STR(log, isCopy2);
	int res;

	PTRACE(1, "jni:FS_fsck() entry");
	res = samfsck_fs(CTX, fsname, logname, CBOOL(repair));
	REL_STR(fsName, fsname, isCopy);
	REL_STR(log, logname, isCopy2);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_fsck() done");
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getDefaultMountOpts(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx,
    jstring fsType, jint dauSz,
    jboolean stripedGrps, jboolean shared, jboolean multiReader) {

	jobject newObj;
	jboolean isCopy;
	char *cstr = GET_STR(fsType, isCopy);
	mount_options_t *mopts;
	int res;

	PTRACE(1, "jni:FS_getDefaultMountOpts()");
	res = get_default_mount_options(CTX, cstr,
	    (int)dauSz, CBOOL(stripedGrps), CBOOL(shared), CBOOL(multiReader),
	    &mopts);
	REL_STR(fsType, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = mntopts2MountOptions(env, mopts);
	free(mopts);
	PTRACE(1, "jni:FS_getDefaultMountOpts() done");
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_setMountOpts(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jobject mntOpts) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	mount_options_t *mntopts;
	int res;

	PTRACE(1, "jni:FS_setMountOpts(...,%s) entry", Str(cstr));
	mntopts = MountOptions2mntopts(env, mntOpts);
	res = change_mount_options(CTX,
	    cstr,  mntopts);
	free(mntopts);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_setMountOpts(...,%s) done", Str(cstr));
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_setLiveMountOpts(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jobject mntOpts) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	mount_options_t *mntopts;
	int res;
	sqm_lst_t *failedopts;

	PTRACE(1, "jni:FS_setLiveMountOpts(...,%s) entry", Str(cstr));
	mntopts = MountOptions2mntopts(env, mntOpts);
	res = change_live_mount_options(CTX, cstr, mntopts, &failedopts);
	free(mntopts);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}

	lst_free_deep(failedopts);
	PTRACE(1, "jni:FS_setLiveMountOpts(...,%s) done", Str(cstr));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_SamfsckJob_getAll(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *fscklst;

	PTRACE(1, "jni:SamfsckJob_getAll() entry");
	if (-1 == get_all_samfsck_info(CTX, &fscklst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, fscklst, BASEPKG"/fs/SamfsckJob",
	    samfsckinfo2SamfsckJob);
	lst_free_deep(fscklst);
	PTRACE(1, "jni:SamfsckJob_getAll() done");
	return (newArr);

}

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getEqOrdinals(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint num, jintArray in_use) {

	int *first_free = NULL;
	sqm_lst_t *lst_in_use = NULL;

	jclass class;
	jmethodID mid;
	jobject newObj;

	lst_in_use = jintArray2lst(env, in_use);

	PTRACE(1, "jni:FS_getEqOrdinals() entry");
	if (-1 == get_equipment_ordinals(CTX,
	    (int)num,
	    &lst_in_use,
	    &first_free)) {
		ThrowEx(env);
		return (NULL);
	}

	class = (*env)->FindClass(env, BASEPKG"/fs/EQ");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, class, "<init>",
	    "([II)V");

	PTRACE(2, "First free eq[%d], no. of eqs in use[%d]",
	    (first_free == NULL) ? -1 : *first_free,
	    (lst_in_use == NULL) ? -1 : lst_in_use->length);

	newObj = (*env)->NewObject(env, class, mid,
	    lst2jintArray(env, lst_in_use),
	    (jint)(*first_free));

	lst_free(lst_in_use);
	if (first_free != NULL) {
		free(first_free);
	}

	PTRACE(1, "jni:jni:FS_getEqOrdinals() done");
	return (newObj);

}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_checkEqOrdinals(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jintArray eqs) {


	sqm_lst_t *lst;

	PTRACE(1, "jni:FS_checkEqOrdinals() entry");
	lst = jintArray2lst(env, eqs);
	if (-1 == check_equipment_ordinals(CTX, lst)) {
		ThrowEx(env);
		return;
	}

	lst_free(lst);
	PTRACE(1, "jni:jni:FS_checkEqOrdinals() done");

}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_resetEqOrdinals(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jintArray eqs) {


	sqm_lst_t *lst;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:FS_resetEqOrdinals(...,%s) entry", Str(cstr));
	lst = jintArray2lst(env, eqs);
	if (-1 == reset_equipment_ordinals(CTX, cstr, lst)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(fsName, cstr, isCopy);
	lst_free(lst);
	PTRACE(1, "jni:jni:FS_resetEqOrdinals() done");

}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_setDeviceState(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName,
    jint state, jintArray eqs) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	sqm_lst_t *lst;
	int res;

	PTRACE(1, "jni:FS_setDeviceState(...,%s)", cstr);
	lst = jintArray2lst(env, eqs);
	res = set_device_state(CTX, cstr, state, lst);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_setDeviceState() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_removeGenericFS(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jstring type) {

	jboolean isCopy;
	jboolean isCopy2;
	char *cstr = GET_STR(fsName, isCopy);
	char *tstr = GET_STR(type, isCopy2);
	int res;

	PTRACE(1, "jni:FS_removeByType(...,%s)", cstr);
	res = remove_generic_fs(CTX, cstr, tstr);
	REL_STR(fsName, cstr, isCopy);
	REL_STR(type, tstr, isCopy2);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_removeByType() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_mountByType(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jstring type) {

	jboolean isCopy;
	jboolean isCopy2;
	char *cstr = GET_STR(fsName, isCopy);
	char *tstr = GET_STR(type, isCopy2);
	int res;

	PTRACE(1, "jni:FS_mountByType(...,%s) entry", Str(cstr));
	res = mount_generic_fs(CTX, cstr, tstr);
	REL_STR(fsName, cstr, isCopy);
	REL_STR(type, tstr, isCopy2);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FS_mountByType() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_setNFSOptions(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring mntPoint, jstring opts) {

	jboolean isCopy;
	jboolean isCopy2;
	char *cstr = GET_STR(mntPoint, isCopy);
	char *ostr = GET_STR(opts, isCopy2);
	int res;

	PTRACE(1, "jni:FS_setNFSOpts(...,%s) entry", Str(cstr));
	res = set_nfs_opts(CTX, cstr, ostr);
	REL_STR(mntPoint, cstr, isCopy);
	REL_STR(opts, ostr, isCopy2);

	if (-1 == res) {
		ThrowEx(env);
		return;
	}

	PTRACE(1, "jni:FS_setNFSOpts() done");

}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getNFSOptions(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring mntPoint) {

	jboolean isCopy;
	char *cstr = GET_STR(mntPoint, isCopy);
	int res;
	jobjectArray newArr;
	sqm_lst_t *lst = NULL;

	PTRACE(1, "jni:FS_getNFSOpts(...,%s) entry", Str(cstr));
	res = get_nfs_opts(CTX, cstr, &lst);
	REL_STR(mntPoint, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(1, "jni:FS_getNFSOpts() done");
	return (newArr);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getFileMetrics(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint which,
    jlong Start, jlong End) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	int res;
	char *buf = NULL;

	PTRACE(1, "jni:FS_getFileMetrics(...,%s) entry", Str(cstr));
	res = get_file_metrics_report(CTX, cstr, which, (time_t)Start,
		(time_t)End, &buf);
	REL_STR(fsName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_getFileMetrics() done");
	return (JSTRING(buf));
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_shrinkRelease(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint eqToRelease,
    jstring kvOptions) {

	jboolean isCopy1;
	jboolean isCopy2;
	char *cstr1 = GET_STR(fsName, isCopy1);
	char *cstr2 = GET_STR(kvOptions, isCopy2);
	int ret;


	PTRACE(1, "jni:FS_shrinkRelease(%s...) entry", Str(cstr1));

	ret = shrink_release(CTX, cstr1, (int)eqToRelease, cstr2);

	REL_STR(fsName, cstr1, isCopy1);
	REL_STR(kvOptions, cstr2, isCopy2);
	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_shrinkRelease() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_shrinkRemove(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint eqToRemove,
    jint replacementEq, jstring kvOptions) {

	jboolean isCopy1;
	jboolean isCopy2;
	char *cstr1 = GET_STR(fsName, isCopy1);
	char *cstr2 = GET_STR(kvOptions, isCopy2);
	int ret;


	PTRACE(1, "jni:FS_shrinkRemove(%s...) entry", Str(cstr1));

	ret = shrink_remove(CTX, cstr1, (int)eqToRemove,
	    (int)replacementEq, cstr2);

	REL_STR(fsName, cstr1, isCopy1);
	REL_STR(kvOptions, cstr2, isCopy2);
	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_shrinkRemove() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_shrinkReplaceDev(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint eqToReplace,
    jobject replacementDisk, jstring kvOptions) {

	jboolean isCopy1;
	jboolean isCopy2;
	char *cstr1 = GET_STR(fsName, isCopy1);
	char *cstr2 = GET_STR(kvOptions, isCopy2);
	disk_t *dsk;
	int ret;


	PTRACE(1, "jni:FS_shrinkReplaceDev(%s...) entry", Str(cstr1));

	dsk = (disk_t *)DiskDev2dsk(env, replacementDisk);
	if (dsk == NULL) {
		REL_STR(fsName, cstr1, isCopy1);
		REL_STR(kvOptions, cstr2, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	ret = shrink_replace_device(CTX, cstr1,
	    (int)eqToReplace, dsk, cstr2);

	REL_STR(fsName, cstr1, isCopy1);
	REL_STR(kvOptions, cstr2, isCopy2);
	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_shrinkReplaceDev() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_shrinkReplaceGroup(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint eqToReplace,
    jobject replacementGroup, jstring kvOptions) {

	jboolean isCopy1;
	jboolean isCopy2;
	char *cstr1 = GET_STR(fsName, isCopy1);
	char *cstr2 = GET_STR(kvOptions, isCopy2);
	striped_group_t *grp;
	int ret;


	PTRACE(1, "jni:FS_shrinkReplaceDev(%s...) entry", Str(cstr1));

	grp = (striped_group_t *)StripedGrp2sgrp(env, replacementGroup);
	if (grp == NULL) {
		REL_STR(fsName, cstr1, isCopy1);
		REL_STR(kvOptions, cstr2, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	ret = shrink_replace_group(CTX, cstr1, (int)eqToReplace,
	    grp, cstr2);

	REL_STR(fsName, cstr1, isCopy1);
	REL_STR(kvOptions, cstr2, isCopy2);
	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_shrinkReplaceDev() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_mountClients(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName,
    jobjectArray clients) {

	jboolean isCopy1;
	char *cstr1 = GET_STR(fsName, isCopy1);
	int ret;
	char **c_clients;
	int len;
	int i;

	PTRACE(1, "jni:FS_mountClients(%s...) entry", Str(cstr1));

	/* Determine the array length */
	len = (int)(*env)->GetArrayLength(env, clients);


	/* convert the array */
	c_clients = (char **)jarray2arrOfPtrs(env, clients,
	    "java/lang/String", String2charr);

	if (c_clients == NULL) {
		REL_STR(fsName, cstr1, isCopy1);
		ThrowEx(env);
		return (NULL);
	}

	ret = mount_clients(CTX, cstr1, c_clients, len);

	REL_STR(fsName, cstr1, isCopy1);
	for (i = 0; i < len; i++) {
		free(c_clients[i]);
	}
	free(c_clients);

	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}


	PTRACE(1, "jni:FS_mountClients() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_unmountClients(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName,
    jobjectArray clients) {

	jboolean isCopy1;
	char *cstr1 = GET_STR(fsName, isCopy1);
	int ret;
	char **c_clients;
	int len;
	int i;

	PTRACE(1, "jni:FS_unmountClients(%s...) entry", Str(cstr1));

	/* Determine the array length */
	len = (int)(*env)->GetArrayLength(env, clients);


	/* convert the array */
	c_clients = (char **)jarray2arrOfPtrs(env, clients,
	    "java/lang/String", String2charr);

	if (c_clients == NULL) {
		REL_STR(fsName, cstr1, isCopy1);
		ThrowEx(env);
		return (NULL);
	}

	ret = unmount_clients(CTX, cstr1, c_clients, len);

	REL_STR(fsName, cstr1, isCopy1);
	for (i = 0; i < len; i++) {
		free(c_clients[i]);
	}
	free(c_clients);

	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}


	PTRACE(1, "jni:FS_unmountClients() done");
	return (ret);
}


JNIEXPORT jint JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_setSharedFSMountOptions(JNIEnv *env,
jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jobjectArray clients,
jobject mountOpts) {

	jboolean isCopy1;
	char *cstr1 = GET_STR(fsName, isCopy1);
	char **c_clients;
	mount_options_t *mount_opts;
	int len;
	int i;
	int ret;

	PTRACE(1, "jni:FS_setSharedFSMountOptions(%s...) entry", Str(cstr1));

	/* Determine the array length */
	len = (int)(*env)->GetArrayLength(env, clients);


	/* convert the array */
	c_clients = (char **)jarray2arrOfPtrs(env, clients,
	    "java/lang/String", String2charr);

	if (c_clients == NULL) {
		REL_STR(fsName, cstr1, isCopy1);
		ThrowEx(env);
		return (NULL);
	}
	mount_opts = MountOptions2mntopts(env, mountOpts);

	ret = change_shared_fs_mount_options(CTX, cstr1, c_clients, len,
	    mount_opts);

	REL_STR(fsName, cstr1, isCopy1);
	for (i = 0; i < len; i++) {
		free(c_clients[i]);
	}
	free(c_clients);
	free(mount_opts);

	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FS_setSharedFSMountOptions() done");
	return (ret);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_FS_getSharedFSSummaryStatus(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {


	jboolean isCopy1;
	char *cstr1 = GET_STR(fsName, isCopy1);
	sqm_lst_t *status_lst;
	jobjectArray newArr;
	int ret;



	PTRACE(1, "jni:FS_getSharedFSSummaryStatus(%s...) entry", Str(cstr1));

	ret = get_shared_fs_summary_status(CTX, cstr1, &status_lst);

	REL_STR(fsName, cstr1, isCopy1);
	if (-1 == ret) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, status_lst, "java/lang/String", charr2String);
	lst_free_deep(status_lst);
	PTRACE(1, "jni:FS_getSharedFSSummaryStatus() done");
	return (newArr);
}
