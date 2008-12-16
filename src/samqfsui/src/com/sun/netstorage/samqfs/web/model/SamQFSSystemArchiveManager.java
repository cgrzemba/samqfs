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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: SamQFSSystemArchiveManager.java,v 1.10 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
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
import java.util.ArrayList;

/**
 *
 * This interface is used to manage archives associated
 * with an individual server.
 *
 */
public interface SamQFSSystemArchiveManager {

    // Archive Default Value
    public static final int DEFAULT_ARCHIVE_AGE = 4;
    public static final int DEFAULT_ARCHIVE_AGE_UNIT =
        SamQFSSystemModel.TIME_MINUTE;

    // post recycle options
    public static final int RECYCLE_RELABEL = 0;
    public static final int RECYCLE_EXPORT = 1;

    /**
     *
     * Returns the global archive directive object for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return A global archive directive object
     *
     */
    public GlobalArchiveDirective getGlobalDirective() throws SamFSException;

    /**
     *
     * Set the Global Archive Directive.  This method makes an
     * RPC call to store the settings on the server.  Call this
     * method after setting properties on <code>global</code>.
     * @param global The GlobalArchiveDirective to be acted on.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void setGlobalDirective(GlobalArchiveDirective global)
        throws SamFSException;

    public FSArchiveDirective[] getFSGeneralArchiveDirective()
        throws SamFSException;

    /**
     *
     * Returns true if <code>groupName</code> is the name of an
     * existing group on this server.
     * @param groupName Name to be checked.
     * @return <code>true</code> if the name is valid,
     * <code>false</code> otherwise.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public boolean isValidGroup(String groupName) throws SamFSException;

    /**
     *
     * Returns true if <code>userName</code> is the name of an
     * existing user on this server.
     * @param userName Name to be checked.
     * @return <code>true</code> if the name is valid,
     * <code>false</code> otherwise.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public boolean isValidUser(String userName) throws SamFSException;


    /**
     * @throws SamFSException if anything unexpected happens.
     * @return String [] of all the names matching the given policy type
     */
    public String [] getAllExplicitlySetPolicyNames() throws SamFSException;

    /**
     *
     * Returns the names of all archive policies for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return the names of all archive policies for this server.
     *
     */
    public String[] getAllArchivePolicyNames() throws SamFSException;

    /**
     *
     * Returns the names of all archive policies except default and all-sets
     * for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return the names of all archive policies except default and all-sets
     * for this server.
     *
     */
    public String[] getAllNonDefaultNonAllSetsArchivePolicyNames()
        throws SamFSException;

    /**
     *
     * Returns all the ArchivePolicy objects for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return all the ArchivePolicy objects for this server.
     *
     */
    public ArchivePolicy[] getAllArchivePolicies() throws SamFSException;


    /**
     *
     * Returns the ArchivePolicy object that is named
     * <code>policyName</code>, or <code>null</code> if there
     * is no matching policy on this server.
     * @param policyName Name of the policy to search for.
     * @throws SamFSException if anything unexpected happens.
     * @return the <code>ArchivePolicy</code> object or
     * <code>null</code>.
     *
     */
    public ArchivePolicy getArchivePolicy(String policyName)
        throws SamFSException;

    /**
     * Rewrite in proper javadoc formatting.
     *
     * This method is for creating a new archive policy from the new
     * archive policy wizard of SAM-QFS Manager. Basically this create
     * method allows the user to create a "simplified" archive policy
     * with one archive set criteria; the criteria specific copy parameters
     * are encapsulated within ArchiveCopyGUIWrapper interface and are
     * assigned to the sole criteria; similarly all the selected filesystems
     * are associated with the sole criteria as well.
     *
     * Pass a null array or array of size zero for policyName "no_archive"
     */
    public ArchivePolicy createArchivePolicy(String policyName,
                                             ArchivePolCriteriaProp critProp,
                                             ArchiveCopyGUIWrapper[] copies,
                                             String[] fsNames)
        throws SamFSException;

    /**
     * This method is for creating a new archive policy from the new
     * ISPolicyWizard available in 4.6+ (IS 2.0).  This takes in a
     * description field and the policy type to indicate the initial policy
     * type (Unassigned).  If the user selects a data class to be associate
     * with the new policy, a separate API call will be issued right after this
     * new policy is created.
     */
    public ArchivePolicy createArchivePolicy(String policyName,
                                             String description,
                                             short policyType,
                                             boolean is46Up,
                                             ArchivePolCriteriaProp critProp,
                                             ArchiveCopyGUIWrapper[] copies,
                                             String[] fsNames)
         throws SamFSException;

    public ArchivePolCriteriaProp getDefaultArchivePolCriteriaProperties();

    // 4.6+
    public ArchivePolCriteriaProp
        getDefaultArchivePolCriteriaPropertiesWithClassName(
            String className, String description);

