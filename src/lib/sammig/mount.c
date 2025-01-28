/*
 * mount.c - interfaces to media mount api for migration toolkit
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

#pragma ident "$Revision: 1.18 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <pthread.h>

#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/proto.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/lib.h"
#include "aml/sam_utils.h"
#include "sam/sam_trace.h"


static char *libname = "libsammig";

extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;
extern void *what_device;

#define	SAM_MIG_TMP_DIR		"/usr/tmp"
#define	SAM_MIG_TMP_DIR_LEN	(sizeof (SAM_MIG_TMP_DIR) + 20)
#define	SAM_MIG_PREFIX_LEN	10

char *
sam_mig_mount_media(
	char *vsn,
	char *s_media)
{
	static char *funcname = "mount_media";

	sam_cmd_fifo_t cmd_block;
	char *fifo_path;	/* path to fifo pipe, from shared memory */
	dev_ent_t *dev;
	dev_ent_t *mount;
	dev_ent_t *dev_head;
	media_t media;
	int rc;
	char tmp_dir[SAM_MIG_TMP_DIR_LEN];
	char tmp_prefix[SAM_MIG_PREFIX_LEN];
	char *path;
	struct stat sb;
	struct CatalogEntry ced;
	struct CatalogEntry *ce = &ced;
	int wait_response = 1;

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG, "%s(%s) [t@%d] %s.%s",
		    libname, funcname, pthread_self(),
		    TrNullString(s_media), TrNullString(vsn));
	}

	if (vsn == NULL || s_media == NULL) {
		return (NULL);
	}

	media = sam_atomedia(s_media);

	if (shm_ptr_tbl == NULL) {
		Trace(TR_ERR, "%s Cannot attach SAM-FS shared memory segment",
		    libname);
		return (NULL);
	}

	dev_head = (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);
	fifo_path = (char *)SHM_REF_ADDR(shm_ptr_tbl->fifo_path);

	mount = NULL;

	for (dev = dev_head; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		if (dev->type == media && strcmp(dev->vsn, vsn) == 0) {
			mount = dev;
			break;
		}
	}

	if (mount != NULL) {
		if (DBG_LVL(SAM_DBG_MIGKIT)) {
			sam_syslog(LOG_DEBUG,
			    "%s(%s) [t@%d] %s.%s already mounted "
			    "device= '%s' eq= %d active= %d open count= %d",
			    libname, funcname, pthread_self(), s_media, vsn,
			    TrNullString(mount->name), mount->eq,
			    mount->active, mount->open_count);
		}
	}

	fifo_path = (char *)SHM_REF_ADDR(((shm_ptr_tbl_t *)
	    master_shm.shared_memory)->fifo_path);

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = CMD_FIFO_MOUNT_S;
	cmd_block.flags = CMD_MOUNT_S_MIGKIT;

	ce = CatalogGetCeByMedia(s_media, vsn, &ced);
	if (ce == NULL) {
		if (CatalogInit(libname) == -1) {
			sam_syslog(LOG_ERR, "Catalog Initialization failed");
			return (NULL);
		}
		ce = CatalogGetCeByMedia(s_media, vsn, &ced);
		if (ce == NULL) {
			sam_syslog(LOG_ERR, "%s(%s) Unable to find VSN %s",
			    libname, funcname, vsn);
			return (NULL);
		}
	}
	cmd_block.eq = ce->CeEq;
	cmd_block.slot = ce->CeSlot;
	cmd_block.media = media;
	strncpy(cmd_block.vsn, vsn, sizeof (vsn_t)-1);

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG,
		    "%s(%s) [t@%d] Send mount command eq= %d slot= %d "
		    "media= %d vsn= '%s'",
		    libname, funcname, pthread_self(), cmd_block.eq,
		    cmd_block.slot, cmd_block.media, cmd_block.vsn);
	}

	if (sam_send_cmd(&cmd_block, wait_response, fifo_path) < 0) {
		sam_syslog(LOG_ERR,
		    "%s(%s) Error sending mount command on FIFO pipe "
		    "errno: %d",
		    libname, funcname, errno);
		return (NULL);
	}

	mount = NULL;
	for (dev = dev_head; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		if (dev->type == media && strcmp(dev->vsn, vsn) == 0) {
			mount = dev;
			if ((int)mount->active <= 0 ||
			    (int)mount->open_count <= 0) {
				sam_syslog(LOG_ERR, "%s(%s) Error: active= %d "
				    "open count= %d",
				    libname, funcname, mount->active,
				    mount->open_count);
				mount = NULL;
			}
			break;
		}
	}

	path = NULL;
	if (mount != NULL && *mount->name != '\0') {

		if (DBG_LVL(SAM_DBG_MIGKIT)) {
			sam_syslog(LOG_DEBUG,
			    "%s(%s) [t@%d] Mount fifo command completed "
			    "successfully device= '%s' vsn= '%s' eq= %d "
			    "active= %d open count= %d",
			    libname, funcname, pthread_self(), mount->name,
			    vsn, mount->eq, mount->active,
			    mount->open_count);
		}

		sprintf(tmp_dir, "%s/%s", SAM_MIG_TMP_DIR, libname);
		rc = stat(tmp_dir, &sb);

		if (rc != 0 || S_ISDIR(sb.st_mode) == 0) {

			if (rc == 0) {
				/* remove existing file */
				(void) unlink(tmp_dir);
			}

			if (mkdir(tmp_dir, 770) != 0) {
				sam_syslog(LOG_ERR,
				    "%s(%s) Failed to create directory %s",
				    libname, funcname, tmp_dir);
				return (NULL);
			}
		}

		sprintf(tmp_prefix, "%d-", mount->eq);

		if ((path = tempnam(tmp_dir, tmp_prefix)) == NULL) {
			sam_syslog(LOG_ERR,
			    "%s(%s) tempnam(%s, %s) failed errno: %d",
			    libname, funcname, tmp_dir, tmp_prefix, errno);
		}
		if (path != NULL && *mount->name != '\0') {
			if (symlink(mount->name, path)) {
				sam_syslog(LOG_ERR,
				    "%s(%s) symlink(%s, %d) failed errno: %d",
				    libname, funcname, mount->name,
				    path, errno);
				free(path);
				path = NULL;
			}
		}
	} else {
		sam_syslog(LOG_ERR,
		    "%s(%s) Could not find mounted device %d.%s",
		    libname, funcname, cmd_block.eq, vsn);
	}

	if (path != NULL) {
		if (DBG_LVL(SAM_DBG_MIGKIT)) {
			sam_syslog(LOG_DEBUG,
			    "%s(%s) [t@%d] Mount complete '%s' device= '%s' "
			    "eq= %d vsn= '%s'",
			    libname, funcname, pthread_self(), path,
			    mount->name, mount->eq, vsn);
		}
	} else {
		sam_syslog(LOG_ERR, "%s(%s) Mount failed", libname, funcname);
	}

	return (path);
}

