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

// ident	$Id: GlobalArchiveDirectiveImpl.java,v 1.17 2008/05/16 18:39:01 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArGlobalDirective;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;
import com.sun.netstorage.samqfs.mgmt.arc.DrvDirective;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import java.util.ArrayList;

public class GlobalArchiveDirectiveImpl implements GlobalArchiveDirective {

    private ArGlobalDirective globalDir = null;
    private SamQFSSystemModelImpl model = null;
    private ArrayList bufDir = new ArrayList();
    private ArrayList minFileSize = new ArrayList();
    private ArrayList bufSize = new ArrayList();
    private ArrayList driveDir = new ArrayList();
    private String archLogfile = null;
    private long interval = -1;
    private int unit = -1;
    private int archMethod = -1;
    private boolean bufTouched = false;
    private boolean minTouched = false;
    private boolean sizeTouched = false;
    private boolean drvTouched = false;
    private boolean intrvTouched = false;


    public GlobalArchiveDirectiveImpl() {
    }

    public GlobalArchiveDirectiveImpl(ArrayList bufDir,
                                      ArrayList minFileSize,
                                      ArrayList bufSize,
                                      ArrayList driveDir,
                                      String archLogfile,
                                      long interval,
                                      int unit,
                                      int archMethod) {
        this.bufDir = bufDir;
        this.minFileSize = minFileSize;
        this.bufSize = bufSize;
        this.driveDir = driveDir;
        this.archLogfile = archLogfile;
        this.interval = interval;
        this.unit = unit;
        this.archMethod = archMethod;
    }

