/* message.c - process incomming messages.
 */
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

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "aml/shm.h"
#include "aml/message.h"
#include "aml/trace.h"
#include "thirdparty.h"
#include "aml/proto.h"
#include "sam/lib.h"

#pragma ident "$Revision: 1.16 $"


/* function prototypes */

void *inc_free(void *);
static void sig_catch();

/* some globals */
extern shm_alloc_t master_shm, preview_shm;

/* Used with tp_stage_req to call TP_API stage function */
tp_issue_list_t   tp_issue_list;

/* Signal catcher to exit thread */
static void
sig_catch()
{
  thr_exit((void *)NULL);
}

/* monitor_msg - thread routine to monitor messages.
 */
void *
tp_monitor_msg(void *vtp_dev)
{
  int  len;
  char *ent_pnt = "tp_monitor_msg";
  sigset_t    signal_set;
  tp_dev_t   *tp_dev = (tp_dev_t *)vtp_dev;
  robo_event_t      *current_event;
  struct sigaction  sig_action;
  message_request_t *message, shutdown;
  enum sam_mess_type mtype;

      /* dummy up a shutdown message */
  (void)memset(&shutdown, 0, sizeof(message_request_t));
  (void)memset (&sig_action, 0, sizeof (struct sigaction));

      /* Zeroing the struct set mutexs and conditions to thread locking */
  memset (&tp_issue_list, 0, sizeof(tp_issue_list));
  shutdown.mtype = MESS_MT_SHUTDOWN;
      /*LINTED constant truncated by assignment */
  shutdown.message.magic = MESSAGE_MAGIC;
  shutdown.message.command = MESS_CMD_SHUTDOWN;
  
      /* Should have been called with all signals blocked, now
       * let sigemt be delivered and just exit when it is.
       */

  sig_action.sa_handler = sig_catch;
  sig_action.sa_flags = 0;
  (void)sigemptyset(&signal_set); 
  (void)sigaddset(&signal_set, SIGEMT);
  (void)sigaction (SIGEMT, &sig_action, (struct sigaction *)NULL);
  (void)thr_sigsetmask(SIG_UNBLOCK, &signal_set, NULL);

  mutex_lock(&tp_dev->mutex);           /* wait for initialize */
  mutex_unlock(&tp_dev->mutex);

  tp_issue_list.tp_dev = tp_dev;
  
  message = (message_request_t *)SHM_REF_ADDR(tp_dev->un->dt.tr.message);

      /* Main loop */
  for (;;)
    {
      current_event = tp_get_free_event(tp_dev);
 
          /* Zeroing the struct has the effect of initializing the
           * mutex and the condition to USYNC_THREAD, just what we want.
           */
      (void)memset (current_event, 0, sizeof(robo_event_t));
      current_event->status.bits =  REST_FREEMEM;
      current_event->type = EVENT_TYPE_MESS;
      
          /* Wait for a message */
      mutex_lock (&message->mutex);
      while (message->mtype == MESS_MT_VOID)
        cond_wait (&message->cond_r, &message->mutex);

          /* copy the request into the event */

      if (message->mtype == MESS_MT_SHUTDOWN)
        {
          if (DBG_LVL (SAM_DBG_DEBUG)) 
            sam_syslog(LOG_DEBUG, "%s: shutdown request:%s:%d.", ent_pnt,
                   __FILE__, __LINE__);
          thr_exit((void *)NULL);
        }

      memcpy(&(current_event->request.message), &(message->message),
             sizeof(sam_message_t));
      mtype = message->mtype;
      message->mtype = MESS_MT_VOID;    /* release the message area */
      message->message.exit_id.pid = 0;
      cond_signal (&message->cond_i);   /* and wake up anyone waiting */
      mutex_unlock (&message->mutex);
      switch (current_event->request.message.command)
        {
        case MESS_CMD_TODO:
        {
          register todo_request_t *todo;
          
          todo = &(current_event->request.message.param.todo_request);
          if(DBG_LVL(SAM_DBG_TMESG))
            sam_syslog(LOG_DEBUG, "MR:tp_todo %d", todo->sub_cmd);
          switch (todo->sub_cmd)
            {
            case TODO_ADD:
              current_event->next = NULL;
              mutex_lock (&tp_issue_list.mutex);
              if (tp_issue_list.count++ == 0) /* if no entries on list */
                tp_issue_list.next = current_event; /* set up first */
              else
                tp_issue_list.last->next = current_event; /* add to end */

              tp_issue_list.last = current_event;
              cond_signal (&tp_issue_list.cond); /* signal tp_add_stage */
              mutex_unlock (&tp_issue_list.mutex);
              break;

            case TODO_CANCEL:
              break;

            default:
              if (DBG_LVL (SAM_DBG_DEBUG))
                sam_syslog(LOG_DEBUG, "%s: Unknown todo subcmd %#x", ent_pnt,
                       todo->sub_cmd);
              disp_of_tp_event (tp_dev, current_event, EINVAL);
              break;
            }
        }
        break;

        case MESS_CMD_TP_MOUNT:
        {
          robo_event_t *old_event;
          register tp_notify_mount_t  *tp_notify;

          tp_notify = &(current_event->request.message.param.tp_notify_mount);
          old_event = (robo_event_t *)tp_notify->event;
          if(DBG_LVL(SAM_DBG_TMESG))
            sam_syslog(LOG_DEBUG, "MR:tp_mount: %#x", old_event);

          /* Check if old_event was generated by us */
          if(old_event->tstamp < tp_dev->start_time)
          {
              /* We probably got event from the previous crashed
               * instance of sam-migd. Ignore it.
               */
              if (DBG_LVL (SAM_DBG_DEBUG))
                  sam_syslog(LOG_DEBUG,
                         "%s: Got old event: event stamp %d, start time %d",
                         ent_pnt, old_event->tstamp, tp_dev->start_time);
              disp_of_tp_event (tp_dev, current_event, EFAULT);
              break;
          }
          
              /* get the response data copied over to the original event. */
          memcpy (&old_event->request.message.param.tp_notify_mount,
                  tp_notify, sizeof(tp_notify_mount_t));
          disp_of_tp_event (tp_dev, old_event, 0); /* signal the requester */
          disp_of_tp_event (tp_dev, current_event, 0); 
        }
        break;
          
        default:
          if (DBG_LVL (SAM_DBG_DEBUG))
            sam_syslog(LOG_DEBUG, "%s: Unknown message command %#x", ent_pnt,
                   current_event->request.message.command);
          disp_of_tp_event (tp_dev, current_event, EINVAL);
          break;
        }
    }
}

