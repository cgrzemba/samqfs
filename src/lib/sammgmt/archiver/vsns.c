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
#pragma	ident	"$Revision: 1.22 $"

static char *_SrcFile = __FILE__;

#include <libgen.h> /* regcmp */
#include <strings.h>
#include <stdlib.h>

#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/diskvols.h"
#include "pub/mgmt/archive.h"
#include "mgmt/util.h"
#include "mgmt/config/media.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/archive_sets.h"
#include "sam/sam_trace.h"

/* DISK_MEDIA is defined in device.h */

/*
 * Replace these:
 * int get_vsn_list(ctx_t *ctx, const char *vsn_reg_exp,
 *	int start,int size,vsn_sort_key_t sort_key,
 *	boolean_t ascending, sqm_lst_t **catalog_entry_list);
 *
 * int get_available_vsns(ctx_t *ctx, upath_t archive_vsn_pool_name,
 *	int start, int size, vsn_sort_key_t sort_key,
 *	boolean_t ascending, sqm_lst_t **catalog_entry_list);
 *
 * int get_properties_of_archive_vsnpool(ctx_t *ctx, const upath_t pool_name,
 *	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
 *	vsnpool_property_t **vsnpool_prop);
 */

static int get_diskvsns_for_map(ctx_t *c, vsn_map_t *map, int start,
	int count, vsn_sort_key_t key, boolean_t ascending,
	vsnpool_property_t **vpp);

static int get_diskvsns_for_pool(vsn_pool_t *pool, vsnpool_property_t **vpp);

static int get_matching_disk_vsns(char *media_type,
    sqm_lst_t *regexps, sqm_lst_t *diskvols, vsnpool_property_t *vpp);


int sort_disk_vol_list(int start, int count, vsn_sort_key_t sort_key,
	boolean_t ascending, sqm_lst_t *dvols);


/* Sorting functions for disk_vol_t */
static int dv_comp_vsn_asc(const void *p1, const void *p2);
static int dv_comp_vsn_des(const void *p1, const void *p2);
static int dv_comp_freespace_asc(const void *p1, const void *p2);
static int dv_comp_freespace_des(const void *p1, const void *p2);
static int dv_comp_host_asc(const void *p1, const void *p2);
static int dv_comp_host_des(const void *p1, const void *p2);
static int dv_comp_path_asc(const void *p1, const void *p2);
static int dv_comp_path_des(const void *p1, const void *p2);

/* Sorting function for vsnpool_property usage */
static int comp_vsnpool_prop(void *p1, void *p2);


/*
 * get information about the vsns assigned to a pool.
 * This function will return a vsnpool properties object that describes
 * the number, freespace and capacity of ALL of the assigned vsns.
 * In addition it will return a list of the vsns (either catalog_entry_t
 * for tape/optical media or disk_vol_t for disk volumes.
 * Which elements go into the list will be determined based on the input
 * arguments start, count, sort key and ascending. The list will not
 * necessarily contain all of the vsns included in the map.
 */
int
get_vsn_pool_properties(
ctx_t *ctx,
vsn_pool_t *pool,
int start,
int count,
vsn_sort_key_t key,
boolean_t ascending,
vsnpool_property_t **vpp)
{


	if (ISNULL(pool, vpp)) {
		return (-1);
	}

	Trace(TR_OPRMSG, "get pool props- %s, %d to %d by %d %d",
	    Str(pool->pool_name), start, count, key, ascending);


	if ((strcmp(pool->media_type, DISK_MEDIA) == 0) ||
	    (strcmp(pool->media_type, STK5800_MEDIA) == 0)) {

		if (get_diskvsns_for_pool(pool, vpp) != 0) {
			return (-1);
		}
		sort_disk_vol_list(start, count, key,
		    ascending, (*vpp)->catalog_entry_list);

	} else {
		/* removable media code */
		if (get_properties_of_archive_vsnpool1(ctx, *pool,
		    start, count, key, ascending, vpp) != 0) {
			return (-1);
		}
	}

	return (0);
}


int
get_vsn_map_properties(
ctx_t *ctx,
vsn_map_t *map,
int start,
int count,
vsn_sort_key_t sort_key,
boolean_t ascending,
vsnpool_property_t **vpp)
{


	if (ISNULL(map, vpp)) {
		return (-1);
	}

	Trace(TR_OPRMSG, "get map props- %s, %d to %d by %d %d",
	    Str(map->ar_set_copy_name), start, count, sort_key, ascending);

	if (strcmp(map->media_type, DISK_MEDIA) == 0 ||
	    strcmp(map->media_type, STK5800_MEDIA) == 0) {

		if (get_diskvsns_for_map(ctx, map, start, count,
		    sort_key, ascending, vpp) != 0) {

			return (-1);
		}
	} else {
		if (get_media_for_map(ctx, map, start, count,
		    sort_key, ascending, vpp) != 0) {

			return (-1);
		}

	}

	return (0);
}


