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

// ident	$Id: CopySettingsTiledView.java,v 1.11 2008/05/16 19:39:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import javax.servlet.http.HttpServletRequest;

/**
 * CopySettingsTiledView
 *
 * handler for:
 *     - default policy : policy details
 *     - custom policy : criteria details
 */

public class CopySettingsTiledView extends CriteriaCopiesTiledViewBase {
    public static final String CRITERIA_COPIES =
        "request_scoped_criteria_copies";

    public CopySettingsTiledView(View parent,
                                 CCActionTableModel model,
                                 String name) {
        super(parent, model, name);
    }

    /**
     * retrieve the criteria copy information for this criteria
     */
    public ArchivePolCriteriaCopy [] getCriteriaCopies() {
        HttpServletRequest request =
            RequestManager.getRequestContext().getRequest();
        ArchivePolCriteriaCopy [] copies =
            (ArchivePolCriteriaCopy [])request.getAttribute(CRITERIA_COPIES);

        if (copies != null)
            return copies;

        // get  the policy
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = (String)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        Integer policyType = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.POLICY_TYPE);

        if (criteriaNumber == null &&
            (policyType.shortValue()) == ArSet.AR_SET_TYPE_DEFAULT) {

            // default to the only criteria of this policy
            criteriaNumber = new Integer(0);
            parent.setPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER,
                                           criteriaNumber);
        }

        // get the criteria
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(criteriaNumber.intValue());
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

    /**
     * override the beginReleaseOptionsTextDisplay() to keep wizard working
     * TODO: Eventually, the new criteria wizard & criteria details page
     * should be changed to use the same scheme.
     */
    public boolean beginReleaseOptionsTextDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        int index = model.getRowIndex();
        ArchivePolCriteriaCopy [] criteriaCopies = getCriteriaCopies();

        CCDropDownMenu field = (CCDropDownMenu)getChild(RELEASE_OPTIONS);

        // determine if release options is set
        int releaseOptions = PolicyUtil.ReleaseOptions.SPACE_REQUIRED;
        if (criteriaCopies[index].isNoRelease())
            releaseOptions += PolicyUtil.ReleaseOptions.WAIT_FOR_ALL;

        if (criteriaCopies[index].isRelease())
            releaseOptions += PolicyUtil.ReleaseOptions.IMMEDIATELY;

        field.setValue(Integer.toString(releaseOptions));
        TraceUtil.trace3("Exiting");

        return true;
    }
}
