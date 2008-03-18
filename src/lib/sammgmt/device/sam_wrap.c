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
#pragma	ident	"$Revision: 1.6 $"
static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "pub/devstat.h"
#include "pub/lib.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "aml/shm.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "aml/robots.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/samapi.h"
#include "sam/lib.h"
#include "sam/devnm.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/nl_samfs.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/device.h"
#include "acssys.h"
#include "acsapi.h"

/*
 * sam_wrap.c contains functions that closely interface with the SAM shared
 * memory or sam media calls. These functions are tightly coupled with the
 * SAM media code and as such serve as a wrapper to SAM media functionality.
 */

#define	VSN_LENGTH	6

static void MsgFunc(int code, char *msg);
static int _attach_get_devent(
    equ_t eq, void **memory, char *fifo_file, dev_ent_t **device);

/*
 * ____________________________________________________________________________
 * SECTION: DISPLAY CONFIGURATION FROM SHARED MEMORY
 * ____________________________________________________________________________
 */

/*
 * get all the libraries from SAM's shared memory
 */
int
get_libraries_from_shm(
sqm_lst_t **lib_lst)	/* OUTPUT - list of library_t */
{
	struct stat	statbuf; /* for devlog mod time */
	stk_param_t	*stkparam;
	shm_alloc_t	shm;
	shm_ptr_tbl_t   *shm_ptr_tbl;
	dev_ent_t	*p, *dev_head;
	node_t		*n, *dn;
	sqm_lst_t		*drv_lst;
	library_t	*lib;
	drive_t		*drive;


	Trace(TR_MISC, "get libraries from shm");

	if (ISNULL(lib_lst)) {
		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * attach to the SHM_MASTER_KEY shared memory and get the device
	 * configuration table
	 */
	shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	shm.shared_memory = shmat(shm.shmid, (void *)NULL, SHM_RDONLY);

	if (shm.shmid < 0 || shm.shared_memory == (void *)-1) {
		samerrno = shm.shmid < 0 ?
		    SE_MASTER_SHM_NOT_FOUND: SE_MASTER_SHM_ATTACH_FAILED;

		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);
		return (-1);
	}

	*lib_lst = lst_create();
	if (*lib_lst == NULL) {

		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)shm.shared_memory;
	if (shm_ptr_tbl == NULL || shm_ptr_tbl->first_dev == NULL) {

		lst_free(*lib_lst);
		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);
		return (-1);
	}

	dev_head = (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(shm_ptr_tbl->first_dev)));

	/* process the device entries for robot entries */
	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if (!IS_ROBOT(p) && !IS_RSC(p)) {
			continue;
		}

		/* robot or sam-client as pseudo lib */

		lib = (library_t *)mallocer(sizeof (library_t));
		if (lib == NULL) {

			lst_free_deep_typed(*lib_lst,
			    FREEFUNCCAST(free_library));
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get libraries from shm failed: %s",
			    samerrmsg);
			return (-1);
		}

		memset(lib, 0, sizeof (library_t));

		/* copy content from dev_ent_t to library_t */
		strlcpy(lib->dis_mes[DIS_MES_NORM], p->dis_mes[DIS_MES_NORM],
		    DIS_MES_LEN + 1);
		strlcpy(lib->dis_mes[DIS_MES_CRIT], p->dis_mes[DIS_MES_CRIT],
		    DIS_MES_LEN + 1);
		strlcpy(lib->dev_status, (char *)sam_devstr(p->status.bits),
		    DEV_STATUS_LEN);
		strlcpy(lib->vendor_id, (char *)p->vendor_id, sizeof (uname_t));
		strlcpy(lib->product_id, (char *)p->product_id,
		    sizeof (uname_t));
		lib->base_info.state = p->state;
		lib->base_info.fseq = p->fseq;
		strlcpy(lib->firmware_version, (char *)p->revision,
		    FIRMWARE_LEN);
		strlcpy(lib->catalog_path, p->dt.rb.name, sizeof (upath_t));
		strlcpy(lib->base_info.name, p->name, sizeof (upath_t));
		strlcpy(lib->base_info.set, p->set, sizeof (uname_t));
		lib->base_info.eq = p->eq;
		strlcpy(lib->base_info.equ_type, dt_to_nm(p->equ_type),
		    sizeof (devtype_t));
		lib->base_info.state = p->state;


		/*
		 * The STK ACSLS network-attached library is fully supported by
		 * the FSM application, i.e. auto-discovery, creation of param
		 * file, interface with the ACSAPI to get the device config and
		 * import cartridges by filter criteria.
		 *
		 * Read the stk parameter file to store the parameters in the
		 * library structure for use later. The other network-attached
		 * libraries require the user to provide the param file and the
		 * application adds its to the mcf file. So the parameter file
		 * is not read/parsed if the library is not DT_STKAPI
		 *
		 * TBD: why is the serial number populated? Serial number should
		 * be obtained from the device and not populated using
		 * hardcoded strings
		 */
		switch (p->equ_type) {
			case DT_STKAPI:
				if (read_parameter_file(
				    lib->base_info.name,
				    p->equ_type,
				    (void **)&stkparam) == -1) {

					free_library(lib);
					lst_free_deep_typed(*lib_lst,
					    FREEFUNCCAST(free_library));
					shmdt(shm.shared_memory);

					Trace(TR_ERR, "get libraries fail: %s",
					    samerrmsg);
					return (-1);
				}

				snprintf(lib->serial_no,
				    sizeof (lib->serial_no),
				    "stk-%s-%d",
				    stkparam->hostname,
				    stkparam->stk_cap.acs_num);

				lib->storage_tek_parameter = stkparam;

				break;

			case DT_IBMATL:
			case DT_SONYPSC:
			case DT_GRAUACI:
				break;
			default:
				strlcpy(lib->serial_no, (char *)p->serial,
				    sizeof (uname_t));
		}

		/* Device log is created by default in DEVLOG_DIR */
		snprintf(lib->log_path, MAX_PATH_LENGTH, "%s/%02d",
		    DEVLOG_DIR, p->eq);
		if (stat(lib->log_path, &statbuf) == 0) {
			lib->log_modtime = statbuf.st_mtime;
		}

		if (lst_append(*lib_lst, lib) != 0) {

			free_library(lib);
			lst_free_deep_typed(*lib_lst,
			    FREEFUNCCAST(free_library));
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get libraries from shm failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	/* if library list is empty, return */
	if ((*lib_lst)->length == 0) {
		return (0);
	}

	/* map the drives to the library */
	drv_lst = lst_create();
	if (drv_lst == NULL) {

		lst_free_deep_typed(*lib_lst, FREEFUNCCAST(free_library));
		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get libraries from shm failed: %s", samerrmsg);
		return (-1);
	}

	/* Read the dev entries again this time, looking for tape entries */
	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if (!IS_TAPE(p)) {
			continue;
		}

		drive = (drive_t *)mallocer(sizeof (drive_t));
		if (drive == NULL) {

			lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
			lst_free_deep_typed(*lib_lst,
			    FREEFUNCCAST(free_library));
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get libraries from shm failed: %s",
			    samerrmsg);
			return (-1);
		}
		memset(drive, 0, sizeof (drive_t));

		strlcpy(drive->dis_mes[DIS_MES_NORM], p->dis_mes[DIS_MES_NORM],
		    DIS_MES_LEN + 1);
		strlcpy(drive->dis_mes[DIS_MES_CRIT], p->dis_mes[DIS_MES_CRIT],
		    DIS_MES_LEN + 1);
		strlcpy(drive->base_info.name, p->name, sizeof (upath_t));
		strlcpy(drive->base_info.set, p->set, sizeof (uname_t));
		drive->base_info.eq = p->eq;
		drive->base_info.fseq = p->fseq;
		drive->base_info.state = p->state;
		strlcpy(drive->firmware_version, (char *)p->revision,
		    FIRMWARE_LEN);
		strlcpy(drive->dev_status, (char *)sam_devstr(p->status.bits),
		    DEV_STATUS_LEN);
		strlcpy(drive->vendor_id, (char *)p->vendor_id,
		    sizeof (uname_t));
		strlcpy(drive->product_id, (char *)p->product_id,
		    sizeof (uname_t));
		strlcpy(drive->base_info.equ_type, dt_to_nm(p->equ_type),
		    sizeof (devtype_t));
		strlcpy(drive->serial_no, (char *)p->serial,
		    sizeof (drive->serial_no));
		drive->base_info.state = p->state;

		/* get the loaded vsn information */
		if (strstr(p->dis_mes[DIS_MES_NORM], "idle") != NULL) {
			strlcpy(drive->loaded_vsn, p->vsn, sizeof (vsn_t));
			drive->load_idletime = time(NULL) - p->mtime;
		}
		/* Device log is created by default in DEVLOG_DIR */
		snprintf(drive->log_path, MAX_PATH_LENGTH, "%s/%02d",
		    DEVLOG_DIR, p->eq);
		if (stat(drive->log_path, &statbuf) == 0) {
			drive->log_modtime = statbuf.st_mtime;
		}
		/* If tapealert is enabled */
		if ((p->tapealert & TAPEALERT_ENABLED) == 0) {
			drive->tapealert_flags = p->tapealert_flags;
		}

		if (lst_append(drv_lst, drive) != 0) {

			free_drive(drive);
			lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
			lst_free_deep_typed(*lib_lst,
			    FREEFUNCCAST(free_library));
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get libraries from shm failed: %s",
			    samerrmsg);
			return (-1);
		}
	}


	/* match tape drives with robots based on the family set name */
	for (n = (*lib_lst)->head; n != NULL; n = n->next) {

		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}
		lib->drive_list = lst_create();
		if (lib->drive_list == NULL) {

			lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
			lst_free_deep_typed(*lib_lst,
			    FREEFUNCCAST(free_library));
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get libraries from shm failed: %s",
			    samerrmsg);
			return (-1);
		}

		for (dn = drv_lst->head; dn != NULL; dn = dn->next) {

			drive = (drive_t *)dn->data;
			if (drive == NULL) {
				continue;
			}
			if ((lib->base_info).fseq == (drive->base_info).fseq) {
				if (lst_append(lib->drive_list, drive) != 0) {


					lst_free_deep_typed(drv_lst,
					    FREEFUNCCAST(free_drive));
					lst_free_deep_typed(*lib_lst,
					    FREEFUNCCAST(free_library));
					shmdt(shm.shared_memory);

					Trace(TR_ERR, "get libraries failed: "
					    "%s", samerrmsg);
					return (-1);
				}
				dn->data = NULL; /* no double free */
				lib->no_of_drives++;
			}
		}
	}

	lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
	shmdt(shm.shared_memory);

	Trace(TR_MISC, "get libraries from shm complete");
	return (0);
}


