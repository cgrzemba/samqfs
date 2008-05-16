/*
 * robots.h - structs and defines for robots.
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

#if !defined(_AML_ROBOTS_H)
#define	_AML_ROBOTS_H

#pragma ident "$Revision: 1.18 $"

#if !defined(_KERNEL)

#include "sam/types.h"
#include "aml/preview.h"
#include "aml/fifo.h"
#include "aml/message.h"

#if defined(DEBUG)
void _Sanity_check(char *SrcFile, int SrcLine);
#define	SANITY_CHECK(_expr)	{ if (!(_expr)) \
	_Sanity_check(_SrcFile, __LINE__); }
#else
#define	SANITY_CHECK(_expr)   {}
#endif



/* Misc constants */

#define	ROBOT_PRIVATE_AREA	1024	/* Size of the private shm area */
#define	INITIAL_MOUNT_TIME	30	/* Seconds before newly mounted */
					/* Media is available for dismount */
#define	IMPORT_WAIT_TIME	(60*1)	/* Allow 1 minutes for import */

/* Some bar code constants */

#define	CLEANING_BAR_LEN	3	/* Length of CLEANING_BAR_CODE */
#define	CLEANING_BAR_CODE	"CLN"	/* Prefix for a cleaning cartridge */
#define	CLEANING_FULL_LEN	5
#define	CLEANING_FULL_CODE	"CLEAN"


/* commands for internal robot requests */

typedef enum robot_intrl {

	/* Requests to the transport */
	ROBOT_INTRL_MOVE_MEDIA = 0x01,
	ROBOT_INTRL_EXCH_MEDIA,
	ROBOT_INTRL_LOAD_MEDIA,		/* used by GRAU/STK/IBM */
	ROBOT_INTRL_FORCE_MEDIA,	/* used by GRAU/STK/IBM/SONY */
	ROBOT_INTRL_DISMOUNT_MEDIA,	/* used by GRAU/STK/IBM/SONY */
	ROBOT_INTRL_VIEW_DATABASE,	/* used by GRAU/STK/IBM */
	ROBOT_INTRL_DRIVE_ACCESS,	/* used by GRAU */
	ROBOT_INTRL_GET_SIDE_INFO,	/* used by GRAU */
	ROBOT_INTRL_QUERY_DRIVE,	/* used by STK/IBM/SONY */
	ROBOT_INTRL_QUERY_ALL_DRIVES,	/* used by STK */
	ROBOT_INTRL_QUERY_MNT_STATUS,	/* used by STK */
	ROBOT_INTRL_SET_CATEGORY,	/* used by IBM */
	ROBOT_INTRL_QUERY_LIBRARY,	/* used by IBM */
	ROBOT_INTRL_EJECT_MEDIA,	/* used by STK */

	/* Requests for the drives */
	ROBOT_INTRL_START_AUDIT,
	ROBOT_INTRL_AUDIT_SLOT,
	ROBOT_INTRL_MOUNT,
	ROBOT_INTRL_FLIPIT,

	/* Requests for all */
	ROBOT_INTRL_INIT,
	ROBOT_INTRL_SHUTDOWN = 0xff
} robot_intrl_t;

/*
 * If the noerror flag is set, then do not down the robot for "empty"
 * type errors.  If the robot is a metrum d-360, then issue a move
 * to the first storage slot if there was an error.  This is susposed
 * to turn off the alarm.  If the second move fails, then down the library.
 * If the waitready flag is set, wait for the library to become ready, if
 * it reports not ready, becoming-ready.
 */

/* Internal request structure */
typedef struct {
	robot_intrl_t	command;	/* command */
	uint_t		slot;		/* slot number */
	uint_t		part;		/* partition */
	uint_t		transport;	/* transport to use */
	uint_t		source;		/* element address of source */
	uint_t		destination1;	/* destination address of first */
	uint_t		destination2;	/* destination address of second */
	uint_t		sequence;	/* if from preview, the sequence # */
	void		*address;	/* generic address, command dependent */
	union {
		struct {
			ushort_t
#if defined(_BIT_FIELDS_HTOL)
				invert1	: 1,
				invert2	: 1,
				quick	: 1,	/* Used for quick shutdown */
				noerror	: 1,

				audit_eod : 1,	/* Find eod when auditing */
				waitready : 1,

				unused	:10;
#else /* defined(_BIT_FIELDS_HTOL) */
				unused	:10,

				waitready : 1,
				audit_eod : 1,	/* Find eod when auditing */

				noerror	: 1,
				quick	: 1,	/* Used for quick shutdown */
				invert2	: 1,
				invert1	: 1;
#endif  /* defined(_BIT_FIELDS_HTOL) */
		} b;
		ushort_t  bits;
	} flags;
} robot_internal_t;


#define	ROBO_EVENT_CHUNK	150	/* Number of entries to malloc */
					/* at a time. */

typedef enum event_type {
	EVENT_TYPE_INTERNAL = 1,
	EVENT_TYPE_MESS
} event_type_t;

/*
 * Events used by robots for worklist.  Each element in the robot has
 * a list of events to work on.  Removal/insertition of enties requires
 * locking of the entire list(the structure containing the pointer to
 * the first entry).  The list manipulation routines assume that the
 * list is locked.  Do not lock an entry without first locking the entire
 * list, failure to do so could cause a deadlock.  The mutex's and cond
 * for entries on the free list are not active.  The free list is locked
 * using the free list mutex.
 */

typedef struct robo_event_s {
	mutex_t		mutex;		   /* Not used when event in on list */
	cond_t		condit;		  /* all entries are lock out when */
					/* the mutex for the list is locked */
	int		completion;
	time_t		tstamp;		  /* timestamp when event created  */
	event_type_t	type;    /* type of event */
	union {
		struct {
			uint_t
#if defined(_BIT_FIELDS_HTOL)
				dont_reque: 1,	/* Dont put entry back on a */
						/* worklist */
				delayed   : 1,	/* Delayed entry, check time */
				sig_cond  : 1,	/* Signal condition when done */
				free_mem  : 1,	/* Issue free on the event */
						/* when done */
				unused    :28;
#else /* defined(_BIT_FIELDS_HTOL) */
				unused    :28,
				free_mem  : 1,	/* Issue free on the event */
						/* when done */
				sig_cond  : 1,	/* Signal condition when done */
				delayed   : 1,	/* Delayed entry, check time */
				dont_reque:1;	/* Dont put entry back on a */
						/* worklist */
#endif  /* defined(_BIT_FIELDS_HTOL) */
		} b;
		uint_t	bits;
	} status;
	union {
		robot_internal_t internal;	/* Internaly generated work */
		sam_message_t	message;	/* Message recieved */
	} request;
	time_t		timeout;	/* When delayed entry times out */
	struct robo_event_s *next;	/* Pointer to next event */
	struct robo_event_s *previous;	/* Pointer to previous event */
} robo_event_t;

#define	REST_DONTREQ	0x80000000
#define	REST_DELAYED	0x40000000
#define	REST_SIGNAL	0x20000000
#define	REST_FREEMEM	0x10000000

#else
error
#endif /* !defined(_KERNEL) */

#endif  /* !defined(_AML_ROBOTS_H) */
