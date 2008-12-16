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

// ident	$Id: WizardResultView.java,v 1.11 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.wizard.CCWizardPage;

import javax.servlet.http.HttpSession;


/**
 * A ContainerView object for the pagelet for Add Stand ALone Save Page.
 *
 */
public class WizardResultView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "WizardResultView";

    // Child view names (i.e. display fields).
    public static final String CHILD_ALERT = "Alert";


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public WizardResultView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public WizardResultView(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3(new StringBuffer().append("Creating child ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else {
            throw new IllegalArgumentException(
                "WizardResultView : Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return child;
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return "/jsp/wizard/WizardResult.jsp";
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        if (wizardModel != null) {
            String wizardResult = (String) wizardModel.getValue(
                Constants.AlertKeys.OPERATION_RESULT);
            String summaryMessage = (String) wizardModel.getValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY);
            String detailMessage = (String) wizardModel.getValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL);

            if (wizardResult != null) {
                // SUCCESS
                if (wizardResult.equals(
                    Constants.AlertKeys.OPERATION_SUCCESS)) {
                    setInfoAlert(summaryMessage, detailMessage);

                } else if (wizardResult.equals(
                    Constants.AlertKeys.OPERATION_FAILED)) {
                    // FAILED
                    int errorCode = Integer.parseInt(
                        (String) wizardModel.getValue(
                            Constants.Wizard.DETAIL_CODE));
                    setErrorAlert(
                        summaryMessage, detailMessage, errorCode);
                } else if (wizardResult.equals(
                    Constants.AlertKeys.OPERATION_WARNING)) {
                    // WARNING
                    setWarningAlert(
                        summaryMessage, detailMessage);
                } else {
                    // Invalid wizard found
                    setErrorAlert("", "", -1100);
                }
            }
        } else {
            // Invalid wizard found
            setErrorAlert("", "", -1100);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * The following three methods are used to set the appropriate alert
     * in the wizard result page.  This three methods are strip down version
     * of the ones in SamUtil.  Wizard Result alert is simplier than the one
     * in SamUtil.  Wizard alert do not have a complicated situation that
     * an info alert is set after an error alert is set (e.g. this happen in
     * summary page while an action API is called then getAll API is called in
     * the same display cycle.
     */

    /**
     * setInfoAlert()
     * Set the info type alert in wizard result page
     */
    void setInfoAlert(
        String summaryMessage,
        String detailMessage) {

        CCAlertInline myAlert = (CCAlertInline) getChild(CHILD_ALERT);
        myAlert.setValue(CCAlertInline.TYPE_INFO);
        myAlert.setSummary(summaryMessage);
        myAlert.setDetail(detailMessage);
    }

    /**
     * setErrorAlert()
     * Set the error type alert in wizard result page
     */
    void setErrorAlert(
        String summaryMessage,
        String detailMessage,
        int errorCode) {

        CCAlertInline myAlert = (CCAlertInline) getChild(CHILD_ALERT);

        if (detailMessage == null) {
            if (errorCode <= -1000 && errorCode >= -3000) {
                if (errorCode == -2800) {
                    // Server is down
                    detailMessage = SamUtil.getResourceStringError(
                        "ErrorHandle.alertElementFailedDetail2");
                } else {
                    // Regular errors
                    detailMessage = SamUtil.getResourceStringError(
                        Integer.toString(errorCode)).toString();
                }
            } else {
                detailMessage = "";
            }
        }

        // Override Error messages
        // 30806 => Timeout (Long/Hung API)
        // 30807 => Network Down
        if (errorCode == 30806) {
            detailMessage = SamUtil.getResourceStringError("-2801");
        } else if (errorCode == 30807) {
            detailMessage = SamUtil.getResourceStringError("-2802");
        }

        myAlert.setValue(CCAlertInline.TYPE_ERROR);
        myAlert.setSummary(summaryMessage);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // If error code is -2800, need to append server name to error message
        if (errorCode == -2800) {
            String serverName =
                (String) wizardModel.getValue(Constants.Wizard.SERVER_NAME);
            myAlert.setDetail(detailMessage, new String[] { serverName });
         } else if (errorCode == -2025) {
            String polName = (String) wizardModel.getValue(
                Constants.Wizard.DUP_POLNAME);
            String criteriaName = (String) wizardModel.getValue(
                Constants.Wizard.DUP_CRITERIA);
            myAlert.setDetail(detailMessage,
                new String[] { criteriaName, polName });
        } else {
            myAlert.setDetail(detailMessage);
        }
    }

    /**
     * setWarningAlert()
     * Set the error type alert in wizard result page
     */
    void setWarningAlert(
        String summaryMessage,
        String detailMessage) {

        CCAlertInline myAlert = (CCAlertInline) getChild(CHILD_ALERT);
        myAlert.setValue(CCAlertInline.TYPE_WARNING);
        myAlert.setSummary(summaryMessage);
        myAlert.setDetail(detailMessage);
    }
}
