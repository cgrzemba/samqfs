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

// ident $Id: ClusterNodeInfo.java,v 1.6 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about a SunCluster node
 */
public class ClusterNodeInfo {

    // constants below must match those in mgmt.h
    final String KEY_SC_NODENAME = "sc_nodename";
    final String KEY_SC_NODEID   = "sc_nodeid";
    final String KEY_SC_NODESTATUS   = "sc_nodestatus";
    final String KEY_SC_NODEPRIVADDR = "sc_nodeprivaddr";

    protected String name, status, privIP;
    protected int id;

    public ClusterNodeInfo(Properties props) throws SamFSException {
        if (props == null)
            return;

        name = props.getProperty(KEY_SC_NODENAME);
        id   = ConversionUtil.strToIntVal(props.getProperty(KEY_SC_NODEID));
        status = props.getProperty(KEY_SC_NODESTATUS);
        privIP = props.getProperty(KEY_SC_NODEPRIVADDR);
    }
    public ClusterNodeInfo(String propStr) throws SamFSException {
        this(ConversionUtil.strToProps(propStr));
    }

    public String getName() { return name; }
    public    int getID()   { return id; }
    public String getStatus() { return status; }
    public String getPrivIP() { return privIP; }


    public String toString() {
        String E = "=", C = ",";
        String s = KEY_SC_NODENAME + E + name + C +
                   KEY_SC_NODEID   + E + id   + C +
                   KEY_SC_NODESTATUS + E + status + C +
                   KEY_SC_NODEPRIVADDR + E + privIP;
        return s;
    }
}
