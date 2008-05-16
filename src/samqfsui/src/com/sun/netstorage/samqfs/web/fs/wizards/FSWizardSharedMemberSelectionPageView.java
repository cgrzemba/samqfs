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

// ident	$Id: FSWizardSharedMemberSelectionPageView.java,v 1.24 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for device selection page.
 *
 */
public class FSWizardSharedMemberSelectionPageView
    extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final
        String PAGE_NAME = "FSWizardSharedMemberSelectionPageView";

    // Child view names (i.e. display fields).
    public static final
        String CHILD_ACTIONTABLE = "SharedMemberSelectionTable";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";
    public static final String CHILD_LABEL = "counterLabel";
    public static final String CHILD_IPLABEL = "ipLabel";
    public static final String CHILD_SECIPLABEL = "secIpLabel";
    public static final String CHILD_METALABEL  = "metaServerLabel";
    public static final String CHILD_METAVERLABEL  = "metaServerVerLabel";
    public static final String CHILD_METAARCHLABEL = "metaServerArchLabel";
    public static final String CHILD_METASERVER    = "metaServer";
    public static final String CHILD_METASERVERVER = "metaServerVer";
    public static final String CHILD_METASERVERARCH = "metaServerArch";
    public static final String CHILD_INIT_SELECTED = "initSelected";
    public static final String CHILD_COUNTER  = "counter";
    public static final String CHILD_SERVERIP = "serverIP";
    public static final String CHILD_SECSERVERIP   = "secServerIP";
    public static final String CHILD_MEMBER_HIDDEN = "MemberHidden";
    public static final String CHILD_CLIENT_HIDDEN = "ClientHidden";
    public static final String CHILD_MDS_HIDDEN = "MDSHidden";
    public static final String CHILD_CLIENT = "clientTextField";
    public static final
        String CHILD_POTENTIAL_SERVER = "potentialMetadataServerTextField";
    public static final String CHILD_PRIMARYIP   = "primaryIPTextField";
    public static final String CHILD_SECONDARYIP = "secondaryIPTextField";

    // for disk discovery javascript message
    public static final String CHILD_HIDDEN_MESSAGE = "HiddenMessage";

    // Hidden field to keep track of all possible values for the secondary ip
    // drop down
    public static final String HIDDEN_IPS = "ipAddresses";

    // Hidden field to keep track of what user selects for the metadata server
    // selected secondary ip.  We need to use javascript to populate and select
    // this value as the dropdown is populated on the fly
    public static final String SELECTED_SECONDARY_IP = "selectedSecondaryIP";


    public FSWizardSharedMemberSelectionPageModel tableModel = null;
    protected int initSelected = 0, totalItems = 0;
    protected boolean previous_error = false;

    private static final String
        CHILD_TILED_VIEW = "FSWizardSharedMemberSelectionPageTiledView";
    public boolean error = false;
    private SamFSException myException = null;


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardSharedMemberSelectionPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public FSWizardSharedMemberSelectionPageView(
        View parent, Model model, String name) {

        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);
        tableModel = new FSWizardSharedMemberSelectionPageModel();
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
            FSWizardSharedMemberSelectionPageTiledView.class);
        registerChild(CHILD_ACTIONTABLE, CCActionTable.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_MEMBER_HIDDEN, CCHiddenField.class);
        registerChild(CHILD_CLIENT_HIDDEN, CCHiddenField.class);
        registerChild(CHILD_MDS_HIDDEN, CCHiddenField.class);
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_IPLABEL, CCLabel.class);
        registerChild(CHILD_SECIPLABEL, CCLabel.class);
        registerChild(CHILD_METALABEL, CCLabel.class);
        registerChild(CHILD_METAVERLABEL, CCLabel.class);
        registerChild(CHILD_METAARCHLABEL, CCLabel.class);
        registerChild(CHILD_PRIMARYIP, CCDropDownMenu.class);
        registerChild(CHILD_SECONDARYIP, CCDropDownMenu.class);
        registerChild(CHILD_SERVERIP, CCDropDownMenu.class);
        registerChild(CHILD_SECSERVERIP, CCDropDownMenu.class);
        registerChild(CHILD_CLIENT, CCCheckBox.class);
        registerChild(CHILD_POTENTIAL_SERVER, CCCheckBox.class);
        registerChild(CHILD_INIT_SELECTED, CCHiddenField.class);
        registerChild(CHILD_COUNTER, CCTextField.class);
        registerChild(CHILD_HIDDEN_MESSAGE, CCHiddenField.class);
        registerChild(CHILD_METASERVER, CCTextField.class);
        registerChild(CHILD_METASERVERVER, CCTextField.class);
        registerChild(CHILD_METASERVERARCH, CCTextField.class);
        registerChild(HIDDEN_IPS, CCHiddenField.class);
        registerChild(SELECTED_SECONDARY_IP, CCHiddenField.class);
        tableModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (name.equals(CHILD_ACTIONTABLE)) {
            TraceUtil.trace3("before CCActionTable");
            CCActionTable child = new CCActionTable(this, tableModel, name);
            // Set the TileView object.
            TraceUtil.trace3("before set");
            child.setTiledView((ContainerView)
                getChild(CHILD_TILED_VIEW));
            TraceUtil.trace3("Exiting CHILD_ACTIONTABLE");
            return child;
        } else if (name.equals(CHILD_TILED_VIEW)) {
            SamWizardModel wm = (SamWizardModel)getDefaultModel();
            FSWizardSharedMemberSelectionPageTiledView child =
                new FSWizardSharedMemberSelectionPageTiledView(this,
                tableModel, wm, name);
            TraceUtil.trace3("after creating tiled view");
            return child;
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            child.setValue(CCAlertInline.TYPE_ERROR);
            TraceUtil.trace3("Exiting CHILD_ALERT");
            return child;
        } else if (name.equals(CHILD_ERROR)) {
            CCHiddenField child =
                new CCHiddenField(this, name,
                    error ? "exception" : "success");
            TraceUtil.trace3("Exiting CHILD_ERROR");
            return child;
        } else if (name.equals(CHILD_MEMBER_HIDDEN) ||
                name.equals(CHILD_CLIENT_HIDDEN) ||
                name.equals(CHILD_MDS_HIDDEN)) {
            CCHiddenField child = null;
            child = new CCHiddenField(this, name, null);
            TraceUtil.trace3("Exiting CHILD_MEMBER_HIDDEN");
            return child;
        } else if (name.equals(CHILD_LABEL)) {
            TraceUtil.trace3("Exiting label");
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_IPLABEL) ||
                   name.equals(CHILD_SECIPLABEL) ||
                   name.equals(CHILD_METALABEL) ||
                   name.equals(CHILD_METAVERLABEL) ||
                   name.equals(CHILD_METAARCHLABEL) ||
                   name.equals(CHILD_LABEL)) {
            TraceUtil.trace3("Exiting ip label");
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_PRIMARYIP) ||
                   name.equals(CHILD_SECONDARYIP) ||
                   name.equals(CHILD_SERVERIP) ||
                   name.equals(CHILD_SECSERVERIP)) {
            TraceUtil.trace3("Exiting serverip");
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(CHILD_CLIENT) ||
                   name.equals(CHILD_POTENTIAL_SERVER)) {
            CCCheckBox checkBoxChild = new
                CCCheckBox(this, name, "true", "false", false);
            TraceUtil.trace3("Exiting checkbox");
            return checkBoxChild;
        } else if (name.equals(CHILD_COUNTER) ||
                   name.equals(CHILD_METASERVER) ||
                   name.equals(CHILD_METASERVERARCH) ||
                   name.equals(CHILD_METASERVERVER)) {
            TraceUtil.trace3("Exiting");
            return new CCTextField(this, name, null);
        } else if (name.equals(CHILD_INIT_SELECTED)) {
            TraceUtil.trace3("Exiting  init selected");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_MESSAGE)) {
            return new CCHiddenField(
                this, name, SamUtil.getResourceString("discovery.disk"));
        } else if (name.equals(HIDDEN_IPS) ||
                   name.equals(SELECTED_SECONDARY_IP)) {
            return new CCHiddenField(this, name, null);
        } else if (tableModel.isChildSupported(name)) {
            // Create child from action table model.
            View child = tableModel.createChild(this, name);
            TraceUtil.trace3("Exiting  suppotrte");
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
        String url =
            previous_error ?
                "/jsp/fs/wizardErrorPage.jsp" :
                "/jsp/fs/FSWizardSharedMemberSelectionPage.jsp";
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);

        try {
            TraceUtil.trace2("Calling populateSharedMemberModel");
            tableModel.populateSharedMemberModel(wm);
            ((CCHiddenField) getChild(CHILD_MEMBER_HIDDEN)).setValue(
                Integer.toString(tableModel.getNumRows()));

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
            ((CCHiddenField) getChild(CHILD_MEMBER_HIDDEN)).setValue("0");
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                error ? "exception" : "success");
            TraceUtil.trace3("Exiting with errors!");
            return;
        }

        // Fill in the initSelected Hiddenfield with the initial number of
        // selected items
        ((CCHiddenField) getChild(CHILD_INIT_SELECTED)).setValue(
            Integer.toString(initSelected) + "," + totalItems);
        ((CCTextField) getChild(CHILD_COUNTER)).setValue(
            Integer.toString(initSelected));

        // fill in client/mds hidden field with latest selection
        ((CCHiddenField) getChild(CHILD_CLIENT_HIDDEN)).setValue(
            wm.getValue(
            Constants.Wizard.SELECTED_CLIENT_INDEX));
        ((CCHiddenField) getChild(CHILD_MDS_HIDDEN)).setValue(
            wm.getValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_INDEX));

        CCDropDownMenu dropDownMenu =
            ((CCDropDownMenu) getChild(CHILD_SERVERIP));
        CCDropDownMenu dropDownMenu2 =
            ((CCDropDownMenu) getChild(CHILD_SECSERVERIP));
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();
            SamQFSSystemModel model = SamUtil.getModel(serverName);

            ((CCTextField) getChild(CHILD_METASERVER)).
                setValue(serverName);
            ((CCTextField) getChild(CHILD_METASERVERVER)).
                setValue(model.getServerProductVersion());
            ((CCTextField) getChild(CHILD_METASERVERARCH)).
                setValue(model.getArchitecture());
            String[] data = fsManager.getIPAddresses(serverName);
            OptionList options = new OptionList(data, data);
            dropDownMenu.setOptions(options);

            // String[] dataHost = new String[data.length + 1];
            StringBuffer ipString = new StringBuffer();
            // dataHost[0] = "---";

            for (int i = 0; i < data.length; i++) {
                // dataHost[i+1] = data[i];
                ipString.append(data[i]).append(";");
            }

            // OptionList options2 = new OptionList(dataHost, dataHost);
            // dropDownMenu2.setOptions(options2);
            if (ipString.length() > 0 &&
                    ipString.charAt(ipString.length() - 1) == ';') {
                ipString.deleteCharAt(ipString.length() - 1);
            }
            ((CCHiddenField) getChild(
                HIDDEN_IPS)).setValue(ipString.toString());

        } catch (SamFSException fsExp) {
            SamUtil.processException(
                fsExp,
                this.getClass(),
                "beginDisplay()",
                "Failed to get metadata host ip addresses",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                "FSWizard.new.error.steps",
                fsExp.getSAMerrno(),
                fsExp.getMessage(),
                serverName);
            error = true;
        }

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

            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                errorSummary,
                code, msgs,
                serverName);
            TraceUtil.trace3("Exiting with errors!");
        }

        ((CCHiddenField) getChild(SELECTED_SECONDARY_IP)).setValue(
            wm.getValue(CHILD_SECSERVERIP));
        ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
            error ? "exception" : "success");
        TraceUtil.trace3("Exiting");
    }

    /**
     * Return error flag to stop GUI from further calling remote methods
     * in tiled view
     */
    public boolean getError() {
        return error;
    }
}
