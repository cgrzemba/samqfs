/*
 * catlib.h - catalog library definitions.
 *
 *	Public interface to the catalog library functions.
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

#ifndef _AML_CATLIB_H
#define	_AML_CATLIB_H

#pragma ident "$Revision: 1.19 $"

#if !defined(HERE)
#define	HERE _SrcFile, __LINE__
#endif /* !defined(HERE) */

/* For debugging with dbx */
#define	DBXWAIT { static int w = 1; while (w); }

/*
 * Catalog entry field identifiers.
 * Used to identify fields in get/set functions.
 * Identifiers are derived from catalog entry field names.
 */
enum CeFields {
	CEF_Status,
	CEF_MediaType,
	CEF_Vsn,
	CEF_Access,
	CEF_Capacity,
	CEF_Space,
	CEF_BlockSize,
	CEF_LabelTime,
	CEF_ModTime,
	CEF_MountTime,
	CEF_BarCode,
	CEF_Eq,
	CEF_Slot,
	CEF_Partition,
	CEF_PtocFwa,
	CEF_LastPos,
	CEF_VolInfo,

	CEF_MAX
} CeFields;

/* Public functions. */
/* Catalog file manipulation. */
int CatalogCreateCatfile(char *FileName, int NumofEntries, void **mp,
	size_t *size, void (*MsgFunc)(int code, char *msg));
int CatalogMmapCatfile(char *FileName, int mode, void **mp, size_t *len,
	void(*MsgFunc)(int code, char *msg));
int CatalogSync(void);
void CatalogTerm(void);

/* Catalog reads. */
struct CatalogEntry *CatalogCheckSlot(struct VolId *vid,
	struct CatalogEntry *ce);
struct CatalogEntry *CatalogCheckVolId(struct VolId *vid,
	struct CatalogEntry *ce);
struct CatalogHdr *CatalogGetCatalogHeader(const char *path);
int CatalogGetEntries(int eq, int start, int end, struct CatalogEntry **cep);
struct CatalogEntry *CatalogGetEntry(struct VolId *vid,
	struct CatalogEntry *ce);
struct CatalogEntry *CatalogGetCeByLoc(int eq, int slot, int part,
	struct CatalogEntry *ce);
struct CatalogEntry *CatalogGetCeByMedia(char *media_type, vsn_t vsn,
	struct CatalogEntry *ce);
struct CatalogEntry *CatalogGetCeByBarCode(int eq, char *media_type,
	char *string, struct CatalogEntry *ce);
struct CatalogEntry *CatalogGetCleaningVolume(int eq, struct CatalogEntry *ce);
struct CatalogHdr *CatalogGetHeader(int eq);
struct CatalogEntry *CatalogGetEntriesByLibrary(int eq, int *numOfEntries);

/* Catalog server functions. */
/* Yes, it's ugly, but it gives us 'caller id'. */

#define	CatalogAssignFreeSlot(a) _CatalogAssignFreeSlot(_SrcFile, __LINE__, \
	(a))
int _CatalogAssignFreeSlot(const char *SrcFile, const int SrcLine,
	int eq);

#define	CatalogExport(a) _CatalogExport(_SrcFile, __LINE__, (a))
int _CatalogExport(const char *SrcFile, const int SrcLine,
	struct VolId *vid);

#define	CatalogFormatPartitions(a, b, c) _CatalogFormatPartitions(_SrcFile, \
	__LINE__, (a), (b), (c))
int _CatalogFormatPartitions(const char *SrcFile, const int SrcLine,
	struct VolId *vid, uint32_t CatStatus, int NumofParts);

#define	CatalogInit(a) _CatalogInit(_SrcFile, __LINE__, a)
int _CatalogInit(const char *SrcFile, const int SrcLine, char *client_name);

#define	CatalogLabelComplete(a) _CatalogLabelComplete(_SrcFile, __LINE__, \
	(a))
int _CatalogLabelComplete(const char *SrcFile, const int SrcLine,
	struct CatalogEntry *ce);

#define	CatalogLabelFailed(a, b) _CatalogLabelFailed(_SrcFile, __LINE__, \
	(a), (b))
int _CatalogLabelFailed(const char *SrcFile, const int SrcLine,
	struct VolId *vi, vsn_t vsn);

#define	CatalogLabelVolume(a, b) _CatalogLabelVolume(_SrcFile, __LINE__, \
	(a), (b))
int _CatalogLabelVolume(const char *SrcFile, const int SrcLine,
	struct VolId *vi, vsn_t vsn);

#define	CatalogLibraryExport(a) _CatalogLibraryExport(_SrcFile, __LINE__, \
	(a))
int _CatalogLibraryExport(const char *SrcFile, const int SrcLine, int eq);

