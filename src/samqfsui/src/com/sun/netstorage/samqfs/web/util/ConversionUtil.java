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

// ident	$Id: ConversionUtil.java,v 1.24 2008/07/03 00:04:31 ronaldso Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import java.util.ArrayList;
import java.util.Properties;
import java.util.StringTokenizer;

public class ConversionUtil {

    // time unit in seconds
    public static final long MINUTE = 60L;
    public static final long HOUR   = MINUTE * 60L;
    public static final long DAY    = HOUR   * 24L;
    public static final long WEEK   = DAY    * 7L;
    public static final long MONTH  = DAY    * 30L;
    public static final long YEAR   = DAY    * 365L;

    /** Creates a new instance of ConversionUtil */
    public ConversionUtil() {
    }

    // Utility method to convert Size Unit from a String to an Int.
    public static int getSizeUnitAsInteger(String sizeUnitStr) {

        int sizeUnitInt = -1;
        if (sizeUnitStr.equals("bytes")) {
            sizeUnitInt = SamQFSSystemModel.SIZE_B;
        } else if (sizeUnitStr.equals("kbytes")) {
            sizeUnitInt = SamQFSSystemModel.SIZE_KB;
        } else if (sizeUnitStr.equals("mbytes")) {
            sizeUnitInt = SamQFSSystemModel.SIZE_MB;
        } else if (sizeUnitStr.equals("gbytes")) {
            sizeUnitInt = SamQFSSystemModel.SIZE_GB;
        } else if (sizeUnitStr.equals("tbytes")) {
            sizeUnitInt = SamQFSSystemModel.SIZE_TB;
        }

        return sizeUnitInt;
    }

    // Utility method to convert Size Unit from an int to a  String.
    public static String getSizeUnitAsString(int sizeUnitInt) {
        String sizeUnitStr = "";
        if (sizeUnitInt == SamQFSSystemModel.SIZE_B) {
            sizeUnitStr = "bytes";
        } else if (sizeUnitInt == SamQFSSystemModel.SIZE_KB) {
            sizeUnitStr = "kbytes";
        } else if (sizeUnitInt == SamQFSSystemModel.SIZE_MB) {
            sizeUnitStr = "mbytes";
        } else if (sizeUnitInt == SamQFSSystemModel.SIZE_GB) {
            sizeUnitStr = "gbytes";
        } else if (sizeUnitInt == SamQFSSystemModel.SIZE_TB) {
            sizeUnitStr = "tbytes";
        }

        return sizeUnitStr;
    }

    // Utility method to convert the Age Unit from a String to an Int.
    public static int getAgeUnitAsInteger(String ageUnitStr) {

        int ageUnitInt = -1;
        if (ageUnitStr.equals("seconds")) {
            ageUnitInt = SamQFSSystemModel.TIME_SECOND;
        } else if (ageUnitStr.equals("minutes")) {
            ageUnitInt = SamQFSSystemModel.TIME_MINUTE;
        } else if (ageUnitStr.equals("hours")) {
            ageUnitInt = SamQFSSystemModel.TIME_HOUR;
        } else if (ageUnitStr.equals("days")) {
            ageUnitInt = SamQFSSystemModel.TIME_DAY;
        } else if (ageUnitStr.equals("weeks")) {
            ageUnitInt = SamQFSSystemModel.TIME_WEEK;
        }

        return ageUnitInt;
    }


    /* null strings are converted to -1. empty to -2 */
    public static int strToIntVal(String str) throws SamFSException {

	int intVal = -1;
	if (str != null) {
            if (str.length() != 0) {
                try {
		    intVal = Integer.parseInt(str);
                } catch (NumberFormatException e) {
		    throw new SamFSException(e.toString());
                }
            } else
                intVal = -2;
	}
	return intVal;
    }

    /* null strings are converted to -1. empty to -2 */
    public static long strToLongVal(String str) throws SamFSException {

	long longVal = -1;
	if (str != null) {
            if (str.length() != 0) {
                try {
		    longVal = Long.parseLong(str);
                } catch (NumberFormatException e) {
		    throw new SamFSException(e.toString());
                }
            } else {
                longVal = -2;
            }
	}
	return longVal;
    }

    public static Integer strToInteger(String str) throws SamFSException {

	if (str == null) {
            return null;
        }

        Integer retVal = null;
        try {
            retVal = new Integer(str);
        } catch (NumberFormatException e) {
            throw new SamFSException(e.toString());
        }
	return retVal;
    }


    /**
     * convert a String that contains comma separated name=value pairs
     * to a Properties object.
     */
    public static Properties strToProps(String str) {

	TraceUtil.trace3("props str: " + str);

	Properties props = new Properties();
        if (str == null)
            return props;

        String[] parts = str.split("\"");
        // quoted substrings have odd indices. will not parse these
        // " is escaped by using ""

        String crtKey = null, val = null;
        String[] pairs; // name=value pairs
        int idx; // index of '=' in a name[=value] pair

        for (int i = 0, j; i < parts.length; i++) {
            if (parts[i].length() == 0) { // as a result of encountering ""
                if (i % 2 == 0)
                    val += "\""; // add back one " to the value
                continue;
            }

            if (i % 2 == 1) { // parts[i] is [part of] a quoted string
                val += parts[i];
                props.setProperty(crtKey, val);

            } else {
                if (i > 0)
                    parts[i] = parts[i].replaceFirst(",", "");
                pairs = parts[i].split(",");
                for (j = 0; j < pairs.length; j++) {
                    idx = pairs[j].indexOf('=');
                    if (idx == -1) {
                        idx = pairs[j].length();
                        val = "";
                    } else
                        val = pairs[j].substring(idx + 1).trim();
                    crtKey = pairs[j].substring(0, idx).trim();
                    props.setProperty(crtKey, val);
                }
            }
        }

	return props;
    }

    /**
     * convert a String that contains 'sep' separated tokens
     * to an array of strings.
     * if string is null the array will be null
     */
    public static String[] strToArray(String str, char sep)
        throws SamFSException {

        ArrayList lst = new ArrayList();
        String[] arr;
        if (str == null)
            return null;
        StringTokenizer st = new StringTokenizer(str);
        while (st.hasMoreTokens()) {
            lst.add(st.nextToken());
        }
        arr = new String[lst.size()];
	return (String[]) lst.toArray(arr);
    }


    public static String arrayToStr(Object[] objs, char sep) {
        return arrayToStr(objs, Character.toString(sep));
    }

    public static String arrayToStr(Object[] objs, String sep) {
        StringBuffer s = new StringBuffer();
        if (null != objs) {
            for (int i = 0; i < objs.length; i++) {
                s.append(objs[i]);
                if (i != objs.length - 1)
                   s.append(sep);
            }
        }
        return s.toString();
    }
}
