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
#pragma ident "$Revision: 1.31 $"

/*
 *	memory_free.c -  memory free functions
 */

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "aml/stager.h"
#include "pub/mgmt/device.h"

#include "mgmt/config/archiver.h"
#include "pub/mgmt/stage.h"
#include "mgmt/config/media.h"
#include "mgmt/config/recycler.h"

#include "mgmt/config/cfg_diskvols.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/mount_cfg.h"
#include "pub/mgmt/load.h"
#include "pub/mgmt/license.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/hosts.h"
#include "pub/mgmt/archive_sets.h"
#include "pub/mgmt/csn_registration.h"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 *	media management free functions
 */

/*
 *	free_catalog_info.
 */
void
free_catalog_info(catalog_info_t *catalog_info_p)
{
	Trace(TR_ALLOC, "freeing catalog_info");
	if (catalog_info_p == NULL)
		return;
	if (catalog_info_p->vsn != NULL)
		free(catalog_info_p->vsn);
	if (catalog_info_p->barcode != NULL)
		free(catalog_info_p->barcode);
	free(catalog_info_p);
	Trace(TR_ALLOC, "finished freeing catalog_info");
}


void
free_catalog_info_list(sqm_lst_t *catalog_info_list)
{
	node_t *node;
	Trace(TR_ALLOC, "freeing catalog_info_list");
	if (catalog_info_list == NULL)
		return;
	node = catalog_info_list->head;
	while (node != NULL) {
		free_catalog_info((catalog_info_t *)node->data);
		node = node->next;
	}
	lst_free(catalog_info_list);
	Trace(TR_ALLOC, "finished freeing catalog_info_list");
}


void
free_drive(drive_t *drive_info_p)
{
	Trace(TR_ALLOC, "freeing drive_t");
	if (drive_info_p == NULL)
		return;
	if (drive_info_p->alternate_paths_list != NULL)
		lst_free_deep(drive_info_p->alternate_paths_list);
	free(drive_info_p);
	Trace(TR_ALLOC, "finished freeing drive_t");
}


/*
 *	free_list_of_media_license().
 *	This function will free the
 *	list of md_license structure.
 */
void
free_list_of_media_license(sqm_lst_t *media_license_list)
{
	node_t *node;
	Trace(TR_ALLOC, "freeing media_license_list");
	if (media_license_list == NULL)
		return;
	node = media_license_list->head;
	while (node != NULL) {
		free((md_license_t *)node->data);
		node = node->next;
	}
	lst_free(media_license_list);
	Trace(TR_ALLOC, "finished freeing media_license_list");
}


/*
 *	free_library.
 *	free structure library_t.
 */
void
free_library(library_t *library_p)
{
	Trace(TR_ALLOC, "freeing library_t");
	if (library_p == NULL) {
		return;
	}
	if (library_p->alternate_paths_list != NULL)
		lst_free_deep(library_p->alternate_paths_list);
	if (library_p->media_license_list != NULL)
		free_list_of_media_license(library_p->media_license_list);
	if (library_p->drive_list != NULL)
		free_list_of_drives(library_p->drive_list);
	free(library_p);
	Trace(TR_ALLOC, "finished freeing library_t");
}


/*
 *	free library_t list.
 */
void
free_list_of_libraries(sqm_lst_t *library_list)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing library_list");
	if (library_list == NULL)
		return;

	node = library_list->head;
	while (node != NULL) {
		free_library((library_t *)node->data);
		node = node->next;
	}
	lst_free(library_list);

	Trace(TR_ALLOC, "finished freeing library_list");
}


/*
 *	free drive list.
 */
void
free_list_of_drives(sqm_lst_t *drive_list)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing list_of_drives");
	if (drive_list == NULL)
		return;
	node = drive_list->head;
	while (node != NULL) {
		free_drive((drive_t *)node->data);
		node = node->next;
	}
	lst_free(drive_list);
	Trace(TR_ALLOC, "finished freeing list_of_drives");
}


/*
 *	free catalog_entries list.
 */
void
free_list_of_catalog_entries(sqm_lst_t *catalog_entry_list)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing list_of_catalog_entries");
	if (catalog_entry_list == NULL)
		return;
	node = catalog_entry_list->head;
	while (node != NULL) {
		free(node->data);
		node = node->next;
	}
	lst_free(catalog_entry_list);
	Trace(TR_ALLOC, "finished freeing list_of_catalog_entries");
}


