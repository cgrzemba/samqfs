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

// ident	$Id: GenericFile.java,v 1.5 2008/05/16 18:39:00 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import java.util.Properties;
import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about a file
 * this is used for sam explorer outputs, reports etc.
 */
public class GenericFile {

    // keys for the name=value pairs returned by the jni layer
    public static final String NAME = "file_name";
    public static final String TYPE = "file_type";
    public static final String SIZE = "size";
    public static final String CREATE_DATE = "created";
    public static final String MODIFIED_DATE = "modified";
    public static final String ACCESSED_DATE = "accessed";
    public static final String PROTECTION = "protection";
    public static final String FORMAT = "format"; // txt, xml, csv etc.

    public static final String FORMAT_TEXT = "txt";
    public static final String FORMAT_XML = "xml";
    public static final String FORMAT_CSV = "csv";

    protected String name, format, desc;
    protected long size;
    protected GregorianCalendar created, modified;

    public GenericFile(Properties props) throws SamFSException {
        if (props == null)
            return;

        name = props.getProperty(NAME);
        size = ConversionUtil.strToLongVal(props.getProperty(SIZE));
        created  = new GregorianCalendar();
        created.setTimeInMillis(1000 * ConversionUtil.strToLongVal(
            props.getProperty(CREATE_DATE)));
        modified = new GregorianCalendar();
        modified.setTimeInMillis(1000 * ConversionUtil.strToLongVal(
            props.getProperty(MODIFIED_DATE)));

        // from the filename extension, get the format
        int extIndex = name.lastIndexOf(".");
        format = name.substring(extIndex + 1);
    }

    public GenericFile(String propStr) throws SamFSException {
        this(ConversionUtil.strToProps(propStr));
    }

    public String getName()   { return name; }
    public long   getSize()   { return size; }
    public String getFormat() { return format; }
    public GregorianCalendar getCreatedTime()  { return created; }
    public GregorianCalendar getModifiedTime() { return modified; }
    public String getDescription()   { return desc; }

    public void setDescription(String desc) { this.desc = desc; }

    public String toString() {
        String E = "=", C = ",";
        String s = NAME + E + name + C +
                   SIZE + E + size + C +
                   CREATE_DATE  + E + created + C +
                   MODIFIED_DATE + E + modified + C +
                   FORMAT + E + (format != null ? format : "null");
        return s;
    }
}
