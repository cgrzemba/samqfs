/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: RecoveryPointSchedule.java,v 1.8 2008/03/17 14:43:45 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import java.util.Properties;


/** recovery point schedule */
public class RecoveryPointSchedule extends Schedule {
    // recovery point specific keys
    public static final String LOCATION = "location";
    public static final String NAMES = "names";
    public static final String PRESCRIPT = "prescript";
    public static final String POSTSCRIPT = "postscript";
    public static final String LOGFILE = "logfile";
    public static final String EXCLUDEDIRS = "excludedirs";
    public static final String COMPRESS = "compress";
    public static final String AUTOINDEX = "autoindex";
    public static final String DISABLED = "disabled";
    public static final String PRESCR_FATAL = "prescrfatal";

    // member variables
    protected String location;
    protected String names;
    protected String prescript;
    protected String postscript;
    protected String logfile;
    protected String [] excludeDirs;
    protected Compression compression;
    protected boolean autoIndex;
    protected boolean disabled;
    protected boolean prescrFatal;

    public RecoveryPointSchedule() {
    }

    public RecoveryPointSchedule(Properties props) {
        super(props);
    }

    public Properties decodeSchedule(Properties props) {

        // decode recovery point specific values
        // location
        this.location = props.getProperty(LOCATION);

        // names
        this.names = props.getProperty(NAMES);

        // prescript path
        this.prescript = props.getProperty(PRESCRIPT);

        // postscript path
        this.postscript = props.getProperty(POSTSCRIPT);

        // logfile
        this.logfile = props.getProperty(LOGFILE);

        // exclude dirs
        String temp = props.getProperty(EXCLUDEDIRS);
        if (temp != null) {
            this.excludeDirs = temp.split(colon);
        }

        // compress
        try {
            temp = props.getProperty(COMPRESS);

            switch (Integer.parseInt(temp)) {
            case 0:
                this.compression = Compression.NONE;
                break;
            case 1:
                this.compression = Compression.GZIP;
                break;
            case 2:
                this.compression = Compression.COMPRESS;
                break;
            }
        } catch (NumberFormatException nfe) {
        }

        // auto index
        temp = props.getProperty(AUTOINDEX);
        this.autoIndex = "1".equals(temp);

        // disabled
        temp = props.getProperty(DISABLED);
        this.disabled = "1".equals(temp);

        // prescr_fatal
        temp = props.getProperty(PRESCR_FATAL);
        this.prescrFatal = "1".equals(temp);

        return props;
    }

    public String encodeSchedule() {
        StringBuffer buf = new StringBuffer();

        // encode the recovery point specific values
        // location
        if (this.location != null && location.length() > 0) {
            buf.append(c).append(LOCATION).append(e).append(this.location);
        }

        // names
        if (this.names != null && names.length() > 0) {
            buf.append(c).append(NAMES).append(e).append(this.names);
        }

        // prescript
        if (this.prescript != null && prescript.length() > 0) {
            buf.append(c).append(PRESCRIPT).append(e).append(this.prescript);
        }

        // postscript
        if (this.postscript != null && postscript.length() > 0) {
            buf.append(c).append(POSTSCRIPT).append(e).append(this.postscript);
        }

        // log file
        if (this.logfile != null && logfile.length() > 0) {
            buf.append(c).append(LOGFILE).append(e).append(this.logfile);
        }

        // exclude dirs
        if (excludeDirs != null && excludeDirs.length > 0) {
            StringBuffer dirs = new StringBuffer();
            for (int i = 0; i < excludeDirs.length; i++) {
                dirs.append(excludeDirs[i]).append(colon);
            }

            if (dirs.length() > 0) {
                buf.append(c).append(EXCLUDEDIRS).append(e)
                    .append(dirs.substring(0, dirs.length() -1));
            }
        }

        // compression
        buf.append(c).append(COMPRESS).append(e)
            .append(Integer.toString(this.compression.getType()));

        // auto index
        String temp = this.autoIndex ? "1" : "0";
        buf.append(c).append(AUTOINDEX).append(e).append(temp);

        // disabled
        temp = this.disabled ? "1" : "0";
        buf.append(c).append(DISABLED).append(e).append(temp);

        // prescript fatal
        temp = this.prescrFatal ? "1" : "0";
        buf.append(c).append(PRESCR_FATAL).append(e).append(temp);

        return buf.toString();
    }

    // setters
    public void setLocation(String loc) { this.location = loc; }
    public void setNames(String names) { this.names = names; }
    public void setPrescript(String script) { this.prescript = script; }
    public void setPostscript(String script) { this.postscript = script; }
    public void setLogFile(String logfile) { this.logfile = logfile; }
    public void setExcludeDirs(String [] dirs) { this.excludeDirs = dirs; }
    public void setCompression(Compression c) { this.compression = c; }
    public void setAutoIndex(boolean b) { this.autoIndex = b; }
    public void setDisabled(boolean b) { this.disabled = b; }
    public void setQuitOnPrescriptFailure(boolean b) { this.prescrFatal = b; }

    // getters
    public String getLocation() { return this.location; }
    public String getNames() { return this.names; }
    public String getPrescript() { return this.prescript; }
    public String getPostscript() { return this.postscript; }
    public String getLogFile() { return this.logfile; }
    public String [] getExcludedDirs() { return this.excludeDirs; }
    public Compression getCompression() { return this.compression; }
    public boolean isAutoIndex() { return this.autoIndex; }
    public boolean isDisabled() { return this.disabled; }
    public boolean isQuitOnPrescriptFailure() { return this.prescrFatal; }
}