/*
 *	free catalog_entries list of list.
 */
void
free_catalog_entry_list_list(sqm_lst_t *catalog_entry_list_list)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing catalog_entry_list_list");
	if (catalog_entry_list_list == NULL)
		return;
	node = catalog_entry_list_list->head;
	while (node != NULL) {
		free_list_of_catalog_entries((sqm_lst_t *)node->data);
		node = node->next;
	}
	lst_free(catalog_entry_list_list);
	Trace(TR_ALLOC, "finished freeing catalog_entry_list_list");
}


/*
 *	recycler free memory functions
 */
/*
 * free the memory_free.cfg_t
 */
void
free_recycler_cfg(recycler_cfg_t *cfg)
{

	Trace(TR_ALLOC, "freeing recycler_cfg");
	if (cfg == NULL)
		return;
	free_list_of_no_rc_vsns(cfg->no_recycle_vsns);
	free_list_of_rc_robot_cfg(cfg->rc_robot_list);
	free(cfg);
	cfg = NULL;

	Trace(TR_ALLOC, "finished freeing recycler_cfg");
}


/*
 * free a list of no_rc_vsns
 */
void
free_list_of_no_rc_vsns(sqm_lst_t *l)
{

	Trace(TR_ALLOC, "freeing list_of_no_rc_vsns");

	/* if the list is not free free its members which are lists */
	if (l != NULL) {
		while (l->length != 0) {
			if (l->head->data != NULL) {
				no_rc_vsns_t *n =
				    (no_rc_vsns_t *)(l->head->data);
				lst_free_deep(n->vsn_exp);
				free(n);
			}
			lst_remove(l, l->head);
		}
		lst_free(l);
	}

	l = NULL;
	Trace(TR_ALLOC, "finished freeing list_of_no_rc_vsns");
}


/*
 * free a list of rc_robot_cfgs
 */
void
free_list_of_rc_robot_cfg(sqm_lst_t *l)
{

	Trace(TR_ALLOC, "freeing list_of_rc_robot_cfg");
	if (l != NULL) {
		lst_free_deep(l);
	}

	l = NULL;
	Trace(TR_ALLOC, "finished freeing list_of_rc_robot_cfg");

}


/*
 *	free_no_rc_vsns(no_rc_vsns_t *vsns)
 *	Memory free functions.
 *	free structure pointer no_rc_vsns_t.
 */
void
free_no_rc_vsns(no_rc_vsns_t *vsns)
{
	Trace(TR_ALLOC, "freeing no_rc_vsns");
	if (vsns == NULL) {
		return;
	}
	if (vsns->vsn_exp != NULL)
		lst_free_deep(vsns->vsn_exp);
	free(vsns);
	Trace(TR_ALLOC, "finished freeing no_rc_vsns");
}


/*
 *	stager free functions
 */
/*
 * free a list of stage buffers.
 */
void
free_stage_buffer_list(sqm_lst_t *l)
{
	Trace(TR_ALLOC, "freeing stage_buffer_list");
	if (l != NULL)
		lst_free_deep(l);
	Trace(TR_ALLOC, "finished freeing stage_buffer_list");
}


/*
 * free a list of stage drives.
 */
void
free_stage_drive_list(sqm_lst_t *l)
{

	Trace(TR_ALLOC, "freeing stage_drive_list");
	if (l != NULL) {
		while (l->length != 0) {
			if (l->head->data != NULL) {
				free(l->head->data);
			}
			lst_remove(l, l->head);
		}
		lst_free(l);
	}

	l = NULL;
	if (l != NULL)
		lst_free_deep(l);
	Trace(TR_ALLOC, "finished freeing stage_drive_list");
}


/*
 *	free_stager_cfg()
 *	free memory structure of stager_cfg_t.
 */
void
free_stager_cfg(stager_cfg_t *cfg)
{
	Trace(TR_ALLOC, "freeing stager_cfg");
	if (cfg == NULL)
		return;
	free_stage_drive_list(cfg->stage_drive_list);
	free_stage_buffer_list(cfg->stage_buf_list);
	free(cfg);
	cfg = NULL;
	Trace(TR_ALLOC, "finished freeing stager_cfg");
}


/*
 *	releaser memory free functions
 */

/*
 * free a list of rl_fs_directives.
 */
