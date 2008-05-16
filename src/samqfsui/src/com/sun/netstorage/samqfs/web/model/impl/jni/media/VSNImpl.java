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

// ident	$Id: VSNImpl.java,v 1.24 2008/05/16 18:39:03 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.media;


import java.util.GregorianCalendar;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.ReservInfo;
import com.sun.netstorage.samqfs.mgmt.media.LabelJob;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.VSN;

import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;


public class VSNImpl implements VSN {

    private SamQFSSystemModelImpl model = null;
    private CatEntry jniVSN = null;
    private int status = 0;

    private Library library = null;
    private int libEq = -1;
    private Drive drive = null;
    private String vsn = new String();
    private int slotNo = -1;
    private String barcode = new String();
    private long capacity = -1;
    private long availableSpace = -1;
    private long blockSize = -1;
    private long accessCount = -1;
    private GregorianCalendar labelTime = null;
    private GregorianCalendar mountTime = null;
    private GregorianCalendar modificationTime = null;
    private boolean reserved = false;
    private GregorianCalendar reservationTime = null;
    private String reservationPolicy = new String();
    private String reservationFS = new String();
    private int reservationType = -1;
    private String reservationNameForType = new String();
    private boolean mediaDamaged = false;
    private boolean duplicateVSN = false;
    private boolean readOnly = false;
    private boolean writeProtected = false;
    private boolean foreignMedia = false;
    private boolean recycled = false;
    private boolean volumeFull = false;
    private boolean unavailable = false;
    private boolean needAudit = false;


    public VSNImpl() {

    }


    public VSNImpl(SamQFSSystemModelImpl model, CatEntry jniVSN) {

        this.model = model;
        this.jniVSN = jniVSN;
        setup();

    }

    public Library getLibrary() throws SamFSException {

        Library lib = this.library;
        try {
            if ((model != null) && (libEq != -1)) {
                lib = model.getSamQFSSystemMediaManager().getLibraryByEQ(libEq);
            }
        } catch (Exception ex) {}

	return lib;

    }

    public Drive getDrive() throws SamFSException {

        Drive drv = this.drive;

        if ((model != null) && (jniVSN != null)) {
            DriveDev dev = Media.isVSNLoaded(model.getJniContext(),
                                             jniVSN.getVSN());
            if (dev != null) {
                drv = new DriveImpl(model, dev);
                this.drive = drv;
            }
        }

	return drv;

    }

    public String getVSN() throws SamFSException {

	return vsn;

    }


    public int getSlotNumber() throws SamFSException {

        return slotNo;

    }


    public String getBarcode() throws SamFSException {

	return barcode;

    }


    public long getCapacity() throws SamFSException {

	return capacity;

    }


    public long getAvailableSpace() throws SamFSException {

	return availableSpace;

    }

    public long getBlockSize() throws SamFSException {

	return blockSize;

    }

    public long getAccessCount() throws SamFSException {

	return accessCount;

    }

    public GregorianCalendar getLabelTime() throws SamFSException {

	return labelTime;

    }

    public GregorianCalendar getMountTime() throws SamFSException {

	return mountTime;

    }

    public GregorianCalendar getModificationTime() throws SamFSException {

	return modificationTime;

    }

    public boolean isReserved() throws SamFSException {

	return reserved;

    }

    public GregorianCalendar getReservationTime() throws SamFSException {

	return reservationTime;

    }

    public String getReservedByPolicyName() throws SamFSException {

        return reservationPolicy;

    }

    public String getReservedByFileSystemName() throws SamFSException {

        return reservationFS;

    }

    public int getReservationByType() throws SamFSException {

        return reservationType;

    }

    public String getReservationNameForType() throws SamFSException {

        return reservationNameForType;

    }

    // attributes of a VSN

    public boolean isMediaDamaged() throws SamFSException {

	return mediaDamaged;

    }

    public void setMediaDamaged(boolean mediaDamaged) throws SamFSException {

	if (mediaDamaged)
            status |= CatEntry.CES_bad_media;
        else
            status &= ~CatEntry.CES_bad_media;

    }

    public boolean isDuplicateVSN() throws SamFSException {

	return duplicateVSN;

    }

    public void setDuplicateVSN(boolean duplicateVSN) throws SamFSException {

	if (duplicateVSN)
            status |= CatEntry.CES_dupvsn;
        else
            status &= ~CatEntry.CES_dupvsn;

    }

    public boolean isReadOnly() throws SamFSException {

	return readOnly;

    }

    public void setReadOnly(boolean readOnly) throws SamFSException {

	if (readOnly)
            status |= CatEntry.CES_read_only;
        else
            status &= ~CatEntry.CES_read_only;

    }

    public boolean isWriteProtected() throws SamFSException {

	return writeProtected;

    }

    public void setWriteProtected(boolean writeProtected)
        throws SamFSException {

	if (writeProtected)
            status |= CatEntry.CES_writeprotect;
        else
            status &= ~CatEntry.CES_writeprotect;

    }

    public boolean isForeignMedia() throws SamFSException {

	return foreignMedia;

    }

