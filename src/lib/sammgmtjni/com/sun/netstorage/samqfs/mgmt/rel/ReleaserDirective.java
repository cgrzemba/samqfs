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

// ident	$Id: ReleaserDirective.java,v 1.10 2008/12/16 00:08:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.rel;

public class ReleaserDirective {

    // these values must match those in release.h
    private static final int RL_releaser_log = 0x00000002;
    private static final int RL_size_priority = 0x00000004;
    private static final int RL_type = 0x00000008;
    private static final int RL_access_weight = 0x00000010;
    private static final int RL_modify_weight = 0x00000020;
    private static final int RL_residence_weight = 0x00000040;
    private static final int RL_simple = 0x00000080;
    private static final int RL_no_release = 0x00000100;
    private static final int RL_rearch_no_release = 0x00000200;
    private static final int RL_display_all_candidates = 0x00000400;
    private static final int RL_list_size = 0x00000800;
    private static final int RL_min_residence_age = 0x00001000;
    private static final int RL_debug_partial = 0x00002000;

    private String logFileName;
    private long minAge;
    private boolean noRelease, rearchNoRelease, logCandidates, debugPartial;
    private int files;  // # of candidates for release during one pass of the fs
    // priority related
    private float sizePrio;
    private short agePrioType;
    private float agePrioSimple; // if type is AGE_PRIO_SIMPLE
    private float agePrioAccess, agePrioModify, agePrioResidence; // detailed

    private int chgFlags;

    // age priority types. must match the enum in pub/mgmt/release.h
    public final static short AGE_PRIO_NOT_SET  = 0;
    public final static short AGE_PRIO_SIMPLE   = 1;
    public final static short AGE_PRIO_DETAILED = 2;

    private ReleaserDirective(String logFileName, long minAge,
        boolean noRelease, boolean rearchNoRelease, boolean logCandidates,
        boolean debugPartial, int files, float sizePrio, short agePrioType,
        float agePrioSimple,
        float agePrioAccess, float agePrioModify, float agePrioResidence,
        int chgFlags) {

            this.logFileName = logFileName;
            this.minAge = minAge;
            this.noRelease = noRelease;
            this.rearchNoRelease = rearchNoRelease;
            this.logCandidates = logCandidates;
            this.debugPartial = debugPartial;
            this.files = files;

            this.sizePrio = sizePrio;
            this.agePrioType = agePrioType;
            this.agePrioSimple = agePrioSimple;
            this.agePrioAccess = agePrioAccess;
            this.agePrioModify = agePrioModify;
            this.agePrioResidence = agePrioResidence;

            this.chgFlags = chgFlags;
    }

    // public functions

    public void setLogFile(String logFileName) {
        chgFlags |= RL_releaser_log;
        this.logFileName = logFileName;
    }
    public String getLogFile() { return logFileName; };

    public void setMinAge(long seconds) {
        chgFlags |= RL_min_residence_age;
        this.minAge = seconds;
    }
    public long getMinAge() { return minAge; };
    public void resetMinAge() {
        chgFlags &= ~RL_min_residence_age;
    }
}
