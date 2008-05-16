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

// ident	$Id: StagerParams.java,v 1.11 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.stg;

import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;
import com.sun.netstorage.samqfs.mgmt.arc.DrvDirective;

public class StagerParams {

    private String logPath;
    private int maxActive, maxRetries;
    private BufDirective bufDirs[];
    private DrvDirective drvDirs[];
    private long chgFlags;
    private long options;

    // options flags must match those defined in pub/mgmt/stage.h
    public static final int ST_LOG_START   = 0x00000001;
    public static final int ST_LOG_ERROR   = 0x00000002;
    public static final int ST_LOG_CANCEL  = 0x00000004;
    public static final int ST_LOG_FINISH  = 0x00000008;
    public static final int ST_LOG_ALL = 0x00000010;


    // these values must match those defined in pub/mgmt/stage.h
    public static final int ST_stage_log   = 0x00000001;
    public static final int ST_max_active  = 0x00000002;
    public static final int ST_max_retries = 0x00000004;
    public static final int ST_log_events  = 0x00000008;

    private StagerParams(String logPath, int maxActive, int maxRetries,
        BufDirective[] bufDirs, DrvDirective[] drvDirs, long options,
	long chgFlags) {
            this.logPath = logPath;
            this.maxActive = maxActive;
            this.maxRetries = maxRetries;
            this.bufDirs = bufDirs;
            this.drvDirs = drvDirs;
	    this.options = options;
            this.chgFlags = chgFlags;
    }

    public StagerParams(BufDirective[] bufDirs, DrvDirective[] drvDirs) {
        this.bufDirs = bufDirs;
        this.drvDirs = drvDirs;
        chgFlags = 0;
    }

    public String getLogPath() { return logPath; }
    public void setLogPath(String newPath) {
        this.logPath = newPath;
        chgFlags |= ST_stage_log;
    }
    public void resetLogPath() {
        chgFlags &= ~ST_stage_log;
    }

    public BufDirective[] getBufDirectives() { return bufDirs; }
    public void setBufDirectives(BufDirective[] bufDirs) {
        this.bufDirs = bufDirs;
    }

    public DrvDirective[] getDrvDirectives() { return drvDirs; }
    public void setDrvDirectives(DrvDirective[] drvDirs) {
        this.drvDirs = drvDirs;
    }

    public String toString() {
        int i;
        String s = ((logPath != null) ? logPath : "nolog") + ","
            + maxActive + "," + maxRetries;

        for (i = 0; i < bufDirs.length; i++)
            s += ((i == 0) ? "\n buffer directives:" : "")
                + "\n  " + bufDirs[i];
        for (i = 0; i < drvDirs.length; i++)
            s += ((i == 0) ? "\n drive directives:" : "")
                + "\n  " + drvDirs[i];
        return s;
    }
}
