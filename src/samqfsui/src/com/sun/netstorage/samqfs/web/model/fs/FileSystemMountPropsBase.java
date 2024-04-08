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

// ident	$Id: FileSystemMountPropsBase.java,v 1.12 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

/**
 * FileSystemMountProperties implementations are expected to extend this class
 */
public class FileSystemMountPropsBase implements FileSystemMountProperties {

    protected FileSystem fs = null;


    // basic mount properties

    protected int hwm = -1;
    protected int lwm = -1;
    protected int stripeWidth = -1;
    protected boolean traceOn = false;


    // shared QFS mount properties

    protected boolean mountInBackground = false;
    protected int noOfMountRetries = -1;
    protected int metadataRefreshRate = -1;
    protected long minBlockAllocation = -1;
    protected int minBlockAllocationUnit = -1;
    protected long maxBlockAllocation = -1;
    protected int maxBlockAllocationUnit = -1;
    protected int readLeaseDuration = -1;
    protected int writeLeaseDuration = -1;
    protected int appendLeaseDuration = -1;
    protected int maxConcurrentStreams = -1;
    protected int minPool = -1;
    protected int leaseTimeout = -1;
    protected boolean multiHostWrite = false;
    protected boolean consistencyChecking = false;


    // SAM mount properties

    protected int defaultPartialReleaseSize = -1;
    protected int defaultPartialReleaseSizeUnit = -1;
    protected int defaultMaxPartialReleaseSize = -1;
    protected int defaultMaxPartialReleaseSizeUnit = -1;
    protected long partialStageSize = -1;
    protected int partialStageSizeUnit = -1;
    protected int noOfStageRetries = -1;
    protected long stageWindowSize = -1;
    protected int stageWindowSizeUnit = -1;
    protected boolean archiverAutoRun = false;
    protected boolean archive = false;

    // general file system mount properties

    protected boolean readOnlyMount = false;
    protected boolean noSetUID = false;
    protected boolean quickWrite = false;


    // performance tuning mount options

    protected long readAhead = -1;
    protected int readAheadUnit = -1;
    protected long writeBehind = -1;
    protected int writeBehindUnit = -1;
    protected long writeThrottle = -1;
    protected int writeThrottleUnit = -1;
    protected int flushBehind = -1;
    protected int flushBehindUnit = -1;
    protected int stageFlushBehind = -1;
    protected int stageFlushBehindUnit = -1;
    protected boolean synchronizedMetadata = false;
    protected int metadataStripeWidth = -1;
    protected boolean softRAID = false;
    protected boolean forceDirectIO = false;
    protected boolean forceNFSAsync = false;


    // direct IO discovery mount properties

    protected int consecutiveReads = -1;
    protected int wellAlignedReadMin = -1;
    protected int wellAlignedReadMinUnit = -1;
    protected int misAlignedReadMin = -1;
    protected int misAlignedReadMinUnit = -1;
    protected int consecutiveWrites = -1;
    protected int wellAlignedWriteMin = -1;
    protected int wellAlignedWriteMinUnit = -1;
    protected int misAlignedWriteMin = -1;
    protected int misAlignedWriteMinUnit = -1;
    protected boolean directIOZeroing = false;

    // OSD width and depth
    protected int objectWidth = -1;
    protected long objectDepth = -1;

    public FileSystem getFileSystem()  { return fs; }
    public void setFileSystem(FileSystem fs)  { this.fs = fs; }


    // basic mount properties

    public int getHWM() { return hwm; }
    public void setHWM(int hwm) { this.hwm = hwm; }

    public int getLWM() { return lwm; }
    public void setLWM(int lwm) { this.lwm = lwm; }


    public int getStripeWidth() { return stripeWidth; }
    public void setStripeWidth(int width) { this.stripeWidth = width; }

    public boolean isTrace() {	return traceOn; }
    public void setTrace(boolean traceOn) { this.traceOn = traceOn; }


    // shared QFS mount properties

    public boolean isMountInBackground() { return mountInBackground; }
    public void setMountInBackground(boolean mountInBackground) {
	this.mountInBackground = mountInBackground;
    }

    public int getNoOfMountRetries() { return noOfMountRetries; }
    public void setNoOfMountRetries(int noOfRetries) {
        this.noOfMountRetries = noOfRetries;
    }

    public int getMetadataRefreshRate() { return metadataRefreshRate; }
    public void setMetadataRefreshRate(int rate) {
	this.metadataRefreshRate = rate;
    }

    public long getMinBlockAllocation() { return minBlockAllocation; }
    public void setMinBlockAllocation(long noOfBlocks) {
	this.minBlockAllocation = noOfBlocks;
    }

