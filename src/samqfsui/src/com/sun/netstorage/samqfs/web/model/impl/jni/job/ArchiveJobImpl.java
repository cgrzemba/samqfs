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

// ident	$Id: ArchiveJobImpl.java,v 1.12 2008/03/17 14:43:49 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.job.*;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.media.VSN;

public class ArchiveJobImpl extends BaseJobImpl implements ArchiveJob {

    private ArCopyJob jniJob = null;

    private FileSystem fs = null;
    private String fsName = null;
    private String fsMountPoint = null;

    private ArchiveCopy copy = null;
    private String policyName = null;
    private int copyNumber = -1;

    private VSN vsn = null;
    private String vsnName = null;
    private int mediaType = -1;
    private boolean loaded = false;

    private VSN stageVSN = null;
    private String stageVSNName = null;
    private int stageMediaType = -1;
    private boolean stageLoaded = false;

    private int filesCopied = 0;
    private long dataVolume = 0; // in kbytes
    private String curFileName = null;
    private int filesToBeCopied = 0;
    private long volumeToBeCopied = 0; // in kbytes
    private int archStatus = -1;

    public ArchiveJobImpl() {
    }

    public ArchiveJobImpl(BaseJob base,
                          FileSystem fs,
                          ArchiveCopy copy,
                          VSN vsn,
                          boolean loaded,
                          VSN stageVSN,
                          boolean stageLoaded,
                          int filesCopied,
                          long dataVolume,
                          String curFileName,
                          int filesToBeCopied,
                          long volumeToBeCopied,
                          int archStatus) throws SamFSException {

        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        this.fs = fs;
        this.copy = copy;
        this.vsn = vsn;
        this.loaded = loaded;
        this.stageVSN = stageVSN;
        this.stageLoaded = stageLoaded;
        this.filesCopied = filesCopied;
        this.dataVolume = dataVolume;
        this.curFileName = curFileName;
        this.filesToBeCopied = filesToBeCopied;
        this.volumeToBeCopied = volumeToBeCopied;
        this.archStatus = archStatus;
    }

    public ArchiveJobImpl(ArCopyJob jniJob) {
        super(jniJob);
        this.jniJob = jniJob;
        update();
    }

    public FileSystem getFileSystem() {
        return fs;
    }

    public void setFileSystem(FileSystem fs) {
        this.fs = fs;
    }

    public String getFileSystemName() {
        if (fs != null)
            fsName = fs.getName();

        return fsName;
    }

    public String getFSMountPoint() throws SamFSException {

        if (fs != null)
            fsMountPoint = fs.getMountPoint();

        return fsMountPoint;

    }

    public ArchiveCopy getCopy() {
        return copy;
    }

    public void setCopy(ArchiveCopy copy) {
        this.copy = copy;
    }

    public String getPolicyName() {

        try {
            if (copy != null)
                policyName = copy.getArchivePolicy().getPolicyName();
        } catch (Exception e) {}

        return policyName;
    }

    public int  getCopyNumber() {
        try {
            if (copy != null)
                copyNumber = copy.getCopyNumber();
        } catch (Exception e) {}

        return copyNumber;
    }

    public VSN getVSN() {
        return vsn;
    }

    public void setVSN(VSN vsn) {
        this.vsn = vsn;
    }

    public String getVSNName() {

        try {
            if (vsn != null)
                vsnName = vsn.getVSN();
        } catch (Exception e) {}

        return vsnName;
    }

    public int getMediaType() {
        try {
            if (vsn != null)
                mediaType = vsn.getLibrary().getMediaType();
        } catch (Exception e) {}

        return mediaType;
    }

    // I think this can be derived from the fact the getDrive() call
    // on VSN should return null in case it is not loaded yet
    public boolean isVSNLoaded() throws SamFSException {
        return loaded;
    }

    public void  setVSNLoaded(boolean loaded) {
        this.loaded = loaded;
    }

    public VSN getStageVSN() {
        return stageVSN;
    }

    public void setStageVSN(VSN stageVSN) throws SamFSException {
        this.stageVSN = stageVSN;
    }

