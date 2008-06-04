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

// ident $Id: SharedHostInfo.java,v 1.1 2008/06/04 18:16:10 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;

/**
 * this class encapsulates information about a shared file system host
 *
 * Keys shared by all classes of host include:
 * hostName = %s
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

    final String KEY_NAME = "hostName";
    final String KEY_TYPE = "type";
    final String KEY_IP_ADDRESSES = "ip_addresses";
    final String KEY_OS = "os";
    final String KEY_VERSION = "version";
    final String KEY_ARCH = "arch";
    final String KEY_MOUNTED = "mounted";
    final String KEY_STATUS = "status";
    final String KEY_ERROR = "error";
    final String KEY_FAULTS = "faults";

    // HOST_DETAILS
    final String KEY_FI_STATUS = "fi_status";
    final String KEY_FI_FLAGS = "fi_flags";
    final String KEY_MNT_CFG = "mnt_cfg";
    final String KEY_MNT_CFG_1 = "mnt_cfg1";
    final String KEY_NO_MSGS = "no_msgs";
    final String KEY_LOW_MSG = "low_msg";


    // definitions

    // type = OSD | client | mds | pmds
    final short TYPE_MDS = 0;
    final short TYPE_PMDS = 1;
    final short TYPE_CLIENT = 2;
    final short TYPE_OSD = 3;

    // arch = x86 | sparc
    final short ARCH_SPARC = 0;
    final short ARCH_X86 = 1;

    // status = ON | OFF
    final short STATUS_ON = 0;
    final short STATUS_OFF = 1;

    // error = assumed_dead | known_dead
    final short ERROR_ASSUMED_DEAD = 0;
    final short ERROR_KNOWN_DEAD = 1;

    protected Properties props = null;

    public SharedHostInfo(String inputString) {
        props = ConversionUtil.strToProps(inputString);
        if (props == null) {
            return;
        }
    }

    public String getName() {
        return props.getProperty(KEY_NAME);
    }

    public String getOS() {
        return props.getProperty(KEY_OS);
    }
    
    public String [] getIPAddresses() {
        String str = props.getProperty(KEY_IP_ADDRESSES);
        if (str.length() == 0) {
            return new String[0];
        }
        return str.split(" ");
    }

    public String getVersion() {
        return props.getProperty(KEY_VERSION);
    }

    public short getType() {
        String str = props.getProperty(KEY_TYPE);
        if ("mds".equals(str)) {
            return TYPE_MDS;
        } else if ("pmds".equals(str)) {
            return TYPE_PMDS;
        } else if ("client".equals(str)) {
            return TYPE_CLIENT;
        } else if ("OSD".equals(str)) {
            return TYPE_OSD;
        }
        return -1;
    }

    public short getArch() {
        String str = props.getProperty(KEY_ARCH);
        if ("sparc".equals(str)) {
            return ARCH_SPARC;
        } else if ("x86".equals(str)) {
            return ARCH_X86;
        }
        return -1;
    }

    public int getMounted() {
        String str = props.getProperty(KEY_MOUNTED);
        if (str.length() == 0) {
            return -1;
        }
        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException ex) {
            return -1;
        }
    }
    
    public short getStatus() {
        String str = props.getProperty(KEY_STATUS);
        if ("ON".equals(str)) {
            return STATUS_ON;
        } else if ("OFF".equals(str)) {
            return STATUS_OFF;
        }
        return -1;
    }

    public short getError() {
        String str = props.getProperty(KEY_ERROR);
        if ("assumed_dead".equals(str)) {
            return ERROR_ASSUMED_DEAD;
        } else if ("known_dead".equals(str)) {
            return ERROR_KNOWN_DEAD;
        }
        return -1;
    }

    public int getFaults() {
        String str = props.getProperty(KEY_FAULTS);
        if (str.length() == 0) {
            return -1;
        }
        try {
            return Integer.parseInt(str);
        } catch (NumberFormatException ex) {
            return -1;
        }
    }

    public String toString() {

        return props.toString();
    }
}
