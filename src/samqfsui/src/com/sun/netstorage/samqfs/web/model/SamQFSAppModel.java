/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: SamQFSAppModel.java,v 1.14 2008/03/17 14:43:42 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;


import com.iplanet.jato.model.Model;
import java.net.UnknownHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;


public interface SamQFSAppModel extends Model {


    public SamQFSSystemModel[] getAllSamQFSSystemModels();


    /**
     * get the model for the specified host.
     * throw SamFSException if no model can be find for specified host
     */
    public SamQFSSystemModel getSamQFSSystemModel(String hostname)
        throws SamFSException;

    public String getInetHostName(String hostName) throws UnknownHostException;


    public void addHost(String hostname) throws SamFSException;


    public void removeHost(String hostname) throws SamFSException;


    public void updateDownServers();


    /**
     * Get an instance of SamQFSSystemSharedFSManager that can be used
     * to manage Shared QFS file systems.
     */
    public SamQFSSystemSharedFSManager getSamQFSSystemSharedFSManager();


    /**
     * frees all the resources used by the AppModel
     * this object should not be used after this method is called
     */
    public void cleanup();

    /**
     * Check if host is already managed by the GUI
     * @param hostName - Server of which we want to check if it is being
     *  managed by the GUI
     * @return true/false
     */
    public boolean isHostBeingManaged(String hostName);
}
