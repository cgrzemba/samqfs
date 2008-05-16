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

// ident	$Id: TaskSchedule.java,v 1.6 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;


public class TaskSchedule {

    /**
     * Returns an array of key value pair strings describing the tasks
     * scheduled in the system.
     *
     * Each key value string may have the following keys:
     * task = <2 character string taskid>
     * starttime   = <date>,
     * periodicity = <period>
     * duration    = <duration>
     * id   =
     *  <1072 character secondary id used to identify instances of
     *   certain types of task see below>
     *
     * <date> format is: YYYYMMDDhhmm
     *
     * <duration> and <period> are an integer value, followed by a unit among:
     * "s" (seconds)
     * "m" (minutes, 60s)
     * "h" (hours, 3 600s)
     * "d" (days, 86 400s)
     * "w" (weeks, 604 800s)
     * "y" (years, 31 536 000s)
     *
     * In addition the following periodicity units are supported for
     * the mgmt tasks:
     * M (months)
     * D (<n>th day of the month)
     * W (<n>th day of the week - 0 = Sunday, 6 = Saturday)
     *
     *
     * IAS Tasklets Task IDS (Task description):
     * AW (Auto Worm)
     * HC (Hash Computation)
     * HI (Hash Indexing)
     * PA (Periodic Audit)
     * AD (Automatic Deletion)
     * DD (De-Duplication)
     *
     * For all IAS tasklet schedules:
     * The keys task, starttime, periodicity and duration are mandatory.
     * The id key should not be present.
     *
     * SAM/QFS Task IDS (Task Description)
     * RC (Recycler Schedule)
     * SN (Snapshot Schedule will contain additional keys)
     * RP (Report Schedule will contain additional keys)
     *
     * The keys task, starttime and periodicity are mandatory for all
     * SAM/QFS schedules. The key duration is not supported. Additional
     * keys may be required for certain task types including the id field
     * which allows the scheduling of multiple instances of a type of task to
     * operate on different targets.
     *
     * @param schedules[] array of key value pair strings
     */
    public static native String[] getTaskSchedules(Ctx c)
	throws SamFSException;


    /**
     * getSpecificTasks retrieves only those task schedules which match
     * specifid task classes and/or id values.)   If either task or id
     * are specified, they must have a match in the configuration file for
     * values to be returned.  If both task and id are NULL, all tasks
     * are returned.
     *
     * The returned key/value pair string is the same as that returned
     * from getTaskSchedules.
     *
     * @param schedules[] array of key value pair strings
     */
    public static native String[] getSpecificTasks(Ctx c, String task,
	String id)
    	throws SamFSException;


    /**
     * setTaskSchedule will either create or update the schedule identified
     * in the key value pair string. The key value string must be in the
     * same format returned by getTaskSchedules.
     *
     * If an id key is present in either the configuration file or the
     * input string its value will be used for the match.
     *
     * @param schedules	key value string
     */
    public static native void setTaskSchedule(Ctx c, String schedule)
    	throws SamFSException;


    /**
     * removeTaskSchedule will remove the schedule identified in the
     * key value pair string. If an id key is present in either the
     * configuration file or the input string its value will be used
     * for the match.
     *
     * @param kv_string key value string
     */
    public static native void removeTaskSchedule(Ctx c, String kv_string)
    	throws SamFSException;
}
