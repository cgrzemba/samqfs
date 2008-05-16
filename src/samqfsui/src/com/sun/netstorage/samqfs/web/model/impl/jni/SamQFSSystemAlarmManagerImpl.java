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

// ident	$Id: SamQFSSystemAlarmManagerImpl.java,v 1.9 2008/05/16 18:39:00 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import java.util.ArrayList;

import com.sun.netstorage.samqfs.mgmt.adm.Fault;
import com.sun.netstorage.samqfs.mgmt.adm.FaultAttr;
import com.sun.netstorage.samqfs.mgmt.adm.FaultSummary;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;
import com.sun.netstorage.samqfs.web.model.impl.jni.alarm.AlarmSummaryImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAlarmManager;
import com.sun.netstorage.samqfs.web.model.impl.jni.alarm.AlarmImpl;

public class SamQFSSystemAlarmManagerImpl implements SamQFSSystemAlarmManager {

    private SamQFSSystemModelImpl theModel;
    private ArrayList alarms = new ArrayList();

    public SamQFSSystemAlarmManagerImpl(SamQFSSystemModel model) {

	theModel = (SamQFSSystemModelImpl) model;
    }

    public AlarmSummary getAlarmSummary() throws SamFSException {

        FaultSummary jniFaultSummary = Fault.getSummary(
            theModel.getJniContext());

        return new AlarmSummaryImpl(jniFaultSummary);
    }


    public Alarm[] getAllAlarms() throws SamFSException {

        byte b = -1;
        FaultAttr [] faults = Fault.getAll(theModel.getJniContext());

        alarms.clear();
        if ((faults != null) && (faults.length > 0)) {
            for (int i = 0; i < faults.length; i++) {
                AlarmImpl a = new AlarmImpl(theModel, faults[i]);
                alarms.add(a);
            }
        }

        return (Alarm[]) alarms.toArray(new Alarm[0]);

    }


    public Alarm getAlarm(long alarmID) throws SamFSException {

        Alarm alarm = null;

        int index = getAlarmIndexInList(alarmID);
        if (index != -1)
            alarm = (Alarm) alarms.get(index);

        return alarm;

    }


    public void deleteAlarm(long[] alarmIDs) throws SamFSException {

        if (alarmIDs != null) {

            Fault.delete(theModel.getJniContext(), alarmIDs);
            for (int i = 0; i < alarmIDs.length; i++) {
                int index = getAlarmIndexInList(alarmIDs[i]);
                if (index != -1)
                    alarms.remove(index);
            }

        }

    }


    public void acknowledgeAlarm(long[] alarmIDs) throws SamFSException {

        AlarmImpl alarm = null;

        if (alarmIDs != null) {

            Fault.ack(theModel.getJniContext(), alarmIDs);
            for (int i = 0; i < alarmIDs.length; i++) {
                alarm = (AlarmImpl) getAlarm(alarmIDs[i]);
                if (alarm != null)
                    alarm.setStatusAcknowledged();
            }

        }

    }

    private int getAlarmIndexInList(long alarmID) {

        int index = -1;
        Alarm temp = null;

        if (alarms != null) {
            for (int i = 0; i < alarms.size(); i++) {
                try {
                    temp = (Alarm) alarms.get(i);
                    if (temp.getAlarmID() == alarmID) {
                        index = i;
                        break;
                    }
                } catch (Exception e) {
                }
            }
        }

        return index;

    }


    public Alarm[] getAllAlarmsByLibName(String libName) throws SamFSException {

        FaultAttr[] faults = Fault.getByLibName(theModel.getJniContext(),
                                                libName);
        ArrayList associatedAlarms = new ArrayList();
        if ((faults != null) && (faults.length > 0)) {
            for (int i = 0; i < faults.length; i++) {
                AlarmImpl a = new AlarmImpl(theModel, faults[i]);
                associatedAlarms.add(a);
            }
        }

        return (Alarm[]) associatedAlarms.toArray(new Alarm[0]);

    }

}
