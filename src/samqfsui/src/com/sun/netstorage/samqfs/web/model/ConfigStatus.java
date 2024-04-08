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

// ident $Id: ConfigStatus.java,v 1.8 2008/12/16 00:12:15 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about the SAM-FS configuration
 * status - currently the state of each file.
 */
public class ConfigStatus {

    final String KEY_CONFIG = "config";  // e.g.: archiver.cmd
    final String KEY_STATUS = "status";  //  OK | WARNINGS | ERRORS | MODIFIED
    final String KEY_MSG    = "message"; // e.g.: run for your life
    final String KEY_VSNS   = "vsns";    // list of vsns with problems


    protected String config, status, msg, vsns[];


    public ConfigStatus(Properties props) throws SamFSException {
        if (props == null)
            return;
        config = props.getProperty(KEY_CONFIG);
        status = props.getProperty(KEY_STATUS);
        msg    = props.getProperty(KEY_MSG);
        vsns   = ConversionUtil.strToArray(props.getProperty(KEY_VSNS), ' ');
    }
    public ConfigStatus(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    public String getConfig() { return config; }
    public String getStatus() { return status; }

    /**
     * message that recommends user action
     */
    public String getMsg()    { return msg; }

    /**
     * list of VSNs (might be empty)
     */
    public String[] getVSNs() { return vsns; }


    public String toString() {
        String E = "=", C = ",";
        String s = KEY_CONFIG + E + config + C +
                   KEY_STATUS + E + status + C +
                   KEY_MSG    + E + msg + C +
                   KEY_VSNS   + E + vsns;
        return s;
    }
}
