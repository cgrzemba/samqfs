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

// ident	$Id: CurrentAlarmSummaryViewBean.java,v 1.26 2008/05/16 19:39:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.alarms;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.LogAndTraceInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import javax.servlet.http.HttpSession;

/**
 *  This class is the view bean for the Current Fault Summary page
 */
public class CurrentAlarmSummaryViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "CurrentAlarmSummary";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/alarms/CurrentAlarmSummary.jsp";

    public static final String LOG_LABEL = "LogLabel";
    public static final String LOG_MENU  = "DropDownMenu";
    public static final String VIEW_BUTTON = "ViewButton";

    public static final String SERVER_NAME = "ServerName";

    // child that used in javascript confirm messages
    public static final String CONFIRM_MESSAGE  = "ConfirmMessageHiddenField";

    // cc components from the corresponding jsp page(s)...
    private static final String CONTAINER_VIEW  = "CurrentAlarmSummaryView";

    // Switch Library Drop Down (For intellistore only)
    public static final String LABEL = "Label";
    public static final String MENU  = "SwitchNodeMenu";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public CurrentAlarmSummaryViewBean() {
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
        registerChild(LABEL, CCLabel.class);
        registerChild(LOG_MENU, CCDropDownMenu.class);
        registerChild(VIEW_BUTTON, CCButton.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
        registerChild(CONTAINER_VIEW, CurrentAlarmSummaryView.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(MENU, CCDropDownMenu.class);
        registerChild(LOG_LABEL, CCLabel.class);
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

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(LABEL) ||
            name.equals(LOG_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(LOG_MENU)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(VIEW_BUTTON)) {
            child = new CCButton(this, name, null);
        // Action table Container.
        } else if (name.equals(CONTAINER_VIEW)) {
            child = new CurrentAlarmSummaryView(this, name);
        // Javascript used StaticTextField
        } else if (name.equals(CONFIRM_MESSAGE)) {
            child = new CCHiddenField(this, name,
                SamUtil.getResourceString("CurrentAlarmSummary.confirmMsg1"));
        } else if (name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, getServerName());
        } else if (name.equals(MENU)) {
            child = new CCDropDownMenu(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // populate the action table model
        CurrentAlarmSummaryView view =
            (CurrentAlarmSummaryView) getChild(CONTAINER_VIEW);

        // Update Masthead faults count
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            // populate the View Log/Trace drop down menu
            populateLogTraceDropDownMenu(sysModel);

            AlarmSummary myAlarmSummary =
                sysModel.getSamQFSSystemAlarmManager().getAlarmSummary();

            HttpSession session =
                RequestManager.getRequestContext().getRequest().getSession();
            String filterMenu = (String) session.getAttribute(
                Constants.SessionAttributes.FAULTFILTER_MENU);

            if (filterMenu == null) {
                filterMenu = Constants.Alarm.ALARM_ALL;
            }
            view.populateData(filterMenu);
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "CurrentAlarmSummaryViewBean()",
                "Failed to populate fault summary information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "CurrentAlarmSummary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    private void populateLogTraceDropDownMenu(SamQFSSystemModel sysModel)
        throws SamFSException {

        String path = null;
        OptionList logTraceFileOptions = new OptionList();
        LogAndTraceInfo [] myLogAndTraceArray = sysModel.getLogAndTraceInfo();

        if (myLogAndTraceArray == null) {
            return;
        }

        for (int i = 0; i < myLogAndTraceArray.length; i++) {
            boolean isLogTraceOn = myLogAndTraceArray[i].isOn();
            path = myLogAndTraceArray[i].getPath();
            if (isLogTraceOn) {
                logTraceFileOptions.add(path, path);
            }
        }

        ((CCDropDownMenu) getChild(
            LOG_MENU)).setOptions(logTraceFileOptions);
        ((CCDropDownMenu) getChild(
            LOG_MENU)).setLabelForNoneSelected(
                "CurrentAlarmSummary.dropdown.noneSelect");
    }
}
