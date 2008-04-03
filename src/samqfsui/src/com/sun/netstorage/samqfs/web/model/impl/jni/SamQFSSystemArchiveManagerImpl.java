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

// ident	$Id: SamQFSSystemArchiveManagerImpl.java,v 1.13 2008/04/03 02:21:40 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.FileUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.ArGlobalDirective;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.arc.DrvDirective;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
import com.sun.netstorage.samqfs.mgmt.rec.LibRecParams;
import com.sun.netstorage.samqfs.mgmt.rec.Recycler;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.rel.ReleaserDirective;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.mgmt.stg.StagerParams;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.BufferDirective;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.FSArchiveDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.model.archive.RecycleParams;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.impl.jni.archive.*;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class SamQFSSystemArchiveManagerImpl
    implements SamQFSSystemArchiveManager {

    private static String GLOBAL = "global properties";

    private SamQFSSystemModelImpl theModel = null;
    private HashMap vsnPoolMap = new HashMap();
    private HashMap policyMap = new HashMap();

    public SamQFSSystemArchiveManagerImpl(SamQFSSystemModel model) {
        theModel = (SamQFSSystemModelImpl) model;
    }

    public GlobalArchiveDirective getGlobalDirective() throws SamFSException {
        ArGlobalDirective dir =
            Archiver.getArGlobalDirective(theModel.getJniContext());

        return new GlobalArchiveDirectiveImpl(theModel, dir);
    }

    // this method needs to be called after appropriate setters are
    // called on GlobalArchiveDirective
    public void setGlobalDirective(GlobalArchiveDirective global)
        throws SamFSException {

        global.changeGlobalDirective();
    }

    public FSArchiveDirective[] getFSGeneralArchiveDirective()
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter getFSGeneralArchiveDirective()");

        ArrayList list = new ArrayList();
        ArFSDirective[] jniDirs =
            Archiver.getArFSDirectives(theModel.getJniContext());
        if ((jniDirs != null) && (jniDirs.length > 0)) {
            for (int i = 0; i < jniDirs.length; i++) {
                String fsName = jniDirs[i].getFSName();
                if (!GLOBAL.equals(fsName)) {
                    list.add(new FSArchiveDirectiveImpl(theModel, jniDirs[i]));
                }
            }
        }

        TraceUtil.trace3("Logic: Exit getFSGeneralArchiveDirective()");

        return (FSArchiveDirective[]) list.toArray(new FSArchiveDirective[0]);
    }

    public boolean isValidGroup(String groupName) throws SamFSException {
        return Archiver.isValidGroup(theModel.getJniContext(), groupName);
    }

    public boolean isValidUser(String userName) throws SamFSException {
        return Archiver.isValidUser(theModel.getJniContext(), userName);
    }

    public String [] getAllExplicitlySetPolicyNames() throws SamFSException {
        // get jni sets
        ArchivePolicy [] policy = getAllArchivePolicies();
        ArrayList setNames = new ArrayList();
        for (int i = 0; i < policy.length; i++) {
            if (policy[i].getPolicyType() ==
                ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
                setNames.add(policy[i].getPolicyName());
            }
        }

        return (String [])setNames.toArray(new String[0]);
    }

    public String[] getAllArchivePolicyNames() throws SamFSException {
        String [] names = Archiver.getCriteriaNames(theModel.getJniContext());
        if (names == null)
            names = new String[0];

        return names;
    }

    /**
     * This method has to be called after getAllArchivePolicies is called
     */
    public String[] getAllNonDefaultNonAllSetsArchivePolicyNames()
        throws SamFSException {

        HashMap policyNameMap = new HashMap();

        for (int i = 0; i < policyMap.size(); i++) {
            Iterator it = policyMap.keySet().iterator();
            while (it.hasNext()) {
                String key = (String) it.next();
                ArchivePolicy policy = (ArchivePolicy) policyMap.get(key);
                short policyType = policy.getPolicyType();


                // TODO: Consider taking explicit_default type out if the setup
                // is a wide open point product setup.  Explicit default
                // is a VALID policy in CIS, but we have no visibility
                // to determine if this is just a placeholder for
                // unassigned data classes in a point product setup.
                if (policyType == ArSet.AR_SET_TYPE_GENERAL ||
                    policyType == ArSet.AR_SET_TYPE_NO_ARCHIVE ||
                    policyType == ArSet.AR_SET_TYPE_UNASSIGNED ||
                    policyType == ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
                    if (!policyNameMap.containsKey(key)) {
                        // always make explicit default goes first
                        if (policyType == ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
                            policyNameMap.put("0", key);
                        } else {
                            policyNameMap.put(key, key);
                        }
                    }
                }
            }
        }

        if (policyNameMap.size() == 0) {
            return new String[0];
        } else {
            return (String []) policyNameMap.values().toArray(new String[0]);
        }
    }

    public ArchivePolicy[]  getAllArchivePolicies() throws SamFSException {
        TraceUtil.trace3("Logic: Enter getAllArchivePolicies()");

        ArSet[] jniSets = Archiver.getArSets(theModel.getJniContext());
        policyMap.clear();
        if (jniSets != null) {
            for (int i = 0; i < jniSets.length; i++) {
                policyMap.put(jniSets[i].getArSetName(),
                              new ArchivePolicyImpl(theModel, jniSets[i]));
            }
        }

        TraceUtil.trace3("Logic: Exit getAllArchivePolicies()");

        return
            (ArchivePolicy[]) policyMap.values().toArray(new ArchivePolicy[0]);
    }

    public ArchivePolicy getArchivePolicy(String policyName)
        throws SamFSException {

        return (ArchivePolicy) policyMap.get(policyName);
    }

    // for 4.5 and older
    public ArchivePolicy createArchivePolicy(String policyName,
                                             ArchivePolCriteriaProp critProp,
                                             ArchiveCopyGUIWrapper[] copies,
                                             String [] fsNames)
        throws SamFSException {
            return createArchivePolicy(
                policyName,
                null, /* policy Description */
                ArSet.AR_SET_TYPE_GENERAL,
                false,
                critProp,
                copies,
                fsNames);
    }

    // for 4.6 (IS 2.0) +
    public ArchivePolicy createArchivePolicy(String policyName,
                                             String policyDescription,
                                             short policyType,
                                             boolean is46Up,
                                             ArchivePolCriteriaProp critProp,
                                             ArchiveCopyGUIWrapper[] copies,
                                             String[] fsNames)
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter createArchivePolicy()");

        if (!SamQFSUtil.isValidString(policyName))
            throw new SamFSException("logic.invalidPolName");

        if (policyMap.size() == 0)
            getAllArchivePolicies(); // just to be doubly sure

        ArchivePolicy existingPol = getArchivePolicy(policyName);
        if (existingPol != null)
            throw new SamFSException("logic.existingPol");

        if ((fsNames == null) || (fsNames.length == 0))
            throw new SamFSException("logic.oneFSNeededForPol");

        // Check the disk archiving problem at this point.
        // We can not change the diskvols.conf without completing all
        // the checks first, i.e., the first copy might not cause problem,
        // but second copy could. In that case partial failure will occur if
        // we changed diskvols.conf for the first copy
        boolean incorrectDiskVol = false;
        if ((copies != null) &&
            (!policyName.equals(ArchivePolicy.POLICY_NAME_NOARCHIVE))) {
            for (int i = 0; i < copies.length; i++) {
                ArchiveCopyImpl cp =
                    (ArchiveCopyImpl) copies[i].getArchiveCopy();
                incorrectDiskVol = isIncorrectDiskVol(cp);
                if (incorrectDiskVol)
                    throw new SamFSException("logic.differentDiskVolExists");
            }
        }

        if ((copies != null) &&
            (!policyName.equals(ArchivePolicy.POLICY_NAME_NOARCHIVE))) {
            for (int i = 0; i < copies.length; i++) {
                ArchiveCopyImpl cp =
                    (ArchiveCopyImpl) copies[i].getArchiveCopy();
                createDiskVolInfo(cp);
            }
        }

        // setup the JNI archive set object
        Copy[] jniCritCopies = new Copy[ArchivePolicyImpl.MAX_COPY_REGULAR];
        CopyParams[] jniCopyParams =
            new CopyParams[ArchivePolicyImpl.MAX_COPY_ALLSETS];
        CopyParams[] jniRCopyParams =
            new CopyParams[ArchivePolicyImpl.MAX_COPY_ALLSETS];
        VSNMap[] jniVSNMaps = new VSNMap[ArchivePolicyImpl.MAX_COPY_ALLSETS];
        VSNMap[] jniRVSNMaps = new VSNMap[ArchivePolicyImpl.MAX_COPY_ALLSETS];
        for (int i = 0; i < ArchivePolicyImpl.MAX_COPY_REGULAR; i++) {
            jniCritCopies[i] = null;
        }
        for (int i = 0; i < ArchivePolicyImpl.MAX_COPY_ALLSETS; i++) {
            jniCopyParams[i] = null;
            jniRCopyParams[i] = null;
            jniVSNMaps[i] = null;
            jniRVSNMaps[i] = null;
        }

        // process the criteria objects
        ArchivePolCriteriaPropImpl prop =
            (ArchivePolCriteriaPropImpl) critProp;

        Criteria crit = prop.getJniCriteria();
        for (int i = 0; i < copies.length; i++) {
            jniCritCopies[i] = ((ArchivePolCriteriaCopyImpl)
                (copies[i].getArchivePolCriteriaCopy())).getJniCopy();
            jniCritCopies[i].setCopyNumber(i+1);
        }

        crit.setCopies(jniCritCopies);
        crit.setDescription(prop.getDescription());
        Criteria[] criteria = new Criteria[fsNames.length];

        for (int i = 0; i < fsNames.length; i++) {
            criteria[i] = new Criteria(fsNames[i], crit);
            criteria[i].setSetName(policyName);
        }

        // setup the copy param and vsn maps
        short numCopies = (short) copies.length;
        short numRCopies = 0;
        short numMaps = numCopies;
        short numRMaps = 0;
        for (int i = 0; i < copies.length; i++) {
            ArchiveCopyImpl cp = (ArchiveCopyImpl) copies[i].getArchiveCopy();
            ArchiveVSNMapImpl map = (ArchiveVSNMapImpl) cp.getArchiveVSNMap();
            jniCopyParams[i] = new CopyParams(policyName + "." + (i+1),
                                              cp.getJniCopyParams());
            if (SamQFSUtil.isValidString(cp.getDiskArchiveVSN())) {
                numMaps--;
            } else {
                jniVSNMaps[i] = map.getJniVSNMap();
                jniVSNMaps[i].setCopyName(policyName + "." + (i+1));
            }
        }

        if (policyName.equals(ArchivePolicy.POLICY_NAME_NOARCHIVE)) {
            policyType = ArSet.AR_SET_TYPE_NO_ARCHIVE;
        }

        ArSet arSet = new ArSet(policyName, policyDescription, policyType,
                                criteria,
                                jniCopyParams, jniRCopyParams,
                                jniVSNMaps, jniRVSNMaps);

        try {
            Archiver.createArSet(theModel.getJniContext(), arSet);
        } catch (SamFSException ex) {
            TraceUtil.trace3
                ("Logic: Exception Occur, errNo = "+ ex.getSAMerrno());
            if (ex.getSAMerrno() == 30136) {
                // need to roll back diskvol.conf file, remove the vsn
                if ((copies != null) &&
                    (!policyName.equals(
                        ArchivePolicy.POLICY_NAME_NOARCHIVE))) {
                    for (int i = 0; i < copies.length; i++) {
                        ArchiveCopyImpl cp =
                            (ArchiveCopyImpl) copies[i].getArchiveCopy();
                        removeDiskVolInfo(cp);
                     }
                }
            }
            throw ex;
        }

        // The following call needs to be made in 4.5- with the older Archive
        // Policy Wizard (criteria creation included).  For 4.6+, this method
        // is not called because:
        //
        // 1. Associate class with Policy is called if user selects a data class
        // when a policy is created.  The following call will be made in
        // archiveManager.associateClassWithPolicy.
        //
        // 2. Archiver does not need to know the changes if an unassigned
        // policy is created without having any data class applying to it.
        if (!is46Up) {
            Archiver.activateCfgThrowWarnings(theModel.getJniContext());
        }

        getAllArchivePolicies();
        TraceUtil.trace3("Logic: Exit createArchivePolicy()");

        return getArchivePolicy(policyName);
    }

    public ArchivePolCriteriaProp getDefaultArchivePolCriteriaProperties() {
        return new ArchivePolCriteriaPropImpl(null, null);
    }

    // 4.6+
    public ArchivePolCriteriaProp
        getDefaultArchivePolCriteriaPropertiesWithClassName(
            String className, String description) {

        return new ArchivePolCriteriaPropImpl(null,
                                              null,
                                              className,
                                              description);
    }

    public ArchiveCopyGUIWrapper getArchiveCopyGUIWrapper() {
        return new ArchiveCopyGUIWrapperImpl();
    }

    /**
     * This function will throw an exception if the named policy is an
     * explicit default policy or if this is an attempt to delete the
     * allsets policy.
     */
    public void deleteArchivePolicy(String policyName) throws SamFSException {

        TraceUtil.trace3("Logic: Enter deleteArchivePolicy()");

        if (!SamQFSUtil.isValidString(policyName)) {
            throw new SamFSException("logic.invalidPol");
        }

        ArchivePolicyImpl policy =
            (ArchivePolicyImpl) policyMap.get(policyName);
        if (policy == null) {
            throw new SamFSException("logic.invalidPol");
	}


	// Disallow deletion of EXPLICIT DEFAULT policies here since
	// the two argument function allows it and in general we don't
	// want them deleted.
	if (policy.getPolicyType() == ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT) {
            throw new SamFSException("logic.invalidPolType");
	}

	deleteArchivePolicy(policy, true);

        TraceUtil.trace3("Logic: Exit deleteArchivePolicy()");

    }

    /**
     * This method allows the deletion of explicit default policies.
     * It is only appropriate to delete an explicit archive policy if
     * the file system is being removed. Callers should check prior to
     * calling this function unless they are deleting the file system.
     */
    public void deleteArchivePolicy(ArchivePolicy policy,
                                    boolean update)
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter deleteArchivePolicy()");

        if (policy == null) {
            throw new SamFSException("logic.invalidPol");
        } else if (
            // Cannot delete a policy if its type is DEFAULT,
            // or ALL_SET
            policy.getPolicyType() == ArSet.AR_SET_TYPE_DEFAULT ||
            policy.getPolicyType() == ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {
            throw new SamFSException("logic.invalidPolType");
        }

        Archiver.deleteArSet(theModel.getJniContext(), policy.getPolicyName());
        policyMap.remove(policy.getPolicyName());
	if (update) {
	    Archiver.activateCfgThrowWarnings(theModel.getJniContext());
	}
        TraceUtil.trace3("Logic: Exit deleteArchivePolicy() update " + update);
    }

    public String[] getAllPoolNames() throws SamFSException {

        TraceUtil.trace3("Logic: Enter getAllPoolNames()");

        com.sun.netstorage.samqfs.mgmt.arc.VSNPool[] pools =
            VSNOp.getPools(theModel.getJniContext());

        String[] names = new String[0];
        if (pools != null) {
            names = new String[pools.length];
            for (int i = 0; i < pools.length; i++)
                names[i] = pools[i].getName();
        }

        TraceUtil.trace3("Logic: Exit getAllPoolNames()");

        return names;
    }


    // Andrei added some stuff on pools.
    // NEEDS TO BE REDONE
    // TBD
    public VSNPool[] getAllVSNPools() throws SamFSException {
        TraceUtil.trace3("Logic: Enter getAllVSNPools()");

        createPoolsFromBackEnd();

        TraceUtil.trace3("Logic: Exit getAllVSNPools()");

        return (VSNPool[]) vsnPoolMap.values().toArray(new VSNPool[0]);
    }

    public VSNPool getVSNPool(String poolName) throws SamFSException {
        // updating pool information
        createPoolsFromBackEnd();

        return (VSNPool) vsnPoolMap.get(poolName);
    }

    public VSNPool createVSNPool(String poolName,
                                 int mediaType,
                                 String vsnExpr)
        throws SamFSException {

        TraceUtil.trace3("Logic: Enter createVSNPool()");

        String[] exprs = SamQFSUtil.getStringsFromCommaStream(vsnExpr);
        com.sun.netstorage.samqfs.mgmt.arc.VSNPool jniPool =
            new com.sun.netstorage.samqfs.mgmt.arc.VSNPool(poolName,
                SamQFSUtil.getMediaTypeString(mediaType), exprs);
        VSNOp.addPool(theModel.getJniContext(), jniPool);
        VSNPoolImpl pool = new VSNPoolImpl(theModel, jniPool);
        vsnPoolMap.put(poolName, pool);

        Archiver.activateCfgThrowWarnings(theModel.getJniContext());

        TraceUtil.trace3("Logic: Exit createVSNPool()");

        return pool;

    }

    public void updateVSNPool(String poolName) throws SamFSException {
        TraceUtil.trace3("Logic: Enter updateVSNPool()");

        VSNPool pool = getVSNPool(poolName);
        if (pool != null)
            ((VSNPoolImpl) pool).update();

        TraceUtil.trace3("Logic: Exit updateVSNPool()");
    }

    public boolean isPoolInUse(String poolName) throws SamFSException {
        TraceUtil.trace3("Logic: Enter isPoolInUse()");

        boolean inUse = false;
        String copy = VSNOp.getCopyUsingPool(theModel.getJniContext(),
            poolName);
        if (SamQFSUtil.isValidString(copy))
            inUse = true;

        TraceUtil.trace3("Logic: Exit isPoolInUse()");

        return inUse;

    }

    public void deleteVSNPool(String poolName) throws SamFSException {
        TraceUtil.trace3("Logic: Enter deleteVSNPool()");

        if (SamQFSUtil.isValidString(poolName)) {
            VSNOp.removePool(theModel.getJniContext(), poolName);
            vsnPoolMap.remove(poolName);
        }

        Archiver.activateCfgThrowWarnings(theModel.getJniContext());

        TraceUtil.trace3("Logic: Exit deleteVSNPool()");
    }

    public boolean isIncorrectDiskVol(ArchiveCopy cp) throws SamFSException {
        TraceUtil.trace3("Logic: Enter isIncorrectDiskVol()");

        boolean incorrectDiskVol = false;
        if ((cp != null) &&
            (SamQFSUtil.isValidString(cp.getDiskArchiveVSN()))) {
            String dvsn = cp.getDiskArchiveVSN();
            String host = cp.getDiskArchiveVSNHost();
            String path = cp.getDiskArchiveVSNPath();
            incorrectDiskVol =
                SamQFSUtil.isDifferentDiskVolInfoPresent(dvsn, host, path,
                                                     theModel.getJniContext());
        }

        TraceUtil.trace3("Logic: Exit isIncorrectDiskVol()");

        return incorrectDiskVol;

    }


    public void createDiskVolInfo(ArchiveCopy cp) throws SamFSException {

        TraceUtil.trace3("Logic: Enter createDiskVolInfo()");

        if ((cp != null) &&
            (SamQFSUtil.isValidString(cp.getDiskArchiveVSN()))) {
            String dvsn = cp.getDiskArchiveVSN();
            String host = cp.getDiskArchiveVSNHost();
            String path = cp.getDiskArchiveVSNPath();

            // create doesn't add if same info already exists
            SamQFSUtil.createDiskVolInfo(dvsn,
                                         host,
                                         path,
                                         theModel.getJniContext());

        }

        TraceUtil.trace3("Logic: Exit createDiskVolInfo()");
    }

    public void removeDiskVolInfo(ArchiveCopy cp) throws SamFSException {

        TraceUtil.trace3("Logic: Enter removeDiskVolInfo()");

        if ((cp != null) &&
            (SamQFSUtil.isValidString(cp.getDiskArchiveVSN()))) {
            String dvsn = cp.getDiskArchiveVSN();
            String vsnPath = cp.getDiskArchiveVSNPath();
            if (!isVSNInUse(dvsn, vsnPath))
                SamQFSUtil.removeDiskVolInfo(dvsn, theModel.getJniContext());
        }

        TraceUtil.trace3("Logic: Exit createDiskVolInfo()");
    }

    private boolean isVSNInUse(String vsnString, String vsnPath)
        throws SamFSException {

        boolean isUse = false;
        ArchivePolicy[]  policies = getAllArchivePolicies();
        for (int i = 0; i < policies.length; i++) {
            ArchiveCopy[] copies = policies[i].getArchiveCopies();
            for (int j = 0; j < copies.length; j++) {
                String vsnCopyString = copies[j].getDiskArchiveVSN();
                String vsnCopyPath = copies[j].getDiskArchiveVSNPath();
                if (vsnString.equals(vsnCopyString) &&
                    vsnPath.equals(vsnCopyPath)) {
                    TraceUtil.trace3("FOUND IT, VSN is in used!");
                    isUse = true;
                    break;
                }
            }
        }
        return isUse;
    }

    private void createPoolsFromBackEnd() throws SamFSException {
        TraceUtil.trace3("Logic: Enter createPoolsFromBackEnd()");
        vsnPoolMap.clear();
        SamQFSUtil.doPrint("jni call for pool.");
        com.sun.netstorage.samqfs.mgmt.arc.VSNPool[] jniPools =
            VSNOp.getPools(theModel.getJniContext());

        if (jniPools == null) {
            SamQFSUtil.doPrint("jni returned null array");
        } else {
            SamQFSUtil.doPrint("length: " +
                               (new Integer(jniPools.length)).toString());
        }

        if ((jniPools != null) && (jniPools.length > 0)) {
            for (int i = 0; i < jniPools.length; i++) {
                VSNPoolImpl pool = new VSNPoolImpl(theModel, jniPools[i]);
                vsnPoolMap.put(pool.getPoolName(), pool);
            }
        }

        TraceUtil.trace3("Logic: Exit createPoolsFromBackEnd()");
    }

    public ArrayList isDuplicateCriteria(
        ArchivePolCriteria curCriteria, String[] fsName, boolean update)
        throws SamFSException {
        ArrayList resultList = new ArrayList();
        boolean isDuplicate = false;
        ArchivePolCriteriaProp curCriteriaProp =
            curCriteria.getArchivePolCriteriaProperties();

        ArchivePolicy[] policies = (ArchivePolicy[])
            policyMap.values().toArray(new ArchivePolicy[0]);
        for (int j = 0; j < fsName.length; j++) {
            for (int i = 0; i < policies.length; i++) {
                ArchivePolCriteria[] criterias =
                    policies[i].getArchivePolCriteriaForFS(fsName[j]);
                if (criterias != null) {
                    for (int k = 0; k < criterias.length; k++) {
                        ArchivePolCriteriaProp critrtiaProp =
                            criterias[k].getArchivePolCriteriaProperties();
                        String startingDir = critrtiaProp.getStartingDir();
                        long minSize = critrtiaProp.getMinSizeInBytes();
                        long maxSize = critrtiaProp.getMaxSizeInBytes();
                        String namePattern = critrtiaProp.getNamePattern();
                        String owner = critrtiaProp.getOwner();
                        String group = critrtiaProp.getGroup();
                        if (startingDir != null && namePattern != null &&
                            owner != null && group != null) {
                            if (startingDir.equals(
                                curCriteriaProp.getStartingDir()) &&
                                minSize == curCriteriaProp.
                                    getMinSizeInBytes() &&
                                maxSize == curCriteriaProp.
                                    getMaxSizeInBytes() &&
                                namePattern.equals(
                                    curCriteriaProp.getNamePattern()) &&
                                owner.equals(curCriteriaProp.getOwner()) &&
                                group.equals(curCriteriaProp.getGroup())) {
                                if (update) {
                                    String polName = curCriteria.
                                        getArchivePolicy().getPolicyName();
                                    if (policies[i].
                                        getPolicyName().equals(polName))
                                        break;
                                }
                                isDuplicate = true;
                                resultList.add(0, new String("true"));
                                String criteriaName = new NonSyncStringBuffer(
                                    "Criteria").append(
                                    Integer.toString(
                                        criterias[k].getIndex())).toString();
                                resultList.add(1, criteriaName);
                                resultList.add(2, policies[i].getPolicyName());
                                break;
                            }
                        }
                    } // criteria loop
                } // criteria != null
                if (isDuplicate) break;
            } // policy loop
            if (isDuplicate) break;
        } // file system loop
        if (!isDuplicate) {
            resultList.add(0, new String("false"));
        }
        return resultList;
    }

    public String getStagerLogFile() throws SamFSException {

        String logfile = new String();
        StagerParams param = Stager.getParams(theModel.getJniContext());
        if ((param != null) && (SamQFSUtil.isValidString(param.getLogPath())))
            logfile = param.getLogPath();

        return logfile;
    }

    public void setStagerLogFile(String logfile) throws SamFSException {

        StagerParams param = Stager.getParams(theModel.getJniContext());
        if (param != null)  {
            param.setLogPath(logfile);
            Stager.setParams(theModel.getJniContext(), param);
            if (!logfile.equals(""))
                FileUtil.createFile(theModel.getJniContext(), logfile);
        }
    }

    public BufferDirective[] getStagerBufDirectives() throws SamFSException {

        StagerParams param = Stager.getParams(theModel.getJniContext());
        BufDirective[] bufs = null;
        if (param != null) {
            bufs = param.getBufDirectives();
        }

        ArrayList list = new ArrayList();
        if ((bufs != null) && (bufs.length > 0)) {
            for (int i = 0; i < bufs.length; i++) {
                list.add(new BufferDirectiveImpl(bufs[i]));
            }
        }

        return (BufferDirective[]) list.toArray(new BufferDirective[0]);
    }

    // this method needs to be called to make the setters on BufferDirective
    // effective
    public void changeStagerDirective(BufferDirective[] dir)
        throws SamFSException {

        if ((dir != null) && (dir.length > 0)) {

            BufDirective[] bufs = new BufDirective[dir.length];
            for (int i = 0; i < dir.length; i++)
                bufs[i] = ((BufferDirectiveImpl) dir[i]).
                    getJniBufferDirective();

            StagerParams param = Stager.getParams(theModel.getJniContext());
            param.setBufDirectives(bufs);
            Stager.setParams(theModel.getJniContext(), param);
        }
    }

    public DriveDirective[] getStagerDriveDirectives() throws SamFSException {

        StagerParams param = Stager.getParams(theModel.getJniContext());
        DrvDirective[] drvs = null;
        if (param != null) {
            drvs = param.getDrvDirectives();
        }

        ArrayList list = new ArrayList();
        if ((drvs != null) && (drvs.length > 0)) {
            for (int i = 0; i < drvs.length; i++) {
                list.add(new DriveDirectiveImpl(theModel, drvs[i]));
            }
        }
        return (DriveDirective[]) list.toArray(new DriveDirective[0]);
    }

    // this method needs to be called to make the setters on DriveDirective
    // effective
    public void changeStagerDriveDirective(DriveDirective[] dir)
        throws SamFSException {

        if ((dir != null) && (dir.length > 0)) {

            DrvDirective[] drvs = new DrvDirective[dir.length];
            for (int i = 0; i < dir.length; i++)
                drvs[i] = ((DriveDirectiveImpl) dir[i]).
                    getJniDriveDirective();

            StagerParams param = Stager.getParams(theModel.getJniContext());
            param.setDrvDirectives(drvs);
            Stager.setParams(theModel.getJniContext(), param);
        }
    }

    public String getReleaserLogFile() throws SamFSException {

        String releaseLog = new String();
        ReleaserDirective rlDir = Releaser.getGlobalDirective(
            theModel.getJniContext());
        if (rlDir != null)
            releaseLog = rlDir.getLogFile();
        return releaseLog;
    }


    public void setReleaserLogFile(String logFile) throws SamFSException {

        ReleaserDirective rlDir = Releaser.getGlobalDirective(
            theModel.getJniContext());
        if (rlDir != null) {
            rlDir.setLogFile(logFile);
            Releaser.setGlobalDirective(theModel.getJniContext(), rlDir);
            if (!logFile.equals(""))
                FileUtil.createFile(theModel.getJniContext(), logFile);
        }
    }


    // returned string can be "72S", "72M", "72H"
    public String getMinReleaseAge() throws SamFSException {

        long releaseAge = -1;
        String ageString = "";

        ReleaserDirective rlDir = Releaser.getGlobalDirective(
            theModel.getJniContext());
        if (rlDir != null) {
            releaseAge = rlDir.getMinAge();
            if (releaseAge >= 0)
            ageString = SamQFSUtil.longToInterval(releaseAge);
        }
        return ageString;
    }

    // ageValue = "72S", "72M", "72H"
    // need to convert it to second value then pass to jni
    public void setMinReleaseAge(String ageValue) throws SamFSException {

        ReleaserDirective rlDir = Releaser.getGlobalDirective(
            theModel.getJniContext());
        if (rlDir != null) {
            int unit = SamQFSUtil.getTimeUnitInteger(ageValue);
            long age = SamQFSUtil.getLongValSecond(ageValue);
            if (unit == -1 && age == -1) {
                rlDir.resetMinAge();
            } else {
                rlDir.setMinAge(SamQFSUtil.convertToSecond(age, unit));
            }
            Releaser.setGlobalDirective(theModel.getJniContext(), rlDir);
        } else {
            TraceUtil.trace1("Logic: In setMinReleaseAge, nothing is set!");
        }
    }

    public String getRecyclerLogFile() throws SamFSException {
        return Recycler.getLogPath(theModel.getJniContext());
    }

    public void setRecyclerLogFile(String logFile) throws SamFSException {
        Recycler.setLogPath(theModel.getJniContext(), logFile);
        if (!logFile.equals(""))
            FileUtil.createFile(theModel.getJniContext(), logFile);
    }

    public int getPostRecycle() throws SamFSException {

        int  postRecycleOpt = -1;
        int jni = Recycler.getActions(theModel.getJniContext());
        if (jni == Recycler.RC_label_on)
            postRecycleOpt = SamQFSSystemArchiveManager.RECYCLE_RELABEL;
        else if (jni == Recycler.RC_export_on)
            postRecycleOpt = SamQFSSystemArchiveManager.RECYCLE_EXPORT;
        else {
            SamQFSUtil.doPrint("Label Jni val: " +
                               Integer.toString(Recycler.RC_label_on));
            SamQFSUtil.doPrint("Export Jni val: " +
                               Integer.toString(Recycler.RC_export_on));
            SamQFSUtil.doPrint("Returned JNI val: " + Integer.toString(jni));
        }
        return postRecycleOpt;
    }

    public void setPostRecycle(int postRecycleOpt) throws SamFSException {

        if (postRecycleOpt == SamQFSSystemArchiveManager.RECYCLE_EXPORT)
            Recycler.addActionExport(theModel.getJniContext(), "root");
        else if (postRecycleOpt == SamQFSSystemArchiveManager.RECYCLE_RELABEL)
            Recycler.addActionLabel(theModel.getJniContext());
    }

    public RecycleParams[] getRecycleParams() throws SamFSException {

        ArrayList list = new ArrayList();
        LibRecParams[] params = Recycler.getAllLibRecParams(
            theModel.getJniContext());
        if ((params != null) && (params.length > 0)) {
            for (int i = 0; i < params.length; i++) {
                list.add(new RecycleParamsImpl(theModel, params[i]));
            }
        }
        return (RecycleParams[]) list.toArray(new RecycleParams[0]);
    }

    public void changeRecycleParams(RecycleParams param)
        throws SamFSException {

        ((RecycleParamsImpl) param).changeParams();
    }

    public void restartArchivingAll() throws SamFSException {
        Archiver.restart(theModel.getJniContext());
    }

    public void idleArchivingAll() throws SamFSException {
        Archiver.idle(theModel.getJniContext());
    }

    public void runNowArchivingAll() throws SamFSException {
        Archiver.run(theModel.getJniContext());
    }

    public void stopArchivingAll() throws SamFSException {
        Archiver.stop(theModel.getJniContext());
    }

    public void rerunArchivingAll() throws SamFSException {
        Archiver.rerun(theModel.getJniContext());
    }

    public void runStagingAll() throws SamFSException {
        Stager.run(theModel.getJniContext());
    }

    public void idleStagingAll() throws SamFSException {
        Stager.idle(theModel.getJniContext());
    }

    // TODO: remove?
    public void restartStagingAll() throws SamFSException {
        // TBD
        // API doesn't exit
        // Stager.restart(theModel.getJniContext());
    }

    /**
     * CIS setup only (4.6 questionable?)
     * Associate Data Class with Policy
     *
     * Associate a data class with an archive set. This can result in the
     * set to which the class was previously assigned becoming an unassigned
     * set.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an intellistor setup.
     *
     * @param className -> Data class name of which you want to associate to a
     * policy
     * @param policyName -> Policy name of which you want to associate the class
     * to
     * @throws SamFSException if anything unexpected happens.
     */
    public void associateClassWithPolicy(String className, String policyName)
        throws SamFSException {

        // No need to validate className and policyName, they are not user
        // input.  It is safe to assume they are valid.

        /*
         * Data classes can be associated with existing general, no_archive
         * and explicitly-default policies.  If you associate a class to a
         * new policy, and the old policy does not have any class associate
         * with it, the old policy will NOT be deleted but its type will be
         * changed to UNASSIGNED.
         */

        /*
         * Policy cannot be default policy type (Policy that is created when
         * a file system is created) and all-sets.
         */

        Archiver.associateClassWithPolicy(theModel.getJniContext(),
                                          className,
                                          policyName);

        // Notify the Archiver about the changes
        Archiver.activateCfgThrowWarnings(theModel.getJniContext());

        // Retrieve all information from the backend, re-populate all
        // logic layer structure
        getAllArchivePolicies();
    }

    /**
     * CIS setup only (4.6 questionable?)
     * Delete Data Class
     *
     * Delete a data class. This function deletes a data class from the archiver
     * configuration and also clears any class related attributes supported in
     * the intellistor environment. This can result in the set to which the
     * class was previously assinged becoming unassigned.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an
     * intellistore.
     *
     * @param className -> Data class of which you want to delete
     * @throws SamFSException if anything unexpected happens.
     */
    public void deleteClass(String className) throws SamFSException {

        // No need to validate className.  They are not user input.  It is safe
        // to assume they are valid

        /*
         * Any data classes can be deleted EXCEPT the default data class that
         * resides in the explicit-default policy. (CIS setup only)
         *
         * User cannot create a custom class with the default class name.
         * Default class can be created only in factory
         */
        Archiver.deleteClass(theModel.getJniContext(), className);

        // Retrieve all information from the backend, re-populate all
        // logic layer structure
        getAllArchivePolicies();
    }


    /**
     * Modify the class order to the order in which the class appear in the
     * specified ArchivePolCriteria array.
     *
     * @since 4.6 & CIS
     * @param String filesystemName - name of the file system to which the
     * order applies. If <code>null</code> or an empty string is specified,
     * the order applies to ALL the filesystems.
     * @param ArchivePolCriteria [] - an array of the criteria in the desired
     * order
     */
    public void setClassOrder(String filesystemName,
                              ArchivePolCriteria [] criteria)
        throws SamFSException {
        // null implementation for now
    }

    /**
     * Added for IS 2.0 (Can be used in FSM 4.5 and 4.6)
     * Get all data classes in this setup
     * Equivalent to loop thru all policies (except the new Unassigned type in
     * version 4.6), then get all the critieria and return.
     *
     * @return All Data Classes in this setup
     * @throws SamFSException if anything unexpected happens.
     */
    public ArchivePolCriteria [] getAllDataClasses() throws SamFSException {

        int counter = 0;
        HashMap criteriaMap = new HashMap();

        // populate policyMap and sync up if it's already populated
        getAllArchivePolicies();

        Iterator it = policyMap.keySet().iterator();
        while (it.hasNext()) {
            String key = (String) it.next();
            ArchivePolicy policy = (ArchivePolicy) policyMap.get(key);
            short policyType = policy.getPolicyType();

            // Exclude unassigned policy, it is a placeholder for
            // policies that do not have data classes (criteria)
            // Also exclude file system default criterias, don't show them
            if (policyType != ArSet.AR_SET_TYPE_UNASSIGNED &&
                policyType != ArSet.AR_SET_TYPE_DEFAULT) {
                ArchivePolCriteria [] criterias =
                    policy.getArchivePolCriteria();
                for (int i = 0; i < criterias.length; i++) {
                    criteriaMap.put(
                        new Integer(counter++),
                        criterias[i]);
                }
            }
        }

        return
            (ArchivePolCriteria [])
                criteriaMap.values().toArray(new ArchivePolCriteria[0]);
    }


    /**
     * Added for IS 2.0 (Can be used in FSM 4.5 and 4.6)
     * Get a data classes in this setup by providing policy name and criteria
     * index.
     *
     * @return Data Classes that match the criteria index of which resides in
     * the given policy
     * @throws SamFSException if anything unexpected happens.
     */
    public ArchivePolCriteria getDataClassByIndex(String policyName, int index)
        throws SamFSException {

        ArchivePolCriteria [] allCriteria = getAllDataClasses();
        for (int i = 0; i < allCriteria.length; i++) {
            if (policyName.equals(
                allCriteria[i].getArchivePolicy().getPolicyName()) &&
                index == allCriteria[i].getIndex()) {
                return allCriteria[i];
            }
        }

        // Nothing found, throws exception
        throw new SamFSException("logic.noSuchCriteria");
    }

    /**
     * Added for IS 2.0
     * Used in New Data Class Wizard
     *
     * @param className to be checked if it is already in used
     * @return boolean if the input className is already in used
     */
    public boolean isClassNameInUsed(String className) throws SamFSException {
        ArchivePolCriteria [] allCriteria = getAllDataClasses();
        for (int i = 0; i < allCriteria.length; i++) {
            if (className.equals(allCriteria[i].
                getArchivePolCriteriaProperties().getClassName())) {
                return true;
            }
        }

        // if code reaches here, input className is not in used
        return false;
    }

    /**
     * Retrieve the top n copies that have the highest utilization rate
     *
     * @param count - n copies with top usage
     * @return a list of formatted strings
     * - name=copy name
     * - type=mediatype
     * - free=freespace in kbytes
     * - usage=%
     */
    public String [] getTopUsageCopies(int count) throws SamFSException {
        if (count <= 0) {
            return new String[0];
        }
        return Archiver.getCopyUtil(theModel.getJniContext(), count);
    }
}
