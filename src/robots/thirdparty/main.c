/* main.c - main routine for the third party daemon.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/param.h>
#include <sys/shm.h>
#include <dirent.h>
#include <unistd.h>

#define MAIN

#include "aml/shm.h"
#include "sam/defaults.h"
#include "sam/devnm.h"
#include "aml/logging.h"
#include "thirdparty.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catlib.h"
#include "pub/mig.h"

#pragma ident "$Revision: 1.21 $"

/* function prototypes */
static int tp_clean(tp_dev_t *tp_dev, dev_ptr_tbl_t *dev_ptr_tbl);
static int tp_initialize(tp_dev_t *, dev_ptr_tbl_t *);

/* some globals */

const char   *program_name = "sam-migd";
mutex_t  lock_time_mutex;                /* for mktime */
tp_dev_t     *what_device;              /* needed by the api support */
shm_alloc_t  master_shm, preview_shm;

int
main(
int argc,
char **argv)
{
  int   what_signal, i, *tmp;
  char  logname[20];
  char  *e_mess, *ec_mess;
  tp_dev_t *tp_dev;
  sigset_t  signal_set, full_block_set;
  struct sigaction  sig_action;
  dev_ptr_tbl_t     *dev_ptr_tbl;
  shm_ptr_tbl_t     *shm_ptr_tbl;
  sam_defaults_t    *defaults;

  tp_dev = (tp_dev_t *)malloc_wait (sizeof(tp_dev_t), 4, 0);
  (void)memset (tp_dev, 0, sizeof(tp_dev_t));
  (void)memset (&lock_time_mutex, 0, sizeof(mutex_t));
  if(argc != 4 )
    exit(1);

  what_device = tp_dev;
  argv++;
  master_shm.shmid = atoi(*argv);
  argv++;
  preview_shm.shmid = atoi(*argv);
  argv++;
  tp_dev->eq = atoi(*argv);
  sprintf(logname, "%s-%d", program_name, tp_dev->eq);
  program_name = logname;
  
  open("/dev/null", O_RDONLY);          /* stdin */
  open("/dev/null", O_RDONLY);          /* stdout */
  open("/dev/null", O_RDONLY);          /* stderr */
  CustmsgInit(1, NULL);
  if((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774)) ==
     (void *)-1)
    exit(2);

  shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
  if((preview_shm.shared_memory = shmat(preview_shm.shmid, NULL, 0774)) ==
     (void *)-1)

    exit(3);

  defaults = GetDefaults();

  dev_ptr_tbl = (dev_ptr_tbl_t *)
    SHM_REF_ADDR(((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);

      /* LINTED pointer cast may result in improper alignment */
  tp_dev->un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[tp_dev->eq]);

  e_mess = tp_dev->un->dis_mes[DIS_MES_NORM];
  ec_mess = tp_dev->un->dis_mes[DIS_MES_CRIT];   
  if (DBG_LVL(SAM_DBG_RBDELAY))
  {
    int wait = 60;

    sam_syslog(LOG_DEBUG, "Waiting for 60 seconds.");
    while (wait > 0 && DBG_LVL (SAM_DBG_RBDELAY))
      {
        sprintf(ec_mess, "waiting for %d seconds pid %d", wait, getpid());
        sleep(10);
        wait -= 10;
      }
    *ec_mess = '\0';
  }

  sigfillset(&full_block_set);          /* used to block all signals */
  
  sigemptyset(&signal_set);             /* signals to except. */
  sigaddset(&signal_set, SIGINT);       /* during sigwait */
  sigaddset(&signal_set, SIGHUP);
  sigaddset(&signal_set, SIGALRM);

  sig_action.sa_handler = SIG_DFL;      /* want to restart system calls */
  sigemptyset(&sig_action.sa_mask);     /* on excepted signals. */
  sig_action.sa_flags = SA_RESTART;

  sigaction(SIGINT, &sig_action, NULL);
  sigaction(SIGHUP, &sig_action, NULL);
  sigaction(SIGALRM, &sig_action, NULL);
  
      /* The default mode is to block everything */
  thr_sigsetmask(SIG_SETMASK, &full_block_set, NULL);
  mutex_init(&tp_dev->mutex, USYNC_THREAD, 0);

      /* grab the lock and hold it until initialization is complete */
  mutex_lock(&tp_dev->mutex);
      /* start the main threads */
  if( thr_create(NULL, MD_THR_STK, tp_monitor_msg, (void *)tp_dev,
                 (THR_BOUND | THR_NEW_LWP | THR_DETACHED), NULL))
    {
      sam_syslog(LOG_CRIT, "Unable to create thread monitor_msg: %m.\n");
      exit(4);
    }

  mutex_lock(&tp_dev->un->mutex);
  tp_dev->un->type = tp_dev->un->equ_type;
  tp_dev->un->status.b.ready = FALSE;
  tp_dev->un->status.b.present = FALSE;
  mutex_unlock(&tp_dev->un->mutex);

  /* Initialize the catalog. */
  if (CatalogInit("sam-migd") == -1) {
		syslog(LOG_ERR,"%s",
			catgets(catfd, SET, 2364, "Catalog initialization failed!"));
		mutex_unlock(&tp_dev->mutex);         
		exit(6);
  }

      /* Initialize the device. */
      /*  If errors here, clear the lock and exit. */

  if ( tp_initialize(tp_dev, dev_ptr_tbl) != 0 )  {
      mutex_unlock(&tp_dev->mutex);         
      exit(5);
  }

      /* Invoke the third party initialize routine */
  tp_dev->init_func(defaults->stages);
  mutex_unlock(&tp_dev->mutex);         /* release the mutex and stand back */
  thr_yield();                          /* let the other threads run */
  mutex_lock(&tp_dev->un->mutex);
  tp_dev->un->status.bits |= (DVST_READY | DVST_PRESENT);
  tp_dev->un->status.bits &= ~DVST_REQUESTED;
  mutex_unlock(&tp_dev->un->mutex);

  for (;;)
    {
      alarm(20);
      what_signal = sigwait(&signal_set); /* wait for a signal */
      switch(what_signal)               /* process the signal */
        {
        case SIGALRM:
          break;
            
        case SIGINT:
          sam_syslog(LOG_INFO, "%s: Shutdown by signal %d", program_name,
		what_signal);
          exit(0);
          break;
 
        case SIGHUP:
          sam_syslog(LOG_INFO, "%s: Shutdown by signal %d", program_name,
		what_signal);
          exit(0);
          break;
          
        default:
          break;
        }
    }
      /* LINTED Function has no return statement */
}

