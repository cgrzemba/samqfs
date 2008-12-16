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

// ident	$Id: ArchivePolicy.java,v 1.11 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;

public interface ArchivePolicy {
    // constants for pre-defined policy names
    public static final String POLICY_NAME_ALLSETS = "allsets";
    public static final String POLICY_NAME_NOARCHIVE = "no_archive";

    // static ints for join methods
    public static final int JOIN_NOT_SET = 0;
    public static final int NO_JOIN = 1;
    public static final int JOIN_PATH = 2;

    // static ints for offline copy methods
    public static final int OC_NOT_SET = 3;
    public static final int OC_NONE = 4;
    public static final int OC_DIRECT = 5;
    public static final int OC_STAGEAHEAD = 6;
    public static final int OC_STAGEALL = 7;

    // static ints for sort method
    public static final int SM_NOT_SET = 8;
    public static final int SM_NONE = 9;
    public static final int SM_AGE = 10;
    public static final int SM_PATH = 11;
    public static final int SM_PRIORITY = 12;
    public static final int SM_SIZE = 13;

    // static ints for reservation
    public static final int RES_FS = 14;
    public static final int RES_POLICY = 15;
    public static final int RES_DIR = 16;
    public static final int RES_USER = 17;
    public static final int RES_GROUP = 18;

    // static ints for unarchive age time reference
    public static int UNARCH_TIME_REF_ACCESS = 11;
    public static int UNARCH_TIME_REF_MODIFICATION = 12;

    // static ints for stage options in criteria
    public static final int STAGE_ASSOCIATIVE = 10001;
    public static final int STAGE_NEVER = 10002;
    public static final int STAGE_DEFAULTS = 10003;
    public static final int STAGE_NO_OPTION_SET = 10004;

    // static ints for release options in criteria
    public static final int RELEASE_NEVER = 10005;
    public static final int RELEASE_PARTIAL = 10006;
    public static final int RELEASE_AFTER_ONE = 10007;
    public static final int RELEASE_DEFAULTS = 10008;
    public static final int RELEASE_NO_OPTION_SET = 10009;

    // getters
    public String getPolicyName();

    public short getPolicyType();

    public String getPolicyDescription();

    public void setPolicyDescription(String description);

    public ArchivePolCriteria[] getArchivePolCriteria();

    public ArchivePolCriteria getArchivePolCriteria(int index);

    // The returned ArchivePolCriteria should be used for adding
    // an ArchivePolCriteria to a policy
    public ArchivePolCriteria getDefaultArchivePolCriteriaForPolicy();

    // 4.6+
    // The returned ArchivePolCriteria should be used for adding
    // an ArchivePolCriteria to a policy with class name
    public ArchivePolCriteria getDefaultArchivePolCriteriaForPolicy(
            String className, String description);

    public void addArchivePolCriteria(ArchivePolCriteria criteria,
                                      String[] fsNames)
        throws SamFSException;

    public void deleteArchivePolCriteria(int criteriaIndex)
        throws SamFSException;

    public ArchiveCopy[] getArchiveCopies();

    public ArchiveCopy getArchiveCopy(int copyNo);

    public void addArchiveCopy(ArchiveCopyGUIWrapper copy)
        throws SamFSException;

    public void deleteArchiveCopy(int copyNo) throws SamFSException;

    /**
     * retrieve the SamQFSSystemModel currently in use
     */
    public SamQFSSystemModel getModel();

    // this method needs to be called when any property is updated by calling
    // the setters
    public void updatePolicy() throws SamFSException;

    public ArchivePolCriteria[] getArchivePolCriteriaForFS(String fsName);
}
