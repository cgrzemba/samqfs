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

// ident	$Id: NewReportViewBean.java,v 1.11 2008/05/16 19:39:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.Option;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Report;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for create media report
 */

public class NewReportViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME   = "NewReport";
    private static final String DISPLAY_URL = "/jsp/admin/NewReport.jsp";

    // Pagelet view for providing user options to generate report content
    public static final String CHILD_NEW_REPORT_VIEW    = "NewReportView";
    public static final String CHILD_REPORT_TYPE_RADIO  = "reportTypeRadio";
    // href to handle 'onClick events of radiobutton
    public static final
        String CHILD_REPORT_TYPE_RADIO_HREF = "reportTypeRadioHref";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    public NewReportViewBean() {
        super(PAGE_NAME, DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();

        // preserve server name in PopUp
        String serverName = getServerName();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_REPORT_TYPE_RADIO, CCRadioButton.class);
        registerChild(CHILD_NEW_REPORT_VIEW, NewReportView.class);
        registerChild(CHILD_REPORT_TYPE_RADIO_HREF, CCHref.class);

        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_NEW_REPORT_VIEW)) {
            child = new NewReportView(this, name);
        } else if (name.equals(CHILD_REPORT_TYPE_RADIO_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_REPORT_TYPE_RADIO)) {
            CCRadioButton radioChild = new CCRadioButton(this, name, null);
            child = radioChild;
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);
        String reportType = (String)
            ((CCRadioButton) getChild(CHILD_REPORT_TYPE_RADIO)).getValue();
        // hide options depending upon the configuration
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            boolean hideAcslsReportOption = true;
            // if no archiving fs are mounted, disable the fs option
            SamQFSSystemFSManager fsManager =
                sysModel.getSamQFSSystemFSManager();

            // if no archiving media is configured, disable the media option
            Library libs[] =
                sysModel.getSamQFSSystemMediaManager().getAllLibraries();
            for (int j = 0; j < libs.length; j++) {
                if (libs[j].getDriverType() == Library.ACSLS) {
                    hideAcslsReportOption = false;
                    break; // no need to continue
                }
            }

            OptionList optionList = new OptionList();
            optionList.add(
                new Option(
                    "reports.type.media",
                    "reports.type.desc.media"));
            if (!hideAcslsReportOption) {
                optionList.add(
                    new Option(
                        "reports.type.acsls",
                        "reports.type.desc.acsls"));
            }

            ((CCRadioButton) getChild(
                CHILD_REPORT_TYPE_RADIO)).setOptions(optionList);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to get the configuration",
                getServerName());

            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "reports.create.msg.unabletodisplayoptions"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        if (reportType == null) { // no radio selection
            ((CCRadioButton) getChild(CHILD_REPORT_TYPE_RADIO)).
                setValue("reports.type.desc.media");
        }
        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/util/CommonPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void handleReportTypeRadioHrefRequest(RequestInvocationEvent event)
    {
        TraceUtil.trace3("Entering");

        // Save selection in Page session
        CCRadioButton radio = (CCRadioButton) getChild(CHILD_REPORT_TYPE_RADIO);
        String reportType = (String) radio.getValue();
        if (reportType != null) {
            setPageSessionAttribute(
                Constants.PageSessionAttributes.REPORT_TYPE, reportType);
            TraceUtil.trace1("Getting report type: " + reportType);
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        CCRadioButton radio = (CCRadioButton) getChild(CHILD_REPORT_TYPE_RADIO);
        String reportType = (String) radio.getValue();
        NewReportView view = (NewReportView) getChild(CHILD_NEW_REPORT_VIEW);

        try {
            if (reportType != null && reportType.trim().length() > 0) {

                if ("reports.type.desc.media".equals(reportType)) {
                    view.submitMediaReportRequest();
                } else if ("reports.type.desc.acsls".equals(reportType)) {
                    view.submitAcslsReportRequest();
                } else if ("reports.type.desc.fs".equals(reportType)) {
                    SamQFSSystemModel sysModel =
                        SamUtil.getModel(getServerName());
                    sysModel.getSamQFSSystemAdminManager().
                        createReport(Report.TYPE_FS, 0);
                }
            }
            SamUtil.setInfoAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "reports.create.msg.success"),
                getServerName());

            setPageSessionAttribute(
                Constants.PageSessionAttributes.PATH_NAME, null);

            setSubmitSuccessful(true);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to send request to create media report",
                getServerName());

            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "reports.create.msg.failed"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        forwardTo(getRequestContext());
    }
}
