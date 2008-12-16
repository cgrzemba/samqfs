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

#pragma ident	"$Revision: 1.7 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>

/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/csn_registration.h"
#include "mgmt/log.h"

/* local headers */
#include "com_sun_netstorage_samqfs_mgmt_reg_Register.h"
#include "jni_util.h"



void *
ByteArray2crypt_str(JNIEnv *env, jbyteArray byteArr) {
	int array_len;
	crypt_str_t *cs;

	if (NULL == byteArr) {
		PTRACE(2, "jni: byte array not set (NULL)");
		return (NULL);
	}
	array_len = (*env)->GetArrayLength(env, byteArr);
	PTRACE(2, "array_len = %d\n", array_len);

	if (array_len == 0) {
		PTRACE(2, "array was zero length\n");
		return (NULL);
	}
	cs = malloc(sizeof (crypt_str_t));
	if (cs == NULL) {
		return (NULL);
	}
	cs->str = malloc(array_len);
	if (cs->str == NULL) {
		free(cs);
		return (NULL);
	}
	(*env)->GetByteArrayRegion(env,
	    byteArr, 0, array_len, (jbyte *)(cs->str));
	cs->str_len = array_len;
	return (cs);
}


jbyteArray
crypt_str2ByteArray(JNIEnv *env, crypt_str_t *cs) {

	if (cs == NULL || cs->str == NULL) {
		return (NULL);
	}
	jbyteArray byteArr  = (*env)->NewByteArray(env, cs->str_len);

	/*
	 * To match the prototype of SetByteArrayRegion we have to
	 * cast explicitly to signed char. Needs testing to verify nothing
	 * untoward occurs.
	 */
	(*env)->SetByteArrayRegion(env, byteArr, 0, cs->str_len,
	    (signed char *)(cs->str));
	return (byteArr);
}

jobject
buildSimpleSignature(JNIEnv *env, void *v_crypt_str, void *hex_public_key) {
	jclass cls;
	jmethodID mid;
	jobject newObj;
	crypt_str_t *cs = (crypt_str_t *)v_crypt_str;

	PTRACE(2, "jni:buildSimpleSignature() entry");
	/* allow a null crypt_str for now */
	if (NULL == hex_public_key) {
		PTRACE(2, "jni:buildSimpleSignature() done (NULL)");
		return (NULL);
	}
	PTRACE(2, "jni:buildSimpleSignature() hex key %s", hex_public_key);
	cls = (*env)->FindClass(env, BASEPKG"/reg/SimpleSignature");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;[B)V");
	newObj = (*env)->NewObject(env, cls, mid,
		JSTRING((char *)hex_public_key),
		crypt_str2ByteArray(env, cs));

	PTRACE(2, "jni:buildSimpleSignature() exit done");
	return (newObj);
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_reg_Register_getRegistration(JNIEnv *env,
    jclass cls, jobject ctx, jstring assetPrefix) {

	jboolean isCopy;
	char *inStr = GET_STR(assetPrefix, isCopy);
	char *resStr = NULL;

	PTRACE(3, "jni:Reg getRegistration() entry");
	if (-1 == cns_get_registration(CTX, inStr, &resStr)) {
		REL_STR(assetPrefix, inStr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	REL_STR(assetPrefix, inStr, isCopy);

	PTRACE(3, "jni:Reg getRegistration() done");
	return (JSTRING(resStr));
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_reg_Register_register(JNIEnv *env,
    jclass cls, jobject ctx, jstring kv_string, jbyteArray clPass,
    jbyteArray proxyPass, jstring clntPubKey) {

	jboolean isCopyKV;
	jboolean isCopyPubKey;
	char *cstrKV = GET_STR(kv_string, isCopyKV);
	char *cstrPubKey = GET_STR(clntPubKey, isCopyPubKey);
	crypt_str_t *client_crypt = NULL;
	crypt_str_t *proxy_crypt = NULL;

	int res;

	PTRACE(1, "jni:Register_register: %s", cstrKV);
	client_crypt = (crypt_str_t *)ByteArray2crypt_str(env, clPass);
	proxy_crypt = (crypt_str_t *)ByteArray2crypt_str(env, proxyPass);

	res = cns_register(CTX, cstrKV, client_crypt, proxy_crypt, cstrPubKey);

	free_crypt_str(proxy_crypt);
	free_crypt_str(client_crypt);
	REL_STR(kv_string, cstrKV, isCopyKV);
	REL_STR(clntPubKey, cstrPubKey, isCopyPubKey);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}

	PTRACE(1, "jni:Register_register() done");
}

JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_reg_Register_getPublicKey(JNIEnv *env,
    jclass cls, jobject ctx) {

	jboolean isCopy;
	char *pub_key = NULL;
	crypt_str_t *signature;
	jobject simpleSign;

	PTRACE(3, "jni:Reg getPublicKey() entry");
	if (-1 == cns_get_public_key(CTX, &pub_key, &signature)) {
		ThrowEx(env);
		return (NULL);
	}


	simpleSign = buildSimpleSignature(env, signature, pub_key);
	free_crypt_str(signature);
/* IS this causing problems ? */
	free(pub_key);

	PTRACE(3, "jni:Reg getPublicKey() done");
	return (simpleSign);
}
