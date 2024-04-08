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

// ident	$Id: NewEditVSNPoolViewBean.java,v 1.29 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.media.AssignMediaView;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the New/Edit VSN Pool page.  This page
 *  has four different modes:
 *
 *  1. Create a new volume pool
 *  2. Edit an existing volume pool
 *  3. Create a new volume pool, and add the pool to the copy vsn
 *  4. Create an expression, and add the expression to the copy vsn
 */
public class NewEditVSNPoolViewBean extends CommonSecondaryViewBeanBase {
    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewEditVSNPool";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/archive/NewEditVSNPool.jsp";

    // pagelet view
    private static final String PAGELET_VIEW = "AssignMediaView";

    // Page Title Attributes and Components.
    private CCPageTitleModel ptModel = null;
    private static final String PAGE_TITLE = "PageTitle";

    // Request Param & Page Session Attributes
    // Used to determine if this pop up is using as new or editing a pool
    public static final String OPERATION = "OPERATION";
    public static final String NEW_POOL = "NEW";
    public static final
        String NEW_POOL_ADD_TO_COPYVSN = "NEW_POOL_ADD_TO_COPYVSN";
    public static final
        String NEW_EXP_ADD_TO_COPYVSN = "NEW_EXP_ADD_TO_COPYVSN";
    public static final String EDIT_POOL = "EDIT";
    public static final String EDIT_EXP = "EDIT_EXP";


    // page session attributes / request params
    public static final String POOL_NAME = "pool_name";
    public static final String EXPRESSION = "expression";
    public static final String MEDIA_TYPE = "media_type";
    public static final String POLICY_INFO = "policy_info";
    public static final String RESET_TYPE = "reset_type";

