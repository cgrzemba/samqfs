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

// ident $Id: PkgInfo.java,v 1.6 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about a Solaris package
 */
public class PkgInfo {

    final String KEY_PKGINST = "PKGINST";
    final String KEY_NAME = "NAME";
    final String KEY_CATEGORY = "CATEGORY";
    final String KEY_ARCH = "ARCH";
    final String KEY_VERSION = "VERSION";
    final String KEY_VENDOR = "VENDOR";
    final String KEY_DESC = "DESC";
    final String KEY_PSTAMP = "PSTAMP";
    final String KEY_INSTDATE = "INSTDATE";
    final String KEY_HOTLINE = "HOTLINE";
    final String KEY_STATUS = "STATUS";

    protected String pkginst, name, categ, arch, ver, vendor, desc, pstamp,
        instdate, hotline, status;


    public PkgInfo(Properties props) throws SamFSException {
        if (props == null)
            return;
        pkginst = props.getProperty(KEY_PKGINST);
        name = props.getProperty(KEY_NAME);
	categ = props.getProperty(KEY_CATEGORY);
        arch = props.getProperty(KEY_ARCH);
        ver = props.getProperty(KEY_VERSION);
        vendor = props.getProperty(KEY_VENDOR);
        desc = props.getProperty(KEY_DESC);
        pstamp = props.getProperty(KEY_PSTAMP);
        instdate = props.getProperty(KEY_INSTDATE);
        hotline = props.getProperty(KEY_HOTLINE);
        status = props.getProperty(KEY_STATUS);
    }
    public PkgInfo(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    public String getPKGINST()  { return pkginst; }
    public String getNAME() { return name; }
    public String getCATEGORY() { return categ; }
    public String getARCH() { return arch; }
    public String getVERSION() { return ver; }
    public String getVENDOR() { return vendor; }
    public String getDESC() { return desc; }
    public String getPSTAMP() { return pstamp; }
    public String getINSTDATE() { return instdate; }
    public String getHOTLINE() { return hotline; }
    public String getSTATUS() { return status; } // eg:"completely installed"

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_PKGINST  + E + pkginst + C +
                   KEY_NAME + E + name + C +
                   KEY_CATEGORY + E + categ + C +
                   KEY_ARCH + E + arch + C +
                   KEY_VERSION  + E + ver + C +
                   KEY_VENDOR   + E + vendor + C +
                   KEY_DESC + E + desc + C +
                   KEY_PSTAMP   + E + pstamp + C +
                   KEY_INSTDATE + E + instdate + C +
                   KEY_HOTLINE  + E + hotline + C +
                   KEY_STATUS   + E + status;
        return s;
    }
}
