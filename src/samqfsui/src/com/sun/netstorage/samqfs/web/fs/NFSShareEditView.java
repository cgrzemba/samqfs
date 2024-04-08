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

// ident	$Id: NFSShareEditView.java,v 1.8 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.NFSOptions;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * NFSShareEditView - view in NFS Shares Page when user wants to edit
 * NFS shares.
 */
public class NFSShareEditView extends RequestHandlingViewBase
    implements CCPagelet {

    // Page Children
    private static final String PAGE_TITLE = "EditPageTitle";

    // Child view names (i.e. display fields).
    private static final String READONLY_LABEL  = "roLabel";
    private static final String READONLY_CHECKBOX = "roCheckBox";
    private static final String ROACCESS_LABEL  = "roAccessLabel";
    private static final String ROACCESS_TEXTFIELD = "roAccessValue";
    private static final String READWRITE_LABEL = "rwLabel";
    private static final String READWRITE_CHECKBOX = "rwCheckBox";
    private static final String RWACCESS_LABEL  = "rwAccessLabel";
    private static final String RWACCESS_TEXTFIELD = "rwAccessValue";
    private static final String ROOT_LABEL = "rootLabel";
    private static final String ROOT_CHECKBOX = "rootCheckBox";
    private static final String ROOTACCESS_LABEL = "rootAccessLabel";
    private static final String ROOTACCESS_TEXTFIELD = "rootAccessValue";
    private static final String ROOTACCESS_HELP_TEXT = "rootAccessHelpText";
    private static final String ROACCESS_HELP_TEXT = "roAccessHelpText";
    private static final String RWACCESS_HELP_TEXT = "rwAccessHelpText";
    private static final
        String ACCESSDELIMIT_HELP_TEXT = "accessDelimitHelpText";

    // For Javasript use in root case
    private static final String CONFIRM_MESSAGE = "ConfirmMessage";


    // private models for various components
    private CCPageTitleModel pageTitleModel;

    // keep track of the server name that is transferred from the VB
    private String serverName;

    public NFSShareEditView(View parent, String name, String serverName) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.serverName = serverName;

        pageTitleModel = createPageTitleModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * registerChildren
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        pageTitleModel.registerChildren(this);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(ROOTACCESS_HELP_TEXT, CCStaticTextField.class);
        registerChild(ROACCESS_HELP_TEXT, CCStaticTextField.class);
        registerChild(RWACCESS_HELP_TEXT, CCStaticTextField.class);
        registerChild(ACCESSDELIMIT_HELP_TEXT, CCStaticTextField.class);
        registerChild(READONLY_CHECKBOX, CCCheckBox.class);
        registerChild(ROACCESS_TEXTFIELD, CCTextField.class);
        registerChild(READWRITE_CHECKBOX, CCCheckBox.class);
        registerChild(RWACCESS_TEXTFIELD, CCTextField.class);
        registerChild(ROOT_CHECKBOX, CCCheckBox.class);
        registerChild(ROOTACCESS_TEXTFIELD, CCTextField.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * createChild
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;

        if (name.equals(PAGE_TITLE)) {
            child = new CCPageTitle(this, pageTitleModel, name);
        } else if (pageTitleModel.isChildSupported(name)) {
            // Create child from page title model.
            child =  pageTitleModel.createChild(this, name);
        } else if (name.equals(READONLY_LABEL) ||
            name.equals(ROACCESS_LABEL) ||
            name.equals(READWRITE_LABEL) ||
            name.equals(RWACCESS_LABEL) ||
            name.equals(ROOT_LABEL) ||
            name.equals(ROOTACCESS_LABEL)) {
            return new CCLabel(this, name, null);

        } else if (name.equals(ROACCESS_TEXTFIELD) ||
            name.equals(RWACCESS_TEXTFIELD) ||
            name.equals(ROOTACCESS_TEXTFIELD)) {
            return new CCTextField(this, name, null);

        } else if (name.equals(READONLY_CHECKBOX) ||
            name.equals(READWRITE_CHECKBOX) ||
            name.equals(ROOT_CHECKBOX)) {
            return new CCCheckBox(this, name, "true", "false", false);

        } else if (name.equals(ROOTACCESS_HELP_TEXT) ||
            name.equals(ROACCESS_HELP_TEXT) ||
            name.equals(RWACCESS_HELP_TEXT) ||
            name.equals(ACCESSDELIMIT_HELP_TEXT)) {
            return new CCStaticTextField(this, name, null);

        } else if (name.equals(CONFIRM_MESSAGE)) {
            return new CCHiddenField(
                this, name,
                SamUtil.getResourceString("filesystem.nfs.validate.errMsg5"));

        } else {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String dirName = getDirName();

       // populate the nfs options
        String reason = "";
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            // set page title to dirName + NFS Options
            pageTitleModel.setPageTitleText(
                SamUtil.getResourceString(
                    "filesystem.nfs.options.pageTitle1", dirName));

            NFSOptions opts = getNFSOptions(sysModel, dirName);

            // get rw, ro and root access lists
            String roAccessList   = opts.getReadOnlyAccessList();
            String rwAccessList   = opts.getReadWriteAccessList();
            String rootAccessList = opts.getRootAccessList();

            CCCheckBox rwaccessBox =
                (CCCheckBox) getChild(READWRITE_CHECKBOX);
            CCTextField rwaccessField =
                (CCTextField) getChild(RWACCESS_TEXTFIELD);

            CCTextField roaccessField =
                (CCTextField) getChild(ROACCESS_TEXTFIELD);
            CCCheckBox roaccessBox =
                (CCCheckBox) getChild(READONLY_CHECKBOX);

            CCTextField rootaccessField =
                (CCTextField) getChild(ROOTACCESS_TEXTFIELD);
            CCCheckBox rootaccessBox =
                (CCCheckBox) getChild(ROOT_CHECKBOX);

            // reset all boxes
            rwaccessField.resetStateData();
            rwaccessField.setValue("");
            rwaccessField.setDisabled(true);
            rwaccessBox.setChecked(false);

            roaccessField.resetStateData();
            roaccessField.setValue("");
            roaccessField.setDisabled(true);
            roaccessBox.setChecked(false);

            rootaccessField.resetStateData();
            rootaccessField.setValue("");
            rootaccessField.setDisabled(true);
            rootaccessBox.setChecked(false);


            if (roAccessList == null && rwAccessList == null) {
                // default options, check rw checkbox
                rwaccessField.setDisabled(false);
                rwaccessBox.setChecked(true);
            } else {
                // check the individual lists
                if (roAccessList != null) {
                    roaccessField.setDisabled(false);
                    roaccessBox.setChecked(true);
                    if (roAccessList.length() > 0) {
                        roaccessField.setValue(roAccessList);
                    } // otherwise empty list
                }
                if (rwAccessList != null) {
                    rwaccessField.setDisabled(false);
                    rwaccessBox.setChecked(true);
                    if (rwAccessList.length() > 0) {
                        rwaccessField.setValue(rwAccessList);
                    }
                }
            }

            if (rootAccessList != null) {
                rootaccessField.setDisabled(false);
                rootaccessBox.setChecked(true);
                if (rootAccessList.length() > 0) {
                    rootaccessField.setValue(rootAccessList);
                }
            }
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay()",
                "Failed to load options",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedLoadOptions",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    // implement the CCPagelet interface

    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        return "/jsp/fs/NFSShareEditPagelet.jsp";
    }

    /**
     * Create pagetitle model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/fs/NFSShareEditPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return  pageTitleModel;
    }


    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String dirName = getDirName();

        // dispaly field values
        boolean roChk = Boolean.valueOf(
            getDisplayFieldStringValue(READONLY_CHECKBOX)).booleanValue();
        boolean rwChk = Boolean.valueOf(
            getDisplayFieldStringValue(READWRITE_CHECKBOX)).booleanValue();
        boolean rootChk = Boolean.valueOf(
            getDisplayFieldStringValue(ROOT_CHECKBOX)).booleanValue();

        String roAccess   = getDisplayFieldStringValue(ROACCESS_TEXTFIELD);
        String rwAccess   = getDisplayFieldStringValue(RWACCESS_TEXTFIELD);
        String rootAccess = getDisplayFieldStringValue(ROOTACCESS_TEXTFIELD);
        roAccess   = roAccess   == null ? "" : roAccess.trim();
        rwAccess   = rwAccess   == null ? "" : rwAccess.trim();
        rootAccess = rootAccess == null ? "" : rootAccess.trim();

        String errorMessage =
            validate(roChk, roAccess, rwChk, rwAccess, rootChk, rootAccess);
        if (errorMessage != null) {
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedEdit",
                -2601,
                errorMessage,
                serverName);
            getParentViewBean().forwardTo(getRequestContext());
            TraceUtil.trace3("Exiting");
            return;
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            NFSOptions opts = getNFSOptions(sysModel, dirName);

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new NonSyncStringBuffer().append(
                    "Start editing NFS options for ").
                    append(dirName).toString());

            opts.setReadOnlyAccessList(roChk ? roAccess : null);
            opts.setReadWriteAccessList(rwChk ? rwAccess : null);
            opts.setRootAccessList(rootChk ? rootAccess : null);
            sysModel.setNFSOptions(opts);

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                new NonSyncStringBuffer().append(
                    "Finished editing NFS options for ").
                    append(dirName).toString());
            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(
                  "filesystem.nfs.msg.edit", dirName),
                serverName);
            setFlag(0);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to edit NFS options",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "NFSDetailsViewBean.error.failedEdit",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * This helper function validates user input.
     * Returns null if no error is found, otherwise return the error message.
     */
    private String validate(
        boolean roChk, String roAccess,
        boolean rwChk, String rwAccess,
        boolean rootChk, String rootAccess) {

        // If read-only is checked and access list is not empty, validate
        if (roChk && roAccess.length() > 0 &&
            !SamUtil.isValidAccessListString(roAccess)) {
            return "filesystem.nfs.validate.errMsg2";
        }

        // If read-write is checked and access list is not empty, validate
        if (rwChk && rwAccess.length() > 0 &&
            !SamUtil.isValidAccessListString(rwAccess)) {
            return "filesystem.nfs.validate.errMsg3";
        }

        // If root is checked and the list is not empty, validate
        if (rootChk && !SamUtil.isValidAccessListString(rootAccess)) {
            return "filesystem.nfs.validate.errMsg4";
        }

        // If both roChk and rwChk are checked and
        // if rwAccess and roAccess are both empty, then alert
        // else if user enters the same userid for readonly, show alert
        if (roChk && rwChk) {
            if (roAccess.length() == 0 && rwAccess.length() == 0) {
                return "filesystem.nfs.validate.errMsg1";
            } else if (isDuplicate(roAccess.split(":"), rwAccess.split(":"))) {
                return "filesystem.nfs.validate.errMsg6";
            }
        }

        // if code reaches here, no error is found
        return null;
    }

    private void setFlag(int flag) {
        if (flag >= 0 && flag <= 2) {
            getParentViewBean().
                setPageSessionAttribute(NFSShareDisplayView.FLAG,
                new Integer(flag));
        }
    }

    private int getSelectedIndex() {
        int index = -1;

        try {
            index = Integer.parseInt((String)
                ((CCHiddenField) getParentViewBean().getChild(
                    NFSDetailsViewBean.SELECTED_INDEX)).getValue());
        } catch (NumberFormatException numEx) {
            // should not reach here
            TraceUtil.trace1(
                "NumberFormatException caught in getSelectedIndex!");
            TraceUtil.trace1("Reason: " + numEx.getMessage());
        }

        return index;
    }

    private String getDirName() {
        String dirNameList =
            (String) ((CCHiddenField) getParentViewBean().getChild(
                NFSDetailsViewBean.DIR_NAMES_LIST)).getValue();
        String [] dirNamesArray = dirNameList.split("###");
        return dirNamesArray[getSelectedIndex()];
    }

    /**
     * This helper method retrieves the NFSOption object given the NFS shared
     * directory name.
     */
    private NFSOptions getNFSOptions(
        SamQFSSystemModel sysModel, String dirName)
        throws SamFSException {

        return sysModel.getNFSOption(dirName);
    }

    /**
     * This helper method checks if there are duplicate entries in the
     * read only and read-write access list.
     */
    private boolean isDuplicate(String [] roArray, String [] rwArray) {
        for (int i = 0; i < roArray.length; i++) {
            for (int j = 0; j < rwArray.length; j++) {
                if (roArray[i].equals(rwArray[j])) {
                    return true;
                }
            }
        }

        // if code reaches here, no duplicate is found
        return false;
    }

}
