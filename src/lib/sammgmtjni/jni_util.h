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
#ifndef	JNI_UTIL_H
#define	JNI_UTIL_H

#pragma ident	"$Id: jni_util.h,v 1.19 2008/03/17 14:43:55 am143972 Exp $"

#define	BASEPKG "com/sun/netstorage/samqfs/mgmt"

#include <alloca.h>
#define	CTX ctxj2c(env, ctx, (ctx_t *)alloca(sizeof (ctx_t)))

#define	CBOOL(jbool) ((jbool == JNI_TRUE) ? B_TRUE : B_FALSE)
#define	JBOOL(boolt) ((boolt == B_TRUE) ? JNI_TRUE : JNI_FALSE)
/* string processing macros */
#define	JSTRING(pchar) (*env)->NewStringUTF(env, (char *)pchar)
#define	GET_STR(jstr, isCopy) (char *) \
	((NULL == jstr) ? NULL : (*env)->GetStringUTFChars(env, jstr, &isCopy))
#define	REL_STR(jstr, cstr, isCopy) \
	if (isCopy == JNI_TRUE && NULL != jstr) \
	(*env)->ReleaseStringUTFChars(env, jstr, cstr)
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "string.h"


/*
 * the JNI wrapper of the C API attempts to provide a consistent response
 * when reporting values (options) that are not set.
 * In the C API, this is not done because the underlying data types include
 * unsigned types, so a single negative value cannot be used for all types.
 * JNI layer will report VALUE_NOT_SET any time a value (option) is not set.
 * In some cases, this requires converting the type-dependent 'special' value
 * (the 'reset' value) received from the C API to VALUE_NOT_SET.
 */
#define	VALUE_NOT_SET -1


void
ThrowEx(JNIEnv *env);

void
ThrowMultiMsgEx(JNIEnv *env, jobjectArray msgs);

void
ThrowWarnings(JNIEnv *env, jobjectArray msgs);

void
ThrowIncompatVerEx(JNIEnv *env, boolean_t clientNewer);

void
ThrowMultiStepOpEx(JNIEnv *env, int failedStep);

/* create a ctx_t structure from a Ctx object */
ctx_t *
ctxj2c(JNIEnv *env, jobject ctx, ctx_t *c);


/* return the jobject corresponding to the specified field */
jobject
getObjFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname, char *typesig);
/*
 * extract a char array from the Java String stored in the specified object
 * field and copy the array into the specified buffer.
 */
void
getStrFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname, char *buf);

/*
 * extract a char array from the Java String stored in the specified object
 * field and copy into a newly created buffer that must be freed by the caller
 */
void
getCharStarFld(JNIEnv *env, jclass cls, jobject obj,
    char *fieldname, char **buf);

boolean_t
getBoolFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

/* extract the value from a Object[] field */
jobjectArray
getJArrFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname, char *elemTypeSig);

/* functions to extract the value from a Java field with an integral type */

jint
getJIntFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

jbyte
getJByteFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

jshort
getJShortFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

jchar
getJCharFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

jlong
getJLongFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);

jfloat
getJFloatFld(JNIEnv *env,
    jclass cls, jobject obj, char *fieldname);


/* convert an unsigned long long to a jstring (does not fit a jlong) */
jstring
ull2jstr(JNIEnv *env, unsigned long long val);

/* convert a jstring to an unsigned long long */
unsigned long long
jstr2ull(JNIEnv *env, jstring jstr);

/* extract the ull from the specified String field */
unsigned long long
jstrFld2ull(JNIEnv *env, jclass cls, jobject obj, char *fieldname);

/*
 * convert a uint_t to a jlong.
 * if the value to be converted is a reset value, return VALUE_NOT_SET
 */
jlong
uint2jlong(uint_t val);

/*
 * convert a sqm_lst_t list to a java array of jtype objects
 * each element is converted from C to Java using c2j function
 */
jobjectArray
lst2jarray(JNIEnv *env,
    sqm_lst_t *lst,
    char *className,
    jobject (*c2j)(JNIEnv *, void *));


/*
 * convert a java array of className objects to a sqm_lst_t list.
 * each element is converted from Java to C using j2c function.
 */
sqm_lst_t *
jarray2lst(JNIEnv *env,
    jobjectArray jarr,
    char *className,
    void * (*j2c)(JNIEnv *, jobject));


/*
 * convert a C array to a java array of className objects.
 * each element is converted from C to Java using c2j function.
 */
jobjectArray
arr2jarray(JNIEnv *env,
    void *arr,
    int elemsz,
    int arr_len,
    char *className,
    jobject (*c2j)(JNIEnv *, void *));


/*
 * convert an arrays of pointers to a java array of className objects.
 * each pointed element is converted from C to Java using c2j function.
 */
jobjectArray
arrOfPtrs2jarray(JNIEnv *env,
    void *arr[],
    int arr_len,
    char *className,
    jobject (*c2j)(JNIEnv *, void *));


/*
 * convert a java array of className objects to an array of pointers.
 * each element is converted from Java to C using j2c function.
 */
void **
jarray2arrOfPtrs(JNIEnv *env,
    jobjectArray jarr,
    char *className,
    void * (*j2c)(JNIEnv *, jobject));


/*
 * convert a C array of boolean_t to a jbooleanArray
 */
jbooleanArray
arr2jboolArray(JNIEnv *env,
    boolean_t arr[],
    int arr_len);


/*
 * convert a jbooleanArray to a C array of boolean_t
 */
void
jboolArray2arr(JNIEnv *env,
    jbooleanArray jboolArr,
    boolean_t arr[]);


/*
 * convert a jlongArray to a C array of long.
 * the function allocates the C array.
 * return the number of elements in the array.
 */
int
jlongArray2newarr(JNIEnv *env,
    jlongArray jlongArr,
    long **parr);


/*
 * a common type of c2j: converts a char array to a String
 */
jobject
charr2String(JNIEnv *env, void *chars);


/*
 * a common type of j2c: converts a String to a char[]
 */
void *
String2charr(JNIEnv *env, jobject strObj);


/*
 * Convert a list of ints to a jintArray
 */
jintArray
lst2jintArray(JNIEnv *env, sqm_lst_t *lst);


/*
 * Convert a jintArray to a list of int
 */
sqm_lst_t *
jintArray2lst(JNIEnv *env, jintArray jintArr);
#endif	/* JNI_UTIL_H */
