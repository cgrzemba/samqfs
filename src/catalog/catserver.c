/*
 * catserver.c - catalog management server.
 */

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

#pragma ident "$Revision: 1.112 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#define	CATALOG_TABLE "CatalogTable"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "aml/archiver.h"
#include "sam/custmsg.h"
#include "sam/defaults.h"
#include "aml/device.h"
#include "aml/remote.h"
#include "aml/message.h"
#include "sam/exit.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "aml/shm.h"
#include "sam/sam_trace.h"
#include "sam/udscom.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#define	MAIN
#include "sam/nl_samfs.h"

/* Local headers. */
#define	CAT_SERVER
#include "catlib_priv.h"
#include "catserver.h"

#if defined(lint)
#include "sam/lint.h"
#undef snprintf
#endif /* defined(lint) */

/* Private functions. */
static void CatchSignals(int SigNum);
static void CheckForCleaning(struct CatalogEntry *ce);
static int CompareVsns(const void *p1, const void *p2);
static void FreeEntry(struct CatalogEntry *ce);
static struct CatalogEntry *GetFreeEntry(int cat_num);
static int GetFreeSlot(int cat_num);
static int GrowCatalog(int cat_num, int increase);
static void LibLogit(int status, char *msg);
static void MakeCatalogTable(void);
static void MakeEmptyCatalog(char *cf_name, enum CH_type ch_type, void **mp,
	size_t *size);
static void MoveCartridge(struct CatalogEntry *ce, int cat_num, int slot);
static struct CatalogEntry *NextCartridgeEntry(struct CatalogEntry *cea,
	int en);
static int SortAndCheckForDuplicateVsns(char *mtype, char *vsn,
	struct CatalogEntry *cea, boolean_t checkall, boolean_t markit);
static void SrvrLogit(char *msg);
static char *VolStringFromCe(struct CatalogEntry *ce);
static char *StringFromVolId(struct VolId *vid);
static void TraceRequest(struct UdsMsgHeader *hdr, struct CsrGeneralRsp *rsp,
	const char *fmt, ...);
static void *ShmatSamfs(int	mode);
static void UpdateRemoteCe(struct CatalogEntry *ce, int flags);
static int VerifyVolume(struct CatalogEntry *ce, struct CatalogEntry *vol,
	int replace);
static void VsnFromBarCode(struct CatalogEntry *ce, sam_defaults_t *defaults);

/* Server functions. */
static void *AssignFreeSlot(void *arg, struct UdsMsgHeader *hdr);
static void *Export(void *arg, struct UdsMsgHeader *hdr);
static void *FormatPartitions(void *arg, struct UdsMsgHeader *hdr);
static void *GetInfo(void *arg, struct UdsMsgHeader *hdr);
static void *LabelComplete(void *arg, struct UdsMsgHeader *hdr);
static void *LabelFailed(void *arg, struct UdsMsgHeader *hdr);
static void *LabelVolume(void *arg, struct UdsMsgHeader *hdr);
static void *LibraryExport(void *arg, struct UdsMsgHeader *hdr);
static void *MediaClosed(void *arg, struct UdsMsgHeader *hdr);
static void *MoveSlot(void *arg, struct UdsMsgHeader *hdr);
static void *ReconcileCatalog(void *arg, struct UdsMsgHeader *hdr);
static void *RemoteSamUpdate(void *arg, struct UdsMsgHeader *hdr);
static void *ReserveVolume(void *arg, struct UdsMsgHeader *hdr);
static void *SetAudit(void *arg, struct UdsMsgHeader *hdr);
static void *SetCleaning(void *arg, struct UdsMsgHeader *hdr);
static void *SetRemoteServer(void *arg, struct UdsMsgHeader *hdr);
static void *SlotInit(void *arg, struct UdsMsgHeader *hdr);
static void *SetField(void *arg, struct UdsMsgHeader *hdr);
static void *UnReserveVolume(void *arg, struct UdsMsgHeader *hdr);
static void *VolumeLoaded(void *arg, struct UdsMsgHeader *hdr);
static void *VolumeUnloaded(void *arg, struct UdsMsgHeader *hdr);

/* Private data. */
static upath_t CatalogTableName;

static struct UdsMsgProcess Table[] = {
	{ AssignFreeSlot, sizeof (struct CsrAssignFreeSlot),
		sizeof (struct CsrGeneralRsp) },
	{ Export, sizeof (struct CsrExport), sizeof (struct CsrGeneralRsp) },
	{ FormatPartitions, sizeof (struct CsrFormatPartitions),
		sizeof (struct CsrGeneralRsp) },
	{ GetInfo, 0, sizeof (struct CsrGetInfoRsp) },
	{ LabelComplete, sizeof (struct CsrLabelComplete) },
	{ LabelFailed, sizeof (struct CsrLabelFailed),
		sizeof (struct CsrGeneralRsp) },
	{ LabelVolume, sizeof (struct CsrLabelVolume),
		sizeof (struct CsrGeneralRsp) },
	{ LibraryExport, sizeof (struct CsrLibraryExport),
		sizeof (struct CsrGeneralRsp) },
	{ MediaClosed, sizeof (struct CsrMediaClosed),
		sizeof (struct CsrGeneralRsp) },
	{ MoveSlot, sizeof (struct CsrMoveSlot),
		sizeof (struct CsrGeneralRsp) },
	{ ReconcileCatalog, sizeof (struct CsrReconcileCatalog),
		sizeof (struct CsrGeneralRsp) },
	{ RemoteSamUpdate, sizeof (struct CsrRemoteSamUpdate),
		sizeof (struct CsrGeneralRsp) },
	{ ReserveVolume, sizeof (struct CsrReserveVolume),
		sizeof (struct CsrGeneralRsp) },
	{ SetAudit, sizeof (struct CsrSetAudit),
		sizeof (struct CsrGeneralRsp) },
	{ SetCleaning, sizeof (struct CsrSetCleaning),
		sizeof (struct CsrGeneralRsp) },
	{ SetRemoteServer, 0, sizeof (struct CsrGeneralRsp) },
	{ SlotInit, sizeof (struct CsrSlotInit),
		sizeof (struct CsrGeneralRsp) },
	{ SetField, sizeof (struct CsrSetField),
		sizeof (struct CsrGeneralRsp) },
	{ UnReserveVolume, sizeof (struct CsrUnReserveVolume),
		sizeof (struct CsrGeneralRsp) },
	{ VolumeLoaded, sizeof (struct CsrVolumeLoaded),
		sizeof (struct CsrGeneralRsp) },
	{ VolumeUnloaded, sizeof (struct CsrVolumeUnloaded),
		sizeof (struct CsrGeneralRsp) },
};

/* Define the argument buffer to determine size required. */
union argbuf {
	struct CsrFormatPartitions CsrFormatPartitions;
	struct CsrLabelComplete CsrLabelComplete;
	struct CsrLabelFailed CsrLabelFailed;
	struct CsrLabelVolume CsrLabelVolume;
	struct CsrLibraryExport CsrLibraryExport;
	struct CsrMediaClosed CsrMediaClosed;
	struct CsrMediaOp CsrMediaOp;
	struct CsrMoveSlot CsrMoveSlot;
	struct CsrReconcileCatalog CsrReconcileCatalog;
	struct CsrRemoteSamUpdate CsrRemoteSamUpdate;
	struct CsrReserveVolume CsrReserveVolume;
	struct CsrSetAudit CsrSetAudit;
	struct CsrSetCleaning CsrSetCleaning;
	struct CsrSlotInit CsrSlotInit;
	struct CsrSetField CsrSetField;
	struct CsrUnReserveVolume CsrUnReserveVolume;
	struct CsrVolumeUnloaded CsrVolumeUnloaded;
};

static struct UdsServer srvr =
	{ SERVER_NAME, SERVER_MAGIC, SrvrLogit, 0, Table, USR_MAX,
	sizeof (union argbuf) };

static sam_defaults_t *defaults;
static char *CatalogDir = SAM_CATALOG_DIR;
static char *McfFileName = SAM_CONFIG_PATH"/"CONFIG;

static int msgs_processed = 0;
static int errors = 0;

static shm_alloc_t master_shm;
static shm_ptr_tbl_t *shm_ptr_tbl;

static boolean_t RemoteServer = FALSE;
static boolean_t IsDaemon = FALSE;
static int Historian = 0;
static int LastMid = 0;

/* Public functions. */
int CvrtCatalog(char *cf_name, int version, void **mp, size_t *size);

/* Public data. */
struct CatalogTableHdr *CatalogTable;
struct CatalogMap *Catalogs;
char *program_name = "catserver";


int
main(
	int argc,
	char *argv[])
{
	struct sigaction sig_action;
	sigset_t block_set;

	/*
	 * Check initiator.
	 * sam_amld will start us with all file descriptors closed.
	 */
	CustmsgInit(0, NULL);	/* Assure message translation */
	if (ShmatSamfs(O_RDONLY) == NULL ||
	    shm_ptr_tbl->sam_amld != getppid()) {
		/*
		 * Not started from sam_amld.
		 */
		if (argc > 1) {
			McfFileName = argv[1];
			printf("Using mcf file %s\n", McfFileName);
		}
		if (argc > 2) {
			CatalogDir = argv[2];
			printf("Using catalog directory %s\n", CatalogDir);
		}
		CustmsgInit(0, NULL);
	} else {
		/*
		 * Started by sam-amld.
		 * Logging goes to syslog.
		 */
		CustmsgInit(1, NULL);
		(void) chdir(CatalogDir);
		TraceInit(program_name, TI_catserver);
		Trace(TR_MISC, "Catalog server started");
		IsDaemon = TRUE;
	}

	shmdt(master_shm.shared_memory);
	defaults = GetDefaults();
	MakeCatalogTable();

	/*
	 * Catch signals.
	 */
	sig_action.sa_handler = CatchSignals;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGHUP, &sig_action, NULL);
	(void) sigaction(SIGINT, &sig_action, NULL);
	/*
	 * Reset our signal mask to not block anything.
	 */
	sigemptyset(&block_set);
	sigprocmask(SIG_SETMASK, &block_set, NULL);

	/*
	 * Receive messages.
	 */
	if (UdsRecvMsg(&srvr) == -1) {
		if (errno != EINTR) {
			LibFatal(UdsRecvMsg, NULL);
			exit(EXIT_FAILURE);
		}
	}
	/*
	 * Invalidate catalogs.
	 */
	CatalogTable->Ct.MfValid = 0;

	Trace(TR_MISC, "Messages processed %d, errors %d",
	    msgs_processed, errors);
	SendCustMsg(HERE, 18005, srvr.UsStop);
	return (EXIT_SUCCESS);
}


/*
 * Assign free slot.
 */
static void *
AssignFreeSlot(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrAssignFreeSlot *a = (struct CsrAssignFreeSlot *)arg;
	struct CatalogEntry *ce;
	int cat_num;
	int slot;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->AsEq)) == -1)  goto out;
	slot = GetFreeSlot(cat_num);
	ce = GetFreeEntry(cat_num);
	if (ce == NULL)  goto out;
	ce->CeSlot = slot;
	rsp.GrStatus = slot;	/* Return slot to caller */

