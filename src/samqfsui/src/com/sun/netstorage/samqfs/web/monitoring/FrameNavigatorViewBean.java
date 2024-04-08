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

// ident	$Id: FrameNavigatorViewBean.java,v 1.14 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.SysInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.alarm.CCAlarm;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import java.util.Properties;
import javax.servlet.ServletException;

/**
 *  This class is ViewBean of the navigation frame of the monitoring dashboard
 */

public class FrameNavigatorViewBean extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */

    private static final String SERVER_NAME  = "ServerName";
    private static final String SELECTED_ID  = "SelectedID";

    // page session attribute to keep track of selectedd page id
    private static final String PSA_PAGE_ID = "psa_pageID";

    private static final String URL = "/jsp/monitoring/FrameNavigator.jsp";
    private static final String PAGE_NAME = "FrameNavigator";

    // Child elements
    private static final String HREF_DAEMONS = "HrefDaemons";
    private static final String HREF_FS = "HrefFS";
    private static final String HREF_COPYFREESPACE = "HrefCopyFreeSpace";
    private static final String HREF_LIBRARIES = "HrefLibraries";
    private static final String HREF_DRIVES = "HrefDrives";
    private static final String HREF_TAPEMOUNTQUEUE = "HrefTapeMountQueue";
    private static final String HREF_QUARANTINEDVSNS = "HrefQuarantinedVSNs";
    private static final String HREF_ARCOPYQUEUE = "HrefArCopyQueue";
    private static final String HREF_STAGINGQUEUE = "HrefStagingQueue";
    private static final String STATIC_TEXT = "StaticText";
    private static final String STATIC_TEXT_TITLE = "StaticTextTitle";
    private static final String ALARM_DAEMONS = "AlarmDaemons";
    private static final String ALARM_FS = "AlarmFS";
    private static final String ALARM_COPYFREESPACE = "AlarmCopyFreeSpace";
    private static final String ALARM_LIBRARIES = "AlarmLibraries";
    private static final String ALARM_DRIVES = "AlarmDrives";
    private static final String ALARM_TAPEMOUNTQUEUE = "AlarmTapeMountQueue";
    private static final String ALARM_QUARANTINEDVSNS = "AlarmQuarantinedVSNs";
    private static final String ALARM_ARCOPYQUEUE = "AlarmArCopyQueue";
    private static final String ALARM_STAGINGQUEUE = "AlarmStagingQueue";

    public static final int PAGE_DAEMON = 1;
    public static final int PAGE_FILESYSTEM = 2;
    public static final int PAGE_COPYFREESPACE = 3;
    public static final int PAGE_LIBRARY = 4;
    public static final int PAGE_DRIVE = 5;
    public static final int PAGE_TAPEMOUNTQUEUE = 6;
    public static final int PAGE_QUARANTINEDVSN = 7;
    public static final int PAGE_ARCOPYQUEUE = 8;
    public static final int PAGE_STAGINGQUEUE = 9;

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    public FrameNavigatorViewBean() {
        super(PAGE_NAME);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        // set the address of the JSP page
        setDefaultDisplayURL(URL);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(SELECTED_ID, CCHiddenField.class);
        registerChild(HREF_DAEMONS, CCHref.class);
        registerChild(HREF_FS, CCHref.class);
        registerChild(HREF_COPYFREESPACE, CCHref.class);
        registerChild(HREF_LIBRARIES, CCHref.class);
        registerChild(HREF_DRIVES, CCHref.class);
        registerChild(HREF_TAPEMOUNTQUEUE, CCHref.class);
        registerChild(HREF_QUARANTINEDVSNS, CCHref.class);
        registerChild(HREF_ARCOPYQUEUE, CCHref.class);
        registerChild(HREF_STAGINGQUEUE, CCHref.class);
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(STATIC_TEXT_TITLE, CCStaticTextField.class);
        registerChild(ALARM_DAEMONS, CCAlarm.class);
        registerChild(ALARM_FS, CCAlarm.class);
        registerChild(ALARM_COPYFREESPACE, CCAlarm.class);
        registerChild(ALARM_LIBRARIES, CCAlarm.class);
        registerChild(ALARM_DRIVES, CCAlarm.class);
        registerChild(ALARM_TAPEMOUNTQUEUE, CCAlarm.class);
        registerChild(ALARM_QUARANTINEDVSNS, CCAlarm.class);
        registerChild(ALARM_ARCOPYQUEUE, CCAlarm.class);
        registerChild(ALARM_STAGINGQUEUE, CCAlarm.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (name.equals(SERVER_NAME) ||
            name.equals(SELECTED_ID)) {
            return new CCHiddenField(this, name, null);
        } else if (name.startsWith("Href")) {
            return new CCHref(this, name, null);
        } else if (name.startsWith("StaticText")) {
            return new CCStaticTextField(this, name, null);
        } else if (name.startsWith("Alarm")) {
            return new CCAlarm(this, name, CCAlarm.SEVERITY_OK);
        } else {
            // Should not come here
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        // Bold the title instruction
        CCStaticTextField title =
            (CCStaticTextField) getChild(STATIC_TEXT_TITLE);
        title.setValue(
            boldIt(SamUtil.getResourceString("Monitor.label.areatomonitor")));

        // Write server name to hidden field
        ((CCHiddenField) getChild(SERVER_NAME)).setValue(getServerName());

        // Write selected node to hidden field
        ((CCHiddenField) getChild(SELECTED_ID)).setValue(getPageID());

        // Assign severity icons
        try {
            setSeverity();
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to retrieve fault status in navigation frame!");
            TraceUtil.trace1("Reason: " + samEx.getMessage());
        }
    }

    private String boldIt(String input) {
        input = input == null ? "" : input;
        return "<b>".concat(input).concat("</b>");
    }

    private String getServerName() {
        String serverName =
            (String) getPageSessionAttribute(
                        Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        if (serverName == null) {
            serverName =
                RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                serverName);
        }
        return serverName;
    }

    private String getPageID() {
        String pageID =
            (String) getPageSessionAttribute(PSA_PAGE_ID);
        if (pageID == null) {
            pageID = RequestManager.getRequest().getParameter(PSA_PAGE_ID);
            setPageSessionAttribute(PSA_PAGE_ID, pageID);
        }
        return pageID;
    }

    /**
     * Request handler for all href links in the navigation frame
     */
    public void handleHrefDaemonsRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_DAEMON);
    }

    public void handleHrefFSRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_FILESYSTEM);
    }

    public void handleHrefCopyFreeSpaceRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_COPYFREESPACE);
    }

    public void handleHrefLibrariesRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_LIBRARY);
    }

    public void handleHrefDrivesRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_DRIVE);
    }

    public void handleHrefTapeMountQueueRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_TAPEMOUNTQUEUE);
    }

    public void handleHrefQuarantinedVSNsRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_QUARANTINEDVSN);
    }

    public void handleHrefArCopyQueueRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_ARCOPYQUEUE);
    }

    public void handleHrefStagingQueueRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        forwardPage(PAGE_STAGINGQUEUE);
    }

    private void forwardPage(int pageID) throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        setPageSessionAttribute(PSA_PAGE_ID, Integer.toString((pageID)));
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Helper function to determine the fault icon for the nav tree node
     *
     * The following is an example of the status array.
     *
     * name=daemons,status=0
     * name=fs,status=0
     * name=copyUtil,status=0
     * name=libraries,status=0
     * name=drives,status=0
     * name=loadQ,status=0
     * name=unusableVsns,status=1
     * name=arcopyQ,status=1
     * name=stageQ,status=0
     *
     * Status:
     *
     * SysInfo.COMPONENT_STATUS_OK  = 0;
     * SysInfo.COMPONENT_STATUS_WARNING  = 1;
     * SysInfo.COMPONENT_STATUS_ERR = 2;
     * SysInfo.COMPONENT_STATUS_FAILURE  = 3;
     */
    private void setSeverity() throws SamFSException {
        // Call API to grab every page status so we know if an alarm
        // icon should be set
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        String [] statusStr = sysModel.getSamQFSSystemAdminManager().
                                        getComponentStatusSummary();
        for (int i = 0; i < statusStr.length; i++) {
            TraceUtil.trace3(statusStr[i]);
            Properties properties =
                ConversionUtil.strToProps(statusStr[i]);

            String key = properties.getProperty("name");
            // Key should not be null, just to be safe
            if (key == null) {
                continue;
            }

            int status = 0;
            try {
                status = Integer.parseInt(properties.getProperty("status"));
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1("Developer bug found. Key is " + key);
                TraceUtil.trace1(
                    "NumberFormatException caught in setSeverity!");
                // skip this entry
                continue;
            }
            TraceUtil.trace2("Key: " + key + " Status: " + status);
            CCAlarm alarmObj = null;
            if ("daemons".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_DAEMONS);
            } else if ("fs".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_FS);
            } else if ("copyUtil".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_COPYFREESPACE);
            } else if ("libraries".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_LIBRARIES);
            } else if ("drives".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_DRIVES);
            } else if ("loadQ".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_TAPEMOUNTQUEUE);
            } else if ("unusableVsns".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_QUARANTINEDVSNS);
            } else if ("arcopyQ".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_ARCOPYQUEUE);
            } else if ("stageQ".equals(key)) {
                alarmObj = (CCAlarm) getChild(ALARM_STAGINGQUEUE);
            }

            // Set the correct alarm icon
            alarmObj.setValue(getAlarmSeverity(status));
        }
    }

    private String getAlarmSeverity(int status) {
        switch (status) {
            case SysInfo.COMPONENT_STATUS_OK:
                return CCAlarm.SEVERITY_OK;
            case SysInfo.COMPONENT_STATUS_WARNING:
                return CCAlarm.SEVERITY_MINOR;
            case SysInfo.COMPONENT_STATUS_ERR:
                return CCAlarm.SEVERITY_MAJOR;
            case SysInfo.COMPONENT_STATUS_FAILURE:
                return CCAlarm.SEVERITY_CRITICAL;
        }
        return CCAlarm.SEVERITY_OK;
    }

}