/*
 * This function should not be called if you have already processed
 * some diskvols as it will fetch the diskvols again and might result
 * in duplication. This function does not sort the results and should not.
 */
static int
get_diskvsns_for_pool(vsn_pool_t *pool, vsnpool_property_t **vpp) {
	sqm_lst_t *dvols = NULL;



	*vpp = (vsnpool_property_t *)mallocer(sizeof (vsnpool_property_t));
	if (*vpp == NULL) {
		setsamerr(SE_NO_MEM);
		return (-1);
	}
	memset(*vpp, 0, sizeof (vsnpool_property_t));
	(*vpp)->catalog_entry_list = lst_create();
	if ((*vpp)->catalog_entry_list == NULL) {
		free_vsnpool_property(*vpp);
		return (-1);
	}
	strlcpy((*vpp)->name, pool->pool_name, sizeof (uname_t));
	strlcpy((*vpp)->media_type, pool->media_type, sizeof (mtype_t));
	if (get_all_disk_vols(NULL, &dvols) != 0) {
		/* free resources allocated here */
		free_vsnpool_property(*vpp);
		return (-1);
	}

	if (get_matching_disk_vsns(pool->media_type, pool->vsn_names,
	    dvols, *vpp) != 0) {
		lst_free_deep(dvols);
		free_vsnpool_property(*vpp);
		return (-1);
	}

	/* set the number of vsns now prior to sorting */
	(*vpp)->number_of_vsn = (*vpp)->catalog_entry_list->length;
	lst_free_deep(dvols);
	return (0);
}


static int
get_diskvsns_for_map(
ctx_t *c,
vsn_map_t *map,
int start,
int count,
vsn_sort_key_t key,
boolean_t ascending,
vsnpool_property_t **vpp)
{

	sqm_lst_t *dvols = NULL;
	sqm_lst_t *pools = NULL;
	node_t *n;


	if (ISNULL(map, vpp)) {
		return (-1);
	}

	/*
	 * Steps:
	 * 1. include any explicitly defined vsns and expand any
	 *    regular expressions
	 * 2. Expand any pools using the same name expansion code.
	 */
	*vpp = (vsnpool_property_t *)mallocer(sizeof (vsnpool_property_t));
	if (*vpp == NULL) {
		return (-1);
	}
	memset(*vpp, 0, sizeof (vsnpool_property_t));
	(*vpp)->catalog_entry_list = lst_create();
	if ((*vpp)->catalog_entry_list == NULL) {
		free_vsnpool_property(*vpp);
		return (-1);
	}
	strlcpy((*vpp)->name, map->ar_set_copy_name, sizeof (uname_t));
	strlcpy((*vpp)->media_type, map->media_type, sizeof (mtype_t));


	if (get_all_disk_vols(NULL, &dvols) != 0) {
		goto err;
	}

	if (get_matching_disk_vsns(map->media_type, map->vsn_names,
	    dvols, *vpp) != 0) {
		goto err;
	}

	/*
	 * if there are no pools included, sort the result and
	 * return success.
	 */
	if (map->vsn_pool_names == NULL || map->vsn_pool_names->length == 0) {

		/* set the count before sorting/trimming */
		(*vpp)->number_of_vsn = (*vpp)->catalog_entry_list->length;

		sort_disk_vol_list(start, count, key, ascending,
		    (*vpp)->catalog_entry_list);

		lst_free_deep(dvols);
		return (0);
	}

	/*
	 * Don't use get_diskvsns_for_pool because it would fetch the pools on
	 * each invocation and could result in incusion of duplicate diskvsns.
	 */
	if (get_all_vsn_pools(c, &pools) != 0) {
		goto err;
	}
	for (n = map->vsn_pool_names->head; n != NULL; n = n->next) {
		vsn_pool_t *pool;
		node_t *in;
		char *name = (char *)n->data;

		for (in = pools->head; in != NULL; in = in->next) {
			pool = (vsn_pool_t *)in->data;
			if (strcmp(pool->pool_name, name) != 0) {
				continue;
			}

			if (get_matching_disk_vsns(pool->media_type,
			    pool->vsn_names, dvols, *vpp) != 0) {
				goto err;
			}
		}

	}

	/* set the count before sorting/trimming */
	(*vpp)->number_of_vsn = (*vpp)->catalog_entry_list->length;

	sort_disk_vol_list(start, count, key, ascending,
	    (*vpp)->catalog_entry_list);

	free_vsn_pool_list(pools);
	lst_free_deep(dvols);
	return (0);

err:

	/* free stuff and return error */
	free_vsn_pool_list(pools);
	lst_free_deep(dvols);
	free_vsnpool_property(*vpp);
	*vpp = NULL;

	return (-1);
}




