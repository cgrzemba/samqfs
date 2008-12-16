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
#pragma ident   "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* POSIX headers. */
#include <unistd.h>


/* System Headers */
#include <sys/types.h>
#include <time.h>


/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "sam/param.h"
#include "sam/devnm.h"
#include "sam/readcfg.h"
#include "sam/lib.h"	/* for StrFromErrno */


/* API headers */
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/config/master_config.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"
#include "parser_utils.h"


/* private function prototypes */
static void handle_mcf_line(void);
static void handle_cfg_msg(char *msg, int lineno, char *line);
static int check_fs_devices(mcf_cfg_t *mcf_chk, base_dev_t *fam_set_dev);
static int dev_error(char *err_msg);
static int check_family_sets(mcf_cfg_t *mcf_chk);
static int check_all_fs_devices(mcf_cfg_t *mcf_chk);
static int init_static_variables();
static void handle_mcf_line(void);


/* Command table */
static DirProc_t dir_proc_table[] = {
	{ NULL, handle_mcf_line, DP_other }
};


/* global to this file */
static char		dir_name[TOKEN_SIZE];
static char		token[TOKEN_SIZE];
static int		max_dev_eq = 0;
static mcf_cfg_t	*mcf;
static char		mcf_name[MAXPATHLEN+1];
static boolean_t	hist_found; /* only allow 1 historian */
static boolean_t	no_cmd_file = B_FALSE;
static char		open_error[80];
static int		mem_error = 0;
static sqm_lst_t		*error_list = NULL;

#define	MAX_PATH	128
#define	MAX_UNAME	32

/*
 * parse the mcf file.
 */
