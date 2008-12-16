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

// ident	$Id: Copy.java,v 1.10 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class Copy {

    private int copyNum;
    private long age, unage;
    boolean release, norelease;
    private long chgFlags;

    /**
     * private constructor
     */
    private Copy(int copyNum, long age, long unage,
        boolean release, boolean norelease, long chgFlags) {
            this.copyNum = copyNum;
            setArchiveAge(age);
            setUnarchiveAge(unage);
            setRelease(release);
            setNoRelease(norelease);
            this.chgFlags = chgFlags;
    }

    public Copy() {
        chgFlags = 0;
    }

    public int getCopyNumber() { return copyNum; }
    public void setCopyNumber(int copyNum) { this.copyNum = copyNum; }

    public long getArchiveAge() { return age; }
    public void setArchiveAge(long age) {
        chgFlags |= AR_CP_ar_age;
        this.age = age;
    }
    public void resetArchiveAge() {
        chgFlags &= ~AR_CP_ar_age;
    }

    public long getUnarchiveAge() { return unage; }
    public void setUnarchiveAge(long unage) {
        chgFlags |= AR_CP_un_ar_age;
        this.unage = unage;
    }
    public void resetUnarchiveAge() {
        chgFlags &= ~AR_CP_un_ar_age;
    }

    public boolean isRelease() { return release; }
    public void setRelease(boolean release) {
        chgFlags |= AR_CP_release;
        this.release = release;
    }
    public void resetRelease() {
        chgFlags &= ~AR_CP_release;
    }

    public boolean isNoRelease() { return norelease; }
    public void setNoRelease(boolean norelease) {
        chgFlags |= AR_CP_norelease;
        this.norelease = norelease;
    }
    public void resetNoRelease() {
        chgFlags &= ~AR_CP_norelease;
    }

    // these flags must match those defined in pub/mgmt/archive.h

    private static final long	AR_CP_ar_age	= 0x00000001;
    private static final long	AR_CP_release	= 0X00000002;
    private static final long	AR_CP_norelease	= 0x00000004;
    private static final long	AR_CP_un_ar_age	= 0x00000008;

    public String toString() {
        return ("copy " + copyNum + ": age=" + age + ",unage=" + unage +
        ",rel=" + release + ",norel=" + norelease + " [cf:" + chgFlags + "]");
    }

}
