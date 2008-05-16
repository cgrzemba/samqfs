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
#pragma ident   "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * diskvol.c
 * implementation of the diskvols.h api allows the addition and removal of
 * new disk volumes and clients.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "mgmt/config/cfg_diskvols.h"
#include "pub/mgmt/error.h"

#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/mgmt/file_util.h"

#include "aml/diskvols.h"

static int add_dictionary_info(sqm_lst_t *l);
static int get_dictionary_info(struct DiskVolsDictionary *dv_dict,
    char *dv_name, fsize_t *capacity, fsize_t *free_space, uint32_t *status);
static int sighup_fsd(void);


extern char *program_name;


/*
 * Return one disk_vol_t based on vsn name.
 */
int
get_disk_vol(
ctx_t *ctx		/* ARGSUSED */,
const vsn_t vol_name,	/* name of diskvol to get */
disk_vol_t **disk_vol)	/* malloced return value */
{

	diskvols_cfg_t *cfg;
	struct DiskVolsDictionary *dv_dict = NULL;

	Trace(TR_MISC, "getting disk volume %s", vol_name);
	if (ISNULL(disk_vol)) {
		Trace(TR_ERR, "getting disk volume %s failed: %s",
		    vol_name, samerrmsg);
		return (-1);
	}

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting disk volume %s failed: %s",
		    vol_name, samerrmsg);
		return (-1);
	}

	if (get_disk_vsn(cfg, vol_name, disk_vol) != 0) {
		free_diskvols_cfg(cfg);

		Trace(TR_ERR, "getting disk volume %s failed: %s", vol_name,
		    samerrmsg);
		return (-1);
	}

	/*
	 * don't handle errors here. If the dict is NULL the individual
	 * status bits will be set in the get_dictionary_info function.
	 */
	dv_dict = DiskVolsNewHandle(program_name,
	    DISKVOLS_VSN_DICT, 0);

	get_dictionary_info(dv_dict, (*disk_vol)->vol_name,
	    &(*disk_vol)->capacity, &(*disk_vol)->free_space,
	    &(*disk_vol)->status_flags);


	DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	free_diskvols_cfg(cfg);

	Trace(TR_MISC, "got disk volume %s", vol_name);
	return (0);
}


/*
 * returns a list of all configured diskvols.
 */
int
get_all_disk_vols(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **vol_list)	/* malloced list of disk_vol_t */
{

	diskvols_cfg_t *cfg;

	Trace(TR_MISC, "getting all disk volumes");

	if (ISNULL(vol_list)) {
		Trace(TR_ERR, "getting all disk volumes failed: %s", samerrmsg);
		return (-1);
	}

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting all disk volumes failed: %s", samerrmsg);
		return (-1);
	}

	/* ignore errors */
	add_dictionary_info(cfg->disk_vol_list);

	*vol_list = cfg->disk_vol_list;
	cfg->disk_vol_list = NULL;
	free_diskvols_cfg(cfg);

	Trace(TR_MISC, "got all disk volumes");
	return (0);
}


/*
 * return a list of all trusted clients.
 */
int
get_all_clients(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **clients)	/* malloced list of host_t */
{

	diskvols_cfg_t *cfg;

	Trace(TR_MISC, "getting all clients");

	if (ISNULL(clients)) {
		Trace(TR_ERR, "getting all clients failed: %s", samerrmsg);
		return (-1);
	}

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting all clients failed: %s", samerrmsg);
		return (-1);
	}

	*clients = cfg->client_list;
	cfg->client_list = NULL;
	free_diskvols_cfg(cfg);

	Trace(TR_MISC, "got all clients");
	return (0);
}


/*
 * add a disk_vol_t
 */
