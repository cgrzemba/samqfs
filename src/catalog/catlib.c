/*
 * catlib.c - provide access to the catalog files and table.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.49 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM-FS headers. */
#include "pub/sam_errno.h"
#include "sam/types.h"
#include "sam/exit.h"
#include "sam/custmsg.h"
#include "aml/device.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/udscom.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "catlib_priv.h"
#include "catserver.h"
#include "catalog331.h"

#if defined(lint)
#include "sam/lint.h"
#undef unlink
#endif /* defined(lint) */

/* Private functions. */
static int Cat331Validate(void *mp, size_t len,
	void (*MsgFunc)(int code, char *msg));
static void ClientLogit(char *msg);
static void ClientMsgFunc(int code, char *msg);
static void *MapFile(char *FileName, int mode, size_t *size,
	void (*MsgFunc)(int code, char *msg));
static void UnmapCatalogs(void);
#if !defined(CAT_SERVER)
static int FindCatalog(int eq);
static int CatalogAccess(char *TableName, void (*MsgFunc)(int code, char *msg));
#endif /* defined(CAT_SERVER) */

/* Private data. */
static char ClientName[16];
static struct UdsClient clnt = { SERVER_NAME, ClientName, SERVER_MAGIC,
	ClientLogit };
static struct CatalogEntry *LastCe = NULL;
static pthread_mutex_t LastCeMutex = PTHREAD_MUTEX_INITIALIZER;
#define	LastCeLock() (void) pthread_mutex_lock(&LastCeMutex);
#define	LastCeUnlock() (void) pthread_mutex_unlock(&LastCeMutex);

/* The tables are local to catlib when in the library. */
static struct CatalogTableHdr nullCatalogTable = { { 0, 0, 0 }, 0 };
struct CatalogMap *Catalogs;
#if !defined(CAT_SERVER)
struct CatalogTableHdr *CatalogTable = &nullCatalogTable;
#endif /* !defined(CAT_SERVER) */


/*
 * Assign free slot in catalog.
 */
int
_CatalogAssignFreeSlot(
	const char *SrcFile,
	const int SrcLine,
	int eq)
{
	struct CsrAssignFreeSlot arg;
	struct CsrGeneralRsp rsp;
	int status;

	arg.AsEq = (uint16_t)eq;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_AssignFreeSlot,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (rsp.GrStatus);
}


/*
 * Export entry from catalog.
 */
int
_CatalogExport(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vid)
{
	struct CsrExport arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SeVid, vid, sizeof (arg.SeVid));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_Export,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Reset all partitions
 */
int
_CatalogFormatPartitions(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vid,
	uint32_t CatStatus,
	int NumParts)
{
	struct CsrFormatPartitions arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.FrVid, vid, sizeof (arg.FrVid));
	arg.FrStatus	= CatStatus;
	arg.FrNumParts	= NumParts;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_FormatPartitions,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Validate a cartridge specification.
 */
struct CatalogEntry *
CatalogCheckSlot(
	struct VolId *vid,
	struct CatalogEntry *ced)
{
	if (vid->ViFlags == VI_logical) {
		return (CatalogGetCeByMedia(vid->ViMtype, vid->ViVsn, ced));
	} else {
		struct CatalogEntry *ce;
		int part;

		if (vid->ViFlags & VI_part)  part = vid->ViPart;
		else  part = 0;
		ce = CatalogGetCeByLoc(vid->ViEq, vid->ViSlot, part, ced);
		if (ce == NULL) {
			return (NULL);
		}

		/*
		 * Assure that a required slot was used.
		 */
		if (!(vid->ViFlags & VI_slot)) {
			struct CatalogHdr *ch;

			ch = CatalogGetHeader(vid->ViEq);
			if (ch == NULL) {
				return (NULL);
			}
			if (ch->ChType != CH_manual) {
				SetErrno = ER_SLOT_REQUIRED;
				return (NULL);
			}
		}
		return (ced);
	}
}


/*
 * Validate a VolId specification.
 */
struct CatalogEntry *
CatalogCheckVolId(
	struct VolId *vid,
	struct CatalogEntry *ced)
{
	if (vid->ViFlags == VI_logical) {
		return (CatalogGetCeByMedia(vid->ViMtype, vid->ViVsn, ced));
	} else {
		struct CatalogEntry *ce;

		int		part;

		if (vid->ViFlags & VI_part) {
			part = vid->ViPart;
		} else {
			part = 0;
		}
		ce = CatalogGetCeByLoc(vid->ViEq, vid->ViSlot, part, ced);
		if (ce == NULL) {
			return (NULL);
		}

		/*
		 * Assure that a required slot was used.
		 */
		if (!(vid->ViFlags & VI_slot)) {
			struct CatalogHdr *ch;

			ch = CatalogGetHeader(vid->ViEq);
			if (ch == NULL) {
				return (NULL);
			}
			if (ch->ChType != CH_manual) {
				SetErrno = ER_SLOT_REQUIRED;
				return (NULL);
			}
		}
		/*
		 * Assure that a required partition was used.
		 */
		if (ce->CePart != 0 && (vid->ViFlags & VI_part) == 0) {
			errno = ER_PARTITION_REQUIRED;
			return (NULL);
		}
		return (ced);
	}
}


/*
 * Return a catalog header given a catalog file name.
 *
 */
struct CatalogHdr *
CatalogGetCatalogHeader(
	const char *path)
{
	int nc;

	if (path == NULL) {
		errno = EINVAL;
		return (NULL);
	}

	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		/* look for CatalogMap for filename specified by user */
		if (strncmp(path, Catalogs[nc].CmFname,
		    sizeof (upath_t)) == 0) {
			/* match on pathname */
			break;
		}
	}

	if (nc >= CatalogTable->CtNumofFiles) {
		/* no match */
		errno = ENOENT;
		return (NULL);
	}

	return (Catalogs[nc].CmHdr);
}


/*
 * Get a pointer to a block of catalog entries; returns number
 * of entries.
 */
