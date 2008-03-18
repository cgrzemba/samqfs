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

// ident	$Id: ArGlobalDirective.java,v 1.13 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class ArGlobalDirective {

    // these change flags must match those in pub/mgmt/archive.h

    private static final long AR_GL_log_path = 0x00000001;
    private static final long AR_GL_ar_interval   = 0x00000002;
    private static final long AR_GL_wait = 0x00000004;
    private static final long AR_GL_scan_method	  = 0x00000008;
    private static final long AR_GL_archivemeta	  = 0x00000010;
    private static final long AR_GL_notify_script = 0x00000020;
    private static final long AR_GL_scan_squash   = 0x00000040;
    private static final long AR_GL_setarchdone   = 0x00000080;

    // Flags for the options field must match those in pub/mgmt/archive.h
    private static final int SCAN_SQUASH_ON = 0x00000001;
    private static final int SETARCHDONE_ON = 0x00000002;

    // private members

    private BufDirective bufDirs[],
        maxDirs[], overflowDirs[];
    private DrvDirective drvDirs[];
    private long interval; // seconds
    private short examMethod;   // examine(scan) method. see ArFSDirective
    private String logFile, notifyScript;
    private boolean wait;
    private boolean arcMeta;
    private Criteria crit[];
    private long chgFlags;
    private int options;
    private String timeouts[];

    /* private constructor */
    private ArGlobalDirective(BufDirective[] bufDirs, BufDirective[] maxDirs,
        BufDirective[] overflowDirs, DrvDirective[] drvDirs, long interval,
        short examMethod, String logFile, String notifyScript, boolean wait,
        boolean arcMeta, Criteria[] crit, long chgFlags, int options,
        String[] timeouts) {
            this.bufDirs = bufDirs;
            this.maxDirs = maxDirs;
            this.overflowDirs = overflowDirs;
            this.drvDirs = drvDirs;
            this.interval = interval;
            this.examMethod = examMethod;
            this.logFile = logFile;
            this.notifyScript = notifyScript;
            this.wait = wait;
            this.arcMeta = arcMeta;
            this.crit = crit;
            this.chgFlags = chgFlags;
	    this.options = options;
            this.timeouts = timeouts;
    }

    public ArGlobalDirective() {
        this.chgFlags = 0;
    }



    public BufDirective[] getBufferDirectives() { return bufDirs; }
    public void setBufferDirectives(BufDirective bufDirs[]) {
        this.bufDirs = bufDirs;
    }

    public BufDirective[] getMaxDirectives() { return maxDirs; }
    public void setMaxDirectives(BufDirective maxDirs[]) {
        this.maxDirs = maxDirs;
    }

    public BufDirective[] getOverflowDirectives() { return overflowDirs; }
    public void setOverflowDirectives(BufDirective overflowDirs[]) {
        this.overflowDirs = overflowDirs;
    }

    public DrvDirective[] getDriveDirectives() { return drvDirs; }
    public void setDriveDirectives(DrvDirective drvDirs[]) {
        this.drvDirs = drvDirs;
    }

    public long getInterval() { return interval; }
    public void setInterval(long interval) {
        this.interval = interval;
        chgFlags |= AR_GL_ar_interval;
    }
    public void resetInterval() {
        chgFlags &= ~AR_GL_ar_interval;
    }

    public short getExamineMethod() { return examMethod; }
    public void setExamineMethod(short examMethod) {
        this.examMethod = examMethod;
        chgFlags |= AR_GL_scan_method;
    }
    public void resetExamineMethod() {
        chgFlags &= ~AR_GL_scan_method;
    }

    public String getLogFile() { return logFile; }
    public void setLogFile(String logFile) {
        this.logFile = logFile;
        chgFlags |= AR_GL_log_path;
    }
    public void resetLogFile() {
        chgFlags &= ~AR_GL_log_path;
    }

    public String getNotifyScript() { return notifyScript; }
    public void setNotifyScript(String notifyScript) {
        this.notifyScript = notifyScript;
        chgFlags |= AR_GL_notify_script;
    }
    public void resetNotifyScript() {
        chgFlags &= ~AR_GL_notify_script;
    }

    public boolean getWait() { return wait; }
    public void setWait(boolean wait) {
        this.wait = wait;
        chgFlags |= AR_GL_wait;
    }
    public void resetWait() {
        chgFlags &= ~AR_GL_wait;
    }

    public boolean getArcMeta() { return arcMeta; }
    public void setArcMeta(boolean arcMeta) {
        this.arcMeta = arcMeta;
        chgFlags |= AR_GL_archivemeta;
    }
    public void resetArcMeta() {
        chgFlags &= ~AR_GL_archivemeta;
    }

    public Criteria[] getCriteria() { return crit; }
    public void setCriteria(Criteria crit[]) {
        this.crit = crit;
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
        chgFlags |= AR_GL_scan_squash;
    }
    public void resetScanListSquash() {
        chgFlags &= ~AR_GL_scan_squash;
    }

    public String[] getTimeouts() { return timeouts; }

    public String toString() {
        int i;
        String s  = "wait=" + (wait ? "T" : "F") + ",int=" + interval + ",log="+
            logFile + ",sh=" + notifyScript + ",arcm="  + (arcMeta ? "T" : "F")
	    + ",scan_squash="
	    + (((options & SCAN_SQUASH_ON) == SCAN_SQUASH_ON) ? "on" : "off")
	    + " opts:0x" + Integer.toHexString(options)
            + " [cf:0x" + Long.toHexString(chgFlags) + "]\n";
        for (i = 0; i < bufDirs.length; i++)
            s += bufDirs[i] + "\n";
        for (i = 0; i < maxDirs.length; i++)
            s += maxDirs[i] + "\n";
        for (i = 0; i < overflowDirs.length; i++)
            s += overflowDirs[i] + "\n";
        for (i = 0; i < drvDirs.length; i++)
            s += drvDirs[i] + "\n";
	if (crit != null)
	    for (i = 0; i < crit.length; i++)
		s += crit[i] + "\n";

        return s;
    }
}
