/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: NewCriteriaWizardImpl.java,v 1.28 2008/03/17 14:43:31 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.WizardResultView;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.model.wizard.WizardInterface;
import java.util.ArrayList;


interface NewCriteriaWizardImplData {
    final String name = "NewCriteriaWizardImpl";
    final String title = "ArchiveWizard.Criteria.title";

    final Class [] pageClass = {
        NewCriteriaMatchCriteria.class,
        NewCriteriaSummary.class,
        WizardResultView.class
    };

    final String [] pageTitle = {
        "NewCriteriaWizard.matchCriteriaPage.title",
        "NewCriteriaWizard.summaryPage.title",
        "wizard.result.steptext"
    };

    final String [][] stepHelp = {
         {"NewCriteriaWizard.matchCriteriaPage.help.text1",
         "NewCriteriaWizard.matchCriteriaPage.help.text2",
         "NewCriteriaWizard.matchCriteriaPage.help.text3",
         "NewCriteriaWizard.matchCriteriaPage.help.text4",
         "NewCriteriaWizard.matchCriteriaPage.help.text5",
         "NewCriteriaWizard.matchCriteriaPage.help.text6",
         "NewCriteriaWizard.matchCriteriaPage.help.text7",
         "NewCriteriaWizard.matchCriteriaPage.help.text8",
         "NewCriteriaWizard.matchCriteriaPage.help.text9"},
         {"NewCriteriaWizard.summaryPage.help.text1"},
         {"wizard.result.help.text1",
          "wizard.result.help.text2"}
    };

    final String [] stepText = {
        "NewCriteriaWizard.matchCriteriaPage.step.text",
        "NewCriteriaWizard.summaryPage.step.text",
        "wizard.result.steptext"
    };

    final String [] stepInstruction = {
        "NewCriteriaWizard.matchCriteriaPage.step.instruction",
        "NewCriteriaWizard.summaryPage.step.instruction",
        "wizard.result.instruction"
    };

    final String [] cancelmsg = {
        "",
        "",
        "",
        "",
        "",
        ""
    };

    final int MATCH_CRITERIA_PAGE = 0;
    final int SUMMARY_PAGE = 1;
    final int RESULT_PAGE = 2;
}

public class NewCriteriaWizardImpl extends SamWizardImpl {

    // wizard constants
    public static final String PAGEMODEL_NAME = "NewCriteriaPageModelName";
    public static final String PAGEMODEL_NAME_PREFIX = "NewCriteriaWizardMode";
    public static final String IMPL_NAME = NewCriteriaWizardImplData.name;
    public static final String IMPL_NAME_PREFIX = "NewCriteriaImpl";
    public static final String IMPL_CLASS_NAME =
        "com.sun.netstorage.samqfs.web.archive.wizards.NewCriteriaWizardImpl";
    public static final String CURRENT_CRITERIA = "DefaultPolicyCriteria";

    // Keep track of the label name and change the label when error occurs
    public static final String VALIDATION_ERROR = "ValidationError";


    public static WizardInterface create(RequestContext requestContext) {
        TraceUtil.initTrace();
        return new NewCriteriaWizardImpl(requestContext);
    }

