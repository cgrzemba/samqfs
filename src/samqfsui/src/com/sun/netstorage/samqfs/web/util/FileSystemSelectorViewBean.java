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

// ident	$Id: FileSystemSelectorViewBean.java,v 1.7 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;

public class FileSystemSelectorViewBean extends CommonSecondaryViewBeanBase {
    private static final String PAGE_NAME = "FileSystemSelector";
    private static final String DEFAULT_URL =
        "/jsp/util/FileSystemSelector.jsp";

    public static final String PAGE_TITLE = "pageTitle";
    public static final String FS = "fileSystem";
    public static final String FS_LABEL = "fileSystemLabel";

    private CCPageTitleModel ptModel = null;

    public FileSystemSelectorViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        ptModel = createPageTitleModel();
        registerChildren();
    }

    public void registerChildren() {
        super.registerChildren();
        ptModel.registerChildren(this);
        registerChild(FS, CCDropDownMenu.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
    }

    public View createChild(String name) {
        if (name.equals(FS)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(FS_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, ptModel, name);
        } else if (ptModel.isChildSupported(name)) {
            return ptModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        return new CCPageTitleModel(RequestManager
                                    .getRequestContext()
                                    .getServletContext(),
                                    "/jsp/util/SelectorPopupPageTitle.xml");
    }

    // handlers for OK and Cancel buttons to prevent stack traces incase of bad
    // or disabled javascript
    public void handleOKButtonRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void handleCancelButtonRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // retrieve all the configured archiving file systems.
        try {
            SamQFSSystemModel model = SamUtil.getModel(getServerName());

            FileSystem [] allfs =
                model.getSamQFSSystemFSManager().getAllFileSystems();
            ArrayList fsList = new ArrayList();
            for (int i = 0; i < allfs.length; i++) {
                if (!allfs[i].isHA() &&
                    (allfs[i].getArchivingType() == FileSystem.ARCHIVING) &&
                    (allfs[i].getState() == FileSystem.MOUNTED))
                    fsList.add(allfs[i].getName());
            }

            if (fsList.size() > 0) {
                String [] fs = new String[fsList.size()];
                fs = (String [])fsList.toArray(fs);

                CCDropDownMenu selector = (CCDropDownMenu)getChild(FS);
                selector.setOptions(new OptionList(fs, fs));
            } else {
                // set indication that no fs were found

                // disable ok button
                ((CCButton)getChild("OKButton")).setDisabled(true);
            }
        } catch (SamFSException sfe) {
            // set indication that no fs were found

            // disable ok button
            ((CCButton)getChild("OKButton")).setDisabled(true);
        }
    }
}
