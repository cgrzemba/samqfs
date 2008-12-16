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
#pragma ident	"$Revision: 1.19 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
/* API header files */
#include "pub/mgmt/types.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/device.h"
#include "mgmt/log.h"
/* non-API header files */
#include "mgmt/config/media.h"
/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_arc_VSNOp.h"
#include "jni_util.h"
#include "pub/mgmt/diskvols.h"

extern jobject catentry2CatEntry(JNIEnv *, void *);
extern jobject diskvol2DiskVol(JNIEnv *, void *);


/* C structure <-> Java class conversion functions */

jobject
pool2Pool(JNIEnv *env, void *v_pool) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	vsn_pool_t *pool = (vsn_pool_t *)v_pool;

	PTRACE(2, "jni:pool2Pool() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/VSNPool");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(pool->pool_name),
	    JSTRING(pool->media_type),
	    lst2jarray(env,
		pool->vsn_names, "java/lang/String", charr2String));
	PTRACE(2, "jni:pool2Pool() done");
	return (newObj);
}


void *
Pool2pool(JNIEnv *env, jobject poolObj) {

	jclass cls;
	vsn_pool_t *pool = (vsn_pool_t *)malloc(sizeof (vsn_pool_t));

	PTRACE(2, "jni:Pool2pool() entry");
	cls = (*env)->GetObjectClass(env, poolObj);

	getStrFld(env, cls, poolObj, "name", pool->pool_name);
	getStrFld(env, cls, poolObj, "mediaType", pool->media_type);
	pool->vsn_names = jarray2lst(env,
	    getJArrFld(env, cls, poolObj, "vsnNames", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);
	PTRACE(2, "jni:Pool2pool() done");
	return (pool);
}


/* deprecated */
jobject
poolprop2PoolProps(JNIEnv *env, void *v_pprop) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	vsnpool_property_t *pprop = (vsnpool_property_t *)v_pprop;

	PTRACE(2, "jni:poolprop2PoolProps() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/VSNPoolProps");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IJJ[L"BASEPKG"/media/CatEntry;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(pprop->name),
	    (jint) pprop->number_of_vsn,
	    (jlong) (pprop->capacity / 1024), // kb
	    (jlong) (pprop->free_space /1024), // kb
	    lst2jarray(env,
		pprop->catalog_entry_list,
		BASEPKG"/media/CatEntry",
		catentry2CatEntry));
	PTRACE(2, "jni:poolprop2PoolProps() done");
	return (newObj);
}


jobject
catpoolprop2CatPoolProps(JNIEnv *env, void *v_pprop) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	vsnpool_property_t *pprop = (vsnpool_property_t *)v_pprop;

	PTRACE(2, "jni:catpoolprop2CatPoolProps() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/CatVSNPoolProps");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IJJ[L"
	    BASEPKG"/media/CatEntry;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(pprop->name),
	    (jint) pprop->number_of_vsn,
	    (jlong) (pprop->capacity / 1024), // kb
	    (jlong) (pprop->free_space /1024), // kb
	    lst2jarray(env,
		pprop->catalog_entry_list,
		BASEPKG"/media/CatEntry",
		catentry2CatEntry),
	    JSTRING(pprop->media_type));
	PTRACE(2, "jni:catpoolprop2CatPoolProps() done");
	return (newObj);
}


jobject
diskpoolprop2DiskPoolProps(JNIEnv *env, void *v_pprop) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	vsnpool_property_t *pprop = (vsnpool_property_t *)v_pprop;

	PTRACE(2, "jni:diskpoolprop2DiskPoolProps() entry");
	cls = (*env)->FindClass(env, BASEPKG"/arc/DiskVSNPoolProps");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;IJJ[L"BASEPKG
	    "/arc/DiskVol;Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(pprop->name),
	    (jint) pprop->number_of_vsn,
	    (jlong) (pprop->capacity / 1024), // kb
	    (jlong) (pprop->free_space /1024), // kb
	    lst2jarray(env,
		pprop->catalog_entry_list,
		BASEPKG"/arc/DiskVol",
		diskvol2DiskVol),
	    JSTRING(pprop->media_type));
	PTRACE(2, "jni:diskpoolprop2DiskPoolProps() done");
	return (newObj);
}


jobject
map2VSNMap(JNIEnv *env, void *v_map) {

	jclass cls;
	jmethodID mid;
	jobject newObj;
	vsn_map_t *map = (vsn_map_t *)v_map;

	PTRACE(2, "jni:map2VSNMap() entry");

	if (v_map == NULL)
	    return (NULL);

	cls = (*env)->FindClass(env, BASEPKG"/arc/VSNMap");
	mid = (*env)->GetMethodID(env, cls, "<init>",
	    "(Ljava/lang/String;Ljava/lang/String;"
	    "[Ljava/lang/String;[Ljava/lang/String;)V");
	newObj = (*env)->NewObject(env, cls, mid,
	    JSTRING(map->ar_set_copy_name),
	    JSTRING(map->media_type),
	    lst2jarray(env,
		map->vsn_names, "java/lang/String", charr2String),
	    lst2jarray(env,
		map->vsn_pool_names, "java/lang/String", charr2String));
	PTRACE(2, "jni:map2VSNMap() done");
	return (newObj);
}


