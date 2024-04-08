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

// ident	$Id: ArchivePolCriteriaCopyImpl.java,v 1.5 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class ArchivePolCriteriaCopyImpl implements ArchivePolCriteriaCopy {
    private ArchivePolCriteria archPolCriteria = null;
    private Copy copy = null;
    private int copyNumber = -1;
    private long archAge = -1;
    private int archAgeUnit = -1;
    private long unarchAge = -1;
    private int unarchAgeUnit = -1;
    private boolean release = false;
    private boolean noRelease = false;

    public ArchivePolCriteriaCopyImpl(ArchivePolCriteria archPolCriteria,
                                      Copy copy,
                                      int copyNumber) {
        String tmp;
        long sec;

        this.archPolCriteria = archPolCriteria;
        this.copy = copy;
        this.copyNumber = copyNumber;

        if (copy != null) {
            sec = copy.getArchiveAge();
            if (sec >= 0) {
                tmp = SamQFSUtil.longToInterval(sec);
                this.archAge = SamQFSUtil.getLongValSecond(tmp);
                this.archAgeUnit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

            sec = copy.getUnarchiveAge();
            if (sec >= 0) {
                tmp = SamQFSUtil.longToInterval(sec);
                this.unarchAge = SamQFSUtil.getLongValSecond(tmp);
                this.unarchAgeUnit = SamQFSUtil.getTimeUnitInteger(tmp);
            }

            this.release = copy.isRelease();
            this.noRelease = copy.isNoRelease();
        } else {
            // set up a "scratch" copy that can be used as "default"
            this.copy = new Copy();
        }
    }

    public ArchivePolCriteria getArchivePolCriteria() {
        return archPolCriteria;
    }

    public void setArchivePolCriteria(ArchivePolCriteria archPolCriteria) {
        this.archPolCriteria = archPolCriteria;
    }

    public Copy getJniCopy() {
        return copy;
    }

    public int getArchivePolCriteriaCopyNumber() {
        return copyNumber;
    }

    public void setArchivePolCriteriaCopyNumber(int copyNumber) {
        this.copyNumber = copyNumber;
    }

    public long getArchiveAge() {
        return archAge;
    }

    public void setArchiveAge(long archAge) {
        this.archAge = archAge;
        if ((archAge != -1) && (archAgeUnit != -1)) {
            copy.setArchiveAge(SamQFSUtil.convertToSecond(archAge,
                                                          archAgeUnit));
        } else {
            copy.resetArchiveAge();
        }
    }

    public int getArchiveAgeUnit() {
        return archAgeUnit;
    }

    public void setArchiveAgeUnit(int archAgeUnit) {
        this.archAgeUnit = archAgeUnit;
        if ((archAge != -1) && (archAgeUnit != -1)) {
            copy.setArchiveAge(SamQFSUtil.convertToSecond(archAge,
                                                          archAgeUnit));
        } else {
            copy.resetArchiveAge();
        }
    }

    public long getUnarchiveAge() {
        return unarchAge;
    }

    public void setUnarchiveAge(long unarchAge) {
        this.unarchAge = unarchAge;
        if ((unarchAge != -1) && (unarchAgeUnit != -1)) {
            copy.setUnarchiveAge(SamQFSUtil.convertToSecond(unarchAge,
                                                            unarchAgeUnit));
        } else {
            copy.resetUnarchiveAge();
        }
    }

    public int getUnarchiveAgeUnit() {
        return unarchAgeUnit;
    }

    public void setUnarchiveAgeUnit(int unarchAgeUnit) {
        this.unarchAgeUnit = unarchAgeUnit;
        if ((unarchAge != -1) && (unarchAgeUnit != -1)) {
            copy.setUnarchiveAge(SamQFSUtil.convertToSecond(unarchAge,
                                                            unarchAgeUnit));
        } else {
            copy.resetUnarchiveAge();
        }
    }

    public boolean isRelease() {
        return release;
    }

    public void setRelease(boolean release) {
        this.release = release;
        copy.setRelease(release);
    }

    public boolean isNoRelease() {
        return noRelease;
    }

    public void setNoRelease(boolean noRelease) {
        this.noRelease = noRelease;
        copy.setNoRelease(noRelease);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        if (archPolCriteria == null) {
            buf.append("Policy criteria is null for this copy object.\n");
        }

        buf.append("Archive Age: ")
            .append(archAge)
            .append("\n")
            .append("Archive Age Unit: ")
            .append(archAgeUnit)
            .append("\n")
            .append("Unarchive Age: ")
            .append(unarchAge)
            .append("\n")
            .append("Unarchive Age Unit: ")
            .append(unarchAgeUnit)
            .append("\n")
            .append("Release: ")
            .append(release)
            .append("\n")
            .append("NoRelease: ")
            .append(noRelease)
            .append("\n");

        return buf.toString();
    }
}
