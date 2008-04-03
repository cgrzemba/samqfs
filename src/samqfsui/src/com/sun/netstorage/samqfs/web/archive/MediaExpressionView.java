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

// ident	$Id: MediaExpressionView.java,v 1.1 2008/04/03 02:21:39 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.media.MediaUtil;
import com.sun.netstorage.samqfs.web.media.VSNDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.media.DiskVolume;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.model.media.VSNWrapper;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.util.LogUtil;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Creates the ArchivePolSummary Action Table and provides
 * handlers for the links within the table.
 */
public class MediaExpressionView extends CommonTableContainerView {

    public static final String TILED_VIEW = "MediaExpressionTiledView";
    private static final String RESERVED_VSN_MESSAGE = "reservedVSNMessage";
    private static final String DELETE_CONFIRMATION = "deleteConfirmation";
    private static final String SELECTED_EXPRESSION = "SelectedExpression";
    private static final String SELECTED_POOL = "SelectedPool";
    private static final String NO_SELECTION_MSG = "NoSelectionMsg";
    private static final String NO_PERMISSION_MSG = "NoPermissionMsg";
    private static final
        String DELETE_POOL_CONFIRMATION = "deletePoolConfirmation";
    private static final String SERVER_NAME = "ServerName";
    public static final String HAS_PERMISSION = "hasPermission";
    private static final String CHILD_POOL_NAME = "VSNPoolNameField";
    private static final String HIDDEN_MEDIA_TYPE = "MediaType";
    private static final String HIDDEN_IS_POOL = "poolBoolean";
    public static final String HIDDEN_EXPRESSIONS = "Expressions";

    private CCActionTableModel model = null;
    private static final String BUTTON_ADD = "ButtonAdd";

    // Maximum number of volumes shown in the matching volume column
    private static final int MAX_VOL_SHOWN = 5;

    // identify if at least one volume is reserved in the matching volumes
    // column
    private boolean reserved = false;

    // boolean to indicate which page is currently using this view
    private boolean useInPoolDetails = false;


    /**
     * Constructor
     * @param parent
     * @param name
     * @param useInPoolDetails - boolean to indicate which page is currently
     *                           using this view
     */
    public MediaExpressionView(
        View parent, String name, boolean useInPoolDetails) {
        super(parent, name);
        this.useInPoolDetails = useInPoolDetails;

        TraceUtil.initTrace();
        CHILD_ACTION_TABLE = "MediaExpressionTable";

        createTableModel();
        registerChildren();
    }