out:
	TraceRequest(hdr, &rsp,
	    "AssignFreeSlot(%d) = %d", a->AsEq, rsp.GrStatus);
	return (&rsp);
}


/*
 * Export catalog entry.
 */
static void *
Export(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	vsn_t	VsnHist;
	mtype_t	MtypeHist;
	static struct CsrGeneralRsp rsp;
	struct CsrExport *a = (struct CsrExport *)arg;
	struct CatalogEntry *ce;

	rsp.GrStatus = -1;
	if (a->SeVid.ViFlags == VI_cart) {
		ce = CS_CatalogGetCeByLoc(a->SeVid.ViEq, a->SeVid.ViSlot,
		    a->SeVid.ViPart);
	} else {
		ce = CS_CatalogGetCeByMedia(a->SeVid.ViMtype, a->SeVid.ViVsn);
	}
	if (ce == NULL) {
		goto out;
	}

	/*
	 * This entry was found in a catalog other than the
	 * Historian. Move the entry to the Historian.
	 */
	if (ce->CeEq != Catalogs[Historian].CmHdr->ChEq) {
		memmove(VsnHist, ce->CeVsn, sizeof (vsn_t));
		memmove(MtypeHist, ce->CeMtype, sizeof (MtypeHist));
		if (RemoteServer) {
			UpdateRemoteCe(ce, RMT_CAT_CHG_FLGS_EXP);
		}
		MoveCartridge(ce, Historian, -1);
		if (defaults->flags & DF_EXPORT_UNAVAIL) {
			/*
			 * Find the entry in the historian
			 */
			ce = CS_CatalogGetCeByMedia(MtypeHist, VsnHist);
			if (ce != NULL) {
				ce->CeStatus |= CES_unavail;
				if (ce->CePart != 0) {
				int en;
				struct CatalogEntry *cea;

				en = 0;
				while ((cea = NextCartridgeEntry(ce,
				    en++)) != NULL) {
					cea->CeStatus |= CES_unavail;
					if (RemoteServer) {
						UpdateRemoteCe(cea,
						    RMT_CAT_CHG_FLGS_EXP);
					}
				}
				}
			}
		}
	} else {
		/*
		 * This entry was found in the Historian
		 * Remove the entry forever.
		 */
		SendCustMsg(HERE, 18022, VolStringFromCe(ce));

		/*
		 * If multi-partition media, find another entry.
		 */
		if (ce->CePart != 0) {
			int en;
			struct CatalogEntry *cea;

			en = 0;
			while ((cea = NextCartridgeEntry(ce,
			    en++)) != NULL) {
				SendCustMsg(HERE, 18022, VolStringFromCe(cea));
				FreeEntry(cea);
			}
		}
		FreeEntry(ce);
	}
	(void) ArchiverCatalogChange();

	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "Export(%s)", StringFromVolId(&a->SeVid));
	return (&rsp);
}


/*
 * Format request causes a reset of current partitions.
 */
static void *
FormatPartitions(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	int cat_num, np;
	vsn_t	vsn;
	static struct CsrGeneralRsp rsp;
	struct CsrFormatPartitions *a = (struct CsrFormatPartitions *)arg;
	struct CatalogEntry *ce;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->FrVid.ViEq)) == -1) {
		goto out;
	}
	ce = CS_CatalogGetCeByLoc(a->FrVid.ViEq, a->FrVid.ViSlot, 0);

	if (ce != NULL) {
		struct CatalogEntry *cea;
		int en;

		/*
		 * Free any existing partitions.
		 */
		en = 0;
		while ((cea = NextCartridgeEntry(ce, en++)) != NULL) {
			SendCustMsg(HERE, 18022, StringFromVolId(&a->FrVid));
			FreeEntry(cea);
		}
		FreeEntry(ce);
	}

	/*
	 * Now set up the new entries.
	 */
	if (a->FrVid.ViSlot == (unsigned)ROBOT_NO_SLOT) {
		a->FrVid.ViSlot = GetFreeSlot(cat_num);
	}

	for (np = 1; np <= a->FrNumParts; np++) {
		ce = GetFreeEntry(cat_num);
		memmove(ce->CeMtype, a->FrVid.ViMtype, sizeof (ce->CeMtype));
		sprintf(vsn, "%s:%d", a->FrVid.ViVsn, (np - 1));
		memmove(ce->CeVsn, vsn, sizeof (ce->CeVsn));
		ce->CeStatus = a->FrStatus;
		ce->CeStatus |= CES_partitioned;
		ce->CeSlot = a->FrVid.ViSlot;
		ce->CePart = (short)np;
	}
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "FormatPartitions(%s, %d)",
	    StringFromVolId(&a->FrVid), a->FrNumParts);
	return (&rsp);
}


/*
 * Return initialization data to client.
 */
/*ARGSUSED0*/
static void *
GetInfo(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGetInfoRsp rsp;

	strcpy(rsp.CatTableName, CatalogTableName);
	TraceRequest(hdr, NULL, "Client connection made.");
	return (&rsp);
}


/*
 * Complete a label operation.
 */
static void *
LabelComplete(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrVolumeLoaded *a = (struct CsrVolumeLoaded *)arg;
	struct CatalogEntry *ce, *cea, *cet;
	int en;

	rsp.GrStatus = -1;
	cea = &a->VlCe;
	ce = CS_CatalogGetCeByLoc(cea->CeEq, cea->CeSlot, cea->CePart);
	if (ce == NULL) {
		goto out;
	}

	/*
	 * Remove the dummy entry for a new VSN.
	 */
	cet = CS_CatalogGetCeByMedia(cea->CeMtype, cea->CeVsn);
	if (cet != NULL && cet != ce) {
		FreeEntry(cet);
	}

	/*
	 * Issue appropriate message to syslog.
	 */
	if (ce->CeStatus & CES_labeled) {
		/* Relabeled */
		if (strcmp(ce->CeVsn, cea->CeVsn) == 0) {
			/* Volume %s relabeled. */
			SendCustMsg(HERE, 18031, VolStringFromCe(ce));
		} else {
			/* Volume %s relabeled with %s. */
			SendCustMsg(HERE, 18032, VolStringFromCe(ce),
			    cea->CeVsn);
		}
	} else {
		/* Volume %s %s.%s labeled. */
		SendCustMsg(HERE, 18033, VolStringFromCe(ce), cea->CeMtype,
		    cea->CeVsn);
	}

	/*
	 * Unreserve a reserved volume if relabeled.
	 */
	if (ce->CeStatus & CES_labeled && ce->CeLabelTime != 0 &&
	    ce->r.CerTime != 0) {
		ce->r.CerTime = 0;
		/* Volume %s unreserved from %s/%s/%s */
		SendCustMsg(HERE, 18035, VolStringFromCe(ce),
		    ce->r.CerAsname, ce->r.CerOwner, ce->r.CerFsname);
		memset(ce->r.CerAsname, 0, sizeof (ce->r.CerAsname));
		memset(ce->r.CerOwner, 0, sizeof (ce->r.CerOwner));
		memset(ce->r.CerFsname, 0, sizeof (ce->r.CerFsname));
	}
	/*
	 * Replace all the information reported.
	 */
	memmove(ce->CeVsn, cea->CeVsn, sizeof (ce->CeVsn));
	memmove(ce->CeMtype, cea->CeMtype, sizeof (ce->CeMtype));
	ce->CeCapacity = cea->CeCapacity;
	ce->CeSpace = cea->CeSpace;
	ce->CeBlockSize = cea->CeBlockSize;
	ce->CeLabelTime = cea->CeLabelTime;
	ce->CeModTime = time(NULL);
	ce->m.CePtocFwa = cea->m.CePtocFwa;

	/*
	 * Merge status bits.
	 */
	ce->CeStatus &= ~(CES_needs_audit | CES_bad_media | CES_recycle |
	    CES_dupvsn | CES_archfull);
	ce->CeStatus |= CES_labeled |
	    (cea->CeStatus & (CES_bad_media | CES_recycle));

	rsp.GrStatus = 0;

out:
	/*
	 * Update status for cartridge.
	 */
	en = 0;
	while (ce != NULL) {
		ce->CeStatus |= CES_inuse;
		ce->CeStatus &= ~CES_occupied;
		if (RemoteServer) {
			UpdateRemoteCe(ce, 0);
		}
		if (ce->CePart == 0) {
			break;
		}
		ce = NextCartridgeEntry(ce, en++);
	}

	TraceRequest(hdr, &rsp, "LabelComplete(%s)", VolStringFromCe(cea));
	return (&rsp);
}


/*
 * Label operation failed. Remove the dummy entry.
 */
static void *
LabelFailed(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrLabelFailed *a = (struct CsrLabelFailed *)arg;
	struct CatalogEntry *cet, *ce;

	/*
	 * Remove the dummy entry if created at the
	 * beginning of the label process.
	 */
	rsp.GrStatus = 0;
	cet = CS_CatalogGetCeByMedia(a->LfVid.ViMtype, a->LfNewVsn);
	ce = CS_CatalogGetCeByMedia(a->LfVid.ViMtype, a->LfVid.ViVsn);
	/*
	 * If ce == NULL, try to find the old entry by eq:slot. It's
	 * possible there is no label or the label is unreadable.
	 */
	if (ce == NULL) {
		ce = CS_CatalogGetCeByLoc(a->LfVid.ViEq, a->LfVid.ViSlot,
		    a->LfVid.ViPart);
	}
	if ((ce != cet) && (ce != NULL) && (cet != NULL)) {
		FreeEntry(cet);
	}

	TraceRequest(hdr, &rsp, "LabelFailed(%s, %s)",
	    StringFromVolId(&a->LfVid), a->LfNewVsn);

	return (&rsp);
}


/*
 * Begin a label operation.
 */
static void *
LabelVolume(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrLabelVolume *a = (struct CsrLabelVolume *)arg;
	struct CatalogEntry *ce, *cet;

	/*
	 * Get the catalog entry.
	 * See if an entry exists for the New VSN.
	 */
	rsp.GrStatus = -1;
	a->LvVid.ViFlags = VI_labeled;
	ce = CS_CatalogGetCeByMedia(a->LvVid.ViMtype, a->LvVid.ViVsn);
	cet = CS_CatalogGetCeByMedia(a->LvVid.ViMtype, a->LvNewVsn);
	if (ce != cet) {
		if (cet != NULL && ce != NULL) {
			/*
			 * The new VSN exists.
			 */
			errno = ER_DUPLICATE_VSN;
			goto out;

		} else if (ce != NULL) {
			int		cn;

			/*
			 * Volume is not being relabeled with the same VSN.
			 * Make a temporary catalog entry.
			 * This will assure that the new VSN cannot
			 * be duplicated.
			 */
			cn = FindCatalog(ce->CeEq);
			if (cn < 0)  goto out;
			cet = GetFreeEntry(cn);
			if (cet == NULL)   goto out;
			cet->CeStatus |= CES_labeled;
			memmove(cet->CeMtype, a->LvVid.ViMtype,
			    sizeof (cet->CeMtype));
			memmove(cet->CeVsn, a->LvNewVsn, sizeof (cet->CeVsn));
		}
	}

	if ((RemoteServer) && (ce != NULL)) {
		UpdateRemoteCe(ce, RMT_CAT_CHG_FLGS_LBL);
	}

	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "LabelVolume(%s %s)",
	    StringFromVolId(&a->LvVid), a->LvNewVsn);
	return (&rsp);
}


