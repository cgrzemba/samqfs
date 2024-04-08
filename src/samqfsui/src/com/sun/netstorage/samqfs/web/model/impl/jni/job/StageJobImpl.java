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

// ident	$Id: StageJobImpl.java,v 1.19 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.stg.job.*;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.model.media.VSN;

public class StageJobImpl extends BaseJobImpl implements StageJob {
    private SamQFSSystemModelImpl model = null;
    private StagerJob jniJob = null;
    private FileSystem fs = null;
    private VSN vsn = null;
    private String fsName = null;
    private String vsnName = null;
    private int mediaType = -1;
    private String position = null;
    private String offset = null;
    private String name = null;
    private String size = null;
    private String complete = null;
    private String username = null;
    private StageJobFileData[] fileData = new StageJobFileData[0];
    private int stateFlag = -1;

    public StageJobImpl() {
    }

    public StageJobImpl(BaseJob base,
                        FileSystem fs,
                        VSN vsn,
                        String position,
                        String offset,
                        String name,
                        String size,
                        String complete,
                        String username,
                        StageJobFileData [] fileData) throws SamFSException {

        super(base.getJobId(),
              base.getCondition(),
              base.getType(),
              base.getDescription(),
              base.getStartDateTime(),
              base.getEndDateTime());

        this.fs = fs;
        this.vsn = vsn;
        this.position = position;
        this.offset = offset;
        this.name = name;
        this.size = size;
        this.complete = complete;
        this.username = username;
        this.fileData = fileData;
    }

    public StageJobImpl(SamQFSSystemModelImpl model, StagerJob jniJob)
        throws SamFSException {

        super(jniJob);
        this.model = model;
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
        if (fs != null) {
            fsName = fs.getName();
        }

        return fsName;
    }


    // the VSN name and Tape Drive name in page catalog should be links
    public VSN getVSN() {
        return vsn;
    }

    public void setVSN(VSN vsn) {
        this.vsn = vsn;
    }

    public String getVSNName() {
        // this doesn't harm, can get rid of it
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

    public String getPosition() {
        return position;
    }

    public void setPosition(String position) {
        if (position != null)
            this.position = position;
    }

    public String getOffset() {
        return offset;
    }

    public void setOffset(String offset) {
        if (offset != null)
            this.offset = offset;
    }

    public String getFileName() {
        return name;
    }

    public void setFileName(String name) {
        if (name != null)
            this.name = name;
    }

    public String getFileSize() {
        return size;
    }

    public void setFileSize(String size) {
        if (size != null)
            this.size = size;
    }

    public String getStagedFileSize() {
        return complete;
    }

    public void setStagedFileSize(String complete) {
        if (complete != null)
            this.complete = complete;
    }

    public String getInitiatingUserName() {
        return username;
    }

    public void setInitiatingUserName(String username) {
        if (username != null)
            this.username = username;
    }

    public StageJobFileData[] getFileData() throws SamFSException {
        fileData = new StageJobFileData[0];
        if (model != null) {
            StgFileInfo[] fInfo =
                jniJob.getFilesInfo(model.getJniContext(),
                                    0,
                                    StageJob.ALL_FILES,
                                    (short) StagerJob.ST_NO_SORT, false);
            if ((fInfo != null) && (fInfo.length > 0)) {
                fileData = new StageJobFileData[fInfo.length];
                for (int i = 0; i < fInfo.length; i++)
                    fileData[i] =
                        new StageJobFileDataImpl(fInfo[i].fileName,
                                                 fInfo[i].size,
                                                 fInfo[i].position,
                                                 fInfo[i].offset,
                                                 fInfo[i].vsn,
                                                 fInfo[i].user);
            }
        }
        return fileData;
    }

    public long getNumberOfFiles() throws SamFSException {
        long no = 0;
        if ((model != null) && (jniJob != null)) {
            no = jniJob.getNumberOfFiles(model.getJniContext());
        }

        return no;
    }

    public StageJobFileData[] getFileData(int start,
                                          int size,
                                          short sortby,
                                          boolean ascending)
        throws SamFSException {

        if (sortby != StageJob.SORT_BY_FILE_NAME) {
            throw new SamFSException("logic.unsupportedSortStage");
        }

        short s = (short) StagerJob.ST_SORT_BY_FILENAME;

        fileData = new StageJobFileData[0];
        if ((model != null) && (jniJob != null)) {
            StgFileInfo[] fInfo =
                jniJob.getFilesInfo(model.getJniContext(), start, size, s,
                                    ascending);
            if ((fInfo != null) && (fInfo.length > 0)) {
                fileData = new StageJobFileData[fInfo.length];
                for (int i = 0; i < fInfo.length; i++)
                    fileData[i] =
                        new StageJobFileDataImpl(fInfo[i].fileName,
                                                 fInfo[i].size,
                                                 fInfo[i].position,
                                                 fInfo[i].offset,
                                                 fInfo[i].vsn,
                                                 fInfo[i].user);
            }
        }

        return fileData;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("toString() needs to be updated for Stage Copy job.\n");
        buf.append(super.toString());

        try {
            if (fs != null) {
                buf.append("Filesystem: ")
                    .append(fs.getName())
                    .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        if (vsn != null)
            try {
                buf.append("VSN: ")
                    .append(vsn.getVSN())
                    .append("\n");
            } catch (Exception e) {
            }

        buf.append("Position: ")
            .append(position)
            .append("\n")
            .append("Offset: ")
            .append(offset)
            .append("\n")
            .append("File Name: ")
            .append(name)
            .append("\n")
            .append("File Size: ")
            .append(size)
            .append("\n")
            .append("Staged: ")
            .append(complete)
            .append("\n")
            .append("Username: ")
            .append(username)
            .append("\n")
            .append("StateFlag: ")
            .append(stateFlag)
            .append("\n");

        return buf.toString();
    }

    public int getStateFlag() {
        return stateFlag;
    }

    private void update() throws SamFSException {
        if (jniJob != null) {

            super.setstartDateTime(SamQFSUtil.
                                   convertTime(jniJob.getCreationTime()));

            vsnName = jniJob.getVSN();
            mediaType = SamQFSUtil.getMediaTypeInteger(jniJob.getMediaType());
            if (jniJob.getCrtFileInfo() != null) {
                position = jniJob.getCrtFileInfo().position;
                offset = jniJob.getCrtFileInfo().offset;
                name = jniJob.getCrtFileInfo().fileName;
                size = jniJob.getCrtFileInfo().size;
                username = jniJob.getCrtFileInfo().user;
            }
            stateFlag = jniJob.getStateFlag();
        }
    }
}
