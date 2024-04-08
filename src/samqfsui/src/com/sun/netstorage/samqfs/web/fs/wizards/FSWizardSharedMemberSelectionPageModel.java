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

// ident	$Id: FSWizardSharedMemberSelectionPageModel.java,v 1.18 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

/**
 * A ContainerView object for the pagelet for device selection page.
 *
 */
public class FSWizardSharedMemberSelectionPageModel extends CCActionTableModel {

    private String[] apiArch;
    private String[] apiVersion;
    private String[] sharedLicense;
    private String[] allHostNames;
    private HashMap ipAddressMap = new HashMap();
    private HashMap archMap = new HashMap();
    private HashMap versionMap = new HashMap();


    public FSWizardSharedMemberSelectionPageModel() {
        super(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/FSWizardSharedMemberSelectionTable.xml");

        setActionValue(
            "SharedMemberCol0",
            "FSWizard.sharedMemberSelectionTable.hostNameHeading");
        setActionValue(
            "SharedMemberCol1",
            "FSWizard.sharedMemberSelectionTable.samVersionHeading");
        setActionValue(
            "SharedMemberCol2",
            "FSWizard.sharedMemberSelectionTable.hardwareArchHeading");
        setActionValue(
            "SharedMemberCol3",
            "FSWizard.sharedMemberSelectionTable.clientHeading");
        setActionValue(
            "SharedMemberCol4",
            "FSWizard.sharedMemberSelectionTable.potentialMetaServerHeading");
        setActionValue(
            "SharedMemberCol5",
            "FSWizard.sharedMemberSelectionTable.primaryIPHeading");
        setActionValue(
            "SharedMemberCol6",
            "FSWizard.sharedMemberSelectionTable.secondaryIPHeading");

        TraceUtil.trace2("Leaving createActionTableModel");
    }

    /**
     * All Allocatable Units are stored wizard Model.
     * This way we can avoid calling the backend everytime the user hits
     * the previous button.
     * Also this will help in keeping track of all the selections that the user
     * made during the course of the wizard, and will he helpful to remove
     * those entries for the Metadata LUN selection page.
     */
    public void populateSharedMemberModel(SamWizardModel wm)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        clear();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);
        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemModel [] allSystemModel =
            appModel.getAllSamQFSSystemModels();
        if (allSystemModel == null || allSystemModel.length == 0) {
            return;
        }

        SamQFSSystemSharedFSManager fsManager =
            appModel.getSamQFSSystemSharedFSManager();

        TraceUtil.trace3("before newhost newhost api version");
        String samfsServerAPIVersion = "samfs4.2";
        String samfsServerArch = "sparc";
        String hostname = null;
        apiArch = new String[allSystemModel.length];
        apiVersion = new String[allSystemModel.length];
        allHostNames = new String[allSystemModel.length];

        int rowIndex = 0;
        String[] newHosts = new String[allSystemModel.length];
        int loc = 0;
        TraceUtil.trace3("before newhost metadta host handle");
        for (int i = 0; i < allSystemModel.length; i++) {
            String hName = allSystemModel[i].getHostname();
            if (hName.equals(serverName)) {
                newHosts[0] = hName;
                loc = i;
                break;
            }
        }
        TraceUtil.trace3("before newhost handle = " +
            Integer.toString(allSystemModel.length));
        TraceUtil.trace3("newhost =" + newHosts[0]);
        TraceUtil.trace3("newhost metahost =" + serverName);

        for (int i = 1; i <= loc; i++) {
            newHosts[i] = allSystemModel[i - 1].getHostname();
        }
        TraceUtil.trace3("loc = " + Integer.toString(loc));
        for (int i = loc + 1; i < allSystemModel.length; i++) {
            newHosts[i] = allSystemModel[i].getHostname();
        }

        TraceUtil.trace3("before newhost trace");
        for (int i = 0; i < newHosts.length; i++) {
            TraceUtil.trace3("newhost is " + newHosts[i]);
            hostname = newHosts[i];
            if (!appModel.getSamQFSSystemModel(hostname).isDown() &&
                !hostname.equals(serverName)) {

                ipAddressMap.put(
                    hostname,
                    Arrays.asList(fsManager.getIPAddresses(hostname)));

                SamQFSSystemModel model = SamUtil.getModel(hostname);
                archMap.put(
                    hostname,
                    model.getArchitecture());
                versionMap.put(
                    hostname,
                    model.getServerProductVersion());

                // We have to show all the servers here.  We cannot omit the
                // servers that are not a part of the cluster.  Those machines
                // can serve as a client of this shared file system.  The
                // nextStep() of this page should have checked if a server
                // that happens to be outside the cluster, it can only be served
                // as a client.

                if (i > 0) {
                    appendRow();
                }
                setValue("hostName", hostname);
                setValue("HiddenHostName", hostname);
                allHostNames[rowIndex] = hostname;

                samfsServerAPIVersion = model.getServerAPIVersion();
                samfsServerArch = model.getArchitecture();
                setValue("hardwareArch", samfsServerArch);
                apiArch[rowIndex] = samfsServerArch;
                samfsServerAPIVersion = model.getServerProductVersion();
                setValue("samVersion", samfsServerAPIVersion);
                apiVersion[rowIndex] = samfsServerAPIVersion;
                setRowIndex(rowIndex);
                rowIndex++;
            }
        } // end for loop

        TraceUtil.trace3("Exiting");
    }

    public String getApiArch(int index) {
        return apiArch[index];
    }

    public String getApiVersion(int index) {
        return apiVersion[index];
    }

    public String getSharedLicense(int index) {
        return sharedLicense[index];
    }
    public String getAllHostNames(int index) {
        return allHostNames[index];
    }

    public String [] getIPAddresses(String serverName) {
        List myList = (List) ipAddressMap.get(serverName);
        return (String []) myList.toArray(new String[myList.size()]);
    }

    public String getArchitecture(String serverName) {
        return (String) archMap.get(serverName);
    }

    public String getServerVersion(String serverName) {
        return (String) versionMap.get(serverName);
    }
}
