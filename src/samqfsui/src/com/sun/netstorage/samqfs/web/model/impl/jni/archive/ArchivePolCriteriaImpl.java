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

// ident	$Id: ArchivePolCriteriaImpl.java,v 1.3 2008/03/17 14:43:48 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.archive;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import java.util.ArrayList;

/*
 * This interface maps to the ar_set_criteria structure in archive.h
 */
public class ArchivePolCriteriaImpl implements ArchivePolCriteria {
    private ArchivePolicyImpl policy = null;
    private int index = -1;
    private ArrayList fsNameList = new ArrayList();
    private ArchivePolCriteriaPropImpl criteriaProp = null;
    private ArrayList criteriaCopy = new ArrayList();

    /**
     * This constructor is used to create ArchivePolCriteria object that
     * maps to back end.
     */
    public ArchivePolCriteriaImpl(ArchivePolicyImpl policy,
                                  int index,
                                  ArrayList criteriaList)
        throws SamFSException {

        this.policy = policy;
        this.index = index;

        Criteria commonCrit = null;
        if ((criteriaList != null) && (criteriaList.size() > 0)) {
            for (int i = 0; i < criteriaList.size(); i++) {
                if (i == 0)
                    commonCrit = (Criteria) criteriaList.get(i);
                fsNameList.add(((Criteria) criteriaList.get(i)).
                               getFilesysName());
            }
        }

        if (commonCrit == null)
            throw new SamFSException("logic.mismatchLogicBackend");

        // Added in 4.6
        if (commonCrit.getDataClassName() != null) {
            criteriaProp = new ArchivePolCriteriaPropImpl(
                this, commonCrit,
                commonCrit.getDataClassName(), commonCrit.getDescription());
        } else {
            criteriaProp = new ArchivePolCriteriaPropImpl(this, commonCrit);
        }

        Copy [] copies =  commonCrit.getCopies();
        if (copies != null) {
            for (int i = 0; i < copies.length; i++) {
                if (copies[i] != null) {
                    criteriaCopy.add(new ArchivePolCriteriaCopyImpl(this,
                                                                    copies[i],
                                                                    (i + 1)));
                }
            }
        }
    }

    /**
     * this constructor is used to create a scratch ArchivePolCriteria object
     */
    public ArchivePolCriteriaImpl(ArchivePolCriteriaPropImpl criteriaProp,
                                  ArchivePolCriteriaCopyImpl[] critCopies) {

        this.criteriaProp = criteriaProp;
        if ((critCopies != null) && (critCopies.length > 0)) {
            for (int i = 0; i < critCopies.length; i++) {
                criteriaCopy.add(critCopies[i]);
            }
        }
    }

    public Criteria[] getJniCriteria() {
        ArrayList crits = new ArrayList();

        // set up the copy objects and use the same set of copy objects
        // in all the criteria instances
        Copy [] copies = new Copy[ArchivePolicyImpl.MAX_COPY_ALLSETS];
        for (int i = 0; i < ArchivePolicyImpl.MAX_COPY_ALLSETS; i++) {
            copies[i] = null;
        }

        for (int i = 0; i < criteriaCopy.size(); i++) {
            ArchivePolCriteriaCopyImpl cp =
                (ArchivePolCriteriaCopyImpl) criteriaCopy.get(i);
            copies[cp.getArchivePolCriteriaCopyNumber() - 1] = cp.getJniCopy();
            copies[cp.getArchivePolCriteriaCopyNumber() - 1].
                setCopyNumber(cp.getArchivePolCriteriaCopyNumber());
        }

        String policyName = "Unknown";
        if (policy != null) {
            policyName = policy.getPolicyName();
        }
        Criteria crit = null;
        if (criteriaProp != null) {
            crit = criteriaProp.getJniCriteria();
        } else {
            crit = new Criteria(policyName, "Unknown");
        }

        crit.setCopies(copies);
        if (fsNameList.size() == 0) {
            // Return the JNI Criteria objects for the scratch object
            crits.add(crit);
        } else {
            for (int i = 0; i < fsNameList.size(); i++) {
                Criteria fsCrit = new Criteria((String) fsNameList.get(i),
                                               crit);
                crits.add(fsCrit);
            }
        }

        return (Criteria[]) crits.toArray(new Criteria[0]);
    }

    public ArchivePolicy getArchivePolicy() {
        return policy;
    }

    public void setArchivePolicy(ArchivePolicy policy) {
        this.policy = (ArchivePolicyImpl) policy;
    }

    public int getIndex() {
        return index;
    }

    public void setIndex(int index) {
        this.index = index;
    }

    public ArchivePolCriteriaProp getArchivePolCriteriaProperties() {
        return criteriaProp;
    }

    public void setArchivePolCriteriaProperties(ArchivePolCriteriaProp prop) {
        this.criteriaProp = (ArchivePolCriteriaPropImpl) prop;
    }

    public ArchivePolCriteriaCopy[] getArchivePolCriteriaCopies() {
        return (ArchivePolCriteriaCopy[])
            criteriaCopy.toArray(new ArchivePolCriteriaCopy[0]);
    }

