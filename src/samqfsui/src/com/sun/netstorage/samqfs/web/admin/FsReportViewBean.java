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

// ident	$Id: FsReportViewBean.java,v 1.6 2008/12/16 00:10:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;

public class FsReportViewBean extends CommonViewBeanBase {

    private static String PAGE_NAME   = "FsReport";
    private static String DEFAULT_URL = "/jsp/admin/FsReport.jsp";

    // children
    public static final String PAGE_TITLE = "PageTitle";
    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String SERVER_NAME = "ServerName";
    public static final String FS_REPORT_VIEW = "FsReportView";

    public FsReportViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Entering");
    }

    public void registerChildren() {
        super.registerChildren();
        TraceUtil.trace3("Entering");
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(FS_REPORT_VIEW, FsReportView.class);
        TraceUtil.trace3("Entering");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(CHILD_STATICTEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(FS_REPORT_VIEW)) {
            return new FsReportView(this, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        FsReportView view = (FsReportView) getChild(FS_REPORT_VIEW);
        view.populateDisplay();

        TraceUtil.trace3("Exiting");
    }
}
