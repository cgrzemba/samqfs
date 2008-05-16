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

// ident	$Id: ArchiveScanJobDataImpl.java,v 1.9 2008/05/16 18:39:02 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.arc.job.Stats;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJobData;

public class ArchiveScanJobDataImpl implements ArchiveScanJobData {
    private int totalNoOfFiles = -1;
    private String totalConsumedSpace = null;
    private int noOfCompletedFiles = -1;
    private String currentConsumedSpace = null;

    public ArchiveScanJobDataImpl() {
    }

    public ArchiveScanJobDataImpl(int tot, String cs, int cf, String ccs) {
        this.totalNoOfFiles = tot;
        if (cs != null)
            this.totalConsumedSpace = cs;
        this.noOfCompletedFiles = cf;
        if (ccs != null)
            this.currentConsumedSpace = ccs;
    }

    public ArchiveScanJobDataImpl(Stats tot, Stats cur) {
        if (tot != null) {
            this.totalNoOfFiles = tot.numofFiles;
            this.totalConsumedSpace = (new Long(tot.size)).toString();
        }

        if (cur != null) {
            this.noOfCompletedFiles = cur.numofFiles;
            this.currentConsumedSpace = (new Long(cur.size)).toString();
        }
    }

    public int getTotalNoOfFiles() {
        return totalNoOfFiles;
    }

    public String getTotalConsumedSpace() {
        return totalConsumedSpace;
    }

    public int getNoOfCompletedFiles() {
        return noOfCompletedFiles;
    }

    public String getCurrentConsumedSpace() {
        return currentConsumedSpace;
    }
}
