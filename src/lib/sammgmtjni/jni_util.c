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
#pragma ident	"$Revision: 1.34 $"

#include <stdlib.h>
#include <jni.h>

#include "jni_util.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/device.h"
#include "mgmt/log.h"


void
ThrowEx(JNIEnv *env) {

	jclass ec;
	jmethodID mid;
	jthrowable throwObj;
	char className[100];

	PTRACE(1, "jni:error %d recvd: %s.", samerrno, samerrmsg);
	if ((samerrno >= SE_SAMRPC_API_BEGIN &&
	    samerrno <= SE_RPC_NOT_YET) ||
	    samerrno == SE_NETWORK_DOWN) {
		strcpy(className, BASEPKG"/SamFSCommException");
	} else {
		switch (samerrno) {
		case SE_RPC_TIMEDOUT:
			strcpy(className,
			    BASEPKG"/SamFSTimeoutException");
			break;
		case SE_RPC_INSECURE_CLIENT:
			strcpy(className,
			    BASEPKG"/SamFSAccessDeniedException");
			break;
		default:
			strcpy(className, BASEPKG"/SamFSException");
		}
	}
	PTRACE(1, "jni:throw %s", className);
	ec = (*env)->FindClass(env, className);
	mid = (*env)->GetMethodID(env, ec, "<init>", "(Ljava/lang/String;I)V");
	throwObj = (*env)->NewObject(env, ec, mid,
	    JSTRING(samerrmsg), (jint)samerrno);
	(*env)->Throw(env, throwObj);
}


void
ThrowMultiMsgEx(JNIEnv *env, jobjectArray msgs) {

	jclass ec;
	jmethodID mid;
	jthrowable throwObj;

	PTRACE(1,
	    "jni:multimsg error %d received: %s.", samerrno, Str(samerrmsg));
	ec = (*env)->FindClass(env, BASEPKG"/SamFSMultiMsgException");
	mid = (*env)->GetMethodID(env, ec, "<init>",
	    "(Ljava/lang/String;I[Ljava/lang/String;)V");
	throwObj = (*env)->NewObject(env, ec, mid,
	    JSTRING(samerrmsg), (jint)samerrno, msgs);
	(*env)->Throw(env, throwObj);
}


void
ThrowWarnings(JNIEnv *env, jobjectArray msgs) {

	jclass ec;
	jmethodID mid;
	jthrowable throwObj;

	PTRACE(1, "jni:warnings received");
	ec = (*env)->FindClass(env, BASEPKG"/SamFSWarnings");
	mid = (*env)->GetMethodID(env, ec, "<init>",
	    "(Ljava/lang/String;[Ljava/lang/String;)V");
	throwObj = (*env)->NewObject(env, ec, mid, JSTRING(samerrmsg), msgs);
	(*env)->Throw(env, throwObj);
}


void
ThrowIncompatVerEx(JNIEnv *env, boolean_t clientNewer) {

	jclass ec;
	jmethodID mid;
	jthrowable throwObj;

	PTRACE(1, "jni:throw SamFSIncompatVerException(%c)",
	    (B_TRUE == clientNewer) ? 'T' : 'F');
	ec = (*env)->FindClass(env, BASEPKG"/SamFSIncompatVerException");
	mid = (*env)->GetMethodID(env, ec, "<init>", "(Z)V");
	throwObj = (*env)->NewObject(env, ec, mid, JBOOL(clientNewer));
	(*env)->Throw(env, throwObj);
}


void
ThrowMultiStepOpEx(JNIEnv *env, int failedStep) {

	jclass ec;
	jmethodID mid;
	jthrowable throwObj;

	PTRACE(1, "jni:throw SamFSMultiStepOpException(%d)", failedStep);
	ec = (*env)->FindClass(env, BASEPKG"/SamFSMultiStepOpException");
	mid = (*env)->GetMethodID(env, ec, "<init>",
	    "(Ljava/lang/String;II)V");
	throwObj = (*env)->NewObject(env, ec, mid,
	    JSTRING(samerrmsg), (jint)samerrno, (jint)failedStep);
	(*env)->Throw(env, throwObj);
}