int				/* Number of entries available, -1 if error */
CatalogGetEntries(
	int eq,			/* Library equipment number */
	int start,		/* Starting entry number */
	int end,		/* Ending entry number */
	struct CatalogEntry **cep) /* Catalog entry pointer of starting entry */
{
	int nc;
	struct CatalogHdr *chp;

	if (start < 0 || start > end || eq < 0) {
		errno = EINVAL;
		return (-1);
	}

	for (nc = 0; Catalogs[nc].CmEq != eq; nc++) {
		if (nc >= CatalogTable->CtNumofFiles) {
			/* no entry found for this eq */
			errno = ENOENT;
			return (-1);
		}
	}

	chp = Catalogs[nc].CmHdr;
	if (start >= chp->ChNumofEntries) {
		return (0);
	}
	if (end > chp->ChNumofEntries) {
		end = chp->ChNumofEntries;
	}
	*cep = (struct CatalogEntry *)&chp->ChTable[start];
	return (end - start);
}


/*
 * Return catalog entry from equipment number, slot, and partition.
 */
struct CatalogEntry *
CatalogGetCeByLoc(
	int eq,
	int slot,
	int pt,
	struct CatalogEntry *ced)
{
	struct CatalogEntry *ce;

	ce = CS_CatalogGetCeByLoc(eq, slot, pt);
	if (ce == NULL) {
		return (NULL);
	}
	LastCeLock();
	LastCe = ce;
	memmove(ced, LastCe, sizeof (struct CatalogEntry));
	LastCeUnlock();
	return (ced);
}


/*
 * Return catalog entry from vsn and media.
 */
struct CatalogEntry *
CatalogGetCeByMedia(
	char *media_type,
	vsn_t vsn,
	struct CatalogEntry *ced)
{
	struct CatalogEntry *ce;

	if (*media_type == '\0' || *vsn == '\0') {
		return (NULL);
	}

	ce = CS_CatalogGetCeByMedia(media_type, vsn);
	if (ce == NULL) {
		return (NULL);
	}
	LastCeLock();
	LastCe = ce;
	memmove(ced, LastCe, sizeof (struct CatalogEntry));
	LastCeUnlock();
	return (ced);
}


/*
 * Return a usable cleaning volume
 */
struct CatalogEntry *
CatalogGetCleaningVolume(
	int eq,
	struct CatalogEntry *ced)
{
	struct CatalogHdr *ch;
	int		nc;
	int		ne;

	/*
	 * Find catalog that matches equipment.
	 */
	if ((nc = FindCatalog(eq)) == -1) {
		return (NULL);
	}
	ch = Catalogs[nc].CmHdr;

	/*
	 * Find a usable cleaning tape.
	 */
	for (ne = 0; ne < ch->ChNumofEntries; ne++) {
		struct CatalogEntry *ce;

		ce = &ch->ChTable[ne];
		if ((ce->CeStatus & CES_cleaning) &&
		    (ce->CeStatus & CES_occupied) &&
		    (ce->CeAccess > 0) &&
		    (ce->CeStatus & CES_bad_media) == 0) {
			LastCeLock();
			LastCe = ce;
			memmove(ced, LastCe, sizeof (struct CatalogEntry));
			LastCeUnlock();
			return (ced);
		}
	}
	return (NULL);
}


/*
 * Return catalog entry from VolId structure.
 */
struct CatalogEntry *
CatalogGetEntry(
	struct VolId *vid,
	struct CatalogEntry *ced)
{
	if (vid->ViFlags == VI_logical) {
		return (CatalogGetCeByMedia(vid->ViMtype, vid->ViVsn, ced));
	} else {
		struct CatalogEntry *ce;
		int part;

		if (vid->ViFlags == VI_cart) {
			part = 0;
		} else {
			part = vid->ViPart;
		}
		ce = CatalogGetCeByLoc(vid->ViEq, vid->ViSlot, part, ced);
		if (ce == NULL) {
			return (NULL);
		}

		/*
		 * Assure that a required slot was used.
		 */
		if (!(vid->ViFlags & VI_slot)) {
			struct CatalogHdr *ch;

			ch = CatalogGetHeader(vid->ViEq);
			if (ch == NULL) {
				return (NULL);
			}
			if (ch->ChType != CH_manual) {
				SetErrno = ER_SLOT_REQUIRED;
				return (NULL);
			}
		}
		return (ced);
	}
}


/*
 * Return catalog entry from barcode
 */
struct CatalogEntry *
CatalogGetCeByBarCode(
	int eq,
	char *media_type,
	char *barcode,
	struct CatalogEntry *ced)
{
	struct CatalogHdr *ch;
	struct CatalogEntry *ce;
	int	nc;
	int	ne;

	if (*barcode == '\0') {
		return (NULL);
	}

	LastCeLock();
	if (LastCe != NULL &&
	    (strcmp(LastCe->CeBarCode, barcode) == 0) &&
	    (strcmp(LastCe->CeMtype, media_type) == 0) &&
	    (eq == LastCe->CeEq)) {
		ce = LastCe;
		memmove(ced, LastCe, sizeof (struct CatalogEntry));
		LastCeUnlock();
		return (ced);
	}
	LastCeUnlock();

	/*
	 * Find catalog that matches equipment
	 */
	if ((nc = FindCatalog(eq)) == -1) {
		return (NULL);
	}
	ch = Catalogs[nc].CmHdr;

	/*
	 * Find the first entry with matching barcode
	 */
	for (ne = 0; ne < ch->ChNumofEntries; ne++) {

		ce = &ch->ChTable[ne];
		if ((strcmp(ce->CeBarCode, barcode) == 0) &&
		    (strcmp(ce->CeMtype, media_type) == 0)) {

			LastCeLock();
			LastCe = ce;
			memmove(ced, LastCe, sizeof (struct CatalogEntry));
			LastCeUnlock();
			return (ced);
		}
	}
	return (NULL);
}


/*
 * Return catalog header.
 */
struct CatalogHdr *
CatalogGetHeader(
	int eq)
{
	int		nc;

	if ((nc = FindCatalog(eq)) == -1) {
		return (NULL);
	}
	return (Catalogs[nc].CmHdr);
}