void
free_list_of_rl_fs_directive(sqm_lst_t *l)
{

	Trace(TR_ALLOC, "freeing list_of_rl_fs_directive");
	if (l != NULL) {
		while (l->length != 0) {
			if (l->head->data != NULL) {
				free(l->head->data);
			}
			lst_remove(l, l->head);
		}
		lst_free(l);
	}

	l = NULL;
	Trace(TR_ALLOC, "finished freeing list_of_rl_fs_directive");
}


/*
 *	archiver free functions.
 */
void
free_vsn_pool_list(sqm_lst_t *l)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing vsn_pool_list");
	if (l != NULL) {
		for (node = l->head; node != NULL; node = node->next) {

			if (node->data != NULL) {
				free_vsn_pool(
				    (vsn_pool_t *)node->data);
			}
		}
		lst_free(l);
	}
	Trace(TR_ALLOC, "finished freeing vsn_pool_list");
}


void
free_vsn_map_list(sqm_lst_t *l)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing list_of_vsn_maps");
	if (l != NULL) {
		for (node = l->head; node != NULL; node = node->next) {

			if (node->data != NULL) {
				free_vsn_map(
				    (vsn_map_t *)node->data);
			}
		}
		lst_free(l);
	}
	Trace(TR_ALLOC, "finished freeing list_of_vsn_maps");
}


/*
 * free a map. Don't add tracing this gets called too much.
 */
