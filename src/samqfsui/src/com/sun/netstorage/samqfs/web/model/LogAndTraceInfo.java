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

// ident $Id: LogAndTraceInfo.java,v 1.10 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;
import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates basic information about a log/trace entity
 */
public class LogAndTraceInfo {

    final String KEY_NAME    = "name";
    final String KEY_PATH    = "path";
    final String KEY_TYPE    = "type";
    final String KEY_FLAGS   = "flags";
    final String KEY_STATE   = "state"; // on/off
    final String KEY_SIZE    = "size";
    final String KEY_MODTIME = "modtime";

    protected String name, type, path, flags;
    protected boolean state;
    protected long size;
    protected GregorianCalendar modTime;

    public LogAndTraceInfo(Properties props) throws SamFSException {
        if (props == null)
            return;
        name = props.getProperty(KEY_NAME);
        path = props.getProperty(KEY_PATH);
        type = props.getProperty(KEY_TYPE);
        flags = props.getProperty(KEY_FLAGS);

        String stateProp = props.getProperty(KEY_STATE);
        state = (stateProp == null) ? false : stateProp.equals("on");

        size = ConversionUtil.strToLongVal(props.getProperty(KEY_SIZE));

        modTime = new GregorianCalendar();
        modTime.setTimeInMillis(1000 *
            ConversionUtil.strToLongVal(props.getProperty(KEY_MODTIME)));
    }
    public LogAndTraceInfo(String infoStr) throws SamFSException {
        this(ConversionUtil.strToProps(infoStr));
    }

    public String getName()  { return name; }
    public String getType()  { return type; }
    public boolean isOn()    { return state; }
    public String getPath()  { return path; }
    public String getFlags() { return flags; }
    public long getSize()  { return size; } // bytes
    public GregorianCalendar getModtime() { return modTime; }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_NAME    + E + name + C +
                   KEY_TYPE    + E + type + C +
                   KEY_PATH    + E + path + C +
                   KEY_STATE   + E + (state ? "on" : "off") + C +
                   KEY_FLAGS   + E + flags + C +
                   KEY_SIZE    + E + size + C +
                   KEY_MODTIME + E + modTime.getTimeInMillis()/1000;
        return s;
    }

}
