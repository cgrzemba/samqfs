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

// ident	$Id: LibrarySelectorViewBean.java,v 1.7 2008/12/16 00:12:26 am143972 Exp $
package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.media.MediaUtil;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;

public class LibrarySelectorViewBean extends CommonSecondaryViewBeanBase {
    private static final String PAGE_NAME = "LibrarySelector";
    private static final String DEFAULT_URL =
        "/jsp/util/LibrarySelector.jsp";

    public static final String PAGE_TITLE = "pageTitle";
    public static final String LIBRARY = "library";
    public static final String LIBRARY_LABEL = "libraryLabel";

    CCPageTitleModel ptModel = null;

    public LibrarySelectorViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        ptModel = createPageTitleModel();
        registerChildren();
    }

    public void registerChildren() {
        super.registerChildren();
        ptModel.registerChildren(this);
        registerChild(LIBRARY, CCDropDownMenu.class);
        registerChild(PAGE_TITLE, CCPageTitle.class);
    }

    public View createChild(String name) {
        if (name.equals(LIBRARY)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(LIBRARY_LABEL)) {
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

    public void handleOKButtonRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        // handle the import
        try {
        String s = getDisplayFieldStringValue(LIBRARY);
        String [] tokens = s.split(":");
        if (tokens != null && tokens.length > 1) {
            String libraryName = tokens[0];
            int driverType = Integer.parseInt(tokens[1]);
            // confirm that we infact a samst library
            if (driverType == Library.SAMST) {
                Library lib =
                    MediaUtil.getLibraryObject(getServerName(), libraryName);
                lib.importVSN();

                SamUtil.setInfoAlert(this,
                                     ALERT,
                                     "success.summary",
                SamUtil.getResourceString("LibrarySummary.action.import",
                                        libraryName),
                                     getServerName());
                ((CCButton)getChild("OKButton")).setDisabled(true);
            }
        }
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleImportButtonRequest",
                                     sfe.getMessage(),
                                     getServerName());

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "LibrarySummary.error.import",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  getServerName());
        }
        forwardTo(getRequestContext());
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // retrieve all the configured libraries here.
        try {
            Library [] all = SamUtil.getModel(getServerName())
                .getSamQFSSystemMediaManager().getAllLibraries();
            if (all != null && all.length > 0) {
                ArrayList names = new ArrayList();
                ArrayList drivers = new ArrayList();
                for (int i = 0; i < all.length; i++) {
                    if (!"historian".equalsIgnoreCase(all[i].getName())) {
                        names.add(all[i].getName());
                        drivers.add(all[i].getName() + ":" +
                        all[i].getDriverType());
                    }
                }

                String [] labels = (String [])names.toArray(new String[0]);
                String [] values = (String [])drivers.toArray(new String[0]);
                ((CCDropDownMenu)getChild(LIBRARY)).setOptions(
                    new OptionList(labels, values));
            } else {
                ((CCButton)getChild("OKButton")).setDisabled(true);
            }
        } catch (SamFSException sfe) {
            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "LibrarySummary.error.populate",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  getServerName());
        }
    }
}
