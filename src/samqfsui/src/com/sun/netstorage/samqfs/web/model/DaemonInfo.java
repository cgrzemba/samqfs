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

// ident $Id: DaemonInfo.java,v 1.6 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Job;

/**
 * this class encapsulates basic information about the SAM-FS/QFS daemon
 */
public class DaemonInfo {

    final String KEY_ID    = "activityid";
    final String KEY_DETAILS   = "details"; // includes name + args
    final String KEY_DESCR = "description"; // descriptive name


    protected String details, descr;


    public DaemonInfo(Properties props) throws SamFSException {
        if (props == null)
            return;
        details = props.getProperty(KEY_DETAILS);
        descr   = props.getProperty(KEY_DESCR);
    }
    public DaemonInfo(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    public String getName() { return details; }
    public String getDescr() { return descr; }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_DETAILS + E + details + C +
                   KEY_DESCR   + E + descr;
        return s;
    }
}
