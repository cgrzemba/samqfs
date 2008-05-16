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

// ident	$Id: PolicyUtil.java,v 1.45 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.math.BigInteger;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import javax.servlet.ServletRequest;

/**
 * PolicyUtil - a helper class to retrieve and cache policy criterias and
 * copies. NOTE: that the values in this class are only cached for one request
 * cycle.
 */
public class PolicyUtil {
    public static final String ARCHIVE_MANAGER =
        "SamQFSSystemArchiveManager";
    public static final String CRITERIA = "CurrentArchivePolCriteria";
    public static final String CRITERIA_COPY =
        "CurrentArchivePolCriterialCopy";
    public static final String POLICY = "CurrentArchivePolicy";
    public static final String ALL_POOLS = "all_archive_vsn_pools";


    /**
     * retrieve the SamQFSSystemArchiveManager - the  version
     * @param serverName - the name of the server
     * @return SamQFSSystemArhiveManager - the archiver manager object
     */
    public static SamQFSSystemArchiveManager getArchiveManager(
        String serverName) throws SamFSException {
        // if already retrieved, return it
        ServletRequest request =
            RequestManager.getRequestContext().getRequest();
        SamQFSSystemArchiveManager archiveManager =
            (SamQFSSystemArchiveManager)request.
            getAttribute(ARCHIVE_MANAGER);

        if (archiveManager != null) {
            return archiveManager;
        }
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        archiveManager = sysModel.getSamQFSSystemArchiveManager();

        if (archiveManager == null) {
            throw new SamFSException(null, -2002);
        }

        request.setAttribute(ARCHIVE_MANAGER, archiveManager);
        return archiveManager;
    }

    /**
     * called by CopyOptionsViewBean and CopyVSNsViewBean to reset the
     * archive manager and clear out any partially saved values
     */
    public static void resetArchiveManager(CommonViewBeanBase vb) {
        // the server name
        String serverName = vb.getServerName();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            sysModel.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     PolicyUtil.class,
                                     "resetArchiveManager",
                                     "Error resetting the archive manager",
                                     serverName);

            SamUtil.setWarningAlert(vb,
                                    vb.CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     PolicyUtil.class,
                                     "resetArchiveManager",
                                     "error resetting the archive manager",
                                     serverName);

            SamUtil.setErrorAlert(vb,
                                  vb.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     PolicyUtil.class,
                                     "resetArchiveManager",
                                     "Error resetting the archive manager",
                                     serverName);

            SamUtil.setErrorAlert(vb,
                                  vb.CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    /**
     * this method is called for copies that have no specifically assigned vsn
     * mappings as a workaround until the logic layer is resolved.
     */
    public static ArchiveVSNMap getAllsetsCopyVSNMap(ArchiveCopy theCopy,
                                              String serverName)
        throws SamFSException {

        // retrieve the allsets policy
        SamQFSSystemModel model = SamUtil.getModel(serverName);

        ArchivePolicy allsetsPolicy = model.getSamQFSSystemArchiveManager().
            getArchivePolicy(ArchivePolicy.POLICY_NAME_ALLSETS);


        ArchiveVSNMap vsnMap = null;

        if (theCopy.getArchivePolicy().getPolicyType() !=
            ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {

            // get the equivalent all sets copy
            ArchiveCopy matchingAllsetsCopy =
                allsetsPolicy.getArchiveCopy(theCopy.getCopyNumber());
            vsnMap = matchingAllsetsCopy.getArchiveVSNMap();
        }

        // get the allsets policy's allsets copy
        if (vsnMap == null)
            vsnMap = allsetsPolicy.getArchiveCopy(5).getArchiveVSNMap();

        // if the vsnmap is still null, the archiver.cmd is in error and
        // there is not much we can do now, throw an exception and exit
        if (vsnMap == null)
            throw new SamFSException(null, -2007);

        return vsnMap;
    }

    public static final ArchivePolCriteriaCopy getArchivePolCriteriaCopy(
        ArchivePolCriteria theCriteria, int copyNumber) {

        ArchivePolCriteriaCopy [] copies =
            theCriteria.getArchivePolCriteriaCopies();

        boolean found = false;
        ArchivePolCriteriaCopy theCopy = null;
        for (int i = 0; i < copies.length && !found; i++) {
            if (copyNumber == copies[i].getArchivePolCriteriaCopyNumber()) {
                found = true;
                theCopy = copies[i];
            }
        }

        return theCopy;
    }

    public static boolean policyExists(String serverName, String policyName) {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        boolean exists = false;
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemModel sysModel =
                appModel.getSamQFSSystemModel(serverName);

            if (sysModel == null)
                throw new SamFSException(null, -2001);

            SamQFSSystemArchiveManager archiveManager =
                sysModel.getSamQFSSystemArchiveManager();

            if (archiveManager == null)
                throw new SamFSException(null, -2002);

            if (archiveManager.getArchivePolicy(policyName) != null)
                exists = true;
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     PolicyUtil.class,
                                     "policyExists",
                                     "unable to determine policy's existence",
                                     serverName);
        }

        TraceUtil.trace3("Exiting");
        return exists;
    }

