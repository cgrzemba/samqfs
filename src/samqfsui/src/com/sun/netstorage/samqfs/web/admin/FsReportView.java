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

// ident	$Id: FsReportView.java,v 1.10 2008/03/17 14:40:41 am143972 Exp $

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

public class FsReportView extends CommonTableContainerView {

    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String CHILD_SAMPLEFS_HREF  = "SampleFsHref";
    public static final String CHILD_SAMPLEFS_NOHREF  = "SampleFsNoHref";
    // Tiled View
    public static String TILED_VIEW = "FsReportTiledView";

    // Page Action Table Model
    private CCActionTableModel tableModel = null;

    public FsReportView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "reportSummaryTable";
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/admin/FsReportSummaryTable.xml");
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(tableModel);
        registerChild(TILED_VIEW, FsReportTiledView.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_SAMPLEFS_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_SAMPLEFS_HREF, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(TILED_VIEW)) {
          return new FsReportTiledView(this, tableModel, name);
        } else if (name.equals(CHILD_STATICTEXT) ||
            name.equals(CHILD_SAMPLEFS_NOHREF)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_SAMPLEFS_HREF)) {
            return new CCHref(this, name, null);
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
        String reportPath = "/xsl/fsReport.xml";

        ((CCHref)getChild(CHILD_SAMPLEFS_HREF)).setValue(reportPath);

        TraceUtil.trace3("Exiting");
    }

    public void populateDisplay() throws ModelControlException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String reportPath = (String) parent.getPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME);

        String root = RequestManager.getRequestContext().
            getServletContext().getRealPath("/");
        try {
            if ("/xsl/fsReport.xml".equals(reportPath)) {
                ((CCHref)getChild(CHILD_SAMPLEFS_HREF)).setVisible(false);
            } else {
                ((CCStaticTextField)getChild(CHILD_SAMPLEFS_NOHREF))
                    .setVisible(false);
            }
            populateSummaryTable();

            // If PATH_NAME == null, then display blank, else display content
            // display content is handled by the jsp (scriptlet)
            if (reportPath != null && reportPath.trim().length() > 0) {
                populateReportContent(reportPath);

            }
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "FsReportView",
                "Failed to display contents of report",
                    parent.getServerName());
                SamUtil.setErrorAlert(
                    parent,
                    parent.CHILD_COMMON_ALERT,
                    "reports.error.populate",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    parent.getServerName());
        }
    }

    public void handleNewButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME, null);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            sysModel.getSamQFSSystemAdminManager()
                .createReport(Report.TYPE_FS, 0);

            SamUtil.setInfoAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "reports.create.msg.success"),
                serverName);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "FsReportView",
                samEx.getMessage(),
                serverName);

            SamUtil.setErrorAlert(
                parent,
                parent.CHILD_COMMON_ALERT,
                "reports.error.create.newFsMetric",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        parent.forwardTo(getRequestContext());
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

        tableModel.beforeFirst();
        int index = -1;

        while (tableModel.next()) {
            if (tableModel.isRowSelected()) {
                index = tableModel.getRowIndex();
            }
        }
        tableModel.setRowIndex(index);
        String path = (String) tableModel.getValue("PathHiddenField");

        try {
            SamQFSSystemModel sysModel =
                SamUtil.getModel(parent.getServerName());

            LogUtil.info(this.getClass(), "handleDeleteButtonRequest",
                new NonSyncStringBuffer().append(
                    "Start deleting the fs report ").append(path).toString());

                sysModel.getSamQFSSystemFSManager().deleteFile(path);

            LogUtil.info(this.getClass(),  "handleDeleteButtonRequest",
                new NonSyncStringBuffer().append(
                    "Done deleting the fs report ").append(path).toString());
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

    public void handleSampleFsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        parent.setPageSessionAttribute(
            Constants.PageSessionAttributes.PATH_NAME,
            (String) getDisplayFieldValue("SampleFsHref"));
        parent.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void initializeTableHeaders() {
        TraceUtil.trace3("Entering");
        tableModel.setActionValue("ReportDateColumn",
            "common.columnheader.createdTime");
        tableModel.setActionValue("NewButton", "common.button.new");
        tableModel.setActionValue("DeleteButton", "common.button.remove");

        TraceUtil.trace3("Exiting");
    }

    private void populateSummaryTable()
        throws ModelControlException, SamFSException {
        TraceUtil.trace3("Entering");

        CCActionTable table = (CCActionTable) getChild(CHILD_ACTION_TABLE);
        tableModel.clear();
        // Sort ReportDateColumn by latest report
        tableModel.setPrimarySortName("DateHiddenField");
        tableModel.setPrimarySortOrder(CCActionTableModelInterface.DESCENDING);

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        SamQFSSystemModel sysModel = SamUtil.getModel(parent.getServerName());
        GenericFile [] reports =
            sysModel.getSamQFSSystemAdminManager().
                            getAllReports(Report.TYPE_FS);
        for (int i = 0; i < reports.length; i++) {
            String type = reports[i].getDescription();
            if (i > 0) {
                tableModel.appendRow();
            }
            String filename = reports[i].getName();
            String path = Report.REPORTS_DIR.concat(filename);
            int start = path.lastIndexOf("-") + 1;
            int end = path.lastIndexOf(".");

            String createdTimeStr = path.substring(start, end);
            long createdTime = Long.parseLong(createdTimeStr);

            tableModel.setValue("DateHiddenField", new Long(createdTime));
            // for display of the time string , use SamUtil.getTimeString()
            tableModel.setValue("ReportDateText",
                SamUtil.getTimeString(createdTime));
            tableModel.setValue("PathHref", path);
            tableModel.setValue("PathNoHref",
                SamUtil.getTimeString(createdTime));
            tableModel.setValue("PathHiddenField", path);
        }

        TraceUtil.trace3("Exiting");
    }

    private void populateReportContent(String reportPath)
        throws SamFSException {

        TraceUtil.trace3("Entering");
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();

        String xslFileName = "/xsl/html/xml2html.xsl";

        if (reportPath.startsWith("/xsl")) { // sample report on local webserver
            try {
                ServletContext sc =
                    RequestManager.getRequestContext().getServletContext();

                this.xhtml = XmlConvertor
                    .convert2Xhtml(sc.getResourceAsStream(reportPath),
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

    public String getXhtmlString() { return xhtml; }
    private String xhtml = ""; // used by JSP
}
