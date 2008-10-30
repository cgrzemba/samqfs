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

// ident    $Id: MountOptions.java,v 1.25 2008/10/30 14:42:29 pg125177 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;


public class MountOptions {

    /* private data */
    private int chgFlags = 0;
    private int ioChgFlags = 0;
    private int samChgFlags = 0;
    private int sharedfsChgFlags = 0;
    private int multirdChgFlags = 0;
    private int qfsChgFlags = 0;
    private int post42ChgFlags = 0;

    // general mount options change flags
    private static final int MNT_SYNC_META = 0x00000001;
    private static final int MNT_SUID = 0x00000002;
    private static final int MNT_NOSUID = 0x00000004;
    private static final int MNT_TRACE = 0x00000008;
    private static final int MNT_NOTRACE = 0x00000010;
    private static final int MNT_STRIPE = 0x00000020;
    private static final int MNT_READONLY = 0x00000040;
    private static final int MNT_QUOTA = 0x00000080;
    private static final int MNT_NOQUOTA = 0x00000100;
    // ... and clear flags
    private static final int CLR_SYNC_META = 0x00010001;
    private static final int CLR_SUID = 0x00060006;
    private static final int CLR_NOSUID = 0x00060006;
    private static final int CLR_TRACE = 0x00180018;
    private static final int CLR_NOTRACE = 0x00180018;
    private static final int CLR_STRIPE = 0x00200020;
    private static final int CLR_READONLY = 0x00400040;
    private static final int CLR_QUOTA = 0x01800180;
    private static final int CLR_NOQUOTA = 0x01800180;

    // io change flags
    private static final int MNT_DIO_RD_CONSEC   = 0x00000001;
    private static final int MNT_DIO_RD_FORM_MIN = 0x00000002;
    private static final int MNT_DIO_RD_ILL_MIN  = 0x00000004;
    private static final int MNT_DIO_WR_CONSEC   = 0x00000008;
    private static final int MNT_DIO_WR_FORM_MIN = 0x00000010;
    private static final int MNT_DIO_WR_ILL_MIN  = 0x00000020;
    private static final int MNT_FORCEDIRECTIO   = 0x00000040;
    private static final int MNT_NOFORCEDIRECTIO = 0x00000080;
    private static final int MNT_SW_RAID = 0x00000100;
    private static final int MNT_NOSW_RAID = 0x00000200;
    private static final int MNT_FLUSH_BEHIND = 0x00000400;
    private static final int MNT_READAHEAD = 0x00000800;
    private static final int MNT_WRITEBEHIND = 0x00001000;
    private static final int MNT_WR_THROTTLE = 0x00002000;
    private static final int MNT_FORCENFSASYNC = 0x00004000;
    private static final int MNT_NOFORCENFSASYNC = 0x00008000;

    // ... and clear flags
    private static final int CLR_DIO_RD_CONSEC   = 0x00010001;
    private static final int CLR_DIO_RD_FORM_MIN = 0x00020002;
    private static final int CLR_DIO_RD_ILL_MIN  = 0x00040004;
    private static final int CLR_DIO_WR_CONSEC   = 0x00080008;
    private static final int CLR_DIO_WR_FORM_MIN = 0x00100010;
    private static final int CLR_DIO_WR_ILL_MIN  = 0x00200020;
    private static final int CLR_FORCEDIRECTIO   = 0x00C000C0;
    private static final int CLR_NOFORCEDIRECTIO = 0x00C000C0;
    private static final int CLR_SW_RAID = 0x03000300;
    private static final int CLR_NOSW_RAID = 0x03000300;
    private static final int CLR_FLUSH_BEHIND = 0x04000400;
    private static final int CLR_READAHEAD = 0x08000800;
    private static final int CLR_WRITEBEHIND = 0x10001000;
    private static final int CLR_WR_THROTTLE = 0x20002000;
    private static final int CLR_FORCENFSASYNC = 0xC000C000;
    private static final int CLR_NOFORCENFSASYNC = 0xC000C000;

    // sam_mount_option_t change flags
    private static final int MNT_HIGH = 0x00000001;
    private static final int MNT_LOW = 0x00000002;
    private static final int MNT_PARTIAL = 0x00000004;
    private static final int MNT_MAXPARTIAL = 0x00000008;
    private static final int MNT_PARTIAL_STAGE = 0x00000010;
    private static final int MNT_STAGE_N_WINDOW = 0x00000020;
    private static final int MNT_STAGE_RETRIES = 0x00000040;
    private static final int MNT_STAGE_FLUSH_BEHIND = 0x00000080;
    private static final int MNT_HWM_ARCHIVE = 0x00000100;
    private static final int MNT_NOHWM_ARCHIVE = 0x00000200;
    private static final int MNT_ARCHIVE = 0x00000400;
    private static final int MNT_NOARCHIVE = 0x00000800;
    // ... and clear flags
    private static final int CLR_HIGH = 0x00010001;
    private static final int CLR_LOW  = 0x00020002;
    private static final int CLR_PARTIAL = 0x00040004;
    private static final int CLR_MAXPARTIAL = 0x00080008;
    private static final int CLR_PARTIAL_STAGE = 0x00100010;
    private static final int CLR_STAGE_N_WINDOW = 0x00200020;
    private static final int CLR_STAGE_RETRIES = 0x00400040;
    private static final int CLR_STAGE_FLUSH_BEHIND = 0x00800080;
    private static final int CLR_HWM_ARCHIVE = 0x03000300;
    private static final int CLR_NOHWM_ARCHIVE = 0x03000300;
    private static final int CLR_ARCHIVE = 0x06000600;
    private static final int CLR_NOARCHIVE = 0x06000600;

