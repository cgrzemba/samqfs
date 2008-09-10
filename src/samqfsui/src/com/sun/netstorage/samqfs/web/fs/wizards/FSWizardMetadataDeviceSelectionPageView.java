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

// ident	$Id: FSWizardMetadataDeviceSelectionPageView.java,v 1.30 2008/09/10 17:40:24 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.SharedDiskCache;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import java.util.ArrayList;

/**
 * A ContainerView object for the pagelet for data device selection page.
 *
 */
public class FSWizardMetadataDeviceSelectionPageView
    extends FSWizardDeviceSelectionPageView {

    // The "logical" name for this page.
    public static final String
        PAGE_NAME = "FSWizardMetadataDeviceSelectionPageView";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardMetadataDeviceSelectionPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public FSWizardMetadataDeviceSelectionPageView(
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
        tableModel.setTitle("FSWizard.metadataDeviceSelectionTable.title");
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

        // clear the table so that next time these is no cache value.
        tableModel.clear();


        TraceUtil.trace3("before getting sharedChecked");
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        Boolean temp = (Boolean)wizardModel
            .getValue(CreateFSWizardImpl.POPUP_SHARED);
        sharedEnabled =
            temp == null ? false : temp.booleanValue();
        DiskCache[] devices;
        if (sharedEnabled) {
            devices = (SharedDiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        } else {
            devices = (DiskCache[]) wizardModel.getValue(
                Constants.Wizard.ALLOCATABLE_DEVICES);
        }

        TraceUtil.trace3("after getting sharedChecked");

        if (devices == null) {
            totalItems = 0;
            TraceUtil.trace2("Device List is Null from Impl class");
            throw new SamFSException(null, -2030);
        } else {
            totalItems = devices.length;
        }

        ArrayList selectedMetadataDevices = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_METADEVICES);

        for (int i = 0; i < devices.length; i++) {
            TraceUtil.trace3("device string is " + devices[i].toString());
            if (i > 0) {
                tableModel.appendRow();
            }

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
                case DiskCache.OSD:
                    partition = "FSWizard.diskType.osd";
                    break;
                default:
                    partition = "";
                    break;
            }
            tableModel.setValue(
                "DevicePath",
                devices[i].getDevicePathDisplayString());
            tableModel.setValue("HiddenDevicePath", devices[i].getDevicePath());
            tableModel.setValue("Partition", partition);

            long dCap = devices[i].getCapacity();
            tableModel.setValue(
                "Capacity", new Capacity(dCap, SamQFSSystemModel.SIZE_MB));

            if (sharedEnabled) {
                String metaDataHostName = (String)
                    wizardModel.getValue(Constants.Wizard.SERVER_NAME);

                String[] serverHosts =
                    ((SharedDiskCache) devices[i]).availFromServers();
                String[] clientHosts =
                    ((SharedDiskCache) devices[i]).availFromClients();
                ArrayList potentialHosts = (ArrayList)wizardModel.getValue(
                    Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_VALUE);
                String displayHosts = metaDataHostName;
                for (int ii = 0; ii < serverHosts.length; ii++) {
                    displayHosts = displayHosts + ";" + serverHosts[ii];
                }
                for (int ii = 0; ii < clientHosts.length; ii++) {
                    displayHosts = displayHosts + ";" + clientHosts[ii];
                }
                tableModel.setValue("AvailableFrom", displayHosts);
                if (!(potentialHosts.size() == 0)) {
                    // if not all potential metadta sever and metadata server
                    // has the same device, disable the checkbox.
                    if (serverHosts.length != potentialHosts.size()) {
                        tableModel.setSelectionVisible(false);
                    }
                }
                /*
                 * don't show a checkbox if this device is already in use on
                 * one of the selected client hosts
                 */
                if (((SharedDiskCache) devices[i]).usedByClient())
                   tableModel.setSelectionVisible(false);
            }

            tableModel.setValue("Vendor", devices[i].getVendor());
            tableModel.setValue("ProductID", devices[i].getProductId());

            if (selectedMetadataDevices != null &&
                selectedMetadataDevices.contains(devices[i].getDevicePath())) {
                tableModel.setRowSelected(true);
                initSelected++;
            }
        }
        TraceUtil.trace2("Finished Adding devices and populateModel");
        TraceUtil.trace3("Exiting");
    }
}
