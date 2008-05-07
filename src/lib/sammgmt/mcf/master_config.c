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
#pragma ident   "$Revision: 1.39 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */



/*
 * master_config.c
 * Used to read the device and family set configuration and
 * can be used directly to add to the configuration by inserting/removing
 * and appending to family sets.
 */

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

#include "pub/mgmt/types.h"
#include "sam/sam_trace.h"
#include "sam/devnm.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/config/master_config.h"
#include "mgmt/config/cfg_fs.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/config/common.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/filesystem.h"

/* full path to the mcf file */
static char *mcf_file = MCF_CFG;


/*
 * private functions.
 */
static int write_mcf(const char *filename, const mcf_cfg_t *mcf);
static int write_mcf_line(FILE *file, base_dev_t *dev);
static int cpy_group_id(devtype_t output, devtype_t input);

static int check_types(const mcf_cfg_t *cfg, const char *family_set,
	sqm_lst_t *ret_lst);

static int pick_eq_from_gap(sqm_lst_t *used, int eqs_needed,
	int round, int gap);


/* private functions added to support comments in the mcf file */
static char *get_next_entry(char *buf, char *entry);
int find_dev_group_by_name(FILE *f, char *name, char **pre_set_region,
	char **comment_region, char **set_region, char **post_set_region);
static int append_to_region(char **dst, int *space_avail, char *src);
static int read_mcf_line(FILE *f, base_dev_t **d, char **contents);
static char *get_next_entry(char *buf, char *entry);
static int insert_region_for_devs(sqm_lst_t *regions, sqm_lst_t *devs);
static int check_and_write_mcf(ctx_t *ctx, sqm_lst_t *regions);
static int write_mcf_from_regions(char *filename, sqm_lst_t *regions);
static int add_new_header(FILE *f, sqm_lst_t *regions, time_t *the_time);
static int get_remainder_of_file(FILE *f, sqm_lst_t *regions);
static int verify_mcf_from_regions(sqm_lst_t *l);

#define	MAX_MCF_LINE 306
#define	iscontinue(c) (c == '\\' ? B_TRUE : B_FALSE)
#define	DEF_REGION_INCR 1024
#define	FAM_SET_COMMENT_FMT "#\n#\n#%s:\n#  added by fsmgmtd %s#\n"
#define	add_comment_fmt "#  grown by fsmgmtd %s"
#define	add_comment_fmt_w_set "#%s:\n#  grown by fsmgmtd %s"
#define	TWO_COMMENT_LINES "#\n#\n"



/*
 * Given an equipment ordinal, get its base_dev_t structure.  Sets mcf_dev
 * to NULL and returns -1 if no device with the ordinal is found.
 */
int
get_device_by_eq(
const mcf_cfg_t *cfg,	/* cfg to search for the device */
const equ_t eq,		/* eq of the device to get */
base_dev_t **ret_val)	/* malloced return value */
{

	node_t *node;
	base_dev_t *dev;

	Trace(TR_OPRMSG, "getting device_by_eq");

	if (ISNULL(cfg, ret_val)) {
		Trace(TR_OPRMSG, "getting device by eq number failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;

			if (dev->eq == eq) {
				*ret_val = (base_dev_t *)mallocer(
				    sizeof (base_dev_t));

				if (*ret_val == NULL) {
					Trace(TR_OPRMSG, "%s %s",
					    "getting device by eq number ",
					    samerrmsg);

					return (-1);
				}

				dev_cpy(*ret_val, dev);
				Trace(TR_OPRMSG, "got device by eq number 0");
				return (0);
			}
		}
	}

	*ret_val = NULL;
	samerrno = SE_DEVICE_DOES_NOT_EXIST;
	/* Device %d does not exist */
	snprintf(samerrmsg, MAX_MSG_LEN,
	    GetCustMsg(SE_DEVICE_DOES_NOT_EXIST), eq);

	Trace(TR_OPRMSG, "getting device by eq number failed: %s", samerrmsg);
	return (-1);
}


/*
 *  Given a family set name, get a list of devices which belong to
 *  that family set.
 */
int
get_family_set_devs(
const mcf_cfg_t *cfg,	/* cfg to search */
const char *family_set,	/* family set name for which to get devices */
sqm_lst_t **mcf_dev_list)	/* malloced list of base_dev_t */
{

	node_t  *node;
	base_dev_t *dev;
	base_dev_t *dest;

	Trace(TR_OPRMSG, "getting family set devices for %s",
	    Str(family_set));

	if (ISNULL(cfg, family_set)) {
		Trace(TR_OPRMSG, "getting family set devices failed: %s",
		    samerrmsg);
		return (-1);
	}


	*mcf_dev_list = lst_create();
	if (*mcf_dev_list == NULL) {
		Trace(TR_OPRMSG, "getting family set devices failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;

			if (strcmp(dev->set, family_set) == 0) {
				dest = (base_dev_t *)mallocer(
				    sizeof (base_dev_t));

				if (dest == NULL) {
					lst_free_deep(*mcf_dev_list);
					Trace(TR_OPRMSG, "%s%s",
					    "getting family set devices ",
					    samerrmsg);
					return (-1);
				}

				dev_cpy(dest, dev);
				if (lst_append(*mcf_dev_list, dest) != 0) {
					lst_free_deep(*mcf_dev_list);
					free(dest);
					Trace(TR_OPRMSG, "%s%s",
					    "getting family set devices ",
					    samerrmsg);
					return (-1);
				}
			}
		}
	}

	if ((*mcf_dev_list)->length == 0) {

		lst_free(*mcf_dev_list);
		samerrno = SE_FAMILY_SET_DOES_NOT_EXIST;
		/* Family set %s does not exist */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FAMILY_SET_DOES_NOT_EXIST), family_set);

		Trace(TR_OPRMSG, "getting family set devices failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "got family set devices for %s %d devs",
	    family_set, (*mcf_dev_list)->length);
	return (0);
}


/*
 * Given an equipment identifier, get its base_dev_t
 * structure. Returns -1 if no device with the name is
 * found
 */
int
get_mcf_dev(
const mcf_cfg_t *cfg,	/* cfg to search */
const char *name,	/* name of device to get */
base_dev_t **val)	/* malloced return value */
{

	node_t *node;
	base_dev_t *dev;

	Trace(TR_OPRMSG, "getting mcf dev %s", Str(name));

	if (ISNULL(cfg, name)) {
		Trace(TR_OPRMSG, "getting mcf device failed: %s", samerrmsg);
		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;
			if (strcmp(dev->name, name) == 0) {
				*val = (base_dev_t *)mallocer(
				    sizeof (base_dev_t));

				if (*val == NULL) {
					Trace(TR_OPRMSG, "%s %s",
					    "getting mcf device",
					    samerrmsg);

					return (-1);
				}

				dev_cpy(*val, dev);

				Trace(TR_OPRMSG, "got mcf device");
				return (0);
			}
		}
	}
	*val = NULL;
	samerrno = SE_DEVICE_DOES_NOT_EXIST;
	/* Device %s does not exist */
	snprintf(samerrmsg, MAX_MSG_LEN,
	    GetCustMsg(SE_DEVICE_DOES_NOT_EXIST), name);

	Trace(TR_OPRMSG, "getting mcf device failed: %s", samerrmsg);
	return (-1);

}


/*
 * A list of base_dev_t under family_set and state = DEV_ON e.g.
 * if family_set is null all devices with the indicated state will be returned.
 * if family_set is non-null and does not exist -1 will be returned and
 *    samerrno will be set to SE_FAMILY_SET_DOES_NOT_EXIST.
 */
int get_mcf_dev_list(
const mcf_cfg_t *cfg,	/* cfg to search */
const char *family_set,	/* family set from which to select devices */
const dstate_t state,	/* state of devices to select */
sqm_lst_t **devs)		/* malloced return list of base_dev_t */
{

	node_t *node;
	base_dev_t *dev;
	sqm_lst_t *tmp;
	int i;

	Trace(TR_OPRMSG, "getting mcf device list for %s",
	    Str(family_set));

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "getting mcf device list failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (family_set != NULL) {
		/* this is just a check to see if set exists. */
		if (get_family_set_devs(cfg, family_set, &tmp) != 0) {
			/* leave samerrno and samerrmsg as set */
			Trace(TR_OPRMSG, "getting mcf device list failed: %s",
			    samerrmsg);
			return (-1);
		}
		lst_free_deep(tmp);
	}

	/* Count valid states and make sure state is in range */
	for (i = 0; dev_state[i] != NULL; i++)
		;
	if (state < 0 || state >= i) {
		samerrno = SE_INVALID_DEVICE_STATE;

		/* Invalid device state %d */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DEVICE_STATE), state);

		Trace(TR_OPRMSG, "getting mcf device list failed: %s",
		    samerrmsg);

		return (-1);
	}


	*devs = lst_create();
	if (*devs == NULL) {
		Trace(TR_OPRMSG, "getting mcf device list failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		dev = (base_dev_t *)node->data;
		if (family_set == NULL ||
		    strcmp(dev->set, family_set) == 0) {
			if (dev->state == state) {

				base_dev_t *cpy = (base_dev_t *)mallocer(
				    sizeof (base_dev_t));
				if (cpy == NULL) {
					lst_free_deep(*devs);
					Trace(TR_OPRMSG, "%s failed: %s",
					    "getting mcf device list",
					    samerrmsg);
					return (-1);
				}

				dev_cpy(cpy, dev);
				if (lst_append(*devs, cpy) != 0) {
					lst_free_deep(*devs);
					free(cpy);
					Trace(TR_OPRMSG, "%s failed: %s",
					    "getting mcf device list",
					    samerrmsg);
					return (-1);
				}
			}
		}
	}

	Trace(TR_OPRMSG, "got mcf device list for %s", Str(family_set));

	return (0);
}


