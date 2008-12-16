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

// ident	$Id: NewReportView.java,v 1.11 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Report;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.propertysheet.CCPropertySheet;



/**
 *  This class is the view bean for create media report
 */

public class NewReportView extends RequestHandlingViewBase
    implements CCPagelet {

    private String reportType = null;

    private static final String PROPERTY_SHEET_MEDIA = "PropertySheet";
    private static final String PROPERTY_SHEET_ACSLS = "PropertySheet2";

    // Page Title Attributes and Components.
    private CCPropertySheetModel pSheetModel1 = null;
    private CCPropertySheetModel pSheetModel2 = null;

    public NewReportView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // for the Media options
        pSheetModel1 = new CCPropertySheetModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/admin/NewMediaReportPropertySheet.xml");
        // recommended default options
        ((CCCheckBox) getChild("check1")).setChecked(true);
        ((CCCheckBox) getChild("check2")).setChecked(true);
        ((CCCheckBox) getChild("check5")).setChecked(true);
        ((CCCheckBox) getChild("check8")).setChecked(true);

        ((CCCheckBox) getChild("check11")).setChecked(true);
        ((CCCheckBox) getChild("check12")).setChecked(true);
        ((CCCheckBox) getChild("check13")).setChecked(true);

        // for the acsls options
        pSheetModel2 = new CCPropertySheetModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/admin/NewAcslsReportPropertySheet.xml");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(PROPERTY_SHEET_MEDIA, CCPropertySheet.class);
        registerChild(PROPERTY_SHEET_ACSLS, CCPropertySheet.class);
        pSheetModel1.registerChildren(this);
        pSheetModel2.registerChildren(this);

        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(PROPERTY_SHEET_MEDIA)) {
            CCPropertySheet psChild =
                new CCPropertySheet(this, pSheetModel1, name);
            child = psChild;
        } else if (
            pSheetModel1 != null && pSheetModel1.isChildSupported(name)) {
            // Create child from property sheet model.
            child = pSheetModel1.createChild(this, name);
            return child;

        } else if (name.equals(PROPERTY_SHEET_ACSLS)) {
            CCPropertySheet psChild =
                new CCPropertySheet(this, pSheetModel2, name);
            child = psChild;
        } else if (
            pSheetModel2 != null && pSheetModel2.isChildSupported(name)) {
            // Create child from property sheet model.
            child = pSheetModel2.createChild(this, name);
            return child;

        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        TraceUtil.trace1("Entering..");

        // If reportType == null, then display media page as default

        TraceUtil.trace3("Report Type: " + this.reportType);
        if (this.reportType != null) {
            if ("reports.type.desc.media".equals(this.reportType)) {
                return "/jsp/admin/NewMediaReportPagelet.jsp";
            } else if ("reports.type.desc.acsls".equals(this.reportType)) {
                return "/jsp/admin/NewAcslsReportPagelet.jsp";
            }
        }
        // else
        return "/jsp/admin/NewMediaReportPagelet.jsp";
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        this.reportType = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.REPORT_TYPE);

        TraceUtil.trace3("Exiting");
    }

    public void submitMediaReportRequest() throws SamFSException {
        TraceUtil.trace3("Entering");

        int inclSections = 0;
        NewReportViewBean parent = (NewReportViewBean)getParentViewBean();

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check1")) ?
                inclSections | Report.INCL_BAD_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check2")) ?
            inclSections | Report.INCL_DUPLICATE_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check3")) ?
                inclSections | Report.INCL_RO_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check4")) ?
                inclSections | Report.INCL_WRITEPROTECT_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check5")) ?
                inclSections | Report.INCL_FOREIGN_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check6")) ?
                inclSections | Report.INCL_RECYCLE_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check7")) ?
                inclSections | Report.INCL_RESERVED_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check8")) ?
                inclSections | Report.INCL_NO_SLOT_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check9")) ?
            inclSections | Report.INCL_INUSE_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check10")) ?
            inclSections | Report.INCL_BLANK_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check11")) ?
                inclSections | Report.INCL_SUMMARY_POOL : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check12")) ?
                inclSections | Report.INCL_SUMMARY_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel1.getValue("check13")) ?
                inclSections | Report.INCL_SUMMARY_MEDIA : inclSections;



        if (inclSections == 0) { // no selection of section
            throw new SamFSException(
                "reports.create.error.noCheckBoxSelection");
        }

        SamQFSSystemModel sysModel = SamUtil.getModel(parent.getServerName());
        sysModel.getSamQFSSystemAdminManager().
            createReport(Report.TYPE_MEDIA, inclSections);

        LogUtil.info(
            this.getClass(),
            "handleSubmitRequest",
            "Sent request to create media report");

        TraceUtil.trace3("Exiting");
    }

    public void submitAcslsReportRequest() throws SamFSException {
        TraceUtil.trace3("Entering");

        int inclSections = 0;
        NewReportViewBean parent = (NewReportViewBean)getParentViewBean();

        inclSections =
            "true".equals((String)pSheetModel2.getValue("drivesValue")) ?
                inclSections | Report.INCL_DRIVE : inclSections;
        inclSections =
            "true".equals((String)pSheetModel2.getValue("locksValue")) ?
                inclSections | Report.INCL_LOCK : inclSections;

        inclSections =
            "true".equals(
                (String)pSheetModel2.getValue("volumesAccessedValue")) ?
                inclSections | Report.INCL_ACCESSED_VSN : inclSections;

        inclSections =
            "true".equals(
                (String)pSheetModel2.getValue("volumesEnteredValue")) ?
                inclSections | Report.INCL_IMPORTED_VSN : inclSections;

        inclSections =
            "true".equals((String)pSheetModel2.getValue("scratchPoolValue")) ?
                inclSections | Report.INCL_ACSLS_POOL : inclSections;

        if (inclSections == 0) { // no selection of section
            throw new SamFSException(
                "reports.create.error.noCheckBoxSelection");
        }

        SamQFSSystemModel sysModel = SamUtil.getModel(parent.getServerName());
        sysModel.getSamQFSSystemAdminManager().
            createReport(Report.TYPE_ACSLS, inclSections);

        LogUtil.info(
            this.getClass(),
            "handleSubmitRequest",
            "Sent request to create media report");

        TraceUtil.trace3("Exiting");
    }
}