/*
 * Returns all catalog entries for specified library.
 * This function allocates memory (via malloc) for returning
 * inuse catalog entries.  IMPORTANT:  The caller of this function
 * is responsible for freeing this memory.  Function also returns
 * number of inuse entries in the catalog.
 */

struct CatalogEntry *
CatalogGetEntriesByLibrary(
	int eq,			/* library equipment number */
	int *numOfEntries	/* number of catalog entries in library */
)
{
	int i, j;
	struct CatalogHdr *ch;
	struct CatalogEntry *ce;
	int numInuse;

	ce = NULL;
	*numOfEntries = 0;

	ch  = CatalogGetHeader(eq);

	LastCeLock();

	/*
	 *	Count inuse entries.
	 */
	numInuse = 0;
	if (ch != NULL && ch->ChNumofEntries > 0) {
		for (i = 0; i < ch->ChNumofEntries; i++) {
			if (ch->ChTable[i].CeStatus & CES_inuse) {
				numInuse++;
			}
		}
	}

	if (numInuse > 0) {
		ce = (struct CatalogEntry *)malloc(numInuse *
		    sizeof (struct CatalogEntry));
		if (ce == NULL) {
			numInuse = 0;
		}
	}

	if (numInuse > 0) {
		j = 0;
		for (i = 0; i < ch->ChNumofEntries; i++) {
			if (ch->ChTable[i].CeStatus & CES_inuse) {
				memmove(&ce[j], &ch->ChTable[i],
				    sizeof (struct CatalogEntry));
				j++;
			}
		}
	}
	LastCeUnlock();

	*numOfEntries = numInuse;
	return (ce);
}


/*
 * Initialize catalog processing.
 */
int				/* 0 if success, -1 if failure */
_CatalogInit(
	const char *SrcFile,
	const int SrcLine,
	char *client_name)
{
	struct CsrGetInfoRsp rsp;
	int status;

	if (CatalogTable->CtNumofFiles != 0 && CatalogSync() == 0) {
		return (0);
	}
	TraceInit(client_name, TI_catserver);
	strncpy(ClientName, client_name, sizeof (ClientName)-1);
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_GetInfo,
	    NULL, 0, &rsp, sizeof (rsp));
	if (status != 0) {
		return (-1);
	}
	status = CatalogAccess(rsp.CatTableName, ClientMsgFunc);
	if ((status == 0) && (strcmp(client_name, "rmtserver") == 0)) {
		status = UdsSendMsg(SrcFile, SrcLine, &clnt,
		    CSR_SetRemoteServer,
		    NULL, 0, &rsp, sizeof (struct CsrGeneralRsp));
	}
	return (status);
}


/*
 * Complete a label operation.
 */
int
_CatalogLabelComplete(
	const char *SrcFile,
	const int SrcLine,
	struct CatalogEntry *ce)
{
	struct CsrLabelComplete arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.LcCe, ce, sizeof (arg.LcCe));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_LabelComplete,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Label operation failed.
 */
int
_CatalogLabelFailed(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	char *new_vsn)
{
	struct	CsrLabelFailed arg;
	struct	CsrGeneralRsp rsp;
	int status;

	memmove(&arg.LfVid, vi, sizeof (arg.LfVid));
	memmove(arg.LfNewVsn, new_vsn, sizeof (arg.LfNewVsn));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_LabelFailed,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Begin a label operation.
 */
int
_CatalogLabelVolume(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	char *new_vsn)
{
	struct CsrLabelVolume arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.LvVid, vi, sizeof (arg.LvVid));
	memmove(arg.LvNewVsn, new_vsn, sizeof (arg.LvNewVsn));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_LabelVolume,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Export all catalog entries to the historian
 */
int
_CatalogLibraryExport(
	const char *SrcFile,
	const int SrcLine,
	int eq)
{
	struct CsrLibraryExport arg;
	struct CsrGeneralRsp rsp;
	int status;

	arg.LeEq    = (uint16_t)eq;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_LibraryExport,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * When the media is closed, update the space and ptocfwa/tape
 * position.
 */
int
_CatalogMediaClosed(
	const char *SrcFile,
	const int SrcLine,
	struct CatalogEntry *ce)
{
	struct CsrMediaClosed arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SfCe, ce, sizeof (arg.SfCe));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_MediaClosed,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Move a slot.
 */
int
_CatalogMoveSlot(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	int DestSlot)
{
	struct CsrMoveSlot arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.MsVid, vi, sizeof (arg.MsVid));
	arg.MsDestSlot = DestSlot;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_MoveSlot,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Reconcile the catalog
 */
int
_CatalogReconcileCatalog(
	const char *SrcFile,
	const int SrcLine,
	int	eq)
{
	struct CsrReconcileCatalog arg;
	struct CsrGeneralRsp rsp;
	int status;

	arg.RcEq = (uint16_t)eq;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_ReconcileCatalog,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Update a Remote SAM catalog entry.
 */
int
_CatalogRemoteSamUpdate(
	const char *SrcFile,
	const int SrcLine,
	struct CatalogEntry *ce,
	uint16_t flags)
{
	struct CsrRemoteSamUpdate arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.RsCe, ce, sizeof (arg.RsCe));
	arg.RsFlags = flags;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_RemoteSamUpdate,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Reserve a volume for archiving.
 */
int
_CatalogReserveVolume(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	time_t  rtime,				/* Time reservation made */
	char *asname,				/* Archive set */
	char *owner,				/* Owner */
	char *fsname)				/* File system */
{
	struct CsrReserveVolume arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.RvVid, vi, sizeof (arg.RvVid));
	if (rtime != 0) {
		arg.RvTime = rtime;
	} else {
		arg.RvTime = time(NULL);
	}
	strncpy(arg.RvAsname, asname, sizeof (arg.RvAsname));
	strncpy(arg.RvOwner, owner, sizeof (arg.RvOwner));
	strncpy(arg.RvFsname, fsname, sizeof (arg.RvFsname));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_ReserveVolume,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Set the audit flag
 */
int
_CatalogSetAudit(
	const char *SrcFile,
	const int SrcLine,
	int eq)
{
	struct CsrSetCleaning arg;
	struct CsrGeneralRsp rsp;
	int status;

	arg.ScEq = (uint16_t)eq;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_SetAudit,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Set the C flag for any cleaning tapes
 */
int
_CatalogSetCleaning(
	const char *SrcFile,
	const int SrcLine,
	int eq)
{
	struct CsrSetCleaning arg;
	struct CsrGeneralRsp rsp;
	int status;

	arg.ScEq = (uint16_t)eq;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_SetCleaning,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (rsp.GrStatus);
}


/*
 * Initialize a single slot
 */
int
_CatalogSlotInit(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	uint32_t CatStatus,
	int TwoSided,
	char *barcode,
	char *altbarcode)
{
	struct CsrSlotInit arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SiVid, vi, sizeof (arg.SiVid));
	memmove(arg.SiBarcode, barcode, sizeof (arg.SiBarcode));
	memmove(arg.SiAltBarcode, altbarcode, sizeof (arg.SiAltBarcode));
	arg.SiStatus   = CatStatus;
	arg.SiTwoSided = TwoSided;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_SlotInit,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Set fields using media type and vsn.
 */
int
_CatalogSetField(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	enum CeFields field,
	uint64_t value,
	uint32_t mask)
{
	struct CsrSetField arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SfVid, vi, sizeof (arg.SfVid));
	arg.SfField = field;
	arg.a.v.SfVal  = value;
	arg.a.v.SfMask = mask;
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_SetField,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status != 0) {
		return (status);
	}
	if (rsp.GrStatus != 0) {
		SetErrno = rsp.GrErrno;
	}
	return (rsp.GrStatus);
}


/*
 * Set fields using equipment number, slot and partition.
 */
int
_CatalogSetFieldByLoc(
	const char *SrcFile,
	const int SrcLine,
	int eq,
	int slot,
	int part,
	enum CeFields field,
	uint32_t val,
	uint32_t mask)
{
	struct VolId vid;

	vid.ViFlags = VI_cart;
	vid.ViEq   = (uint16_t)eq;
	vid.ViSlot = slot;
	vid.ViPart = (uint16_t)part;
	if (vid.ViPart != 0) {
		vid.ViFlags |= VI_part;
	}
	return (_CatalogSetField(SrcFile, SrcLine, &vid, field, val, mask));
}


/*
 * Set string fields.
 */
int
_CatalogSetString(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi,
	enum CeFields field,
	char *string)
{
	struct CsrSetField arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SfVid, vi, sizeof (arg.SfVid));
	arg.SfField = field;
	memmove(arg.a.SfString, string, sizeof (arg.a.SfString));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_SetField,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status != 0) {
		return (status);
	}
	if (rsp.GrStatus != 0) {
		SetErrno = rsp.GrErrno;
	}
	return (rsp.GrStatus);
}


