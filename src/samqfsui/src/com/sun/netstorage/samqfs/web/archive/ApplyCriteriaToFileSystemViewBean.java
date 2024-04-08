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

// ident	$Id: ApplyCriteriaToFileSystemViewBean.java,v 1.15 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import javax.servlet.ServletException;

/**
 * this viewbean is responsible for the 'apply criteria to filesystems popup'
 */
public class ApplyCriteriaToFileSystemViewBean
    extends CommonSecondaryViewBeanBase {

    private static final String PAGE_NAME = "ApplyCriteriaToFileSystem";
    private static final String DEFAULT_URL =
        "/jsp/archive/ApplyCriteriaToFileSystem.jsp";

    // children
    private static final String ALERT = "alert";
    private static final String FS_TABLE = "filesystemTable";
    private static final String PAGE_TITLE = "pageTitle";

    // hidden fields
    private static final String FILENAMES = "filenames";
    private static final String ERROR_MESSAGES = "errorMessages";
    private static final String [] errorMessages =
        {"ApplyPolWizard.page1.errMsg",
         "ApplyPolWizard.page3.errMsg2",
         "ApplyPolWizard.page3.errMsg3"};

    private static final String FS_NAME = "hiddenFSName";

    // FS selection table model
    private CCActionTableModel fsTableModel = null;
    private CCPageTitleModel pageTitleModel = null;

    // useful hidden fields
    private static final String SELECTED_FS = "selected_filesystems";
    private static final String ERROR_FS = "error_no_fs_selected";

    /** constructor */
    public ApplyCriteriaToFileSystemViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.trace3("Entering");

        createFSTableModel();
        createPageTitleModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** register this container view's children */
    public void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(FS_TABLE, CCActionTable.class);
        registerChild(ERROR_MESSAGES, CCHiddenField.class);
        registerChild(SELECTED_FS, CCHiddenField.class);
        registerChild(ERROR_FS, CCHiddenField.class);
        fsTableModel.registerChildren(this);
        pageTitleModel.registerChildren(this);

        TraceUtil.trace3("Exiting");
    }

    /** create a child of this container view */
    public View createChild(String name) {
        if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, pageTitleModel, name);
        } else if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(FS_TABLE)) {
            return new CCActionTable(this, fsTableModel, name);
        } else if (fsTableModel.isChildSupported(name)) {
            return fsTableModel.createChild(this, name);
        } else if (pageTitleModel.isChildSupported(name)) {
            return pageTitleModel.createChild(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(FILENAMES) ||
                   name.equals(ERROR_MESSAGES) ||
                   name.equals(SELECTED_FS) ||
                   name.equals(ERROR_FS)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * parse the request parameters and return the parameter at the
     * requested index.
     *
     * NOTE: the parameter String is of the format:
     * serverName-_-policyName-_-criteriaNumber
     * thus: server name = 0, policy name = 1, criteria number = 3
     *
     * in addition to returning the requested parameter, saves the rest of the
     * parameters in page session to ensure that the parameter string is parsed
     * once.
     */
    private String parseRequestParameters(int index) {

        String psAttributes = RequestManager.getRequestContext().getRequest().
            getParameter(CriteriaDetailsViewBean.PS_ATTRIBUTES);

        String [] parameters = psAttributes.split("-_-");

        // save server name in page session
        setPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME, parameters[0]);

        // save policy name in page session
        setPageSessionAttribute(Constants.Archive.POLICY_NAME, parameters[1]);

        // save criteria number in page session
        Integer ci = new Integer(parameters[2]);
        setPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER, ci);

        // finally return the requested parameter
        return parameters[index];
    }

    /**
     * return the name of the policy we are dealing with
     */
    public String getPolicyName() {
        String policyName = (String)
            getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        if (policyName == null)
            policyName = parseRequestParameters(1);

        return policyName;
    }

    /**
     * return the index of the criteria we are dealing with
     */
    public int getCriteriaNumber() {


        Integer criteriaNumber = (Integer)
            getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        if (criteriaNumber == null)
            criteriaNumber = new Integer(parseRequestParameters(2));

        return criteriaNumber.intValue();
    }

    /**
     * set default display values here to void hefty price of setting them
     * elsewhere.
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(evt);

        // set the column headings
        initializeTableHeaders();

        // populate selection table model
        populateFSTableModel();
        ((CCHiddenField)getChild(ERROR_FS)).setValue(
            SamUtil.getResourceString("archiving.applycriteria.nofs"));
        TraceUtil.trace3("Exiting");
    }

    /*
     * create the FileSystem selection table model
     */
    private void createFSTableModel() {
        this.fsTableModel = new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/ApplyCriteriaToFileSystemTable.xml");

        this.fsTableModel.setSelectionType(CCActionTableModel.MULTIPLE);
    }

    private void createPageTitleModel() {
        this.pageTitleModel = new CCPageTitleModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/archive/ApplyCriteriaToFileSystemPageTitle.xml");
    }

    private void populateFSTableModel() {
        // retrieve the table child and its model
        // use the already assigned model to maintain state information
        CCActionTable table = (CCActionTable)getChild(FS_TABLE);
        CCActionTableModel tableModel = (CCActionTableModel)table.getModel();
        tableModel.clear();

        ArrayList list = getAssignableFileSystems();
        NonSyncStringBuffer buffer = new NonSyncStringBuffer();

        if (list != null) {
            Iterator it = list.iterator();
            int counter = 0;
            while (it.hasNext()) {
                if (counter > 0)
                    tableModel.appendRow();

                FileSystem fs = (FileSystem)it.next();
                tableModel.setValue("fsname", fs.getName());
                tableModel.setValue("hiddenFSName", fs.getName());
                tableModel.setValue("mountpoint", fs.getMountPoint());

                ((CCCheckBox)table.getChild(CCActionTable.
                    CHILD_SELECTION_CHECKBOX + counter++)).setTitle("");

                buffer.append(fs.getName()).append(";");
            }
        }
        ((CCHiddenField)getChild(FILENAMES)).setValue(buffer.toString());
        table.setModel(tableModel);
    }

    private void initializeTableHeaders() {
        fsTableModel.setActionValue("fsnamecol", "archiving.fs.name");
        fsTableModel.setActionValue("mountpointcol", "archiving.fs.mountpoint");
    }

    private ArrayList getAssignableFileSystems() {
        ArrayList assignableFSList = new ArrayList();
        String serverName = getServerName();

        int index = -1;
        try {
            // retrieve the system model
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            // retrieve the policy in question
            String policyName = getPolicyName();
            ArchivePolicy policy = sysModel.getSamQFSSystemArchiveManager().
                getArchivePolicy(policyName);

            // retrieve the currently selected criteria index
            index = getCriteriaNumber();
            ArchivePolCriteria criteria = policy.getArchivePolCriteria(index);

            // file systems assigned to this policy
            FileSystem [] criteriaFS = criteria.getFileSystemsForCriteria();

            // all file systems
            FileSystem [] allFS = sysModel.getSamQFSSystemFSManager().
                getAllFileSystems(FileSystem.ARCHIVING);

            // get the difference between the list and return those
            // filesystems that are not already associated with this criteria
            for (int i = 0; i < allFS.length; i++) {
                String allfs_name = allFS[i].getName();
                boolean found = false;

                for (int j = 0; j < criteriaFS.length && !found; j++) {
                    if (allfs_name.equals(criteriaFS[j].getName())) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    assignableFSList.add(allFS[i]);
            }
            // trim the list to size and return it
            assignableFSList.trimToSize();
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                    this.getClass(),
                                    "getAssibleFileSystems",
                                    "Unable to retrieve files systems",
                                    serverName);
            // show alert
            String cis = SamUtil.getResourceString(
                "archiving.criterianumber",
                new String [] {Integer.toString(index)});
            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  SamUtil.getResourceString(
                                    "archiving.criteria.fsload.failure",
                                    new String [] {cis}),
                                    sfe.getSAMerrno(),
                                    sfe.getMessage(),
                                    serverName);
        }

        return assignableFSList;
    }

    /**
     *  handler for the Submit button.
     *
     * @param rie
     * @throws ServletException
     * @throws IOException
     */
    public void handleSubmitRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        // get the server name
        String serverName = getServerName();
        String policyName = getPolicyName();
        int criteriaIndex = getCriteriaNumber();

        try {
            // retrieve the model
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            // apply the criteria to the selected file systems
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            ArchivePolCriteria theCriteria =
                thePolicy.getArchivePolCriteria(criteriaIndex);

            String [] fs_names =
                getDisplayFieldStringValue(SELECTED_FS).split(";");

            // loop through each selected file system and add the criteria to
            // the fs
            SamFSWarnings warning = null;
            for (int i = 0; i < fs_names.length; i++) {
                try {
                    FileSystem fs = sysModel.
                        getSamQFSSystemFSManager().getFileSystem(fs_names[i]);
                    fs.addPolCriteria(new ArchivePolCriteria[] {theCriteria});
                } catch (SamFSWarnings sfw) {
                    if (warning == null)
                        warning = sfw;
                }
            }

            // if a warning was encountered in the loop, throw it
            if (warning != null)
                throw warning;

            // if we get here, everything worked.
            SamUtil.setInfoAlert(this,
                                 ALERT,
                                 "success.summary",
                                 "archiving.applycriteriatofs.success",
                                 serverName);
            setSubmitSuccessful(true);
        } catch (SamFSWarnings sfw) {
            setSubmitSuccessful(true);

            SamUtil.processException(sfw,
                    getClass(),
                    "handleSubmitRequest",
                    "ArchveConfig.warning",
                    serverName);

                SamUtil.setWarningAlert(this,
                        ALERT,
                        "ArchiveConfig.info.saved",
                        "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                    getClass(),
                    "handleSubmitRequest",
                    "ArchiveConfig.error",
                    serverName);

            SamUtil.setErrorAlert(this,
                    ALERT,
                    "PolFileSystem.error.failedAddFs",
                    smme.getSAMerrno(),
                    smme.getMessage(),
                    serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                    getClass(),
                    "handleSubmitRequest",
                    "ArchiveConfig.error",
                    serverName);

            SamUtil.setErrorAlert(this,
                    ALERT,
                    "PolFileSystem.error.failedAddFs",
                    sfe.getSAMerrno(),
                    sfe.getMessage(),
                    serverName);
        }

        // finally cycle the page
        forwardTo();
        TraceUtil.trace3("Exiting");
    }
}
