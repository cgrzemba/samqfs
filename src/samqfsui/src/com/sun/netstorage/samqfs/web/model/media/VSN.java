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

// ident	$Id: VSN.java,v 1.13 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.media;


import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.mgmt.SamFSException;


public interface VSN {


    public static final int LABEL = 0;

    public static final int RELABEL = 1;

    public static final int OWNER = 2;

    public static final int GROUP = 3;

    public static final int STARTING_DIR = 4;


    public Library getLibrary() throws SamFSException;


    public Drive getDrive() throws SamFSException;


    public String getVSN() throws SamFSException;


    public int getSlotNumber() throws SamFSException;


    public String getBarcode() throws SamFSException;


    public long getCapacity() throws SamFSException;


    public long getAvailableSpace() throws SamFSException;


    public long getBlockSize() throws SamFSException;


    public long getAccessCount() throws SamFSException;


    public GregorianCalendar getLabelTime() throws SamFSException;


    public GregorianCalendar getMountTime() throws SamFSException;


    public GregorianCalendar getModificationTime() throws SamFSException;


    public boolean isReserved() throws SamFSException;


    public GregorianCalendar getReservationTime() throws SamFSException;


    // probably there should be just one policy
    public String getReservedByPolicyName() throws SamFSException;

    public String getReservedByFileSystemName() throws SamFSException;

    public int getReservationByType() throws SamFSException;

    public String getReservationNameForType() throws SamFSException;


    // attributes of a VSN

    public boolean isMediaDamaged() throws SamFSException;

    public void setMediaDamaged(boolean mediaDamaged) throws SamFSException;


    public boolean isDuplicateVSN() throws SamFSException;

    public void setDuplicateVSN(boolean duplicateVSN) throws SamFSException;


    public boolean isReadOnly() throws SamFSException;

    public void setReadOnly(boolean readOnly) throws SamFSException;


    public boolean isWriteProtected() throws SamFSException;

    public void setWriteProtected(boolean writeProtected)
        throws SamFSException;


    public boolean isForeignMedia() throws SamFSException;

    public void setForeignMedia(boolean foreignMedia) throws SamFSException;


    public boolean isRecycled() throws SamFSException;

    public void setRecycled(boolean recycle) throws SamFSException;


    public boolean isVolumeFull() throws SamFSException;

    public void setVolumeFull(boolean volumeFull) throws SamFSException;


    public boolean isUnavailable() throws SamFSException;

    public void setUnavailable(boolean unavailable) throws SamFSException;


    public boolean isNeedAudit() throws SamFSException;

    public void setNeedAudit(boolean needAudit) throws SamFSException;



    // this method needs to be called to make effect of an attribute change
    public void changeAttributes() throws SamFSException;


    public void audit() throws SamFSException;


    public void load() throws SamFSException;


    public void export() throws SamFSException;


    /**
     * type = VSN.LABEL for labeling, VSN.RELABEL for relabeling
     */
    public long label(int type, String labelName, long blockSize)
        throws SamFSException;


    public void clean() throws SamFSException;

    /**
     * type = VSN.OWNER, VSN.GROUP, VSN.STARTING_DIR
     */
    public void reserve(String policyName, String filesystemName,
                        int type, String name) throws SamFSException;
}