    public NewCriteriaWizardImpl(RequestContext requestContext) {
        super(requestContext, PAGEMODEL_NAME);

        TraceUtil.trace3("Entering");

        // retrieve and save the server name
        String serverName = requestContext.getRequest().getParameter(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "WizardImpl Const' serverName is ").append(serverName).toString());
        wizardModel.setValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
            serverName);

        // retrieve and save policy name
        String policyName = requestContext.getRequest().getParameter(
            Constants.SessionAttributes.POLICY_NAME);
        TraceUtil.trace3(new NonSyncStringBuffer(
            "WizardImpl Const' policyName is ").append(policyName).toString());
        wizardModel.setValue(
            Constants.SessionAttributes.POLICY_NAME,
            policyName);

        initializeWizard(requestContext);
        initializeWizardControl(requestContext);
        TraceUtil.trace3("Exiting");
    }

    public static CCWizardWindowModel createModel(String cmdChild) {
        return getWizardWindowModel(IMPL_NAME,
                                    NewCriteriaWizardImplData.title,
                                    IMPL_CLASS_NAME,
                                    cmdChild);
    }

    private void initializeWizard(RequestContext requestContext) {
        TraceUtil.trace3("Entering");
        wizardName = NewCriteriaWizardImplData.name;
        wizardTitle = NewCriteriaWizardImplData.title;
        pageClass = NewCriteriaWizardImplData.pageClass;
        pageTitle = NewCriteriaWizardImplData.pageTitle;
        stepHelp = NewCriteriaWizardImplData.stepHelp;
        stepText = NewCriteriaWizardImplData.stepText;
        stepInstruction = NewCriteriaWizardImplData.stepInstruction;

        cancelMsg = NewCriteriaWizardImplData.cancelmsg;

        pages = new int [] {
                 NewCriteriaWizardImplData.MATCH_CRITERIA_PAGE,
                 NewCriteriaWizardImplData.SUMMARY_PAGE,
                 NewCriteriaWizardImplData.RESULT_PAGE };

        setShowResultsPage(true);
        initializeWizardPages(pages);
        TraceUtil.trace3("Exiting");
    }

    public Class getPageClass(String pageId) {
        Class result = super.getPageClass(pageId);
        return result;
    }

    public boolean nextStep(WizardEvent event) {
        String pageId = event.getPageId();
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // make this wizard active
        super.nextStep(event);

        int page = pageIdToPage(pageId);

        if (pages[page] == NewCriteriaWizardImplData.MATCH_CRITERIA_PAGE) {
            return processMatchCriteriaPage(event);
        }

        return true;
    }

    public boolean previousStep(WizardEvent event) {
        String pageId = event.getPageId();
        TraceUtil.trace2(new NonSyncStringBuffer(
            "Entered with pageID = ").append(pageId).toString());

        // make this wizard active
        super.previousStep(event);

        // always return true (show no error message) for previous button
        return true;
    }

    public boolean finishStep(WizardEvent event) {
        if (!super.finishStep(event))
            return true;
        SamQFSSystemModel sysModel = null;

        try {
            sysModel = SamUtil.getModel(getServerName());
            ArchivePolCriteria theCriteria = getCurrentPolicyCriteria();

            // add the file systems
            Object [] fsList = (Object []) wizardModel.getValues(
                NewCriteriaMatchCriteria.APPLY_TO_FS);
            String [] selectedFSList = new String[fsList.length];
            for (int i = 0; i < fsList.length; i++) {
                selectedFSList[i] = (String) fsList[i];
            }

            // now get the policy, update & save it
            ArchivePolicy thePolicy = sysModel.getSamQFSSystemArchiveManager()
                                        .getArchivePolicy(getPolicyName());
            thePolicy.addArchivePolCriteria(theCriteria, selectedFSList);

            LogUtil.info(
                this.getClass(),
                "finishStep",
                new NonSyncStringBuffer(
                    "Done creating new criteria for policy ").append(
                    getPolicyName()).toString());

            wizardModel.setValue(
                Constants.AlertKeys.OPERATION_RESULT,
                Constants.AlertKeys.OPERATION_SUCCESS);
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                "success.summary");
            wizardModel.setValue(
                Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                SamUtil.getResourceString(
                    "NewCriteriaWizard.success.message", getPolicyName()));

            TraceUtil.trace2(new NonSyncStringBuffer(
                "Succesfully created new criteria for policy: ").append(
                getPolicyName()).toString());
            return true;
        } catch (SamFSException ex) {
            boolean multiMsgOccurred = false;
            boolean warningOccurred = false;
            String processMsg = null;

            if (ex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                multiMsgOccurred = true;
            } else if (ex instanceof SamFSWarnings) {
                warningOccurred = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
            } else {
                processMsg = new NonSyncStringBuffer(
                    "Failed to add new criteria for policy ").append(
                    getPolicyName()).toString();
            }

            SamUtil.processException(
                ex,
                this.getClass(),
                "finishStep()",
                processMsg,
                getServerName());

            int errCode = ex.getSAMerrno();
            if (multiMsgOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.error.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else if (warningOccurred) {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_WARNING);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "ArchiveConfig.error");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    "ArchiveConfig.warning.detail");
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            } else {
                wizardModel.setValue(
                    Constants.AlertKeys.OPERATION_RESULT,
                    Constants.AlertKeys.OPERATION_FAILED);
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_SUMMARY,
                    "NewCriteriaWizard.error.summary");
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_RESULT_ALERT_DETAIL,
                    ex.getMessage());
                wizardModel.setValue(
                    Constants.Wizard.DETAIL_CODE, Integer.toString(errCode));
            }
            return true;
        }
    }

    private boolean processMatchCriteriaPage(WizardEvent event) {
        // parameters in this method go to the criteria properties object
        ArchivePolCriteria criteria = getCurrentPolicyCriteria();
        ArchivePolCriteriaProp properties =
            criteria.getArchivePolCriteriaProperties();

        // validate starting directory
        String startingDir = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.STARTING_DIR);
        startingDir = startingDir != null ? startingDir.trim() : "";
        // starting dir can't be blank
        if (startingDir.equals("")) {
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.STARTING_DIR_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error.startingDirEmpty",
                true);
            return false;
        }
        // starting dir can't contain a space
        if (startingDir.indexOf(' ') != -1) {
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.STARTING_DIR_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error.startingDirSpace",
                true);
            return false;
        }
        // starting dir can't be absolute
        if (startingDir.charAt(0) == '/') {
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.STARTING_DIR_LABEL,
            "NewCriteriaWizard.matchCriteriaPage.error.startingDirAbsolutePath",
                true);
            return false;
        }
        properties.setStartingDir(startingDir);
        wizardModel.setValue(NewCriteriaSummary.STARTING_DIR_TEXT, startingDir);

        // validate name pattern
        String namePattern = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.NAME_PATTERN);
        namePattern = namePattern != null ? namePattern.trim() : "";
        // only validate name pattern if a value is type
        if (!namePattern.equals("")) {
            if (namePattern.indexOf(' ') != -1) {
                // no spaces allowed
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.NAME_PATTERN_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.namePattern",
                    true);
                return false;
            }
            if (!PolicyUtil.isValidNamePattern(namePattern)) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.NAME_PATTERN_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.namePattern",
                    true);
                return false;
            }
        }
        properties.setNamePattern(namePattern);
        wizardModel.setValue(NewCriteriaSummary.NAME_PATTERN_TEXT, namePattern);

        // validate min size and max size
        String drivesMin =
            (String)wizardModel.getValue(NewCriteriaMatchCriteria.MINSIZE);
        String drivesMax =
            (String)wizardModel.getValue(NewCriteriaMatchCriteria.MAXSIZE);

        drivesMin = drivesMin != null ? drivesMin.trim() : "";
        drivesMax = drivesMax != null ? drivesMax.trim() : "";

        boolean mn = false;
        boolean mnu = false;
        boolean mx = false;
        boolean mxu = false;

        long min = -1;
        int minu = -1;
        long max = -1;
        int maxu = -1;

        // if only one of min or max is set reject it
        // min without max
        if (!drivesMin.equals("")) {
            String drivesMinSizeUnit = (String)wizardModel.getValue(
                NewCriteriaMatchCriteria.MINSIZE_UNITS);

            // verify minisize
            try {
                min = Long.parseLong(drivesMin);
                if (min < 0)  {
                    setJavascriptErrorMessage(
                        event,
                        NewCriteriaMatchCriteria.MINSIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.minSize",
                        true);
                    return false;
                } else {
                    mn = true;
                    // now that we have a valid size, verify the units
                  if (!SelectableGroupHelper.NOVAL.equals(drivesMinSizeUnit)) {
                        minu = Integer.parseInt(drivesMinSizeUnit);
                        mnu = true;
                        // check that the min size value is within the
                        // acceptable range of 0 bytes and 7813 PB
                        if (PolicyUtil.isOverFlow(min, minu)) {
                            setJavascriptErrorMessage(event,
                                       NewCriteriaMatchCriteria.MINSIZE_LABEL,
                                      "CriteriaDetails.error.overflowminsize",
                                                      true);
                            return false;
                        }
                  } else {
                      setJavascriptErrorMessage(event,
                                        NewCriteriaMatchCriteria.MINSIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.minSizeUnit",
                                                true);
                        return false;
                    }
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.MINSIZE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.minSize",
                    true);
                return false;
            }
        }

        if (!drivesMax.equals("")) {
            // verify the max size
            String drivesMaxSizeUnit = (String)wizardModel.getValue(
                NewCriteriaMatchCriteria.MAXSIZE_UNITS);
            try {
                max = Long.parseLong(drivesMax);
                if (max < 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCriteriaMatchCriteria.MAXSIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.maxSize",
                        true);
                    return false;
                } else {
                    mx = true;
                  if (!SelectableGroupHelper.NOVAL.equals(drivesMaxSizeUnit)) {
                      maxu = Integer.parseInt(drivesMaxSizeUnit);
                      mxu = true;

                      // check that the max size value is within the acceptable
                      // range of 0 bytes to 7813 PB
                      if (PolicyUtil.isOverFlow(max, maxu)) {
                          setJavascriptErrorMessage(event,
                                       NewCriteriaMatchCriteria.MAXSIZE_LABEL,
                                      "CriteriaDetails.error.overflowmaxsize",
                                                    true);
                          return false;
                      }
                  } else {
                      setJavascriptErrorMessage(event,
                                       NewCriteriaMatchCriteria.MAXSIZE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.maxSizeUnit",
                                                true);
                      return false;
                  }
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(event,
                    NewCriteriaMatchCriteria.MAXSIZE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.maxSize",
                    true);
                return false;
            }
        }

        // check if max is really greater than min if both values are defined
        if (!drivesMax.equals("") && !drivesMin.equals("") &&
            !PolicyUtil.isMaxGreaterThanMin(min, minu, max, maxu)) {
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.MINSIZE_LABEL,
                "NewCriteriaWizard.matchCriteriaPage.error.minMaxSize",
                true);
            return false;
        }


        // if we get this far, all four values checked out
        properties.setMinSize(min);
        properties.setMinSizeUnit(minu);
        properties.setMaxSize(max);
        properties.setMaxSizeUnit(maxu);
        wizardModel.setValue(
            NewCriteriaSummary.MIN_SIZE_TEXT,
            min == -1 ?
                "" :
                new NonSyncStringBuffer().
                    append(min).append(" ").
                    append(SamUtil.getSizeUnitL10NString(minu)).toString());
        wizardModel.setValue(
            NewCriteriaSummary.MAX_SIZE_TEXT,
            max == -1 ?
                "" :
                new NonSyncStringBuffer().
                    append(max).append(" ").
                    append(SamUtil.getSizeUnitL10NString(maxu)).toString());

        // Validate Access Age
        String accessAge = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.ACCESS_AGE);
        String accessAgeUnit = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.ACCESS_AGE_UNITS);
        accessAge = accessAge != null ? accessAge.trim() : "";

        if (!accessAge.equals("")) {
            long accessAgeValue = -1;

            // if access Age is defined, a unit must be defined as well
            if (accessAgeUnit.equals(SelectableGroupHelper.NOVAL)) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.ACCESS_AGE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAgeUnit",
                    true);
                return false;
            }

            try {
                // make sure accessAge is a positive integer
                accessAgeValue = Long.parseLong(accessAge);
                if (accessAgeValue <= 0) {
                    setJavascriptErrorMessage(
                        event,
                        NewCriteriaMatchCriteria.ACCESS_AGE_LABEL,
                        "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                        true);
                    return false;
                }
            } catch (NumberFormatException nfe) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.ACCESS_AGE_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.accessAge",
                    true);
                return false;
            }

            properties.setAccessAge(accessAgeValue);
            properties.setAccessAgeUnit(Integer.parseInt(accessAgeUnit));

            wizardModel.setValue(
                NewCriteriaSummary.ACCESS_AGE_TEXT,
                new NonSyncStringBuffer().
                    append(accessAge).append(" ").
                    append(SamUtil.getTimeUnitL10NString(
                        Integer.parseInt(accessAgeUnit))).toString());
        } else {
            // reset access age to -1
            properties.setAccessAge(-1);
            properties.setAccessAgeUnit(-1);

            wizardModel.setValue(
                NewCriteriaSummary.ACCESS_AGE_TEXT, "");
        }

        // validate owner
        String owner = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.OWNER);
        owner = owner != null ? owner.trim() : "";
        if (!owner.equals("")) {
            if (owner.indexOf(' ') != -1 ||
                !PolicyUtil.isUserValid(owner, getServerName())) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.OWNER_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.owner",
                    true);
                return false;
            }
        }
        properties.setOwner(owner);
        wizardModel.setValue(NewCriteriaSummary.OWNER_TEXT, owner);

        // validate group
        String group = (String)
            wizardModel.getValue(NewCriteriaMatchCriteria.GROUP);
        group = group != null ? group.trim() : "";
        if (!group.equals("")) {
            if (group.indexOf(' ') != -1) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.GROUP_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.group",
                    true);
                return false;
            }
            if (!PolicyUtil.isGroupValid(group, getServerName())) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.GROUP_LABEL,
                    "NewCriteriaWizard.matchCriteriaPage.error.groupNotExist",
                    true);
                return false;
            }
        }
        properties.setGroup(group);
        wizardModel.setValue(NewCriteriaSummary.GROUP_TEXT, group);

        // validate staging
        String polName = (String) wizardModel.getValue(
                            Constants.SessionAttributes.POLICY_NAME);
        if (!polName.equals("no_archive")) {
            String stagingStr = (String)
                wizardModel.getValue(NewCriteriaMatchCriteria.STAGING);
            int staging = -1;
            if (!stagingStr.equals(SelectableGroupHelper.NOVAL))
                staging = Integer.parseInt(stagingStr);
            properties.setStageAttributes(staging);
            wizardModel.setValue(
                NewCriteriaSummary.STAGING_TEXT,
                getStagingOptionString(staging));

            // validate releasing
            String releasingStr = (String)
                wizardModel.getValue(NewCriteriaMatchCriteria.RELEASING);
            int releasing =  -1;
            if (!releasingStr.equals(SelectableGroupHelper.NOVAL))
                releasing = Integer.parseInt(releasingStr);
            properties.setReleaseAttributes(releasing);
            wizardModel.setValue(
                NewCriteriaSummary.RELEASING_TEXT,
                getReleasingOptionString(releasing));
        } else {
            wizardModel.setValue(
                NewCriteriaSummary.RELEASING_TEXT, "");
            wizardModel.setValue(
                NewCriteriaSummary.STAGING_TEXT, "");
        }

        //  Apply to FS
        Object [] fsList = (Object []) wizardModel.getValues(
            NewCriteriaMatchCriteria.APPLY_TO_FS);
        String [] selectedFSList = new String[fsList.length];
        if (fsList.length == 0) {
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.APPLY_TO_FS_TEXT,
                "Reservation.page2.errMsg",
                true);
            return false;
        } else {
            StringBuffer fsStringBuf = new StringBuffer();
            for (int i = 0; i < fsList.length; i++) {
                selectedFSList[i] = (String) fsList[i];
                if (fsStringBuf.length() > 0) {
                    fsStringBuf.append("<br>");
                }
                fsStringBuf.append((String) fsList[i]);
            }
            wizardModel.setValue(
                NewCriteriaSummary.APPLY_FS_TEXT, fsStringBuf.toString());
        }

        // check duplicate
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            ArrayList resultList = sysModel.getSamQFSSystemArchiveManager().
                isDuplicateCriteria(criteria, selectedFSList, false);
            String duplicate = (String) resultList.get(0);
            if (duplicate.equals("true")) {
                setJavascriptErrorMessage(
                    event,
                    NewCriteriaMatchCriteria.STARTING_DIR_LABEL,
                    "archiving.criteria.wizard.duplicate",
                    true);
                return false;
            }
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "finishStep()",
                "Failed to retrieve system model and check if dup criteria!",
                getServerName());
            setJavascriptErrorMessage(
                event,
                NewCriteriaMatchCriteria.STARTING_DIR_LABEL,
                "archiving.criteria.wizard.duplicate",
                true);
            return false;
        }

        // if we get this far, the whole page checked out
        return true;
    }

    /**
     * retrieve the current criteria. The first time this method is called, it
     * retrieves the default criteria for the current policy
     */
    private ArchivePolCriteria getCurrentPolicyCriteria() {
        ArchivePolCriteria criteria =
            (ArchivePolCriteria)wizardModel.getValue(CURRENT_CRITERIA);

        // if this is the first its asked for, retrieve it
        if (criteria != null) {
            return criteria;
        }

        try {
            // retrieve the policy
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            ArchivePolicy thePolicy =
                sysModel.getSamQFSSystemArchiveManager().
                    getArchivePolicy(getPolicyName());

            // get the default policy criteria
            criteria = thePolicy.getDefaultArchivePolCriteriaForPolicy();

            // save
            wizardModel.setValue(CURRENT_CRITERIA, criteria);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                         this.getClass(),
                         "Model Error",
                         "Unable to retrieve Policy",
                         getServerName());
        }

        return criteria;
    }

    // helper function to get display string of staging options
    private String getStagingOptionString(int optionValue) {
        String optionString = "NewCriteriaWizard.criteria.staging.auto";
        switch (optionValue) {
            case ArchivePolicy.STAGE_ASSOCIATIVE:
                optionString = "NewCriteriaWizard.criteria.staging.associative";
                break;
            case ArchivePolicy.STAGE_NEVER:
                optionString = "NewCriteriaWizard.criteria.staging.never";
                break;
            case ArchivePolicy.STAGE_DEFAULTS:
                optionString = "NewCriteriaWizard.criteria.staging.defaults";
                break;
        }
        return SamUtil.getResourceString(optionString);
    }

    // helper function to get display string of releasing options
    private String getReleasingOptionString(int optionValue) {
        String optionString = "NewCriteriaWizard.criteria.releasing.reached";
        switch (optionValue) {
            case ArchivePolicy.RELEASE_NEVER:
                optionString = "NewCriteriaWizard.criteria.releasing.never";
                break;
            case ArchivePolicy.RELEASE_PARTIAL:
                optionString = "NewCriteriaWizard.criteria.releasing.partial";
                break;
            case ArchivePolicy.RELEASE_AFTER_ONE:
                optionString = "NewCriteriaWizard.criteria.releasing.onecopy";
                break;
            case ArchivePolicy.RELEASE_DEFAULTS:
                optionString = "NewCriteriaWizard.criteria.releasing.defaults";
                break;

        }
        return SamUtil.getResourceString(optionString);
    }

    private String getServerName() {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private String getPolicyName() {
        String policyName = (String) wizardModel.getValue(
            Constants.SessionAttributes.POLICY_NAME);
        return policyName == null ? "" : policyName;
    }

    /**
     * setJavascriptErrorMessage(wizardEvent, labelName, message, setLabel)
     * labelName = Name of the label of which you want to highlight
     * message   = Javascript message in pop up
     * setLabel  = boolean set to false for previous button
     */
    private void setJavascriptErrorMessage(
        WizardEvent event, String labelName, String message, boolean setLabel) {
        event.setSeverity(WizardEvent.ACKNOWLEDGE);
        event.setErrorMessage(message);
        if (setLabel) {
            wizardModel.setValue(VALIDATION_ERROR, labelName);
        }
    }
}