/*
 * vsnpool_properties must already be created
 * when this gets called.
 */
static int
get_matching_disk_vsns(
char *media,
sqm_lst_t *regexps,
sqm_lst_t *dvols,
vsnpool_property_t *vpp) {

	node_t *n;
	node_t *dn;


	if (ISNULL(regexps, dvols, vpp, media)) {
		return (-1);
	}

	/*
	 * compare a list of regexps to a list of diskvols, create
	 * a list diskvols that match any of the regexps
	 */
	for (n = regexps->head; n != NULL; n = n->next) {
		char *exp = (char *)n->data;
		char *cmp_exp = regcmp(exp, NULL);


		if (cmp_exp == NULL) {
			return (-1);
		}

		for (dn = dvols->head; dn != NULL; dn = dn->next) {
			disk_vol_t *dv = (disk_vol_t *)dn->data;
			char *reg_rtn;

			/*
			 * If the media type of the volume does not
			 * match the requested media type continue.
			 * The disk volume structure indicates honeycomb
			 * volumes with the DV_STK5800_VOL status flag.
			 */
			if (dv == NULL) {
				continue;
			}
			if ((strcmp(media, STK5800_MEDIA) == 0 &&
			    !(dv->status_flags & DV_STK5800_VOL)) ||
			    (strcmp(media, DISK_MEDIA) == 0 &&
			    dv->status_flags & DV_STK5800_VOL)) {
				continue;
			}

			reg_rtn = regex(cmp_exp, dv->vol_name);
			if (reg_rtn == NULL) {
				/* regexp did not match */
				continue;
			}

			/* add the diskvol to the output */
			if (lst_append(vpp->catalog_entry_list, dv) != 0) {
				free(cmp_exp);
				goto err;
			}

			vpp->capacity += (dv->capacity / 1024);
			vpp->free_space += (dv->free_space / 1024);
			vpp->number_of_vsn++;

			/*
			 * Remove the data from the list so we don't over
			 * count and because it has been inserted in
			 * the vpp
			 */
			dn->data = NULL;


		}
		free(cmp_exp);
	}
	return (0);

err:
	/*
	 * nothing to free, elements are moved from one struct to another
	 * but nothing is created
	 */
	return (-1);
}





