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

// ident	$Id: MultiHostStatusDisplayBean.java,v 1.1 2008/07/16 23:47:28 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.util.JSFUtil;
import java.util.ArrayList;
import java.util.List;
import com.sun.netstorage.samqfs.web.model.fs.MultiHostStatus;

/** add clients */
public class MultiHostStatusDisplayBean {
    
    private int total, succeeded, failed, pending;
    private String selectedHost;
    private MultiHostStatus status = null;

    public MultiHostStatusDisplayBean() {

        this.status = initializeStatus(getCurrentJobId());
    }

    /** return the current job */
    private long getCurrentJobId() {
        return 0;
    }

    /** return the MultiHostStatus object for the current job */
    private MultiHostStatus initializeStatus(long jobId) {
        return null;
    }

    // resource strings
    public String getTitleText() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.title");
    }
    
    public String getFailedHostLabel() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.failedlabel");
    }
    
    public String getHostErrorLabel() {
        return JSFUtil.getMessage("fs.multihoststatus.addclients.hosterrorlabel");
    }
    
    // status data
    public int getTotal() {
        return total;
    }
    
    public int getSucceeded() {
        return succeeded;
    }
    
    public int getFailed() {
        return failed;
    }
    
    public int getPending() {
        return pending;
    }
    
    public List<String> getFailedHostList() {
        return new ArrayList<String>();
    }
    
    public String getHostErrorDetails() {
        return "";
    }
    
    public String getSelected() {
        return selectedHost;
    }
    
    public void setSelectedHost(String host) {
        selectedHost = host;
    }
}
