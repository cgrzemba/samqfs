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

// ident	$Id: LabelTapeViewBean.java,v 1.30 2008/12/16 00:12:13 am143972 Exp $

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
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the LabelTape page
 */

public class LabelTapeViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "LabelTape";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/LabelTape.jsp";

    private static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;

    private static final String SUBMIT_BUTTON = "Submit";
    private static final String RESET_BUTTON  = "Reset";
    private static final String TYPE_VALUE = "typeValue";
    private static final String NAME_VALUE = "nameValue";
    private static final String SIZE_VALUE = "sizeValue";

    /**
     * Constructor
     */
    public LabelTapeViewBean() {
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
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(SamUtil.getResourceString(
                    "LabelTape.errMsg1")).append("###").append(
                    SamUtil.getResourceString("LabelTape.errMsg2")).append(
                    "###").append(SamUtil.getResourceString(
                    "LabelTape.renderMsg")).toString());
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer(
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
     * disablePageButtons()
     * Called if error occurred in the page
     */
    private void disablePageButtons() {
        TraceUtil.trace3("Entering");
        ((CCButton) getChild(SUBMIT_BUTTON)).setDisabled(true);
        ((CCButton) getChild(RESET_BUTTON)).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    /**
     * disableInputFields()
     * Called when error occurred in the page
     */
    private void disableInputFields() {
        TraceUtil.trace3("Entering");
        ((CCRadioButton) getChild(TYPE_VALUE)).setDisabled(true);
        ((CCTextField) getChild(NAME_VALUE)).setDisabled(true);
        ((CCDropDownMenu) getChild(SIZE_VALUE)).setDisabled(true);
        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/LabelTapePageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Function to create the PropertySheet model
     * based on the xml file.
     */
    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/media/LabelTapePropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Function to retrieve the real data from API,
     * then assign these data to propertysheet model to display
     */
    private void loadPropertySheetModel
        (CCPropertySheetModel propertySheetModel) {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();

        // pre-select to relabel
        propertySheetModel.setValue(TYPE_VALUE, "LabelTape.type2");

        try {
            VSN myVSN = getThisVSN();
            String barcode = myVSN.getBarcode();
            if (barcode == null) {
                ((CCTextField)getChild(NAME_VALUE)).setValue(
                    "LabelTape.insertnewvalue");
            } else {
                ((CCTextField)getChild(NAME_VALUE)).setValue(barcode);
            }
            long blockSize = myVSN.getBlockSize();
            propertySheetModel.setValue(SIZE_VALUE, new Long(blockSize));
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
                "LabelTape.error.loadpsheet",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
            // Disable fields and buttons if error populating page
            disablePageButtons();
            disableInputFields();
        }
        TraceUtil.trace3("Exiting");
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String labelType = (String) getDisplayFieldValue(TYPE_VALUE);
        String labelName = (String) getDisplayFieldValue(NAME_VALUE);
        long blockSize =
            Long.parseLong((String) getDisplayFieldValue(SIZE_VALUE));

        try {
            VSN myVSN = getThisVSN();

            if (labelType.equals("LabelTape.type1")) {
                // Labeling a tape
                LogUtil.info(this.getClass(), "handleLabelTapeHrefRequest",
                    new NonSyncStringBuffer("Start labeling tape to ").
                    append(labelName).toString());
                long jobID = myVSN.label(VSN.LABEL, labelName, blockSize);
                LogUtil.info(this.getClass(), "handleLabelTapeHrefRequest",
                    new NonSyncStringBuffer("Done labeling tape to ").
                    append(labelName).append(" with Job ID ").append(
                    Long.toString(jobID)).toString());

                setLabelTapeSuccessAlert(
                    "success.summary",
                    true,
                    labelName,
                    jobID);

            } else if (labelType.equals("LabelTape.type2")) {
                // Re-labeling a tape
                LogUtil.info(this.getClass(), "handleLabelTapeHrefRequest",
                    new NonSyncStringBuffer(
                    "Start re-labeling tape to ").append(labelName).toString());
                long jobID = myVSN.label(VSN.RELABEL, labelName, blockSize);
                LogUtil.info(this.getClass(), "handleLabelTapeHrefRequest",
                    new NonSyncStringBuffer(
                    "Done re-labeling tape to ").append(labelName).toString());

                setLabelTapeSuccessAlert(
                    "success.summary",
                    false,
                    labelName,
                    jobID);
            }
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleLabelTapeHrefRequest",
                "Failed to label/relabel VSN",
                getServerName());
            if (labelType.equals("LabelTape.type1")) {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonSecondaryViewBeanBase.ALERT,
                    "VSNSummary.error.label",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            } else {
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonSecondaryViewBeanBase.ALERT,
                    "VSNSummary.error.relabel",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }
        getParentViewBean().forwardTo(getRequestContext());
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

        // FROM LIBRARY
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

    private void setLabelTapeSuccessAlert(String summary,
                                          boolean isLabel,
                                          String newVSNString,
                                          long jobID) {
        String message;
        if (jobID < 0) {
            if (isLabel) {
                message = SamUtil.getResourceString(
                    "VSNSummary.msg.label", newVSNString);
            } else {
                message = SamUtil.getResourceString(
                    "VSNSummary.msg.relabel", newVSNString);
            }
        } else {
            if (isLabel) {
                message = SamUtil.getResourceString(
                        "VSNSummary.msg.label",
                        new String [] {newVSNString, Long.toString(jobID)});
            } else {
                message = SamUtil.getResourceString(
                    "VSNSummary.msg.relabel",
                    new String [] {newVSNString, Long.toString(jobID)});
            }
        }
        MediaUtil.setLabelTapeInfoAlert(
            getParentViewBean(),
            CommonSecondaryViewBeanBase.ALERT,
            summary,
            message);

        setSubmitSuccessful(true);
    }
}