/*
 * get all standalone drives from SAM shared memory
 */
int
get_sdrives_from_shm(sqm_lst_t **drv_lst)
{
	shm_alloc_t	shm;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	dev_ent_t	*p, *dev_head;
	node_t		*n;
	drive_t		*drive;

	Trace(TR_MISC, "get sdrives from shm");

	if (ISNULL(drv_lst)) {
		Trace(TR_ERR, "get sdrives failed from shm: %s", samerrmsg);
		return (-1);
	}

	/*
	 * attach to the SHM_MASTER_KEY shared memory and get the device
	 * configuration table
	 */
	shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	shm.shared_memory = shmat(shm.shmid, (void *)NULL, SHM_RDONLY);

	if (shm.shmid < 0 || shm.shared_memory == (void *)-1) {
		samerrno = shm.shmid < 0 ?
		    SE_MASTER_SHM_NOT_FOUND: SE_MASTER_SHM_ATTACH_FAILED;

		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get sdrives from shm failed: %s", samerrmsg);
		return (-1);
	}

	*drv_lst = lst_create();
	if (*drv_lst == NULL) {

		shmdt(shm.shared_memory);
		Trace(TR_ERR, "get sdrives from shm failed: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)shm.shared_memory;
	if (shm_ptr_tbl == NULL || shm_ptr_tbl->first_dev == NULL) {

		lst_free(*drv_lst);
		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get sdrives from shm failed: %s", samerrmsg);
		return (-1);
	}

	dev_head = (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(shm_ptr_tbl->first_dev)));

	/*
	 * process device entries, a standalone drive entry is
	 * identified by its type and an empty family set name
	 */
	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if ((IS_OPTICAL(p) || IS_TAPE(p)) &&
		    p->fseq == 0 && strcmp(p->set, "") == 0) {

			drive = (drive_t *)mallocer(sizeof (drive_t));
			if (drive == NULL) {

				lst_free_deep_typed(*drv_lst,
				    FREEFUNCCAST(free_drive));
				shmdt(shm.shared_memory);

				Trace(TR_ERR, "get sdrives from shm failed: %s",
				    samerrmsg);
				return (-1);
			}
			memset(drive, 0, sizeof (drive_t));

			strlcpy(drive->dis_mes[DIS_MES_NORM],
			    p->dis_mes[DIS_MES_NORM], DIS_MES_LEN + 1);
			strlcpy(drive->dis_mes[DIS_MES_CRIT],
			    p->dis_mes[DIS_MES_CRIT], DIS_MES_LEN + 1);
			strlcpy(drive->base_info.name,
			    p->name, sizeof (upath_t));
			strlcpy(drive->dev_status,
			    (char *)sam_devstr(p->status.bits), DEV_STATUS_LEN);

			strlcpy(drive->base_info.set, p->set, sizeof (uname_t));
			drive->base_info.eq = p->eq;
			drive->base_info.fseq = p->fseq;
			strlcpy(drive->base_info.equ_type,
			    dt_to_nm(p->equ_type), sizeof (devtype_t));
			drive->base_info.state = p->state;

			strlcpy(drive->vendor_id, (char *)p->vendor_id,
			    sizeof (uname_t));
			strlcpy(drive->product_id, (char *)p->product_id,
			    sizeof (uname_t));
			strlcpy(drive->firmware_version, (char *)p->revision,
			    sizeof (FIRMWARE_LEN));
			strlcpy(drive->serial_no, (char *)p->serial,
			    sizeof (drive->serial_no));

			drive->alternate_paths_list = NULL;
			strlcpy(drive->loaded_vsn,
			    p->vsn, sizeof (drive->loaded_vsn));

			if (lst_append(*drv_lst, drive) != 0) {

				free_drive(drive);
				lst_free_deep_typed(*drv_lst,
				    FREEFUNCCAST(free_drive));
				shmdt(shm.shared_memory);

				Trace(TR_ERR, "get sdrives from shm failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	shmdt(shm.shared_memory);
	Trace(TR_MISC, "get sdrives from shm success");
	return (0);
}


/*
 * get available media types from shm
 */
int
get_available_mtype_from_shm(sqm_lst_t **mtype_lst)
{
	mtype_t		mtype;
	shm_alloc_t	shm;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	dev_ent_t	*p, *dev_head;

	Trace(TR_MISC, "get available mtype from shm");

	if (ISNULL(mtype_lst)) {

		Trace(TR_ERR, "get avail mtype from shm failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * attach to the SHM_MASTER_KEY shared memory and get the device
	 * configuration table
	 */
	shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	shm.shared_memory = shmat(shm.shmid, (void *)NULL, SHM_RDONLY);

	if (shm.shmid < 0 || shm.shared_memory == (void *)-1) {
		samerrno = shm.shmid < 0 ?
		    SE_MASTER_SHM_NOT_FOUND: SE_MASTER_SHM_ATTACH_FAILED;

		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get avail mtype from shm failed: %s", samerrmsg);
		return (-1);

	}

	*mtype_lst = lst_create();
	if (*mtype_lst == NULL) {

		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get avail mtype from shm failed: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)shm.shared_memory;
	if (shm_ptr_tbl == NULL || shm_ptr_tbl->first_dev == NULL) {

		lst_free(*mtype_lst);
		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get avail mtype from shm failed: %s", samerrmsg);
		return (-1);
	}

	dev_head = (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(shm_ptr_tbl->first_dev)));

	/* process the device entries for robot entries */
	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if (!IS_OPTICAL(p) && !IS_TAPE(p) && !IS_RSC(p)) {
			continue;
		}

		if (IS_RSC(p)) {
			if (get_rd_mediatype(p->eq, mtype) == -1) {

				lst_free_deep(*mtype_lst);
				shmdt(shm.shared_memory);

				Trace(TR_ERR, "get avail mtype failed:%s",
				    samerrmsg);
				return (-1);
			}
		} else {
			strlcpy(mtype, dt_to_nm(p->equ_type), sizeof (mtype_t));
		}

		/* check if mtype is already added to the out lst */
		if (lst_search(
		    *mtype_lst, mtype, (lstsrch_t)strcmp) != NULL) {

			/* got this one already, don't add duplicate */
			continue;
		}

		char *str = copystr(mtype);
		if (str == NULL) {

			lst_free_deep(*mtype_lst);
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get avail mtype from shm failed: %s",
			    samerrmsg);
			return (-1);
		}

		if (lst_append(*mtype_lst, str) != 0) {

			free(str);
			lst_free_deep(*mtype_lst);
			shmdt(shm.shared_memory);

			Trace(TR_ERR, "get avail mtype from shm failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	shmdt(shm.shared_memory);

	Trace(TR_MISC, "get avail mtype from shm complete");
	return (0);
}


/*
 * Given a volume id, check if it is loaded in a drive. If the volume
 * is not loaded, return an empty drive, else return the properties of
 * the drive that has loaded the volume.
 */
int
is_vsn_loaded(
ctx_t *ctx,		/* ARGSUSED */
vsn_t vsn,		/* INPUT - Volume identifier  */
drive_t **drive)	/* OUTPUT - drive_t */
{
	shm_alloc_t	shm;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	dev_ent_t	*p, *dev_head;

	Trace(TR_MISC, "check if vsn %s is loaded", vsn);

	if (ISNULL(drive)) {
		Trace(TR_ERR, "check if vsn is loaded failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * attach to the SHM_MASTER_KEY shared memory and get the device
	 * configuration table
	 */
	shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	shm.shared_memory = shmat(shm.shmid, (void *)NULL, SHM_RDONLY);

	if (shm.shmid < 0 || shm.shared_memory == (void *)-1) {
		samerrno = shm.shmid < 0 ?
		    SE_MASTER_SHM_NOT_FOUND: SE_MASTER_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "check if vsn is loaded failed: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)shm.shared_memory;
	dev_head = (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(shm_ptr_tbl->first_dev)));

	*drive = (drive_t *)mallocer(sizeof (drive_t));
	if (*drive == NULL) {
		shmdt(shm.shared_memory);

		Trace(TR_ERR, "check if vsn is loaded failed: %s", samerrmsg);
		return (-1);
	}
	memset(*drive, 0, sizeof (drive_t));

	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if ((IS_OPTICAL(p) || IS_TAPE(p)) && strcmp(p->vsn, vsn) == 0) {

			strlcpy((*drive)->base_info.name, p->name,
			    sizeof (upath_t));
			strlcpy((*drive)->base_info.set, p->set,
			    sizeof (uname_t));
			(*drive)->base_info.eq = p->eq;
			(*drive)->base_info.fseq = p->fseq;
			strlcpy((*drive)->base_info.equ_type,
			    dt_to_nm(p->equ_type), sizeof (devtype_t));
			(*drive)->base_info.state = p->state;

			(*drive)->alternate_paths_list = NULL;

			strlcpy((*drive)->dev_status,
			    (char *)sam_devstr(p->status.bits), DEV_STATUS_LEN);

			strlcpy((*drive)->vendor_id, (char *)p->vendor_id,
			    sizeof (uname_t));
			strlcpy((*drive)->product_id, (char *)p->product_id,
			    sizeof (uname_t));
			strlcpy((*drive)->serial_no, (char *)p->serial,
			    sizeof ((*drive)->serial_no));

			strlcpy((*drive)->loaded_vsn, p->vsn,
			    sizeof ((*drive)->loaded_vsn));
		}
	}

	shmdt(shm.shared_memory);
	Trace(TR_MISC, "check if vsn %s is loaded success", vsn);
	return (0);
}


/*
 * Get a list of drives that are currently used to label volumes
 * TBD: Rename this function
 */
int
get_tape_label_running_list(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **drv_lst)		/* OUTPUT - a list of drive_t */
{
	shm_alloc_t	shm;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	dev_ent_t	*p, *dev_head;	/* pointer to first device */
	drive_t		*drive;

	Trace(TR_MISC, "get drives that are currently labeling");
	if (ISNULL(drv_lst)) {
		Trace(TR_ERR, "get drives that are labeling failed %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * attach to the SHM_MASTER_KEY shared memory and get the device
	 * configuration table
	 */
	shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	shm.shared_memory = shmat(shm.shmid, (void *)NULL, SHM_RDONLY);

	if (shm.shmid < 0 || shm.shared_memory == (void *)-1) {
		samerrno = shm.shmid < 0 ?
		    SE_MASTER_SHM_NOT_FOUND: SE_MASTER_SHM_ATTACH_FAILED;

		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "check if vsn is loaded failed: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)shm.shared_memory;
	dev_head = (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(shm_ptr_tbl->first_dev)));

	*drv_lst = lst_create();
	if (*drv_lst == NULL) {

		shmdt(shm.shared_memory);

		Trace(TR_ERR, "get drives that are labeling failed %s",
		    samerrmsg);
		return (-1);
	}

	for (p = dev_head; p;
	    p = (p->next == NULL) ?
	    NULL :
	    (dev_ent_t *)((void *)
	    ((char *)shm.shared_memory + (int)(p->next)))) {

		if (IS_TAPE(p) && (p->status.bits & DVST_LABELLING)) {

			drive = (drive_t *)mallocer(sizeof (drive_t));
			if (drive == NULL) {

				lst_free_deep_typed(*drv_lst,
				    FREEFUNCCAST(free_drive));
				shmdt(shm.shared_memory);

				Trace(TR_ERR, "get labeling drives failed: %s",
				    samerrmsg);
				return (-1);
			}
			memset(drive, 0, sizeof (drive_t));

			strlcpy((drive)->base_info.name, p->name,
			    sizeof (upath_t));
			strlcpy((drive)->base_info.set, p->set,
			    sizeof (uname_t));
			(drive)->base_info.eq = p->eq;
			(drive)->base_info.fseq = p->fseq;
			strlcpy((drive)->base_info.equ_type,
			    dt_to_nm(p->equ_type), sizeof (devtype_t));
			(drive)->base_info.state = p->state;

			(drive)->alternate_paths_list = NULL;

			strlcpy((drive)->dev_status,
			    (char *)sam_devstr(p->status.bits), DEV_STATUS_LEN);

			strlcpy((drive)->vendor_id, (char *)p->vendor_id,
			    sizeof (uname_t));
			strlcpy((drive)->product_id, (char *)p->product_id,
			    sizeof (uname_t));
			strlcpy((drive)->serial_no, (char *)p->serial,
			    sizeof ((drive)->serial_no));

			strlcpy((drive)->loaded_vsn, p->vsn,
			    sizeof ((drive)->loaded_vsn));

			if (lst_append(*drv_lst, drive) != 0) {

				free_drive(drive);
				lst_free_deep_typed(*drv_lst,
				    FREEFUNCCAST(free_drive));
				shmdt(shm.shared_memory);

				Trace(TR_ERR, "get labeling drives failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	shmdt(shm.shared_memory);
	Trace(TR_MISC, "get drives that are labeling success");
	return (0);
}


/* required to process messages from the catalog.msg */
static void
MsgFunc(
int code,
char *msg)
{
	error(code, 0, msg);
}


/*
 * attach to the SHM_MASTER_KEY shared memory and get the  device configuration
 * table.
 *
 * this function first gets the shared memory segment identified by
 * SHM_MASTER_KEY and attaches to it. It then gets the device pointer table
 * and the device configuration table
 *
 * callers should use shmdt to detach from the attached segment
 */
static int
_attach_get_devent(
equ_t eq,		/* IN - removable media equipment identifier */
void **memory, 		/* OUT - start addr of attached shared memory segment */
char *fifo_file,	/* OUT - fifo file name - of MAXPATHLEN length  */
dev_ent_t **device)	/* OUT - device configuration table */
{
	shm_ptr_tbl_t *shm_ptr_tbl = NULL;
	int shmid;			/* shared memory segment identifier */
	dev_ptr_tbl_t *dev_ptr_tbl; 	/* device pointer table */

	Trace(TR_MISC, "attach to shm and get device cfg table");

	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		samerrno = SE_MASTER_SHM_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}

	/*
	 * attach to the first available address of the shared memory segment
	 * identified by shmid, only read permissions for user, grp and others
	 */
	if ((*memory = shmat(shmid, (void *)NULL, SHM_RDONLY)) == (void *)-1) {

		samerrno = SE_MASTER_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)(*memory);
	if (shm_ptr_tbl == NULL) {

		shmdt(*memory);

		samerrno = SE_MASTER_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}

	snprintf(fifo_file, MAXPATHLEN, "%s" "/" CMD_FIFO_NAME,
	    (((char *)(*memory)) + shm_ptr_tbl->fifo_path));

	/* LINTED E_BAD_PTR_CAST_ALIGN */
	dev_ptr_tbl = (dev_ptr_tbl_t *)
	    ((char *)(*memory) + shm_ptr_tbl->dev_table);
	if (dev_ptr_tbl == NULL) {

		shmdt(*memory);

		samerrno = SE_MASTER_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}
	/* check if the removable media equipment id is valid */
	if ((eq > dev_ptr_tbl->max_devices) ||
	    (dev_ptr_tbl->d_ent[eq] == 0)) {

		shmdt(*memory);

		samerrno = SE_INVALID_EQ;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}

	/* LINTED E_BAD_PTR_CAST_ALIGN */
	*device = (dev_ent_t *)
	    ((char *)(*memory) + (int)dev_ptr_tbl->d_ent[eq]);

	if (*device == NULL) {

		shmdt(*memory);

		samerrno = SE_MASTER_SHM_ATTACH_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}

	if (!(IS_OPTICAL(*device) || IS_TAPE(*device) || IS_ROBOT(*device))) {

		shmdt(*memory);

		samerrno = SE_NOT_A_REMOVABLE_MEDIA_DEVICE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);
		Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "attach to shm and get device cfg table success");
	return (0);
}

/*
 * ____________________________________________________________________________
 * SECTION: REMOVABLE MEDIA OPERATIONS
 * ____________________________________________________________________________
 */

/*
 * Unload the volume specified by the equipment number. If wait is set to 'true'
 * the load operation will wait for the operation to complete before returning.
 *
 */
int
rb_unload(
ctx_t *ctx,		/* ARGSUSED */
equ_t eq,		/* INPUT - equipment number of device */
boolean_t wait)		/* INPUT - wait for the operation to complete */
{
	void  		*memory;
	size_t		len;
	sam_cmd_fifo_t	cmd_block;
	dev_ent_t	*device;
	int		open_fd, fifo_fd;
	char		fifo_file[MAXPATHLEN]	= {0};

	Trace(TR_MISC, "Start unload eq: %d", eq);

	if (_attach_get_devent(eq, &memory, fifo_file, &device) != 0) {
		Trace(TR_ERR, "unload media from drive failed: %s", samerrmsg);
		return (-1);
	}

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = eq;

	if (!device->status.b.ready) {

		shmdt(memory);

		samerrno = SE_UNLOAD_A_NOT_LOADED_DEVICE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);
		Trace(TR_ERR, "unload eq: %d failed: %s", eq, samerrmsg);
		return (-1);
	}

	if (IS_TAPE(device) &&
	    ((device->state == DEV_UNAVAIL) ||
	    (device->state == DEV_OFF) ||
	    (device->state == DEV_DOWN))) {

		if (((open_fd = open(device->name, O_RDONLY)) < 0) &&
		    errno == EBUSY) {

			if (wait) {
				while (((open_fd =
				    open(device->name, O_RDONLY)) < 0) &&
				    (errno == EBUSY))
					sleep(5);
			} else {
				shmdt(memory);

				/* Device %d is open by another process */
				samerrno = SE_DEVICE_OPEN_BY_ANOTHER_PROCESS;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), eq);
				Trace(TR_ERR, "unload eq: %d failed: %s",
				    eq, samerrmsg);
				return (-1);
			}
		}
		if (open_fd >= 0) {
			struct  mtop  tape_op;

			tape_op.mt_op		= MTOFFL;
			tape_op.mt_count	= 0;
			(void) ioctl(open_fd, MTIOCTOP, &tape_op);
			close(open_fd);
		}
	}

	cmd_block.magic	= CMD_FIFO_MAGIC;
	cmd_block.slot	= ROBOT_NO_SLOT;
	cmd_block.cmd	= CMD_FIFO_UNLOAD;

	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {

		shmdt(memory);

		/* Unable to open command fifo */
		samerrno = SE_UNABLE_TO_OPEN_COMMAND_FIFO;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "unload eq: %d failed: %s", eq, samerrmsg);
		return (-1);
	}

	len = sizeof (sam_cmd_fifo_t);
	if (write(fifo_fd, &cmd_block, len) != len) {

		close(fifo_fd);
		shmdt(memory);

		samerrno = SE_CMD_FIFO_WRITE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}
	close(fifo_fd);

	if (wait) {
		if (IS_ROBOT(device)) {
			while (device->state != DEV_OFF)
				sleep(5);
		} else {
			while (device->status.b.ready ||
			    device->status.b.unload)
				sleep(5);
			while (device->open_count)
				sleep(5);
		}
	}
	shmdt(memory);
	Trace(TR_MISC, "unload success");
	return (0);
}


/*
 * Load the volume specified by the equipment number, slot and partition into
 * the removable media drive. If wait is true, the load operation will wait
 * for the operation to complete before terminating.
 *
 * Note: Callers must not set long wait times if their call is synchronous
 * The fsmgmt client should set wait to B_FALSE if the call is synchronous
 *
 */
int
rb_load_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - equipment number of device */
const int slot,		/* INPUT - slot number */
const int partition,	/* INPUT - partition */
boolean_t wait)		/* INPUT - wait for the operation to complete */
{
	struct VolId		vid;
	struct CatalogEntry	ce;
	int wait_time		= (wait == B_TRUE) ? 120 : 0;
	char 			vsn[20] = {0};

	Trace(TR_MISC, "load volume in eq:slot[:partition] %d:%d[:%d]",
	    eq, slot, partition);

	if ((eq > MAX_DEVICES) || (slot < 0) || (slot > MAX_SLOTS)) {

		samerrno = SE_INVALID_EQ_ORD;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "load vsn failed: %s", samerrmsg);
		return (-1);
	}

	if (partition >= 0) {
		snprintf(vsn, sizeof (vsn), "%d:%d:%d", eq, slot, partition);
	} else {
		snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);
	}

	/* Initialize the Catalog */
	if (CatalogInit("rb_load_from_eq") == -1) {

		samerrno = SE_CATALOG_INIT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "load vsn[%d:%d] failed: %s",
		    eq, slot, samerrmsg);
		return (-1);
	}

	if (StrToVolId(vsn, &vid) != 0) {

		samerrno = SE_VOLUME_SPECIFICATION_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    eq, slot, partition);
		Trace(TR_ERR, "load vsn failed: %s", samerrmsg);
		return (-1);
	}

	if (CatalogGetEntry(&vid, &ce) == NULL) {

		/* Unable to get the catalog entry from the volume id */
		samerrno = SE_GET_CATALOG_ENTRY_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "load vsn failed: %s", samerrmsg);
		return (-1);
	}

	/* Send the load request to SAM */
	if (sam_load(
	    (ushort_t)eq,
	    ce.CeVsn,
	    ce.CeMtype,
	    slot,
	    partition,
	    wait_time) !=  0) {

		samerrno = SE_LOAD_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    slot, eq, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "load vsn %s failed: %s", ce.CeVsn, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "load %d:%d[%d] request queued successfully",
	    eq, slot, partition);
	return (0);
}


/*
 * Set or clear the flags for the given VSN identified by its eq and slot
 *
 *	The following flags are used.
 *	A	needs audit
 *	C	element address contains cleaning cartridge
 *	E	volume is bad
 *	N	volume is not in SAM format
 *	R	volume is read-only (software flag)
 *	U	volume is unavailable (historian only)
 *	W	volume is physically write-protected
 *	X	slot is an export slot
 *	b	volume has a bar code
 *	c	volume is scheduled for recycling
 *	f	volume found full by archiver
 *	d	volume has a duplicate vsn
 *	l	volume is labeled
 *	o	slot is occupied
 *	p	high priority volume
 */
int
rb_chmed_flags_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - equipment number of device */
const int slot,		/* INPUT - slot number */
boolean_t flag_set,	/* INPUT - set or unset flag */
uint32_t mask)		/* INPUT - the mask to be set */
{
	uint32_t	fvalue;
	struct VolId	vid;
	struct CatalogEntry ce;
	char vsn[20] = {0};

	fvalue = (flag_set == B_TRUE) ? mask : 0;

	Trace(TR_MISC, "Modifying the attributes of a vsn[eq = %d]", eq);

	snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);

	if (CatalogInit("rb_chmed_from_eq") == -1) {

		samerrno = SE_CATALOG_INIT_FAILED;	/* sam error 2364 */
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Failed to modify VSN attributes: %s", samerrmsg);
		return (-1);
	}

	if (StrToVolId(vsn, &vid) != 0) {

		samerrno = SE_VOLUME_SPECIFICATION_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    eq, slot, -1 /* no partition specified */);
		Trace(TR_ERR, "modify attributes failed: %s", samerrmsg);
		return (-1);
	}

	if (CatalogGetEntry(&vid, &ce) == NULL) {

		/* Unable to get the catalog entry from the volume id */
		samerrno = SE_GET_CATALOG_ENTRY_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "Failed to modify VSN attributes: %s", samerrmsg);
		return (-1);
	}

	if (CatalogSetField(&vid, CEF_Status, fvalue, mask) != 0) {

		/* unable to set the attributes of the vsn */
		samerrno = SE_SET_CATALOG_FIELD_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "Failed to modify VSN attributes: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "Modified the attributes of vsn[eq = %d]", eq);
	return (0);
}


