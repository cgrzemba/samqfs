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

// ident	$Id: TimeConvertor.java,v 1.4 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;


import java.util.Comparator;

public class TimeConvertor implements Comparable, Comparator {
    private long size;
    private int unit;
    private long carryOverSize;
    private StringBuffer sizeStr;

    // Unit definition
    public static final int UNIT_MILLI_SEC = 0;
    public static final int UNIT_SEC = 1;
    public static final int UNIT_MIN = 2;
    public static final int UNIT_HR = 3;
    public static final int UNIT_DAY = 4;

    private static final int MSEC_IN_SEC = 1000;
    private static final int SEC_IN_MIN = 60;
    private static final int MIN_IN_HR = 60;
    private static final int HR_IN_DAY = 24;

    // static place holders
    public static final long MSEC = 1;
    public static final long SEC  = MSEC * MSEC_IN_SEC;
    public static final long MIN  = SEC * SEC_IN_MIN;
    public static final long HR   = MIN * MIN_IN_HR;
    public static final long DAY  = HR * HR_IN_DAY;

    public static final long [] unitSizes = {MSEC, SEC, MIN, HR, DAY};

    // Determine which utility method to call when converting resource bundle
    // string
    public boolean jsf = false;

    /**
     * examples:
     * newTimeConvertor(60, ...TimeConvertor.UNIT_SEC) -> 1 MIN
     * newTimeConvertor(3600, ...TimeConvertor.UNIT_MIN) -> 2 DAYS 12 HRS
     */
    public static TimeConvertor newTimeConvertor(long size, int unit) {
        return new TimeConvertor(size, unit);
    }
    public static TimeConvertor newTimeConvertor(
        long size, int unit, boolean jsf) {
        return new TimeConvertor(size, unit, jsf);
    }

    protected TimeConvertor() {
    }

    public TimeConvertor(long size, int unit) {
        this(size, unit, false);
    }

    public TimeConvertor(long size, int unit, boolean jsf) {
        this.size = size;
        this.unit = unit;
        this.jsf = jsf;

        if (size <= 0) {
            sizeStr = new StringBuffer();
        } else {
            format();
        }
    }

    protected void format() {
        if (sizeStr == null) {
            sizeStr = new StringBuffer();
        }

        if (isSizeOverLimit(size, unit)) {
            long newSize = getNewSize(size, unit);
            carryOverSize = getNewSizeRemainder(size, unit);
            size = newSize;
            unit++;
            format();
            return;
        } else {
            sizeStr.append(size).append(" ").append(
                SamUtil.getDurationL10NString(size > 1, unit, jsf));
            if (carryOverSize != 0) {
                sizeStr.append(" ").append(carryOverSize).append(" ").append(
                    SamUtil.getDurationL10NString(
                        carryOverSize > 1, unit - 1, jsf));
            }
            return;
        }
    }

    private boolean isSizeOverLimit(long size, int unit) {
        switch (unit) {
            case UNIT_MILLI_SEC:
                return size >= (long) MSEC_IN_SEC;
            case UNIT_SEC:
                return size >= (long) SEC_IN_MIN;
            case UNIT_MIN:
                return size >= (long) MIN_IN_HR;
            case UNIT_HR:
                return size >= (long) HR_IN_DAY;
            case UNIT_DAY:
            default:
                return false;
        }
    }

    private long getNewSize(long size, int unit) {
        switch (unit) {
            case UNIT_MILLI_SEC:
                return size / (long) MSEC_IN_SEC;
            case UNIT_SEC:
                return size / (long) SEC_IN_MIN;
            case UNIT_MIN:
                return size / (long) MIN_IN_HR;
            case UNIT_HR:
                return size / (long) HR_IN_DAY;
        }
        return -1;
    }

    private long getNewSizeRemainder(long size, int unit) {
        switch (unit) {
            case UNIT_MILLI_SEC:
                return size % (long) MSEC_IN_SEC;
            case UNIT_SEC:
                return size % (long) SEC_IN_MIN;
            case UNIT_MIN:
                return size % (long) MIN_IN_HR;
            case UNIT_HR:
                return size % (long) HR_IN_DAY;
        }
        return -1;
    }

    private long convertToSmallestUnit(long size, int unit) {
        if (unit == UNIT_MILLI_SEC) {
            return size;
        }
        switch (unit) {
            case UNIT_SEC:
                return
                    convertToSmallestUnit(size * (long) SEC_IN_MIN, unit - 1);
            case UNIT_MIN:
                return
                    convertToSmallestUnit(size * (long) MIN_IN_HR, unit - 1);
            case UNIT_HR:
                return
                    convertToSmallestUnit(size * (long) HR_IN_DAY, unit - 1);
            default:
                return size;
        }
    }

    protected void setSize(long size) {
        this.size = size;
    }

    protected void setUnit(int unit) {
        this.unit = unit;
    }

    public long getSize() {
        return this.size;
    }

    public int getUnit() {
        return this.unit;
    }

    public Object getValue() {
        return toString();
    }

    public String toString() {
        return sizeStr.toString();
    }

    // implement the Comparable interface
    /**
     * Same as compareTo(TimeConvertor c);
     */
    public int compareTo(Object o) {
        return compareTo((TimeConvertor)o);
    }

    // implement the Comparator interface
    /**
     * Compares this TimeConvertor object to the passed in TimeConvertor object.
     *
     * @return If "this" TimeConvertor is less than the passed in TimeConvertor,
     * the return -1.  If "this" TimeConvertor is greater than the passed in
     * TimeConvertor, then return 1.  If they are the same, return 0;
     */
    public int compareTo(TimeConvertor c) {
        long this_size = convertToSmallestUnit(this.size, this.unit);
        long c_size = convertToSmallestUnit(c.size, c.unit);

        if (this_size < c_size) {
            return -1;
        } else if (this_size > c_size) {
            return 1;
        } else {
            return 0;
        }
    }

    // implement the comparator interface so that action table sorting
    // will work
    /**
     * Same as compare(TimeConvertor c1, TimeConvertor c2);
     */
    public int compare(Object o1, Object o2) {
        return compare((TimeConvertor)o1, (TimeConvertor)o2);
    }

    /**
     * Same as calling c1.compareTo(c2);
     */
    public int compare(TimeConvertor c1, TimeConvertor c2) {
        return c1.compareTo(c2);
    }

    public boolean isJsf() {
        return jsf;
    }
}
