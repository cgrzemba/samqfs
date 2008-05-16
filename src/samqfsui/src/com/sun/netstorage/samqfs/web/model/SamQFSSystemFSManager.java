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

// ident	$Id: SamQFSSystemFSManager.java,v 1.46 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiStepOpException;
import com.sun.netstorage.samqfs.mgmt.fs.FSArchCfg;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.FileSystemMountProperties;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericMountOptions;
import com.sun.netstorage.samqfs.web.model.fs.Metric;
import com.sun.netstorage.samqfs.web.model.fs.RemoteFile;
import com.sun.netstorage.samqfs.web.model.fs.RestoreDumpFile;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import com.sun.netstorage.samqfs.web.util.Filter;

/**
 *
 * This interface is used to manage file systems associated
 * with an individual server.
 *
 */
public interface SamQFSSystemFSManager {

    // static integers representing the removable media status bits
    public static int FS_STATUS_MOUNTED = 20001;
    public static int FS_STATUS_BEING_MOUNTED = 20002;
    public static int FS_STATUS_BEING_UNMOUNTED = 20003;
    public static int FS_STATUS_FS_DATA_BEING_ARCHIVED = 20004;
    public static int FS_STATUS_FS_DATA_BEING_RELEASED = 20005;
    public static int FS_STATUS_FS_DATA_BEING_STAGED = 20006;
    public static int FS_STATUS_SAMFS_VERSION_1 = 20007;
    public static int FS_STATUS_SAMFS_VERSION_2 = 20008;
    public static int FS_STATUS_SHARED = 20009;
    public static int FS_STATUS_SINGLE_WRITER = 20010;
    public static int FS_STATUS_MULTI_READER = 20011;
    public static int FS_STATUS_MR_DEVICE = 20012;
    public static int FS_STATUS_MD_DEVICE = 20013;

    public static final int RESTORE_REPLACE_NEVER = 0;
    public static final int RESTORE_REPLACE_ALWAYS = 1;
    public static final int RESTORE_REPLACE_WITH_NEWER = 2;

    public static final int ARCHIVE = 0;
    public static final int RELEASE = 1;
    public static final int STAGE   = 2;

    /**
     *
     * Returns the names of all SAM/QFS file systems on this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Array of file system names
     *
     */
    public String[] getAllFileSystemNames() throws SamFSException;


    /**
     *
     * Returns the names of all files systems including mcf, vfstab
     * @throws SamFSException if anything unexpected happens.
     * @return Array of file system names
     *
     */
    public String[] getFileSystemNamesAllTypes() throws SamFSException;


    /**
     *
     * Returns the FileSystem objects representing all SAM/QFS
     * file systems on this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Array of file system objects
     *
     */
    public FileSystem[] getAllFileSystems() throws SamFSException;


    /**
     *
     * Returns the FileSystem objects representing all SAM/QFS
     * file systems on this server of the specified archiving type.
     * @param archivingType Archiving type to search for.
     * Valid values are: <CODE>FileSystem.SEPARATE_METADATA</CODE> and
     * <CODE>FileSystem.COMBINED_METADATA</CODE>
     * @throws SamFSException if anything unexpected happens.
     * @return Array of file system objects
     *
     */
    public FileSystem[] getAllFileSystems(int archivingType)
        throws SamFSException;



    /**
     *
     * Returns GenericFileSystem objects representing all NON SAM/QFS
     * file systems on this server.
     * @throws SamFSException if anything unexpected happens.
     * @return Array of GenericFileystem objects
     *
     */
    public GenericFileSystem[] getNonSAMQFileSystems()
        throws SamFSException;

    /**
     * Returns the FileSystem object with the specified name.
     * @param fsName Name to search for.
     * @throws SamFSException if anything unexpected happens.
     * @return A FileSystem object, or <CODE>null</CODE> if there is no match.
     * @since 4.4
     */
    public GenericFileSystem getGenericFileSystem(String fsName)
        throws SamFSException;

