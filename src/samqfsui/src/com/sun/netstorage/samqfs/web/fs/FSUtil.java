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

// ident	$Id: FSUtil.java,v 1.23 2008/06/11 16:58:00 ronaldso Exp $

/**
 * This util class contains a few declaration of the file system
 * description (the new terminology introduced in 4.4) and a helper
 * method to retrieve the file system description that is i18n-ed.
 * The methods will be used by the filter handler in FS Summary page as well.
 */

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.model.fs.SharedMember;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import javax.servlet.http.HttpServletRequest;

public class FSUtil {

    public static final int FS_DESC_UNKNOWN = -1;
    public static final int FS_DESC_QFS = 0;
    public static final int FS_DESC_QFS_ARCHIVING = 1;
    public static final int FS_DESC_QFS_CLIENT = 2;
    public static final int FS_DESC_QFS_CLIENT_ARCHIVING = 3;
    public static final int FS_DESC_QFS_SERVER = 4;
    public static final int FS_DESC_QFS_SERVER_ARCHIVING = 5;
    public static final int FS_DESC_QFS_POTENTIAL_SERVER = 6;
    public static final int FS_DESC_QFS_POTENTIAL_SERVER_ARCHIVING = 7;
    public static final int FS_DESC_UFS = 8;
    public static final int FS_DESC_NFS_CLIENT = 9;
    public static final int FS_DESC_ZFS = 10;
    public static final int FS_DESC_VXFS = 11;

    public static final String DOTDOTDOT = "...";
    public static final int MAX_MOUNT_POINT_LENGTH = 20;

    /**
     * This method returns the File System Description (int)
     * that is used for display in FS Summary, and FS Details.
     * MUST BE MODIFIED. NO REMOTE CALL NECESSARY IF FileSystem OBJ AVAILABLE.
     */

    public static int getFileSystemDescription(GenericFileSystem myGenericFS) {
        TraceUtil.trace3("Entering");
        int fsDesc = FS_DESC_UNKNOWN;

        switch (myGenericFS.getFSTypeByProduct()) {

            case GenericFileSystem.FS_NONSAMQ:
                // Non-SAM file system
                if (myGenericFS.getFSTypeName().equalsIgnoreCase("ufs")) {
                    fsDesc = FS_DESC_UFS;
                } else if (myGenericFS.getFSTypeName().
                    equalsIgnoreCase("vxfs")) {
                    fsDesc = FS_DESC_VXFS;
                } else if (myGenericFS.getFSTypeName().
                    equalsIgnoreCase("zfs")) {
                    fsDesc = FS_DESC_ZFS;
                } else {
                    TraceUtil.trace1(
                        "Unknown fs desc: Non-SAM, non-ufs, non-vxfs.");
                }
                break;

            default:
                FileSystem myFS = (FileSystem) myGenericFS;

                // SAM/Q file system
                switch (myFS.getShareStatus()) {
                    case FileSystem.SHARED_TYPE_MDS:
                    case FileSystem.SHARED_TYPE_PMDS:
                    case FileSystem.SHARED_TYPE_CLIENT:
                    case FileSystem.SHARED_TYPE_UNKNOWN:
                        // shared fs
                        switch (myFS.getArchivingType()) {
                            case FileSystem.ARCHIVING:
                                fsDesc = getSharedFSDescription(myFS, true);
                                break;
                            case FileSystem.NONARCHIVING:
                                fsDesc = getSharedFSDescription(myFS, false);
                                break;
                            default:
                                TraceUtil.trace1(new StringBuffer(
                                    "Unknown fs desc: shared-SAM, ").append(
                                        "archiving status unknown").toString());
                                break;
                        }
                        break;

                    case FileSystem.UNSHARED:
                        // unshared
                        switch (myFS.getArchivingType()) {
                            case FileSystem.ARCHIVING:
                                fsDesc = FS_DESC_QFS_ARCHIVING;
                                break;

                            case FileSystem.NONARCHIVING:
                                fsDesc = FS_DESC_QFS;
                                break;

                            default:
                                TraceUtil.trace1(new StringBuffer(
                                    "Unknown fs desc: unshared-SAM, ").append(
                                        "archiving status unknown").toString());
                                break;
                        }
                        break;

                    default:
                        TraceUtil.trace1(
                            "Unknown fs type: shared status unknown");
                        break;
                }
        }

        TraceUtil.trace3("Exiting");
        return fsDesc;
    }