    // sharedfs_mount_option_t change flags
    private static final int MNT_SHARED = 0x00000001;
    private static final int MNT_BG = 0x00000002;
    private static final int MNT_RETRY = 0x00000004;
    private static final int MNT_MINALLOCSZ = 0x00000008;
    private static final int MNT_MAXALLOCSZ = 0x00000010;
    private static final int MNT_RDLEASE = 0x00000020;
    private static final int MNT_WRLEASE = 0x00000040;
    private static final int MNT_APLEASE = 0x00000080;
    private static final int MNT_MH_WRITE = 0x00000100;
    private static final int MNT_NOMH_WRITE = 0x00000200;
    private static final int MNT_NSTREAMS = 0x00000400;
    private static final int MNT_META_TIMEO = 0x00000800;
    private static final int MNT_LEASE_TIMEO = 0x00001000;
    // ... and clear flags
    private static final int CLR_SHARED = 0x00010001;
    private static final int CLR_BG = 0x00020002;
    private static final int CLR_RETRY = 0x00040004;
    private static final int CLR_MINALLOCSZ = 0x00080008;
    private static final int CLR_MAXALLOCSZ = 0x00100010;
    private static final int CLR_RDLEASE = 0x00200020;
    private static final int CLR_WRLEASE = 0x00400040;
    private static final int CLR_APLEASE = 0x00800080;
    private static final int CLR_MH_WRITE = 0x03000300;
    private static final int CLR_NOMH_WRITE = 0x03000300;
    private static final int CLR_NSTREAMS = 0x04000400;
    private static final int CLR_META_TIMEO = 0x08000800;
    private static final int CLR_LEASE_TIMEO = 0x10001000;


    // multireader_mount_options_t change flags
    private static final int MNT_WRITER = 0x00000001;
    private static final int MNT_SHARED_WRITER = 0x00000002;
    private static final int MNT_READER = 0x00000004;
    private static final int MNT_SHARED_READER = 0x00000008;
    private static final int MNT_INVALID = 0x00000010;
    // ... and clear flags
    private static final int CLR_WRITER = 0x00010001;
    private static final int CLR_SHARED_WRITER = 0x00020002;
    private static final int CLR_READER = 0x00040004;
    private static final int CLR_SHARED_READER = 0x00080008;
    private static final int CLR_INVALID = 0x00100010;

    // qfs_mount_options_t change flags
    private static final int MNT_QWRITE = 0x00000001;
    private static final int MNT_NOQWRITE = 0x00000002;
    private static final int MNT_MM_STRIPE = 0x00000004;
    // ... and clear flags
    private static final int CLR_QWRITE = 0x00010001;
    private static final int CLR_NOQWRITE = 0x00020002;
    private static final int CLR_MM_STRIPE = 0x00040004;

    /* post_4_2_options_t change flags */
    private static final int MNT_DEF_RETENTION = 0x00000001;
    private static final int MNT_ABR = 0x00000002;
    private static final int MNT_NOABR = 0x00000004;
    private static final int MNT_DMR = 0x00000008;
    private static final int MNT_NODMR = 0x00000010;
    private static final int MNT_DIO_SZERO = 0x00000020;
    private static final int MNT_NODIO_SZERO = 0x00000040;
    private static final int MNT_CATTR = 0x00000080;
    private static final int MNT_NOCATTR = 0x00000100;

    // ... and clear flags
    private static final int CLR_DEF_RETENTION = 0x00010001;
    private static final int CLR_ABR = 0x00060006;
    private static final int CLR_NOABR = 0x00060006;
    private static final int CLR_DMR = 0x00180018;
    private static final int CLR_NODMR = 0x00180018;
    private static final int CLR_DIO_SZERO = 0x00600060;
    private static final int CLR_NODIO_SZERO = 0x00600060;
    private static final int CLR_CATTR = 0x01800180;
    private static final int CLR_NOCATTR = 0x01800180;

    /* rel_4_6_options_t change flags */
    private static final int MNT_WORM_EMUL = 0x00000001;
    private static final int MNT_WORM_LITE = 0x00000002;
    private static final int MNT_WORM_EMUL_LITE = 0x00000004;
    private static final int MNT_CDEVID = 0x00000008;
    private static final int MNT_NOCDEVID = 0x00000010;
    private static final int MNT_CLMGMT = 0x00000020;
    private static final int MNT_NOCLMGMT = 0x00000040;
    private static final int MNT_CLFASTSW = 0x00000080;
    private static final int MNT_NOCLFASTSW = 0x00000100;
    private static final int MNT_NOATIME = 0x00000200;
    private static final int MNT_ATIME = 0x00000400;
    private static final int MNT_MIN_POOL = 0x00000800;

    private static final int CLR_WORM_EMUL = 0x00010001;
    private static final int CLR_WORM_LITE = 0x00020002;
    private static final int CLR_WORM_EMUL_LITE = 0x00040004;
    private static final int CLR_CDEVID = 0x00180018;
    private static final int CLR_NOCDEVID = 0x00180018;
    private static final int CLR_CLMGMT = 0x00600060;
    private static final int CLR_NOCLMGMT = 0x00600060;
    private static final int CLR_CLFASTSW = 0x01800180;
    private static final int CLR_NOCLFASTSW = 0x01800180;
    private static final int CLR_NOATIME = 0x02000200;
    private static final int CLR_ATIME = 0x04000400;
    private static final int CLR_MIN_POOL = 0x08000800;

    /* rel_5_0_options_t change_flags */
    private static final int MNT_OBJ_WIDTH		= 0x00000001;
    private static final int MNT_OBJ_DEPTH		= 0x00000002;
    private static final int MNT_OBJ_POOL		= 0x00000004;
    private static final int MNT_OBJ_SYNC_DATA		= 0x00000008;
    private static final int MNT_LOGGING		= 0x00000010;
    private static final int MNT_NOLOGGING		= 0x00000020;
    private static final int MNT_SAM_DB			= 0x00000040;
    private static final int MNT_NOSAM_DB		= 0x00000080;
    private static final int MNT_XATTR			= 0x00000100;
    private static final int MNT_NOXATTR		= 0x00000200;

    private static final int 	CLR_OBJ_WIDTH		= 0x00010001;
    private static final int 	CLR_OBJ_DEPTH		= 0x00020002;
    private static final int 	CLR_OBJ_POOL		= 0x00040004;
    private static final int 	CLR_OBJ_SYNC_DATA	= 0x00080008;
    private static final int 	CLR_LOGGING		= 0x00300030;
    private static final int 	CLR_NOLOGGING		= 0x00300030;
    private static final int 	CLR_SAM_DB		= 0x00C000C0;
    private static final int 	CLR_NOSAM_DB		= 0x00C000C0;
    private static final int 	CLR_XATTR		= 0x03000300;
    private static final int 	CLR_NOXATTR		= 0x03000300;



    /* private instance fields */

    // basic options
    private short high, low, stripeWidth;
    private boolean trace;
    private boolean quota;
    private int rd_ino_buf_size;
    private int wr_ino_buf_size;

