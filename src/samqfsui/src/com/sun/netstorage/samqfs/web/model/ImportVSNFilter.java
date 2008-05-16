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

// ident    $Id: ImportVSNFilter.java,v 1.9 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.media.Media;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import java.util.Properties;

/**
 * this class encapsulates information on Import VSN Filter Criteria
 */
public class ImportVSNFilter {

    // constants below must match those in mgmt.h
    private static final String KEY_EQ_TYPE = "equ_type";
    private static final String KEY_FILTER_TYPE = "filter_type";
    private static final String KEY_SCRATCH_POOL_ID = "scratch_pool_id";
    private static final String KEY_START_VSN = "start_vsn";
    private static final String KEY_END_VSN = "end_vsn";
    private static final String KEY_VSN_EXPRESSION = "vsn_expression";

    protected String eqType, startVSN, endVSN, regEx;
    protected int filterType, poolID;

    public ImportVSNFilter(Properties props) throws SamFSException {
        if (props == null)
            return;

        eqType = props.getProperty(KEY_EQ_TYPE);
        startVSN = props.getProperty(KEY_START_VSN);
        endVSN = props.getProperty(KEY_END_VSN);
        regEx = props.getProperty(KEY_VSN_EXPRESSION);

        filterType  =
            ConversionUtil.strToIntVal(props.getProperty(KEY_FILTER_TYPE));
        poolID =
            ConversionUtil.strToIntVal(props.getProperty(KEY_SCRATCH_POOL_ID));
    }

    public ImportVSNFilter(String propStr) throws SamFSException {
        this(ConversionUtil.strToProps(propStr));
    }


    public ImportVSNFilter(
        int filterType, String eqType,
        String startVSN, String endVSN, String regEx,
        int poolID)
        throws SamFSException {

        // Filter Type is mandatory and it ranges from 0 to 4
        // as defined in Media.java (JNI)
        if (filterType < Media.FILTER_BY_SCRATCH_POOL ||
            filterType > Media.NO_FILTER) {
            throw new SamFSException(null, -2530);
        }

        // eqType is mandatory but do not check if it is included in this
        // constructor.  Presentation layer is going to construct this object
        // without eqType.  eqType will be added by the logic layer after
        // presentation layer constructs the object and pass on to logic layer.

        // Now base on the Filter Type, check if the corresponding fields are
        // filled in
        switch (filterType) {
            case Media.FILTER_BY_SCRATCH_POOL:
                if (poolID == -1) {
                    throw new SamFSException(
                        "Invalid Pool ID.  Developer's bug found!");
                }
                break;

            case Media.FILTER_BY_VSN_RANGE:
                if (startVSN == null || startVSN.length() == 0 ||
                    endVSN   == null || endVSN.length() == 0) {
                    throw new SamFSException(null, -2532);
                }
                startVSN = startVSN.trim();
                endVSN   = endVSN.trim();

                if (!SamUtil.isValidVSNString(startVSN) ||
                    !SamUtil.isValidVSNString(endVSN)) {
                    throw new SamFSException(null, -2532);
                }
                break;

            case Media.FILTER_BY_VSN_EXPRESSION:
                if (regEx == null || regEx.length() == 0) {
                    throw new SamFSException(null, -2533);
                }
                regEx.trim();
                if (!SamUtil.isValidString(regEx)) {
                    throw new SamFSException(null, -2533);
                }
                break;
        }

        // No error found in parameters, save to object
        this.filterType = filterType;
        this.eqType = eqType;
        this.startVSN = startVSN;
        this.endVSN = endVSN;
        this.regEx = regEx;
        this.poolID = poolID;
    }

    public void setEQType(String eqType) {
        this.eqType = eqType;
    }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_FILTER_TYPE + E + filterType + C +
                   KEY_EQ_TYPE  + E + eqType + C +
                   KEY_SCRATCH_POOL_ID + E + poolID + C +
                   KEY_START_VSN + E + startVSN + C +
                   KEY_END_VSN + E + endVSN + C +
                   KEY_VSN_EXPRESSION + E + regEx + C;
        return s;
    }
}
