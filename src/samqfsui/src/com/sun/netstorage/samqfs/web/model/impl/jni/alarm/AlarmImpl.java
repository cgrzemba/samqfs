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

// ident	$Id: AlarmImpl.java,v 1.16 2008/12/16 00:12:19 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.alarm;

import java.util.GregorianCalendar;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Fault;
import com.sun.netstorage.samqfs.mgmt.adm.FaultAttr;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;


public class AlarmImpl implements Alarm {

    private SamQFSSystemModelImpl model = null;
    private FaultAttr fault = null;
    private long index = -1; // same as error ID
    private String compId = new String();
    private int severity = -1;
    private String description = new String(); // same as message
    private GregorianCalendar dateTimeGenerated = null;
    private int status = -1; // same as state
    private String hostName = new String();
    private int associatedEquipOrdinal = -1;
    private Library associatedLibrary = null;
    private String libName = new String();
    private FileSystem associatedFileSystem = null;


    public AlarmImpl() {
    }


    public AlarmImpl(long index, int severity, String description,
                     GregorianCalendar dateTimeGenerated, int status,
                     Library associatedLibrary,
                     FileSystem associatedFileSystem) {

        this.index = index;
	this.severity = severity;
	this.description = description;
	this.dateTimeGenerated = dateTimeGenerated;
	this.status = status;

	this.associatedLibrary = associatedLibrary;
	this.associatedFileSystem = associatedFileSystem;

        try {
            if (associatedLibrary != null)
                associatedEquipOrdinal
                    = associatedLibrary.getEquipOrdinal();
            else if (associatedFileSystem != null)
                associatedEquipOrdinal
                    = associatedFileSystem.getEquipOrdinal();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public AlarmImpl(SamQFSSystemModelImpl model, FaultAttr fault) {

        this.model = model;
        this.fault = fault;
        update();

    }


    // getters


    public long getAlarmID() throws SamFSException {

        return index;

    }


    public int getSeverity() throws SamFSException {

	return severity;

    }


    public String getDescription() throws SamFSException {

	return description;

    }


    public GregorianCalendar getDateTimeGenerated() throws SamFSException {

	return dateTimeGenerated;

    }


    public int getStatus() throws SamFSException {

	return status;

    }


    public void setStatusAcknowledged() {

        this.status = Alarm.ACKNOWLEDGED;

    }


    public String getComponentName() {

        return compId;

    }


    public String getHostName() {

        return hostName;

    }


    public void setStatus(int status) throws SamFSException {

        switch (status) {

            case Alarm.ACKNOWLEDGED:
                this.status = status;
                if ((model != null) && (fault != null)) {
                    long[] list = new long[1];
                    list[0] = fault.getErrID();
                    Fault.ack(model.getJniContext(), list);
                }
                break;
            default:
                this.status = -1;
        }

    }


    /**
     * Alarm could be from many different sources; the only
     * thing that is guaranteed is that there would be an
     * equipment ordinal number associated with the alarm;
     * among all other getter methods, only one will return
     * non-null value
     */

    public int getAssociatedEquipOrdinal() throws SamFSException {

	return associatedEquipOrdinal;

    }


    public Library getAssociatedLibrary() throws SamFSException {

        if ((SamQFSUtil.isValidString(libName)) &&
            (associatedEquipOrdinal > 0) &&
            (associatedEquipOrdinal <= MAXEQ)) {
            associatedLibrary =
                model.getSamQFSSystemMediaManager().
                    getLibraryByEQ(associatedEquipOrdinal);
        } else {
            associatedLibrary = null;
        }

	return associatedLibrary;

    }

    public String getAssociatedLibraryName() throws SamFSException {

        return
            libName.equals(FaultAttr.NO_LIBRARY) ?
                "" :
                libName;
    }

    public FileSystem getAssociatedFileSystem() throws SamFSException {

	return associatedFileSystem;

    }

    public String toString() {

        StringBuffer buf = new StringBuffer();

        try {
            if (associatedLibrary != null)
                buf.append("Library: " + associatedLibrary.getName() + "\n\n");

            else if (associatedFileSystem != null) {
                buf.append("Filesystem: " + associatedFileSystem.getName());
                buf.append("\n\n");
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        buf.append("Alarm Index: " + index + "\n");
        buf.append("Severity: " + severity + "\n");
        buf.append("Description: " + description + "\n");
        buf.append("Generated on : " + SamQFSUtil.dateTime(dateTimeGenerated)
                   + "\n");
        buf.append("Status: " + status + "\n");
        buf.append("Associated Equip Ordinal: " +
                    associatedEquipOrdinal + "\n");
        buf.append("Library Name: " + libName + "\n");

        return buf.toString();

    }

    private void update() {

        if (fault != null) {

            index = fault.getErrID();
            compId = fault.getComponentID();

            switch (fault.getSeverity()) {
                case FaultAttr.FLT_SEV_CRITICAL:
                    severity = Alarm.CRITICAL;
                    break;
                case FaultAttr.FLT_SEV_MAJOR:
                    severity = Alarm.MAJOR;
                    break;
                case FaultAttr.FLT_SEV_MINOR:
                    severity = Alarm.MINOR;
                    break;
                default:
                    severity = Alarm.OK;
            }

            description = fault.getMessage();
            dateTimeGenerated = SamQFSUtil.convertTime(fault.getTimestamp());

            if (fault.getState() == FaultAttr.FLT_ST_UNRESOLVED) {
                status = Alarm.ACTIVE;
            } else if (fault.getState() == FaultAttr.FLT_ST_ACKED) {
                status = Alarm.ACKNOWLEDGED;
            }

            associatedEquipOrdinal = fault.getEq();
            libName = fault.getLibName();

            hostName = fault.getHostname();

        }
    }
}