    // general options
    private boolean readOnly, noSetUID;
    // SAM options
    private int partialRelKb, maxPartialRelKb;
    private int stageRetries, stageFlushBehindKb;
    private long stageWinKb, partialStageKb;
    private boolean arcAutorun;
    private boolean archive;
    private boolean arscan;

    // sharedfs options
    private boolean backgr, multiWrite;
    private long minBlocks, maxBlocks; /* 1024 byte blocks. must be mul.of 8 */
    private int rdLease, wrLease, appLease;
    private short retry;
    private int nStreams, metaTimeout, leaseTimeout;
    // QFS options
    private boolean quickWrite;
    // performance tuning (metadata)
    private short syncMeta;
    private short metaStripeWidth;
    // performance tuning (I/O)
    private long rdAheadKb, wrBehindKb, wrThrottleKb;
    private int flushBehindKb;
    private boolean forceNFSAsync;
    private boolean softRAID, forceDIO;

    // direct I/O options
    private int dioRdConsec, dioWrConsec;
    private int dioRdFormMinKb, dioRdIllMinKb, dioWrFormMinKb, dioWrIllMinKb;

    // multireader options (not currently used by upper layers)
    private boolean reader, writer;
    private int invalid;
    private boolean refresh_at_eof;

    // post 4.2 options (not currently supported by upper layers)
    private int defRetention;
    // abr, dmr, dio_szero, cattr
    private boolean appBasedRecovery, directedMirrorReads, directIOZeroing;
    private boolean consistencyChecking;

    // Release 4.6 options
    private int rel46ChgFlags;
    private boolean worm_emul;
    private boolean worm_lite;
    private boolean emul_lite;
    private boolean cdevid;
    private boolean clustermgmt;
    private boolean clusterfastsw;
    private boolean noatime;
    private short atime;
    private int min_pool;

    // Release 5.0 options
    private int rel50ChgFlags;
    private int objWidth;
    private long objDepth;
    private int objPool;
    private int objSyncData;
    private boolean logging;
    private boolean samDB;
    private boolean xattr;


    /**
     * private constructor
     */
    private MountOptions(
        // general options
        boolean readOnly,
        short syncMeta, boolean noSetUID, short stripeWidth, boolean trace,
        boolean quota, int rd_ino_buf_size, int wr_ino_buf_size, int chgFlags,
        // SAM options
        short high, short low, int partialRelKb, int maxPartialRelKb,
        long partialStageKb, long stageWinKb, int stageRetries,
        int stageFlushBehindKb, boolean arcAutorun, boolean archive,
        boolean arscan, int samChgFlags,
        // shared FS options
        boolean backgr, short retry, long minBlocks, long maxBlocks,
        int rdLease, int wrLease, int appLease, boolean multiWrite,
        int nStreams, int metaTimeout, int leaseTimeout, int sharedfsChgFlags,
        // multireader options
        boolean writer, boolean reader, int invalid, boolean refresh_at_eof,
        int multirdChgFlags,
        // qfs options
        boolean quickWrite, short metaStripeWidth, int qfsChgFlags,
        // I/O options
        int dioRdConsec, int dioRdFormMinKb, int dioRdIllMinKb,
        int dioWrConsec, int dioWrFormMinKb, int dioWrIllMinKb,
        boolean forceDIO, boolean softRAID, int flushBehindKb,
        long rdAheadKb, long wrBehindKb, long wrThrottleKb,
        boolean forceNFSAsync, int ioChgFlags,
        // Post 4.2 options
        int post42ChgFlags,
        int defRetention,
        boolean appBasedRecovery,
        boolean directedMirrorReads,
        boolean directIOZeroing,
        boolean consistencyChecking,
    // Release 4.6 options
        int rel46ChgFlags,
        boolean worm_emul,
        boolean worm_lite,
        boolean emul_lite,
        boolean cdevid,
        boolean clustermgmt,
        boolean clusterfastsw,
        boolean noatime,
        short atime,
        int min_pool,
	// Release 5.0 Options
	int rel50ChgFlags,
	int objWidth,
	long objDepth,
	int objPool,
	int objSyncData,
	boolean logging,
	boolean samDB,
	boolean xattr) {
            this.readOnly = readOnly;
            this.syncMeta = syncMeta;
            this.noSetUID = noSetUID;
            this.stripeWidth = stripeWidth;
            this.trace = trace;
            this.quota = quota;
            this.rd_ino_buf_size = rd_ino_buf_size;
            this.wr_ino_buf_size = wr_ino_buf_size;
            this.chgFlags = chgFlags;
            this.high = high;
            this.low  = low;
            this.partialRelKb    = partialRelKb;
            this.maxPartialRelKb = maxPartialRelKb;
            this.partialStageKb  = partialStageKb;
            this.stageWinKb = stageWinKb;
            this.stageRetries = stageRetries;
            this.stageFlushBehindKb = stageFlushBehindKb;
            this.arcAutorun  = arcAutorun;
            this.archive = archive;
            this.arscan = arscan;
            this.samChgFlags = samChgFlags;
            // shared QFS
            this.backgr = backgr;
            this.retry  = retry;
            this.minBlocks   = minBlocks;
            this.maxBlocks   = maxBlocks;
            this.rdLease = rdLease;
            this.wrLease = wrLease;
            this.appLease = appLease;
            this.multiWrite = multiWrite;
            this.nStreams = nStreams;
            this.metaTimeout = metaTimeout;
	    this.leaseTimeout = leaseTimeout;
            this.sharedfsChgFlags = sharedfsChgFlags;
            // multi reader
            this.writer  = writer;
            this.reader  = reader;
            this.invalid = invalid;
            this.refresh_at_eof = refresh_at_eof;
            this.multirdChgFlags = multirdChgFlags;
            // qfs options
            this.quickWrite = quickWrite;
            this.metaStripeWidth = metaStripeWidth;
            this.qfsChgFlags = qfsChgFlags;
            // I/O
            this.dioRdConsec = dioRdConsec;
            this.dioRdFormMinKb = dioRdFormMinKb;
            this.dioRdIllMinKb  = dioRdIllMinKb;
            this.dioWrConsec    = dioWrConsec;
            this.dioWrFormMinKb = dioWrFormMinKb;
            this.dioWrIllMinKb  = dioWrIllMinKb;
            this.forceDIO = forceDIO;
            this.softRAID = softRAID;
            this.flushBehindKb  = flushBehindKb;
            this.rdAheadKb  = rdAheadKb;
            this.wrBehindKb = wrBehindKb;
            this.wrThrottleKb = wrThrottleKb;
	    this.forceNFSAsync  = forceNFSAsync;
            this.ioChgFlags = ioChgFlags;
            // Post4.2
            this.defRetention = defRetention;
            this.appBasedRecovery = appBasedRecovery;
            this.directedMirrorReads = directedMirrorReads;
            this.directIOZeroing = directIOZeroing;
            this.consistencyChecking = consistencyChecking;
            this.post42ChgFlags  = post42ChgFlags;
            // release 4.6
            this.rel46ChgFlags  = rel46ChgFlags;
            this.worm_emul = worm_emul;
            this.worm_lite = worm_lite;
            this.emul_lite = emul_lite;
            this.cdevid    = cdevid;
            this.clustermgmt = clustermgmt;
            this.clusterfastsw  = clusterfastsw;
            this.noatime = noatime;
            this.atime  = atime;
            this.min_pool = min_pool;
	    this.rel50ChgFlags = rel50ChgFlags;
	    this.objWidth = objWidth;
	    this.objDepth = objDepth;
	    this.objPool = objPool;
	    this.objSyncData = objSyncData;
	    this.logging = logging;
	    this.samDB = samDB;
	    this.xattr = xattr;
    }