int
sort_disk_vol_list(
int start,
int count,
vsn_sort_key_t sort_key,
boolean_t ascending,
sqm_lst_t *dvols)
{


	node_t *n;
	disk_vol_t **dvs;
	int i;
	int length = 0;
	int (*comp) (const void *, const void *);
	node_t *the_rest = NULL;

	/*
	 * if the first element is to be higher than the length of the
	 * list then nothing will be returned. Remove all of the elements
	 * free them and return the empty list.
	 */
	if (start > dvols->length) {
		while (dvols->head != NULL) {
			free(dvols->head->data);
			lst_remove(dvols, dvols->head);
		}
		return (0);
	}


	/* a -1 means get all */
	if (count == -1) {
		count = dvols->length;
	}

	/*
	 * convert to array index terms from the pure count entered
	 * by the caller. For the old tape function a -1 start value
	 * was legal. So if there is a -1 start convert it to zero.
	 */
	if (start > 0) {
		start = start - 1;
	} else if (start == -1) {
		start = 0;
	}


	/* determine sort key and order */
	switch (sort_key) {
	case VSN_SORT_BY_VSN:
		if (ascending) {
			comp = dv_comp_vsn_asc;
		} else {
			comp = dv_comp_vsn_des;
		}
		break;

	case VSN_SORT_BY_SLOT:
		/*
		 * Sort by slot makes no sense for disk vsns but
		 * GUI requested it map to sorting by vsn name
		 */
		if (ascending) {
			comp = dv_comp_vsn_asc;
		} else {
			comp = dv_comp_vsn_des;
		}
		break;

	case VSN_SORT_BY_PATH:
		if (ascending) {
			comp = dv_comp_path_asc;
		} else {
			comp = dv_comp_path_des;
		}
		break;
	case VSN_SORT_BY_HOST:
		if (ascending) {
			comp = dv_comp_host_asc;
		} else {
			comp = dv_comp_host_des;
		}
		break;
	case VSN_SORT_BY_FREESPACE:
		if (ascending) {
			comp = dv_comp_freespace_asc;
		} else {
			comp = dv_comp_freespace_des;
		}
		break;
	default:
		/* VSN_NO_SORT */
		comp = NULL;
	}

	if (comp == NULL) {
		trim_list(dvols, start, count);

		return (0);
	}


	/* create an array of pointers to the elements in the input */
	dvs = (disk_vol_t **)calloc(sizeof (disk_vol_t *), dvols->length);
	if (dvs == NULL) {
		return (-1);
	}

	/* put elements into the array */
	for (n = dvols->head, i = 0; n != NULL; n = n->next, i++) {
		dvs[i] = (disk_vol_t *)n->data;
		n->data = NULL;
	}


	/* sort the array */
	qsort(dvs, dvols->length, sizeof (disk_vol_t *), comp);

	/*
	 * traverse the sorted array assigning back into the input list
	 * but now in sorted order. Once the end of the requested data is
	 * met, setup the tail pointer of the list to the new end,
	 * continue to itterate throught the list and array freeing
	 * diskvols and nodes.
	 */
	for (i = 0, n = dvols->head; i < dvols->length; i++) {
		/* get rid of unneeded elements at the head of the list */
		if (i < start) {
			free(dvs[i]);
			continue;
		} else if (i >= (start + count)) {
			node_t *tmp;

			/*
			 * free the unneeded elements and the nodes
			 * that extend past the requested portion of the list
			 */
			free(dvs[i]);
			tmp = n;
			n = n->next;
			the_rest = n;
			free(tmp);
			continue;
		} else if (i == (start + count - 1)) {
			node_t *tmp;
			/* set the tail to the last requested node */

			n->data = dvs[i];
			dvols->tail = n;
			tmp = n->next;
			the_rest = n->next;
			n->next = NULL;
			n = tmp;
			length++;
		} else {

			/*
			 * put the element into the list and
			 * increment the node n
			 */
			n->data = dvs[i];
			if (i == (dvols->length - 1)) {
				dvols->tail = n;
				the_rest = n->next;
				n->next =  NULL;
			} else {

				n = n->next;
			}
			length++;
		}
	}

	/*
	 * if there are nodes past the end of the requested portion of the
	 * list free them. This loop actually handles cases where data from
	 * the beginning was not included in the returned list. So cases where
	 * start > 0. Because in those cases there would be elements in the
	 * list that were not handled in the above loop.
	 */
	if (length < i) {
		while (the_rest != NULL) {
			node_t *tmp = the_rest->next;
			free(the_rest);
			the_rest = tmp;
		}
	}

	dvols->length = length;

	free(dvs);
	return (0);
}


static int
dv_comp_vsn_asc(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p1)->vol_name,
	    (*(disk_vol_t **)p2)->vol_name));
}


static int
dv_comp_vsn_des(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p2)->vol_name,
	    (*(disk_vol_t **)p1)->vol_name));
}


static int
dv_comp_freespace_asc(const void *p1, const void *p2) {
	return ((int)((*(disk_vol_t **)p1)->free_space -
	    (*(disk_vol_t **)p2)->free_space));
}


static int
dv_comp_freespace_des(const void *p1, const void *p2) {
	return ((int)((*(disk_vol_t **)p2)->free_space -
	    (*(disk_vol_t **)p1)->free_space));
}


static int
dv_comp_host_asc(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p1)->host,
	    (*(disk_vol_t **)p2)->host));
}


static int
dv_comp_host_des(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p2)->host,
	    (*(disk_vol_t **)p1)->host));
}


static int
dv_comp_path_asc(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p1)->path,
	    (*(disk_vol_t **)p2)->path));
}


static int
dv_comp_path_des(const void *p1, const void *p2) {
	return (strcmp((*(disk_vol_t **)p2)->path,
	    (*(disk_vol_t **)p1)->path));
}

/*
 * get utilization of archive copies sorted by usage
 * This can be used to get copies with low free space
 *
 * Input:
 *	int count	- n copies with top usage
 *
 * Returns a list of formatted strings
 *	name=copy name
 *	type=
 *	capacity=capacity in kbytes
 *	free=freespace in kbytes
 *	usage=%
 */
