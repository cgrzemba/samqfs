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

// ident	$Id: CopyVSNExtensionViewBean.java,v 1.1 2008/04/03 02:21:39 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;



/**
 *  This class is the view bean for the copy vsn extension page
 */

public class CopyVSNExtensionViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "CopyVSNExtension";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/archive/CopyVSNExtension.jsp";

    private static final String POLICY_INFO = "SAMQFS_policy_info";
    private static final String MEDIA_TYPE = "SAMQFS_media_type";

    // Page components
    private static final String RADIO_TYPE_1 = "RadioType1";
    private static final String RADIO_TYPE_2 = "RadioType2";
    private static final String RADIO_TYPE_3 = "RadioType3";
    private static final String EXISTING_POOL = "ExistingPool";
    private static final String LABEL = "Label";
    private static final String HELP_TEXT = "HelpText";
    private static final String HIDDEN_MEDIA_TYPE = "MediaType";
    private static final String HIDDEN_SERVER_NAME = "ServerName";
    private static final String HIDDEN_POLICY_NAME = "PolicyName";
    private static final String HIDDEN_COPY_NUMBER = "CopyNumber";
    
    // Page Title Attributes
    private CCPageTitleModel pageTitleModel = null;

    private static OptionList radioOptions1 = new OptionList(
        new String [] {"CopyVSNs.extension.choice.createexp"},
        new String [] {"createexp"});

    private static OptionList radioOptions2 = new OptionList(
        new String [] {"CopyVSNs.extension.choice.createpool"},
        new String [] {"createpool"});

    private static OptionList radioOptions3 = new OptionList(
        new String [] {"CopyVSNs.extension.choice.useexistingpool"},
        new String [] {"useexistingpool"});
    /**
     * Constructor
     */
    public CopyVSNExtensionViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(RADIO_TYPE_1, CCRadioButton.class);
        registerChild(EXISTING_POOL, CCSelectableList.class);
        registerChild(LABEL, CCLabel.class);
        registerChild(HELP_TEXT, CCStaticTextField.class);
        registerChild(HIDDEN_MEDIA_TYPE, CCHiddenField.class);
        registerChild(HIDDEN_SERVER_NAME, CCHiddenField.class);
        registerChild(HIDDEN_POLICY_NAME, CCHiddenField.class);
        registerChild(HIDDEN_COPY_NUMBER, CCHiddenField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (name.equals(RADIO_TYPE_1)) {
            CCRadioButton myChild = new CCRadioButton(this, name, null);
            myChild.setOptions(radioOptions1);
            return (View) myChild;
        } else if (name.equals(RADIO_TYPE_2)) {
            CCRadioButton myChild = new CCRadioButton(this, RADIO_TYPE_1, null);
            myChild.setOptions(radioOptions2);
            return (View) myChild;
        } else if (name.equals(RADIO_TYPE_3)) {
            CCRadioButton myChild = new CCRadioButton(this, RADIO_TYPE_1, null);
            myChild.setOptions(radioOptions3);
            return (View) myChild;
        } else if (name.equals(EXISTING_POOL)) {
            return new CCSelectableList(this, name, null);
        } else if (name.equals(LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(HELP_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(HIDDEN_MEDIA_TYPE)
                || name.equals(HIDDEN_SERVER_NAME)
                || name.equals(HIDDEN_POLICY_NAME)
                || name.equals(HIDDEN_COPY_NUMBER) ) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException(new NonSyncStringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        super.beginDisplay(event);
        saveParam();
        populateExistingPoolBox();

        // set default radio button to the first item if it has no value
        CCRadioButton radio = (CCRadioButton) getChild(RADIO_TYPE_1);
        if (radio.getValue() == null) {
            radio.setValue("createexp");
        }
    }

    /**
     * disablePageButtons()
     * Called if there are any errors displaying the initial values of the
     * checkboxes
     */
    private void disablePageButtons() {
        ((CCButton) getChild("Submit")).setDisabled(true);
    }

    /**
     * Instantiate the page title model
     *
     * @return CCPageTitleModel The page title model of CopyVSNExtension Page
     */
    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/archive/CopyVSNExtensionPageTitle.xml");
        }

        return pageTitleModel;
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        CCSelectableList poolBox = (CCSelectableList) getChild(EXISTING_POOL);
        Object [] selectedPoolArray = (Object []) poolBox.getValues();
        selectedPoolArray =
            selectedPoolArray == null ? new Object[0] : selectedPoolArray;

        TraceUtil.trace3("About to add selected pool to the copy vsn, " +
            "number of pools about to be added: " + selectedPoolArray.length);

        String radioValue = (String) getDisplayFieldValue(RADIO_TYPE_1);
System.out.println("radio: " + radioValue);
        if (!radioValue.equals("useexistingpool")) {
            forwardToMediaAssigner(radioValue.equals("createpool"));
            return;
        }

        try {
            // At least one volume pool has to be selected.
            if (selectedPoolArray.length == 0) {
                throw new SamFSException("CopyVSNs.extension.error");
            }

            String serverName = getServerName();
            String policyName = getPolicyName();

            int copyNumber = getCopyNumber();

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            // make sure the policy wasn't deleted in the process
            if (thePolicy == null) {
                throw new SamFSException(null, -2000);
            }

            ArchiveCopy theCopy = thePolicy.getArchiveCopy(copyNumber);
            if (theCopy == null) {
                throw new SamFSException(null, -2006);
            }
            ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();

            StringBuffer poolBuf = new StringBuffer();

            String existingPoolStr = vsnMap.getPoolExpression();
            existingPoolStr = existingPoolStr == null ? "" : existingPoolStr;

            TraceUtil.trace3("Old Pool Expression: " + existingPoolStr);
System.out.println("Old Pool Expression: " + existingPoolStr);
            poolBuf.append(existingPoolStr);

            // Append the new selected pool to the list
            for (int i = 0; i < selectedPoolArray.length; i++) {
                if (poolBuf.length() > 0) {
                    poolBuf.append(",");
                }
                poolBuf.append(selectedPoolArray[i]);
            }

            TraceUtil.trace3("New Pool Expression: " + poolBuf);
System.out.println("New Pool Expression: " + poolBuf);
            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                "Start editing attribute of tape");

            vsnMap.setPoolExpression(poolBuf.toString());
            vsnMap.setWillBeSaved(true);
            thePolicy.updatePolicy();

            LogUtil.info(
                this.getClass(),
                "handleSubmitRequest",
                "Done editing attribute of tape");
            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "CopyVSNs.extension.success.addpool"),
                getServerName());

            setSubmitSuccessful(true);
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(
                sfw,
                this.getClass(),
                "handleSubmitRequest",
                "Warnings caught while adding pools to copy volume list",
                getServerName());
            SamUtil.setWarningAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "ArchiveConfig.error.summary",
                sfw.getMessage());
            setSubmitSuccessful(true);
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest",
                "Failed to set Media Attributes",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                SamUtil.getResourceString(
                    "CopyVSNs.extension.error.addpool"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        forwardTo(getRequestContext());
    }

    private void saveParam() {
        String policyInfo = getPolicyInfo();

        ((CCHiddenField) getChild(HIDDEN_MEDIA_TYPE)).setValue(
                                Integer.toString(getMediaType()));
        ((CCHiddenField) getChild(HIDDEN_SERVER_NAME)).setValue(
                                getServerName());
        ((CCHiddenField) getChild(HIDDEN_POLICY_NAME)).setValue(
                                getPolicyName());
        ((CCHiddenField) getChild(HIDDEN_COPY_NUMBER)).setValue(
                                Integer.toString(getCopyNumber()));

        TraceUtil.trace3("CopyVSNExtension: policyInfo is " + policyInfo);
        TraceUtil.trace3("CopyVSNExtension: mediaType is " + getMediaType());
    }

    private void populateExistingPoolBox() {
        StringBuffer poolBuf = new StringBuffer();
        OptionList poolOptions = null;
        int mediaType = getMediaType();

        try {
            VSNPool [] allPools = PolicyUtil.getAllVSNPools(getServerName());
            for (int i = 0; i < allPools.length; i++) {
                if (allPools[i].getMediaType() == mediaType &&
                    notUsedInThisCopy(allPools[i])) {
                    poolBuf.append(allPools[i].getPoolName()).append(",");
                }
            }
            String [] poolArray = null;
            if (poolBuf.length() != 0) {
                poolBuf.deleteCharAt(poolBuf.length() - 1);
                poolArray = poolBuf.toString().split(",");
            } else {
                poolArray = new String[0];
            }
            poolOptions = new OptionList(poolArray, poolArray);

        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException caught!", samEx);
            SamUtil.processException(
                samEx,
                this.getClass(),
                "populateExistingPoolBox()",
                "Failed to populate existing volume pool information",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonSecondaryViewBeanBase.ALERT,
                "VSNPoolSummary.error.failedPopulate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        CCSelectableList poolBox = (CCSelectableList) getChild(EXISTING_POOL);
        poolBox.setOptions(poolOptions);
    }

    /**
     * Avoid "NotImplement" error message
     * @param event
     * @throws javax.servlet.ServletException
     * @throws java.io.IOException
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void handleCancelRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        forwardTo(getRequestContext());
    }

    private void forwardToMediaAssigner(boolean newPool) {
        NewEditVSNPoolViewBean target =
            (NewEditVSNPoolViewBean) getViewBean(NewEditVSNPoolViewBean.class);
        target.setPageSessionAttribute(
            target.OPERATION,
            newPool ?
                "NEW_POOL_ADD_TO_COPYVSN" :
                "NEW_EXP_ADD_TO_COPYVSN");
        target.setPageSessionAttribute(
            target.MEDIA_TYPE,
            (String) getDisplayFieldValue(HIDDEN_MEDIA_TYPE));
        target.setPageSessionAttribute(
            target.POLICY_INFO,
            (String) getDisplayFieldValue(HIDDEN_POLICY_NAME) + "." +
            (String) getDisplayFieldValue(HIDDEN_COPY_NUMBER));
        target.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            getServerName());
        target.forwardTo(getRequestContext());
    }

    private boolean notUsedInThisCopy(VSNPool pool) throws SamFSException {
        String serverName = getServerName();
        String policyName = getPolicyName();
        int copyNumber = getCopyNumber();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        ArchivePolicy thePolicy = sysModel.
            getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
        // make sure the policy wasn't deleted in the process
        if (thePolicy == null) {
            throw new SamFSException(null, -2000);
        }

        ArchiveCopy theCopy = thePolicy.getArchiveCopy(copyNumber);
        if (theCopy == null) {
            throw new SamFSException(null, -2006);
        }
        ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();
        String [] existingPoolArray =
            vsnMap.getPoolExpression() == null ||
            vsnMap.getPoolExpression().length() == 0 ?
                new String[0] : vsnMap.getPoolExpression().split(",");
        for (int i = 0; i < existingPoolArray.length; i++) {
            if (pool.getPoolName().equals(existingPoolArray[i])) {
                // the pool is already used by this copy vsn, don't show
                return false;
            }
        }
        return true;
    }

    private String getPolicyInfo() {
        // first check the page session
        String policyInfo = (String) getPageSessionAttribute(POLICY_INFO);

        // second check the request
        if (policyInfo == null) {
            policyInfo = RequestManager.getRequest().getParameter(POLICY_INFO);

            if (policyInfo != null) {
                setPageSessionAttribute(POLICY_INFO, policyInfo);
            }

            if (policyInfo == null) {
                throw new IllegalArgumentException("Policy Info not supplied");
            }
        }

        return policyInfo == null ? "" : policyInfo;
    }

    private int getMediaType() {
        // first check the page session
        String typeString = (String) getPageSessionAttribute(MEDIA_TYPE);

        // second check the request
        if (typeString == null) {
            typeString = RequestManager.getRequest().getParameter(MEDIA_TYPE);

            if (typeString != null) {
                setPageSessionAttribute(MEDIA_TYPE, typeString);
            }

            if (typeString == null) {
                throw new IllegalArgumentException("Media Type not supplied");
            }
        }

        return typeString == null ? -1 : Integer.parseInt(typeString);
    }

    private String getPolicyName() {
        String policyInfo = getPolicyInfo();
        if (policyInfo.length() == 0) {
            return "";
        }
        String [] infoArray = policyInfo.split("\\.");
        infoArray = infoArray == null ? new String[0] : infoArray;
        return infoArray.length < 2 ? "" : infoArray[0];
    }

    private int getCopyNumber() {
        String policyInfo = getPolicyInfo();
        if (policyInfo.length() == 0) {
            return -1;
        }
        String [] infoArray = policyInfo.split("\\.");
        infoArray = infoArray == null ? new String[0] : infoArray;
        return infoArray.length < 2 ? -1 : Integer.parseInt(infoArray[1]);
    }
}
