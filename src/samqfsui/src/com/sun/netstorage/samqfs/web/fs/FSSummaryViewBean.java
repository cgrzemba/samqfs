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

// ident	$Id: FSSummaryViewBean.java,v 1.26 2008/09/03 19:46:03 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;

/**
 *  This class is the view bean for the File Systems Summary page
 */

public class FSSummaryViewBean extends CommonViewBeanBase {

    // Page information...
    public static final String PAGE_NAME = "FSSummary";
    private static final String DEFAULT_DISPLAY_URL = "/jsp/fs/FSSummary.jsp";

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW = "FileSystemSummaryView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    public static final String CHILD_CONFIRM_MSG1 = "ConfirmMsg1";
    public static final String CHILD_CONFIRM_MSG2 = "ConfirmMsg2";

    // Hidden field - samfs license type
    public static final String
        CHILD_HIDDEN_LICENSE_TYPE = "LicenseTypeHiddenField";

    // Used for HtmlUnit, top.serverName not available in non-frame setup
    public static final String SERVER_NAME = "ServerName";

    /**
     * Constructor
     */
    public FSSummaryViewBean() {
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
        registerChild(CHILD_CONTAINER_VIEW, FileSystemSummaryView.class);
        registerChild(CHILD_HIDDEN_LICENSE_TYPE, CCHiddenField.class);
        registerChild(CHILD_CONFIRM_MSG1, CCHiddenField.class);
        registerChild(CHILD_CONFIRM_MSG2, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
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
        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            FileSystemSummaryView child =
                new FileSystemSummaryView(this, name, getServerName());
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_HIDDEN_LICENSE_TYPE) ||
                   name.equals(SERVER_NAME)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_CONFIRM_MSG1)
                    || name.equals(CHILD_CONFIRM_MSG2)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
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
     * Create the pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        CCPageTitleModel model =
            PageTitleUtil.createModel("/jsp/fs/FSSummaryPageTitle.xml");
        model.setPageTitleText("FSSummary.title");
        TraceUtil.trace3("Exiting");
        return model;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        CCHiddenField field = (CCHiddenField) getChild(CHILD_CONFIRM_MSG1);
        field.setValue(SamUtil.getResourceString("FSSummary.confirmMsg1"));

        String serverName = getServerName();

        // set samfs license type hidden field
        String licenseTypeString = "SAMQFS";
        switch (SamUtil.getSystemType(serverName)) {
            case SamQFSSystemModel.SAMFS:
                licenseTypeString = "SAMFS";
                break;
            case SamQFSSystemModel.SAMQFS:
                licenseTypeString = "SAMQFS";
                break;
            case SamQFSSystemModel.QFS:
            default:
                licenseTypeString = "QFS";
                break;
        }

        ((CCHiddenField)
            getChild(CHILD_HIDDEN_LICENSE_TYPE)).setValue(licenseTypeString);

        field = (CCHiddenField)getChild(CHILD_CONFIRM_MSG2);
        field.setValue(
            new NonSyncStringBuffer(
                SamUtil.getResourceString("FSSummary.confirmMsg2")).
                append("\n").
                append(SamUtil.getResourceString(
                    "FSSummary.confirmMsg.unshareDir")).
                toString());
        try {
            // populate the AT's model data here, for error handling
            FileSystemSummaryView view =
                (FileSystemSummaryView) getChild(CHILD_CONTAINER_VIEW);
            view.populateData();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginDisplay()",
                "Unable to populate FS Summary table",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSSummary.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        // remove if HtmlUnit is out of the picture
        ((CCHiddenField) getChild(SERVER_NAME)).setValue(getServerName());

        TraceUtil.trace3("Exiting");
    }

    /* Overrides the getServerName in CommonViewBeanBase */
    public String getServerName() {
        // Grab the server name from the Request.  DO NOT REMOVE THIS LINE.
        // This is critical to grab the server name and save it as a page
        // session attribute as CommonViewBeanBase.getServerName() will not
        // return the change server drop down value yet (Navigation frame may
        // not loaded completely yet when the content frame is loaded)
        // SAVE the server name in beginDisplay().

        String serverName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        if (serverName != null) {
            return serverName;
        }

        // Server name is not available in the page session, retrieve it from
        // the request and save it in page session
        serverName = RequestManager.getRequestContext().getRequest().
            getParameter(Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
        return serverName;
    }
}
