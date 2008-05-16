/* thirdparty.h - structs and such for thirdpary daemons.
 *
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
 * $Revision: 1.13 $
 */

#if !defined(_THIRDPARTY_H)
#define _THIRDPARTY_H

#include "aml/device.h"
#include "aml/preview.h"
#include "../lib/librobots.h"
#include "pub/mig.h"

#define REQUEST_NOT_COMPLETE   -100     /* used for request completion */

/* Constants for symlinks created by sam_mig_mount_media() to point
 * to the real device. The name for symlink is constructed by tempnam()
 * with the directory name "SAM_MIG_TMP_DIR/sam-migd-<migd-eq>" and prefix
 * "<device_eq>-" . Length based on eq being short and at most 5 characters
 * long when represented by string.
 */

#define SAM_MIG_TMP_DIR       "/usr/tmp"
#define SAM_MIG_TMP_DIR_LEN   (sizeof(SAM_MIG_TMP_DIR) + 20)
#define SAM_MIG_PFX_LEN       10


    /* The list of stage requests for this third party device are kept
     * on a linked list.  If the prev entry is NULL this is the head of
     * the list.  If next is NULL, this is the tail of the list.
     *
     * You must be holding the stage_mutex to link and unlink these
     * entries from the list.
     */
typedef struct tp_stage_list_t{
  tp_stage_t  tp_stage;                 /* must be first */
  int    fd;                            /* stage fd (if staging) */
  union{
    struct {
      u_int
#if	defined(_BIT_FIELDS_HTOL)
		swrite :1,                 /* first swrite issued */

        unused :31;
#else	/* defined(_BIT_FIELDS_HTOL) */
        unused :31,

		swrite :1;                 /* first swrite issued */
#endif  /* defined(_BIT_FIELDS_HTOL) */
    }b;
    u_int  bits;
  }flags;
  sam_handle_t    handle;               /* handle from request */
  struct tp_dev_t *tp_dev;              /* pointer to the device */
  struct tp_stage_list_t *prev;         /* previous stage request */
  struct tp_stage_list_t *next;         /* next stage request */
}tp_stage_list_t;

#define  TP_STAGE_SWRITE 0x80000000

typedef struct tp_dev_t{
  mutex_t   mutex;                      /* main mutex */
  equ_t     eq;                         /* my equipment number */
  dev_ent_t *un;                        /* my device entry */

      /* Don't change order unless you also change initialize */
  int (*init_func)(int);                /* address of tp init function */
  int (*stage_func)(tp_stage_list_t *); /* address of tp stage function */
  int (*cancel_func)(tp_stage_list_t *); /* address of tp cancel function */
      /* stage list management information */
  mutex_t         stage_mutex;          /* mutex for stage list management */  
  tp_stage_list_t *head;                /* first on list */
      /* message list management information */
  mutex_t         list_mutex;           /* mutex for list management */
  cond_t          list_condit;          /* for list management */
  mutex_t         free_mutex;           /* mutex for the free list */
  int             inc_free_running;
  int             free_count;           /* count of events on free list */
  int             active_count;         /* count of events on the active list*/
  robo_event_t    *free;                /* free list of events */
  time_t          start_time;           /* time this instance started  */
}tp_dev_t;

/* List of stage request received from the message area */

typedef struct {
  mutex_t    mutex;                     /* mutex to lock list */
  cond_t     cond;
  int        count;                     /* Number of entries on the list */
  tp_dev_t   *tp_dev;
  robo_event_t  *next;                  /* pointer to the next entry */
  robo_event_t  *last;                  /* pointer to the last entry */
}tp_issue_list_t;

robo_event_t *tp_get_free_event(tp_dev_t *);
tp_stage_list_t *build_stage_entry (tp_dev_t *, robo_event_t *);
void *tp_monitor_msg(void *);
void disp_of_tp_event (tp_dev_t *, robo_event_t *, int);

/*
  Local variables:
  eval:(gnu-c-mode)
  End:
 */

#endif /* _THIRDPARTY_H */