int
parse_mcf(
char *mcf_file_name,	/* file to parse */
mcf_cfg_t **ret_val)	/* malloced return value */
{

	int errors = 0;

	Trace(TR_OPRMSG, "parsing mcf %s", mcf_file_name);

	if (init_static_variables() != 0) {
		return (-1);
	}

	*ret_val = NULL;

	strlcpy(mcf_name, mcf_file_name, sizeof (mcf_name));

	mcf = (mcf_cfg_t *)mallocer(sizeof (mcf_cfg_t));
	if (mcf == NULL) {
		Trace(TR_OPRMSG, "parsing mcf failed: %s", samerrmsg);
		return (-1);
	}

	mcf->mcf_devs = lst_create();
	if (mcf->mcf_devs == NULL) {
		Trace(TR_OPRMSG, "parsing mcf failed: %s", samerrmsg);
		return (-1);
	}

	/* put the current time in the mcf_cfg_t */
	mcf->read_time = time(0);
	errors = ReadCfg(mcf_name, dir_proc_table, dir_name,
	    token, handle_cfg_msg);

	if (errors != 0) {

		if (mem_error != 0) {
			setsamerr(mem_error);
		} else if (errors == -1) {
			if (no_cmd_file) {
				/*
				 * The absence of a cmd file is not an error
				 * return 0 and the cfg that was initialized
				 * above
				 */
				Trace(TR_OPRMSG, "no mcf to parse");
				*ret_val = mcf; /* don't return NULL */
				return (0);
			}

			/* report other access errors */
			samerrno = SE_CFG_OPEN_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), mcf_name,
			    open_error);

		} else if (errors == 1) {
			samerrno = SE_CONFIG_CONTAINED_ERROR;
			/* 1 error in configuration file %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_CONTAINED_ERROR),
			    mcf_file_name);

		} else if (errors > 0) {
			samerrno = SE_CONFIG_CONTAINED_ERRORS;
			/* %d errors in configuration file %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS),
			    mcf_file_name, errors);
		}
		free_mcf_cfg(mcf);
		Trace(TR_OPRMSG, "parsing mcf failed: %s", samerrmsg);
		return (-1);
	}

	errors = 0;
	/* Perform additional checks */
	errors += check_family_sets(mcf);
	errors += check_all_fs_devices(mcf);
	if (errors != 0) {
		if (errors != 1) {
			samerrno = SE_CONFIG_CONTAINED_ERRORS;
			/* %d errors in configuration file %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS),
			    mcf_file_name, errors);

			free_mcf_cfg(mcf);
			Trace(TR_OPRMSG, "parsing mcf failed: %s", samerrmsg);
			return (-1);
		} else {
			samerrno = SE_CONFIG_CONTAINED_ERROR;
			/* 1 error in configuration file %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CONFIG_CONTAINED_ERROR),
			    mcf_file_name);
			free_mcf_cfg(mcf);

			Trace(TR_OPRMSG, "parsing mcf failed: %s", samerrmsg);
			return (-1);
		}
	}

	Trace(TR_OPRMSG, "parsed mcf");
	*ret_val = mcf;

	return (0);

}


/*
 * get the parsing errors from the most recent parsing.
 */
int
get_master_config_parsing_errors(
sqm_lst_t **l)	/* malloced list of parsing_error_t */
{

	return (dup_parsing_error_list(error_list, l));
}


/*
 * setup initial values of all statics.
 */
static int
init_static_variables(void) {

	Trace(TR_DEBUG, "initializing static variables");

	max_dev_eq = 0;
	hist_found = B_FALSE;
	*open_error = '\0';
	mem_error = 0;
	no_cmd_file = B_FALSE;

	lst_free_deep(error_list);
	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
			samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "initialized static variables");
	return (0);
}


/*
 * verify that the device types are appropriate.
 */
static int
check_all_fs_devices(
mcf_cfg_t *mcf_chk)	/* cfg to check */
{

	node_t *node;
	int numeric_type;
	base_dev_t *dev;
	int errors = 0;

	Trace(TR_DEBUG, "checking all fs devices");

	for (node = mcf_chk->mcf_devs->head; node != NULL;
	    node = node->next) {

		dev = (base_dev_t *)node->data;
		numeric_type = nm_to_dtclass(dev->equ_type);
		if ((numeric_type & DT_CLASS_MASK) != DT_FAMILY_SET) {
			continue;
		}

		errors += check_fs_devices(mcf_chk, dev);
	}

	Trace(TR_OPRMSG, "checked all fs devices found %d error(s)", errors);
	return (errors);
}


/*
 * checks the devices of the family set pointed to by fam_set_ent.
 *
 * This function does the checking that is done by the gatherFsDevices
 * funtion of fsconfig.c.
 */
static int
check_fs_devices(
mcf_cfg_t *mcf_chk,		/* cfg to check in */
base_dev_t *fam_set_dev)	/* family set for which to check devices */
{

	node_t		*node;
	base_dev_t	*dev;
	int		fam_set_cnt = 0;
	int		fam_set_type = -1;
	int		meta_cnt = 0;
	int		dev_cnt = 0;
	int		dev_type = 0;
	char		err_msg[MAX_LINE];
	int		errors = 0;

	/* First stab at tracking appropriate devices. */
	boolean_t str_grp_fnd = B_FALSE;
	boolean_t md_fnd = B_FALSE;
	boolean_t mr_fnd = B_FALSE;

	Trace(TR_OPRMSG, "checking fs devices for %s", fam_set_dev->name);
	fam_set_type = nm_to_dtclass(fam_set_dev->equ_type);

	/*
	 * This loop does 3 things.
	 * 1. It counts the occurance of family sets with the same name as
	 *    the one passed in.
	 * 2. It counts metadata and data devices.
	 * 3. It verifies that the device type is appropriate to the file
	 *    system and the other devices in it.
	 *	if (fs type == ma)
	 *		then
	 *	if (fs type == ms)
	 *		then ... are valid device types.
	 *
	 * 1 and 2 are only checked on loop exit.
	 */
	for (node = mcf_chk->mcf_devs->head; node != NULL;
	    node = node->next) {

		dev = (base_dev_t *)node->data;
		dev_type = nm_to_dtclass(dev->equ_type);

		/*
		 * count the occurences of devices with the same name
		 * as the family set passed in.
		 */
		if (strncmp(fam_set_dev->name, dev->name,
		    sizeof (fam_set_dev->name)) == 0 &&
		    ((dev_type & DT_CLASS_MASK) == DT_FAMILY_SET)) {
			fam_set_cnt++;
			continue;
		}

		/* continue if dev is not from this family set */
		if (strncmp(fam_set_dev->set, dev->set,
		    sizeof (fam_set_dev->set)) != 0 ||
		    (dev_type & DT_CLASS_MASK) == DT_FAMILY_SET) {
			continue;
		}

		/*
		 * count the number of meta and data devices in the family set
		 */
		dev_cnt++;
		if (dev_type == DT_META) {
			meta_cnt++;
		}


		/*
		 * How about adding some device checking here too
		 * for instance given fs type check appropriate devices
		 * given other devices found make sure no conflicts.
		 */
		/* if ms file system, only md is allowed. */
		if ((fam_set_type == DT_DISK_SET) && (dev_type != DT_DATA)) {
			/* File system %s has invalid devices. */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17215),
			    fam_set_dev->name);
			dev_error(err_msg);
			errors++;
			continue;
		}

		/* check valid ma or mat file system devices. */
		if ((fam_set_type == DT_META_SET ||
		    fam_set_type == DT_META_OBJ_TGT_SET) &&
		    (!is_stripe_group(dev_type)) &&
		    (dev_type != DT_META) &&
		    (dev_type != DT_DATA) &&
		    (dev_type != DT_RAID)) {

			/* File system %s has invalid devices. */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17215),
			    fam_set_dev->name);
			dev_error(err_msg);
			errors++;
			continue;
		}

		/* check device mixing is appropriate */
		/* can't mix md with mr */
		if (dev_type == DT_DATA) {
			md_fnd = B_TRUE;
			if (str_grp_fnd | mr_fnd) {
				/* File system %s has invalid devices. */
				snprintf(err_msg, sizeof (err_msg),
				    GetCustMsg(17215),
				    fam_set_dev->name);
				dev_error(err_msg);
				errors++;
				continue;
			}
		}
		/* can't mix mr with md */
		if (dev_type == DT_RAID) {
			mr_fnd = B_TRUE;
			if (md_fnd) {
				/* File system %s has invalid devices. */
				snprintf(err_msg, sizeof (err_msg),
				    GetCustMsg(17215),
				    fam_set_dev->name);
				dev_error(err_msg);
				errors++;
				continue;
			}
		}

		/* can't mix stripe with md */
		if (is_stripe_group(dev_type)) {
			str_grp_fnd = B_TRUE;
			if (md_fnd) {
				/* File system %s has invalid devices. */
				snprintf(err_msg, sizeof (err_msg),
				    GetCustMsg(17215),
				    fam_set_dev->name);
				dev_error(err_msg);
				errors++;
				continue;
			}
		}

		/* if mb file system, only mm and oXXX is allowed. */
		if ((fam_set_type == DT_META_OBJECT_SET) &&
		    (dev_type != DT_META) && !is_osd_group(dev_type)) {
			/* File system %s has invalid devices. */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17215),
			    fam_set_dev->name);
			dev_error(err_msg);
			errors++;
			continue;
		}
	}

	if (fam_set_cnt != 1) {

		if (fam_set_cnt > 1) {
			/* Family set %s is duplicated in mcf %s */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17211),
			    fam_set_dev->name, mcf_name);
			dev_error(err_msg);
		} else {
			/* Family set %s is missing from mcf %s */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17212),
			    fam_set_dev->name, mcf_name);
			dev_error(err_msg);
		}
		return (++errors);
	}

	if (dev_cnt == 0) {
		/* File system %s has no devices. */
		snprintf(err_msg, sizeof (err_msg), GetCustMsg(17213),
		    fam_set_dev->name);
		dev_error(err_msg);
		return (++errors);
	}

	if (dev_cnt > L_FSET) {
		/* File system %s has too many (%d) devices (limit = %d). */
		snprintf(err_msg, sizeof (err_msg), GetCustMsg(17239),
		    fam_set_dev->name, dev_cnt, L_FSET);
		dev_error(err_msg);
		return (++errors);
	}

	if (dev_cnt == meta_cnt) {
		/* File system %s has no data devices. */
		snprintf(err_msg, sizeof (err_msg), GetCustMsg(17237),
		    fam_set_dev->name);
		dev_error(err_msg);
		return (++errors);
	}

	/*
	 * If this is a qfs fs and there are no mm partitions then this
	 * must be a shared fs or something is wrong.  It is possible that
	 * even if this is a shared fs something is wrong.
	 */
	if (meta_cnt == 0 && (fam_set_type == DT_META_SET) &&
	    strncmp(fam_set_dev->additional_params, "shared",
	    sizeof ("shared")) != 0) {
		/* File system %s has no metadata devices. */
		snprintf(err_msg, sizeof (err_msg),
		    "File system %s has no metadata devices.",
		    fam_set_dev->name);
		dev_error(err_msg);
		return (++errors);

	}
	Trace(TR_OPRMSG, "checked fs devices found %d error(s)", errors);
	return (errors);
}


