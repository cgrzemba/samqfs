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

// ident $Id: SharedHostInfo.java,v 1.11 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.web.model.fs.SharedFSFilter;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.JSFUtil;

/**
 * this class encapsulates information about a shared file system host
 *
 * Keys shared by all classes of host include:
 * hostname = %s
 * type = OSD | client | mds | pmds
 * ip_addresses = space separated list of ips.
 * os = Operating System Version
 * version = sam/qfs version
 * arch = x86 | sparc
 * mounted = %d ( -1 means not mounted otherwise time mounted in seconds)
 * status = ON | OFF
 * error = assumed_dead | known_dead
 *
 * faults = %d (only on storage nodes. This key only present if faults
 *		    exist)
 *
 * If HOST_DETAILS flag is set in options the following will be obtained
 * for clients and real and potential metadata servers. Information
 * about decoding these can be found in the pub/mgmt/hosts.h header file.
 *
 * fi_status=<hex 32bit map>
 * fi_flags = <hex 32bit map>
 * mnt_cfg = <hex 32bit map>
 * mnt_cfg1 = <hex 32bit map>
 * no_msgs= int32
 * low_msg = uint32
 */
public class SharedHostInfo {

    private static final String KEY_NAME = "hostname";
    private static final String KEY_TYPE = "type";
    private static final String KEY_IP_ADDRESSES = "ip_addresses";
    private static final String KEY_OS = "os";
    private static final String KEY_VERSION = "version";
    private static final String KEY_ARCH = "arch";
    private static final String KEY_MOUNTED = "mounted";
    private static final String KEY_STATUS = "status";
    private static final String KEY_ERROR = "error";
    private static final String KEY_FAULTS = "faults";

    // HOST_DETAILS
    private static final String KEY_FI_STATUS = "fi_status";
    private static final String KEY_FI_FLAGS = "fi_flags";
    private static final String KEY_MNT_CFG = "mnt_cfg";
    private static final String KEY_MNT_CFG_1 = "mnt_cfg1";
    private static final String KEY_NO_MSGS = "no_msgs";
    private static final String KEY_LOW_MSG = "low_msg";


    // definitions

    // type = OSD | client | mds | pmds
    public static final short TYPE_MDS = 0;
    public static final short TYPE_PMDS = 1;
    public static final short TYPE_CLIENT = 2;
    public static final short TYPE_OSD = 4;

    // state = ON | OFF
    public static final short STATE_ON = 0;
    public static final short STATE_OFF = 1;

    // overall status
    public static final short STATUS_ASSUMED_DEAD = 0;
    public static final short STATUS_KNOWN_DEAD = 1;
    public static final short STATUS_ACCESS_DISABLED = 2;
    public static final short STATUS_FAULTS = 3;
    public static final short STATUS_UNMOUNTED = 4;
    public static final short STATUS_OK = 5;

    protected Properties props = null;

    public SharedHostInfo(String inputString) {
        props = ConversionUtil.strToProps(inputString);
        if (props == null) {
            return;
        }
    }

    public String getName() {
        return props.getProperty(KEY_NAME, "");
    }

    public String getOS() {
        return props.getProperty(KEY_OS, "");
    }

    public String [] getIPAddresses() {
        String str = getIPAddressesStr();
        if (str.length() == 0) {
            return new String[0];
        }
        return str.split(" ");
    }

    public String getIPAddressesStr() {
        return props.getProperty(KEY_IP_ADDRESSES, "");
    }

    public String getVersion() {
        return props.getProperty(KEY_VERSION, "");
    }

    public short getType() {
        String str = props.getProperty(KEY_TYPE, "");
        if ("mds".equals(str)) {
            return TYPE_MDS;
        } else if ("pmds".equals(str)) {
            return TYPE_PMDS;
        } else if ("client".equals(str)) {
            return TYPE_CLIENT;
        } else if ("OSD".equals(str)) {
            return TYPE_OSD;
        }
        return (short) -1;
    }

    public String getTypeString() {
        switch (getType()) {
            case TYPE_MDS:
                return JSFUtil.getMessage("SharedFS.text.mds");
            case TYPE_PMDS:
                return JSFUtil.getMessage("SharedFS.text.pmds");
            case TYPE_CLIENT:
                return JSFUtil.getMessage("SharedFS.text.client");
            case TYPE_OSD:
                return JSFUtil.getMessage("SharedFS.text.osd");
            default:
                return "";
        }
    }