/*
 * eq_ordinal can range from 1 - 65535 but should be kept small to minimize the
 * size of tables in the file system code.
 *
 * if there are n devices in the mcf, there must be a c free numbers somewhere
 * in the first n+c equipment ordinals.  Must consider all devices in mcf
 * before free can be selected since the numbers are not necessarily used in
 * order.
 */
int
get_available_eq_ord(
const mcf_cfg_t *cfg,	/* cfg as input */
sqm_lst_t **ret_val,	/* malloced list of equ_t */
int count)		/* how many eqs to return */
{

	boolean_t	*is_used;
	int		dev_cnt = 0;
	uint16_t	i;
	base_dev_t	*dev;
	node_t		*node;
	sqm_lst_t		*l;

	Trace(TR_OPRMSG, "getting available eq ordinals");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "getting available eq ordinals failed: %s",
		    samerrmsg);
		return (-1);
	}


	dev_cnt = cfg->mcf_devs->length;

	/*
	 * allocate dev_cnt + c + 1 booleans so we can 1 index the array and
	 * also have 1 extra for the case where all eq numbers are sequential
	 * and start at 1. Calloc initializes to 0(so B_FALSE)
	 */
	is_used = (boolean_t *)calloc((dev_cnt + 1 + count),
	    sizeof (boolean_t));

	/* now set flags for each devices eq to true if in range. */
	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		dev = (base_dev_t *)node->data;
		if (dev->eq < dev_cnt + count) {
			is_used[dev->eq] = B_TRUE;
		}
	}


	l = lst_create();
	if (l == NULL) {
		free(is_used);
		Trace(TR_OPRMSG, "getting available eq ordinals failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* now find an entry that has not been used. */
	for (i = 1; i <= dev_cnt + count + 1; i++) {
		if (is_used[i] == B_FALSE) {
			equ_t *tmp_int;
			tmp_int = (equ_t *)mallocer(sizeof (equ_t));
			if (tmp_int == NULL) {
				free(is_used);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting available eq ordinals",
				    samerrmsg);
				return (-1);
			}

			*tmp_int = i;
			if (lst_append(l, tmp_int) != 0) {
				free(is_used);
				free(tmp_int);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting available eq ordinals",
				    samerrmsg);
				return (-1);
			}
			if (l->length == count) {
				break;
			}
		}
	}

	free(is_used);
	if (l->length != count) {
		samerrno = SE_INSUFFICIENT_EQS;

		/* There are not %d available equipment ordinals */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INSUFFICIENT_EQS), count);

		Trace(TR_OPRMSG, "getting available eq ordinals failed: %s",
		    samerrmsg);

		return (-1);
	}
	*ret_val = l;

	Trace(TR_OPRMSG, "got available eq ordinals");
	return (0);
}


/*
 * returns true if group_id is available.
 */
boolean_t
is_striped_group_id_available(
mcf_cfg_t *mcf,		/* cfg to check against */
devtype_t group_id)	/* group id to check */
{

	node_t *n;

	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {
		if (strcmp(((base_dev_t *)n->data)->equ_type,
		    group_id) == 0) {

			return (B_FALSE);
		}
	}
	return (B_TRUE);
}


/*
 * returns count striped group ids that are free for the named fs.
 * if the fs does not exist the first available striped group name will
 * be returned so g0. Does not return an error if none are available. This
 * condition must be externally checked and would be indicated by a return
 * list length of 0.
 */
int
get_available_striped_group_id(
mcf_cfg_t *mcf,		/* mcf to check against */
uname_t fs_name,	/* name of fs that the striped group is for */
sqm_lst_t **ret_list,	/* malloced list of devtype_t */
int count)		/* how many ids to return */
{

	devtype_t *val;
	sqm_lst_t *fs_devs;
	sqm_lst_t *g_ids;
	node_t *n;
	int i, num_type;

	Trace(TR_OPRMSG, "getting available striped group ids");
	if (ISNULL(mcf, fs_name, ret_list)) {
		Trace(TR_OPRMSG, "getting available striped group ids %s%s",
		    " failed: ", samerrmsg);
		return (-1);
	}
	if (count == 0) {
		return (0);
	}

	g_ids = lst_create();
	if (g_ids == NULL) {
		Trace(TR_OPRMSG, "getting available striped group ids %s%s",
		    " failed: ", samerrmsg);
		return (-1);
	}

	/*
	 * create the list even if there are 0 needed ids. The reason is
	 * we can't return insufficient IDs error from here so we should
	 * always return a list of some length
	 */
	(*ret_list) = lst_create();
	if (*ret_list == NULL) {
		Trace(TR_OPRMSG, "getting available striped group ids %s%s",
		    " failed: ", samerrmsg);
		goto err;
	}


	/*
	 * Not an error if family set does not exist- this may be true
	 * when creating the file system. It is an error if any
	 * other error is encountered.
	 */
	if (get_family_set_devs(mcf, fs_name, &fs_devs) != 0) {
		if (samerrno == SE_FAMILY_SET_DOES_NOT_EXIST) {
			for (i = 0; i < count; i++) {
				val = (devtype_t *)mallocer(sizeof (devtype_t));
				if (val == NULL) {
					goto err;
				}

				cpy_group_id(*val, dev_nmsg[i]);
				if (lst_append(*ret_list, val) != 0) {
					free(val);
					goto err;
				}
			}
			return (0);
		}
		goto err;
	}


	/*
	 * build a list of all ids found, there will be no striped groups
	 * if this is for create. But some may exist for a grow.
	 */
	for (n = fs_devs->head; n != NULL; n = n->next) {
		base_dev_t *dt = (base_dev_t *)n->data;
		num_type = nm_to_dtclass(dt->equ_type);


		if (is_stripe_group(num_type)) {
			int *nt = (int *)mallocer(sizeof (int));
			if (nt == NULL) {
				goto err;
			}

			*nt = num_type;
			if (lst_append(g_ids, nt) != 0) {
				free(nt);
				goto err;
			}
		}
	}

	/*
	 * start at 0 and work up through group ids.  If its a free id
	 * append it to ret_list
	 */
	for (i = 0; i <= dev_nmsg_size; i++) {
		for (n = g_ids->head; n != NULL; n = n->next) {
			num_type = nm_to_dtclass(dev_nmsg[i]);
			if (num_type == *(int *)n->data) {
				break;
			}
		}

		if (n == NULL) {
			/*
			 * If n == NULL it means the id was not found
			 * and so it is free. Append it to the results list.
			 */
			val = (devtype_t *)mallocer(sizeof (devtype_t));
			if (val == NULL) {
				goto err;
			}

			cpy_group_id(*val, dev_nmsg[i]);
			if (lst_append(*ret_list, val) != 0) {
				free(val);
				goto err;
			}
			if ((*ret_list)->length == count)
				break;
		}
	}

	lst_free_deep(g_ids);
	lst_free_deep(fs_devs);
	Trace(TR_OPRMSG, "got %d available striped group ids",
	    (*ret_list)->length);
	return (0);


err:
	lst_free(g_ids);
	lst_free_deep(*ret_list);
	*ret_list = NULL;
	Trace(TR_OPRMSG, "getting available striped group ids failed: %s",
	    samerrmsg);
	return (-1);
}


/*
 * strips unwanted spaces from gids.
 */
static int
cpy_group_id(
devtype_t gid,		/* output */
devtype_t input)	/* devtype to copy */
{

	if (input[2] == ' ') {
		strlcpy(gid, input, 3);
	} else if (input[3] == ' ') {
		strlcpy(gid, input, 4);
	} else {
		strlcpy(gid, input, 5);
	}

	return (0);
}


/*
 * Get a list of devices of the same type, e.g. g10 or mm
 * If family_set is non-null and does not exist -1 will be returned and
 *    samerrno will be set to SE_FAMILY_SET_DOES_NOT_EXIST.
 * If family_set is null all devices with the indicated type will be returned.
 * If an invalid eq_type is specified samerrno is set to SE_INVALID_EQ_TYPE.
 */
int
get_devices_by_type(
const mcf_cfg_t *cfg,	/* cfg to search */
const char *family_set,	/* family set name to select from */
const char *eq_type,	/* device type to include */
sqm_lst_t **dev_list)	/* malloced list of base_dev_t */
{

	node_t *node;
	node_t *tmp;
	base_dev_t *dev;
	devtype_t temp_type;

	Trace(TR_OPRMSG, "getting devices by type");

	if (ISNULL(cfg, eq_type)) {
		Trace(TR_OPRMSG, "getting devices by type failed: %s",
		    samerrmsg);
		return (-1);
	}

	strlcpy(temp_type, eq_type, sizeof (temp_type));
	if (nm_to_dtclass(temp_type) == 0) {
		samerrno = SE_INVALID_EQ_TYPE;

		/* Invalid device type %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_EQ_TYPE), eq_type);

		Trace(TR_OPRMSG, "getting devices by type failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (family_set != NULL) {
		if (get_family_set_devs(cfg, family_set, dev_list) != 0) {
			/* leave samerrno and samerrmsg as set */
			Trace(TR_OPRMSG, "getting devices by type failed: %s",
			    samerrmsg);
			return (-1);
		}
	} else {

		*dev_list = lst_create();
		if (*dev_list == NULL) {
			Trace(TR_OPRMSG, "getting devices by type failed: %s",
			    samerrmsg);

			return (-1);
		}

		/* put all devices in the list */
		for (node = cfg->mcf_devs->head;
		    node != NULL;
		    node = node->next) {
			base_dev_t *dest =
			    (base_dev_t *)mallocer(sizeof (base_dev_t));

			if (dest == NULL) {
				lst_free_deep(*dev_list);
				*dev_list = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting devices by type ",
				    samerrmsg);
				return (-1);
			}

			dev_cpy(dest, (base_dev_t *)node->data);
			if (lst_append(*dev_list, dest) != 0) {
				free(dest);
				lst_free_deep(*dev_list);
				*dev_list = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting devices by type ",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/* get rid of devices that don't match the type. */
	node = (*dev_list)->head;
	while (node != NULL) {
		dev = (base_dev_t *)node->data;
		if (strncmp(dev->equ_type, eq_type, strlen(eq_type)+1) != 0) {
			free(node->data);
			tmp = node;
			node = node->next;
			if (lst_remove(*dev_list, tmp) != 0) {
				lst_free_deep(*dev_list);
				*dev_list = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting devices by type ",
				    samerrmsg);
				return (-1);
			}
		} else {
			node = node->next;
		}
	}
	Trace(TR_OPRMSG, "got devices by type");
	return (0);
}