/*
 * Set string fields using equipment number, slot and partition.
 */
int
_CatalogSetStringByLoc(
	const char *SrcFile,
	const int SrcLine,
	int eq,
	int slot,
	int part,
	enum CeFields field,
	char *string)
{
	struct VolId vid;
	int status;

	if (part == 0) {
		vid.ViFlags = VI_cart;
	} else {
		vid.ViFlags = VI_onepart;
	}
	vid.ViEq   = (uint16_t)eq;
	vid.ViSlot = slot;
	vid.ViPart = (uint16_t)part;
	status = _CatalogSetString(SrcFile, SrcLine, &vid, field, string);
	return (status);
}


/*
 * Return character string for catalog entry status.
 */
char *
CatalogStatusToStr(
	uint32_t status,		/* Status from catalog entry */
	char status_str[])		/* Mode string */
{
	status_str[0] =  status & CES_needs_audit ? 'A' : '-';
	status_str[1] =  status & CES_inuse ? 'i' : '-';
	status_str[2] =  status & CES_labeled ? 'l' :
	    (status & CES_non_sam ? 'N' : '-');
	status_str[3] =  status & CES_bad_media ? 'E' : '-';
	status_str[4] =  status & CES_occupied ? 'o' : '-';
	status_str[5] =  status & CES_cleaning ? 'C' :
	    (status & CES_priority ? 'p' : '-');
	status_str[6] =  status & CES_bar_code ? 'b' : '-';
	status_str[7] =  status & CES_writeprotect ? 'W' : '-';
	status_str[8] =  status & CES_read_only ? 'R' : '-';
	status_str[9] =  status & CES_recycle ? 'c' : '-';
	status_str[10] = status & CES_dupvsn ? 'd' :
	    (status & CES_unavail ? 'U' : '-');
	status_str[11] = status & CES_export_slot ? 'X' :
	    (status & CES_archfull ? 'f' : '-');
	status_str[12] = '\0';
	return (status_str);
}


/*
 * Convert catalog entry to a string.
 */
char *
CatalogStrFromEntry(
	struct CatalogEntry *ce,
	char *string,
	size_t size)
{
	char stbuf[16];
	char cpbuf[16];
	char spbuf[16];
	char bsbuf[16];

	(void) StrFromFsize((fsize_t)ce->CeCapacity * 1024, 1,
	    cpbuf, sizeof (cpbuf));
	(void) StrFromFsize((fsize_t)ce->CeSpace * 1024, 1,
	    spbuf, sizeof (spbuf));
	if (ce->CeBlockSize > 1000) {
		sprintf(bsbuf, "%4dk", ce->CeBlockSize/1024);
	} else {
		sprintf(bsbuf, "%4d ", ce->CeBlockSize);
	}

	snprintf(string, size,
	    "%4d:%d %2s %-24s %s %s %s %s %9d %d %d  %d %s/%s/%s",
	    ce->CeSlot,
	    ce->CePart,
	    ce->CeMtype,
	    (*ce->CeVsn != '\0') ? ce->CeVsn : "?",
	    CatalogStatusToStr(ce->CeStatus, stbuf),
	    cpbuf,
	    spbuf,
	    bsbuf,
	    ce->CeLabelTime,
	    ce->CeEq,
	    ce->CeMid,
	    ce->r.CerTime,
	    ce->r.CerAsname,
	    ce->r.CerOwner,
	    ce->r.CerFsname);

	return (string);
}


