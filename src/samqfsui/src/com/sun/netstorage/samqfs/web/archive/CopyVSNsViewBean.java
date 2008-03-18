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

// ident	$Id: CopyVSNsViewBean.java,v 1.26 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.taglib.html.CCStaticTextFieldTag;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Policy Copy VSN Map Details outer-most containerview
 */
public class CopyVSNsViewBean extends CommonViewBeanBase {
    private static final String PAGE_NAME = "CopyVSNs";
    private static final String DEFAULT_URL = "/jsp/archive/CopyVSNs.jsp";

    // children
    public static final String PAGE_TITLE = "pageTitle";
    public static final String BREADCRUMB = "BreadCrumb";

    // bread crumbing urls
    private static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    private static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    private static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    private static final String POLICY_TAPECOPY_HREF = "PolicyTapeCopyHref";
    private static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    private static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    private static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";
    private static final String COPY_VSNS_HREF = "CopyVSNsHref";

    // child views
    public static final String MEDIA_TYPE_LABEL = "mediaTypeLabel";
    public static final String MEDIA_TYPE = "mediaType";
    public static final String VSNS_DEFINED_LABEL = "vsnsDefinedLabel";
    public static final String VSNS_DEFINED = "vsnsDefined";
    public static final String POOL_LABEL = "poolDefinedLabel";
    public static final String POOL = "poolDefined";
    public static final String MEMBERS_LABEL = "availableMembersLabel";
    public static final String MEMBERS = "availableMembers";
    public static final String FREE_SPACE_LABEL = "freeSpaceLabel";
    public static final String FREE_SPACE = "freeSpace";

    public static final String MESSAGE = "message";

    public static final String PT_FILE = "/jsp/archive/CopyVSNsPageTitle.xml";
    private CCPageTitleModel ptModel = null;

    // hidden field for js
    public static final String HARD_RESET = "hardReset";
    public static final String ALL_POOLS = "all_pools";

    /** create an instance of this view bean */
    public CopyVSNsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // create the page title model
        ptModel = PageTitleUtil.createModel(PT_FILE);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this view's children */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        // register typical view bean children
        super.registerChildren();

        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(VSNS_DEFINED, CCLabel.class);
        registerChild(POOL_LABEL, CCLabel.class);
        registerChild(MEMBERS_LABEL, CCLabel.class);
        registerChild(FREE_SPACE_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE, CCDropDownMenu.class);
        registerChild(VSNS_DEFINED, CCTextField.class);
        registerChild(POOL, CCDropDownMenu.class);
        registerChild(MEMBERS, CCStaticTextFieldTag.class);
        registerChild(FREE_SPACE, CCStaticTextField.class);
        registerChild(HARD_RESET, CCHiddenField.class);
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(POLICY_TAPECOPY_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(COPY_VSNS_HREF, CCHref.class);
        registerChild(MESSAGE, CCStaticTextField.class);
        registerChild(ALL_POOLS, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, ptModel);
        TraceUtil.trace3("Exiting");
    }

