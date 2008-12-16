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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: StkNetLibParam.java,v 1.10 2008/12/16 00:08:56 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class StkNetLibParam extends NetLibParam {

    private String access = null;  // user_id used by client for access control
                                   // no user_id is represented by null string
    private String acsServerName;  // hostname for server that is running ACSLS
                                   // If null or empty, localhost will be used
    private int acsPort = 50004;   // port no. for SSI services on ACSLS server
                                   // default is 50004

    // hostname of SAM server on lan connecting to ACSLS host
    // Only sites where a multihomed SAM-FS server is used need to supply this
    private String samServerName = "localhost"; // ssi_host

    // fixed port number used by SAM for incoming ACSLS responses
    // 0 means port to be dynamically allocated
    private int samRecvPort = 0;   // ssi_inet_portnum

    // port to which SAM sends requests on ACSLS server.
    // 0 means query the portmapper on the ACSLS server
    private int samSendPort = 0;  // csi_hostport

    // these are not used by the GUI, but need to be preserved
    private StkCap stkCap;
    private StkCapacity[] stkCapacities;
    private StkDevice[] stkDevices;

    /**
     * private constructor
     */
    private StkNetLibParam(String path,
        String access, String acsServerName, int acsPort,
        String samServerName, int samRecvPort, int samSendPort,
        StkCap stkCap, StkCapacity[] stkCapacities, StkDevice[] stkDevices) {

        super(path);
        this.access = access;
        this.acsServerName = acsServerName;
        this.samRecvPort = samRecvPort;
        this.acsPort = acsPort;
        this.samServerName = samServerName;
        this.samSendPort = samSendPort;
        this.stkCap = stkCap;
        this.stkCapacities = stkCapacities;
        this.stkDevices = stkDevices;

    }

    public  StkNetLibParam(String path, String acsServerName, int acsPort) {

	super(path);
	this.acsServerName = acsServerName;
	this.acsPort = acsPort;

    }

    public  StkNetLibParam(String path, String acsServerName,
        int acsPort, StkDevice[] stkDevices) {

        super(path);
        this.acsServerName = acsServerName;
        this.acsPort = acsPort;
        this.stkDevices = stkDevices;

    }

    public String getAcsServerName() { return acsServerName; }
    public int getAcsPort() { return acsPort; }
    public String getAccess() { return access; }
    public String getSamServerName() { return samServerName; }
    public int getSamRecvPort() { return samRecvPort; }
    public int getSamSendPort() { return samSendPort; }
    public StkCap getStkCap() { return stkCap; }
    public StkCapacity[] getStkCapacity() { return stkCapacities; }
    public StkDevice[] getStkDevice() { return stkDevices; }

    public void setAccess(String access) { this.access = access; }
    public void setSamServerName(String samServerName) {
        this.samServerName = samServerName;
    }
    public void setSamRecvPort(int samRecvPort) {
        this.samRecvPort = samRecvPort;
    }
    public void setSamSendPort(int samSendPort) {
        this.samSendPort = samSendPort;
    }
    public void setAcsPort(int acsPort) {
        this.acsPort = acsPort;
    }
    public String toString() {
        String s = "path = " + getPath() +
                ", acsServerName = " + acsServerName +
                ", acsPort = " + acsPort +
                ", access = " + (access != null ? access : "null") +
                ", samServerName = " + samServerName +
                ", samRecvPort = " + samRecvPort +
                ", samSendPort = " + samSendPort;
        if (stkCap != null) {
            s += "\n    stkCap = " + stkCap;
        }
        if (stkCapacities != null) {
            for (int i = 0; i < stkCapacities.length; i++) {
                s += "\n     capacities" + i + ": " + stkCapacities[i];
            }
        } else {
            s += "\n    capacities = null";
        }
        if (stkDevices != null) {
            for (int i = 0; i < stkDevices.length; i++) {
                s += "\n     stkDevices" + i + ": " + stkDevices[i];
            }
        }
        return s;
    }
}