    /**
     * Returns the SAM-FS/QFS FileSystem object with the specified name.
     * @param fsName Name to search for.
     * @throws SamFSException if anything unexpected happens.
     * @return A FileSystem object, or <CODE>null</CODE> if there is no match.
     */
    public FileSystem getFileSystem(String fsName) throws SamFSException;


    /**
     * Returns a FileSystemMountProperties object with default values as
     * populated by the C library.
     * @param fsType
     * @param archType
     * @param dauSize
     * @param stripedGrp
     * @param shareStatus
     * @param multiReader
     * @throws SamFSException if anything unexpected happens.
     * @return A FileSystemMountProperties object.
     */
    public FileSystemMountProperties getDefaultMountProperties(int fsType,
                        int archType, int dauSize, boolean stripedGrp,
                        int shareStatus, boolean multiReader)
        throws SamFSException;


    /**
     * Create an (arhiving or non-arhiving) unshared QFS file system
     *
     * @param fsName
     * @param fsType
     * @param archType - ignored
     * @param equipOrdinal
     * @param mountPoint
     * @param shareStatus - ignored
     * @param DAUSize
     * @param mountProps
     * @param metadataDevices
     * @param dataDevices
     * @param stripedGroups
     * @param singleDAU
     * @param mountAtBoot
     * @param createMountPoint
     * @param mountAfterCreate
     * @param FSArchCfg - archiving config information for the new fs
     *
     * @throws SamFSMultiStepOpException if any particular step fails.
     * @throws SamFSException if anything unexpected happens.
     * @return A FileSystem object.
     */
    public FileSystem createFileSystem(String fsName,
                                       int fsType,
                                       int archType,
                                       int equipOrdinal,
                                       String mountPoint,
                                       int shareStatus,
                                       int DAUSize,
                                       FileSystemMountProperties mountProps,
                                       DiskCache[] metadataDevices,
                                       DiskCache[] dataDevices,
                                       StripedGroup[] stripedGroups,
                                       boolean singleDAU,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate,
                                       FSArchCfg archiveConfig)
        throws SamFSMultiStepOpException, SamFSException;


    /**
     * Create a non-archiving unshared HA-QFS file system
     *
     * @param hostnames - first host in this list will do the mkfs
     * @param fsName
     * @param fsType - future use. ignored if not FileSystem.SEPARATE_METADATA
     * @param equipOrdinal
     * @param mountPoint
     * @param DAUSize
     * @param mountProps - syncmeta will be set to 1
     * @param metadataDevices
     * @param dataDevices
     * @param stripedGroups
     * @param singleDAU
     * @param createMountPoint
     * @param mountAfterCreate
     *
     * @throws SamFSMultiHostException each exception inside this may be a
     * SamFSMultiStepOpException.
     * @throws SamFSException
     * @return A FileSystem object.
     */
    public FileSystem createHAFileSystem(String[] hostnames,
                                         String fsName,
                                         int fsType,
                                         int equipOrdinal,
                                         String mountPoint,
                                         int DAUSize,
                                         FileSystemMountProperties mountProps,
                                         DiskCache[] metadataDevices,
                                         DiskCache[] dataDevices,
                                         StripedGroup[] stripedGroups,
                                         boolean singleDAU,
                                         boolean createMountPoint,
                                         boolean mountAfterCreate)
        throws SamFSMultiHostException;


    /**
     * This method is used to add hosts to an existing HA-FS
     * fs must be unmounted on the specified host, before this method is called
     *
     * @param fs - files system of which a host is about to see in a HA setup
     * @param host - the host of the HA-setup
     */
    public void addHostToHAFS(FileSystem fs, String host)
        throws SamFSMultiStepOpException, SamFSException;

