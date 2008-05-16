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

// ident	$Id: CopyInfo.java,v 1.8 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;

/**
 * This class contains the copy information like
 * copy type, and the corresponding copy wrapper object
 * May be expanded later to include more info
 */
public class CopyInfo {

    private String copyType = new String();
    private ArchiveCopyGUIWrapper wrapper = null;

    public CopyInfo(String copyType, ArchiveCopyGUIWrapper wrapper) {

        if (copyType != null) {
            this.copyType = copyType;
        }

        if (wrapper != null) {
            this.wrapper = wrapper;
        }
    }

    public String getCopyType() {
        return copyType;
    }

    public ArchiveCopyGUIWrapper getCopyWrapper() {
        return wrapper;
    }

    public String toString() {
        NonSyncStringBuffer strBuf = new NonSyncStringBuffer();

        if (wrapper == null) {
            strBuf.append("Copy Type: ").append(
                copyType).append(" wrapper is null!");
        } else {
            strBuf.append("Copy Type: ").append(copyType).append(
                " Archive Age: ").append(
                wrapper.getArchivePolCriteriaCopy().getArchiveAge()).
                append(" buffer size: ").append(
                wrapper.getArchiveCopy().getBufferSize());
        }

        return strBuf.toString();
    }
}