    /*
     * public constructor, sets only the basic SAM-FS/QFS mount options
     */
    public MountOptions(short high, short low, short stripeWidth,
        boolean trace) {
        setHWM(high);
        setLWM(low);
        setStripeWidth(stripeWidth);
        setTrace(trace);
    }
    public MountOptions() { }

    /* public methods */

    // basic mount options

    public short getHWM() { return high; }
    public void resetHWM() {
        samChgFlags |= CLR_HIGH;
    }
    public void setHWM(short high) {
        this.high = high;
        samChgFlags = samChgFlags & ~CLR_HIGH | MNT_HIGH;
    }

    public short getLWM() { return low; }
    public void resetLWM() {
        samChgFlags |= CLR_LOW;
    }
    public void setLWM(short low) {
        this.low = low;
        samChgFlags = samChgFlags & ~CLR_LOW | MNT_LOW;
    }

    public short getStripeWidth() { return stripeWidth; }
    public void resetStripeWidth() {
        chgFlags |= CLR_STRIPE;
    }
    public void setStripeWidth(short stripeWidth) {
        this.stripeWidth = stripeWidth;
        chgFlags = chgFlags & ~CLR_STRIPE | MNT_STRIPE;
    }

    public boolean isTrace() { return trace; }
    public void resetTrace() {
        chgFlags |= CLR_TRACE;
    }
    public void setTrace(boolean traceOn) {
        trace = traceOn;
        if (trace)
            chgFlags = chgFlags & ~CLR_TRACE | MNT_TRACE;
        else
            chgFlags = chgFlags & ~CLR_NOTRACE | MNT_NOTRACE;
    }

    // general file system mount options

    public boolean isReadOnlyMount() { return readOnly; }
    public void resetReadOnlyMount() {
        chgFlags |= CLR_READONLY;
    }
    public void setReadOnlyMount(boolean readOnly) {
        this.readOnly = readOnly;
        chgFlags = chgFlags & ~CLR_READONLY | MNT_READONLY;
    }

    public boolean isNoSetUID() { return noSetUID; }
    public void resetNoSetUID() {
        chgFlags |= CLR_NOSUID;
    }
    public void setNoSetUID(boolean noSetUID) {
        this.noSetUID = noSetUID;
        chgFlags &= chgFlags & ~CLR_NOSUID;
        chgFlags |= (noSetUID ? MNT_NOSUID : MNT_SUID);
    }

    public boolean isQuickWrite() { return quickWrite; }
    public void resetQuickWrite() {
        qfsChgFlags |= CLR_QWRITE;
    }
    public void setQuickWrite(boolean quickWrite) {
        this.quickWrite = quickWrite;
        qfsChgFlags &= ~CLR_QWRITE;
        qfsChgFlags |= (quickWrite ? MNT_QWRITE : MNT_NOQWRITE);
    }


    // SAM mount options (except basic options - hwm&lwm) */

    public int getDefaultPartialReleaseSize() { return partialRelKb; }
    public void resetDefaultPartialReleaseSize() {
        samChgFlags |= CLR_PARTIAL;
    }
    public void setDefaultPartialReleaseSize(int partialRelKb) {
        this.partialRelKb = partialRelKb;
        samChgFlags = samChgFlags & ~CLR_PARTIAL | MNT_PARTIAL;
    }

    public int getDefaultMaxPartialReleaseSize() { return maxPartialRelKb; }
    public void resetDefaultMaxPartialReleaseSize() {
        samChgFlags |= CLR_MAXPARTIAL;
    }
    public void setDefaultMaxPartialReleaseSize(int maxPartialRelKb) {
        this.maxPartialRelKb = maxPartialRelKb;
        samChgFlags = samChgFlags & ~CLR_MAXPARTIAL | MNT_MAXPARTIAL;
    }

    public long getPartialStageSize() { return partialStageKb; }
    public void resetPartialStageSize() {
        samChgFlags |= CLR_PARTIAL_STAGE;
    }
    public void setPartialStageSize(long partialStageKb) {
        this.partialStageKb = partialStageKb;
        samChgFlags = samChgFlags & ~CLR_PARTIAL_STAGE | MNT_PARTIAL_STAGE;
    }

    public int getNoOfStageRetries() { return stageRetries; }
    public void resetNoOfStageRetries() {
        samChgFlags |= CLR_STAGE_RETRIES;
    }
    public void setNoOfStageRetries(int stageRetries) {
        this.stageRetries = stageRetries;
        samChgFlags = samChgFlags & ~CLR_STAGE_RETRIES | MNT_STAGE_RETRIES;
    }

    public long getStageWindowSize() { return stageWinKb; }
    public void resetStageWindowSize() {
        samChgFlags |= CLR_STAGE_N_WINDOW;
    }
    public void setStageWindowSize(long stageWinKb) {
         this.stageWinKb = stageWinKb;
         samChgFlags = samChgFlags & ~CLR_STAGE_N_WINDOW | MNT_STAGE_N_WINDOW;
    }

