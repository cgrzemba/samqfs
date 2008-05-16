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

// ident $Id: NFSOptions.java,v 1.9 2008/05/16 18:39:00 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates NFS options for a particular directory
 */
public class NFSOptions {

    protected String dirName;
    protected boolean shared;
    protected String shareState;
    protected String roClients, rwClients, rootClients;

    protected static final String KEY_DIRNAME = "dirname";
    protected static final String KEY_SHSTATE = "nfs";
    protected static final String KEY_RO = "ro";
    protected static final String KEY_RW = "rw";
    protected static final String KEY_ROOT = "root";

    public static final String NFS_SHARED = "yes";
    public static final String NFS_NOTSHARED  = "no";
    public static final String NFS_CONFIGURED = "config";

    // next 2 constructors are used internally by the logic layer
    public NFSOptions(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }
    public NFSOptions(Properties props) throws SamFSException {
        if (props == null) {
            return;
        }

        dirName = props.getProperty(KEY_DIRNAME);
        setShareState(props.getProperty(KEY_SHSTATE));
        roClients = props.getProperty(KEY_RO);
        rwClients = props.getProperty(KEY_RW);
        rootClients = props.getProperty(KEY_ROOT);
    }

    /**
     * valid shareState vals are NFS_SHARED NFS_NOTSHARED and NFS_CONFIGURED
     */
    public NFSOptions(String dirName, String shareState) {
        this.dirName = dirName;
        setShareState(shareState);
    }

    public String getDirName() { return dirName; }

    public boolean isShared() { return shared; }
    public void setShareState(String shareState) {
        this.shareState = shareState;
        shared = NFS_SHARED.equals(shareState);
    }

    public String getShareState() { return shareState; }

    public String getReadOnlyAccessList() { return roClients; }
    /**
     * null argument means restore to default (everybody)
     */
    public void setReadOnlyAccessList(String clients) {
        roClients = clients;
    }

    public String getReadWriteAccessList() { return rwClients; }
    /**
     * null argument means restore to default (everybody)
     */
    public void setReadWriteAccessList(String clients) {
        rwClients = clients;
    }

    public String getRootAccessList() { return rootClients; }
    /**
     * null argument means restore to default (nobody)
     */
    public void setRootAccessList(String clients) {
        rootClients = clients;
    }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_DIRNAME + E + dirName + C +
                   KEY_SHSTATE + E + shareState;
        if (null != roClients)
            s += C + KEY_RO + E + roClients;
        if (null != rwClients)
            s += C + KEY_RW + E + rwClients;
        if (null != rootClients)
            s += C + KEY_ROOT + E + rootClients;
        return s;
    }
}
