/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: FrameViewBean.java,v 1.9 2008/03/17 14:43:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCHiddenField;

/**
 *  This class is the view bean for each frame in the monitoring multi-frame
 *  pop up
 */

public class FrameViewBean extends ViewBeanBase {

    private static final String pageName   = "Frame";
    private static final String displayURL = "/jsp/monitoring/Frame.jsp";

    // cc components from the corresponding jsp page(s)...
    private static final String CONTAINER_VIEW  = "SingleTableView";

    // Hidden field to hold page ID
    private static final String PAGE_ID = "PageID";

    // Server Name
    private static final String SERVER_NAME = "ServerName";

    /**
     * Constructor
     */
    public FrameViewBean() {
        super(pageName);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        // set the address of the JSP page
        setDefaultDisplayURL(displayURL);
        registerChildren();
        saveRequestInformation();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(CONTAINER_VIEW, SingleTableView.class);
        registerChild(PAGE_ID, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {

        View child = null;

        if (name.equals(CONTAINER_VIEW)) {
            child = new SingleTableView(this, name);
        } else if (name.equals(PAGE_ID) ||
                   name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        super.beginDisplay(event);
        ((CCHiddenField) getChild(PAGE_ID)).
            setValue(Integer.toString(getPageID()));
        ((CCHiddenField) getChild(SERVER_NAME)).
            setValue(getPageSessionAttribute("SERVER_NAME"));
    }

    private void saveRequestInformation() {
        int pageID = Integer.parseInt(
            RequestManager.getRequest().getParameter("PAGE_ID"));
        TraceUtil.trace2("pageID is " + pageID);
        setPageSessionAttribute("PAGE_ID", new Integer(pageID));

        String serverName = RequestManager.getRequest().getParameter(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        serverName = serverName == null ? "" : serverName;
        TraceUtil.trace3("serverName is ".concat(serverName));
        setPageSessionAttribute("SERVER_NAME", serverName);
    }

    /**
     * Should call after saveRequestInformation()
     */
    private int getPageID() {
        return ((Integer) getPageSessionAttribute("PAGE_ID")).intValue();
    }
}
