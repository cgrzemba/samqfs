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

// ident	$Id: ArchivePolicyImpl.java,v 1.29 2008/05/16 18:39:01 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.impl.jni.*;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;

public class ArchivePolicyImpl implements ArchivePolicy {
    public static int MAX_COPY_REGULAR = 4;
    public static int MAX_COPY_ALLSETS = 5;

    private SamQFSSystemModelImpl model = null;
    private String policyName;
    private short policyType = -1;
    private ArrayList polCriteriaList = new ArrayList();
    private ArrayList archiveCopyList = new ArrayList();
    private ArSet jniArchiveSet = null;
    private String description;

    /**
     * if managing a 4.3 server, then we need to convert disk vsn info
     * from the old style (copy params) to the new style (vsn map).
     * this code can be eliminated once UI drops 4.3 support.
     */
    private void diskCParams43ToMap44(ArchiveCopy archiveCopy, String cpName)
        throws SamFSException {

        if (("1.3".compareTo(model.getServerAPIVersion()) > 0) &&
            (archiveCopy != null)) {
            String dvsn = archiveCopy.getDiskArchiveVSN();
            // if a disk vsn is defined on this 4.3 server
            if (SamQFSUtil.isValidString(dvsn)) {
                ArchiveVSNMapImpl map
                    = new ArchiveVSNMapImpl(archiveCopy,
                                            new VSNMap(cpName,
                                                       "dk",
                                                       new String[] { dvsn },
                                                       new String[0]));
                archiveCopy.setArchiveVSNMap(map);
            }
        }
    }