/*
 * Synchronize with catalog files.
 * Check each catalog file for a change.
 * If one has changed, mmap the new one.
 * Void the pointer cache.
 */
int		/* 0 if no change, >0 if changed, -1 if error */
CatalogSync(void)
{
	int changed;
	int nc;

	if (CatalogTable->Ct.MfValid == 0) {
		CatalogTerm();
		return (1);
	}

	/*
	 * Check the catalog files.
	 */
	changed = 0;
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;

		ch = Catalogs[nc].CmHdr;
		if ((ch == NULL) || (ch->ChVersion != CF_VERSION)) {
			struct CatalogMap *cm;
			size_t	size;
			void	*mp;
			int		version;

			/*
			 * This catalog has changed.
			 * Clear the pointer cache.
			 */
			LastCeLock();
			changed++;
			LastCe = NULL;
			cm = &Catalogs[nc];
			version = CatalogMmapCatfile(cm->CmFname, O_RDONLY,
			    &mp, &size, NULL);
			if (version != CF_VERSION) {
				LibError(NULL, EXIT_FAILURE, 610,
				    cm->CmFname, cm->CmSize);
				LastCeUnlock();
				return (-1);
			}
			Trace(TR_MISC, "Catalog %s changed size, %d to %d",
			    cm->CmFname, cm->CmSize, size);
			if (munmap((char *)cm->CmHdr, cm->CmSize) == -1) {
				LibError(NULL, EXIT_FAILURE, 611, cm->CmFname);
				LastCeUnlock();
				return (-1);
			}
			cm->CmHdr  = mp;
			cm->CmSize = size;
			cm->CmEq   = cm->CmHdr->ChEq;
			LastCeUnlock();
		}
	}
	return (changed);
}


/*
 * Terminate catalog processing.
 */
void
CatalogTerm(void)
{
	LastCeLock();
	if (CatalogTable->CtNumofFiles != 0) {
		UnmapCatalogs();
	}
	LastCeUnlock();
}


/*
 * Unreserve a volume for archiving.
 */
int
_CatalogUnreserveVolume(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vi)
{
	struct CsrUnReserveVolume arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.UrVid, vi, sizeof (arg.UrVid));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_UnReserveVolume,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/*
 * Return VolId struct from catalog entry.
 */
struct VolId *
	CatalogVolIdFromCe(
	struct CatalogEntry *ce,	/* Catalog entry */
	struct VolId *vi)		/* Caller provided */
{
	memmove(vi->ViMtype, ce->CeMtype, sizeof (vi->ViMtype));
	memmove(vi->ViVsn, ce->CeVsn, sizeof (vi->ViVsn));
	vi->ViEq   = ce->CeEq;
	vi->ViSlot = ce->CeSlot;
	vi->ViPart = ce->CePart;
	if (*vi->ViMtype != 0 && *vi->ViVsn != '\0') {
		vi->ViFlags = VI_logical;
	}
	if (ce->CePart == 0) {
		vi->ViFlags |= VI_cart;
	} else {
		vi->ViFlags |= VI_onepart;
	}
	return (vi);
}


/*
 * When media is loaded, the drive will partially fill in a CatalogEntry
 * structure.
 *
 * The catalog manager will verify or change the actual catalog entry
 * as needed.
 */
int
_CatalogVolumeLoaded(
	const char *SrcFile,
	const int SrcLine,
	struct CatalogEntry *ce)
{
	struct CsrVolumeLoaded arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.VlCe, ce, sizeof (arg.VlCe));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_VolumeLoaded,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	(void) CatalogSync();
	return (0);
}


/*
 * Unloading of a volume requires setting the occupied bit
 * If media and vsn are zero, use the bar code
 * Add the vsn to the catalog if not found
 */
int
_CatalogVolumeUnloaded(
	const char *SrcFile,
	const int SrcLine,
	struct VolId *vid,
	char *barcode)
{
	struct CsrVolumeUnloaded arg;
	struct CsrGeneralRsp rsp;
	int status;

	memmove(&arg.SfVid, vid, sizeof (arg.SfVid));
	memmove(arg.SfBarcode, barcode, sizeof (arg.SfBarcode));
	status = UdsSendMsg(SrcFile, SrcLine, &clnt, CSR_VolumeUnloaded,
	    &arg, sizeof (arg), &rsp, sizeof (rsp));
	if (status == -1) {
		return (-1);
	}
	if (rsp.GrStatus == -1) {
		SetErrno = rsp.GrErrno;
		return (-1);
	}
	return (0);
}


/* Private catalog functions. */


/*
 * Find catalog.
 */
#if !defined(CAT_SERVER)
static
#endif /* defined(CAT_SERVER) */
int		/* Catalog number.  -1 if not found */
FindCatalog(
	int eq)
{
	int nc;

	/*
	 * Find catalog that matches equipment.
	 */
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;

		ch = Catalogs[nc].CmHdr;
		if (ch->ChEq == eq) {
			return (nc);
		}
	}
	SetErrno = ER_ROBOT_CATALOG_MISSING;
	return (-1);
}


/*
 * Return catalog entry given mid.
 */
struct CatalogEntry *
_Cl_IfGetCeByMid(
	int mid,
	struct CatalogEntry *ced)
{
	int nc;
	struct CatalogEntry *ce;

	LastCeLock();
	if (LastCe != NULL && mid == LastCe->CeMid)  {
		ce = LastCe;
		memmove(ced, LastCe, sizeof (struct CatalogEntry));
		LastCeUnlock();
		return (ced);
	}
	LastCeUnlock();

	/*
	 * Look at all catalogs.
	 */
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;
		int ne;

		ch = Catalogs[nc].CmHdr;

