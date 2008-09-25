/*
 * udscom.h - UDS communication definitions.
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

#ifndef UDSCOM_H
#define	UDSCOM_H

#ifdef sun
#pragma ident "$Revision: 1.16 $"
#endif

/* Basic message types. */
/* Client/server functions will define additional message types. */
enum UdsMsgType {
	UM_null,
	UM_ack,
	UM_nak,
	UM_max
};

/* Message arguments. */
struct UmNak {
	int	Errno;
	char	msg[80];
};

/* The UDS message header. */
struct UdsMsgHeader {
	int	UhMagic;		/* Server specific magic number */
	char	UhName[16];		/* Client identification */
	pid_t	UhPid;			/* Client's pid */
	char	UhSrcFile[24];		/* Source file name */
	int	UhSrcLine;		/* Source file line number */
	int	UhType;			/* Message type */
	int	UhArgSize;		/* Size of following message argument */
	/* 'UhArgSize' (bytes) argument follows. */
};

/*
 * Client Definitions.
 */
struct UdsClient {
	char	*UcServerName;		/* Server's UDS socket name */
	char	*UcClientName;		/* Client identification */
	int	UcMagic;		/* Associated magic number */
	void 	(*UcLog)(char *msg);	/* Optional function for */
					/* logging messages */
};

/*
 * Server Definitions.
 *
 * The server provides a message processing table in the form of an array
 * of the following structs.
 */
struct UdsMsgProcess {
	/* Processing function */
	void *	(* UmFunc)		/* Message function */
	(void *arg,			/* Clients message argument */
	struct UdsMsgHeader *hdr);	/* Message header */
	size_t	UmArgSize;		/* Expected size of the client's */
					/* argument */
	size_t	UmRspSize;		/* Size of the server's response */
};

/*
 * Arguments to UdsRecvMsg.
 */
struct UdsServer {
	char	*UsServerName;		/* Server's UDS socket name */
	int	UsMagic;		/* Associated magic number */
	void 	(*UsLog)(char *msg);	/* Optional function for */
					/* logging messages */
	volatile int UsStop;		/* Stop receiving when set */
	struct UdsMsgProcess *UsTable;	/* Message processing table */
	int	UsNumofTable;		/* Number of entries in table */
	size_t	UsArgbufSize;		/* Size of message argument buffer. */
};

/* Functions. */
int UdsSendMsg(const char *SrcFile, const int SrcLine, struct UdsClient *clnt,
	int SrvrMsgType, void *arg, int arg_size, void *rsp,
	int rsp_size);
int UdsRecvMsg(struct UdsServer *srvr);

#endif /* UDSCOM_H */