    public ArchivePolicyImpl(SamQFSSystemModelImpl model,
                             ArSet jniArchiveSet) throws SamFSException {

        TraceUtil.trace3("Logic: Enter ArchivePolicyImpl()");

        this.model = model;
        this.jniArchiveSet = jniArchiveSet;

        if ((model != null) && (jniArchiveSet != null)) {

            policyName = jniArchiveSet.getArSetName();
            this.description = jniArchiveSet.getDescription();

            policyType = jniArchiveSet.getArSetType();

            // Set up the criteria cluster.
            // A cluster of criteria consists of a list of criteria that
            // differs only in file system name.
            // Each cluster makes one ArchivePolCriteria from
            // SAM-QFS Manager standpoint.
            Criteria[] crits = jniArchiveSet.getCriteria();
            if ((crits != null) && (crits.length > 0)) {

                ArrayList clusters = new ArrayList();
                for (int i = 0; i < crits.length; i++) {
                    if (crits[i] != null) {
                        // this criteria has not been taken care of yet
                        ArrayList list = new ArrayList();
                        list.add(crits[i]);
                        for (int j = (i+1); j < crits.length; j++) {
                            if ((crits[j] != null) &&
                                (crits[j].sameAs(crits[i]))) {
                                list.add(crits[j]);
                                crits[j] = null; // crits[j] is now processed
                            }
                        }
                        clusters.add(list);
                    } // crits[i] is now processed
                }

                if (clusters.size() > 0) {
                    for (int i = 0; i < clusters.size(); i++) {
                        ArchivePolCriteriaImpl polCrit =
                            new ArchivePolCriteriaImpl(this,
                                                       i,
                                                       (ArrayList)
                                                       clusters.get(i));
                        polCriteriaList.add(polCrit);
                    }
                }
            }

            // Set up the copy objects
            // There are  4 of them in regular case, ALLSETS have 5
            CopyParams[] cparamList = jniArchiveSet.getCopies();
            VSNMap[] mapList = jniArchiveSet.getMaps();
            ArchiveVSNMapImpl map = null;
            ArchiveCopyImpl archiveCopy = null;
            String cpName = null;

            for (int i = 0; i < MAX_COPY_ALLSETS; i++) {
                // There are four cases here
                TraceUtil.trace1("CP: "
                                 + cparamList[i]
                                 + "  map:"
                                 + mapList[i]);

                if ((cparamList[i] != null) && (mapList[i] != null)) {
                    // Case 1
                    // 4.4 generic copy or 4.3 Tape archive copy
                    map = new ArchiveVSNMapImpl(null, mapList[i]);
                    archiveCopy =
                        new ArchiveCopyImpl(this, map, i+1, cparamList[i]);
                    map.setArchiveCopy(archiveCopy);
                    archiveCopyList.add(archiveCopy);
                } else if ((cparamList[i] != null) && (mapList[i] == null)) {
                    // Case 2
                    // 4.4 copy with no map defined or
                    // 4.3 disk archive copy or
                    // archiver.cmd is in error
                    switch (policyType) {
                    case ArSet.AR_SET_TYPE_ALLSETS_PSEUDO:
                        // create empty map if allsets not specifically set
                        cpName = ArchivePolicy.POLICY_NAME_ALLSETS
                            + ((i == MAX_COPY_ALLSETS - 1) ? ""
                                                           : ("." + (i + 1)));
                        map = new ArchiveVSNMapImpl(cpName);
                        TraceUtil.trace3("created allsets policy " + cpName);
                        break;
                    case ArSet.AR_SET_TYPE_DEFAULT:
                    case ArSet.AR_SET_TYPE_GENERAL:
                    case ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT:
                        case ArSet.AR_SET_TYPE_UNASSIGNED:

                    // inherit map from corresponding allsets copy
                        ArchivePolicy allsetsPolicy =
                            model.getSamQFSSystemArchiveManager().
                            getArchivePolicy(ArchivePolicy.POLICY_NAME_ALLSETS);
                        // get the equivalent all sets copy
                        ArchiveCopy matchingAllsetsCopy =
                            allsetsPolicy.getArchiveCopy(i + 1);

                        if (matchingAllsetsCopy != null) { // should always be
                            ArchiveVSNMapImpl origMap = (ArchiveVSNMapImpl)
                                matchingAllsetsCopy.getArchiveVSNMap();
                            if (origMap.isEmpty()) {
                                if (null != (matchingAllsetsCopy =
                                             allsetsPolicy.getArchiveCopy(5)))
                                        origMap = (ArchiveVSNMapImpl)
                                        matchingAllsetsCopy.getArchiveVSNMap();
                            }
                            cpName = policyName + "." + (i + 1);
                            map = new ArchiveVSNMapImpl(origMap, cpName);
                        }
                        break;
                    default:
                        map = null;
                    }

                    archiveCopy =
                        new ArchiveCopyImpl(this, map, i+1, cparamList[i]);
                    if (map != null)
                        map.setArchiveCopy(archiveCopy);

                    String dvsn = archiveCopy.getDiskArchiveVSN();
                    if (SamQFSUtil.isValidString(dvsn)) { // then 4.3
                        DiskVol vol = DiskVol.get(model.getJniContext(), dvsn);
                        if (SamQFSUtil.isValidString(vol.getHost()))
                            archiveCopy.setDiskArchiveVSNHost(vol.getHost());
                        if (SamQFSUtil.isValidString(vol.getPath()))
                            archiveCopy.setDiskArchiveVSNPath(vol.getPath());
                        diskCParams43ToMap44(archiveCopy, cpName);
                    }
                    archiveCopyList.add(archiveCopy);

                } else if ((cparamList[i] == null) && (mapList[i] != null)) {
                    // Case 3
                    // archiver.cmd is in error
                    throw new SamFSException("logic.archiver.cmd.error");
                } else {
                    // Case 4
                    // No copy - nothing to do.
                }
            }
            // Rearchive copy parameters and vsn maps are not handled
            // in SAM-QFS Manager yet.

            diskCParams43ToMap44(archiveCopy, cpName);

        } // for each archive copy

        TraceUtil.trace3("Logic: Exit ArchivePolicyImpl()");
    }

    public String getPolicyName() {
        return policyName;
    }

    public void setPolicyName(String policyName) {
        this.policyName = policyName;
    }

    public String getPolicyDescription() {
        return this.description;
    }

    public void setPolicyDescription(String desc) {
        this.description = desc;
    }

    public SamQFSSystemModel getModel() {
        return model;
    }

    public void setModel(SamQFSSystemModelImpl model) {
        this.model = model;
    }

    public short getPolicyType() {
        return policyType;
    }

    public ArchivePolCriteria[] getArchivePolCriteria() {
        return (ArchivePolCriteria[])
            polCriteriaList.toArray(new ArchivePolCriteria[0]);
    }

