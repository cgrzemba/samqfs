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

// ident	$Id: NewFileSystemPopupViewBean.java,v 1.2 2008/09/04 19:30:34 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

public class NewFileSystemPopupViewBean extends CommonSecondaryViewBeanBase {

    public static final String PAGE_NAME = "NewFileSystemPopup";
    public static final String DEFAULT_URL = "/jsp/fs/NewFileSystemPopup.jsp";

    // child views
    public static final String PAGE_TITLE = "PageTitle";
    public static final String CANCEL = "Cancel";
    public static final String OK = "Ok";
    public static final String QFS = "QFSRadioButton";
    public static final String UFS = "UFSRadioButton";
    public static final String ARCHIVING = "archivingCheckBox";
    public static final String SHARED = "sharedCheckBox";
    public static final String HPC = "HPCCheckBox";
    public static final String HAFS = "HAFSCheckBox";
    public static final String MATFS = "matfsCheckBox";
    public static final String WIZARD_VIEW = "NewFileSystemPopupView";

    // its not necessary to register labels and static fields
    public static final String LABEL = "Label";
    public static final String TEXT = "Text";

    // hidden
    public static final String ARCHIVE_MEDIA = "hasArchiveMedia";
    public static final String SERVER_NAME = "serverName";
    public static final String MESSAGES = "messages";

    public NewFileSystemPopupViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(OK, CCButton.class);
        registerChild(CANCEL, CCButton.class);
        registerChild(QFS, CCRadioButton.class);
        registerChild(UFS, CCRadioButton.class);
        registerChild(ARCHIVING, CCCheckBox.class);
        registerChild(SHARED, CCCheckBox.class);
        registerChild(HPC, CCCheckBox.class);
        registerChild(HAFS, CCCheckBox.class);
        registerChild(MATFS, CCCheckBox.class);
        registerChild(ARCHIVE_MEDIA, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(MESSAGES, CCHiddenField.class);
        registerChild(WIZARD_VIEW, NewFileSystemPopupView.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.endsWith(LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.endsWith(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(OK) ||
            name.equals(CANCEL)) {
            return new CCButton(this, name, null);
        } else if (name.equals(QFS)) {
            CCRadioButton rbutton = new CCRadioButton(this, QFS, "qfs");
            rbutton.setOptions(
                new OptionList(new String[] {"fs.newfs.popup.fstype.qfs"},
                               new String[] {"qfs"}));
            return rbutton;
        } else if (name.equals(UFS)) {
            CCRadioButton rbutton = new CCRadioButton(this, QFS, "qfs");
            rbutton.setOptions(
                new OptionList(new String[] {"fs.newfs.popup.fstype.ufs"},
                               new String[] {"ufs"}));
            return rbutton;
        } else if (name.equals(ARCHIVING)||
            name.equals(SHARED)||
            name.equals(HPC) ||
            name.equals(HAFS) ||
            name.equals(MATFS)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.contains(ARCHIVE_MEDIA)||
            name.equals(SERVER_NAME) ||
            name.equals(MESSAGES)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(WIZARD_VIEW)) {
            return new NewFileSystemPopupView(this, name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    /**
     * handler for the Ok button.
     * This will be used to create the proto file system.
     * @param evt
     */
    public void handleOkRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        // create the proto file system

        forwardTo(getRequestContext());
    }

    /* called before this container generating its display content */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        String serverName = getServerName();
        try {

            // check if this server has any archive media configured.
            SamQFSSystemModel model = SamUtil.getModel(serverName);
            boolean hasArchiveMedia = model.hasArchivingMedia();

            ((CCHiddenField)getChild(ARCHIVE_MEDIA)).setValue(hasArchiveMedia);
            ((CCHiddenField)getChild(SERVER_NAME)).setValue(serverName);

            // error messages
            StringBuffer messages = new StringBuffer();
            messages.append(SamUtil.getResourceString(
                "fs.newfs.archivemedia.missing"))
                .append(";")
                .append(SamUtil.getResourceString(
                    "fs.newfs.hahpc.unsupported"));

            ((CCHiddenField)getChild(MESSAGES)).setValue(messages.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "beginDisplay",
                                     "Unable to retrieve archive media",
                                     serverName);
        }
    }

    /**
     * Only display the HAFS checkbox if the server being managed is part of
     * a sun cluster configuration.
     * @param evt
     * @return true if a part of a cluster, false if not. A true return value
     * will result in the check box being displayed otherwise it will not be
     * dipslayed.
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginHAFSCheckBoxDisplay(ChildDisplayEvent evt)
        throws ModelControlException {

        boolean isClusterNode = false;

        try {
            isClusterNode = SamUtil.isClusterNode(getServerName());
        } catch (SamFSException sfe) {
            TraceUtil.trace1(
                "Exception caught checking if server is a part of cluster!");
            TraceUtil.trace1("Reason: " + sfe.getMessage());
        }

        return isClusterNode;
    }
}