robo_event_t *
tp_get_free_event(tp_dev_t *tp_dev)
{
  char *ent_pnt = "tp_get_free_event";
  char *e_mess = tp_dev->un->dis_mes[DIS_MES_NORM];
  robo_event_t  *ret;
  
  mutex_lock (&tp_dev->free_mutex);
  if (tp_dev->free_count < 40 && !tp_dev->inc_free_running)
    {
      sigset_t   signal_set;
      (void)sigemptyset(&signal_set); 
      (void)sigaddset(&signal_set, SIGEMT);
      tp_dev->inc_free_running++;
      thr_sigsetmask (SIG_BLOCK, &signal_set, NULL);
      thr_create (NULL, MD_THR_STK, &inc_free, (void *)tp_dev,
                  THR_DETACHED, NULL);
      thr_sigsetmask (SIG_UNBLOCK, &signal_set, NULL);
      thr_yield();
    }
  
  while (tp_dev->free_count <= 0)
    {
      mutex_unlock(&tp_dev->free_mutex);
      if (DBG_LVL (SAM_DBG_DEBUG))
        {
          sam_syslog(LOG_INFO, "%s: Unable to obtain free event.", ent_pnt);
          memccpy (e_mess, "unable to obtain free event", '\0', DIS_MES_LEN);
          sleep(2);
          *e_mess = '\0';
        }
      else
        sleep(2);
    }

  ret = tp_dev->free;
  ETRACE((LOG_NOTICE, "EV:LfGf: %#x.", ret));
  tp_dev->free_count--;
  tp_dev->free = ret->next;
  mutex_unlock (&tp_dev->free_mutex);
  ret->tstamp = time(NULL);
  return(ret);
}

/* inc_free - Get a new batch of events for the device free list.
 * Runs as a thread.
 */

void *inc_free(void *vtp_dev)
{
  char *ent_pnt = "inc_free";
  tp_dev_t     *tp_dev = (tp_dev_t *)vtp_dev;
  robo_event_t  *new, *end;

  new = init_list(ROBO_EVENT_CHUNK << 1);
#if defined(DEBUG)
  if (DBG_LVL (SAM_DBG_DEBUG))
    sam_syslog(LOG_DEBUG, "%s: Filling free list.", ent_pnt);
#endif 
  mutex_lock(&tp_dev->free_mutex);
  if (tp_dev->free == NULL)
    tp_dev->free = new;
  else
    {
      for (end = tp_dev->free; end->next != NULL; end = end->next);
      end->next = new;
      new->previous = end;
    }
  tp_dev->free_count += ROBO_EVENT_CHUNK;
  tp_dev->inc_free_running = 0;
  mutex_unlock(&tp_dev->free_mutex);
  thr_exit((void *)NULL);
}