    private static int getSharedFSDescription(
        FileSystem myFS, boolean isArchiving) {

        switch (myFS.getShareStatus()) {
            case FileSystem.SHARED_TYPE_MDS:
                if (isArchiving) {
                    return FS_DESC_QFS_SERVER_ARCHIVING;
                } else {
                    return FS_DESC_QFS_SERVER;
                }
            case FileSystem.SHARED_TYPE_PMDS:
                if (isArchiving) {
                    return FS_DESC_QFS_POTENTIAL_SERVER_ARCHIVING;
                } else {
                    return FS_DESC_QFS_POTENTIAL_SERVER;
                }
            case FileSystem.SHARED_TYPE_CLIENT:
                if (isArchiving) {
                    return FS_DESC_QFS_CLIENT_ARCHIVING;
                } else {
                    return FS_DESC_QFS_CLIENT;
                }
            default:
                TraceUtil.trace1(
                    "Shared_FS_STATUS is " + myFS.getShareStatus());
                TraceUtil.trace1("Unknown fs desc: Unknown share type!");
                return FS_DESC_UNKNOWN;
        }
    }

    public static String getFileSystemDescriptionString(
        GenericFileSystem myGenericFS) {
        return getFileSystemDescriptionString(myGenericFS, false);
    }

    public static String getFileSystemDescriptionString(
        GenericFileSystem myGenericFS, boolean jsf) {

        StringBuffer myBuffer = new StringBuffer();

        int description = getFileSystemDescription(myGenericFS);
        switch (description) {

            case FS_DESC_QFS:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.qfs") :
                        SamUtil.getResourceString("filesystem.desc.qfs"));
                 break;
            case FS_DESC_QFS_ARCHIVING:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.qfs.archiving") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.archiving"));
                 break;
            case FS_DESC_QFS_CLIENT:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.qfs.client") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.client"));
                 break;
            case FS_DESC_QFS_CLIENT_ARCHIVING:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage(
                            "filesystem.desc.qfs.client.archiving") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.client.archiving"));
                 break;
            case FS_DESC_QFS_SERVER:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.qfs.server") :
                        SamUtil.getResourceString("filesystem.desc.qfs.server"));
                 break;
            case FS_DESC_QFS_SERVER_ARCHIVING:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage(
                            "filesystem.desc.qfs.server.archiving") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.server.archiving"));
                 break;
            case FS_DESC_QFS_POTENTIAL_SERVER:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage(
                            "filesystem.desc.qfs.potential.server") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.potential.server"));
                 break;
            case FS_DESC_QFS_POTENTIAL_SERVER_ARCHIVING:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage(
                            "filesystem.desc.qfs.potential.server.archiving") :
                        SamUtil.getResourceString(
                            "filesystem.desc.qfs.potential.server.archiving"));
                 break;
            case FS_DESC_UFS:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.ufs") :
                        SamUtil.getResourceString("filesystem.desc.ufs"));
                 break;
            case FS_DESC_ZFS:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.zfs") :
                        SamUtil.getResourceString("filesystem.desc.zfs"));
                 break;
            case FS_DESC_VXFS:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.vxfs") :
                        SamUtil.getResourceString("filesystem.desc.vxfs"));
                 break;
            case FS_DESC_NFS_CLIENT:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.nfs.client") :
                        SamUtil.getResourceString("filesystem.desc.nfs.client"));
                 break;
            default:
                 myBuffer.append(
                     jsf ?
                        JSFUtil.getMessage("filesystem.desc.unknown") :
                        SamUtil.getResourceString("filesystem.desc.unknown"));
                 return myBuffer.toString();
        }

        // determine if the fs is ha
        if (myGenericFS.isHA()) {
            myBuffer.append(",").append(
                jsf ?
                    JSFUtil.getMessage("filesystem.desc.ha") :
                    SamUtil.getResourceString("filesystem.desc.ha"));
        }

        return myBuffer.toString();
    }

    /**
     * @param Takes it the original mount point string
     * @return Returns the shortened version of mount point string if the
     *  original string exceeds 25 in size
     *
     * If the original mountPoint is 25 in size or less, return right away
     * Otherwise, use ... to truncate the string and make the string 25 in size
     */
    public static String truncateMountPointString(String mountPoint) {
        if (mountPoint == null) {
            return "";
        } else if (mountPoint.length() <= MAX_MOUNT_POINT_LENGTH) {
            return mountPoint;
        }

        // NOTE: MOUNT POINT has to start with a SLASH!

        String [] mountPointArray = mountPoint.split("/");
        StringBuffer myBuffer = null;

        try {
            // One occurence of "/", take the first 8 characters, then ...,
            // follow by the last 16 characters.
            if (mountPointArray.length == 2) {
                return new StringBuffer("/").append(
                    mountPointArray[1].substring(0, 8)).
                    append(DOTDOTDOT).append(mountPointArray[1].substring(
                        mountPointArray[1].length() -
                            (MAX_MOUNT_POINT_LENGTH - 12),
                        mountPointArray[1].length())).toString();
            }

            // There are at least 2 slashes in the string.
            // The first element of the array is always empty (start with
            // slash).  Check the second element of the array, try to use the
            // whole string unless it is more than 5, then depends on the size
            // of the second element, determine how many characters we want to
            // use for the last element.  DOTDOTDOT is going to be inserted in
            // the middle.

            myBuffer = new StringBuffer("/");

            // mountPointArray[0] should be empty because mount point is an
            // absolute path.  i.e. start with a slash

            if (mountPointArray[1].length() > 8) {
                myBuffer.append(mountPointArray[1].substring(0, 8));
            } else {
                // use the full element
                myBuffer.append(mountPointArray[1]);
                myBuffer.append("/");
            }

            // Append "..."
            myBuffer.append(DOTDOTDOT);

            int lastSize = 0;
            if (MAX_MOUNT_POINT_LENGTH - myBuffer.length() <
                mountPointArray[mountPointArray.length - 1].length()) {
                lastSize =
                    mountPointArray[mountPointArray.length - 1].length() -
                        (MAX_MOUNT_POINT_LENGTH - myBuffer.length() + 1);
            }

            myBuffer.append(
                mountPointArray[mountPointArray.length - 1].substring(
                    lastSize,
                    mountPointArray[mountPointArray.length - 1].length()));
        } catch (StringIndexOutOfBoundsException ex) {
            TraceUtil.trace1("StringIndexOutOfBoundsException CAUGHT!");
            TraceUtil.trace1("Reason: ex.getMessage()");
            return mountPoint;
        }
        return myBuffer.toString();
    }

    /**
     * To return the english phrase of a Shared File System Description
     */
    public static String getSharedFSDescriptionString(int sharedFSType) {
        switch (sharedFSType) {
            case SharedMember.TYPE_MD_SERVER:
                return SamUtil.getResourceString("SharedFSDetails.type.mds");
            case SharedMember.TYPE_POTENTIAL_MD_SERVER:
                return SamUtil.getResourceString("SharedFSDetails.type.pmds");
            case SharedMember.TYPE_CLIENT:
                return SamUtil.getResourceString("SharedFSDetails.type.client");
            default:
                return
                    SamUtil.getResourceString("SharedFSDetails.type.unknown");
        }
    }

    /**
     * This helper method grabs the root file system ("/", <ufs>) as we always
     * use this file system to set ALL NFS shares instead of grabbing any other
     * file systems to call the setNFSOptions API.
     */
    public static GenericFileSystem getRootFileSystem(
        SamQFSSystemModel sysModel) throws SamFSException {
        GenericFileSystem [] genfs =
            sysModel.getSamQFSSystemFSManager().getNonSAMQFileSystems();
        for (int i = 0; i < genfs.length; i++) {
            if ("/".equals(genfs[i].getMountPoint())) {
                return genfs[i];
            }
        }

        // If code reaches here, "/" directory is not found. Throw exception
        throw new SamFSException("Root directory not found!");
    }

    /**
     * Figure out the right icon to show based on copy type and damage status
     * Used in File Browser View and File Details Pop Up
     */
    public static String getImage(
        String copyStr, int mediaType, boolean isDamaged) {
        if (BaseDevice.MTYPE_DISK == mediaType) {
            // Disk Copy
            return isDamaged ?
                Constants.Image.ICON_DISK_PREFIX.concat(copyStr).
                    concat(Constants.Image.ICON_DAMAGED_SUFFIX) :
                Constants.Image.ICON_DISK_PREFIX.concat(copyStr).
                    concat(Constants.Image.ICON_SUFFIX);
        } else if (BaseDevice.MTYPE_STK_5800 == mediaType) {
            // Honeycomb ST5800 Copy
            return isDamaged ?
                Constants.Image.ICON_HONEYCOMB.concat(copyStr).
                    concat(Constants.Image.ICON_DAMAGED_SUFFIX) :
                Constants.Image.ICON_HONEYCOMB.concat(copyStr).
                    concat(Constants.Image.ICON_SUFFIX);
        } else if (-1 == mediaType) {
            // Developer's bug
            return "";
        } else {
            // Tape Copy
            return isDamaged ?
                Constants.Image.ICON_TAPE_PREFIX.concat(copyStr).
                    concat(Constants.Image.ICON_DAMAGED_SUFFIX) :
                Constants.Image.ICON_TAPE_PREFIX.concat(copyStr).
                    concat(Constants.Image.ICON_SUFFIX);
        }
    }

    /**
     * Method to retrieve file system name from the request, and save it in the
     * session.
     */
    public static String getFSName() {
        HttpServletRequest request = JSFUtil.getRequest();
        String fsName =
            request.getParameter(Constants.PageSessionAttributes.FS_NAME);
        JSFUtil.setAttribute(Constants.PageSessionAttributes.FS_NAME, fsName);

        return fsName;
    }
}