    public boolean isArchiverAutoRun() { return arcAutorun; }

    public void resetArchiverAutoRun() {
        samChgFlags |= CLR_HWM_ARCHIVE;
    }

    public void setArchiverAutoRun(boolean arcAutorun) {
        this.arcAutorun = arcAutorun;
        samChgFlags &= ~CLR_HWM_ARCHIVE;
        samChgFlags |= (arcAutorun ? MNT_HWM_ARCHIVE : MNT_NOHWM_ARCHIVE);
    }

    public boolean isArchive() { return archive; }

    public void resetArchive() {
        samChgFlags |= CLR_ARCHIVE;
    }

    public void setArchive(boolean archive) {
        this.archive = archive;
        samChgFlags &= ~CLR_ARCHIVE;
        samChgFlags |= (archive ? MNT_ARCHIVE : MNT_NOARCHIVE);
    }


    // sharedfs mount properties

    public boolean isMountInBackground() { return backgr; }
    public void resetMountInBackground() {
        sharedfsChgFlags |= CLR_BG; }
    public void setMountInBackground(boolean backgr) {
        this.backgr = backgr;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_BG | MNT_BG;
    }

    public short getNoOfMountRetries() { return retry; }
    public void resetNoOfMountRetries() {
        sharedfsChgFlags |= CLR_RETRY;
    }
    public void setNoOfMountRetries(short retry) {
        this.retry = retry;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_RETRY | MNT_RETRY;
    }

    public int getMetadataRefreshRate() { return metaTimeout; }
    public void resetMedataRefreshRate() {
        sharedfsChgFlags |= CLR_META_TIMEO;
    }
    public void setMetadataRefreshRate(int metaTimeout) {
        this.metaTimeout = metaTimeout;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_META_TIMEO | MNT_META_TIMEO;
    }

    public long getMinBlockAllocation() { return minBlocks; }
    public void resetMinBlockAllocation() {
        sharedfsChgFlags |= CLR_MINALLOCSZ; }
    public void setMinBlockAllocation(long minBlocks) {
        this.minBlocks = minBlocks;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_MINALLOCSZ | MNT_MINALLOCSZ;
    }

    public long getMaxBlockAllocation() { return maxBlocks; }
    public void resetMaxBlockAllocation() {
        sharedfsChgFlags |= CLR_MAXALLOCSZ;
    }
    public void setMaxBlockAllocation(long maxBlocks) {
        this.maxBlocks = maxBlocks;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_MAXALLOCSZ | MNT_MAXALLOCSZ;
    }

    public int getReadLeaseDuration() { return rdLease; }
    public void resetReadLeaseDuration() {
        sharedfsChgFlags |= CLR_RDLEASE;
    }
    public void setReadLeaseDuration(int rdLease) {
        this.rdLease = rdLease;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_RDLEASE | MNT_RDLEASE;
    }

    public int getWriteLeaseDuration() { return wrLease; }
    public void resetWriteLeaseDuration() {
        sharedfsChgFlags |= CLR_WRLEASE;
    }
    public void setWriteLeaseDuration(int wrLease) {
        this.wrLease = wrLease;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_WRLEASE | MNT_WRLEASE;
    }

    public int getAppendLeaseDuration() { return appLease; }
    public void resetAppendLeaseDuration() {
        sharedfsChgFlags |= CLR_APLEASE;
    }
    public void setAppendLeaseDuration(int appLease) {
        this.appLease = appLease;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_APLEASE | MNT_APLEASE;
    }

    public int getLeaseTimeout() { return leaseTimeout; }
    public void resetLeaseTimeout() {
        sharedfsChgFlags |= CLR_LEASE_TIMEO;
    }
    public void setLeaseTimeout(int leaseTimeout) {
        this.leaseTimeout = leaseTimeout;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_LEASE_TIMEO |
        MNT_LEASE_TIMEO;
    }

    public int getMaxConcurrentStreams() { return nStreams; }
    public void resetMaxConcurrentStreams() {
        sharedfsChgFlags |= CLR_NSTREAMS;
    }
    public void setMaxConcurrentStreams(int nStreams) {
        this.nStreams = nStreams;
        sharedfsChgFlags = sharedfsChgFlags & ~CLR_NSTREAMS | MNT_NSTREAMS;
    }

    public boolean isMultiHostWrite() { return multiWrite; }
    public void resetMultiHostWrite() {
        sharedfsChgFlags |= CLR_MH_WRITE;
    }
    public void setMultiHostWrite(boolean multiWrite) {
        this.multiWrite = multiWrite;
        sharedfsChgFlags &= ~CLR_MH_WRITE;
        sharedfsChgFlags |= (multiWrite ? MNT_MH_WRITE : MNT_NOMH_WRITE);
    }


    // performance tuning (metadata)

    public short isSynchronizedMetadata() { return syncMeta; }
    public void resetSynchronizedMetadata() {
        chgFlags |= CLR_SYNC_META;
    }
    public void setSynchronizedMetadata(short syncMeta) {
        this.syncMeta = syncMeta;
        chgFlags = chgFlags & ~CLR_SYNC_META | MNT_SYNC_META;
    }

    public int getMetadataStripeWidth() { return metaStripeWidth; }
    public void resetMetadataStripeWidth() {
        qfsChgFlags |= CLR_MM_STRIPE;
    }
    public void setMetadataStripeWidth(int width) {
        this.metaStripeWidth = (short)width;
        qfsChgFlags = qfsChgFlags & ~CLR_MM_STRIPE | MNT_MM_STRIPE;
    }

    // performance tuning (I/O)

    public long getReadAhead() { return rdAheadKb; }
    public void resetReadAhead() {
        ioChgFlags |= CLR_READAHEAD;
    }
    public void setReadAhead(long rdAheadKb) {
        this.rdAheadKb = rdAheadKb;
        ioChgFlags = ioChgFlags & ~CLR_READAHEAD | MNT_READAHEAD;
    }


    public long getWriteBehind() { return wrBehindKb; }
    public void resetWriteBehind() {
        ioChgFlags |= CLR_WRITEBEHIND;
    }
    public void setWriteBehind(long wrBehindKb) {
        this.wrBehindKb = wrBehindKb;
        ioChgFlags = ioChgFlags & ~CLR_WRITEBEHIND | MNT_WRITEBEHIND;
    }


