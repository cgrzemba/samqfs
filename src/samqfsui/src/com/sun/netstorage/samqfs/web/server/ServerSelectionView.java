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

// ident    $Id: ServerSelectionView.java,v 1.41 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSAccessDeniedException;
import com.sun.netstorage.samqfs.mgmt.SamFSCommException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SystemCapacity;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;

import com.sun.web.ui.common.CCSeverity;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alarm.CCAlarmObject;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;

import java.io.IOException;
import java.util.Map;
import javax.servlet.ServletException;

public class ServerSelectionView extends MultiTableViewBase {

    // child name for tiled view class
    public static final String CHILD_TILED_VIEW = "ServerSelectionTiledView";

    public static final String SERVER_TABLE = "ServerSelectionTable";

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public ServerSelectionView(View parent, Map models, String name) {
        super(parent, models, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        super.registerChildren();
        registerChild(CHILD_TILED_VIEW, ServerSelectionTiledView.class);
    }

    public View createChild(String name) {
        View child = null;
        if (name.equals(CHILD_TILED_VIEW)) {
            child = new ServerSelectionTiledView(
                this, getTableModel(SERVER_TABLE), name);
        } else if (name.equals(SERVER_TABLE)) {
            child = createTable(name, CHILD_TILED_VIEW);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null) {
                child = super.isChildSupported(name).createChild(this, name);
            }
        }

        if (child == null) {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        return (View) child;
    }

    public void handleAddButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleRemoveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("User trying to remove a host ...");
        String rowKey = getSelectedRowKey();

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            if (appModel == null) {
                TraceUtil.trace1("AppModel is null!");
                throw new SamFSException(null, -2501);
            }

            LogUtil.info(this.getClass(), "handleRemoveButtonRequest",
                new StringBuffer().append(
                    "Start removing server ").append(rowKey).toString());
            appModel.removeHost(rowKey);
            LogUtil.info(this.getClass(), "handleRemoveButtonRequest",
                new StringBuffer().append(
                    "Done removing server ").append(rowKey).toString());
            TraceUtil.trace3("Done removing host!");
            setSuccessAlert("ServerSelection.action.delete", rowKey);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleRemoveButtonRequest",
                "Failed to remove server",
                rowKey);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
                "ServerSelection.error.delete",
                ex.getSAMerrno(),
                ex.getMessage(),
                rowKey);
        }
        getParentViewBean().forwardTo(getRequestContext());
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        initializeTableHeaders();

        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            // enable the Add Button back in AT
            ((CCButton) getChild("AddButton")).setDisabled(false);
        } else {
            // disable the radio button row selection column
            CCActionTableModel model = getTableModel(SERVER_TABLE);
            model.setSelectionType("none");
        }

        // Disable Tooltip
        CCActionTable myTable = (CCActionTable) getChild(SERVER_TABLE);
        CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
            CCActionTable.CHILD_SELECTION_RADIOBUTTON);
        myRadio.setTitle("");
        myRadio.setTitleDisabled("");
    }

    private String getSelectedRowKey() throws ModelControlException {
        // restore the selected rows
        CCActionTable actionTable =
            (CCActionTable) getChild(SERVER_TABLE);

        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(mcex, this.getClass(),
                "getSelectedRowKey",
                "Failed to retrieve selected row key",
                "Unknown");
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        CCActionTableModel serverModel = getTableModel(SERVER_TABLE);

        // single select Action Table, so just take the first row in the
        // array
        Integer [] selectedRows = serverModel.getSelectedRows();
        int row = selectedRows[0].intValue();
        serverModel.setRowIndex(row);

        // return server name
        String [] returnArray =
            ((String) serverModel.getValue(
                "HiddenInfo")).split(ServerUtil.delimitor);

        TraceUtil.trace3(new StringBuffer().append(
            "Selected server name is ").append(returnArray[0]).toString());

        return returnArray[0];
    }

    private void setSuccessAlert(String msg, String item) {
        SamUtil.setInfoAlert(
            getParentViewBean(),
            ServerCommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            "");
    }

    private void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(SERVER_TABLE);
        model.setRowSelected(false);

        model.setActionValue(
            "NameColumn",
            "ServerSelection.heading.name");
        model.setActionValue(
            "FaultsColumn",
            "ServerSelection.heading.faults");
        model.setActionValue(
            "VersionColumn",
            "ServerSelection.heading.version");
        model.setActionValue(
            "ArchColumn",
            "ServerSelection.heading.architecture");
        model.setActionValue(
            "DiskCacheColumn",
            "ServerSelection.heading.diskcache");
        model.setActionValue(
            "MediaCapacityColumn",
            "ServerSelection.heading.mediacapacity");
    }

    public void populateTableModels() throws SamFSException {
        populateServerTable();
    }

    private void populateServerTable() throws SamFSException {

        // The following variables are amount per host based.
        long serverDiskCache = 0;
        long serverAvailableDiskCache = 0;
        long serverMediaCapacity = 0;
        long serverAvailableMediaCapacity = 0;
        int alarmType = -1;
        int diskConsumed = -1, mediaConsumed = -1;
        String hostName = "";
        String versionNumber = "";
        String architecture = "";
        String diskCacheString = "";
        String mediaCapacityString = "";

        // Retrieve the handle of the Server Selection Table
        CCActionTableModel model = getTableModel(SERVER_TABLE);
        model.clear();

        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        if (appModel == null) {
            throw new SamFSException(null, -2501);
        }

        // try to re-establish connection to DOWN servers
        appModel.updateDownServers();

        SamQFSSystemModel[] allSystemModel =
            appModel.getAllSamQFSSystemModels();
        if (allSystemModel == null) {
            return;
        }

        int numOfServers = 0;
        for (int i = 0; i < allSystemModel.length; i++) {
            // append new row
            if (numOfServers > 0) {
                model.appendRow();
            }

            try {
                hostName = allSystemModel[i].getHostname();
                model.setValue("NameText", hostName);

                if (allSystemModel[i].isClusterNode()) {
                    model.setValue(
                        "NameText",
                        SamUtil.getResourceString(
                            "ServerSelection.host.clusterword",
                            new String [] {
                                hostName,
                                allSystemModel[i].getClusterName()}));
                }

                TraceUtil.trace2(
                    new StringBuffer("hostName: ").append(hostName).
                    toString());

                // DOWN fault
                // try to use the hostName to get sysModel
                // display DOWN fault icon if isDown flag is true
                if (allSystemModel[i].isDown()) {
                    TraceUtil.trace2("System is DOWN");
                    if (allSystemModel[i].isAccessDenied()) {
                        TraceUtil.trace1(new StringBuffer(
                            "Access Denied to server ").
                            append(hostName).toString());
                        alarmType = ServerUtil.ALARM_ACCESS_DENIED;
                    } else {
                        TraceUtil.trace2("Down: some other problem");
                        alarmType = ServerUtil.ALARM_DOWN;
                    }
                } else if (!allSystemModel[i].isServerSupported()) {
                    TraceUtil.trace1(new StringBuffer(
                        "Server ").append(hostName).append(
                        " is not supported.  Out of rev.").toString());
                    alarmType = ServerUtil.ALARM_NOT_SUPPORTED;
                } else {
                    TraceUtil.trace2("Server is supported!");

                    if (hostName != null) {
                        versionNumber =
                            ServerUtil.getVersionString(SamUtil.getModel(
                                hostName).getServerProductVersion());

                        architecture = SamUtil.swapArchString(
                            allSystemModel[i].getArchitecture());
                    }

                    AlarmSummary myAlarmSummary = allSystemModel[i].
                        getSamQFSSystemAlarmManager().getAlarmSummary();

                    int critical = myAlarmSummary.getCriticalTotal();
                    int major    = myAlarmSummary.getMajorTotal();
                    int minor    = myAlarmSummary.getMinorTotal();

                    // No fault
                    if (myAlarmSummary == null) {
                        alarmType = ServerUtil.ALARM_NO_ERROR;
                    } else {
                        alarmType = getMostSevereAlarm(critical, major, minor);
                        TraceUtil.trace2(new StringBuffer(
                            "AlarmType is ").append(alarmType).toString());
                    }


                    // Disk Cache / Media Capacity
                    SystemCapacity mySystemCapacity =
                        allSystemModel[i].getCapacity();
                    serverDiskCache =
                        mySystemCapacity.getDiskCacheKB() == -1 ?
                        0 : mySystemCapacity.getDiskCacheKB();
                    serverAvailableDiskCache =
                        mySystemCapacity.getAvailDiskCacheKB() == -1 ?
                            0 : mySystemCapacity.getAvailDiskCacheKB();

                    diskConsumed =
                        serverDiskCache == 0 ?
                            0 :
                            100 - (int) (serverAvailableDiskCache * 100 /
                                                        serverDiskCache);
                    if (diskConsumed < 0 || diskConsumed > 100) {
                        diskCacheString = Constants.Image.ICON_BLANK_ONE_PIXEL;
                    } else {
                        diskCacheString = new StringBuffer(
                               Constants.Image.USAGE_BAR_DIR).
                               append(diskConsumed).append(".gif").toString();
                    }

                    serverMediaCapacity =
                        mySystemCapacity.getMediaKB() == -1 ?
                        0 : mySystemCapacity.getMediaKB();
                    serverAvailableMediaCapacity =
                        mySystemCapacity.getAvailMediaKB() == -1 ?
                            0 : mySystemCapacity.getAvailMediaKB();

                    mediaConsumed =
                        serverAvailableMediaCapacity == 0 ?
                            0 :
                            100 - (int) (serverAvailableMediaCapacity * 100 /
                                                serverAvailableMediaCapacity);
                    if (mediaConsumed < 0 || mediaConsumed > 100) {
                        mediaCapacityString =
                            Constants.Image.ICON_BLANK_ONE_PIXEL;
                    } else {
                        mediaCapacityString = new StringBuffer(
                               Constants.Image.USAGE_BAR_DIR).
                               append(mediaConsumed).append(".gif").toString();
                    }
                }
                numOfServers++;
            } catch (SamFSAccessDeniedException denyEx) {
                // DO NOT need to call processException
                // We do not need to try to reconnect to the server
                // User intervention is required here
                TraceUtil.trace1(
                    "Access Denied to server " + hostName, denyEx);
                alarmType = ServerUtil.ALARM_ACCESS_DENIED;
            } catch (SamFSCommException comEx) {
                // allSystemModel[i].setDown();
                TraceUtil.trace1(
                    "Communication exception while connecting to server " +
                    hostName, comEx);
                SamUtil.processException(
                    comEx,
                    this.getClass(),
                    "initModelRows()",
                    "Communication exception occur while connecting to server",
                    hostName);
                alarmType = ServerUtil.ALARM_DOWN;
            } catch (SamFSException samEx) {
                TraceUtil.trace1(
                    "General SAM FS exception while connecting to server " +
                    hostName, samEx);
                TraceUtil.trace1("Reason: " + samEx.getMessage());
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "initModelRows()",
                    "General SAM FS exception occur while connecting to server",
                    hostName);
                alarmType = ServerUtil.ALARM_DOWN;
            } catch (Exception genEx) {
                // for safety purposes
                TraceUtil.trace1(
                    "Unknown Exception while connecting to server " + hostName,
                    genEx);
                SamUtil.processException(
                    genEx,
                    this.getClass(),
                    "initModelRows()",
                    "Unknown exception occur while connecting to server",
                    hostName);
                alarmType = ServerUtil.ALARM_DOWN;
            }

            // Clear version, disk cache, and media capacity field if
            // the server is down or out of rev, this is needed for simulator
            // purpose.

            switch (alarmType) {
                case ServerUtil.ALARM_DOWN:
                case ServerUtil.ALARM_ACCESS_DENIED:
                case ServerUtil.ALARM_NOT_SUPPORTED:
                    versionNumber = "";
                    architecture = "";
                    diskCacheString = Constants.Image.ICON_BLANK_ONE_PIXEL;
                    mediaCapacityString = Constants.Image.ICON_BLANK_ONE_PIXEL;
                    mediaConsumed = -1;
                    diskConsumed = -1;
                    break;

                default:
                    break;
            }

            model.setValue("VersionText", versionNumber);
            model.setValue("ArchText", architecture);

            model.setValue("DiskUsageBarImage", diskCacheString);
            model.setValue("DiskCapacityText",
                diskConsumed == -1 ?
                    "" :
                    new StringBuffer("(").append(
                        new Capacity(
                            serverDiskCache, SamQFSSystemModel.SIZE_KB)).
                            append(")").toString());
            model.setValue("DiskUsageText", new Integer(diskConsumed));

            model.setValue("MediaUsageBarImage", mediaCapacityString);
            model.setValue("MediaCapacityText",
                mediaConsumed == -1 ?
                    "" :
                    new StringBuffer("(").append(
                    new Capacity(
                        serverMediaCapacity, SamQFSSystemModel.SIZE_KB)).
                        append(")").toString());
            model.setValue("MediaUsageText", new Integer(mediaConsumed));

            model.setValue("AccessDeniedText", "");
            String info =
                new StringBuffer(hostName).append(ServerUtil.delimitor).
                    append(alarmType).toString();
            model.setValue("HiddenInfo", info);
            model.setValue(
                ServerSelectionTiledView.CHILD_ALARM_HREF, info);
            model.setValue(
                ServerSelectionTiledView.CHILD_SERVER_HREF, info);
            model.setValue(
                ServerSelectionTiledView.CHILD_DISK_CACHE_HREF, info);
            model.setValue(
                ServerSelectionTiledView.CHILD_MEDIA_CAPACITY_HREF, info);

            SamUtil.doPrint(new StringBuffer("Host: ").append(hostName)
                .append(" Alarm: ").append(alarmType).toString());

            // Set Alarm Icon
            setAlarmInModel(alarmType);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * To return
     */
    private int getMostSevereAlarm(int critical, int major, int minor) {
        if (critical != 0) {
            return ServerUtil.ALARM_CRITICAL;
        } else if (major != 0) {
            return ServerUtil.ALARM_MAJOR;
        } else if (minor != 0) {
            return ServerUtil.ALARM_MINOR;
        } else {
            return ServerUtil.ALARM_NO_ERROR;
        }
    }

    private void setAlarmInModel(int alarmType) {
        CCActionTableModel model = getTableModel(SERVER_TABLE);

        switch (alarmType) {
            case ServerUtil.ALARM_DOWN:
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.DOWN));
                break;

            case ServerUtil.ALARM_CRITICAL:
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.CRITICAL));
                break;

            case ServerUtil.ALARM_MAJOR:
                // MAJOR
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.MAJOR));
                break;

            case ServerUtil.ALARM_MINOR:
                // MINOR
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.MINOR));
                break;

            case ServerUtil.ALARM_ACCESS_DENIED:
                // ACCESS DENIED
                // no alarm icon, set to OK
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.OK));
                // Set message to denied
                model.setValue("AccessDeniedText",
                    SamUtil.getResourceString("access.denied"));
                break;

            case ServerUtil.ALARM_NOT_SUPPORTED:
                // SERVER NOT SUPPORTED, OUT OF REV
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.OK));
                // Set message to denied
                model.setValue("AccessDeniedText",
                    SamUtil.getResourceString("server.notsupported"));
                break;

            default:
                // Not in use
                model.setValue("Alarm", new CCAlarmObject(CCSeverity.OK));
                break;
        }
    }
} // end of ServerSelectionView class
