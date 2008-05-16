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

// ident	$Id: CommonSecondaryViewBeanBase.java,v 1.17 2008/05/16 18:39:06 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.masthead.CCSecondaryMasthead;
import com.sun.web.ui.view.html.CCButton;

/**
 *  This class is the base class for all the secondary(pop-up) viewbeans
 */

public class CommonSecondaryViewBeanBase extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */
    protected static final String CHILD_SECONDARY_MASTHEAD
        = "SecondaryMasthead";

    public static final String ALERT = "Alert";
    public static final String STATE_DATA = "stateData";

    // submit & cancel buttons
    public static final String SUBMIT = "Submit";
    public static final String CANCEL = "Cancel";

    private boolean success = false;

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    protected CommonSecondaryViewBeanBase(String pageName, String displayURL) {
        super(pageName);
        setDefaultDisplayURL(displayURL);
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(CHILD_SECONDARY_MASTHEAD, CCSecondaryMasthead.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(STATE_DATA, CCHiddenField.class);
    }

    /**
     * Check the child component is valid
     *
     * @param name of child compoment
     * @return the boolean variable
     */
    protected boolean isChildSupported(String name) {
        if (name.equals(CHILD_SECONDARY_MASTHEAD) ||
            name.equals(ALERT) ||
            name.equals(STATE_DATA)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        // SecondaryMasthead Component
        if (name.equals(CHILD_SECONDARY_MASTHEAD)) {
            CCSecondaryMasthead child = new CCSecondaryMasthead(this, name);
            child.setSrc(Constants.SecondaryMasthead.PRODUCT_NAME_SRC);
            child.setHeight(Constants.ProductNameDim.HEIGHT);
            child.setWidth(Constants.ProductNameDim.WIDTH);
            child.setAlt(Constants.SecondaryMasthead.PRODUCT_NAME_ALT);
            return child;
        } else if (name.equals(ALERT)) {
            // initliaze an info alert for begin<child>Display methods
            CCAlertInline alert = new CCAlertInline(this, name, null);
            alert.setType(CCAlertInline.TYPE_INFO);
            return alert;
        } else if (name.equals(STATE_DATA)) {
            return new CCHiddenField(this, name, null);
        } else {
            return null;
        }
    }

    /*
     * retrieve the server name. First check the request, for initial requests,
     * otherwise check the page session
     */
    public String getServerName() {
        // first check the page session
        String serverName = (String)getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        // second check the request
        if (serverName == null) {
            serverName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

            if (serverName != null) {
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                    serverName);
            }
        }


        // if server name is null, something is not right
        if (serverName == null)
            throw new IllegalArgumentException("Server Name not supplied");

        // return the server name
        return serverName;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        CCButton submit = (CCButton)getChild(SUBMIT);
        CCButton cancel = (CCButton)getChild(CANCEL);

        if (this.success) {
            submit.setDisabled(true);
            cancel.setValue("common.button.close");
            cancel.setExtraHtml("onClick=\"cleanupPopup(this); \"");
        } else {
            submit.setDisabled(false);
            cancel.setValue("common.button.cancel");
            cancel.setExtraHtml("onClick=\"window.close()\"");
        }
    }

    public void setSubmitSuccessful(boolean success) {
        this.success = success;
    }
}