/*
 * Move all entries from a catalog to the historian
 */
static void *
LibraryExport(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrLibraryExport *a = (struct CsrLibraryExport *)arg;
	struct CatalogHdr *ch;
	int cat_num, ne;

	rsp.GrStatus = -1;
	if (a->LeEq != Catalogs[Historian].CmHdr->ChEq) {
		if ((cat_num = FindCatalog(a->LeEq)) == -1)  goto out;
		ch = Catalogs[cat_num].CmHdr;
		for (ne = 0; ne < ch->ChNumofEntries; ne++) {
			struct CatalogEntry *ce;

			ce = &ch->ChTable[ne];
			if (ce->CeStatus & CES_inuse) {
				MoveCartridge(ce, Historian, -1);
				if (RemoteServer) {
					UpdateRemoteCe(ce,
					    RMT_CAT_CHG_FLGS_EXP);
				}
			}
		}
		rsp.GrStatus = 0;
		(void) ArchiverCatalogChange();
	} else {
		errno = EINVAL;
	}

out:
	TraceRequest(hdr, &rsp, "LibraryExport(%d)", a->LeEq);
	return (&rsp);
}


/*
 * Process media closed event.
 * Update the space for all media.
 * For optical, update the ptocfwa; for tape, update the position.
 */
static void *
MediaClosed(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrMediaClosed *a = (struct CsrMediaClosed *)arg;
	struct CatalogEntry *ce, *cea;

	rsp.GrStatus = -1;
	cea = &a->SfCe;
	ce = CS_CatalogGetCeByLoc(cea->CeEq, cea->CeSlot, cea->CePart);
	if (ce == NULL)  goto out;
	ce->CeSpace = cea->CeSpace;
	ce->CeModTime = time(NULL);
	ce->m.CeLastPos = cea->m.CeLastPos;
	if ((ce->CeCapacity != cea->CeCapacity) &&
	    (!(ce->CeStatus & CES_capacity_set))) {
		ce->CeCapacity = cea->CeCapacity;
	}
	if (RemoteServer) {
		UpdateRemoteCe(ce, 0);
	}
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "VolumeClosed(%s, %llu, %llu)",
	    VolStringFromCe(cea), cea->CeSpace, cea->m.CeLastPos);
	return (&rsp);
}


/*
 * Process move slot.
 */
static void *
MoveSlot(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrMoveSlot *a = (struct CsrMoveSlot *)arg;
	struct CatalogEntry *ce, *ced;
	int en;
	int src;

	rsp.GrStatus = -1;
	if ((ce = CS_CatalogGetEntry(&a->MsVid)) == NULL)  goto out;

	/*
	 * Check destination.
	 */
	src = a->MsVid.ViSlot;
	a->MsVid.ViSlot = a->MsDestSlot;
	ced = CS_CatalogGetEntry(&a->MsVid);
	a->MsVid.ViSlot = src;
	if (ced != NULL) {
		errno = ER_DST_SLOT_IS_OCCUPIED;
		goto out;
	}
	en = 0;
	while (ce != NULL) {
		struct CatalogEntry *ce_tochange;

		ce_tochange = ce;
		if (ce->CePart != 0)  ce = NextCartridgeEntry(ce, en++);
		else  ce = NULL;
		ce_tochange->CeSlot = a->MsDestSlot;
	}
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "MoveSlot(%s, %d)", StringFromVolId(&a->MsVid),
	    a->MsDestSlot);
	return (&rsp);
}


/*
 * Free the catalog entry for any unoccupied slots.
 * Check for any duplicate entries.
 */
static void *
ReconcileCatalog(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrReconcileCatalog *a = (struct CsrReconcileCatalog *)arg;
	struct CatalogHdr *ch;
	int cat_num, ne;
	int num_dups;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->RcEq)) == -1)  goto out;
	ch = Catalogs[cat_num].CmHdr;
	for (ne = 0; ne < ch->ChNumofEntries; ne++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[ne];
		if (ce->CeStatus & CES_reconcile) {
			FreeEntry(ce);
		}
	}
	num_dups = SortAndCheckForDuplicateVsns(NULL, NULL, NULL, TRUE, TRUE);
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "ReconcileCatalog(%d) %d", a->RcEq, num_dups);
	return (&rsp);
}


/*
 * Update a Remote SAM catalog entry.
 */
static void *
RemoteSamUpdate(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrRemoteSamUpdate *a = (struct CsrRemoteSamUpdate *)arg;
	struct CatalogEntry *ce, *cea;
	int en, cat_num;

	rsp.GrStatus = -1;
	cea = &a->RsCe;
	if ((cat_num = FindCatalog(cea->CeEq)) == -1) {
		goto out;
	}

	ce = CS_CatalogGetCeByMedia(cea->CeMtype, cea->CeVsn);

	/*
	 * If relabel, remove the catalog entry.  Another request from the
	 * server will create a new catalog entry with the new vsn name.
	 * (Careful, currently RsFlags is set only if a relabel.)
	 */
	if (ce != NULL && a->RsFlags) {
		FreeEntry(ce);
		ce = NULL;
		goto out;
	}

	if (ce == NULL) {
		int slot;

		slot = GetFreeSlot(cat_num);
		ce = GetFreeEntry(cat_num);
		if (ce == NULL) {
			errno = EINVAL;
			goto out;
		}
		ce->CeSlot = slot;
	}

	if (ce->CeEq == Catalogs[Historian].CmHdr->ChEq) {
		/*
		 * Just move the historian entry to the library catalog.
		 */
		MoveCartridge(ce, cat_num, -1);
		rsp.GrStatus = 0;
	} else {
		memmove(ce->CeMtype, cea->CeMtype, sizeof (ce->CeMtype));
		memmove(ce->CeVsn, cea->CeVsn, sizeof (ce->CeVsn));
		ce->CeAccess = cea->CeAccess;
		ce->CeCapacity = cea->CeCapacity;
		ce->CeSpace	= cea->CeSpace;
		ce->CeBlockSize = cea->CeBlockSize;
		ce->CeLabelTime = cea->CeLabelTime;
		ce->CeModTime = cea->CeModTime;
		ce->CeMountTime = cea->CeMountTime;
		memmove(ce->CeBarCode, cea->CeBarCode, sizeof (ce->CeBarCode));
		ce->m.CePtocFwa = cea->m.CePtocFwa;
		ce->CePart = cea->CePart;
		(void) ArchiverCatalogChange();
		rsp.GrStatus = 0;
	}

out:
	/*
	 * Update status for cartridge.
	 */
	en = 0;
	while (ce != NULL) {
		ce->CeStatus = cea->CeStatus;
		if (ce->CePart == 0)  break;
		ce = NextCartridgeEntry(ce, en++);
	}

	TraceRequest(hdr, &rsp, "RemoteSamUpdate(%s)", VolStringFromCe(cea));
	return (&rsp);
}


/*
 * Reserve a volume for archiving.
 */
static void *
ReserveVolume(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrReserveVolume *a = (struct CsrReserveVolume *)arg;
	struct CatalogEntry *ce;

	rsp.GrStatus = -1;
	if ((ce = CS_CatalogGetEntry(&a->RvVid)) == NULL) {
		goto out;
	}
	if (ce->r.CerTime != 0) {
		/*
		 * Already reserved.
		 * Check reservation.
		 */
		Trace(TR_MISC, "Volume %s is already reserved to %s/%s/%s",
		    StringFromVolId(&a->RvVid), ce->r.CerAsname,
		    ce->r.CerOwner, ce->r.CerFsname);
		if (strcmp(ce->r.CerAsname, a->RvAsname) != 0 ||
		    strcmp(ce->r.CerOwner, a->RvOwner) != 0 ||
		    strcmp(ce->r.CerFsname, a->RvFsname) != 0) {
			errno = ER_VOLUME_ALREADY_RESERVED;
		} else {
			/*
			 * No error if same reservation.
			 */
			rsp.GrStatus = 0;
		}
		goto out;
	}

	rsp.GrStatus = 0;
	ce->r.CerTime = a->RvTime;
	memmove(ce->r.CerAsname, a->RvAsname, sizeof (ce->r.CerAsname));
	memmove(ce->r.CerOwner, a->RvOwner, sizeof (ce->r.CerOwner));
	memmove(ce->r.CerFsname, a->RvFsname, sizeof (ce->r.CerFsname));
	/* Volume %s reserved to %s/%s/%s */
	SendCustMsg(HERE, 18034, VolStringFromCe(ce), ce->r.CerAsname,
	    ce->r.CerOwner, ce->r.CerFsname);

out:
	TraceRequest(hdr, &rsp, "ReserveVolume(%s, %s/%s/%s)",
	    StringFromVolId(&a->RvVid), a->RvAsname,
	    a->RvOwner, a->RvFsname);
	return (&rsp);
}

/*
 * Set the audit flag for all entries.
 */
static void *
SetAudit(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrSetAudit *a = (struct CsrSetAudit *)arg;
	struct CatalogHdr *ch;
	int cat_num, ne;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->SaEq)) == -1)  goto out;
	ch = Catalogs[cat_num].CmHdr;

	for (ne = 0; ne < ch->ChNumofEntries; ne++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[ne];
		if ((ce->CeStatus &CES_inuse) &&
		    (!(ce->CeStatus & CES_cleaning)) &&
		    (!(ce->CeStatus & CES_non_sam))) {
				ce->CeStatus |= CES_needs_audit;
		}
	}
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "SetAudit(%d)", a->SaEq);
	return (&rsp);
}


/*
 * Set the cleaning flag for any cleaning tapes.
 */
static void *
SetCleaning(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrSetCleaning *a = (struct CsrSetCleaning *)arg;
	struct CatalogHdr *ch;
	int cat_num, ne;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->ScEq)) == -1)  goto out;
	ch = Catalogs[cat_num].CmHdr;

	for (ne = 0; ne < ch->ChNumofEntries; ne++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[ne];
		if (ce->CeStatus & CES_inuse) {
			CheckForCleaning(ce);
		}
	}
	rsp.GrStatus = 0;

out:
	TraceRequest(hdr, &rsp, "SetCleaning(%d)", a->ScEq);
	return (&rsp);
}


/*
 * It is now known that this is a SAM-Remote server
 * and that initialization is complete.
 */
static void *
SetRemoteServer(
	/* LINTED argument unused in function */
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;

	RemoteServer = TRUE;

	TraceRequest(hdr, NULL, "RemoteServer set.");
	return (&rsp);
}


/*
 * Initialize a slot.
 */