    public int getMinBlockAllocationUnit() { return minBlockAllocationUnit; }
    public void setMinBlockAllocationUnit(int unit) {
        this.minBlockAllocationUnit = unit;
    }

    public long getMaxBlockAllocation() { return maxBlockAllocation; }
    public void setMaxBlockAllocation(long noOfBlocks) {
	this.maxBlockAllocation = noOfBlocks;
    }

    public int getMaxBlockAllocationUnit() { return maxBlockAllocationUnit; }
    public void setMaxBlockAllocationUnit(int unit) {
	this.maxBlockAllocationUnit = unit;
    }

    public int getReadLeaseDuration() {	return readLeaseDuration; }
    public void setReadLeaseDuration(int duration) {
	this.readLeaseDuration = duration;
    }

    public int getWriteLeaseDuration() { return writeLeaseDuration; }
    public void setWriteLeaseDuration(int duration) {
	this.writeLeaseDuration = duration;
    }

    public int getAppendLeaseDuration() { return appendLeaseDuration; }
    public void setAppendLeaseDuration(int duration) {
	this.appendLeaseDuration = duration;
    }

    public int getMaxConcurrentStreams() { return maxConcurrentStreams; }
    public void setMaxConcurrentStreams(int noOfStreams) {
	this.maxConcurrentStreams = noOfStreams;
    }

    public int getMinPool() { return minPool; }
    public void setMinPool(int minPool) {
	this.minPool = minPool;
    }

    public int getLeaseTimeout() { return leaseTimeout; }
    public void setLeaseTimeout(int leaseTimeout) {
	this.leaseTimeout = leaseTimeout;
    }

    public boolean isMultiHostWrite() {	return multiHostWrite; }
    public void setMultiHostWrite(boolean write) {
        this.multiHostWrite = write;
    }

    public boolean isConsistencyChecking() { return consistencyChecking; }
    public void setConsistencyChecking(boolean consistencyChecking) {
	this.consistencyChecking = consistencyChecking;
    }

    // SAM mount properties

    public int getDefaultPartialReleaseSize() {
	return defaultPartialReleaseSize;
    }
    public void setDefaultPartialReleaseSize(int size) {
	this.defaultPartialReleaseSize = size;
    }

    public int getDefaultPartialReleaseSizeUnit() {
	return defaultPartialReleaseSizeUnit;
    }
    public void setDefaultPartialReleaseSizeUnit(int unit) {
	this.defaultPartialReleaseSizeUnit = unit;
    }

    public int getDefaultMaxPartialReleaseSize() {
	return defaultMaxPartialReleaseSize;
    }
    public void setDefaultMaxPartialReleaseSize(int size) {
	this.defaultMaxPartialReleaseSize = size;
    }

    public int getDefaultMaxPartialReleaseSizeUnit() {
	return defaultMaxPartialReleaseSizeUnit;
    }
    public void setDefaultMaxPartialReleaseSizeUnit(int unit) {
	this.defaultMaxPartialReleaseSizeUnit = unit;
    }

    public long getPartialStageSize() {	return partialStageSize; }
    public void setPartialStageSize(long size) {
	this.partialStageSize = size;
    }

    public int getPartialStageSizeUnit() { return partialStageSizeUnit; }
    public void setPartialStageSizeUnit(int unit) {
	this.partialStageSizeUnit = unit;
    }

    public int getNoOfStageRetries() { return noOfStageRetries; }
    public void setNoOfStageRetries(int number) {
	this.noOfStageRetries = number;
    }

    public long getStageWindowSize() { return stageWindowSize; }
    public void setStageWindowSize(long size) {
	this.stageWindowSize = size;
    }

    public int getStageWindowSizeUnit() { return stageWindowSizeUnit; }
    public void setStageWindowSizeUnit(int unit) {
	this.stageWindowSizeUnit = unit;
    }

    public boolean isArchiverAutoRun() { return archiverAutoRun; }
    public void setArchiverAutoRun(boolean auto) {
	this.archiverAutoRun = auto;
    }

    public boolean isArchive() { return archive; }
    public void setArchive(boolean archive) { this.archive = archive; }


    // general file system mount properties

    public boolean isReadOnlyMount() {	return readOnlyMount; }
    public void setReadOnlyMount(boolean readOnly) {
        this.readOnlyMount = readOnly;
    }

    public boolean isNoSetUID() { return noSetUID; }
    public void setNoSetUID(boolean noSetUID) {
	this.noSetUID = noSetUID;
    }

    public boolean isQuickWrite() { return quickWrite; }
    public void setQuickWrite(boolean quickWrite) {
	this.quickWrite = quickWrite;
    }