/*
 * Move VSN from slot A to slot B, the VSN is specified by the library eq
 * and slot in which the VSN currently resides.
 */
int
rb_move_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - equipment number of the library */
const int slot,		/* INPUT - source slot number */
int dest_slot)		/* INPUT - destination slot number */
{
	char vsn[20] = {0};

	Trace(TR_MISC, "Move VSN[eq = %d] from slot %d to %d",
	    eq, slot, dest_slot);

	/* Volume specification eq:slot */
	snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);

	if (SamMoveCartridge(vsn, dest_slot, 0, MsgFunc) == -1) {
		samerrno = SE_SAM_MOVE_CARTRIDGE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), vsn, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "Moved VSN[eq = %d] from slot %d to %d",
	    eq, slot, dest_slot);
	return (0);
}


/*
 * Clean drive. The drive is identified by its equipment number
 */
int
rb_clean_drive(
ctx_t *ctx,	/* ARGSUSED */
equ_t eq)	/* INPUT - equipment number of the cleaning drive */
{
	void		*memory = NULL;
	sam_cmd_fifo_t	cmd_block;
	size_t		len;
	int		fifo_fd;
	dev_ent_t	*device;
	char		fifo_file[MAXPATHLEN] = {0};

	Trace(TR_MISC, "cleaning drive identified by eq %d", eq);

	if (_attach_get_devent(eq, &memory, fifo_file, &device) != 0) {
		Trace(TR_ERR, "clean drive failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * To be discussed with Eagan:
	 * Why are we getting the device using fseq? To test if it is an IBM
	 * drive?
	 */
	if (device->fseq != 0) {
		shm_ptr_tbl_t *shm_ptr_tbl = (shm_ptr_tbl_t *)(memory);
		/* LINTED E_BAD_PTR_CAST_ALIGN */
		dev_ptr_tbl_t *dev_ptr_tbl = (dev_ptr_tbl_t *)
		    ((char *)(memory) + shm_ptr_tbl->dev_table);
		/* LINTED E_BAD_PTR_CAST_ALIGN */
		device = (dev_ent_t *)
		    ((char *)(memory) + (int)dev_ptr_tbl->d_ent[device->fseq]);
		if (device == NULL) {

			shmdt(memory);

			samerrno = SE_MASTER_SHM_ATTACH_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "Failed to attach: %s", samerrmsg);
			return (-1);
		}

		if (device->equ_type == DT_IBMATL) {

			shmdt(memory);

			samerrno = SE_IBM3494_SUPPORT_ONLY_AUTO_DRIVE_CLEAN;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "clean drive failed: %s", samerrmsg);
			return (-1);
		}
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq	= eq;
	cmd_block.magic	= CMD_FIFO_MAGIC;
	cmd_block.slot	= ROBOT_NO_SLOT;
	cmd_block.cmd	= CMD_FIFO_CLEAN;

	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {

		shmdt(memory);

		samerrno = SE_UNABLE_TO_OPEN_COMMAND_FIFO;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "clean drive failed: %s", samerrmsg);
		return (-1);
	}

	len = sizeof (sam_cmd_fifo_t);
	if (write(fifo_fd, &cmd_block, len) != len) {

		close(fifo_fd);

		samerrno = SE_CMD_FIFO_WRITE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}
	close(fifo_fd);

	shmdt(memory);

	Trace(TR_MISC, "finished cleaning drive for eq %d", eq);
	return (0);
}


