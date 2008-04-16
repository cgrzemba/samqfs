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

// ident	$Id: ServerUtil.java,v 1.19 2008/04/16 17:07:26 ronaldso Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SystemCapacity;
import com.sun.netstorage.samqfs.web.model.SystemInfo;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;

import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCClientSniffer;
import com.sun.web.ui.common.CCSeverity;

import java.util.StringTokenizer;

public class ServerUtil {

    public static final String lineBreak = "<br>";
    public static final String delimitor = "###";
    public static final int ALARM_NO_ERROR = 0;
    public static final int ALARM_CRITICAL = 1;
    public static final int ALARM_MAJOR = 2;
    public static final int ALARM_MINOR = 3;
    public static final int ALARM_DOWN  = 4;
    public static final int ALARM_ACCESS_DENIED = 5;
    public static final int ALARM_NOT_SUPPORTED = 6;

    /**
     * @param - none
     * @return - boolean If the client browser is supported
     * Detect if client browser is supported
     */
    public static boolean isClientBrowserSupported() {
        TraceUtil.trace3("Entering");
        boolean isBrowserSupported = false;
        CCClientSniffer cs = new CCClientSniffer(
            RequestManager.getRequestContext().getRequest());

        if (cs != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Client Browser User Agent: ").append(cs.getUserAgent()).
                toString());
            if (cs.isGecko() || cs.isNav7up() || cs.isIe6up()) {
                SamUtil.doPrint("Browser is supported");
                isBrowserSupported = true;
            } else if (cs.isIe() && isIe55up(cs.getUserAgent())) {
                SamUtil.doPrint("Browser is supported");
                isBrowserSupported = true;
            } else {
                SamUtil.doPrint("Browser is not supported");
            }
        } else {
            SamUtil.doPrint("ClientSniffer object is null.");
        }
        TraceUtil.trace3("Exiting");
        return isBrowserSupported;
    }

    /**
     * isIe55up()
     * Detect if user agent is Internet Explorer 5.5 or above
     */
    private static boolean isIe55up(String agentStr) {
        TraceUtil.trace3("Entering");
        boolean returnValue = true;

        StringTokenizer firstTokens = new StringTokenizer(agentStr, ";");
        if (firstTokens.countTokens() < 2) {
            // error, the agentStr for IE should have 4 ";" in the String
            returnValue = false;
        }

        String tmpStr = firstTokens.nextToken();
        StringTokenizer secondTokens = new StringTokenizer(
            (String) firstTokens.nextToken(), " ");
        if (secondTokens.countTokens() < 2) {
            // firstTokens[1] should look like "MSIE XXXX"
            returnValue = false;
        }

        tmpStr = secondTokens.nextToken();
        StringTokenizer thirdTokens = new StringTokenizer(
            (String) secondTokens.nextToken(), ".");
        if (thirdTokens.countTokens() < 2) {
            // secondTokens[1] should look like "5.5", or "5.01", and so on
            returnValue = false;
        }

        tmpStr = thirdTokens.nextToken();
        try {
            int minorVersion = Integer.parseInt(
                (String) thirdTokens.nextToken());
            if (minorVersion < 5) {
                returnValue = false;
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace3(new NonSyncStringBuffer("Number Format ").
                append("Exception caught while fetching browser information").
                toString());
            returnValue = false;
        }

        TraceUtil.trace3("Exiting");
        return returnValue;
    }

    public static int getServerMostSevereAlarm(AlarmSummary myAlarmSummary) {
        int critical = myAlarmSummary.getCriticalTotal();
        int major    = myAlarmSummary.getMajorTotal();
        int minor    = myAlarmSummary.getMinorTotal();

        // No fault
        if (myAlarmSummary == null) {
            return CCSeverity.OK;
        } else if (critical != 0) {
            return CCSeverity.CRITICAL;
	} else if (major != 0) {
            return CCSeverity.MAJOR;
	} else if (minor != 0) {
            return CCSeverity.MINOR;
	} else {
            return CCSeverity.OK;
        }
    }

    public static int getLibraryMostSevereAlarm(Alarm [] allAlarms)
        throws SamFSException {
        int [] alarmInfo = SamUtil.getAlarmInfo(allAlarms);

        switch (alarmInfo[0]) {
            case ServerUtil.ALARM_CRITICAL:
                return CCSeverity.CRITICAL;

            case ServerUtil.ALARM_MAJOR:
                return CCSeverity.MAJOR;

            case ServerUtil.ALARM_MINOR:
                return CCSeverity.MINOR;

            default:
                return CCSeverity.OK;
        }
    }

    /**
     * Create OS String
     */
    public static String createOSString(
        String name, String release, String version) {
        return new NonSyncStringBuffer(name).append(
            " ").append(SamUtil.getResourceString(
            "ServerConfiguration.general.word.release")).append(
            " ").append(release).append(" ").append(
            SamUtil.getResourceString(
            "ServerConfiguration.general.word.version")).append(" ").
            append(version).toString();
    }

    /**
     * By using a SamQFSSystemModel object, generate a server entry
     * that is used in the Site Information Page
     */
    public static String createServerEntry(SamQFSSystemModel sysModel)
        throws SamFSException {
        NonSyncStringBuffer myBuf = new NonSyncStringBuffer("");
        String diskCacheWord =
            SamUtil.getResourceString("SiteInformation.server.word.diskCache");
        String memoryWord =
            SamUtil.getResourceString("SiteInformation.server.word.memory");
        String cpusWord =
            SamUtil.getResourceString("SiteInformation.server.word.cpus");

        SystemInfo mySystemInfo = sysModel.getSystemInfo();
        SystemCapacity mySystemCapacity = sysModel.getCapacity();

        String memoryValue  =
            new Capacity(
                mySystemInfo.getMemoryMB(),
                SamQFSSystemModel.SIZE_MB).toString();
        String osString =
            createOSString(mySystemInfo.getOSname(),
                mySystemInfo.getRelease(),
                mySystemInfo.getVersion());

        String serverDiskCache =
            mySystemCapacity.getDiskCacheKB() == -1 ?
                new NonSyncStringBuffer("0 ").
                    append(SamUtil.getSizeUnitL10NString(
                        SamQFSSystemModel.SIZE_KB)).toString() :
                generateNumberWithUnitString(mySystemCapacity.getDiskCacheKB());

        String serverName = sysModel.getHostname();
        String samfsServerAPIVersion = sysModel.getServerAPIVersion();

        // If the server api version cannot be obtained,
        // samfsServerAPIVersion defaults to the previous version
        samfsServerAPIVersion = (samfsServerAPIVersion == null) ?
            "1.3" : samfsServerAPIVersion;

        if (SamUtil.isVersionCurrentOrLaterThan(samfsServerAPIVersion, "1.4")) {
            if (sysModel.isClusterNode()) {
                serverName = SamUtil.getResourceString(
                    "ServerSelection.host.clusterword",
                    new String [] {serverName, sysModel.getClusterName()});
             }
        }

        // Append Server Name
        myBuf.append("<b>").append(serverName);

        // Append the IP Addresses
        myBuf.append("     ").append(
            createIPAddressesString(mySystemInfo.getIPAddresses()));
        myBuf.append("</b><br />");

        // Append Operating System information
        myBuf.append(osString).append("<br />");

        // Append # of cpus and amount of memory (TODO)
        myBuf.append(Integer.toString(mySystemInfo.getCPUs())).append(" ").
            append(cpusWord).append(" ");

        // Append total Disk cache
        myBuf.append(serverDiskCache).append(" ").append(diskCacheWord);

        // Append memory string
        myBuf.append("<br />").append(memoryValue).
            append(" ").append(memoryWord);

        return myBuf.toString();
    }

    /**
     * Create the IP Address portion
     */
    private static String createIPAddressesString(String origIPAddressString) {
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        String [] ipArray = origIPAddressString.split(" ");
        buffer.append(" ( <i>");
        for (int i = 0; i < ipArray.length; i++) {
            if (i > 0) {
                buffer.append(" ,  ");
            }
            buffer.append(ipArray[i]);
        }
        buffer.append(" </i>)");
        return buffer.toString();
    }

    /**
     * This method takes in a string which is about to be converted back to
     * long.  The unit of this long value is always in KB (from the logic layer)
     * thus this method only takes 1 value.  Then the long value along with KB
     * will be passed into Capacity Constructor in order to generate a number
     * string with units in the proper manner.
     */
    public static String generateNumberWithUnitString(String numberString) {
        long number = -1;
        try {
            number = Long.parseLong(numberString);
        } catch (NumberFormatException numEx) {
            // This should not happen.  It is a developer bug if the code
            // reaches here.
            return Long.toString(number);
        }

        Capacity myCapacity = new Capacity(number, SamQFSSystemModel.SIZE_KB);
        return myCapacity.toString();
    }

    /**
     * This method is equivalent to the previous one, except it takes a long
     * instead of a string
     */
    public static String generateNumberWithUnitString(long number) {
        Capacity myCapacity = new Capacity(number, SamQFSSystemModel.SIZE_KB);
        return myCapacity.toString();
    }

    /**
     * This method generates the version string that is consistent with the
     * naming convention that is used by the marketing folks.
     */
    public static String getVersionString(String guiVersionString) {

        if (guiVersionString.compareTo("5.0") >= 0) {
            return SamUtil.getResourceString("ServerSelection.version.50");
        } else if (guiVersionString.compareTo("4.6") >= 0) {
            return SamUtil.getResourceString("ServerSelection.version.46");
        } else {
            return SamUtil.getResourceString("ServerSelection.version.unknown");
        }
    }
}