int
sam_mig_release_device(
	char *device)
{
	static char *funcname = "release_device";

	upath_t path;
	dev_ent_t *dev;
	dev_ent_t *release;
	dev_ent_t *dev_head;
	int ret;

	if (device == NULL) {
		return (-1);
	}

	if (DBG_LVL(SAM_DBG_MIGKIT)) {
		sam_syslog(LOG_DEBUG, "%s(%s) [t@%d] %s",
		    libname, funcname, pthread_self(), device);
	}

	if (shm_ptr_tbl == NULL) {
		Trace(TR_ERR, "%s Cannot attach SAM-FS shared memory segment",
		    libname);
		return (-1);
	}

	dev_head = (struct dev_ent *)SHM_REF_ADDR(shm_ptr_tbl->first_dev);

	memset(path, 0, sizeof (path));

	if (readlink(device, path, sizeof (path)) < 0) {
		sam_syslog(LOG_ERR, "%s(%s) readlink(%s) failed errno: %d",
		    libname, funcname, device, errno);
		return (-1);
	}

	release = NULL;
	for (dev = dev_head; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {
		if (strcmp(dev->name, path) == 0) {
			release = dev;
			break;
		}
	}

	if (release != NULL) {
		ret = unlink(device);
		if (ret < 0) {
			sam_syslog(LOG_ERR,
			    "%s(%s) unlink(%s) failed errno: %d",
			    libname, funcname, device, errno);
		}
		DEC_ACTIVE(release);
		DEC_OPEN(release);

		if (DBG_LVL(SAM_DBG_MIGKIT)) {
			sam_syslog(LOG_DEBUG,
			    "%s(%s) [t@%d] Release complete '%s' device= '%s' "
			    "eq= %d active= %d open count= %d",
			    libname, funcname, pthread_self(), device,
			    release->name, release->eq,
			    release->active, release->open_count);
		}

	} else {
		sam_syslog(LOG_ERR, "%s(%s) Unable to find device %s->%s",
		    libname, funcname, path, device);
	}
	return (0);
}
