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

// ident    $Id: MDSAddresses.java,v 1.7 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates information about a metadata server and its ip
 */
public class MDSAddresses {

    // constants below must match those in mgmt.h
    final String KEY_MDS_HOSTNAME = "hostname";
    final String KEY_MDS_IP_ADDRESSES = "ipaddresses";

    protected String hostName;
    protected String [] ipAddresses;


    public MDSAddresses(Properties props) throws SamFSException {
        if (props == null)
            return;

        String ipAddressesString = null;

        hostName = props.getProperty(KEY_MDS_HOSTNAME);
        ipAddressesString = props.getProperty(KEY_MDS_IP_ADDRESSES);

        ipAddresses = ipAddressesString.split(" ");

    }
    public MDSAddresses(String propStr) throws SamFSException {
        this(ConversionUtil.strToProps(propStr));
    }

    public MDSAddresses(String hostName, String [] ipAddresses) {
        this.hostName = hostName;
        this.ipAddresses  = ipAddresses;
    }

    public String getHostName() { return hostName; }
    public String [] getIPAddress() { return ipAddresses; }


    public String toString() {
        String E = "=", C = ",", S = " ";
        String s = KEY_MDS_HOSTNAME  + E + hostName + C +
                    KEY_MDS_IP_ADDRESSES + E;

        for (int i = 0; i < ipAddresses.length; i++) {
            if (i > 0) s += S;
            s += ipAddresses[i];
        }
        return s;
    }
}