    public static DiskVSNHostBean getSamQFSServerInfo() throws SamFSException {
        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();

        if (appModel == null)
            throw new SamFSException(null, -2501);

        appModel.updateDownServers();
        SamQFSSystemModel [] allModels = appModel.getAllSamQFSSystemModels();

        boolean hasOlderServer = false;
        List hosts = new ArrayList();
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        for (int i = 0; i < allModels.length; i++) {
            if (!allModels[i].isDown()) {
                hosts.add(allModels[i].getHostname());

                // if this server supports the remote file chooser, add to the
                // list of servers suppporting the remote file chooser
                if ("1.3".compareTo(allModels[i].getServerAPIVersion()) <= 0) {
                    buffer.append(allModels[i].getHostname())
                        .append(";");
                } else {
                    hasOlderServer = true;
                } // end if 1.3
            } // end if isDown
        } // end for

        // convert list to array
        String [] hostArray = new String[hosts.size()];
        hostArray = (String [])hosts.toArray(hostArray);

        // trim the extra ;
        String hostString = buffer.toString();
        if (hostString.length() > 0 && hostString.lastIndexOf(";") != -1) {
            hostString = hostString.substring(0, hostString.lastIndexOf(";"));
        }

        return new DiskVSNHostBean(hostArray, hostString, hasOlderServer);
    }

    /**
     * translate a time unit from int to user legible string
     */
    public static String getTimeUnitAsString(int value) {
        switch (value) {
            case SamQFSSystemModel.TIME_SECOND:
                return SamUtil.getResourceString("ArchiveSetup.seconds");
            case SamQFSSystemModel.TIME_MINUTE:
                return SamUtil.getResourceString("ArchiveSetup.minutes");
            case SamQFSSystemModel.TIME_HOUR:
                return SamUtil.getResourceString("ArchiveSetup.hours");
            case SamQFSSystemModel.TIME_DAY:
                return SamUtil.getResourceString("ArchiveSetup.days");
            case SamQFSSystemModel.TIME_WEEK:
                return SamUtil.getResourceString("ArchiveSetup.weeks");
            default:
                return SamUtil.getResourceString("ArchiveSetup.seconds");
        }
    }

    /**
     * A utility method to validate name patterns.
     * NOTE: this method is a clone of
     * NewArchivePolWizardImpl.isValidNamePattern()
     *
     * @param namePattern - a non-null string
     * @return boolean - true if name patttern is valid else false
     */
    public static boolean isValidNamePattern(String namePattern) {
        // The code is commented out because it is not doing the right thing.
        // Simply checking an asterisk that has to have a backslash in front
        // of it is a wrong idea.  For example this method will return false
        // if user enters \.*txt, which is a legal regular expression.

        // Return true for now in all cases.  We need an method from the
        // underlying layer to validate the regex for us.  The SAM folks are
        // using their pattern matching so java.util.regex.Pattern will not
        // solve the problem completely neither.

        return true;
    }

