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

// ident	$Id: EditVSNViewBean.java,v 1.30 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;

import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import java.io.IOException;
import javax.servlet.ServletException;



/**
 *  This class is the view bean for the Edit VSN Attribute page
 */

public class EditVSNViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "EditVSN";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/media/EditVSN.jsp";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;

    private static final String DAMAGED = "damagedValue";
    private static final String DUPLICATE = "duplicateValue";
    private static final String READ_ONLY = "readonlyValue";
    private static final String WRITE_PROTECTED = "writeprotectedValue";
    private static final String FOREIGN = "foreignValue";
    private static final String RECYCLE = "recycleValue";
    private static final String FULL = "fullValue";
    private static final String UNAVAILABLE = "unavailableValue";
    private static final String NEED_AUDIT = "needAuditValue";

    /**
     * Constructor
     */
    public EditVSNViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
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
        TraceUtil.trace3(new NonSyncStringBuffer().append("Entering: name is ").
            append(name).toString());

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
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer().append(
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
        loadPropertySheetModel(propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * disableCheckBoxes()
     * Called if there are any errors displaying the initial values of the
     * checkboxes
     */
    private void disableCheckBoxes() {
        TraceUtil.trace3("Entering");
        ((CCCheckBox)getChild(DAMAGED)).setDisabled(true);
        ((CCCheckBox)getChild(DUPLICATE)).setDisabled(true);
        ((CCCheckBox)getChild(READ_ONLY)).setDisabled(true);
        ((CCCheckBox)getChild(WRITE_PROTECTED)).setDisabled(true);
        ((CCCheckBox)getChild(FOREIGN)).setDisabled(true);
        ((CCCheckBox)getChild(RECYCLE)).setDisabled(true);
        ((CCCheckBox)getChild(FULL)).setDisabled(true);
        ((CCCheckBox)getChild(UNAVAILABLE)).setDisabled(true);
        ((CCCheckBox)getChild(NEED_AUDIT)).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    /**
     * disablePageButtons()
     * Called if there are any errors displaying the initial values of the
     * checkboxes
     */
    private void disablePageButtons() {
        TraceUtil.trace3("Entering");
        ((CCButton) getChild("Submit")).setDisabled(true);
        ((CCButton) getChild("Reset")).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate the page title model
     *
     * @return CCPageTitleModel The page title model of EditVSN Page
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/EditVSNPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Instantiate the PropertySheet model
     *
     * @return CCPropertySheetModel The Property Sheet Model of EditVSN Page
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/media/EditVSNPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Function to retrieve the real data from API,
     * then assign these data to propertysheet model to display
     *
     * @param propetySheetModel The Property Sheet Model of EditVSN page
     */
    private void loadPropertySheetModel(
        CCPropertySheetModel propertySheetModel) {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();

        try {
            VSN myVSN = null;

            try {
                myVSN = getThisVSN();
            } catch (NumberFormatException numEx) {
                // Could not retrieve VSN
                SamUtil.doPrint(new NonSyncStringBuffer().append(
                    "NumberFormatException caught while ").append(
                    "parsing slotKey to integer.").toString());
                throw new SamFSException(null, -2507);
            }

            // populate the checkbox base on the VSN Attributes
            ((CCCheckBox) getChild(DAMAGED)).setChecked(
                myVSN.isMediaDamaged());
            ((CCCheckBox) getChild(DUPLICATE)).setChecked(
                myVSN.isDuplicateVSN());
            ((CCCheckBox) getChild(READ_ONLY)).setChecked(
                myVSN.isReadOnly());
            ((CCCheckBox) getChild(WRITE_PROTECTED)).setChecked(
                myVSN.isWriteProtected());
            ((CCCheckBox) getChild(FOREIGN)).setChecked(
                myVSN.isForeignMedia());
            ((CCCheckBox) getChild(RECYCLE)).setChecked(
                myVSN.isRecycled());
            ((CCCheckBox) getChild(FULL)).setChecked(
                myVSN.isVolumeFull());
            ((CCCheckBox) getChild(UNAVAILABLE)).setChecked(
                myVSN.isUnavailable());
            ((CCCheckBox) getChild(NEED_AUDIT)).setChecked(
                myVSN.isNeedAudit());

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadPropertySheetModel",
                "Failed to retrieve VSN information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "EditVSN.error.loadpsheet",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
            disablePageButtons();
            disableCheckBoxes();
        }
        TraceUtil.trace3("Exiting");
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        VSN myVSN = null;
        String vsnName = "";

        try {
            // Check Permission
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG)) {
                throw new SamFSException("common.nopermission");
            }

            try {
                myVSN = getThisVSN();
                vsnName = myVSN.getVSN();
            } catch (NumberFormatException numEx) {
                // Could not retrieve VSN
                SamUtil.doPrint("NumberFormatException caught while " +
                    "parsing slotKey to integer.");
                throw new SamFSException(null, -2507);
            }

            LogUtil.info(
                this.getClass(),
                "executeEditVSN",
                "Start editing attribute of tape");

            // Call API

            // damaged data
            myVSN.setMediaDamaged(Boolean.valueOf(
                (String) getDisplayFieldValue(DAMAGED)).booleanValue());

            // duplicate vsn
            myVSN.setDuplicateVSN(Boolean.valueOf(
                (String) getDisplayFieldValue(DUPLICATE)).booleanValue());

            // read-only
            myVSN.setReadOnly(Boolean.valueOf(
                (String) getDisplayFieldValue(READ_ONLY)).booleanValue());

            // write-protected
            myVSN.setWriteProtected(Boolean.valueOf(
                (String) getDisplayFieldValue(WRITE_PROTECTED)).booleanValue());

            // foreign media
            myVSN.setForeignMedia(Boolean.valueOf(
                (String) getDisplayFieldValue(FOREIGN)).booleanValue());

            // recycle
            myVSN.setRecycled(Boolean.valueOf(
                (String) getDisplayFieldValue(RECYCLE)).booleanValue());

            // volume is full
            myVSN.setVolumeFull(Boolean.valueOf(
                (String) getDisplayFieldValue(FULL)).booleanValue());

            // Unavailable
            myVSN.setUnavailable(Boolean.valueOf(
                (String) getDisplayFieldValue(UNAVAILABLE)).booleanValue());

            // Need Audit
            myVSN.setNeedAudit(Boolean.valueOf(
                (String) getDisplayFieldValue(NEED_AUDIT)).booleanValue());

            // Make effect of Attribute change
            myVSN.changeAttributes();
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                "Done editing attribute of tape");
            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "EditVSN.action.editattributes", vsnName),
                getServerName());

            setSubmitSuccessful(true);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to set Media Attributes",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "EditVSN.error.editattributes", vsnName),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
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

    private int getEQValue() throws NumberFormatException {
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

        return Integer.parseInt(eqValue);
    }

    private String getVSNName() {
        // first check the page session
        String vsnName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.VSN_NAME);

        // second check the request
        if (vsnName == null) {
            vsnName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.VSN_NAME);

            if (vsnName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.VSN_NAME,
                    vsnName);
            }

            if (vsnName == null) {
                throw new IllegalArgumentException("VSN_NAME not supplied");
            }
        }

        return vsnName;
    }

    private int getSlotNumber() throws SamFSException {
        // first check the page session
        String slotNum = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);

        // second check the request
        if (slotNum == null) {
            slotNum = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SLOT_NUMBER);

            if (slotNum != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.SLOT_NUMBER,
                    slotNum);
            }

            if (slotNum == null) {
                throw new IllegalArgumentException("Slot Number not supplied");
            }
        }

        return Integer.parseInt(slotNum);
    }

    private VSN getThisVSN() throws SamFSException, NumberFormatException {
        VSN myVSN = null;
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        // FROM LIBRARY / HISTORIAN
        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
                getLibraryByName(getLibraryName());
        if (myLibrary == null) {
            throw new SamFSException(null, -2501);
        }

        myVSN = myLibrary.getVSN(getSlotNumber());
        if (myVSN == null) {
            throw new SamFSException(null, -2507);
        }

        return myVSN;
    }
}