		/*
		 * Find first entry with matching mid.
		 */
		for (ne = 0; ne < ch->ChNumofEntries; ne++) {

			ce = &ch->ChTable[ne];
			if (mid == ce->CeMid) {
				LastCeLock();
				LastCe = ce;
				memmove(ced, LastCe,
				    sizeof (struct CatalogEntry));
				LastCeUnlock();
				return (ced);
			}
		}
	}
	return (NULL);
}


/* Catalog file manipulation functions.	*/


/*
 * Validate a 3.3.1 catalog.
 */
int
Cat331Validate(
	void *mp,
	size_t len,
	void (*MsgFunc)(int code, char *msg))
{
	catalog_tbl_t *ct3;

	ct3 = (catalog_tbl_t *)mp;
	if (ct3->version != CATALOG_CURRENT_VERSION) {
		LibError(MsgFunc, EXIT_FAILURE, 18450, CATALOG_CURRENT_VERSION,
		    ct3->version);
		return (-1);
	}

	/*
	 * Test the time stamp for a reasonable value.
	 */
	if (ct3->audit_time != 0) {
		if (ct3->audit_time < 0 || ct3->audit_time >= time(NULL)) {
			LibError(MsgFunc, EXIT_FAILURE, 18201);
			return (-1);
		}
	}

	if (ct3->media != 0 && strcmp(sam_mediatoa(ct3->media), "??") == 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18202, ct3->media);
		return (-1);
	}

	/*
	 * The catalog header table count field MUST match the file size, or we
	 * really have a suspect file here.
	 */
	if (ct3->count < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18203);
		return (-1);
	} else if (len != sizeof (catalog_tbl_t) +
	    (ct3->count * sizeof (cat_ent_t))) {

		int count;

		count = (len - sizeof (catalog_tbl_t)) / sizeof (cat_ent_t);
		LibError(MsgFunc, EXIT_FAILURE, 18204, ct3->count, count);
		return (-1);
	}
	return (331);
}


/*
 * Access the catalog files.
 * mmap() the catalog table and catalog files.
 * Dev_note:  The error handling should be tidied up.
 * Dev_note:  The catalog table should be removed and its function
 *            performed in Catalogs.
 */
#if !defined(CAT_SERVER)
static
#endif /* defined(CAT_SERVER) */
int			/* 0 if success, -1 if failure */
CatalogAccess(
	char *TableName,	/* Name of the Catalog table file */
	void (*MsgFunc)(int code, char *msg))	/* Error handler to call */
{
	size_t	size;
	void	*mp;
	int	nc;

	/*
	 * Map the catalog table.
	 */
#if !defined(CAT_SERVER)
	mp = MapFile(TableName, O_RDONLY, &size, MsgFunc);
#else /* !defined(CAT_SERVER) */
	mp = MapFile(TableName, O_RDWR, &size, MsgFunc);
#endif /* !defined(CAT_SERVER) */
	if (mp == NULL) {
		return (-1);
	}
	CatalogTable = mp;
	Catalogs = NULL;
	if (CatalogTable->Ct.MfMagic != CT_MAGIC ||
	    CatalogTable->Ct.MfLen != size ||
	    CatalogTable->Ct.MfValid == 0 ||
	    CatalogTable->CtTime > time(NULL)) {
		goto error;
	}

	/*
	 * Allocate the catalog maps.
	 * Map the catalog files.
	 */
	Catalogs = malloc(CatalogTable->CtNumofFiles *
	    sizeof (struct CatalogMap));
	if (Catalogs == NULL) {
		goto error;
	}
	memset(Catalogs, 0, CatalogTable->CtNumofFiles *
	    sizeof (struct CatalogMap));

	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogMap *cm;
		int	version;

		cm = &Catalogs[nc];
		strncpy(cm->CmFname, CatalogTable->CtFname[nc],
		    sizeof (cm->CmFname));
		version = CatalogMmapCatfile(cm->CmFname, O_RDONLY, &mp, &size,
		    MsgFunc);
		if (version != CF_VERSION) {
			goto error;
		}
		cm->CmHdr  = mp;
		cm->CmSize = size;
		cm->CmEq   = cm->CmHdr->ChEq;
	}
	return (0);

error:
	return (-1);
}


/*
 * Create an empty memory image of a catalog file.
 * Returns start of image and size.
 */
int						/* -1 if error */
CatalogCreateCatfile(
	char *FileName,				/* Catalog file name */
	int NumofEntries,			/* Number of entries */
	void **mp,				/* Start of image */
	size_t *size,				/* Size of image created */
	void (*MsgFunc)(int code, char *msg))	/* Error handler to call */
{
	struct CatalogEntry ceblank;
	struct CatalogHdr chblank;
	int	fd;
	int	n;

	/*
	 * Unlink the file because the file by this name may be mmpap-ed.
	 */
	if (unlink(FileName) == -1 && errno != ENOENT) {
		LibError(MsgFunc, 0, 617, FileName);
	}
	fd = open(FileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		LibError(MsgFunc, EXIT_FATAL, 574, FileName);
		return (-1);
	}
	*size = sizeof (struct CatalogHdr) +
	    NumofEntries * sizeof (struct CatalogEntry);

	/*
	 * Write the file header.
	 */
	memset(&chblank, 0, sizeof (chblank));
	chblank.ChMagic = CF_MAGIC;
	chblank.ChVersion = CF_VERSION;
	strncpy(chblank.ChFname, FileName, sizeof (chblank.ChFname));
	chblank.ChNumofEntries = NumofEntries;
	if (write(fd, &chblank, sizeof (chblank)) != sizeof (chblank)) {
		LibError(MsgFunc, EXIT_FATAL, 615, FileName);
		(void) close(fd);
		return (-1);
	}

	/*
	 * Write blank entries.
	 */
	memset(&ceblank, 0, sizeof (ceblank));
	for (n = 0; n < NumofEntries; n++) {
		if (write(fd, &ceblank,
		    sizeof (ceblank)) != sizeof (ceblank)) {
			LibError(MsgFunc, EXIT_FATAL, 615, FileName);
			(void) close(fd);
			return (-1);
		}
	}
	if (close(fd) == -1) {
		LibError(MsgFunc, EXIT_FATAL, 615, FileName);
		return (-1);
	}

	/*
	 * Memory map the file.
	 */
	*mp = MapFile(FileName, O_RDWR, size, MsgFunc);
	if (NULL == *mp) {
		return (-1);
	}
	return (0);
}