/*
 * A list of family set names.
 */
int
get_family_set_names(
const mcf_cfg_t *cfg,	/* cfg to search */
sqm_lst_t **family_sets)	/* malloced return list of char[] */
{

	node_t *node;
	int numeric_type;
	base_dev_t *dev;
	char *s;

	Trace(TR_OPRMSG, "getting family set names");
	if (ISNULL(cfg, family_sets)) {
		Trace(TR_OPRMSG, "getting family set names failed: %s",
		    samerrmsg);
		return (-1);
	}


	*family_sets = lst_create();
	if (*family_sets == NULL) {
		Trace(TR_OPRMSG, "getting family set names failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		dev = (base_dev_t *)node->data;
		numeric_type = nm_to_dtclass(dev->equ_type);
		if ((numeric_type & DT_FAMILY_SET) == DT_FAMILY_SET) {
			s = (char *)strdup(dev->set);
			if (s == NULL) {
				lst_free_deep(*family_sets);
				*family_sets = NULL;
				setsamerr(SE_NO_MEM);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting family set names ",
				    samerrmsg);

				return (-1);
			}
			if (lst_append(*family_sets, s) != 0) {
				lst_free_deep(*family_sets);
				*family_sets = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting family set names ",
				    samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_OPRMSG, "got family set names");
	return (0);
}


/*
 * get a list of the family set names of filesystems.
 */
int
get_fs_family_set_names(
const mcf_cfg_t *cfg,	/* cfg to search */
sqm_lst_t **fs)		/* malloced list of char[] */
{

	node_t *node;
	int numeric_type;
	base_dev_t *dev;
	char *s;

	Trace(TR_OPRMSG, "getting fs family set names");
	if (ISNULL(cfg, fs)) {
		Trace(TR_OPRMSG, "getting fs family set names failed: %s",
		    samerrmsg);
		return (-1);
	}

	*fs = lst_create();
	if (*fs == NULL) {
		Trace(TR_OPRMSG, "getting fs family set names failed: %s",
		    samerrmsg);

		return (-1);
	}

	for (node = cfg->mcf_devs->head; node != NULL; node = node->next) {
		dev = (base_dev_t *)node->data;
		numeric_type = nm_to_dtclass(dev->equ_type);
		if ((numeric_type == DT_DISK_SET) ||
		    (numeric_type == DT_META_SET) ||
		    (numeric_type == DT_META_OBJECT_SET) ||
		    (numeric_type == DT_META_OBJ_TGT_SET))	{

			s = (char *)strdup(dev->set);
			if (s == NULL) {
				lst_free_deep(*fs);
				*fs = NULL;
				setsamerr(SE_NO_MEM);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting fs family set names ",
				    samerrmsg);

				return (-1);
			}

			if (lst_append(*fs, s) != 0) {
				free(s);
				lst_free_deep(*fs);
				*fs = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting fs family set names ",
				    samerrmsg);

				return (-1);
			}
		}
	}
	Trace(TR_OPRMSG, "got fs family set names");
	return (0);

}


/*
 * If an error is encountered this function returns a -1 and sets
 * samerrno and samerrmsg. In addition it sets the boolean to false.
 */
int
check_fs_eq_types(
const mcf_cfg_t *cfg,	/* cfg to check */
const char *family_set,	/* family set to check */
boolean_t *eq_valid)	/* return value */
{

	sqm_lst_t *l;

	Trace(TR_DEBUG, "checking fs eq types for set %s", Str(family_set));

	if (ISNULL(family_set, eq_valid)) {
		Trace(TR_DEBUG, "checking fs eq types failed: %s",
		    samerrmsg);
		return (-1);
	}

	l = lst_create();
	if (l == NULL) {
		Trace(TR_DEBUG, "checking fs eq types failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (check_types(cfg, family_set, l) != 0) {
		*eq_valid = B_FALSE;
		lst_free(l);
		Trace(TR_DEBUG, "checking fs eq types failed: %s",
		    samerrmsg);
		return (-1);
	}
	*eq_valid = B_TRUE;

	/* free the list of types since no one wants it */
	lst_free_deep(l);
	Trace(TR_DEBUG, "checked fs eq types");
	return (0);
}


/*
 * if ret_list is null this function simply checks the device types for the
 * family_set. Otherwise it checks the current types for the family set
 * and returns a list of valid types that could be added.
 */
static int
check_types(
const mcf_cfg_t *cfg,	/* cfg to check in */
const char *family_set,	/* family set name to check devices for */
sqm_lst_t *ret_lst)	/* optional return of malloced valid device types */
{

	base_dev_t	*dev;
	base_dev_t	*fam_set_dev = NULL;
	node_t		*node;
	sqm_lst_t		*l = NULL;
	boolean_t	str_grp_fnd = B_FALSE;
	boolean_t	md_fnd = B_FALSE;
	boolean_t	mr_fnd = B_FALSE;
	boolean_t	mm_fnd = B_TRUE;
	int		fam_set_type = -1;
	int		dev_type = -1;

	Trace(TR_DEBUG, "checking types %s entry", Str(family_set));
	if (ISNULL(cfg, family_set)) {
		Trace(TR_DEBUG, "checking types failed: %s", samerrmsg);
		return (-1);
	}

	if (get_mcf_dev(cfg, family_set, &fam_set_dev) != 0) {
		/* leave samerrno as set by get_mcf_dev */
		Trace(TR_DEBUG, "checking types failed: %s", samerrmsg);
		return (-1);
	}

	/* determine the file system type. */
	fam_set_type = nm_to_dtclass(fam_set_dev->equ_type);
	if (fam_set_type != DT_DISK_SET &&
	    fam_set_type != DT_META_SET &&
	    fam_set_type != DT_META_OBJECT_SET &&
	    fam_set_type != DT_META_OBJ_TGT_SET) {

		samerrno = SE_INVALID_FS_TYPE;
		/* %s is not a valid file system type */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FS_TYPE),
		    fam_set_dev->equ_type);
		free(fam_set_dev);

		Trace(TR_DEBUG, "checking types failed: %s", samerrmsg);
		return (-1);
	}

	if (get_family_set_devs(cfg, family_set, &l) != 0) {
		free(fam_set_dev);

		/* leave samerrno set as get_family_set_devs set it. */
		Trace(TR_DEBUG, "checking types failed: %s", samerrmsg);
		return (-1);
	}

	for (node = l->head; node != NULL; node = node->next) {
		dev = (base_dev_t *)node->data;
		dev_type = nm_to_dtclass(dev->equ_type);

		if (strcmp(dev->name,
		    fam_set_dev->name) == 0) {
			/* already made sure it was a vaild fs type */
			continue;
		}

		/* sam can only have md */
		if ((fam_set_type == DT_DISK_SET) && (dev_type != DT_DATA)) {
			samerrno = SE_INVALID_DEVICE_IN_FS;

			/* Device %s(%s) is invalid in file system %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INVALID_DEVICE_IN_FS),
			    Str(dev->name), Str(dev->equ_type),
			    Str(family_set));

			goto err;
		}

		/* check valid sam qfs devices. */
		if ((fam_set_type == DT_META_SET) &&
		    (!is_stripe_group(dev_type)) &&
		    (dev_type != DT_META) &&
		    (dev_type != DT_DATA) &&
		    (dev_type != DT_RAID)) {

			samerrno = SE_INVALID_DEVICE_IN_FS;
			/* Device %s(%s) is invalid in file system %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INVALID_DEVICE_IN_FS),
			    dev->name, dev->equ_type, family_set);

			goto err;
		}


		/* check device mixing is appropriate */
		/* can't mix md with mr */
		if (dev_type == DT_DATA) {
			md_fnd = B_TRUE;
			if (str_grp_fnd | mr_fnd) {
				samerrno = SE_INVALID_DEVICE_MIX_IN_FS;
				/*
				 * File system %s contains %s
				 * devices and %s devices.
				 */
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_INVALID_DEVICE_MIX_IN_FS),
				    fam_set_dev->name, "md",
				    (str_grp_fnd) ? "gxx" : "mr");

				goto err;
			}
		}
		/* can't mix mr with md */
		if (dev_type == DT_RAID) {
			mr_fnd = B_TRUE;
			if (md_fnd) {
				samerrno = SE_INVALID_DEVICE_MIX_IN_FS;
				/*
				 * File system %s contains %s
				 * devices and %s devices.
				 */
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_INVALID_DEVICE_MIX_IN_FS),
				    fam_set_dev->name, "mr", "md");
				goto err;
			}
		}

		/* can't mix stripe with md */
		if (is_stripe_group(dev_type)) {
			str_grp_fnd = B_TRUE;
			if (md_fnd) {
				samerrno = SE_INVALID_DEVICE_MIX_IN_FS;
				/*
				 * File system %s contains %s
				 * devices and %s devices.
				 */
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_INVALID_DEVICE_MIX_IN_FS),
				    fam_set_dev->name, "gxx", "md");

				goto err;
			}
		}

		if (dev_type == DT_META) {
			mm_fnd = B_TRUE;
		}

	}

	/* Now build a list of valid device types given what we know. */
	if (ret_lst != NULL) {
		if (fam_set_type == DT_DISK_SET) {
			if (lst_append(ret_lst, (char *)strdup("md")) != 0) {
				goto err;
			}
		} else if (fam_set_type == DT_META_SET ||
		    fam_set_type == DT_META_OBJ_TGT_SET) {
			if (md_fnd) {
				/* if md only md and mm */
				if (lst_append(ret_lst,
				    (char *)strdup("md")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("mm")) != 0) {
					goto err;
				}
			} else if (str_grp_fnd || mr_fnd) {
				if (lst_append(ret_lst,
				    (char *)strdup("mm")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("gxx")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("mr")) != 0) {
					goto err;
				}
			} else if (mm_fnd) {
				if (lst_append(ret_lst,
				    (char *)strdup("mm")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("gxx")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("mr")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("md")) != 0) {
					goto err;
				}
			}
		} else if (fam_set_type == DT_META_OBJECT_SET) {
			if (mm_fnd) {
				if (lst_append(ret_lst,
				    (char *)strdup("mm")) != 0) {
					goto err;
				}
				if (lst_append(ret_lst,
				    (char *)strdup("oxx")) != 0) {
					goto err;
				}
			}
		}
	}
	lst_free_deep(l);
	free(fam_set_dev);
	Trace(TR_DEBUG, "checked types");
	return (0);

