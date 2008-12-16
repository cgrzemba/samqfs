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

// ident	$Id: MediaUtil.java,v 1.24 2008/12/16 00:12:14 am143972 Exp $

/**
 * This util class contains a few helper methods that are used in code related
 * to library and drives.
 */

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.ContainerView;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;

import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCSeverity;
import com.sun.web.ui.view.alarm.CCAlarmObject;

import com.sun.web.ui.view.alert.CCAlertInline;
import java.util.HashMap;
import java.util.Iterator;

public class MediaUtil {

    /**
     * This method returns the library object with the specified server name
     * and library name
     *
     * @param serverName - The name of the server of which the library is in
     * @param libName    - The name of the library of which is to retrieve
     * @return The library object itself
     */
    public static Library getLibraryObject(String serverName, String libName)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(libName);
        if (myLibrary == null) {
            TraceUtil.trace1("MediaUtil.getLibraryObject returns null!");
            TraceUtil.trace1("MediaUtil: serverName is " + serverName +
                " libName is " + libName);
            throw new SamFSException(null, -2502);
        } else {
            return myLibrary;
        }
    }

    /**
     * This method is explicitly used by Label Tape in Media Management Tab.
     * This method is used in VSN Summary and VSN Details page.
     *
     * There is a slight different in between this method and the original
     * setInfoAlert() method.
     * -> Instead of using "success.detail", use "issue.detail"
     *
     */
    public static void setLabelTapeInfoAlert(ContainerView view,
        String varName, String errMsg, String errorMsg) {
        TraceUtil.trace3("Entering");
        // see if there is an error
        CCAlertInline alert = (CCAlertInline) view.getChild(varName);

        boolean errorAlertPresent = false;
        String detailMsg = "";

        // see if there is already a successful alert message in the
        // CCAlert component, but need to check if there is a message
        // in there as well as Lockhart defaults over to
        // CCAlertInline.TYPE_INFO when calling getValue()

        String alertValue = (String) alert.getValue();
        detailMsg = alert.getDetail();
        if (alertValue != null) {
            if (alertValue.equals(CCAlertInline.TYPE_ERROR) &&
                detailMsg != null) {
                TraceUtil.trace3("FOUND AN ERROR ALERT");
                errorAlertPresent = true;
                // get the detail message
                alert.setValue(CCAlertInline.TYPE_WARNING);
                alert.setSummary("success.summary");
            }
        }

        if (!errorAlertPresent) {
            alert.setValue(CCAlertInline.TYPE_INFO);
            alert.setSummary(errMsg);
            alert.setDetail(errorMsg);
        } else {
            alert.setDetail(errorMsg.concat("<br>").concat(detailMsg));
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * OBSOLETE
     *
     * Return the number of attributes in a VSN
     * @param vsn - the VSN Object of which you request the number of attributes
     * @return number of attributes in a VSN
     */
    public static int getTotalNumberOfFlags(VSN vsn) throws SamFSException {
        int totalFlags = 0;

        if (vsn.isMediaDamaged()) {
            totalFlags++;
        }
        if (vsn.isDuplicateVSN()) {
            totalFlags++;
        }
        if (vsn.isReadOnly()) {
            totalFlags++;
        }
        if (vsn.isWriteProtected()) {
            totalFlags++;
        }
        if (vsn.isForeignMedia()) {
            totalFlags++;
        }
        if (vsn.isRecycled()) {
            totalFlags++;
        }
        if (vsn.isVolumeFull()) {
            totalFlags++;
        }
        if (vsn.isUnavailable()) {
            totalFlags++;
        }
        if (vsn.isNeedAudit()) {
            totalFlags++;
        }
        return totalFlags;
    }

    /**
     * Return the word description of the media attributes of a VSN
     * @param vsn - the VSN Object of which you request the word description
     * @return the word description (the most important attribute)
     * of the media attributes of the VSN follow by ... if there are more than
     * one flag.
     */
    public static String getFlagsInformation(VSN vsn) throws SamFSException {
        StringBuffer buffer = new StringBuffer();
        String dotDotDot = "...";
        String comma = ", ";

        /*
         * Below is the importance sequence of the media flags.
         * We only show the most important flag, and follow by ... if there
         * is more than one flag.
         *
         * Damaged Media
         * Needs Audit
         * Unavailable
         * Duplicate VSN
         * Volume is full
         * Recycle
         * Read-only
         * Write-protected
         * Foreign Media
         */

        // First of all, state if the vsn is reserved

        boolean atLeastOneFlag = false;

        if (vsn.isReserved()) {
            buffer.append(SamUtil.getResourceString("EditVSN.reserved"));
            buffer.append(comma);
        }

        if (vsn.isMediaDamaged()) {
            buffer.append(SamUtil.getResourceString("EditVSN.damagemedia"));
            atLeastOneFlag = true;
        }

        if (vsn.isNeedAudit()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.needaudit"));
            }
        }
        if (vsn.isUnavailable()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.unavailable"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isDuplicateVSN()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.duplicatevsn"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isVolumeFull()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.volumefull"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isRecycled()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.recycle"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isReadOnly()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.readonly"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isWriteProtected()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.writeprotected"));
                atLeastOneFlag = true;
            }
        }
        if (vsn.isForeignMedia()) {
            if (atLeastOneFlag) {
                return buffer.append(dotDotDot).toString();
            } else {
                buffer.append(
                    SamUtil.getResourceString("EditVSN.foreignmedia"));
                atLeastOneFlag = true;
            }
        }

        return buffer.toString();
    }

    public static String createLibraryEntry(Library myLibrary)
        throws SamFSException {
        StringBuffer myBuf = new StringBuffer();
        String availableSlotWord =
            SamUtil.getResourceString("ServerConfiguration.media.word.slot");

        String prodID = myLibrary.getProductID();
        String vendor = myLibrary.getVendor();
        int type  = myLibrary.getDriverType();
        int slots = myLibrary.getNoOfCatalogEntries();
        Drive [] allDrives = myLibrary.getDrives();

        if (prodID.equals("")) {
            // User library name instead if product ID is blank
            myBuf.append("<b>").append(myLibrary.getName()).
                append("</b><br />");
        } else {
            myBuf.append("<b>").append(vendor).append(" ").append(
                prodID).append("</b><br />");
        }

        // Check the number of the drives inside the library
        // If no Drives are found, show no drives are found.
        if (allDrives != null) {
            if (allDrives.length == 0) {
                myBuf.append(SamUtil.getResourceString(
                    "ServerConfiguration.media.noDrive"));
            } else {
                myBuf.append(createDriveString(allDrives)).append(
                    "<br>").append(slots).append(" ").append(
                    availableSlotWord);
            }
        } else {
            myBuf.append(SamUtil.getResourceString(
                "ServerConfiguration.media.noDrive"));
        }

        return myBuf.toString();
    }

    private static String createDriveString(Drive [] allDrives)
        throws SamFSException {
        HashMap driveMap = new HashMap();
        StringBuffer myBuf = new StringBuffer("");
        String driveWord =
            SamUtil.getResourceString("ServerConfiguration.media.word.drive");

        // Iterate all the drives, collect information on the drives
        // Drives can be different in a library
        // Use product ID as the key
        for (int i = 0; i < allDrives.length; i++) {
            if (driveMap.containsKey(allDrives[i].getProductID())) {
                int count = ((Integer) driveMap.get(
                                allDrives[i].getProductID())).intValue();
                driveMap.put(allDrives[i].getProductID(),
                             new Integer(count + 1));
            } else {
                driveMap.put(allDrives[i].getProductID(), new Integer(1));
            }
        }

        // iterate the HashMap, form display line
        Iterator it1 = driveMap.keySet().iterator();
        while (it1.hasNext()) {
            if (myBuf.length() > 0) {
                myBuf.append("<br>");
            }
            String productID = (String) it1.next();
            int numOfDrives = ((Integer) driveMap.get(productID)).intValue();
            myBuf.append(numOfDrives).append(" ").append(
                    productID).append(" ").append(driveWord);
        }

        return myBuf.toString();
    }

    public static CCAlarmObject getAlarm(int alarmType) {
        switch (alarmType) {
            case ServerUtil.ALARM_CRITICAL:
                return new CCAlarmObject(CCSeverity.CRITICAL);

            case ServerUtil.ALARM_MAJOR:
                return new CCAlarmObject(CCSeverity.MAJOR);

            case ServerUtil.ALARM_MINOR:
                return new CCAlarmObject(CCSeverity.MINOR);

            default:
                return new CCAlarmObject(CCSeverity.OK);
        }
    }

    /**
     * This method checks if the library is an ACSLS library and it is version
     * 4.5 or newer.  This is used to determine if the Library Drive Summary
     * page and Library Drive Details Page should have any shared content.
     */
    public static boolean isDriveSharedCapable(
        String serverName, String libName) throws SamFSException {

        // server is 4.5 or newer and driver is ACSLS, return true
        return getLibraryObject(serverName, libName).getDriverType()
                                                        == Library.ACSLS;
    }

    /**
     * Return the word description of the media attributes of a VSN
     * @param vsn - the VSN Object of which you request the word description
     * @param maxShown - Maximum flags shown in the GUI
     * @return the word description (the most important attribute)
     * of the media attributes of the VSN follow by ... if there are more than
     * one flag.
     */
    public static String getFlagString(VSN vsn, int maxShown)
        throws SamFSException {
        StringBuffer buffer = new StringBuffer();
        String dotDotDot = " ... ";
        String comma = ", ";
        boolean needTotal = false;
        int count = 0;

        /**
         * Below is the importance sequence of the media flags.
         * We only show the most important flag, and follow by ... if there
         * is more than one flag.
         *
         * Damaged Media
         * Needs Audit
         * Unavailable
         * Duplicate VSN
         * Volume is full
         * Recycle
         * Read-only
         * Write-protected
         * Foreign Media
         */

        // First of all, state if the vsn is reserved

        if (vsn.isReserved()) {
            buffer.append(SamUtil.getResourceString("EditVSN.reserved"));
            buffer.append(comma);
            count++;
        }

        if (vsn.isMediaDamaged()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.damagemedia")).append(comma);
                count ++;
            }
        }

        if (vsn.isNeedAudit()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.needaudit")).append(comma);
                count ++;
            }
        }
        if (vsn.isUnavailable()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.unavailable")).append(comma);
                count ++;
            }
        }
        if (vsn.isDuplicateVSN()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.duplicatevsn")).append(comma);
                count ++;
            }
        }
        if (vsn.isVolumeFull()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.volumefull")).append(comma);
                count ++;
            }
        }
        if (vsn.isRecycled()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.recycle")).append(comma);
                count ++;
            }
        }
        if (vsn.isReadOnly()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.readonly")).append(comma);
                count ++;
            }
        }
        if (vsn.isWriteProtected()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.writeprotected")).append(comma);
                count ++;
            }
        }
        if (vsn.isForeignMedia()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(dotDotDot);
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "EditVSN.foreignmedia")).append(comma);
                count ++;
            }
        }

        if (needTotal) {
            return buffer.append("(").append(count).append(")").toString();
        } else if (buffer.length() > 0) {
            return buffer.toString().substring(0, buffer.length() - 2);
        } else {
            return "";
        }
    }

    /**
     * construct the 'media flags' display string
     */
    public static String getFlagString(DiskVolume vsn, int maxShown) {
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        int count = 0;
        boolean needTotal = false;

        // check bad media
        if (vsn.isBadMedia()) {
            buffer.append(SamUtil.getResourceString(
                "archiving.diskvsn.flags.badmedia")).append(", ");
            count++;
        }

        // check unavailable
        if (vsn.isUnavailable()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(" ... ");
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.unavailable")).append(", ");
                count ++;
            }
        }

        // check readOnly
        if (vsn.isReadOnly()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(" ... ");
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.readonly")).append(", ");
                count ++;
            }
        }

        // check labeleled
        if (vsn.isLabeled()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(" ... ");
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.labeled")).append(", ");
                count ++;
            }
        }

        // check unknown
        if (vsn.isUnknown()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(" ... ");
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.unknown")).append(", ");
                count ++;
            }
        }

        // check remote
        if (vsn.isRemote()) {
            if (needTotal) {
                count++;
            } else if (count >= maxShown) {
                buffer.append(" ... ");
                needTotal = true;
                count++;
            } else {
                buffer.append(SamUtil.getResourceString(
                    "archiving.diskvsn.flags.remote"));
                count ++;

            }
        }

        if (needTotal) {
            return buffer.append("(").append(count).append(")").toString();
        } else if (buffer.length() > 0) {
            return buffer.toString().substring(0, buffer.length() - 2);
        } else {
            return "";
        }
    }

    public static boolean isDiskType(int mediaType) {
        return mediaType == BaseDevice.MTYPE_DISK ||
               mediaType == BaseDevice.MTYPE_STK_5800;
    }
}
