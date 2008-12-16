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

// ident	$Id: FSAddPoliciesViewBean.java,v 1.20 2008/12/16 00:12:09 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import java.util.StringTokenizer;
import javax.servlet.ServletException;

/**
 * ViewBean used to display the 'File System Add Archive Policy' page
 */

public class FSAddPoliciesViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSAddPolicies";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/fs/FSAddPolicies.jsp";

    // Used for constructing the Action Table
    private static final String CHILD_CONTAINER_VIEW = "FSAddPoliciesView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // Child View Names used to set Javascript Values
    public static final String
        CHILD_COMMAND_ADD_POLICIES_HREF = "AddPoliciesHref";
    public static final String CHILD_COMMAND_CANCEL_HREF = "CancelHref";
    public static final String
        CHILD_ADD_POLICIES_HIDDEN_FIELD = "AddPoliciesHiddenField";
    private boolean error = false;

    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String ADDED_CRITERIA = "addedPolicyCriteria";

    /**
     * Constructor
     */
    public FSAddPoliciesViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_CONTAINER_VIEW, FSAddPoliciesView.class);
        registerChild(
            CHILD_COMMAND_ADD_POLICIES_HREF, CCStaticTextField.class);
        registerChild(CHILD_COMMAND_CANCEL_HREF, CCStaticTextField.class);
        registerChild(
            CHILD_ADD_POLICIES_HIDDEN_FIELD, CCStaticTextField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(ADDED_CRITERIA, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create child component
     * @param: child name: name
     * @return: child component
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering with name = " + name);

        if (name.equals(ADDED_CRITERIA)) {
            return new CCHiddenField(this, name, null);
        } else if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new FSAddPoliciesView(this, name);
        } else if (name.equals(CHILD_STATICTEXT)) {
            CCStaticTextField child = new CCStaticTextField(this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_COMMAND_ADD_POLICIES_HREF)) {
            CCStaticTextField child = null;
            ViewBean vb = getViewBean(FSArchivePoliciesViewBean.class);
            child = new CCStaticTextField(this, name,
                ((CommonTableContainerView)
                vb.getChild(FSArchivePoliciesViewBean.CHILD_CONTAINER_VIEW)).
                    getChild(FSArchivePoliciesView.CHILD_ADD_CRITERIA_HREF).
                        getQualifiedName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_COMMAND_CANCEL_HREF)) {
            CCStaticTextField child = null;
            ViewBean vb = getViewBean(FSArchivePoliciesViewBean.class);
            child = new CCStaticTextField(this, name,
                ((CommonTableContainerView)
                vb.getChild(FSArchivePoliciesViewBean.CHILD_CONTAINER_VIEW)).
                    getChild(FSArchivePoliciesView.CHILD_CANCEL_HREF).
                        getQualifiedName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_ADD_POLICIES_HIDDEN_FIELD)) {
            CCStaticTextField child = null;
            ViewBean vb = getViewBean(FSArchivePoliciesViewBean.class);
            child = new CCStaticTextField(this, name,
                ((CommonTableContainerView)
                vb.getChild(FSArchivePoliciesViewBean.CHILD_CONTAINER_VIEW)).
                    getChild(FSArchivePoliciesView.
                        CHILD_ADD_CRITERIA_HIDDEN_FIELD).getQualifiedName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {

            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Create PageTitle Model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/FSAddPolicyCriteriaPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public String getFSName() {
        TraceUtil.trace3("Entering");
        String fsName = (String)getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        if (fsName == null) {
            fsName = RequestManager.getRequest().
                getParameter(Constants.Parameters.FS_NAME);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME,
                fsName);
        }

        TraceUtil.trace3("Exiting");
        return fsName;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        String fsName = getFSName();
        String serverName = getServerName();

        FSAddPoliciesView view =
            (FSAddPoliciesView) getChild(CHILD_CONTAINER_VIEW);
        try {
            view.populateData();
        } catch (SamFSException ex) {
            error = true;
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginDisplay()",
                "Unable to populate archive policy criteria data",
                serverName);

            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FSAddPolicyCriteria.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        // disable the submit button by default.
        ((CCButton)getChild("Submit")).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fsName = getFSName();

        String policyCriteriaString =
            (String) getDisplayFieldValue(ADDED_CRITERIA);
        TraceUtil.trace2("Got new policy criteria: " + policyCriteriaString);

        StringTokenizer tokens = new StringTokenizer(policyCriteriaString, ",");
        int count = tokens.countTokens();
        String[] policyNames = new String[count];
        int[] criteriaNumbers = new int[count];
        for (int i = 0; i < count; i++) {
            String token = tokens.nextToken();
            int index = token.indexOf(':');
            policyNames[i] = token.substring(0, index);
            criteriaNumbers[i] = Integer.parseInt(token.substring(index + 1));
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            FileSystem fs =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            ArchivePolCriteria[] policyCriteria = new ArchivePolCriteria[count];
            for (int i = 0; i < count; i++) {
                ArchivePolicy policy =
                    sysModel.getSamQFSSystemArchiveManager().getArchivePolicy(
                        policyNames[i]);
                if (policy == null) {
                    throw new SamFSException(null, -1001);
                }
                policyCriteria[i] = policy.getArchivePolCriteria(
                        criteriaNumbers[i]);

                if (policyCriteria[i] == null) {
                    throw new SamFSException(null, -1001);
                }
            }

            LogUtil.info(
                this.getClass(),
                "handleAddCriteriaHrefRequest",
                "Start adding policy criteria");

            fs.addPolCriteria(policyCriteria);

            LogUtil.info(
                this.getClass(),
                "handleAddCriteriaHrefRequest",
                "Done adding policy criteria");

            // set feedback alert
            SamUtil.setInfoAlert(this,
                                 ALERT,
                                 "success.summary",
                                 "FSArchivePolicies.msg.addPolicyCriteria",
                                 serverName);

            setSubmitSuccessful(true);
        } catch (SamFSException ex) {
            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;

            // catch exceptions where the archiver.cmd has errors
            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
            } else if (ex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
                setSubmitSuccessful(true);
            } else {
                processMsg = "Failed to add policy criteria";
                errMsg = "FSArchivePolicies.error.addPolicyCriteria";
                errCause = ex.getMessage();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "handleAddCriteriaHrefRequest()",
                processMsg,
                serverName);

            if (!warning) {
                SamUtil.setErrorAlert(this,
                                      ALERT,
                                      errMsg,
                                      ex.getSAMerrno(),
                                      errCause,
                                      serverName);
            } else {
                SamUtil.setWarningAlert(this,
                                        ALERT,
                                        errMsg,
                                        errCause);
            }
        }
        forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }


    /**
     * implement a handler for the cancel button. This will prevent an
     * exception in a case where the user launches the popup and clicks on the
     * cancel button without performing any other action
     */
    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ModelControlException, ServletException {
        forwardTo(getRequestContext());
    }
}