    public GlobalArchiveDirectiveImpl(SamQFSSystemModelImpl model,
                                      ArGlobalDirective globalDir)
        throws SamFSException {

        this.model = model;
        this.globalDir = globalDir;

        if (globalDir != null) {
            BufDirective[] bufDirs1 = globalDir.getBufferDirectives();
            BufDirective[] bufDirs2 = globalDir.getMaxDirectives();
            BufDirective[] bufDirs3 = globalDir.getOverflowDirectives();
            DrvDirective[] drvDirs = globalDir.getDriveDirectives();
            this.archLogfile = globalDir.getLogFile();

            long sec = globalDir.getInterval();
            if (sec >= 0) {
                String tmp = SamQFSUtil.longToInterval(sec);
                this.interval = SamQFSUtil.getLongValSecond(tmp);
                this.unit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

            this.archMethod =
                SamQFSUtil.getLogicScanType(globalDir.getExamineMethod());

            if (bufDirs1 != null) {
                for (int i = 0; i < bufDirs1.length; i++)
                    bufSize.add(new BufferDirectiveImpl(bufDirs1[i]));
            }
            if (bufDirs2 != null) {
                for (int i = 0; i < bufDirs2.length; i++)
                    bufDir.add(new BufferDirectiveImpl(bufDirs2[i]));
            }
            if (bufDirs3 != null) {
                for (int i = 0; i < bufDirs3.length; i++)
                    minFileSize.add(new BufferDirectiveImpl(bufDirs3[i]));
            }
            if (drvDirs != null) {
                for (int i = 0; i < drvDirs.length; i++)
                    driveDir.add(new DriveDirectiveImpl(model, drvDirs[i]));
            }
        }
    }

    public BufferDirective[] getMaxFileSize() {
        return (BufferDirective[]) bufDir.toArray(new BufferDirective[0]);
    }

    public void addMaxFileSizeDirective(BufferDirective dir) {
        bufDir.add(dir);
        bufTouched = true;
    }

    public void deleteMaxFileSizeDirective(BufferDirective dir) {
        int index = bufDir.indexOf(dir);
        if (index != -1) {
            bufDir.remove(index);
            bufTouched = true;
        }
    }

    public BufferDirective[] getMinFileSizeForOverflow() {
        return (BufferDirective[]) minFileSize.toArray(new BufferDirective[0]);
    }

    public void addMinFileSizeForOverflow(BufferDirective dir) {
        minFileSize.add(dir);
        minTouched = true;
    }

    public void deleteMinFileSizeForOverflow(BufferDirective dir) {
        int index = minFileSize.indexOf(dir);
        if (index != -1) {
            minFileSize.remove(index);
            minTouched = true;
        }
    }

    public BufferDirective[] getBufferSize() {
        return (BufferDirective[]) bufSize.toArray(new BufferDirective[0]);
    }

    public void addBufferSize(BufferDirective dir) {
        // TBD: Potential bug; need to check whether the directive with
        // the given media type already exits; if yes then remove first.
        bufSize.add(dir);
        sizeTouched = true;
    }

    public void deleteBufferSize(BufferDirective dir) {
        int index = bufSize.indexOf(dir);
        if (index != -1) {
            bufSize.remove(index);
            sizeTouched = true;
        }
    }

    public DriveDirective[] getDriveDirectives() {
        return (DriveDirective[]) driveDir.toArray(new DriveDirective[0]);
    }

    public void addDriveDirective(DriveDirective dir) {
        driveDir.add(dir);
        drvTouched = true;
    }

    public void deleteDriveDirective(DriveDirective dir) {
        int index = driveDir.indexOf(dir);
        if (index != -1) {
            driveDir.remove(index);
            drvTouched = true;
        }
    }

    public String getArchiveLogfile() {
        return archLogfile;
    }

    public void setArchiveLogfile(String logfile) {
        this.archLogfile = logfile;
        if (SamQFSUtil.isValidString(logfile))
            globalDir.setLogFile(logfile);
        else
            globalDir.resetLogFile();
    }

    public long getInterval() {
        return interval;
    }

    public void setInterval(long interval) {
        this.interval = interval;
        intrvTouched = true;
    }

    public int getIntervalUnit() {
        return unit;
    }

    public void setIntervalUnit(int unit) {
        this.unit = unit;
        intrvTouched = true;
    }

    public int getArchiveScanMethod() {
        return archMethod;
    }

    public void setArchiveScanMethod(int method) {
        this.archMethod = method;
        if (method != -1)
            globalDir.setExamineMethod(SamQFSUtil.getJniScanType(method));
        else
            globalDir.resetExamineMethod();
    }


    // this method needs to be called for any change in
    // GlobalArchiveDirective to take effect, i.e. after the
    // client is done calling changeXXX() or setXXX() methods,
    // this method needs to be called for these changes to be
    // effective
    public void changeGlobalDirective() throws SamFSException {
        if (sizeTouched) {
            BufDirective[] buf = new BufDirective[bufSize.size()];
            for (int i = 0; i < bufSize.size(); i++)
                buf[i] = ((BufferDirectiveImpl) bufSize.get(i))
                    .getJniBufferDirective();
            globalDir.setBufferDirectives(buf);
        }

        if (minTouched) {
            BufDirective[] buf = new BufDirective[minFileSize.size()];
            for (int i = 0; i < minFileSize.size(); i++)
                buf[i] = ((BufferDirectiveImpl) minFileSize.get(i))
                    .getJniBufferDirective();
            globalDir.setOverflowDirectives(buf);
        }

        if (bufTouched) {
            BufDirective[] buf = new BufDirective[bufDir.size()];
            for (int i = 0; i < bufDir.size(); i++)
                buf[i] = ((BufferDirectiveImpl) bufDir.get(i))
                    .getJniBufferDirective();
            globalDir.setMaxDirectives(buf);
        }

        if (drvTouched) {
            DrvDirective[] drv = new DrvDirective[driveDir.size()];
            for (int i = 0; i < driveDir.size(); i++)
                drv[i] = ((DriveDirectiveImpl) driveDir.get(i))
                    .getJniDriveDirective();
            globalDir.setDriveDirectives(drv);
        }

        if (intrvTouched) {
            if ((interval != -1) && (unit != -1))
                globalDir.setInterval(SamQFSUtil.convertToSecond(interval,
                                                                 unit));
            else
                globalDir.resetInterval();
        }

        if (SamQFSUtil.isValidString(globalDir.getLogFile()))
            FileUtil.createFile(model.getJniContext(), globalDir.getLogFile());

        Archiver.setArGlobalDirective(model.getJniContext(), globalDir);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Maximum File Size: \n\n");
        try {
            if (bufDir != null) {
                for (int i = 0; i < bufDir.size(); i++)
                    buf.append(((BufferDirective) bufDir.get(i)).toString())
                        .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        buf.append("Minimum File Size For Overflow: \n\n");
        try {
            if (minFileSize != null) {
                for (int i = 0; i < minFileSize.size(); i++)
                    buf.append((BufferDirective) minFileSize.get(i))
                        .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        buf.append("Buffer Size: \n\n");
        try {
            if (bufSize != null) {
		for (int i = 0; i < bufSize.size(); i++)
		    buf.append((BufferDirective)bufSize.get(i))
                .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        buf.append("Drive Directive: \n\n");
        try {
            if (driveDir != null) {
		for (int i = 0; i < driveDir.size(); i++)
		    buf.append((DriveDirective) driveDir.get(i))
                .append("\n\n");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        buf.append("Archive Logfile: ")
            .append(archLogfile)
            .append("\n")
            .append("Interval: ")
            .append(interval)
            .append("\n")
            .append("Archive Scan Method: ")
            .append(archMethod)
            .append("\n");

        return buf.toString();
    }
}
