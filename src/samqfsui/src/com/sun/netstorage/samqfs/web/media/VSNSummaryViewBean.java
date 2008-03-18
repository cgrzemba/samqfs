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

// ident	$Id: VSNSummaryViewBean.java,v 1.31 2008/03/17 14:43:40 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;

import java.io.IOException;

import javax.servlet.ServletException;


/**
 * This class is the view bean for the VSNSummary page.  VSN Summary page can
 * be accessed from the Library Summary or Library Details Page.  It can also
 * directly accessed by clicking on the Tape VSN tree node. Pay attention to the
 * flow on how the server name and library name are fetched.
 */

public class VSNSummaryViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "VSNSummary";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/media/VSNSummary.jsp";

    public static final
        String CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD = "ConfirmMessageHiddenField";

    // cc components from the corresponding jsp page(s)...
    public static final String CHILD_CONTAINER_VIEW = "VSNSummaryView";
    public static final String CHILD_BREADCRUMB = "BreadCrumb";
    public static final
        String CHILD_LIBRARY_SUMMARY_HREF = "LibrarySummaryHref";
    public static final
        String LIBRARY_DRIVE_SUMMARY_HREF = "LibraryDriveSummaryHref";

    // Hidden field for VSN being loaded
    public static final
        String CHILD_LOADVSN_HIDDEN_FIELD = "LoadVSNHiddenField";
    // Hidden field for Library Name
    public static final
        String CHILD_LIBRARY_NAME_HIDDEN_FIELD = "LibraryNameHiddenField";

    // Hidden Field for serverName
    public static final
        String CHILD_SERVER_NAME_HIDDEN_FIELD = "ServerNameHiddenField";

    // Switch Library Drop Down
    public static final String LABEL = "Label";
    public static final String MENU  = "SwitchLibraryMenu";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel  = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    // For Switch Library Drop Down
    private Library allLibrary [];

    // Contain the string that is used to build the switch library drop down
    private String libraryContent = null;

    /**
     * Constructor
     */
    public VSNSummaryViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();

        registerChild(CHILD_LOADVSN_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_LIBRARY_NAME_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_CONTAINER_VIEW, VSNSummaryView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_LIBRARY_SUMMARY_HREF, CCHref.class);
        registerChild(LIBRARY_DRIVE_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_SERVER_NAME_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(LABEL, CCLabel.class);
        registerChild(MENU, CCDropDownMenu.class);
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
        } else if (name.equals(CHILD_LIBRARY_SUMMARY_HREF) ||
            name.equals(LIBRARY_DRIVE_SUMMARY_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_LOADVSN_HIDDEN_FIELD) ||
            name.equals(CHILD_LIBRARY_NAME_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, null);
        // Action table Container.
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            child = new VSNSummaryView(
                this, name, getServerName(), getLibraryName());
        // javascript text
        } else if (name.equals(CHILD_CONFIRM_MESSAGE_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("media.confirm.unreserve.vsn")).
                    append("###").append(
                    SamUtil.getResourceString("media.confirm.export.vsn")).
                    toString());
        // For server name hidden field
        } else if (name.equals(CHILD_SERVER_NAME_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel(
                "VSNSummary.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
            // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(MENU)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(LABEL)) {
            child = new CCLabel(this, name, null);
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/VSNSummaryPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    private void loadPageTitleModel(String libName) {
        TraceUtil.trace3("Entering");

        // first clear the model
        pageTitleModel.clear();
        pageTitleModel.setValue(
            "VSNSearchPageActionButton",
            "VSNSummary.pageactionbutton");

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSearchPageActionButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.PARENT,
            "VSNSummaryViewBean");
        BreadCrumbUtil.breadCrumbPathForward(
            this,
            PageInfo.getPageInfo().getPageNumber(this.getName()));
        forwardTo(getViewBean(VSNSearchViewBean.class));

        TraceUtil.trace3("Exiting");
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("LibrarySummaryHref");
        ViewBean targetViewBean = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetViewBean.getName()),
            str);
        forwardTo(targetViewBean);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function in breadcrumb link
     *
     * @param event RequestInvocation Event
     */
    public void handleLibraryDriveSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue(LIBRARY_DRIVE_SUMMARY_HREF);
        ViewBean targetView = getViewBean(LibraryDriveSummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.EQ);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) {
        TraceUtil.trace3("Entering");

        String libraryName = getLibraryName();

        // update pageTitleModel
        loadPageTitleModel(libraryName);

        // populate the action table model
        VSNSummaryView view = (VSNSummaryView) getChild(CHILD_CONTAINER_VIEW);

        try {
            view.populateData();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "VSNSummaryViewBean()",
                "Failed to populate VSN Summary information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "VSNSummary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        // Fill in the VSN name to the Hidden Field that is being loaded
        String loadVSN = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.VSN_BEING_LOADED);

        if (loadVSN != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "VB: beginDisplay() Set value to ").append(loadVSN).toString());
            ((CCHiddenField) getChild("LoadVSNHiddenField")).setValue(loadVSN);
        } else {
            SamUtil.doPrint("VB: beginDisplay() Set value to empty string");
            ((CCHiddenField) getChild("LoadVSNHiddenField")).setValue("");
        }

        // Fill in the Library Name to the Hidden Field
        ((CCHiddenField)
            getChild("LibraryNameHiddenField")).setValue(libraryName);
        // Fill in server name for pop ups and switch library drop down
        ((CCHiddenField) getChild(
            CHILD_SERVER_NAME_HIDDEN_FIELD)).setValue(getServerName());
        TraceUtil.trace3("Exiting");
    }

    private String getLibraryName() {
        // In CIS setup, VSN Summary is named as Tape VSNs.  It is a first level
        // leaf in the navigation tree.  If library name is null, return the
        // first available library name.
        String libraryName =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME);
        if (libraryName != null) {
            return libraryName;
        } else {
            // Try to grab from the Request.
            // Falls in this case when user uses switch library drop down
            libraryName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.LIBRARY_NAME);
            if (libraryName != null) {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME, libraryName);
                return libraryName;
            }
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            allLibrary =
                sysModel.getSamQFSSystemMediaManager().getAllLibraries();

            if (allLibrary == null || allLibrary.length == 0) {
                return "";
            } else {
                getParentViewBean().setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME,
                    allLibrary[0].getName());
                return allLibrary[0].getName();
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Error retreiving first available library!");
            samEx.printStackTrace();
            return "";
        }
    }

    private boolean clickFromNavigationTree() {
        Boolean myBoolean = (Boolean)
            getPageSessionAttribute(Constants.PageSessionAttributes.FROM_TREE);
        if (myBoolean == null) {
            // Click from tree
            return true;
        }
        return myBoolean.booleanValue();
    }

    /**
     * Hide label if there are less than 1 library configured and if
     * user arrives this page by clicking on the nav tree
     */
    public boolean beginLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        // reset library content
        libraryContent = "";

        // hide drop down if there is no library or user comes to VSN page
        // via Library Summary Page
        if (!clickFromNavigationTree()) {
            return false;
        }

        StringBuffer buf = new StringBuffer();
        try {
            if (allLibrary == null) {
                allLibrary =
                    SamUtil.getModel(getServerName()).
                        getSamQFSSystemMediaManager().getAllLibraries();
            }

            // hide drop down if there is still no library
            if (allLibrary == null) {
                return false;
            } else if (allLibrary.length == 0) {
                return false;
            }

            for (int i = 0; i < allLibrary.length; i++) {
                if (buf.length() > 0) {
                    buf.append("###");
                }
                // Add all libraries except historian
                if (!Constants.MediaAttributes.HISTORIAN_NAME.
                    equalsIgnoreCase(allLibrary[i].getName())) {
                    buf.append(allLibrary[i].getName());
                } else {
                    // if Historian is found, check to see if the total number
                    // of libraries is less than or equals to 2.  If yes, there
                    // is only 1 real library in the setup and we can safely
                    // hide the switch library drop down.
                    if (allLibrary.length < 3) {
                        return false;
                    }
                }
            }
        } catch (SamFSException samEx) {
            // Mostly it will not throw exception, simple getter function
            TraceUtil.trace1(
                "Error retreiving library name while populating " +
                "switch library drop down!");
            return false;
        }

        libraryContent = buf.toString();

        return true;
    }

    /**
     * Hide drop down if user navigates from the Library Summary Page
     */
    public boolean beginSwitchLibraryMenuDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        // Check if libraryContent is an empty string.  If yes, hide the menu.
        // Otherwise populate the menu.
        TraceUtil.trace3("libraryContent is " + libraryContent);
        if (libraryContent.length() == 0) {
            return false;
        }

        CCDropDownMenu menu = (CCDropDownMenu) getChild(MENU);
        menu.setOptions(
            new OptionList(
               libraryContent.split("###"),
               libraryContent.toString().split("###")));
        menu.setValue(getLibraryName());
        return true;
    }

    /**
     * Overrides the getServerName in CommonViewBeanBase.
     * This method is used due to the use of HtmlUnit test suite.  Jolly
     * needs to use an explicit URL to get around the frameset.  CommonVBB
     * does not have access to the Request and this we need to override
     * getServerName here.
     * You can safely remove this method when we move on to Canoo test suite
     * in the future.
     */
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
