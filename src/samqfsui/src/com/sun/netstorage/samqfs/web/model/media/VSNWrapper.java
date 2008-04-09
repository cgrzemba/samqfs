/*
 *    SAM-QFS_notice_begin
 *
 *      Solaris 2.x Sun Storage & Archiving Management File System
 *
 *      Copyright (c) 2007 Sun Microsystems, Inc.
 *      All Rights Reserved.
 *
 *      Government Rights Notice
 *      Use, duplication, or disclosure by the U.S. Government is
 *      subject to restrictions set forth in the Sun Microsystems,
 *      Inc. license agreements and as provided in DFARS 227.7202-1(a)
 *      and 227.7202-3(a) (1995), DRAS 252.227-7013(c)(ii) (OCT 1988),
 *      FAR 12.212(a)(1995), FAR 52.227-19, or FAR 52.227-14 (ALT III),
 *      as applicable.  Sun Microsystems, Inc.
 *
 *    SAM-QFS_notice_end
 */

// ident    $Id: VSNWrapper.java,v 1.2 2008/04/09 20:37:31 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.media;

/**
 * Wrapper of VSN [] & DiskVol [].  Either array will be populated after
 * the evaluation of VSN expression defined in the VSN Browser Pop Up.
 */

public class VSNWrapper {

    public VSN [] allTapeVSNs;
    public DiskVolume[] allDiskVSNs;
    public long freeSpaceInMB;
    public int totalNumberOfVSNs;

    // Expression used in search criteria to come up with matching VSNs
    public String expressionUsed;

    /** Creates a new instance of VSNWrapper */
    public VSNWrapper() {
    }

    public VSNWrapper(
        VSN [] vsns, DiskVolume[] diskvolumes,
        long freeSpaceInMB, int totalNumberOfVSNs,
        String expressionUsed) {
        this.allTapeVSNs = vsns;
        this.allDiskVSNs = diskvolumes;
        this.freeSpaceInMB = freeSpaceInMB;
        this.totalNumberOfVSNs = totalNumberOfVSNs;
        this.expressionUsed = expressionUsed;
    }

    public VSN [] getAllTapeVSNs() {
        return allTapeVSNs;
    }

    public DiskVolume [] getAllDiskVSNs() {
        return allDiskVSNs;
    }

    public long getFreeSpaceInMB() {
        return freeSpaceInMB;
    }

    public int getTotalNumberOfVSNs() {
        return totalNumberOfVSNs;
    }

    public String getExpressionUsed() {
        return expressionUsed;
    }   
}