    public ArchivePolCriteria getArchivePolCriteria(int index) {
        TraceUtil.trace3("Logic: Enter getArchivePolCriteria(index)");

        ArchivePolCriteria[] crits =  getArchivePolCriteria();
        ArchivePolCriteria crit = null;
        if ((crits != null) && (crits.length > 0)) {
            for (int i = 0; i < crits.length; i++) {
                if (crits[i].getIndex() == index) {
                    crit = crits[i];
                    break;
                }
            }
        }

        TraceUtil.trace3("Logic: Exit getArchivePolCriteria(index)");
        return crit;
    }

    public ArchivePolCriteria getDefaultArchivePolCriteriaForPolicy() {

        TraceUtil.trace3("Logic: Enter " +
                         "getDefaultArchivePolCriteriaForPolicy()");

        ArchivePolCriteriaPropImpl prop =
            new ArchivePolCriteriaPropImpl(null, null);

        ArchiveCopy[] copies = getArchiveCopies();
        ArchivePolCriteriaCopyImpl[] critCopies =
            new ArchivePolCriteriaCopyImpl[copies.length];
        for (int i = 0; i < copies.length; i++) {
            critCopies[i] =
                new ArchivePolCriteriaCopyImpl(null,
                                               null,
                                               copies[i].getCopyNumber());
        }

        TraceUtil.trace3("Logic: Exit " +
                         "getDefaultArchivePolCriteriaForPolicy()");

        return new ArchivePolCriteriaImpl(prop, critCopies);
    }

    // CIS Only used in New Data Class Wizard (without copy information)
    public ArchivePolCriteria
        getDefaultArchivePolCriteriaForPolicy(String className,
                                              String classDescription) {

        TraceUtil.trace3(new StringBuffer("Logic: Enter ")
         .append("getDefaultArchivePolCriteriaForPolicy(className,description)")
                         .toString());

        ArchivePolCriteriaPropImpl prop =
            new ArchivePolCriteriaPropImpl(null,
                                           null,
                                           className,
                                           classDescription);

        ArchiveCopy[] copies = getArchiveCopies();
        ArchivePolCriteriaCopyImpl[] critCopies =
            new ArchivePolCriteriaCopyImpl[copies.length];
        for (int i = 0; i < copies.length; i++) {
            critCopies[i] =
                new ArchivePolCriteriaCopyImpl(null,
                                               null,
                                               copies[i].getCopyNumber());
        }

        TraceUtil.trace3("Logic: Exit " +
                         "getDefaultArchivePolCriteriaForPolicy()");

        return new ArchivePolCriteriaImpl(prop, critCopies);
    }

