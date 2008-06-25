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
#pragma ident	"$Revision: 1.15 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
#include "libgen.h"
/* API header files */
#include "pub/mgmt/hosts.h"
#include "mgmt/log.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_fs_Host.h"
#include "jni_util.h"

extern int crtprio; // crt trace priority

/* C Structure -> Java class conversion function */

jobject
host2Host(JNIEnv *env, void *v_host_info) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	host_info_t *h = (host_info_t *)v_host_info;

	PTRACE(3, "jni:%s|%d|%d|%d|%d",
	    Str(h->host_name),
	    (NULL == h->ip_addresses) ? -1 : h->ip_addresses->length,
	    h->server_priority,
	    h->state,
	    h->current_server);

	PTRACE(2, "jni:host2Host() entry");
	cls = (*env)->FindClass(env, BASEPKG"/fs/Host");
	/* call the private constructor to initialize all fields */
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;[Ljava/lang/String;IZI)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(h->host_name),
	    lst2jarray(env, h->ip_addresses, "java/lang/String", charr2String),
	    (jint)h->server_priority,
	    JBOOL(h->current_server),
	    (jint)h->state);

	PTRACE(2, "jni:host2Host() done");
	return (newObj);
}

host_info_t *
Host2host(JNIEnv *env, jobject hostObj) {

	jclass cls;
	host_info_t *host = (host_info_t *)malloc(sizeof (host_info_t));
	host->host_name = (char *)malloc(sizeof (upath_t));
	memset(host->host_name, 0, sizeof (upath_t));

	PTRACE(2, "jni:Host2host() entry");
	cls = (*env)->GetObjectClass(env, hostObj);
	getStrFld(env, cls, hostObj, "name", host->host_name);
	PTRACE(2, "jni:Host2host() got hostname[%s]", host->host_name);
	host->ip_addresses = jarray2lst(env,
	    getJArrFld(env,
		cls, hostObj, "ipAddrs", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);

	host->server_priority = (int)getJIntFld(env, cls, hostObj, "srvPrio");
	host->state = 0;
	host->current_server = getBoolFld(env, cls, hostObj, "isCrtServer");

	PTRACE(3, "jni:%s|%d|%d|%d|%d",
	    Str(host->host_name),
	    (NULL == host->ip_addresses) ? -1 : host->ip_addresses->length,
	    host->server_priority,
	    host->state,
	    host->current_server);
	PTRACE(2, "jni:Host2host() done");
	return (host);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_getSharedFSHosts(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jint options) {

	jobjectArray newArr;
	sqm_lst_t *kvlst;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Host_getSharedFSHosts(...,%s)", Str(cstr));

	if (-1 == get_shared_fs_hosts(CTX, cstr, options, &kvlst)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:shared host information obtained");

	newArr = lst2jarray(env, kvlst, "java/lang/String", charr2String);
	REL_STR(fsName, cstr, isCopy);
	lst_free_deep(kvlst);
	PTRACE(1, "jni:Host_getSharedFSHosts() done");
	return (newArr);

}



JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_getConfig(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jobjectArray newArr;
	sqm_lst_t *hlst;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Host_getConfig(...,%s)", Str(cstr));

	if (-1 == get_host_config(CTX, cstr, &hlst)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:host config information obtained");

	newArr = lst2jarray(env, hlst, BASEPKG"/fs/Host", host2Host);
	REL_STR(fsName, cstr, isCopy);
	free_list_of_host_info(hlst);
	PTRACE(1, "jni:Host_getConfig() done");
	return (newArr);

}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_removeFromConfig(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jstring hostName) {

	jboolean isCopy; /* used by string macros */
	char *fstr = GET_STR(fsName, isCopy);
	char *hstr = GET_STR(hostName, isCopy);

	PTRACE(1, "jni:Host_removeFromConfig entry");
	PTRACE(1, "jni:Removing host[%s] from fs[%s]", Str(hstr), Str(fstr));

	if (-1 == remove_host(CTX, fstr, hstr)) {
		REL_STR(fsName, fstr, isCopy);
		REL_STR(hostName, hstr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(fsName, fstr, isCopy);
	REL_STR(hostName, hstr, isCopy);

	PTRACE(1, "jni:Host_removeFromConfig() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_addToConfig(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName, jobject hostInfo) {

	jboolean isCopy; /* used by string macros */
	char *fstr = GET_STR(fsName, isCopy);
	host_info_t *host;

	PTRACE(1, "jni:Host_addToConfig entry");
	PTRACE(1, "jni:Adding to fs[%s]", Str(fstr));

	host = (host_info_t *)Host2host(env, hostInfo);

	if (-1 == add_host(CTX, fstr, host)) {
		REL_STR(fsName, fstr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(fsName, fstr, isCopy);
	free_host_info(host);

	PTRACE(1, "jni:Host_addToConfig() done");
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_discoverIPsAndNames(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst;
	jobjectArray newArr;

	PTRACE(1, "jni:discoverIPsAndNames entry");
	if (-1 == discover_ip_addresses(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);
	PTRACE(1, "jni:discoverIPsAndNames done");
	return (newArr);
}


JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_getAdvancedNetCfg(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {


	jobjectArray newArr;
	sqm_lst_t *lst;
	jboolean isCopy; /* used by string macros */
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Host_getAdvancedNetCfg(...,%s)", Str(cstr));

	if (-1 == get_advanced_network_cfg(CTX, cstr, &lst)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:advanced net cfg information obtained");

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	REL_STR(fsName, cstr, isCopy);
	lst_free_deep(lst);

	PTRACE(1, "jni:Host_getAdvancedNetCfg() done");
	return (newArr);
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_setAdvancedNetCfg(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName,
	jobjectArray host_strs) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Host_setAdvancedNetCfg() entry");
	if (-1 == set_advanced_network_cfg(CTX, cstr,
	    jarray2lst(env, host_strs, "java/lang/String", String2charr))) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(fsName, cstr, isCopy);

	PTRACE(1, "jni:Host_setAdvancedNetCfg() done");
}


JNIEXPORT jstring JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_getMetadataServerName(
	JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);
	char *res = NULL;

	PTRACE(1, "jni:Host_getMetadataServerName() entry");
	if (-1 == get_mds_host(CTX, cstr, &res)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return (NULL);
	}

	REL_STR(fsName, cstr, isCopy);

	PTRACE(1, "jni:Host_getMetadataServerName() done");
	return (JSTRING(res));
}

JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_fs_Host_setClientState(JNIEnv *env,
	jclass cls /*ARGSUSED*/, jobject ctx, jstring fsName,
	jobjectArray host_strs, jint state) {

	jboolean isCopy;
	char *cstr = GET_STR(fsName, isCopy);

	PTRACE(1, "jni:Host_setAdvancedNetCfg() entry");
	if (-1 == set_host_state(CTX, cstr,
	    jarray2lst(env, host_strs, "java/lang/String", String2charr),
	    state)) {
		REL_STR(fsName, cstr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(fsName, cstr, isCopy);

	PTRACE(1, "jni:Host_setAdvancedNetCfg() done");
}