    public String getStageVSNName() {

        try {
            if (stageVSN != null)
                stageVSNName = stageVSN.getVSN();
        } catch (Exception e) {}

        return vsnName;
    }

    public int getStageMediaType() {

        try {
            if (stageVSN != null)
                stageMediaType = vsn.getLibrary().getMediaType();
        } catch (Exception e) {}

        return stageMediaType;
    }

    // I think this can be derived from the fact the getDrive() call
    // on VSN should return null in case it is not loaded yet
    public boolean isStageVSNLoaded() throws SamFSException {
        return stageLoaded;
    }

    public void  setStageVSNLoaded(boolean stageLoaded) {
        this.stageLoaded = stageLoaded;
    }

    public int getTotalNoOfFilesAlreadyCopied() {
        return filesCopied;
    }

    public void setTotalNoOfFilesAlreadyCopied(int filesCopied)
        throws SamFSException {

        this.filesCopied = filesCopied;
    }

    public long getDataVolumeAlreadyCopied() {
        return dataVolume;
    }

    public void setDataVolumeAlreadyCopied(long dataVolume) {
        this.dataVolume = dataVolume;
    }

    public String getCurrentFileName() {
        return curFileName;
    }

    public void setCurrentFileName(String curFileName) {
        this.curFileName = curFileName;
    }

    public int getTotalNoOfFilesToBeCopied() {
        return filesToBeCopied;
    }

    public void setTotalNoOfFilesToBeCopied(int filesToBeCopied) {
        this.filesToBeCopied = filesToBeCopied;
    }

    public long getDataVolumeToBeCopied() {
        return volumeToBeCopied;
    }

    // setter is not currently used
    public void setDataVolumeToBeCopied(long volumeToBeCopied) {
        this.volumeToBeCopied = volumeToBeCopied;
    }

    public int getArchivingStatus() {
        return archStatus;
    }

    public void setArchivingStatus(int archStatus) {
        this.archStatus = archStatus;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("toString() needs to be updated for Archive Copy job.\n");
        buf.append(super.toString());

        try {

            if (fs != null) {
                buf.append("Filesystem: ").append(fs.getName()).append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (copy != null)
            buf.append("Archive Copy: ").append(copy.toString()).append("\n");
        if (vsn != null)
            try {
                buf.append("VSN: ").append(vsn.getVSN()).append("\n");
            } catch (Exception e) {
            }
        buf.append("VSN Loaded: ").append(loaded).append("\n");

        if (stageVSN != null)
            try {
                buf.append("StageVSN: ").append(stageVSN.getVSN()).append("\n");
            } catch (Exception e) {
            }

        buf.append("StageVSN Loaded: ")
            .append(stageLoaded)
            .append("\n")
            .append("Total No Of Files Already Copied: ")
            .append(filesCopied)
            .append("\n")
            .append("DataVolume Already Copied: ")
            .append(dataVolume)
            .append("\n")
            .append("Current FileName: ")
            .append(curFileName)
            .append("\n")
            .append("Total No Of Files To Be Copied: ")
            .append(filesToBeCopied)
            .append("\n")
            .append("Total DataVolume To Be Copied: ")
            .append(volumeToBeCopied)
            .append("\n");

        return buf.toString();
    }

    private void update() {
        if (jniJob != null) {

            // API does not exist for commented out stuff
            super.setstartDateTime(SamQFSUtil.
                                   convertTime(jniJob.getCreationTime()));

            fsName = jniJob.getFSName();
            // fsMountPoint = new String();

            policyName = SamQFSUtil.getCriteriaName(jniJob.getARSetName());
            copyNumber = SamQFSUtil.getCopyNumber(jniJob.getARSetName());

            vsnName = jniJob.getVSN();
            mediaType = SamQFSUtil.getMediaTypeInteger(jniJob.getMediaType());
            dataVolume = jniJob.getBytesWritten(); // in kbytes
            filesToBeCopied = jniJob.getFiles();
            volumeToBeCopied = jniJob.getSpaceNeeded();
        }
    }
}