/*
 * This takes an argument to facilitate reuse.  So doesn't simply reuse
 * the globals.
 */
static int
check_family_sets(
mcf_cfg_t *mcf_chk)	/* cfg to check */
{

	int numeric_type = 0;
	boolean_t  famset_found = FALSE;
	node_t	   *node;
	node_t	   *inner_node;
	base_dev_t  *dev;
	int errors = 0;
	char err_msg[MAX_LINE] = {'\0'};

	Trace(TR_OPRMSG, "checking family sets");

	/*
	 * This code checks that valid family sets are specified for
	 * each device.  Valid here means simply that the family set exists.
	 */
	for (node = mcf_chk->mcf_devs->head; node != NULL;
	    node = node->next) {

		/* if device has no set skip it-  */
		dev = (base_dev_t *)node->data;
		if (*dev->set == '-' || *dev->set == '\0') {
			continue;
		}

		/*
		 * if the device is a family set or a remote sam server
		 * continue because it is not necessary to find the family
		 * set defined elsewhere.
		 */
		numeric_type = nm_to_dtclass(dev->equ_type);
		if ((numeric_type & DT_FAMILY_SET) == DT_FAMILY_SET ||
		    numeric_type == DT_PSEUDO_SS) {
			dev->fseq = dev->eq;
			continue;
		}

		/*
		 * search through the existing devices locating family sets.
		 * For each fam set check if this device belongs to it.
		 */
		famset_found = FALSE;
		for (inner_node = mcf_chk->mcf_devs->head;
		    inner_node != NULL;
		    inner_node = inner_node->next) {

			base_dev_t *dp = (base_dev_t *)inner_node->data;

			numeric_type = nm_to_dtclass(dp->equ_type);
			if ((numeric_type & DT_FAMILY_SET) == DT_FAMILY_SET &&
			    strcmp(dp->set, dev->set) == 0) {
				dev->fseq = dp->eq;
				famset_found = TRUE;
				break;
			}
		}

		if (!famset_found) {
			/* No family set device for eq %d, set '%s' */
			snprintf(err_msg, sizeof (err_msg), GetCustMsg(17240),
			    dev->eq, dev->set);
			Trace(TR_OPRMSG, "error checking family sets %s",
			    err_msg);
			dev_error(err_msg);
			errors++;
		}

	}
	Trace(TR_OPRMSG, "checked family sets found %d error(s)", errors);
	return (errors);
}


