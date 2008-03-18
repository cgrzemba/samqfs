/* mount_api.c - media mount api interface for third party api
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/file.h>
#include <unistd.h>

#include "aml/shm.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "thirdparty.h"
#include "aml/proto.h"

#pragma ident "$Revision: 1.17 $"

/* Some globals */
extern tp_dev_t  *what_device;
extern shm_alloc_t master_shm;


char *
sam_mig_mount_media(char *vsn,
               char *s_media)
{
  int tmp, err = 0;
  char *ent_pnt = "sam_mig_mount_media";
  char *path = NULL;
  media_t media;
  dev_ent_t  *un;
  robo_event_t   *event;
  sam_handle_t   *handle;
  dev_ptr_tbl_t   *dev_ptr_tbl;
  sam_resource_t *resource;
  tp_notify_mount_t *tp_mount;
  char tmp_dir[SAM_MIG_TMP_DIR_LEN];
  char tmp_pfx[SAM_MIG_PFX_LEN];
      
  if (vsn == NULL || strlen(vsn) > sizeof(vsn_t) ||
      s_media == NULL || (tmp = nm_to_device(s_media)) == -1 ||
      what_device == NULL)
    {
      sam_syslog(LOG_ERR, "%s:Return from mount with invalid request.", ent_pnt);
      errno = EINVAL;
      return(NULL);
    }

  if(DBG_LVL (SAM_DBG_MIGKIT))
      sam_syslog(LOG_DEBUG, "%s: media %s, vsn %s", ent_pnt, s_media, vsn);
  
      /* LINTED pointer cast may result in improper alignment */
  dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);
  
  media = tmp;
  handle = (sam_handle_t *)malloc_wait(sizeof(*handle), 5, 0);
  resource = (sam_resource_t *)malloc_wait(sizeof(*resource), 5, 0);
  memset (handle, 0, sizeof(*handle));
  memset (resource, 0, sizeof(*resource));

  event = malloc_wait(sizeof(robo_event_t), 2, 0);
  memset(event, 0, sizeof(robo_event_t));
  mutex_init(&event->mutex, USYNC_THREAD, NULL);
  event->completion = REQUEST_NOT_COMPLETE;
  event->status.bits = REST_SIGNAL;
  mutex_lock (&event->mutex);

  handle->fseq = what_device->eq;
  handle->fifo_cmd.ptr = (void *)event;
  handle->pid = getpid();
  handle->gid = getgid();
  handle->uid = getuid();
  resource->access = FREAD;
  memccpy (&resource->archive.vsn, vsn, '\0', sizeof(vsn_t));
  resource->archive.rm_info.media = media;
  resource->archive.rm_info.valid = TRUE;
      /* Fake a fs mount request */
  if (add_preview_fs (handle, resource, PRV_MIG_PRIO_DEFAULT,
                      CB_NOTIFY_TP_LOAD))
    {
      err = errno;
      sam_syslog(LOG_ERR, "%s: add_preview_fs() failed: %m", ent_pnt);
      goto clean_up;
    }

      /* Wait for media to be mounted */
  while(event->completion == REQUEST_NOT_COMPLETE)
    cond_wait (&event->condit, &event->mutex);
  
  mutex_unlock (&event->mutex);
  free(resource);
  free(handle);
  mutex_destroy (&event->mutex);
  cond_destroy (&event->condit); 
      /* Get information from the mount */
  tp_mount =
    (tp_notify_mount_t *)&event->request.message.param.tp_notify_mount;

  if (tp_mount->errflag != 0)
    {
      int err = tp_mount->errflag;
      sam_syslog(LOG_ERR, "%s: Return from mount with err %s.", ent_pnt,
             error_handler(err));
      free(event);
      errno = err;
      return(NULL);
    }
  
      /* Do some sanity checking */
  if (tp_mount->eq <= 0 || (int) tp_mount->eq > dev_ptr_tbl->max_devices ||
      dev_ptr_tbl->d_ent[tp_mount->eq] == 0)
    {
      sam_syslog(LOG_ERR, "%s:Return from mount with illegal equipment.", ent_pnt);
      free(event);
      errno = EFAULT;
      return(NULL);
    }

  un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[tp_mount->eq]);
  if (un->active != 1 && DBG_LVL (SAM_DBG_DEBUG))
    sam_syslog(LOG_DEBUG, "%s: Active count not 1.", ent_pnt);

  mutex_destroy (&event->mutex);
  cond_destroy (&event->condit); 
  free(event);

  sprintf(tmp_dir, "%s/sam-migd-%d", SAM_MIG_TMP_DIR, what_device->eq);
  sprintf(tmp_pfx, "%d-", tp_mount->eq);
  if ((path = tempnam (tmp_dir, tmp_pfx)) == NULL)
    {
      err = errno;
      sam_syslog(LOG_ERR, "%s: tempnam: %m.", ent_pnt);
      errno = err;
      return(NULL);
    }

  if (symlink (un->name, path))
    {
      err = errno;
      sam_syslog(LOG_ERR, "%s: symlink: %m.", ent_pnt);
      free(path);
      errno = err;
      return(NULL);
    }

  if(DBG_LVL (SAM_DBG_MIGKIT))
      sam_syslog(LOG_DEBUG, "%s: mounted at %s", ent_pnt, path);
  return(path);
  
clean_up:
  mutex_unlock (&event->mutex);      
  mutex_destroy (&event->mutex);
  cond_destroy (&event->condit); 
  free(event);
  free(handle);
  free(resource);
  errno = err;
  return(NULL);
}

int
sam_mig_release_device(char *device)
{
  char  *path;
  char  *ent_pnt = "sam_mig_release_device";
  dev_ent_t  *un;
  dev_ptr_tbl_t   *dev_ptr_tbl;

  if (device == NULL)
  {
      sam_syslog(LOG_DEBUG, "%s: NULL device", ent_pnt);
      return(-1);
  }
  
  if(DBG_LVL (SAM_DBG_MIGKIT))
      sam_syslog(LOG_DEBUG, "%s: device %s", ent_pnt, device);
  
      /* LINTED pointer cast may result in improper alignment */
  dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
    ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);
  
  path = (char *)malloc_wait(MAXPATHLEN, 5, 0);
  memset(path, 0, MAXPATHLEN);
  if (readlink (device, path, MAXPATHLEN) < 0)
    {
      sam_syslog(LOG_ERR, "%s: readlink: %m", ent_pnt);
      free(path);
      return(-1);
    }
  
  un = (dev_ent_t *)
    SHM_REF_ADDR( ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
      
  for (;un != NULL ; un = (dev_ent_t *)SHM_REF_ADDR(un->next))
    {
      if (strcmp(un->name, path) == 0)
        break;
    }

  if (un == NULL)
    {
      sam_syslog(LOG_ERR, "%s:Unable to find device %s:%s.",
             ent_pnt, device, path);
      free(path);
      return(-1);
    }


  if(unlink(device) && DBG_LVL (SAM_DBG_DEBUG)) 
    sam_syslog(LOG_DEBUG, "%s: unlink: %m.", ent_pnt);

  free(path);
  mutex_lock(&un->mutex);
  DEC_ACTIVE(un);
  check_preview(un);
  mutex_unlock(&un->mutex);  
  return(0);
}

/*
  Local variables:
  eval:(gnu-c-mode)
  End:
 */
