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

// ident	$Id: AppServletBase.java,v 1.26 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.view.ViewBean;
import com.sun.web.common.ConsoleServletBase;
import com.sun.web.ui.view.alert.CCAlertInline;
import java.io.IOException;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;

/**
 * The application servlet.
 *
 * @version 1.7 03/25/02
 * @author  Sun Microsystems, Inc.
 */
public class AppServletBase extends ConsoleServletBase {

    /**
     * Default constructor
     */
    public AppServletBase() {
        super();
    }

    /**
     * Initialize servlet base
     *
     * @param config The ServletConfig object for this instance.
     */
    public void init(ServletConfig config) throws ServletException {
        super.init(config);
    }

    /**
     * onUncaughtException to catch the uncaught exception from
     * pages and then forward to error page.
     */
    protected void onUncaughtException(RequestContext context, Exception e)
        throws ServletException, IOException {
        TraceUtil.trace1("Entering AppServletBase::onUncaughtException");
        TraceUtil.trace1("Reason: " + e.getLocalizedMessage());
        String errorMsgs = e.getMessage();
        ViewBean errorVB =
            context.getViewBeanManager().getViewBean(ErrorHandleViewBean.class);
        CCAlertInline alert = (CCAlertInline) errorVB.getChild
            (ErrorHandleViewBean.CHILD_ALERT);
        alert.setSummary("ErrorHandle.alertElement");
        alert.setDetail(
            "ErrorHandle.alertElementFailedDetail",
            new String[] { errorMsgs });

        // print the stack trace to the console_debug file
        e.fillInStackTrace();
        e.printStackTrace(System.out);
        errorVB.forwardTo(context);
    }

    /**
     * Load the JNI Library exactly once
     */
    static {
        System.loadLibrary("fsmgmtjni");
    }
}
