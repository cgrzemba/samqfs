/*
 * ----- obj_ioctl.c - iscsitgd supported ioctl utility functions.
 *
 *	Processes the ioctl utility functions supported on the SAM File
 *	System for iscsitgd.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.2 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/flock.h>
#include <sys/dkio.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sys/cred.h"
#include "sam/types.h"
#include "sam/uioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "pub/rminfo.h"

#include "samfs.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "ioblk.h"
#include "extern.h"
#include "arfind.h"
#include "trace.h"
#include "debug.h"
#include "kstats.h"
#include "qfs_log.h"

#include "scsi_osd.h"
#include "pub/objioctl.h"
#include "pub/objnops.h"
#include "pub/osdfsops.h"
#include "osdfs.h"

static int sam_obj_cdb_service(void *handle, osd_cdb_t *cdb, void * datain,
    void *dataout, uint64_t *residual);


/* ARGSUSED */
int			/* ERRNO if an error occurred, 0 if successful. */
sam_obj_ioctl_cmd(
	sam_node_t *ioctl_ip,	/* Pointer to "/mountpoint/file" inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_mount_t *mp;
	int	error	= 0;

	mp  = ioctl_ip->mp;
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		return (EPERM);
	}

	switch (cmd) {
	case C_OBJATTACH: {		/* Associate LUN with File System */
		struct obj_ioctl_attach *objattach;
		osdfs_t *osdfsp;
		uint64_t *temp;

		if (SAM_IS_CLIENT_OR_READER(mp)) {
			error = EINVAL;
			break;
		}

		objattach = (struct obj_ioctl_attach *)(void *)arg;
		error = samfs_osdfs_alloc(&osdfsp, CRED());
		error = OSDFS_LUNATTACH(osdfsp, objattach->fsname, CRED());
		if (error) {
			cmn_err(CE_WARN, "Failed to attach %s error %d\n",
			    &objattach->fsname[0], error);
			break;
		}
		temp = (void *)&osdfsp;
		objattach->handle = *temp;
		}

		break;

	case C_OBJDETACH: {	/* Disassociate File System from ioctl */
		struct obj_ioctl_detach *objdetach;

		if (SAM_IS_CLIENT_OR_READER(mp)) {
			error = EINVAL;
			break;
		}

		objdetach = (struct obj_ioctl_detach *)(void *)arg;
		error = OSDFS_LUNDETACH((osdfs_t *)objdetach->handle, 0,
		    CRED());
		if (error) {
			cmn_err(CE_WARN, "Failed to detach FS error %d\n",
			    error);
			break;
		}
		error = samfs_osdfs_free((osdfs_t *)objdetach->handle,
		    CRED());
		}
		break;

	case C_OBJOSDCDB: {
		struct obj_ioctl_osdcdb *objcdb;
		void *datain = NULL;
		void *dataout = NULL;

		objcdb = (struct obj_ioctl_osdcdb *)(void *)arg;
		if (SAM_IS_CLIENT_OR_READER(mp)) {
			error = EINVAL;
			break;
		}
		datain = (void *)objcdb->datain.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			datain = (void *)objcdb->datain.p64;
		}

		dataout = (void *)objcdb->dataout.p32;
		if (curproc->p_model != DATAMODEL_ILP32) {
			dataout = (void *)objcdb->dataout.p64;
		}
		error = sam_obj_cdb_service((void *)objcdb->handle,
		    (osd_cdb_t *)&objcdb->cdb, datain, dataout,
		    &objcdb->residual);
		}
		break;

	default:
		error = ENOTTY;
		break;
	}

	if (error) {
		cmn_err(CE_WARN,
		    "sam_obj_ioctl_cmd encounted error %d\n", error);
	}

	return (error);
}

/*
 * sam_obj_cdb_service - Provides CDB based OSD services via an ioctl call.
 *
 * If the caller is actually a "Remote OSD Service Proxy" e.g. JAVA, RPC etc.
 * it is the responsibility of the caller to perform any Endianess required.
 *
 */
