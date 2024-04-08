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

// ident	$Id: ServerInfo.java,v 1.13 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import java.io.Serializable;

/**
 * This class contains the server information like
 * license type, samfsServerAPIVersion
 * May be expanded later to include more info
 */
public class ServerInfo implements Serializable {

    private String samfsServerName = new String();
    private String samfsServerAPIVersion = new String();
    private short licenseType = -1;

    // NOTE:
    // If you use this constructor, you MUST immediately call the setters for
    // Version and License type or this object will be left unstable.
    public ServerInfo(String hostName) {
        this.samfsServerName = hostName;
    }

    public ServerInfo(String name, String version, short type) {

        if ((SamQFSUtil.isValidString(name)) ||
            (SamQFSUtil.isValidString(version))) {

            this.samfsServerName = name;
            this.samfsServerAPIVersion = version;
            this.licenseType = type;
        }
    }

    public void setSamfsServerAPIVersion(String version) {
        this.samfsServerAPIVersion = version;
    }

    public void setServerLicenseType(short ltype) {
        this.licenseType = ltype;
    }

    public String getSamfsServerName() { return samfsServerName; }

    public String getSamfsServerAPIVersion() { return samfsServerAPIVersion; }

    public short getServerLicenseType() { return licenseType; }

    public String toString() {
        String str = new StringBuffer("Server Name: ").append(
        samfsServerName).append(" samfs server api version: ").append(
        samfsServerAPIVersion).append(" license type: ").append(
		licenseType).append("tabsModel info: ").toString();

        return str;
    }
}
