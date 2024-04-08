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

// ident	$Id: FSWizardSharedMemberSelectionPageTiledView.java,v 1.26 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.RequestHandlingTiledViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import java.util.ArrayList;
import javax.servlet.http.HttpServletRequest;

/**
 * This class is a Tiled view class for FSWizardSharedMemberSelectionPageView
 * actiontable
 */

public class FSWizardSharedMemberSelectionPageTiledView extends
    RequestHandlingTiledViewBase {

    private FSWizardSharedMemberSelectionPageModel model;
    private View parent;
    private FSWizardSharedMemberSelectionPageView newView;
    private ArrayList selectedClientIndex;
    private ArrayList selectedpotentialIndex;
    private ArrayList selectedPrimaryIPIndex;
    private ArrayList selectedSecondaryIPIndex;
    private SamWizardModel wizardModel;
    public int disableIP = -1;

    private String serverName;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public FSWizardSharedMemberSelectionPageTiledView(
        View parent, FSWizardSharedMemberSelectionPageModel model,
        SamWizardModel wizardModel, String name) {
        super(parent, name);
        this.parent = parent;
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.model = model;
        this.wizardModel = wizardModel;

        registerChildren();
        setPrimaryModel(model);
        selectedClientIndex = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_CLIENT_INDEX);
        selectedpotentialIndex = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_POTENTIAL_METADATA_SERVER_INDEX);
        selectedPrimaryIPIndex = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_PRIMARYIP_INDEX);

        selectedSecondaryIPIndex = (ArrayList)wizardModel.getValue(
            Constants.Wizard.SELECTED_SECONDARYIP_INDEX);
        serverName = (String) wizardModel.getValue(
            Constants.Wizard.SERVER_NAME);
        TraceUtil.trace3("Exiting");

    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        model.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    public void mapRequestParameters(HttpServletRequest request)
        throws ModelControlException {
        ContainerView myParent = (ContainerView) parent;
        CCHiddenField hidden = (CCHiddenField) myParent.getChild(
            FSWizardSharedMemberSelectionPageView.CHILD_MEMBER_HIDDEN);
        String numRows = (String) hidden.getValue();
        if (!numRows.equals("0")) {
            super.mapRequestParameters(request);
        }
    }

    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        if (model.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return model.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /*
     * display shared client host check box cell
     * in the tiled view.
     */
    public boolean beginClientTextFieldDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (((FSWizardSharedMemberSelectionPageView) parent).getError()) {
            TraceUtil.trace3("Error in view, skip clientTextField display!");
            return false;
        }
        int index = model.getRowIndex();
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "beginClientTextFieldDisplay(): Row index is ")
            .append(index).toString());
        String hostName = null;

        // retrieve the latest selection again
        ContainerView myParent = (ContainerView) parent;
        CCHiddenField clientHidden = (CCHiddenField) myParent.getChild(
            FSWizardSharedMemberSelectionPageView.CHILD_CLIENT_HIDDEN);
        CCHiddenField mdsHidden = (CCHiddenField) myParent.getChild(
            FSWizardSharedMemberSelectionPageView.CHILD_MDS_HIDDEN);
        selectedpotentialIndex = (ArrayList) mdsHidden.getValue();
        selectedClientIndex = (ArrayList) clientHidden.getValue();

        hostName = model.getAllHostNames(index);
        String metaArch = model.getArchitecture(hostName);
        TraceUtil.trace3("ARCHITECTURE " + metaArch);
        metaArch = metaArch == null ? "" : metaArch;

        String metaVersion = model.getServerVersion(hostName);
        CCCheckBox clientCheckBox = (CCCheckBox)
            getChild("clientTextField", index);
        clientCheckBox.setValue(
            getEditableValue("clientTextField", index));
        clientCheckBox.resetStateData();

        String arch = model.getApiArch(index);
        if (arch == null) {
            TraceUtil.trace3("architecture equals NULL");
            arch = "sparc";
        }

        TraceUtil.trace3("client ARCHITECTURE " + arch);
        String version = model.getApiVersion(index);
        if (version == null) {
            TraceUtil.trace3("version equals NULL");
            version = "1.1";
        }

        if (!metaVersion.equals(version)) {
            ((CCCheckBox)
                getChild("clientTextField")).setDisabled(true);
            disableIP = index;
        } else {
            ((CCCheckBox)
                getChild("clientTextField")).setDisabled(false);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }


    /*
     * display potential metadata server host check box cell
     * in the tiled view.
     */
    public boolean beginPotentialMetadataServerTextFieldDisplay(
        ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (((FSWizardSharedMemberSelectionPageView) parent).getError()) {
            TraceUtil.trace3("Error in view, skip pmdsTextField display!");
            return false;
        }
        int index = model.getRowIndex();
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "beginPotentialMetadataServerTextFieldDisplay():Row index is ")
            .append(index).toString());
        String hostName = model.getAllHostNames(index);

        String metaArch = model.getArchitecture(hostName);
        TraceUtil.trace3("ARCHITECTURE " + metaArch);
        metaArch = metaArch == null ? "" : metaArch;

        String metaVersion = model.getServerVersion(hostName);

        CCCheckBox potentialMDSCheckBox = (CCCheckBox)
            getChild("potentialMetadataServerTextField", index);
        potentialMDSCheckBox.setValue(
            getEditableValue("potentialMetadataServerTextField", index));
        potentialMDSCheckBox.resetStateData();

        String arch = model.getApiArch(index);
        if (arch == null) {
            TraceUtil.trace3("architecture equals NULL");
            arch = "sparc";
        }

        String version = model.getApiVersion(index);
        if (version == null) {
            TraceUtil.trace3("PotentialMetadataServer version equals NULL");
            version = "1.1";
        }
        if (!metaVersion.equals(version) ||
            (!metaArch.equals(arch) && !metaArch.equals(""))) {
            ((CCCheckBox) getChild(
                "potentialMetadataServerTextField")).setDisabled(true);
        } else {
            ((CCCheckBox) getChild(
                "potentialMetadataServerTextField")).setDisabled(false);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    /*
     * process check box cell of an host in the tiled view.
     * To remember which check box was checked when previous button
     * is checked.
     */
    private String getEditableValue(String type, int index) {
        TraceUtil.trace3("Entering");
        String value = "false";
        if (type.equals("clientTextField")) {
            value =
                Boolean.toString(
                    selectedClientIndex != null &&
                    selectedClientIndex.contains(Integer.toString(index)));
        } else if (type.equals("potentialMetadataServerTextField")) {
            value =
                Boolean.toString(
                    selectedpotentialIndex != null &&
                    selectedpotentialIndex.contains(Integer.toString(index)));
        } else {
            value = "false";
        }

        TraceUtil.trace3("Exiting");
        return value;
    }

    /*
     * display primary IP address cell in the tiled view.
     */
    public boolean beginPrimaryIPTextFieldDisplay(
        ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (((FSWizardSharedMemberSelectionPageView) parent).getError()) {
            TraceUtil.trace3("Error in view, skip primaryIP display!");
            return false;
        }

        int index = model.getRowIndex();
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "beginPrimaryIPTextFieldDisplay():Row index is ")
            .append(index).toString());

        String hostName = model.getAllHostNames(index);
        OptionList options = null;

        if (disableIP == index) {
            ((CCDropDownMenu) getChild(
                "primaryIPTextField", index)).setOptions(options);
        } else {
            CCDropDownMenu primDropDown = (CCDropDownMenu) getChild(
                "primaryIPTextField", index);
            primDropDown.setOptions(
                getOptionValue(true, index, hostName));
            primDropDown.resetStateData();
            String selectedIP =
                getUserSelectedValue(true, index);
            if (selectedIP != null) {
                primDropDown.setValue(selectedIP);
            }
        }

        // To enable/disable based on the check box selections

        String isClientSelected =
            getEditableValue("clientTextField", index);
        String isPMDSSelected =
            getEditableValue("potentialMetadataServerTextField", index);

        if (isClientSelected.equals("true") ||
            isPMDSSelected.equals("true")) {
            ((CCDropDownMenu) getChild(
                "primaryIPTextField", index)).setDisabled(false);
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    /*
     * display secondary IP address cell in the tiled view.
     */
    public boolean beginSecondaryIPTextFieldDisplay(
        ChildDisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (((FSWizardSharedMemberSelectionPageView) parent).getError()) {
            TraceUtil.trace3("Error in view, skip secondary display!");
            return false;
        }
        int index = model.getRowIndex();
        SamUtil.doPrint(new NonSyncStringBuffer().append(
            "beginsecondaryIPTextFieldDisplay():Row index is ")
            .append(index).toString());
        String hostName = model.getAllHostNames(index);
        OptionList options = null;

        CCDropDownMenu secDropDown =
            (CCDropDownMenu) getChild("secondaryIPTextField", index);
        if (disableIP == index) {
            secDropDown.setOptions(options);
        } else {
            secDropDown.setOptions(getOptionValue(false, index, hostName));
            String selectedIP =
                getUserSelectedValue(false, index);
            if (selectedIP != null) {
                secDropDown.setValue(selectedIP);
            }
        }

        // To enable/disable based on the check box selections
        String isClientSelected =
            getEditableValue("clientTextField", index);
        String isPMDSSelected =
            getEditableValue("potentialMetadataServerTextField", index);

        secDropDown.setDisabled(
            !"true".equals(isClientSelected) &&
            !"true".equals(isPMDSSelected));

        TraceUtil.trace3("Exiting");
        return true;
    }

    /*
     * get real IP address and handle ip address sequence
     * for an host.
     */
    private OptionList getOptionValue(
        boolean primaryIP,
        int index,
        String hostName) throws ModelControlException {
        TraceUtil.trace3("Entering");

        OptionList options = null;
        int ipSeqFlag = 0;

        String[] data = model.getIPAddresses(hostName);
        if (data == null) {
            TraceUtil.trace3("NULL ip address");
            data = new String[1];
            data[0] = "---";
        } else {
            TraceUtil.trace3("# of ip found: " + data.length);
        }

        if (primaryIP) {
            TraceUtil.trace3("In primary ip text field");
            options = new OptionList(data, data);
        } else {
            TraceUtil.trace3("In secondary ip text field");

            // dataHost length is the same as data length.  dataHost has
            // "---" always on the top.  The primary ip address selection
            // should NOT be populated in the secondary ip address menu.
            String[] dataHost = new String[data.length];
            dataHost[0] = "---";

            int addedIndex = 1;
            for (int i = 0; i < data.length; i++) {
                // Do not show option that is used by the primary ip
                // address menu
                String selectedPrimIP = getUserSelectedValue(true, index);
                if (selectedPrimIP == null) {
                    if (i == 0) {
                        // Do not add the first entry if nothing has been
                        // selected. The first entry is automatically selected
                        // by the primary IP drop down
                        continue;
                    } else {
                        // not the first entry, not selected by primary, add
                        dataHost[addedIndex++] = data[i];
                    }
                } else if (!data[i].equals(selectedPrimIP)) {
                    // data[i] is not selected by primary, add
                    dataHost[addedIndex++] = data[i];
                } else {
                    TraceUtil.trace3("Already selected. Skip " + data[i]);
                }
            }
            options = new OptionList(dataHost, dataHost);
        }

        TraceUtil.trace3("Exiting");
        return options;
    }

    /**
     * Return the user selected value for the ip drop down menu
     */
    private String getUserSelectedValue(boolean primary, int index)
        throws ModelControlException {
        String[] ipArray = null;

        ArrayList checkArrayList =
            primary ?
                selectedPrimaryIPIndex :
                selectedSecondaryIPIndex;

        if (checkArrayList != null) {
            ipArray = (String[])
                checkArrayList.toArray(new String[0]);
            if (ipArray.length <= index) {
                return null;
            } else {
                return ipArray[index];
            }
        } else {
            return null;
        }
    }
}
