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

// ident	$Id: WizardModel.java,v 1.10 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.InvalidContextException;

import com.sun.netstorage.samqfs.web.util.TraceUtil;

// Need to extend DefaultModel to add a WIZARD_CONTEXT
// More sophisticated models can implement other techniques
// to map values from the wizard to the fields

public class WizardModel extends DefaultModel {
    final String WIZARD_CONTEXT = "SAM_WIZARD_CONTEXT";

    public WizardModel() {
        this(null);
    }

    public WizardModel(String name) {
        super();
        TraceUtil.initTrace();
        setName(name);
        addContext(WIZARD_CONTEXT);
        TraceUtil.trace3("Exiting");
    }

    public void selectWizardContext() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(WIZARD_CONTEXT);
        } catch (InvalidContextException e) {
            // this means that the wizard has not set any
            // values in the model
            //
            // Should never fail.
        }
        TraceUtil.trace3("Exiting");
    }

    public void selectDefaultContext() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(DEFAULT_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // this means that the wizard has not set any
            // values in the model
            //
            // Should never fail.
        }
        TraceUtil.trace3("Exiting");
    }

    public void clearWizardData() {
        TraceUtil.trace3("Entering");
        try {
            selectContext(WIZARD_CONTEXT);
            clear();
        } catch (InvalidContextException e) {
            // This should never fail
        }

        // Restore the default context.
        try {
            selectContext(DefaultModel.DEFAULT_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // never fails.
        }
        TraceUtil.trace3("Exiting");
    }

    // This method should probably be synchronized
    //
    /**
     * This is an example on how to get data set by the wizard
     * from the model. More sophisticated models may need more
     * sophisticated techniques. This is called from the beginDisplay
     * methods of the container views for their fields values.
     * For simplicity we assume only single valued values.
     */

    public Object getWizardValue(String fieldName) {

        // Set the model to retrieve data from the WIZARD_CONTEXT
        // in the model
        //
        TraceUtil.trace3("Entering");
        // NEW
        String cntxt = getCurrentContextName();
        try {
            selectContext(WIZARD_CONTEXT);
        } catch (InvalidContextException e) {
            // this means that the wizard has not set any
            // This should never fail
        }
        Object obj = getValue(fieldName);

        // Restore the default context.
        try {
            selectContext(DefaultModel.DEFAULT_CONTEXT_NAME);
        } catch (InvalidContextException e) {
            // never fails.
            TraceUtil.trace1(
                "InvalidContextException caught in getWizardValue()");
            TraceUtil.trace1("Reason: " + e.getMessage());
        }
        // If there is no wizard value return the default
        // context value
        //
        if (obj == null) {
            obj = getValue(fieldName);
        }
        // NEW: Restore the original context
        //
        try {
            selectContext(cntxt);
        } catch (InvalidContextException e) {
            // this means that the wizard has not set any
            // values in the model
            //
            // Should never fail.
            TraceUtil.trace1(
                "InvalidContextException caught in getWizardValue()");
            TraceUtil.trace1("Reason: " + e.getMessage());
        }

        TraceUtil.trace3("Exiting");
        return obj;
    }
}