    /**
     * This method is used to remove hosts from an existing HA-FS
     * fs must be unmounted on the specified host, before this method is called
     *
     * @param fs - files system of which a host is about to lose provisioning
     *  in a HA setup
     * @param host - the host of the HA-setup
     */
    public void removeHostFromHAFS(FileSystem fs, String host)
        throws SamFSException;


    /**
     *
     * Delete the specified file system.
     * @param fs FileSystem to act on
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void deleteFileSystem(GenericFileSystem fs) throws SamFSException;


    /**
     *
     * Create the specified directory.
     * @param fullPath Full path of directory to be created.
     * @throws SamFSException if anything unexpected happens.
     *
     */
    public void createDirectory(String fullPath) throws SamFSException;


    /**
     *
     * Returns an array of <code>DiskCache</code> objects
     * representing the available allocatable units.
     * @param hosts - if non-null, then return ONLY HA allocatable units,
     * accessible from the specified hosts
     * @throws SamFSException if anything unexpected happens.
     * @return An array of <code>DiskCache</code> objects
     * representing the available allocatable units.
     *
     */
    public DiskCache[] discoverAvailableAllocatableUnits(String[] haHosts)
        throws SamFSException;


    /**
     *
     * Creates a new <code>StripedGroup</code> object with the specified
     * properties.
     * @param name
     * @param disks
     * @throws SamFSException if anything unexpected happens.
     * @return A new <code>StripedGroup</code> object with the specified
     * properties.
     *
     */
    public StripedGroup createStripedGroup(String name, DiskCache[] disks)
        throws SamFSException;

    /**
     *
     * Check slices for overlaps.
     * @param slices
     * @throws SamFSException if anything unexpected happens.
     * @return An array of strings
     *
     */
    public String[] checkSlicesForOverlaps(String[] slices)
        throws SamFSException;


    /**
     *
     * To retrieve all the available criteria for a given file system
     *
     * @param fs - file system of which the criteria is associated
     * @return - array of criteria that are available for the file system
     */
    public ArchivePolCriteria[] getAllAvailablePolCriteria(FileSystem fs)
        throws SamFSException;

    /**
     * create a UFS filesystem
     */
    public GenericFileSystem createUFS(DiskCache dev, String mountPoint,
                                       GenericMountOptions mountOpts,
                                       boolean mountAtBoot,
                                       boolean createMountPoint,
                                       boolean mountAfterCreate)
        throws SamFSMultiStepOpException, SamFSException;


    /**
     *  remote call that retrieves up to maxEntries directory entries.
     * the result of this method is cached
     */
    public RemoteFile[] getDirEntries(int maxEntries, String dirPath,
                                      Filter filter) throws SamFSException;


    /**
     * starts metadata dump and returns job id
     */
    public long startMetadataDump(String fsName, String dumpName)
        throws SamFSException;


    //  Dump control

    /**
     *  @return the list of all available dump files.
     */
    public RestoreDumpFile[] getAvailableDumpFiles(
        String fsName, String directory)
        throws SamFSException;

    /**
     *  enable dump for use. will index and/or decompress
     *  @return jobID
     */
    public long enableDumpFileForUse(
        String fsName, String directory, String dumpFilename)
        throws SamFSException;

    /**
     * Removes files created by enableDumpFileForUse
     *
     * @param dumpName the name may include the full path
     */
    public void cleanDump(String fsName, String directory, String dumpName)
        throws SamFSException;


    /**
     * @return Information about the specified file from specified dump
     * directory paramater is taken away as the back end in 4.6 returns the
     * full qualified path of a dumpFile.
     */
    public RestoreFile getRestoreFile(String fileName) throws SamFSException;

    /**
     * @return Returns a restore file object that will allow restoration of
     * the entire file system.
     */
    public RestoreFile getEntireFSRestoreFile() throws SamFSException;

    /**
     * Restore specified inodes. Return jobID (4.6 above)
     * directory paramater is taken away as the back end in 4.6 returns the
     * full qualified path of a dumpFile.
     */
    public long restoreFiles(String fsName,
                             String dumpFile,
                             int replaceType,
                             RestoreFile[] files)
        throws SamFSException;


