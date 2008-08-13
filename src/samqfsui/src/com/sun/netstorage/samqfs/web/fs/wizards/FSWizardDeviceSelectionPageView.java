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

// ident	$Id: FSWizardDeviceSelectionPageView.java,v 1.20 2008/08/13 20:56:13 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for device selection page.
 *
 */
public class FSWizardDeviceSelectionPageView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "FSWizardDeviceSelectionPageView";

    // Child view names (i.e. display fields).
    public static final String CHILD_ACTIONTABLE = "DeviceSelectionTable";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";
    public static final String CHILD_LABEL = "counterLabel";
    public static final String CHILD_INIT_SELECTED = "initSelected";
    public static final String CHILD_COUNTER = "counter";

    protected CCActionTableModel tableModel = null;
    protected int initSelected = 0, totalItems = 0;
    protected boolean previous_error = false;

    private static final String
        CHILD_TILED_VIEW = "FSWizardDeviceSelectionPageTiledView";
    private boolean error = false;

    // protected String sharedChecked = null;
    protected boolean sharedEnabled = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardDeviceSelectionPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public FSWizardDeviceSelectionPageView(
        View parent, Model model, String name) {

        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);
        createActionTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Child manipulation methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_TILED_VIEW,
            FSWizardDeviceSelectionPageTiledView.class);
        registerChild(CHILD_ACTIONTABLE, CCActionTable.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_INIT_SELECTED, CCHiddenField.class);
        registerChild(CHILD_COUNTER, CCTextField.class);
        tableModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering to create child " + name);
        if (name.equals(CHILD_ACTIONTABLE)) {
            CCActionTable child = new CCActionTable(this, tableModel, name);
            // Set the TileView object.
            child.setTiledView((ContainerView)
                getChild(CHILD_TILED_VIEW));
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_TILED_VIEW)) {
            FSWizardDeviceSelectionPageTiledView child =
                new FSWizardDeviceSelectionPageTiledView(this,
                tableModel, name);
            return child;
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            child.setValue(CCAlertInline.TYPE_ERROR);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_ERROR)) {
            CCHiddenField child = null;
            if (error) {
                child = new CCHiddenField(this, name, "exception");
            } else {
                child = new CCHiddenField(this, name, "success");
            }
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_LABEL)) {
            TraceUtil.trace3("Exiting");
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_COUNTER)) {
            TraceUtil.trace3("Exiting");
            return new CCTextField(this, name, null);
        } else if (name.equals(CHILD_INIT_SELECTED)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (tableModel.isChildSupported(name)) {
            // Create child from action table model.
            View child = tableModel.createChild(this, name);
            return child;
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * Derived class should overwrite this method.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        String url = null;

        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        /*
        sharedChecked = (String) wm.getWizardValue(
            NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        */

        Boolean temp = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled = temp == null ? false : temp.booleanValue();

        if (!previous_error) {
            if (sharedEnabled) {
                // if (sharedChecked != null && sharedChecked.equals("true")) {
                url = "/jsp/fs/FSWizardSharedDeviceSelectionPage.jsp";
            } else {
                url = "/jsp/fs/FSWizardDeviceSelectionPage.jsp";
            }
        } else {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }


    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)
            wm.getValue(Constants.Wizard.SERVER_NAME);

        try {
            TraceUtil.trace3("Calling populateActionTableModel");
            populateActionTableModel();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "createActionTableModel()",
                "Failed to populate ActionTable's data",
                serverName);
            error = true;
            CCHiddenField temp = (CCHiddenField) getChild(CHILD_ERROR);
            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                "FSWizard.new.error.steps",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        if (error) {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue("exception");
        } else {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue("success");
        }

        //sharedChecked = (String) wm.getWizardValue(
        //    NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        Boolean temp = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled = temp == null ? false : temp.booleanValue();

        String t = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "FSWizard.new.error.steps";
            previous_error = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary =
                    (String) wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previous_error = false;
                } else {
                    previous_error = true;
                }
            }
            if (previous_error) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    serverName);
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }

        // Fill in the initSelected Hiddenfield with the initial number of
        // selected items
        ((CCHiddenField) getChild(CHILD_INIT_SELECTED)).setValue(
            Integer.toString(initSelected) + "," + totalItems);
        ((CCTextField) getChild(CHILD_COUNTER)).setValue(
            Integer.toString(initSelected));

        TraceUtil.trace3("Exiting");
    }

    /**
     * Create an Empty ActionTable Model and set all the column headings
     */
    protected void createActionTableModel() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String samfsServerAPIVersion =
            (String) wm.getValue(Constants.Wizard.SERVER_API_VERSION);

        Boolean temp = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled =
            temp == null ? false : temp.booleanValue();

        /*
        sharedChecked = (String) wm.getWizardValue(
            NewWizardFSNameView.CHILD_SHARED_CHECKBOX);
        if (sharedChecked != null && sharedChecked.equals("true")) {
        */
        
        if (sharedEnabled) {
            tableModel = new CCActionTableModel(
               RequestManager.getRequestContext().getServletContext(),
               "/jsp/fs/FSWizardSharedDeviceSelectionTable.xml");
            tableModel.setActionValue(
                "ColAvailableFrom",
                "FSWizard.deviceSelectionTable.availableFromHeading");
            tableModel.setActionValue(
                "ColDevicePath",
                "FSWizard.deviceSelectionTable.deviceNameHeading");
            tableModel.setActionValue(
                "ColPartition",
                "FSWizard.deviceSelectionTable.partitionTypeHeading");
            tableModel.setActionValue(
                "ColVendor",
                "FSWizard.deviceSelectionTable.vendorHeading");
            tableModel.setActionValue(
                "ColProductID",
                "FSWizard.deviceSelectionTable.productIDHeading");
            tableModel.setActionValue(
                "ColCapacity",
                "FSWizard.deviceSelectionTable.capacityHeading");
        } else {
            tableModel = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/fs/FSWizardDeviceSelectionTable.xml");

            tableModel.setActionValue(
                "ColDevicePath",
                "FSWizard.deviceSelectionTable.deviceNameHeading");
            tableModel.setActionValue(
                "ColPartition",
                "FSWizard.deviceSelectionTable.typeHeading");
            tableModel.setActionValue(
                "ColCapacity",
                "FSWizard.deviceSelectionTable.capacityHeading");
            tableModel.setActionValue(
                "ColVendor",
                "FSWizard.deviceSelectionTable.vendorHeading");
            tableModel.setActionValue(
                "ColProductID",
                "FSWizard.deviceSelectionTable.productIDHeading");
        }

        TraceUtil.trace2("Leaving createActionTableModel");
    }

    /**
     * All Allocatable Units are stored wizard Model.
     * This way we can avoid calling the backend everytime the user hits
     * the previous button.
     * Also this will help in keeping track of all the selections that the user
     * made during the course of the wizard, and will he helpful to remove
     * those entries for the Metadata LUN selection page.
     */
    protected void populateActionTableModel() throws SamFSException {
        TraceUtil.trace3("Entering");
        // This method should be over-written by the derived class
        TraceUtil.trace3("Exiting");
    }

    public boolean beginCounterLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        // fsType is null when page is used in Grow Wizard
        String fsType = (String) wm.getValue(CreateFSWizardImpl.FSTYPE_KEY);

        CCLabel counterLabel =
            ((CCLabel) getChild(FSWizardDeviceSelectionPageView.CHILD_LABEL));
        if (fsType == null || fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
            counterLabel.setVisible(false);
            TraceUtil.trace3("Returned false");
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    public boolean beginCounterDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        // fsType is null when page is used in Grow Wizard
        String fsType = (String) wm.getValue(CreateFSWizardImpl.FSTYPE_KEY);

        CCTextField counter = ((CCTextField)
            getChild(FSWizardDeviceSelectionPageView.CHILD_COUNTER));
        if (fsType == null || fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
            counter.setVisible(false);
            TraceUtil.trace3("Returned false");
            return false;
        }

        TraceUtil.trace3("Exiting");
        return true;
    }
}