/*
 * Mmap a catalog file.
 * Validate and return version number of catalog.
 * Caller can provide an error handler.
 */
int				/* Version number of catalog -1 if error */
CatalogMmapCatfile(
	char *FileName,
	int mode,		/* O_RDONLY = read only, read/write otherwise */
	void **mp,		/* Start of a catalog header */
	size_t *size,		/* Return mapped size */
	void (*MsgFunc)(int code, char *msg))	/* Error handler to call */
{
	struct CatalogHdr *ch;

#if !defined(CAT_SERVER)
	mode = O_RDONLY;
#else /* defined(CAT_SERVER) */
	mode = O_RDWR;
#endif /* defined(CAT_SERVER) */
	*mp = MapFile(FileName, mode, size, MsgFunc);
	if (NULL == *mp) {
		return (-1);
	}

	/*
	 * Validate catalog.
	 * Assume a 3.5 or later catalog.
	 */
	ch = (struct CatalogHdr *)*mp;
	SetErrno = 0;
	if (ch->ChMagic != CF_MAGIC) {

		/*
		 * Not a 3.5 catalog, try 3.3.1.
		 */
		return (Cat331Validate(*mp, *size, MsgFunc));
	}

	/*
	 * Check version.
	 */
	if (ch->ChVersion != CF_VERSION) {
		LibError(MsgFunc, EXIT_FAILURE, 18202,
		    ch->ChVersion, CF_VERSION);
		return (ch->ChVersion);
	}

	/*
	 * Test the time stamp for a reasonable value.
	 */
	if (ch->ChAuditTime != 0) {
		if (ch->ChAuditTime < 0 || ch->ChAuditTime > time(NULL)) {
			LibError(MsgFunc, EXIT_FAILURE, 18203);
			return (-1);
		}
	}

	if (*ch->ChMediaType != '\0' && strcmp(ch->ChMediaType, "??") == 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18204, ch->ChMediaType);
		return (-1);
	}

	/*
	 * The catalog header table count field MUST match the file size, or we
	 * really have a suspect file here.
	 */
	if (ch->ChNumofEntries < 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18205);
		return (-1);
	} else if (*size != sizeof (struct CatalogHdr) +
	    (ch->ChNumofEntries * sizeof (struct CatalogEntry))) {

		int count;

		count = (*size - sizeof (struct CatalogHdr)) /
		    sizeof (struct CatalogEntry);
		LibError(MsgFunc, EXIT_FAILURE, 18206,
		    ch->ChNumofEntries, count);
		return (-1);
	}
	return (ch->ChVersion);
}


/*
 * Return catalog entry from equipment number, slot, and partition.
 */
struct CatalogEntry *
CS_CatalogGetCeByLoc(
	int eq,
	int slot,
	int pt)
{
	struct CatalogHdr *ch;
	struct CatalogEntry *ce;
	int nc;
	int ne;
	boolean_t slot_found;

	LastCeLock();
	if (LastCe != NULL && (LastCe->CeStatus & CES_inuse) &&
	    eq == LastCe->CeEq && slot == LastCe->CeSlot &&
	    (0 == pt || pt == LastCe->CePart)) {
		ce = LastCe;
		LastCeUnlock();
		return (ce);
	}
	LastCeUnlock();

	/*
	 * Find catalog that matches equipment.
	 */
	if ((nc = FindCatalog(eq)) == -1) {
		return (NULL);
	}
	ch = Catalogs[nc].CmHdr;

	/*
	 * Find first entry with matching slot.
	 * And matching partition if set.
	 */
	slot_found = FALSE;
	for (ne = 0; ne < ch->ChNumofEntries; ne++) {

		ce = &ch->ChTable[ne];
		if ((ce->CeStatus & CES_inuse) && slot == ce->CeSlot) {
			slot_found = TRUE;
			if (0 == pt || pt == ce->CePart)  {
				LastCeLock();
				LastCe = ce;
				LastCeUnlock();
				return (ce);
			}
		}
	}
	if (!slot_found) {
		SetErrno = ER_NOT_VALID_SLOT_NUMBER;
	} else {
		SetErrno = ER_NOT_VALID_PARTITION;
	}
	return (NULL);
}


/*
 * Return catalog entry from vsn and media.
 */
struct CatalogEntry *
CS_CatalogGetCeByMedia(
	char *media_type,
	vsn_t vsn)
{
	struct CatalogEntry *ce;
	int nc;

	if (*media_type == '\0' || *vsn == '\0') {
		return (NULL);
	}

	LastCeLock();
	if (LastCe != NULL && (LastCe->CeStatus & CES_inuse) &&
	    strcmp(LastCe->CeMtype, media_type) == 0 &&
	    strcmp(LastCe->CeVsn, vsn) == 0) {
		ce = LastCe;
		LastCeUnlock();
		return (ce);
	}
	LastCeUnlock();

	/*
	 * Look at all catalogs.
	 */
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;
		int ne;

		ch = Catalogs[nc].CmHdr;

		/*
		 * Find first entry with matching VSN.
		 */
		for (ne = 0; ne < ch->ChNumofEntries; ne++) {

			ce = &ch->ChTable[ne];
			if ((ce->CeStatus & CES_inuse) &&
			    strcmp(ce->CeMtype, media_type) == 0 &&
			    strcmp(ce->CeVsn, vsn) == 0) {
				LastCeLock();
				LastCe = ce;
				LastCeUnlock();
				return (ce);
			}
		}
	}
	if (sam_atomedia(media_type) == 0) {
		SetErrno = ER_INVALID_MEDIA_TYPE;
	} else {
		SetErrno = ER_VOLUME_NOT_FOUND;
	}
	return (NULL);
}


/*
 * Return catalog entry from barcode
 */
