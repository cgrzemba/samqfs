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

// ident	$Id: FileSystemMountPropertiesImpl.java,v 1.20 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.fs;


import com.sun.netstorage.samqfs.mgmt.fs.MountOptions;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountPropsBase;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;


public class FileSystemMountPropertiesImpl extends FileSystemMountPropsBase
    implements FileSystemMountProperties {

    protected MountOptions opt = null;

    // static ints for variables with units

    private static final int UNIT_DEF_PAR_REL = 1;
    private static final int UNIT_DEF_MAX_PAR_REL = 2;
    private static final int UNIT_PAR_STG = 3;
    private static final int UNIT_STG_WIN = 4;

    private static final int UNIT_RD_AHD = 5;
    private static final int UNIT_WR_BHD = 6;
    private static final int UNIT_WR_THR = 7;
    private static final int UNIT_FL_BHD = 8;
    private static final int UNIT_STG_FL_BHD = 9;

    private static final int UNIT_WALGN_RD_MIN = 10;
    private static final int UNIT_MALGN_RD_MIN = 11;
    private static final int UNIT_WALGN_WR_MIN = 12;
    private static final int UNIT_MALGN_WR_MIN = 13;

    private static final int UNIT_MIN_BLK_ALLOC = 14;
    private static final int UNIT_MAX_BLK_ALLOC = 15;


    public FileSystemMountPropertiesImpl()  {
        opt = new MountOptions(
                (short) DEFAULT_HWM, (short) DEFAULT_LWM, (short) 1, false);
        setup();
    }

    public FileSystemMountPropertiesImpl(MountOptions opt) {
        if (opt != null) {
        this.opt = opt;
            setup();
        }
    }



    public MountOptions getJniMountOptions() { return opt; }
    public void setJniMountOptions(MountOptions opt) {
        if (opt != null) {
            this.opt = opt;
            setup();
        }
    }


    // setters for basic mount props

    public void setHWM(int hwm)  {
        this.hwm = hwm;
        if (opt != null)
            if (hwm == -1)
                opt.resetHWM();
            else
                opt.setHWM((short) hwm);
    }

    public void setLWM(int lwm)  {
        this.lwm = lwm;
        if (opt != null)
            if (lwm == -1)
                opt.resetLWM();
            else
                opt.setLWM((short) lwm);
    }

    public void setStripeWidth(int width)  {
	this.stripeWidth = width;
        if (opt != null)
            if (width == -1)
                opt.resetStripeWidth();
            else
                opt.setStripeWidth((short) width);
    }

    public void setTrace(boolean traceOn)  {
	this.traceOn = traceOn;
        if (opt != null)
            opt.setTrace(traceOn);
    }


    // setters for shared QFS mount props

    public void setMountInBackground(boolean mountInBackground) {
	this.mountInBackground = mountInBackground;
        if (opt != null)
            opt.setMountInBackground(mountInBackground);
    }

    public void setNoOfMountRetries(int noOfRetries)  {
	this.noOfMountRetries = noOfRetries;
        if (opt != null)
            if (noOfRetries == -1)
                opt.resetNoOfMountRetries();
            else
                opt.setNoOfMountRetries((short) noOfRetries);
    }

    public void setMetadataRefreshRate(int rate)  {
	this.metadataRefreshRate = rate;
        if (opt != null)
            if (rate == -1)
                // Andrei typo
                opt.resetMedataRefreshRate();
            else
                opt.setMetadataRefreshRate(rate);
    }

    public void setMinBlockAllocation(long noOfBlocks)  {
	this.minBlockAllocation = noOfBlocks;
        if (opt != null) {
            if (noOfBlocks != -1) {
                if (minBlockAllocationUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(noOfBlocks,
                        minBlockAllocationUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMinBlockAllocation(tmpSize);
                }
            } else {
                opt.resetMinBlockAllocation();
            }
        }
    }

    public void setMinBlockAllocationUnit(int unit) {
        this.minBlockAllocationUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (minBlockAllocation != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        minBlockAllocation, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMinBlockAllocation(tmpSize);
                }
            } else {
                opt.resetMinBlockAllocation();
            }
        }
    }

    public void setMaxBlockAllocation(long noOfBlocks) {
	this.maxBlockAllocation = noOfBlocks;
        if (opt != null) {
            if (noOfBlocks != -1) {
                if (maxBlockAllocationUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(noOfBlocks,
                        maxBlockAllocationUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMaxBlockAllocation(tmpSize);
                }
            } else {
                opt.resetMaxBlockAllocation();
            }
        }
    }

    public void setMaxBlockAllocationUnit(int unit) {
	this.maxBlockAllocationUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (maxBlockAllocation != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        maxBlockAllocation, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMaxBlockAllocation(tmpSize);
                }
            } else {
                opt.resetMaxBlockAllocation();
            }
        }
    }

    public void setReadLeaseDuration(int duration) {
	this.readLeaseDuration = duration;
        if (opt != null) {
            if (duration == -1)
                opt.resetReadLeaseDuration();
            else
                opt.setReadLeaseDuration(duration);
        }
    }

    public void setWriteLeaseDuration(int duration) {
	this.writeLeaseDuration = duration;
        if (opt != null) {
            if (duration == -1)
                opt.resetWriteLeaseDuration();
            else
                opt.setWriteLeaseDuration(duration);
        }
    }

    public void setAppendLeaseDuration(int duration) {
	this.appendLeaseDuration = duration;
        if (opt != null) {
            if (duration == -1)
                opt.resetAppendLeaseDuration();
            else
                opt.setAppendLeaseDuration(duration);
        }
    }

    public void setMaxConcurrentStreams(int noOfStreams) {
	this.maxConcurrentStreams = noOfStreams;
        if (opt != null) {
            if (noOfStreams == -1)
                opt.resetMaxConcurrentStreams();
            else
                opt.setMaxConcurrentStreams(noOfStreams);
        }
    }

    public void setMinPool(int minPool) {
	this.minPool = minPool;
        if (opt != null) {
            if (minPool == -1)
                opt.resetMinPool();
            else
                opt.setMinPool(minPool);
        }
    }

    public void setLeaseTimeout(int leaseTimeout) {
	this.leaseTimeout = leaseTimeout;
        if (opt != null) {
            if (leaseTimeout == -1)
                opt.resetLeaseTimeout();
            else
                opt.setLeaseTimeout(leaseTimeout);
        }
    }

    public void setMultiHostWrite(boolean write) {
        this.multiHostWrite = write;
        if (opt != null)
            opt.setMultiHostWrite(write);
    }

    public void setConsistencyChecking(boolean consistencyChecking) {
	this.consistencyChecking = consistencyChecking;
        if (opt != null)
            opt.setConsistencyChecking(consistencyChecking);
    }


    // setters for SAM mount props

    public void setDefaultPartialReleaseSize(int size) {
	this.defaultPartialReleaseSize = size;
        if (opt != null) {
            if (size != -1) {
                if (defaultPartialReleaseSizeUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(size,
                        defaultPartialReleaseSizeUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setDefaultPartialReleaseSize((int) tmpSize);
                }
            } else {
                opt.resetDefaultPartialReleaseSize();
            }
        }
    }

    public void setDefaultPartialReleaseSizeUnit(int unit) {
	this.defaultPartialReleaseSizeUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (defaultPartialReleaseSize != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        defaultPartialReleaseSize, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setDefaultPartialReleaseSize((int) tmpSize);
                }
            } else {
                opt.resetDefaultPartialReleaseSize();
            }
        }
    }

    public void setDefaultMaxPartialReleaseSize(int size) {
	this.defaultMaxPartialReleaseSize = size;
        if (opt != null) {
            if (size != -1) {
                if (defaultMaxPartialReleaseSizeUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(size,
                        defaultMaxPartialReleaseSizeUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setDefaultMaxPartialReleaseSize((int) tmpSize);
                }
            } else {
                opt.resetDefaultMaxPartialReleaseSize();
            }
        }
    }

    public void setDefaultMaxPartialReleaseSizeUnit(int unit) {
	this.defaultMaxPartialReleaseSizeUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (defaultMaxPartialReleaseSize != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        defaultMaxPartialReleaseSize, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setDefaultMaxPartialReleaseSize((int) tmpSize);
                }
            } else {
                opt.resetDefaultMaxPartialReleaseSize();
            }
        }
    }

    public void setPartialStageSize(long size) {
	this.partialStageSize = size;
        if (opt != null) {
            if (size != -1) {
                if (partialStageSizeUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(size,
                        partialStageSizeUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setPartialStageSize(tmpSize);
                }
            } else {
                opt.resetPartialStageSize();
            }
        }
    }

    public void setPartialStageSizeUnit(int unit) {
	this.partialStageSizeUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (partialStageSize != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        partialStageSize, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setPartialStageSize(tmpSize);
                }
            } else {
                opt.resetPartialStageSize();
            }
        }
    }

    public void setNoOfStageRetries(int number) {
	this.noOfStageRetries = number;
        if (opt != null) {
            if (number == -1)
                opt.resetNoOfStageRetries();
            else
                opt.setNoOfStageRetries(number);
        }
    }

    public void setStageWindowSize(long size) {
	this.stageWindowSize = size;
        if (opt != null) {
            if (size != -1) {
                if (stageWindowSizeUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(size,
                        stageWindowSizeUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setStageWindowSize(tmpSize);
                }
            } else {
                opt.resetStageWindowSize();
            }
        }
    }

    public void setStageWindowSizeUnit(int unit) {
	this.stageWindowSizeUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (stageWindowSize != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        stageWindowSize, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setStageWindowSize(tmpSize);
                }
            } else {
                opt.resetStageWindowSize();
            }
        }
    }

    public void setArchiverAutoRun(boolean auto) {
	this.archiverAutoRun = auto;
        if (opt != null)
            opt.setArchiverAutoRun(auto);
    }

    public void setArchive(boolean archive) {
	this.archive = archive;
        if (opt != null)
            opt.setArchive(archive);
    }


    // setters for general file system mount props

    public void setReadOnlyMount(boolean readOnly) {
	this.readOnlyMount = readOnly;
        if (opt != null)
            opt.setReadOnlyMount(readOnly);
    }
    public void setNoSetUID(boolean noSetUID) {
	this.noSetUID = noSetUID;
        if (opt != null)
            opt.setNoSetUID(noSetUID);
    }
    public void setQuickWrite(boolean quickWrite) {
	this.quickWrite = quickWrite;
        if (opt != null)
            opt.setQuickWrite(quickWrite);
    }

    // setters for performance tuning mount props

    public void setReadAhead(long readAhead) {
	this.readAhead = readAhead;
        if (opt != null) {
            if (readAhead != -1) {
                if (readAheadUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(readAhead,
                        readAheadUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setReadAhead(tmpSize);
                }
            } else {
                opt.resetReadAhead();
            }
        }
    }

    public void setReadAheadUnit(int unit) {
	this.readAheadUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (readAhead != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        readAhead, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setReadAhead(tmpSize);
                }
            } else {
                opt.resetReadAhead();
            }
        }
    }

    public void setWriteBehind(long writeBehind) {
	this.writeBehind = writeBehind;
        if (opt != null) {
            if (writeBehind != -1) {
                if (writeBehindUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(writeBehind,
                        writeBehindUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWriteBehind(tmpSize);
                }
            } else {
                opt.resetWriteBehind();
            }
        }
    }

    public void setWriteBehindUnit(int unit) {
        this.writeBehindUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (writeBehind != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        writeBehind, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWriteBehind(tmpSize);
                }
            } else {
                opt.resetWriteBehind();
            }
        }
    }

    public void setWriteThrottle(long writeThrottle) {
	this.writeThrottle = writeThrottle;
        if (opt != null) {
            if (writeThrottle != -1) {
                if (writeThrottleUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(writeThrottle,
                        writeThrottleUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWriteThrottle(tmpSize);
                }
            } else {
                opt.resetWriteThrottle();
            }
        }
    }

    public void setWriteThrottleUnit(int unit) {
	this.writeThrottleUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (writeThrottle != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        writeThrottle, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWriteThrottle(tmpSize);
                }
            } else {
                opt.resetWriteThrottle();
            }
        }
    }

    public void setFlushBehind(int flushBehind) {
	this.flushBehind = flushBehind;
        if (opt != null) {
            if (flushBehind != -1) {
                if (flushBehindUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(flushBehind,
                        flushBehindUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setFlushBehind((int) tmpSize);
                }
            } else {
                opt.resetFlushBehind();
            }
        }
    }

    public void setFlushBehindUnit(int unit) {
        this.flushBehindUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (flushBehind != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        flushBehind, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setFlushBehind((int) tmpSize);
                }
            } else {
                opt.resetFlushBehind();
            }
        }
    }

    public void setStageFlushBehind(int flushBehind) {
	this.stageFlushBehind = flushBehind;
        if (opt != null) {
            if (stageFlushBehind != -1) {
                if (stageFlushBehindUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(stageFlushBehind,
                        stageFlushBehindUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setStageFlushBehind((int) tmpSize);
                }
            } else {
                opt.resetStageFlushBehind();
            }
        }
    }

    public void setStageFlushBehindUnit(int unit) {
	this.stageFlushBehindUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (stageFlushBehind != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        stageFlushBehind, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setStageFlushBehind((int) tmpSize);
                }
            } else {
                opt.resetStageFlushBehind();
            }
        }
    }

    public void setSynchronizedMetadata(boolean sync) {
	this.synchronizedMetadata = sync;
        short val = 0;
        if (sync)
            val = 1;
        if (opt != null)
            opt.setSynchronizedMetadata(val);
    }

    public void setMetadataStripeWidth(int width) {
	this.metadataStripeWidth = width;
        if (opt != null) {
            if (width == -1)
                opt.resetMetadataStripeWidth();
            else
                opt.setMetadataStripeWidth(width);
        }
    }

    public void setSoftRAID(boolean softRAID) {
	this.softRAID = softRAID;
        if (opt != null)
            opt.setSoftRAID(softRAID);
    }

    public void setForceDirectIO(boolean forceDirectIO) {
	this.forceDirectIO = forceDirectIO;
        if (opt != null)
            opt.setForceDirectIO(forceDirectIO);
    }

    public void setForceNFSAsync(boolean forceNFSAsync) {
	this.forceNFSAsync = forceNFSAsync;
        if (opt != null)
            opt.setForceNFSAsync(forceNFSAsync);
    }


    // setters for direct IO discovery mount props

    public void setConsecutiveReads(int number) {
	this.consecutiveReads = number;
        if (opt != null) {
            if (number == -1)
                opt.resetConsecutiveReads();
            else
                opt.setConsecutiveReads(number);
        }
    }

    public void setWellAlignedReadMin(int readMin) {
	this.wellAlignedReadMin = readMin;
        if (opt != null) {
            if (readMin != -1) {
                if (wellAlignedReadMinUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(readMin,
                        wellAlignedReadMinUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWellAlignedReadMin((int) tmpSize);
                }
            } else {
                opt.resetWellAlignedReadMin();
            }
        }
    }

    public void setWellAlignedReadMinUnit(int unit) {
	this.wellAlignedReadMinUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (wellAlignedReadMin != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        wellAlignedReadMin, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWellAlignedReadMin((int) tmpSize);
                }
            } else {
                opt.resetWellAlignedReadMin();
            }
        }
    }

    public void setMisAlignedReadMin(int readMin) {
	this.misAlignedReadMin = readMin;
        if (opt != null) {
            if (readMin != -1) {
                if (misAlignedReadMinUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(readMin,
                        misAlignedReadMinUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMisAlignedReadMin((int) tmpSize);
                }
            } else {
                opt.resetMisAlignedReadMin();
            }
        }
    }

    public void setMisAlignedReadMinUnit(int unit)  {
	this.misAlignedReadMinUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (misAlignedReadMin != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        misAlignedReadMin, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMisAlignedReadMin((int) tmpSize);
                }
            } else {
                opt.resetMisAlignedReadMin();
            }
        }
    }

    public void setConsecutiveWrites(int number)  {
	this.consecutiveWrites = number;
        if (opt != null) {
            if (number == -1)
                opt.resetConsecutiveWrites();
            else
                opt.setConsecutiveWrites(number);
        }
    }

    public void setWellAlignedWriteMin(int writeMin) {
	this.wellAlignedWriteMin = writeMin;
        if (opt != null) {
            if (writeMin != -1) {
                if (wellAlignedWriteMinUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(writeMin,
                        wellAlignedWriteMinUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWellAlignedWriteMin((int) tmpSize);
                }
            } else {
                opt.resetWellAlignedWriteMin();
            }
        }
    }

    public void setWellAlignedWriteMinUnit(int unit) {
	this.wellAlignedWriteMinUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (wellAlignedWriteMin != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        wellAlignedWriteMin, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setWellAlignedWriteMin((int) tmpSize);
                }
            } else {
                opt.resetWellAlignedWriteMin();
            }
        }
    }

    public void setMisAlignedWriteMin(int writeMin) {
	this.misAlignedWriteMin = writeMin;
        if (opt != null) {
            if (writeMin != -1) {
                if (misAlignedWriteMinUnit != -1) {
                    long tmpSize = SamQFSUtil.convertSize(writeMin,
                        misAlignedWriteMinUnit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMisAlignedWriteMin((int) tmpSize);
                }
            } else {
                opt.resetMisAlignedWriteMin();
            }
        }
    }

    public void setMisAlignedWriteMinUnit(int unit) {
        this.misAlignedWriteMinUnit = unit;
        if (opt != null) {
            if (unit != -1) {
                if (misAlignedWriteMin != -1) {
                    long tmpSize = SamQFSUtil.convertSize(
                        misAlignedWriteMin, unit,
                        SamQFSSystemModel.SIZE_KB);
                    if (tmpSize != -1)
                        opt.setMisAlignedWriteMin((int) tmpSize);
                }
            } else {
                opt.resetMisAlignedWriteMin();
            }
        }
    }

    public void setDirectIOZeroing(boolean directIOZeroing) {
	this.directIOZeroing = directIOZeroing;
        if (opt != null)
            opt.setDirectIOZeroing(directIOZeroing);
    }



    private void setup() {

        String byteVal = null;

        // basic mount properties
        hwm = opt.getHWM();
        lwm = opt.getLWM();
        stripeWidth = opt.getStripeWidth();
        traceOn = opt.isTrace();

        // shared QFS mount properties
        mountInBackground = opt.isMountInBackground();
        noOfMountRetries = opt.getNoOfMountRetries();
        metadataRefreshRate = opt.getMetadataRefreshRate();


        minBlockAllocation = opt.getMinBlockAllocation();
        minBlockAllocationUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_MIN_BLK_ALLOC, minBlockAllocation);

        maxBlockAllocation = opt.getMaxBlockAllocation();
        maxBlockAllocationUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_MAX_BLK_ALLOC, maxBlockAllocation);

        readLeaseDuration = opt.getReadLeaseDuration();
        writeLeaseDuration = opt.getWriteLeaseDuration();
        appendLeaseDuration = opt.getAppendLeaseDuration();
        maxConcurrentStreams = opt.getMaxConcurrentStreams();
        minPool = opt.getMinPool();
        leaseTimeout = opt.getLeaseTimeout();
        multiHostWrite = opt.isMultiHostWrite();
        consistencyChecking = opt.isConsistencyChecking();

        // SAM mount properties
        defaultPartialReleaseSize = opt.getDefaultPartialReleaseSize();
        defaultPartialReleaseSizeUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_DEF_PAR_REL, defaultPartialReleaseSize);

        defaultMaxPartialReleaseSize =
            opt.getDefaultMaxPartialReleaseSize();
        defaultMaxPartialReleaseSizeUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_DEF_MAX_PAR_REL, defaultMaxPartialReleaseSize);

        partialStageSize = opt.getPartialStageSize();
        partialStageSizeUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_PAR_STG, partialStageSize);

        noOfStageRetries = opt.getNoOfStageRetries();

        stageWindowSize = opt.getStageWindowSize();
        stageWindowSizeUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_STG_WIN, stageWindowSize);

        archiverAutoRun = opt.isArchiverAutoRun();

        // general file system mount properties
        readOnlyMount = opt.isReadOnlyMount();
        noSetUID = opt.isNoSetUID();
        quickWrite = opt.isQuickWrite();

        // performance tuning mount options
        readAhead = opt.getReadAhead();
        readAheadUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnitLong(UNIT_RD_AHD, readAhead);

        writeBehind = opt.getWriteBehind();
        writeBehindUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnitLong(UNIT_WR_BHD, writeBehind);

        writeThrottle = opt.getWriteThrottle();
        writeThrottleUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnitLong(UNIT_WR_THR, writeThrottle);

        flushBehind = opt.getFlushBehind();
        flushBehindUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_FL_BHD, flushBehind);

        stageFlushBehind = opt.getStageFlushBehind();
        stageFlushBehindUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_STG_FL_BHD, stageFlushBehind);

        short val = opt.isSynchronizedMetadata();
        if (val == 1)
            synchronizedMetadata = true;

        metadataStripeWidth = opt.getMetadataStripeWidth();

        softRAID = opt.isSoftRAID();

        forceDirectIO = opt.isForceDirectIO();

        forceNFSAsync = opt.isForceNFSAsync();

        directIOZeroing = opt.isDirectIOZeroing();

        // direct IO discovery mount properties
        consecutiveReads = opt.getConsecutiveReads();

        wellAlignedReadMin = opt.getWellAlignedReadMin();
        wellAlignedReadMinUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_WALGN_RD_MIN, wellAlignedReadMin);

        misAlignedReadMin = opt.getMisAlignedReadMin();
        misAlignedReadMinUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_MALGN_RD_MIN, misAlignedReadMin);

        consecutiveWrites = opt.getConsecutiveWrites();

        wellAlignedWriteMin = opt.getWellAlignedWriteMin();
        wellAlignedWriteMinUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_WALGN_WR_MIN, wellAlignedWriteMin);

        misAlignedWriteMin = opt.getMisAlignedWriteMin();
        misAlignedWriteMinUnit = SamQFSSystemModel.SIZE_KB;
        setUpUnit(UNIT_MALGN_WR_MIN, misAlignedWriteMin);

    }


    private String getBytesFromKB(long kb) {

        long val = kb * 1024;
        String retVal = null;

        if (val >= 0) {
            try {
                retVal = (new Long(val)).toString();
            } catch (NumberFormatException nfe) {}
        }

        return retVal;

    }


    private void setUpUnit(int field, long fieldVal) {

        if (fieldVal > 0) {

            String byteVal = getBytesFromKB(fieldVal);
            if (SamQFSUtil.isValidString(byteVal)) {

                String size = SamQFSUtil.stringToFsize(byteVal);

                switch (field) {

                case UNIT_MIN_BLK_ALLOC:
                    minBlockAllocation = SamQFSUtil.getLongVal(size);
                    minBlockAllocationUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_MAX_BLK_ALLOC:
                    maxBlockAllocation = SamQFSUtil.getLongVal(size);
                    maxBlockAllocationUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_DEF_PAR_REL:
                    defaultPartialReleaseSize = SamQFSUtil.getIntegerVal(size);
                    defaultPartialReleaseSizeUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_DEF_MAX_PAR_REL:
                     defaultMaxPartialReleaseSize =
                         SamQFSUtil.getIntegerVal(size);
                     defaultMaxPartialReleaseSizeUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_PAR_STG:
                    partialStageSize = SamQFSUtil.getIntegerVal(size);
                    partialStageSizeUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_STG_WIN:
                    stageWindowSize = SamQFSUtil.getIntegerVal(size);
                    stageWindowSizeUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_FL_BHD:
                    flushBehind = SamQFSUtil.getIntegerVal(size);
                    flushBehindUnit = SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_STG_FL_BHD:
                    stageFlushBehind = SamQFSUtil.getIntegerVal(size);
                    stageFlushBehindUnit = SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_WALGN_RD_MIN:
                    wellAlignedReadMin = SamQFSUtil.getIntegerVal(size);
                    wellAlignedReadMinUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_MALGN_RD_MIN:
                    misAlignedReadMin = SamQFSUtil.getIntegerVal(size);
                    misAlignedReadMinUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_WALGN_WR_MIN:
                    wellAlignedWriteMin = SamQFSUtil.getIntegerVal(size);
                    wellAlignedWriteMinUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_MALGN_WR_MIN:
                    misAlignedWriteMin = SamQFSUtil.getIntegerVal(size);
                    misAlignedWriteMinUnit =
                        SamQFSUtil.getSizeUnitInteger(size);
                    break;

                }

            }

        }

    }


    private void setUpUnitLong(int field, long fieldVal) {

        if (fieldVal > 0) {

            String byteVal = getBytesFromKB(fieldVal);
            if (SamQFSUtil.isValidString(byteVal)) {

                String size = SamQFSUtil.stringToFsize(byteVal);

                switch (field) {

                case UNIT_RD_AHD:
                    readAhead = SamQFSUtil.getLongVal(size);
                    readAheadUnit = SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_WR_BHD:
                    writeBehind = SamQFSUtil.getLongVal(size);
                    writeBehindUnit = SamQFSUtil.getSizeUnitInteger(size);
                    break;

                case UNIT_WR_THR:
                    writeThrottle = SamQFSUtil.getLongVal(size);
                    writeThrottleUnit = SamQFSUtil.getSizeUnitInteger(size);
                    break;
                }
            }
        }
    }

    public String getUnsupportedMountOptions() {
        return opt.getUnsupportedMountOptions();
    }
}
