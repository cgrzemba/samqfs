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

// ident        $Id: SamFSConnection.java,v 1.14 2008/12/16 00:08:53 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class SamFSConnection {

    long connHandle; /* copied to Ctx and later used by native code */
    String serverAPIver;
    String SAMver;
    String hostName;
    String serverArch = new String();

    private SamFSConnection(long connHandle, String APIver, String SAMver,
        String hostName, String serverArch) {

        this.connHandle = connHandle;
        this.serverAPIver = APIver;
        this.SAMver = SAMver;
        this.hostName = hostName;
        this.serverArch = serverArch;
    }

    /*
     * public methods
     */

    public String getServerAPIver() {
        return serverAPIver;
    }

    public String getServerVer() {
        return SAMver;
    }

    public String getServerHostname() {
        return hostName;
    }

    public String getServerArch() {
        return serverArch;
    }

    /**
     * create a new connection. use default timeout
     */
    public static native SamFSConnection getNew(String serverName)
        throws SamFSException;

    /**
     * get the default timeout in seconds
     */
    public static native long getDefaultTimeout();

    /**
     * create new connection with the specified timeout
     */
    public static native SamFSConnection getNewSetTimeout(String serverName,
        long secs) throws SamFSException;

    /*
     * releases the resources associated with this connection
     */
    public native void destroy()
        throws SamFSException;

    /**
     * forces the user to acknowledge the fact that an unexpected configuration
     * change (manual modification of the SAM-FS configuration files)
     * occurred on the SAM-FS/QFS server associated with this connection.
     * This should be called after a 'not initialized' exception is received
     */
    public native void reinit()
        throws SamFSException;

    /**
     * get the timeout interval (in seconds) for this connection.
     * This applies to all calls that use Ctx objects mapped to this connection.
     */
    public native long getTimeout() throws SamFSException;
    /**
     * change the timeout interval for this connection.
     * This applies to all calls that use Ctx objects mapped to this connection.
     */
    public native void setTimeout(long secs) throws SamFSException;

    public String toString() {

        String s  = "SAMQconn[" + connHandle + "," + serverAPIver +
            "," + SAMver + "," + hostName + "," + serverArch + ",";
        try {
            s += getTimeout() +"s]";
        } catch (SamFSException e) {
            s += "err]";
        }
        return s;
    }
}
