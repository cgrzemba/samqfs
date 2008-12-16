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

// ident	$Id: ConversionUtil.java,v 1.26 2008/12/16 00:12:26 am143972 Exp $

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
     * Convert a String that contains comma separated name=value pairs
     * to a Properties object.
     *
     * Keys must not contain quotes, commas or equals signs.
     *
     * If a value being passed in contains commas, leading or trailing spaces,
     * or a double quote the entire value must be enclosed in double quotes.
     * Also a " inside of a value must be escaped as ""
     *
     * Note that split is not used because it makes it difficult to
     * handle strings that can contain the delimiter around which you
     * are splitting. For example trailing delimiters that are part of
     * a value are lost. Splitting on multiple delimiters resulted in
     * in too much record keeping since the values strings can contain
     * '=' ',' and '\"'.
     */
    public static Properties strToProps(String str) {
	CharSequence quotes = "\"\"".subSequence(0, 2);
	CharSequence quote = "\"".subSequence(0, 1);
	Properties props = new Properties();

	if (str == null) {
	    return (props);
	}

	int strLen = str.length();
	int i = 0;
	do {
	    String key;
	    String value;
	    boolean enclosed = false;
	    int valStart;
	    int valEnd = 0;
	    int quoteCnt = 0;
	    int keyStart = i;

	    while (i < strLen && str.charAt(i) != '=') {
		i++;
	    }

	    if (i == 0 || i == strLen) {
		/* error */
		return (props);
	    }

	    key = str.substring(keyStart, i);
	    key = key.trim();

	    i++; // move past the '='
	    while (i < strLen &&
		   (str.charAt(i) == ' ' || str.charAt(i) == '\t')) {
		i++;
	    }
	    if (i > strLen) {
		/* key with no value */
		return (props);
	    }

	    /*
	     * valStart intentionally includes any leading double quotes to
	     * allow subsequent trimming.
	     */
	    valStart = i;
	    if (str.charAt(i) == '\"') {
		/*
		 * The value is quoted. Move past the quote before beginning to
		 * count quotes in the next loop
		 */
		i++;
		enclosed = true;
	    }

	    /*
	     * Now find the end of the value. Note that a comma could
	     * occur in the value but if so, the entire value must be
	     * enclosed in quotes. Also note that quotes could occur
	     * in the value but they must be escaped by doubling them
	     * so " in the value would be "". This allows the % 2 test
	     * to work.
	     */
	    while (i < strLen) {
		char curChar = str.charAt(i);

		if (enclosed && curChar == '\"') {
		    quoteCnt++;
		    i++;
		} else if (enclosed && curChar == ',' &&
			   ((quoteCnt % 2) == 1)) {

		    /*
		     * Found valEnd since ',' comes after an odd number
		     * of quotes
		     */
		    valEnd = i;
		    break;

		} else if (!enclosed && curChar == ',') {
		    /* Found comma ending the value since it is not enclosed */
		    valEnd = i;
		    break;
		} else {
		    i++;
		}
	    }

	    if (i == strLen) {
		/* If end of input found, value goes to last char */
		valEnd = i;
	    }

	    if (valEnd <= valStart) {
		/* No value found */
		return (props);
	    }

	    /*
	     * Found val but must:
	     * 1. Remove trailing whitespace
	     * 2. Replace any "" with "
	     * 3. If enclosed remove the trailing "
	     */
	    value = str.substring(valStart, valEnd);

	    /* Remove trailing whitespace */
	    value = value.trim();

	    /*
	     * Now that whitespace is gone remove any enclosing quotes and
	     * translate any escaped "" to ".
	     */
	    if (enclosed) {
		value = value.substring(1, value.length() - 1);
		value = value.replace(quotes, quote);
	    }

	    props.setProperty(key, value);

	    i++;
	} while (i < strLen);

	return (props);
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