#define	CatalogMediaClosed(a) _CatalogMediaClosed(_SrcFile, __LINE__, \
	(a))
int _CatalogMediaClosed(const char *SrcFile, const int SrcLine,
	struct CatalogEntry *ce);
#define	CatalogMoveSlot(a, b) _CatalogMoveSlot(_SrcFile, __LINE__, \
	(a), (b))
int _CatalogMoveSlot(const char *SrcFile, const int SrcLine,
	struct VolId *vi, int DestSlot);

#define	CatalogReconcileCatalog(a) _CatalogReconcileCatalog(_SrcFile, \
	__LINE__, (a))
int _CatalogReconcileCatalog(const char *SrcFile, const int SrcLine, int eq);

#define	CatalogRemoteSamUpdate(a, b) _CatalogRemoteSamUpdate(_SrcFile, \
	__LINE__, (a), (b))
int _CatalogRemoteSamUpdate(const char *SrcFile, const int SrcLine,
	struct CatalogEntry *ce, uint16_t flags);

#define	CatalogReserveVolume(a, b, c, d, e) _CatalogReserveVolume(_SrcFile, \
	__LINE__, (a), (b), (c), (d), (e))
int _CatalogReserveVolume(const char *SrcFile, const int SrcLine,
	struct VolId *vi, time_t rtime, char *asname, char *owner,
	char *fsname);

#define	CatalogSetAudit(a) _CatalogSetAudit(_SrcFile, __LINE__, (a))
int _CatalogSetAudit(const char *SrcFile, const int SrcLine, int eq);

#define	CatalogSetCleaning(a) _CatalogSetCleaning(_SrcFile, __LINE__, (a))
int _CatalogSetCleaning(const char *SrcFile, const int SrcLine, int eq);

#define	CatalogSlotInit(a, b, c, d, e) _CatalogSlotInit(_SrcFile, \
	__LINE__, (a), (b), (c), (d), (e))
int _CatalogSlotInit(const char *SrcFile, const int SrcLine,
	struct VolId *vid, uint32_t CatStatus, int NumofParts, char *BarCode,
	char *AltBarCode);

#define	CatalogSetField(a, b, c, d) _CatalogSetField( \
	_SrcFile, __LINE__, (a), (b), (c), (d))
int _CatalogSetField(const char *SrcFile, const int SrcLine,
	struct VolId *vid, enum CeFields field, uint64_t value, uint32_t mask);

#define	CatalogSetFieldByLoc(a, b, c, d, e, f) _CatalogSetFieldByLoc(_SrcFile, \
	__LINE__, (a), (b), (c), (d), (e), (f))
int _CatalogSetFieldByLoc(const char *SrcFile, const int SrcLine,
	int eq, int slot, int part, enum CeFields field,
	uint32_t value, uint32_t mask);

#define	CatalogSetString(a, b, c) _CatalogSetString( \
	_SrcFile, __LINE__, (a), (b), (c))
int _CatalogSetString(const char *SrcFile, const int SrcLine,
	struct VolId *vid, enum CeFields field, char *string);

#define	CatalogSetStringByLoc(a, b, c, d, e) _CatalogSetStringByLoc( \
	_SrcFile, __LINE__, (a), (b), (c), (d), (e))
int _CatalogSetStringByLoc(const char *SrcFile, const int SrcLine,
	int eq, int slot, int part, enum CeFields field,
	char *string);

#define	CatalogSetTraceFlags(a) _CatalogSetTraceFlags(_SrcFile, __LINE__, a)
int _CatalogSetTraceFlags(const char *SrcFile, const int SrcLine,
	uint32_t flags);

struct VolId *CatalogVolIdFromCe(struct CatalogEntry *ce, struct VolId *vi);

#define	CatalogUnreserveVolume(a) _CatalogUnreserveVolume(_SrcFile, __LINE__, \
	(a))
int _CatalogUnreserveVolume(const char *SrcFile, const int SrcLine,
	struct VolId *vid);

#define	CatalogVolumeLoaded(a) _CatalogVolumeLoaded(_SrcFile, __LINE__, \
	(a))
int _CatalogVolumeLoaded(const char *SrcFile, const int SrcLine,
	struct CatalogEntry *ce);

#define	CatalogVolumeUnloaded(a, b) _CatalogVolumeUnloaded(_SrcFile, \
	__LINE__, (a), (b))
int _CatalogVolumeUnloaded(const char *SrcFile, const int SrcLine,
	struct VolId *vid, char *string);

char *CatalogStatusToStr(uint32_t status, char buf[]);
char *CatalogStrFromEntry(struct CatalogEntry *ce, char *string, size_t len);

#endif /* _AML_CATLIB_H */