struct CatalogEntry *
CS_CatalogGetCeByBarCode(
	int eq,
	char *media_type,
	char *barcode)
{
	struct CatalogHdr *ch;
	struct CatalogEntry *ce;
	int nc;
	int ne;

	if (*barcode == '\0') {
		return (NULL);
	}

	LastCeLock();
	if (LastCe != NULL &&
	    (strcmp(LastCe->CeBarCode, barcode) == 0) &&
	    (strcmp(LastCe->CeMtype, media_type) == 0) &&
	    (eq == LastCe->CeEq)) {
		ce = LastCe;
		LastCeUnlock();
		return (ce);
	}
	LastCeUnlock();

	/*
	 * Find catalog that matches equipment
	 */
	if ((nc = FindCatalog(eq)) == -1) {
		return (NULL);
	}
	ch = Catalogs[nc].CmHdr;

	/*
	 * Find the first entry with matching barcode
	 */
	for (ne = 0; ne < ch->ChNumofEntries; ne++) {

		ce = &ch->ChTable[ne];
		if ((strcmp(ce->CeBarCode, barcode) == 0) &&
		    (strcmp(ce->CeMtype, media_type) == 0)) {

			LastCeLock();
			LastCe = ce;
			LastCeUnlock();
			return (ce);
		}
	}
	return (NULL);
}


/*
 * Return catalog entry from VolId structure.
 */
struct CatalogEntry *
CS_CatalogGetEntry(
	struct VolId *vid)
{
	if (vid->ViFlags == VI_logical) {
		return (CS_CatalogGetCeByMedia(vid->ViMtype, vid->ViVsn));
	} else {
		struct CatalogEntry *ce;
		int part;

		if (vid->ViFlags == VI_cart) {
			part = 0;
		} else {
			part = vid->ViPart;
		}
		ce = CS_CatalogGetCeByLoc(vid->ViEq, vid->ViSlot, part);
		if (ce == NULL) {
			return (NULL);
		}

		/*
		 * Assure that a required slot was used.
		 */
		if (!(vid->ViFlags & VI_slot)) {
			struct CatalogHdr *ch;

			ch = CatalogGetHeader(vid->ViEq);
			if (ch == NULL) {
				return (NULL);
			}
			if (ch->ChType != CH_manual) {
				SetErrno = ER_SLOT_REQUIRED;
				return (NULL);
			}
		}
		return (ce);
	}
}


/* Private functions. */

/*
 * Log a communication error.
 */
static void
ClientLogit(
	char *msg)
{
	Trace(TR_MISC, "%s", msg);
}


/*
 * Log a library error.
 */
static void
ClientMsgFunc(
	int code,
	char *msg)
{
	Trace(TR_MISC, "Catalog file error %d %s", code, msg);
}


/*
 * Mapin a file.
 */
static void *			/* NULL if failed */
MapFile(
	char *FileName,
	int mode,		/* O_RDONLY = read only, read/write otherwise */
	size_t *size,		/* Return mapped size */
	void (*MsgFunc)(int code, char *msg))	/* Error handler to call */
{
	struct stat st;
	void	*mp;
	int	fd;
	int	prot;

	*size = 0;
	prot = (O_RDONLY == mode) ? PROT_READ : PROT_READ | PROT_WRITE;
	mode = (O_RDONLY == mode) ? O_RDONLY : O_RDWR;

	/*
	 * Check the catalog file and open it.
	 * File type must be regular or symlink.
	 * If file size is a gigabyte or greater, it is likely to be a bad file.
	 */
	if (stat(FileName, &st) < 0) {
		LibError(MsgFunc, EXIT_FATAL, 616, FileName);
		return (NULL);
	}
	SetErrno = 0;
	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
		LibError(MsgFunc, EXIT_FAILURE, 18200, FileName);
		return (NULL);
	}
#if defined(DEBUG)
	if (strcmp(FileName, "TooSmallTest") == 0) {
		st.st_size = sizeof (struct CatalogHdr)-1;
	}
	if (strcmp(FileName, "TooBigTest") == 0) {
		st.st_size = 1024 * 1024 * 1024 + 1;
	}
#endif /* defined(DEBUG) */
	if (st.st_size == 0) {
		LibError(MsgFunc, EXIT_FAILURE, 18209, FileName);
		return (NULL);
	}
	if ((st.st_size < sizeof (struct CatalogHdr)) &&
		(st.st_size < sizeof (struct CatalogTableHdr))) {
		LibError(MsgFunc, EXIT_FAILURE, 18210, FileName);
		return (NULL);
	}
	if (st.st_size > 1024 * 1024 * 1024) {
		LibError(MsgFunc, EXIT_FAILURE, 18201, FileName);
		return (NULL);
	}
	if ((fd = open(FileName, mode)) < 0) {
		LibError(MsgFunc, EXIT_FATAL, 613, FileName);
		return (NULL);
	}

	/*
	 * Memory map the file.
	 */
	mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
	(void) close(fd);
	if (MAP_FAILED == mp) {
		LibError(MsgFunc, EXIT_FATAL, 609, FileName);
		return (NULL);
	}
	*size = st.st_size;
	return (mp);
}


/*
 * Unmap catalogs.
 */
static void
UnmapCatalogs(void)
{
	/*
	 * Unmap the catalog files.
	 */
	if (Catalogs != NULL) {
		int		n;

		for (n = 0; n < CatalogTable->CtNumofFiles; n++) {
			struct CatalogTableHdr *ct;
			struct CatalogMap *cm;

			cm = &Catalogs[n];
			if (cm != NULL) {
				ct = &CatalogTable[n];
				if (munmap((char *)cm->CmHdr,
				    cm->CmSize) == -1) {
					LibError(NULL, EXIT_FAILURE, 611,
					    ct->CtFname);
				}
			}
		}
		free(Catalogs);
		Catalogs = NULL;
	}

	/*
	 * Unmap the catalog table.
	 */
	if (CatalogTable != &nullCatalogTable) {
		if (munmap((char *)CatalogTable,
		    CatalogTable->Ct.MfLen) == -1) {
			LibError(NULL, EXIT_FAILURE, 611, "CatalogTable");
		}
		CatalogTable = &nullCatalogTable;
	}
	LastCe = NULL;
}