    /**
     * Removes all files associated with a metadata snapshot
     *
     *
     * @since 4.5
     */
    public void deleteDump(String fsName, String directory, String dumpName)
        throws SamFSException;

    /**
     * Sets or clears the "retain dump permanently" flag.
     *
     * @param dumpName the name may include the full path
     * @since 4.5
     */
    public void setIsDumpRetainedPermanently(String fsName,
                                             String directory,
                                             String dumpName,
                                             boolean retainValue)
                                             throws SamFSException;
    /**
     * Convert stage file details into a StageFile object
     *
     * @since 4.6
     */
    public StageFile parseStageFileDetails(String details)
        throws SamFSException;

    /**
     * Get file details in File Browser, both live and recovery point content
     *
     * @since 4.6
     */
    public DirInfo getAllFilesInformation(
                                        String fsName,
                                        String snapPath,
                                        String relativeDir,
                                        String lastFile,
                                        int maxEntries,
                                        int whichDetails,
                                        Filter filter) throws SamFSException;

    /**
     * Get file details of a single file in File Browser, both live and
     * recovery point content
     *
     * @since 4.6
     */
    public StageFile getFileInformation(
                                        String fsname,
                                        String snapPath,
                                        String filePath,
                                        int whichDetails) throws SamFSException;

    /**
     * stage the given file[s]
     *
     * @since 4.5
     */
    public long stageFiles(int copy,
                           String [] filePaths,
                           int options) throws SamFSException;

    /**
     * get the metric report for the specified fs
     *
     * @since 4.6
     */
    public Metric getMetric(
        String fsName, int type, long startTime, long endTime)
        throws SamFSException;

    /**
     * archive files and directories or set archive options for them
     *
     * @param list of files to be archived
     * @param options defined in Archiver.java (JNI)
     * @return job ID
     * @since 4.6
     */
    public long archiveFiles(String [] files, int options)
        throws SamFSException;


    /**
     * release files and directories or set archive options for them
     *
     * @param list of files to be archived
     * @param options defined in Archiver.java (JNI)
     * @param release partial size. If PARTIAL is specified but partial_size is
     *  not, the mount option partial will be applied.
     * @return job ID
     * @since 4.6
     */
    public long releaseFiles(String [] files, int options, int partialSize)
        throws SamFSException;

    /**
     * Change File Attributes (Archive, Release, Stage)
     *
     * @param Type of attribute to be changed (ARCHIVE, RELEASE, STAGE)
     * @param file name of which the attributes will be applied
     * @param New attributes to be applied to the files
     * @param Existing attributes of the files
     * @param if recursive check box is checked (false for file entries)
     * @param Partial Size (only applicable to RELEASE)
     * @return job ID
     * @since 4.6
     */
    public long changeFileAttributes(
        int type, String file,
        int newOption, int existingOption,
        boolean recursive, int partialSize)
        throws SamFSException;


    /**
     * Delete a list of files
     *
     * @param String[] list of fully qualified fileNames
     * @since 4.6
     */
    public void deleteFiles(String[] paths) throws SamFSException;

    /**
     * Delete a single file
     *
     * @param String fully qualified fileName
     * @since 4.6
     */
    public void deleteFile(String path) throws SamFSException;

    /**
     * Get all known directories containing snapshots for a given filesystem.
     *
     * @since 4.6
     * @return a list of directory paths.
     */
    public String [] getIndexDirs(String fsName) throws SamFSException;

    /**
     * Get all indexed snapshots for a given filesystem.  Returns an array
     * of key/value pairs of the form  "name=%s,date=%ld"
     *
     * @since 4.6
     * @return all indexed snapshots for a given file system
     */
    public String [] getIndexedSnaps(String fsName, String directory)
        throws SamFSException;
}