static void *
SlotInit(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrSlotInit *a = (struct CsrSlotInit *)arg;
	struct CatalogEntry *ce, *cea;
	int cat_num;

	rsp.GrStatus = -1;
	if ((cat_num = FindCatalog(a->SiVid.ViEq)) == -1) {
		goto out;
	}
	if (*a->SiBarcode != '\0') {
		/*
		 * Check if entry is found in historian
		 */
		ce = CS_CatalogGetCeByBarCode(Catalogs[Historian].CmHdr->ChEq,
		    a->SiVid.ViMtype, a->SiBarcode);
		if (ce != NULL) {
			MoveCartridge(ce, cat_num, a->SiVid.ViSlot);
			ce = CS_CatalogGetCeByBarCode(a->SiVid.ViEq,
			    a->SiVid.ViMtype, a->SiBarcode);
			if ((ce != NULL) && (ce->CeStatus & CES_unavail)) {
				ce->CeStatus &= ~CES_unavail;
				if ((RemoteServer) &&
				    (!(ce->CeStatus & CES_cleaning))) {
					UpdateRemoteCe(ce, 0);
				}
				/*
				 * N.B. Bad indentation to meet cstyle
				 * requirements.
				 */
				if (ce->CePart != 0) {
				int en;
				struct CatalogEntry *cea;

				en = 0;
				while ((cea = NextCartridgeEntry(ce, en++))
				    != NULL) {
					cea->CeStatus &= ~CES_unavail;
					if ((RemoteServer) &&
					    (!(cea->CeStatus & CES_cleaning))) {
						UpdateRemoteCe(cea, 0);
					}
				}
				}
			}
			ce = NULL;
			rsp.GrStatus = 0;
			goto out;
		}

		/*
		 * Verify barcode is correct
		 */
		if (a->SiVid.ViSlot != (unsigned)ROBOT_NO_SLOT) {
			ce = CS_CatalogGetCeByBarCode(a->SiVid.ViEq,
			    a->SiVid.ViMtype, a->SiBarcode);
			if (ce != NULL) {
				if (ce->CeSlot != a->SiVid.ViSlot) {
					/*
					 * Cartridges have been moved around.
					 * Move the catalog entry to the new
					 * slot.
					 */
					ce->CeStatus &= ~CES_reconcile;
					MoveCartridge(ce, cat_num,
					    a->SiVid.ViSlot);
					ce = NULL;
					rsp.GrStatus = 0;
					goto out;
				} else {
					ce->CeStatus &= ~CES_reconcile;
				}
		/* N.B. More bad indentation to meet cstyle requirements. */
		} else {
		/*
		 * Check if slot is already full with different
		 * volume.  If so, export this old entry to
		 * the historian.
		 */
		ce = CS_CatalogGetCeByLoc(a->SiVid.ViEq,
		    a->SiVid.ViSlot, a->SiVid.ViPart);
		if ((ce != NULL) && (strcmp(ce->CeBarCode,
		    a->SiBarcode) != 0)) {
			MoveCartridge(ce, Historian, -1);
			if (defaults->flags & DF_EXPORT_UNAVAIL) {
				ce = CS_CatalogGetCeByBarCode(Historian,
				    a->SiVid.ViMtype, a->SiBarcode);
				/*
				 * N.B. More bad indentation to meet cstyle
				 * requirements.
				 */
				if (ce != NULL) {
				ce->CeStatus |= CES_unavail;
				if (RemoteServer) {
					UpdateRemoteCe(ce,
					    RMT_CAT_CHG_FLGS_EXP);
				}
				if (ce->CePart != 0) {
				int en;
				struct CatalogEntry *cea;

				en = 0;
				while ((cea = NextCartridgeEntry(ce, en++))
				    != NULL) {
					cea->CeStatus |= CES_unavail;
					if (RemoteServer) {
						UpdateRemoteCe(cea,
						    RMT_CAT_CHG_FLGS_EXP);
					}
				}
				}
				}
			}
			ce = NULL;
		}
		}
		}

		if ((a->SiVid.ViSlot == (unsigned)ROBOT_NO_SLOT) ||
		    ce == NULL) {
			/*
			 * Barcoded media without a catalog entry.
			 * This is most likely imported media.
			 */
			if (a->SiVid.ViSlot == (unsigned)ROBOT_NO_SLOT) {
				a->SiVid.ViSlot = GetFreeSlot(cat_num);
			}
			ce = GetFreeEntry(cat_num);
			ce->CeStatus |= CES_needs_audit;
			ce->CeSlot = a->SiVid.ViSlot;
			memmove(ce->CeMtype, a->SiVid.ViMtype,
			    sizeof (ce->CeMtype));
			if (a->SiTwoSided) {
				ce->CePart = 1;
				cea = GetFreeEntry(cat_num);
				cea->CeStatus |= CES_needs_audit;
				cea->CeSlot = a->SiVid.ViSlot;
				memmove(cea->CeMtype, a->SiVid.ViMtype,
				    sizeof (cea->CeMtype));
				memmove(cea->CeBarCode, a->SiAltBarcode,
				    sizeof (cea->CeBarCode));
				if (defaults->flags & DF_LABEL_BARCODE) {
					VsnFromBarCode(cea, defaults);
					cea->CeStatus |= CES_labeled;
					cea->CeStatus &= ~CES_needs_audit;
				} else if (*cea->CeVsn != 0) {
					cea->CeStatus |= CES_labeled;
					cea->CeStatus &= ~CES_needs_audit;
				} else {
					cea->CeStatus |= CES_needs_audit;
				}
				cea->CePart = 2;
			}
		}

		memmove(ce->CeBarCode, a->SiBarcode, sizeof (ce->CeBarCode));
		/*
		 * Check if default is label = barcode
		 */
		if ((defaults->flags & DF_LABEL_BARCODE) && (*ce->CeVsn == 0)) {
			VsnFromBarCode(ce, defaults);
			ce->CeStatus |= CES_labeled;
			ce->CeStatus &= ~CES_needs_audit;
		} else if (*ce->CeVsn != 0) {
			ce->CeStatus |= CES_labeled;
			ce->CeStatus &= ~CES_needs_audit;
		} else {
			ce->CeStatus |= CES_needs_audit;
		}

		/*
		 * Check for cleaning tape
		 */
		if (!(ce->CeStatus & CES_cleaning))
			CheckForCleaning(ce);

		rsp.GrStatus = 0;
		goto out;

	} else {
		rsp.GrStatus = 0;
		ce = CS_CatalogGetCeByLoc(a->SiVid.ViEq, a->SiVid.ViSlot,
		    a->SiVid.ViPart);
		if (ce == NULL && a->SiStatus & CES_inuse) {
			/*
			 * There is no catalog entry for this slot but
			 * the library is reporting a cartridge in this slot
			 * or a volume is being imported.  Make a catalog
			 * catalog entry for this slot.
			 */
			if (a->SiVid.ViSlot == (unsigned)ROBOT_NO_SLOT) {
				a->SiVid.ViSlot = GetFreeSlot(cat_num);
			}
			ce = GetFreeEntry(cat_num);
			ce->CeSlot = a->SiVid.ViSlot;
			ce->CePart = a->SiVid.ViPart;
			memmove(ce->CeMtype, a->SiVid.ViMtype,
			    sizeof (ce->CeMtype));
			if (*a->SiVid.ViVsn != '\0') {
				memmove(ce->CeVsn, a->SiVid.ViVsn,
				    sizeof (ce->CeVsn));
			} else {
				ce->CeStatus |= CES_needs_audit;
			}
			if (a->SiTwoSided) {
				ce->CePart = 1;
				cea = GetFreeEntry(cat_num);
				cea->CeStatus |= CES_needs_audit;
				cea->CeSlot = a->SiVid.ViSlot;
				memmove(cea->CeMtype, a->SiVid.ViMtype,
				    sizeof (cea->CeMtype));
				cea->CePart = 2;
			}
		} else {
			if (ce != NULL && (!(a->SiStatus & CES_inuse))) {
				/*
				 * There is a catalog entry for this slot but
				 * the library is reporting this slot is empty.
				 * Set CES_reconcile. ReconcileCatalog will free
				 * the entry later if needed.
				 */
				ce->CeStatus |= CES_reconcile;
				ce = NULL;
				rsp.GrStatus = 0;
			}
		}
	}


out:
	if (rsp.GrStatus == 0) {
		int en;
		uint32_t mask;

		/*
		 * Update status for cartridge.
		 */
		en = 0;
		mask = CES_inuse | CES_occupied | CES_bar_code | CES_reconcile;
		while (ce != NULL) {
			ce->CeStatus &= ~mask;
			ce->CeStatus |= a->SiStatus;
			if (*ce->CeVsn == '\0') {
				ce->CeStatus &= ~CES_labeled;
				ce->CeStatus |= CES_needs_audit;
			} else {
				ce->CeStatus &= ~CES_needs_audit;
				ce->CeStatus |= CES_labeled;
			}
			if ((RemoteServer) && (ce != NULL) &&
			    (!(ce->CeStatus & CES_cleaning))) {
				UpdateRemoteCe(ce, 0);
			}
			if (ce->CePart == 0) {
				break;
			}
			ce = NextCartridgeEntry(ce, en++);
		}
	}

	TraceRequest(hdr, &rsp, "SlotInit(%s, %08x, %d, %s, %s)",
	    StringFromVolId(&a->SiVid), a->SiStatus, a->SiTwoSided,
	    a->SiBarcode, a->SiAltBarcode);
	return (&rsp);
}


/*
 * Set a field of a catalog entry.
 */