    public String getArch() {
        String str = props.getProperty(KEY_ARCH, "");
        if ("sparc".equals(str)) {
            return JSFUtil.getMessage("common.text.sparc");
        } else if ("x86".equals(str)) {
            return JSFUtil.getMessage("common.text.x86");
        }
        return "";
    }

    public Long getMounted() {
        String str = props.getProperty(KEY_MOUNTED, "");
        if (str.length() == 0) {
            return new Long(-1);
        }
        try {
            return new Long(str);
        } catch (NumberFormatException ex) {
            return new Long(-1);
        }
    }

    public short getStatus() {
        String str = props.getProperty(KEY_STATUS, "");
        if ("ON".equals(str)) {
            return STATE_ON;
        } else if ("OFF".equals(str)) {
            return STATE_OFF;
        }
        return -1;
    }

    public String getError() {
        return props.getProperty(KEY_ERROR, "");
    }

    public int getFaults() {
        String str = props.getProperty(KEY_FAULTS, "");
        if (str.length() == 0) {
            return -1;
        }
        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException ex) {
            return -1;
        }
    }

    public String getStatusIcon() {
        switch (getStatusValue()) {
            case STATUS_ASSUMED_DEAD:
            case STATUS_KNOWN_DEAD:
                return "/images/error_13x.gif";
            case STATUS_ACCESS_DISABLED:
            case STATUS_UNMOUNTED:
                return "/images/disabled_13x.png";
            case STATUS_OK:
            default:
                return "/images/OK_13x.png";
        }
    }

    public String getStatusString() {
        switch (getStatusValue()) {
            case STATUS_ASSUMED_DEAD:
                return JSFUtil.getMessage("SharedFS.state.assumeddead");
            case STATUS_KNOWN_DEAD:
                return JSFUtil.getMessage("SharedFS.state.knowndead");
            case STATUS_ACCESS_DISABLED:
                return
                    getType() == TYPE_OSD ?
                        JSFUtil.getMessage("SharedFS.state.allocdisabled") :
                        JSFUtil.getMessage("SharedFS.state.accessdisabled");
            case STATUS_UNMOUNTED:
                return JSFUtil.getMessage("SharedFS.state.unmounted");
            case STATUS_OK:
            default:
                return JSFUtil.getMessage("SharedFS.state.ok");
        }
    }

    public short getStatusValue() {
        if ("assumed_dead".equals(getError())) {
            return STATUS_ASSUMED_DEAD;
        } else if ("known_dead".equals(getError())) {
            return STATUS_KNOWN_DEAD;
        } else if (STATE_OFF == getStatus()) {
            return STATUS_ACCESS_DISABLED;
        } else if (0 >= getMounted()) {
            return STATUS_UNMOUNTED;
        } else {
            return STATUS_OK;
        }
    }

    /**
     * TODO: Fix this
     */
    public short getFaultStatus() {
        return 0;
    }

    public String getFaultString() {
        switch (getFaultStatus()) {
            case 1:
                return "Critical";
            case 2:
                return "Major";
            case 3:
                return "Minor";
            case 0:
            default:
                return "";
        }
    }

    public String getFaultIcon() {
        switch (getFaultStatus()) {
            case 1:
                return "";
            case 2:
                return "";
            case 3:
                return "";
            case 0:
            default:
                return Constants.Image.JSF_ICON_BLANK_ONE_PIXEL;
        }
    }

    public String getUsageBar() {
        return Constants.Image.JSF_USAGE_BAR_DIR + getUsage() + ".gif";
    }

    public int getUsage() {
        return 34;
    }

    public String getCapacityStr() {
        long cap = getCapacityInKb();
        if (cap < 0) {
            return "";
        }
        return
            "(" +
            Capacity.newCapacityInJSF(cap, SamQFSSystemModel.SIZE_KB) +
            ")";
    }

    public long getCapacityInKb() {
        return (long) 1325488;
    }

    public String getIscsiid() {
        return "c1t2d3s4";
    }

    public boolean suffices(SharedFSFilter filter) {
        if (filter.getTarget().
            equals(SharedFSFilter.TARGET_IP_ADDRESS)) {
            return checkIPAddress(filter);
        } else if (filter.getTarget().
            equals(SharedFSFilter.TARGET_STATUS)) {
            return checkStatus(filter);
        } else if (filter.getTarget().
            equals(SharedFSFilter.TARGET_TYPE)) {
            return checkType(filter);
        } else {
            return checkString(filter);
        }
    }

    private boolean checkStatus(SharedFSFilter filter) {
        boolean is = filter.getCondition().equals(SharedFSFilter.COND_IS);

        String content = filter.getContent();
        if (content.equals(SharedFSFilter.STATUS_OK)) {
            return
                is ?
                    SharedHostInfo.STATUS_OK == getStatusValue() :
                    SharedHostInfo.STATUS_OK != getStatusValue();
        } else if (content.equals(SharedFSFilter.STATUS_MOUNTED)) {
            return
                // Note: since we have to check against "STATUS_UNMOUNTED", the
                // conditions are flipped.
                is ?
                    SharedHostInfo.STATUS_UNMOUNTED != getStatusValue() :
                    SharedHostInfo.STATUS_UNMOUNTED == getStatusValue();
        } else if (content.equals(SharedFSFilter.STATUS_ACCESS_ENABLED)) {
            return
                // Note: since we have to check against "STATUS_DISABLED", the
                // conditions are flipped.
                is ?
                    SharedHostInfo.STATUS_ACCESS_DISABLED != getStatusValue() :
                    SharedHostInfo.STATUS_ACCESS_DISABLED == getStatusValue();
        } else if (content.equals(SharedFSFilter.STATUS_IN_ERROR)) {
            return
                is ?
                    SharedHostInfo.STATUS_ASSUMED_DEAD == getStatusValue() ||
                    SharedHostInfo.STATUS_KNOWN_DEAD == getStatusValue() :
                    SharedHostInfo.STATUS_ASSUMED_DEAD != getStatusValue() ||
                    SharedHostInfo.STATUS_KNOWN_DEAD != getStatusValue();
        } else {
            // Should not get here
            return false;
        }
    }

    private boolean checkType(SharedFSFilter filter) {
        boolean is = filter.getCondition().equals(SharedFSFilter.COND_IS);

        String content = filter.getContent();
        if (content.equals(SharedFSFilter.TYPE_MDS_PMDS)) {
            return
                is ?
                    SharedHostInfo.TYPE_MDS == getType() ||
                    SharedHostInfo.TYPE_PMDS == getType() :
                    SharedHostInfo.TYPE_MDS != getType() ||
                    SharedHostInfo.TYPE_PMDS != getType();
        } else if (content.equals(SharedFSFilter.TYPE_CLIENTS)) {
            return
                is ?
                    SharedHostInfo.TYPE_CLIENT == getType() :
                    SharedHostInfo.TYPE_CLIENT != getType();
        } else {
            // Should not get here
            return false;
        }
    }

    private boolean checkString(SharedFSFilter filter) {
        String target = filter.getTarget();
        String checkAgainst = null;

        if (SharedFSFilter.TARGET_HOST_NAME.equals(target)) {
            checkAgainst = getName();
        } else {
            // Should not get here
            return false;
        }
        if (filter.getCondition().
            equals(SharedFSFilter.COND_CONTAINS)) {
            return checkAgainst.contains(filter.getContent());
        } else if (filter.getCondition().
            equals(SharedFSFilter.COND_DOES_NOT_CONTAIN)) {
            return !checkAgainst.contains(filter.getContent());
        } else if (filter.getCondition().
            equals(SharedFSFilter.COND_STARTS_WITH)) {
            return checkAgainst.startsWith(filter.getContent());
        } else if (filter.getCondition().
            equals(SharedFSFilter.COND_ENDS_WITH)) {
            return checkAgainst.endsWith(filter.getContent());
        } else {
            // Should not get here
            return false;
        }
    }

    private boolean checkIPAddress(SharedFSFilter filter) {
        String [] ipAddresses = getIPAddresses();

        boolean returnValue = false;

        for (int i = 0; i < ipAddresses.length; i++) {
            if (filter.getCondition().
                equals(SharedFSFilter.COND_CONTAINS)) {
                returnValue = ipAddresses[i].contains(filter.getContent());
            } else if (filter.getCondition().
                equals(SharedFSFilter.COND_DOES_NOT_CONTAIN)) {
                returnValue = !ipAddresses[i].contains(filter.getContent());
            } else if (filter.getCondition().
                equals(SharedFSFilter.COND_STARTS_WITH)) {
                returnValue = ipAddresses[i].startsWith(filter.getContent());
            } else if (filter.getCondition().
                equals(SharedFSFilter.COND_ENDS_WITH)) {
                returnValue = ipAddresses[i].endsWith(filter.getContent());
            }

            // Once if returnValue is true, at least one of the IP matches
            // the criteria.  Means we need to include this SharedHostInfo
            // object for the filter.
            if (returnValue) {
                return true;
            }
        }
        // If code reaches here, nothing is found.  Do not include this object
        // for the filter
        return false;
    }

    public String toString() {

        return props.toString();
    }
}
