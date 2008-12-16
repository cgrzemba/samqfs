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

// ident	$Id: ArchiveCopyGUIWrapperImpl.java,v 1.5 2008/12/16 00:12:20 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;

public class ArchiveCopyGUIWrapperImpl implements ArchiveCopyGUIWrapper {

    private ArchivePolCriteriaCopyImpl archPolCriteriaCopy = null;
    private ArchiveCopyImpl archCopy = null;

    public ArchiveCopyGUIWrapperImpl() {
        ArchiveVSNMapImpl map = new ArchiveVSNMapImpl();

        archCopy = new ArchiveCopyImpl(null, map, -1, null);
        map.setArchiveCopy(archCopy);

        archPolCriteriaCopy = new ArchivePolCriteriaCopyImpl(null, null, -1);
    }

    public ArchivePolCriteriaCopy getArchivePolCriteriaCopy() {
        return archPolCriteriaCopy;
    }

    public ArchiveCopy getArchiveCopy() {
        return archCopy;
    }
}
