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

// ident	$Id: BufferDirectiveImpl.java,v 1.4 2008/05/16 18:39:01 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

public class BufferDirectiveImpl implements BufferDirective {

    private BufDirective buf = null;
    private int mediaType = -1;
    private long size = -1;
    private boolean lock = false;

    // TBD
    // unit of size needs to be fixed
    public BufferDirectiveImpl() {
    }

    public BufferDirectiveImpl(int mediaType, long size, boolean lock) {
        /* check validity of media type. TBD. */

        this.mediaType = mediaType;
        this.size = size;
        this.lock = lock;

        buf = new BufDirective(SamQFSUtil.getMediaTypeString(mediaType),
                               String.valueOf(size),
                               lock);
    }

    public BufferDirectiveImpl(BufDirective buf) {

        this.buf = buf;
        if (buf != null) {
            this.mediaType =
                SamQFSUtil.getMediaTypeInteger(buf.getMediaType());
            try {
                String jniSize = buf.getSize();
                if (SamQFSUtil.isValidString(jniSize)) {
                    this.size = Long.parseLong(jniSize);
                }
            } catch (NumberFormatException ne) {
                SamQFSUtil.doPrint(buf.getSize());
                SamQFSUtil.doPrint("Unit needs to be fixed.");
                this.size = -1;
                // TBD. This value so that I remember
            }
            this.lock = buf.isLock();
        }
    }

    public BufDirective getJniBufferDirective() {
        return buf;
    }

    public int getMediaType() {
        return mediaType;
    }

    public long getSize() {
        return size;
    }

    public void setSize(long size) {
        this.size = size;
        if (buf != null) {
            if (size != -1)
                buf.setSize(String.valueOf(size));
            else
                buf.resetSize();
        }
    }

    public String getSizeString() {
        String sizeStr = new String();
        if (buf != null) {
            String jniSize = buf.getSize();
            if (SamQFSUtil.isValidString(jniSize))
                sizeStr = jniSize;
        }

        return sizeStr;
    }

    public void setSizeString(String sizeStr) {
        if (SamQFSUtil.isValidString(sizeStr)) {

            this.size = -1;
            try {
                this.size = Long.parseLong(sizeStr);
            } catch (Exception e) {
            }
            if (buf != null)
                buf.setSize(sizeStr);

        } else {
            this.size = -1;
            if (buf != null)
                buf.resetSize();
        }
    }

    public boolean isLocked() {
        return lock;
    }

    public void setLocked(boolean lock) {
        this.lock = lock;
        if (buf != null)
            buf.setLock(lock);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Media Type: ")
            .append(mediaType)
            .append("\n")
            .append("Size: ")
            .append(size)
            .append("\n")
            .append("Locked: ")
            .append(lock)
            .append("\n");

        return buf.toString();
    }
}
