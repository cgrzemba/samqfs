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

// ident	$Id: WizardServlet.java,v 1.10 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.sun.web.ui.servlet.wizard.WizardWindowServlet;


/**
 * The application developer must create a Servlet subclass of
 * com.sun.web.ui.servlet.wizard.WizardWindowServlet.
 * This is because the wizard requires resource bundles that
 * that can only be loaded by the application context class
 * loader. Creating a wizard Servlet subclass ensures that
 * the application can load its resource bundle.
 *
 * @version 1.2 05/29/03
 * @author  Sun Microsystems, Inc.
 *
 */
public class WizardServlet extends WizardWindowServlet {

    public static final String DEFAULT_MODULE_URL = "../wizard";

    // Constructor
    public WizardServlet() {
        super();
    }

    public String getModuleURL() {
        // The superclass can be configured from init params specified at
        // deployment time.  If the superclass has been configured with
        // a different module URL, it will return a non-null value here.
        // If it has not been configured with a different URL, we use our
        // (hopefully) sensible default.
        String result = (super.getModuleURL() != null)
            ? super.getModuleURL()
            : DEFAULT_MODULE_URL;

        return result;
    }
}
