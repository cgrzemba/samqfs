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

// ident    $Id: FileCopyDetails.java,v 1.10 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.util.Properties;


/**
 * this class encapsulates archive copy details of a file
 */
public class FileCopyDetails {

    // Keys have to be the same as described in FileUtil.java in JNI
    // Keys will not show up if the number of count is zero.

    /**
     * KEY_COPY => copy #
     * KEY_SEG_COUNT => # of segments
     * KEY_DAMAGED => # of damaged segment
     * KEY_STALE => # of stale segments
     * KEY_CREATED => copy creation time
     * KEY_MEDIA => media type
     * KEY_VSN => vsns that holds this copy
     * KEY_INCONSISTENT   => file was modified while archive copy was made
     */

    public static final String KEY_COPY = "copy";
    public static final String KEY_SEG_COUNT = "seg_count";
    public static final String KEY_DAMAGED = "damaged";
    public static final String KEY_STALE = "stale";
    public static final String KEY_CREATED = "created";
    public static final String KEY_MEDIA = "media";
    public static final String KEY_VSN = "vsn";
    public static final String KEY_INCONSISTENT = "inconsistent";

    // class variables to hold information after parsing the string from jni
    protected int copy = -1, mediaType = -1;
    protected int segCount = -1, damaged = -1, stale = -1, inconsistent = -1;
    protected long created;
    protected String [] vsns;


    public FileCopyDetails(Properties props) throws SamFSException {
        if (props == null) {
            return;
        }

        String copyStr = props.getProperty(KEY_COPY);
        String createdStr = props.getProperty(KEY_CREATED);
        String mediaStr = props.getProperty(KEY_MEDIA);
        String vsnStr = props.getProperty(KEY_VSN);
        String segCountStr = props.getProperty(KEY_SEG_COUNT);
        String damagedStr = props.getProperty(KEY_DAMAGED);
        String staleStr = props.getProperty(KEY_STALE);
        String inconStr = props.getProperty(KEY_INCONSISTENT);

        // copy & created should exist all the time
        try {
            copy = Integer.parseInt(copyStr);
            created   = Long.parseLong(createdStr);
        } catch (NullPointerException nullEx) {
        } catch (NumberFormatException numEx) {
        }

        // damaged exists if number is at least 1
        try {
            damaged = Integer.parseInt(damagedStr);
        } catch (NullPointerException nullEx) {
        } catch (NumberFormatException numEx) {
        }

        // segCount exists if file is segmented
        try {
            segCount = Integer.parseInt(segCountStr);
        } catch (NullPointerException nullEx) {
        } catch (NumberFormatException numEx) {
        }

        // if vsn key is absent, no vsn is found. It should not happen.
        if (vsnStr == null) {
            vsns = new String[0];
        } else {
            vsns = vsnStr.split(" ");
        }

        // Convert 2-letter media type to BaseDevice definitions
        if (mediaStr != null) {
            mediaType = SamQFSUtil.getEquipTypeInteger(mediaStr);
        }

        try {
            stale = Integer.parseInt(staleStr);
        } catch (NullPointerException nullEx) {
        } catch (NumberFormatException numEx) {
        }

        try {
            inconsistent = Integer.parseInt(inconStr);
        } catch (NullPointerException nullEx) {
        } catch (NumberFormatException numEx) {
        }
    }

    public FileCopyDetails(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    public int getCopyNumber() { return copy; }
    public long getCreatedTime() { return created; }
    public int getMediaType() { return mediaType; }
    public String [] getVSNs() { return vsns; }
    public boolean isInconsistent() { return inconsistent >= 1; }
    public int getInconsistentCount() { return inconsistent; };
    public int getSegCount() { return segCount; }
    public int getStaleCount() { return stale; }
    public int getDamagedCount() { return damaged; }
    public boolean isDamaged() { return damaged >= 1; }

    public String getVSNsInString() {
        StringBuffer vsnBuf = new StringBuffer();
        for (int i = 0; i < vsns.length; i++) {
            if (vsnBuf.length() > 0) vsnBuf.append(" ");
            vsnBuf.append(vsns[i]);
        }
        return vsnBuf.toString();
    }

    /**
     * Used in File Browser Table to keep a string for each entry that
     * stores the copy information of the file.  It is used by the Stage
     * and Restore pop up window.
     */
    public String encode() {
        String E = "=", C = ",";
        String s =
            KEY_COPY + E + copy + C +
            KEY_MEDIA + E + SamQFSUtil.getMediaTypeString(mediaType) + C +
            KEY_DAMAGED + E + damaged;
        return s;
    }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_COPY + E + copy + C +
                   KEY_CREATED + E + created + C +
                   KEY_MEDIA + E + mediaType + C +
                   KEY_VSN + E + getVSNsInString() + C +
                   KEY_INCONSISTENT + E + inconsistent + C +
                   KEY_SEG_COUNT + E + segCount + C +
                   KEY_STALE + E + stale + C +
                   KEY_DAMAGED + E + damaged;

        return s;
    }
}
