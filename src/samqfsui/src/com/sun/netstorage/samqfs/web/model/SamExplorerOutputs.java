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

// ident	$Id: SamExplorerOutputs.java,v 1.11 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;
import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Report;
import com.sun.netstorage.samqfs.web.util.SamUtil;

/**
 * this class encapsulates basic information about a SAM Explorer Output
 */
public class SamExplorerOutputs {

    // constants below must match those in mgmt.h
    final String KEY_EXPLORER_PATH = "path";
    final String KEY_EXPLORER_NAME = "name";
    final String KEY_EXPLORER_SIZE = "size";
    final String KEY_EXPLORER_CREATED  = "created";
    final String KEY_EXPLORER_MODIFIED = "modified";
    final String KEY_EXPLORER_FORMAT = "format"; // txt, xml, csv etc.
    final String KEY_EXPLORER_DESC   = "desc"; // description

    public static final String FORMAT_TEXT = "txt";
    public static final String FORMAT_XML  = "xml";
    public static final String FORMAT_CSV  = "csv";

    protected String path, name, format, desc;
    protected long size;
    protected GregorianCalendar created, modified;

    public SamExplorerOutputs(Properties props) throws SamFSException {
        if (props == null)
            return;

        path = props.getProperty(KEY_EXPLORER_PATH);
        name = props.getProperty(KEY_EXPLORER_NAME);
        size =
            ConversionUtil.strToLongVal(props.getProperty(KEY_EXPLORER_SIZE));
        created  = new GregorianCalendar();
        created.setTimeInMillis(1000 * ConversionUtil.strToLongVal(
            props.getProperty(KEY_EXPLORER_CREATED)));
        modified = new GregorianCalendar();
        modified.setTimeInMillis(1000 * ConversionUtil.strToLongVal(
            props.getProperty(KEY_EXPLORER_MODIFIED)));
        format = props.getProperty(KEY_EXPLORER_FORMAT);
        desc = props.getProperty(KEY_EXPLORER_DESC);

        if (path != null && path.startsWith(Report.REPORTS_DIR)) {
            // name and desc is not filled in C layer, so fill it here
            String str[] =
                path.substring(Report.REPORTS_DIR.length()).split("-");
            name = str[0];
            desc = SamUtil.getResourceString("reports.type.desc."+ str[0]);
        }
    }

    public SamExplorerOutputs(String propStr) throws SamFSException {
        this(ConversionUtil.strToProps(propStr));
    }

    public String getName() { return name; }
    public String getPath() { return path; }
    public long   getSize() { return size; }
    public String getFormat() { return format; }
    public GregorianCalendar getCreatedTime()  { return created; }
    public GregorianCalendar getModifiedTime() { return modified; }
    public String getDescription()   { return desc; }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_EXPLORER_PATH + E + path    + C +
                   KEY_EXPLORER_NAME + E + name    + C +
                   KEY_EXPLORER_SIZE + E + size    + C +
                   KEY_EXPLORER_CREATED  + E + created + C +
                   KEY_EXPLORER_MODIFIED + E + modified + C +
                   KEY_EXPLORER_FORMAT + E +
                                    (format != null ? format : "null") + C +
                   KEY_EXPLORER_DESC + E + (format != null ? desc : "null");
        return s;
    }
}