void *
VSNMap2map(JNIEnv *env, jobject mapObj) {

	jclass cls;
	vsn_map_t *map = NULL;

	if (mapObj == NULL) {
	    return (NULL);
	}
	PTRACE(2, "jni:VSNMap2map() entry");
	map = (vsn_map_t *)malloc(sizeof (vsn_map_t));
	cls = (*env)->GetObjectClass(env, mapObj);
	getStrFld(env, cls, mapObj, "copyName", map->ar_set_copy_name);
	getStrFld(env, cls, mapObj, "mediaType", map->media_type);
	map->vsn_names = jarray2lst(env,
	    getJArrFld(env, cls, mapObj, "vsnNames", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);
	map->vsn_pool_names = jarray2lst(env,
	    getJArrFld(env, cls, mapObj, "poolNames", "[Ljava/lang/String;"),
	    "java/lang/String",
	    String2charr);
	PTRACE(2, "jni:VSNMap2map() done");
	return (map);
}



/* native functions implementation */


/* Part1/2: VSN pools functions */

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getPools(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *pools;

	PTRACE(1, "jni:VSNOp_getPools() entry");
	if (-1 == get_all_vsn_pools(CTX, &pools)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, pools, BASEPKG"/arc/VSNPool", pool2Pool);
	free_vsn_pool_list(pools);
	PTRACE(1, "jni:VSNOp_getPools() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getPool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring poolName) {

	jboolean isCopy;
	jobject newObj;
	vsn_pool_t *pool;
	char *cstr = GET_STR(poolName, isCopy);
	int res;

	PTRACE(1, "jni:VSNOp_getPool(...,%s)", cstr);
	res = get_vsn_pool(CTX, cstr, &pool);
	REL_STR(poolName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = pool2Pool(env, pool);
	free_vsn_pool(pool);
	PTRACE(1, "jni:VSNOp_getPool() done");
	return (newObj);
}

/* deprecated */
JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getPoolProps(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring poolName,
    jint start, jint size, jshort sortMet, jboolean asc) {

	jboolean isCopy;
	jobject newObj;
	vsnpool_property_t *pprop;
	char *cstr = GET_STR(poolName, isCopy);
	int res;

	PTRACE(1, "jni:VSNOp_getPoolProps(...,%s)", cstr);
	res = get_properties_of_archive_vsnpool(CTX, cstr, (int)start,
	    (int)size, (vsn_sort_key_t)sortMet, CBOOL(asc), &pprop);
	REL_STR(poolName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = poolprop2PoolProps(env, pprop);
	free_vsnpool_property(pprop);
	PTRACE(1, "jni:VSNOp_getPoolProps() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getCopyUsingPool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring poolName) {

	jboolean isCopy;
	char *cstr = GET_STR(poolName, isCopy);
	int res;
	boolean_t in_use;
	uname_t copyname;

	PTRACE(1, "jni:VSNOp_getCopyUsingPool(...,%s) entry", Str(cstr));
	res = is_pool_in_use(CTX, cstr, &in_use, copyname);
	REL_STR(poolName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	PTRACE(1, "jni:VSNOp_getCopyUsingPool(...,%s) done", Str(cstr));
	if (in_use == B_TRUE)
		return (JSTRING(copyname));
	else
		return (NULL);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_addPool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject poolObj) {

	vsn_pool_t *pool;
	int res;

	PTRACE(1, "jni:VSNOp_addPool() entry");
	pool = Pool2pool(env, poolObj);
	res = add_vsn_pool(CTX, pool);
	free_vsn_pool(pool);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:VSNOp_addPool() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_modifyPool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject poolObj) {

	vsn_pool_t *pool;
	int res;

	PTRACE(1, "jni:VSNOp_modifyPool() entry");
	pool = Pool2pool(env, poolObj);
	res = modify_vsn_pool(CTX, pool);
	free_vsn_pool(pool);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:VSNOp_modifyPool() done");

}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_removePool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring poolName) {

	jboolean isCopy;
	char *cstr = GET_STR(poolName, isCopy);
	int res;

	PTRACE(1, "jni:VSNOp_removePool(...,%s) entry", Str(cstr));
	res = remove_vsn_pool(CTX, cstr);
	REL_STR(poolName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:VSNOp_removePool(%s) done", Str(cstr));
}


/* Part2/2: VSN Maps functions */

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getMaps(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx) {

	jobjectArray newArr;
	sqm_lst_t *maps;

	PTRACE(1, "jni:VSNOp_getMaps() entry");
	if (-1 == get_all_vsn_copy_maps(CTX, &maps)) {
		ThrowEx(env);
		return (NULL);
	}
	newArr = lst2jarray(env, maps, BASEPKG"/arc/VSNMap", map2VSNMap);
	free_vsn_map_list(maps);
	PTRACE(1, "jni:VSNOp_getMaps() done");
	return (newArr);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getMap(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring copyName) {

	jboolean isCopy;
	jobject newObj;
	vsn_map_t *map;
	char *cstr = GET_STR(copyName, isCopy);
	int res;

	PTRACE(1, "jni:VSNOp_getMap(...,%s) entry", Str(cstr));
	res = get_vsn_copy_map(CTX, cstr, &map);
	REL_STR(copyName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return (NULL);
	}
	newObj = map2VSNMap(env, map);
	free_vsn_map(map);
	PTRACE(1, "jni:VSNOp_getMap(...,%s) done", Str(cstr));
	return (newObj);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_addMap(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject mapObj) {

	vsn_map_t *map;
	PTRACE(1, "jni:VSNOp_addMap() entry");
	map = VSNMap2map(env, mapObj);
	if (-1 == add_vsn_copy_map(CTX, map)) {
		ThrowEx(env);
		return;
	}
	free_vsn_map(map);
	PTRACE(1, "jni:VSNOp_addMap() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_modifyMap(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject mapObj) {

	vsn_map_t *map;
	int res;

	PTRACE(1, "jni:VSNOp_modifyMap() entry");
	map = VSNMap2map(env, mapObj);
	res = modify_vsn_copy_map(CTX, map);
	free_vsn_map(map);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:VSNOp_modifyMap() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_removeMap(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jstring copyName) {

	jboolean isCopy;
	char *cstr = GET_STR(copyName, isCopy);
	int res;

	PTRACE(1, "jni:VSNOp_removeMap(...,%s) entry", Str(cstr));
	res = remove_vsn_copy_map(CTX, cstr);
	REL_STR(copyName, cstr, isCopy);
	if (-1 == res) {
		ThrowEx(env);
		return;
	}
	PTRACE(1, "jni:VSNOp_removeMap(...,%s) done", Str(cstr));
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getPoolPropsByPool(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject poolObj,
    jint start, jint size, jshort sortMet, jboolean asc) {

	vsn_pool_t *pool;
	jobject newObj;
	vsnpool_property_t *pprop;
	int res;

	pool = Pool2pool(env, poolObj);

	PTRACE(1, "jni:VSNOp_getPoolPropsByPool(...,%s)", pool->media_type);
	res = get_vsn_pool_properties(CTX, pool, (int)start,
	    (int)size, (vsn_sort_key_t)sortMet, CBOOL(asc), &pprop);

	if (-1 == res) {
		free_vsn_pool(pool);
		ThrowEx(env);
		return (NULL);
	}

	if ((strcmp(pool->media_type, DISK_MEDIA) == 0) ||
	    (strcmp(pool->media_type, STK5800_MEDIA) == 0)) {
		newObj = diskpoolprop2DiskPoolProps(env, pprop);
	} else {
		newObj = catpoolprop2CatPoolProps(env, pprop);
	}
	free_vsn_pool(pool);
	free_vsnpool_property(pprop);
	PTRACE(1, "jni:VSNOp_getPoolPropsByPool() done");
	return (newObj);
}


JNIEXPORT jobject JNICALL
Java_com_sun_netstorage_samqfs_mgmt_arc_VSNOp_getPoolPropsByMap(JNIEnv *env,
    jclass cls /*ARGSUSED*/, jobject ctx, jobject mapObj,
    jint start, jint size, jshort sortMet, jboolean asc) {

	vsn_map_t *map;
	jobject newObj;
	vsnpool_property_t *pprop;
	int res;

	map = VSNMap2map(env, mapObj);

	PTRACE(1, "jni:VSNOp_getPoolPropsByMap(...,%s)", map->media_type);
	res = get_vsn_map_properties(CTX, map, (int)start,
	    (int)size, (vsn_sort_key_t)sortMet, CBOOL(asc), &pprop);

	if (-1 == res) {
		free_vsn_map(map);
		ThrowEx(env);
		return (NULL);
	}

	if ((strcmp(map->media_type, DISK_MEDIA) == 0) ||
	    (strcmp(map->media_type, STK5800_MEDIA) == 0)) {
		newObj = diskpoolprop2DiskPoolProps(env, pprop);
	} else {
		newObj = catpoolprop2CatPoolProps(env, pprop);
	}
	free_vsn_map(map);
	free_vsnpool_property(pprop);
	PTRACE(1, "jni:VSNOp_getPoolPropsByMap() done");
	return (newObj);
}
