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

// ident	$Id: ChangeStatusViewBean.java,v 1.31 2008/03/17 14:43:38 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.Constants;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Change Status page
 */

public class ChangeStatusViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ChangeStatus";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/ChangeStatus.jsp";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel  = null;

    private static final int PAGE_LIBRARY_SUMMARY = 100;
    private static final int PAGE_LIBRARY_DRIVE_SUMMARY = 101;

    /**
     * Constructor
     */
    public ChangeStatusViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        createPageTitleModel();
        createPropertySheetModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");

    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Entering: child name = ").append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else  {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);

        // always set drop down value to ON
        propertySheetModel.setValue("newValue", "0");

        try {
            loadPropertySheetModel();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadPropertySheetModel",
                "Failed to retrieve current state",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "ChangeStatus.error.loadpsheet",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
            disableAllFields();
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Create page title model
     *
     */

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/ChangeStatusPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to create the PropertySheet model
     * based on the xml file.
     *
     */
    private void createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/media/ChangeStatusPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to disable all fields in the page when error occurs
     *
     */
    private void disableAllFields() {
        TraceUtil.trace3("Entering");
        ((CCButton) getChild("Submit")).setDisabled(true);
        ((CCDropDownMenu) getChild("newValue")).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Function to retrieve the real data from API,
     * then assign these data to propertysheet model to display
     *
     */
    private void loadPropertySheetModel() throws SamFSException {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();
        populateStatusValue();
        TraceUtil.trace3("Exiting");
    }


    /**
     * populateStatusValue()
     * Pre-populate the current state value of the device
     */
    private void populateStatusValue() throws SamFSException {
        TraceUtil.trace3("Entering");
        int current_state = -1;

        // Change Status page is accessible from three places:
        // (1) Library Summary Page
        // (2) Library Drive Summary Page
        //
        // Use parentPage to determine the parent of the pop up
        // then use the appropriate key to populate the state dropdown
        //
        // For (1), use libraryString
        // For (2), use libraryString and eqValue
        //

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        switch (getParentPage()) {
            case PAGE_LIBRARY_SUMMARY:
                current_state = MediaUtil.getLibraryObject(
                    getServerName(), getLibraryName()).getState();
                break;

            case PAGE_LIBRARY_DRIVE_SUMMARY:
                current_state = getSelectedLibraryDrive(
                    sysModel,
                    MediaUtil.getLibraryObject(
                        getServerName(), getLibraryName())).
                    getState();
                break;

        }

        propertySheetModel.setValue(
            "currentValue", SamUtil.getStateString(current_state));

        TraceUtil.trace3("Exiting");
    }

    private Drive getSelectedLibraryDrive(
        SamQFSSystemModel sysModel,
        Library myLibrary)
        throws SamFSException {

        Drive [] allDrives = myLibrary.getDrives();

        try {
            for (int i = 0; i < allDrives.length; i++) {
               if (allDrives[i].getEquipOrdinal() ==
                   Integer.parseInt(getEQValue())) {
                    return allDrives[i];
                }
            }

            // Throw Exception if no drive is found
            throw new SamFSException(null, -2503);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace3(
                "NumberFormatException caught while parsing EQ value");
            throw new SamFSException(null, -2503);
        }
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        boolean hasError = false;
        int value = 0;
        Drive myDrive = null;

        try {
            value = Integer.parseInt(
                propertySheetModel.getValue("newValue").toString());
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("Developers' bug found in handleSubmitRequest");
            TraceUtil.trace1("Reason: " + numEx.getMessage());
        }

        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "Change Status: value is ").append(value).toString());

        try {
            switch (getParentPage()) {
                case PAGE_LIBRARY_SUMMARY:
                    Library myLibrary = MediaUtil.getLibraryObject(
                        getServerName(), getLibraryName());
                    LogUtil.info(this.getClass(),
                        "handleSubmitRequest",
                        new NonSyncStringBuffer().append(
                            "Start changing state of Library named ").
                            append(getLibraryName()).toString());
                    SamUtil.doPrint(new NonSyncStringBuffer().append(
                        "Set Library status to ").append(value).toString());
                    myLibrary.setState(value);
                    LogUtil.info(this.getClass(),
                        "handleChangeStatusHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Done changing state of library named").
                            append(getLibraryName()).toString());
                    setSuccessAlert(
                        "LibrarySummary.action.changestatus", getLibraryName());
                    break;

                case PAGE_LIBRARY_DRIVE_SUMMARY:
                    myDrive = getSelectedLibraryDrive(
                        SamUtil.getModel(getServerName()),
                        MediaUtil.getLibraryObject(
                            getServerName(), getLibraryName()));
                    LogUtil.info(this.getClass(),
                        "handleChangeStatusHrefRequest",
                        new NonSyncStringBuffer(
                            "Start changing status of drive EQ ").
                            append(getEQValue()).toString());
                    myDrive.setState(value);
                    LogUtil.info(this.getClass(),
                        "handleChangeStatusHrefRequest",
                        new NonSyncStringBuffer(
                            "Done changing status of drive EQ ").
                            append(getEQValue()).toString());
                    setSuccessAlert(
                        "LibraryDriveSummary.action.changestatus",
                        getEQValue());
                    break;
            }
        } catch (SamFSException samEx) {
            hasError = true;
            TraceUtil.trace1("Failed to change library/drive state");
            setErrorAlertAndProcessException(samEx);
        }

        // If there is no error, check the new state of the device
        // First check if the state has been changed
        // If not, sleep for 5 sec.  Otherwise, return
        if (!hasError) {
            try {
                switch (getParentPage()) {
                    case PAGE_LIBRARY_SUMMARY:
                        Library myLibrary = MediaUtil.getLibraryObject(
                            getServerName(), getLibraryName());

                        if (myLibrary.getState() != value) {
                            // sleep for 5 seconds,
                            // give time for underlying call to be completed
                            TraceUtil.trace2("Sleep 5 seconds ...");
                            Thread.sleep(5000);
                        }
                        break;

                    case PAGE_LIBRARY_DRIVE_SUMMARY:
                        myDrive = getSelectedLibraryDrive(
                            SamUtil.getModel(getServerName()),
                            MediaUtil.getLibraryObject(
                                getServerName(), getLibraryName()));
                        if (myDrive.getState() != value) {
                            TraceUtil.trace2("Sleep 5 seconds ...");
                            Thread.sleep(5000);
                        }
                        break;
                }
            } catch (SamFSException samEx) {
                TraceUtil.trace1(
                    "Failed to retrieve library state after changing it!");
                setErrorAlertAndProcessException(samEx);

            } catch (InterruptedException intEx) {
                // impossible for other thread to interrupt this thread
                // Continue to load the page
                TraceUtil.trace3("InterruptedException Caught: Reason: "
                    + intEx.getMessage());
            }
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(String msg, String item) {
        SamUtil.setInfoAlert(
            this,
            CommonSecondaryViewBeanBase.ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            getServerName());

        setSubmitSuccessful(true);
    }

    private void setErrorAlertAndProcessException(SamFSException samEx) {
        SamUtil.processException(
            samEx,
            this.getClass(),
            "handleSubmitRequest",
            "Failed to change state of a library/drive",
            getServerName());
        String errMsg = null;

        switch (getParentPage()) {
            case PAGE_LIBRARY_SUMMARY:
                errMsg = "LibrarySummary.error.changestatus";
                break;
            case PAGE_LIBRARY_DRIVE_SUMMARY:
                errMsg = "LibraryDriveSummary.error.changestatus";
                break;
        }

        SamUtil.setErrorAlert(
            this,
            CommonSecondaryViewBeanBase.ALERT,
            errMsg,
            samEx.getSAMerrno(),
            samEx.getMessage(),
            getServerName());
    }

    private String getLibraryName() {
        // first check the page session
        String libraryName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);

        // second check the request
        if (libraryName == null) {
            libraryName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.LIBRARY_NAME);

            if (libraryName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.LIBRARY_NAME,
                    libraryName);
            }

            if (libraryName == null) {
                throw new IllegalArgumentException("Library Name not supplied");
            }
        }

        return libraryName;
    }

    private String getEQValue() {
        // first check the page session
        String eqValue = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.EQ);

        // second check the request
        if (eqValue == null) {
            eqValue = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.EQ);

            if (eqValue != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.EQ,
                    eqValue);
            }

            if (eqValue == null) {
                throw new IllegalArgumentException("EQ value not supplied");
            }
        }

        return eqValue;
    }

    private int getParentPage() {
        // first check the page session
        Integer parentPage = (Integer) getPageSessionAttribute(
            Constants.PageSessionAttributes.PARENT);

        // second check the request
        if (parentPage == null) {
            String parentPageString = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.PARENT);

            if (parentPageString != null) {
                int parent = -1;
                if (parentPageString.equals("LibrarySummaryView")) {
                    parent = PAGE_LIBRARY_SUMMARY;
                } else if (parentPageString.equals("LibraryDriveSummaryView")) {
                    parent = PAGE_LIBRARY_DRIVE_SUMMARY;
                }
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.PARENT,
                    new Integer(parent));
            }
            parentPage = (Integer) getPageSessionAttribute(
                Constants.PageSessionAttributes.PARENT);
            if (parentPage == null) {
                throw new IllegalArgumentException("Parent Page not supplied");
            }
        }

        return parentPage.intValue();
    }
}
