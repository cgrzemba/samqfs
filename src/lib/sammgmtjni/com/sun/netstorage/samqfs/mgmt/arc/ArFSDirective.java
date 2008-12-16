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

// ident	$Id: ArFSDirective.java,v 1.15 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class ArFSDirective {

    // these change flags must match those in pub/mgmt/archive.h

    private static final long AR_FS_log_path    = 0x00000001;
    private static final long AR_FS_fs_interval = 0x00000002;
    private static final long AR_FS_wait	= 0x00000004;
    private static final long AR_FS_scan_method = 0x00000008;
    private static final long AR_FS_archivemeta = 0x00000010;
    private static final long AR_FS_scan_squash = 0x00000040;
    private static final long AR_FS_setarchdone = 0x00000080;
    private static final long AR_FS_bg_interval	= 0x00000100;
    private static final long AR_FS_bg_time	= 0x00000200;

    // Flags for the options field must match those in pub/mgmt/archive.h
    private static final int SCAN_SQUASH_ON = 0x00000001;
    private static final int SETARCHDONE_ON = 0x00000002;

    // private members

    private String fsName;
    private Criteria crit[];
    private String logPath;
    private long interval;
    private short examMethod; // examine/scan method. list of valid vals below
    private boolean wait;
    private boolean arcMeta;
    private int numCopies;
    private Copy metadataCopies[];
    private long chgFlags;
    private int options;
    private long backgroundInterval;
    private int backgroundTime; // hhmm

    // valid values for examine(scan) method
    public static final int EM_NOT_SET = -1;
    public static final int EM_SCAN    = 1; /* Traditional scan  */
    public static final int EM_SCANDIRS   = 2; /* Scan only directories */
    public static final int EM_SCANINODES = 3; /* Scan only inodes */
    public static final int EM_NOSCAN  = 4; /* Continuous archiving */

    /**
     * private constructor
     */
    private ArFSDirective(String fsName, Criteria[] crit, String logPath,
        short examMethod, long interval, boolean wait, boolean arcMeta,
        int numCopies, Copy[] metadataCopies, long chgFlags, int options,
	long backgroundInterval, int backgroundTime) {
            this.fsName = fsName;
            this.crit = crit;
            this.logPath = logPath;
            this.interval = interval;
            this.examMethod = examMethod;
            this.wait = wait;
            this.arcMeta = arcMeta;
            this.numCopies = numCopies;
            this.metadataCopies = metadataCopies;
            this.chgFlags = chgFlags;
	    this.options = options;
	    this.backgroundInterval = backgroundInterval;
	    this.backgroundTime = backgroundTime;
    }

    /**
     * public constructor
     */
    public ArFSDirective() {
        chgFlags = 0;
    }

    public String getFSName() { return fsName; }

    public Criteria[] getCriteria() { return crit; }
    public void setCriteria(Criteria[] crit) {
        this.crit = crit;
    }

    public short getExamineMethod() { return examMethod; }
    public void setExamineMethod(short examMethod) {
        this.examMethod = examMethod;
        chgFlags |= AR_FS_scan_method;
    }
    public void resetExamineMethod() {
        chgFlags &= ~AR_FS_scan_method;
    }

    public String getLogPath() { return logPath; }
    public void setLogPath(String logPath) {
        this.logPath = logPath;
        chgFlags |= AR_FS_log_path;
    }
    public void resetLogPath() {
        chgFlags &= ~AR_FS_log_path;
    }

    public long getInterval() { return interval; }
    public void setInterval(long interval) {
        this.interval = interval;
        chgFlags |= AR_FS_fs_interval;
    }
    public void resetInterval() {
        chgFlags &= ~AR_FS_fs_interval;
    }

    public boolean isWait() { return wait; }
    public void setWait(boolean wait) {
        this.wait = wait;
        chgFlags |= AR_FS_wait;
    }
    public void resetWait() {
        chgFlags &= ~AR_FS_wait;
    }

    public boolean isArcMeta() { return arcMeta; }
    public void setArcMeta(boolean arcMeta) {
        this.arcMeta = arcMeta;
        chgFlags |= AR_FS_archivemeta;
    }
    public void resetArcMeta() {
        chgFlags &= ~AR_FS_archivemeta;
    }

    public Copy[] getMetadataCopies() { return metadataCopies; }
    public void setMedatataCopies(Copy[] metadataCopies) {
        this.metadataCopies = metadataCopies;
    }

    public boolean getScanListSquash() {
	return ((options & SCAN_SQUASH_ON) == SCAN_SQUASH_ON);
    }
    public void setScanListSquash(boolean scanSquash) {
	if (scanSquash) {
	    options |= SCAN_SQUASH_ON;
	} else {
	    options &= ~SCAN_SQUASH_ON;
	}
        chgFlags |= AR_FS_scan_squash;
    }
    public void resetScanListSquash() {
        chgFlags &= ~AR_FS_scan_squash;
    }

    public long getBackgroundInterval() { return backgroundInterval; }
    public void setBackgroundInterval(long backgroundInterval) {
        this.backgroundInterval = backgroundInterval;
        chgFlags |= AR_FS_bg_interval;
    }
    public void resetBackgroundInterval() {
        chgFlags &= ~AR_FS_bg_interval;
    }

    public int getBackgroundTime() { return backgroundTime; }
    public void setBackgroundTime(int backgroundTime) {
        this.backgroundTime = backgroundTime;
        chgFlags |= AR_FS_bg_time;
    }
    public void resetBackgroundTime() {
        chgFlags &= ~AR_FS_bg_time;
    }



    public String toString() {
        int i;
        String s = fsName + ",log=" + ((logPath == null) ? "-" : logPath) +
            ",intv=" + interval +
            ",wait=" + (wait ? "T," : "F,") +
            ",arcm=" + (arcMeta ? "T," : "F,") +
            ",exam=" + examMethod +
	    ",options:0x" + Integer.toHexString(options) + "," +
            numCopies + " metadata copies" +
            ((numCopies == 0) ? "\n" : ":\n");

        for (i = 0; i < metadataCopies.length; i++)
            if (metadataCopies[i] != null)
                s += " " + metadataCopies[i] + "\n";
            else s += " null\n";
        for (i = 0; i < crit.length; i++)
            s += crit[i] + "\n";
        return s;
    }
}
