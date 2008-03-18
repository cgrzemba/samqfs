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

// ident	$Id: CriteriaDetailsCopyTiledView.java,v 1.7 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;
import javax.servlet.http.HttpServletRequest;


public class CriteriaDetailsCopyTiledView extends CriteriaCopiesTiledViewBase {
    public static final String CRITERIA_COPIES =
        "request_scoped_criteria_copies";
    public CriteriaDetailsCopyTiledView(View parent,
                                        CCActionTableModel model,
                                        String name) {
        super(parent, model, name);
    }

    /**
     * retrieve the criteria copy objects
     */
    public ArchivePolCriteriaCopy [] getCriteriaCopies() {
        HttpServletRequest request =
            RequestManager.getRequestContext().getRequest();
        ArchivePolCriteriaCopy [] copies =
            (ArchivePolCriteriaCopy [])request.getAttribute(CRITERIA_COPIES);

        if (copies != null)
            return copies;

        // parent & server name
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = (String)parent.getServerName();

        // get  the policy
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaIndex = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);
        int index = criteriaIndex.intValue();

        // get the criteria
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(index);
            copies = criteria.getArchivePolCriteriaCopies();
            request.setAttribute(CRITERIA_COPIES, copies);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "getCriteriaCopies",
                                     "Unable to load policy criteria copies",
                                    serverName);
        }

        // return the criteria copies
        return copies;
    }
}