    public long getWriteThrottle() { return wrThrottleKb; }
    public void resetWriteThrottle() {
        ioChgFlags |= CLR_WR_THROTTLE;
    }
    public void setWriteThrottle(long wrThrottleKb) {
        this.wrThrottleKb = wrThrottleKb;
        ioChgFlags = ioChgFlags & ~CLR_WR_THROTTLE | MNT_WR_THROTTLE;
    }

    public boolean isForceNFSAsync() { return forceNFSAsync; }
    public void resetForceNFSAsync() {
        ioChgFlags |= CLR_FORCENFSASYNC;
    }
    public void setForceNFSAsync(boolean forceNFSAsync) {
        this.forceNFSAsync = forceNFSAsync;
        ioChgFlags &= ~CLR_FORCENFSASYNC;
        ioChgFlags |= (forceNFSAsync ? MNT_FORCENFSASYNC :
        MNT_NOFORCENFSASYNC);
    }



    public int getFlushBehind() { return flushBehindKb; }
    public void resetFlushBehind() {
        ioChgFlags |= CLR_FLUSH_BEHIND;
    }
    public void setFlushBehind(int flushBehindKb) {
        this.flushBehindKb = flushBehindKb;
        ioChgFlags = ioChgFlags & ~CLR_FLUSH_BEHIND | MNT_FLUSH_BEHIND;
    }

    public int getStageFlushBehind() { return stageFlushBehindKb; }
    public void resetStageFlushBehind() {
        samChgFlags |= CLR_STAGE_FLUSH_BEHIND;
    }
    public void setStageFlushBehind(int stageFlushBehindKb) {
        this.stageFlushBehindKb = stageFlushBehindKb;
        samChgFlags = samChgFlags & ~CLR_STAGE_FLUSH_BEHIND
            | MNT_STAGE_FLUSH_BEHIND;
    }

    public boolean isSoftRAID() { return softRAID; }
    public void resetSoftRAID() {
        ioChgFlags |= CLR_SW_RAID;
    }
    public void setSoftRAID(boolean softRAID) {
        this.softRAID = softRAID;
        ioChgFlags &= ~CLR_SW_RAID;
        ioChgFlags |= (softRAID ? MNT_SW_RAID : MNT_NOSW_RAID);
    }

    public boolean isForceDirectIO() { return forceDIO; }
    public void resetForceDirectIO() {
        ioChgFlags |= CLR_FORCEDIRECTIO;
    }
    public void setForceDirectIO(boolean forceDIO) {
        this.forceDIO = forceDIO;
        ioChgFlags &= ~CLR_FORCEDIRECTIO;
        ioChgFlags |= (forceDIO ? MNT_FORCEDIRECTIO : MNT_NOFORCEDIRECTIO);
    }


    // direct IO mount options

    public int getConsecutiveReads() { return dioRdConsec; }
    public void resetConsecutiveReads() {
        ioChgFlags |= CLR_DIO_RD_CONSEC;
    }
    public void setConsecutiveReads(int dioRdConsec) {
        this.dioRdConsec = dioRdConsec;
        ioChgFlags = ioChgFlags & ~CLR_DIO_RD_CONSEC | MNT_DIO_RD_CONSEC;
    }

    public int getWellAlignedReadMin() { return dioRdFormMinKb; }
    public void resetWellAlignedReadMin() {
        ioChgFlags |= CLR_DIO_RD_FORM_MIN;
    }
    public void setWellAlignedReadMin(int dioRdFormMinKb) {
        this.dioRdFormMinKb = dioRdFormMinKb;
        ioChgFlags = ioChgFlags & ~CLR_DIO_RD_FORM_MIN | MNT_DIO_RD_FORM_MIN;
    }

    public int getMisAlignedReadMin() { return dioRdIllMinKb; }
    public void resetMisAlignedReadMin() {
        ioChgFlags |= CLR_DIO_RD_ILL_MIN;
    }
    public void setMisAlignedReadMin(int dioRdIllMinKb) {
        this.dioRdIllMinKb = dioRdIllMinKb;
        ioChgFlags = ioChgFlags & ~CLR_DIO_RD_ILL_MIN | MNT_DIO_RD_ILL_MIN;
    }

    public int getConsecutiveWrites() { return dioWrConsec; }
    public void resetConsecutiveWrites() {
        ioChgFlags |= CLR_DIO_WR_CONSEC;
    }
    public void setConsecutiveWrites(int dioWrConsec) {
        this.dioWrConsec = dioWrConsec;
        ioChgFlags = ioChgFlags & ~CLR_DIO_WR_CONSEC | MNT_DIO_WR_CONSEC;
    }

    public int getWellAlignedWriteMin() { return dioWrFormMinKb; }
    public void resetWellAlignedWriteMin() {
        ioChgFlags |= CLR_DIO_WR_FORM_MIN;
    }
    public void setWellAlignedWriteMin(int dioWrFormMinKb) {
        this.dioWrFormMinKb = dioWrFormMinKb;
        ioChgFlags = ioChgFlags & ~CLR_DIO_WR_FORM_MIN | MNT_DIO_WR_FORM_MIN;
    }

    public int getMisAlignedWriteMin() { return dioWrIllMinKb; }
    public void resetMisAlignedWriteMin() {
        ioChgFlags |= CLR_DIO_WR_ILL_MIN;
    }
    public void setMisAlignedWriteMin(int dioWrIllMinKb) {
        this.dioWrIllMinKb = dioWrIllMinKb;
        ioChgFlags = ioChgFlags & ~CLR_DIO_WR_ILL_MIN | MNT_DIO_WR_ILL_MIN;
    }

    public boolean isConsistencyChecking() { return consistencyChecking; }
    public void resetConsistencyChecking() {
        post42ChgFlags |= CLR_CATTR;
    }
    public void setConsistencyChecking(boolean consistencyChecking) {
        this.consistencyChecking = consistencyChecking;
         post42ChgFlags &= ~CLR_CATTR;
         post42ChgFlags |= (consistencyChecking ? MNT_CATTR : MNT_NOCATTR);
    }

