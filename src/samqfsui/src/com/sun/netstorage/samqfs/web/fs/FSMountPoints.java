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

// ident	$Id: FSMountPoints.java,v 1.1 2009/01/29 15:50:19 ronaldso Exp $

/**
 * Helper class to sort the File system mount points in length to determine
 * what file system user is currently browsing.
 */

package com.sun.netstorage.samqfs.web.fs;

import java.util.Comparator;

public class FSMountPoints implements Comparable, Comparator {

    String fsName = null, mountPoint = null;

    public FSMountPoints(String fsName, String mountPoint) {
        this.fsName = fsName;
        this.mountPoint = mountPoint;
    }

    public String getFSName() {
        return this.fsName;
    }

    public String getMountPoint() {
        return this.mountPoint;
    }

    // implement the Comparable interface
    public int compareTo(Object o) {
        return compareTo((FSMountPoints)o);
    }

    // implement the Comparator interface
    /**
     * Compares this capacity object to the passed in capacity object.
     *
     * @return If "this" mount point length is less than the passed in
     * mount point, otherwise return -1.  If "this" mount point length is
     * greater than the passed in mount point, then return 1.
     * If they are the same, return 0;
     */
    public int compareTo(FSMountPoints p) {
        int this_length = this.mountPoint.length();
        int p_length    = p.getMountPoint().length();

        if (this_length < p_length) {
            return -1;
        } else if (this_length > p_length) {
            return 1;
        } else {
            return 0;
        }
    }

    // implement the comparator interface so that action table sorting
    // will work
    /**
     * Same as compare(FSMountPoints p1, FSMountPoints p2);
     */
    public int compare(Object o1, Object o2) {
        return compare((FSMountPoints)o1, (FSMountPoints)o2);
    }

    /**
     * Same as calling p1.compareTo(c2);
     */
    public int compare(FSMountPoints p1, FSMountPoints p2) {
        return p1.compareTo(p2);
    }
}

