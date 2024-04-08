/*	  derrno.h */
/*    d_errno values for das aci calls */

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

#ifndef _GENERIC_DERRNO_H
#define	_GENERIC_DERRNO_H

#pragma ident "$Revision: 1.16 $"


#ifndef D_ERROR_INCL

#define	D_ERROR_INCL 1


#define	EOK	  0		/* request successful	    */
#define	ERPC	 1		/* rpc failure		*/
#define	EINVALID	2		/* aci parameter invalid	*/
#define	ENOVOLUME    3		/* volume not found of this type    */
#define	ENODRIVE	4		/* drive not in Grau ATL	*/
#define	EDRVOCCUPIED 5		/* the requested drive is in use    */

/* the robot has a physical problem with the volume */
#define	EPROBVOL	6

#define	EAMU	 7		/* an internal error in the AMU	*/

/* the DAS was unable to communicate with the AMU */
#define	EAMUCOMM	8

#define	EROBOT	9		/* the robotic system is not functioning */

/* the AMU was unable to communicate with the robot */
#define	EROBOTCOMM  10

#define	ENODAS	11		/* the DAS system is not active	*/

/* the drive did not contain an unloaded volume */
#define	EDEVEMPTY   12

#define	ENOTREG	13		/* invalid registration	    */
#define	EBADHOST    14		/* invalid hostname or ip address    */
#define	ENOAREA	15		/* the area name does not exist	*/

/* the client is not authorized to make this request */
#define	ENOTAUTH    16

/* the dynamic area became full, insertion stopped */
#define	EDYNFULL    17

/* the drive is currently available to another client */
#define	EUPELSE	18

#define	EBADCLIENT  19		/* the client does not exist	*/
#define	EBADDYN	20		/* the dynamic area does not exist    */
#define	ENOREQ	21		/* no request exists with this number    */
#define	ERETRYL	22		/* retry attempts exceeded	 */
#define	ENOTMOUNTED 23		/* requested volser is not mounted    */
#define	EINUSE	24		/* requested volser is in use	*/
#define	ENOSPACE    25		/* no space availble to add range    */
#define	ENOTFOUND   26		/* the range or object was not found    */

/* the request was cancelled by aci_cancel() */
#define	ECANCELLED  27

#define	EDASINT	28		/* internal DAS error */
#define	EACIINT	29		/* internal ACI error */
#define	EMOREDATA   30		/* for a query more data are available */

/* it is defined for HPUX11(comes from UnixWare) */
#ifdef ENOMATCH

#undef ENOMATCH
#endif
#define	ENOMATCH    31		/* things don't match together */
#define	EOTHERPOOL  32		/* volser is still in another pool */
#define	ECLEANING   33		/* drive in cleaning */
#define	ETIMEOUT    34		/* The aci request timed out */
/* --- new Ver. DAS 3.0 --- */
#define	ESWITCHINPROG 35	/* The AMU starts a switch */
#define	ENOPOOL	36		/* Poolname not defined */
#define	EAREAFULL   37		/* Area is full */

/* Robot is not ready because of a HICAP request */
#define	EHICAPINUSE 38

#define	ENODOUBLESIDE 39	/* The volser has no two sides */
#define	EEXUP	40		/* The drive is EXUP for another client */

/* the robot has a problem with handling the device */
#define	EPROBDEV    41
#define	ECOORDINATE 42		/* one or more coordinates are wrong */
#define	EAREAEMPTY  43		/* area is empty */
#define	EBARCODE    44		/* Barcode read error */

/* Client tries to allocate volsers that are already allocated */
#define	EUPOWN	45

#define	ENOTSUPPHCMD  46	/* Not supported host command */
#define	EDATABASE   47		/* Database error */
#define	ENOROBOT    48		/* Robot is not configured */
#define	EINVALIDDEV 49		/* The device is invalid */
#define	EINDOUBTEXEC 50		/* Request was already sent to robot */
#define	ENOLONGNAMES 51		/* No long drive names  */

/* number of Error codes in header file increment when adding */
/* new codes to the end of the list */
#define	NO_ECODES   52

