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

// ident    $Id: AdvancedNetworkConfigDisplayView.java,v 1.10 2008/10/01 22:43:32 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.web.model.MDSAddresses;

import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemSharedFSManager;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;

import java.util.ArrayList;

public class AdvancedNetworkConfigDisplayView
        extends CommonTableContainerView {

    // Hidden field to keep track of MDS and PMDS host names to pass to popup
    private static String MDS_LIST = "MDSNames";

    // Hidden field to keep track all the host in the action table
    // This is used in the javascript to grep which host name are selected to
    // perform the "Modify Settings" operation
    private static String ALL_SHARED_HOST_NAMES = "AllSharedHostNames";

    // Hidden field to store the Shared FS Name, eventually passed to Pop Up
    private static String FS_NAME = "FSName";

    // Hidden name to keep track of MDS of this shared file system
    public static final String MDS_NAME = "MDServerName";

    private CCActionTableModel tableModel = null;
    private SharedMember [] sharedMember  = null;
    private String [] metadataHosts = null;
    private String MDServerName = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AdvancedNetworkConfigDisplayView(
        View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "AdvancedNetworkConfigDisplayTable";

        try {
            // to fill up sharedMember and metadataHost array
            getAllSharedMemberInformation();

        } catch (SamFSMultiHostException multiEx) {
            String errMsg = SamUtil.handleMultiHostException(multiEx);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "AdvancedNetworkConfig.setup.error.populate",
                multiEx.getSAMerrno(),
                SamUtil.handleMultiHostException(multiEx),
                getServerName());
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "AdvancedNetworkConfigDisplayView()",
                "Failed to retrieve shared members information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "AdvancedNetworkConfig.setup.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        createTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(tableModel);
        registerChild(MDS_LIST, CCHiddenField.class);
        registerChild(ALL_SHARED_HOST_NAMES, CCHiddenField.class);
        registerChild(FS_NAME, CCHiddenField.class);
        registerChild(MDS_NAME, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append(
            "Entering: name is ").append(name).toString());

        View child = null;
        if (name.equals(MDS_LIST) ||
            name.equals(ALL_SHARED_HOST_NAMES) ||
            name.equals(FS_NAME) ||
            name.equals(MDS_NAME)) {
            child = new CCHiddenField(this, name, null);

        } else {
            child = super.createChild(tableModel, name);
        }

        TraceUtil.trace3("Exiting");
        return child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeTableHeaders();

        // Remove check boxes selection if user does not have authorization.
        // The Modify Settings button will be enabled upon selection made
        // by user in the client side.
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            // disable the radio button row selection column
            tableModel.setSelectionType("none");
        } else {
            // Make sure Modify Button starts with a disabled state
            // Button will be enabled upon closing pop up if not making
            // this call!
            ((CCButton) getChild("ModifyButton")).setDisabled(true);
        }

        // Set shared fs name and mds in hidden fields for pop up
        ((CCHiddenField) getChild(FS_NAME)).setValue(getFSName());
        ((CCHiddenField) getChild(MDS_NAME)).setValue(MDServerName);

        TraceUtil.trace3("Exiting");
    }

    public void createTableModel() {
        StringBuffer xmlBuf =
            new StringBuffer(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>").append(
                "<!DOCTYPE table SYSTEM \"tags/dtd/table.dtd\">").append(
                "<table>").append(
                    "<actions>").append(
                    "<cc name=\"ModifyButton\" ").append(
                        "tagclass=\"com.sun.web.ui.taglib.html.CCButtonTag\"> ")
                        .append(
                            "<attribute name=\"dynamic\" value=\"true\" /> ")
                        .append(
                            "<attribute name=\"disabled\" value=\"true\" /> ")
                        .append(
                            "<attribute name=\"defaultValue\" value=\"").append(
                            "AdvancedNetworkConfig.setup.button.submit\"")
                        .append("/> ")
                        .append(
                            "<attribute name=\"onClick\" value=\"").append(
                            "return launchConfigWindow(); \"").
                        append("/> ").append(
                    "</cc>").append(
                    "</actions>").append(createColumnXMLString("Host"));

        for (int i = 0; i < metadataHosts.length; i++) {
            xmlBuf = xmlBuf.append(createColumnXMLString(metadataHosts[i]));
        }

        xmlBuf = xmlBuf.append("</table>");

        tableModel = new CCActionTableModel(xmlBuf.toString());
    }

    private StringBuffer createColumnXMLString(String item) {
        return new StringBuffer(
            "<column name=\"").append(item).append(
            "_Column\" extrahtml = \"nowrap\" > ").append(
            "<cc name=\"").append(item).append("_Text\" ").append(
            "tagclass=\"com.sun.web.ui.taglib.html.CCStaticTextFieldTag\" />").
            append("</column> ");
    }

    private void initializeTableHeaders() {
        tableModel.setRowSelected(false);

        tableModel.setActionValue(
            "Host_Column", "AdvancedNetworkConfig.display.heading.hostname");
        /** Columns are generated dynamically */
        for (int i = 0; i < metadataHosts.length; i++) {
            tableModel.setActionValue(
                metadataHosts[i] + "_Column",
                SamUtil.getResourceString(
                    "AdvancedNetworkConfig.display.heading.interface",
                    metadataHosts[i]));
        }
    }

    public void populateTableModels()
        throws SamFSMultiHostException, SamFSException {
        populateCurrentSetupTable();
    }

    public void populateCurrentSetupTable()
        throws SamFSMultiHostException, SamFSException {
        TraceUtil.trace3("Entering");

        ArrayList errorHostNames = new ArrayList();
        ArrayList errorExceptions = new ArrayList();

        CCActionTable myTable = (CCActionTable) getChild(CHILD_ACTION_TABLE);

        for (int i = 0; i < sharedMember.length; i++) {
            // append new row
            if (i > 0) {
                tableModel.appendRow();
            }

            // Disable Tooltip
            CCCheckBox myCheckBox =
                (CCCheckBox) myTable.getChild(
                     CCActionTable.CHILD_SELECTION_CHECKBOX + i);
            myCheckBox.setTitle("");
            myCheckBox.setTitleDisabled("");

            tableModel.setValue("Host_Text", sharedMember[i].getHostName());

            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemSharedFSManager fsManager =
                appModel.getSamQFSSystemSharedFSManager();

            try {
                MDSAddresses [] myAddresses =
                    fsManager.getAdvancedNetworkConfig(
                        sharedMember[i].getHostName(), getFSName());

                if (myAddresses == null || myAddresses.length == 0) {
                    for (int j = 0; j < metadataHosts.length; j++) {
                        // address is null, nothing is set for these
                        // participating hosts
                        tableModel.setValue(
                            metadataHosts[j] + "_Text",
                            "");
                    }
                }

                for (int j = 0; j < metadataHosts.length; j++) {
                    if (sharedMember[i].getHostName().
                        equals(metadataHosts[j])) {
                        // Participating Host and MDS/PMDS is actually the
                        // same machine.  Show nothing
                        tableModel.setValue(
                            metadataHosts[j] + "_Text",
                            "");
                    } else {
                        tableModel.setValue(
                            metadataHosts[j] + "_Text",
                            getAddresses(myAddresses, metadataHosts[j]));
                    }
                }
            } catch (SamFSException ex) {
               errorHostNames.add(sharedMember[i].getHostName());
               errorExceptions.add(ex);

               for (int j = 0; j < metadataHosts.length; j++) {
                   tableModel.setValue(
                       metadataHosts[j] + "_Text",
                       SamUtil.getResourceString(
                           "AdvancedNetworkConfig.display.notavailable"));
               }
               continue;
            }
        }

        if (!errorHostNames.isEmpty()) {
            throw new SamFSMultiHostException(
                "logic.sharedFSOperationPartialFailure",
                (SamFSException[])
                errorExceptions.toArray(new SamFSException[0]),
                (String[]) errorHostNames.toArray(new String[0]));
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * This method will get all the necessary shared member information
     * "sharedMember" and "metadataHosts" (class variables).
     */
    private void getAllSharedMemberInformation()
        throws SamFSMultiHostException, SamFSException {
        SamQFSSystemSharedFSManager fsManager =
            SamQFSFactory.getSamQFSAppModel().getSamQFSSystemSharedFSManager();

        SamFSMultiHostException multiEx = null;

        try {
            sharedMember =
                fsManager.getSharedMembers(getServerName(), getFSName());
        } catch (SamFSMultiHostException e) {
            // If we have a multihost exception, we need to handle partial
            // failure.  getPartialResult() will give back some partail
            // results.
            sharedMember = (SharedMember[]) e.getPartialResult();
            if (sharedMember != null) {
                TraceUtil.trace3("exception shared member length = " +
                    Integer.toString(sharedMember.length));
            }
            SamUtil.doPrint(new StringBuffer().
                append("error code is ").
                append(e.getSAMerrno()).toString());

            // save the exception
            multiEx = e;
        }

        if (sharedMember == null || sharedMember.length == 0) {
            sharedMember  = new SharedMember[0];
            metadataHosts = new String[0];
            throw multiEx;
        }

        StringBuffer mdsHostsBuf = new StringBuffer();
        StringBuffer allHostsBuf = new StringBuffer();

        for (int i = 0; i < sharedMember.length; i++) {
            if (allHostsBuf.length() != 0) {
                allHostsBuf.append(",");
            }
            allHostsBuf.append(sharedMember[i].getHostName());

            switch (sharedMember[i].getType()) {
                case SharedMember.TYPE_MD_SERVER:
                    // Save the MDS name
                    MDServerName = sharedMember[i].getHostName();
                    if (mdsHostsBuf.length() != 0) {
                        mdsHostsBuf.append(",");
                    }
                    mdsHostsBuf.append(sharedMember[i].getHostName());
                    break;

                case SharedMember.TYPE_POTENTIAL_MD_SERVER:
                    if (mdsHostsBuf.length() != 0) {
                        mdsHostsBuf.append(",");
                    }
                    mdsHostsBuf.append(sharedMember[i].getHostName());
                    break;
            }
        }

        metadataHosts = (mdsHostsBuf.length() == 0) ?
            new String[0] : mdsHostsBuf.toString().split(",");

        // Setting both hidden fields for pop up and javascript (modify setting)
        ((CCHiddenField) getChild(MDS_LIST)).setValue(mdsHostsBuf.toString());
        ((CCHiddenField) getChild(ALL_SHARED_HOST_NAMES)).
            setValue(allHostsBuf.toString());

        // If there is a partial failure in the process, bubble it up
        if (multiEx != null) {
            throw multiEx;
        }
    }


    /**
     * This method returns the corresponding IPAddresses that the participating
     * host communicates with each of the metadata/potential metadata servers.
     */
    private String getAddresses(MDSAddresses [] myAddresses, String mdsName) {
        for (int i = 0; i < myAddresses.length; i++) {
            if (mdsName.equals(myAddresses[i].getHostName())) {
                String [] ipAddresses = myAddresses[i].getIPAddress();
                StringBuffer buf = new StringBuffer();
                for (int j = 0; j < ipAddresses.length; j++) {
                    if (buf.length() > 0) {
                        buf.append(", ");
                    }
                    buf.append(ipAddresses[j]);
                }
                return buf.toString();
            }
        }
        return "";
    }

    private String getFSName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

} // end of AdvancedNetworkConfigDisplayView class