err:
	if (ret_lst != NULL) {
		/* depopluate the ret_lst  */
		for (node = ret_lst->head; node != NULL;
		    node = node->next) {

			free(node->data);
		}
	}
	lst_free_deep(l);
	free(fam_set_dev);
	Trace(TR_DEBUG, "checking types failed: %s",
	    samerrmsg);
	return (-1);
}


/*
 * If the family set indicated already includes invalid devices this
 * function will return the same errors that check_eq_types returns.
 */
int
get_valid_eq_types(
const mcf_cfg_t *cfg,		/* cfg as input */
const char *family_set,		/* family set to get types for */
sqm_lst_t **valid_eq_types)	/* malloced return list of valid dev types */
{

	sqm_lst_t *l;


	Trace(TR_DEBUG, "getting valid eq types");

	if (ISNULL(cfg, family_set, valid_eq_types)) {
		Trace(TR_DEBUG, "getting valid eq types failed: %s",
		    samerrmsg);
		return (-1);
	}
	*valid_eq_types = NULL;

	l = lst_create();
	if (l == NULL) {
		Trace(TR_DEBUG, "getting valid eq types failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (check_types(cfg, family_set, l) != 0) {
		/* leave samerrno as set */

		lst_free_deep(l);
		Trace(TR_DEBUG, "getting valid eq types failed: %s",
		    samerrmsg);

		return (-1);
	}
	*valid_eq_types = l;
	Trace(TR_DEBUG, "got valid eq types");
	return (0);
}


/*
 * relies on the fact that ordering of devices in structure is same as the
 * ordering in the file written out.
 */
int
verify_mcf_cfg(mcf_cfg_t *real_mcf)
{

	sqm_lst_t		*l;
	char		ver_path[MAXPATHLEN+1];
	int		err;


	Trace(TR_DEBUG, "verifying mcf cfg");
	if (ISNULL(real_mcf)) {
		Trace(TR_DEBUG, "verifying mcf cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (mk_wc_path(MCF_CFG, ver_path, sizeof (ver_path)) != 0) {
		return (-1);
	}

	if (write_mcf(ver_path, real_mcf) != 0) {
		unlink(ver_path);
		return (-1);
	}

	err = check_config_with_fsd(ver_path, NULL, NULL, NULL, &l);
	if (err == 0) {
		unlink(ver_path);
		Trace(TR_DEBUG, "verified mcf cfg");
		return (0);
	}

	if (-1 != err) {
		unlink(ver_path);
		samerrno = SE_VERIFY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno),
		    Str(l->head->data));
		lst_free_deep(l);
	}

	unlink(ver_path);
	Trace(TR_DEBUG, "verifying mcf cfg failed: %s", samerrmsg);
	return (-1);
}


/*
 * Read the mcf configuration from the default location and
 * build the mcf_cfg_t structure.
 */
int
read_mcf_cfg(
mcf_cfg_t **mcf_ret)	/* malloced return */
{

	Trace(TR_OPRMSG, "reading mcf cfg file %s", mcf_file);

	if (ISNULL(mcf_ret)) {
		Trace(TR_OPRMSG, "reading mcf cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (parse_mcf(mcf_file, mcf_ret) != 0) {
		*mcf_ret = NULL;
		Trace(TR_OPRMSG, "reading mcf cfg failed: %s", samerrmsg);
		return (-1);
	} else {
		Trace(TR_OPRMSG, "read mcf cfg");
		return (0);
	}
}


/*
 * write the cfg to an alternate location.
 */
int
dump_mcf(
const mcf_cfg_t *mcf,	/* cfg to write out */
const char *location)	/* location at which to write the mcf */
{

	Trace(TR_OPRMSG, "dumping mcf %s", Str(location));

	if (ISNULL(mcf, location)) {
		Trace(TR_OPRMSG, "dumping mcf failed: %s", samerrmsg);
		return (-1);
	}

	if (strcmp(location, MCF_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;
		/* cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_OPRMSG, "dumping mcf failed: %s", samerrmsg);

		return (-1);
	}
	if (write_mcf(location, mcf) != 0) {
		/* Leave the errors as set by write_mcf */

		Trace(TR_OPRMSG, "dumping mcf failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "dumped mcf");
	return (0);
}


/*
 * internal write funtion.
 */
static int
write_mcf(
const char *filename,	/* file to write to */
const mcf_cfg_t *mcf)	/* cfg to write */
{

	FILE *mcf_p = NULL;
	int fd;
	node_t *node;
	uname_t fset = { '\0'  };
	time_t the_time;
	char err_buf[80];

	Trace(TR_FILES, "writing mcf %s", filename);

	if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		mcf_p = fdopen(fd, "w");
	}
	if (mcf_p == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    "Open failed for %s, %s", filename, err_buf);

		Trace(TR_DEBUG, "writing mcf failed: %s", samerrmsg);
		return (-1);
	}

	fprintf(mcf_p, "# mcf\n");
	fprintf(mcf_p, "#\n");
	the_time = time(0);
	fprintf(mcf_p, "# Generated by config api %s#\n", ctime(&the_time));
	fprintf(mcf_p, "# Equipment\t\tEq\tEq\tFamily\tDevice\tAdditional\n");
	fprintf(mcf_p, "# Identifier\t\tOrd\tType\tSet\tState\tParameters\n");
	fprintf(mcf_p,
	    "#-----------\t\t---\t----\t------\t------\t----------\n");

	Trace(TR_DEBUG, "Wrote Header");
	for (node = mcf->mcf_devs->head; node != NULL; node = node->next) {
		base_dev_t *dev = (base_dev_t *)node->data;
		if (strcmp(fset, dev->set) != 0) {
			strlcpy(fset, dev->set, sizeof (fset));
			fprintf(mcf_p, "#\n#\n");
		}
		write_mcf_line(mcf_p, dev);
	}


	fclose(mcf_p);
	Trace(TR_FILES, "wrote mcf");
	return (0);
}


/*
 * private function to write a single line into cfg file.
 */
static int
write_mcf_line(
FILE *file,
base_dev_t *dev)	/* dev to be described by mcf line */
{

	fprintf(file, "%s\t%d\t%s\t%s\t", (*dev).name,
	    (*dev).eq, (*dev).equ_type, (*dev).set);

	if (dev->state < 0 || dev->state >= 6) {
		fprintf(file, "%d\t", dev->state);
	} else {
		fprintf(file, "%s\t", dev_state[(*dev).state]);
	}
	fprintf(file, "%s", (*dev).additional_params);
	fprintf(file, "\n");
	fflush(file);
	return (0);
}


/*
 * returns the number of bytes transfered to str.
 */
static int
sprint_mcf_line(
char *str,
base_dev_t *dev)	/* dev to be described by mcf line */
{
	char *c = str;

	str += sprintf(str, "%s\t%d\t%s\t%s\t", (*dev).name,
	    (*dev).eq, (*dev).equ_type, (*dev).set);

	if (dev->state < 0 || dev->state >= 6) {
		str += sprintf(str, "%d\t", dev->state);
	} else {
		str += sprintf(str, "%s\t", dev_state[(*dev).state]);
	}

	str += sprintf(str, "%s", (*dev).additional_params);
	str += sprintf(str, "\n");

	return (strlen(c));
}


/*
 * free the mcf_cfg_t
 */
void
free_mcf_cfg(mcf_cfg_t *mcf)
{

	Trace(TR_ALLOC, "freeing mcf cfg");

	if (mcf == NULL)
		return;

	lst_free_deep(mcf->mcf_devs);
	free(mcf);
}


/*
 * copy the non-NULL src to the non NULL dest
 */
int
dev_cpy(
base_dev_t *dst,	/* result of copy */
base_dev_t *src)	/* device to copy */
{

	memset(dst, 0, sizeof (base_dev_t));

	strlcpy((dst)->name, src->name,
	    sizeof ((dst)->name));

	(dst)->eq = src->eq;
	strlcpy((dst)->equ_type, src->equ_type, sizeof ((dst)->equ_type));
	strlcpy((dst)->set, src->set,
	    sizeof ((dst)->set));
	(dst)->state = src->state;
	strlcpy((dst)->additional_params, src->additional_params,
	    sizeof ((dst)->additional_params));

	return (0);
}



/*
 * This fuction should be called in succession on each host that will
 * be part of the shared file system. The in_use list from each call
 * should be being passed to the next call. When each host has been
 * called first_free will be set to the the starting point of a range
 * of free equipment ordinals that is at least eqs_needed ordinals
 * long. Because first free is malloced, it must be freed after each
 * call.
 *
 * If there are insufficent available equipment
 * ordinals an error will be returned.
 *
 * It is important to note that in_use is an input and output argument.
 * first_eq is an output argument.
 *
 * in_use when created by this function will be in increasing
 * order. If it is created in any other manner it must still be in
 * increading order.
 */
int
get_equipment_ordinals(
ctx_t *ctx,		/* ARGSUSED */
int eqs_needed,		/* number of ordinals that are needed */
sqm_lst_t **in_use,	/* ordinals that are in use */
int **first_free)	/* The first free value */
{

	sqm_lst_t *used;
	boolean_t done;
	node_t *in;
	node_t *out;
	mcf_cfg_t *mcf;
	int *res;
	int highest = 0;

	if (ISNULL(first_free)) {
		Trace(TR_ERR, "get equipment ordinals failed: %s", samerrmsg);
		return (-1);
	}


	if (*in_use != NULL) {
		used = *in_use;
	} else {
		*in_use = lst_create();
		used = *in_use;
	}

	if (read_mcf_cfg(&mcf) != 0) {
		/* Leave the samerrno that read set */
		Trace(TR_ERR, "get equipment ordinals failed: %s", samerrmsg);
		return (-1);
	}




	/*
	 * loop through the devices in the mcf inserting their ordinals
	 * into the in_use list in order.
	 */
	for (out = mcf->mcf_devs->head; out != NULL; out = out->next) {
		int mcf_ord = ((base_dev_t *)out->data)->eq;
		done = B_FALSE;

		for (in = used->head; in != NULL; in = in->next) {
			int in_use_ord =  *(int *)(in->data);
			if (in_use_ord == mcf_ord) {
				done = B_TRUE;
				break;
			}
			if (in_use_ord > mcf_ord) {
				int *ord = (int *)malloc(sizeof (int));
				*ord = mcf_ord;

				if (lst_ins_before(used, in, ord) != 0) {
					free_mcf_cfg(mcf);
					Trace(TR_ERR,
					    "get equip ordinals failed: %s",
					    samerrmsg);
					return (-1);
				}
				out->data = NULL;
				done = B_TRUE;
				break;
			}
		}
		if (!done) {
			int *ord = (int *)malloc(sizeof (int));
			*ord = mcf_ord;

			if (lst_append(used, ord) != 0) {
				free_mcf_cfg(mcf);
				Trace(TR_ERR, "get equip ordinals failed: %s",
				    samerrmsg);
				return (-1);
			}
			out->data = NULL;
		}
	}

	free_mcf_cfg(mcf);

	res = (int *)mallocer(sizeof (int));
	if (res == NULL) {
		Trace(TR_ERR, "get equip ordinals failed: %s",
		    samerrmsg);
		return (-1);
	}


	/*
	 * if used is emtpy it means the mcf was emtpy and no
	 * in_use ordinals were passed in. In this case EQ_INTERVAL
	 * should be returned as the first available equipment ordinal.
	 */
	if (used->length == 0) {
		*res = EQ_INTERVAL;
		*first_free = res;
		return (0);
	}


	/*
	 * We now have a sorted list of all of the equipment ordinals
	 * on each host that is going to participate in the shared
	 * file system. Pick the highest and add 100 unless the
	 * highest is very large. In which case check the gap sizes
	 * between ordinals and pick a large gap, add 100 to the low
	 * end and use that.
	 */
	highest = *(int *)used->tail->data;
	if (highest <= 10000) {

		/*
		 * pick a number that is higher than current highest
		 * by 100 and falls on a multiple of 10.
		 */
		*res = (((int)(highest / EQ_ROUND)) + 1) *
		    EQ_ROUND + EQ_INTERVAL;

		*first_free = res;
		return (0);

	}

	if ((*res = pick_eq_from_gap(used, eqs_needed,
	    EQ_ROUND, EQ_INTERVAL)) == 0) {

		if ((*res = pick_eq_from_gap(used,
		    eqs_needed, 1, 0)) == 0) {

			samerrno = SE_INSUFFICIENT_EQS;

			/* There are not %d available equipment ordinals */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INSUFFICIENT_EQS), eqs_needed);

			Trace(TR_OPRMSG, "getting eq ordinals failed: %s",
			    samerrmsg);

			free(res);
			return (-1);
		}
	}

	*first_free = res;
	return (0);
}


