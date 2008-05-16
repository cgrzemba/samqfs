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

// ident	$Id: FSArchivePoliciesModel.java,v 1.23 2008/05/16 18:38:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.ArrayList;

/**
 * A CCActionTableModel for the FSArchivePolicies Action table.
 */

public final class FSArchivePoliciesModel extends CCActionTableModel {

    protected String serverName;
    protected String fsName;

    // policy:criteria pairs that can be reordered, separated by comma
    private String reorderCriteriaString = "";
    // policy:criteria pairs that can NOT be reordered, separated by comma
    private String nonReorderCriteriaString = "";
    // number of criteria that can be reordered
    private int numReorderCriteria = 0;

    public FSArchivePoliciesModel(String serverName, String fsName) {
        super(RequestManager.getRequestContext().getServletContext(),
             "/jsp/fs/FSArchivePoliciesTable.xml");
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        this.serverName = serverName;
        this.fsName = fsName;
        initModel();
        initProductName();
        TraceUtil.trace3("Exiting");
    }

    private void initModel() {

        TraceUtil.trace3("Entering");
        // create action buttons
        setActionValue("SamQFSWizardNewPolicyButton",
            "FSArchivePolicies.button.AddPolicy");
        setActionValue("AddCriteriaButton",
            "FSArchivePolicies.button.AddCriteria");
        setActionValue("RemoveButton", "FSArchivePolicies.button.Remove");
        setActionValue("ReorderButton", "FSArchivePolicies.button.Reorder");

        // create column headings
        setActionValue("PositionColumn",
            "FSArchivePolicies.heading.Position");
        setActionValue("PolicyNameColumn",
            "FSArchivePolicies.heading.PolicyName");
        setActionValue("StartingDirectoryColumn",
            "FSArchivePolicies.heading.StartingDirectory");
        setActionValue("NameColumn",
            "FSArchivePolicies.heading.Name");
        setActionValue("OwnerColumn",
            "FSArchivePolicies.heading.Owner");
        setActionValue("GroupColumn",
            "FSArchivePolicies.heading.Group");
        setActionValue("MinSizeColumn",
            "FSArchivePolicies.heading.MinSize");
        setActionValue("MaxSizeColumn",
            "FSArchivePolicies.heading.MaxSize");
        setActionValue("AccessAgeColumn",
            "FSArchivePolicies.heading.AccessAge");
        setActionValue("ArchiveAgeColumn",
            "FSArchivePolicies.heading.ArchiveAge");
        setActionValue("MediaTypeColumn", "archiving.media.type");
        TraceUtil.trace3("Exiting");
    }

    private void initProductName() {
        TraceUtil.trace3("Entering");
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    // fill up the table with data
    public void initModelRows() throws SamFSException {
        TraceUtil.trace3("Entering");
        clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        FileSystem fs =
            sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        ArchivePolCriteria[] policyCriteria = fs.getArchivePolCriteriaForFS();

        if (policyCriteria == null) {
            TraceUtil.trace3("Exiting");
            return;
        }

        StringBuffer reorderStringBuffer =
            new StringBuffer();
        StringBuffer nonReorderStringBuffer =
            new StringBuffer();

        ArrayList globalCriteria = new ArrayList();
        int index = 0;

        for (int i = 0; i < policyCriteria.length; i++) {

            TraceUtil.trace3("policy = " +
                policyCriteria[i].getArchivePolicy().getPolicyName());
            TraceUtil.trace3("; criteria #= " + policyCriteria[i].getIndex());

            ArchivePolCriteriaProp prop =
                policyCriteria[i].getArchivePolCriteriaProperties();

            TraceUtil.trace3("Global ? : " + prop.isGlobal());
            if (prop.isGlobal()) {
                // global criteria will come (when sorted by position) after
                // all non-global custom and no_archive criteria, and before
                // the default policy
                globalCriteria.add(policyCriteria[i]);
            } else {
                if (index > 0) {
                    reorderStringBuffer.append(',');
                }
                String policyCriteriaStr =
                    processPolicyCriteria(policyCriteria[i], index++);
                reorderStringBuffer.append(policyCriteriaStr);
            }
        }

        reorderCriteriaString = reorderStringBuffer.toString();
        numReorderCriteria = index;

        // now process global criteria
        int size = globalCriteria.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) {
                nonReorderStringBuffer.append(',');
            }
            ArchivePolCriteria criteria =
                (ArchivePolCriteria) globalCriteria.get(i);
            String policyCriteriaStr =
                processPolicyCriteria(criteria, index++);
            nonReorderStringBuffer.append(policyCriteriaStr);
        }

        // now process default policy
        ArchivePolicy defaultPolicy =
            sysModel.getSamQFSSystemArchiveManager().getArchivePolicy(fsName);
        if (defaultPolicy != null) {
            ArchivePolCriteria[] criteria =
                defaultPolicy.getArchivePolCriteria();
            for (int i = 0; i < criteria.length; i++) {
                if ((size + i) > 0) {
                    nonReorderStringBuffer.append(',');
                }
                String policyCriteriaStr =
                    processPolicyCriteria(criteria[i], index++);
                nonReorderStringBuffer.append(policyCriteriaStr);
            }
        }