    /**
     * utility method to determine if a given group already exists in the
     * system
     *
     * @param group name - name of the group
     * @return exists - true if group exists, else false
     */
    public static boolean isGroupValid(String group, String serverName) {
        boolean result = false;
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            result =
                sysModel.getSamQFSSystemArchiveManager().isValidGroup(group);
        } catch (SamFSException sfe) {
            // process exception:
            // very unlikenly condition unless the system is unusable
            SamUtil.processException(sfe,
                                     PolicyUtil.class,
                                    "isGroupValid",
                                    "validate group",
                                    serverName);
        }
        return result;
    }

    /**
     * utility method to determine if a given user exists in the system
     *
     * @param user name - name of the user
     * @return result - true if user is valid
     */
    public static boolean isUserValid(String user, String serverName) {
        boolean result = false;
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            result =
                sysModel.getSamQFSSystemArchiveManager().isValidUser(user);
        } catch (SamFSException sfe) {
            // process exception:
            // very unlikenly condition unless the system is unusable
            SamUtil.processException(sfe,
                                     PolicyUtil.class,
                                    "isUserValid",
                                    "validate user",
                                    serverName);
        }
        return result;
    }

    // storage size related calculations
    /**
     * returns true if max size is greater than or equal to min size
     */
    public static final boolean isMaxGreaterThanMin(long minVal,
                                                    int minUnit,
                                                    long maxVal,
                                                    int maxUnit) {
        boolean result = false;
        if (getBigSize(maxVal, maxUnit).compareTo(
            getBigSize(minVal, minUnit)) >= 0)
            result = true;
        return result;
    }

    // helper function to get display string of staging options
    public static String getStagingOptionString(int optionValue) {
        String optionString = "";
        switch (optionValue) {
            case ArchivePolicy.STAGE_ASSOCIATIVE:
                optionString = "archiving.criteria.staging.associative";
                break;
            case ArchivePolicy.STAGE_NEVER:
                optionString = "archiving.criteria.staging.never";
                break;
            case ArchivePolicy.STAGE_DEFAULTS:
                optionString = "archiving.criteria.staging.defaults";
                break;
        }
        return SamUtil.getResourceString(optionString);
    }

    // helper function to get display string of releasing options
    public static String getReleasingOptionString(int optionValue) {
        String optionString = "";
        switch (optionValue) {
            case ArchivePolicy.RELEASE_NEVER:
                optionString = "archiving.criteria.releasing.never";
                break;
            case ArchivePolicy.RELEASE_PARTIAL:
                optionString = "archiving.criteria.releasing.partial";
                break;
            case ArchivePolicy.RELEASE_AFTER_ONE:
                optionString = "archiving.criteria.releasing.onecopy";
                break;
            case ArchivePolicy.RELEASE_DEFAULTS:
                optionString = "archiving.criteria.releasing.defaults";
                break;

        }
        return SamUtil.getResourceString(optionString);
    }

    public static BigInteger getBigSize(long size, int unit) {
        BigInteger newsize = new BigInteger(new Long(size).toString());
        switch (unit) {
            case SamQFSSystemModel.SIZE_B:
                break;
            case SamQFSSystemModel.SIZE_KB:
                newsize = newsize.multiply(new BigInteger(
                     new Long(KB).toString()));
                break;
            case SamQFSSystemModel.SIZE_MB:
                newsize = newsize.multiply(new BigInteger(
                     new Long(MB).toString()));
                break;
            case SamQFSSystemModel.SIZE_GB:
                newsize = newsize.multiply(new BigInteger(
                     new Long(GB).toString()));
                break;
            case SamQFSSystemModel.SIZE_TB:
                newsize = newsize.multiply(new BigInteger(
                     new Long(TB).toString()));
                break;
            case SamQFSSystemModel.SIZE_PB:
                newsize = newsize.multiply(new BigInteger(
                     new Long(PB).toString()));
                break;
            default:
                break;
        }

        return newsize;
    }

    public static boolean isOverFlow(long size, int sizeunit) {
        boolean isoverflow = false;
        if (getBigSize(size, sizeunit).compareTo(
            getBigSize(SIZELIMIT, SIZELIMIT_UNIT)) > 0)
            isoverflow = true;
        return isoverflow;
    }

    public static boolean isValidSize(long size, int unit) {
        if (size <= 0) {
            return false;
        }

        return !isOverFlow(size, unit);
    }

    public static boolean isValidTime(long time, int unit) {
        if (time <= 0) {
            return false;
        }

        return !isTimeOverFlow(time, unit);
    }

    public static boolean isTimeOverFlow(long time, int unit) {
        int index = unit - 5;

        // safety check
        if (index < 0 || index > 9) {
            return false;
        }

        BigInteger value = new BigInteger(new Long(time).toString());
        value =
            value.multiply(new BigInteger(Long.toString(SIZE_VALUES[index])));

        // largest acceptable size by C-API 2147483646 seconds
        int result = value.compareTo(new BigInteger("2147483646"));

        return (result > 0);
    }

    public static String getPolicyTypeString(ArchivePolicy policy) {
        String type = "";

        switch (policy.getPolicyType()) {
            case ArSet.AR_SET_TYPE_DEFAULT:
                type = "archiving.policy.type.default";
                break;
            case ArSet.AR_SET_TYPE_NO_ARCHIVE:
                type = "archiving.policy.type.noarchive";
                break;
            case ArSet.AR_SET_TYPE_ALLSETS_PSEUDO:
                type = "archiving.policy.type.settabledefaults";
                break;
            case ArSet.AR_SET_TYPE_GENERAL:
                type = "archiving.policy.type.custom";
                break;
            case ArSet.AR_SET_TYPE_UNASSIGNED:
                type = "archiving.policy.type.unassign";
                break;
            case ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT:
                type = "archiving.policy.type.explicitdefault";
                break;
            default:
        }

        return type;
    }

    public static String getArchiveAgeString(ArchivePolCriteria criteria)
        throws SamFSException {
        ArchivePolCriteriaCopy [] copy =
            criteria.getArchivePolCriteriaCopies();

        if (copy == null || copy.length == 0) {
            return "";
        }

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        for (int i = 0; i < copy.length; i++) {
            buf.append(SamUtil.getResourceString("archiving.copynumber",
                Integer.toString(copy[i].getArchivePolCriteriaCopyNumber())))
               .append(": ")
               .append(copy[i].getArchiveAge())
               .append(" ")
           .append(SamUtil.getTimeUnitL10NString(copy[i].getArchiveAgeUnit()))
               .append("<br>");
        }

        String result = buf.toString();

        // remove trailing comma
        // result = result.substring(0, result.lastIndexOf(","));

        return result;
    }

    public static final String getMediaTypeString(ArchivePolCriteria criteria)
        throws SamFSException {

        ArchivePolCriteriaCopy [] copy = criteria.getArchivePolCriteriaCopies();
        if (copy == null || copy.length == 0)
            return "";

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        for (int i = 0; i < copy.length; i++) {
            buf.append(SamUtil.getResourceString("archiving.copynumber",
                Integer.toString(copy[i].getArchivePolCriteriaCopyNumber())))
                .append(": ");

            // now the media Type
            ArchiveVSNMap vsnMap = criteria.getArchivePolicy().
                getArchiveCopy(copy[i].getArchivePolCriteriaCopyNumber()).
                getArchiveVSNMap();

            int mediaType =  vsnMap != null ? vsnMap.getArchiveMediaType() : -1;
            String mediaTypeString = "";
            if (mediaType < 0) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.unknown");
            } else if (mediaType == BaseDevice.MTYPE_DISK) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.disk");
            } else {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.tape");
            }

            buf.append(mediaTypeString).append("<br>");
        }

        return buf.toString();
    }

    public static String getPolicyCopyString(ArchivePolCriteria criteria)
        throws SamFSException {
        ArchivePolCriteriaCopy [] copy =
            criteria.getArchivePolCriteriaCopies();

        if (copy == null || copy.length == 0) {
            return "";
        }

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        for (int i = 0; i < copy.length; i++) {
            // Copy Number and Archive Age
            buf.append("<b>").append(
                SamUtil.getResourceString("archiving.copynumber",
                Integer.toString(copy[i].getArchivePolCriteriaCopyNumber())))
                .append(": </b>")
                .append(copy[i].getArchiveAge())
                .append(" ").append(
                SamUtil.getTimeUnitL10NString(copy[i].getArchiveAgeUnit()));

            // Comma and Media Type
            buf.append(", ");

            ArchiveVSNMap vsnMap = criteria.getArchivePolicy().
                getArchiveCopy(copy[i].getArchivePolCriteriaCopyNumber()).
                getArchiveVSNMap();

            int mediaType =  vsnMap != null ? vsnMap.getArchiveMediaType() : -1;
            String mediaTypeString;
            if (mediaType < 0) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.unknown");
            } else if (mediaType == BaseDevice.MTYPE_DISK) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.disk");
            } else {
                mediaTypeString = SamUtil.getMediaTypeString(mediaType);
            }

            buf.append(mediaTypeString);
            if (i % 2 == 1) {
                buf.append("<br>");
            } else {
                buf.append("&nbsp;&nbsp;&nbsp;");
            }
        }

        String result = buf.toString();

        return result;

    }

    public static String getVSNString(String []vsns) {
        if (vsns == null || vsns.length == 0) {
            return "";
        }

        // use StringBuffer so we can determine string length;
        StringBuffer buf = new StringBuffer();
        boolean longEnough = false;

        // limit result string to 40 characters for now
        int length = 60;

        for (int i = 0; i < vsns.length && !longEnough; i++) {

            // if appending the next vsn would make the string too long, quit
            if ((buf.length() +  vsns[i].length()) >= length) {
                longEnough = true;
                continue;
            } else {
                buf.append(vsns[i]).append(", ");
            }
        }

        // remove the trailing comma
        if (buf.length() > 0 && buf.lastIndexOf(",") != -1) {
            buf.deleteCharAt(buf.lastIndexOf(","));
        }

        String size = vsns.length > 0 ? ("" + vsns.length) : "";
        // now append the appropriate suffix : (N) or ... (N)
        if (!longEnough) {
            buf.append("(").append(size).append(")");
        } else {
            buf.append("... (").append(size).append(")");
        }

        return buf.toString();
    }

    public static String getPolicyFSString(ArchivePolicy policy)
        throws SamFSException {
        ArrayList fsList = new ArrayList();

        // find the unique file system for each criteria
        ArchivePolCriteria [] criteria = policy.getArchivePolCriteria();
        for (int i = 0; i < criteria.length; i++) {
            FileSystem [] filesystems = criteria[i].getFileSystemsForCriteria();
            for (int j = 0; j < filesystems.length; j++) {
                String fsName = filesystems[j].getName();
                if (!fsList.contains(fsName)) {
                    fsList.add(fsName);
                }
            }
        }

        // now constuct a single string out of all the names
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        Iterator it = fsList.iterator();
        while (it.hasNext()) {
            buffer.append((String)it.next()).append(", ");
        }

        // now remove the trailing comma+space ", " and return the result
        String temp = buffer.toString();

        int index = temp.lastIndexOf(", ");

        if (index > 0) {
            return temp.substring(0, index);
        } else {
            return "";
        }
    }

    public static String getSizeString(long size, int unit) {
        String result = "";

        if (size > 0) {
            result = result.concat(NumberFormat.getInstance().format(size))
                .concat(" ")
                .concat(SamUtil.getSizeUnitL10NString(unit));
        }

        return result;
    }

    /**
     * helper method to retrieve all the VSN Pools in a particular server. This
     * method caches its results in the ServletRequest object to prevent remote
     * calls for pool requests with the same request cycle
     */
    public static VSNPool [] getAllVSNPools(String serverName)
        throws SamFSException {

        ServletRequest request =
            RequestManager.getRequestContext().getRequest();

        VSNPool [] allPools = (VSNPool []) request.getAttribute(ALL_POOLS);
        if (allPools == null || allPools.length == 0) {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            allPools = sysModel.
                getSamQFSSystemArchiveManager().getAllVSNPools();

            request.setAttribute(ALL_POOLS, allPools);
        }

        return allPools;
    }

    /**
     * Determine if the pool name is already in use.
     * @param serverName
     * @param poolName
     * @return true or false if the pool name is already in use
     * @throws com.sun.netstorage.samqfs.mgmt.SamFSException
     */
    public static boolean poolExists(String serverName, String poolName)
        throws SamFSException {
        VSNPool [] allPools = getAllVSNPools(serverName);
        allPools = allPools == null ? new VSNPool[0] : allPools;

        for (int i = 0; i < allPools.length; i++) {
            if (allPools[i].getPoolName().equals(poolName)) {
                return true;
            }
        }
        return false;
    }

    /**
     * retrieve an array of all the pools with the given media type
     * if -1 is provided for media type, the return all the vsn pool names
     *
     * @param mediaType
     * @param serverName
     * @return vsnPoolNames
     */
    public static String [] getVSNPoolNames(int mediaType, String serverName)
        throws SamFSException {

        String [] vsnPoolNames = null;

        VSNPool [] allPools = getAllVSNPools(serverName);

        if (mediaType == -999) { // return all pool names
            vsnPoolNames = new String[allPools.length];
            for (int i = 0; i < allPools.length; i++) {
                vsnPoolNames[i] = allPools[i].getPoolName();
            }
        } else { // return pools matching media type
            ArrayList list = new ArrayList();
            for (int i = 0; i < allPools.length; i++) {
                if (allPools[i].getMediaType() == mediaType) {
                    list.add(allPools[i].getPoolName());
                }
            }

            vsnPoolNames = new String[list.size()];
            vsnPoolNames = (String [])list.toArray(vsnPoolNames);
        }

        // make sure we don't return a null
        if (vsnPoolNames == null)
            vsnPoolNames = new String[0];

        return vsnPoolNames;
    }

    /**
     * construct a string of the format :
     *  poolName (media type)
     * for each vsn pool
     *
     * @return String []
     * @param String serverName -
     */
    public static final String [][] getAllVSNPoolNames(String serverName)
        throws SamFSException {
        VSNPool [] pool = getAllVSNPools(serverName);
        String [][]poolName = new String[2][pool.length];

        for (int i = 0; i < pool.length; i++) {
            String name = pool[i].getPoolName();
            int mediaType = pool[i].getMediaType();
            name = name.concat(" (")
            .concat(SamUtil.getMediaTypeString(mediaType)).concat(")");

            poolName[0][i] = name;
            poolName[1][i] = pool[i].getPoolName().concat(":")
                .concat(Integer.toString(mediaType));
        }

        return poolName;
    }

    /**
     * encode vsn pools and media types into a string of the form :
     * mt1=pool1,pool2;mt2=;
     */
    public static String encodePoolMediaTypeString(String serverName)
        throws SamFSException {

        SamQFSSystemModel model = SamUtil.getModel(serverName);

        int [] mediaTypes = model.
            getSamQFSSystemMediaManager().getAvailableArchiveMediaTypes();


        // initialize every available media type
        HashMap map = new HashMap();
        for (int i = 0; i < mediaTypes.length; i++) {
            String key = Integer.toString(mediaTypes[i]);
            NonSyncStringBuffer buf = new NonSyncStringBuffer();

            buf.append(key)
                .append("=")
                .append(SelectableGroupHelper.NOVAL_LABEL);

            map.put(key, buf);
        }

        // loop the pools once and set those of available media Types
        VSNPool [] pool = getAllVSNPools(serverName);
        for (int i = 0; i < pool.length; i++) {
            String key = Integer.toString(pool[i].getMediaType());
            NonSyncStringBuffer buf = (NonSyncStringBuffer)map.get(key);

            if (buf != null) {
                buf.append(",")
                    .append(pool[i].getPoolName());
            }
        }

        Iterator it = map.values().iterator();
        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        while (it.hasNext()) {
            buf.append(((NonSyncStringBuffer)it.next()).toString())
                .append(";");
        }

        String result = buf.toString();
        if (result != null && result.length() > 0) {
            // remove trailing ;
            result = result.substring(0, result.length() - 1);
        }

        return result;
    }

    public static Object [][] getAvailableMediaTypes(String serverName)
        throws SamFSException {
        Object [][] mediaTypes = new Object[2][];
        int [] mt = SamUtil.getModel(serverName)
            .getSamQFSSystemMediaManager().getAvailableArchiveMediaTypes();

        String [] labels = new String[mt.length];
        String [] values = new String[mt.length];
        for (int i = 0; i < mt.length; i++) {
            labels[i] = SamUtil.getMediaTypeString(mt[i]);
            values[i] = Integer.toString(mt[i]);
        }
        mediaTypes[0] = labels;
        mediaTypes[1] = values;

        return mediaTypes;
    }

    public static int getPercentUsage(long c, long f) {
        if (c <= 0 || f < 0)
            return -1;

        double capacity = (double)c;
        double free = (double)f;

        // return percent_used;
        return (100 - (int)((free/capacity) * 100));
    }

    // sizes : size units expressed in bytes
    public static long BYTE = 1;
    public static long KB = 1024;
    public static long MB = KB * 1024;
    public static long GB = MB * 1024;
    public static long TB = GB * 1024;
    public static long PB = TB * 1024;
    public static long SIZELIMIT = 8000000;
    public static int SIZELIMIT_UNIT = SamQFSSystemModel.SIZE_TB;

    // times : time units expressed in seconds
    public static long SECOND = 1;
    public static long MINUTE = SECOND * 60;
    public static long HOUR   = MINUTE * 60;
    public static long DAY    = HOUR   * 24;
    public static long WEEK   = DAY    * 7;

    // constants defined as 5(secs) - 9(wks) thus : unit - 5 = Size values index
    public static long [] SIZE_VALUES = {SECOND, MINUTE, HOUR, DAY, WEEK};

    // release options
    public interface ReleaseOptions {
        public static int SPACE_REQUIRED = 0;
        public static int IMMEDIATELY = 2;
        public static int WAIT_FOR_ALL = 4;
    }
}

