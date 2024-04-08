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

// ident	$Id: MonitoringFrameFormatViewBean.java,v 1.6 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.view.html.CCStaticTextField;

/**
 *  This class is ViewBean of the Base Frame Format.
 */

public class MonitoringFrameFormatViewBean extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */

    private static final String BROWSER_TITLE  = "BrowserTitle";
    private static final String TEXT = "Text";
    private static final String SERVER_NAME = "ServerName";

    private static final
        String URL = "/jsp/monitoring/MonitoringFrameFormat.jsp";
    private static final String PAGE_NAME = "MonitoringFrameFormat";

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    public MonitoringFrameFormatViewBean() {
        super(PAGE_NAME);

        // set the address of the JSP page
        setDefaultDisplayURL(URL);
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(BROWSER_TITLE, CCStaticTextField.class);
        registerChild(SERVER_NAME, CCStaticTextField.class);
        registerChild(TEXT, CCStaticTextField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (name.equals(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCStaticTextField(this, name, whichServer());
        } else if (name.equals(BROWSER_TITLE)) {
            return new CCStaticTextField(this, name, null);
        } else {
            // Should not come here
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Save the server name from the request
     */
    private String whichServer() {
        // first check the page session
        String serverName = (String) getPageSessionAttribute(
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

            if (serverName == null) {
                throw new IllegalArgumentException("Server Name not supplied");
            }
        }

        return serverName;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        ((CCStaticTextField) getChild(BROWSER_TITLE)).
            setValue("Monitor.main.browsertitle");
    }

}
