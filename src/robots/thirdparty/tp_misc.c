/* tp_misc.c - misc routines for thirdparty devices.
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

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "aml/shm.h"
#include "aml/trace.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "thirdparty.h"

#pragma ident "$Revision: 1.16 $"

/* some globals */
extern shm_alloc_t master_shm, preview_shm;

/* Build a third party stage struct from a todo request */
tp_stage_list_t *
build_stage_entry (tp_dev_t *tp_dev,
                   robo_event_t *event)
{
  char *ent_pnt = "build_stage_entry";
  register todo_request_t *todo;
  tp_stage_list_t *entry;

  todo = &(event->request.message.param.todo_request);

  entry = (tp_stage_list_t *)malloc_wait (sizeof(*entry), 3, 0);
  memset (entry, 0, sizeof(*entry));
  entry->tp_dev = tp_dev;
  entry->fd = -1;
  entry->tp_stage.offset = todo->resource.archive.rm_info.file_offset;
  entry->tp_stage.size = todo->resource.archive.rm_info.size;
  entry->tp_stage.position = todo->resource.archive.rm_info.position;
  entry->tp_stage.inode = todo->handle.id.ino;
  memccpy (entry->tp_stage.vsn, todo->resource.archive.vsn, '\0',
           sizeof(vsn_t) - 1);
  entry->handle = todo->handle;
  entry->tp_stage.fseq =  todo->handle.fseq;
  entry->tp_stage.media_type = tp_dev->un->equ_type;
  return (entry);
}

/* disp_of_event - dispose of event
 *
 * clean up event
 */

void
disp_of_tp_event(tp_dev_t *tp_dev,
                 robo_event_t *event,
                 int completion)
{
  char    tos;
  char   *ent_pnt = "disp_of_tp_event";

      /* send condition signal if requested. */
      
  if(event->status.b.sig_cond)     
    {
      if (event->status.b.free_mem && DBG_LVL (SAM_DBG_DEBUG))
        sam_syslog(LOG_DEBUG, "%s: signal and free.", ent_pnt);

      ETRACE((LOG_NOTICE, "DsTpEv:%#x:Signal", event)); 
      mutex_lock(&event->mutex);
          /* if not changed, then it must be ok */
      if (event->completion == REQUEST_NOT_COMPLETE)
        event->completion = completion;
      else
        write_event_exit(event, completion, NULL);
      cond_signal(&event->condit);      /* signal the waiting thread */
      mutex_unlock(&event->mutex);
      return;                           /* can't free signaled event */
    }

  /* ensure an exit response is sent */
  write_event_exit(event, completion, NULL);
 
      /* disp_of_event no longer puts stuff back on the free list */
  if (event->status.b.free_mem && ((void *)event > (void *)&tos))
    {
      if (DBG_LVL (SAM_DBG_DEBUG))
        sam_syslog(LOG_INFO, "%s: free in stack %#x, %#x.", ent_pnt,
             event, &tos);
      return;
    }

  if(event->status.b.free_mem)          /* if the event was malloc'ed and */
    {                                   /* the caller wants us to free it */
      mutex_destroy (&event->mutex);    /* destroy mutex  */
      cond_destroy (&event->condit);    /* and condition before */
      free(event);                      /* freeing the memory */
      ETRACE((LOG_NOTICE, "DsTpEv %#x Free",event)); 
    }
  
  return;
}