        nonReorderCriteriaString = nonReorderStringBuffer.toString();
        TraceUtil.trace3("Exiting");
    }

    protected String processPolicyCriteria(ArchivePolCriteria policyCriteria,
                                           int index)  throws SamFSException {

        String  position = null,
                policyName = null,
                startingDir = null,
                name = null,
                owner = null,
                group = null;
        int criteriaNumber,
            minSizeUnit = 0,
            maxSizeUnit = 0,
            accessAgeUnit = 0;
        long minSize = 0,
             maxSize = 0,
             accessAge = 0;
        StringBuffer archiveAgeBuf = new StringBuffer();

        boolean isDefaultPolicy  = false;
        boolean isGlobalCriteria  = false;

        ArchivePolicy policy = policyCriteria.getArchivePolicy();
        policyName = policy.getPolicyName();

        position = Integer.toString(index + 1);

        ArchivePolCriteriaProp prop =
            policyCriteria.getArchivePolCriteriaProperties();

        if (prop != null) {
            startingDir = prop.getStartingDir();
            name = prop.getNamePattern();
            owner = prop.getOwner();
            group = prop.getGroup();
            minSize = prop.getMinSize();
            minSizeUnit = prop.getMinSizeUnit();
            maxSize = prop.getMaxSize();
            maxSizeUnit = prop.getMaxSizeUnit();
            accessAge = prop.getAccessAge();
            accessAgeUnit = prop.getAccessAgeUnit();
            isGlobalCriteria = prop.isGlobal();
        }

        short policyType = policy.getPolicyType();
        isDefaultPolicy = policyType == ArSet.AR_SET_TYPE_DEFAULT;

        criteriaNumber = policyCriteria.getIndex();

        ArchivePolCriteriaCopy[] copies =
            policyCriteria.getArchivePolCriteriaCopies();
        if (copies != null) {
            for (int j = 0; j < copies.length; j++) {
                archiveAgeBuf.append("copy ")
                    .append(copies[j].getArchivePolCriteriaCopyNumber())
                    .append(": ")
                    .append(copies[j].getArchiveAge())
                    .append(" ")
                    .append(SamUtil.getTimeUnitL10NString(
                        copies[j].getArchiveAgeUnit()))
                    .append("<br>");
            }
        }

        if (index > 0) {
            appendRow();
        }

        String policyCriteriaStr =
            new StringBuffer(policyName)
                .append(':')
                .append(criteriaNumber).toString();

        setValue("Position", position);
        if (isDefaultPolicy) {
            setValue("GlobalImage", Constants.Image.ICON_REQUIRED);
            setValue("DefaultImage", Constants.Image.ICON_REQUIRED);
        } else if (isGlobalCriteria) {
            setValue("GlobalImage", Constants.Image.ICON_REQUIRED);
            setValue("DefaultImage", Constants.Image.ICON_BLANK);
        } else {
            setValue("GlobalImage", Constants.Image.ICON_BLANK);
            setValue("DefaultImage", Constants.Image.ICON_BLANK);
        }
        setValue("PolicyName", policyName);
        setValue("PolicyHref",
            new StringBuffer(policyName).
                append(':').
                append(policyType).toString());
        setValue("PolicyNameHiddenField", policyName);
        setValue("CriteriaNumberHiddenField", new Integer(criteriaNumber));
        setValue("IsGlobalCriteriaHiddenField",
            isGlobalCriteria ? "1" : "0");
        setValue("ArchiveAge", archiveAgeBuf.toString());

        if (isDefaultPolicy) {
            setValue("StartingDirectory", "");
            setValue("Name", "");
            setValue("Owner", "");
            setValue("Group", "");
            setValue("MinSize", "");
            setValue("MaxSize", "");
            setValue("AccessAge", "");
            // default policy links to policy details page
            setValue("CriteriaHref",
                new StringBuffer(policyName).
                    append(':').
                    append(policyType).toString());
        } else {
            setValue("StartingDirectory", startingDir);
            setValue("Name", name);
            setValue("Owner", owner);
            setValue("Group", group);
            setValue("MinSize",
                     PolicyUtil.getSizeString(minSize, minSizeUnit));
            setValue("MaxSize",
                     PolicyUtil.getSizeString(maxSize, maxSizeUnit));

            // Do not show access age if it is -1
            if (accessAge > 0) {
                setValue("AccessAge",
                    new StringBuffer()
                        .append(accessAge).append(" ")
                        .append(SamUtil.getTimeUnitL10NString(accessAgeUnit)));
            } else {
                setValue("AccessAge", "");
            }

            // others link to criteria details page
            setValue("CriteriaHref",
                new StringBuffer(policyName).
                    append(':').
                    append(policyType).
                    append(':').
                    append(criteriaNumber).toString());
        }
        // populate the media type column
        setValue("MediaType", PolicyUtil.getMediaTypeString(policyCriteria));

        return policyCriteriaStr;
    }

    public String getReorderCriteriaString() {
        return reorderCriteriaString;
    }

    public String getNonReorderCriteriaString() {
        return nonReorderCriteriaString;
    }

    public int getNumReorderCriteria() {
        return numReorderCriteria;
    }
}
