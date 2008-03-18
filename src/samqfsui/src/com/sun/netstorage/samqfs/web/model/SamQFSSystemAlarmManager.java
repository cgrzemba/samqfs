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

// ident	$Id: SamQFSSystemAlarmManager.java,v 1.7 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;


/**
 *
 * This interface is used to manage alarms associated
 * with an individual server.
 *
 */
public interface SamQFSSystemAlarmManager {

    /**
     *
     * Returns the AlarmSummary for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return AlarmSummary
     *
     */
    public AlarmSummary getAlarmSummary() throws SamFSException;


    /**
     *
     * Returns an array containing all alarm objects for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return an array containing all alarm objects for this server.
     *
     */
    public Alarm[] getAllAlarms() throws SamFSException;

    /**
     *
     * Returns an array containing all alarm objects for the library.
     * @throws SamFSException if anything unexpected happens.
     * @return an array containing all alarm objects for the library.
     *
     */
    public Alarm[] getAllAlarmsByLibName(String libName) throws SamFSException;


    /**
     *
     * Searches for the alarm with the specified ID.
     * @param alarmID
     * @throws SamFSException if anything unexpected happens.
     * @return The matching alarm or <code>null</code> if there
     * was no match.
     *
     */
    public Alarm getAlarm(long alarmID) throws SamFSException;


    /**
     *
     * Acknowledge the alarms that have IDs contained in the array
     * that was passed in.
     * @param alarmIDs
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void acknowledgeAlarm(long[] alarmIDs) throws SamFSException;


    /**
     *
     * Delete the alarms that have IDs contained in the array
     * that was passed in.
     * @param alarmIDs
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void deleteAlarm(long[] alarmIDs) throws SamFSException;
}
