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

// ident $Id: RestoreFileImpl.java,v 1.10 2008/03/17 14:43:49 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import java.io.File;
import java.util.GregorianCalendar;
import java.util.Properties;


/**
 *  This class represents a file whose inode will be restored and which will
 * optionally be staged.
 */
public class RestoreFileImpl implements RestoreFile {

    protected boolean isDirectory;
    protected String absolutePath;
    protected String parentPath;
    protected String fileName;
    protected String restorePath;

    protected String fileDetails;
    protected String prot, size, user, group;
    protected GregorianCalendar crTime, modTime;
    protected String crTimeStr, modTimeStr;

    protected Boolean isDamaged;
    protected Boolean isOnline;

    protected String[] archCopies; // this array may have "holes" ("" = nocopy)
    protected int stageCopy = SYS_STG_COPY; // let system pick copy

    protected final String KEY_PROT    = "protection";
    protected final String KEY_SIZE    = "size";
    protected final String KEY_USER    = "user";
    protected final String KEY_GROUP   = "group";
    protected final String KEY_CRTIME  = "created";
    protected final String KEY_MODTIME = "modified";
    protected final String KEY_DAMAGED = "damaged";
    protected final String KEY_ONLINE = "online";

    public RestoreFileImpl(String absolutePath, String[] details)
        throws SamFSException {

        // parse file details
        fileDetails = details[0];
        Properties props = ConversionUtil.strToProps(fileDetails);
        prot  = props.getProperty(KEY_PROT);
        if (prot != null) {// inspect protection mask
	    if (prot.charAt(0) == 'd') {
                this.isDirectory = true;
            }
        }
        size  = props.getProperty(KEY_SIZE);
        user  = props.getProperty(KEY_USER);
        group = props.getProperty(KEY_GROUP);
        crTimeStr = props.getProperty(KEY_CRTIME);
	if (crTimeStr != null) {
	    crTime = new GregorianCalendar();
	    crTime.setTimeInMillis(ConversionUtil.strToLongVal(crTimeStr));
        }
	modTimeStr = props.getProperty(KEY_MODTIME);
	if (modTimeStr != null) {
	    modTime = new GregorianCalendar();
	    modTime.setTimeInMillis(ConversionUtil.strToLongVal(modTimeStr));
        }

        isDamaged = propertyValueToBoolean(props.getProperty(KEY_DAMAGED));
        isOnline = propertyValueToBoolean(props.getProperty(KEY_ONLINE));

        archCopies = new String[details.length - 1];
        for (int copynum = 0; copynum + 1 < details.length; copynum++) {
            archCopies[copynum] = details[copynum + 1];
	}
        // Save various versions of the path
        this.absolutePath = absolutePath;
        this.restorePath = absolutePath;
        File file = new File(absolutePath);
        this.fileName = file.getName();
        this.parentPath = file.getParent();
    }

    private Boolean propertyValueToBoolean(String propertyVal) {
        if (propertyVal == null) {
            return null;
        }

        if (propertyVal.equals("1")) {
            return Boolean.TRUE;
        } else {
            return Boolean.FALSE;
        }
    }

    public boolean isDirectory() {
        return this.isDirectory;
    }
    public String getParentPath() {
        return this.parentPath;
    }
    public String getFileName() {
        return this.fileName;
    }
    public String getAbsolutePath() {
        return this.absolutePath;
    }
    public String getRestorePath() {
        return this.restorePath;
    }
    public void setRestorePath(String restorePath) {
        this.restorePath = restorePath;
    }

    public String getProtection() { return prot; }
    public String getSize() { return size; }
    public String getUser() { return user; }
    public String getGroup() { return group; }
    public GregorianCalendar getCreationTime() { return crTime; }
    public GregorianCalendar getModTime() { return modTime; }
    public Boolean getIsDamaged() { return isDamaged; }
    public Boolean getIsOnline() { return isOnline; }

    public String[] getArchCopies() {
        return archCopies;
    }

    public void setStageCopy(int copy) {
        stageCopy = copy;
    }
    public int getStageCopy() {
        return stageCopy;
    }

    public String toString() {
        String s = absolutePath + "\t" + fileDetails;
        for (int i = 0; i < archCopies.length; i++)
            if (archCopies[i] != null)
                if (archCopies[i].length() > 0)
                    s += "\n\t\t" + archCopies[i];
        return s;
    }

}