static void *
SetField(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrSetField *a = (struct CsrSetField *)arg;
	struct CatalogEntry *ce;
	int		en;

	rsp.GrStatus = -1;
	if ((ce = CS_CatalogGetEntry(&a->SfVid)) == NULL) {
		goto out;
	}
	if (a->SfVid.ViFlags == VI_logical) {
		a->SfVid.ViFlags &= ~VI_part;	/* Avoid cartridge action */
	}
	en = 0;
	while (ce != NULL) {
		struct CatalogEntry *ce_tofree;

		ce_tofree = NULL;
		switch (a->SfField) {
		case CEF_Status:
			ce->CeStatus = (ce->CeStatus & ~a->a.v.SfMask) |
			    a->a.v.SfVal;
			/*
			 * If clearing inuse, free the entry.
			 */
			if (!(ce->CeStatus & CES_inuse)) {
				ce_tofree = ce;
			}
			/*
			 * Notify archiver of interesting changes.
			 */
			if (a->a.v.SfMask &
			    (CES_read_only |
			    CES_unavail |
			    CES_recycle |
			    CES_non_sam |
			    CES_bad_media)) {
				(void) ArchiverCatalogChange();
			}
			break;
		case CEF_MediaType:
			if (SortAndCheckForDuplicateVsns(a->a.SfString,
			    ce->CeVsn, ce, FALSE, FALSE) != 0) {
				errno = ER_DUPLICATE_VSN;
				goto out;
			}
			memmove(ce->CeMtype, a->a.SfString,
			    sizeof (ce->CeMtype));
			break;
		case CEF_Vsn:
			/* Avoid cartridge action */
			a->SfVid.ViFlags &= ~VI_part;
			if (SortAndCheckForDuplicateVsns(ce->CeMtype,
			    a->a.SfString, ce, FALSE, FALSE) != 0) {
				errno = ER_DUPLICATE_VSN;
				goto out;
			}
			memmove(ce->CeVsn, a->a.SfString, sizeof (ce->CeVsn));
			break;
		case CEF_VolInfo:
			memmove(ce->CeVolInfo, a->a.SfString,
			    sizeof (ce->CeVolInfo));
			break;
		case CEF_Slot:	ce->CeSlot = a->a.v.SfVal; break;
		case CEF_Partition: ce->CePart = a->a.v.SfVal; break;
		case CEF_Access: ce->CeAccess = a->a.v.SfVal; break;
		case CEF_Capacity: ce->CeCapacity = a->a.v.SfVal; break;
		case CEF_Space:	ce->CeSpace = a->a.v.SfVal; break;
		case CEF_BlockSize: ce->CeBlockSize = a->a.v.SfVal; break;
		case CEF_LabelTime: ce->CeLabelTime = a->a.v.SfVal; break;
		case CEF_ModTime: ce->CeModTime   = a->a.v.SfVal; break;
		case CEF_MountTime: ce->CeMountTime = a->a.v.SfVal; break;
		case CEF_BarCode:
			memmove(ce->CeBarCode, a->a.SfString,
			    sizeof (ce->CeBarCode));
			break;
		case CEF_PtocFwa: ce->m.CePtocFwa = a->a.v.SfVal; break;
		case CEF_LastPos: ce->m.CeLastPos = a->a.v.SfVal; break;
		default:
			errno = EINVAL;
			goto out;
		}

		if (RemoteServer) {
			UpdateRemoteCe(ce, 0);
		}

		if (a->SfVid.ViFlags == VI_cart && ce->CePart != 0) {
			/*
			 * If cartridge reference and multi-partition volume,
			 * find another entry.
			 */
			ce = NextCartridgeEntry(ce, en++);
		} else {
			ce = NULL;
		}
		if (ce_tofree != NULL) {
			FreeEntry(ce_tofree);
		}
	}
	rsp.GrStatus = 0;

out:
	if (a->SfField == CEF_MediaType ||
	    a->SfField == CEF_Vsn ||
	    a->SfField == CEF_BarCode) {
		TraceRequest(hdr, &rsp, "SetField(%s, %d, %s)",
		    StringFromVolId(&a->SfVid), a->SfField, a->a.SfString);
	} else if (a->SfField == CEF_Status) {
		TraceRequest(hdr, &rsp, "SetField(%s, status, %08llx, %08x)",
		    StringFromVolId(&a->SfVid), a->a.v.SfVal,
		    a->a.v.SfMask);
	} else {
		TraceRequest(hdr, &rsp, "SetField(%s, %d, %llu)",
		    StringFromVolId(&a->SfVid), a->SfField, a->a.v.SfVal);
	}
	return (&rsp);
}


/*
 * Unreserve a volume for archiving.
 */
static void *
UnReserveVolume(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrUnReserveVolume *a = (struct CsrUnReserveVolume *)arg;
	struct CatalogEntry *ce;

	rsp.GrStatus = -1;
	if ((ce = CS_CatalogGetEntry(&a->UrVid)) == NULL) {
		goto out;
	}
	if (ce->r.CerTime == 0) {
		errno = ER_VOLUME_NOT_RESERVED;
		goto out;
	}
	rsp.GrStatus = 0;
	ce->r.CerTime = 0;
	/* Volume %s unreserved from %s/%s/%s */
	SendCustMsg(HERE, 18035, VolStringFromCe(ce),
	    ce->r.CerAsname, ce->r.CerOwner, ce->r.CerFsname);
	memset(ce->r.CerAsname, 0, sizeof (ce->r.CerAsname));
	memset(ce->r.CerOwner, 0, sizeof (ce->r.CerOwner));
	memset(ce->r.CerFsname, 0, sizeof (ce->r.CerFsname));

out:
	TraceRequest(hdr, &rsp, "UnReserveVolume(%s)",
	    StringFromVolId(&a->UrVid));
	return (&rsp);
}


/*
 * Process volume loaded in a drive.
 * If this is an audit, fill in the catalog entry.
 * If not, verify catalog entry fields.
 */
static void *
VolumeLoaded(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrVolumeLoaded *a = (struct CsrVolumeLoaded *)arg;
	struct CatalogEntry *ce, *cea;
	int en;
	int ret;

	cea = &a->VlCe;
	rsp.GrStatus = -1;
	ce = CS_CatalogGetCeByLoc(cea->CeEq, cea->CeSlot, cea->CePart);

	/*
	 * Handle a blank check or scan error
	 */
	if (ce != NULL) {
		if ((*ce->CeVsn != '\0') && (*cea->CeVsn == '\0')) {
			ce->CeStatus |= (CES_needs_audit | CES_bad_media);
			goto out;
		}
	}

	if (ce == NULL || ce->CeStatus & CES_needs_audit ||
	    *ce->CeVsn == '\0' || !(ce->CeStatus & CES_labeled)) {
		struct CatalogEntry *ceh;

		/*
		 * Volume not found in catalog, or needs audit, or unlabeled.
		 */
		ceh = CS_CatalogGetCeByMedia(cea->CeMtype, cea->CeVsn);
		if (ceh != NULL && ce != ceh) {
			int cat_num;

			cat_num = FindCatalog(ceh->CeEq);
			if (cat_num != Historian) {

				if (SortAndCheckForDuplicateVsns(cea->CeMtype,
				    cea->CeVsn, ce, FALSE, TRUE) != 0) {
					/*
					 * Fill in vsn and label time for easy
					 * identification
					 */
					memmove(ce->CeVsn, cea->CeVsn,
					    sizeof (ce->CeVsn));
					ce->CeStatus |= CES_labeled;
					ce->CeStatus &= ~CES_needs_audit;
					ce->CeLabelTime = cea->CeLabelTime;
					ce->CeMountTime = time(NULL);
					ce->CeAccess++;
					errno = ER_DUPLICATE_VSN;
					goto out;
				}
			}

			/*
			 * Volume found in historian.
			 * Verify loaded information with historian.
			 */
			if ((cat_num = FindCatalog(cea->CeEq)) == -1) {
				goto out;
			}
			(void) VerifyVolume(ceh, cea, 0);
			/*
			 * Free cartridge in this catalog.
			 * The historian may have a different number
			 * of partitions. The volume loaded shows only
			 * one partition.
			 */
			en = 0;
			while (ce != NULL) {
				struct CatalogEntry *ce_tofree;

				ce_tofree = ce;
				if (ce->CePart != 0) {
					ce = NextCartridgeEntry(ce, en++);
				} else {
					ce = NULL;
				}
				FreeEntry(ce_tofree);
			}
			/*
			 * Move cartridge from historian.
			 */
			MoveCartridge(ceh, cat_num, cea->CeSlot);
			/*
			 * Find entry again.  The catalog may have moved.
			 */
			ce = CS_CatalogGetCeByLoc(cea->CeEq, cea->CeSlot,
			    cea->CePart);
			if ((ce != NULL) && (ce->CeStatus & CES_unavail)) {
			ce->CeStatus &= ~CES_unavail;
			if (ce->CePart != 0) {
				int en;
				struct CatalogEntry *cea;

				en = 0;
				while ((cea = NextCartridgeEntry(ce,
				    en++)) != NULL) {
					cea->CeStatus &= ~CES_unavail;
				}
			}
			}
			(void) ArchiverCatalogChange();
		} else {
			if (ce == NULL) {
				int cat_num;

				if ((cat_num = FindCatalog(cea->CeEq)) == -1) {
					goto out;
				}
				ce = GetFreeEntry(cat_num);
				if (ce == NULL) {
					errno = EINVAL;
					goto out;
				}
				ce->CeStatus |= CES_inuse;
				ce->CeSlot = cea->CeSlot;
				ce->CePart = cea->CePart;
			}
			/*
			 * Keep original barcode, access count, mod time,
			 * and archiver info.
			 * Replace all the information reported.
			 */
			memmove(ce->CeMtype, cea->CeMtype,
			    sizeof (ce->CeMtype));
			memmove(ce->CeVsn, cea->CeVsn, sizeof (ce->CeVsn));
			if (!(ce->CeStatus & CES_capacity_set))
				ce->CeCapacity = cea->CeCapacity;
			ce->CeSpace = cea->CeSpace;
			ce->CeBlockSize = cea->CeBlockSize;
			ce->CeLabelTime = cea->CeLabelTime;
			ce->m.CePtocFwa = cea->m.CePtocFwa;
		}
		if (ce->CeVsn[0] != '\0') {
			ce->CeStatus |= CES_labeled;
			ce->CeStatus &= ~CES_needs_audit;
		}
	} else {
		if (cea->CeLabelTime == 0 && ce->CeLabelTime != 0) {
			(void) VerifyVolume(ce, cea, 0);
		} else {
			(void) VerifyVolume(ce, cea, 1);
			if (ce->CeSpace > ce->CeCapacity) {
				ce->CeSpace = ce->CeCapacity;
			}
		}
		ce->CeBlockSize = cea->CeBlockSize;
	}

	/*
	 * Check for duplicate VSNs
	 */
	ret = SortAndCheckForDuplicateVsns(ce->CeMtype, ce->CeVsn, ce,
	    FALSE, TRUE);
	if (ret != 0) {
		/*
		 * Fill in vsn and label time for easy identification
		 */
		memmove(ce->CeVsn, cea->CeVsn, sizeof (ce->CeVsn));
		ce->CeStatus |= (CES_dupvsn | CES_labeled);
		ce->CeStatus &= ~CES_needs_audit;
		ce->CeLabelTime = cea->CeLabelTime;
		ce->CeMountTime = time(NULL);
		ce->CeAccess++;
		errno = ER_DUPLICATE_VSN;
		goto out;
	} else {
		ce->CeStatus &= ~CES_dupvsn;
	}

	/*
	 * Merge status bits.
	 */
	ce->CeStatus &= ~(CES_occupied | CES_needs_audit | CES_writeprotect);
	ce->CeStatus |= CES_inuse | (cea->CeStatus &
	    (CES_bad_media | CES_writeprotect |
	    CES_needs_audit | CES_bar_code));

	/*
	 * Update access information.
	 */
	if (ce->CeStatus & CES_cleaning) {
		if (ce->CeAccess != 0) {
			ce->CeAccess--;
		}
	} else {
		ce->CeAccess++;
	}
	rsp.GrStatus = 0;

out:
	/*
	 * Update status for cartridge.
	 */
	ce->CeMountTime = time(NULL);
	en = 0;
	while (ce != NULL) {
		ce->CeStatus |= CES_inuse;
		ce->CeStatus &= ~CES_occupied;
		if (RemoteServer) {
			UpdateRemoteCe(ce, 0);
		}
		if (ce->CePart == 0) {
			break;
		}
		ce = NextCartridgeEntry(ce, en++);
	}

	TraceRequest(hdr, &rsp, "VolumeLoaded(%s)", VolStringFromCe(cea));
	return (&rsp);
}


/*
 * Set fields needed when media is unloaded
 */
static void *
VolumeUnloaded(
	void *arg,
	struct UdsMsgHeader *hdr)
{
	static struct CsrGeneralRsp rsp;
	struct CsrVolumeUnloaded *a = (struct CsrVolumeUnloaded *)arg;
	struct CatalogEntry *ce;
	int		en;