/*
 * Export the VSN identified by its eq and slot number. For network-
 * attached StorageTek automated libraries only, the volume can be
 * exported to the Cartridge Access Port (CAP) in addition with
 * updating the SAM catalog.
 *
 */
int
rb_export_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - device's equipment number */
const int slot,		/* INPUT - slot number which need to be exported */
boolean_t stk_cap)	/* INPUT - export to STK CAP, only for STK library */
{
	char	vsn[20] = {0};

	Trace(TR_MISC, "exporting VSN[eq = %d, slot = %d]", eq, slot);

	snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);

	if (SamExportCartridge(
	    vsn,
	    0, /* don't wait for response */
	    stk_cap, /* if true, export to STK CAP */
	    MsgFunc /* for errors */) == -1) {

		samerrno = SE_SAM_EXPORT_CARTRIDGE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), vsn, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "export vsn failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "request to export vsn issued successfully");
	return (0);
}


/*
 * Import vsn into a library or the historian. The vsn is specified by its
 * equipment identifier. The options used for importing vsns are specified as
 * import_option_t. For network-attached STK libraries, either the vsn or
 * the volume count and pool name must be specified as import options.
 */
int
rb_import(
ctx_t *ctx,		/* ARGSUSED */
equ_t eq,		/* INPUT - equipment number of library or historian */
const import_option_t *options)	/* INPUT - import option */
{
	void  		*memory;
	sam_cmd_fifo_t	cmd_block;
	size_t		len;
	int		mtype;
	int		fifo_fd;
	dev_ent_t	*device;
	char		fifo_file[MAXPATHLEN];
	boolean_t	add_flag = B_FALSE;

	Trace(TR_MISC, "import into eq %d", eq);

	if (_attach_get_devent(eq, &memory, fifo_file, &device) != 0) {
		Trace(TR_ERR, "import failed: %s", samerrmsg);
		return (-1);
	}
	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.magic	= CMD_FIFO_MAGIC;
	cmd_block.slot	= ROBOT_NO_SLOT;
	cmd_block.part	= 0;
	cmd_block.eq	= eq;

	switch (device->equ_type) {
	case DT_HISTORIAN:
		/*
		 * import -v volser | -b barcode [-n] -m type eq
		 *
		 * The media type and the (barcode or vsn) must be specified
		 * If the barcode is specified, vsn will be ignored
		 *
		 * Audit option is not allowed for historian, if specified in
		 * import options, it is ignored
		 *
		 */
		if (ISNULL(options) ||
		    (strlen(options->mtype) == 0) ||
		    ((strlen(options->vsn) == 0) &&
		    (strlen(options->barcode) == 0))) {

			/* error: media type is not specified */
			/* error: barcode or volser must be specified */

			shmdt(memory);

			samerrno = SE_IMPORT_HISTORIAN_INVALID_OPTION;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "Import failed: %s", samerrmsg);
			return (-1);
		}

		if ((mtype = media_to_device((char *)options->mtype)) == -1) {
			shmdt(memory);

			samerrno = SE_UNKNOWN_MEDIA_TYPE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "Import failed: %s", samerrmsg);
			return (-1);
		}

		cmd_block.cmd	= CMD_FIFO_ADD_VSN;
		cmd_block.media	= mtype;

		if (strlen(options->barcode) > 0) {
			cmd_block.flags |= ADDCAT_BARCODE;
			memccpy(cmd_block.info, options->barcode,
			    '\0', BARCODE_LEN + 1);
		} else {
			strlcpy(cmd_block.vsn, options->vsn, sizeof (vsn_t));
		}
		break;

	case DT_STKAPI:
		/*
		 * For Network-attached StorageTek automated libraries, the
		 * scratch pool name and number of vsns to be imported from
		 * the pool can be specified instead of the vsn name
		 *
		 * Either a -c num -s pool identifier or a -v volser is required
		 * If vsn name is provided, then pool and vol count are ignored
		 */
		if ((options == NULL) ||
		    ((strlen(options->vsn) == 0) &&
		    (options->pool < 0 || options->vol_count <= 0))) {

			shmdt(memory);

			samerrno = SE_IMPORT_STK_INVALID_OPTION;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "Import failed: %s", samerrmsg);
			return (-1);
		}
		if ((options != NULL) &&
		    (options->pool >= 0 && options->vol_count > 0)) {

			/* use import -c num -s pool eq */
			if (stk_import_vsns(
			    device->dt.rb.port_num,
			    fifo_file,
			    options->pool,
			    options->vol_count,
			    &(device->vsn[0])) != 0) {

				shmdt(memory);

				Trace(TR_ERR, "Import failed: %s", samerrmsg);
				return (-1);
			}

			/* import requests have been queued with acs */
			shmdt(memory);

			Trace(TR_MISC, "Import request complete");
			return (0);
		}
		/* use import -v volser eq */
		/* FALL THROUGH */
	case DT_GRAUACI:
		/* FALL THROUGH */
	case DT_IBMATL:
		/* FALL THROUGH */
	case DT_SONYPSC:
		if (ISNULL(options) || strlen(options->vsn) == 0) {

			shmdt(memory);
			samerrno = SE_IMPORT_MUST_BE_VSN_OR_SCRATCH_POOL;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "Import failed: %s", samerrmsg);
			return (-1);
		}

		/* FALL THROUGH */
	default:
		/* use import -v volser eq */
		if (!(device->status.b.ready)) {

			shmdt(memory);

			samerrno = SE_DEVICE_NOT_READY;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), eq);
			Trace(TR_ERR, "Import failed: %s", samerrmsg);
			return (-1);
		}

		if (options != NULL) {

			if (strlen(options->vsn) > 0) {
				add_flag = B_TRUE;

				strlcpy(cmd_block.vsn, options->vsn,
				    sizeof (vsn_t));
			}

			if (options->foreign_tape) {

				/* If vsn is input, use CMD_ADD_VSN_STRANGE */
				cmd_block.flags |= add_flag ?
				    CMD_ADD_VSN_STRANGE :
				    CMD_IMPORT_STRANGE;
			}

			if (options->audit) {

				/* If vsn is input , use CMD_ADD_VSN_AUDIT */
				cmd_block.flags |= add_flag ?
				    CMD_ADD_VSN_AUDIT: CMD_IMPORT_AUDIT;
			}

		}
		cmd_block.cmd = ((options != NULL) && add_flag) ?
		    CMD_FIFO_ADD_VSN : CMD_FIFO_IMPORT;

	}
	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {

		shmdt(memory);

		samerrno = SE_UNABLE_TO_OPEN_COMMAND_FIFO;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "Import failed: %s", samerrmsg);
		return (-1);
	}

	len = sizeof (sam_cmd_fifo_t);
	if (write(fifo_fd, &cmd_block, len) != len) {

		close(fifo_fd);

		samerrno = SE_CMD_FIFO_WRITE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}
	close(fifo_fd);

	Trace(TR_MISC, "Import request complete");
	return (0);
}