    /**
     * Register page children
     */
    public void registerChildren() {
        super.registerChildren(model);
        registerChild(TILED_VIEW, MediaExpressionTiledView.class);
        registerChild(RESERVED_VSN_MESSAGE, CCStaticTextField.class);
        registerChild(CHILD_POOL_NAME, CCHiddenField.class);
        registerChild(HIDDEN_IS_POOL, CCHiddenField.class);
        registerChild(HIDDEN_MEDIA_TYPE, CCHiddenField.class);
        registerChild(HIDDEN_EXPRESSIONS, CCHiddenField.class);
        registerChild(DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(SELECTED_EXPRESSION, CCHiddenField.class);
        registerChild(SELECTED_POOL, CCHiddenField.class);
        registerChild(HAS_PERMISSION, CCHiddenField.class);
        registerChild(NO_SELECTION_MSG, CCHiddenField.class);
        registerChild(NO_PERMISSION_MSG, CCHiddenField.class);
        registerChild(DELETE_POOL_CONFIRMATION, CCHiddenField.class);
    }

    public View createChild(String name) {
        if (name.equals(TILED_VIEW)) {
            return new MediaExpressionTiledView(this, model, name);
        } else if (name.equals(RESERVED_VSN_MESSAGE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_POOL_NAME)
                   || name.equals(HIDDEN_MEDIA_TYPE)
                   || name.equals(HIDDEN_EXPRESSIONS)
                   || name.equals(HIDDEN_IS_POOL)
                   || name.equals(DELETE_CONFIRMATION)
                   || name.equals(SERVER_NAME)
                   || name.equals(SELECTED_EXPRESSION)
                   || name.equals(SELECTED_POOL)
                   || name.equals(HAS_PERMISSION)
                   || name.equals(NO_SELECTION_MSG)
                   || name.equals(DELETE_POOL_CONFIRMATION)
                   || name.equals(NO_PERMISSION_MSG)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(model, name, TILED_VIEW);
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        initTableHeaders();
        populateTable();
        populateDaggerMessage();
        populateHiddenFields();
    }

    private void createTableModel() {
        model = new CCActionTableModel(
           RequestManager.getRequestContext().getServletContext(),
           "/jsp/archive/MediaAssignmentTable.xml");
    }

    private void initTableHeaders() {
        model.setTitle(
            useInPoolDetails ?
                "MediaAssignment.tabletitle.copyvsn" :
                SamUtil.getResourceString(
                    "MediaAssignment.heading.expression.copyvsn",
                    new String [] {
                        (String) getParentViewBean().getPageSessionAttribute(
                            Constants.Archive.POLICY_NAME),
                        ((Integer) getParentViewBean().getPageSessionAttribute(
                            Constants.Archive.COPY_NUMBER)).toString()}));
        // set column headers
        model.setActionValue(
            "ColumnExpression",
            useInPoolDetails ?
                "MediaAssignment.heading.expression" :
                "MediaAssignment.heading.expression.copyvsn");
        model.setActionValue(
            "ColumnFreeSpace", "MediaAssignment.heading.freespace");
        model.setActionValue(
            "ColumnMatchingVolume", "MediaAssignment.heading.matchingvolume");
    }

    private boolean hasPermission() {
    	if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            TraceUtil.trace2(
                "User has " + Authorization.MEDIA_OPERATOR + " permission.");
            return true;
        } else {
            return false;
        }
    }

    private void populateDaggerMessage() {
        CCStaticTextField child =
                (CCStaticTextField)getChild(RESERVED_VSN_MESSAGE);
        if (reserved) {
            String msg =
                SamUtil.getResourceString("archiving.reservedvsninpool",
                                          Constants.Symbol.DAGGER);
            child.setValue(msg);
        } else {
            child.setValue("");
        }
    }

    private void populateTable() {
        // Disable table buttons if user does not have permission to modify
        // settings here
        boolean permission = hasPermission();
        ((CCButton) getChild(BUTTON_ADD)).setDisabled(!permission);
        ((CCHiddenField) getChild(HAS_PERMISSION)).setValue(
            Boolean.toString(permission));

        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        String serverName = parent.getServerName();

        // Buffer to build the matching volumes string
        StringBuffer buf = new StringBuffer();
        StringBuffer isPoolBuf = new StringBuffer();

        try {
            // TODO: Disabling selection tooltips


            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            String [] expressions = null;
            int mediaType = -1;

            if (useInPoolDetails) {
                VSNPool pool = ((VSNPoolDetailsViewBean) parent).getVSNPool();
                mediaType = pool.getMediaType();
                expressions =
                    pool.getVSNExpression() == null ||
                    pool.getVSNExpression().length() == 0 ?
                        new String[0] :
                        pool.getVSNExpression().split(",");
            } else {
                ArchiveVSNMap vsnMap = ((CopyVSNsViewBean) parent).getVSNMap();
                mediaType = vsnMap.getArchiveMediaType();
                expressions =
                    vsnMap.getMapExpression() == null ||
                    vsnMap.getMapExpression().length() == 0 ?
                        new String[0] :
                        vsnMap.getMapExpression().split(",");
            }

            // set hidden media type field
            ((CCHiddenField) getChild(HIDDEN_MEDIA_TYPE)).setValue(mediaType);

            // clear the model
            model.clear();

            for (int i = 0; i < expressions.length; i++) {
                if (i > 0) {
                    model.appendRow();
                }
                model.setValue("TextExpression", expressions[i]);
                model.setValue("TextExpressionHref", "");
                buf.append(expressions[i]).append(",");
                isPoolBuf.append("false,");

                // Retrieve free space and matching volume information
                VSNWrapper wrapper =
                    sysModel.getSamQFSSystemMediaManager().
                        evaluateVSNExpression(
                            mediaType, null, -1,
                            null, null, expressions[i],
                            null,
                            SamQFSSystemMediaManager.MAXIMUM_ENTRIES_FETCHED);
                long freeSpace = wrapper.getFreeSpaceInMB();
                model.setValue(
                    "TextFreeSpace",
                    new Capacity(freeSpace, SamQFSSystemModel.SIZE_MB));
                model.setValue(
                    "HiddenFreeSpace",
                    new Long(freeSpace));

                String volString =
                    createVolumeString(wrapper, mediaType);
                model.setValue(
                    "TextMatchingVolume",
                    volString.endsWith(")") ?
                        "" : volString);
                model.setValue(
                    "TextMatchingVolumeWithLink",
                    volString.endsWith(")") ?
                        volString : "");
            }

            // if this view is used by the copy vsn page, we need to show the
            // participating volume pools in the table
            if (!useInPoolDetails) {
                ArchiveVSNMap vsnMap = ((CopyVSNsViewBean) parent).getVSNMap();
                expressions =
                    vsnMap.getPoolExpression() == null ||
                    vsnMap.getPoolExpression().length() == 0 ?
                        expressions = new String[0] :
                        vsnMap.getPoolExpression().split(",");

                TraceUtil.trace3(
                    "Pool Expression: " + vsnMap.getPoolExpression());
                TraceUtil.trace3("Number of pools: " + expressions.length);

                for (int i = 0; i < expressions.length; i++) {
                    if (buf.length() > 0) {
                        model.appendRow();
                    }
                    model.setValue("TextExpression", expressions[i]);
                    model.setValue("TextExpressionHref", expressions[i]);
                    buf.append(expressions[i]).append(",");
                    isPoolBuf.append("true,");

                    // Retrieve free space and matching volume information
                    VSNWrapper wrapper =
                        sysModel.getSamQFSSystemMediaManager().
                            evaluateVSNExpression(
                                mediaType, null, -1,
                                null, null, null,
                                expressions[i],
                                SamQFSSystemMediaManager.
                                    MAXIMUM_ENTRIES_FETCHED);
                    long freeSpace = wrapper.getFreeSpaceInMB();
                    model.setValue(
                        "TextFreeSpace",
                        new Capacity(freeSpace, SamQFSSystemModel.SIZE_MB));
                    model.setValue(
                        "HiddenFreeSpace",
                        new Long(freeSpace));

                    String volString =
                        createVolumeString(wrapper, mediaType);
                    model.setValue(
                        "TextMatchingVolume",
                        volString.endsWith(")") ?
                            "" : volString);
                    model.setValue(
                        "TextMatchingVolumeWithLink",
                        volString.endsWith(")") ?
                            volString : "");
                }
            }

            ((CCHiddenField) getChild(HIDDEN_EXPRESSIONS)).setValue(
                buf.length() == 0 ?
                    "" :
                    buf.deleteCharAt(buf.length() - 1).toString());
            ((CCHiddenField) getChild(HIDDEN_IS_POOL)).setValue(
                isPoolBuf.length() == 0 ?
                    "" :
                    isPoolBuf.deleteCharAt(isPoolBuf.length() - 1).toString());

        } catch (SamFSWarnings sfw) {
            SamUtil.processException(
                sfw,
                getClass(),
                "populateTableModel",
                "Unable to retrieve VSN pools",
                serverName);
            SamUtil.setWarningAlert(
                parent,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error.summary",
                "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(
                smme,
                getClass(),
                "populate Table Model",
                "Unable to retrieve VSN Pools",
                serverName);
            SamUtil.setErrorAlert(
                parent,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error.summary",
                smme.getSAMerrno(),
                "ArchiveConfig.error.detail",
                serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                getClass(),
                "populateTableModel",
                "Unable to retrieve VSN Pools",
                serverName);
            SamUtil.setErrorAlert(
                parent,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error.summary",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }

        // Disable the Delete button if there is no entry in the table
        try {
            ((CCButton) getChild("ButtonDelete")).
                        setDisabled(model.getSize() == 0);
        } catch (ModelControlException modelEx) {
            TraceUtil.trace1("Failed to disable the delete button!");
            TraceUtil.trace1(modelEx.getMessage());
        }
    }

    public void handleButtonDeleteRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        String serverName = (String) parent.getServerName();

        if (useInPoolDetails) {
            handlePoolDetailsDelete(parent, serverName);
        } else {
            handleCopyVSNsDelete(parent, serverName);
        }
    }

    private void handleCopyVSNsDelete(
        CommonViewBeanBase parent, String serverName) {
        String selectedExpression =
            (String) getDisplayFieldValue(SELECTED_EXPRESSION);
        String selectedPool =
            (String) getDisplayFieldValue(SELECTED_POOL);
        String policyName = (String) parent.getPageSessionAttribute(
                                        Constants.Archive.POLICY_NAME);
        Integer copyNumber = (Integer) parent.getPageSessionAttribute(
                                        Constants.Archive.COPY_NUMBER);

        selectedPool = selectedPool == null ? "" : selectedPool;
        selectedExpression =
            selectedExpression == null ? "" : selectedExpression;
System.out.println("selected pool: " + selectedPool);
System.out.println("selectedExpression: " + selectedExpression);
        TraceUtil.trace3("Delete button clicked in Copy VSNs page table.");
        TraceUtil.trace3(
            "Working on: " +  policyName + "." + copyNumber.toString());
        TraceUtil.trace3("SelectedPool is " + selectedPool);
        TraceUtil.trace3("SelectedExpression is " + selectedExpression);

        try {
            ArchivePolicy thePolicy = ((CopyVSNsViewBean) parent).getPolicy();
            ArchiveVSNMap vsnMap = ((CopyVSNsViewBean) parent).getVSNMap();
            String expressions = vsnMap.getMapExpression();
            String poolExpressions = vsnMap.getPoolExpression();
            expressions = expressions == null ? "" : expressions;
            poolExpressions = poolExpressions == null ? "" : poolExpressions;
System.out.println("existing expressions: " + expressions);
System.out.println("existing poolExpressions: " + poolExpressions);

            String newExpression = removeSelectedFieldsFromString(
                expressions.split(","), selectedExpression.split(","));
            String newPoolExpressions = removeSelectedFieldsFromString(
                poolExpressions.split(","), selectedPool.split(","));
System.out.println("after expressions: " + newExpression);
System.out.println("after poolExpressions: " + newPoolExpressions);

            // Update VSN Map and push the new settings to the new policy
            vsnMap.setMapExpression(
                newExpression.length() == 0 ? "" : newExpression);
            vsnMap.setPoolExpression(
                newPoolExpressions.length() == 0 ? null : newPoolExpressions);
            vsnMap.setWillBeSaved(true);

            thePolicy.updatePolicy();

        } catch (SamFSWarnings warnEx) {
            TraceUtil.trace1("SamFSWarning caught!", warnEx);
            SamUtil.setWarningAlert(
                parent,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error.summary",
                warnEx.getMessage());
        } catch (SamFSException ex) {
            TraceUtil.trace1("SamFSException caught!", ex);
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleButtonDeleteRequest",
                ex.getMessage(),
                serverName);

            // Setting error alerts
            SamUtil.setErrorAlert(
                    parent,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    serverName);
        }

        getParentViewBean().forwardTo(getRequestContext());
    }

    private void handlePoolDetailsDelete(
        CommonViewBeanBase parent, String serverName) {
        CommonViewBeanBase summaryVB =
            (CommonViewBeanBase) getViewBean(VSNPoolSummaryViewBean.class);
        CommonViewBeanBase target = null;
        String poolName = (String)
            parent.getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);
        String selectedExpression =
            (String) getDisplayFieldValue(SELECTED_EXPRESSION);

        TraceUtil.trace2("SelectedExpression is " + selectedExpression);

        String [] selectedArray = selectedExpression.split(",");

        // boolean to indicate if user is deleting the volume pool (when all
        // expressions are selected)
        boolean deletePool = false;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            SamQFSSystemArchiveManager archiveManager =
                sysModel.getSamQFSSystemArchiveManager();
            VSNPool thePool = archiveManager.getVSNPool(poolName);

            TraceUtil.trace2("" +
                "EXISTING EXPRESSION: " + thePool.getVSNExpression());

            String [] existingArray =
                thePool.getVSNExpression() == null ?
                    new String[0] :
                    thePool.getVSNExpression().split(",");

            // If user selects all entries in the table, the volume pool will
            // be removed.  This function will be called after user has been
            // warned by the javascript alert.
            if (existingArray.length == selectedArray.length) {
                deletePool = true;

                TraceUtil.trace2(
                    "All expressions selected.  Delete volume pool!");

                LogUtil.info(this.getClass(), "handleButtonDeleteRequest",
                    "Start deleting volume pool " + poolName);
                archiveManager.deleteVSNPool(poolName);
                LogUtil.info(this.getClass(), "handleButtonDeleteRequest",
                    "Done deleting volume pool " + poolName);

                target = summaryVB;
            } else {
                // User is deleting existing expressions.  Form a new
                // expression and update the pool
                String newExpression =
                    removeSelectedFieldsFromString(
                        existingArray, selectedArray);

                TraceUtil.trace3("New Expression: " + newExpression);

                LogUtil.info(
                    this.getClass(),
                    "handleButtonDeleteRequest",
                    "Start setting volume expression of volume pool " +
                        poolName);
                thePool.setMemberVSNs(thePool.getMediaType(), newExpression);
                LogUtil.info(
                    this.getClass(),
                    "handleButtonDeleteRequest",
                    "Done setting volume expression of volume pool " +
                        poolName);

                target = parent;
            }
        } catch (SamFSWarnings warnEx) {
            target = deletePool ? summaryVB : parent;
            TraceUtil.trace1("SamFSWarning caught!", warnEx);
            SamUtil.setWarningAlert(
                target,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error.summary",
                warnEx.getMessage());
        } catch (SamFSMultiMsgException multiEx) {
            target = parent;
            TraceUtil.trace1("SamFSMultiMsgException caught!", multiEx);
            SamUtil.processException(
                multiEx,
                this.getClass(),
                "handleButtonDeleteRequest",
                Constants.Config.ARCHIVE_CONFIG,
                serverName);
            SamUtil.setErrorAlert(
                target,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveConfig.error",
                multiEx.getSAMerrno(),
                multiEx.getMessage(),
                serverName);
            getParentViewBean().forwardTo(getRequestContext());
            return;
        } catch (SamFSException ex) {
            target = parent;
            TraceUtil.trace1("SamFSException caught!", ex);
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleButtonDeleteRequest",
                ex.getMessage(),
                serverName);