    /**
     * Comment written in 4.6
     *
     * The following method is used in New Criteria Wizard.  You can see the
     * comments below when the first criteria settings of this policy were
     * copied over and set to the new criteria.  This is done due to the removal
     * of the second step of the wizard that asks for all the copy settings
     * like archive/unarchive age.  The setup now gets a little weird because
     * we are getting a default criteria in the first step of the wizard, while
     * a default Copy is instantiated but is never used within the wizard.
     * In finishStep while addArchivePolCriteria is called, we trash that
     * default copy and push in the one that we create below.
     */
    public void addArchivePolCriteria(ArchivePolCriteria polCriteria,
                                      String[] fsNames) throws SamFSException {

        TraceUtil.trace3("Logic: Enter addArchivePolCriteria()");

        if ((polCriteria != null) && (fsNames != null) &&
            (fsNames.length > 0)) {

            // Added in 4.6 after removing 2nd step in Add Criteria Wizard
            // that takes in Archive & Unarchive age, copy release options
            // Simply copy over the settings from the first criteria of this
            // policy
            ArchivePolCriteria firstCriteria = getArchivePolCriteria(0);
            ArchivePolCriteriaCopy [] copies =
                firstCriteria.getArchivePolCriteriaCopies();
            ArchivePolCriteriaCopy [] newCopies =
                new ArchivePolCriteriaCopy[copies.length];

            for (int i = 0; i < copies.length; i++) {
                newCopies[i] = new ArchivePolCriteriaCopyImpl(null,
                                                              null,
                                                              i + 1);

                newCopies[i].setArchiveAge(copies[i].getArchiveAge());
                newCopies[i].setArchiveAgeUnit(copies[i].getArchiveAgeUnit());
                newCopies[i].setUnarchiveAge(copies[i].getUnarchiveAge());
                newCopies[i]
                    .setUnarchiveAgeUnit(copies[i].getUnarchiveAgeUnit());
                newCopies[i].setNoRelease(copies[i].isNoRelease());
                newCopies[i].setRelease(copies[i].isRelease());
            }

            polCriteria.setArchivePolCriteriaCopies(newCopies);

            // END OF 4.6 addition ///////////////////////////////////////

            ArrayList newCrits = new ArrayList();
            Criteria[] jniCriteria =
                ((ArchivePolCriteriaImpl) polCriteria).getJniCriteria();
            for (int i = 0; i < fsNames.length; i++) {
                Criteria crit = new Criteria(fsNames[i], jniCriteria[0]);
                crit.setSetName(getPolicyName());
                crit.setDescription(
                    polCriteria.getArchivePolCriteriaProperties().
                        getDescription());
                newCrits.add(crit);
            }

            // this needs to be done at this point, so that Copy objects
            // for JNI layer are set up properly for the next part
            ArchivePolCriteriaImpl policyCriteria =
                new ArchivePolCriteriaImpl(this, polCriteriaList.size(),
                                           newCrits);
            polCriteriaList.add(policyCriteria);

            updateCriteria();

            // GUI code should have done this. Talk to Yaping.
            try {
                Archiver.modifyArSet(model.getJniContext(), jniArchiveSet);
            } catch (SamFSException e) {
                model.getSamQFSSystemArchiveManager().
                    getAllArchivePolicies();
                throw e;
            }

            // GUI details page is doing something funny. I took it out
            // of the above and put it in here because of bugid 5110286.
            // Something is wrong with Shadrack's stuff. Need to talk.
            model.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        TraceUtil.trace3("Logic: Exit addArchivePolCriteria()");
    }

    /**
     * Only call this method in 4.5 & below, use ArchiveManager.deleteClass()
     * to delete a class (criteria) for 4.6 and Intellistor
     */
    public void deleteArchivePolCriteria(int criteriaIndex)
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter deleteArchivePolCriteria()");

        for (int i = 0; i < polCriteriaList.size(); i++) {
            ArchivePolCriteriaImpl polCrit = (ArchivePolCriteriaImpl)
                polCriteriaList.get(i);
            if (polCrit.getIndex() == criteriaIndex) {
                // Remove the policy if this criteria is the only criteria
                // in the policy
                if (polCriteriaList.size() == 1) {
                    getModel().getSamQFSSystemArchiveManager().
                        deleteArchivePolicy(policyName);
                } else {
                    polCriteriaList.remove(i);
                }
            }
        }

        updateCriteria();
        Archiver.modifyArSet(model.getJniContext(), jniArchiveSet);
        Archiver.activateCfgThrowWarnings(model.getJniContext());

        TraceUtil.trace3("Logic: Exit deleteArchivePolCriteria()");
    }

    public ArchiveCopy[] getArchiveCopies() {
        return (ArchiveCopy[]) archiveCopyList.toArray(new ArchiveCopy[0]);
    }

    public ArchiveCopy getArchiveCopy(int copyNo) {
        TraceUtil.trace3("Logic: Enter getArchiveCopy(int)");

        ArchiveCopy target = null;
        for (int i = 0; i < archiveCopyList.size(); i++) {
            ArchiveCopy copy = (ArchiveCopy) archiveCopyList.get(i);
            if (copy.getCopyNumber() == copyNo) {
                target = copy;
                break;
            }
        }

        TraceUtil.trace3("Logic: Exit getArchiveCopy(int)");
        return target;
    }

    public void addArchiveCopy(ArchiveCopyGUIWrapper copyWrapper)
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter addArchiveCopy()");

        ArchiveCopy copy = copyWrapper.getArchiveCopy();