/*
 * If there is a large gap between equipment ordinals, we want to not
 * go higher than the highest. This could happen for instance if 45000
 * was used for the eq of the historian. This function looks for a gap
 * large enough to hold the family set. It will not return a number
 * from outside the range defined by the elements in the used list.
 * Setting round ensures that the number returned will be a multiple
 * of round.
 * Setting gap ensures a gap of at least that many ordinals
 * will be left below the new equipment ordinal.
 */
static int
pick_eq_from_gap(
sqm_lst_t *used,
int eqs_needed,
int round, /* set to 1 to not round */
int gap) /* set to 0 to have no mandatory gap */
{


	node_t *in;


	/* round can not be 0. */
	if (round == 0) {
		round = 1;
	}

	/*
	 * Look through used for the first place where
	 * there is a gap between two in use numbers that is large enough
	 * to accomodate the number of eqs_needed and the round and gap
	 * specified.
	 */
	for (in = used->head; in != NULL; in = in->next) {
		int low =  *(int *)in->data; /* low end of current gap */
		int high;	/* high end of current gap */
		int bottom;	/* beginning of the range to return */
		int top;	/* top of the range to return. */


		/*
		 * determine the bottom and top numbers in the range that
		 * would be returned if the current low and high are used
		 * to set the bounds of the gap.
		 */
		bottom = (((int)(low / round)) + 1) * round + gap;
		top = bottom + eqs_needed - 1;

		/* this handles the end of the inputs (end of the list) */
		if (in->next == NULL) {

			if (top < EQU_MAX) {
				return (bottom);
			} else {
				return (0);
			}
		}
		high = *(int *)in->next->data;



		if (top >= high) {
			continue;
		} else {
			/* the gap is big enough to support the needed eqs */
			return (bottom);
		}

	}

	/* no gap that was large enough has been found */
	return (0);
}



/*
 * This function can be called to check if a list of equipment ordinals
 * are available for use. If any of the ordinals are already in use an
 * exception will be returned.
 */
int
check_equipment_ordinals(
ctx_t *ctx	/* ARGSUSED */,
sqm_lst_t *eqs) {	/* list of equipment ordinals to check */


	mcf_cfg_t *mcf;
	node_t *n, *m;

	if (ISNULL(eqs)) {
		Trace(TR_ERR, "check equipment ordinals failed: %s",
			samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {
		/* Leave the samerrno that read set */
		Trace(TR_ERR, "check equipment ordinals failed: %s",
			samerrmsg);
		return (-1);
	}

	for (n = eqs->head; n != NULL; n = n->next) {
		int i = *(int *)n->data;

		for (m = mcf->mcf_devs->head; m != NULL; m = m->next) {
			base_dev_t *dev = m->data;
			if (dev->eq == i) {
				free_mcf_cfg(mcf);
				samerrno = SE_EQU_ORD_IN_USE;
				snprintf(samerrmsg, MAX_MSG_LEN,
					GetCustMsg(SE_EQU_ORD_IN_USE), i);

				Trace(TR_ERR,
				    "equipment ordinal %d is already in use",
				    dev->eq);
				return (-1);
			}
		}
	}

	Trace(TR_MISC, "specified equipment ordinals were all available");

	free_mcf_cfg(mcf);

	return (0);
}


/*
 * Remove the named family set from the mcf file.
 * When this function returns without error the following will be true:
 * 1. The devices will be gone from the mcf.
 * 2. Any fam-set-eq: comments will be gone.
 * 3. sam-fsd will have executed cleanly indicating no errors.
 * 4. The preexisting mcf will have been backed up.
 * 5. samd config will NOT have been called.
 */
int
remove_family_set(ctx_t *c, char *name) {
	FILE *f;
	sqm_lst_t *regions;
	upath_t err_buf;
	char *pre_set;
	char *set_comment;
	char *set;
	char *post_set;
	time_t the_time;

	Trace(TR_MISC, "removing family set %s", Str(name));
	if (ISNULL(name)) {
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);
		return (-1);
	}

	f = fopen(mcf_file, "r");
	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s  */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), mcf_file, err_buf);
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);
		return (-1);
	}

	regions = lst_create();
	if (regions == NULL) {
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);
		return (-1);
	}
	the_time = time(0);
	if (add_new_header(f, regions, &the_time) != 0) {
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);
		fclose(f);
		lst_free_deep(regions);
		return (-1);
	}

	if (find_dev_group_by_name(f, name, &pre_set, &set_comment,
	    &set, &post_set) != 0) {
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);

		fclose(f);
		lst_free_deep(regions);
		return (-1);
	}

	/* No longer needed so free them. */
	free(set);
	free(set_comment);

	if (pre_set) {
		size_t len;
		char *c;

		/*
		 * Make sure to not endlessly increase the number of
		 * #\n lines. When adding a family set two #\n lines are
		 * added that are not bounded by the family set comment. So
		 * if there are 3 or more #\n only lines in a row at the
		 * end of the pre-set region * remove two of them.
		 */
		len = strlen(pre_set);
		c = pre_set + len - strlen(TWO_COMMENT_LINES);
		if (strcmp(c, TWO_COMMENT_LINES) == 0) {
			*c = '\0';
		}


		if (lst_append(regions, pre_set) != 0) {
			Trace(TR_ERR, "removing family set %s failed %d %s",
			    name, samerrno, samerrmsg);
			fclose(f);
			free(pre_set);
			free(post_set);
			lst_free_deep(regions);
			return (-1);
		}
	}

	if (post_set) {
		if (lst_append(regions, post_set) != 0) {
			Trace(TR_ERR, "removing family set %s failed %d %s",
			    name, samerrno, samerrmsg);
			fclose(f);
			free(post_set);
			lst_free_deep(regions);
			return (-1);
		}
	}
	fclose(f);

	if (check_and_write_mcf(c, regions) != 0) {
		lst_free_deep(regions);
		Trace(TR_ERR, "removing family set %s failed %d %s", name,
			samerrno, samerrmsg);
		return (-1);
	}

	lst_free_deep(regions);
	Trace(TR_MISC, "removed family set %s", name);

	return (0);
}