/* create a parsing_error_t struct and put it in list */
static int
dev_error(char *err_msg)
{

	parsing_error_t *err;


	err = (parsing_error_t *)mallocer(sizeof (parsing_error_t));
	if (err == NULL) {
		return (-1);
	}

	*err->input = char_array_reset;
	err->error_type = ERROR;
	err->severity = 1;
	err->line_num = -1;
	strlcpy(err->msg, err_msg, sizeof (err->msg));
	if (lst_append(error_list, err) != 0) {
		free(err);
		return (-1);
	}
	Trace(TR_DEBUG, "mcf device error: %s", err->msg);
	return (0);
}


/*
 * Process mcf line.
 */
static void
handle_mcf_line(void)
{

	node_t	*node;
	char	*p;
	int	i;
	long	val;
	int numeric_type = 0;
	base_dev_t	*cur_dev;

	Trace(TR_DEBUG, "handling mcf line: %s", dir_name);

	cur_dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
	if (cur_dev == NULL) {
		ReadCfgError(samerrno);
	}
	memset(cur_dev, 0, sizeof (base_dev_t));
	cur_dev->state = DEV_ON;

	/*
	 * Device name is a upath_t in dev_ent_t so 128 chars.
	 */
	if (strlen(dir_name) > MAX_PATH - 1) {
		/* Device path name '%s' exceeds %d characters */
		ReadCfgError(17241, dir_name, MAX_PATH - 1);
	}

	strcpy(cur_dev->name, dir_name);


	/*
	 * Verify unique device name.
	 */
	for (node = mcf->mcf_devs->head; node != NULL; node = node->next) {
		base_dev_t *tmp_dev = (base_dev_t *)node->data;
		if (strcmp(tmp_dev->name,
		    cur_dev->name) == 0) {
			if ((strcmp(cur_dev->name, "nodev")) != 0) {
				/*
				 * Equipment name '%s' already
				 * in use by eq %d
				 */
				ReadCfgError(17242, cur_dev->name,
				    tmp_dev->eq);
			}
		}
	}

	/*
	 * Equipment ordinal.
	 */
	if (ReadCfgGetToken() == 0) {
		/* Equipment ordinal missing */
		ReadCfgError(17243);
	}

	errno = 0;
	val = strtoll(token, &p, 10);
	if (*p != '\0' || errno != 0) {
		/* Invalid equipment ordinal '%s' */
		ReadCfgError(17244, token);
	}

	if (val == 0 || val > EQU_MAX) {
		/*
		 * Equipment ordinal %d must be in the
		 * range 1 <= eq_ord <= %d
		 */
		ReadCfgError(17245, val, EQU_MAX);
	}
	cur_dev->eq = (equ_t)val;

	/*
	 * Verify unique equipment ordinal.
	 */
	for (node = mcf->mcf_devs->head; node != NULL; node = node->next) {
		base_dev_t *tmp_dev = (base_dev_t *)node->data;
		if (tmp_dev->eq == cur_dev->eq) {
			/* Equipment ordinal %d already in use */
			ReadCfgError(17246, cur_dev->eq);
		}
	}

	/*
	 * In the config api whatever type the user enters is kept.
	 * so in this step the check simply verifies that the type
	 * is a correct type. This is different than readmcf.c which
	 * converts the type to a generic type that will be automatically
	 * reconverted when the device is probed. The generic type will
	 * be generated and stored in the base_dev_t numeric type field
	 * for use in error checking.
	 */
	if (ReadCfgGetToken() == 0) {
		/* Device type missing */
		ReadCfgError(17247);
	}

	numeric_type = nm_to_dtclass(token);
	if (numeric_type == 0) {
		/* Invalid device type '%s' */
		ReadCfgError(17248, token);
	}

	strcpy(cur_dev->equ_type, token);
	if (numeric_type == DT_HISTORIAN) {
		if (hist_found) {
			/* Only one historian allowed '%s' */
			ReadCfgError(17249, token);
		}
		hist_found = TRUE;
	}

	/*
	 * Family set name. This keeps the '-' too if no family_set name
	 */
	if (ReadCfgGetToken() != 0) {
		if (strlen(token) > MAX_UNAME - 1) {
			ReadCfgError(17250, token, MAX_UNAME - 1);
		}
		strcpy(cur_dev->set, token);
	}

	/*
	 * Device state.
	 */
	if (ReadCfgGetToken() != 0 && strcmp(token, "-") != 0) {
		for (i = 0; dev_state[i] != NULL; i++) {
			if (strcmp(dev_state[i], token) == 0) {
				cur_dev->state = i;
				break;
			}
		}
		if (dev_state[i] == NULL) {
			/* Invalid device state '%s' */
			ReadCfgError(17251, token);
		}
	}



	/*
	 * Additional parameters:  Catalog file, raw device path, etc.
	 */
	if (ReadCfgGetToken() != 0 && strcmp(token, "-") != 0) {
		if (strlen(token) > MAX_PATH - 1) {
			/* Path name '%s' exceeds %d characters */
			ReadCfgError(17252, token, MAX_PATH - 1);
		}
		switch (numeric_type & DT_CLASS_MASK) {
		case DT_PSEUDO:
			if (numeric_type != DT_HISTORIAN) {
				break;
			}
			/* FALLTHROUGH */

		case DT_ROBOT:
			strcpy(cur_dev->additional_params, token);
			break;

		case DT_TAPE:
			strcpy(cur_dev->additional_params, token);
			break;

		case DT_DISK: {
			char *rdsk;

			/*
			 * compare token to dev->name,
			 * and ensure they're related
			 */
			dsk2rdsk(cur_dev->name, &rdsk);
			if (rdsk == NULL || strncmp(rdsk, token,
			    strlen(rdsk)) != 0) {
				/*
				 * Optional raw device name doesn't
				 * match device
				 */
				ReadCfgError(17258, token, cur_dev->name);
			}
			strcpy(cur_dev->additional_params, token);
			free(rdsk);
			break;
		}

		case DT_FAMILY_SET:
			strcpy(cur_dev->additional_params, token);
			break;

		default:
			if (numeric_type == DT_PSEUDO_SC) {
				strcpy(cur_dev->additional_params, token);
			}
			break;
		}
	}

	if (ReadCfgGetToken() != 0) {
		/* Extra fields on line */
		ReadCfgError(17253);
	}



	if ((numeric_type & DT_FAMILY_SET) == DT_FAMILY_SET) {
		if (*cur_dev->set == '\0') {
			/* Family set name missing  */
			ReadCfgError(17254);
		}

		/*
		 * Verify unique family set name in fam set definition.
		 */
		for (node = mcf->mcf_devs->head; node != NULL;
		    node = node->next) {

			base_dev_t *tmp_dev = (base_dev_t *)node->data;
			if (strcmp(tmp_dev->set,
			    cur_dev->set) == 0) {
				/* Family set name '%s' in use by eq %d	 */
				ReadCfgError(17255, cur_dev->set,
				    tmp_dev->eq);
			}
		}
		if (numeric_type == DT_DISK_SET &&
		    strcmp(cur_dev->name, cur_dev->set) != 0) {
			/*
			 * Device name '%s' must match family
			 * set name '%s'
			 */
			ReadCfgError(17256, cur_dev->name,
			    cur_dev->set);
		}
	}

	/*
	 * Add device entry.
	 */
	if (cur_dev->eq > (equ_t)max_dev_eq) {
		max_dev_eq = cur_dev->eq;
	}
	if (lst_append(mcf->mcf_devs, cur_dev) != 0) {
		free(cur_dev);
		ReadCfgError(samerrno);
	}

	Trace(TR_DEBUG, "handled mcf line");
}



