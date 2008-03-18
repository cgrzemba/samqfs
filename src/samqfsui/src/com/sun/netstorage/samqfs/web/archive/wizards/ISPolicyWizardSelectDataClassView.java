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

// ident	$Id: ISPolicyWizardSelectDataClassView.java,v 1.8 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for Select Data Class View
 *
 */
public class ISPolicyWizardSelectDataClassView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "ISPolicyWizardSelectDataClassView";

    // Child view names (i.e. display fields).
    public static final String CHILD_ACTIONTABLE = "SelectDataClassTable";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";
    public static final String ALL_CLASS_NAMES = "AllClassNames";
    public static final String SELECTED_CLASS = "SelectedClass";

    private CCActionTableModel tableModel;
    private boolean error = false;
    private boolean previousError = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public ISPolicyWizardSelectDataClassView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public ISPolicyWizardSelectDataClassView(
        View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);

        createActionTableHeader();
        try {
            createActionTableModel();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "ISPolicyWizardSelectDataClassView()",
                "Failed to populate data class data",
                getServerName());
            error = true;
            SamUtil.setErrorAlert(
                this,
                ISPolicyWizardSelectDataClassView.CHILD_ALERT,
                "archiving.policy.wizard.selectdataclass.getdataclass.failed",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

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
        registerChild(CHILD_ACTIONTABLE, CCActionTable.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(ALL_CLASS_NAMES, CCHiddenField.class);
        registerChild(SELECTED_CLASS, CCHiddenField.class);
        tableModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_ACTIONTABLE)) {
            child = new CCActionTable(this, tableModel, name);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else if (name.equals(CHILD_ERROR)) {
            if (error) {
                child = new CCHiddenField(
                    this, name, Constants.Wizard.EXCEPTION);
            } else {
                child = new CCHiddenField(this, name, Constants.Wizard.SUCCESS);
            }
        } else if (name.equals(ALL_CLASS_NAMES) ||
            name.equals(SELECTED_CLASS)) {
            child = new CCHiddenField(this, name, null);
        } else if (tableModel.isChildSupported(name)) {
            child = tableModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "ISPolicyWizardSelectDataClassView : Invalid child name [" +
                name + "]");
        }

        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String url = null;
        if (!previousError) {
            url = "/jsp/archive/wizards/ISPolicyWizardSelectDataClass.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        if (error) {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.EXCEPTION);
        } else {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.SUCCESS);
        }

        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "NewArchivePolWizard.error.carryover";
            previousError = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previousError = false;
                } else {
                    previousError = true;
                }
            }

            if (previousError) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    getServerName());
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
        TraceUtil.trace3("Exiting");
    }

    private void createActionTableHeader() {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/wizards/ISPolicyWizardSelectDataClassTable.xml");
        tableModel.clear();

        tableModel.setActionValue(
            "NameColumn",
            "archiving.dataclass.name");
        tableModel.setActionValue(
            "DescColumn",
            "archiving.dataclass.summary.table.header.description");
        tableModel.setActionValue(
            "PolicyColumn",
            "archiving.policy.wizard.selectdataclass.currentlyapply");
    }

    private void createActionTableModel() throws SamFSException {
        TraceUtil.trace3("Entering");

        StringBuffer buf = new StringBuffer();
        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String selectDataClass =
            (String) wm.getValue(ISPolicyWizardImpl.SELECTED_DATA_CLASS);
        selectDataClass = selectDataClass != null ? selectDataClass.trim() : "";

        ((CCHiddenField) getChild(SELECTED_CLASS)).setValue(selectDataClass);

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        // get all the data classes (criteria)
        ArchivePolCriteria [] allCriterias =
                sysModel.getSamQFSSystemArchiveManager().getAllDataClasses();
        if (allCriterias == null) {
            throw new SamFSException(null, -2020);
        }

        for (int i = 0; i < allCriterias.length; i++) {
            if (buf.length() > 0) {
                tableModel.appendRow();
                buf.append("###");
            }

            ArchivePolicy thisPolicy = allCriterias[i].getArchivePolicy();

            ArchivePolCriteriaProp prop =
                allCriterias[i].getArchivePolCriteriaProperties();

            // filter out file system default criteria(s) & explicit default
            // data class in explicit policy
            if (thisPolicy.getPolicyType() == ArSet.AR_SET_TYPE_DEFAULT ||
                (thisPolicy.getPolicyType() ==
                    ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT &&
                Constants.Cis.DEFAULT_CLASS_NAME.equals(prop.getClassName()))) {
                continue;
            }


            buf.append(prop.getClassName());
            tableModel.setValue("NameText", prop.getClassName());
            tableModel.setValue("DescText", prop.getDescription());
            tableModel.setValue(
                "PolicyText",
                allCriterias[i].getArchivePolicy().getPolicyName());

            if (selectDataClass.equals(prop.getClassName())) {
                tableModel.setRowSelected(true);
            }
        }

        ((CCHiddenField) getChild(ALL_CLASS_NAMES)).setValue(buf.toString());
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