    public void setForeignMedia(boolean foreignMedia) throws SamFSException {

	if (foreignMedia)
            status |= CatEntry.CES_non_sam;
        else
            status &= ~CatEntry.CES_non_sam;

    }

    public boolean isRecycled() throws SamFSException {

	return recycled;

    }

    public void setRecycled(boolean recycled) throws SamFSException {

	if (recycled)
            status |= CatEntry.CES_recycle;
        else
            status &= ~CatEntry.CES_recycle;

    }

    public boolean isVolumeFull() throws SamFSException {

	return volumeFull;

    }

    public void setVolumeFull(boolean volumeFull) throws SamFSException {

	if (volumeFull)
            status |= CatEntry.CES_archfull;
        else
            status &= ~CatEntry.CES_archfull;

    }

    public boolean isUnavailable() throws SamFSException {

	return unavailable;

    }

    public void setUnavailable(boolean unavailable) throws SamFSException {

	if (unavailable)
            status |= CatEntry.CES_unavail;
        else
            status &= ~CatEntry.CES_unavail;

    }

    public boolean isNeedAudit() throws SamFSException {

        return needAudit;
    }

    public void setNeedAudit(boolean needAudit) throws SamFSException {

        if (needAudit)
            status |= CatEntry.CES_needs_audit;
        else
            status &= ~CatEntry.CES_needs_audit;

    }

    // this method needs to be called to make effect of an attribute change
    public void changeAttributes() throws SamFSException {

        if (model != null) {
            Media.chgMediaStatus(model.getJniContext(), libEq, slotNo,
                                 true, status);
	    Media.chgMediaStatus(model.getJniContext(), libEq, slotNo,
                                 false, ~status);
            updateStatus();
        }

    }

    /**
     * The following operations are not tested against optical media.  It is
     * under the impression that we do not need to make any mods to support
     * these operations and SAM will take care of it.
     *
     * Operations: AUDIT, LOAD, EXPORT, LABEL
     */

    public void audit() throws SamFSException {

        if (model != null) {
            Media.auditSlot(model.getJniContext(), libEq, slotNo, -1, false);
        }

    }

    public void load() throws SamFSException {

        if (model != null) {
            Media.load(model.getJniContext(), libEq, slotNo, -1, false);
        }

    }

    public void export() throws SamFSException {

        if (model != null) {
            Media.exportCartridge(model.getJniContext(), libEq, slotNo, false);
        }

    }

    /**
     * type = VSN.LABEL for labeling, VSN.RELABEL for relabeling
     */
    public long label(int type, String labelName, long blockSize)
        throws SamFSException {

        long jobId = -1;

        if (SamQFSUtil.isValidString(labelName) && (model != null)) {
            String oldVSN = new String();
            if (type == VSN.RELABEL)
                oldVSN = vsn;
            Media.tapeLabel(model.getJniContext(), libEq, slotNo, -1,
                            labelName, oldVSN, blockSize, false, false);
            this.vsn = labelName;
            this.blockSize = blockSize;
            this.labelTime = new GregorianCalendar();

            LabelJob[] jniJobs = null;
            try {
                jniJobs = LabelJob.getAll(model.getJniContext());
            } catch (Exception e) {
                model.processException(e);
            }

            if ((jniJobs != null) && (jniJobs.length > 0)) {
                for (int i = 0; i < jniJobs.length; i++) {
                    String loadedVSN = jniJobs[i].getDrive().getLoadedVSN();
                    if ((oldVSN.equals(loadedVSN)) ||
                        (vsn.equals(loadedVSN))) {
                        jobId = jniJobs[i].getID();
                        break;
                    }
                }
            }

        }

        return jobId;

    }

    public void clean() throws SamFSException {

        Drive drv = getDrive();
        if ((model != null) && (drv != null)) {
            Media.cleanDrive(model.getJniContext(), drv.getEquipOrdinal());
        }

    }


    /**
     * type = VSN.OWNER, VSN.GROUP, VSN.STARTING_DIR
     */
    public void reserve(String policyName, String filesystemName,
                        int type, String name) throws SamFSException {

        if (SamQFSUtil.isValidString(policyName))
            reservationPolicy = policyName;
        else
            reservationPolicy = null;
        if (SamQFSUtil.isValidString(filesystemName))
            reservationFS = filesystemName;
        else
            reservationFS = null;
        if ((type == VSN.OWNER) || (type == VSN.GROUP) ||
            (type == VSN.STARTING_DIR)) {
            reservationType = type;
            reservationNameForType = name;
        } else
            reservationNameForType = null;

        long time = SamQFSUtil.convertTime(new GregorianCalendar());
        ReservInfo info =
            new ReservInfo(time, reservationPolicy, reservationNameForType,
                           reservationFS);

        // changes needed for optical media.
        // TBD
        if (model != null) {
            if ((reservationPolicy != null) || (reservationFS != null) ||
                (reservationNameForType != null)) {
                Media.reserve(model.getJniContext(), libEq, slotNo, -1, info);
            } else if (model != null) {
                Media.unreserve(model.getJniContext(), libEq, slotNo, -1);
            }

            CatEntry[] cat = Media.getCatEntriesForVSN(model.getJniContext(),
                                                       jniVSN.getVSN());

            if (cat.length == 1) {
                jniVSN = cat[0];
            } else {
                // more than one vsn exists for the given vsn name
                int targetEq = jniVSN.getLibraryEqu();
                int match = -1;
                for (int i = 0; i < cat.length; i++) {
                    if (targetEq == cat[i].getLibraryEqu()) {
                        match = i;
                        break;
                    }
                }
                if (match != -1)
                    jniVSN = cat[match];
            }
            setup();

        }

    }

