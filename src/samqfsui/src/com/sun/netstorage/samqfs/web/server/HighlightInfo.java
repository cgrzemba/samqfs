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

// ident	$Id: HighlightInfo.java,v 1.9 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

/**
 * This class contains the Version Highlight information.
 * The data structure is as follow:
 *
 * - Type
 * - Name
 * - VersionInfo - Number###Status
 */

public class HighlightInfo {

    private String featureType = new String();
    private String featureName = new String();
    private String serverVersion = new String();
    private String versionInfo = new String();

    public HighlightInfo(
        String featureType, String featureName,
        String serverVersion, String versionInfo) {

        this.featureType   = featureType;
        this.featureName   = featureName;
        this.serverVersion = serverVersion;
        this.versionInfo   = versionInfo;
    }

    public String getFeatureType() {
        return (featureType == null) ?  "" : featureType;
    }

    public String getFeatureName() {
        return (featureName == null) ? "" : featureName;
    }

    public String getServerVersion() {
        return (serverVersion == null) ? "" : serverVersion;
    }

    public String getVersionInfo() {
        return (versionInfo == null) ? "" : versionInfo;
    }

    public String toString() {
        String str =
            "Type: " + featureType + "   Name: " + featureName +
            "Server Version: " + serverVersion +
            " Version: " + versionInfo;
        return str;
    }
}