    /**
     * Constructor
     */
    public NewEditVSNPoolViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = createPageTitleModel();
        registerChildren();
        saveRequestParam();
        TraceUtil.trace3("Exiting");
    }


    /**
     * @Override
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(PAGE_TITLE, CCPageTitle.class);
        ptModel.registerChildren(this);
        registerChild(PAGELET_VIEW, AssignMediaView.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * @Override
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(PAGELET_VIEW)) {
            short mode = -1;
            if (getPageMode().equals(EDIT_POOL)) {
                mode = AssignMediaView.MODE_EDIT_POOL;
            } else if (getPageMode().equals(EDIT_EXP)) {
                mode = AssignMediaView.MODE_COPY_INFO_EDIT_EXP;
            } else if (getPageMode().equals(NEW_POOL_ADD_TO_COPYVSN)) {
                mode = AssignMediaView.MODE_COPY_INFO_NEW_POOL;
            } else if (getPageMode().equals(NEW_EXP_ADD_TO_COPYVSN)) {
                mode = AssignMediaView.MODE_COPY_INFO_NEW_EXP;
            } else {
                mode = AssignMediaView.MODE_NEW_POOL;
            }
            return new AssignMediaView(this, name, mode);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, ptModel, name);
        } else if (ptModel.isChildSupported(name)) {
            return ptModel.createChild(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else  {
            throw new IllegalArgumentException("Invalid child [" + name + "]");
        }
    }

    /**
     * @Override
     * @param event - Display Event
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        // Set the correct page title and page title help
        if (getPageMode().equals(NEW_POOL)) {
            // New Volume Pool
            ptModel.setPageTitleText("NewEditVSNPool.pageTitle.new");
            ptModel.setPageTitleHelpMessage(
                "NewEditVSNPool.pageTitle.new.help");
        } else if (getPageMode().equals(EDIT_POOL)) {
            // Edit Volume Pool
            String poolName = (String) getPageSessionAttribute(POOL_NAME);

            // Figure out if user is adding or editing an expression
            String expression = (String) getPageSessionAttribute(EXPRESSION);
            if (expression == null) {
                // adding new expression
                ptModel.setPageTitleText(
                    SamUtil.getResourceString(
                        "NewEditVSNPool.pageTitle.edit.newexpression",
                        new String [] {poolName}));
                ptModel.setPageTitleHelpMessage(
                    SamUtil.getResourceString(
                        "NewEditVSNPool.pageTitle.edit.newexpression.help",
                        new String [] {poolName}));
            } else {
                ptModel.setPageTitleText(
                    SamUtil.getResourceString(
                        "NewEditVSNPool.pageTitle.edit.oldexpression",
                        new String [] {expression, poolName}));
                ptModel.setPageTitleHelpMessage(
                    SamUtil.getResourceString(
                        "NewEditVSNPool.pageTitle.edit.oldexpression.help",
                        new String [] {expression, poolName}));
            }
        } else if (getPageMode().equals(EDIT_EXP)) {
            String expression = (String) getPageSessionAttribute(EXPRESSION);

            // Editing expression
            ptModel.setPageTitleText(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.edit.editexpression",
                    new String [] {expression}));
            ptModel.setPageTitleHelpMessage(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.edit.editexpression.help",
                    new String [] {expression}));
        } else if (getPageMode().equals(NEW_POOL_ADD_TO_COPYVSN)) {
            ptModel.setPageTitleText(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.new.addtocopyvsn",
                    new String [] {
                        getPolicyName(),
                        Integer.toString(getCopyNumber())}));
            ptModel.setPageTitleHelpMessage(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.new.addtocopyvsn.help",
                    new String [] {
                        getPolicyName(),
                        Integer.toString(getCopyNumber())}));
        } else if (getPageMode().equals(NEW_EXP_ADD_TO_COPYVSN)) {
            ptModel.setPageTitleText(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.newexp.addtocopyvsn",
                    new String [] {
                        getPolicyName(),
                        Integer.toString(getCopyNumber())}));
            ptModel.setPageTitleHelpMessage(
                SamUtil.getResourceString(
                    "NewEditVSNPool.pageTitle.newexp.addtocopyvsn.help",
                    new String [] {
                        getPolicyName(),
                        Integer.toString(getCopyNumber())}));
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler for the submit button
     * @param evt Request Invocation Event
     * @throws javax.servlet.ServletException
     * @throws java.io.IOException
     */
    public void handleSubmitRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {

        String serverName = getServerName();
        AssignMediaView view = (AssignMediaView) getChild(PAGELET_VIEW);
        String poolName = view.getPoolName();

        // run validation, refresh page to show error alert if there are any
        // user input errors.
        if (!view.validate(true)) {
            getParentViewBean().forwardTo(getRequestContext());
            return;
        }

        String successMessage = null, failedMessage = null;

        try {
            String expression = view.getExpression();
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemArchiveManager archiveManager =
                sysModel.getSamQFSSystemArchiveManager();

            TraceUtil.trace3("handleSubmit: getPageMode(): " + getPageMode());

            if (getPageMode().equals(NEW_POOL) ||
                getPageMode().equals(NEW_POOL_ADD_TO_COPYVSN)) {
                successMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.new.success", poolName);
                failedMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.new.failed", poolName);

                // catch the warning here because we still need to add the pool
                // to the copy vsn
                SamFSWarnings thisWarning = null;
                try {
                    int mediaType = view.getMediaType();
                    archiveManager.createVSNPool(
                        poolName, mediaType, expression);
                } catch (SamFSWarnings warningEx) {
                    thisWarning = warningEx;
                }

                if (getPageMode().equals(NEW_POOL_ADD_TO_COPYVSN)) {
                    // Add to Copy VSN
                    // Update success and fail message.  If the code reaches
                    // here, the pool is created successfully.
                    successMessage = SamUtil.getResourceString(
                        "CopyVSNs.addpool.success",
                        new String [] {
                            getPolicyName(),
                            Integer.toString(getCopyNumber())});
                    failedMessage = SamUtil.getResourceString(
                        "CopyVSNs.addpool.failed",
                        new String [] {
                            getPolicyName(),
                            Integer.toString(getCopyNumber())});

                    // Add to Copy VSN
                    ArchivePolicy thePolicy = getArchivePolicy(archiveManager);
                    ArchiveVSNMap vsnMap = getVSNMap(thePolicy);

                    String existingExpression = vsnMap.getPoolExpression();
                    existingExpression =
                        existingExpression == null ? "" : existingExpression;
                    String newExpression =
                        existingExpression.length() == 0 || isResetMediaType() ?
                                poolName :
                                existingExpression + "," + poolName;

                    TraceUtil.trace3("oldExpression: " + existingExpression);
                    TraceUtil.trace3("newExpression: " + newExpression);

                    // Update VSN Map and push the new settings to the new
                    // policy
                    vsnMap.setPoolExpression(newExpression);

                    // hard reset media type, purge the old expressions
                    if (isResetMediaType()) {
                        vsnMap.setArchiveMediaType(view.getMediaType());
                        vsnMap.setMapExpression("");
                    }
                    vsnMap.setWillBeSaved(true);

                    thePolicy.updatePolicy();

                    if (thisWarning != null) {
                        throw thisWarning;
                    }
                }
            } else if (getPageMode().equals(EDIT_POOL)) {
                successMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.edit.success", poolName);
                failedMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.edit.failed", poolName);
                VSNPool thePool = archiveManager.getVSNPool(poolName);
                if (thePool == null)
                    throw new SamFSException(null, -2010);

                // Add new expression to the existing expression list
                // or edit an existing expression (check page session attribute)
                String editExpression =
                    (String) getPageSessionAttribute(EXPRESSION);
                if (editExpression == null) {
                    // add new expression
                    TraceUtil.trace2("Adding new expression: " + expression +
                                     " to " + thePool.getVSNExpression());

                    thePool.setMemberVSNs(thePool.getMediaType(),
                        thePool.getVSNExpression() + "," + expression);
                } else {
                    // edit existing expression
                    String [] existingExpression =
                        thePool.getVSNExpression().split(",");
                    String [] newExpression =
                        new String[existingExpression.length];
                    for (int i = 0; i < existingExpression.length; i++) {
                        if (existingExpression[i].equals(editExpression)) {
                            newExpression[i] = expression;
                        } else {
                            newExpression[i] = existingExpression[i];
                        }
                    }
                    // Now create the new expression string
                    StringBuffer buf = new StringBuffer();
                    for (int i = 0; i < newExpression.length; i++) {
                        buf.append(newExpression[i]).append(",");
                    }
                    if (buf.length() != 0) {
                        buf.deleteCharAt(buf.length() - 1);
                    }

                    TraceUtil.trace2("Editing expression: " +
                        thePool.getVSNExpression() + " to " + buf.toString());

                    thePool.setMemberVSNs(
                        thePool.getMediaType(), buf.toString());
                }
            } else if (getPageMode().equals(EDIT_EXP)
                    || getPageMode().equals(NEW_EXP_ADD_TO_COPYVSN)) {
                String editExpression =
                    (String) getPageSessionAttribute(EXPRESSION);
                String policyName = getPolicyName();
                int copyNumber = getCopyNumber();

                TraceUtil.trace3("Policy Name: " + policyName);
                TraceUtil.trace3("Copy Number: " + copyNumber);

                ArchivePolicy thePolicy = getArchivePolicy(archiveManager);
                ArchiveVSNMap vsnMap = getVSNMap(thePolicy);

                String existingExpression = vsnMap.getMapExpression();
                existingExpression =
                    existingExpression == null ? "" : existingExpression;
                String newExpression = null;

                // Editing an expression
                if (getPageMode().equals(EDIT_EXP)) {
                    successMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.editexp.success", editExpression);
                    failedMessage = SamUtil.getResourceString(
                        "archiving.vsnpool.editexp.failed", editExpression);
                    newExpression =
                        replaceExpression(
                            existingExpression.split(","),
                            editExpression,
                            expression);
                // Adding an expression
                } else {
                    successMessage = SamUtil.getResourceString(
                        "CopyVSNs.addexp.success",
                        new String [] {
                            getPolicyName(),
                            Integer.toString(getCopyNumber())});
                    failedMessage = SamUtil.getResourceString(
                        "CopyVSNs.addexp.failed",
                        new String [] {
                            getPolicyName(),
                            Integer.toString(getCopyNumber())});

                    newExpression =
                        existingExpression.length() == 0 || isResetMediaType() ?
                            expression :
                            existingExpression + "," + expression;
                }

                TraceUtil.trace3("oldExpression: " + existingExpression);
                TraceUtil.trace3(
                    "Editing: " + editExpression + " to " + expression);
                TraceUtil.trace3("newExpression: " + newExpression);

                // Update VSN Map and push the new settings to the new policy
                vsnMap.setMapExpression(newExpression);

                // If user is resetting the media type, purge old expressions
                if (isResetMediaType()) {
                    vsnMap.setArchiveMediaType(view.getMediaType());
                    vsnMap.setPoolExpression("");
                }
                vsnMap.setWillBeSaved(true);

                thePolicy.updatePolicy();
            }
            SamUtil.setInfoAlert(
                this,
                ALERT,
                "success.summary",
                successMessage,
                serverName);
            setSubmitSuccessful(true);
        } catch (SamFSWarnings sfw) {
            SamUtil.setWarningAlert(
                this,
                ALERT,
                "ArchiveConfig.warning.summary",
                "ArchiveConfig.warning.detail");
            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                getClass(),
                "handleSubmitRequest",
                "Error validating user input",
                serverName);
            SamUtil.setErrorAlert(
                this,
                ALERT,
                failedMessage,
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }

        forwardTo(getRequestContext());
    }

    /**
     * safety method to prevent users from seeing a stack trace should the
     * javascript malfunction.
     *
     * @param evt
     * @throws ServletException
     * @throws IOException
     */
    public void handleCancelRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        forwardTo(getRequestContext());
    }

    private CCPageTitleModel createPageTitleModel() {
        return new CCPageTitleModel(
           RequestManager.getRequestContext().getServletContext(),
           "/jsp/archive/NewEditVSNPoolPageTitle.xml");
    }

    private String replaceExpression(
        String [] existingArray, String expression, String newExpression) {
        StringBuffer newExpressionBuf = new StringBuffer();
        for (int i = 0; i < existingArray.length; i++) {
            if (existingArray[i].equals(expression)) {
                newExpressionBuf.append(newExpression).append(",");
            } else {
                newExpressionBuf.append(existingArray[i]).append(",");
            }
        }
        return newExpressionBuf.substring(
                        0, newExpressionBuf.length() - 1).toString();
    }

    /**
     * Determine if we are creating a new vsn pool or editing an existing one
     * @return boolean - true if creating a new pool else false
     */
    protected String getPageMode() {
        String operation = (String)getPageSessionAttribute(OPERATION);

        if (operation == null) {
            operation = RequestManager.getRequest().getParameter(OPERATION);
            setPageSessionAttribute(OPERATION, operation);
        }

        return operation;
    }

    private void saveRequestParam() {
        String poolName = (String) getPageSessionAttribute(POOL_NAME);
        if (poolName == null) {
            poolName = RequestManager.getRequest().getParameter(POOL_NAME);
            setPageSessionAttribute(POOL_NAME, poolName);
        }
        String mediaType = (String) getPageSessionAttribute(MEDIA_TYPE);
        if (mediaType == null) {
            mediaType = RequestManager.getRequest().getParameter(MEDIA_TYPE);
            setPageSessionAttribute(MEDIA_TYPE, mediaType);
        }
        String expression = (String) getPageSessionAttribute(EXPRESSION);
        if (expression == null) {
            expression = RequestManager.getRequest().getParameter(EXPRESSION);
            setPageSessionAttribute(EXPRESSION, expression);
        }

        // In the format of <policy_name.copy_number>
        String policyInfo = (String) getPageSessionAttribute(POLICY_INFO);
        if (policyInfo == null) {
            policyInfo = RequestManager.getRequest().getParameter(POLICY_INFO);
            setPageSessionAttribute(POLICY_INFO, policyInfo);
        }
    }

    private ArchivePolicy getArchivePolicy(
        SamQFSSystemArchiveManager archiveManager) throws SamFSException {
        ArchivePolicy thePolicy =
            archiveManager.getArchivePolicy(getPolicyName());
        // make sure the policy wasn't deleted in the process
        if (thePolicy == null) {
            throw new SamFSException(null, -2000);
        }
        return thePolicy;
    }

    private ArchiveVSNMap getVSNMap(ArchivePolicy thePolicy)
        throws SamFSException {
        ArchiveCopy theCopy = thePolicy.getArchiveCopy(getCopyNumber());
        if (theCopy == null) {
            throw new SamFSException(null, -2006);
        }
        return theCopy.getArchiveVSNMap();
    }

    private int getCopyNumber() {
        String [] policyInfoArray = getPolicyInfoArray();
        return Integer.parseInt(policyInfoArray[1]);
    }

    private String getPolicyName() {
        String [] policyInfoArray = getPolicyInfoArray();
        return policyInfoArray[0];
    }

    private String [] getPolicyInfoArray() {
        String policyInfo = (String) getPageSessionAttribute(POLICY_INFO);
        if (policyInfo == null || policyInfo.length() == 0) {
            return new String[0];
        } else {
            return policyInfo.split("\\.");
        }
    }

    private boolean isResetMediaType() {
        String resetType = (String) getPageSessionAttribute(RESET_TYPE);
        if (resetType == null || resetType.length() == 0) {
            return false;
        } else {
            return Boolean.valueOf(resetType).booleanValue();
        }
    }
}
