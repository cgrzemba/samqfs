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

// ident	$Id: FrameHeaderViewBean.java,v 1.10 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.web.ui.model.CCMastheadModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.masthead.CCPrimaryMasthead;

/**
 *  This class is ViewBean of the Base Frame Format.
 */

public class FrameHeaderViewBean extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */

    protected static final String MASTHEAD   = "Masthead";
    protected static final String ALARM_HREF = "AlarmCountHref";
    protected static final String REFRESH_HREF = "RefreshHref";
    protected static final String PR_HIDDEN  = "PreferenceHidden";

    private static final String URL = "/jsp/util/FrameHeader.jsp";
    private static final String PAGE_NAME = "FrameHeader";

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    public FrameHeaderViewBean() {
        super(PAGE_NAME);

        // set the address of the JSP page
        setDefaultDisplayURL(URL);
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(MASTHEAD, CCPrimaryMasthead.class);
        registerChild(ALARM_HREF, CCHref.class);
        registerChild(REFRESH_HREF, CCHref.class);
        registerChild(PR_HIDDEN, CCHiddenField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (name.equals(ALARM_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setExtraHtml(
                "onClick=\"forwardToAlarmSummary(); return false; \"");
            return child;
        } else if (name.equals(MASTHEAD)) {
            CCMastheadModel mastheadModel = new CCMastheadModel();
            boolean hasError = false;

            try {
                mastheadModel =
                    MastheadUtil.setMastHeadModel(
                        mastheadModel, getPageName(), getServerName());
                mastheadModel.setAlarmCountHREF(ALARM_HREF);
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "createChild",
                    "Failed to create Masthead Model",
                    getServerName());

                // Do not set Alert here
                // Simply not show the fault icons and additional buttons
            }

            return new CCPrimaryMasthead(this, mastheadModel, name);
        } else if (name.equals(REFRESH_HREF)) {
            CCHref child = new CCHref(this, name, null);
            child.setExtraHtml("onClick=\"doRefreshTarget(); return false; \"");
            return child;
        } else if (name.equals(ALARM_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(PR_HIDDEN)) {
            return new CCHiddenField(this, name, null);
        } else {
            // Should not come here
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    private String getServerName() {
        String serverName =
            (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        if (serverName == null) {
            serverName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                serverName);
        }

        return serverName;
    }

    public String getPageName() {
        // Grab the page name from the Request.  DO NOT REMOVE THIS LINE.
        // This is critical to grab the page name and save it as a page
        // session attribute.  The page name is set for the HELP button so
        // the appropriate HELP page will be launched.

        String pageName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.PAGE_NAME);
        if (pageName != null) {
            return pageName;
        }

        // Server name is not available in the page session, retrieve it from
        // the request and save it in page session
        pageName = RequestManager.getRequestContext().getRequest().
            getParameter(Constants.PageSessionAttributes.PAGE_NAME);
        setPageSessionAttribute(
            Constants.PageSessionAttributes.PAGE_NAME, pageName);
        return pageName;
    }
}