int
add_disk_vol(
ctx_t *ctx		/* ARGSUSED */,
disk_vol_t *volume)	/* volume to add */
{

	diskvols_cfg_t *cfg;
	node_t *n;


	Trace(TR_MISC, "adding disk volume");

	if (ISNULL(volume)) {
		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}
	if (check_disk_vol(volume) != 0) {
		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}
	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}

	for (n = cfg->disk_vol_list->head; n != NULL; n = n->next) {
		disk_vol_t *tmp = (disk_vol_t *)n->data;

		if (tmp == NULL) {
			continue;
		}
		if (strcmp(volume->vol_name, tmp->vol_name) == 0) {
			free_diskvols_cfg(cfg);

			samerrno = SE_ALREADY_EXISTS;

			/* %s already exists */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_ALREADY_EXISTS), volume);

			Trace(TR_ERR, "adding disk volume %s failed: %s",
			    Str(volume), samerrmsg);
			return (-1);

		}
	}


	/*
	 * If a local volume is being created, create the directory.
	 * Note that this check is also sufficient to guarantee this is not
	 * a honeycomb volume.
	 */
	if (*volume->host == '\0') {
		if (create_dir(NULL, volume->path) != 0) {
			free_diskvols_cfg(cfg);
			Trace(TR_ERR, "adding disk volume failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	if (lst_append(cfg->disk_vol_list, volume) != 0) {
		free_diskvols_cfg(cfg);
		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}

	if (write_diskvols_cfg(ctx, cfg, B_FALSE) != 0) {
		/* remove user provided argument so it does not get freed */
		lst_remove(cfg->disk_vol_list, cfg->disk_vol_list->tail);
		free_diskvols_cfg(cfg);

		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}
	/* remove the user provided argument so it does not get freed */
	lst_remove(cfg->disk_vol_list, cfg->disk_vol_list->tail);
	free_diskvols_cfg(cfg);

	if (sighup_fsd() != 0) {
		samerrno = SE_ACTIVATE_DISKVOLS_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_DISKVOLS_CFG_FAILED));

		Trace(TR_ERR, "adding disk volume failed: %s", samerrmsg);
		return (-1);
	}


	Trace(TR_MISC, "added disk volume");
	return (0);
}


/*
 * remove a disk_vol_t
 */
int
remove_disk_vol(
ctx_t *ctx		/* ARGSUSED */,
const vsn_t vsn_name)	/* name of disk_vol_t to remove */
{

	diskvols_cfg_t *cfg;
	disk_vol_t *dv;
	node_t *n;

	Trace(TR_MISC, "removing disk volume %s", Str(vsn_name));

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "removing disk volume %s failed: %s",
		    Str(vsn_name), samerrmsg);
		return (-1);
	}

	if (get_disk_vsn(cfg, vsn_name, &dv) != 0) {
		free_diskvols_cfg(cfg);
		free(dv);

		Trace(TR_ERR, "removing disk volume %s failed: %s",
		    Str(vsn_name), samerrmsg);
		return (-1);
	}

	free(dv);
	for (n = cfg->disk_vol_list->head; n != NULL; n = n->next) {
		dv = (disk_vol_t *)n->data;
		if (strcmp(dv->vol_name, vsn_name) == 0) {

			if (lst_remove(cfg->disk_vol_list, n) != 0) {
				free_diskvols_cfg(cfg);
				Trace(TR_ERR,
				    "removing disk volume %s failed: %s",
				    Str(vsn_name), samerrmsg);
				return (-1);
			}
			free(dv);
		}

	}

	if (write_diskvols_cfg(ctx, cfg, B_FALSE) != 0) {
		free_diskvols_cfg(cfg);
		Trace(TR_ERR, "removing disk volume %s failed: %s",
		    Str(vsn_name), samerrmsg);
		return (-1);
	}

	if (sighup_fsd() != 0) {
		samerrno = SE_ACTIVATE_DISKVOLS_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_DISKVOLS_CFG_FAILED));

		Trace(TR_ERR, "removing disk volume failed: %s", samerrmsg);
		return (-1);
	}


	free_diskvols_cfg(cfg);
	Trace(TR_MISC, "removed disk volume %s", Str(vsn_name));
	return (0);


}


/*
 * add a client
 */
