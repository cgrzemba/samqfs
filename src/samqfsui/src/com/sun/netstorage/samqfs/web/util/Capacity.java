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

// ident	$Id: Capacity.java,v 1.13 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;

import java.util.Comparator;

public class Capacity implements Comparable, Comparator {
    private long size;
    private int unit;
    NonSyncStringBuffer sizeStr;
    private boolean jsf = false;

    // Values of SamQFSSystemModel unit constants should be transparent to us
    // since they are arbitrary, so we will use local constants that we can
    // ensure are sequential.  We do math on these values.
    // Also, these values are used as array indexes to the unitSizes array.
    private static final int UNIT_B = 0;
    private static final int UNIT_KB = 1;
    private static final int UNIT_MB = 2;
    private static final int UNIT_GB = 3;
    private static final int UNIT_TB = 4;
    private static final int UNIT_PB = 5;

    // static place holders
    public static final long BYTE = 1;
    public static final long KB   = 1024;
    public static final long MB   = KB * KB;
    public static final long GB   = MB * KB;
    public static final long TB   = GB * KB;
    public static final long PB   = TB * KB;

    public static final long [] unitSizes = {BYTE, KB, MB, GB, TB, PB};

    /**
     * examples:
     * newCapacity(2048, ...Model.SIZE_KB) -> 2.00 MB
     * newCapacity(2500, ...Model.SIZE_KB) -> 2.44 MB
     */
    public static Capacity newCapacity(long size, int unit) {
        return new Capacity(size, unit, false, false);
    }

    public static Capacity newCapacityInJSF(long size, int unit) {
        return new Capacity(size, unit, false, true);
    }

    /**
     * examples:
     * newExactCapacity(2048, SIZE_KB) -> 2 MB
     * newExactCapacity(2500, SIZE_KB) -> 2500 KB
     */
    public static Capacity newExactCapacity(long size, int unit) {
        return new Capacity(size, unit, true, false);
    }

    protected Capacity() {
    }

    public Capacity(long size, int unit) {
        this(size, unit, false, false);
    }

    protected Capacity(long size, int unit, boolean exact, boolean jsf) {
        this.size = size;
        this.unit = convertSysUnitToLocalUnit(unit);
        this.jsf = jsf;

        format(0, (exact ? -1 : 2), exact);
        // 2 decimal digits by default, unless exact mode requested
    }


    // We can't depend on the system model constants to stay the same.
    protected int convertSysUnitToLocalUnit(int sysModelUnit) {
        switch (sysModelUnit) {
            case SamQFSSystemModel.SIZE_B:
                return UNIT_B;
            case SamQFSSystemModel.SIZE_KB:
                return UNIT_KB;
            case SamQFSSystemModel.SIZE_MB:
                return UNIT_MB;
            case SamQFSSystemModel.SIZE_GB:
                return UNIT_GB;
            case SamQFSSystemModel.SIZE_TB:
                return UNIT_TB;
            case SamQFSSystemModel.SIZE_PB:
                return UNIT_PB;
            default:
                return -1;
        }
    }

    protected int convertLocalUnitToSysUnit(int localUnit) {
        switch (localUnit) {
            case UNIT_B:
                return SamQFSSystemModel.SIZE_B;
            case UNIT_KB:
                return SamQFSSystemModel.SIZE_KB;
            case UNIT_MB:
                return SamQFSSystemModel.SIZE_MB;
            case UNIT_GB:
                return SamQFSSystemModel.SIZE_GB;
            case UNIT_TB:
                return SamQFSSystemModel.SIZE_TB;
            case UNIT_PB:
                return SamQFSSystemModel.SIZE_PB;
            default:
                return -1;
        }
    }

    protected void format(int decPart, int precision, boolean exact) {

        if (sizeStr == null)
            sizeStr = new NonSyncStringBuffer();

        if (unit < UNIT_PB) {

            // we want three significant figures
            if (size >= 1024) {
                int newDecPart = (int) (size % 1024);
                if (exact && newDecPart != 0) {
                    sizeStr.append(size);
                    return;
                }
                size = size / 1024;
                unit = unit + 1;
                format(newDecPart, precision, exact);
                return;
            } else {
            // now process decimal part. 'precision' = # of decimal digits
                sizeStr.append(size);
                // Don't add anything after the decimal if units are bytes.
                // Can't have fractional bytes, can we?
                if (precision > 0 && unit > UNIT_B) {
                    sizeStr.append(".");
                    double decPartDbl = ((double) decPart) / 1024;
                    for (int i = 0; i < precision; i++) {
                        decPartDbl *= 10;
                        if (decPartDbl < 1)
                            sizeStr.append("0"); // fill with zero-es
                        else
                           sizeStr.append((int)decPartDbl);
                        decPartDbl = decPartDbl - (int) decPartDbl;
                    }
                }
            }
        } else {
            sizeStr.append(size);
        }

        // add unit name
        sizeStr.append(" ");

        // map unit back to its system model value for String look up
        int sunit = convertLocalUnitToSysUnit(unit);

	sizeStr.append(SamUtil.getSizeUnitL10NString(sunit, jsf));
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
        // return this.unit as a system model unit;
        return convertLocalUnitToSysUnit(this.unit);
    }

    /**
     * @param precision number of decimal digits (default: 2)
     * if negative precision then use exact mode - no decimal digits
     */
    public void format(int precision) {
        format(0, precision, precision < 0);
    }

    /**
     * @deprecated
     */
    public void format() {
        format(0, 2, false);
    }

    public Object getValue() {
        return toString();
    }

    public String toString() {
        String temp = sizeStr.toString();

        if (temp.indexOf('-') != -1) {
            return "";
        } else {
            return temp;
        }
    }

    // implement the Comparable interface
    /**
     * Same as compareTo(Capacity c);
     */
    public int compareTo(Object o) {
        return compareTo((Capacity)o);
    }

    // implement the Comparator interface
    /**
     * Compares this capacity object to the passed in capacity object.
     *
     * @return If "this" capacity is less than the passed in capacity,
     * the return -1.  If "this" capacity is greater than the passed in
     * capacity, then return 1.  If they are the same, return 0;
     */
    public int compareTo(Capacity c) {
        long this_size = this.size * unitSizes[unit] / KB;
        long c_size = c.getSize() * unitSizes[c.getUnit()] / KB;

        if (this_size < c_size)
            return -1;
        if (this_size > c_size)
            return 1;
        // else we need to take into account values after the decimal point
        return this.toString().compareTo(c.toString());
    }

    // implement the comparator interface so that action table sorting
    // will work
    /**
     * Same as compare(Capacity c1, Capacity c2);
     */
    public int compare(Object o1, Object o2) {
        return compare((Capacity)o1, (Capacity)o2);
    }

    /**
     * Same as calling c1.compareTo(c2);
     */
    public int compare(Capacity c1, Capacity c2) {
        return c1.compareTo(c2);
    }


    public static void main(String [] args) {
        if (args.length < 2) {
            System.out.println("usage : capacity <size unit> [-e]");
            System.exit(1);
        }
        boolean exact = (args.length == 3);
        if (exact)
            System.out.println("[exact mode requested - no rounding]");

        long size = Long.parseLong(args[0]);
        int unit = Integer.parseInt(args[1]);

        Capacity c = new Capacity(size, unit, exact, false);

        System.out.println("capacity.toString : " + c);
        System.out.println("capacity.getValue : " + c.getValue());
    }
}
