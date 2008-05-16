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

// ident	$Id: SharedDiskCacheImpl.java,v 1.9 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;

import java.util.ArrayList;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;


public class SharedDiskCacheImpl extends DiskCacheImpl
    implements SharedDiskCache {

    private ArrayList clients;
    private ArrayList servers;
    private ArrayList clientDevpaths;
    private ArrayList serverDevpaths;
    private boolean usedByClient;

    /* following methods are used internally by the logic tier */

    public SharedDiskCacheImpl(DiskCacheImpl dc) {
	super(dc.getJniDisk());
	clients = new ArrayList();
	servers = new ArrayList();
	clientDevpaths = new ArrayList();
	serverDevpaths = new ArrayList();
    }

    public void addClient(String client) { clients.add(client); }
    public void addServer(String server) { servers.add(server); }
    public void addClientDevpath(String dev) { clientDevpaths.add(dev); }
    public void addServerDevpath(String dev) { serverDevpaths.add(dev); }
    public void setUsedByClient(boolean used) { usedByClient = used; }

    /* following methods should be used by the GUI */


    /* clients that can see this device */
    public String[] availFromClients() {
	return (String[])clients.toArray(new String[0]);
    }

    /* true if already in use on at least one of the client hosts */
    public boolean usedByClient() {
	return usedByClient;
    }

    /* potential metadata servers that can see this device */
    public String[] availFromServers() {
	return (String[])servers.toArray(new String[0]);
    }

    /* client device names (paths) */
    public String[] getClientDevpaths() {
	return (String[])clientDevpaths.toArray(new String[0]);
    }

    /* server device names (paths) */
    public String[] getServerDevpaths() {
	return (String[])serverDevpaths.toArray(new String[0]);
    }

    public String toString() {
	int i;
	String s = super.toString() + "Available remotely from: ";
	for (i = 0; i < servers.size(); i++)
	    s += " " + servers.get(i) + " as " + serverDevpaths.get(i);
	for (i = 0; i < clients.size(); i++)
	    s += " " + clients.get(i) + " as " + clientDevpaths.get(i);
	if (usedByClient)
	    s += " already used by client(s)!";
	return (s);
    }
}
