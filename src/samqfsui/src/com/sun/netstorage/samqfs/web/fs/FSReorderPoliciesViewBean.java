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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: FSReorderPoliciesViewBean.java,v 1.25 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestContext;
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
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;

/**
 *  This class is the view bean for the  FSReorderPolicies page
 */

public class FSReorderPoliciesViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSReorderPolicies";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/fs/FSReorderPolicies.jsp";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // Child View Names used to set Javascript Values
    public static final String
        CHILD_REORDER_NEWORDER_HIDDEN_FIELD = "ReorderNewOrderHiddenField";
    public static final String
        CHILD_REORDER_SAVEDIR_HIDDEN_FIELD = "ReorderSaveDirectoryHiddenField";

    public static final String CHILD_RADIO = "radio";

    // used in Javascript to determine the size of the up/down idiom
    public static final String CHILD_IDIOMSIZE = "IdiomSize";

    // used in Javascript for the UP / DOWN button
    public static final String CHILD_BUTTON = "Button";
    public static final String CHILD_STATICTEXT = "StaticText";

    public static final String NEW_ORDER = "newOrder";

    /**
     * Constructor
     */
    public FSReorderPoliciesViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_IDIOMSIZE, CCStaticTextField.class);
        registerChild(CHILD_BUTTON, CCButton.class);
        registerChild(CHILD_REORDER_NEWORDER_HIDDEN_FIELD,
            CCStaticTextField.class);
        registerChild(CHILD_REORDER_SAVEDIR_HIDDEN_FIELD,
            CCStaticTextField.class);
        registerChild(CHILD_RADIO, CCStaticTextField.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);

        registerChild(NEW_ORDER, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        if (name.equals(NEW_ORDER)) {
            return new CCHiddenField(this, name, null);
        } else if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_IDIOMSIZE)) {
            RequestContext rq = RequestManager.getRequestContext();
            HttpServletRequest httprq = rq.getRequest();
            CCStaticTextField child = new CCStaticTextField(
                this, name, (String) httprq.getParameter("size"));
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_BUTTON)) {
            TraceUtil.trace3("Exiting");
            return (new CCButton(this, name, null));
        } else if (name.equals(CHILD_REORDER_NEWORDER_HIDDEN_FIELD)) {
            ViewBean vb = getViewBean(FSArchivePoliciesViewBean.class);
            CCStaticTextField child = new CCStaticTextField(
                this,
                name,
                ((CommonTableContainerView) vb.getChild(
                    FSArchivePoliciesViewBean.CHILD_CONTAINER_VIEW)).getChild(
                        FSArchivePoliciesView.
                            CHILD_REORDER_NEWORDER_HIDDEN_FIELD).
                                getQualifiedName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_RADIO)) {
            CCStaticTextField child =
                new CCStaticTextField(
                    this, name, "FSReorderPolicies.reorderCriteria");
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_STATICTEXT)) {
            CCStaticTextField child = new CCStaticTextField(this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        // preserve request attributes
        String serverName = getServerName();
        String fsName = getFSName();

        // make sure the unreorderable string is transfered to page session
        getUnreorderableCriteriaString();
        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/FSReorderPolicyCriteriaPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private String getFSName() {
        TraceUtil.trace3("Entering");
        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        if (fsName == null) {
            fsName = RequestManager.getRequest().
                getParameter(Constants.Parameters.FS_NAME);
            setPageSessionAttribute(Constants.PageSessionAttributes.FS_NAME,
                                    fsName);
        }

        TraceUtil.trace3("Exiting");
        return fsName;
    }

    private String getUnreorderableCriteriaString() {
        String key = "fs.fs_archive_policies.unreorderable_criteria_string";
        String urc = (String)getPageSessionAttribute(key);
        if (urc == null) {
            urc = RequestManager.getRequest().getParameter("unreorderable");
            setPageSessionAttribute(key, urc);
        }

        return urc;
    }

    /**
     * Avoid error message in console_debug_log for not implementing this
     * method.
     * @param event
     * @throws javax.servlet.ServletException
     * @throws java.io.IOException
     */
    public void handleCancelRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        this.forwardTo(getRequestContext());
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String fsName = getFSName();

        String newPolicyOrder =
            (String) getDisplayFieldValue(NEW_ORDER);
        TraceUtil.trace2("Reorder string = " + newPolicyOrder);

        // e.g.Reorder string = DeleteMe:0,Data:0
        String [] reorderables = newPolicyOrder.split(",");

        // get non-reorderable criteria
        String nonReorderString = getUnreorderableCriteriaString();
        TraceUtil.trace2("noReorderString = " + nonReorderString);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            FileSystem fs =
                sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
            if (fs == null) {
                throw new SamFSException(null, -1000);
            }

            ArchivePolCriteria [] policyCriteria =
                                fs.getArchivePolCriteriaForFS();
            ArchivePolCriteria [] newPolicyCritera =
                new ArchivePolCriteria[policyCriteria.length];

            // Simply put the criteria in the right order
            for (int i = 0; i < reorderables.length; i++) {
                String [] eachCriteria = reorderables[i].split(":");
                String policyName = eachCriteria[0];
                int critNumber = Integer.parseInt(eachCriteria[1]);

                for (int j = 0; j < policyCriteria.length; j++) {
                    if (policyName.equals(
                        policyCriteria[j].getArchivePolicy().getPolicyName())
                        &&
                        critNumber == policyCriteria[j].getIndex()) {
                        newPolicyCritera[i] = policyCriteria[j];
                        break;
                    }
                }
            }

            // Copy the rest of criteria back in, default policy, etc.
            for (int i = reorderables.length; i < policyCriteria.length; i++) {
                newPolicyCritera[i] = policyCriteria[i];
            }

            LogUtil.info(
                this.getClass(),
                "handleReorderPoliciesHrefRequest",
                "Start reordering policy criteria");

            fs.reorderPolCriteria(newPolicyCritera);

            LogUtil.info(
                this.getClass(),
                "handleReorderPoliciesHrefRequest",
                "Done reordering policy criteria");

            String alertMsg =
                SamUtil.getResourceString(
                    "FSArchivePolicies.msg.reorderPolicyCriteria",
                    getFSName());
            // set feedback alert
            SamUtil.setInfoAlert(this,
                                 ALERT,
                                 "success.summary",
                                 alertMsg,
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
                setSubmitSuccessful(true);
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to reorder policy";
                errMsg = "FSArchivePolicies.error.reorder";
                errCause = ex.getMessage();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "handleReorderPoliciesHrefRequest()",
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
}
