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

// ident	$Id: FrameMastheadViewBean.java,v 1.5 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.masthead.CCSecondaryMasthead;

/**
 *  This class is the view bean for the Monitoring console masthead
 */

public class FrameMastheadViewBean extends ViewBeanBase {

    protected static final
        String SECONDARY_MASTHEAD = "SecondaryMasthead";

    // Page information...
    private static final String PAGE_NAME = "FrameMasthead";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/monitoring/FrameMasthead.jsp";
    /**
     * Constructor
     */
    public FrameMastheadViewBean() {
        super(PAGE_NAME);
        setDefaultDisplayURL(DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(SECONDARY_MASTHEAD, CCSecondaryMasthead.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append(
            "Entering: child name = ").append(name).toString());

        if (name.equals(SECONDARY_MASTHEAD)) {
            CCSecondaryMasthead child = new CCSecondaryMasthead(this, name);
            child.setAlt("Monitor.main.browsertitle");
            TraceUtil.trace3("Exiting");
            return child;
        } else  {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }
    }
}
