/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#ifndef _FAULTS_H
#define	_FAULTS_H

#pragma ident	"$Revision: 1.21 $"

/*
 * faults.h - SAM-FS API fault handling API and data structures.
 */

#include "sam/types.h"

#include "mgmt/config/common.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"


/* the path to the main faults repository */
#define	FAULTLOG VAR_DIR"/faults/faultlog.bin"
#define	NO_DEL_FAULT VAR_DIR"/faults/fault_no_del_log.bin"

/* path to the main faults directory */
#define	FAULTLOG_DIR VAR_DIR"/faults"

/* the rotate script for the fault log */
#define	TRACE_ROTATE "trace_rotate"  /* Name of trace rotation script */

#define	MAXLINE 1024+256
#define	CMD_ARG 256 /* length of commands */
#define	DEFAULTS_MIN 50 /* Minimum # of faults */
#define	DEFAULTS_MAX 700 /* Max number of faults */
#define	MV	"/bin/mv"

/* max allowable size of persistence file */
#define	MAX_FSIZE	(1024*1024)

/*
 * The convention to get ALL of a certain
 * attribute. That is, "dont care."
 * So, a GET_ALL for 'state' means, do the
 * operation on all fault state types that exist.
 * And, of course, a GET_ALL for number of faults
 * is a request for ALL faults that exist, which may
 * further be qualified by other attributes.
 * A better label, better than GET_ALL, would certainly
 * be more helpful. Till such time, this will have to do.
 */
#define	GET_ALL -1

/* no library specified */
#define	NO_LIBRARY  "No Library"

/* no eq # */
#define	NO_EQ 0

/* fault summary */
typedef struct fault_summary {
	int	num_critical_faults;
	int	num_major_faults;
	int	num_minor_faults;
} fault_summary_t;

/* the state of the fault */
typedef enum fault_state {
	UNRESOLVED,		/* Fault yet unresolved */
	ACK,			/* Fault has been ACKnowledged */
	DELETED			/* Fault has been deleted */
} fault_state_t;

/*
 * The severity of the fault
 * These severities group the error
 * severities defined in syslog.h.
 * So a syslog.h severity of 0 [LOG_EMERG]
 * and a 1 [LOG_ALERT] are labeled as
 * CRITICAL, 2 and 3 as MAJOR, and anything
 * below is MINOR.
 */
typedef enum fault_sev {
	CRITICAL,
	MAJOR,
	MINOR
} fault_sev_t;

/* The main faults attribute structure */
typedef struct fault_attr {
	long			errorID;	/* Error ID */
	uname_t			compID;		/* archiver etc */
	fault_sev_t		errorType;	/* Crit, Alert etc */
	time_t			timestamp;	/* Date n Time */
	upath_t			hostname;	/* System ID */
	char			msg[MAXLINE];	/* The message string */
	fault_state_t	state;			/* unresolved etc */
	uname_t			library;
	equ_t			eq;
} fault_attr_t;

/* deprecated in API_VERSION 1.5.0 - The client's  request args structure */
typedef struct flt_req {
	int		numFaults;
	fault_sev_t	sev;		/* Severity of faults  */
	fault_state_t	state;		/* State of faults */
	long		errorID;	/* the error ID */
} flt_req_t;

/* flt_updt is deprecated in API_VERSION 1.5.0 */

/* API functions */

/*
 * deprecated in API_VERSION 1.5.0. Use get_all_faults instead
 * Get all faults which match the request arguments.
 *
 * Parameters:
 *	flt_req_t fault_req		 attributes of faults
 *	sqm_lst_t **faults_list	 list of faults requested
 *
 * Comments on Usage:
 *	The fault_req struct fields have to be filled in with
 *	valid values when passing-in the struct to the routine.
 *	A "GET_ALL" (-1) for any field would denote that one
 *	does not care for that field: perform operation on ALL
 *	values of that field. Pls. see comment for the #define
 *	GET_ALL for more illustration.
 */
int get_faults(ctx_t *ctx, flt_req_t fault_req, sqm_lst_t **faults_list);


/*
 * Get all the faults in the system
 * Parameters:
 *	sqm_lst_t ** faults_list	list of faults
 */
int get_all_faults(ctx_t *ctx, sqm_lst_t **faults_list);

/*
 * Get faults by the lib. family setname.
 *
 * Parameters:
 *	uname_t library			Tape library family-set name
 *	sqm_lst_t **faults_list	list of faults requested
 */
int get_faults_by_lib(ctx_t *ctx, uname_t library,
    sqm_lst_t **faults_list);

/*
 * Get faults by eq#
 *
 * Parameters:
 *	equ_t eq			eq#
 *	sqm_lst_t **faults_list	list of faults requested
 */
int get_faults_by_eq(ctx_t *ctx, equ_t eq,
    sqm_lst_t **faults_list);

/* update_fault is deprecated in API_VERSION 1.5.0 */

/*
 * delete_faults deletes the faults identified by the errorID from the
 * FAULTLOG. If some faults are not found, -1 is returned but the
 * faults that could be found are deleted
 *
 * Parameters:
 * 	ctx_t   *ctx	context argument (only used by RPC layer)
 *	int	num	num of faults to be deleted
 * 	long errorID[]	error ids of faults to be deleted
 *
 */
int delete_faults(ctx_t *ctx, int num, long errorID[]);

/*
 * ack_faults acknowledges the faults identified by the errorID from the
 * FAULTLOG. If the state could not be changed to some of the faults
 * because the fault is not found, it is considered a partial success
 * but the return code is -1 with samerrmsg stating the errorids for
 * which the state could not be changed. The state of the faults that
 * could be found is changed to ACK
 *
 * Parameters:
 * 	ctx_t   *ctx	context argument (only used by RPC layer)
 *	int	num	num of faults to be ACK
 * 	long errorID[]	error ids of faults to be ACK
 *
 */
int ack_faults(ctx_t *ctx, int num, long errorID[]);

/*
 * Return the status of fault generation.
 * That is, is alerts is turned ON or OFF in defauts.conf
 *
 * Parameters:
 *	boolean_t *faults_gen_status	The value of the faults status
 */
int is_faults_gen_status_on(ctx_t *ctx, boolean_t *faults_gen_status);

/*
 * Return the fault summary, just the count of major, minor and critical
 * faults
 *
 * Parameters:
 *	ctx_t	 *ctx	context argument (only used by RPC layer)
 *	fault_summary_t	*summary (count of major, minor, critical faults)
 */
int get_fault_summary(ctx_t *ctx, fault_summary_t *fault_summary);

#endif /* _FAULTS_H */