    /** create an named child view */
    public View createChild(String name) {
        if (name.equals(HARD_RESET) ||
            name.equals(ALL_POOLS)) {
            return new CCHiddenField(this, name, "false");
        } else if (name.equals(MEDIA_TYPE_LABEL) ||
            name.equals(VSNS_DEFINED_LABEL) ||
            name.equals(POOL_LABEL) ||
            name.equals(MEMBERS_LABEL) ||
            name.equals(FREE_SPACE_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(MEDIA_TYPE) ||
                   name.equals(POOL)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(VSNS_DEFINED)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(MEMBERS) ||
                   name.equals(FREE_SPACE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(MESSAGE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("CopyVSNs.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(POLICY_TAPECOPY_HREF) ||
                   name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF) ||
                   name.equals(COPY_VSNS_HREF)) {
            return new CCHref(this, name, null);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }


    /** handler for the cancel button */
    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // we'll need these
        String serverName = getServerName();
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer copyNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);
        Integer copyType =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);

        boolean valid = true;
        boolean exception = true;
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            // make sure the policy wasn't deleted in the process
            if (thePolicy == null)
                throw new SamFSException(null, -2000);


            ArchiveCopy theCopy =
                    thePolicy.getArchiveCopy(copyNumber.intValue());

            if (theCopy == null)
                throw new SamFSException(null, -2006);

            // TODO: this should would reqardless, since we don't save copies
            // that use the allsets vsn map
            ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();

            // errors
            String errors = "";

            // media type
            String mediaTypeString = getDisplayFieldStringValue(MEDIA_TYPE);
            int mediaType = Integer.parseInt(mediaTypeString);

            String vsnsDefined = getDisplayFieldStringValue(VSNS_DEFINED);
            vsnsDefined = vsnsDefined == null ? "" : vsnsDefined.trim();

            String poolName = getDisplayFieldStringValue(POOL);
            if (poolName.equals(SelectableGroupHelper.NOVAL_LABEL) ||
                poolName.equals(SelectableGroupHelper.NOVAL)) {
                poolName = null;
            }

            if (vsnsDefined.equals("") && poolName.equals("")) {
                valid = false;
                errors = errors.concat(SamUtil.getResourceString(
                    "archiving.copyvsns.error"));
                ((CCLabel)getChild(VSNS_DEFINED_LABEL)).setShowError(true);
                ((CCLabel)getChild(POOL_LABEL)).setShowError(true);
            }

            if (valid) {
                vsnMap.setArchiveMediaType(mediaType);
                vsnMap.setMapExpression(vsnsDefined);
                vsnMap.setPoolExpression(poolName);

                // set save flag
                vsnMap.setWillBeSaved(true);
                // now update the policy to persist the vsn map settings
                thePolicy.updatePolicy();

                // if we get here, a hard reset is not required
                ((CCHiddenField)getChild(HARD_RESET)).setValue("true");

                // finally forward to the calling page
                forwardToPreviousPage(true);
                exception = false;
            } else {
                SamUtil.setErrorAlert(this,
                                      CHILD_COMMON_ALERT,
                                      "ArchivePolCopy.error.save",
                                      -2028,
                                      errors,
                                      serverName);
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     getClass(),
                                     "handleSaveRequest",
                                     "unable to save copy vsns",
                                     serverName);

            SamUtil.setWarningAlert(this,
                                    CHILD_COMMON_ALERT,
                                    "ArchiveConfig.warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "handleSaveRequest",
                                     "unable to save copy vsns",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleSaveRequest",
                                     "unable to save copy vsns",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
        // set the reset condition
        ((CCHiddenField)getChild(HARD_RESET)).setValue("true");
        if (exception) {
            forwardTo(getRequestContext());
        }
    }

    /**
     * handler for the reset button
     *
     * not really necessary since reset is handled by browser's form but,
     * just in case the javascriprt fails, we need something to handle the
     * request cleanly
     */
    public void handleResetRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        PolicyUtil.resetArchiveManager(this);
        ((CCHiddenField)getChild(HARD_RESET)).setValue("false");

        forwardTo(getRequestContext());
    }

    /** handler for the save button */
    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        Boolean bool = Boolean.valueOf(getDisplayFieldStringValue(HARD_RESET));

        if (bool.booleanValue()) {
            PolicyUtil.resetArchiveManager(this);

            ((CCHiddenField)getChild(HARD_RESET)).setValue("false");
        }

        forwardToPreviousPage(false);
    }

    /**
     * we can get to this method from either, handle save or handle cancel.
     */
    private void forwardToPreviousPage(boolean save) {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase target = null;

        // get the last link in the breadcrumb
        Integer[] temp = (Integer [])getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length-1].intValue();

        // get the name of the link
        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetHref = pageInfo.getPagePath(index).getCommandField();

        // two possible pages can go back to
        if (targetHref.equals(PolicyDetailsViewBean.POLICY_DETAILS_HREF))
            target =
                (CommonViewBeanBase)getViewBean(PolicyDetailsViewBean.class);

        // display field value for the Href
        String s = Integer.toString
                (BreadCrumbUtil.inPagePath(path, index, path.length-1));

        if (save) {
            SamUtil.setInfoAlert(target,
                                 "Alert",
                                 "success.summary",
                                 "PolCopyVSN.save",
                                 getServerName());
        }

        // go back to the last page in the breadcrumb
         BreadCrumbUtil.breadCrumbPathBackward(this,
                PageInfo.getPageInfo().getPageNumber(target.getName()), s);

         forwardTo(target);

