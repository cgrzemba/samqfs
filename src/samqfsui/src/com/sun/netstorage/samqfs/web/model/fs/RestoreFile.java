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

// ident	$Id: RestoreFile.java,v 1.11 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import java.util.GregorianCalendar;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * This interface represents a file whose inode will be restored and
 * which will optionally be staged.
 */
public interface RestoreFile {


    public boolean isDirectory();


    /**
     * Returns fully qualified path, starting with root
     * This is getParentPath and getFileName combined.
     */
    public String getAbsolutePath();

    /**
     * @return the directory path which contains the file indicated in
     * getFileName() or the directory one level up from the directory in
     * absolute path.  Basically, you take the directory in absolute path and
     * you lopp off the last part after the final  "/" and return that.
     */
    public String getParentPath();

    /**
     * For files, returns the file name sans directory path.
     * For directories, returns the last directory in the path.
     */
    public String getFileName();

    public String getProtection();
    public String getSize(); // bytes
    public String getUser();
    public String getGroup();
    public GregorianCalendar getCreationTime();
    public GregorianCalendar getModTime();
    public Boolean getIsDamaged();
    public Boolean getIsOnline();
    public String getRestorePath();
    public void setRestorePath(String path);


    // The return array will always have length 4.  Some elements may be null.
    public String[] getArchCopies();

    /**
     *  specify which copy (if any) should be staged
     * @param copy 0-3 = stage this copy,
     *   SYS_STG_COPY = let the system pick the copy
     *   NO_STG_COPY  =  dont'stage file
     *   STG_COPY_ASINDUMP = stage if the file is marked as online in dump
     */
    public void setStageCopy(int copy);
    public int getStageCopy();
    // these constants must match those define in restore.h
    public static final int SYS_STG_COPY = 1000;
    public static final int NO_STG_COPY  = 2000;
    public static final int STG_COPY_ASINDUMP = 3000;
}
