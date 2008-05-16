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
#pragma ident	"$Revision: 1.7 $"

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>

/* API header files */
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/task_schedule.h"
#include "mgmt/log.h"

/* local header files */
#include "com_sun_netstorage_samqfs_mgmt_adm_TaskSchedule.h"
#include "jni_util.h"

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_TaskSchedule_getTaskSchedules
	(JNIEnv *env, jclass cls  /*ARGSUSED*/, jobject ctx) {

	sqm_lst_t *lst = NULL;
	jobjectArray newArr;

	PTRACE(3, "jni:TaskSchedules getTaskSchedules() entry");
	if (-1 == get_task_schedules(CTX, &lst)) {
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);

	PTRACE(3, "jni:TaskSchedules getTaskSchedules done");
	return (newArr);
}

JNIEXPORT jobjectArray JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_TaskSchedule_getSpecificTasks
	(JNIEnv *env, jclass cls  /*ARGSUSED*/, jobject ctx, jstring task,
		jstring id) {

	sqm_lst_t		*lst = NULL;
	jboolean	isCopy;
	jboolean	isCopy2;
	char		*taskStr = GET_STR(task, isCopy);
	char		*idStr = GET_STR(id, isCopy2);

	jobjectArray newArr;

	PTRACE(3, "jni:TaskSchedules getSpecificTasks() entry");
	if (-1 == get_specific_tasks(CTX, taskStr, idStr, &lst)) {
		REL_STR(task, taskStr, isCopy);
		REL_STR(id, idStr, isCopy2);
		ThrowEx(env);
		return (NULL);
	}

	newArr = lst2jarray(env, lst, "java/lang/String", charr2String);
	lst_free_deep(lst);
	REL_STR(task, taskStr, isCopy);
	REL_STR(id, idStr, isCopy2);

	PTRACE(3, "jni:TaskSchedules getSpecificTasks done");
	return (newArr);
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_TaskSchedule_setTaskSchedule
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring schedule) {

	jboolean isCopy;
	char *schedStr = GET_STR(schedule, isCopy);

	PTRACE(3, "jni:TaskSchedules setTaskSchedules() entry");
	if (-1 == set_task_schedule(CTX, schedStr)) {
		REL_STR(schedule, schedStr, isCopy);
		ThrowEx(env);
	}

	REL_STR(schedule, schedStr, isCopy);

	PTRACE(3, "jni:TaskSchedules setTaskSchedules() done");
}


JNIEXPORT void JNICALL
Java_com_sun_netstorage_samqfs_mgmt_adm_TaskSchedule_removeTaskSchedule
(JNIEnv *env, jclass cls /*ARGSUSED*/, jobject ctx, jstring schedule) {

	jboolean isCopy;
	char *schedStr = GET_STR(schedule, isCopy);

	PTRACE(3, "jni:TaskSchedules removeTaskSchedules() entry");
	if (-1 == remove_task_schedule(CTX, schedStr)) {
		REL_STR(schedule, schedStr, isCopy);
		ThrowEx(env);
		return;
	}

	REL_STR(schedule, schedStr, isCopy);

	PTRACE(3, "jni:TaskSchedules removeTaskSchedules() done");
}
