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

// ident	$Id: ServerCommonViewBeanBase.java,v 1.15 2008/10/01 22:43:34 ronaldso Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.ChildContentDisplayEvent;
import com.iplanet.jato.view.event.JspEndChildDisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.web.ui.taglib.WizardWindowTag;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.HelpLinkConstants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCMastheadModel;
import com.sun.web.ui.model.CCTabsModel;
import com.sun.web.ui.taglib.html.CCButtonTag;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.masthead.CCPrimaryMasthead;
import com.sun.web.ui.view.tabs.CCNodeEventHandlerInterface;
import com.sun.web.ui.view.tabs.CCTabs;

import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.jsp.JspException;
import javax.servlet.jsp.tagext.Tag;

/**
 *  This class is the base class for all the viewbeans except wizards and
 *  pop ups.
 */

public class ServerCommonViewBeanBase extends ViewBeanBase
    implements CCNodeEventHandlerInterface {

    /**
     * cc components from the corresponding jsp page(s)...
     */
    protected static final String CHILD_MASTHEAD = "Masthead";
    protected static final String CHILD_TABS = "Tabs";
    public static final String CHILD_COMMON_ALERT = "Alert";

    private String fileName = null, pageName = null;
    private int TAB_NAME;

    // for refresh link
    public static final String CHILD_REFRESH_HREF = "RefreshHref";

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    protected ServerCommonViewBeanBase(
        String pageName, String displayURL, int tabName) {
        super(pageName);
        this.pageName = pageName;
        this.fileName = getPageID();

        TAB_NAME = tabName;

        // Show incompatible browser error alert if browser is unsupported
        if (displayURL.equals("/jsp/server/ServerSelection.jsp") &&
            !ServerUtil.isClientBrowserSupported()) {
            displayURL = "/jsp/util/UnsupportBrowser.jsp";
        }

        // set the address of the JSP page
        setDefaultDisplayURL(displayURL);
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(CHILD_REFRESH_HREF, CCHref.class);
        registerChild(CHILD_MASTHEAD, CCPrimaryMasthead.class);
        registerChild(CHILD_TABS, CCTabs.class);
        registerChild(CHILD_COMMON_ALERT, CCAlertInline.class);
    }

    /**
     * Check the child component is valid
     *
     * @param name of child compoment
     * @return the boolean variable
     */
    protected boolean isChildSupported(String name) {
        if (name.equals(CHILD_MASTHEAD)
            || name.equals(CHILD_TABS)
            || name.equals(CHILD_REFRESH_HREF)
            || name.equals(CHILD_COMMON_ALERT)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        View child = null;

        // Masthead Component
        if (name.equals(CHILD_MASTHEAD)) {
            CCMastheadModel mastheadModel = new CCMastheadModel();
            mastheadModel.clear();
            mastheadModel.setSrc("masthead.logo");
            mastheadModel.setAlt("masthead.altText");
            mastheadModel.setShowUserRole(true);
            mastheadModel.setShowServer(true);
            mastheadModel.setShowDate(true);

            // add help link
            mastheadModel.setHelpFileName(setFileName(getPageID()));

            // add a refresh button here
            if (mastheadModel.getNumLinks() < 1) {
                mastheadModel.addLink(
                    CHILD_REFRESH_HREF,
                    "Masthead.refreshLinkName",
                    "Masthead.refreshLinkTooltip",
                    "Masthead.refreshLinkStatus");
            }

            child = new CCPrimaryMasthead(this, mastheadModel, name);
        } else if (name.equals(CHILD_REFRESH_HREF)) {
            child = new CCHref(this, name, null);

        // Tabs Component
        } else if (name.equals(CHILD_TABS)) {
            CCTabsModel tabsModel =
                ServerTabsUtil.createTabsModel(TAB_NAME);
            CCTabs myChild = new CCTabs(this, tabsModel, name);
            myChild.resetStateData();
            child = myChild;
        // Alert Inline
        } else if (name.equals(CHILD_COMMON_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else {
            // Should not come here
            return null;
        }

        return (View) child;
    }

    /**
     * Function to handle the nodeClick event (Tab clicking handler)
     *
     * @param RequestInvocationEvent
     * @param tab's id
     */
    public void nodeClicked(RequestInvocationEvent event, int id) {
        if (ServerTabsUtil.handleTabClickRequest(
            this, event, id) == false) {
            TraceUtil.trace1("TAB Click NOT handled by ServerTabsUtil!");
        }
        return;
    }

    public void handlePreferenceHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        this.forwardTo();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle when the refresh button in Masthead is clicked
     */
    public void handleRefreshHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace2("REFRESH BUTTON IS CLICKED!!!");
        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * since the CCActionTable doesn't recognize 'unknown' tags, use a
     * regular CCWizardWindowTag in the table XML and swap its HTML with
     * that our of special WizardWindowTag here.
     *
     * Leave this method here just in case if we need a wizard in this server
     * tab.
     */
    public String endChildDisplay(ChildContentDisplayEvent ccde)
        throws ModelControlException {
        String childName = ccde.getChildName();

        String html = super.endChildDisplay(ccde);
        // if its one of our special wizards
        // i.e. name contains keyword "SamQFSWizard"
        if (childName.indexOf(Constants.Wizard.WIZARD_BUTTON_KEYWORD) != -1) {
            // retrieve the html from a WizardWindowTag
            JspEndChildDisplayEvent event = (JspEndChildDisplayEvent)ccde;
            CCButtonTag sourceTag = (CCButtonTag)event.getSourceTag();
            Tag parentTag = sourceTag.getParent();

            // get the peer view of this tag
            View theView = getChild(childName);

            // instantiate the wizard tag
            WizardWindowTag tag = new WizardWindowTag();
            tag.setBundleID(sourceTag.getBundleID());
            tag.setDynamic(sourceTag.getDynamic());

            // try to retrieve the correct HTML from the tag
            try {
                html = tag.getHTMLStringInternal(parentTag,
                    event.getPageContext(), theView);
            } catch (JspException je) {
                throw new IllegalArgumentException("Error retrieving the " +
                    "WizardWIndowTag html : " + je.getMessage());
            }
        }
        return html;
    }

    protected String getPageID() {
        return pageName;
    }

    private static String setFileName(String fileName) {
        if (fileName.equals("ServerSelection")) {
            return HelpLinkConstants.SERVERS;
        } else if (fileName.equals("SiteInformation")) {
            return HelpLinkConstants.SITE_CONFIG;
        } else if (fileName.equals("VersionHighlight")) {
            return HelpLinkConstants.VERSION_HIGHLIGHT;
        } else {
            return "";
        }
    }
}