    // performance tuning mount options

    public long getReadAhead() { return readAhead; }
    public void setReadAhead(long readAhead) {
	this.readAhead = readAhead;
    }

    public int getReadAheadUnit() { return readAheadUnit; }
    public void setReadAheadUnit(int unit) {
	this.readAheadUnit = unit;
    }

    public long getWriteBehind() { return writeBehind; }
    public void setWriteBehind(long writeBehind) {
	this.writeBehind = writeBehind;
    }

    public int getWriteBehindUnit() { return writeBehindUnit; }
    public void setWriteBehindUnit(int unit) {
        this.writeBehindUnit = unit;
    }

    public long getWriteThrottle() { return writeThrottle; }
    public void setWriteThrottle(long writeThrottle) {
	this.writeThrottle = writeThrottle;
    }


    public int getWriteThrottleUnit() {	return writeThrottleUnit; }
    public void setWriteThrottleUnit(int unit) {
	this.writeThrottleUnit = unit;
    }

    public int getFlushBehind() { return flushBehind; }
    public void setFlushBehind(int flushBehind) {
	this.flushBehind = flushBehind;
    }

    public int getFlushBehindUnit() { return flushBehindUnit; }
    public void setFlushBehindUnit(int unit) {
        this.flushBehindUnit = unit;
    }

    public int getStageFlushBehind() { return stageFlushBehind; }
    public void setStageFlushBehind(int flushBehind) {
	this.stageFlushBehind = flushBehind;
    }

    public int getStageFlushBehindUnit() { return stageFlushBehindUnit; }
    public void setStageFlushBehindUnit(int unit) {
	this.stageFlushBehindUnit = unit;
    }

    public boolean isSynchronizedMetadata() { return synchronizedMetadata; }
    public void setSynchronizedMetadata(boolean sync) {
	this.synchronizedMetadata = sync;
    }

    public int getMetadataStripeWidth() { return metadataStripeWidth; }
    public void setMetadataStripeWidth(int width) {
	this.metadataStripeWidth = width;
    }

    public boolean isSoftRAID() { return softRAID; }
    public void setSoftRAID(boolean softRAID) {
	this.softRAID = softRAID;
    }

    public boolean isForceDirectIO() { return forceDirectIO; }
    public void setForceDirectIO(boolean forceDirectIO) {
	this.forceDirectIO = forceDirectIO;
    }

    public boolean isForceNFSAsync() { return forceNFSAsync; }
    public void setForceNFSAsync(boolean forceNFSAsync) {
	this.forceNFSAsync = forceNFSAsync;
    }


    // direct IO discovery mount properties

    public int getConsecutiveReads() { return consecutiveReads; }
    public void setConsecutiveReads(int number) {
	this.consecutiveReads = number;
    }

    public int getWellAlignedReadMin() { return wellAlignedReadMin; }
    public void setWellAlignedReadMin(int readMin) {
	this.wellAlignedReadMin = readMin;
    }

    public int getWellAlignedReadMinUnit() { return wellAlignedReadMinUnit; }
    public void setWellAlignedReadMinUnit(int unit) {
	this.wellAlignedReadMinUnit = unit;
    }

    public int getMisAlignedReadMin() { return misAlignedReadMin; }
    public void setMisAlignedReadMin(int readMin) {
	this.misAlignedReadMin = readMin;
    }

    public int getMisAlignedReadMinUnit() { return misAlignedReadMinUnit; }
    public void setMisAlignedReadMinUnit(int unit)  {
	this.misAlignedReadMinUnit = unit;
    }

    public int getConsecutiveWrites() { return consecutiveWrites; }
    public void setConsecutiveWrites(int number)  {
	this.consecutiveWrites = number;
    }

    public int getWellAlignedWriteMin() { return wellAlignedWriteMin; }
    public void setWellAlignedWriteMin(int writeMin) {
	this.wellAlignedWriteMin = writeMin;
    }

    public int getWellAlignedWriteMinUnit() { return wellAlignedWriteMinUnit; }
    public void setWellAlignedWriteMinUnit(int unit) {
	this.wellAlignedWriteMinUnit = unit;
    }

    public int getMisAlignedWriteMin() { return misAlignedWriteMin; }
    public void setMisAlignedWriteMin(int writeMin) {
	this.misAlignedWriteMin = writeMin;
    }

    public int getMisAlignedWriteMinUnit() { return misAlignedWriteMinUnit; }
    public void setMisAlignedWriteMinUnit(int unit) {
        this.misAlignedWriteMinUnit = unit;
    }

    public boolean isDirectIOZeroing() { return directIOZeroing; }
    public void setDirectIOZeroing(boolean directIOZeroing) {
	this.directIOZeroing = directIOZeroing;
    }

