/*
 *  robots.h - for robot daemon
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

/*
 * $Revision: 1.16 $
 */

#if !defined(_ROBOTSD_H)
#define	_ROBOTSD_H

#include "aml/device.h"
#include "aml/shm.h"
#include "sam/types.h"


/*
 * identifers for  the children of robots.  All children are passed
 * the path to the main FIFO as argv[1], master shared memory segment
 * id as argv[2] and the preview shared memory segment id as argv[3].
 */

#define	  RAPID_CHNG_CMD  "sam-rapidchgd"
#define	  GENERIC_CMD	"sam-genericd"
#define	  GRAU_ACI_CMD    "sam-genericd"
#define	  STK_API_CMD	"sam-stkd"
#define	  IBM_ATL_CMD	"sam-ibm3494d"
#define	  SONY_API_CMD	  "sam-sonyd"
#define	  STK_SSI_CMD	"ssi.sh"	/* Needed if stk's defined */
#define	  RMT_SAM_SRV	"sam-serverd"
#define	  RMT_SAM_CLI	"sam-clientd"
#define	  HISTORIAN	"sam-historiand"


#define	  NUMBER_OF_ROBOT_TYPES   8	/* robots that need to be in table */
#define	  DEFAULT_RESTART_DELAY   15    /* time to wait to restart */
#define	  RESTART	 1
#define	  NO_RESTART	0

/* some structures  */

typedef struct {
	int	    type;		  /* type of device */
	int	    auto_restart;	  /* should it be auto-restarted */
	char	   *cmd;		  /* what command */
}robots_t;

/* Too keep track of the children, children are so hard to keep track of */
typedef struct {
	int	    pid;		/* pid of the child */
	int	    status;		/* exit status */
	int	    eq;			/* equipment number */
	dstate_t	oldstate;	/* state last time we looked */
	dev_ent_t	*device;	/* ptr into shm segment for device */
	robots_t	*who;		/* what robot */
	time_t	 start_time;		/* when did it start */
	time_t	 restart_time;		/* time to restart */
}rob_child_pids_t;

#if defined(ROBOTS_MAIN)

rob_child_pids_t *pids;

robots_t robots[NUMBER_OF_ROBOT_TYPES] = {
	DT_LMS4500,  RESTART, RAPID_CHNG_CMD,
	DT_GRAUACI,  RESTART, GRAU_ACI_CMD,
	DT_STKAPI,   RESTART, STK_API_CMD,
	DT_IBMATL,   RESTART, IBM_ATL_CMD,
	DT_SONYPSC,  RESTART, SONY_API_CMD,
	DT_PSEUDO_SC, RESTART, RMT_SAM_CLI,
	DT_PSEUDO_SS, RESTART, RMT_SAM_SRV
};
robots_t generic_robot = {DT_SCSI_R, RESTART, GENERIC_CMD};
#else
extern rob_child_pids_t *pids;
extern robots_t robots[];
extern robots_t generic_robot;
#endif

/* function prototypes  */

int build_pids(shm_ptr_tbl_t *);
void check_children(rob_child_pids_t *);
void kill_children(rob_child_pids_t *);
void reap_child(rob_child_pids_t *);
void start_a_process(rob_child_pids_t *);
void start_children(rob_child_pids_t *);

#endif  /* _ROBOTSD_H */
