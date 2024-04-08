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

// ident	$Id: AssignMediaView.java,v 1.6 2008/12/17 21:41:42 ronaldso Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.NewEditVSNPoolViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.media.VSNWrapper;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 * AssignMediaView is the view class of the AssignMediaPagelet.  This pagelet
 * is used in multiple places:
 *
 * 1. New Volume Pool
 * 2. Edit Volume Pool
 * 3. Policy Details page
 * 4. New Policy Copy page
 * 5. New Policy Wizard
 * 6. New FS Wizard
 */
public class AssignMediaView extends RequestHandlingViewBase
    implements CCWizardPage, CCPagelet {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "AssignMediaView";

    public static final String ALERT = "Alert";
    public static final String VOLUME_TABLE = "VolumeTable";
    public static final String TEXT_NAME_HELP = "TextNameHelp";
    public static final String LABEL_NAME = "LabelName";
    public static final String VALUE_NAME = "ValueName";
    public static final String LABEL_TYPE = "LabelType";
    public static final String MENU_TYPE = "MenuType";
    public static final String LABEL_METHOD = "LabelMethod";
    public static final String RADIO_METHOD = "RadioMethod";
    public static final String LABEL_INCLUDE = "LabelInclude";
    public static final
        String TEXT_INSTRUCTION_TOFROM = "TextInstructionToFrom";
    public static final
        String TEXT_INSTRUCTION_RANGE = "TextInstructionRange";
    public static final String LABEL_FROM = "LabelFrom";
    public static final String LABEL_TO = "LabelTo";
    public static final String LABEL_EXPRESSION = "LabelExpression";
    public static final String VALUE_FROM = "ValueFrom";
    public static final String VALUE_TO = "ValueTo";
    public static final String VALUE_EXPRESSION = "ValueExpression";
    public static final String BUTTON_SELECTED = "ButtonSelected";
    public static final String BUTTON_ALL = "ButtonAll";
    public static final String HREF_RADIO_METHOD = "HrefRadioMethod";
    public static final String HIDDEN_PAGE_MODE = "HiddenPageMode";
    public static final String HIDDEN_SHOW_TABLE = "HiddenShowTable";
    public static final
        String HIDDEN_SELECTION_METHOD = "HiddenSelectionMethod";
    public static final
        String HIDDEN_NO_MEDIA_TYPE_MESSAGE = "HiddenNoMediaTypeMessage";
    public static final String EXPRESSION_USED = "HiddenExpressionUsed";

    // Only used in wizards to determine if user is going to create a pool or
    // an expression
    public static final String MEDIA_ASSIGN_MODE = "media_assign_mode";

    private Map tableModels = null;

    // Page Mode related information
    private short pageMode = -1;
    // TODO: replace with enum java 5.0
    public static final short MODE_WIZARD_NEW_POOL = 0;
    public static final short MODE_WIZARD_NEW_EXP = 1;
    public static final short MODE_NEW_POOL = 2;
    public static final short MODE_EDIT_POOL = 3;
    public static final short MODE_NEW_COPY = 4;
    public static final short MODE_COPY_INFO_EDIT_EXP = 5;
    public static final short MODE_COPY_INFO_NEW_POOL = 6;
    public static final short MODE_COPY_INFO_NEW_EXP = 7;

    // Determine if the action table needed to be shown
    private static final String PSA_SHOW_TABLE = "psa_show_table";

    // Number of maximum flags we want to show for VSN
    private static final int MAXIMUM_FLAGS = 2;



    /**
     * This constructor is used in non-wizard situations.  WizardModel is
     * required in the wizard.  This variable will be null if this pagelet
     * is not used in the wizards.
     */
    public AssignMediaView(View parent, String name, short pageMode) {
        this(parent, null, name);
        this.pageMode = pageMode;

        // Pre-populate values to pool name and pre-select media type
        // if user is editing an existing expression of a volume pool
        if (pageMode == MODE_EDIT_POOL ||
            pageMode == MODE_COPY_INFO_EDIT_EXP ||
            pageMode == MODE_COPY_INFO_NEW_POOL ||
            pageMode == MODE_COPY_INFO_NEW_EXP) {
            prepopulateRequestParams();
        }
    }

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AssignMediaView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    /**
     * This constructor is used by wizard.  wizardModel is the instance that
     * carries all wizard information.
     */
    public AssignMediaView(View parent, Model wizardModel, String name) {
        super(parent, name);
        TraceUtil.initTrace();

        // Set wizard model if used in wizards
        if (wizardModel != null) {
            String mediaViewMode =
                (String) wizardModel.getValue(MEDIA_ASSIGN_MODE);
            if ("pool".equals(mediaViewMode)) {
                pageMode = MODE_WIZARD_NEW_POOL;
            } else {
                pageMode = MODE_WIZARD_NEW_EXP;
            }
            setDefaultModel(wizardModel);
        }

        initializeTableModels();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * register page children
     */
    public void registerChildren() {
        getTableModel(VOLUME_TABLE).registerChildren(this);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(VOLUME_TABLE, CCActionTable.class);
        registerChild(TEXT_NAME_HELP, CCStaticTextField.class);
        registerChild(LABEL_NAME, CCLabel.class);
        registerChild(VALUE_NAME, CCTextField.class);
        registerChild(LABEL_TYPE, CCLabel.class);
        registerChild(MENU_TYPE, CCDropDownMenu.class);
        registerChild(LABEL_METHOD, CCLabel.class);
        registerChild(RADIO_METHOD, CCRadioButton.class);
        registerChild(LABEL_INCLUDE, CCLabel.class);
        registerChild(TEXT_INSTRUCTION_TOFROM, CCStaticTextField.class);
        registerChild(TEXT_INSTRUCTION_RANGE, CCStaticTextField.class);
        registerChild(LABEL_FROM, CCLabel.class);
        registerChild(LABEL_TO, CCLabel.class);
        registerChild(LABEL_EXPRESSION, CCLabel.class);
        registerChild(VALUE_FROM, CCTextField.class);
        registerChild(VALUE_TO, CCTextField.class);
        registerChild(VALUE_EXPRESSION, CCTextField.class);
        registerChild(BUTTON_SELECTED, CCButton.class);
        registerChild(BUTTON_ALL, CCButton.class);
        registerChild(HREF_RADIO_METHOD, CCHref.class);
        registerChild(HIDDEN_PAGE_MODE, CCHiddenField.class);
        registerChild(HIDDEN_SHOW_TABLE, CCHiddenField.class);
        registerChild(HIDDEN_SELECTION_METHOD, CCHiddenField.class);
        registerChild(HIDDEN_NO_MEDIA_TYPE_MESSAGE, CCHiddenField.class);
        registerChild(EXPRESSION_USED, CCHiddenField.class);
    }

    /**
     * create child for JATO display cycle
     */
    public View createChild(String name) {
        if (name.equals(VOLUME_TABLE)) {
            return new CCActionTable(this, getTableModel(name), name);
        } else if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(RADIO_METHOD)) {
            return new CCRadioButton(this, name, null);
        } else if (name.equals(MENU_TYPE)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.startsWith("Text")) {
            return new CCStaticTextField(this, name, null);
        } else if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.startsWith("Value")) {
            return new CCTextField(this, name, null);
        } else if (name.startsWith("Button")) {
            return new CCButton(this, name, null);
        } else if (name.startsWith("Hidden")) {
            return new CCHiddenField(this, name, null);
        } else if (name.startsWith("Href")) {
            return new CCHref(this, name, null);
        } else {
            CCActionTableModel model = getTableModel(VOLUME_TABLE);
            if (model != null && model.isChildSupported(name)) {
                return model.createChild(this, name);
            }
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // retrieve information from request, save it, and setting page mode
        // of this pagelet
        setPageMode(event);

        // determine the default value of selection method radio button
        initializeSelectionMethod();

        // populate error message hidden field(s)
        populateHiddenMessages();

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            // Populate media type drop down based on available media type
            initializeMediaTypeMenu(sysModel);

            // determine if the action table needs to be shown, populate
            // table content when necessary
            initializeTableComponent(sysModel);

        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException caught!", samEx);
            TraceUtil.trace1(samEx.getMessage());

            SamUtil.setErrorAlert(
                this,
                ALERT,
                "AssignMedia.error.initializepage",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    /** populate the criteria table model */
    public void populateTableModel(SamQFSSystemModel sysModel)
        throws SamFSException {
        // Retrieve the handle of the Server Selection Table
        CCActionTableModel tableModel = getTableModel(VOLUME_TABLE);
        tableModel.clear();

        SamQFSSystemMediaManager mediaManager =
                                    sysModel.getSamQFSSystemMediaManager();
        VSNWrapper wrapper = null;

        int mediaType = Integer.parseInt(
                    (String) getDisplayFieldValue(MENU_TYPE));
        String showType = (String) getDisplayFieldValue(RADIO_METHOD);

        TraceUtil.trace2("populateTableModel(): showType is " + showType);

        if ("range".equals(showType)) {
            String startVSN = (String) getDisplayFieldValue(VALUE_FROM);
            String endVSN   = (String) getDisplayFieldValue(VALUE_TO);

            TraceUtil.trace2(startVSN + " to " + endVSN);

            wrapper = mediaManager.evaluateVSNExpression(
                mediaType, null, -1,
                startVSN, endVSN, null,
                null, mediaManager.MAXIMUM_ENTRIES_FETCHED);
            if (startVSN.length() == 0 && endVSN.length() == 0) {
                tableModel.setTitle(
                    SamUtil.getResourceString(
                        "AssignMedia.table.title.range.blank"));
            } else {
                tableModel.setTitle(
                    SamUtil.getResourceStringWithoutL10NArgs(
                        "AssignMedia.table.title.range",
                        new String [] {
                            SamUtil.getMediaTypeString(mediaType),
                            startVSN,
                            endVSN,
                            Capacity.newCapacity(
                                wrapper.getFreeSpaceInMB(),
                                SamQFSSystemModel.SIZE_MB).toString()}));
            }
        } else {
            String expression = (String) getDisplayFieldValue(VALUE_EXPRESSION);

            TraceUtil.trace2("Expression is " + expression);

            wrapper = mediaManager.evaluateVSNExpression(
                mediaType, null, -1,
                null, null, expression,
                null, mediaManager.MAXIMUM_ENTRIES_FETCHED);
            if (expression.length() == 0) {
                tableModel.setTitle(
                    SamUtil.getResourceString(
                        "AssignMedia.table.title.regex.blank"));
            } else {
                tableModel.setTitle(
                    SamUtil.getResourceStringWithoutL10NArgs(
                        "AssignMedia.table.title.regex",
                        new String [] {
                            SamUtil.getMediaTypeString(mediaType),
                            expression,
                            Capacity.newCapacity(
                                wrapper.getFreeSpaceInMB(),
                                SamQFSSystemModel.SIZE_MB).toString()}));
            }
        }

        // Save the expression in hidden field to make it handy to reference
        // back to parent page or other purposes in the client side
        ((CCHiddenField) getChild(
            EXPRESSION_USED)).setValue(wrapper.getExpressionUsed());

        if (wrapper == null) {
            TraceUtil.trace1("populateTableModel(): Wrapper is null!");

            SamUtil.setErrorAlert(
                this,
                ALERT,
                "AssignMedia.error.populatevolume",
                -1,
                "",
                getServerName());
            return;
        }

        TraceUtil.trace2(
            "Total Matching VSNs: " + wrapper.getTotalNumberOfVSNs());

        if (MediaUtil.isDiskType(getMediaType())) {
            populateDiskVolumns(wrapper);
        } else {
            populateTapeVolumns(wrapper);
        }
    }

    /**
     * implement the CCPagelet interface
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        return "/jsp/media/AssignMediaPagelet.jsp";
    }

    /**
     * Get the expression that represents user's volumn selection
     * @return Expression that represents user's volumn selection
     */
    public String getExpression() throws SamFSException {
        String showType = (String) getDisplayFieldValue(RADIO_METHOD);
        VSNWrapper wrapper = null;
        SamQFSSystemMediaManager mediaManager =
            SamUtil.getModel(getServerName()).getSamQFSSystemMediaManager();
        if ("range".equals(showType)) {
            String startVSN = (String) getDisplayFieldValue(VALUE_FROM);
            String endVSN   = (String) getDisplayFieldValue(VALUE_TO);
            wrapper = mediaManager.evaluateVSNExpression(
                getMediaType(), null, -1,
                startVSN, endVSN, null,
                null, mediaManager.MAXIMUM_ENTRIES_FETCHED);
        } else {
            String expression = (String) getDisplayFieldValue(VALUE_EXPRESSION);
            wrapper = mediaManager.evaluateVSNExpression(
                getMediaType(), null, -1,
                null, null, expression,
                null, mediaManager.MAXIMUM_ENTRIES_FETCHED);
        }

        TraceUtil.trace3(
            "RETURN expression used: " + wrapper.getExpressionUsed());

        return wrapper.getExpressionUsed();
    }

    /**
     * @return pool name that either user is about to create, or about to be
     * edited by the user
     */
    public String getPoolName() {
        String poolName = null;
        if (pageMode == MODE_NEW_POOL ||
            pageMode == MODE_COPY_INFO_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_POOL) {
            poolName = (String) getDisplayFieldValue(VALUE_NAME);
        } else if (pageMode == MODE_EDIT_POOL) {
            NewEditVSNPoolViewBean parent =
                (NewEditVSNPoolViewBean) getParentViewBean();
            poolName = (String)
                parent.getPageSessionAttribute(parent.POOL_NAME);
        } else {
            // this case is never used
            poolName = "";
        }

        TraceUtil.trace2("getPoolName(): " + poolName);

        return poolName;
    }

    /**
     * @return the media type of the volume pool that user is about to create
     */
    public int getMediaType() {
        int mediaType = -1;
        if (pageMode == MODE_COPY_INFO_NEW_POOL) {
            mediaType =
                Integer.parseInt((String) getParentViewBean().
                    getPageSessionAttribute(NewEditVSNPoolViewBean.MEDIA_TYPE));
        } else {
            mediaType =
                Integer.parseInt((String) getDisplayFieldValue(MENU_TYPE));
        }

        TraceUtil.trace2("getMediaType(): " + mediaType);

        return mediaType;
    }

    /**
     * Handler for the selection method radio button
     * Triggered when user switches between media type
     */
    public void handleHrefRadioMethodRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Handler for the Show Selected Volume button
     */
    public void handleButtonSelectedRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        if (!validate(false)) {
            getParentViewBean().forwardTo(getRequestContext());
        } else {
            handleButtons();
        }
    }

    /**
     * Handler for the Show All Volume button
     */
    public void handleButtonAllRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        ((CCRadioButton) getChild(RADIO_METHOD)).setValue("regex");
        ((CCTextField) getChild(VALUE_EXPRESSION)).setValue(".");
        handleButtons();
    }

    private void handleButtons() {
        resetTable();

        getParentViewBean().setPageSessionAttribute(
                        PSA_SHOW_TABLE, new Boolean(true));
        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * This method is responsible of taking in request information if this
     * pagelet is a pop up, or to retrieve information from the wizardModel
     * if this pagelet is used in a wizard.  This method will preserve all the
     * necessary information that this pagelet needs, like media type (for edit
     * pool), copy number if there is any, etc.
     */
    private void setPageMode(DisplayEvent event) throws ModelControlException {
        if (pageMode == MODE_WIZARD_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_EXP) {
            super.beginDisplay(event);
            SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
            // To get around wizard not catching the resource bundle
            populateLabels();
        }

        ((CCHiddenField) getChild(HIDDEN_PAGE_MODE)).
            setValue(Integer.toString(pageMode));
    }

    /**
     * Validate user input
     * @param poolNameRequired
     * @param event - wizard event, null when view is not used within wizard
     * @return
     */
    public boolean validate(boolean poolNameRequired) {
        // check pool name & media type if it is needed
        TraceUtil.trace2(
            "validate: page mode: " + pageMode +
            " poolNameRequired: " + poolNameRequired);

        String errorMessage = null;
        boolean error = false;

        if (pageMode == MODE_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_EXP) {
            // Only validate the pool name field when we are about to create
            // a pool
            if (poolNameRequired) {
                String poolName = (String) getDisplayFieldValue(VALUE_NAME);
                poolName = poolName == null ? "" : poolName.trim();

                if (poolName.length() == 0) {
                    TraceUtil.trace1("No pool name is defined!");

                    errorMessage = "AssignMedia.error.nopoolname";
                    error = true;

                } else if (!SamUtil.isValidNameString(poolName)) {
                    // Check for well-formness
                    errorMessage = "AssignMedia.error.poolname.nonwellform";
                    error = true;

                } else {
                    try {
                        if (PolicyUtil.poolExists(getServerName(), poolName)) {
                            // Check if pool name is already in use
                            errorMessage =
                                "AssignMedia.error.poolname.duplicate";
                            error = true;
                        }
                    } catch (SamFSException samEx) {
                        TraceUtil.trace1(
                            "Failed to check if pool name is in use!", samEx);
                        errorMessage = "AssignMedia.error.poolname.duplicate";
                        error = true;
                    }
                }

                if (error) {
                    SamUtil.setErrorAlert(
                        this,
                        ALERT,
                        SamUtil.getResourceString(errorMessage),
                        -1,
                        "",
                        getServerName());
                    setLabelError(LABEL_NAME);
                    return false;
                }
            }

            if (getMediaType() ==
                Integer.parseInt(SelectableGroupHelper.NOVAL)) {
                TraceUtil.trace1("No media type is defined!");
                    SamUtil.setErrorAlert(
                        this,
                        ALERT,
                        SamUtil.getResourceString(
                            "AssignMedia.error.nomediatype"),
                        -1,
                        "",
                        getServerName());
                setLabelError(LABEL_TYPE);
                return false;
            }
        }
        // Check volumes to include fields based on selection method
        String showType = (String) getDisplayFieldValue(RADIO_METHOD);
        if ("range".equals(showType)) {
            TraceUtil.trace3("showType: range");

            String startVSN = (String) getDisplayFieldValue(VALUE_FROM);
            String endVSN   = (String) getDisplayFieldValue(VALUE_TO);
            startVSN = startVSN == null ? "" : startVSN;
            endVSN = endVSN == null ? "" : endVSN;

            TraceUtil.trace3(">>> start:" + startVSN);
            TraceUtil.trace3(">>> end" + endVSN);

            if (startVSN.length() == 0 && endVSN.length() == 0) {
                // error
                TraceUtil.trace1("No start and end volume is defined!");
                SamUtil.setErrorAlert(
                    this,
                    ALERT,
                    SamUtil.getResourceString("AssignMedia.error.nostartend"),
                    -1,
                    "",
                    getServerName());
                setLabelError(LABEL_FROM);
                return false;
            }
        } else {
            TraceUtil.trace3("showType: expression");

            String expression = (String) getDisplayFieldValue(VALUE_EXPRESSION);
            expression = expression == null ? "" : expression;

            TraceUtil.trace3(">>> expression:" + expression);

            if (expression.length() == 0) {
                // error
                TraceUtil.trace1("No expression is defined!");
                SamUtil.setErrorAlert(
                    this,
                    ALERT,
                    SamUtil.getResourceString("AssignMedia.error.noexpression"),
                    -1,
                    "",
                    getServerName());
                return false;
            }
        }
        return true;
    }

    /**
     * determine if the action table needs to be shown
     */
    private void initializeTableComponent(SamQFSSystemModel sysModel)
        throws SamFSException {
        Boolean showTable = (Boolean) getParentViewBean().
                                getPageSessionAttribute(PSA_SHOW_TABLE);
        ((CCHiddenField) getChild(HIDDEN_SHOW_TABLE)).setValue(
            showTable == null ?
                Boolean.toString(false) :
                showTable.toString());

        TraceUtil.trace3("initializeTableComponent: " +
            (showTable == null ?
                        Boolean.toString(false) :
                        showTable.toString()));

        // If table needs to be shown, initialize headers and populate content
        if (showTable != null && showTable.booleanValue()) {
            // Initialize Table Headers of the matching volumes
            initializeTableHeaders();

            // populate table model
            populateTableModel(sysModel);
        }
    }

    /**
     * This method populates the content of the media type drop down menu.
     * The content varies due to the availability of media types of the current
     * server.  This method will also figure out the appropriate default value
     * of this drop down if no value is set for this menu.  If disk is available
     * in the server and the copy number is 1, disk should be the default value.
     */
    private void initializeMediaTypeMenu(SamQFSSystemModel sysModel)
        throws SamFSException {
        CCDropDownMenu menu = (CCDropDownMenu) getChild(MENU_TYPE);

        int [] mediaTypes = sysModel.getSamQFSSystemMediaManager().
            getAvailableArchiveMediaTypes();

        String [] labels, values;
        int counter = 0;

        if (pageMode == MODE_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_EXP) {
            labels = new String[mediaTypes.length + 1];
            values = new String[mediaTypes.length + 1];
            labels[0] = SelectableGroupHelper.NOVAL_LABEL;
            values[0] = SelectableGroupHelper.NOVAL;
            counter = 1;
        } else {
            labels = new String[mediaTypes.length];
            values = new String[mediaTypes.length];
        }

        for (int i = 0; i < mediaTypes.length; i++, counter++) {
            labels[counter] = SamUtil.getMediaTypeString(mediaTypes[i]);
            values[counter] = Integer.toString(mediaTypes[i]);
        }

        menu.setOptions(new OptionList(labels, values));
    }

    /**
     * This method populates a default value of the selection method radio
     * button component.
     */
    private void initializeSelectionMethod() {
        CCRadioButton radio = (CCRadioButton) getChild(RADIO_METHOD);

        radio.setOptions(
            new OptionList(
                new String [] {
                    SamUtil.getResourceString("AssignMedia.method.range"),
                    SamUtil.getResourceString("AssignMedia.method.regex")},
                new String [] {
                    "range",
                    "regex"}));

        // Initialize default selection method to range if nothing is selected,
        // and this pop up is not being used by editing existing volume
        // expression.
        CCHiddenField method =
            (CCHiddenField) getChild(HIDDEN_SELECTION_METHOD);
        if (radio.getValue() == null) {
            radio.setValue("range");
            method.setValue("range");
        } else {
            method.setValue(radio.getValue());
        }
    }

    private void initializeTableModels() {
	tableModels = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model = new CCActionTableModel(
            sc, "/jsp/media/AssignMediaTable.xml");

	tableModels.put(VOLUME_TABLE, model);
    }

        /** initialize table header and radio button */
    private void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(VOLUME_TABLE);

        // set the column headers
        model.setActionValue(
            "ColName",
            SamUtil.getResourceString("AssignMedia.table.name"));
        model.setActionValue(
            "ColUsage",
            SamUtil.getResourceString("AssignMedia.table.usage"));
        model.setActionValue(
            "ColMediaFlag",
            SamUtil.getResourceString("AssignMedia.table.flag"));
        model.setActionValue(
            "ColAddInfo",
            MediaUtil.isDiskType(getMediaType()) ?
                SamUtil.getResourceString("AssignMedia.table.resource") :
                SamUtil.getResourceString("AssignMedia.table.accesscount"));
    }

    private void populateDiskVolumns(VSNWrapper wrapper) throws SamFSException {
        DiskVolume [] diskVols = wrapper.getAllDiskVSNs();
        CCActionTableModel actionTableModel = getTableModel(VOLUME_TABLE);

        for (int i = 0;
             i < diskVols.length &&
             i < SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED; i++) {
            if (i > 0) {
                actionTableModel.appendRow();
            }
            actionTableModel.setValue("VSNName", diskVols[i].getName());

            int usage = getUsage(
                            diskVols[i].getAvailableSpaceKB(),
                            diskVols[i].getCapacityKB());
            if (usage == -1) {
                actionTableModel.setValue("UsageText", "");
            } else {
                actionTableModel.setValue("UsageText", new Integer(usage));
            }
            actionTableModel.setValue(
                "TotalText",
                new Capacity(
                    diskVols[i].getCapacityKB(), SamQFSSystemModel.SIZE_KB));

            actionTableModel.setValue("UsageBarImage", getImageString(usage));
            actionTableModel.setValue(
                "MediaFlagText",
                MediaUtil.getFlagString(diskVols[i], MAXIMUM_FLAGS));

            String hostName = diskVols[i].getRemoteHost();
            String pathName = "";
            String serverName = getServerName();

            if (serverName.equals(hostName) ||
                hostName == null ||
                hostName.length() <= 0) {
                hostName = SamUtil.getResourceString(
                               "archiving.diskvsn.currentserver",
                               serverName);
            }

            // honeycomb targets
            if (diskVols[i].isHoneyCombVSN()) {
                String [] hp = hostName.split(":");
                pathName = hostName;
                hostName = hp[0];
            } else {
                pathName = diskVols[i].getPath();
            }

            actionTableModel.setValue(
                "AddInfoText",
                SamUtil.getResourceStringWithoutL10NArgs(
                    "AssignMedia.table.disk.resource",
                    new String [] {
                        pathName,
                        hostName}));
        }

        // Notify users that there are more than MAXIMUM_ENTRIES_FETCHED matches
        TraceUtil.trace3("Wrapper Total: " + wrapper.getTotalNumberOfVSNs());
        TraceUtil.trace3("MAXIMUM_ENTRIES_FETCHED: " +
                        SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED);
        if (wrapper.getTotalNumberOfVSNs() >
            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED) {
            int totalEntry = wrapper.getTotalNumberOfVSNs();
            SamUtil.setInfoAlert(
                this,
                ALERT,
                SamUtil.getResourceString(
                    "AssignMedia.table.tightenRange.summary",
                    new String [] {
                        Integer.toString(
                            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED),
                        Integer.toString(totalEntry),
                        Capacity.newCapacity(
                            wrapper.getFreeSpaceInMB(),
                            SamQFSSystemModel.SIZE_MB).toString()}),
                "",
                getServerName());
            actionTableModel.setTitle(
                SamUtil.getResourceString(
                    "AssignMedia.table.title.partial",
                    Integer.toString(
                        SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED)));
        }
    }

    private void populateTapeVolumns(VSNWrapper wrapper) throws SamFSException {
        CCActionTableModel actionTableModel = getTableModel(VOLUME_TABLE);
        VSN [] vsns = wrapper.getAllTapeVSNs();
        for (int i = 0;
             i < vsns.length &&
             i < SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED; i++) {
            if (i > 0) {
                actionTableModel.appendRow();
            }
            actionTableModel.setValue("VSNName", vsns[i].getVSN());

            int usage = getUsage(
                            vsns[i].getAvailableSpace(),
                            vsns[i].getCapacity());
            if (usage == -1) {
                actionTableModel.setValue("UsageText", "");
            } else {
                actionTableModel.setValue("UsageText", new Integer(usage));
            }
            actionTableModel.setValue(
                "TotalText",
                new Capacity(
                    vsns[i].getCapacity(), SamQFSSystemModel.SIZE_KB));
            actionTableModel.setValue("UsageBarImage", getImageString(usage));
            actionTableModel.setValue(
                "MediaFlagText",
                MediaUtil.getFlagString(vsns[i], MAXIMUM_FLAGS));
            actionTableModel.setValue(
                "AddInfoText",
                new Long(vsns[i].getAccessCount()));
        }

        // Notify users that there are more than MAXIMUM_ENTRIES_FETCHED matches
        TraceUtil.trace3("Wrapper Total: " + wrapper.getTotalNumberOfVSNs());
        TraceUtil.trace3("MAXIMUM_ENTRIES_FETCHED: " +
            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED);

        if (wrapper.getTotalNumberOfVSNs() >
            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED) {
            int totalEntry = wrapper.getTotalNumberOfVSNs();
            SamUtil.setInfoAlert(
                this,
                ALERT,
                SamUtil.getResourceString(
                    "AssignMedia.table.tightenRange.summary",
                    new String [] {
                        Integer.toString(
                            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED),
                        Integer.toString(totalEntry),
                    Capacity.newCapacity(
                            wrapper.getFreeSpaceInMB(),
                            SamQFSSystemModel.SIZE_MB).toString()}),
                "",
                getServerName());
            actionTableModel.setTitle(
                SamUtil.getResourceString(
                    "AssignMedia.table.title.partial",
                    Integer.toString(
                        SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED)));
        }
    }

    private void prepopulateRequestParams() {
        // Pre-populate values to pool name and pre-select media type
        NewEditVSNPoolViewBean parent =
             (NewEditVSNPoolViewBean) getParentViewBean();
        String poolName = null, mediaType = null, expression = null;

        if (pageMode == MODE_EDIT_POOL) {
            poolName =
                (String) parent.getPageSessionAttribute(parent.POOL_NAME);

            if (poolName.length() != 0) {
                CCTextField poolNameField = (CCTextField) getChild(VALUE_NAME);
                poolNameField.setValue(poolName);
                poolNameField.setDisabled(true);
            }
        }
        mediaType =
                (String) parent.getPageSessionAttribute(parent.MEDIA_TYPE);
        expression =
            (String) parent.getPageSessionAttribute(parent.EXPRESSION);

        expression = expression == null ? "" : expression;
        mediaType = mediaType == null ? "" : mediaType;

        if (mediaType.length() != 0) {
            CCDropDownMenu typeMenu = (CCDropDownMenu) getChild(MENU_TYPE);
            typeMenu.setValue(mediaType);
            typeMenu.setDisabled(true);
        }

        if (expression.length() != 0) {
            CCTextField expField = (CCTextField) getChild(VALUE_EXPRESSION);
            expField.setValue(expression);
            CCRadioButton radio = (CCRadioButton) getChild(RADIO_METHOD);
            radio.setValue("regex");

            // Show the result table and matching volumes
            getParentViewBean().setPageSessionAttribute(
                            PSA_SHOW_TABLE, new Boolean(true));
        }
    }

    /**
     * Populate messages into corresponding hidden fields.  These messages
     * are used in client-side javascript.
     */
    private void populateHiddenMessages() {
        ((CCHiddenField) getChild(HIDDEN_NO_MEDIA_TYPE_MESSAGE)).
            setValue(SamUtil.getResourceString("AssignMedia.button.error"));
    }

    private int getUsage(long available, long total) {
        if (total == 0) {
            return -1;
        }
        return (int) (100 * (total - available) / total);
    }

    private String getImageString(int usage) {
        if (usage < 0) {
            return Constants.Image.ICON_BLANK_ONE_PIXEL;
        } else {
            return new StringBuffer(Constants.Image.USAGE_BAR_DIR).
                    append(usage).append(".gif").toString();
        }
    }

    private CCActionTableModel getTableModel(String name) {
        return (CCActionTableModel) tableModels.get(name);
    }

    /**
     * Retrieve current server name
     */
    private String getServerName() {
        if (pageMode == MODE_WIZARD_NEW_POOL ||
            pageMode == MODE_WIZARD_NEW_EXP) {
            return (String) ((SamWizardModel) getDefaultModel()).getValue(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        } else {
            // Main page
            return ((CommonSecondaryViewBeanBase)
                    getParentViewBean()).getServerName();
        }
    }

    public void resetTable() {
        try {
            CCActionTable theTable =
                (CCActionTable) getChild(VOLUME_TABLE);
            theTable.resetStateData();
        } catch (ModelControlException modelEx) {
            TraceUtil.trace1("Failed to reset table state data!", modelEx);
        }
    }
    /**
     * This method populates all the i18n-ed labels and help text.  This is
     * needed in the wizard because the wizard does not pick up the resource
     * bundle for some reason.
     */
    private void populateLabels() {
        ((CCLabel) getChild(LABEL_NAME)).setValue(
            SamUtil.getResourceString("AssignMedia.label.name"));
        ((CCLabel) getChild(LABEL_TYPE)).setValue(
            SamUtil.getResourceString("AssignMedia.label.type"));
        ((CCLabel) getChild(LABEL_METHOD)).setValue(
            SamUtil.getResourceString("AssignMedia.label.method"));
        ((CCLabel) getChild(LABEL_INCLUDE)).setValue(
            SamUtil.getResourceString("AssignMedia.label.include"));
        ((CCLabel) getChild(LABEL_FROM)).setValue(
            SamUtil.getResourceString("AssignMedia.label.from"));
        ((CCLabel) getChild(LABEL_TO)).setValue(
            SamUtil.getResourceString("AssignMedia.label.to"));
        ((CCLabel) getChild(LABEL_EXPRESSION)).setValue(
            SamUtil.getResourceString("AssignMedia.label.expression"));
        ((CCStaticTextField) getChild(TEXT_NAME_HELP)).setValue(
            SamUtil.getResourceString("AssignMedia.help.name"));
        ((CCStaticTextField) getChild(TEXT_NAME_HELP)).setValue(
            SamUtil.getResourceString("AssignMedia.help.name"));
        ((CCStaticTextField) getChild(TEXT_INSTRUCTION_TOFROM)).setValue(
            SamUtil.getResourceString("AssignMedia.text.instruction.tofrom"));
        ((CCStaticTextField) getChild(TEXT_INSTRUCTION_RANGE)).setValue(
            SamUtil.getResourceString("AssignMedia.text.instruction.range"));
        ((CCButton) getChild(BUTTON_SELECTED)).setValue(
            SamUtil.getResourceString("AssignMedia.button.showselected"));
        ((CCButton) getChild(BUTTON_ALL)).setValue(
            SamUtil.getResourceString("AssignMedia.button.showall"));
    }

    /**
     * Change label to red to indicate where the error happens to be
     * @param componentName
     */
    private void setLabelError(String componentName) {
        ((CCLabel) getChild(componentName)).setShowError(true);
    }

    /**
     * Hide components if we are only editing an expression
     */
    public boolean beginLabelNameDisplay(ChildDisplayEvent event) {
        return pageMode != MODE_COPY_INFO_EDIT_EXP &&
               pageMode != MODE_COPY_INFO_NEW_EXP &&
               pageMode != MODE_WIZARD_NEW_EXP;
    }

    public boolean beginValueNameDisplay(ChildDisplayEvent event) {
        return pageMode != MODE_COPY_INFO_EDIT_EXP &&
               pageMode != MODE_COPY_INFO_NEW_EXP &&
               pageMode != MODE_WIZARD_NEW_EXP;
    }

    public boolean beginTextNameHelpDisplay(ChildDisplayEvent event) {
        return pageMode != MODE_COPY_INFO_EDIT_EXP &&
               pageMode != MODE_COPY_INFO_NEW_EXP &&
               pageMode != MODE_WIZARD_NEW_EXP;
    }
}
