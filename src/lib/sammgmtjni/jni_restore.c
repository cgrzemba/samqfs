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
#pragma ident	"$Revision: 1.20 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/restore.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_fs_Restore.h"
#include "jni_util.h"


/* native functions implementation */

JNIEXPORT void  JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_setParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
    jstring parameters) {

	jboolean isCopy, isCopy2;
	char *cstr = GET_STR(fsname, isCopy);
	char *pstr = GET_STR(parameters, isCopy2);

	PTRACE(1, "jni:Restore_setParams()");
	if (-1 == set_csd_params(CTX, cstr, pstr)) {
		REL_STR(fsname, cstr, isCopy);
		REL_STR(parameters, pstr, isCopy2);
		ThrowEx(env);
		return;
	}
	REL_STR(fsname, cstr, isCopy);
	REL_STR(parameters, pstr, isCopy2);

	PTRACE(1, "jni:Restore_setParams() done");
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getParams(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname) {

	char *str = NULL;
	jboolean isCopy;
	char *cstr = GET_STR(fsname, isCopy);

	PTRACE(1, "jni:Restore_getParams(..., %s)", Str(cstr));
	if (-1 == get_csd_params(CTX, cstr, &str)) {
		REL_STR(fsname, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, cstr, isCopy);

	PTRACE(1, "jni:Restore_getParams() done %s", Str(str));

	return (JSTRING(str));
}

// getDumps(fsname);
// Calls list_dumps
JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getDumps__Lcom_sun_netstorage_samqfs_mgmt_Ctx_2Ljava_lang_String_2
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname) {

	sqm_lst_t *dumpslst = NULL;
	jobjectArray newArr;
	jboolean isCopy;
	char *cstr = GET_STR(fsname, isCopy);

	PTRACE(1, "jni:Restore_getDumps(...%s) entry", Str(cstr));
	if (-1 == list_dumps(CTX, cstr, &dumpslst)) {
		REL_STR(fsname, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, cstr, isCopy);

	newArr = lst2jarray(env, dumpslst, "java/lang/String", charr2String);
	lst_free_deep(dumpslst);
	PTRACE(1, "jni:Restore_getDumps() done");
	return (newArr);

}

// getDumps(fsname, directory);
// Calls list_dumps_by_dir
JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getDumps__Lcom_sun_netstorage_samqfs_mgmt_Ctx_2Ljava_lang_String_2Ljava_lang_String_2
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
	jstring directory) {

	sqm_lst_t *dumpslst = NULL;
	jobjectArray newArr;
	jboolean isFsNameCopy;
	jboolean isDirectoryCopy;
	char *cstr_fsname = GET_STR(fsname, isFsNameCopy);
	char *cstr_directory = GET_STR(directory, isDirectoryCopy);

	PTRACE(1, "jni:Restore_getDumps(...%s, %s) entry", Str(cstr_fsname),
		Str(cstr_directory));
	if (-1 == list_dumps_by_dir(CTX, cstr_fsname, cstr_directory,
		&dumpslst)) {

		REL_STR(fsname, cstr_fsname, isFsNameCopy);
		REL_STR(directory, cstr_directory, isDirectoryCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, cstr_fsname, isFsNameCopy);
	REL_STR(directory, cstr_directory, isDirectoryCopy);

	newArr = lst2jarray(env, dumpslst, "java/lang/String", charr2String);
	lst_free_deep(dumpslst);
	PTRACE(1, "jni:Restore_getDumps() done");
	return (newArr);

}

// getDumpStatus(fsName);
// Calls get_Dump_Status
JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getDumpStatus__Lcom_sun_netstorage_samqfs_mgmt_Ctx_2Ljava_lang_String_2_3Ljava_lang_String_2
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
	jobjectArray dumps) {

	sqm_lst_t *statuslst = NULL;
	jobjectArray newArr;
	jboolean isCopy;
	char *cstr = GET_STR(fsname, isCopy);

	PTRACE(1, "jni:Restore_getDumpStatus() entry");
	if (-1 == get_dump_status(CTX, cstr,
	    jarray2lst(env, dumps, "java/lang/String", String2charr),
	    &statuslst)) {
		REL_STR(fsname, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, cstr, isCopy);
	newArr = lst2jarray(env, statuslst, "java/lang/String", charr2String);
	lst_free_deep(statuslst);

	PTRACE(1, "jni:Restore_getDumpStatus() done");
	return (newArr);
}

// getDumpStatus(fsName, directory);
// Calls get_Dump_Status_by_dir
JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getDumpStatus__Lcom_sun_netstorage_samqfs_mgmt_Ctx_2Ljava_lang_String_2Ljava_lang_String_2_3Ljava_lang_String_2
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
	jstring directory, jobjectArray dumps) {

	sqm_lst_t *statuslst = NULL;
	jobjectArray newArr;
	jboolean isFsNameCopy;
	jboolean isDirectoryCopy;
	char *cstr_fsname = GET_STR(fsname, isFsNameCopy);
	char *cstr_directory = GET_STR(directory, isDirectoryCopy);

	PTRACE(1, "jni:Restore_getDumpStatus() entry");
	if (-1 == get_dump_status_by_dir(CTX, cstr_fsname, cstr_directory,
	    jarray2lst(env, dumps, "java/lang/String", String2charr),
	    &statuslst)) {
		REL_STR(fsname, cstr_fsname, isFsNameCopy);
		REL_STR(directory, cstr_directory, isDirectoryCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, cstr_fsname, isFsNameCopy);
	REL_STR(directory, cstr_directory, isDirectoryCopy);
	newArr = lst2jarray(env, statuslst, "java/lang/String", charr2String);
	lst_free_deep(statuslst);

	PTRACE(1, "jni:Restore_getDumpStatus() done");
	return (newArr);
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_decompressDump(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath) {

	char *str = NULL;

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);

	PTRACE(1, "jni:Restore_decompressDump(...,%s, %s) entry",
	    Str(fstr), Str(dstr));
	if (-1 == decompress_dump(CTX, fstr, dstr, &str)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);

	PTRACE(1, "jni:Restore_decompressDump() done");
	return (JSTRING(str));
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_cleanDump(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath) {

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);

	PTRACE(1, "jni:Restore_cleanDump(...,%s, %s) entry",
	    Str(fstr), Str(dstr));
	if (-1 == cleanup_dump(CTX, fstr, dstr)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		ThrowEx(env);
		return;
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);

	PTRACE(1, "jni:Restore_cleanDump() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_deleteDump(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath) {

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);

	PTRACE(1, "jni:Restore_deleteDump(...,%s, %s) entry",
	    Str(fstr), Str(dstr));
	if (-1 == delete_dump(CTX, fstr, dstr)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		ThrowEx(env);
		return;
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);

	PTRACE(1, "jni:Restore_deleteDump() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_setIsDumpRetainedPermanently
	(JNIEnv *env, jclass cls, /*ARGSUSED*/ jobject ctx,
	jstring dumpPath, jboolean retainValue) {

	jboolean isCopy;
	char *pDumpPathStr = GET_STR(dumpPath, isCopy);

	PTRACE(1, "jni:Restore_setIsDumpRetainedPermanently(...,%s,%d) entry",
	Str(pDumpPathStr), retainValue);

	if (JNI_TRUE == retainValue) {
	// Call setter
		if (-1 == set_snapshot_locked(CTX, pDumpPathStr)) {
			REL_STR(dumpPath, pDumpPathStr, isCopy);
			ThrowEx(env);
		return;
		}
	} else {
	// Call clearer
	if (-1 == clear_snapshot_locked(CTX, pDumpPathStr)) {
		REL_STR(dumpPath, pDumpPathStr, isCopy);
		ThrowEx(env);
		return;
	}
	}

    REL_STR(dumpPath, pDumpPathStr, isCopy);

    PTRACE(1, "jni:Restore_setIsDumpRetainedPermanently() done");
}

JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_takeDump(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath) {

	char *str = NULL;

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);

	PTRACE(1, "jni:Restore_takeDump(...,%s, %s) entry",
	    Str(fstr), Str(dstr));
	if (-1 == take_dump(CTX, fstr, dstr, &str)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);

	PTRACE(1, "jni:Restore_takeDump() done");
	return (JSTRING(str));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_listVersions(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath,
    jint entries, jstring filepath, jstring restrictions) {

	jobjectArray newArr;
	sqm_lst_t *versionlst = NULL;

	jboolean isCopy, isCopy2, isCopy3, isCopy4;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);
	char *cstr = GET_STR(filepath, isCopy3);
	char *rstr = GET_STR(restrictions, isCopy4);

	PTRACE(1, "jni:Restore_listVersions(...,%s, %s) entry",
	    Str(cstr), Str(rstr));
	if (-1 == list_versions(CTX,
	    fstr, dstr, (int)entries, cstr, rstr, &versionlst)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		REL_STR(filepath, cstr, isCopy3);
		REL_STR(restrictions, rstr, isCopy4);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);
	REL_STR(filepath, cstr, isCopy3);
	REL_STR(restrictions, rstr, isCopy4);

	newArr = lst2jarray(env, versionlst, "java/lang/String", charr2String);
	lst_free_deep(versionlst);
	PTRACE(1, "jni:Restore_listVersions() done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getVersionDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
    jstring dumppath, jstring filepath) {

	sqm_lst_t *detailslst = NULL;
	jobjectArray newArr;

	jboolean isCopy, isCopy2, isCopy3;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);
	char *cstr = GET_STR(filepath, isCopy3);

	PTRACE(1, "jni:Restore_getVersionDetails(...,%s)", Str(cstr));
	if (-1 == get_version_details(CTX, fstr, dstr, cstr, &detailslst)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		REL_STR(filepath, cstr, isCopy3);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);
	REL_STR(filepath, cstr, isCopy3);

	newArr = lst2jarray(env, detailslst, "java/lang/String", charr2String);
	lst_free_deep(detailslst);

	PTRACE(1, "jni:Restore_getVersionDetails() done");
	return (newArr);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_searchFiles(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath,
    jint entries, jstring filepath, jstring restrictions) {

	char *str = NULL;

	jboolean isCopy, isCopy2, isCopy3, isCopy4;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);
	char *cstr = GET_STR(filepath, isCopy3);
	char *rstr = GET_STR(restrictions, isCopy4);

	PTRACE(1, "jni:Restore_searchFiles(...,%s, %s) entry",
	    Str(cstr), Str(rstr));
	if (-1 == search_versions(CTX,
	    fstr, dstr, (int)entries, cstr, rstr, &str)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		REL_STR(filepath, cstr, isCopy3);
		REL_STR(restrictions, rstr, isCopy4);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);
	REL_STR(filepath, cstr, isCopy3);
	REL_STR(restrictions, rstr, isCopy4);

	PTRACE(1, "jni:Restore_searchFiles() done");
	return (JSTRING(str));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getSearchResults(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname) {

	jobjectArray newArr;
	sqm_lst_t *versionlst = NULL;
	jboolean isCopy;
	char *fstr = GET_STR(fsname, isCopy);

	PTRACE(1, "jni:Restore_getSearchResults(..., %s)", Str(fstr));
	if (-1 == get_search_results(CTX, fstr, &versionlst)) {
		REL_STR(fsname, fstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);

	newArr = lst2jarray(env, versionlst, "java/lang/String", charr2String);
	lst_free_deep(versionlst);

	PTRACE(1, "jni:Restore_getSearchResults() done");
	return (newArr);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_restoreInodes(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring dumppath,
    jobjectArray filepaths, jobjectArray destinations, jintArray copies,
    jint replaceType) {

	char *str = NULL;

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(dumppath, isCopy2);

	PTRACE(1, "jni:Restore_restoreInodes() entry");

	if (-1 == restore_inodes(CTX, fstr,
	    dstr, jarray2lst(env, filepaths, "java/Lang/String", String2charr),
	    jarray2lst(env, destinations, "java/Lang/String", String2charr),
	    jintArray2lst(env, copies), replaceType,
	    &str)) {

		REL_STR(fsname, fstr, isCopy);
		REL_STR(dumppath, dstr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}
	REL_STR(fsname, fstr, isCopy);
	REL_STR(dumppath, dstr, isCopy2);
	PTRACE(1, "jni:Restore_restoreInodes() done");
	return (JSTRING(str));

}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getIndexDirs(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname) {

	jboolean isCopy;
	char *fstr = GET_STR(fsname, isCopy);
	sqm_lst_t *dirlist = NULL;
	jobjectArray newArr;

	PTRACE(1, "jni:Restore_getIndexDirs(..., %s) entry", Str(fstr));
	if (-1 == get_indexed_snapshot_directories(CTX, fstr, &dirlist)) {
		REL_STR(fsname, fstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);

	newArr = lst2jarray(env, dirlist, "java/lang/String", charr2String);
	lst_free_deep(dirlist);

	PTRACE(1, "jni:Restore_getIndexDirs() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Restore_getIndexedSnaps(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring snapdir) {

	jboolean isCopy, isCopy2;
	char *fstr = GET_STR(fsname, isCopy);
	char *dstr = GET_STR(snapdir, isCopy2);
	sqm_lst_t *snaplist = NULL;
	jobjectArray newArr;

	PTRACE(1, "jni:Restore_getIndexedSnaps(..., %s) entry", Str(fstr));
	if (-1 == get_indexed_snapshots(CTX, fstr, dstr, &snaplist)) {
		REL_STR(fsname, fstr, isCopy);
		REL_STR(snapdir, dstr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);
	REL_STR(snapdir, dstr, isCopy2);

	newArr = lst2jarray(env, snaplist, "java/lang/String", charr2String);
	lst_free_deep(snaplist);

	PTRACE(1, "jni:Restore_getIndexedSnaps() done");
	return (newArr);
}
