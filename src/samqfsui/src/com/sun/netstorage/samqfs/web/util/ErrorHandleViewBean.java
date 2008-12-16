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

// ident	$Id: ErrorHandleViewBean.java,v 1.12 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.sun.web.ui.model.CCMastheadModel;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.masthead.CCPrimaryMasthead;
import javax.servlet.http.HttpServletRequest;


/**
 *  This class is common class for
 *  all uncatch error.
 */

public class ErrorHandleViewBean extends ViewBeanBase {

    // Page information...
    private static final String PAGE_NAME  = "ErrorHandle";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/util/ErrorHandle.jsp";

    public static final String CHILD_ALERT  = "Alert";
    public static final String CHILD_MASTHEAD = "Masthead";
    public static final String IS_POP_UP = "isPopUP";

    private CCPageTitleModel pageTitleModel  = null;
    private boolean popup = false;

    /**
     * Constructor
     */
    public ErrorHandleViewBean() {
        super(PAGE_NAME);
        setDefaultDisplayURL(DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        HttpServletRequest httprq =
            RequestManager.getRequestContext().getRequest();
        String popupValue = (String)
            httprq.getParameter("com_sun_web_ui_popup");
        if (popupValue != null) {
            if (popupValue.equals("true")) {
                popup = true;
            }
        }
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_MASTHEAD, CCPrimaryMasthead.class);
        registerChild(IS_POP_UP, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            TraceUtil.trace3("Exiting");
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child = new CCAlertInline(this, name, null);
            child.setValue(CCAlertInline.TYPE_ERROR);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(IS_POP_UP)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, Boolean.toString(popup));
        } else if (name.equals(CHILD_MASTHEAD)) {
            CCMastheadModel mastheadModel = new CCMastheadModel();
            mastheadModel.setSrc(Constants.Masthead.PRODUCT_NAME_SRC);
            mastheadModel.setAlt(Constants.Masthead.PRODUCT_NAME_ALT);
            mastheadModel.setShowUserRole(true);
            mastheadModel.setShowServer(true);
            mastheadModel.setShowDate(true);

            CCPrimaryMasthead child =
                new CCPrimaryMasthead(this, mastheadModel, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else {
            throw new IllegalArgumentException
                ("Invalid child name [" + name + "]");
        }
    }

    /**
     * Function to create the pagetitle model based on the xml file
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        pageTitleModel = PageTitleUtil.createModel(
            "/jsp/util/ErrorHandlePageTitle.xml");
        pageTitleModel.setValue(
            "PageButton0",
            popup ?
                "PopupErrorHandle.PageButton1" : "ErrorHandle.PageButton1");
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }
}