/*
 * convert an unsigned long long to a jstring (does not fit a jlong)
 */
jstring
ull2jstr(JNIEnv *env, unsigned long long val) {
	static char buf[21]; /* size of 2^64-1 in decimal representation */

	buf[20] = '\0';
	if (val == fsize_reset)	// field not set
		return (NULL);
	else
		return (JSTRING(ulltostr(val, &buf[20])));
}

/*
 * convert a jstring to an unsigned long long
 */
unsigned long long
jstr2ull(JNIEnv *env, jstring jstr) {
	jboolean isCopy;
	char *cstr;
	uint64_t val;

	if (NULL == jstr)
		return (0);
	cstr = GET_STR(jstr, isCopy);
	if (-1 == str_to_fsize(cstr, &val)) {
		PTRACE(1, "jni:jstr2ull(%s) failed calling str_to_fsize",
		    Str(cstr));
		REL_STR(jstr, cstr, isCopy);
		return (0);
	}
	REL_STR(jstr, cstr, isCopy);
	return (val);
}

/* extract the ull from the specified String field */
unsigned long long
jstrFld2ull(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env,
	    cls, fieldname, "Ljava/lang/String;");
	jstring s = (jstring)(*env)->GetObjectField(env, obj, fid);
	return (jstr2ull(env, s));
}


/*
 * convert a uint_t to a jlong.
 * if the value to be converted is a reset value, return VALUE_NOT_SET
 */
jlong
uint2jlong(uint_t val) {
	if (val == uint_reset)
		return (VALUE_NOT_SET);
	else
		return ((jlong)val);
}


/*
 * extract a char array from the Java String stored in the specified object
 * field and copy the array into the specified buffer.
 */
void
getStrFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname, char *buf) {
	jfieldID fid;
	jboolean isCopy;
	jstring s;
	char *cstr;

	if (NULL == obj) {
		PTRACE(1, "jni:getStrFld(...,%s,...):obj arg is null",
		    fieldname);
		buf[0] = '\0';
		return;
	}
	fid = (*env)->GetFieldID(env, cls, fieldname, "Ljava/lang/String;");
	if (NULL == fid) (*env)->ExceptionDescribe(env);
	s = (jstring)(*env)->GetObjectField(env, obj, fid);
	if (NULL == s) {
		buf[0] = '\0';
	} else {
		cstr = GET_STR(s, isCopy);
		strcpy(buf, cstr);
		REL_STR(s, cstr, isCopy);
	}
}


/*
 * extract a char array from the Java String stored in the specified object
 * field and copy into a newly created buffer that must be freed by the caller
 */
void
getCharStarFld(JNIEnv *env, jclass cls, jobject obj,
    char *fieldname, char **buf) {
	jfieldID fid;
	jboolean isCopy;
	jstring s;
	char *cstr;

	if (NULL == obj) {
		PTRACE(1, "jni:getCharStarFld(...,%s,...):obj arg is null",
		    fieldname);
		*buf = (char *)malloc(sizeof (char));
		(*buf)[0] = '\0';
		return;
	}
	fid = (*env)->GetFieldID(env, cls, fieldname, "Ljava/lang/String;");
	if (NULL == fid) (*env)->ExceptionDescribe(env);
	s = (jstring)(*env)->GetObjectField(env, obj, fid);
	if (NULL == s) {
		*buf = (char *)malloc(sizeof (char));
		(*buf)[0] = '\0';
	} else {
		cstr = GET_STR(s, isCopy);
		*buf = strdup(cstr);
		REL_STR(s, cstr, isCopy);
	}
}


boolean_t
getBoolFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "Z");
	return (CBOOL((*env)->GetBooleanField(env, obj, fid)));
}

jobject
getObjFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname, char *sig) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, sig);
	return ((*env)->GetObjectField(env, obj, fid));
}

jobjectArray
getJArrFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname, char *sig) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, sig);
	return ((*env)->GetObjectField(env, obj, fid));
}