/*
 *	Audit slot in a robot
 *
 *	Parameters:
 *	eq		- device's equipment number.
 *	slot		- slot number.
 *	partition	- partition number; For optical drives.
 *	boolean_t eod	- EOD flag
 */
int rb_auditslot_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - device's equipment number */
const int slot,		/* INPUT - slot number */
const int partition,	/* ARGSUSED */
boolean_t eod)		/* ARGSUSED */
{
	Trace(TR_MISC, "audit slot %d for eq %d", slot, eq);

	if (sam_audit(eq, slot, 0 /* don't wait for response */) == -1) {

		samerrno = SE_SAM_AUDIT_SLOT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "audit slot request failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "audit request complete");
	return (0);
}


/*
 * change the state of the library or drive, specified by its equipment
 * identifier
 * The new state can be one of the following:-
 * DEV_ON	-  Normal
 * DEV_RO	-  Read only operation
 * DEV_IDLE	-  No new opens allowed
 * DEV_UNAVAIL	-  Unavailable for file system
 * DEV_OFF	-  Off to this machine
 * DEV_DOWN	-  Maintenance use only
 */
int
change_state(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - device's equipment number */
dstate_t state)		/* INPUT - new state */
{
	void		*memory;
	sam_cmd_fifo_t	cmd_block;
	dev_ent_t	*device;	/* Device entry */
	int		fifo_fd;
	int		size;
	char		fifo_file[MAXPATHLEN];

	Trace(TR_MISC, "change state for eq %d %s", eq, dev_state[state]);

	if (state < DEV_ON || state > DEV_DOWN) {

		samerrno = SE_UNKNOWN_STATE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), state);
		Trace(TR_ERR, "change state failed: %s", samerrmsg);
		return (-1);
	}

	if (_attach_get_devent(eq, &memory, fifo_file, &device) != 0) {
		Trace(TR_ERR, "change state failed: %s", samerrmsg);
		return (-1);
	}

	switch (state) {
		case DEV_DOWN:
			if (device->state != DEV_OFF) {

				shmdt(memory);

				samerrno = SE_CUR_STATE_MUST_BE_OFF;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), dev_state[state]);
				Trace(TR_ERR, "change state failed: %s",
				    samerrmsg);
				return (-1);
			}
			break;

		case DEV_IDLE:
			if (device->state != DEV_ON) {

				shmdt(memory);

				samerrno = SE_CUR_STATE_MUST_BE_ON;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), dev_state[state]);
				Trace(TR_ERR, "change state failed: %s",
				    samerrmsg);
				return (-1);
			}
			break;

		case DEV_ON:
		case DEV_UNAVAIL:
		case DEV_OFF:
		case DEV_RO:
			break;

		default:
			/* unsupported state */

			shmdt(memory);

			samerrno = SE_UNKNOWN_STATE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    state);
			Trace(TR_ERR, "change state failed: %s", samerrmsg);
			return (-1);
	}

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq	= eq;
	cmd_block.state	= state;
	cmd_block.magic	= CMD_FIFO_MAGIC;
	cmd_block.cmd	= CMD_FIFO_SET_STATE;

	fifo_fd = open(fifo_file, O_WRONLY | O_NONBLOCK);
	if (fifo_fd < 0) {

		shmdt(memory);

		samerrno = SE_CMD_FIFO_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "change state failed: %s", samerrmsg);
		return (-1);
	}

	size = write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);
	shmdt(memory);

	if (size != sizeof (sam_cmd_fifo_t)) {

		samerrno = SE_CMD_FIFO_WRITE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "change state failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "finished changing state() for eq %d", eq);
	return (0);
}


