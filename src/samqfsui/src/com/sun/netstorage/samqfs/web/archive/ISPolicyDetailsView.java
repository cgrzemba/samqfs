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

// ident	$Id: ISPolicyDetailsView.java,v 1.12 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.DefaultModel;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.util.ArrayList;
import java.util.List;

public class ISPolicyDetailsView extends RequestHandlingViewBase
    implements CCPagelet {

    // labels
    private static final String LABEL_SUFFIX = "Label";

    // tiled view
    private static final String TILED_VIEW = "ISPolicyDetailsTiledView";

    // child views
    private static final String COPY_LIST = "availableCopyList";
    private static final String TV_PREFIX = "tiledViewPrefix";
    private static final String UI_RESULT = "uiResult";
    private static final String INSTRUCTION = "instruction";
    private static final String MIGRATE_TO_POOL = "migrateToPool";
    private static final String MIGRATE_FROM_POOL = "migrateFromPool";

    public ISPolicyDetailsView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(TILED_VIEW, ISPolicyDetailsTiledView.class);
        registerChild(TV_PREFIX, CCHiddenField.class);
        registerChild(COPY_LIST, CCHiddenField.class);
        registerChild(MIGRATE_TO_POOL, CCDropDownMenu.class);
        registerChild(MIGRATE_FROM_POOL, CCDropDownMenu.class);
        registerChild(UI_RESULT, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(INSTRUCTION)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.endsWith(LABEL_SUFFIX)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(TILED_VIEW)) {
            return new ISPolicyDetailsTiledView(this, name);
        } else if (name.equals(COPY_LIST) ||
                   name.equals(UI_RESULT) ||
                   name.equals(TV_PREFIX)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(MIGRATE_TO_POOL) ||
                   name.equals(MIGRATE_FROM_POOL)) {
            return new CCDropDownMenu(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String policyName = parent.getPolicyName();

        // policy description
        ((CCStaticTextField)getChild(INSTRUCTION)).setValue(
            SamUtil.getResourceString("archiving.policy.details.instruction",
                                      policyName));
        // populate stage & release drop downs
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MIGRATE_TO_POOL);
        dropDown.setOptions(new OptionList(
                                SelectableGroupHelper.MigrateToPool.labels,
                                SelectableGroupHelper.MigrateToPool.values));

        dropDown = (CCDropDownMenu)getChild(MIGRATE_FROM_POOL);
        dropDown.setOptions(new OptionList(
                                SelectableGroupHelper.MigrateFromPool.labels,
                                SelectableGroupHelper.MigrateFromPool.values));

        // get a list of the available copies
        ((CCHiddenField)getChild(COPY_LIST))
            .setValue(getAvailableCopyListString());

        ((CCHiddenField)getChild(TV_PREFIX)).setValue(TILED_VIEW);

        TraceUtil.trace3("Exiting");
    }

    /**
     * return an encoded, semi-colon delimited, string of the all existing
     * copies in this policy
     */
    private String getAvailableCopyListString() {
        TraceUtil.trace3("Entering");

        ISPolicyDetailsViewBean parent = (ISPolicyDetailsViewBean)
            getParentViewBean();

        String copyList = "";
        String serverName = parent.getServerName();
        try {
            ArchiveCopy [] copy = SamUtil.getModel(serverName)
                .getSamQFSSystemArchiveManager()
                .getArchivePolicy(parent.getPolicyName()).getArchiveCopies();

            for (int i = 0; copy != null && i < copy.length; i++) {
                copyList = copyList
                    .concat(Integer.toString(copy[i].getCopyNumber()))
                    .concat(";");
            }

            int index = copyList.lastIndexOf(";");
            if (index > 0)
                copyList = copyList.substring(0, index);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "getAvailableCopyListString",
                                     "unable to retrieve available copies",
                                     serverName);
        }

        TraceUtil.trace3("Exiting");
        return copyList;
    }

    /**
     * takes a string of the form 1:2 and returns an int array
     */
    protected int [] getCopyNumberArray(String s) {
        String [] raw = s.split(":");

        int [] result = new int[raw.length];
        for (int i = 0; i < raw.length; i++) {
            result[i] = Integer.parseInt(raw[i]);
        }

        return result;
    }

    /**
     * the uiResult looks like this : n=2&m=3&d=1
     * [0][...] = new, [1][...] = modified, [2][...] deleted
     */
    protected int [][] parseUIResult(String uiResult) {
        int [][] copyInfo = new int[3][];

        String [] s = uiResult.split("&");

        // new copies
        String [] ns = s[0].split("=");
        int [] newCopies = null;
        if (ns.length == 2) {
            newCopies = getCopyNumberArray(ns[1]);
        }
        copyInfo[0] = newCopies;

        // modified copies
        String [] ms = s[1].split("=");
        int [] modifiedCopies = null;
        if (ms.length == 2) {
            modifiedCopies = getCopyNumberArray(ms[1]);
        }
        copyInfo[1] = modifiedCopies;

        // deleted copies
        String [] ds = s[2].split("=");
        int [] deletedCopies = null;
        if (ds.length == 2) {
            deletedCopies = getCopyNumberArray(ds[1]);
        }
        copyInfo[2] = deletedCopies;

        // return the copy array
        return copyInfo;
    }

    /**
     * add new copies
     */
    private List addNewCopy(int [] copyNumber)
        throws SamFSException, ModelControlException {
        List errors = new ArrayList();
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        SamQFSSystemArchiveManager archiveManager = SamUtil
            .getModel(parent.getServerName()).getSamQFSSystemArchiveManager();
        ArchivePolicy thePolicy =
            archiveManager.getArchivePolicy(parent.getPolicyName());

        for (int i = 0; i < copyNumber.length; i++) {
            ArchiveCopyGUIWrapper wrapper =
                archiveManager.getArchiveCopyGUIWrapper();
            errors = validateUserInput(copyNumber[i],
                                       wrapper.getArchiveCopy(),
                                       wrapper.getArchivePolCriteriaCopy());

            // if no errors encountered add the new copy
            thePolicy.addArchiveCopy(wrapper);
        }
        return errors;
    }

    private List saveModifiedCopy(int [] copyNumber)
        throws SamFSException, ModelControlException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = parent.getPolicyName();

        ArchivePolicy thePolicy = SamUtil.getModel(serverName)
            .getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
        for (int i = 0; i < copyNumber.length; i++) {
            ArchiveCopy theCopy = thePolicy.getArchiveCopy(copyNumber[i]);

            // retrieve the first criteria of the policy
            ArchivePolCriteria theCriteria =
                thePolicy.getArchivePolCriteria(0);
            ArchivePolCriteriaCopy theCriteriaCopy = PolicyUtil.
                getArchivePolCriteriaCopy(theCriteria, copyNumber[i]);

            List e = validateUserInput(copyNumber[i], theCopy, theCriteriaCopy);
            if (e.size() > 0) {
                errors.addAll(e);
            }
        }

        TraceUtil.trace3("Exiting");
        return errors;
    }

    private List deleteCopy(int [] copyNumber) throws SamFSException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();

        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        SamQFSSystemArchiveManager archiveManager = SamUtil
            .getModel(parent.getServerName())
            .getSamQFSSystemArchiveManager();
        ArchivePolicy thePolicy =
            archiveManager.getArchivePolicy(parent.getPolicyName());
        for (int i = 0; i < copyNumber.length; i++) {
            thePolicy.deleteArchiveCopy(copyNumber[i]);
        }

        TraceUtil.trace3("Exiting");
        return errors;
    }

    private List validateUserInput(int copyNumber,
                                   ArchiveCopy theCopy,
                                   ArchivePolCriteriaCopy theCriteriaCopy)
        throws SamFSException, ModelControlException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();

        // get the archive manager
        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        SamQFSSystemArchiveManager archiveManager = SamUtil
            .getModel(parent.getServerName())
            .getSamQFSSystemArchiveManager();

        // get the tiled view's model
        ISPolicyDetailsTiledView tiledView =
            (ISPolicyDetailsTiledView)getChild(TILED_VIEW);
        DefaultModel model = (DefaultModel)tiledView.getPrimaryModel();

        // set the model to the right tile
        model.setLocation(copyNumber - 1); // tile index = copy no - 1

        // copy time
        String sCopyTime = (String)model.getValue(tiledView.COPY_TIME);
        long copyTime = Integer.parseInt(sCopyTime);
        int copyTimeUnit =
            Integer.parseInt((String)model.getValue(tiledView.COPY_TIME_UNIT));
        theCriteriaCopy.setArchiveAge(copyTime);
        theCriteriaCopy.setArchiveAgeUnit(copyTimeUnit);

        // never expire
        String neverExpire = (String)model.getValue(tiledView.NEVER_EXPIRE);
        long expirationTime = -1;
        int expirationTimeUnit = -1;

        if (neverExpire.equals("false")) {
            // expiration time
            expirationTime = -1;
            expirationTimeUnit = Integer.parseInt(
                (String)model.getValue(tiledView.EXPIRATION_TIME_UNIT));

            String sExpirationTime =
                (String)model.getValue(tiledView.EXPIRATION_TIME);
            if ((sExpirationTime != null) && (sExpirationTime.length() > 0)) {
                expirationTime = Integer.parseInt(sExpirationTime);

                if (expirationTime <= 0) {
                    errors.add(
                      "archiving.policydetails.error.expirationtime.negative");
                }
            }
        }
        theCriteriaCopy.setUnarchiveAge(expirationTime);
        theCriteriaCopy.setUnarchiveAgeUnit(expirationTimeUnit);

        // releaser behavior
        String releaserBehavior = (String)
            model.getValue(tiledView.RELEASER_BEHAVIOR);
        if (releaserBehavior == null) releaserBehavior = "";
        if (releaserBehavior.equals(SelectableGroupHelper.NOVAL)) {
            theCriteriaCopy.setRelease(false);
        } else if (releaserBehavior.equals("true")) {
            theCriteriaCopy.setRelease(true);
            theCriteriaCopy.setNoRelease(false);
        } else if (releaserBehavior.equals("false")) {
            theCriteriaCopy.setRelease(false);
            theCriteriaCopy.setNoRelease(true);
        }

        // recyling
        boolean enableRecycling = Boolean.valueOf(
            (String)model.getValue(tiledView.ENABLE_RECYCLING)).booleanValue();
        theCopy.setIgnoreRecycle(!enableRecycling);

        ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();

        // if this copy is using an allsets vsn map and the user does not wish
        // to create an independent map for it, return
        if (!saveOwnVSNMap(vsnMap))
            return errors;

        // the copy's vsn pools
        String mediaPoolName = null, scratchPoolName = null, poolNames = "";
        String mediaPool = (String)model.getValue(tiledView.MEDIA_POOL);
        if (!mediaPool.equals(SelectableGroupHelper.NOVAL)) {
            String [] token = mediaPool.split(":");
            int mediaType = Integer.parseInt(token[1]);
            vsnMap.setArchiveMediaType(mediaType);
            poolNames = poolNames.concat(token[0]);
        } else {
            errors.add("archiving.policydetails.mediapool.null");
        }

        String scratchPool = (String)model.getValue(tiledView.SCRATCH_POOL);
        if (!scratchPool.equals(SelectableGroupHelper.NOVAL)) {
            String[] token = scratchPool.split(":");
            poolNames = poolNames.concat(",").concat(token[0]);
        }
        vsnMap.setPoolExpression(poolNames);
        vsnMap.setWillBeSaved(true);

        TraceUtil.trace3("Exiting");
        return errors;
    }

    /**
     * handler for the save button
     */
    public List save() throws ModelControlException, SamFSException {
        TraceUtil.trace3("Entering");

        ISPolicyDetailsTiledView tiledView =
            (ISPolicyDetailsTiledView)getChild(TILED_VIEW);
        DefaultModel model = (DefaultModel)tiledView.getPrimaryModel();

        ISPolicyDetailsViewBean parent =
            (ISPolicyDetailsViewBean)getParentViewBean();
        String serverName = parent.getServerName();
        String policyName = parent.getPolicyName();

        // the uiResult looks like this : n=2&m=3&d=1
        // [0][...] = new, [1][...] = modified, [2][...] deleted
        int [] [] uiResult =
            parseUIResult(getDisplayFieldStringValue(UI_RESULT));

        List errors = new ArrayList();
        // add new copies if any have been added
        if (uiResult[0] != null) {
            List e = addNewCopy(uiResult[0]);
            if (e != null) {
                errors.addAll(e);
            }
        }

        // save modifications to existing copies
        if (uiResult[1] != null) {
            List e = saveModifiedCopy(uiResult[1]);
            if (e != null) {
                errors.addAll(e);
            }
        }

        // delete any copies marked for deletion
        if (uiResult[2] != null) {
            List e = deleteCopy(uiResult[2]);
            if (e != null) {
                errors.addAll(e);
            }
        }

        // stage and release options
        int stage = Integer
            .parseInt(getDisplayFieldStringValue(MIGRATE_FROM_POOL));
        int release = Integer
            .parseInt(getDisplayFieldStringValue(MIGRATE_TO_POOL));

        ArchivePolicy thePolicy = SamUtil.getModel(serverName)
            .getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

        // Change the changes to ALL the criteria
        ArchivePolCriteria [] theCriteria = thePolicy.getArchivePolCriteria();
        for (int i = 0; i < theCriteria.length; i++) {
            theCriteria[i].getArchivePolCriteriaProperties()
                .setStageAttributes(stage);
            theCriteria[i].getArchivePolCriteriaProperties()
                .setReleaseAttributes(release);
        }
        TraceUtil.trace3("Exiting");
        return errors;
    }

    private boolean saveOwnVSNMap(ArchiveVSNMap map) {
        return true;
    }

    /**
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String jsp = null;

        int policyType =
            ((ISPolicyDetailsViewBean)getParentViewBean()).getPolicyType();
        if (policyType == ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {
            jsp = "/jsp/archive/BlankPagelet.jsp";
        } else {
           jsp = "/jsp/archive/ISPolicyDetailsPagelet.jsp";
        }

        TraceUtil.trace3("Exiting");
        return jsp;
    }
}
