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

// ident	$Id: DriveDirectiveImpl.java,v 1.3 2008/03/17 14:43:48 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.DrvDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.media.Library;

public class DriveDirectiveImpl implements DriveDirective {

    private DrvDirective drv = null;
    private SamQFSSystemModelImpl model = null;
    private Library library = null;
    private int count = -1;

    public DriveDirectiveImpl(SamQFSSystemModelImpl model) {
        this.model = model;
    }

    public DriveDirectiveImpl(SamQFSSystemModelImpl model,
                              Library library,
                              int count) {
        this.model = model;
        this.library = library;
        this.count = count;

        if (library != null) {
            // TBD
            // Remove the try-catch block after fixing Library interface
            try {
                this.drv = new DrvDirective(library.getName(), count);
            } catch (Exception removeIt) {}
        }
    }

    public DriveDirectiveImpl(SamQFSSystemModelImpl model,
                              DrvDirective drv) throws SamFSException {

        this.drv = drv;
        if (drv != null)
            count = drv.getCount();
    }

    public DrvDirective getJniDriveDirective() {
        return drv;
    }

    public String getLibraryName() {
        String name = new String();
        if (drv != null)
            name = drv.getAutoLib();

        return name;
    }

    public Library getLibrary() throws SamFSException {
        if ((model != null) && (drv != null))
            library = model.getSamQFSSystemMediaManager()
                .getLibraryByName(drv.getAutoLib());

        return library;
    }

    public void setLibrary(Library library) {
        this.library = library;
    }

    public int getCount() {
        return count;
    }

    public void setCount(int count) {
        this.count = count;
        if (drv != null) {
            if (count != -1)
                drv.setCount(count);
            else
                drv.resetCount();
        }
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        if (library != null) {
            try {
                buf.append("Library: ")
                    .append(library.getName())
                    .append("\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            buf.append("Library: ")
                .append(getLibraryName());
        }

        buf.append("Count: ")
            .append(count)
            .append("\n");
        return buf.toString();
    }
}