/* ARGSUSED */
static int
sam_obj_cdb_service(void *handle, osd_cdb_t *cdb, void *datain, void *dataout,
    uint64_t *residual)
{

	osd_current_command_page_t curcmdpg;
	int error = 0;

	bzero(&curcmdpg, sizeof (osd_current_command_page_t));

	switch (cdb->ocdb_service_action) {
	case OSD_APPEND: {	/* Append data */
		}
		break;
	case OSD_CLEAR: {
		}
		break;
	case OSD_CREATE: {
		struct objnode *parobjp = NULL;
		uint64_t num;
		uint64_t idlist;

		error = OSDFS_PAROGET((osdfs_t *)handle,
		    cdb->ocdb_create.ocmd_partition_id, &parobjp);
		if (error) {
			break;
		}

		/*
		 * This interface will only create 1 object.
		 */
		num = cdb->ocdb_create.ocmd_number_of_user_objects;
		error = OBJNODE_CREATE(parobjp, 0,
		    /* &cdb->ocdb_create.ocmd_requested_user_object_id, */
		    &num, (uint64_t *)&idlist,
		    (void *)&cdb->ocdb_capability,
		    (void *)&cdb->ocdb_security_parameters,
		    (void *)&curcmdpg, CRED());
		if (error) {
			OBJNODE_RELE(parobjp);
			break;
		}

		/*
		 * Handle attributes.
		 */

		OBJNODE_RELE(parobjp);

		}
		break;

	case OSD_CREATE_AND_WRITE: {
		}
		break;

	case OSD_CREATE_COLLECTION: {
		struct objnode *parobjp;
		struct objnode *colobjp;

		error = OSDFS_PAROGET((osdfs_t *)handle,
		    cdb->ocdb_create_collection.ocmd_partition_id, &parobjp);
		if (error) {
			break;
		}

		error = OBJNODE_COL_CREATE(parobjp, 0,
		    &cdb->ocdb_create_collection.
		    ocmd_requested_collection_object_id, &colobjp,
		    (void *)&cdb->ocdb_capability,
		    (void *)&cdb->ocdb_security_parameters,
		    (void *)&curcmdpg, CRED());
		if (error) {
			OBJNODE_RELE(parobjp);
			break;
		}

		/*
		 * Handle attributes.
		 */

		OBJNODE_RELE(colobjp);
		OBJNODE_RELE(parobjp);
		}
		break;

	case OSD_CREATE_PARTITION: {
		struct objnode *rootobjp;
		struct objnode *parobjp;

		error = OSDFS_GET_ROOTOBJ((osdfs_t *)handle, &rootobjp,
		    CRED());
		if (error) {
			break;
		}

		error = OBJNODE_PAR_CREATE(rootobjp, 0,
		    &cdb->ocdb_create_partition.ocmd_requested_partition_id,
		    &parobjp,
		    (void *)&cdb->ocdb_capability,
		    (void *)&cdb->ocdb_security_parameters,
		    (void *)&curcmdpg,
		    CRED());
		if (error) {
			break;
		}

		/*
		 * Handle Attributes.
		 */

		OBJNODE_RELE(parobjp);
		}
		break;

	case OSD_FLUSH: {
		}
		break;
	case OSD_FLUSH_COLLECTION: {
		}
		break;
	case OSD_FLUSH_OSD: {
		}
		break;
	case OSD_FLUSH_PARTITION: {
		}
		break;
	case OSD_FORMAT_OSD: {
		}
		break;
	case OSD_GET_ATTRIBUTES: {
		}
		break;
	case OSD_GET_MEMBER_ATTRIBUTES: {
		}
		break;
	case OSD_LIST: {
		}
		break;
	case OSD_LIST_COLLECTION: {
		}
		break;
	case OSD_OBJECT_STRUCTURE_CHECK: {
		}
		break;
	case OSD_PERFORM_SCSI_COMMAND: {
		}
		break;
	case OSD_PERFORM_TASK_MANAGEMENT_FUNCTION: {
		}
		break;
	case OSD_PUNCH: {
		}
		break;
	case OSD_QUERY: {
		}
		break;
	case OSD_READ: {
		struct objnode *uobjp = NULL;

		error = OSDFS_OGET((osdfs_t *)handle,
		    cdb->ocdb_read.ocmd_partition_id,
		    cdb->ocdb_read.ocmd_user_object_id,
		    &uobjp);
		if (error) {
			break;
		}

		error = OBJNODE_READ(uobjp,
		    cdb->ocdb_read.ocmd_starting_byte_address,
		    cdb->ocdb_read.ocmd_length, datain, UIO_USERSPACE,
		    cdb->ocdb_options_byte, residual,
		    (void *)&cdb->ocdb_capability,
		    (void *)&cdb->ocdb_security_parameters,
		    (void *)&curcmdpg, CRED());

		/*
		 * Handle attributes.
		 */

		OBJNODE_RELE(uobjp);
		}
		break;
	case OSD_READ_MAP: {
		}
		break;
	case OSD_REMOVE: {
		struct objnode *uobjp = NULL;

		error = OSDFS_OGET((osdfs_t *)handle,
		    cdb->ocdb_remove.ocmd_partition_id,
		    cdb->ocdb_remove.ocmd_user_object_id,
		    &uobjp);
		if (error) {
			break;
		}

		error = OBJNODE_REMOVE(uobjp, NULL, NULL, (void *)&curcmdpg,
		    CRED());

		/*
		 * Handle attributes.
		 */

		OBJNODE_RELE(uobjp);

		}
		break;
	case OSD_REMOVE_COLLECTION: {
		}
		break;
	case OSD_REMOVE_MEMBER_OBJECTS: {
		}
		break;
	case OSD_REMOVE_PARTITION: {
		}
		break;
	case OSD_SET_ATTRIBUTES: {
		}
		break;
	case OSD_SET_KEY: {
		}
		break;
	case OSD_SET_MASTER_KEY: {
		}
		break;
	case OSD_SET_MEMBER_ATTRIBUTES: {
		}
		break;
	case OSD_WRITE: {
		struct objnode *uobjp = NULL;

		error = OSDFS_OGET((osdfs_t *)handle,
		    cdb->ocdb_write.ocmd_partition_id,
		    cdb->ocdb_write.ocmd_user_object_id,
		    &uobjp);
		if (error) {
			break;
		}

		error = OBJNODE_WRITE(uobjp,
		    cdb->ocdb_write.ocmd_starting_byte_address,
		    cdb->ocdb_write.ocmd_length, dataout, UIO_USERSPACE,
		    cdb->ocdb_options_byte, residual,
		    (void *)&cdb->ocdb_capability,
		    (void *)&cdb->ocdb_security_parameters,
		    (void *)&curcmdpg, CRED());

		/*
		 * Handle Attributes
		 */

		OBJNODE_RELE(uobjp);

		}
		break;
	default:
		error = EINVAL;
		break;
	}

	if (error) {
		cmn_err(CE_WARN,
		    "sam_obj_cdb_service: Service Action 0x%x Error %d\n",
		    cdb->ocdb_service_action, error);
	}

	return (error);

}