    public ArchiveCopyGUIWrapper getArchiveCopyGUIWrapper();
    /**
     *
     * Delete the specified policy.
     * @param policyName Policy object representing the policy to be deleted.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void deleteArchivePolicy(String policyName)
        throws SamFSException;

    /**
     *
     * Delete the specified policy.
     * @param policy Policy object representing the policy to be deleted.
     * @param update if true activate the archiver config.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void deleteArchivePolicy(ArchivePolicy policy, boolean update)
        throws SamFSException;

    /**
     *
     * Get an array of all valid pool names for this server.
     * @return An array of pool names.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public String[] getAllPoolNames() throws SamFSException;

    /**
     *
     * Get an array of all <code>VSNPool</code> objects for this server.
     * @return An array of <code>VSNPool</code> objects.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public VSNPool[] getAllVSNPools() throws SamFSException;

    /**
     *
     * Get the VSNPool object with name <code>poolName</code>.
     * @param poolName The name of the pool to be retrieved.
     * @return The matching <code>VSNPool</code> object, or
     * <code>null</code> if none is found.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public VSNPool getVSNPool(String poolName) throws SamFSException;

    /**
     *
     * Create a new VSNPool object with the specified properties.
     * @param poolName Name of pool to be created
     * @param mediaType
     * @param vsnExpression
     * @return The new <code>VSNPool</code> object.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public VSNPool createVSNPool(String poolName, int mediaType,
                                 String vsnExpression)
        throws SamFSException;

    /**
     *
     * Update the <code>VSNPool</code> object with name
     * <code>poolName</code> to match the actual configuration
     * on the server.
     * @param poolName The name of the pool to be updated.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void updateVSNPool(String poolName) throws SamFSException;

    /**
     *
     * Find out if the named pool is currently in use.
     * @param poolName The name of the pool to be checked.
     * @return <code>true</code> if the named pool is in use,
     * <code>false</code> otherwise.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public boolean isPoolInUse(String poolName) throws SamFSException;

    /**
     *
     * Delete the pool with name <code>poolname</code>.
     * @param poolname The name of the pool to be deleted.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void deleteVSNPool(String poolname) throws SamFSException;

    /**
     *
     * Find out the criteria is duplicate.
     * @param criteria The criteria need to be checked.
     * @param fsName The file system names the criteria is used for.
     * @param update <code>true</code> if update criteria
     * <code>false</code> if creating new one
     * return <code>ArrayList</code> contains
     * <code>true</code> if the criteria is duplicate and the criteria name
     * the new/modified one would have matched.
     * <code>false</code> otherwise.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public ArrayList isDuplicateCriteria(
        ArchivePolCriteria criteria, String[] fsName, boolean update)
        throws SamFSException;

    /**
     * Returns the name of the stager log file for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Log file name
     */
    public String getStagerLogFile() throws SamFSException;

    /**
     * Sets the name of the stager log file for this server.
     * @param logfile
     * @throws SamFSException if anything unexpected happens.
     */
    public void setStagerLogFile(String logfile) throws SamFSException;

    /**
     * Gets all the buffer directives for this server's stager.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of buffer directives.
     */
    public BufferDirective[] getStagerBufDirectives() throws SamFSException;

    /**
     * This method needs to be called to make the setters on BufferDirective
     * effective
     * @param dirs An array of <code>BufferDirective</code> objects.
     * @throws SamFSException if anything unexpected happens.
     */
    public void changeStagerDirective(BufferDirective[] dirs)
        throws SamFSException;

    /**
     * Gets all the drive directives for this server's stager.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of drive directives.
     */
    public DriveDirective[] getStagerDriveDirectives() throws SamFSException;

    /**
     * This method needs to be called to make the setters on DriveDirective
     * effective
     * @param dirs An array of <code>DriveDirective</code> objects.
     * @throws SamFSException if anything unexpected happens.
     */
    public void changeStagerDriveDirective(DriveDirective[] dirs)
        throws SamFSException;

    /**
     * Returns the name of the releaser log file for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Log file name
     */
    public String getReleaserLogFile() throws SamFSException;

    /**
     * Sets the name of the releaser log file for this server.
     * @param logFile
     * @throws SamFSException if anything unexpected happens.
     */
    public void setReleaserLogFile(String logFile) throws SamFSException;

    /**
     * Get the minimum release age for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Age
     */
    public String getMinReleaseAge() throws SamFSException;

    /**
     * Set the minimum release age for this server.
     * @throws SamFSException if anything unexpected happens.
     */
    public void setMinReleaseAge(String age) throws SamFSException;

    /**
     * Returns the name of the recycler log file for this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Log file name
     */
    public String getRecyclerLogFile() throws SamFSException;

    /**
     * Sets the name of the recycler log file for this server.
     * @param logFile
     * @throws SamFSException if anything unexpected happens.
     */
    public void setRecyclerLogFile(String logFile) throws SamFSException;

