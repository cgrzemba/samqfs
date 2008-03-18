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

// ident	$Id: ShowLogViewBean.java,v 1.16 2008/03/17 14:40:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextArea;
import com.sun.web.ui.view.html.CCTextField;

import java.io.IOException;
import java.util.GregorianCalendar;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Log and Trace Display page
 */

public class ShowLogViewBean extends ShowPopUpWindowViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ShowLog";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/admin/ShowLog.jsp";

    private static final String STATIC_TEXT = "StaticText";
    private static final String LAST_UPDATE_TEXT = "LastUpdateText";
    private static final String TEXT_FIELD = "TextField";
    private static final String DROP_DOWN_MENU = "DropDownMenu";
    private static final String REFRESH_BUTTON = "RefreshButton";
    private static final String ERROR_MESSAGE  = "ErrorMessage";
    private static final String AUTO_REFRESH_RATE = "AutoRefreshRate";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public ShowLogViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        super.setWindowSizeNormal(true);
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(LAST_UPDATE_TEXT, CCStaticTextField.class);
        registerChild(DROP_DOWN_MENU, CCDropDownMenu.class);
        registerChild(TEXT_FIELD, CCTextField.class);
        registerChild(REFRESH_BUTTON, CCButton.class);
        registerChild(ERROR_MESSAGE, CCHiddenField.class);
        registerChild(AUTO_REFRESH_RATE, CCHiddenField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        View child = null;
        if (name.equals(STATIC_TEXT) ||
            name.equals(LAST_UPDATE_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(TEXT_FIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(DROP_DOWN_MENU)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            // Set child options
            myChild.setOptions(createOptionList());
            child = (View) myChild;
        } else if (name.equals(REFRESH_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(ERROR_MESSAGE) ||
            name.equals(AUTO_REFRESH_RATE)) {
            child = new CCHiddenField(this, name, null);
        } else if (super.isChildSupported(name)) {
            child = super.createChild(name);
          // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     * @Override
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering beginDisplay()!");
        pageTitleModel.setPageTitleText(
            new StringBuffer(getPathName()).append(" (").append(
            getServerName()).append(")").toString());
        ((CCHiddenField) getChild(ERROR_MESSAGE)).
            setValue(SamUtil.getResourceString("ShowLog.error.javascript"));
        ((CCButton) getChild(REFRESH_BUTTON)).
            setDisplayLabel("ShowLog.button.refresh");
        ((CCStaticTextField) getChild(LAST_UPDATE_TEXT)).setValue(
            SamUtil.getTimeString(new GregorianCalendar()));

        populateTextArea();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate text in the text area component
     */
    private void populateTextArea() {
        StringBuffer buf = new StringBuffer();

        CCTextArea myTextArea =
            (CCTextArea) getChild(ShowPopUpWindowViewBeanBase.TEXT_AREA);
        CCTextField myTextField = (CCTextField) getChild(TEXT_FIELD);
        int showLast = 50;

        try {
            String textFieldValue = (String) myTextField.getValue();
            if (textFieldValue != null) {
                showLast = Integer.parseInt(textFieldValue);
            } else {
                myTextField.setValue(Integer.toString(showLast));
            }
        } catch (NumberFormatException numEx) {
            myTextField.setValue(Integer.toString(showLast));
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            String [] content = sysModel.getSamQFSSystemAdminManager().
                tailFile(getPathName(), showLast);
            if (content == null) {
                buf.append(SamUtil.getResourceString("ShowLog.empty"));
            } else {
                for (int i = 0; i < content.length; i++) {
                    if (i > 0) {
                        buf.append("\n");
                    }
                    buf.append(content[i]);
                }
            }
        } catch (SamFSException samEx) {
            myTextArea.setValue(SamUtil.getResourceString("ShowLog.error"));
            TraceUtil.trace1("Failed to populate log/trace file content!");
            TraceUtil.trace1("Reason: " + samEx.getMessage());
            SamUtil.processException(samEx, this.getClass(),
                "beginDisplay()",
                "Failed to populate log and trace files content",
                getServerName());
            return;
        }

        if (buf.length() == 0) {
            buf.append(SamUtil.getResourceString("ShowLog.empty"));
        }

        myTextArea.setValue(buf.toString());
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/admin/ShowPopUpPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private OptionList createOptionList() {
        return new OptionList(
            new String[] {
                "ShowLog.dropdown.none",  // labels
                "ShowLog.dropdown.choice.30sec",
                "ShowLog.dropdown.choice.1min",
                "ShowLog.dropdown.choice.2min",
                "ShowLog.dropdown.choice.5min",
                "ShowLog.dropdown.choice.10min"},
            new String[] {
                "99999000",  // values
                "30000",
                "60000",
                "120000",
                "300000",
                "600000"});
    }

    public void handleRefreshButtonRequest(RequestInvocationEvent event)
         throws ServletException, IOException {
         this.forwardTo();
    }

    private String getPathName() {
        // first check the page session
        String pathName = (String) getPageSessionAttribute("PSG_PATH_NAME");

        // second check the request
        if (pathName == null) {
            pathName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.PATH_NAME);

            if (pathName != null) {
                setPageSessionAttribute(
                    "PSG_PATH_NAME",
                    pathName);
            } else {
                throw new IllegalArgumentException("Path Name not supplied");
            }
        }

        return pathName;
    }
}