/**
 * this helper bean is used to carry the list of live servers in the systems
 * i.e. servers that we can connect together with a list of servers that
 * support the remote file chooser i.e. 4.4 and later
 *
 * NOTE: This is NOT a general purpose class. It is a special purpose class
 * used by the New Disk VSN popup.
 */
class DiskVSNHostBean {
    /**
     * an array of hostnames for those servers we're able to connect to
     */
    private String [] liveHosts;

    /**
     * a semi-colon [;] delimited string of hostnames support the remote file
     * choser i.e. version 4.4 and later
     */
    private String RFCCapableHosts;

    /**
     * a flag to indicate whether the list of live servers include a server
     * that does not support the remote file chooser.
     */
    private boolean hasOlderServer = false;

    public DiskVSNHostBean(String [] hosts, String rfchosts, boolean hos) {
        liveHosts = hosts;
        RFCCapableHosts = rfchosts;
        hasOlderServer = hos;
    }

    public void setLiveHosts(String [] hosts) {
        liveHosts = hosts;
    }

    public void setRFCcapableHosts(String rfchosts) {
        RFCCapableHosts = rfchosts;
    }

    public String [] getLiveHosts() {
        return liveHosts;
    }

    public String getRFCCapableHosts() {
        return RFCCapableHosts;
    }

    public void setHasOlderServer(boolean hos) {
        hasOlderServer = hos;
    }

    public boolean hasOlderServer() {
        return hasOlderServer;
    }
}