	rsp.GrStatus = -1;
	if ((a->SfVid.ViFlags & VI_logical) == VI_logical) {
		ce = CS_CatalogGetCeByMedia(a->SfVid.ViMtype, a->SfVid.ViVsn);
	} else if ((a->SfVid.ViFlags & VI_cart) == VI_cart) {
		ce = CS_CatalogGetCeByLoc(a->SfVid.ViEq, a->SfVid.ViSlot,
		    a->SfVid.ViPart);
		if (ce == NULL) {
			/*
			 * Try to find an entry for the cartridge.
			 */
			ce = CS_CatalogGetCeByLoc(a->SfVid.ViEq,
			    a->SfVid.ViSlot, 0);
			if (ce != NULL) {
				ce->CePart = a->SfVid.ViPart;
			}
		}
	}
	/*
	 * If still no ce, try getting by barcode.
	 */
	if (ce == NULL) {
		ce = CS_CatalogGetCeByBarCode(a->SfVid.ViEq,
		    a->SfVid.ViMtype, a->SfBarcode);
	}
	if (ce == NULL) {
		TraceRequest(hdr, &rsp,
		    "VolumeUnloaded(%s, flags=0x%x, bc=%s):no entry found",
		    StringFromVolId(&a->SfVid), a->SfVid.ViFlags, a->SfBarcode);
	}

	/*
	 * Update status for the cartridge.
	 */
	en = 0;
	while (ce != NULL) {
		rsp.GrStatus = 0;
		ce->CeStatus |= CES_occupied;
		ce->CeStatus &= ~CES_unavail;
		if (RemoteServer) {
			UpdateRemoteCe(ce, 0);
		}
		if (ce->CePart == 0) {
			break;
		}
		ce = NextCartridgeEntry(ce, en++);
	}

	TraceRequest(hdr, &rsp, "VolumeUnloaded(%s, %s)",
	    StringFromVolId(&a->SfVid), a->SfBarcode);
	return (&rsp);
}



/* Private functions. */

/*
 * Catch signals.
 */
static void
CatchSignals(
	int SigNum)
{
	switch (SigNum) {

	case SIGHUP:
	case SIGINT:
		srvr.UsStop = SigNum;
		break;

	default:
		break;
	}
	TraceSignal(SigNum);
}


/*
 * Check if this is a cleaning tape.
 * If it is a cleaning tape, clear the capacity and space.
 * If it is a cleaning tape but the cleaning bit is not already set,
 * initialize the access (cleaning cycle count) and set the cleaning bit.
 */
static void
CheckForCleaning(
	struct CatalogEntry *ce)
{
	int match;

	match = !memcmp(ce->CeBarCode, CLEANING_BAR_CODE, CLEANING_BAR_LEN) ||
	    !memcmp(ce->CeBarCode, CLEANING_FULL_CODE, CLEANING_FULL_LEN);

	if (!match) {
		ce->CeStatus &= ~CES_cleaning;
		return;
	}
	/*
	 * Set the capacity and space to zero for cleaning tapes.
	 */
	ce->CeCapacity = 0;
	ce->CeSpace = 0;

	/*
	 * If not already marked a cleaning tape, initialize the cycle count.
	 */
	if ((ce->CeStatus & CES_cleaning) == 0) {
		switch (sam_atomedia(ce->CeMtype)) {

			case DT_3592:
			case DT_IBM3580:
				/* As per IBM GA32-0415-00, page 25 */
				ce->CeAccess = 50;
				break;

			case DT_LINEAR_TAPE:
				ce->CeAccess = 20;
				break;

			case DT_EXABYTE_TAPE:
				ce->CeAccess = 12;
				break;

			case DT_SONYAIT:
			case DT_SONYSAIT:
				ce->CeAccess = 50;
				break;

			case DT_9840:
			case DT_9940:
				ce->CeAccess = 100;
				break;

			case DT_TITAN:
				ce->CeAccess = 50;
				break;

			default:
				ce->CeAccess = 30;
				break;
		}
		ce->CeStatus |= CES_cleaning;
	}
}


/*
 * Compare VSNs.
 */
static int
CompareVsns(
	const void *p1,
	const void *p2)
{
	struct CatalogEntry *ce1 = *(struct CatalogEntry **)p1;
	struct CatalogEntry *ce2 = *(struct CatalogEntry **)p2;
	int cmp;

	cmp = strcmp(ce1->CeMtype, ce2->CeMtype);
	if (cmp != 0) {
		return (cmp);
	}
	return (strcmp(ce1->CeVsn, ce2->CeVsn));
}


/*
 * Free a catalog entry.
 */
static void
FreeEntry(
	struct CatalogEntry *ce)
{
	uint32_t eq;
	int		mid;

	eq   = ce->CeEq;
	mid  = ce->CeMid;
	memset(ce, 0, sizeof (struct CatalogEntry));
	ce->CeEq   = eq;
	ce->CeMid    = mid;
}


/*
 * Get a free entry in a catalog.
 * Search the catalog for the first entry not in use.
 * If the catalog is full, grow it.
 */
static struct CatalogEntry *
GetFreeEntry(
	int cat_num)
{
	/*
	 * This loop construction accounts for a change in the addresses
	 * of the mmapped catalog.
	 */
	do {
		struct CatalogHdr *ch;
		int ne;

		ch = Catalogs[cat_num].CmHdr;

		/*
		 * Find first entry not in use.
		 */
		for (ne = 0; ne < ch->ChNumofEntries; ne++) {
			struct CatalogEntry *ce;

			ce = &ch->ChTable[ne];
			if (!(ce->CeStatus & CES_inuse)) {
				ce->CeStatus |= CES_inuse;
				return (ce);
			}
		}
		/*
		 * Catalog is full.
		 */
	} while (GrowCatalog(cat_num, CATALOG_TABLE_INCR) != -1);
	return (NULL);
}


/*
 * Find a free slot in a catalog.
 * Return the lowest number slot that is not in use.
 */
static int
GetFreeSlot(
	int cat_num)
{
	struct CatalogHdr *ch;
	int ne;
	int slot;

	ch = Catalogs[cat_num].CmHdr;
	for (slot = 0; slot < MAX_SLOTS; slot++) {

		/*
		 * Search for slot in use.
		 */
		for (ne = 0; ne < ch->ChNumofEntries; ne++) {
			struct CatalogEntry *ce;

			ce = &ch->ChTable[ne];
			if ((ce->CeStatus & CES_inuse) && ce->CeSlot == slot) {
				break;
			}
		}
		if (ne == ch->ChNumofEntries) {
			return (slot);
		}
	}
	return (slot);
}


/*
 * Increase the size of a catalog.
 */
static int			/* -1 if failed */
GrowCatalog(
	int cat_num,		/* Catalog number */
	int increase)		/* Increase needed */
{
	struct CatalogHdr *ch_old;
	struct CatalogHdr *ch;
	void	*mp;
	size_t	size_old;
	size_t	size;
	int NumofEntries;
	int ne;

	ch_old = Catalogs[cat_num].CmHdr;
	size_old = sizeof (struct CatalogHdr) +
	    ch_old->ChNumofEntries * sizeof (struct CatalogEntry);
	NumofEntries = ch_old->ChNumofEntries + increase;
	if (CatalogCreateCatfile(CatalogTable->CtFname[cat_num], NumofEntries,
	    &mp, &size, LibLogit) == -1) {
		SysError(HERE, "Catalog %s size increase to %d failed",
		    CatalogTable->CtFname[cat_num], NumofEntries);
		return (-1);
	}

	/*
	 * Copy the old entries to the new.
	 */
	ch = (struct CatalogHdr *)mp;
	memmove(ch, ch_old, size_old);
	ch->ChNumofEntries = NumofEntries;

	/*
	 * Fill in the fields.
	 */
	for (ne = ch_old->ChNumofEntries; ne < ch->ChNumofEntries; ne++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[ne];
		ce->CeEq   = ch->ChEq;
		ce->CeMid    = LastMid++;
	}

	/*
	 * Stale the old catalog.
	 */
	ch_old->ChVersion *= 10;
	(void) CatalogSync();
	Trace(TR_MISC, "Catalog %s increased to %d entries",
	    CatalogTable->CtFname[cat_num], NumofEntries);
	return (0);
}


/*
 * Log a library error.
 */
static void
LibLogit(
	int exit_status,
	char *msg)
{
	if (exit_status != 0) {
		errno = 0;
		SysError(HERE, msg);
	} else {
		Trace(TR_MISC, msg);
	}
}


/*
 * Move the entries for a cartridge to destination catalog.
 * All entries that have the same slot in the equipment are moved.
 */
static void
MoveCartridge(
	struct CatalogEntry *ce,	/* Entry to move */
	int cat_num,	/* Destination catalog */
	int slot)	/* Destination slot. If -1, select first available */
{
	int	en;		/* entry number */

	if (slot == -1)  slot = GetFreeSlot(cat_num);
	en = 0;
	while (ce != NULL) {
		struct CatalogEntry *ced;	/* destination catalog entry */
		struct CatalogEntry *ce_tofree;
		uint16_t eq;
		uint16_t seq = ce->CeEq;
		uint32_t sslot = ce->CeSlot;
		uint16_t spart = ce->CePart;
		int mid;


		ced = GetFreeEntry(cat_num);
		/*
		 * Find entry again.  The catalog may have moved.
		 */
		ce = CS_CatalogGetCeByLoc(seq, sslot, spart);

		/*
		 * Clear CES_inuse so a searcher won't find both entries
		 */
		if (ce != NULL) ce->CeStatus &= ~CES_inuse;

		eq  = ced->CeEq;
		mid = ced->CeMid;
		memmove(ced, ce, sizeof (struct CatalogEntry));
		ced->CeStatus |= CES_inuse;
		ced->CeEq = eq;
		ced->CeMid = mid;
		ced->CeSlot = slot;
		if (cat_num == Historian) {
			ced->CeStatus |= CES_occupied;
			SendCustMsg(HERE, 18020, VolStringFromCe(ce));
		} else {
			SendCustMsg(HERE, 18021, VolStringFromCe(ce), eq);
		}

		/*
		 * If multi-partition media, find another entry.
		 */
		ce_tofree = ce;
		if (ce->CePart != 0) {
			while ((ce = NextCartridgeEntry(ce, en++)) != NULL) {

				if (!(ce->CeStatus & CES_inuse)) {
					continue;
				}
				/*
				 * Verify that it's not already there.
				 * This should never happen.
				 */
				ced = CS_CatalogGetCeByLoc(eq, slot,
				    ce->CePart);
				if (ced == NULL ||
				    !(ced->CeStatus & CES_inuse)) {
					break;
				}
				SendCustMsg(HERE, 18023,
				    VolStringFromCe(ce), eq);
				FreeEntry(ced);
			}
		} else {
			ce = NULL;
		}
		FreeEntry(ce_tofree);
	}
}


/*
 * Return the next entry for a cartridge.
 */
static struct CatalogEntry *
NextCartridgeEntry(
	struct CatalogEntry *cea,
	int en)
{
	static struct CatalogEntry *ce, *ce_end;

	if (0 == en) {
		struct CatalogHdr *ch;
		int		nc;

		if ((nc = FindCatalog(cea->CeEq)) == -1) {
			return (NULL);
		}
		ch = Catalogs[nc].CmHdr;
		ce = &ch->ChTable[0];
		ce_end = &ch->ChTable[ch->ChNumofEntries];
	}

	/*
	 * Find next entry with matching slot.
	 */
	while (ce < ce_end) {
		struct CatalogEntry *cet;

		cet = ce++;
		if (cet != cea && (cet->CeStatus & CES_inuse) &&
		    cet->CeSlot == cea->CeSlot) {
			return (cet);
		}
	}
	return (NULL);
}