        TraceUtil.trace3("Exiting");
    }

    /** populate the page with the preset copy vsns */
    private void populateCopyVSNsPage() {
        String serverName = getServerName();
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer copyNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);
        Integer copyType =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            // make sure the policy wasn't deleted in the process
            if (thePolicy == null)
                throw new SamFSException(null, -2000);


            ArchiveCopy theCopy =
                    thePolicy.getArchiveCopy(copyNumber.intValue());

            if (theCopy == null)
                throw new SamFSException(null, -2006);

            ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();

            if (vsnMap.inheritedFromALLSETS()) {
                CCStaticTextField message =
                    (CCStaticTextField)getChild(MESSAGE);
                message.setValue("archiving.copyvsns.usingallsets");
            }

            // now retrieve the copy vsn parameters and polulate the page
            // media type
            CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MEDIA_TYPE);
            int mediaType = vsnMap.getArchiveMediaType();

            // if 4.3 disk copy, start with a zero length array
            int [] mediaTypes = null;
            if (sysModel.getServerAPIVersion().compareTo("1.3") < 0 &&
                mediaType == BaseDevice.MTYPE_DISK) {
                mediaTypes = new int[0];
            } else {
                mediaTypes = sysModel.getSamQFSSystemMediaManager().
                    getAvailableArchiveMediaTypes();
            }

            boolean exist = false;
            for (int i = 0; i < mediaTypes.length; i++) {
                if (mediaTypes[i] == mediaType) {
                    exist = true;
                    break;
                }
            }

            int count = exist ? mediaTypes.length : mediaTypes.length + 1;
            String [] mediaTypeLabels = new String[count];
            String [] mediaTypeValues = new String[count];

            int index = 0;
            if (!exist) {
                mediaTypeLabels[0] = SamUtil.getMediaTypeString(mediaType);
                mediaTypeValues[0] = Integer.toString(-999);

                index = 1;

                // disable the save button
                ((CCButton)getChild("Save")).setDisabled(true);
            }

            for (int j = 0, i = index; j < mediaTypes.length; i++, j++) {
                mediaTypeLabels[i] = SamUtil.getMediaTypeString(mediaTypes[j]);
                mediaTypeValues[i] = Integer.toString(mediaTypes[j]);
            }

            dropDown.setOptions(
                new OptionList(mediaTypeLabels, mediaTypeValues));

            String mediaTypeString = mediaType == -1 ?
                "media.type.unknown" : Integer.toString(mediaType);

            dropDown.setValue(mediaTypeString);

            // vsns defined
            String vsnsDefined = vsnMap.getMapExpression();
            CCTextField textField = (CCTextField)getChild(VSNS_DEFINED);
            textField.setValue(vsnsDefined);

            // vsn pool name
            dropDown = (CCDropDownMenu)getChild(POOL);

            String [] pools =
                PolicyUtil.getVSNPoolNames(mediaType, serverName);

            String [] poolNames = new String[pools.length + 1];
            String [] poolValues = new String[pools.length + 1];

            poolNames[0] = SelectableGroupHelper.NOVAL_LABEL;
            poolValues[0] = SelectableGroupHelper.NOVAL;

            for (int i = 0, j = 1; i < pools.length; i++, j++) {
                poolNames[j] = pools[i];
                poolValues[j] = pools[i];
            }

            dropDown.setOptions(new OptionList(poolNames, poolValues));
            dropDown.setValue(vsnMap.getPoolExpression());

            // space available
            CCStaticTextField text = (CCStaticTextField)getChild(FREE_SPACE);
            text.setValue(new Capacity(vsnMap.getAvailableSpace(),
                                       SamQFSSystemModel.SIZE_MB));

            // member vsns available
            text = (CCStaticTextField)getChild(MEMBERS);
            text.setValue(getVSNString(vsnMap.getMemberVSNNames()));

            String ps = PolicyUtil.encodePoolMediaTypeString(serverName);
            ((CCHiddenField)getChild(ALL_POOLS)).setValue(ps);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setWarningAlert(this,
                                    CHILD_COMMON_ALERT,
                                    "ArchiveConfig..warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    /**
     * concatinate an array of vsn names into a single comma delimited list
     *
     */
    private String getVSNString(String [] vsns) {
        if (vsns == null)
            return "";

        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        for (int i = 0; i < vsns.length; i++) {
            buf.append(vsns[i]).append(", ");
        }

        String temp = buf.toString();
        if (temp.length() > 0) {
            temp = temp.substring(0, temp.lastIndexOf(","));
        }

        return temp;
    }

    /** begin display */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // initialize page title buttons
        ptModel.setValue("Save", "archiving.save");
        ptModel.setValue("Reset", "archiving.reset");
        ptModel.setValue("Cancel", "archiving.cancel");

        // set the page title text
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer copyNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);

        ptModel.setPageTitleText(
            SamUtil.getResourceString("archiving.policy.copyvsns.pagetitle",
                new String[] {policyName,
            SamUtil.getResourceString("archiving.copynumber",
                                      new String [] {copyNumber.toString()})}));

        // populate the copy
        populateCopyVSNsPage();

        // disable save & reset button if no media authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {

            ((CCButton)getChild("Save")).setDisabled(true);
            ((CCButton)getChild("Reset")).setDisabled(true);
        }
    }

    // bread crumbing links
    // handle breadcrumb to the policy summary page
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        ViewBean target = getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the policy details summary page - incase we loop
    // back here
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        ViewBean target = getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the criteria details summary page - incase we loop
    // back here
    public void handleCriteriaDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem page
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_SUMMARY_HREF);
        ViewBean target = getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem deatils page
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_DETAILS_HREF);
        ViewBean target = getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the fs archive policies
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_ARCHIVEPOL_HREF);
        ViewBean target = getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }
}