    /**
     * Get the post recycle value.
     * @throws SamFSException if anything unexpected happens.
     * @return post recycle value.
     */
    public int getPostRecycle() throws SamFSException;

    /**
     * Set the post recycle value.
     * @param postRecycleOpt
     * @throws SamFSException if anything unexpected happens.
     */
    public void setPostRecycle(int postRecycleOpt) throws SamFSException;

    /**
     * Get the recycle parameters.
     * @throws SamFSException if anything unexpected happens.
     * @return An array of recycle parameters.
     */
    public RecycleParams[] getRecycleParams() throws SamFSException;

    /**
     * This method needs to be called to make the setters on RecycleParams
     * effective
     * @param dirs An array of <code>RecycleParams</code> objects.
     * @throws SamFSException if anything unexpected happens.
     */
    public void changeRecycleParams(RecycleParams param) throws SamFSException;

    /**
     * Restart archiving
     * @throws SamFSException if anything unexpected happens.
     */
    public void restartArchivingAll() throws SamFSException;

    /**
     * Idle archiving
     * @throws SamFSException if anything unexpected happens.
     */
    public void idleArchivingAll() throws SamFSException;

    /**
     * Run archiving now
     * @throws SamFSException if anything unexpected happens.
     */
    public void runNowArchivingAll() throws SamFSException;

    /**
     * Rerun archiving
     * @throws SamFSException if anything unexpected happens.
     */
    public void rerunArchivingAll() throws SamFSException;

    /**
     * Stop archiving
     * @throws SamFSException if anything unexpected happens.
     */
    public void stopArchivingAll() throws SamFSException;

    /**
     *
     * Run staging
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void runStagingAll() throws SamFSException;

    /**
     * Idle staging
     * @throws SamFSException if anything unexpected happens.
     */
    public void idleStagingAll() throws SamFSException;

    /**
     * Restart staging
     * @throws SamFSException if anything unexpected happens.
     */
    public void restartStagingAll() throws SamFSException;

    /**
     * CIS setup only (4.6 questionable?)
     * Associate Data Class with Policy
     *
     * Associate a data class with an archive set. This can result in the
     * set to which the class was previously assigned becoming an unassigned
     * set.
     *
     * Data classes can be associated with existing general, no_archive and
     * explicitly-default policies.  If you associate a class to a new policy,
     * and the old policy does not have any class associate with it, the old
     * policy will NOT be deleted but its type will be changed to UNASSIGNED.
     *
     * Policy cannot be default or all-sets typed.
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
        throws SamFSException;

    /**
     * Modify the class order to the order in which the class appear in the
     * specified ArchivePolCriteria array.
     *
     * @since 4.6 & CIS
     * @param String filesystemName - name of the file system to which the
     * order applies. If <code>null</code> or an empty string is specified, the
     * order applies to ALL the filesystems.
     *
     * @param ArchivePolCriteria [] - an array of the criteria in the desired
     * order
     */
    public void setClassOrder(String filesystemName,
                              ArchivePolCriteria [] criteria)
        throws SamFSException;

    /**
     * CIS setup only (4.6 questionable?)
     * Delete Data Class
     *
     * Delete a data class. This function deletes a data class from the archiver
     * configuration and also clears any class related attributes supported in
     * the intellistor environment. This can result in the set to which the
     * class was previously assinged becoming unassigned.
     *
     * Any data classes can be deleted EXCEPT the default data class that
     * resides in the explicit-default policy.
     *
     * Limits: It is unclear at this time if this function will be
     * supported outside of the limited environment of an
     * intellistore.
     *
     * @param className -> Data class of which you want to delete
     * @throws SamFSException if anything unexpected happens.
     */
    public void deleteClass(String className) throws SamFSException;

    /**
     * Added for IS 2.0 (Can be used in FSM 4.5 and 4.6)
     * Get all data classes in this setup
     * Equivalent to loop thru all policies (except the new Unassigned type in
     * version 4.6), then get all the critieria and return.
     *
     * @return All Data Classes in this setup
     * @throws SamFSException if anything unexpected happens.
     */
    public ArchivePolCriteria [] getAllDataClasses() throws SamFSException;

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
        throws SamFSException;

    /**
     * Added for IS 2.0
     * Used in New Data Class Wizard
     *
     * @param className to be checked if it is already in used
     * @return boolean if the input className is already in used
     * @throws SamFSException if anything unexpected happens.
     */
    public boolean isClassNameInUsed(String className) throws SamFSException;

    /**
     * Retrieve the top n copies that have the highest utilization rate
     *
     * @param count - n copies with top usage
     * @return a list of formatted strings
     *
     * - name=copy name
     * - type=mediatype
     * - free=freespace in kbytes
     * - usage=%
     */
    public String [] getTopUsageCopies(int count) throws SamFSException;

}
