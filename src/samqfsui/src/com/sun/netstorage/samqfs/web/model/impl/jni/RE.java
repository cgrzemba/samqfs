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

// ident	$Id: RE.java,v 1.8 2008/05/16 18:39:00 am143972 Exp $
package com.sun.netstorage.samqfs.web.model.impl.jni;

import java.util.ArrayList;

public class RE {

    public static String genRange(char low, char high) {
	StringBuffer range = new StringBuffer(6);
	if (low == high)
	    range.append(low);
	else
	    range.append("[").append(low).append("-").append(high).append("]");
	return range.toString();
    }

    public static final String r09 = "[0-9]";

    public static char incDigit(char c) {
	return Character.forDigit(Character.digit(c, 10) + 1, 10);
    }
    public static char decDigit(char c) {
	return Character.forDigit(Character.digit(c, 10) - 1, 10);
    }

    /* return null if arguments have cannot be used to generate a range */
    protected static String getPrefix(String startVSN, String endVSN) {
	int maxsz = startVSN.length();
	int i = 0;
	StringBuffer prefix = new StringBuffer(maxsz + 1);
	if (maxsz != endVSN.length())
	    return null;

	while (startVSN.charAt(i) == endVSN.charAt(i)) {
	    prefix.append(startVSN.charAt(i++));
            if (i == maxsz)
                    break;
        }

	return prefix.toString();
    }

    /**
     * @return null if invalid arguments
     *
     * Algorithm :
     *
     * Start VSN:  188765
     * End VSN:    454321
     *
     * ones place:  18876[5-9]
     * tens place:   1887[7-9][0-9]
     * 100's place:   188[8-9][0-9][0-9]
     * 1000's place:   18[9][0-9][0-9][0-9]
     * 10000's place    1[9][0-9][0-9][0-9][0-9]
     * 100000's place:   [2-3][0-9][0-9][0-9][0-9][0-9]
     *
     *   This gets us 188765 - 399999.  Now we pick up the rest:
     *
     * 10000's place: 4[0-4][0-9][0-9][0-9][0-9]
     * 1000's place: 45[0-3][0-9][0-9][0-9]
     * 100's place: 454[0-2][0-9][0-9]
     * 10's place: 4543[0-1][0-9]
     * 1's place: 45432[0-1]
     */
    public static StringBuffer[] convertToREs(String startVSN, String endVSN) {

	String prefix = getPrefix(startVSN, endVSN);
	int vsnlen = startVSN.length();
	int preflen = (null == prefix) ? vsnlen - 1 : prefix.length();
	ArrayList res;

	int count = 0, i, j;
	StringBuffer re;

	if (null == prefix)
	    return null;
        if (preflen == vsnlen) { // startVSN and endVSN are identical
            StringBuffer res1[] = { new StringBuffer(startVSN) };
            return res1;
        }

        /* check if startVSN and endVSN can specify a valid range */
        if (startVSN.substring(preflen).compareTo(
            endVSN.substring(preflen))  > 0)
            return null;

        res = new ArrayList(2 * (vsnlen - preflen) - 1);

	/* Step 1/2: generate REs based on the first VSN */
	for (i = vsnlen - 1; i >= preflen; i--) {
	    char c = startVSN.charAt(i);
	    if (!Character.isDigit(c))
		return null;
	    re = new StringBuffer(vsnlen + 1);
	    // keep all chars up to c unchanged
	    re.append(startVSN.substring(0, i));
	    // generate range for c
	    if (i == vsnlen - 1) {
                if (i != preflen)
                    re.append(genRange(c, '9'));
                else
                    re.append(genRange(c, endVSN.charAt(i)));
            } else
		if (i == preflen) {
                    char c2 = decDigit(endVSN.charAt(i));
                    if (c == c2)
                        continue;
		    re.append(genRange(incDigit(c), c2));
                } else
		    if (c != '9')
			re.append(genRange(incDigit(c), '9'));
		    else
			continue;
	    // generate 0-9 for each char after c
	    for (j = i + 1; j < vsnlen; j++)
		re.append(r09);
	    res.add(re);
	}


	/* Step 2/2: generate REs based on the second VSN */
	for (i = preflen + 1; i < vsnlen; i++) {
	    char c = endVSN.charAt(i);
	    if (!Character.isDigit(c))
		return null;
	    re = new StringBuffer(vsnlen + 1);
	    // keep all chars up to c unchanged
	    re.append(endVSN.substring(0, i));
	    // generate range for c
	    if (i == vsnlen - 1)
                re.append(genRange('0', c));
	    else
                if (c != '0')
		    re.append(genRange('0', decDigit(c)));
		else
		    continue;
	    // generate 0-9 for each char after c
	    for (j = i + 1; j < vsnlen; j++)
		re.append(r09);
	    res.add(re);
	}

	return ((StringBuffer[])res.toArray(new StringBuffer[0]));
    }

    /**
     * @return a space-delimited list of REs or null if invalid arguments
     */
    public static String convertToREsString(String startVSN, String endVSN) {
        StringBuffer sbufs[] = convertToREs(startVSN, endVSN);
        StringBuffer concat;

        if (sbufs == null)
                return null;
        concat = new StringBuffer();
        for (int i = 0; i < sbufs.length; i++) {
            concat.append(sbufs[i]);
            if (i != sbufs.length - 1)
                concat.append(' ');
        }
        return concat.toString();
    }


    // test code
    public static final void main(String args[]) {
	StringBuffer res[] = convertToREs(args[0], args[1]);
	if (null == res) {
	    System.out.println("wrong arguments");
	    System.exit(-1);
	}
        System.out.println("list of RE-s:");
	for (int i = 0; i < res.length; i++)
	    System.out.println("\t" + res[i]);

        System.out.println("\nRE-s as a String:\n\t" +
            convertToREsString(args[0], args[1]));
    }

}