/*
 * Finds the device group in the file. The name must either be the
 * family set name or the device name for groups that do not have
 * a family set name.
 *
 * On return a malloced region will have been created for any of the
 * 4 posible regions in the file: the pre-set region, the comment region
 * for the named set, the set itself and the post set region.
 * All together these four regions represent the entire remainder of the
 * file from the postion of f when the function is called.
 * Any of the regions can be null except the set region in which case
 * a -1 is returned and samerrno is set to SE_NOT_FOUND.
 */
int
find_dev_group_by_name(
FILE *f,
char *name,
char **pre_set_region,
char **comment_region,
char **set_region,
char **post_set_region) {

	int ret;
	char setcomment[34];
	base_dev_t *dev;
	char *cur_line;
	char *regions[4] = {NULL, NULL, NULL, NULL};
	int nmore[4] = {0, 0, 0, 0};
	/*
	 * switch index 0 = pre set, 1 = set comments,
	 * 2 = set devs, 3 = post set
	 */
	int cur_region = 0;


	snprintf(setcomment, sizeof (setcomment), "#%s:", name);

	*pre_set_region = NULL;
	*comment_region = NULL;
	*set_region = NULL;
	*post_set_region = NULL;

	/*
	 * we are assembling 4 regions
	 * 1. The region from current location to the set comments
	 * 2. The set comments these extend from #setname: to the first mcf
	 * entry entry for the set. If the first entry encountered is not
	 * for this set disregard this comment region and look for another.
	 * 3. The set itself
	 * 4. The post set region
	 *
	 * If the line is a comment and is not the '#set:'  line
	 * simply append it to the current region.
	 */

	/* get the current position incase it is needed. */
	while ((ret = read_mcf_line(f, &dev, &cur_line)) == 0) {

		/*
		 * dev == NULL means a comment was encounterd.
		 * If you got a comment, there are three cases:
		 * 1. It is the first line of the set comment
		 * 2. It is the first comment line following the set
		 * 3. It is just a comment to be added to the current
		 * region
		 */
		if (dev == NULL && cur_region == 0) {
			char *tmp = cur_line;
			while (isspace(*tmp)) {
				tmp++;
			}
			if (strstr(tmp, setcomment) == tmp) {
				/*
				 * this means we are beginning the region with
				 * comments about the set we are looking for.
				 */
				cur_region = 1;
			}

			if (append_to_region(&regions[cur_region],
				&nmore[cur_region], cur_line) != 0) {
				goto err;
			}

		} else if (dev == NULL && cur_region == 2) {
			/*
			 * this means the post set region may have begun. It
			 * is possible that this is an intraset comment but
			 * that will be handled below.
			 */
			cur_region = 3;
			if (append_to_region(&regions[cur_region],
			    &nmore[cur_region], cur_line) != 0) {
				goto err;
			}
		} else if (dev == NULL) {
			/* just a comment to be appended to current region */
			if (append_to_region(&regions[cur_region],
			    &(nmore[cur_region]), cur_line) != 0) {
				return (-1);
			}



		/*
		 * The device was non-null so the line was for a device.
		 * There are 4 cases:
		 * 1. It is a device prior to the set
		 * 2. It is a device for the set
		 * 3. It is the first device after the set
		 * 4. comments have been encountered after the set but
		 *    this new line is for a device that is part of the
		 *    set. In this case add the post_set_region to the
		 *    set and reset parsing for region 3. This handling is
		 *    required because comments that come after the family
		 *    set are not considered related to it.
		 */
		} else if (strcmp(dev->name, name) == 0 ||
			strcmp(dev->set, name) == 0) {

			/* A device we are looking for was found */
			switch (cur_region) {
			case 0:
			case 1:
				/* set current region to the set's region */
				cur_region = 2;
				break;
			case 2:
				/* a device for the set */
				break;

			case 3:
				/*
				 * This case handles when comments come between
				 * the devices of the set we are looking for.
				 * So everything that was included in the post
				 * set region needs to be appended to the
				 * set's region and the set becomes the active
				 * region again.
				 */
				cur_region = 2;
				if (append_to_region(&regions[2],
					&nmore[2], regions[3]) != 0) {

					goto err;
				}
				free(regions[3]);
				regions[3] = NULL;
				nmore[3] = 0;
				break;
			}


			/*
			 * For All of the above cases append the current line
			 * to the set's region
			 */
			if (append_to_region(&regions[cur_region],
				&nmore[cur_region], cur_line) != 0) {

				goto err;
			}

		} else {
			/*
			 * a device was found but not one from the set
			 * we are looking for
			 */
			switch (cur_region) {
			case 0:
				break;
			case 1:
				/*
				 * This means we thought we were in the set
				 * comment region but have returned to
				 * finding non-set devices. So reset to the
				 * pre set region and copy anything we have
				 * found in region 1 back to region 0.
				 */
				if (append_to_region(&regions[0],
					&nmore[0], regions[1]) != 0) {
					goto err;
				}
				cur_region = 0;

				/* free up and reset region 1 */
				free(regions[1]);
				regions[1] = NULL;
				nmore[1] = 0;
				break;
			case 2:
				/*
				 * we were finding set devices and we now have
				 * a non-set device so we are done with the
				 * set region.
				 */
				cur_region = 3;
				break;
			case 3:
				/*
				 * continue to append to the post set region
				 * until no more lines are read from the file
				 */
				break;
			}
			/* append the device to the region */
			if (append_to_region(&regions[cur_region],
			    &nmore[cur_region], cur_line) != 0) {
				goto err;
			}
		}
	} /* while (read_mcf_line() == 0) */

	if (ret == -1) {
		goto err;
	}
	if (regions[2] == NULL) {
		/* did not find the set */
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), name);

		goto err;
	}

	*pre_set_region = regions[0];
	*comment_region = regions[1];
	*set_region = regions[2];
	*post_set_region = regions[3];

	return (0);

err:

	/* free each region */
	free(regions[0]);
	free(regions[1]);
	free(regions[2]);
	free(regions[3]);

	*pre_set_region = NULL;
	*comment_region = NULL;
	*set_region = NULL;
	*post_set_region = NULL;

	Trace(TR_ERR, "finding_dev_group_by_name failed %d %s",
	    samerrno, samerrmsg);

	return (-1);
}


/*
 * appends src to dst, growing the dst array if needed. The size of
 * space available is updated to give the space remaining in the
 * possibly grown dst region. dst can be null, if it is it will
 * be created.
 */