int
add_client(
ctx_t *ctx		/* ARGSUSED */,
host_t new_client)	/* host name of new client */
{

	diskvols_cfg_t *cfg;
	node_t *n;
	char *tmp_client;

	Trace(TR_MISC, "adding client %s", Str(new_client));

	if (ISNULL(new_client)) {
		Trace(TR_ERR, "adding client %s failed: %s", Str(new_client),
		    samerrmsg);
		return (-1);
	}

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "adding client %s failed: %s", Str(new_client),
		    samerrmsg);
		return (-1);
	}

	for (n = cfg->client_list->head; n != NULL; n = n->next) {
		if (strcmp(new_client, (char *)n->data) == 0) {
			free_diskvols_cfg(cfg);
			/* The client is already present return success */
			return (0);
		}
	}

	if ((tmp_client = strdup(new_client)) == NULL) {
		free_diskvols_cfg(cfg);
		setsamerr(SE_NO_MEM);
		return (-1);
	}

	if (lst_append(cfg->client_list, tmp_client) != 0) {
		free_diskvols_cfg(cfg);
		free(tmp_client);
		Trace(TR_ERR, "adding client %s failed: %s",
		    Str(new_client), samerrmsg);
		return (-1);
	}

	if (write_diskvols_cfg(ctx, cfg, B_FALSE) != 0) {
		free_diskvols_cfg(cfg);
		Trace(TR_ERR, "adding client %s failed: %s",
		    Str(new_client), samerrmsg);
		return (-1);
	}

	if (sighup_fsd() != 0) {
		samerrno = SE_ACTIVATE_DISKVOLS_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_DISKVOLS_CFG_FAILED));

		Trace(TR_ERR, "adding client failed: %s", samerrmsg);
		return (-1);
	}

	free_diskvols_cfg(cfg);
	Trace(TR_MISC, "added client %s", Str(new_client));
	return (0);

}


/*
 * remove a client
 */
int
remove_client(
ctx_t *ctx	/* ARGSUSED */,
host_t client)	/* name of client to remove */
{

	diskvols_cfg_t *cfg;
	node_t *n;

	Trace(TR_MISC, "removing client %s", Str(client));

	if (read_diskvols_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "removing client %s failed: %s",
		    Str(client), samerrmsg);
		return (-1);
	}

	/* find the node with the client. */
	for (n = cfg->client_list->head; n != NULL; n = n->next) {
		if (strcmp(client, (char *)n->data) == 0) {
			break;
		}
	}

	if (n == NULL) {
		free_diskvols_cfg(cfg);
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
		    client);
		Trace(TR_ERR, "removing client %s failed: %s", Str(client),
		    samerrmsg);
		return (-1);
	}

	/* Free the data and remove the node from the list. */
	free(n->data);
	if (lst_remove(cfg->client_list, n) != 0) {
		free_diskvols_cfg(cfg);
		Trace(TR_ERR, "removing client %s failed: %s", Str(client),
		    samerrmsg);
		return (-1);
	}

	/* write the modified file */
	if (write_diskvols_cfg(ctx, cfg, B_FALSE) != 0) {
		free_diskvols_cfg(cfg);
		Trace(TR_ERR, "removing client %s failed: %s", Str(client),
		    samerrmsg);
		return (-1);
	}

	if (sighup_fsd() != 0) {
		samerrno = SE_ACTIVATE_DISKVOLS_CFG_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACTIVATE_DISKVOLS_CFG_FAILED));

		Trace(TR_ERR, "removing client failed: %s", samerrmsg);
		return (-1);
	}

	free_diskvols_cfg(cfg);
	Trace(TR_MISC, "removed client %s", Str(client));
	return (0);
}



/*
 * function to add capacity info and status flags for the
 * disk volumes in the list l
 */
static int
add_dictionary_info(
sqm_lst_t *l)	/* list of disk_vol_t */
{

	node_t *n;
	struct DiskVolsDictionary *dv_dict = NULL;


	/*
	 * don't handle errors here. If the dict is NULL the individual
	 * status bits will be set in the get_dictionary_info function.
	 */
	dv_dict = DiskVolsNewHandle(program_name,
	    DISKVOLS_VSN_DICT, 0);


	for (n = l->head; n != NULL; n = n->next) {
		disk_vol_t *dv = (disk_vol_t *)n->data;

		/* if dictionary open failed set status to unknown */
		if (dv_dict == NULL) {
			dv->status_flags |= DV_DB_NOT_INIT;
		} else if (get_dictionary_info(dv_dict, dv->vol_name,
		    &dv->capacity, &dv->free_space,
		    &dv->status_flags) != 0) {

			/* ignore errors, the flags have been set */
			continue;
		}
	}

	DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);

	return (0);
}