/*
 * Pre-reserve a volume for archiving. The volume is specified by equpiment
 * identifier, and the slot and partition it occupies. The time the volume is
 * reserved, archive set sname,  owner, and fsname are provided in
 * reserve_option_t.
 *
 */
int
rb_reserve_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - eq number */
const int slot,		/* INPUT - slot number */
const int partition,	/* INPUT - partition */
const reserve_option_t *option)	/* INPUT - time, owner, archve set name etc. */
{
	char vsn[20] = {0};
	char option_str[128]; /* 32 + 32 + 32 + buffer */

	Trace(TR_MISC, "reserve volume[eq=%d, slot=%d, partition=%d]",
	    eq, slot, partition);

	if (ISNULL(option)) {
		Trace(TR_ERR, "reserve volume failed: %s", samerrmsg);
		return (-1);
	}

	if (partition >= 0) {
		snprintf(vsn, sizeof (vsn), "%d:%d:%d", eq, slot, partition);
	} else {
		snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);
	}

	snprintf(option_str, sizeof (option_str), "%s/%s/%s",
	    option->CerAsname,
	    option->CerOwner,
	    option->CerFsname);

	if (SamReserveVolume(vsn, option->CerTime, option_str, MsgFunc) == -1) {

		samerrno = SE_SAM_RESERVE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), vsn, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "reserve volume failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "reserve volume request complete");
	return (0);
}


