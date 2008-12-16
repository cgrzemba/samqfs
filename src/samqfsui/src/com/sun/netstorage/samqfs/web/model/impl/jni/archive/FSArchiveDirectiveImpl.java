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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: FSArchiveDirectiveImpl.java,v 1.5 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.web.model.archive.FSArchiveDirective;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class FSArchiveDirectiveImpl implements FSArchiveDirective {

    private ArFSDirective jniArFSDirective = null;
    private SamQFSSystemModelImpl model = null;
    private String fsName = null;
    private String archLogfile = null;
    private long interval = -1;
    private int unit = -1;
    private int archMethod = -1;
    private boolean intrvTouched = false;

    public FSArchiveDirectiveImpl(SamQFSSystemModelImpl model,
                                  ArFSDirective jniArFSDirective) {

        this.model = model;
        this.jniArFSDirective = jniArFSDirective;

        if ((model == null) || (jniArFSDirective == null))
            throw new IllegalArgumentException(); // TODO: Put a string. Later.

        this.fsName = jniArFSDirective.getFSName();
        this.archLogfile = jniArFSDirective.getLogPath();

        long sec = jniArFSDirective.getInterval();
        if (sec >= 0) {
            String tmp = SamQFSUtil.longToInterval(sec);
            this.interval = SamQFSUtil.getLongValSecond(tmp);
            this.unit = SamQFSUtil.getTimeUnitInteger(tmp);
        }

        this.archMethod =
            SamQFSUtil.getLogicScanType(jniArFSDirective.getExamineMethod());
    }

    public ArFSDirective getJniArFSDirective() {
        return jniArFSDirective;
    }

    public String getFileSystemName() {
        return fsName;
    }

    public String getFSArchiveLogfile() {
        return archLogfile;
    }

    public void setFSArchiveLogfile(String logfile) {
        this.archLogfile = logfile;
        if (SamQFSUtil.isValidString(logfile))
            jniArFSDirective.setLogPath(logfile);
        else
            jniArFSDirective.resetLogPath();
    }

    public long getFSInterval() {
        return interval;
    }

    public void setFSInterval(long interval) {
        this.interval = interval;
        intrvTouched = true;
    }

    public int getFSIntervalUnit() {
        return unit;
    }

    public void setFSIntervalUnit(int unit) {
        this.unit = unit;
        intrvTouched = true;
    }

    public int getFSArchiveScanMethod() {
        return archMethod;
    }

    public void setFSArchiveScanMethod(int method) {
        this.archMethod = method;
        if (method != -1)
            jniArFSDirective.setExamineMethod(SamQFSUtil.
                                              getJniScanType(method));
        else
            jniArFSDirective.resetExamineMethod();
    }

    // this method needs to be called for any change in
    // GlobalArchiveDirective to take effect, i.e. after the
    // client is done calling changeXXX() or setXXX() methods,
    // this method needs to be called for these changes to be
    // effective
    public void changeFSDirective() throws SamFSException {
        if (intrvTouched) {
            if ((interval != -1) && (unit != -1))
                jniArFSDirective.setInterval(SamQFSUtil.
                                             convertToSecond(interval, unit));
            else
                jniArFSDirective.resetInterval();
        }

        if (SamQFSUtil.isValidString(jniArFSDirective.getLogPath()))
            FileUtil.createFile(model.getJniContext(),
                          jniArFSDirective.getLogPath());

        Archiver.setArFSDirective(model.getJniContext(), jniArFSDirective);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("FS name: ")
            .append(fsName)
            .append("\n")
            .append("Archive Logfile: ")
            .append(archLogfile)
            .append("\n")
            .append("Interval: ")
            .append(interval)
            .append("\n")
            .append("Interval Unit: ")
            .append(unit)
            .append("\n")
            .append("Archive Scan Method: ")
            .append(archMethod)
            .append("\n");

        return buf.toString();
    }
}
