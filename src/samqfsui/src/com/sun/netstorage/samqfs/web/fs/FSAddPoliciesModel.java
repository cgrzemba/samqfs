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

// ident	$Id: FSAddPoliciesModel.java,v 1.15 2008/03/17 14:43:32 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
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
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.table.CCActionTable;

/**
 * A CCActionTableModel for the FSAddPolicies Action table.
 */

public final class FSAddPoliciesModel extends CCActionTableModel {

    // keep track of how many rows in the action table
    private int numRows = 0;

    protected String fsName = null;
    protected String serverName = null;

    public FSAddPoliciesModel(String serverName, String fsName) {
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/FSAddPoliciesTable.xml");
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

        // create column headings
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

    public void initModelRows(CCActionTable theTable) throws SamFSException {
        TraceUtil.trace3("Entering");

        clear();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        FileSystem fs =
            sysModel.getSamQFSSystemFSManager().getFileSystem(fsName);
        if (fs == null) {
            throw new SamFSException(null, -1000);
        }

        ArchivePolCriteria[] policyCriteria =
            sysModel.getSamQFSSystemFSManager().getAllAvailablePolCriteria(fs);

        if (policyCriteria == null) {
            TraceUtil.trace3("Exiting");
            return;
        }

        for (int i = 0; i < policyCriteria.length; i++) {
            String  policyName = null,
                    startingDir = null,
                    name = null,
                    owner = null,
                    group = null;
            int criteriaNumber,
                minSizeUnit,
                maxSizeUnit,
                accessAgeUnit;
            long minSize,
                 maxSize,
                 accessAge;
            StringBuffer archiveAgeBuf = new StringBuffer();

            ArchivePolicy policy = policyCriteria[i].getArchivePolicy();
            policyName = policy.getPolicyName();

            if (policyName == null || "".equals(policyName)) {
                continue;
            }

            ArchivePolCriteriaProp prop =
                policyCriteria[i].getArchivePolCriteriaProperties();

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

            criteriaNumber = policyCriteria[i].getIndex();

            ArchivePolCriteriaCopy[] copies =
                policyCriteria[i].getArchivePolCriteriaCopies();
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

            if (i > 0) {
                appendRow();
            }

            String policyCriteriaStr =
                new StringBuffer(policyName)
                    .append(':')
                    .append(criteriaNumber).toString();

            setValue("PolicyName", policyName);
            setValue("PolicyNameHiddenField", policyName);
            setValue("CriteriaNumberHiddenField", new Integer(criteriaNumber));
            setValue("StartingDirectory", startingDir);
            setValue("Name", name);
            setValue("Owner", owner);
            setValue("Group", group);
            setValue("MinSize", PolicyUtil.getSizeString(minSize, minSizeUnit));
            setValue("MaxSize", PolicyUtil.getSizeString(maxSize,
            maxSizeUnit));

            // populate the media type column
            setValue("MediaType",
                     PolicyUtil.getMediaTypeString(policyCriteria[i]));

            String accessage = "";
            if (accessAge > 0) {
                accessage = new StringBuffer()
                    .append(accessAge).append(" ")
                    .append(SamUtil.getTimeUnitL10NString(accessAgeUnit))
                    .toString();
            }

            setValue("AccessAge", accessage);
            setValue("ArchiveAge", archiveAgeBuf.toString());

            // disable tool tips
            ((CCCheckBox)theTable.getChild(CCActionTable.
              CHILD_SELECTION_CHECKBOX + i)).setTitle("");
        }

        TraceUtil.trace3("Exiting");
    }
}
