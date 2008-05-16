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

// ident	$Id: Drive.java,v 1.16 2008/05/16 18:39:04 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.media;


import com.sun.netstorage.samqfs.mgmt.SamFSException;


public interface Drive extends BaseDevice {

    public Library getLibrary() throws SamFSException;

    public String getLibraryName();


    public VSN getVSN() throws SamFSException;

    public String getVSNName();


    public String getVendor() throws SamFSException;


    public String getProductID() throws SamFSException;


    public String getSerialNo() throws SamFSException;


    public String getFirmwareLevel() throws SamFSException;


    public int[] getDetailedStatus();


    public void idle() throws SamFSException;


    public void unload() throws SamFSException;


    public void clean() throws SamFSException;


    public String[] getMessages();

    public boolean unLabeled();

    public boolean isShared();

    public void setShared(boolean shared) throws SamFSException;

    /**
     * Retrieve the load idle time for the drive
     * @since 4.6
     * @return the load idle time for the drive
     */
    public long getLoadIdleTime();

    /**
     * Set the load idle time for the drive
     * @since 4.6
     * @param loadIdleTime - the load idle time for the drive
     */
    public void setLoadIdleTime(long loadIdleTime);

    /**
     * Retrieve the tape alert flags for the drive
     * @since 4.6
     * @return the tape alert flags for the drive
     */
    public long getTapeAlertFlags();

    /**
     * Set the tape alert flags for the drive
     * @since 4.6
     * @param tapeAlertFlags - the tape alert flags for the drive
     */
    public void setTapeAlertFlags(long tapeAlertFlags);


}
