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

// ident	$Id: SamWizardModel.java,v 1.8 2008/05/16 18:39:07 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.InvalidContextException;

import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * This is the SAMQFS MANAGER wizard model implementation designed to
 * share data collected during a wizard session and the application
 * that originated the wizard session.
 *
 * There are two contexts in this model:
 *  - DEFAULT_CONTEXT - which all DefaultModels have
 *  - WIZARD_CONTEXT - which is created to allow the application to
 *    discard the data collected during a wizard session vs. preserving
 *    it as the the actual data used by the application.
 */

public class SamWizardModel extends DefaultModel {

    private final String WIZARD_CONTEXT_NAME = "SAM_WIZARD_CONTEXT";

    public SamWizardModel() {
        this(null);
    }

    public SamWizardModel(String name) {
        super();
        TraceUtil.initTrace();
        setName(name);
        addContext(WIZARD_CONTEXT_NAME);
        setUseDefaultValues(false);
        TraceUtil.trace3("Exiting");
    }

    public void selectWizardContext() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(WIZARD_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // Should never fail.
        }
        TraceUtil.trace3("Exiting");
    }

    public void selectDefaultContext() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(DEFAULT_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // Should never fail.
        }
        TraceUtil.trace3("Exiting");
    }

    public void clearWizardData() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(WIZARD_CONTEXT_NAME);
            clear();
        } catch (InvalidContextException e) {
            // This should never fail
        }

        // Restore the default context.
        try {
            selectContext(DEFAULT_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // never fails.
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Get the Value stored in the Wizard Context
     */
    public Object getWizardValue(String fieldName) {
        return getContextValue(WIZARD_CONTEXT_NAME, fieldName);
    }

    /**
     * Get the Value stored in the Default Context
     */
    public Object getDefaultContextValue(String fieldName) {
        return getContextValue(DEFAULT_CONTEXT_NAME, fieldName);
    }

    /**
     * Set the value in the Wizard Context
     */
    public void setWizardContextValue(String fieldName, Object value) {
        setContextValue(WIZARD_CONTEXT_NAME, fieldName, value);
    }

    /**
     * Set the value in the Default Context
     */
    public void setDefaultContextValue(String fieldName, Object value) {
        setContextValue(DEFAULT_CONTEXT_NAME, fieldName, value);
    }

    /**
     * Helper method to get the Value stored in the given Context
     */
    private Object getContextValue(String contextName, String fieldName) {
        // Remember the current context
        String currentContextName = getCurrentContextName();
        if (currentContextName.equals(contextName)) {
            return getValue(fieldName);
        }

        // CurrentContext is not WizardContext,
        // So set the Context to retrieve data from the WIZARD_CONTEXT
        try {
            selectContext(contextName);
        } catch (InvalidContextException e) {
            // This should never fail
        }
        Object obj = getValue(fieldName);

        // Restore the default context.
        try {
            selectContext(currentContextName);
        } catch (InvalidContextException e) {
            // never fails.
        }
        return obj;
    }

    /**
     * Helper method to Set the value in the given Context
     */
    private void setContextValue(
        String contextName, String fieldName, Object value) {
        // Remember the current context
        String currentContextName = getCurrentContextName();
        if (currentContextName.equals(contextName)) {
            setValue(fieldName, value);
            return;
        }

        // CurrentContext is not WizardContext,
        // So set the Context to retrieve data from the WIZARD_CONTEXT
        try {
            selectContext(contextName);
            setValue(fieldName, value);
        } catch (InvalidContextException e) {
            // This should never fail
            TraceUtil.trace1(
                "InvalidContextException caught in setContextValue()");
            TraceUtil.trace1("Reason: " + e.getMessage());
        }

        // Restore the current context.
        try {
            selectContext(currentContextName);
        } catch (InvalidContextException e) {
            // never fails.
            TraceUtil.trace1(
                "InvalidContextException caught in setContextValue()");
            TraceUtil.trace1("Reason: " + e.getMessage());
        }
    }
}