static int
append_to_region(char **dst, int *space_avail, char *src) {
	size_t total_size;
	size_t srclen;
	int region_incr;

	if (ISNULL(src)) {
		Trace(TR_ERR, "append to region failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "appending to region space available = %d",
	    *space_avail);

	srclen = strlen(src);
	if (srclen >= *space_avail) {

		/*
		 * In general we are adding a line at a time but want
		 * to increase the memory at a higher rate to avoid
		 * continual reallocs. However, we must allow for
		 * the addition of larger srcs.
		 */
		if (srclen >= DEF_REGION_INCR) {
			region_incr = srclen + 1;
		} else {
			region_incr = DEF_REGION_INCR;
		}
		if (*dst == NULL) {
			*dst = realloc(*dst, region_incr);
			if (*dst == NULL) {
				setsamerr(SE_NO_MEM);
				/* Out of memory */
				Trace(TR_ERR, "append to region failed %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		} else {
			total_size = strlen(*dst) + *space_avail + region_incr;
			*dst = (char *)realloc(*dst, total_size);
			if (*dst == NULL) {
				setsamerr(SE_NO_MEM);
				/* Out of memory */
				Trace(TR_ERR, "append to region failed %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
		strcat(*dst, src);
		*space_avail += region_incr - srclen;
	} else {
		strcat(*dst, src);
		*space_avail -= srclen;
	}
	Trace(TR_OPRMSG, "appended to region space available = %d",
	    *space_avail);
	return (0);
}


/*
 * Read lines from the mcf file.
 * If the line is not for a comment this populates the first four
 * entries in the base_dev and returns a pointer to a statically allocated
 * string that represents the entire line.
 * Returns -1 on error, 1 if eof is encountered an zero if a line was
 * successfully read.
 */
static int
read_mcf_line(FILE *f, base_dev_t **d, char **contents) {
	static base_dev_t dev;
	static char buf_stor[MAX_MCF_LINE];
	char *buf;
	char *cont_char;
	size_t cnt;
	char entry[32];

	/* initialize locals and returns */
	*buf_stor = '\0';
	memset(&dev, 0, sizeof (base_dev_t));
	*d = NULL;
	*contents = NULL;

	buf = buf_stor;

	/*
	 * assebmle a full line - this includes grabbing up continuation
	 * lines (unless the line is for a comment- these do not continue).
	 */
	if (fgets(buf, MAX_MCF_LINE, f) != NULL) {
		/* if it is a comment return */
		while (isspace(*buf)) {
			buf++;
		}

		cnt = strlen(buf);
		if (cnt == 0 || *buf == '#') {
			/* line was comment or white space only */
			*contents = buf_stor;
			return (0);
		}

		/*
		 * Its not a comment so it must be an mcf entry. Since actual
		 * entries can have continuation characters check for them and
		 * reassemble an entire line.
		 * continuation lines must end with '\' followed by a newline
		 */

		cont_char = strrchr(buf, '\\');
		while (cont_char != NULL) {
			if (fgets(buf+cnt, MAX_MCF_LINE - cnt, f) == NULL) {
				/*
				 * We were expecting a continued line but
				 * hit EOF. This is legal according
				 * to the sam-fsd behavior so just break
				 * out and try to handle this.
				 */
				break;
			}
			cont_char = strrchr(buf+cnt, '\\');
			cnt += strlen(buf+cnt);
		}


		/*
		 * now get the first 4 tokens that make up each mcf line
		 * Note that the state and addn params fields are in
		 * many cases optional and to simplify this code these
		 * will not be populated in the resulting devs.
		 */
		if ((buf = get_next_entry(buf, dev.name)) == NULL) {
			samerrno = SE_MCF_LINE_ERROR;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), buf_stor);
			Trace(TR_ERR, "reading name from mcf line failed %s",
			    samerrmsg);
			return (-1);
		}

		if ((buf = get_next_entry(buf, entry)) == NULL) {
			samerrno = SE_MCF_LINE_ERROR;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), buf_stor);
			Trace(TR_ERR, "reading eq from mcf line failed %s",
			    samerrmsg);

			return (-1);
		}
		dev.eq = atoi(entry);

		if ((buf = get_next_entry(buf, dev.equ_type)) == NULL) {
			samerrno = SE_MCF_LINE_ERROR;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), buf_stor);
			Trace(TR_ERR, "reading eq type from mcf failed %s",
			    buf_stor);
			return (-1);
		}

		if ((buf = get_next_entry(buf, dev.set)) == NULL) {
			samerrno = SE_MCF_LINE_ERROR;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), buf_stor);
			Trace(TR_ERR, "reading set name from mcf failed %s",
			    samerrmsg);
			return (-1);
		}

		*d = &dev;
		*contents = buf_stor;
	}

	if (*d == NULL && *contents == NULL) {
		/* EOF was encountered */
		return (1);
	}
	Trace(TR_OPRMSG, "read mcf line %s", *contents);
	return (0);
}


/*
 * copies the first non-whitespace entry in buf into entry
 * and returns a pointer to the first character after entry
 * or to NULL if EOF or '\0' is reached"
 */
static char *
get_next_entry(char *buf, char *entry) {

	/* bypass leading white space */
	while (isspace(*buf) || iscontinue(*buf)) {
		buf++;
		if (*buf == '\0') {
			return (NULL);
		}
	}
	sscanf(buf, "%s", entry);
	buf += strlen(entry);
	return (buf);
}

/*
 * Append devices to the named family set.
 * Post conditions:
 * 1. Family set comment extended to include current date.
 * 2. New devs appended in order.
 * 3. mcf written and verified.
 * 4. Backup copy of pre-existing mcf has been made
 * 5. samd config has NOT been called.
 */