/*
 * return the capacity, free space and status for the named disk volume from
 * the diskvols dictionary.
 */
int
get_dictionary_info(
struct DiskVolsDictionary *dv_dict,
char *dv_name,		/* IN -- the name of the volume to get info for. */
fsize_t *capacity,	/* OUT -- capacity of the disk volume */
fsize_t *free_space,	/* OUT -- free space of the disk volume */
uint32_t *status)	/* OUT -- media flags for the volume */
{


	struct DiskVolumeInfo *dvi;

	Trace(TR_OPRMSG, "getting diskvols info for %s", Str(dv_name));

	if (dv_dict == NULL) {

		*status = DV_DB_NOT_INIT;

		setsamerr(SE_DV_DICT_OPEN_FAILED);
		Trace(TR_ERR, "Opening diskvols dictionary failed.");

		/* could not get diskvols details */
		return (-1);
	}


	if (dv_dict->Get(dv_dict, dv_name, &dvi) != 0) {
		/* dictionary does not know about dv */
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), dv_name);

		Trace(TR_ERR, "The disk volume dictionary does not %s %s",
		    "contain information about", dv_name);

		*capacity = 0;
		*free_space = 0;
		*status |= DV_UNKNOWN_VOLUME;
		return (-1);
	} else if (DISKVOLS_IS_HONEYCOMB(dvi)) {
		/*
		 * SAM does not get sizes from Honeycomb
		 * so set them to 0
		 */
		*capacity = 0;
		*free_space =  0;
		*status |= dvi->DvFlags;
	} else {
		*capacity = dvi->DvCapacity;
		*free_space =  dvi->DvSpace;
		*status |= dvi->DvFlags;
	}


	Trace(TR_OPRMSG, "got diskvols info for %s", Str(dv_name));
	return (0);
}


int
set_disk_vol_flags(
ctx_t *ctx	/* ARGSUSED */,
char *vol_name,
uint32_t flag)
{

	struct DiskVolumeInfo *dvi;
	struct DiskVolsDictionary *dv_dict = NULL;


	Trace(TR_MISC, "setting disk volume flags for %s 0x%x",
	    Str(vol_name), flag);

	if (ISNULL(vol_name)) {
		Trace(TR_ERR, "setting disk volume flags failed: %s",
		    samerrmsg);
		return (-1);
	}


	/*
	 * don't handle errors here. If the dict is NULL the individual
	 * status bits will be set in the get_dictionary_info function.
	 */
	dv_dict = DiskVolsNewHandle(program_name,
	    DISKVOLS_VSN_DICT, 0);

	if (dv_dict == NULL) {
		setsamerr(SE_DV_DICT_OPEN_FAILED);
		Trace(TR_ERR, "Opening diskvols dictionary failed.");

		/* could not get diskvols details */
		return (-1);
	}

	if (dv_dict->Get(dv_dict, vol_name, &dvi) != 0) {
		/* dictionary does not know about dv */
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), vol_name);

		Trace(TR_ERR, "The disk volume dictionary does not %s %s",
		    "contain information about", vol_name);

		DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		return (-1);
	}

	dvi->DvFlags = (uint_t)flag;

	if (dv_dict->Put(dv_dict, vol_name, dvi) != 0) {
		setsamerr(SE_DV_DICT_PUT_FAILED);
		Trace(TR_ERR, "failed to set diskvolume flags for %s",
		    vol_name);
		DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
		return (-1);
	}

	DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);

	Trace(TR_MISC, "set disk volume flags for %s 0x%x",
	    Str(vol_name), flag);

	return (0);
}

static int
sighup_fsd(void)
{
	int rval;

	rval = signal_process(0, "sam-fsd", SIGHUP);

	return (rval);
}
