/*
 * catserver.h - Catalog server client/server definitions.
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
 * or https://illumos.org/license/CDDL.
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

#ifndef CATSERVER_H
#define	CATSERVER_H

#pragma ident "$Revision: 1.26 $"

#define	SERVER_NAME "Catalog"
#define	SERVER_MAGIC 003012423

enum CatServerReq {
	CSR_AssignFreeSlot,	/* Assign a free slot in catalog */
	CSR_Export,		/* Export entry from catalog */
	CSR_FormatPartitions,	/* Update partitions based on format request */
	CSR_GetInfo,		/* Get client required information */
	CSR_LabelComplete,	/* Complete a label operation */
	CSR_LabelFailed,	/* Remove dummy entry after a label failure */
	CSR_LabelVolume,	/* Begin a label operation */
	CSR_LibraryExport,	/* Export all catalog entries to historian */
	CSR_MediaClosed,	/* Write to media is completed update catalog */
	CSR_MoveSlot,		/* Move a cartridge slot */
	CSR_ReconcileCatalog,	/* Free entry for any unoccupied slot */
	CSR_RemoteSamUpdate,	/* Update a remote SAM catalog entry */
	CSR_ReserveVolume,	/* Reserve a volume for archiving */
	CSR_SetAudit,		/* Set audit flag for all entries */
	CSR_SetCleaning,	/* Set cleaning flag for any cleaning tapes */
	CSR_SetRemoteServer,	/* Remote server and catalog is initialized */
	CSR_SlotInit,		/* Initialize a slot */
	CSR_SetField,		/* Set field in catalog by media */
	CSR_UnReserveVolume,	/* Unreserve a volume for archiving */
	CSR_VolumeLoaded,	/* Volume has been loaded into a drive */
	CSR_VolumeUnloaded,	/* Volume has been unloaded from a drive */
	USR_MAX
};

/* Arguments for requests. */

struct CsrAssignFreeSlot {
	uint16_t AsEq;
};


struct CsrExport {
	struct VolId SeVid;
};

struct CsrFormatPartitions {
	struct VolId FrVid;
	uint32_t	FrStatus;
	int		FrNumParts;
};

struct CsrLibraryExport {
	uint16_t LeEq;
};

struct CsrLabelComplete {
	struct CatalogEntry LcCe;
};

struct CsrLabelFailed {
	struct 	VolId LfVid;
	vsn_t	LfNewVsn;
};

struct CsrLabelVolume {
	struct VolId LvVid;
	vsn_t	LvNewVsn;
};

struct CsrMediaOp {
	uint16_t SfEq;
	char	SfMediaType[3];
	vsn_t	SfVsn;
};

struct CsrMediaClosed {
	struct CatalogEntry SfCe;
};

struct CsrMoveSlot {
	struct VolId MsVid;
	int	MsDestSlot;
};

struct CsrReconcileCatalog {
	uint16_t RcEq;
};

struct CsrRemoteSamUpdate {
	struct CatalogEntry RsCe;
	uint16_t RsFlags;
};

struct CsrReserveVolume {
	struct VolId RvVid;
	time_t  RvTime;				/* Time reservation made */
	uname_t RvAsname;			/* Archive set */
	uname_t RvOwner;			/* Owner */
	uname_t RvFsname;			/* File system */
};

struct CsrSetAudit {
	uint16_t SaEq;
};

struct CsrSetCleaning {
	uint16_t ScEq;
};

struct CsrSlotInit {
	struct VolId SiVid;
	uint32_t SiStatus;			/* Status bits to set */
	int	SiTwoSided;			/* Two sided media in library */
	char	SiBarcode[BARCODE_LEN + 1];
	char	SiAltBarcode[BARCODE_LEN + 1];
};

struct CsrSetField {
	struct VolId SfVid;
	enum CeFields SfField;		/* Field to change */
	union {
		struct {
			uint64_t SfVal;		/* Value for field */
			uint32_t SfMask;	/* Mask for status field */
		} v;
		char SfString[BARCODE_LEN + 1];
	} a;
};

struct CsrSetString {
	struct VolId SfVid;
	enum CeFields SfField;		/* Field to change */
	char	SfString[BARCODE_LEN + 1];
};

struct CsrUnReserveVolume {
	struct VolId UrVid;
};

struct CsrVolumeLoaded {
	struct CatalogEntry VlCe;
};

struct CsrVolumeUnloaded {
	struct VolId SfVid;
	char    SfBarcode[BARCODE_LEN + 1];
};


/* Responses. */

struct CsrGetInfoRsp {
	upath_t	CatTableName;		/* Path for catalog table file */
};

struct CsrGeneralRsp {
	int		GrStatus;
	int		GrErrno;
};

#endif /* CATSERVER_H */
