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

// ident	$Id: RunSAMExplorerViewBean.java,v 1.11 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Run SAM Explorer Pop Up Page.
 */

public class RunSAMExplorerViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "RunSAMExplorer";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/admin/RunSAMExplorer.jsp";

    public static final String STATIC_TEXT  = "StaticText";
    public static final String RADIO_BUTTON = "RadioButton";
    public static final String TEXTFIELD = "TextField";
    public static final String ERROR_MESSAGE = "ErrorMessage";
    public static final String CONFIRM_MESSAGE = "ConfirmMessage";


    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // For Radio Button Group (Hardcoded string, no need for L10N)
    private static OptionList options = new OptionList(
        new String [] {"/tmp", "/var/tmp"}, // label
        new String [] {"/tmp", "/var/tmp"}); // value


    /**
     * Constructor
     */
    public RunSAMExplorerViewBean() {
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
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(RADIO_BUTTON, CCRadioButton.class);
        registerChild(TEXTFIELD, CCTextField.class);
        registerChild(ERROR_MESSAGE, CCHiddenField.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
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
        TraceUtil.trace3(new StringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(STATIC_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(RADIO_BUTTON)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(options);
            child = myChild;
        } else if (name.equals(ERROR_MESSAGE)) {
            child = new CCHiddenField(this, name,
                SamUtil.getResourceString("RunSAMExplorer.error.linenumber"));
        } else if (name.equals(CONFIRM_MESSAGE)) {
            child = new CCHiddenField(this, name,
                SamUtil.getResourceString("RunSAMExplorer.confirm.run"));
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);

        // Preserve Server Name
        String serverName = getServerName();

        // Pre-select /var/tmp if nothing is selected
        CCRadioButton radio = (CCRadioButton) getChild(RADIO_BUTTON);
        if (radio.getValue() == null) {
            radio.setValue("/var/tmp");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate the page title model
     *
     * @return CCPageTitleModel The page title model of RunSAMExplorer Page
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/admin/RunSAMExplorerPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String selection = (String) getDisplayFieldValue(RADIO_BUTTON);
        int includedLines = Integer.parseInt(
            ((String) getDisplayFieldValue(TEXTFIELD)).trim());

        try {
	    SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            long jobID = sysModel.runSamExplorer(selection, includedLines);

            BaseJob job = null;
            if (jobID > 0) {
                // Job may have been very short lived and may already be done.
                job = sysModel.getSamQFSSystemJobManager().getJobById(jobID);
            }
            SamUtil.setInfoAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
		jobID <= 0 ?
		    SamUtil.getResourceString(
			"RunSAMExplorer.success.generatereport",
			new String [] {selection}) :
                    SamUtil.getResourceString(
			"RunSAMExplorer.success.generatereportwithjobid",
			new String [] {selection, String.valueOf(jobID)}),
                getServerName());

            setSubmitSuccessful(true);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest()",
                "Failed to generate SAM Explorer report",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "RunSAMExplorer.error.generatereport",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }
}
