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

// ident	$Id: StageJobFileDataImpl.java,v 1.9 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;

public class StageJobFileDataImpl implements StageJobFileData {
    private String fileName = null;
    private String fileSizeInBytes = null;
    private String position = null;
    private String offset = null;
    private String vsn = null;
    private String user = null;

    public StageJobFileDataImpl() {
    }

    public StageJobFileDataImpl(String fileName,
                                String fileSizeInBytes,
                                String position,
                                String offset,
                                String vsn,
                                String user) {
        if (fileName != null)
            this.fileName = fileName;

        if (fileSizeInBytes != null)
            this.fileSizeInBytes = fileSizeInBytes;

        if (position != null)
            this.position = position;

        if (offset != null)
            this.offset = offset;

        if (vsn != null)
            this.vsn = vsn;

        if (user != null)
            this.user = user;
    }

    public String getFileName() {
        return fileName;
    }

    public String getFileSizeInBytes() {
        return fileSizeInBytes;
    }

    public String getPosition() {
        return position;
    }

    public String getOffset() {
        return offset;
    }

    public String getVSN() {
        return vsn;
    }

    public String getUser() {
        return user;
    }
}