jint
getJIntFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "I");
	return ((*env)->GetIntField(env, obj, fid));
}

jbyte
getJByteFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "B");
	return ((*env)->GetBooleanField(env, obj, fid));
}

jchar
getJCharFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "C");
	return ((*env)->GetCharField(env, obj, fid));
}

jshort
getJShortFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "S");
	return ((*env)->GetShortField(env, obj, fid));
}

jlong
getJLongFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "J");
	return ((*env)->GetLongField(env, obj, fid));
}

jfloat
getJFloatFld(JNIEnv *env, jclass cls, jobject obj, char *fieldname) {
	jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "F");
	return ((*env)->GetFloatField(env, obj, fid));
}


/* create a ctx_t structure from a Ctx object */
ctx_t *
ctxj2c(JNIEnv *env, jobject jctx, ctx_t *c) {

	jclass cls;

	if (NULL == c) {
		PTRACE(1, "jni: cannot allocate memory for ctx");
		return (NULL);
	}

#ifndef LOCALMGMT
	if (NULL == jctx) {
		PTRACE(1, "jni:NULL context argument");
		return (NULL);
	}
#endif
	cls = (*env)->GetObjectClass(env, jctx);
	c->handle = (samrpc_client_t *)getJLongFld(env, cls, jctx, "handle");
	getStrFld(env, cls, jctx, "dumpPath", c->dump_path);
	getStrFld(env, cls, jctx, "readPath", c->read_location);
	getStrFld(env, cls, jctx, "userID", c->user_id);

	// TRACE("jni:ctx(%s,%s)", Str(c->dump_path), Str(c->read_location));
	return (c);
}


/*
 * convert a sqm_lst_t list to a java array of className objects.
 * each element is converted from C to Java using c2j function.
 */
jobjectArray
lst2jarray(JNIEnv *env,
    sqm_lst_t *lst,
    char *className,
    jobject (*c2j)(JNIEnv *, void *)) {

	jclass cls;
	jobjectArray jarr;
	node_t *node;
	int idx = 0;

	if (NULL == lst) {
		PTRACE(1, "jni:lst2jarray(null,%s). return.", className);
		return (NULL);
	} else {
		PTRACE(2, "jni:lst2jarray(lst[%d],%s)", lst->length, className);
	}
	cls = (*env)->FindClass(env, className);
	if (NULL == cls) {
		(*env)->ExceptionDescribe(env);
		PTRACE(1, "jni:class %s not found", className);
		exit(-1);
	}
	jarr = (*env)->NewObjectArray(env, lst->length, cls, NULL);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create object array");
		exit(-1);
	}
	node = lst->head;
	while (node) {
		// TRACE("jni:lst2jarray:building node %d", idx);
		(*env)->SetObjectArrayElement(env, jarr, idx,
		    c2j(env, node->data));
		node = node->next;
		idx++;
	}
	PTRACE(2, "jni:lst2jarray() done");
	return (jarr);
}


/*
 * convert a java array of className objects to a sqm_lst_t list.
 * each element is converted from Java to C using j2c function.
 */
sqm_lst_t *
jarray2lst(JNIEnv *env,
    jobjectArray jarr,
    char *className,
    void * (*j2c)(JNIEnv *, jobject)) {

	sqm_lst_t *lst;
	int idx, n;

	if (NULL == jarr) {
		PTRACE(1, "jni:NULL array passed to jarray2lst()");
		return (NULL);
	}
	n = (int)(*env)->GetArrayLength(env, jarr);
	PTRACE(2, "jni:jarray2lst(jarr[%d],%s)", n, className);
	lst = lst_create();

	for (idx = 0; idx < n; idx++)
		if (-1 == lst_append(lst,
		    j2c(env, (*env)->GetObjectArrayElement(env, jarr, idx)))) {
			lst_free(lst);
			lst = NULL;
			break;
		}

	PTRACE(2, "jni:jarray2lst() done");
	return (lst);
}


/*
 * convert a C array to a java array of className objects.
 * each element is converted from C to Java using c2j function.
 */
