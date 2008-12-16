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

// ident	$Id: RemoteFile.java,v 1.12 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.io.File;
import java.util.Properties;

public class RemoteFile extends File {

    protected boolean isDirectory;
    protected long lastModifiedTime; // in seconds
    protected long size;
    protected String linkTarget;

    public RemoteFile(String name) {
        super(name);
    }

    public RemoteFile(String name,
                      long size,
                      long modifiedTime, /* in seconds */
                      boolean isDir) {
        super(name);
        this.size = size;
        this.lastModifiedTime = modifiedTime;
        this.isDirectory = isDir;
    }

    void setLinkTarget(String targetPath) {
        this.linkTarget = targetPath;
    }
    // file types. must return file status values in restore.h
    static final int TYPE_MISSING = 0x0;
    static final int TYPE_REGFILE = 0x1;
    static final int TYPE_DIRFILE = 0x2;
    static final int TYPE_RELFILE = 0x3;
    static final int TYPE_NOTFILE = 0x4; // special file
    // file properties
    static final String KEY_SIZE = "size";
    static final String KEY_CREATED  = "created";
    static final String KEY_MODIFIED = "modified";
    static final String KEY_LINKTARGET = "target";
    public RemoteFile(String name, int status, Properties fileProps)
        throws SamFSException {
        this(name,
             ConversionUtil.strToLongVal(fileProps.getProperty(KEY_SIZE)),
             ConversionUtil.strToLongVal(fileProps.getProperty(KEY_MODIFIED)),
             status == TYPE_DIRFILE);
        setLinkTarget(fileProps.getProperty(KEY_LINKTARGET));
    }

    public long length() {
        return size;
    }

    /** return the last time the file was modified in seconds */
    public long lastModified() {
        return lastModifiedTime;
    }

    public boolean isDirectory() {
        return isDirectory;
    }

    public boolean isFile() {
        return (!isDirectory);
    }

    public String getLinkTarget() {
        return linkTarget;
    }

    public String toString() {
        String s = getName() + (isDirectory() ? "/" : "");
        s += ((linkTarget == null) ? "" : ("-> " + linkTarget)) + "\t" + size;
        return s;
    }

}
