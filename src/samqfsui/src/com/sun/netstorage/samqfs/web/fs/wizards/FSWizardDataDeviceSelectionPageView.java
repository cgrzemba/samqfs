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

// ident	$Id: FSWizardDataDeviceSelectionPageView.java,v 1.30 2008/07/16 21:55:56 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.view.table.CCActionTable;
import java.util.ArrayList;

/**
 * A ContainerView object for the pagelet for data device selection page.
 *
 */
public class FSWizardDataDeviceSelectionPageView
    extends FSWizardDeviceSelectionPageView {

    // The "logical" name for this page.
    public static final String
        PAGE_NAME = "FSWizardDataDeviceSelectionPageView";

    // key used to store previously selected fs type
    public static final String PREV_FSTYPE_KEY = "PREV_FS_TYPE";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardDataDeviceSelectionPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public FSWizardDataDeviceSelectionPageView(
        View parent, Model model, String name) {

        super(parent, model, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // Since this page (data device selection page) is shared by UFS and QFS
        // and table selection type is different between UFS (single selection)
        // and QFS (multiple selection), we need to reset actiontable state data
        // if fs type has changed.
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // fsType is null when this view is used in the Grow wizard
        String fsType = (String)
            wizardModel.getValue(CreateFSWizardImpl.FSTYPE_KEY);
        String prevFSType = (String) wizardModel.getValue(PREV_FSTYPE_KEY);

        if (prevFSType != null && !prevFSType.equals(fsType)) {
            CCActionTable table = (CCActionTable) getChild(CHILD_ACTIONTABLE);
            table.resetStateData();
        }
        wizardModel.setValue(PREV_FSTYPE_KEY, fsType);

        if (fsType != null && fsType.equals(CreateFSWizardImpl.FSTYPE_UFS)) {
            tableModel.setSelectionType(CCActionTableModelInterface.SINGLE);
        } else {
            tableModel.setSelectionType(CCActionTableModelInterface.MULTIPLE);
        }

        super.beginDisplay(event);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create an Empty ActionTable Model and set all the column headings
     */
    protected void createActionTableModel() {
        super.createActionTableModel();
        tableModel.setTitle("FSWizard.dataDeviceSelectionTable.title");
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
        // if (sharedChecked != null && sharedChecked.equals("true")) {
        if (sharedEnabled) {
            devices = (SharedDiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        } else {
            devices = (DiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        }
        totalItems = 0;
        if (devices == null) {
            TraceUtil.trace2("Device List is Null from Impl class");
            throw new SamFSException(null, -2030);
        }

        ArrayList selectedMetadataDevices = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_METADEVICES);
        ArrayList selectedDevices = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_DATADEVICES);

        String samfsServerAPIVersion =
            (String) wizardModel.getValue(Constants.Wizard.SERVER_API_VERSION);

        for (int i = 0; i < devices.length; i++) {
            // Skip the devices that are already selected by the user
            // as metadataDevices
            if (selectedMetadataDevices != null &&
                selectedMetadataDevices.contains(devices[i].getDevicePath())) {
                continue;
            }

            if (i > 0) {
                tableModel.appendRow();
            }

            totalItems++;

            String partition = null;
            int diskType = devices[i].getDiskType();
            switch (diskType) {
                case DiskCache.SLICE:
                    partition = "FSWizard.diskType.slice";
                    break;
                case DiskCache.SVM_LOGICAL_VOLUME:
                    partition = "FSWizard.diskType.svm";
                    break;
                case DiskCache.VXVM_LOGICAL_VOLUME:
                    partition = "FSWizard.diskType.vxvm";
                    break;
                case DiskCache.VXVM_LOGICAL_VOLUME_MIRROR:
                    partition = "FSWizard.diskType.vxvm.mirror";
                    break;
                case DiskCache.SVM_LOGICAL_VOLUME_MIRROR:
                    partition = "FSWizard.diskType.svm.mirror";
                    break;
                case DiskCache.SVM_LOGICAL_VOLUME_RAID_5:
                    partition = "FSWizard.diskType.svm.raid5";
                    break;
                case DiskCache.VXVM_LOGICAL_VOLUME_RAID_5:
                    partition = "FSWizard.diskType.vxvm.raid5";
                    break;
                case DiskCache.ZFS_ZVOL:
                    partition = "FSWizard.diskType.zvol";
                    break;
                default:
                    partition = "";
                    break;
            }

            String pathString = devices[i].getDevicePath();

            String [] sliceElement = pathString.split("/");

            // if the slice is a SVM volume, we need to show the disk group
            if ((diskType == DiskCache.SVM_LOGICAL_VOLUME ||
                diskType == DiskCache.SVM_LOGICAL_VOLUME_MIRROR ||
                diskType == DiskCache.SVM_LOGICAL_VOLUME_RAID_5) &&
                sliceElement.length == 6) {
                tableModel.setValue(
                    "DevicePath", sliceElement[3] + "/" + sliceElement[5]);
            } else {
                int index = pathString.indexOf("/dsk/");
                tableModel.setValue(
                    "DevicePath", pathString.substring(index + 5));
            }
            tableModel.setValue("HiddenDevicePath", pathString);
            // if (!(sharedChecked != null && sharedChecked.equals("true"))) {
            if (sharedEnabled) {
                tableModel.setValue("Partition", partition);
            }

            long dCap = devices[i].getCapacity();
            tableModel.setValue(
                "Capacity", new Capacity(dCap, SamQFSSystemModel.SIZE_MB));

            // if (sharedChecked != null && sharedChecked.equals("true")) {
            if (sharedEnabled) {
                String metaDataHostName = (String)
                    wizardModel.getValue(Constants.Wizard.SERVER_NAME);

                String[] serverHosts =
                    ((SharedDiskCache) devices[i]).availFromServers();
                String[] clientHosts =
                    ((SharedDiskCache) devices[i]).availFromClients();
                TraceUtil.trace3("client host is " + clientHosts.toString());

                ArrayList chosenClientHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_CLIENT_INDEX);
                String displayHosts = metaDataHostName;
                for (int ii = 0; ii < clientHosts.length; ii++) {
                    displayHosts = displayHosts + ";" + clientHosts[ii];
                }
                for (int ii = 0; ii < serverHosts.length; ii++) {
                    displayHosts = displayHosts + ";" + serverHosts[ii];
                }

                ArrayList potentialHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE);
                ArrayList seleectedClientHosts =
                    (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_CLIENT);

                // Show all hosts in the availablefrom column.
                // check all client and potential metadata server with
                // metadata server.  If there is no common device, disable
                // it.
                tableModel.setValue("AvailableFrom", displayHosts);

                if (((SharedDiskCache) devices[i]).usedByClient())
                   tableModel.setSelectionVisible(false);

                if (!(seleectedClientHosts.size() == 0) &&
                    !(potentialHosts.size() == 0)) {
                    if (serverHosts.length != potentialHosts.size() ||
                        clientHosts.length != seleectedClientHosts.size()) {
                            tableModel.setSelectionVisible(false);
                    }

                } else if (!(seleectedClientHosts.size() == 0)) {
                    if (clientHosts.length == 0) {
                            tableModel.setSelectionVisible(false);
                    }
                    if (serverHosts.length != potentialHosts.size() ||
                        clientHosts.length != seleectedClientHosts.size()) {
                            tableModel.setSelectionVisible(false);
                    }
                } else if (!(potentialHosts.size() == 0)) {
                    if (serverHosts.length != potentialHosts.size() ||
                        clientHosts.length != seleectedClientHosts.size()) {
                            tableModel.setSelectionVisible(false);
                    }
                }
            }

            tableModel.setValue("Vendor", devices[i].getVendor());
            tableModel.setValue("ProductID", devices[i].getProductId());

            if (selectedDevices != null &&
                selectedDevices.contains(devices[i].getDevicePath())) {
                tableModel.setRowSelected(true);
                initSelected++;
            }
        }
        TraceUtil.trace2("Finished Adding devices and populateModel");
        TraceUtil.trace3("Exiting");
    }
}