/*
 * Unreserve a volume for archiving. The volume is specified by the equipment
 * Identifier, slot and partition
 */
int
rb_unreserve_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - equipment identifier */
const int slot,		/* INPUT - slot number */
const int partition)	/* INPUT - partition */
{

	char vsn[20] = {0};

	Trace(TR_MISC, "unreserve volume[eq=%d, slot=%d, partition=%d",
	    eq, slot, partition);

	if (partition >= 0) {
		snprintf(vsn, sizeof (vsn), "%d:%d:%d", eq, slot, partition);
	} else {
		snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);
	}

	if (SamUnreserveVolume(vsn, MsgFunc) == -1) {

		samerrno = SE_SAM_UNRESERVE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), vsn, "");
		strlcat(samerrmsg, sam_errno(errno), MAX_MSG_LEN);
		Trace(TR_ERR, "unreserve volume failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "unreserve volume request complete");
	return (0);
}


/*
 * Label VSN. The tape is specified by equipment identifier and the slot in
 * the library containing the tape cartridge. The labels must conform to ANSI
 * X3.27-1987 File Structure and Labeling of Magnetic Tapes for Information
 * Interchange. The VSN must be one to six characters in length. All characters
 * must be selected from the 26 upper-case letters, the 10 digits, and the
 * following special characters: !"%&'()*+,-./:;<=>?_.
 *
 * If the media being labeled was previously labeled, the old name must be
 * specified in old_vsn. If the media is blank, the new_vsn must be filled and
 * old_vsn must be empty.
 *
 * Note: Callers must not set wait to B_TRUE if the call is syncronous, the
 * operation might take a long time
 *
 */
