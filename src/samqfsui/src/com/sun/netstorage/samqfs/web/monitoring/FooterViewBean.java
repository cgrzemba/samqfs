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

// ident	$Id: FooterViewBean.java,v 1.3 2008/05/16 18:39:04 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.sun.web.ui.view.html.CCButton;

/**
 *  This class is the view bean for the footer frame
 *  pop up
 */

public class FooterViewBean extends ViewBeanBase {

    private static final String pageName   = "Frame";
    private static final String displayURL = "/jsp/monitoring/Footer.jsp";

    // cc components from the corresponding jsp page(s)...
    private static final String CLOSE_BUTTON = "CloseButton";

    /**
     * Constructor
     */
    public FooterViewBean() {
        super(pageName);

        // set the address of the JSP page
        setDefaultDisplayURL(displayURL);
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(CLOSE_BUTTON, CCButton.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {

        View child = null;

        if (name.equals(CLOSE_BUTTON)) {
            child = new CCButton(this, name, null);
        } else {
            throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }
        return (View) child;
    }
}