#define		is_scsi_robot(a) (((a) & DT_SCSI_ROBOT_MASK) == DT_SCSI_R)
#define		is_scsi_tape(a) (is_tape(a))


/*
 * nm_to_dtclass.c  - Check device mnemonic to be legal
 *
 * Description:
 *    Convert two character device mnemonic to device type.
 *	    If the device is any of the scsi robots or optical devices,
 *	    return the generic type for scsi robots or optical. (rb or od)
 *	    If the device is a scsi tape and uses the standard st device
 *	    driver, return the device type for standard tape. (tp)
 *
 * On entry:
 *    nm  = The mnemonic name for the device.
 *
 * Returns:
 *    The device type or 0 if the mnemonic is not recognized.
 *
 */
dtype_t
nm_to_dtclass(char *nm)
{

	int i;
	dtype_t dt;
	char *nmp = nm + 1;

	if (*nm == 'z') {
		dt = DT_THIRD_PARTY | (*(++nm) & 0xff);
		return (dt);
	}
	/* Striped groups - g0 - g127 */
	if (*nm == 'g' && *nmp >= '0' && *nmp <= '9') {
		dt = strtol(nmp, NULL, 10);
		if (dt < dev_nmsg_size) {
			dt = DT_STRIPE_GROUP | dt;
			return (dt);
		}
	}
	/* Target OSD groups - o0 - o127 */
	if (*nm == 'o' && *nmp >= '0' && *nmp <= '9') {
		dt = strtol(nmp, NULL, 10);
		if (dt >= 0 && dt < dev_nmog_size) {
			dt = DT_OBJECT_DISK | dt;
			return (dt);
		}
	}
	for (i = 0; i >= 0; i++) {
		if (dev_nm2dt[i].dt == 0)
			return (0);
		if (strcmp(nm, dev_nm2dt[i].nm) == 0)
			break;
	}

	dt = dev_nm2dt[i].dt;
	if (is_scsi_tape(dt))
		return (DT_TAPE);
	if (is_scsi_robot(dt))
		return (DT_ROBOT);
	if (is_optical(dt))
		return (DT_OPTICAL);


	return (dt);
}