/*
 * If checkall, check each entry in each catalog against all
 * catalog entries. Mark any duplicates found and
 * return number of duplicates.
 * If not checkall, check all entries in all catalogs for a
 * particular media type and vsn. Mark it as dup if markit.
 */
static int
SortAndCheckForDuplicateVsns(
	char *mtype,
	char *vsn,
	struct CatalogEntry *cea,
	boolean_t checkall,	/* TRUE-compare all vsns */
				/*    FALSE-compare single vsn */
	boolean_t markit)
{

	int num_dups = 0;
	int nc, total_ne, ne;
	struct CatalogEntry *ce;
	/* sorted pointers to catalog entries */
	struct CatalogEntry **sort_index;
	struct CatalogHdr *ch;

	/* Count the number of entries in all catalogs */
	total_ne = 0;
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		ch = Catalogs[nc].CmHdr;
		total_ne += ch->ChNumofEntries;
	}

	SamMalloc(sort_index, total_ne * sizeof (struct CatalogEntry *));

	/* Fill in index of pointers to each in_use catalog entry */
	total_ne = 0;
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		ch = Catalogs[nc].CmHdr;

		for (ne = 0; ne < ch->ChNumofEntries; ne++) {
			/* Only consider entries actually inuse */
			ce = &ch->ChTable[ne];
			if (ce->CeStatus & CES_inuse) {
				sort_index[total_ne] = ce;
				total_ne++;
			}
		}
	}

	/* Sort the list by vsn name */
	qsort(sort_index, total_ne,
	    sizeof (struct CatalogEntry *), CompareVsns);

	if (checkall && markit) {
		/*
		 * Run through the big list and
		 * clear all the dup flags.
		 */
		for (ne = 0; ne < total_ne; ne++) {
			ce = sort_index[ne];
			ce->CeStatus &= ~CES_dupvsn;
		}
	}

	if (checkall) {
		/*
		 * Compare adjacent list entries for
		 * duplicate media type and vsn.
		 */
		for (ne = 0; ne < (total_ne -1); ne++) {
			struct CatalogEntry	*tmp_ce1, *tmp_ce2;

			tmp_ce1 = sort_index[ne];
			tmp_ce2 = sort_index[ne+1];
			if ((tmp_ce1->CeStatus & CES_inuse) &&
			    (*tmp_ce1->CeVsn != '\0') &&
			    (strcmp(tmp_ce1->CeVsn, tmp_ce2->CeVsn) == 0) &&
			    (strcmp(tmp_ce1->CeMtype, tmp_ce2->CeMtype) == 0)) {
				num_dups = num_dups + 2;
				if (markit) {
					tmp_ce1->CeStatus |= CES_dupvsn;
					tmp_ce2->CeStatus |= CES_dupvsn;
				}
			}
		}
	} else {
		/*
		 * Compare a single vsn against all vsns in
		 * the big list for duplicate media type and vsn.
		 */
		for (ne = 0; ne < total_ne; ne++) {
			ce = sort_index[ne];
			if (cea != NULL && ce == cea)  continue;
			if ((ce->CeStatus & CES_inuse) &&
			    (*ce->CeVsn != '\0') &&
			    (strcmp(ce->CeVsn, vsn) == 0) &&
			    (strcmp(ce->CeMtype, mtype) == 0)) {
				num_dups = num_dups + 2;
				if (markit) {
					ce->CeStatus |= CES_dupvsn;
					cea->CeStatus |= CES_dupvsn;
				}
			}
		}
	}

	SamFree(sort_index);
	return (num_dups);
}


/*
 * Log a communication error.
 */
static void
SrvrLogit(
	char *msg)
{
	errors++;
	Trace(TR_ERR, "%s", msg);
}


/*
 * Return string from catalog entry.
 * Format eq:slot:partition mtype.VSN
 */
static char *
VolStringFromCe(
	struct CatalogEntry *ce)	/* Catalog entry */
{
	struct VolId vid;

	return (StringFromVolId(CatalogVolIdFromCe(ce, &vid)));
}


/*
 * Return string from volume id.
 * Format eq:slot:partition mtype.VSN
 */
static char *
StringFromVolId(
	struct VolId *vid)
{
	static char buf[256];
	char *p, *p_end;

	p = buf;
	p_end = buf + sizeof (buf) - 1;

	/*
	 * Include valid fields.
	 */
	if (vid->ViFlags & VI_eq) {
		p += snprintf(p, p_end - p, "%d", vid->ViEq);
	}
	if (vid->ViFlags & VI_slot) {
		p += snprintf(p, p_end - p, ":%d", vid->ViSlot);
	}
	if (vid->ViFlags & VI_part) {
		p += snprintf(p, p_end - p, ":%d", vid->ViPart);
	}
	if (vid->ViFlags & VI_onepart)  *p++ = ' ';
	if (vid->ViFlags & VI_logical) {
		if (vid->ViFlags & VI_mtype) {
			if (*vid->ViMtype == '\0') {
				p += snprintf(p, p_end - p, "_.");
			} else {
				p += snprintf(p, p_end - p, "%s.",
				    vid->ViMtype);
			}
		} else {
			p += snprintf(p, p_end - p, "??.");
		}
		if (vid->ViFlags & VI_vsn) {
			p += snprintf(p, p_end - p, "%s", vid->ViVsn);
		}
	}
	*p = '\0';
	return (buf);
}


/*
 * Trace server request.
 * Returns errno to caller.
 */
static void
TraceRequest(
	struct UdsMsgHeader *hdr,
	struct CsrGeneralRsp *rsp,
	const char *fmt,		/* printf() style format. */
	...)
{
	char msg[256];

	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsprintf(msg, fmt, args);
		va_end(args);
	} else {
		*msg = '\0';
	}
	TraceName = hdr->UhName;
	TracePid = hdr->UhPid;
	if (rsp != NULL && rsp->GrStatus == -1) {
		rsp->GrErrno = errno;
		_Trace(TR_err, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	} else {
		_Trace(TR_misc, hdr->UhSrcFile, hdr->UhSrcLine, "%s", msg);
	}
	TraceName = program_name;
	TracePid = getpid();
	msgs_processed++;
}


/*
 * Send a changed catalog entry to the remote client
 * in order to update the remote catalog.
 */
void
UpdateRemoteCe(
	struct CatalogEntry *ce,
	int flags)
{
	struct dev_ent *dev, *devhead;
	rmt_mess_cat_chng_t *change;

	if (ShmatSamfs(O_RDWR) != NULL) {
		devhead =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	} else {
		shmdt(master_shm.shared_memory);
		return;
	}

	for (dev = devhead; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		rmt_mess_t *msg;
		message_request_t *message;

		if (!IS_RSS(dev)) {
			continue;
		}

		message = (message_request_t *)SHM_REF_ADDR(dev->dt.ss.message);

		mutex_lock(&message->mutex);
		while (message->mtype != MESS_MT_VOID)
			cond_wait(&message->cond_i, &message->mutex);

		message->message.command =
		    (enum sam_message)RMT_MESS_CATALOG_CHANGE;
		message->mtype = MESS_MT_RS_SERVER;
		msg = &message->message.param.rmt_message;
		memset(msg, 0, sizeof (*msg));
		change = &msg->messages.cat_change;
		change->entry.status.bits = ce->CeStatus;
		if (flags == RMT_CAT_CHG_FLGS_EXP) {
			change->entry.status.bits |= CES_unavail;
		}
		change->entry.media = sam_atomedia(ce->CeMtype);
		change->entry.slot = ce->CeSlot;
		change->entry.partition = ce->CePart;
		memcpy(change->entry.vsn, ce->CeVsn,
		    sizeof (change->entry.vsn));
		msg->messages.cat_change.eq = ce->CeEq;
		msg->messages.cat_change.flags = flags;
		cond_signal(&message->cond_r);
		mutex_unlock(&message->mutex);
	}

	shmdt(master_shm.shared_memory);
	return;

}


/*
 * Verify catalog entry data with loaded volume.
 */
static int				/* 0 if no errors */
VerifyVolume(
	struct CatalogEntry *ce,	/* Catalog entry */
	struct CatalogEntry *vol,	/* Data from loaded volume */
	int replace)			/* Replace field if non-zero */
{
	int errors;

	errors = 0;
	if ((*vol->CeVsn != '\0') &&
	    (strcmp(ce->CeMtype, vol->CeMtype)) != 0) {
		errors++;
		SendCustMsg(HERE, 18029, VolStringFromCe(ce), vol->CeMtype);
		if (replace) {
			memmove(ce->CeMtype, vol->CeMtype,
			    sizeof (ce->CeMtype));
		}
	}
	if ((*vol->CeVsn != '\0') &&
	    (strcmp(ce->CeVsn, vol->CeVsn)) != 0) {
		errors++;
		SendCustMsg(HERE, 18024, VolStringFromCe(ce), vol->CeVsn);
		if (replace) {
			memmove(ce->CeVsn, vol->CeVsn, sizeof (ce->CeVsn));
		}
	}
	if ((ce->CeLabelTime != 0) &&
	    (ce->CeLabelTime != vol->CeLabelTime)) {
		errors++;
		SendCustMsg(HERE, 18028, VolStringFromCe(ce), vol->CeLabelTime);
		if (replace)  ce->CeLabelTime = vol->CeLabelTime;
	}
	if ((*vol->CeBarCode != '\0') &&
	    (strcmp(ce->CeBarCode, vol->CeBarCode)) != 0) {
		errors++;
		SendCustMsg(HERE, 18025, VolStringFromCe(ce), vol->CeBarCode);
	}
	if ((ce->CeCapacity != vol->CeCapacity) &&
	    (!(ce->CeStatus & CES_capacity_set))) {
		errors++;
		SendCustMsg(HERE, 18026, VolStringFromCe(ce), vol->CeCapacity);
		if (replace)  ce->CeCapacity = vol->CeCapacity;
	}
	if ((ce->CeBlockSize != 0) &&
	    (ce->CeBlockSize != vol->CeBlockSize)) {
		errors++;
		SendCustMsg(HERE, 18027, VolStringFromCe(ce), vol->CeBlockSize);
		if (replace)  ce->CeBlockSize = vol->CeBlockSize;
	}
	if ((ce->CeStatus & CES_writeprotect) !=
	    (vol->CeStatus & CES_writeprotect)) {
		SendCustMsg(HERE, 18030, VolStringFromCe(ce),
		    (ce->CeStatus & CES_writeprotect) ? 'W' : '-',
		    (vol->CeStatus & CES_writeprotect) ? 'W' : '-');
		/* Status bits altered later. */
	}
	return (errors);
}


/*
 * Create the vsn given the barcode
 */