        int[] possible = new int[MAX_COPY_REGULAR];
        for (int i = 0; i < MAX_COPY_REGULAR; i++)
            possible[i] = i + 1;
        for (int i = 0; i < archiveCopyList.size(); i++) {
            ArchiveCopy currentCopy = (ArchiveCopy) archiveCopyList.get(i);
            possible[currentCopy.getCopyNumber() - 1] = -1;
        }
        int available = -1;
        for (int i = MAX_COPY_REGULAR; i > 0; i--) {
            if (possible[i-1] != -1) {
                available = possible[i - 1];
            }
        }

        if (available == -1)
            throw new SamFSException("logic.noCopyAvailable");

        boolean incorrectDiskVol = ((SamQFSSystemArchiveManagerImpl)
                                    model.getSamQFSSystemArchiveManager()).
                                    isIncorrectDiskVol(copy);
        if (incorrectDiskVol)
            throw new SamFSException("logic.differentDiskVolExists");

        // diskvols.conf dumping was not supported in 1.0
        ((SamQFSSystemArchiveManagerImpl)
             model.getSamQFSSystemArchiveManager()).createDiskVolInfo(copy);

        copy.setArchivePolicy(this);
        ((ArchiveCopyImpl) copy).setCopyNumber(available);
        archiveCopyList.add(copy);

        ArchivePolCriteriaCopyImpl critCpOrig =
            (ArchivePolCriteriaCopyImpl)
            copyWrapper.getArchivePolCriteriaCopy();

        for (int i = 0; i < polCriteriaList.size(); i++) {
            ArchivePolCriteriaImpl crit =
                (ArchivePolCriteriaImpl) polCriteriaList.get(i);
            ArchivePolCriteriaCopyImpl critCp =
                new ArchivePolCriteriaCopyImpl(crit,
                                               critCpOrig.getJniCopy(),
                                               available);
            crit.addArchivePolCriteriaCopy(critCp);
        }

        updateCriteria();
        updateRegularCopies();

        try {
            Archiver.modifyArSet(model.getJniContext(), jniArchiveSet);
        } catch (SamFSException e) {
            model.getSamQFSSystemArchiveManager().
                getAllArchivePolicies();
            throw e;
        }

        /**
         * revert the mapping done by updateRegularCopies() for 4.3 disk
         * archiving (if any) for this copy
         */
        diskCParams43ToMap44(copy, this.getPolicyName() + "." + available);