/*
 * This message is a standin for the ConfigFileMsg function from
 * src/fs/fsd/fsd.c
 * This function is for processing configuration file processing messages.
 *
 * It is called by from readcfg.c when errors are encountered.
 *
 * If the msg field is null it means no error has occured.  In that case the
 * original code only printed the line and the line number if verbose was
 * specified.  That is what would be occuring in the else branch of the
 * main if.  We don't need or want to do that.
 *
 * This function builds up a list of errors that will be available to the
 * caller of parse_mcf.
 */
static void
handle_cfg_msg(
char *msg,	/* the error message */
int lineno,	/* input line number(if any) */
char *line)	/* the input line(if any) that contained the error */
{

	parsing_error_t *err;
	char err_buf[80];

	if (line != NULL) {
		if (msg != NULL) {


			/*
			 * Error encountered while reading the file.
			 * Error message.
			 */
			Trace(TR_OPRMSG, "line: %d: %s", lineno, line);
			Trace(TR_OPRMSG, "error: %s", msg);
			err = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));
			if (err == NULL) {
				mem_error = samerrno;
				return;
			}
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;

			if (lst_append(error_list, err) != 0) {
				mem_error = samerrno;
				return;
			}
		}

	} else if (lineno >= 0) {
		/*
		 * This branch is entered if an error is encountered after
		 * finished handling lines.  Also it is entered 1 time
		 * at end if any errors were encountered.
		 */
		Trace(TR_OPRMSG, "post mcf parsing error %s", msg);
	} else if (lineno < 0) {
		/* fopen error branch */
		if (errno == ENOENT) {
			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			snprintf(open_error, sizeof (open_error),
			    StrFromErrno(errno, err_buf,
			    sizeof (err_buf)));
			Trace(TR_OPRMSG, "mcf open error %s",
			    open_error);
		}
	}
}