static void
VsnFromBarCode(
	struct CatalogEntry *ce,
	sam_defaults_t *defaults)
{
	char *tmpvsn;
	int barcode_len;
	int vsn_len;

	memset(ce->CeVsn, 0, sizeof (ce->CeVsn));
	*(ce->CeBarCode + BARCODE_LEN) = '\0';
	barcode_len = strlen(ce->CeBarCode);

	if (is_tape(sam_atomedia(ce->CeMtype))) vsn_len = 6;
	else vsn_len = 31;

	if (defaults->flags & DF_BARCODE_LOW) {
		if (barcode_len > vsn_len) {
			strncpy(ce->CeVsn,
			    (ce->CeBarCode + (barcode_len - vsn_len)), vsn_len);
		} else {
			strncpy(ce->CeVsn, ce->CeBarCode, vsn_len);
		}
	} else {
		strncpy(ce->CeVsn, ce->CeBarCode, vsn_len);
	}

	/*
	 * Remove embedded blanks and make all uppercase
	 */
	if (is_tape(sam_atomedia(ce->CeMtype))) {
		for (tmpvsn = ce->CeVsn; *tmpvsn != '\0'; tmpvsn++) {
			if (*tmpvsn == ' ') *tmpvsn = '_';
			else (*tmpvsn) = toupper(*tmpvsn);
		}
	}
}


/* Catalog file processing. */


/*
 * Message function for catalog file processing.
 */

static char ErrMsg[256];

static void
MsgFunc(
/* LINTED argument unused in function */
	int status,
	char *msg)
{
	strncpy(ErrMsg, msg, sizeof (ErrMsg)-1);
}


/*
 * Process catalog files.
 */
static void
MakeCatalogTable(
	void)
{
	struct dev_ent *dev, *devhead;
	size_t	CtSize;		/* Size of catalog table */
	int fd;
	int nc;
	int num_dups;

	snprintf(CatalogTableName, sizeof (CatalogTableName),
	    "%s/catalog/%s", SAM_VARIABLE_PATH, "CatalogTable");
	/*
	 * Invalidate existing catalog table.
	 */
	CatalogTable = MapFileAttach(CatalogTableName, CT_MAGIC, O_RDWR);
	if (CatalogTable != NULL) {
		Trace(TR_MISC, "Invalidate %s", CatalogTableName);
		CatalogTable->Ct.MfValid = 0;
		(void) MapFileDetach(CatalogTable);
		(void) unlink(CatalogTableName);
	}

	/*
	 * Make initial catalog table.
	 */
	CtSize = sizeof (struct CatalogTableHdr);
	SamMalloc(CatalogTable, CtSize);
	CatalogTable->Ct.MfMagic = CT_MAGIC;
	CatalogTable->Ct.MfValid = 1;
	CatalogTable->CtTime = time(NULL);
	CatalogTable->CtNumofFiles = 0;

	/*
	 * If the SAM shared memory segment is available, attach it to
	 * find the equipment.  Otherwise, read the mcf for equipment.
	 */
	if (ShmatSamfs(O_RDWR) != NULL) {
		devhead =
		    (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	} else {
		extern int read_mcf(char *mcf, dev_ent_t **head,
		    int *maxdevice);
		int maxdevice;

		if (read_mcf(McfFileName, &devhead, &maxdevice) == 0) {
			SendCustMsg(HERE, 0, "Errors in mcf file %s",
			    McfFileName);
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * Find all devices that have catalogs.
	 */
	LastMid = 1;
	for (dev = devhead; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		struct CatalogHdr *ch;
		struct stat st;
		enum CH_type ch_type;
		upath_t	cf_name;
		size_t size;	/* Size of memory mapped catalog file */
		void *mp;
		int ne;
		int version;

		if (!(dev->equ_type == DT_HISTORIAN) &&
		    !(IS_ROBOT(dev) || DT_PSEUDO_SC == dev->equ_type) &&
		    !(0 == dev->fseq))	/* Manual drive */
			continue;

		if (dev->equ_type == DT_HISTORIAN) {
			ch_type = CH_historian;
		} else if (dev->fseq != 0) {
			ch_type = CH_library;
		} else {
			ch_type = CH_manual;
		}

		/*
		 * Set catalog name.
		 * Use full path if given, otherwise the catalog is in our
		 * directory.
		 * If no name given, invent it from equipment identification.
		 */
		if (*dev->dt.rb.name == '/') {
			strncpy(cf_name, dev->dt.rb.name, sizeof (cf_name)-1);
		} else if (*dev->dt.rb.name != '\0') {
			sprintf(cf_name, "%s/%s",  CatalogDir, dev->dt.rb.name);
		} else {
			if (dev->fseq != 0) {
				if (*dev->set != '\0')
					sprintf(cf_name, "%s/%s", CatalogDir,
					    dev->set);
				else
					sprintf(cf_name, "%s/hist%d",
					    CatalogDir, dev->eq);
			} else {
				sprintf(cf_name, "%s/man%d", CatalogDir,
				    dev->eq);
			}
		}

		/*
		 * Check the catalog.
		 */
		if (stat(cf_name, &st) == -1 && ENOENT == errno) {
			/*
			 * No catalog file.
			 * Make an empty one.
			 */
			MakeEmptyCatalog(cf_name, ch_type, &mp, &size);
		} else {
			/*
			 * Mmap the catalog file.
			 */
			version = CatalogMmapCatfile(cf_name, O_RDWR,
			    &mp, &size, MsgFunc);
			if (version == -1) {
				SendCustMsg(HERE, 18001, cf_name, ErrMsg);
				mp = NULL;
			} else if (version != CF_VERSION) {
				if (CvrtCatalog(cf_name, version,
				    &mp, &size) == -1) {
					if (munmap(mp, size) == -1) {
						SysError(HERE,
						    "Cannot munmap %s",
						    cf_name);
					}
					mp = NULL;
				}
			}
		}
		if (mp == NULL) {
			char bak_name[256];

			/*
			 * Something is seriously wrong - we don't have
			 * a catalog yet.  * Backup the one we have and
			 * make an empty one.
			 */
			if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
				(void) snprintf(bak_name, sizeof (bak_name),
				    "%s.bad", cf_name);
				if (rename(cf_name, bak_name) == -1) {
					SysError(HERE, "Cannot rename %s to %s",
					    cf_name, bak_name);
				}
				SendCustMsg(HERE, 18006, cf_name, bak_name);
				MakeEmptyCatalog(cf_name, ch_type, &mp, &size);
			}
			if (mp == NULL) {

				/*
				 * Still no success.
				 */
				SysError(HERE,
				    "Exiting - Cannot create catalog %s",
				    cf_name);
				exit(EXIT_NORESTART);
			}
		}

		/*
		 * Add space for this catalog file to the catalog table.
		 */
		strncpy(CatalogTable->CtFname[CatalogTable->CtNumofFiles],
		    cf_name, sizeof (CatalogTable->CtFname[0]));
		CatalogTable->CtNumofFiles++;
		CtSize += sizeof (CatalogTable->CtFname[0]);
		ch = (struct CatalogHdr *)mp;
		if (ch->ChEq != dev->eq) {

			/*
			 * Newly made or converted catalog, or eq changed.
			 */
			ch->ChEq = dev->eq;
			ch->ChType = ch_type;
			for (ne = 0; ne < ch->ChNumofEntries; ne++) {
				ch->ChTable[ne].CeEq = ch->ChEq;
			}
		}

		if (IsDaemon && dev->fseq != 0) {
			/*
			 * Enter the catalog in the device table.
			 */
			memmove(dev->dt.rb.name, cf_name, sizeof (upath_t));
		}
		Trace(TR_MISC, "Adding catalog %s, %d VSNs", cf_name,
		    ch->ChNumofEntries);
		LastMid += ch->ChNumofEntries;
		SamRealloc(CatalogTable, CtSize);
		if (munmap(mp, size) == -1) {
			SysError(HERE, "Cannot munmap %s", cf_name);
		}
	}

	if (CatalogTable->CtNumofFiles == 0) {
		SendCustMsg(HERE, 18002);
	}

	/*
	 * Write the catalog table file.
	 */
	CtSize = STRUCT_RND(CtSize);
	SamRealloc(CatalogTable, CtSize);
	CatalogTable->Ct.MfLen = CtSize;
	CatalogTable->CtLastMid = LastMid;
	fd = open(CatalogTableName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		SysError(HERE, "Catalog table %s: cannot create",
		    CatalogTableName);
		return;
	}
	if (write(fd, CatalogTable, CtSize) != CtSize) {
		SysError(HERE, "Catalog table %s: cannot write",
		    CatalogTableName);
		return;
	}
	if (close(fd) == -1) {
		SysError(HERE, "Cannot close catalog table %s",
		    CatalogTableName);
		return;
	}
	SamFree(CatalogTable);
	Trace(TR_MISC, "Made catalog table %s (%d bytes, %d entries)",
	    CatalogTableName, CatalogTable->Ct.MfLen, CatalogTable->CtLastMid);

	/*
	 * Map the table and catalogs.
	 */
	nc = CatalogAccess(CatalogTableName, MsgFunc);
	if (nc == -1) {
		SysError(HERE, "Catalog table %s: Cannot mmap",
		    CatalogTableName);
		return;
	}

	/*
	 * Find the historian.
	 */
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;

		ch = Catalogs[nc].CmHdr;
		if (ch->ChType != CH_historian)  continue;
		Historian = nc;
		break;
	}

	/*
	 * Enter mid in catalog entries.
	 */
	LastMid = 1;
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;
		int		ne;

		ch = Catalogs[nc].CmHdr;

		for (ne = 0; ne < ch->ChNumofEntries; ne++) {
			struct CatalogEntry *ce;

			ce = &ch->ChTable[ne];
			ce->CeMid = LastMid++;
		}
	}

	/*
	 * Check all vsns in each catalog for duplicates.
	 */
	num_dups = SortAndCheckForDuplicateVsns(NULL, NULL, NULL, TRUE, TRUE);

	if (num_dups != 0) {
		Trace(TR_MISC, "%d duplicate entries", num_dups);
	}
	shmdt(master_shm.shared_memory);
}


/*
 * Make an empty catalog.
 */
static void
MakeEmptyCatalog(
	char *cf_name,			/* Catalog file name */
	enum CH_type ch_type,		/* Catalog type */
	void **mp,			/* Mmaped catalog */
	size_t *size)			/* Size of mmaped file */
{
	int entries;

	if (ch_type != CH_manual)  entries = CATALOG_TABLE_INCR;
	else  entries = 1;
	if (CatalogCreateCatfile(cf_name, entries, mp, size, MsgFunc) == -1) {
		SendCustMsg(HERE, 18004, cf_name);
		return;
	}
	SendCustMsg(HERE, 18003, cf_name);
}


/*
 * Attach SAM-FS shared memory segment.
 */
static void *
ShmatSamfs(
	int	mode)	/* O_RDONLY = read only, read/write otherwise */
{
	/*
	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		return (NULL);
	}
	mode = (O_RDONLY == mode) ? SHM_RDONLY : 0;
	master_shm.shared_memory = shmat(master_shm.shmid, NULL, mode);
	if (master_shm.shared_memory == (void *)-1) {
		return (NULL);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
		errno = ENOCSI;
		return (NULL);
	}
	return (shm_ptr_tbl);
}
