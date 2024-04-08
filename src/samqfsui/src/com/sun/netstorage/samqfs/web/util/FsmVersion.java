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

// ident	$Id: FsmVersion.java,v 1.9 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import java.security.InvalidParameterException;

/**
 * This class is a helper class for managing backwards compatibility within the
 * UI.  Given a certain UI version (either the current UI version, or a version
 * that first contained a certain feature), you can determine whether
 * a particular server has an API version that is compatible with or newer than
 * the UI version in question.
 */
public class FsmVersion {

    public static final String CURRENT_UI_VERSION = "5.0";

    // This is the product version of the FS Manager being compared to.
    protected String uiVersion = null;

    protected String serverName = null;

    // This is the API version that is fully compatible with the UI version
    protected String compatibleAPIVersion = null;

    // This is the API version of the server in question.
    protected String serverAPIVersion = null;

    protected boolean isServerCompatibleWithUI;

    private FsmVersion() {

    }

    /**
     * Compares the current UI version with the API version of the given server.
     *
     * @param serverName Provide the name of the server in question.
     * Version comparisons will be done against the API version of this server.
     */
    public FsmVersion(String serverName) {
        this(CURRENT_UI_VERSION,  serverName);
    }

    /**
     * Compares the given UI version with the API version of the given server.
     *
     * @param uiVersion Provide the version of the UI that contains
     * the feature in question.
     *
     * @param serverName Provide the name of the server in question.
     * Version comparisons will be done against the API version of this server.
     */
    public FsmVersion(String uiVersion, String serverName) {
        if (uiVersion == null) {
            // Developer error
            throw new
                InvalidParameterException("You must specify the ui version.");
        }
        this.uiVersion = uiVersion;

        if (serverName == null) {
            // Developer error
            throw new InvalidParameterException(
                "You must specify the server name.");
        }
        this.serverName = serverName;

        compatibleAPIVersion = SamUtil.getAPIVersionFromUIVersion(uiVersion);
        try {
            serverAPIVersion = SamUtil.getAPIVersion(serverName);
        } catch (SamFSException e) {
            // Hmmm.... bad.  Assume compatibility and trace.
            TraceUtil.trace1(SamUtil.getResourceString(
                                    "VersionUtil.serverAPIError",
                                    new String [] {
                                            serverName,
                                            compatibleAPIVersion,
                                            uiVersion}), e);
            serverAPIVersion = compatibleAPIVersion;
        }

        isServerCompatibleWithUI = SamUtil.isVersionCurrentOrLaterThan(
                                            serverAPIVersion,
                                            compatibleAPIVersion);
    }

    public boolean isAPICompatibleWithUI() {
        return isServerCompatibleWithUI;
    }
    public String getUIVersion() {
        return uiVersion;
    }

    public String getServerName() {
        return serverName;
    }
}
