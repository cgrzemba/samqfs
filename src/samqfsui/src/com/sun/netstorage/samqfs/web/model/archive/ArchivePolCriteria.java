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

// ident	$Id: ArchivePolCriteria.java,v 1.4 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;

/*
 * This interface maps to the ar_set_criteria structure in archive.h
 */
public interface ArchivePolCriteria {

    public ArchivePolicy getArchivePolicy();

    public void setArchivePolicy(ArchivePolicy policy);

    public int getIndex();

    public ArchivePolCriteriaProp getArchivePolCriteriaProperties();

    public ArchivePolCriteriaCopy[] getArchivePolCriteriaCopies();

    // Added in 4.6
    public void setArchivePolCriteriaCopies(ArchivePolCriteriaCopy[] copies);

    public FileSystem[] getFileSystemsForCriteria() throws SamFSException;

    // DO NOT CALL THESE METHODS ON THE SCRATCH OBJECTS
    // (like the ones used in "Add Policy Criteria" type wizards)
    public void addFileSystemForCriteria(String fileSystem)
        throws SamFSException;

    public void deleteFileSystemForCriteria(String fileSystem)
        throws SamFSException;

    /**
     * This function should be called instead of the one argument variety if
     * the caller is going to explicitly activate the archiver configuration
     * as part of another operation.
     */
    public void deleteFileSystemForCriteria(String fileSystem, boolean activate)
        throws SamFSException;

    /*
     * Throws an exception if the named file system is the
     * last one for the criteria. If no exception then the
     * criteria can be removed.
     */
    public void canFileSystemBeRemoved(String fileSystem)
        throws SamFSException;

    public boolean isLastFSForCriteria(String filesystem);
    public boolean isForDefaultPolicy();
}
