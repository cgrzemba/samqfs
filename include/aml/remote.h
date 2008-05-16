/*
 * remote.h - remote sam stuff
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


#if !defined(_SAM_REMOTE_H)
#define	_SAM_REMOTE_H

#pragma ident "$Revision: 1.24 $"

#include <netinet/in.h>

#include "sam/types.h"
#include "aml/device.h"
#include "sam/resource.h"
#include "aml/catlib.h"
#include "driver/samrd_def.h"

/*
 * Timeout for the robot servicing client's request after connection to
 * the client have been established, seconds
 * Based on pseudo device timeout plus possible delays over the network
 */

#define	RMT_TIMEOUT	(SAMRD_TIMEOUT + 15)


/* For catalog entries to match */
#define	CES_MATCH (CES_inuse | CES_labeled)
#define	CES_NOT_ON (CES_needs_audit | CES_cleaning | CES_non_sam)

#define	RMT_SAM_VERSION	400
#define	RMT_SAM_PORT	1000
#define	RMT_SAM_PORTS	5000
#define	RMT_SAM_SERVICE	"rmtsam"
#define	RMT_SAM_MAX_CLIENTS	10

/*
 * Service name for remote daemon, registered with spm.
 */
#define	SAMREMOTE_SPM_SERVICE_NAME	"remote"

/*
 * Client server struct for remote sam.  Each server has one for each
 * possible client.  Each client has one.  Kept in the master shared
 * memory segment.  Pointed to by the dev_ent.dt specific area.
 */
typedef struct srvr_clnt {
#ifdef	sun
	mutex_t	sc_mutex;
#endif	/* sun */
	union {
		struct in6_addr	ct6;
		struct in_addr	ct;
	} ctl_addr;
	int	index;		/* Client index 0:RMT_SAM_MAX_CLIENTS - 1 */
	ushort_t	flags;
	ushort_t	port;		/* Port number of connected peer */
} srvr_clnt_t;

#define	control_addr ctl_addr.ct
#define	control_addr6 ctl_addr.ct6

#define	SRVR_CLNT_CONNECTED 0x0001
#define	SRVR_CLNT_IPV6    0x0002
#define	SRVR_CLNT_PRESENT 0x0004

typedef struct rmt_vsn_equ_list {
	void		*comp_exp;		/* Compiled expression */
	dev_ent_t	*un;			/* Equipment to search */
	struct rmt_vsn_equ_list	*next;		/* Next entry */
	media_t		media;
	ushort_t	comp_type;		/* Regular expression type */
} rmt_vsn_equ_list_t;

#define	 RMT_COMP_TYPE_REGCOMP	1
#define	 RMT_COMP_TYPE_COMPILE	2

typedef struct {
	int		fd;			/* Control fd */
	dev_ent_t	*un;			/* Devi ent of the server */
	char		*host_name;		/* Client's host name */
	srvr_clnt_t	*srvr_clnt;
	struct in_addr	client_addr;
	struct in6_addr	client_addr6;
	union {
		struct {
			uint_t
#if defined(_BIT_FIELDS_HTOL)
				vsn_list_active	:1,
				unused		:31;
#else /* defined(_BIT_FIELDS_HTOL) */
				unused		:31,
				vsn_list_active	:1;
#endif /* defined(_BIT_FIELDS_HTOL) */
		} b;
		uint_t	bits;
	} flags;
	rmt_vsn_equ_list_t	*first_equ;	/* First eq to search */
						/* for vsns */
} rmt_sam_client_t;

#define	CLIENT_VSN_LIST  0x80000000

typedef enum rmt_sam_command {
	/*
	 * The following are from client to server.
	 * Response back to the client for RMT_SAM_LOAD, and RMT_SAM_ARCHIVE
	 * will be on the data port provided by the
	 * client.  If the connection to that port cannot be established,
	 * then the failure will be on the control socket.  It is up to
	 * the client(control thread) to notify the requester.
	 */
	RMT_SAM_SEND_VSNS = 1,		/* Send list of usable vsns */
	RMT_SAM_CONNECT,		/* Connect with server. Sent to the */
					/* well known port */
	RMT_SAM_DISCONNECT,		/* Disconnect from server */
	RMT_SAM_HEARTBEAT,		/* Server alive? */

	/* The following are server to client */
	RMT_SAM_UPDATE_VSN,		/* Update vsn(s) */
	RMT_SAM_REQ_RESP		/* Response to request */
} rmt_sam_command_t;

/* Initial connect request */
typedef struct rmt_sam_connect {
	uname_t		fset_name;
} rmt_sam_connect_t;

