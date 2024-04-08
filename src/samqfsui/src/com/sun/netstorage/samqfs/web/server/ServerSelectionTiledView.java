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

// ident	$Id: ServerSelectionTiledView.java,v 1.40 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.event.TiledViewRequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.ErrorHandleViewBean;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import java.io.IOException;
import javax.servlet.ServletException;



/**
 * The ContainerView used to register and create children for "column"
 * elements defined in the model's XML document.
 *
 * This ContainerView demonstrates how to evoke JATO's TiledView
 * behavior, which automatically appends the current model row index
 * to each child's qualified name. This approach can be useful when
 * retrieving values from rows of text fields, for example.
 *
 * Note that "column" element children must be in a separate
 * ContainerView when implementing JATO's TiledView. Otherwise,
 * HREFs and buttons used for table actions will be incorrectly
 * indexed as if they were also tiled. This results in JATO
 * throwing RuntimeException due to malformed qualified field
 * names because the tile index is not found during a request
 * event.
 *
 * That said, a separate ContainerView object is not necessary
 * when TiledView behavior is not required. All children can be
 * registered and created in the same ContainerView. By default,
 * CCActionTable will attempt to retrieve all children from the
 * ContainerView given to the CCActionTable.setContainerView method.
 *
 */

/**
 * This is the TiledView class for Server Selection Page. This is used to
 * identify if an alarm HREF link is needed. Also this tiledview class is needed
 * to avoid concurrency issue.
 */

public class ServerSelectionTiledView extends CommonTiledViewBase {
    // Child view names (i.e. display fields).
    public static final String CHILD_ALARM_HREF = "AlarmHref";
    public static final String CHILD_SERVER_HREF = "ServerHref";
    public static final String CHILD_VERSION_HREF = "VersionHref";
    public static final String CHILD_DISK_CACHE_HREF = "DiskCacheHref";
    public static final String CHILD_MEDIA_CAPACITY_HREF = "MediaCapacityHref";
    public static final String HIDDEN_INFO = "HiddenInfo";

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public ServerSelectionTiledView(
        View parent, CCActionTableModel model, String name) {
        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Request Handlers
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    public void handleServerHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        handleForward(event, 0);
        TraceUtil.trace3("Exiting");
    }