int
tp_initialize (tp_dev_t *tp_dev,
            dev_ptr_tbl_t *dev_ptr_tbl)
{
  int   err = 0;
  int   (**func)();
  char  *tp_so_library = NULL, *t_str;
  char  **ent_pnt;
  char  in_line[MAXPATHLEN];
  char  *ent_pnts[] = {
    "usam_mig_initialize", "usam_mig_stage_file_req", "usam_mig_cancel_stage_req", NULL
  };
  void  *api_handle;
  FILE *conf;

  /* Clean after the crashed sam-migd */
  if(tp_clean(tp_dev, dev_ptr_tbl) != 0)
      return(-1);
  
  memset (in_line, 0, DIS_MES_LEN);
  sprintf(in_line, " ");
  memccpy(tp_dev->un->dis_mes[DIS_MES_CRIT], in_line, '\0', DIS_MES_LEN);
  
  tp_so_library = strdup(tp_dev->un->name);

  func = &tp_dev->init_func;
      /* Find the entry points */
  if ((api_handle = dlopen(tp_so_library, RTLD_NOW | RTLD_GLOBAL)) == NULL)
    {
      memset (in_line, 0, DIS_MES_LEN);
      sprintf(in_line, catgets(catfd,SET,9254,"Error in configuration file"));
      memccpy(tp_dev->un->dis_mes[DIS_MES_CRIT], in_line, '\0', DIS_MES_LEN);
      sam_syslog(LOG_ERR,
             catgets(catfd,SET,2483,
                     "The shared object library %s cannot be loaded: %s."), 
             tp_so_library, dlerror());
      free (tp_so_library);
      return(-1);
    }

  for (ent_pnt = &ent_pnts[0]; *ent_pnt != NULL; ent_pnt++, func++)
    {
      *func = (int (*)())dlsym(api_handle, *ent_pnt);
      if (*func == NULL)
        {
          sam_syslog(LOG_ERR, catgets(catfd,SET,1049,
                                  "Error mapping symbol -%s-:%s."),
                 *ent_pnt, dlerror());
          err = 1;
        }
    }
  
  if (DBG_LVL (SAM_DBG_DEBUG))
    sam_syslog(LOG_DEBUG, "Loading of %s %s.", tp_so_library,
           err ? "failed" : "complete");

  free (tp_so_library);
  if (err)
    {
      dlclose (api_handle);
      return(-1);
    }

      /* initialize the stage list */
  tp_dev->head = NULL;
  mutex_init(&tp_dev->stage_mutex, USYNC_THREAD, NULL);
  
      /* allocate the free list */
  tp_dev->free = init_list (ROBO_EVENT_CHUNK); 
  tp_dev->free_count = ROBO_EVENT_CHUNK;
  mutex_init(&tp_dev->free_mutex, USYNC_THREAD, NULL);
  mutex_init(&tp_dev->list_mutex, USYNC_THREAD, NULL);
  cond_init(&tp_dev->list_condit, USYNC_THREAD, NULL);
  return(0);
}


