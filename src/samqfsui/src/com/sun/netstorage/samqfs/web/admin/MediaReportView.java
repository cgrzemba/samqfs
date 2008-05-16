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

// ident	$Id: MediaReportView.java,v 1.13 2008/05/16 19:39:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.Report;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.GenericFile;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.XmlConvertor;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.xml.transform.TransformerException;


public class MediaReportView extends CommonTableContainerView {

    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String CHILD_SAMPLEMEDIA_HREF = "SampleMediaHref";
    public static final String CHILD_SAMPLESTK_HREF = "SampleStkHref";
    public static final String CHILD_SAMPLEMEDIA_NOHREF  = "SampleMediaNoHref";
    public static final String CHILD_SAMPLESTK_NOHREF  = "SampleStkNoHref";

    private static final String SAMPLE_MEDIA_REPORT = "/xsl/mediaReport.xml";
    private static final String SAMPLE_ACSLS_REPORT = "/xsl/acslsReport.xml";

    // Tiled View
    public static String TILED_VIEW = "MediaReportTiledView";
    // Page Action Table Model
    private CCActionTableModel tableModel = null;

    public MediaReportView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "reportSummaryTable";
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/admin/MediaReportSummaryTable.xml");
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(tableModel);
        registerChild(TILED_VIEW, MediaReportTiledView.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_SAMPLEMEDIA_HREF, CCHref.class);
        registerChild(CHILD_SAMPLEMEDIA_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_SAMPLESTK_HREF, CCHref.class);
        registerChild(CHILD_SAMPLESTK_NOHREF, CCStaticTextField.class);

        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(TILED_VIEW)) {
          return new MediaReportTiledView(this, tableModel, name);
        } else if (name.equals(CHILD_STATICTEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_SAMPLEMEDIA_HREF)
            || name.equals(CHILD_SAMPLESTK_HREF)) {
            return new CCHref(this, name, null);

        } else if (name.equals(CHILD_STATICTEXT) ||
            name.equals(CHILD_SAMPLEMEDIA_NOHREF) ||
            name.equals(CHILD_SAMPLESTK_NOHREF)) {
            return new CCStaticTextField(this, name, null);

        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        // set the column headings
        initializeTableHeaders();
        ((CCButton)getChild("DeleteButton")).setDisabled(true);
        // sample reports are stored in the webserver
        String root = RequestManager.getRequestContext().
            getServletContext().getRealPath("/");
        ((CCHref) getChild(CHILD_SAMPLEMEDIA_HREF)).
            setValue(SAMPLE_MEDIA_REPORT);
        ((CCHref)getChild(CHILD_SAMPLESTK_HREF)).
            setValue(SAMPLE_ACSLS_REPORT);

        TraceUtil.trace3("Exiting");
    }

    public void populateDisplay() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String reportPath = (String) parent.getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        String root = RequestManager.getRequestContext().
            getServletContext().getRealPath("/");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            boolean hideAcslsReportSample = true;
            // if no archiving media is configured, hide the sample stk report
            Library libs[] =
                sysModel.getSamQFSSystemMediaManager().getAllLibraries();
            for (int j = 0; j < libs.length; j++) {
                if (libs[j].getDriverType() == Library.ACSLS) {
                    hideAcslsReportSample = false;
                    break; // no need to continue
                }
            }

            if (hideAcslsReportSample) {
                ((CCHref)getChild(CHILD_SAMPLESTK_HREF)).setVisible(false);
                ((CCStaticTextField)
                    getChild(CHILD_SAMPLESTK_NOHREF)).setVisible(false);

            } else {
                if (SAMPLE_ACSLS_REPORT.equals(reportPath)) {
                    ((CCHref)getChild(CHILD_SAMPLESTK_HREF)).setVisible(false);
                } else {
                    ((CCStaticTextField)
                        getChild(CHILD_SAMPLESTK_NOHREF)).setVisible(false);
                }
            }

            populateSummaryTable();

            // If PATH_NAME == null, then display blank, else display content
            // display content is handled by the jsp (scriptlet)
            if (reportPath != null && reportPath.trim().length() > 0) {
                populateReportContent(reportPath);
            }

            if (SAMPLE_MEDIA_REPORT.equals(reportPath)) {
                ((CCHref)getChild(CHILD_SAMPLEMEDIA_HREF)).setVisible(false);

            } else {
                ((CCStaticTextField)
                    getChild(CHILD_SAMPLEMEDIA_NOHREF)).setVisible(false);
            }

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "MediaReportView",
                "Failed to display contents of report",
                    parent.getServerName());
            TraceUtil.trace1("ERROR: " + ex.getMessage());
            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "reports.error.populate",
                ex.getSAMerrno(),
                ex.getMessage(),
                parent.getServerName());
        }
    }

    public void handleDeleteButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME, null);

        CCActionTable table = (CCActionTable) getChild(CHILD_ACTION_TABLE);
        table.restoreStateData();
        CCActionTableModel tmodel = (CCActionTableModel) table.getModel();

        tmodel.beforeFirst();
        int index = -1;

        while (tmodel.next()) {
            if (tmodel.isRowSelected()) {
                index = tmodel.getRowIndex();
            }
        }
        tmodel.setRowIndex(index);
        String path = (String) tmodel.getValue("PathHiddenField");

        try {
            SamQFSSystemModel sysModel =
                SamUtil.getModel(parent.getServerName());

            LogUtil.info(this.getClass(), "handleDeleteButtonRequest",
                new NonSyncStringBuffer().append(
                    "Start deleting the report ").append(path).toString());

                sysModel.getSamQFSSystemFSManager().deleteFile(path);

            LogUtil.info(this.getClass(),  "handleDeleteButtonRequest",
                new NonSyncStringBuffer().append(
                    "Done deleting the report ").append(path).toString());
            SamUtil.setInfoAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString("reports.success.text.delete"),
                serverName);

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "FsReportView",
                ex.getMessage(),
                serverName);

            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "reports.error.deleteFs",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleSampleStkHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        ((CommonViewBeanBase) getParentViewBean()).setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME,
            (String) getDisplayFieldValue("SampleStkHref"));

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleSampleMediaHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        ((CommonViewBeanBase) getParentViewBean()).setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME,
            (String) getDisplayFieldValue("SampleMediaHref"));

        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");
        tableModel.setActionValue("ReportDescColumn", "reports.summary.desc");
        tableModel.setActionValue("ReportDateColumn",
            "common.columnheader.createdTime");
        tableModel.setActionValue("ReportSizeColumn",
            "common.columnheader.size");
        tableModel.setActionValue("NewButton", "common.button.new");
        tableModel.setActionValue("DeleteButton", "common.button.remove");
        TraceUtil.trace3("Exiting");
    }

    private void populateSummaryTable() throws SamFSException {
        TraceUtil.trace3("Entering");
        // retrive the assigned model to maintain state information
        tableModel.clear();
        // Sort ReportDateColumn by latest report
        tableModel.setPrimarySortName("DateHiddenField");
        tableModel.setPrimarySortOrder(CCActionTableModelInterface.DESCENDING);

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        SamQFSSystemModel sysModel = SamUtil.getModel(parent.getServerName());
        GenericFile reports [] =
            sysModel.getSamQFSSystemAdminManager().
                getAllReports(Report.TYPE_ALL);

        for (int i = 0; i < reports.length; i++) {
            String type = reports[i].getDescription();
            // filter fs reports
            if ((type == null) ||
                SamUtil.getResourceString(
                    "reports.type.desc.fs").equals(type)) {
                continue;
            }
            if (i > 0) {
                tableModel.appendRow();
            }

            String filename = reports[i].getName();
            String path = Report.REPORTS_DIR.concat(filename);
            // parse the time from the filename, time is the last part of the
            // filename before the extension
            int start = filename.lastIndexOf("-") + 1;
            int end = filename.lastIndexOf(".");

            String createdTimeStr = filename.substring(start, end);
            long createdTime = Long.parseLong(createdTimeStr);

            tableModel.setValue("DateHiddenField", new Long(createdTime));
            tableModel.setValue("ReportDateText",
                SamUtil.getTimeString(createdTime));
            tableModel.setValue("ReportNameNoHref",
                SamUtil.getTimeString(createdTime));
            tableModel.setValue("ReportNameHref", path);
            tableModel.setValue("ReportDescText", reports[i].getDescription());
            tableModel.setValue("PathHiddenField", path);
            tableModel.setValue("ReportSizeText",
                Capacity.newCapacity(
                    reports[i].getSize(), SamQFSSystemModel.SIZE_B).toString());

        }

        TraceUtil.trace3("Exiting");
    }

    private void populateReportContent(String reportPath)
        throws SamFSException {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Get content for report " + reportPath);

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String root = RequestManager.getRequestContext().
            getServletContext().getRealPath("/");

        String xslFileName = "/xsl/html/xml2html.xsl";

        if (reportPath.equals(SAMPLE_MEDIA_REPORT) ||
            reportPath.equals(SAMPLE_ACSLS_REPORT)) {
            // sample report on local webserver
            try {
                ServletContext sc =
                    RequestManager.getRequestContext().getServletContext();
                this.xhtml = XmlConvertor.convert2Xhtml(
                    sc.getResourceAsStream(reportPath),
                    sc.getResourceAsStream(xslFileName));
            } catch (IOException ioEx) {
                throw new SamFSException(ioEx.getLocalizedMessage());
            } catch (TransformerException trEx) {
                throw new SamFSException(trEx.getLocalizedMessage());
            }
        } else {
            // Get the report content from the SAM-QFS server
            SamQFSSystemModel sysModel =
                SamUtil.getModel(parent.getServerName());
            this.xhtml = sysModel.getSamQFSSystemAdminManager().
                getXHtmlReport(reportPath, xslFileName);
        }
        TraceUtil.trace3("Exiting");
    }

    public String getXhtmlString() {
        TraceUtil.trace3("View: getXhtmlString() returns: " + xhtml);
        return xhtml;
    }

    private String xhtml = ""; // used by JSP

}
