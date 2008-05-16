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

// ident	$Id: StkClntConn.java,v 1.6 2008/05/16 18:35:29 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class StkClntConn {

    // hostname of ACSLS server
    private String acsServerName = "localhost";

    // portnum for SAM services on ACSLS server
    private String acsPort = "50004";

    // user_id for access control, null = no user_id
    private String access = null;

    // hostname of SAM server on lan connecting to ACSLS host
    // Only sites where a multihomed SAM-FS server is used need to supply this
    private String samServerName = "localhost";

    // fixed port number used by SAM for incoming ACSLS responses
    // 0 means port to be dynamically allocated
    private String samRecvPort = "0";

    // port to which SAM sends requests on ACSLS server.
    // 0 means query the portmapper on the ACSLS server
    private String samSendPort = "0";

    // TBD : All port number should be int, talk to api dev

    /**
     * public constructor
     */
    public StkClntConn(String acsServerName, String acsPort) {

        this.acsServerName = acsServerName;
        this.acsPort = acsPort;

    }

    // private constructor (use for jni)
    private StkClntConn(String acsServerName, String acsPort, String access,
        String samServerName, String samRecvPort, String samSendPort) {

        this(acsServerName, acsPort);

        this.access = access;
        this.samServerName = samServerName;
        this.samRecvPort = samRecvPort;
        this.samSendPort = samSendPort;


    }

    public String getAcsServerName() { return acsServerName; }
    public String getAcsPort() { return acsPort; }
    public String getAccess() { return access; }
    public String getSamServerName() { return samServerName; }
    public String getSamRecvPort() { return samRecvPort; }
    public String getSamSendPort() { return samSendPort; }


    public void setAcsServerName(String acsServerName) {
        acsServerName = acsServerName;
    }
    public void setAcsPort(String acsPort) {
        acsPort = acsPort;
    }
    public void setAccess(String access) {
        access = access;
    }
    public void setSamServerName(String samServerName) {
        samServerName = samServerName;
    }


    /**
     * Sets the port number used by SAM for incoming ACSLS responses
     * Valid values are 1024 - 65535, and 0.
     */
    public void setSamRecvPort(String samRecvPort) {
        samRecvPort = samRecvPort;
    }


    /**
     * Sets the port to which SAM sends requests on ACSLS server.
     * Valid values are 1024 - 65535, and 0.
     */
    public void setSamSendPort(String samSendPort) {
        samSendPort = samSendPort;
    }

    public String toString() {
        String s = "acsServerName = " + acsServerName +
                ", acsPort = " + acsPort +
                ", access = " + access +
                ", samServerName = " + samServerName +
                ", samRecvPort = " + samRecvPort +
                ", samSendPort = " + samSendPort;
        return s;
    }
}
