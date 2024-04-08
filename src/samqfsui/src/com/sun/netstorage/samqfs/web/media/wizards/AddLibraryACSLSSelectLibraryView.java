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

// ident	$Id: AddLibraryACSLSSelectLibraryView.java,v 1.10 2008/12/16 00:12:15 am143972 Exp $

package com.sun.netstorage.samqfs.web.media.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.web.ui.view.html.CCRadioButton;

import com.sun.web.ui.view.wizard.CCWizardPage;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;

import java.util.HashMap;
import java.util.Iterator;

/**
 * A ContainerView object for the pagelet of Add Library Wizard
 * ACSLS Select Library View.
 *
 */
public class AddLibraryACSLSSelectLibraryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME   = "AddLibraryACSLSSelectLibrary";

    // Child view names (i.e. display fields).
    public static final String CHILD_ACTIONTABLE    = "SelectLibraryTable";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";

    private CCActionTableModel tableModel;
    private boolean error = false, previousError = false;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public AddLibraryACSLSSelectLibraryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public AddLibraryACSLSSelectLibraryView(
        View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        createActionTableHeader(wizardModel);

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Child manipulation methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ACTIONTABLE, CCActionTable.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        tableModel.registerChildren(this);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_ACTIONTABLE)) {
            child = new CCActionTable(this, tableModel, name);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild = new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_ERROR);
            child = (View) myChild;
        } else if (name.equals(CHILD_ERROR)) {
            if (error) {
                child = new CCHiddenField(
                    this, name, Constants.Wizard.EXCEPTION);
            } else {
                child = new CCHiddenField(this, name, Constants.Wizard.SUCCESS);
            }
        } else if (tableModel.isChildSupported(name)) {
            child = tableModel.createChild(this, name);
        } else {
            throw new IllegalArgumentException(
                "AddLibraryACSLSSelectLibraryView: Invalid child name ["
                + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return child;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String url = null;
        if (!previousError) {
            url = "/jsp/media/wizards/AddLibraryACSLSSelectLibrary.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }

        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        try {
            createActionTableModel(wizardModel);
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "AddLibraryACSLSSelectLibraryView()",
                "Failed to populate ACSLS Library Information",
                getServerName());
            error = true;
            SamUtil.setErrorAlert(
                this,
                AddLibraryACSLSSelectLibraryView.CHILD_ALERT,
                "AddLibrary.acsls.selectlibrary.error.populate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        // Disable Tooltip
        CCActionTable myTable = (CCActionTable) getChild(CHILD_ACTIONTABLE);
        CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
            CCActionTable.CHILD_SELECTION_RADIOBUTTON);
        myRadio.setTitle("");
        myRadio.setTitleDisabled("");

        showErrorIfNeeded(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private void createActionTableHeader(SamWizardModel wizardModel) {
        tableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/media/wizards/AddLibraryACSLSSelectLibraryTable.xml");
        tableModel.clear();

        tableModel.setActionValue(
            "SerialColumn",
            "AddLibrary.acsls.selectLibrary.selectlibrarytable.heading.serial");
        tableModel.setActionValue(
            "TypeColumn",
            "AddLibrary.acsls.selectLibrary.selectlibrarytable.heading.type");

        // set table title
        String acslsHostName = (String)
            wizardModel.getValue(AddLibrarySelectTypeView.ACSLS_HOST_NAME);
        tableModel.setTitle(
            SamUtil.getResourceString(
                "AddLibrary.acsls.selectlibrary.tabletitle", acslsHostName));
    }

    private void createActionTableModel(SamWizardModel wizardModel)
        throws SamFSException {
        HashMap libraryMap = createLibraryHashMap();

        int tableIndex = 0;
        Iterator it    = libraryMap.keySet().iterator();

        while (it.hasNext()) {
            if (tableIndex > 0) {
                tableModel.appendRow();
            }

            String key   = (String) it.next();
            String value = (String) libraryMap.get(key);

            tableModel.setValue("SerialText", key);
            tableModel.setValue("SerialHidden", key);

            String [] strArray  = value.split(ServerUtil.delimitor);
            String [] typeArray = strArray[1].split(",");
            NonSyncStringBuffer typeBuf = new NonSyncStringBuffer();

            for (int i = 0; i < typeArray.length; i++) {
                int mediaType = Integer.parseInt(typeArray[i]);
                if (typeBuf.length() != 0) {
                    typeBuf.append(", ");
                }
                typeBuf.append(SamUtil.getMediaTypeString(mediaType));
            }
            tableModel.setValue("TypeText", typeBuf.toString());
            tableModel.setValue("TypeHidden", typeBuf.toString());

            String selectedSerialNo = (String)
                wizardModel.getValue(AddLibraryImpl.SA_ACSLS_SERIAL_NO);
            if (key.equals(selectedSerialNo)) {
                tableModel.setRowSelected(true);
            }

            tableIndex++;
        }
    }

    /**
     * This hashMap is built to prepare the discovered STK ACSLS
     * library information.  discoverSTKLibraries(conns) returns an array of
     * Library, but we do not show each library as a separate item in the
     * action table.  Each entry in the action table represents ONE physical
     * libraries that may have 2 or more library objects but with different
     * media types.  At the end of the day user may create two VIRTUAL libraries
     * if the selected physical libraries contain 2 media types.
     *
     * The HashMap structure will be constructed in the following way:
     *
     * Key   => Serial Number of the Library
     * Value =>
     *  A String with the following format: C###TYPE1,TYPE2
     *  C ==> Count of Virtual Libraries
     *  TYPE1,2,etc ==> Media Type of each VIRTUAL libraries
     */
    private HashMap createLibraryHashMap() throws SamFSException {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        HashMap libraryMap = new HashMap();
        Library [] myDiscoveredLibraries =
            (Library []) wizardModel.getValue(
                AddLibraryImpl.SA_DISCOVERD_STK_LIBRARY_ARRAY);

        for (int i = 0; i < myDiscoveredLibraries.length; i++) {
            String serialNo = myDiscoveredLibraries[i].getSerialNo();
            if (libraryMap.containsKey(serialNo)) {
                libraryMap.put(
                    serialNo,
                    createUpdatedValue(
                        (String) libraryMap.get(serialNo),
                        getMediaType(myDiscoveredLibraries[i])));
            } else {
                libraryMap.put(
                    serialNo,
                    new NonSyncStringBuffer("1").append(ServerUtil.delimitor).
                        append(getMediaType(myDiscoveredLibraries[i])).
                        toString());
            }
        }

        return libraryMap;
    }

    /**
     * Helper function of createLibraryHashMap()
     */
    private String createUpdatedValue(String entry, int mediaType) {
        String [] entryArray = entry.split(ServerUtil.delimitor);
        int newCount = Integer.parseInt(entryArray[0]) + 1;
        return
            new NonSyncStringBuffer().append(newCount).
                append(ServerUtil.delimitor).
                append(entryArray[1]).append(
                (entryArray[1].length() == 0) ? "" : ",").append(mediaType).
                toString();
    }

    /**
     * Helper function of createLibraryHashMap()
     */
    private int getMediaType(Library myLibrary) throws SamFSException {
        Drive [] myDrives = myLibrary.getDrives();
        if (myDrives != null && myDrives.length != 0) {
            return myDrives[0].getEquipType();
        } else {
            throw new SamFSException(null, -2516);
        }
    }

    private String getServerName() {
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        return (String) wm.getValue(Constants.Wizard.SERVER_NAME);
    }

    private void showErrorIfNeeded(SamWizardModel wm) {
        if (error) {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.EXCEPTION);
        } else {
            ((CCHiddenField) getChild(CHILD_ERROR)).setValue(
                Constants.Wizard.SUCCESS);
        }

        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "AddLibrary.error.carryover";
            previousError = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                previousError =
                    !errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT);
            }

            if (previousError) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    getServerName());
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }
}