    public void optimizeForOracle(boolean cluster) {
        // if fs is shared
        if (fs.getShareStatus() != FileSystem.UNSHARED) {
            setMultiHostWrite(true);
            setQuickWrite(true);
            if (cluster) {
                setStripeWidth(1);
                setSynchronizedMetadata(true);
                setForceDirectIO(true);
                setMaxConcurrentStreams(1024);
            }
        }
    }

    // NOTE: no-op implementation to quiet the compiler
    public String getUnsupportedMountOptions() {
        return "";
    }

    // OSD mount options
    public long getObjectDepth() { return objectDepth; }
    public void setObjectDepth(long depth) { objectDepth = depth; }

    public int getObjectWidth() { return objectWidth; }
    public void setObjectWidth(int width) { objectWidth = width; }

    public String toString() {

        StringBuffer buf = new StringBuffer();

        // add file system
        if (fs != null) {
            try {
                buf.append("Mount properties for Filesystem " + fs.getName());
                buf.append("\n\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        buf.append("Basic mount properties\n");
        buf.append("----------------------\n");
        buf.append("HWM: " + hwm + "\n");
	buf.append("LWM: " + lwm + "\n");
	buf.append("Stripe Width: " + stripeWidth + "\n");
        buf.append("Trace On: " + traceOn + "\n\n");

	buf.append("shared QFS mount properties\n");
        buf.append("---------------------------\n");
	buf.append("Mount In Background: " + mountInBackground + "\n");
        buf.append("No Of Mount Retries: " + noOfMountRetries + "\n");
	buf.append("Metadata Refresh Rate: " + metadataRefreshRate + "\n");
	buf.append("Minimum Block Allocation: " + minBlockAllocation + "\n");
        buf.append("Maximum Block Allocation: " + maxBlockAllocation + "\n");
	buf.append("Read Lease Duration: " + readLeaseDuration + "\n");
	buf.append("Write Lease Duration: " + writeLeaseDuration + "\n");
        buf.append("Append Lease Duration: " + appendLeaseDuration + "\n");
	buf.append("Maximum Concurrent Streams: " + maxConcurrentStreams +
                   "\n");
	buf.append("MultiHost Write: " + multiHostWrite + "\n\n");


        buf.append("SAM mount properties\n");
	buf.append("--------------------\n");
	buf.append("Default Partial Release Size: " +
                   defaultPartialReleaseSize + "\n");
	buf.append("Default Maximum Partial Release Size: " +
                   defaultMaxPartialReleaseSize + "\n");
        buf.append("Partial Stage Size: " + partialStageSize + "\n");
	buf.append("No Of Stage Retries: " + noOfStageRetries + "\n");
	buf.append("Stage Window Size: " + stageWindowSize + "\n");
	buf.append("Archiver AutoRun: " + archiverAutoRun + "\n\n");

	buf.append("General file system mount properties\n");
        buf.append("------------------------------------\n");
	buf.append("ReadOnly Mount: " + readOnlyMount + "\n");
	buf.append("NoSetUID: " + noSetUID + "\n");
	buf.append("QuickWrite: " + quickWrite + "\n\n");

        buf.append("Performance tuning mount options\n");
	buf.append("--------------------------------\n");
	buf.append("Read Ahead: " + readAhead + "\n");
        buf.append("Write Behind: " + writeBehind + "\n");
        buf.append("Write BehindUnit: " + writeBehindUnit + "\n");
	buf.append("Write Throttle: " + writeThrottle + "\n");
	buf.append("Flush Behind: " + flushBehind + "\n");
        buf.append("Stage Flush Behind: " + stageFlushBehind + "\n");
	buf.append("Synchronized Metadata: " + synchronizedMetadata + "\n");
	buf.append("Metadata Stripe Width: " + metadataStripeWidth + "\n");
        buf.append("Soft RAID: " + softRAID + "\n");
	buf.append("Force Direct IO: " + forceDirectIO + "\n\n");

	buf.append("Direct IO discovery mount properties\n");
	buf.append("------------------------------------\n");
	buf.append("Consecutive Reads: " + consecutiveReads + "\n");
        buf.append("Well Aligned Read Min: " + wellAlignedReadMin + "\n");
	buf.append("Mis-aligned Read Min: " + misAlignedReadMin + "\n");
	buf.append("Consecutive Writes: " + consecutiveWrites + "\n");
	buf.append("Well Aligned Write Min: " + wellAlignedWriteMin + "\n");
	buf.append("Mis-aligned Write Min: " + misAlignedWriteMin + "\n\n");

	return buf.toString();
    }
}
