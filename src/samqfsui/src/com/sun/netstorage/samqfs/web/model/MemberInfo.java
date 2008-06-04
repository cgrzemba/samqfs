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

// ident $Id: MemberInfo.java,v 1.1 2008/06/04 18:16:10 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.Properties;

/**
 * This is a helper class to parse the string from the backend and decode it
 * to a class object called MemberInfo.  This class is used in the Shared File
 * System Details page.
 *
 * Possible input strings:
 * "pmds=8,unmounted=2,off=0,error=0" (for shared file system details)
 * "clients=1024,unmounted=24,off=2,error=0" (for clients table)
 * "storage_nodes=124,unmounted=0,off=1,error=0" (for storage nodes table)
 */
public class MemberInfo {

    protected String KEY_PMDS = "pmds";
    protected String KEY_UNMOUNTED = "unmounted";
    protected String KEY_OFF = "off";
    protected String KEY_ERROR = "error";
    protected String KEY_CLIENTS = "clients";
    protected String KEY_STORAGE_NODES = "storage_nodes";
    protected String infoStr = null;
    protected Properties props = null;

    /**
     * Class constructor
     * @param infoStr initial string got from the backend
     */
    public MemberInfo(String infoStr) {
        TraceUtil.trace3("MemberInfo: " + infoStr);
        this.infoStr = infoStr;
        this.props = ConversionUtil.strToProps(infoStr);
    }

    /**
     * Determine if the input string is used in the summary section
     * @return true if the input string is used in the summary section
     */
    public boolean isSummary() {
        return infoStr.indexOf(KEY_PMDS) != -1;
    }

    /**
     * Determine if the input string is used in the clients section
     * @return true if the input string is used in the clients section
     */
    public boolean isClients() {
        return infoStr.indexOf(KEY_CLIENTS) != -1;
    }

    /**
     * Determine if the input string is used in the storage nodes section
     * @return true if the input string is used in the storage nodes section
     */
    public boolean isStorageNodes() {
        return infoStr.indexOf(KEY_STORAGE_NODES) != -1;
    }

    /**
     * Return the number of potential metadata servers.
     * This number is only available when the string is used in the summary
     * section.
     * @return number of potential metadata servers
     */
    public int getPmds() {
        // Only applicable in summary section
        if (!isSummary()) {
            return -1;
        }
        String propStr = props.getProperty(KEY_PMDS);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo::getPmds()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the number of clients.
     * This number is only available when the string is used in the clients
     * section.
     * @return number of clients
     */
    public int getClients() {
        // Only applicable in clients section
        if (!isClients()) {
            return -1;
        }
        String propStr = props.getProperty(KEY_CLIENTS);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo::getClients()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the number of storage nodes.
     * @return number of storage nodes
     */
    public int getStorageNodes() {
        // Only applicable in clients section
        if (!isStorageNodes()) {
            return -1;
        }
        String propStr = props.getProperty(KEY_STORAGE_NODES);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo:getStorageNodes()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the number of servers in good state.
     * This call is not applicable in the summary section.
     * @return number of servers that are OK
     */
    public int getOk() {
        // Not applicable in the summary section
        if (isSummary()) {
            return -1;
        }

        int ok = -1;
        if (isClients()) {
            ok = getClients() - getUnmounted() - getOff() - getError();
        } else if (isStorageNodes()) {
            ok = getStorageNodes() - getUnmounted() - getOff() - getError();
        }
        return ok;
    }

    /**
     * Return the number of unmounted servers.
     * @return number of unmounted servers
     */
    public int getUnmounted() {
        String propStr = props.getProperty(KEY_UNMOUNTED);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo::getUnmounted()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the number of disabled servers.
     * @return number of disabled servers
     */
    public int getOff() {
        String propStr = props.getProperty(KEY_OFF);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo::getOff()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the number of erroneus servers.
     * @return number of erroneus servers
     */
    public int getError() {
        String propStr = props.getProperty(KEY_ERROR);
        try {
            return Integer.parseInt(propStr);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException caught in MemberInfo::getError()!",
                numEx);
        }
        return -1;
    }

    /**
     * Return the input string
     * @Override
     * @return shrink option string
     */
    public String toString() {
        return infoStr;
    }
}