    public boolean isDirectIOZeroing() { return directIOZeroing; }
    public void resetDirectIOZeroing() {
        post42ChgFlags |= CLR_DIO_SZERO;
    }
    public void setDirectIOZeroing(boolean directIOZeroing) {
        this.directIOZeroing = directIOZeroing;
         post42ChgFlags &= ~CLR_DIO_SZERO;
         post42ChgFlags |= (directIOZeroing ? MNT_DIO_SZERO : MNT_NODIO_SZERO);
    }


    /* 4.6 Mount options */
    public boolean isWormEmulation() { return worm_emul; }
    public void resetWormEmulation() {
    rel46ChgFlags |= CLR_WORM_EMUL;
    }
    public void setWormEmulation(boolean newValue) {
    this.worm_emul = newValue;
    rel46ChgFlags = rel46ChgFlags & ~CLR_WORM_EMUL | MNT_WORM_EMUL;
    }

    public boolean isWormLite() { return worm_lite; }
    public void resetWormLite() {
    rel46ChgFlags |= CLR_WORM_LITE;
    }
    public void setWormLite(boolean newValue) {
    this.worm_lite = newValue;
    rel46ChgFlags = rel46ChgFlags & ~CLR_WORM_LITE | MNT_WORM_LITE;
    }

    public boolean isWormEmulationLite() { return emul_lite; }
    public void resetWormEmulationLite() {
    rel46ChgFlags |= CLR_WORM_EMUL_LITE;
    }
    public void setWormEmulationLite(boolean newValue) {
    this.emul_lite = newValue;
    rel46ChgFlags = rel46ChgFlags & ~CLR_WORM_EMUL_LITE |
        MNT_WORM_EMUL_LITE;
    }

    public boolean isCdevid() { return cdevid; }
    public void resetCdevid() {
    rel46ChgFlags |= CLR_CDEVID;
    }
    public void setCdevid(boolean newValue) {
    this.cdevid = newValue;
    rel46ChgFlags &= ~CLR_CDEVID;
    rel46ChgFlags |= ((newValue) ? MNT_CDEVID : MNT_NOCDEVID);
    }

    public boolean isClustermgmt() { return clustermgmt; }
    public void resetClustermgmt() {
    rel46ChgFlags |= CLR_CLMGMT;
    }

    public void setClustermgmt(boolean newValue) {
    this.clustermgmt = newValue;
    rel46ChgFlags &= ~CLR_CLMGMT;
    rel46ChgFlags |= ((newValue) ? MNT_CLMGMT : MNT_NOCLMGMT);
    }

    public boolean isClusterfastsw() { return clusterfastsw; }
    public void resetClusterfastsw() {
    rel46ChgFlags |= CLR_CLFASTSW;
    }
    public void setClusterfastsw(boolean newValue) {
    this.clusterfastsw = newValue;
    rel46ChgFlags &= ~CLR_CLFASTSW;
    rel46ChgFlags |= ((newValue) ? MNT_CLFASTSW : MNT_NOCLFASTSW);
    }

    public boolean isNoATime() { return noatime; }
    public void resetNoATime() {
    rel46ChgFlags |= CLR_NOATIME;
    }
    public void setNoATime(boolean newValue) {
    this.noatime = newValue;
    // Note that noatime is not the opposite of atime!
    rel46ChgFlags = rel46ChgFlags & ~CLR_NOATIME | MNT_NOATIME;
    }

    public short getATime() { return atime; }
    public void resetAtime() {
    rel46ChgFlags |= CLR_ATIME;
    }
    public void setATime(short atime) {
    this.atime = atime;
    // Note atime is not the opposite of noatime!
    rel46ChgFlags = rel46ChgFlags & ~CLR_ATIME | MNT_ATIME;
    }

    public int getMinPool() { return min_pool; }
    public void resetMinPool() {
    rel46ChgFlags |= CLR_MIN_POOL;
    }
    public void setMinPool(int minPool) {
    this.min_pool = minPool;
    rel46ChgFlags = rel46ChgFlags & ~CLR_MIN_POOL | MNT_MIN_POOL;
    }


    public int getObjWidth() { return objWidth; }
    public void resetObjWidth() {
	rel50ChgFlags |= CLR_OBJ_WIDTH;
    }
    public void setObjWidth(int objWidth) {
	    this.objWidth = objWidth;
	    rel50ChgFlags = rel50ChgFlags & ~CLR_OBJ_WIDTH | MNT_OBJ_WIDTH;
    }

    public long getObjDepth() { return objDepth; }
    public void resetObjDepth() {
	rel50ChgFlags |= CLR_OBJ_DEPTH;
    }
    public void setObjDepth(long objDepth) {
	this.objDepth = objDepth;
	rel50ChgFlags = rel50ChgFlags & ~CLR_OBJ_DEPTH | MNT_OBJ_DEPTH;
    }

    public int getObjPool() { return objPool; }
    public void resetObjPool() {
	rel50ChgFlags |= CLR_OBJ_POOL;
    }
    public void setObjPool(int objPool) {
	this.objPool = objPool;
	rel50ChgFlags = rel50ChgFlags & ~CLR_OBJ_POOL | MNT_OBJ_POOL;
    }

    public int getObjSyncData() { return objSyncData; }
    public void resetObjSyncData() {
	rel50ChgFlags |= CLR_OBJ_SYNC_DATA;
    }
    public void setObjSyncData(int objSyncData) {
	this.objSyncData = objSyncData;
	rel50ChgFlags = rel50ChgFlags & ~CLR_OBJ_SYNC_DATA | MNT_OBJ_SYNC_DATA;
    }

    public boolean getLogging() { return logging; }
    public void resetLogging() {
	rel50ChgFlags |= CLR_LOGGING;
    }
    public void setLogging(boolean logging) {
	    this.logging = logging;
	    rel50ChgFlags &= ~CLR_LOGGING;
	    rel50ChgFlags |= ((logging) ? MNT_LOGGING : MNT_LOGGING);
    }

    public boolean getSamDB() { return samDB; }
    public void resetSamDB() {
	rel50ChgFlags |= CLR_SAM_DB;
    }
    public void setSamDB(boolean samDB) {
	this.samDB = samDB;
	rel50ChgFlags &= ~CLR_SAM_DB;
	rel50ChgFlags |= ((samDB) ? MNT_SAM_DB : MNT_NOSAM_DB);
    }

