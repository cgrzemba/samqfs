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
#pragma ident   "$Revision: 1.23 $"

/* Solaris header files */
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/sammgmt_rpc.h"
#include "mgmt/log.h"
/* JNI header files */
#include "com_sun_netstorage_samqfs_mgmt_SamFSConnection.h"
#include "jni_util.h"



/*
 * return the connection handle pointer
 */
static jlong
getConnHandle(JNIEnv *env, jobject samConn) {

	jclass cls = (*env)->GetObjectClass(env, samConn);
	jfieldID connHandleFID =
		    (*env)->GetFieldID(env, cls, "connHandle", "J");
	return ((*env)->GetLongField(env, samConn, connHandleFID));
}


static jobject
getNew(JNIEnv *env, jclass connCls, jstring hostName, jlong timeout) {

	char *utf;
	samrpc_client_t *client;
	ctx_t ctx;
	char svr_hostname[MAXHOSTNAMELEN + 1];

	jlong jhandle;
	jmethodID mid;
	jobject newObj;
	jboolean isCopy;
	char *srv_api_ver = NULL, *sam_ver = NULL;
	int verdiff;

	PTRACE(1, "jni:SamFSConnection_getNew() entry");

	utf = GET_STR(hostName, isCopy);
	PTRACE(1, "jni:hostname=%s.", utf);

#ifdef LOCALMGMT
	/* create handle */
	client = (samrpc_client_t *)malloc(sizeof (samrpc_client_t));
	/* initialize library */
	PTRACE(1, "jni:initializing mgmt lib");
	if (-1 == init_sam_mgmt(&ctx)) {
		REL_STR(hostName, utf, isCopy);
		ThrowEx(env);
		return;
	}

#else   // use RPC

	/* create handle (initializes the library too) */
	PTRACE(1, "jni:creating new rpc client, timeout=%ld", (time_t)timeout);
	client = samrpc_create_clnt_timed(utf, (time_t)timeout);

	if (NULL == client) {
		REL_STR(hostName, utf, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	/* check client vs server API version */
	ctx.dump_path[0] = ctx.read_location[0] = ctx.user_id[0] = '\0';
	ctx.handle = client;
	srv_api_ver = get_samfs_lib_version(&ctx);
	if (NULL != srv_api_ver && NULL != samrpc_version()) {
		verdiff = strcmp(samrpc_version(), srv_api_ver);
		if (verdiff > 0) {
			/* client is newer */
			PTRACE(1, "jni:clientver:%s serverver:%s",
			    samrpc_version(), srv_api_ver);
		}
		if (verdiff < 0) {
			/* server is newer */
			REL_STR(hostName, utf, isCopy);
			PTRACE(1, "jni:clientver:%s serverver:%s",
			    samrpc_version(), srv_api_ver);
			ThrowIncompatVerEx(env, B_FALSE);
			return (NULL);
		}
		// versions match
		PTRACE(1, "jni:client/server versions match (%s)", srv_api_ver);

		// shared fs is supported in 1.2,
		// get_server_info is available in 1.2
		memset(svr_hostname, 0, MAXHOSTNAMELEN + 1);
		if (strcmp(srv_api_ver, "1.1") > 0) {
			if (get_server_info(&ctx, svr_hostname) < 0) {
				ThrowEx(env);
				return (NULL);
			}
			PTRACE(2, "jni:hostname of server[%s]", svr_hostname);
		}

	}
	/* now get SAM-FS/QFS version */
	sam_ver = get_samfs_version(&ctx);
#endif
	if (NULL == client) {
		REL_STR(hostName, utf, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:mgmt lib initialized");
#ifdef LOCALMGMT
	client->svr_name = "";
#endif
	jhandle = (jlong)client;


	/* call private constructor */
	mid = (*env)->GetMethodID(env,
	    connCls, "<init>",
	    "(JLjava/lang/String;"
	    "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, connCls, mid, jhandle,
	    JSTRING(srv_api_ver), JSTRING(sam_ver), JSTRING(svr_hostname),
	    JSTRING(client->svr_name));
	REL_STR(hostName, utf, isCopy);
	PTRACE(1, "jni:SamFSConnection_getNew() exit");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_getNew(
    JNIEnv *env, jclass connCls, jstring hostName) {
	return (getNew(env, connCls, hostName, DEF_TIMEOUT_SEC));
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_getNewSetTimeout(
    JNIEnv *env, jclass connCls, jstring hostName, jlong timeout) {
	return (getNew(env, connCls, hostName, timeout));
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_destroy(JNIEnv *env,
    jobject samConn) {

	jlong connHandle;
	jclass cls = (*env)->GetObjectClass(env, samConn);
	jfieldID connHandleFID =
	    (*env)->GetFieldID(env, cls, "connHandle", "J");

	PTRACE(1, "jni:SamFSConnection_destroy() entry");
	connHandle = getConnHandle(env, samConn);
	if (0 == connHandle) {
		PTRACE(1, "jni:connection already destroyed");
		return;
	}
	PTRACE(1, "jni:destroying connection handle %lld",
	    connHandle);
	samrpc_destroy_clnt((samrpc_client_t *)connHandle);
	/* Should connHandle be free'd here? */

	/* reset handle to 0 */
	(*env)->SetLongField(env, samConn, connHandleFID, (jlong)0);

	PTRACE(1, "jni:SamFSConnection_destroy() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_reinit(JNIEnv *env,
    jobject samConn) {

	ctx_t ctx;

	PTRACE(1, "jni:SamFSConnection_reinit() entry");
	ctx.dump_path[0] = '\0';
	ctx.read_location[0] = '\0';
	ctx.user_id[0] = '\0';
	ctx.handle = (samrpc_client_t *)getConnHandle(env, samConn);

	/* reinitialize library */
	PTRACE(1, "jni:reinitializing libfsmgmt");
	if (-1 == init_sam_mgmt(&ctx)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:SamFSConnection_reinit() done");
}


JNIEXPORT jlong JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_getDefaultTimeout(
	JNIEnv *env, jclass cls /*ARGSUSED*/) {
	return ((jlong)DEF_TIMEOUT_SEC);
}


JNIEXPORT jlong JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_getTimeout(JNIEnv *env,
    jobject samConn) {

#ifdef LOCALMGMT
	/*
	 * The reference to samrpc_get_timeout() causes a link error,
	 * even if this method is not called.
	 */

	return (0);
#else
	time_t secs;
	jlong connHandle;

	PTRACE(1, "jni:SamFSConnection_getTimeout()");
	connHandle = getConnHandle(env, samConn);
	if (-1 == samrpc_get_timeout((samrpc_client_t *)connHandle, &secs)) {
		ThrowEx(env);
		return (-1);
	}
	PTRACE(1, "jni:SamFSConnection_getTimeout() done");
	return ((jlong)secs);
#endif
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_SamFSConnection_setTimeout(JNIEnv *env,
    jobject samConn, jlong secs) {

#ifdef LOCALMGMT
	/*
	 *	The reference to samrpc_set_timeout() causes a link error,
	 *	even if this method is not called.
	 */
#else
	jlong connHandle;

	PTRACE(1, "jni:SamFSConnection_setTimeout()");
	connHandle = getConnHandle(env, samConn);
	if (-1 == samrpc_set_timeout((samrpc_client_t *)connHandle,
	    (time_t)secs)) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:SamFSConnection_setTimeout() done");
#endif
}