jobjectArray
arr2jarray(JNIEnv *env,
    void *start,
    int elemsz,
    int arr_len,
    char *className,
    jobject (*c2j)(JNIEnv *, void *)) {

	jclass cls;
	jobjectArray jarr;
	int idx = 0;

	PTRACE(2, "jni:arr2jarray([%d],%s)", arr_len, className);
	cls = (*env)->FindClass(env, className);
	if (NULL == cls) {
		(*env)->ExceptionDescribe(env);
		PTRACE(1, "jni:class %s not found", className);
		exit(-1);
	}
	jarr = (*env)->NewObjectArray(env, arr_len, cls, NULL);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create object array");
		exit(-1);
	}
	while (idx < arr_len) {
		// TRACE("jni:building node %d", idx);
		(*env)->SetObjectArrayElement(env, jarr, idx,
		    c2j(env, ((char *)start) + idx * elemsz));
		idx++;
	}
	PTRACE(2, "jni:arr2jarray() done");
	return (jarr);
}

/*
 * convert an array of pointers to a java array of className objects.
 * each pointed element is converted from C to Java using c2j function.
 */
jobjectArray
arrOfPtrs2jarray(JNIEnv *env,
    void *arr[],
    int arr_len,
    char *className,
    jobject (*c2j)(JNIEnv *, void *)) {

	jclass cls;
	jobjectArray jarr;
	int idx = 0;

	PTRACE(2, "jni:arrOfPtrs2jarray(arr[%d],%s)", arr_len, className);
	cls = (*env)->FindClass(env, className);
	if (NULL == cls) {
		(*env)->ExceptionDescribe(env);
		PTRACE(1, "jni:class %s not found", className);
		exit(-1);
	}
	jarr = (*env)->NewObjectArray(env, arr_len, cls, NULL);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create object array");
		exit(-1);
	}
	while (idx < arr_len) {
		// TRACE("jni:building node %d", idx);
		(*env)->SetObjectArrayElement(env,
		    jarr, idx, c2j(env, arr[idx]));
		idx++;
	}
	PTRACE(2, "jni:arrOfPtrs2jarray() done");
	return (jarr);
}


/*
 * convert a java array of className objects to an array of pointers.
 * each element is converted from Java to C using j2c function.
 */
void **
jarray2arrOfPtrs(JNIEnv *env,
    jobjectArray jarr,
    char *className,
    void * (*j2c)(JNIEnv *, jobject)) {

	void **arr;
	int idx, n;

	if (NULL == jarr) {
		PTRACE(1, "jni:NULL array passed to jarray2arrOfPtrs()");
		return (NULL);
	}
	n = (int)(*env)->GetArrayLength(env, jarr);
	PTRACE(2, "jni:jarray2arrOfPtrs(jarr[%d],%s)", n, className);
	arr = malloc(n * sizeof (void *));

	for (idx = 0; idx < n; idx++)
		arr[idx] =
		    j2c(env, (*env)->GetObjectArrayElement(env, jarr, idx));
	PTRACE(2, "jni:jarray2arrOfPtrs() done");
	return (arr);
}


/*
 * convert a C array of boolean_t to a jbooleanArray
 */
jbooleanArray
arr2jboolArray(JNIEnv *env,
    boolean_t arr[],
    int arr_len) {

	jbooleanArray jarr;
	int idx = 0;
	jboolean *p = (jboolean *) malloc(arr_len * sizeof (jboolean));

	PTRACE(2, "jni:arr2jboolArray([%d])", arr_len);
	jarr = (*env)->NewBooleanArray(env, (jsize)arr_len);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create jboolArray");
		exit(-1);
	}
	while (idx < arr_len) {
		// TRACE("jni:building node %d", idx);
		p[idx] = JBOOL(arr[idx]);
		idx++;
	}
	(*env)->SetBooleanArrayRegion(env, jarr, 0, arr_len, p);
	free(p);
	PTRACE(2, "jni:arr2jboolArray() done");
	return (jarr);
}

/*
 * convert a jbooleanArray to a C array of boolean_t
 */