    public void setArchivePolCriteriaCopies(ArchivePolCriteriaCopy[] copies) {
        criteriaCopy.clear();
        if ((copies != null) && (copies.length > 0)) {
            for (int i = 0; i < copies.length; i++) {
                criteriaCopy.add(copies[i]);
            }
        }
    }

    public void addArchivePolCriteriaCopy(ArchivePolCriteriaCopy copy) {
        criteriaCopy.add(copy);
    }

    public void removeArchivePolCriteriaCopy(int copyNo) {
        for (int i = 0; i < criteriaCopy.size(); i++) {
            ArchivePolCriteriaCopy cp = (ArchivePolCriteriaCopy)
                criteriaCopy.get(i);
            if (cp.getArchivePolCriteriaCopyNumber() == copyNo) {
                criteriaCopy.remove(i);
                break;
            }
        }
    }

    public FileSystem[] getFileSystemsForCriteria() throws SamFSException {
        ArrayList fsList = new ArrayList();

        if ((policy != null) && (policy.getModel() != null)) {
            FileSystem[] totalFSList = policy.getModel().
                getSamQFSSystemFSManager().getAllFileSystems();
            if (totalFSList != null) {
                for (int i = 0; i < fsNameList.size(); i++) {
                    String name = (String) fsNameList.get(i);
                    for (int j = 0; j < totalFSList.length; j++) {
                        if (name.equals(totalFSList[j].getName())) {
                            fsList.add(totalFSList[j]);
                            break;
                        }
                    }
                }
            }
        }

        return (FileSystem[]) fsList.toArray(new FileSystem[0]);
    }

    public void addFileSystemForCriteria(String fileSystem)
        throws SamFSException {
        addFileSystemForCriteria(fileSystem, true);
    }

    public void addFileSystemForCriteria(String fileSystem, boolean activate)
        throws SamFSException {

        if ((policy != null) && (SamQFSUtil.isValidString(fileSystem))) {
            fsNameList.add(fileSystem);
            policy.updatePolicy(activate);
        }
    }

    public void deleteFileSystemForCriteria(String fileSystem)
        throws SamFSException {
        deleteFileSystemForCriteria(fileSystem, true);
    }

    public void deleteFileSystemForCriteria(String fileSystem,
                                            boolean activate)
        throws SamFSException {

        if ((policy != null) && (SamQFSUtil.isValidString(fileSystem))) {

	    boolean isDefault = this.isForDefaultPolicy();
        if (fsNameList.size() == 1 &&
            fileSystem.equals((String) fsNameList.get(0)) &&
            !isDefault) {
            throw new SamFSException("logic.lastFSForCriteria");
        } else {
            int index = fsNameList.indexOf(fileSystem);
            if (index != -1) {
                fsNameList.remove(index);
            }
            policy.updatePolicy(activate);
        }

        }
    }

    public boolean isLastFSForCriteria(String fileSystem) {
        boolean last = false;
        if ((policy != null) && (SamQFSUtil.isValidString(fileSystem))) {
            if (fsNameList.size() == 1 &&
                fileSystem.equals((String) fsNameList.get(0))) {
                last = true;
            }
        }
        return last;
    }

    public boolean isForDefaultPolicy() {
        boolean isDefault = false;

        if (criteriaProp != null && criteriaProp.isForDefaultPolicy()) {
            isDefault = true;
        }
        return isDefault;
    }

    /*
     * Throws an exception if the named file system is not an explicit
     * default criteria and the file system is the last remaining one
     * for the criteria. If no exception is returned it means that
     * either the fs can be removed from the criteria or for the case
     * of a default policy the policy can be deleted.
     */
    public void canFileSystemBeRemoved(String fileSystem)
        throws SamFSException {

        if ((policy != null) && (SamQFSUtil.isValidString(fileSystem))) {

            if (fsNameList.size() == 1 &&
                fileSystem.equals((String) fsNameList.get(0)) &&
                !criteriaProp.isForDefaultPolicy()) {
                throw new SamFSException("logic.lastFSForCriteria");
            }
        }
    }

    public boolean isFSPresent(String fsName) {

        boolean present = false;

        int index = fsNameList.indexOf(fsName);
        if (index != -1) {
            present = true;
        }

        return present;
    }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Policy Name & Class Description: \n");
        if (getArchivePolicy() != null) {
            buf.append(policy.getPolicyName())
                .append("\n");
        }

        buf.append("Policy criteria index number: ")
            .append(getIndex())
            .append("\n")
            .append("Criteria Property: \n");

        if (criteriaProp != null) {
            buf.append(criteriaProp.toString())
                .append("\n");
        }

        buf.append("Criteria Copies: \n");
        ArchivePolCriteriaCopy[] copies = getArchivePolCriteriaCopies();
        if ((copies != null) && (copies.length > 0)) {
            for (int i = 0; i < copies.length; i++) {
                buf.append(copies[i].toString() + "\n");
            }
        }

        buf.append("Criteria File Systems: \n");
        String[] names = (String[]) fsNameList.toArray(new String[0]);
        if ((names != null) && (names.length > 0)) {
            for (int i = 0; i < names.length; i++) {
                buf.append(names[i])
                    .append("\n");
            }
        }

        return buf.toString();
    }
}
