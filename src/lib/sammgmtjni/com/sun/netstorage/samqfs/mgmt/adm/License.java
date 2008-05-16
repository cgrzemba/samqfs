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
// ident	$Id: License.java,v 1.9 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.adm;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.MdLicense;

/**
 *  this class provides access to SAM-FS/QFS license-related information
 */
public class License {
    // License information
    private short type   = -1;
    private short fsType = -1;
    private long expire  = -1;
    private long hostID  = -1;
    private int featureFlags = -1;   // bit mask of all feature available
    private MdLicense[] mdLicenses;

    /**
     * private constructor (no public constructor needed)
     */
    private License(short type, long expire, short fsType,
        long hostID, int featureFlags, MdLicense [] mdLicenses) {

            this.type = type;
            this.fsType = fsType;
            this.expire = expire;
            this.hostID = hostID;
            this.featureFlags = featureFlags;
            this.mdLicenses = mdLicenses;

    }

    // these constants below must match those in pub/mgmt/license.h
    // license expiration info
    public static final short NON_EXPIRING  = 0; // License never expires
    public static final short EXPIRING = 1; // License expires on exp_date
    public static final short DEMO = 2; // DEMO lic. exp. after 14 days
    public static final short QFS_TRIAL = 3; // License expires after 60days
    public static final short QFS_SPECIAL = 4; // License never expires

    // licensed FS type
    public static final short QFS    = 1; // stand alone QFS
    public static final short SAMFS  = 2; // SAM-FS
    public static final short SAMQFS = 3; // SAM-QFS

    // remote sam server supported
    private static final int REMOTE_SAM_SVR_SUPPORT  = 0x00000001;
    // remote sam client supported
    private static final int REMOTE_SAM_CLNT_SUPPORT = 0x00000002;
    // migration toolkit supported
    private static final int MIG_TOOLKIT_SUPPORT = 0x00000004;
    // fast file system supported
    private static final int FAST_FS_SUPPORT = 0x00000008;
    // database feature supported
    private static final int DATABASE_SUPPORT = 0x00000010;
    // foreign tape supported
    private static final int FOREIGN_TAPE_SUPPORT = 0x00000020;
    // shared file system supported
    private static final int SHARED_FS_SUPPORT = 0x00000040;
    // segment supported
    private static final int SEGMENT_SUPPORT = 0x00000080;
    // SAN API supported
    private static final int SANAPI_SUPPORT = 0x00000100;
    // standalone qfs supported
    private static final int STANDALONE_QFS_SUPPORT = 0x00000200;

    // public methods

    public short getLicenseType() { return type; }

    public long getExpiration() { return expire; }

    public short getFSType() { return fsType; }

    public long getHostID() { return hostID; }

    public boolean isRemoteSamServerSupported() {
        return (((featureFlags & License.REMOTE_SAM_SVR_SUPPORT)
            == License.REMOTE_SAM_SVR_SUPPORT) ? true : false);
    }

    public boolean isRemoteSamClientSupported() {
        return (((featureFlags & License.REMOTE_SAM_CLNT_SUPPORT)
            == License.REMOTE_SAM_CLNT_SUPPORT) ? true : false);
    }

    public boolean isMigrationToolkitSupported() {
        return (((featureFlags & License.MIG_TOOLKIT_SUPPORT)
            == License.MIG_TOOLKIT_SUPPORT) ? true : false);
    }

    public boolean isFastFileSystemSupported() {
        return (((featureFlags & License.FAST_FS_SUPPORT)
            == License.FAST_FS_SUPPORT) ? true : false);
    }

    public boolean isDatabaseSupported() {
        return (((featureFlags & License.DATABASE_SUPPORT)
            == License.DATABASE_SUPPORT) ? true : false);
    }

    public boolean isForeignTapeSupported() {
        return (((featureFlags & License.FOREIGN_TAPE_SUPPORT)
            == License.FOREIGN_TAPE_SUPPORT) ? true : false);
    }

    public boolean isSegmentSupported() {
        return (((featureFlags & License.SEGMENT_SUPPORT)
            == License.SEGMENT_SUPPORT) ? true : false);
    }

    public boolean isSharedFileSystemSupported() {
        return (((featureFlags & License.SHARED_FS_SUPPORT)
            == License.SHARED_FS_SUPPORT) ? true : false);
    }

    public boolean isSANAPISupported() {
        return (((featureFlags & License.SANAPI_SUPPORT)
            == License.SANAPI_SUPPORT) ? true : false);
    }

    public boolean isStandaloneQFSSupported() {
        return (((featureFlags & License.STANDALONE_QFS_SUPPORT)
            == License.STANDALONE_QFS_SUPPORT) ? true : false);
    }

    public MdLicense[] getMediaLicenses() { return mdLicenses; }

    public String toString() {
        String s = "License Type: " + fsType + " " + type + "," +
        " Expiration: " + expire +
        " Host ID: " + Long.toHexString(hostID) +
        "\nRemote server: " + isRemoteSamServerSupported() +
        "\nRemote client: " + isRemoteSamClientSupported() +
        "\nMigration: " + isMigrationToolkitSupported() +
        "\nQFS: " + isStandaloneQFSSupported() +
        "\nData Base features: " + isDatabaseSupported() +
        "\nForeign tape support: " + isForeignTapeSupported() +
        "\nSAN API support: " + isSANAPISupported() +
        "\nSegment feature: " + isSegmentSupported() +
        "\nShared filesystem support: " + isSharedFileSystemSupported() +
        "\nFast filesystem support: " + isFastFileSystemSupported() + "\n";

        if (mdLicenses != null) {
          for (int i = 0; i < mdLicenses.length; i++) {
              s += "   " + mdLicenses[i] + "\n";
          }
        } else {
              s += "  none\n";
        }

        return s;
    }

    public static native short getFSType(Ctx x) throws SamFSException;

    public static native License getLicense(Ctx c) throws SamFSException;
}