void
jboolArray2arr(JNIEnv *env,
    jbooleanArray jboolArr,
    boolean_t arr[]) {		// put the result here

	jint len = (*env)->GetArrayLength(env, jboolArr);
	jboolean *p = (jboolean *) malloc(len * sizeof (jboolean));
	int idx;

	(*env)->GetBooleanArrayRegion(env, jboolArr, 0, len, p);
	for (idx = 0; idx < len; idx++)
		arr[idx] = CBOOL(p[idx]);
	free(p);
}


/*
 * convert a jlongArray to a C array of long.
 * return number of array elements.
 */
int
jlongArray2newarr(JNIEnv *env,
    jlongArray jlongArr,
    long **parr) {		// allocate then put the result here

	jint len = (*env)->GetArrayLength(env, jlongArr);
	jlong *p = (jlong *) malloc(len * sizeof (jlong));
	int idx;

	(*env)->GetLongArrayRegion(env, jlongArr, 0, len, p);
	*parr = (long *)malloc(len * sizeof (long));
	for (idx = 0; idx < len; idx++)
		(*parr)[idx] = (long)p[idx];
	free(p);
	return ((int)len);
}


/*
 * a common type of c2j: converts a char array to a String
 */
jobject
charr2String(JNIEnv *env, void *chars) {
	return (JSTRING((char *)chars));

}


/*
 * a common type of j2c: converts a String to a char[]
 */
void *
String2charr(JNIEnv *env, jobject strObj) {

	jboolean isCopy;
	jstring s = (jstring)strObj;
	char *newstr, *cstr = GET_STR(s, isCopy);

	newstr = (char *)strdup(cstr);
	REL_STR(s, cstr, isCopy);
	return (newstr);
}

/*
 * convert a C list of int to a jIntArray
 */
jintArray
lst2jintArray(JNIEnv *env,
    sqm_lst_t *lst) {

	jintArray jarr;
	node_t *node = NULL;
	int idx = 0;
	jint *p = NULL;

	if (NULL == lst) {
		PTRACE(1, "jni:lst2jintArray(null). return.");
		return (NULL);
	} else {
		PTRACE(2, "jni:lst2jintArray(lst[%d])", lst->length);
		if (lst->length > 0) {
			p = (jint *) malloc(lst->length * sizeof (jint));
		}
	}

	jarr = (*env)->NewIntArray(env, lst->length);
	if (NULL == jarr) {
		PTRACE(1, "jni:cannot create jintArray");
		return (NULL);
	}

	node = lst->head;
	while (node) {
		// TRACE("jni:lst2jintArray:building node %d", idx);
		p[idx] = *(int *)node->data;
		PTRACE(2, "eq: %d\n", p[idx]);
		node = node->next;
		idx++;
	}
	(*env)->SetIntArrayRegion(env, jarr, 0, lst->length, p);

	if (p != NULL) {
		free(p);
	}
	PTRACE(2, "jni:lst2jintArray() done");
	return (jarr);
}

/*
 * convert a jintArray to a C list of int
 */
sqm_lst_t *
jintArray2lst(JNIEnv *env,
    jintArray jintArr) {

	sqm_lst_t *lst;
	int idx, len, *i;
	jint *p;

	if (NULL == jintArr) {
		PTRACE(1, "jni:NULL array passed to jintArray2lst()");
		return (NULL);
	}
	len = (int)(*env)->GetArrayLength(env, jintArr);
	p = (jint *) malloc(len * sizeof (jint));
	PTRACE(2, "jni:jintArray2lst(jintArr[%d])", len);
	lst = lst_create();

	(*env)->GetIntArrayRegion(env, jintArr, 0, len, p);

	for (idx = 0; idx < len; idx++) {
		i = (int *)malloc(sizeof (int));
		*i = (int)p[idx];
		if (-1 == lst_append(lst, i)) {
			lst_free(lst);
			lst = NULL;
			break;
		}
	}
	free(p);

	PTRACE(2, "jni:jintArray2lst() done");
	return (lst);

}