    public boolean getXattr() { return xattr; }
    public void resetXattr() {
	rel50ChgFlags |= CLR_XATTR;
    }
    public void setXattr(boolean xattr) {
	this.xattr = xattr;
	rel50ChgFlags |= MNT_XATTR;
	rel50ChgFlags &= ~CLR_XATTR;
	rel50ChgFlags |= ((xattr) ? MNT_XATTR : MNT_NOXATTR);
    }

    /*
     * return an array of key value strings.
     * Each string will be key = value
     * where key is the name of the mount option in the samfs.cmd file
     */
    public String getUnsupportedMountOptions() {
        StringBuffer sb = new StringBuffer();

        if (this.worm_emul) {
            sb.append("worm_emul = on,");
        }

        if (this.worm_lite) {
            sb.append("worm_lite = on,");
        }

        if (this.emul_lite) {
            sb.append("emul_lite = on,");
        }

        if (this.defRetention != 43200) {
            sb.append("default_retention = ");
            sb.append(this.defRetention);
            sb.append(",");
        }

        if (!this.cdevid) {
            sb.append("cdevid = off,");
        }

        if (this.clustermgmt) {
            sb.append("clustermgmt = on,");
        }

        if (this.clusterfastsw) {
            sb.append("clusterfastsw = on,");
        }

        if (!this.directedMirrorReads) {
            sb.append("dmr = off,");
        }

        if (!this.appBasedRecovery) {
            sb.append("abr = off,");
        }

        if (this.atime != 0) {
            sb.append("atime = ");
            sb.append(this.atime);
            sb.append(",");
        }

        if (!this.quota) {
            sb.append("noquota = on,");
        }

        if (this.reader) {
            sb.append("reader = on,");
        }

        if (this.writer) {
            sb.append("writer = on,");
        }

        if (this.refresh_at_eof) {
            sb.append("refresh_at_eof = on,");
        }

        if (this.invalid != 0) {
            sb.append("invalid = ");
            sb.append(this.invalid);
            sb.append(",");
        }

        if (this.wr_ino_buf_size != 512) {
            sb.append("wr_ino_buf_size = ");
            sb.append(this.wr_ino_buf_size);
            sb.append(",");
        }

        if (this.rd_ino_buf_size != 16384) {
            sb.append("rd_ino_buf_size = ");
            sb.append(this.rd_ino_buf_size);
            sb.append(",");
        }

        if (this.min_pool != 64) {
            sb.append("min_pool = ");
            sb.append(this.min_pool);
            sb.append(",");
        }

        if (!this.arscan) {
            sb.append("noarscan = on,");
        }

        String res = null;
        if (sb.length() != 0) {
            /* remove last comma just in case */
            if (sb.charAt(sb.length() - 1) == ',') {
                sb.setCharAt(sb.length() - 1, ' ');
            }
            res = sb.toString();
        }

        return (res);
    }

    public String toString() {
        String s = "ro:" + (readOnly ? "T" : "F") + ",syncMeta:" + syncMeta +
        ",nosuid:" + (noSetUID ? "T" : "F") + ",stripe:" + stripeWidth +
        ",trace:" + (trace ? "T" : "F") +
        " [cf:0x" + Integer.toHexString(chgFlags) + "]\n " +
        // I/O options
        "dioRdC:" + dioRdConsec + ",dioRdF:" + dioRdFormMinKb + "k,dioRdI:" +
        dioRdIllMinKb + "k,dioWrC:" + dioWrConsec + ",dioWrF:" + dioWrFormMinKb
        + "k,dioWrI:" + dioWrIllMinKb + "k,forceDio:" + (forceDIO ? "T" : "F")
        + ",swRaid:" + (softRAID ? "T" : "F") + ",flshB:" + flushBehindKb +
        "k,rdA:" + rdAheadKb + "k,wrB:" + wrBehindKb + "k,wrT:" +
        wrThrottleKb + "k,nfsasy:" + ((forceNFSAsync) ? "T" : "F")
        + "[iocf:0x" + Integer.toHexString(ioChgFlags) + "]\n " +
        // SAM options
        "hwm:" + high + ",lwm:" + low +
        ",parRel:" + partialRelKb + "k,maxParRel:" + maxPartialRelKb + "k" +
        ",parStg:" + partialStageKb + "k,stgWin:" + stageWinKb + "k,stgRetr:" +
        stageRetries + ",stgFlshB:" + stageFlushBehindKb + ",arcArun:" +
        (arcAutorun ? "T" : "F") +
        " [samcf:0x" + Integer.toHexString(samChgFlags) + "]\n " +
        // shared FS options
        "bkgr:" + (backgr ? "T" : "F") + ",retry:" + retry +
        ",minBlks:" + minBlocks + ",maxBlks:" + maxBlocks +
        ",rdLs:" + rdLease + ",wrLs:" + wrLease + ",mhWr:" +
        (multiWrite ? "T" : "F") +
        ",nStrm:" + nStreams + ",mTimeo:" + metaTimeout +
        ",ltimeo:" + leaseTimeout +
        " [shcf:0x" + Integer.toHexString(sharedfsChgFlags) + "]\n " +
        // multi-reader options
        "wrr:" + (writer ? "T" : "F") + ",rdr:" + (reader ? "T" : "F") +
        ",inv:" + Integer.toHexString(invalid) +
        " [mrcf:0x" + Integer.toHexString(multirdChgFlags) + "]" +
        // Post 4.2 options
        ",defRetention:" + defRetention +
        ",abr:" + (appBasedRecovery ? "T" : "F") +
        ",dmr:" + (directedMirrorReads ? "T" : "F") +
            ",dio_szero:" + (directIOZeroing ? "T" : "F") +
        ",cattr:" + (consistencyChecking ? "T" : "F") +
            "\n[46cf:0x" + Integer.toHexString(rel46ChgFlags) + "]\n" +
        ",worm_emul:" + (worm_emul ? "T" : "F") +
        ",worm_lite:" + (worm_lite ? "T" : "F") +
        ",emul_lite:" + (emul_lite ? "T" : "F") +
        ",cdevid:" + (cdevid ? "T" : "F") +
        ",clustermgmt:" + (clustermgmt ? "T" : "F") +
        ",clusterfastsw:" + (clusterfastsw ? "T" : "F") +
        ",noatime:" + (noatime ? "T" : "F") +
        ",atime:" + atime +
        ",min_pool:" + min_pool;

        return s;
    }
}
