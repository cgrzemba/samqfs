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

// ident	$Id: ReservationMethodHelper.java,v 1.10 2008/12/16 00:10:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.web.util.SamUtil;

/**
 * This utility class helps us abstract reservation method, which can have
 * upto three values, into a single object.
 *
 * reservation method = set | fs | {dir | user | group | owner}
 */
public class ReservationMethodHelper {
    // symbolic constants for the various reservation method types
    public static final int RM_SET = 0x01;
    public static final int RM_OWNER = 0x02; // don't check for this one
    public static final int RM_FS = 0x04;
    public static final int RM_DIR = 0x08;
    public static final int RM_USER = 0x10;
    public static final int RM_GROUP = 0x20;

    // reservation by set or policy reservation
    private int set = 0x00;

    // reservation by file system
    private int fs = 0x00;

    // encapsulatees reservation by dir | user | group | owner
    private int attributes = 0x00;

    public ReservationMethodHelper() {
    }

    /**
     * set the aggregated reservation method value. This method is called to
     * pass up the aggregated reservation method from the logic layer. This
     * method also breaks down the array to its components for use by UI.
     */
    public void setValue(int rm) {
        rm = rm > 0 ? rm : 0;
        // check for set
        set = rm & RM_SET;

        // check for fs
        fs = rm & RM_FS;

        // finally get the attributes
        int temp = rm - set - fs;

        // exclude 'owner' since its not really a reservation method
        if (temp > 0) {
            attributes = temp == RM_OWNER ? 0 : temp;
        } else {
            attributes = 0;
        }
    }

    /**
     * return the aggregated reservation method value. This method is called
     * to pass down the aggregated reservation method to the logic layer
     */
    public int getValue() {
        return set | fs | attributes;
    }


    // setter and getter methods for the component fields
    // these are only to be used by the UI when set display fields and
    // retrieving values from the display fields.
    public int getSet() {
        return set;
    }

    public void setSet(int set) {
        this.set = set;
    }

    public int getFS() {
        return fs;
    }

    public void setFS(int fs) {
        this.fs = fs;
    }

    public int getAttributes() {
        return attributes;
    }

    public void setAttributes(int attributes) {
        this.attributes = attributes;
    }

    /**
     * overwrite the java.lang.Object toString method
     *
     * @return - a | delimited list of the reservation method combo
     * e.g. fs | policy | directory
     */
    public String toString() {
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();

        // policy
        if (this.set != 0) {
            buffer.append(SamUtil.getResourceString(
                          "archiving.reservation.method.policy")).append(" | ");

        }

        // fs
        if (this.fs != 0) {
            buffer.append(SamUtil.getResourceString(
                          "archiving.reservation.method.fs")).append(" | ");
        }

        // whichever attribute is set
        switch (this.attributes) {
        case RM_DIR:
            buffer.append(SamUtil.getResourceString(
                         "archiving.reservation.method.dir")).append(" | ");
            break;
        case ReservationMethodHelper.RM_USER:
            buffer.append(SamUtil.getResourceString(
                          "archiving.reservation.method.user")).append(" | ");
            break;
        case ReservationMethodHelper.RM_GROUP:
            buffer.append(SamUtil.getResourceString(
                          "archiving.reservation.method.group")).append(" | ");
            break;
        default:
        }

        // now remove the trailing " | "
        String temp = buffer.toString().trim();
        if (temp.length() > 0) {
            temp = temp.substring(0, temp.length() - 1);
        }

        return temp;
    }
}