int
append_to_family_set(
ctx_t *c,
char *set_name,
sqm_lst_t *base_devs)
{
	FILE *f;
	time_t the_time;
	upath_t err_buf;
	char *pre_set;
	char *set_comment;
	char *set;
	char *post_set;
	char *add_comment;
	sqm_lst_t *regions;

	Trace(TR_MISC, "appending to family set %s", Str(set_name));
	if (ISNULL(set_name, base_devs)) {
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	f = fopen(mcf_file, "r");
	if (f == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), mcf_file, err_buf);

		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);

		return (-1);
	}

	regions = lst_create();
	if (regions == NULL) {
		fclose(f);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	the_time = time(0);
	if (add_new_header(f, regions, &the_time) != 0) {
		fclose(f);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * find the set to append to
	 */
	if (find_dev_group_by_name(f, set_name, &pre_set, &set_comment,
	    &set, &post_set) != 0) {
		fclose(f);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);

		return (-1);
	}
	fclose(f);

	/* append the regions they are not null */
	if (pre_set) {
		if (lst_append(regions, pre_set) != 0) {
			free(pre_set);
			free(set_comment);
			free(set);
			free(post_set);
			lst_free_deep(regions);
			Trace(TR_ERR, "append to family set %s failed %d %s",
			    set_name, samerrno, samerrmsg);
			return (-1);
		}
	}
	if (set_comment) {
		if (lst_append(regions, set_comment) != 0) {
			free(set_comment);
			free(set);
			free(post_set);
			lst_free_deep(regions);
			Trace(TR_ERR, "append to family set %s failed %d %s",
			    set_name, samerrno, samerrmsg);

			return (-1);
		}
	}

	/*
	 * create a memory region large enough for the comment including
	 * date and set_name
	 */
	add_comment = mallocer(strlen(add_comment_fmt_w_set) + 26 + 32);
	if (add_comment == NULL) {
		free(set);
		free(post_set);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	if (set_comment) {
		/* append a new line to the previously existing set comment */
		sprintf(add_comment, add_comment_fmt, ctime(&the_time));
	} else {
		/*
		 * There was no previous set comment. So create it now.
		 * Include a line indicating when it was appended to.
		 */
		sprintf(add_comment, add_comment_fmt_w_set,
		    set_name, ctime(&the_time));
	}
	if (lst_append(regions, add_comment) != 0) {
		free(add_comment);
		free(set);
		free(post_set);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * set is always non-null at this point or error would have
	 * resulted from find_dev_group_by_name
	 */
	if (lst_append(regions, set) != 0) {
		free(set);
		free(post_set);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	/* append the new devices */
	if (insert_region_for_devs(regions, base_devs) != 0) {
		free(post_set);
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}

	if (post_set) {
		if (lst_append(regions, post_set) != 0) {
			free(post_set);
			lst_free_deep(regions);
			Trace(TR_ERR, "append to family set %s failed %d %s",
			    set_name, samerrno, samerrmsg);
			return (-1);
		}
	}

	if (check_and_write_mcf(c, regions) != 0) {
		lst_free_deep(regions);
		Trace(TR_ERR, "appending to family set %s failed %d %s",
		    set_name, samerrno, samerrmsg);
		return (-1);
	}
	lst_free_deep(regions);
	Trace(TR_ERR, "appended to family set %s", set_name);

	return (0);
}


/*
 * Add a family set to the mcf file.
 * Post-conditions:
 * 1. family set has been added.
 * 2. set comment has been added.
 * 3. mcf has been writen and verified.
 * 4. A backup of the previously existing mcf has been made.
 * 5. samd config has not been called.
 */
int
add_family_set(ctx_t *c, sqm_lst_t *devs) {
	sqm_lst_t *regions;
	upath_t err_buf;
	char *set_comment;
	char *set_name;
	FILE *f;
	time_t the_time;
	char *name;

	Trace(TR_MISC, "adding family set");
	if (ISNULL(devs)) {
		Trace(TR_ERR, "adding family set failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}
	name = Str((char *)devs->head->data);

	regions = lst_create();
	if (regions == NULL) {
		Trace(TR_ERR, "adding family set %s failed %d %s", name,
			samerrno, samerrmsg);
		return (-1);
	}
	the_time = time(0);
	f = fopen(mcf_file, "r");
	if (f == NULL && errno != ENOENT) {

		lst_free(regions);

		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), mcf_file, err_buf);

		Trace(TR_ERR, "adding family set %s failed %d %s", name,
			samerrno, samerrmsg);

		return (-1);

	} else if (f == NULL && errno == ENOENT) {

		/*
		 * ENOENT means no mcf exists. Note that the absence of a
		 * mcf is not an error at all, just assumed to be
		 * a fresh system.
		 */
		if (add_new_header(f, regions, &the_time) != 0) {
			lst_free_deep(regions);
			Trace(TR_ERR, "adding family set %s failed %d %s",
			    name, samerrno, samerrmsg);

			return (-1);
		}
	} else {
		/*
		 * !ENOENT means an mcf exists. Note that the absence of a
		 * mcf is not an error at all, just assumed to be
		 * a fresh system.
		 */

		if (add_new_header(f, regions, &the_time) != 0) {
			lst_free_deep(regions);
			fclose(f);
			Trace(TR_ERR, "adding family set %s failed %d %s",
			    name, samerrno, samerrmsg);

			return (-1);
		}
		if (get_remainder_of_file(f, regions) != 0) {
			lst_free_deep(regions);
			fclose(f);
			Trace(TR_ERR, "adding family set %s failed %d %s",
			    name, samerrno, samerrmsg);
			return (-1);
		}
		fclose(f);
	}

	/*
	 * create a memory region large enough for the comment including
	 * date and set_name
	 */
	set_name = ((base_dev_t *)(devs->head->data))->set;
	if (strcmp(set_name, "-") == 0) {
		set_name = ((base_dev_t *)(devs->head->data))->name;
	}
	set_comment = mallocer(strlen(FAM_SET_COMMENT_FMT) +
		26 + strlen(set_name) + 3);
	if (set_comment == NULL) {
		lst_free_deep(regions);
		Trace(TR_ERR, "adding family set %s failed %d %s",
		    name, samerrno, samerrmsg);
		return (-1);
	}
	sprintf(set_comment, FAM_SET_COMMENT_FMT, set_name, ctime(&the_time));

	if (lst_append(regions, set_comment) != 0) {
		lst_free_deep(regions);
		Trace(TR_ERR, "adding family set %s failed %d %s",
		    name, samerrno, samerrmsg);
		return (-1);
	}

	if (insert_region_for_devs(regions, devs) != 0) {
		lst_free_deep(regions);
		Trace(TR_ERR, "adding family set %s failed %d %s",
		    name, samerrno, samerrmsg);
		return (-1);
	}

	if (check_and_write_mcf(c, regions) != 0) {
		lst_free_deep(regions);
		Trace(TR_ERR, "adding family set %s failed %d %s",
		    name, samerrno, samerrmsg);
		return (-1);
	}

	lst_free_deep(regions);
	Trace(TR_MISC, "added family set %s", name);
	return (0);
}


static int
insert_region_for_devs(sqm_lst_t *regions, sqm_lst_t *devs) {
	char *set_region;
	int chars_added = 0;
	char *print_at;
	node_t *n;

	/*
	 * create a region large enough to hold as many
	 * devices as are in the input list- as if each
	 * required a full length entry.
	 *
	 * each line can be
	 *   device path (<= 127)
	 * + 31 for fam_set
	 * + 5 for eq ordinal (must be < 65534)
	 * + 2 for eq type
	 * + upto 7 for on, off, down, or unavail
	 * + 127 for addn parameters.
	 * + 7 for white space, newline and terminator
	 */
	set_region = (char *)mallocer((devs->length * 306));
	if (set_region == NULL) {
		Trace(TR_ERR, "inserting region failed failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	print_at = set_region;
	for (n = devs->head; n != NULL; n = n->next) {

		chars_added = sprint_mcf_line(print_at,
			(base_dev_t *)n->data);
		print_at += chars_added;
	}
	if (lst_append(regions, set_region) != 0) {
		free(set_region);
		Trace(TR_ERR, "inserting region failed failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	return (0);
}


static int
check_and_write_mcf(ctx_t *ctx, sqm_lst_t *regions) {

	if (verify_mcf_from_regions(regions) != 0) {
		return (-1);
	}

	/*
	 * if there is a ctx and its dump path is not empty, dump the mcf
	 * and return without writing it to the actual location.
	 */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		char *dmp_loc;
		dmp_loc = assemble_full_path(ctx->dump_path, MCF_DUMP_FILE,
			B_FALSE, NULL);
		if (write_mcf_from_regions(dmp_loc, regions) != 0) {
			Trace(TR_ERR, "writing mcf cfg failed: %s",
				samerrmsg);
			return (-1);
		}
		strlcpy(ctx->read_location, ctx->dump_path, sizeof (upath_t));

		Trace(TR_FILES, "wrote mcf cfg to %s", dmp_loc);
		return (0);
	}

	/* possibly backup the mcf (see backup_cfg for details) */
	if (backup_cfg(mcf_file) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "backing up the mcf failed: %s", samerrmsg);
	}

	if (write_mcf_from_regions(mcf_file, regions) != 0) {
		Trace(TR_ERR, "writing mcf cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* always backup new mcf file */
	backup_cfg(mcf_file);

	Trace(TR_OPRMSG, "wrote mcf cfg");
	return (0);
}

static int
write_mcf_from_regions(char *filename, sqm_lst_t *regions) {
	FILE *mcf_p = NULL;
	int fd;
	node_t *n;
	char err_buf[80];

	if ((fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		mcf_p = fdopen(fd, "w");
	}
	if (mcf_p == NULL) {
		samerrno = SE_CFG_OPEN_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			"Open failed for %s, %s", filename, err_buf);

		Trace(TR_ERR, "writing mcf failed: %s", samerrmsg);
		return (-1);
	}

	for (n = regions->head; n != NULL; n = n->next) {
		if (n->data != NULL) {
			fprintf(mcf_p, (char *)n->data);
		}
	}

	fclose(mcf_p);
	Trace(TR_FILES, "wrote mcf");
	return (0);
}


/*
 * When this function returns the following will be true:
 * 1. The old header if present will have been removed
 * 2. Any preheader comments that were in the file will
 *    be in a region.
 * 3. The new header will be a region
 * 4. The position in f will be set to the end of the old header
 *    thus it is setup to read the remainder of the file.
 */
#define	OH_GEN_LINE "# Generated by config api "
#define	NH_GEN_LINE "# Generated by fsmgmtd "
#define	GEN_LINE_FMT "# Generated by fsmgmtd %s"
#define	BACK_LINE_FMT "#\n\
# A backup copy of the previously existing file can be\n\
# found in the %s directory\n"

#define	HEADER_FMT_4_5 "# mcf\n#\n# Generated by fsmgmtd %s\n\
#\n\
# A backup copy of the previously existing file can be\n\
# found in the %s directory\n#\n\
# Equipment         Eq     Eq      Family    Device    Additional\n\
# Identifier        Ord    Type    Set       State     Parameters\n\
# -----------       ---    ----    ------    ------    ----------\n"

static int
add_new_header(FILE *f, sqm_lst_t *regions, time_t *the_time) {

	char buf[1024];
	sqm_lst_t *tmp_list = NULL;
	boolean_t found_4_3_header = B_FALSE;
	boolean_t found_4_4_header = B_FALSE;

	if (f != NULL) {

		tmp_list = lst_create();
		/*
		 * look for either the string "Generated by config API" or
		 * the string "Generated by fsmgmtd"
		 * If the former is encountered:
		 * replace it with the new line and append the
		 * lines telling the user where the backups can be found.
		 * If the latter is encountered:
		 * replace it with a line that has the new time
		 * If neither is encountered:
		 * add the whole new header with the time and the
		 * directory in which the backups can be found. This
		 * could require reading the whole file and then all
		 * of the strings that were found.
		 */
		while (fgets(buf, MAX_MCF_LINE, f) != NULL) {
			if (strstr(buf, OH_GEN_LINE) != NULL) {
				found_4_3_header = B_TRUE;
				snprintf(buf, sizeof (buf), GEN_LINE_FMT,
				    ctime(the_time));

				if (lst_append(tmp_list, strdup(buf)) != 0) {
					lst_free_deep(tmp_list);
					return (-1);
				}
				snprintf(buf, sizeof (buf), BACK_LINE_FMT,
					CFG_BACKUP_DIR);
				if (lst_append(tmp_list, strdup(buf)) != 0) {
					lst_free_deep(tmp_list);
					return (-1);
				}
				break;
			} else if (strstr(buf, NH_GEN_LINE) != NULL) {
				found_4_4_header = B_TRUE;
				snprintf(buf, sizeof (buf), GEN_LINE_FMT,
				    ctime(the_time));

				if (lst_append(tmp_list, strdup(buf)) != 0) {
					lst_free_deep(tmp_list);
					return (-1);
				}
				break;
			} else {
				if (lst_append(tmp_list, strdup(buf)) != 0) {
					lst_free_deep(tmp_list);
				}
			}
		}
	}
	if (f == NULL || (!found_4_3_header && !found_4_4_header)) {

		/*
		 * no header found so add one, free the tmp_list and
		 * rewind the file
		 */
		lst_free_deep(tmp_list);
		snprintf(buf, sizeof (buf), HEADER_FMT_4_5, ctime(the_time),
		    CFG_BACKUP_DIR);
		if (lst_append(regions, strdup(buf)) != 0) {
			return (-1);
		}
		if (f != NULL) {
			rewind(f);
		}
	} else {
		/* move the header to the regions list */
		if (lst_concat(regions, tmp_list) != 0) {
			lst_free_deep(tmp_list);
			return (-1);
		}
	}

	return (0);
}


static int
get_remainder_of_file(FILE *f, sqm_lst_t *regions) {
	char *file_end;
	int index = 0;
	int remaining;
	char *str;
	int cur_size;

	file_end = (char *)realloc(NULL, DEF_REGION_INCR);
	if (file_end == NULL) {
		setsamerr(SE_NO_MEM);
		return (-1);
	}

	cur_size = remaining = DEF_REGION_INCR;

	/* start with a null terminator */
	*file_end = '\0';

	str = file_end;
	while (fgets(str, remaining, f) != NULL) {
		int chars_added = strlen(str);
		remaining = remaining - chars_added;
		index += chars_added;
		if (remaining == 1) {
			file_end = realloc(file_end,
			    (cur_size + DEF_REGION_INCR));

			if (file_end == NULL) {
				setsamerr(SE_NO_MEM);
				return (-1);
			}
			remaining += DEF_REGION_INCR;
			cur_size += DEF_REGION_INCR;
		}
		str = file_end + index;
	}

	if (lst_append(regions, file_end) != 0) {
		free(file_end);
		return (-1);
	}

	return (0);
}

static int
verify_mcf_from_regions(sqm_lst_t *regions)
{

	sqm_lst_t		*l;
	char		ver_path[MAXPATHLEN+1];
	int		err;


	Trace(TR_DEBUG, "verifying mcf cfg");
	if (ISNULL(regions)) {
		Trace(TR_DEBUG, "verifying mcf cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (mk_wc_path(MCF_CFG, ver_path, sizeof (ver_path)) != 0) {
		return (-1);
	}

	if (write_mcf_from_regions(ver_path, regions) != 0) {
		unlink(ver_path);
		return (-1);
	}

	err = check_config_with_fsd(ver_path, NULL, NULL, NULL, &l);
	if (err == 0) {
		unlink(ver_path);
		Trace(TR_DEBUG, "verified mcf cfg");
		return (0);
	}

	if (-1 != err) {
		samerrno = SE_VERIFY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno),
		    Str(l->head->data));
		lst_free_deep(l);
	}

	unlink(ver_path);
	Trace(TR_DEBUG, "verifying mcf cfg failed: %s", samerrmsg);
	return (-1);
}
