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

// ident	$Id: FileSystemMountProperties.java,v 1.18 2008/11/06 14:59:06 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.fs;

/**
 * mount options for SAM-FS/QFS filesystems
 */
public interface FileSystemMountProperties extends GenericMountOptions {

    public static final int DEFAULT_HWM = 80;
    public static final int DEFAULT_LWM = 60;

    public FileSystem getFileSystem();
    public void setFileSystem(FileSystem fs);

    public void optimizeForOracle(boolean cluster);

    // basic mount properties

    public int getHWM();
    public void setHWM(int hwm);

    public int getLWM();
    public void setLWM(int lwm);

    public int getStripeWidth();
    public void setStripeWidth(int width);

    public boolean isTrace();
    public void setTrace(boolean traceOn);


    public boolean isQuickWrite();
    public void setQuickWrite(boolean quickWrite);


    // shared QFS mount properties

    public boolean isMountInBackground();
    public void setMountInBackground(boolean mountInBackground);

    public int getNoOfMountRetries();
    public void setNoOfMountRetries(int noOfRetries);

    public int getMetadataRefreshRate();
    public void setMetadataRefreshRate(int rate);

    public long getMinBlockAllocation();
    public void setMinBlockAllocation(long noOfBlocks);

    public int getMinBlockAllocationUnit();
    public void setMinBlockAllocationUnit(int unit);

    public long getMaxBlockAllocation();
    public void setMaxBlockAllocation(long noOfBlocks);

    public int getMaxBlockAllocationUnit();
    public void setMaxBlockAllocationUnit(int unit);

    public int getReadLeaseDuration();
    public void setReadLeaseDuration(int duration);

    public int getWriteLeaseDuration();
    public void setWriteLeaseDuration(int duration);

    public int getAppendLeaseDuration();
    public void setAppendLeaseDuration(int duration);

    public int getMaxConcurrentStreams();
    public void setMaxConcurrentStreams(int noOfStreams);

    public int getMinPool();
    public void setMinPool(int minPool);

    public int getLeaseTimeout();
    public void setLeaseTimeout(int leaseTimeout);

    public boolean isMultiHostWrite();
    public void setMultiHostWrite(boolean write);

    public boolean isConsistencyChecking();
    public void setConsistencyChecking(boolean consistencyChecking);


    // SAM mount properties

    public int getDefaultPartialReleaseSize();
    public void setDefaultPartialReleaseSize(int size);

    public int getDefaultPartialReleaseSizeUnit();
    public void setDefaultPartialReleaseSizeUnit(int unit);

    public int getDefaultMaxPartialReleaseSize();
    public void setDefaultMaxPartialReleaseSize(int size);

    public int getDefaultMaxPartialReleaseSizeUnit();
    public void setDefaultMaxPartialReleaseSizeUnit(int unit);

    public long getPartialStageSize();
    public void setPartialStageSize(long size);

    public int getPartialStageSizeUnit();
    public void setPartialStageSizeUnit(int unit);

    public int getNoOfStageRetries();
    public void setNoOfStageRetries(int number);

    public long getStageWindowSize();
    public void setStageWindowSize(long size);

    public int getStageWindowSizeUnit();
    public void setStageWindowSizeUnit(int unit);

    public boolean isArchiverAutoRun();
    public void setArchiverAutoRun(boolean auto);

    public boolean isArchive();
    public void setArchive(boolean archive);

    // performance tuning mount options

    public long getReadAhead();
    public void setReadAhead(long readAhead);

    public int getReadAheadUnit();
    public void setReadAheadUnit(int unit);

    public long getWriteBehind();
    public void setWriteBehind(long writeBehind);

    public int getWriteBehindUnit();
    public void setWriteBehindUnit(int unit);

    public long getWriteThrottle();
    public void setWriteThrottle(long writeThrottle);

    public int getWriteThrottleUnit();
    public void setWriteThrottleUnit(int unit);

    public int getFlushBehind();
    public void setFlushBehind(int flushBehind);

    public int getFlushBehindUnit();
    public void setFlushBehindUnit(int unit);

    public int getStageFlushBehind();
    public void setStageFlushBehind(int flushBehind);

    public int getStageFlushBehindUnit();
    public void setStageFlushBehindUnit(int unit);

    public boolean isSynchronizedMetadata();
    public void setSynchronizedMetadata(boolean sync);

    public int getMetadataStripeWidth();
    public void setMetadataStripeWidth(int width);

    public boolean isSoftRAID();
    public void setSoftRAID(boolean softRAID);

    public boolean isForceDirectIO();
    public void setForceDirectIO(boolean forceDirectIO);

    public boolean isForceNFSAsync();
    public void setForceNFSAsync(boolean forceNFSAsync);


    // direct IO discovery mount properties

    public int getConsecutiveReads();
    public void setConsecutiveReads(int number);

    public int getWellAlignedReadMin();
    public void setWellAlignedReadMin(int readMin);

    public int getWellAlignedReadMinUnit();
    public void setWellAlignedReadMinUnit(int unit);

    public int getMisAlignedReadMin();
    public void setMisAlignedReadMin(int readMin);

    public int getMisAlignedReadMinUnit();
    public void setMisAlignedReadMinUnit(int unit);

    public int getConsecutiveWrites();
    public void setConsecutiveWrites(int number);

    public int getWellAlignedWriteMin();
    public void setWellAlignedWriteMin(int writeMin);

    public int getWellAlignedWriteMinUnit();
    public void setWellAlignedWriteMinUnit(int unit);

    public int getMisAlignedWriteMin();
    public void setMisAlignedWriteMin(int writeMin);

    public int getMisAlignedWriteMinUnit();
    public void setMisAlignedWriteMinUnit(int unit);

    public boolean isDirectIOZeroing();
    public void setDirectIOZeroing(boolean directIOZeroing);

    // unsupported mount options
    public String getUnsupportedMountOptions();

    // OSD mount options
    public long getObjectDepth();
    public void setObjectDepth(long depth);
    public int getObjectWidth();
    public void setObjectWidth(int width);
}