        TraceUtil.trace3("Logic: Exit addArchiveCopy()");
    }

    public void deleteArchiveCopy(int copyNo) throws SamFSException {

        TraceUtil.trace3("Logic: Enter deleteArchiveCopy()");

        if ((archiveCopyList.size() == 1) &&
            (((ArchiveCopy) archiveCopyList.get(0)).
             getCopyNumber() == copyNo)) {
            throw new SamFSException("logic.lastCopy");
        }

        for (int i = 0; i < archiveCopyList.size(); i++) {
            ArchiveCopy copy = (ArchiveCopy) archiveCopyList.get(i);
            if (copy.getCopyNumber() == copyNo) {
                archiveCopyList.remove(i);
                break;
            }
        }

        for (int i = 0; i < polCriteriaList.size(); i++) {
            ArchivePolCriteriaImpl crit =
                (ArchivePolCriteriaImpl) polCriteriaList.get(i);
            crit.removeArchivePolCriteriaCopy(copyNo);
        }

        updateCriteria();
        updateRegularCopies();
        Archiver.modifyArSet(model.getJniContext(), jniArchiveSet);
        Archiver.activateCfgThrowWarnings(model.getJniContext());

        TraceUtil.trace3("Logic: Exit deleteArchiveCopy()");
    }

    public void updatePolicy() throws SamFSException {
        updatePolicy(true);
    }

    public void updatePolicy(boolean activate) throws SamFSException {

        TraceUtil.trace3("Logic: Enter updatePolicy()");

        updateCriteria();
        updateRegularCopies();

        Archiver.modifyArSet(model.getJniContext(), jniArchiveSet);

        if (activate)
            Archiver.activateCfgThrowWarnings(model.getJniContext());

        TraceUtil.trace3("Logic: Exit updatePolicy()");
    }

    public ArchivePolCriteria[] getArchivePolCriteriaForFS(String fsName) {

        TraceUtil.trace3("Logic: Enter getArchivePolCriteriaForFS(fsName)");

        ArrayList list = new ArrayList();
        if (SamQFSUtil.isValidString(fsName)) {
            for (int i = 0; i < polCriteriaList.size(); i++) {
                ArchivePolCriteriaImpl polCrit = (ArchivePolCriteriaImpl)
                    polCriteriaList.get(i);
                if (polCrit.isFSPresent(fsName)) {
                    list.add(polCrit);
                }
            }
        }

        TraceUtil.trace3("Logic: Exit getArchivePolCriteriaForFS(fsName)");

        return (ArchivePolCriteria[]) list.toArray(new ArchivePolCriteria[0]);
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Policy Name: ")
            .append(getPolicyName())
            .append("\n")
            .append("Policy Type: ")
            .append(getPolicyType())
            .append("\n");

        ArchivePolCriteria[] crits = getArchivePolCriteria();
        if ((crits != null) && (crits.length > 0)) {
            for (int i = 0; i < crits.length; i++) {
                buf.append("Criteria: \n")
                    .append(crits[i].toString())
                    .append("\n");
            }
        }

        ArchiveCopy[] copies = getArchiveCopies();
        if ((copies != null) && (copies.length > 0)) {
            for (int i = 0; i < copies.length; i++) {
                buf.append("Copy: \n")
                    .append(copies[i].toString())
                    .append("\n");
            }
        }

        return buf.toString();
    }

    private void updateCriteria() {
        TraceUtil.trace3("Logic: Enter updateCriteria()");

        ArrayList newTotalCrits = new ArrayList();
        for (int i = 0; i < polCriteriaList.size(); i++) {
            ArchivePolCriteriaImpl polCrit = (ArchivePolCriteriaImpl)
                polCriteriaList.get(i);
            Criteria[] crits = polCrit.getJniCriteria();
            if (crits != null) {
                for (int j = 0; j < crits.length; j++) {
                    newTotalCrits.add(crits[j]);
                }
            }
        }
        jniArchiveSet.setCriteria((Criteria[])
                                  newTotalCrits.toArray(new Criteria[0]));
        TraceUtil.trace3("Logic: Exit updateCriteria()");
    }

    private void updateRegularCopies() {
        TraceUtil.trace3("Logic: Enter updateRegularCopies()");

        CopyParams[] cParams = new CopyParams[MAX_COPY_ALLSETS];
        VSNMap maps[] = new VSNMap[MAX_COPY_ALLSETS], jniMap;
        for (int i = 0; i < MAX_COPY_ALLSETS; i++) {
            cParams[i] = null;
            maps[i] = null;
        }

        for (int i = 0; i < archiveCopyList.size(); i++) {
            ArchiveCopyImpl copy = (ArchiveCopyImpl) archiveCopyList.get(i);
            int copyNumber = copy.getCopyNumber() - 1;
            cParams[copyNumber] = copy.getJniCopyParams();

            ArchiveVSNMapImpl map =
                (ArchiveVSNMapImpl) copy.getArchiveVSNMap();
            if (map != null) {
                /*
                 * only set jni map if modified by user AND if non-empty
                 */
                jniMap = map.getJniVSNMap();
                if (map.getWillBeSaved() && !jniMap.isEmpty())
                    maps[copyNumber] = jniMap;

                /*
                 * if managing a 4.3 server, then we need to move disk vsn info
                 * from vsn map to copy params.
                 * this code can be eliminated once UI drops 4.3 support.
                 */
                if (("1.3".compareTo(model.getServerAPIVersion()) > 0) &&
                    jniMap.getMediaType().equals("dk")) {
                    String dvsn = null;
                    // if a disk vsn is defined for current copy
                    if (!jniMap.isEmpty() &&
                        SamQFSUtil.isValidString(dvsn = jniMap
                                                 .getVSNNames()[0])) {
                        copy.setDiskArchiveVSN(dvsn);
                        maps[copyNumber] = null;
                    } else
                        TraceUtil.trace1("no disk vsn in dk VSNMap!");
                }
            }
        }

        jniArchiveSet.setCopies(cParams);
        jniArchiveSet.setMaps(maps);

        TraceUtil.trace3("Logic: Exit updateRegularCopies()");
    }
}
