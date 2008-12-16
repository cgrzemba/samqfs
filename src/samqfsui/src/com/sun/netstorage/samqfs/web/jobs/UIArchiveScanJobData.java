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

// ident        $Id: UIArchiveScanJobData.java,v 1.3 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJobData;

public class UIArchiveScanJobData implements ArchiveScanJobData {
    private String label = null;
    private ArchiveScanJobData data = null;

    public UIArchiveScanJobData(ArchiveScanJobData data) {
        this(data, null);
    }

    public UIArchiveScanJobData(ArchiveScanJobData data, String label) {
        this.data = data;
        this.label = label;
    }

    public String getFileTypeLabel() {
        return this.label;
    }

    public void setFileTypeLabel(String label) {
        this.label = label;
    }

    // implement ArchiveScanJobData
    public int getTotalNoOfFiles() {
        return this.data.getTotalNoOfFiles();
    }

    public String getTotalConsumedSpace() {
        return this.data.getTotalConsumedSpace();
    }

    public int getNoOfCompletedFiles() {
        return this.data.getNoOfCompletedFiles();
    }

    public String getCurrentConsumedSpace() {
        return this.data.getCurrentConsumedSpace();
    }
}
