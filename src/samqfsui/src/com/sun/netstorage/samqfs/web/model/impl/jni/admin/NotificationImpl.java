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

// ident	$Id: NotificationImpl.java,v 1.16 2008/12/16 00:12:19 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.admin;


import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.NotifSummary;
import com.sun.netstorage.samqfs.web.model.admin.Notification;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class NotificationImpl implements Notification {


    private NotifSummary jniNotif = null;

    private String name = new String();
    private String emailAddress = new String();
    private String origEmailAddress = new String();
    private boolean deviceDownNotify = false;
    private boolean archiverInterruptNotify = false;
    private boolean reqMediaNotify = false;
    private boolean recycleNotify = false;
    private boolean dumpInterruptNotify = false;
    private boolean fsNospaceNotify = false;

    // API version 1.5.4 and above
    private boolean hwmExceedNotify = false;
    private boolean acslsErrNotify = false;
    private boolean acslsWarnNotify = false;
    private boolean dumpWarnNotify = false;
    private boolean longWaitTapeNotify = false;
    // valid only  in intellistore
    private boolean fsInconsistentNotify = false;
    private boolean systemHealthNotify = false;


    public NotificationImpl() {
    }


    public NotificationImpl(String name, String emailAddress,
        boolean deviceDownNotify,
        boolean archiverInterruptNotify,
        boolean reqMediaNotify,
        boolean recycleNotify) throws SamFSException {

        if ((!SamQFSUtil.isValidString(name)) ||
            (!SamQFSUtil.isValidString(emailAddress)))
            throw new SamFSException("logic.invalidNameOrAddress");

        this.name = name;
        this.emailAddress = emailAddress;
        this.deviceDownNotify = deviceDownNotify;
        this.archiverInterruptNotify = archiverInterruptNotify;
        this.reqMediaNotify = reqMediaNotify;
        this.recycleNotify = recycleNotify;

    }

    public NotificationImpl(String name, String emailAddress,
        boolean deviceDownNotify,
        boolean archiverInterruptNotify,
        boolean reqMediaNotify,
        boolean recycleNotify,
        boolean dumpInterruptNotify,
        boolean fsNospaceNotify) throws SamFSException {

        this(name, emailAddress,
            deviceDownNotify,
            archiverInterruptNotify,
            reqMediaNotify,
            recycleNotify);

        this.dumpInterruptNotify = dumpInterruptNotify;
        this.fsNospaceNotify = fsNospaceNotify;

    }

    public NotificationImpl(String name, String emailAddress,
        boolean deviceDownNotify,
        boolean archiverInterruptNotify,
        boolean reqMediaNotify,
        boolean recycleNotify,
        boolean dumpInterruptNotify,
        boolean fsNospaceNotify,
        boolean hwmExceedNotify,
        boolean acslsErrNotify,
        boolean acslsWarnNotify,
        boolean dumpWarnNotify,
        boolean longWaitTapeNotify,
        boolean fsInconsistentNotify,
        boolean systemHealthNotify) throws SamFSException {


        this(name, emailAddress,
            deviceDownNotify,
            archiverInterruptNotify,
            reqMediaNotify,
            recycleNotify);

        this.dumpInterruptNotify = dumpInterruptNotify;
        this.fsNospaceNotify = fsNospaceNotify;
        this.hwmExceedNotify = hwmExceedNotify;
        this.acslsErrNotify = acslsErrNotify;
        this.acslsWarnNotify = acslsWarnNotify;
        this.dumpWarnNotify = dumpWarnNotify;
        this.longWaitTapeNotify = longWaitTapeNotify;
        this.fsInconsistentNotify = fsInconsistentNotify;
        this.systemHealthNotify = systemHealthNotify;
    }

    public NotificationImpl(NotifSummary notif) {

        this.jniNotif = notif;
        setup();

    }


    public NotifSummary getJniNotif() {

        return jniNotif;

    }


    public String getName() throws SamFSException {

        return name;

    }


    public void setName(String name) throws SamFSException {

        this.name = name;

    }


    public String getEmailAddress() throws SamFSException {

        return emailAddress;

    }


    public String getOrigEmailAddress() throws SamFSException {

        return origEmailAddress;

    }


    public void setEmailAddress(String email) throws SamFSException {

        this.emailAddress = email;

    }


    public boolean isDeviceDownNotify() throws SamFSException {

        return deviceDownNotify;

    }


    public void setDeviceDownNotify(boolean devDown) throws SamFSException {

        this.deviceDownNotify = devDown;

    }


    public boolean isArchiverInterruptNotify() throws SamFSException {

        return archiverInterruptNotify;

    }


    public void setArchiverInterruptNotify(boolean archIntr)
    throws SamFSException {

        this.archiverInterruptNotify = archIntr;

    }


    public boolean isReqMediaNotify() throws SamFSException {

        return reqMediaNotify;

    }


    public void setReqMediaNotify(boolean reqMedia) throws SamFSException {

        this.reqMediaNotify = reqMedia;

    }


    public boolean isRecycleNotify() throws SamFSException {

        return recycleNotify;

    }


    public void setRecycleNotify(boolean recycle) throws SamFSException {

        this.recycleNotify = recycle;

    }


    public boolean isDumpInterruptNotify() throws SamFSException {
        return dumpInterruptNotify;
    }


    public void setDumpInterruptNotify(boolean dumpIntr) throws SamFSException {
        this.dumpInterruptNotify = dumpIntr;
    }


    public boolean isFsNospaceNotify() {
        return fsNospaceNotify;
    }


    public void setFsNospaceNotify(boolean fsNospace) {
        this.fsNospaceNotify = fsNospace;
    }

    public boolean isHwmExceedNotify() {
        return hwmExceedNotify;
    }
    public void setHwmExceedNotify(boolean hwmExceed) {
        this.hwmExceedNotify = hwmExceed;
    }

    public boolean isAcslsErrNotify() {
        return acslsErrNotify;
    }
    public void setAcslsErrNotify(boolean acslsErr) {
        this.acslsErrNotify = acslsErr;
    }

    public boolean isFsInconsistentNotify() {
        return fsInconsistentNotify;
    }
    public void setFsInconsistentNotify(boolean fsInconsistent) {
        this.fsInconsistentNotify = fsInconsistent;
    }

    public boolean isAcslsWarnNotify() {
        return acslsWarnNotify;
    }
    public void setAcslsWarnNotify(boolean acslsWarn) {
        this.acslsWarnNotify = acslsWarn;
    }

    public boolean isDumpWarnNotify() {
        return dumpWarnNotify;
    }
    public void setDumpWarnNotify(boolean dumpWarn) {
        this.dumpWarnNotify = dumpWarn;
    }

    public boolean isLongWaitTapeNotify() {
        return longWaitTapeNotify;
    }

    public void setLongWaitTapeNotify(boolean longWaitTape) {
        this.longWaitTapeNotify = longWaitTape;
    }

    public boolean isSystemHealthNotify() {
        return systemHealthNotify;
    }
    public void setSystemHealthNotify(boolean systemHealth) {
        this.systemHealthNotify = systemHealth;
    }
    public String toString() {

        StringBuffer buf = new StringBuffer();

        if (name != null)
            buf.append("Name: " + name + "\n");
        if (emailAddress != null)
            buf.append("Email Address: " + emailAddress + "\n");
        buf.append("Device Down Notify: " + deviceDownNotify + "\n");
        buf.append("Archiver Interrupt Notify: " +
            archiverInterruptNotify + "\n");
        buf.append("Req Media Notify: " + reqMediaNotify + "\n");
        buf.append("Recycle Notify: " + recycleNotify  + "\n");
        buf.append("Dump Interrupt Notify: " + dumpInterruptNotify  + "\n");
        return buf.toString();

    }


    private void setup() {

        if (jniNotif != null) {

            name = jniNotif.getAdminName();
            emailAddress = jniNotif.getEmailAddr();
            origEmailAddress = emailAddress;
            boolean[] bools = jniNotif.getSubj();
            deviceDownNotify = bools[NotifSummary.NOTIF_SUBJ_DEVICEDOWN];
            archiverInterruptNotify = bools[NotifSummary.NOTIF_SUBJ_ARCHINTR];
            reqMediaNotify = bools[NotifSummary.NOTIF_SUBJ_MEDREQD];
            recycleNotify =  bools[NotifSummary.NOTIF_SUBJ_RECYSTATRC];
            dumpInterruptNotify = bools[NotifSummary.NOTIF_SUBJ_DUMPINTR];
            fsNospaceNotify = bools[NotifSummary.NOTIF_SUBJ_FSNOSPACE];

            if (jniNotif.getSubj().length == 6) {
                return;
            } // 4.6 and below subj = 6
            hwmExceedNotify = bools[NotifSummary.NOTIF_SUBJ_HWMEXCEED];
            acslsErrNotify = bools[NotifSummary.NOTIF_SUBJ_ACSLSERR];
            acslsWarnNotify = bools[NotifSummary.NOTIF_SUBJ_ACSLSWARN];
            dumpWarnNotify = bools[NotifSummary.NOTIF_SUBJ_DUMPWARN];
            longWaitTapeNotify = bools[NotifSummary.NOTIF_SUBJ_LONGWAITTAPE];
            fsInconsistentNotify =
                bools[NotifSummary.NOTIF_SUBJ_FSINCONSISTENT];
            systemHealthNotify = bools[NotifSummary.NOTIF_SUBJ_SYSTEMHEALTH];
        }
    }
}
