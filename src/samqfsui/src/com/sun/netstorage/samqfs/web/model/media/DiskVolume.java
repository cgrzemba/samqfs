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

// ident $Id: DiskVolume.java,v 1.6 2008/03/17 14:43:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.media;


/**
 *  information about a disk volume (VSN)
 */
public interface DiskVolume {


    public String getName();

    /**
     *  return null if local disk VSN
     */
    public String getRemoteHost();

    /**
     *  return absolute disk path
     */
    public String getPath();

    /**
     * return capacity in bytes
     */
    public long getCapacityKB();

    /**
     * return available space in kbytes
     */
    public long getAvailableSpaceKB();

    /*
     * disk VSN media flags information
     */

    public boolean isLabeled();
    public void setLabeled(boolean labeled);

    public boolean isReadOnly();
    public void setReadOnly(boolean readOnly);

    public boolean isBadMedia();
    public void setBadMedia(boolean badMedia);

    public boolean isUnavailable();
    public void setUnavailable(boolean unavailable);

    public boolean isRemote();

    public boolean isUnknown();


    // honey comb archiving target
    public boolean isHoneyCombVSN();
}