int
rb_tplabel_from_eq(
ctx_t *ctx,		/* ARGSUSED */
const equ_t eq,		/* INPUT - equipment number */
const int slot,		/* INPUT - slot number */
const int partition,	/* INPUT - partition */
vsn_t new_vsn,		/* INPUT - new label, maximum length is 6 characters */
vsn_t old_vsn,		/* INPUT - old label, if VSN was previously labeled */
			/* empty if media is not previously labeled */
uint_t blksize,		/* INPUT - size of the tape block in units of 1024 */
			/* a value of 0 indicates default, if not 0, it must */
			/* 16, 32, 64, 128, 256, 512, 1024, or 2048 */
boolean_t wait,		/* wait for the labeling operation to complete */
boolean_t erase)	/* erase the media completely before labeling */
{
	struct VolId		vid;
	struct CatalogHdr	*ch;
	struct CatalogEntry	ced;
	struct CatalogEntry	*ce;
	size_t			len;
	sam_cmd_fifo_t		cmd_block;	/* Command block */
	int			wait_response;
	int			fifo_fd;	/* Command FIFO descriptor */
	char 			vsn[20] = {0};
	boolean_t		relabel;

	Trace(TR_MISC, "label tape volume[eq=%d, slot=%d, partition=%d]",
	    eq, slot, partition);

	/* If old_vsn is empty, the media was not previously labeled */
	relabel = (strlen(old_vsn) > 0) ? B_TRUE : B_FALSE;

	/* wait for response is set to 1, if wait is B_TRUE */
	wait_response = (wait == B_TRUE) ? 1 : 0;

	if (strlen(new_vsn) == 0) {

		samerrno = SE_VSN_NOT_SPECIFIED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (relabel && (strlen(old_vsn) == 0)) {

		samerrno = SE_EXISTING_VSN_NOT_SPECIFIED_FOR_RELABEL;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	/* size of tape block must be 16, 32, 64, 128, 256, 512, 1024, 2048 */
	if (blksize != 0) {
		if ((blksize != 16) && (blksize != 32) && (blksize != 64) &&
		    (blksize != 128) && (blksize != 256) && (blksize != 512) &&
		    (blksize != 1024) && (blksize != 2048)) {

			samerrno = SE_NOT_A_LEGAL_BLOCK_SIZE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "label tape failed: %s", samerrmsg);
			return (-1);
		}
		blksize <<= 10;
	}

	if (strlen(new_vsn) > VSN_LENGTH) {

		samerrno = SE_INVALID_VSN_LENGTH;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), new_vsn);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (CatalogInit("rb_tplabel") == -1) {

		samerrno = SE_CATALOG_INIT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	/* construct volume identifier from eq, slot and partition */
	if (partition >= 0) {
		snprintf(vsn, sizeof (vsn), "%d:%d:%d", eq, slot, partition);
	} else {
		snprintf(vsn, sizeof (vsn), "%d:%d", eq, slot);
	}
	if (StrToVolId(vsn, &vid) != 0) {

		samerrno = SE_VOLUME_SPECIFICATION_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    eq, slot, partition);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	/* validate that the volume specified by vid exists */
	if ((ce = CatalogCheckVolId(&vid, &ced)) == NULL) {

		samerrno = SE_VOLUME_SPECIFICATION_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    eq, slot, partition);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	/* find the catalog that matches the equipment identifier */
	ch = CatalogGetHeader(ce->CeEq);
	if (ch == NULL) {

		samerrno = SE_VOLUME_SPECIFICATION_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    eq, slot, partition);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	/* Cannot label volume in historian */
	if (ch->ChType == CH_historian) {

		samerrno = SE_CANNOT_LABEL_VOLUME_IN_HISTORIAN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (is_ansi_tp_label(new_vsn, VSN_LENGTH) != 1) {

		samerrno = SE_INVALID_CHAR_IN_VSN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), new_vsn);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (strlen(old_vsn) > VSN_LENGTH) {

		samerrno = SE_INVALID_VSN_LENGTH;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), old_vsn);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (is_ansi_tp_label(old_vsn, VSN_LENGTH) != 1) {

		samerrno = SE_INVALID_CHAR_IN_VSN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), old_vsn);
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	if (relabel) {
		if (strncmp(old_vsn, ce->CeVsn, VSN_LENGTH) != 0) {

			samerrno = SE_OLD_VSN_DOES_NOT_MATCH_MEDIA_VSN;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    ce->CeVsn, old_vsn);
			Trace(TR_ERR, "label tape failed: %s", samerrmsg);
			return (-1);
		}
	}

	/* all input validations are complete, generate cmd to send to FIFO */

	memset(&cmd_block, 0, sizeof (sam_cmd_fifo_t));
	cmd_block.magic		= CMD_FIFO_MAGIC;
	cmd_block.cmd		= CMD_FIFO_LABEL;
	cmd_block.block_size	= blksize;
	cmd_block.flags		= 0;
	cmd_block.eq		= ce->CeEq;
	cmd_block.slot		= ce->CeSlot;
	cmd_block.part		= ce->CePart;
	cmd_block.media		= sam_atomedia(ce->CeMtype);

	strlcpy(cmd_block.old_vsn, old_vsn, sizeof (cmd_block.old_vsn));
	strlcpy(cmd_block.vsn, new_vsn, sizeof (cmd_block.vsn));

	if (erase) {
		cmd_block.flags |= CMD_LABEL_ERASE;
	}
	if (relabel) {
		cmd_block.flags |= CMD_LABEL_RELABEL;
	}
	if (ch->ChType == CH_library) {
		cmd_block.flags |= CMD_LABEL_SLOT;
	}

	if (wait_response) {
		set_exit_id(0, &cmd_block.exit_id);
		if (create_exit_FIFO(&cmd_block.exit_id) < 0) {

			samerrno = SE_CANNOT_CREATE_RESPONSE_FIFO;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "label tape failed: %s", samerrmsg);
			return (-1);
		}
	} else {
		cmd_block.exit_id.pid = 0;
	}

	if ((fifo_fd = open(SAM_FIFO_PATH"/"CMD_FIFO_NAME, O_WRONLY)) < 0) {

		samerrno = SE_UNABLE_TO_OPEN_COMMAND_FIFO;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

		Trace(TR_ERR, "label tape failed:  %s", samerrmsg);
		return (-1);
	}

	len = sizeof (sam_cmd_fifo_t);
	if (write(fifo_fd, &cmd_block, len) != len) {

		close(fifo_fd);

		samerrno = SE_CMD_FIFO_WRITE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "label tape failed: %s", samerrmsg);
		return (-1);
	}

	close(fifo_fd);

	if (wait_response) {
		char resp_string[256];
		int completion;

		if (read_server_exit_string(
		    &cmd_block.exit_id,
		    &completion,
		    resp_string,
		    255,
		    -1) < 0) {

			unlink_exit_FIFO(&cmd_block.exit_id);

			samerrno = SE_CANNOT_RETRIEVE_CMD_RESPONSE;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "label tape failed: %s", samerrmsg);
			return (-1);
		}

		unlink_exit_FIFO(&cmd_block.exit_id);
		if (completion != 0) {

			samerrno = SE_WAIT_RESPONSE_CANNOT_BE_COMPLETED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "label tape failed: %s", samerrmsg);
			return (-1);
		}
	}

	Trace(TR_MISC, "finished doing tpabel for eq %d", eq);
	return (0);
}


/*
 *	import_all().
 *	This function will accept the beginning VSN and ending VSN
 *	as parameters.  This function automatically generates the VSNs
 *	between beginning and ending VSN and imports each one.
 *	Actual number of imported VSNs is returned.
 */
int
import_all(
ctx_t *ctx,			/* ARGSUSED */
vsn_t begin_vsn,		/* beginning VSN */
vsn_t end_vsn,			/* ending VSN */
int *total_num,			/* the total imported vsn numbers */
equ_t eq,			/* device equipment number */
import_option_t *options)	/* import options */
{
	int beg_num, end_num;
	int i;
	int str_len;
	int end_str_len;
	vsn_t vsn, new_vsn;
	int count;
	char *ch;

	Trace(TR_MISC, "doing import all vsns for eq %d", eq);
	if (ISNULL(options)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	beg_num = get_vsn_num(begin_vsn, &str_len);
	end_num = get_vsn_num(end_vsn, &end_str_len);
	*total_num = 0;
	count = end_num - beg_num;
	strlcpy(vsn, begin_vsn, sizeof (vsn));
	strlcpy(new_vsn, begin_vsn, sizeof (new_vsn));
	options->pool = options->vol_count = -1; // not used

	for (i = 0; i < count + 1; i++) {

		strlcpy(options->vsn, new_vsn, sizeof (options->vsn));

		if (rb_import(NULL, eq, options) == 0) {
			*total_num = *total_num + 1;
		}
		ch = gen_new_vsn(options->vsn, str_len, beg_num, new_vsn);
		if (ch == NULL) {
			samerrno = SE_VSN_GENERATION_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
		beg_num ++;
	}
	Trace(TR_MISC, "finished doing import all vsns for eq %d", eq);
	return (0);
}


/*
 *	import_stk_vsns().
 *	This function will accept the beginning VSN and ending VSN
 *	as parameters.  This function automatically generates the VSNs
 *	between beginning and ending VSN and imports each one.
 *	Actual number of imported VSNs is returned.
 */
int
import_stk_vsns(
ctx_t *ctx,			/* ARGSUSED */
equ_t eq,			/* device equipment number */
import_option_t *options,	/* import options */
sqm_lst_t *vsn_list)		/* a list of vsns needed import */
{
	node_t *node_vsn;
	char *new_vsn;
	int total_num = 0;

	Trace(TR_MISC, "doing import stk vsns for eq %d", eq);
	if (ISNULL(options, vsn_list)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	/*
	 * importing a vsn range from a stk library
	 * use import -v volser eq
	 */
	options->pool = options->vol_count = -1; // not used
	node_vsn = vsn_list->head;
	while (node_vsn != NULL) {
		new_vsn =  (char *)node_vsn->data;
		node_vsn = node_vsn->next;
		if (ISNULL(new_vsn)) {
			continue;
		}
		strlcpy(options->vsn, new_vsn, sizeof (options->vsn));

		if (rb_import(NULL, eq, options) == 0) {
			total_num = total_num + 1;
		}
	}
	Trace(TR_MISC, "finished importing %d stk vsns for eq %d",
	    total_num, eq);
	return (0);
}
