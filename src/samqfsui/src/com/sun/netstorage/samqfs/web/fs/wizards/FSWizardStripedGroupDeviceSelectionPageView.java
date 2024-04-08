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

// ident	$Id: FSWizardStripedGroupDeviceSelectionPageView.java,v 1.26 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import java.util.ArrayList;

/**
 * A ContainerView object for the pagelet for data device selection page.
 *
 */
public class FSWizardStripedGroupDeviceSelectionPageView
    extends FSWizardDeviceSelectionPageView {

    // The "logical" name for this page.
    public static final String
        PAGE_NAME = "FSWizardStripedGroupDeviceSelectionPageView";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardStripedGroupDeviceSelectionPageView(
        View parent, Model model) {

        this(parent, model, PAGE_NAME);
    }

    public FSWizardStripedGroupDeviceSelectionPageView(
        View parent, Model model, String name) {

        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create an Empty ActionTable Model and set all the column headings
     */
    protected void createActionTableModel() {
        super.createActionTableModel();
        tableModel.setTitle("FSWizard.stripedGroupDeviceSelectionTable.title");
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
    protected void populateActionTableModel() throws SamFSException {
        TraceUtil.trace3("Entering");

        initSelected = 0;
        tableModel.clear();

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        DiskCache[] devices;

        if (sharedEnabled) {
            devices = (SharedDiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        } else {
            devices = (DiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        }

        // filter osd-devices
        Boolean hpc = (Boolean)
            wizardModel.getValue(CreateFSWizardImpl.POPUP_HPC);
        devices = SamUtil.filterInOSDDevices(devices, hpc != null ?
                                             hpc.booleanValue() : false);

        totalItems   = 0;
        if (devices == null) {
            TraceUtil.trace2("Device List is Null from Impl class");
            throw new SamFSException(null, -2001);
        }

        int groupNum = ((Integer) wizardModel.getValue(
            Constants.Wizard.STRIPED_GROUP_NUM)).intValue();
        ArrayList selectedMetadataDevices = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_METADEVICES);
        ArrayList selectedStripedGroupDevices = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES);
        ArrayList thisGroupDevices = null;
        if (selectedStripedGroupDevices != null &&
            selectedStripedGroupDevices.size() > groupNum) {
            thisGroupDevices =
                (ArrayList) selectedStripedGroupDevices.get(groupNum);
        }

        for (int i = 0; i < devices.length; i++) {
            // skip the devices that are already selected by the user
            // as metadataDevices
            if (selectedMetadataDevices != null &&
                selectedMetadataDevices.contains(devices[i].getDevicePath())) {
                continue;
            }

            // skip the devices that are already selected by the user
            // as stripedGroupDevices in previous groups
            if (selectedStripedGroupDevices != null) {
                boolean found = false;
                for (int j = 0; j < groupNum; j++) {
                    ArrayList groupDevices = (ArrayList)
                        selectedStripedGroupDevices.get(j);
                    if (groupDevices.contains(devices[i].getDevicePath())) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
            }

            if (i > 0) {
                tableModel.appendRow();
            }

            totalItems++;

            String partition = null;
            int diskType = devices[i].getDiskType();
            if (diskType == DiskCache.SLICE) {
                partition = "FSWizard.diskType.slice";
            } else if (diskType == DiskCache.SVM_LOGICAL_VOLUME) {
                partition = "FSWizard.diskType.svm";
            } else if (diskType == DiskCache.VXVM_LOGICAL_VOLUME) {
                partition = "FSWizard.diskType.vxvm";
            } else if (diskType == DiskCache.SVM_LOGICAL_VOLUME_MIRROR) {
                partition = "FSWizard.diskType.svm.mirror";
            } else if (diskType == DiskCache.VXVM_LOGICAL_VOLUME_MIRROR) {
                partition = "FSWizard.diskType.vxvm.mirror";
            } else if (diskType == DiskCache.SVM_LOGICAL_VOLUME_RAID_5) {
                partition = "FSWizard.diskType.svm.raid5";
            } else if (diskType == DiskCache.VXVM_LOGICAL_VOLUME_RAID_5) {
                partition = "FSWizard.diskType.vxvm.raid5";
            } else if (diskType == DiskCache.OSD) {
                partition = "FSWizard.diskType.osd";
            }

            tableModel.setValue(
                "DevicePath",
                devices[i].getDevicePathDisplayString());
            tableModel.setValue("HiddenDevicePath", devices[i].getDevicePath());
            tableModel.setValue("Partition", partition);

            long dCap = devices[i].getCapacity();
            tableModel.setValue(
                "Capacity", new Capacity(dCap, SamQFSSystemModel.SIZE_MB));

            tableModel.setValue("Vendor", devices[i].getVendor());
            tableModel.setValue("ProductID", devices[i].getProductId());

            if (thisGroupDevices != null &&
                thisGroupDevices.contains(devices[i].getDevicePath())) {
                tableModel.setRowSelected(true);
                initSelected++;
            }

            // If it is shared, adding available from column
            // so that the customer can know what is going on.
            // if (sharedChecked != null && sharedChecked.equals("true")) {
            if (sharedEnabled) {
                String metaDataHostName = (String)
                    wizardModel.getValue(Constants.Wizard.SERVER_NAME);

                String[] serverHosts =
                    ((SharedDiskCache) devices[i]).availFromServers();
                String[] clientHosts =
                    ((SharedDiskCache) devices[i]).availFromClients();
                TraceUtil.trace3("client host is " + clientHosts.toString());

                String displayHosts = metaDataHostName;
                for (int ii = 0; ii < clientHosts.length; ii++) {
                    displayHosts = displayHosts + ";" + clientHosts[ii];
                }
                ArrayList potentialHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE);
                ArrayList seleectedClientHosts =
                    (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_CLIENT);

                tableModel.setValue("AvailableFrom", displayHosts);
                if (!(seleectedClientHosts.size() == 0)) {
                    if (clientHosts.length == 0) {
                        tableModel.setSelectionVisible(false);
                    }
                } else if (!(potentialHosts.size() == 0)) {
                    if (serverHosts.length != potentialHosts.size()) {
                        tableModel.setSelectionVisible(false);
                    }
                }

            }

        }
        TraceUtil.trace2("Finished Adding devices and populateModel");
        TraceUtil.trace3("Exiting");
    }
}