#if defined(MAIN)
api_messages_t  grau_messages[NO_ECODES] = {
	API_ERR_OK, 0, 0, EOK, "request successful",
	API_ERR_TR, 4, 4, ERPC, "rpc failure",
	API_ERR_TR, 0, 0, EINVALID, "aci parameter invalid",
	API_ERR_TR, 0, 0, ENOVOLUME, "volume not found of this type",
	API_ERR_DD, 0, 0, ENODRIVE, "drive not in Grau ATL",
	API_ERR_DD, 5, 60, EDRVOCCUPIED, "requested drive is in use",
	API_ERR_TR, 2, 30, EPROBVOL, "robot has a physical problem with volume",
	API_ERR_TR, 4, 10, EAMU, "an internal error in the AMU",
	API_ERR_TR, 4, 10, EAMUCOMM, "DAS was unable to communicate with AMU",
	API_ERR_TR, 4, 10, EROBOT, "robotic system is not functioning",
	API_ERR_TR, 4, 10, EROBOTCOMM,
	    "AMU was unable to communicate with robot",
	API_ERR_TR, 4, 30, ENODAS, "DAS system is not active",
	API_ERR_TR, 4, 20, EDEVEMPTY,
	    "drive did not contain an unloaded volume",
	API_ERR_TR, 0, 0, ENOTREG, "invalid registration",
	API_ERR_DL, 0, 0, EBADHOST, "invalid hostname or ip address",
	API_ERR_TR, 0, 0, ENOAREA, "area name does not exist",
	API_ERR_TR, 0, 0, ENOTAUTH,
	    "client is not authorized to make the request",
	API_ERR_TR, 2, 10, EDYNFULL,
		"dynamic area became full, insertion stopped",
	API_ERR_DD, 5, 60, EUPELSE,
	    "drive is currently unavailable to this client",
	API_ERR_DL, 0, 0, EBADCLIENT, "client does not exist",
	API_ERR_DL, 2, 10, EBADDYN, "dynamic area does not exist",
	API_ERR_TR, 0, 0, ENOREQ, "no request exists with this number",
	API_ERR_TR, 0, 0, ERETRYL, "retry attempts exceeded",
	API_ERR_TR, 0, 0, ENOTMOUNTED, "requested volser is not mounted",
	API_ERR_TR, 5, 60, EINUSE, "requested volser is in use",
	API_ERR_TR, 0, 0, ENOSPACE, "no space availble to add range",
	API_ERR_TR, 0, 0, ENOTFOUND, "range or object was not found",
	API_ERR_TR, 0, 0, ECANCELLED, "request was cancelled by aci_cancel",
	API_ERR_TR, 0, 0, EDASINT, "internal DAS error",
	API_ERR_TR, 0, 0, EACIINT, "internal DAS error",
	API_ERR_TR, 0, 0, EMOREDATA, "more data",
	API_ERR_TR, 0, 0, ENOMATCH, "things dont match together",
	API_ERR_TR, 0, 0, EOTHERPOOL, "volser is still in another pool",
	API_ERR_TR, 20, 30, ECLEANING, "drive in cleaning",
	API_ERR_TR, 0, 0, ETIMEOUT, "aci request timed out",
	API_ERR_TR, 5, 60, ESWITCHINPROG, "The AMU has started a switch",
	API_ERR_TR, 0, 0, ENOPOOL, "Poolname not defined",
	API_ERR_TR, 5, 60, EAREAFULL, "Area is full",
	API_ERR_TR, 5, 60, EHICAPINUSE,
	    "Robot is not ready because of a HICAP request",
	API_ERR_TR, 0, 0, ENODOUBLESIDE, "The volser does not have two sides",
	API_ERR_TR, 5, 60, EEXUP, "The drive is EXUP for another client",
	API_ERR_DD, 2, 10, EPROBDEV,
	    "The robot has a problem with handling the device",
	API_ERR_TR, 0, 0, ECOORDINATE, "One or more coordinates are wrong",
	API_ERR_TR, 5, 60, EAREAEMPTY, "Area is empty",
	API_ERR_TR, 3, 10, EBARCODE, "Barcode read error",
	API_ERR_TR, 0, 0, EUPOWN,
	    "Client tries to allocate volsers that are already allocated",
	API_ERR_TR, 0, 0, ENOTSUPPHCMD, "Unsupported host command",
	API_ERR_DL, 5, 10, EDATABASE, "Database error",
	API_ERR_DL, 0, 0, ENOROBOT, "Robot is not configured",
	API_ERR_DD, 0, 0, EINVALIDDEV, "The device is invalid",
	API_ERR_DL, 0, 0, EINDOUBTEXEC, "Request was already sent to robot",
	API_ERR_DD, 0, 0, ENOLONGNAMES, "No long drive names"
};
#else
extern api_messages_t grau_messages[NO_ECODES];
#endif

#endif				/* D_ERROR_INCL   */

#endif /* _GENERIC_DERRNO_H */