    public String toString() {

        StringBuffer buf = new StringBuffer();

        if (library != null) {
            try {
                buf.append("Library: " + library.getName() + "\n\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        if (drive != null) {
            try {
                buf.append("Drive: " + drive.toString() + "\n\n");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }


        buf.append("VSN: " + vsn + "\n");
        buf.append("Slot: " + slotNo + "\n");
        buf.append("Barcode: " + barcode + "\n");
        buf.append("Capacity: " + capacity + "\n");
        buf.append("Available Space: " + availableSpace + "\n");
        buf.append("Block Size: " + blockSize + "\n");
        buf.append("Label Time: " + SamQFSUtil.dateTime(labelTime) + "\n");
        buf.append("Mount Time: " + SamQFSUtil.dateTime(mountTime) + "\n");
        buf.append("Modification Time: " +
                    SamQFSUtil.dateTime(modificationTime) + "\n");
        buf.append("Reserved: " + reserved + "\n");
        buf.append("Reservation Time: " +
                    SamQFSUtil.dateTime(reservationTime) + "\n");
        buf.append("Media Damaged: " + mediaDamaged + "\n");
        buf.append("Duplicate VSN: " + duplicateVSN + "\n");
        buf.append("Read Only: " + readOnly + "\n");
        buf.append("Write Protected: " + writeProtected + "\n");
        buf.append("Foreign Media: " + foreignMedia + "\n");
        buf.append("Recycled: " + recycled + "\n");
        buf.append("Volume Full: " + volumeFull + "\n");
        buf.append("Unavailable: " + unavailable + "\n");
        buf.append("Need audit: " + needAudit + "\n");

        return buf.toString();

    }

    private void setup() {

        if (jniVSN != null) {

            status = jniVSN.getStatus();
            libEq = jniVSN.getLibraryEqu();
            vsn = jniVSN.getVSN();
            slotNo = jniVSN.getSlot();
            barcode = jniVSN.getBarcode();

            capacity = jniVSN.getCapacity();
            availableSpace = jniVSN.getFreeSpace();

            blockSize =
                SamQFSUtil.convertSize(jniVSN.getBlockSz(),
                                       SamQFSSystemModel.SIZE_B,
                                       SamQFSSystemModel.SIZE_KB);
            accessCount = jniVSN.getAccessCount();

            labelTime = SamQFSUtil.convertTime(jniVSN.getLabelTime());
            mountTime = SamQFSUtil.convertTime(jniVSN.getMountTime());
            modificationTime = SamQFSUtil.convertTime(jniVSN.getModTime());

            // reservation settings
            ReservInfo jniRes = jniVSN.getReservationInfo();
            if (jniRes != null) {
                reservationNameForType = jniRes.getResOwner();
                if (SamQFSUtil.isValidString(reservationNameForType)) {
                    reservationType = VSN.OWNER;
                }
                reservationPolicy = jniRes.getResCopyName();
                if (SamQFSUtil.isValidString(reservationPolicy)) {
                    reservationPolicy =
                        SamQFSUtil.getCriteriaName(reservationPolicy);
                }
                reservationFS = jniRes.getResFS();

                if ((SamQFSUtil.isValidString(reservationNameForType)) ||
                    (SamQFSUtil.isValidString(reservationPolicy)) ||
                    (SamQFSUtil.isValidString(reservationFS))) {
                    reserved = true;
                    reservationTime = SamQFSUtil.
                        convertTime(jniRes.getResTime());
                }
            }

            updateStatus();
        }

    }

    private void updateStatus() {

        mediaDamaged =
            (status & CatEntry.CES_bad_media) == CatEntry.CES_bad_media;
        duplicateVSN =
            (status & CatEntry.CES_dupvsn) == CatEntry.CES_dupvsn;
        readOnly =
            (status & CatEntry.CES_read_only) == CatEntry.CES_read_only;
        writeProtected =
            (status & CatEntry.CES_writeprotect) == CatEntry.CES_writeprotect;
        foreignMedia =
            (status & CatEntry.CES_non_sam) == CatEntry.CES_non_sam;
        recycled =
            (status & CatEntry.CES_recycle) == CatEntry.CES_recycle;
        volumeFull =
            (status & CatEntry.CES_archfull) == CatEntry.CES_archfull;
        unavailable =
            (status & CatEntry.CES_unavail) == CatEntry.CES_unavail;
        needAudit =
            (status & CatEntry.CES_needs_audit) == CatEntry.CES_needs_audit;
    }
}