void
free_vsn_map(vsn_map_t *map)
{
	node_t *node;
	if (map == NULL) {
		return;
	}

	if (map->vsn_names != NULL) {
		for (node = map->vsn_names->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}
	if (map->vsn_pool_names != NULL) {
		for (node = map->vsn_pool_names->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}

	lst_free(map->vsn_names);
	lst_free(map->vsn_pool_names);
	free(map);
}


/*
 * free a pool. Don't add tracing this gets called too much.
 */
void
free_vsn_pool(vsn_pool_t *pool)
{
	node_t *node;

	if (pool == NULL) {
		return;
	}

	if (pool->vsn_names != NULL) {
		for (node = pool->vsn_names->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}

	lst_free(pool->vsn_names);
	free(pool);

}


/*
 * free an ar_set_copy_params, don't add tracing this gets called too much.
 */
void
free_ar_set_copy_params(ar_set_copy_params_t *p)
{
	node_t *node;
	if (p == NULL)
		return;

	if (p->priority_lst != NULL) {
		for (node = p->priority_lst->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}

	lst_free(p->priority_lst);
	free(p);
}


void
free_ar_set_copy_params_list(sqm_lst_t *l)
{
	node_t *node;

	Trace(TR_ALLOC, "freeing list_of_ar_set_copy_params");

	if (l != NULL) {
		for (node = l->head; node != NULL; node = node->next) {

			if (node->data != NULL) {
				free_ar_set_copy_params(
				    (ar_set_copy_params_t *)node->data);
			}
		}
		lst_free(l);
	}
	Trace(TR_ALLOC, "finished freeing list_of_ar_set_copy_params");
}


void
free_ar_fs_directive(ar_fs_directive_t *fs)
{
	int i;

	Trace(TR_ALLOC, "freeing ar_fs_directive");
	if (fs == NULL)
		return;

	free_ar_set_criteria_list(fs->ar_set_criteria);
	for (i = 0; i < MAX_COPY; i++) {
		if (fs->fs_copy[i] != NULL)
			free(fs->fs_copy[i]);
	}

	free(fs);
	Trace(TR_ALLOC, "finished freeing ar_fs_directive");
}


void
free_ar_fs_directive_list(sqm_lst_t *ar_fs_p)
{
	node_t *node;
	Trace(TR_ALLOC, "freeing ar_fs_directive_list");
	if (ar_fs_p != NULL) {
		for (node = ar_fs_p->head; node != NULL; node = node->next) {

			if (node->data != NULL) {
				free_ar_fs_directive(
				    (ar_fs_directive_t *)node->data);
			}
		}
		lst_free(ar_fs_p);
	}
	Trace(TR_ALLOC, "finished freeing ar_fs_directive_list");
}


void
free_ar_global_directive(ar_global_directive_t *g)
{
	Trace(TR_ALLOC, "freeing ar_global_directive");
	if (g == NULL) {
		return;
	}
	free_buffer_directive_list(g->ar_bufs);
	free_buffer_directive_list(g->ar_max);
	free_drive_directive_list(g->ar_drives);
	free_ar_set_criteria_list(g->ar_set_lst);
	free_buffer_directive_list(g->ar_overflow_lst);
	lst_free_deep(g->timeouts);
	free(g);
	Trace(TR_ALLOC, "finished freeing ar_global_directive");
}


void
free_ar_set_criteria_list(sqm_lst_t *ar_set_list)
{
	node_t *node;
	Trace(TR_ALLOC, "freeing ar_set_criteria_list");
	if (ar_set_list != NULL) {
		for (node = ar_set_list->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free_ar_set_criteria(
				    (ar_set_criteria_t *)node->data);
			}
		}
		lst_free(ar_set_list);
	}
	Trace(TR_ALLOC, "finished freeing ar_set_criteria_list");
}


void
free_ar_set_criteria(ar_set_criteria_t *as)
{
	int i;
	Trace(TR_ALLOC, "freeing ar_set_criteria");
	if (as == NULL)
		return;

	for (i = 0; i < MAX_COPY; i++) {
		if (as->arch_copy[i] != NULL)
			free(as->arch_copy[i]);
	}
	free(as->description);
	free(as->class_attrs);

	free(as);
	Trace(TR_ALLOC, "finished freeing ar_set_criteria");
}


void
free_drive_directive_list(sqm_lst_t *ar_drives)
{
	Trace(TR_ALLOC, "freeing drive_directive_list");
	lst_free_deep(ar_drives);
	Trace(TR_ALLOC, "finished freeing drive_directive_list");
}


void
free_buffer_directive_list(sqm_lst_t *l)
{
	Trace(TR_ALLOC, "freeing buffer_directive_list");
	lst_free_deep(l);
	Trace(TR_ALLOC, "finished freeing buffer_directive_list");
}


void
free_archiver_cfg(archiver_cfg_t *cfg)
{

	Trace(TR_ALLOC, "freeing archiver_cfg");
	if (cfg == NULL)
		return;

	free_ar_fs_directive_list(cfg->ar_fs_p);
	free_ar_set_copy_params_list(cfg->archcopy_params);
	free_vsn_pool_list(cfg->vsn_pools);
	free_vsn_map_list(cfg->vsn_maps);

	/* free the fields of the global directives. */
	free_buffer_directive_list(cfg->global_dirs.ar_bufs);
	free_buffer_directive_list(cfg->global_dirs.ar_max);
	free_drive_directive_list(cfg->global_dirs.ar_drives);
	free_ar_set_criteria_list(cfg->global_dirs.ar_set_lst);
	free_buffer_directive_list(cfg->global_dirs.ar_overflow_lst);
	free(cfg);
	Trace(TR_ALLOC, "finished freeing archiver_cfg");
}


/*
 *	mount_cfg free functioons
 */
void
free_mount_cfg(mount_cfg_t *cfg)
{

	Trace(TR_ALLOC, "freeing mount_cfg");
	if (cfg == NULL)
		return;

	if (cfg->fs_list == NULL) {
		free(cfg);
		return;
	}
	free_list_of_fs(cfg->fs_list);
	free(cfg);
	Trace(TR_ALLOC, "finished freeing mount_cfg");
}


/*
 *	file system free functions.
 */


void
free_fs(fs_t *fs)
{
	if (fs == NULL) {
		return;
	}

	Trace(TR_ALLOC, "freeing fs");


	if (fs->mount_options != NULL) {
		free(fs->mount_options);
	}

	if (fs->meta_data_disk_list != NULL) {
		lst_free_deep(fs->meta_data_disk_list);
	}

	if (fs->data_disk_list != NULL) {
		lst_free_deep(fs->data_disk_list);
	}

	if (fs->striped_group_list != NULL) {
		free_list_of_striped_groups(fs->striped_group_list);
	}


	if (fs->hosts_config) {
		free_list_of_host_info(fs->hosts_config);
	}

	free(fs);
	fs = NULL;

	Trace(TR_ALLOC, "finished freeing fs");
}


void
free_list_of_striped_groups(sqm_lst_t *l)
{
	node_t *n;

	Trace(TR_ALLOC, "freeing list_of_striped_groups");
	if (l == NULL) {
		return;
	}

	for (n = l->head; n != NULL; n = n->next) {
		free_striped_group((striped_group_t *)n->data);
	}
	lst_free(l);
	l = NULL;
	Trace(TR_ALLOC, "finished freeing list_of_striped_groups");
}


void
free_striped_group(striped_group_t *sg)
{
	node_t *n;

	Trace(TR_ALLOC, "freeing striped_group");
	if (sg == NULL) {
		return;
	}

	if (sg->disk_list != NULL) {
		for (n = sg->disk_list->head; n != NULL; n = n->next) {
			free((disk_t *)n->data);
		}

		lst_free(sg->disk_list);
	}
	free(sg);
	sg = NULL;
	Trace(TR_ALLOC, "finished freeing striped_group");
}


void
free_list_of_fs(sqm_lst_t *fs_list)
{
	node_t *n;
	Trace(TR_ALLOC, "freeing list_of_fs");
	if (fs_list == NULL) {
		return;
	}

	for (n = fs_list->head; n != NULL; n = n->next) {
		free_fs((fs_t *)n->data);
	}
	lst_free(fs_list);
	Trace(TR_ALLOC, "finished freeing list_of_fs");
}

void
free_list_of_samfsck_info(sqm_lst_t *info_list)
{
	Trace(TR_ALLOC, "freeing list_of_samfsck_info");

	if (info_list != NULL) {
		lst_free_deep(info_list);
	}

	Trace(TR_ALLOC, "finished freeing list_of_samfsck_info");
}

/*
 *	diskvol memory free functions.
 */

/*
 * free the diskvols_cfg.
 */
void
free_diskvols_cfg(diskvols_cfg_t *dv)
{
	Trace(TR_ALLOC, "freeing diskvols_cfg");
	if (dv == NULL)
		return;

	lst_free_deep(dv->disk_vol_list);
	dv->disk_vol_list = NULL;

	lst_free_deep(dv->client_list);
	dv->client_list = NULL;

	free(dv);
	Trace(TR_ALLOC, "finished freeing diskvols_cfg");
}


/*
 * free a pool. Don't add tracing this gets called too much.
 */
void
free_vsnpool_property(vsnpool_property_t *pool_prop)
{
	node_t *node;

	if (pool_prop == NULL) {
		return;
	}
	if (pool_prop->catalog_entry_list != NULL) {
		for (node = pool_prop->catalog_entry_list->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}
	lst_free(pool_prop->catalog_entry_list);
	free(pool_prop);
}


/*
 * To free list of vsnpool_property, use lst_free_deep_typed()
 * i.e. lst_free_deep_typed(*l, FREEFUNCCAST(free_vsnpool_property));
 */


/*
 * free a stk parameter.
 */
void
free_stk_param(stk_param_t *stk_para)
{
	node_t *node;

	if (stk_para == NULL) {
		return;
	}
	if (stk_para->stk_device_list != NULL) {
		for (node = stk_para->stk_device_list->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}
	lst_free(stk_para->stk_device_list);
	if (stk_para->stk_capacity_list != NULL) {
		for (node = stk_para->stk_capacity_list->head;
		    node != NULL; node = node->next) {

			if (node->data != NULL) {
				free(node->data);
			}
		}
	}
	lst_free(stk_para->stk_capacity_list);
	free(stk_para);
}


/*
 * free nwlib_base_param
 */
void
free_nwlib_base_param(nwlib_base_param_t *param)
{
	if (param == NULL) {
		return;
	}
	lst_free_deep(param->drive_lst);
	free(param);
}


/*
 *	load.h memory free functions.
 */

void
free_pending_load_info(pending_load_info_t *load_info)
{
	Trace(TR_ALLOC, "freeing pending_load_info");

	if (load_info) {
		free(load_info);
	}

	Trace(TR_ALLOC, "finished freeing pending_load_info");
}


void
free_list_of_pending_load_info(sqm_lst_t *load_info_list)
{
	Trace(TR_ALLOC, "freeing pending_load_info");

	if (load_info_list) {
		lst_free_deep(load_info_list);
	}

	Trace(TR_ALLOC, "finished freeing pending_load_info");
}


/*
 *	stage.h memory free functions.
 */

void
free_stager_info(stager_info_t *info)
{
	Trace(TR_ALLOC, "freeing stager_info");

	if (info) {
		if (info->active_stager_info) {
			lst_free_deep(info->active_stager_info);
		}
		if (info->stager_streams) {
			free_list_of_stager_stream(info->stager_streams);
		}
		free(info);
	}

	Trace(TR_ALLOC, "finished freeing stager_info");
}


void
free_stager_stream(stager_stream_t *stream)
{
	Trace(TR_ALLOC, "freeing stager_stream");

	if (stream) {
		if (stream->current_file) {
			free(stream->current_file);
		}
		free(stream);
	}

	Trace(TR_ALLOC, "finished freeing stager_stream");
}


void
free_list_of_active_stager_info(sqm_lst_t *info)
{
	Trace(TR_ALLOC, "freeing list_of_active_stager_info");

	if (info) {
		lst_free_deep(info);
	}

	Trace(TR_ALLOC, "finished freeing list_of_active_stager_info");
}


void
free_list_of_stager_stream(sqm_lst_t *streams)
{
	node_t *n;
	Trace(TR_ALLOC, "freeing list_of_stager_stream");

	if (streams) {
		for (n = streams->head; n != NULL; n = n->next) {
			free_stager_stream((stager_stream_t *)n->data);
		}
		lst_free(streams);
	}

	Trace(TR_ALLOC, "finished freeing list_of_stager_stream");
}


void
free_arch_set(arch_set_t *st) {
	int i;


	if (st == NULL)
		return;

	if (st->criteria != NULL) {
		free_ar_set_criteria_list(st->criteria);
	}

	for (i = 0; i <= MAX_COPY; i++) {
		free_ar_set_copy_params(st->copy_params[i]);
		free_ar_set_copy_params(st->rearch_copy_params[i]);
		free_vsn_map(st->vsn_maps[i]);
		free_vsn_map(st->rearch_vsn_maps[i]);
	}
	free(st->description);
	free(st);
}


void
free_arch_set_list(sqm_lst_t *l) {
	node_t *n;


	Trace(TR_ALLOC, "freeing list of arch_sets");

	if (l != NULL) {
		for (n = l->head; n != NULL; n = n->next) {

			if (n->data != NULL) {
				free_arch_set((arch_set_t *)n->data);
			}
		}
		lst_free(l);
	}

	Trace(TR_ALLOC, "finished freeing list of arch_sets");
}


void
free_license_info(license_info_t *license)
{
	if (license == NULL) {
		return;
	}

	Trace(TR_ALLOC, "freeing license");


	if (license->media_list != NULL) {
		lst_free_deep(license->media_list);
	}

	free(license);
	license = NULL;

	Trace(TR_ALLOC, "finished freeing license");
}


/*
 *  AU free functions (declared in pub/mgmt/device.h)
 */


void
free_au(
au_t *au)
{
	if (NULL == au)
		return;
	free(au->raid);
	if (NULL != au->scsiinfo) {
		free(au->scsiinfo->dev_id);
		free(au->scsiinfo);
	}
	free(au);
}


void
free_au_list(
sqm_lst_t *lst)
{
	node_t *node;

	if (NULL == lst)
		return;
	node = lst->head;
	while (NULL != node) {
		free_au((au_t *)node->data);
		node = node->next;
	}
	lst_free(lst);
}


void
free_host_info(host_info_t *h) {

	Trace(TR_ALLOC, "freeing host_info_t");

	free(h->host_name);
	lst_free_deep(h->ip_addresses);
	free(h);

	Trace(TR_ALLOC, "finished freeing host_info_t");
}

void
free_list_of_host_info(sqm_lst_t *l) {
	node_t *n;

	Trace(TR_ALLOC, "freeing list of host_info_t");

	if (l) {
		for (n = l->head; n != NULL; n = n->next) {
			free_host_info((host_info_t *)n->data);
		}
		lst_free(l);
	}

	Trace(TR_ALLOC, "finished freeing list of host_info_t");
}

void
free_crypt_str(crypt_str_t *cs) {

	if (cs == NULL) {
		return;
	}
	if (cs->str != NULL) {
		if (cs->str_len > 0) {
			memset(cs->str, 0, (size_t)cs->str_len);
		}
		free(cs->str);
	}
	free(cs);
}

/*
 *      free_medias_type().
 */
void
free_medias_type(
medias_type_t *mtype)
{
	if (mtype == NULL) {
		return;
	}
	if (mtype->sam_id != NULL) {
		free(mtype->sam_id);
	}
	if (mtype->vendor_id != NULL) {
		free(mtype->vendor_id);
	}
	if (mtype->product_id != NULL) {
		free(mtype->product_id);
	}
	free(mtype);
}