/* tp_clean - cleans after the previous instance of sam-migd if that
 * one crashed. Identified places to be cleaned up so far are:
 * - if crash happened after vsn was mounted the drives are still unreleased,
 *   look for those drives based on symlinks created as the path to the
 *   devices.
 *
 * For very first run creates directory for the symlinks.
 */   
int
tp_clean (tp_dev_t *tp_dev,
          dev_ptr_tbl_t *dev_ptr_tbl)
{
  int i;
  char tmp_dir[SAM_MIG_TMP_DIR_LEN];
  int eq;
  char *dev_name = NULL;
  char  *ent_pnt = "tp_clean";
  int err = 0;
  struct stat  stat_buf;
  DIR *dirp;
  struct dirent  *direntp;
  dev_ent_t  *un;
    
    
  sprintf(tmp_dir, "%s/sam-migd-%d", SAM_MIG_TMP_DIR, tp_dev->eq);
  if(stat(tmp_dir, &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode)) 
    {
      if((dirp = opendir(tmp_dir)) != NULL)
        {
          chdir(tmp_dir);
          while((direntp = readdir(dirp)) != NULL)
            {
              if(strcmp(direntp->d_name, ".") == 0 ||
                 strcmp(direntp->d_name, "..") == 0)
                continue;

              if((lstat(direntp->d_name, &stat_buf) != 0 ) ||
                 !S_ISLNK(stat_buf.st_mode))
                {
                      /* Doesn't look like symlink was created with
                       * sam_mig_mount_media(), just remove it
                       */
                  unlink(direntp->d_name);
                  if(DBG_LVL (SAM_DBG_DEBUG))
                    sam_syslog(LOG_DEBUG, "unlinked foreign file %s",
                           direntp->d_name);
                  continue;
                }

                  /* symlink name was constructed as "<eq>-<unique>",
                   * so we can use atoi to get eq back.
                   */
              eq = atoi(direntp->d_name);
              if(eq > 0 && eq < dev_ptr_tbl->max_devices)
                {
                  if(dev_name == NULL)
                    dev_name = (char *)malloc_wait(MAXPATHLEN, 5, 0);
                  memset(dev_name, 0, MAXPATHLEN);

                  un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[eq]);
                  if (readlink (direntp->d_name, dev_name, MAXPATHLEN) < 0)
                    {
                      sam_syslog(LOG_ERR, "%s: readlink: %m", ent_pnt);
                    }
                  else
                    if(strcmp(un->name, dev_name) == 0)
                      {
                        mutex_lock(&un->mutex);
                        DEC_ACTIVE(un);
                        check_preview(un);
                        mutex_unlock(&un->mutex);
                        if(DBG_LVL (SAM_DBG_DEBUG))
                          sam_syslog(LOG_DEBUG, "Cleaned up %s, "
                                 "released eq %d, dev %s",
                                 direntp->d_name, eq, dev_name);
                      }
                }
              unlink(direntp->d_name);
            }
          chdir(SAM_EXECUTE_PATH);
          closedir(dirp);
        }
      else
        sam_syslog(LOG_ERR, "%s: opendir: %m", ent_pnt);
    }
  else
    {
          /* Need this directory for creating symlinks to the real device */
      unlink(tmp_dir);
      if(mkdir(tmp_dir, 0770) != 0)
        {
          sam_syslog(LOG_ERR, "%s: mkdir(%s): %m.", ent_pnt, tmp_dir);
          err = -1;
        }
    }
    
  if(dev_name != NULL)
    free(dev_name);
  return(err);
}


/*
  Local variables:
  eval:(gnu-c-mode)
  End:
 */
