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

#pragma ident	"$Id"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>

/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/log.h"

/* local header files */

#include "com_sun_netstorage_samqfs_mgmt_GetList.h"
#include "com_sun_netstorage_samqfs_mgmt_FileUtil.h"
#include "jni_util.h"

/* native functions implementation */

/*
 *  Original call to get a list of files in a directory.
 */
JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getDirEntries(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint entries, jstring filepath,
    jstring restrictions) {

	jobjectArray newArr;
	sqm_lst_t *dirlst = NULL;

	jboolean isCopy, isCopy2;
	char *cstr = GET_STR(filepath, isCopy);
	char *rstr = GET_STR(restrictions, isCopy2);

	PTRACE(1, "jni:FileUtil_getDirEntries(...%s, %s) entry",
	    Str(cstr), Str(rstr));
	if (-1 == list_dir(CTX, (int)entries, cstr, rstr, &dirlst)) {
		REL_STR(filepath, cstr, isCopy);
		REL_STR(restrictions, rstr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(filepath, cstr, isCopy);
	REL_STR(restrictions, rstr, isCopy2);

	newArr = lst2jarray(env, dirlst, "java/lang/String", charr2String);
	lst_free_deep(dirlst);
	PTRACE(1, "jni:FileUtil_getDirEntries() done");
	return (newArr);
}

/*
 *  Updated version of getDirEntries.  Returns the first <n> files
 *  that match the criteria specified in alphabetical order.  Returns
 *  also the total number of files in the directory.  The total number
 *  of files can be used by the caller to determine if another call to
 *  the server is required to get all members of a directory.
 *
 *  'relativePath' is the starting point relative to the filesystem
 *  root (mountpoint).
 *
 *  'lastFile' is used as a starting point.  If not set (i.e., NULL),
 *  will start at the first file in the specified directory.  If it is set,
 *  the files returned will start with files that sort immediately after the
 *  one provided.
 *  For example:
 *    first call - get 1024 files from directory "/var":  Set relativePath
 *    to NULL, lastFile to NULL.
 *    total files is > 1024?
 *    second call - starting with last file returned set relativePath to
 *    "/var" and lastFile to the last entry received.
 *    if lastFile == "q", returns file list starting with "/var/q1"
 *
 *  Note that this reads from a live filesystem, and as such the directory
 *  may be changing between calls to the server.  The total number of
 *  files may change between calls.
 */
JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getDirectoryEntries(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jint entries, jstring relativePath,
    jstring lastFile, jstring restrictions) {

	jobjectArray	newArr;
	sqm_lst_t		*dirlst = NULL;
	uint32_t	totalFiles;
	jclass		class;
	jobject		newObj;
	jmethodID	mid;

	jboolean isCopy, isCopy2, isCopy3;
	char *cstr = GET_STR(relativePath, isCopy);
	char *rstr = GET_STR(restrictions, isCopy2);
	char *fstr = GET_STR(lastFile, isCopy3);

	PTRACE(1, "jni:FileUtil_getDirectoryEntries(...%s, %s) entry",
	    Str(cstr), Str(rstr));
	if (-1 == list_directory(CTX, (int)entries, cstr, fstr, rstr,
			&totalFiles, &dirlst)) {
		REL_STR(relativePath, cstr, isCopy);
		REL_STR(restrictions, rstr, isCopy2);
		REL_STR(lastFile, fstr, isCopy3);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(relativePath, cstr, isCopy);
	REL_STR(lastFile, fstr, isCopy3);
	REL_STR(restrictions, rstr, isCopy2);

	class = (*env)->FindClass(env, BASEPKG"/GetList");
	mid = (*env)->GetMethodID(env, class, "<init>",
	    "(I[Ljava/lang/String;)V");

	newArr = lst2jarray(env, dirlst, "java/lang/String", charr2String);

	/* return from server is remaining files, GUI wants total */
	totalFiles += dirlst->length;
	newObj = (*env)->NewObject(env, class, mid, (jint)totalFiles, newArr);

	lst_free_deep(dirlst);
	PTRACE(1, "jni:FileUtil_getDirectoryEntries() done");
	return (newObj);
}


JNIEXPORT jintArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getFileStatus(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobjectArray filepaths) {

	sqm_lst_t *statuslst = NULL;
	jintArray newArr;
	sqm_lst_t *filelst =
	    jarray2lst(env, filepaths, "java/lang/String", String2charr);

	PTRACE(1, "jni:FileUtil_getFileStatus() entry");
	if (-1 == get_file_status(CTX, filelst, &statuslst)) {
		ThrowEx(env);
		lst_free_deep(filelst);
		return (NULL);
	}
	newArr = lst2jintArray(env, statuslst),
	lst_free(statuslst);
	lst_free_deep(filelst);

	PTRACE(1, "jni:FileUtil_getfileStatus() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getFileDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname,
    jobjectArray files) {

	sqm_lst_t *detailslst = NULL;
	sqm_lst_t *filelst =
		jarray2lst(env, files, "java/lang/String", String2charr);
	jobjectArray newArr;

	jboolean isCopy;
	char *fstr = GET_STR(fsname, isCopy);

	PTRACE(1, "jni:FileUtil_getFileDetails(...,%s)", Str(fstr));
	if (-1 == get_file_details(CTX, fstr, filelst,  &detailslst)) {

		REL_STR(fsname, fstr, isCopy);
		lst_free_deep(filelst);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsname, fstr, isCopy);

	newArr = lst2jarray(env, detailslst, "java/lang/String", charr2String);
	lst_free_deep(detailslst);
	lst_free_deep(filelst);

	PTRACE(1, "jni:FileUtil_getFileDetails() done");
	return (newArr);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_createFile(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring file) {

	jboolean isCopy;
	char *cstr = GET_STR(file, isCopy);
	int res;

	PTRACE(1, "jni:FileUtil_createFile(...,%s) entry", Str(cstr));
	res = create_file(CTX, cstr);
	REL_STR(file, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FileUtil_createFile(...,%s) done", Str(cstr));
}

JNIEXPORT jboolean JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_fileExists(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring filePath) {

	jboolean isCopy, exist;
	char *cstr = GET_STR(filePath, isCopy);
	int res;

	PTRACE(3, "jni:FileUtil_fileExists() entry");
	res = file_exists(CTX, cstr);
	REL_STR(filePath, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (JNI_FALSE);
	}
	exist = (res == 0) ? JNI_TRUE : JNI_FALSE;
	PTRACE(3, "jni:FileUtil_fileExists() done, return %d", exist);
	return (exist);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_tailFile(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring filePath, jint number) {

	char **arrOfPtrs = NULL;
	jobjectArray newArr;
	char *cstr = NULL;
	jboolean isCopy;
	uint32_t count = (int)number;

	cstr = GET_STR(filePath, isCopy);
	PTRACE(3, "jni:FileUtil tailFile() entry");
	if (-1 == tail(CTX, cstr, &count,
		&arrOfPtrs, NULL /* data ptr not used */)) {
		REL_STR(filePath, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(filePath, cstr, isCopy);
	newArr = arrOfPtrs2jarray(env, (void**)arrOfPtrs, count,
		"java/lang/String", charr2String);

	free(arrOfPtrs);

	PTRACE(3, "jni:FileUtil tailFile() done");
	return (newArr);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_createDir(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring dir) {

	jboolean isCopy;
	char *cstr = GET_STR(dir, isCopy);
	int res;

	PTRACE(1, "jni:FileUtil_createDir(...,%s) entry", Str(cstr));
	res = create_dir(CTX, cstr);
	REL_STR(dir, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FileUtil_createDir(...,%s) done", Str(cstr));
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getTxtFile(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring filePath, jint start,
    jint number) {

	char **arrOfPtrs = NULL;
	jobjectArray newArr;
	char *cstr = NULL;
	jboolean isCopy;
	uint32_t count = (int)number;

	cstr = GET_STR(filePath, isCopy);
	PTRACE(3, "jni:util tailFile() entry");
	if (-1 == get_txt_file(CTX, cstr, start, &count,
		&arrOfPtrs, NULL /* data ptr not used */)) {
		REL_STR(filePath, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(filePath, cstr, isCopy);
	newArr = arrOfPtrs2jarray(env, (void**)arrOfPtrs, count,
	    "java/lang/String", charr2String);

	free(arrOfPtrs);

	PTRACE(3, "jni:util tailFile() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getExtFileDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobjectArray files,
    jint whichDetails) {

	sqm_lst_t *detailslst = NULL;
	sqm_lst_t *filelst =
		jarray2lst(env, files, "java/lang/String", String2charr);
	jobjectArray newArr;

	jboolean isCopy;

	PTRACE(1, "jni:FileUtil_getExtFileDetails(...)");
	if (-1 == get_extended_file_details(CTX, filelst,
		(int)whichDetails, &detailslst)) {

		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, detailslst, "java/lang/String", charr2String);
	lst_free_deep(detailslst);
	lst_free_deep(filelst);

	PTRACE(1, "jni:FileUtil_getExtFileDetails() done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_getCopyDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring file_path,
    jint whichDetails) {

	sqm_lst_t *detailslst = NULL;
	jobjectArray newArr;

	jboolean isCopy;
	char *fstr = GET_STR(file_path, isCopy);

	PTRACE(1, "jni:FileUtil_getCopyDetails(...,%s)", Str(fstr));
	if (-1 == get_copy_details(CTX, fstr, (int)whichDetails,
		&detailslst)) {

		REL_STR(file_path, fstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(file_path, fstr, isCopy);

	newArr = lst2jarray(env, detailslst, "java/lang/String", charr2String);
	lst_free_deep(detailslst);

	PTRACE(1, "jni:FileUtil_getCopyDetails() done");
	return (newArr);
}

/*
 *  Combination function to do both the directory listing and get file
 *  details in a single call to the server.  Like getDirectoryEntries,
 *  the total number of files returned in the object may change between calls
 *  to the server.
 */
JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_listCollectFileDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring snappath,
    jstring relativePath, jstring lastFile, jint howmany, jint whichDetails,
    jstring restrictions) {

	sqm_lst_t		*detailslst = NULL;
	jobjectArray	newArr;
	uint32_t	totalFiles;
	jclass		class;
	jobject		newObj;
	jmethodID	mid;

	jboolean isCopy1;
	jboolean isCopy2;
	jboolean isCopy3;
	jboolean isCopy4;
	jboolean isCopy5;
	char *fstr = GET_STR(fsname, isCopy1);
	char *sstr = GET_STR(snappath, isCopy2);
	char *dstr = GET_STR(relativePath, isCopy3);
	char *rstr = GET_STR(restrictions, isCopy4);
	char *lstr = GET_STR(lastFile, isCopy5);


	PTRACE(1, "jni:FileUtil_listCollectFileDetails(...)");
	if (-1 == list_and_collect_file_details(CTX,
		fstr, sstr, dstr, lstr, howmany, whichDetails, rstr,
		&totalFiles, &detailslst)) {

		ThrowEx(env);
		REL_STR(fsname, fstr, isCopy1);
		REL_STR(snappath, sstr, isCopy2);
		REL_STR(relativePath, dstr, isCopy3);
		REL_STR(restrictions, rstr, isCopy4);
		REL_STR(lastFile, lstr, isCopy5);
		return (NULL);
	}

	class = (*env)->FindClass(env, BASEPKG"/GetList");
	mid = (*env)->GetMethodID(env, class, "<init>",
	    "(I[Ljava/lang/String;)V");

	newArr = lst2jarray(env, detailslst, "java/lang/String", charr2String);

	/* return from server is remaining files, GUI wants total */
	totalFiles += detailslst->length;

	newObj = (*env)->NewObject(env, class, mid, (jint)totalFiles, newArr);

	lst_free_deep(detailslst);
	REL_STR(fsname, fstr, isCopy1);
	REL_STR(snappath, sstr, isCopy2);
	REL_STR(relativePath, dstr, isCopy3);
	REL_STR(restrictions, rstr, isCopy4);
	REL_STR(lastFile, lstr, isCopy5);

	PTRACE(1, "jni:FileUtil_listCollectFileDetails() done");
	return (newObj);
}

/*
 *  Similar to listCollect above, this function gets details for
 *  one and only one file.
 */
JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_collectFileDetails(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsname, jstring snappath,
    jstring filePath, jint whichDetails) {

	sqm_lst_t *detailslst = NULL;
	sqm_lst_t *filelst = lst_create();

	jboolean isCopy1;
	jboolean isCopy2;
	jboolean isCopy3;

	char *fstr;
	char *sstr;
	char *dstr;
	char *result;

	int st = 0;

	PTRACE(1, "jni:FileUtil_collectFileDetails(...)");

	/* make sure our list created ok */
	if (filelst == NULL) {
		ThrowEx(env);
		return (NULL);
	}

	fstr = GET_STR(fsname, isCopy1);
	sstr = GET_STR(snappath, isCopy2);
	dstr = GET_STR(filePath, isCopy3);

	st = lst_append(filelst, dstr);

	if (st == 0) {
		st == collect_file_details(CTX, fstr, sstr, "", filelst,
		    (int)whichDetails, &detailslst);
	}

	if (st == 0) {
	    /* we should have gotten one, and only one, return */
	    if (detailslst->length == 1) {
		result = (char *)(detailslst->head->data);
		lst_free(detailslst);
	    } else {
		    lst_free_deep(detailslst);
	    }
	}

	// Not lst_free_deep as we don't want to free the string
	lst_free(filelst);

	REL_STR(fsname, fstr, isCopy1);
	REL_STR(snappath, sstr, isCopy2);
	REL_STR(filePath, dstr, isCopy3);

	if (st != 0) {
		ThrowEx(env);
		return (NULL);
	}

	PTRACE(1, "jni:FileUtil_collectFileDetails() done");
	return (JSTRING(result));
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_deleteFile(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring path) {

	jboolean isCopy;
	char *cstr = GET_STR(path, isCopy);
	int res;

	sqm_lst_t *lst = str2lst(cstr, ",");
	PTRACE(1, "jni:FileUtil delete (%s)", Str(cstr));

	res = delete_files(CTX, lst);

	lst_free_deep(lst);
	REL_STR(path, cstr, isCopy);

	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FileUtil deleteFile() done");
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_FileUtil_deleteFiles(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobjectArray filepaths) {

	int res;
	sqm_lst_t *lst =
		jarray2lst(env, filepaths, "java/lang/String", String2charr);

	PTRACE(1, "jni:FileUtil delete list of files");

	res = delete_files(CTX, lst);

	lst_free_deep(lst);

	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:FileUtil deleteFiles() done");
}