    public void handleAlarmHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        handleForward(event, 1);
        TraceUtil.trace3("Exiting");
    }

    public void handleVersionHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(VersionHighlightViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        targetView.setPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH,
            (Integer []) getParentViewBean().getPageSessionAttribute(
                Constants.SessionAttributes.PAGE_PATH));
        targetView.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void handleForward(
        RequestInvocationEvent event, int choice)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        // If code reaches here, it means the connection to the server is down.
        // Otherwise, the javscript should create an url and forward the page
        // to the three-frame mode.
        model.setRowIndex(((TiledViewRequestInvocationEvent)
            event).getTileNumber());
        String str = null;

        // Get the server name from the HREF
        switch (choice) {
            // Server
            case 0:
                str = (String) getDisplayFieldValue(CHILD_SERVER_HREF);
                break;

            // Faults
            case 1:
                str = (String) getDisplayFieldValue(CHILD_ALARM_HREF);
                break;
        }

        TraceUtil.trace3("handleForward: str is: " + str);

        // When the server is down, we try to reconnect the server.
        // Then check the server to see if it's still down,
        // if yes, go to Error Page
        // otherwise, refresh server selection page with the latest
        // server status
        String [] tmpArray = str.split(ServerUtil.delimitor);
        switch (Integer.parseInt(tmpArray[1])) {
            case ServerUtil.ALARM_DOWN:
                executeDownRoutine(tmpArray[0]);
                break;

            case ServerUtil.ALARM_ACCESS_DENIED:
                executeAccessDeniedRoutine(tmpArray[0]);
                break;

            case ServerUtil.ALARM_NOT_SUPPORTED:
                executeNotSupportedRoutine(tmpArray[0]);
                break;

            default:
                // Should not reach here
                getParentViewBean().forwardTo(getRequestContext());
                break;
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * This method is used only in the Server Selection page.  Do not use the
     * one in SamUtil because we do not need to check if the server is marked
     * as DOWN or whatsoever.  Server Selection Page has its own logic on what
     * to do in any server state.
     */
    private SamQFSSystemModel getSystemModel(String str)
        throws ModelControlException {

        SamQFSSystemModel sysModel = null;
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            sysModel = appModel.getSamQFSSystemModel(str);
        } catch (SamFSException samEx) {
            // Wrap it up to ModelControlException
            TraceUtil.trace1(new NonSyncStringBuffer(
                "Exception occurred: ").append(samEx.getMessage()).
                toString());
            LogUtil.error(getClass(), "handleForward",
                samEx.getMessage());
            throw new ModelControlException(
                "Fatal Error occurred in handleForward(). " +
                "Reason: " + samEx.getMessage());
        }

        return sysModel;
    }

    /**
     * This method will be called explicitly when the DOWN icon is shown
     * next to the server name
     */
    private void executeDownRoutine(String serverName)
        throws ModelControlException {

        TraceUtil.trace3("Entering executeDownRoutine!");

        boolean hasError = false;
        ViewBean targetView = null;
        SamQFSSystemModel sysModel = getSystemModel(serverName);

        try {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "Server is marked as DOWN already in the Server Selection ")
                .append(" page. Attempt to reconnect to ")
                .append(serverName).append("...").toString());

            sysModel.reconnect();
            // now after reconnect, check if the machine is STILL down

            hasError = sysModel.isDown();

        } catch (Exception e) {
            // Double protection, if exception is thrown, go to error page
            hasError = true;
        }


        // if no exception is thrown or the machine is NOT down,
        // refresh Server Selection Page
        // Otherwise, go to Error ViewBean
        if (!hasError) {
            TraceUtil.trace1("hasError: " + hasError);
            targetView = getParentViewBean();
        } else {
            targetView = getViewBean(ErrorHandleViewBean.class);
            String errorMsgs = null;
            String errorDetails = null;

            SamUtil.doPrint(new NonSyncStringBuffer(
                "Version Status: ").append(sysModel.getVersionStatus()).
                toString());

            // Now the server is marked as DOWN.  But why?
            // Check the version number now.  If the version status
            // is VERSION_SAME, this means version of RPC library is
            // ok.  The only known reason of which the server is down
            // is because the management station cannot communicate
            // with the server.

            switch (sysModel.getVersionStatus()) {
                // If the version status is the same, the RPC library version
                // is ok.  The only known reason of which the server is down
                // is because the management station cannot communicate
                // with the server.
                case SamQFSSystemModel.VERSION_SAME:
                    errorMsgs =
                        SamUtil.getResourceString("ErrorHandle.alertElement2");
                    errorDetails = SamUtil.getServerDownMessage();
                    break;

                // If the RPC library is either newer or older, show the
                // appropriate messages
                case SamQFSSystemModel.VERSION_CLIENT_NEWER:
                    errorMsgs = SamUtil.getResourceString(
                        "ErrorHandle.alertElement3");
                    errorDetails = SamUtil.getResourceString(
                        "ErrorHandle.alertElementFailedDetail3");
                    break;

                default:
                    errorMsgs = SamUtil.getResourceString(
                        "ErrorHandle.alertElement3");
                    errorDetails = SamUtil.getResourceString(
                        "ErrorHandle.alertElementFailedDetail4");
                    break;

            }

            CCAlertInline alert = (CCAlertInline)
                targetView.getChild(ErrorHandleViewBean.CHILD_ALERT);
            alert.setSummary(errorMsgs);
            alert.setDetail(ServerUtil.lineBreak + errorDetails,
                new String[] { serverName });
        }

        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
        targetView.forwardTo(getRequestContext());
        return;
    }

    /**
     * This method is called explicitly when the ACCESS DENIED message is
     * shown next to the server name
     */
    private void executeAccessDeniedRoutine(String serverName)
        throws ModelControlException {
        ViewBean targetView = getViewBean(ErrorHandleViewBean.class);
        CCAlertInline alert = (CCAlertInline)
            targetView.getChild(ErrorHandleViewBean.CHILD_ALERT);
        alert.setType(CCAlertInline.TYPE_ERROR);
        alert.setSummary(SamUtil.getResourceString(
            "ErrorHandle.accessDeniedSummary", new String[] {serverName}));
        alert.setDetail(
            ServerUtil.lineBreak + SamUtil.getAccessDeniedMessage(),
            new String[] { serverName });
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
        targetView.forwardTo(getRequestContext());
    }

    private void executeNotSupportedRoutine(String serverName)
        throws ModelControlException {
        ViewBean targetView = getViewBean(ErrorHandleViewBean.class);
        CCAlertInline alert = (CCAlertInline)
            targetView.getChild(ErrorHandleViewBean.CHILD_ALERT);
        alert.setType(CCAlertInline.TYPE_ERROR);
        String serverVersion =
            getSystemModel(serverName).getServerProductVersion();
        alert.setSummary(
            SamUtil.getResourceString(
                "ErrorHandle.serverNotSupportedSummary",
                new String[] {serverName, serverVersion}));
        alert.setDetail(
            ServerUtil.lineBreak + SamUtil.getResourceString(
                "ErrorHandle.serverNotSupportedDetail"),
            new String[] { serverName });
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
        targetView.forwardTo(getRequestContext());

    }
}