int
get_copy_utilization(
ctx_t *ctx,		/* ARGSUSED */
int count,		/* input - n copies with top usage */
sqm_lst_t **strlst)	/* return - list of formatted strings */
{
	char buffer[BUFSIZ] = {0};
	sqm_lst_t *archsets = NULL; /* list of all archive sets */
	sqm_lst_t *vsnprops = NULL; /* list of all vsnpool_property */
	node_t *node = NULL;
	int i = 0;
	int ret = -1;

	if (ISNULL(strlst)) {
		Trace(TR_ERR, "get copy utilization failed: %s",
		    samerrmsg);
		return (-1);
	}

	vsnprops = lst_create();
	if (vsnprops == NULL) {
		Trace(TR_ERR, "get copy utilization failed: %s",
		    samerrmsg);
		return (-1);
	}

	if ((get_all_arch_sets(NULL, &archsets)) != 0) {
		Trace(TR_ERR, "get copy utilization failed: %s",
		    samerrmsg);
		lst_free(vsnprops);
		return (-1);
	}

	/*
	 * Each archive set consists of upto 4 vsn_maps (one per copy)
	 * Get the vsn mappings for each copy, these have the capacity
	 * and free space
	 * sort the vsnpool_property list by usage
	 * Return the top # (count) requested
	 */
	node = archsets->head;
	while (node != NULL) {
		arch_set_t *ar_set = (arch_set_t *)node->data;
		vsnpool_property_t *vsnprop = NULL;

		/*  vsn_maps are indexed by copy number - 1 */
		for (i = 0; i < (MAX_COPY + 1); i++) {
			if (ar_set->vsn_maps[i] == NULL) {
				continue;
			}
			if (get_vsn_map_properties(
			    NULL,
			    ar_set->vsn_maps[i],
			    0, /* start */
			    -1, /* get all */
			    VSN_NO_SORT,
			    B_TRUE,
			    &vsnprop) == 0) {

				ret = lst_append(vsnprops, vsnprop);
				if (ret != 0) {
					Trace(TR_ERR, "get copy util err:%s",
					    samerrmsg);
				}
			} else {
				Trace(TR_ERR, "get copy util err:%s",
				    samerrmsg);
			}
		}
		node = node->next;
	}
	free_arch_set_list(archsets);

	/* sort the vsnmaps by usage */
	lst_sort(vsnprops, comp_vsnpool_prop);

	*strlst = lst_create();
	if (*strlst == NULL) {
		lst_free_deep_typed(vsnprops,
		    FREEFUNCCAST(free_vsnpool_property));
		return (-1);
	}

	/* request to return only top count */
	for (i = 0, node = vsnprops->head;
	    (node != NULL && i < count);
	    i++, node = node->next) {

		vsnpool_property_t *prop = (vsnpool_property_t *)node->data;

		int percent_usage = 0;
		/* 100% utilization will have free space = 0 */
		if (prop->capacity != 0) {
			double usage =
			    (double)(prop->capacity - prop->free_space) /
			    (double)prop->capacity;

			percent_usage = (int)(usage * 100);
		}
		snprintf(buffer, sizeof (buffer),
		    "%s=%s,%s=%s,%s=%lld,%s=%lld,%s=%d",
		    KEY_NAME, prop->name,
		    KEY_TYPE, prop->media_type,
		    KEY_CAPACITY, prop->capacity,
		    KEY_FREE, prop->free_space,
		    KEY_USAGE, percent_usage);

		ret = lst_append(*strlst, strdup(buffer));
		if (ret != 0) {
			Trace(TR_ERR, "get copy util err: %s",
			    samerrmsg);
		}
	}
	lst_free_deep_typed(vsnprops,
	    FREEFUNCCAST(free_vsnpool_property));
	return (0);
}

static int
comp_vsnpool_prop(void *p1, void *p2) {
	vsnpool_property_t *vp1 = (vsnpool_property_t *)p1;
	vsnpool_property_t *vp2 = (vsnpool_property_t *)p2;

	double usage1 = 1.0;
	int percent_usage1 = 0;
	double usage2 = 1.0;
	int percent_usage2 = 0;

	/* compare the usage (%) rather than just free space */
	/* If freespace is 0, don't calculate usage % */

	if (vp1 != NULL && vp1->capacity != 0 && vp1->free_space != 0) {
		usage1 = (double)vp1->free_space / (double)vp1->capacity;
	}
	if (vp2 != NULL && vp2->capacity != 0 && vp2->free_space != 0) {
		usage2 = (double)vp2->free_space / (double)vp2->capacity;
	}
	percent_usage1 = (int)(usage1 * 100);
	percent_usage2 = (int)(usage2 * 100);
	return (percent_usage1 - percent_usage2);
}