/* Initial connect request response */
typedef struct rmt_sam_cnt_resp {
	uname_t		fset_name;	/* Leave in same order as connect */
	int		err;
	ushort_t	serv_port;	/* Port to use for control */
	ushort_t	pad;
} rmt_sam_cnt_resp_t;

typedef struct rmt_sam_vsn_entry {
	CatalogEntry_t	ce;		/* Catalog entry */
	uint_t 		upd_flags;	/* How to apply this update */
	char		pad[4];
} rmt_sam_vsn_entry_t;

/*
 * Update catalog information for a vsn.  Up to 100 vsn entries sent
 * at a time.  This would equal about 8k. This will be done
 * with two writes, the first will be the command block, the second
 * will be the data.  The command block will have the first vsn
 * and the count includes that vsn, so a vsn update of 1 vsn will
 * consist of only the command block.
 */
typedef struct rmt_sam_update_vsn {
	int		count;			/* Count of vsns sent */
	char		pad[4];
	rmt_sam_vsn_entry_t	vsn_entry;	/* The first one */
} rmt_sam_update_vsn_t;

typedef enum rmt_resp_type {
	RESP_TYPE_UNK,
	RESP_TYPE_CMD,
	RESP_TYPE_HEARTBEAT
} rmt_resp_type_t;

typedef struct rmt_sam_req_resp {
	uint_t		err;
	uint_t		flags;
#ifndef BYTE_SWAP
	rmt_resp_type_t	type;
#else	/* BYTE_SWAP */
	uint_t		type;
#endif	/* BYTE_SWAP */
} rmt_sam_req_resp_t;

typedef union rmt_sam_ureq {
	rmt_sam_connect_t	connect_req;
	rmt_sam_cnt_resp_t 	con_response;
	rmt_sam_req_resp_t	req_response;
	rmt_sam_update_vsn_t	update_vsn;
} rmt_sam_ureq_t;

typedef struct rmt_sam_request {
	uint_t		version;	/* Version of remote sam */
	uint_t		flags;		/* Flags at the command level */
	uint_t		mess_addr;	/* Address of waiting process/thread */
					/* (can be null). */
#ifndef BYTE_SWAP
	rmt_sam_command_t command;	/* The command */
#else	/* BYTE_SWAP */
	uint_t		command;	/* The command */
#endif	/* BYTE_SWAP */
	ushort_t	sin_port;	/* Data port to connect to optional */
	char		pad[6];
	rmt_sam_ureq_t	request;
} rmt_sam_request_t;

/*
 * The device message area is used to send messages from a device on the
 * server to the server or pseudo device. The message type is
 * MESS_MT_RS_SERVER(defined in message.h) and the command is mapped to
 * one of these.
 */
typedef enum rmt_mess_cmd {
	RMT_MESS_CMD_SEND,		/* Send message to client */
	RMT_MESS_CATALOG_CHANGE		/* Catalog changed */
} rmt_mess_cmd_t;

/*
 * The data portion of the message is one of the following structs.
 */
typedef struct rmt_mess_arch {
	int	position;
	int	err;
	uint_t	space;
} rmt_mess_arch_t;

typedef struct rmt_mess_send {
	int	client_index;
	rmt_sam_request_t	mess;	/* Message to send to client */
} rmt_mess_send_t;

typedef struct rmt_cat_ent {
	union {
		unsigned int	flags;
		unsigned int	bits;
	} status;
	media_t		media;
	vsn_t		vsn;
	int		slot;
	int		partition;
} rmt_cat_ent_t;

typedef struct rmt_mess_cat_chng {
	uint_t		flags;
	equ_t		eq;
	rmt_cat_ent_t	entry;
} rmt_mess_cat_chng_t;

#define	RMT_CAT_CHG_FLGS_LBL	1		/* Media is being relabeled */
#define	RMT_CAT_CHG_FLGS_EXP	2		/* Media is being exported */

/* A union of all the message structs */
typedef union rmt_mess_u {
	rmt_mess_send_t		send;
	rmt_mess_cat_chng_t	cat_change;
} rmt_mess_u_t;

/* This is the complete message sent by the device. */
typedef struct rmt_mess {		/* Archive finished */
	struct rmt_message	*addr;	/* Address waiting process (optional) */
	rmt_mess_u_t		messages;
} rmt_mess_t;

/*
 * If the sender of the message is waiting for a response, then "addr"
 * points to this struct.  Upon completion of the request, "complete"
 * is set to the result of the request and the condition is signaled.
 */
typedef struct rmt_message {
#ifdef	sun
	mutex_t		mutex;
	cond_t		cond;
#endif	/* sun */
	int		complete;	/* Set to -1 before starting */
	rmt_mess_u_t	messages;
} rmt_message_t;


#endif	/* !defined(_SAM_REMOTE_H) */
