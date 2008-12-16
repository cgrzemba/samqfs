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

// ident	$Id: HeaderViewBean.java,v 1.10 2008/12/16 00:12:23 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;

/**
 *  This class is the view bean for the header frame
 *  pop up
 */

public class HeaderViewBean extends ViewBeanBase {

    private static final String pageName   = "Frame";
    private static final String displayURL = "/jsp/monitoring/Header.jsp";

    // cc components from the corresponding jsp page(s)...
    private static final String LABEL = "Label";
    private static final String SERVER_NAME = "ServerName";
    private static final String AUTO_REFRESH = "AutoRefresh";
    private static final String REFRESH_BUTTON = "RefreshButton";

    /**
     * Constructor
     */
    public HeaderViewBean() {
        super(pageName);

        // set the address of the JSP page
        setDefaultDisplayURL(displayURL);
        registerChildren();
        getServerName();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(LABEL, CCLabel.class);
        registerChild(SERVER_NAME, CCStaticTextField.class);
        registerChild(REFRESH_BUTTON, CCButton.class);
        registerChild(AUTO_REFRESH, CCDropDownMenu.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {

        View child = null;

        if (name.equals(LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(AUTO_REFRESH)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(REFRESH_BUTTON)) {
            return new CCButton(this, name, null);
        } else {
            throw new IllegalArgumentException(new StringBuffer(
                "Invalid child name [").append(name).append("]").toString());
        }
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        String serverName = getServerName();

        ((CCStaticTextField) getChild(SERVER_NAME)).setValue(
            new StringBuffer("<b>").append(
                serverName).append("</b>").toString());
        ((CCDropDownMenu) getChild(AUTO_REFRESH)).setOptions(
            new OptionList(
                SelectableGroupHelper.refreshInterval.labels,
                SelectableGroupHelper.refreshInterval.values));
    }

    private String getServerName() {
        String serverName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        if (serverName == null || "".equals(serverName.trim())) {
            serverName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        }
        return serverName;
    }
}
