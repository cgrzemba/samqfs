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

// ident	$Id: ImportVSNViewBean.java,v 1.18 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.StkNetLibParam;
import com.sun.netstorage.samqfs.mgmt.media.StkPool;
import com.sun.netstorage.samqfs.web.model.ImportVSNFilter;

import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Import VSN page.  This page will only
 *  be accessed by ACSLS Libraries that are version 4.5+.  No need to check
 *  for versions non Library Type.
 */

public class ImportVSNViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ImportVSN";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/media/ImportVSN.jsp";

    public static final String BREADCRUMB = "BreadCrumb";

    public static final String STATIC_TEXT = "StaticText";
    public static final String ROW_ID_STATIC_TEXT = "RowIDStaticText";
    public static final String COL_ID_STATIC_TEXT = "ColIDStaticText";
    public static final String LABEL = "Label";

    public static final String NONE = "None";
    public static final String SCRATCH_POOL = "ScratchPool";
    public static final String VSN_RANGE = "VSNRange";
    public static final String REGEX = "RegEx";
    public static final String START_FIELD = "StartField";
    public static final String END_FIELD = "EndField";
    public static final String REGEX_FIELD = "RegExField";
    public static final String SCRATCH_POOL_MENU = "ScratchPoolMenu";

    public static final String FILTER_BUTTON = "FilterButton";

    public static final String LIBRARY_SUMMARY_HREF = "LibrarySummaryHref";
    public static final
        String LIBRARY_DRIVE_SUMMARY_HREF = "LibraryDriveSummaryHref";

    // Key to retain Filter content to avoid calling API every time when
    // the page is loaded. (save as ImportVSNFilterHelper object)
    public static final String PSA_FILTER_CONTENT   = "psa_filter_content";

    // Key to retain user filter settings
    public static final String PSA_FILTER_CRITERIA  = "psa_filter_criteria";

    // Key to retain libraries list that share the same ACS Server
    public static final String PSA_SAME_ACSLS_LIB   = "psa_same_acsls_lib";

    // cc components from the corresponding jsp page(s)...
    public static final String CONTAINER_VIEW = "ImportVSNView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // For BreadCrumb
    private CCBreadCrumbsModel breadCrumbsModel    = null;

    // table models
    private Map models = null;

    // OptionList for each of the radio button
    private static OptionList radioOptionsNone = new OptionList(
        new String [] {"ImportVSN.radio.none"},
        new String [] {"ImportVSN.radio.none"});

    private static OptionList radioOptionsPool = new OptionList(
        new String [] {"ImportVSN.radio.scratchpool"},
        new String [] {"ImportVSN.radio.scratchpool"});

    private static OptionList radioOptionsRange = new OptionList(
        new String [] {"ImportVSN.radio.vsnrange"},
        new String [] {"ImportVSN.radio.vsnrange"});

    private static OptionList radioOptionsRegEx = new OptionList(
        new String [] {"ImportVSN.radio.regex"},
        new String [] {"ImportVSN.radio.regex"});

    /**
     * Constructor
     */
    public ImportVSNViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        createPageTitleModel();
        registerChildren();
        initializeTableModels();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CONTAINER_VIEW, ImportVSNView.class);
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(LIBRARY_SUMMARY_HREF, CCHref.class);
        registerChild(LIBRARY_DRIVE_SUMMARY_HREF, CCHref.class);
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(ROW_ID_STATIC_TEXT, CCStaticTextField.class);
        registerChild(COL_ID_STATIC_TEXT, CCStaticTextField.class);
        registerChild(LABEL, CCLabel.class);
        registerChild(FILTER_BUTTON, CCButton.class);
        registerChild(NONE, CCRadioButton.class);
        registerChild(SCRATCH_POOL, CCRadioButton.class);
        registerChild(VSN_RANGE, CCRadioButton.class);
        registerChild(REGEX, CCRadioButton.class);
        registerChild(START_FIELD, CCTextField.class);
        registerChild(END_FIELD, CCTextField.class);
        registerChild(REGEX_FIELD, CCTextField.class);
        registerChild(SCRATCH_POOL_MENU, CCDropDownMenu.class);

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
        } else if (name.equals(BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("ImportVSN.browsertitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(LIBRARY_SUMMARY_HREF) ||
            name.equals(LIBRARY_DRIVE_SUMMARY_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(FILTER_BUTTON)) {
            child = new CCButton(this, name, null);
        } else if (name.equals(STATIC_TEXT) ||
            name.equals(ROW_ID_STATIC_TEXT) ||
            name.equals(COL_ID_STATIC_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(NONE)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(radioOptionsNone);
            child = (View) myChild;
        } else if (name.equals(SCRATCH_POOL)) {
            CCRadioButton myChild = new CCRadioButton(this, NONE, null);
            myChild.setOptions(radioOptionsPool);
            child = (View) myChild;
        } else if (name.equals(VSN_RANGE)) {
            CCRadioButton myChild = new CCRadioButton(this, NONE, null);
            myChild.setOptions(radioOptionsRange);
            child = (View) myChild;
        } else if (name.equals(REGEX)) {
            CCRadioButton myChild = new CCRadioButton(this, NONE, null);
            myChild.setOptions(radioOptionsRegEx);
            child = (View) myChild;
        } else if (name.equals(START_FIELD) ||
            name.equals(END_FIELD) ||
            name.equals(REGEX_FIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(SCRATCH_POOL_MENU)) {
            child = new CCDropDownMenu(this, name, null);
        // Action table Container.
        } else if (name.equals(CONTAINER_VIEW)) {
            child = new ImportVSNView(this, models, name);
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

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String libShareAcsServer = null;

        // set page title
        pageTitleModel.setPageTitleText(
             SamUtil.getResourceString(
                 "ImportVSN.pagetitle",
                 new String [] { getLibraryName() }));
        pageTitleModel.setPageTitleHelpMessage("ImportVSN.instruction");

        // Fresh off from start, populate all filter information.
        // save in page session, and skip populating table model
        try {
            populateFilterComponents();

            // retrieve list of libraries that share the same acs server
            libShareAcsServer =
                (String) getPageSessionAttribute(PSA_SAME_ACSLS_LIB);
            if (libShareAcsServer == null) {
                Library myLibrary =
                    SamUtil.getModel(getServerName()).
                        getSamQFSSystemMediaManager().
                        getLibraryByName(getLibraryName());
                libShareAcsServer =
                    myLibrary.getLibraryNamesWithSameACSLSServer();
                setPageSessionAttribute(PSA_SAME_ACSLS_LIB, libShareAcsServer);
            }


        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to populate filter content",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ImportVSN.error.populate.filter",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
            TraceUtil.trace3("Exiting, fail to populate filter content!");
        }

        CCRadioButton radio = (CCRadioButton) getChild(NONE);
        if (radio.getValue() == null) {
            radio.setValue("ImportVSN.radio.none");
        }

        String filterCriteria =
            (String) getParentViewBean().getPageSessionAttribute(
                PSA_FILTER_CRITERIA);

        if (filterCriteria == null) {
            TraceUtil.trace3("Exiting, don't populate AT Model");
            return;
        }

        // Filter is already populated with user settings, now go populate
        // table model and show to user

        try {
            // populate the action table model
            ImportVSNView view = (ImportVSNView) getChild(CONTAINER_VIEW);
            view.populateTableModel(serverName);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to populate VSNs for this ACSLS Library",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ImportVSN.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    private void initializeTableModels() {
	models = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model = new CCActionTableModel(
            sc, "/jsp/media/ImportVSNTable.xml");

	models.put(ImportVSNView.VSN_TABLE, model);
    }

    private void populateFilterComponents() throws SamFSException {
        int [] pools = {0};

        ImportVSNFilterHelper helper =
            (ImportVSNFilterHelper) getPageSessionAttribute(PSA_FILTER_CONTENT);

        if (helper == null) {
            Library myLibrary =
                SamUtil.getModel(getServerName()).getSamQFSSystemMediaManager().
                    getLibraryByName(getLibraryName());
            if (myLibrary == null) {
                // the library info is not found
                // could be that cataserverd is not running
                throw new SamFSException(null, -2517);
            }
            StkNetLibParam param = myLibrary.getStkNetLibParam();
            // param can be null (no Exception is thrown), so check
            if (param == null) {
                // the client connection info is not found
                // could be that cataserverd is not running
                throw new SamFSException(null, -2517);
            }

            StkPool [] stkPools =  myLibrary.getPhyConfForStkLib();

            if (stkPools == null) {
                pools = new int[0];
            } else {
                pools = new int[stkPools.length];
                for (int i = 0; i < stkPools.length; i++) {
                    pools[i] = stkPools[i].getPoolID();
                }
            }
            // preserve filter content
            setPageSessionAttribute(
                PSA_FILTER_CONTENT,
                new ImportVSNFilterHelper(pools));
        } else {
            // load filter content
            pools  = helper.getPools();
        }

        CCDropDownMenu poolMenu  = (CCDropDownMenu) getChild(SCRATCH_POOL_MENU);
        poolMenu.setOptions(createOptionList(pools));
    }

    private OptionList createOptionList(int [] poolIDs) {
        String [] poolIDArray = new String[poolIDs.length];

        for (int i = 0; i < poolIDs.length; i++) {
            poolIDArray[i] = Integer.toString(poolIDs[i]);
        }

        return new OptionList(poolIDArray, poolIDArray);
    }

    public void handleFilterButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        ImportVSNFilter myFilter = null;

        String selection = (String) getDisplayFieldValue(NONE);
        try {
            if (selection.equals("ImportVSN.radio.none")) {
                myFilter =
                    new ImportVSNFilter(
                        Media.NO_FILTER, null,
                        null, null, null,
                        -1);
            } else if (selection.equals("ImportVSN.radio.scratchpool")) {
                String pool = (String) getDisplayFieldValue(SCRATCH_POOL_MENU);
                myFilter =
                    new ImportVSNFilter(
                        Media.FILTER_BY_SCRATCH_POOL,
                        null, null, null, null,
                        Integer.parseInt(pool));


            } else if (selection.equals("ImportVSN.radio.vsnrange")) {
                String startVSN = (String) getDisplayFieldValue(START_FIELD);
                String endVSN   = (String) getDisplayFieldValue(END_FIELD);
                startVSN = startVSN.trim();
                endVSN   = endVSN.trim();

                myFilter =
                    new ImportVSNFilter(
                        Media.FILTER_BY_VSN_RANGE, null,
                        startVSN, endVSN, null,
                        -1);

            } else if (selection.equals("ImportVSN.radio.regex")) {
                String regEx = (String) getDisplayFieldValue(REGEX_FIELD);
                regEx = regEx.trim();

                myFilter =
                    new ImportVSNFilter(
                        Media.FILTER_BY_VSN_EXPRESSION,
                        null, null, null, regEx,
                        -1);
            }
        } catch (SamFSException samEx) {
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ImportVSN.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        if (myFilter != null) {
            setPageSessionAttribute(PSA_FILTER_CRITERIA, myFilter.toString());
        }
        forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(LIBRARY_SUMMARY_HREF);
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

    public void handleLibraryDriveSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(LIBRARY_DRIVE_SUMMARY_HREF);
        ViewBean targetViewBean =
            getViewBean(LibraryDriveSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetViewBean.getName()),
            str);
        forwardTo(targetViewBean);
        TraceUtil.trace3("Exiting");
    }

    private String getLibraryName() {
        String libraryName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        if (libraryName == null) { // must have come from common tasks
            libraryName = RequestManager
                .getRequestContext()
                .getRequest()
                .getParameter("LIBRARY_NAME");
            setPageSessionAttribute(Constants
                                    .PageSessionAttributes
                                    .LIBRARY_NAME,
                                    libraryName);
        }

        return libraryName;
    }
}
