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

// ident	$Id: PolicySummaryViewBean.java,v 1.9 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;



/**
 * the policy summary page
 */
public class PolicySummaryViewBean extends CommonViewBeanBase {
    // children
    public static final String CONTAINER_VIEW = "PolicySummaryView";
    public static final String PAGE_TITLE = "PageTitle";
    public static final String POLICY_TYPES = "policyTypes";
    public static final String POLICY_NAMES = "policyNames";
    public static final String POLICY_NAME = "policyToDelete";

    public static final String POLICY_DELETE_CONFIRMATION =
        "policyDeleteConfirmation";

    private static final String DEFAULT_URL =
        "/jsp/archive/PolicySummary.jsp";
    private static final String PAGE_NAME = "PolicySummary";

    /** Creates a new instance of PolicySummaryViewBean */
    public PolicySummaryViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        // initialize tracing
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * register this viewbean's children
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(POLICY_TYPES, CCHiddenField.class);
        registerChild(POLICY_NAMES, CCHiddenField.class);
        registerChild(POLICY_NAME, CCHiddenField.class);
        registerChild(POLICY_DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(CONTAINER_VIEW, PolicySummaryView.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * create the named child
     */
    public View createChild(String name) {
        if (name.equals(POLICY_TYPES) ||
            name.equals(POLICY_NAMES) ||
            name.equals(POLICY_NAME) ||
            name.equals(POLICY_DELETE_CONFIRMATION)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(CONTAINER_VIEW)) {
            return new PolicySummaryView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw
                new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * begin processing the PolicySummary.jsp jsp
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        CCHiddenField child =
            (CCHiddenField)getChild(POLICY_DELETE_CONFIRMATION);
        child.setValue(
            SamUtil.getResourceString("archiving.policy.delete.confirm"));

        // we need to popuate the model here so that alert messages can show
        PolicySummaryView view =
            (PolicySummaryView)getChild(CONTAINER_VIEW);

        view.populateTableModel();
    }
}