            // Setting error alerts
            String errorMessage = null;
            if (deletePool) {
                errorMessage = SamUtil.getResourceString(
                            "VSNPoolSummary.error.failedDelete", poolName);
            } else {
                errorMessage = SamUtil.getResourceString(
                    "MediaAssignment.error.delete.expression.pool", poolName);
            }
            SamUtil.setErrorAlert(
                    target,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    errorMessage,
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    serverName);
            getParentViewBean().forwardTo(getRequestContext());
            return;
        }

        if (deletePool) {
            target = summaryVB;

            // sleep for 5 seconds to allow sam-amld to start
            try {
                Thread.sleep(5000);
            } catch (InterruptedException iex) {
                TraceUtil.trace3("InterruptedException Caught: Reason: " +
                                 iex.getMessage());
            }

            SamUtil.setInfoAlert(
                target,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "VSNPoolSummary.delete.alert", poolName),
                "",
                serverName);

            if (target == summaryVB) {
                parent.removePageSessionAttribute(
                    Constants.SessionAttributes.PAGE_PATH);
            }
            parent.forwardTo(target);
        } else {
            target = parent;
            SamUtil.setInfoAlert(
                target,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "MediaAssignment.delete.expression", poolName),
                "",
                serverName);
            parent.forwardTo(getRequestContext());
        }
    }

    private String createVolumeString(VSNWrapper wrapper, int mediaType)
        throws SamFSException {
        StringBuffer buf = new StringBuffer();

        // boolean to identify when not all the volumes are shown in the column
        boolean moreThanMax = false;

        if (MediaUtil.isDiskType(mediaType)) {
            DiskVolume [] vsns = wrapper.getAllDiskVSNs();
            for (int i = 0; i < MAX_VOL_SHOWN && i < vsns.length; i++) {
                buf.append(vsns[i].getName());
                buf.append(", ");
            }
            moreThanMax = vsns.length > MAX_VOL_SHOWN;
        } else {
            VSN [] vsns = wrapper.getAllTapeVSNs();
            for (int i = 0; i < MAX_VOL_SHOWN && i < vsns.length; i++) {
                buf.append(vsns[i].getVSN());
                if (vsns[i].isReserved()) {
                    reserved = true;
                    buf.append(Constants.Symbol.DAGGER);
                }
                buf.append(", ");
            }
            moreThanMax = vsns.length > MAX_VOL_SHOWN;
        }

        // remove trailing comma & space
        if (buf.length() > 0) {
            buf = new StringBuffer(buf.substring(0, buf.length() - 2));
        }

        if (moreThanMax) {
            // add ... with total number of volumes parenthesis
            buf.append(" ... (").
                append(wrapper.getTotalNumberOfVSNs()).
                append(")");
        }

        return buf.toString();
    }

    /**
     * Given two arrays of String objects.  Remove all elements in Array B
     * from Array A if they exist.  Return Array A afterwards.
     * @param existingArray
     * @param selectedArray
     * @return
     */
    private String removeSelectedFieldsFromString(
        String [] existingArray, String [] selectedArray) {
        StringBuffer newExpressionBuf = new StringBuffer();
        boolean match = false;
        for (int i = 0; i < existingArray.length; i++) {
            for (int j = 0; j < selectedArray.length; j++) {
                if (existingArray[i].equals(selectedArray[j])) {
                    match = true;
                    break;
                }
            }

            if (!match) {
                newExpressionBuf.append(existingArray[i]).append(",");
            } else {
                // reset flag
                match = false;
            }
        }
        if (newExpressionBuf.length() > 0) {
            return newExpressionBuf.substring(
                        0, newExpressionBuf.length() - 1).toString();
        } else {
            return "";
        }
    }

    private void populateHiddenFields() {
        CommonViewBeanBase parent = (CommonViewBeanBase) getParentViewBean();
        String poolName = (String) parent.
            getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);
        String serverName = parent.getServerName();

        ((CCHiddenField)getChild(SERVER_NAME)).setValue(serverName);
        ((CCHiddenField) getChild(CHILD_POOL_NAME)).setValue(poolName);

        // set delete confirmation msg
        ((CCHiddenField)getChild(DELETE_CONFIRMATION)).setValue(
            useInPoolDetails ?
                SamUtil.getResourceString(
                    "MediaAssignment.delete.confirm.pool") :
                SamUtil.getResourceString(
                    "MediaAssignment.delete.confirm.copyvsn"));
        ((CCHiddenField)getChild(NO_SELECTION_MSG)).setValue(
            SamUtil.getResourceString("common.noneselected"));
        ((CCHiddenField)getChild(DELETE_POOL_CONFIRMATION)).setValue(
            useInPoolDetails ?
                SamUtil.getResourceString(
                    "MediaAssignment.deletepool.confirm.pool") :
                SamUtil.getResourceString(
                    "MediaAssignment.deletepool.confirm.copyvsn"));
        ((CCHiddenField)getChild(NO_PERMISSION_MSG)).setValue(
            useInPoolDetails ?
                SamUtil.getResourceString(
                    "MediaAssignment.deletepool.confirm.pool") :
                SamUtil.getResourceString(
                    "MediaAssignment.deletepool.confirm.copyvsn"));
    }

    /**
     * Handle breadcrumb link back to VSN Details Page
     * @param event
     * @throws javax.servlet.ServletException
     * @throws java.io.IOException
     */
    public void handleVSNHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        String s = (String) getDisplayFieldValue("VSNHref");

        // Convert value into an array
        String [] tokenArray = s.split(";");

        // Set library name and slot number attributes before forwarding
        // page back to the VSN Details page
        source.setPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME, tokenArray[0]);

        source.setPageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER, tokenArray[1]);

        BreadCrumbUtil.breadCrumbPathForward(source,
        PageInfo.getPageInfo().getPageNumber(source.getName()));

        source.forwardTo(
            (CommonViewBeanBase)getViewBean(VSNDetailsViewBean.class));
    }
}
