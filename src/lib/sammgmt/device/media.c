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
/*
 *	media.c -  APIs for device.h: calling media discovery and then
 *	get a list of directly attached library and remaining drive list,
 *	get catalog, vsn list and capacity based on MCF parsing information.
 */
#pragma	ident	"$Revision: 1.81 $"

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <unistd.h>

#include "pub/devstat.h"
#include "pub/lib.h"
#include "sam/lib.h"
#include "sam/types.h"
#include "sam/devnm.h"
#include "mgmt/util.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/media.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/archive.h"
#include "mgmt/config/master_config.h"
#include "pub/mgmt/license.h"
#include "parser_utils.h"
#include "mgmt/config/common.h"

#include <libgen.h>

#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "sam/param.h"
#include "aml/shm.h"	/* aml/device.h is included inside */
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"

#include <sys/types.h>
#include <sys/wait.h>


/*
 *	for device status
 */
#include "sam/defaults.h"
#include "aml/robots.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

#include "src/catalog/include/catlib_priv.h"
#include "sam/sam_trace.h"
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */
#define	SHM_ADDR(a, x)	((char *)a.shared_memory + (int)(x))

#define	EQ_ORD_NOT_DEFINED	0
#define	CATALOG_ENTRY_ITEMS	17
#define	CATALOG_SET_ITEMS	3
#define	CATALOG_ITEM_MAX_LENGTH	128


static int
is_robots(char *type);

static char *mcf_file = MCF_CFG;

static struct CatalogTableHdr nullCatalogTable = { { 0, 0, 0 }, 0 };

struct CatalogMap *Catalogs;

struct CatalogTableHdr *CatalogTable = &nullCatalogTable;

static int vsn_compare_vsn_ascending(void *, void *);

static int vsn_compare_vsn_descending(void *, void *);

static int vsn_compare_freespace_ascending(void *, void *);

static int vsn_compare_freespace_descending(void *, void *);

static int vsn_compare_slot_ascending(void *, void *);

static int vsn_compare_slot_descending(void *, void *);

static int
check_lib_dependency(char *);

static int
get_matching_media_vsns(sqm_lst_t *, sqm_lst_t *, vsnpool_property_t *);

int list_volumes(ctx_t *ctx, int lib_eq, char *restrictions,
    sqm_lst_t **vol_lst);


/*
 * discover all libraries that can be potentially added to the control of
 * SAM.
 * The following algorithm is used to discover unused libraries
 * 1) Read the mcf file to get the paths that have been added to SAM
 * 2) Discover any direct attached libraries
 * 3) scsi READ ELEMENT STATUS to get drives hosted by the library
 *
 * TBD: If a list of standalone drives is also to be identified, the user
 * should indicate that by an input param.
 */
int
discover_media(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t **lib_lst)	/* OUTPUT - list of libraries (library_t) */
{
	sqm_lst_t			*mcf_paths;

	Trace(TR_MISC, "discover media");

	if (ISNULL(lib_lst)) {

		Trace(TR_ERR, "discover media failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * get the libraries (direct-attached, network-attached) and drives
	 * that have been added to SAM's master config file
	 */
	if (get_paths_in_mcf(PATH_LIBRARY, &mcf_paths) != 0) {

		Trace(TR_ERR, "discover media failed: %s", samerrmsg);
		return (-1);
	}

	/* discover libraries, exclude the ones in mcf */
	if (discover_library(mcf_paths, lib_lst) != 0) {

		lst_free_deep(mcf_paths);

		Trace(TR_ERR, "discover media failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Currently in 4.6, there is no support for adding standalone drives
	 * to SAM from the File System Manager GUI. This support was available
	 * in 4.1 but was not deemed very useful to users, it was seldom used.
	 *
	 * Future: Take an input to indicate if standalone drives are to be
	 * discovered. To get the standalone drives, filter the drives that are
	 * a part of the library
	 */

	lst_free_deep(mcf_paths);

	Trace(TR_MISC, "finished discovering media()");
	return (0);
}


/* get_all_libraries() moved to media_util.c */


/* get library by path */
int
get_library_by_path(
ctx_t *ctx,			/* ARGSUSED */
const upath_t path,		/* INPUT - library's path */
library_t **library)		/* OUTPUT - structure library_t */
{
	sqm_lst_t		*lib_lst;
	library_t	*lib;
	node_t		*n;
	boolean_t	match_found = B_FALSE;

	if (ISNULL(path, library)) {
		Trace(TR_ERR, "get library by path failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "get library by path %s", path);

	if (get_all_libraries(ctx, &lib_lst) != 0) {
		Trace(TR_ERR, "get library by path failed: %s", samerrmsg);
		return (-1);
	}

	for (n = lib_lst->head; n != NULL; n = n->next) {

		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}

		if (strncmp(lib->base_info.name, path, strlen(path)) == 0) {
			*library = lib;

			n->data = NULL; /* no double free */
			match_found = B_TRUE;
			break;
		}
	}
	lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

	if (match_found == B_FALSE) {
		samerrno = SE_NO_LIBRARY_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get library by path failed: %s", samerrmsg);
		return (-1);

	}

	Trace(TR_MISC, "get library by path %s complete", path);
	return (0);
}


/* get a library by its family set name */
int
get_library_by_family_set(
ctx_t *ctx,			/* ARGSUSED */
const uname_t set,		/* INPUT - family set name */
library_t **library) 		/* OUTPUT - structure library_t */
{
	sqm_lst_t		*lib_lst;
	library_t	*lib;
	node_t		*n;
	boolean_t	match_found = B_FALSE;

	Trace(TR_MISC, "get library by family set %s", set);

	if (ISNULL(library)) {
		Trace(TR_ERR, "get library by set failed: %s", samerrmsg);
		return (-1);
	}

	if (get_all_libraries(ctx, &lib_lst) != 0) {
		Trace(TR_ERR, "get library by set failed: %s", samerrmsg);
		return (-1);
	}

	for (n = lib_lst->head; n != NULL; n = n->next) {
		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}

		if (strcmp(lib->base_info.set, set) == 0) {
			*library = lib;
			n->data = NULL; /* no double free */
			match_found = B_TRUE;
		}
	}
	lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

	if (match_found == B_FALSE) {
		samerrno = SE_NO_LIBRARY_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get library by set failed: %s", samerrmsg);
		return (-1);

	}
	Trace(TR_MISC, "get library by set %s complete", set);
	return (0);
}

/*
 * Given a list of libraries, find the library that matches the family set name
 */
int
find_library_by_family_set(
sqm_lst_t *lib_lst,
const char *set,
library_t **library)
{
	node_t		*n;
	library_t	*lib;
	boolean_t	match_found = B_FALSE;

	if (lib_lst == NULL) {
		return (get_library_by_family_set(NULL, set, library));
	}

	for (n = lib_lst->head; n != NULL; n = n->next) {
		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}

		if (strcmp(lib->base_info.set, set) == 0) {
			*library = lib;
			n->data = NULL; /* no double free */
			match_found = B_TRUE;
		}
	}

	if (match_found == B_FALSE) {
		samerrno = SE_NO_LIBRARY_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get library by set failed: %s", samerrmsg);
		return (-1);

	}
	return (0);
}


/* get library by its equipment identifier */
int
get_library_by_equ(
ctx_t *ctx,			/* ARGSUSED */
equ_t equ,			/* INPUT - equipment number of given library */
library_t **library)		/* OUTPUT - structure library_t */
{
	sqm_lst_t		*lib_lst;
	library_t	*lib;
	node_t		*n;
	boolean_t	match_found = B_FALSE;

	Trace(TR_MISC, "get library by equ %d", equ);

	if (ISNULL(library)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (get_all_libraries(ctx, &lib_lst) != 0) {
		Trace(TR_ERR, "get library by eq failed: %s", samerrmsg);
		return (-1);
	}

	for (n = lib_lst->head; n != NULL; n = n->next) {
		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}
		if (lib->base_info.eq == equ) {
			*library = lib;
			n->data = NULL;
			match_found = B_TRUE;
		}
	}
	lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

	if (match_found == B_FALSE) {
		samerrno = SE_NO_LIBRARY_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get library by eq failed: %s", samerrmsg);
		return (-1);

	}

	Trace(TR_MISC, "get library by equ %d complete", equ);
	return (0);
}


/*
 * Validate the library input by checking for the following:
 * 1. library is not NULL
 * 2. library/drive device path is not NULL
 * 3. library/drive type is supported
 * 4. library/drives family set name is not empty
 * 5. library has drive entries
 * 6. library/drives are not already configured in mcf
 * 7. duplicate catalog entry in mcf
 * 8. library does not have mixed media types
 */
static int
pre_addlibrary_validate(library_t *lib, mcf_cfg_t *mcf)
{
	node_t		*n, *p;
	int		match_flag	= 0;
	drive_t		*drv;
	devtype_t	equ = {0};
	base_dev_t	*dev;

	if (ISNULL(lib, mcf)) {
		return (-1);
	}

	/* Check for library path */
	if (lib->base_info.name[0] == '\0') {

		samerrno = SE_INVALID_LIBRARY_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/* Check for library type */
	if (strcmp(lib->base_info.equ_type, UNDEFINED_EQU_TYPE) == 0) {

		samerrno = SE_UNSUPPORTED_LIBRARY;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/* check for family set name */
	if (lib->base_info.set[0] == '\0') {

		samerrno = SE_INVALID_FAMILYSET_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/* check if drive entries are provided */
	if ((lib->no_of_drives == 0) || (lib->drive_list == NULL) ||
	    (lib->drive_list->length == 0)) {

		samerrno = SE_LIBRARY_HAS_NO_DRIVE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    lib->base_info.set);
		return (-1);
	}
	/* check if the library/drives is already added to master config file */
	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {

		dev = (base_dev_t *)n->data;
		if (strcmp(lib->base_info.name, dev->name) == 0 ||
		    strcmp(lib->base_info.set, dev->set) == 0) {

			samerrno = SE_LIBRARY_ALREADY_EXIST;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    lib->base_info.name);
			return (-1);
		}
		/* duplicate catalog entry in master config file */
		if ((strlen(lib->catalog_path) > 0) &&
		    (strcmp(lib->catalog_path, dev->additional_params) == 0)) {

			samerrno = SE_CATALOG_ALREADY_EXIST;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    lib->base_info.additional_params);
			return (-1);
		}
		for (p = lib->drive_list->head; p != NULL; p = p->next) {
			drv = (drive_t *)p->data;
			if (drv == NULL) {
				continue;
			}

			if (drv->base_info.name[0] == '\0') {

				samerrno =  SE_INVALID_DRIVE_NAME;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				return (-1);
			}

			if (strcmp(drv->base_info.name, dev->name) == 0) {

				samerrno = SE_DRIVE_ALREADY_EXIST;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), drv->base_info.name);
				return (-1);
			}

			if (strcmp(drv->base_info.equ_type,
			    UNDEFINED_EQU_TYPE) == 0) {

				samerrno = SE_UNSUPPORTED_DRIVE;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				return (-1);
			}
			/* check for mixed drive type */
			if (equ[0] == '\0') {
				strcpy(equ, drv->base_info.equ_type);
			} else if (strcmp(equ, drv->base_info.equ_type) != 0) {

				samerrno = SE_MIXED_MEDIA_LIBRARY;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), lib->base_info.name);
				return (-1);
			}
		}
	}

	return (0);

}

/* insert the library dev and the drive dev entry into the mcf structure */
static int
build_dev(library_t *lib, mcf_cfg_t *mcf)
{

	node_t		*n, *p;
	sqm_lst_t		*eqs;
	int		eqs_needed	= 0;
	drive_t		*drv;
	base_dev_t	*dev;

	if (ISNULL(lib, mcf, mcf->mcf_devs)) {
		return (-1);
	}
	eqs_needed = lib->no_of_drives + 1; /* one for library */

	if (get_available_eq_ord(mcf, &eqs, eqs_needed) != 0) {
		return (-1);
	}

	if (eqs->length != eqs_needed) {
		/* unable to get eqs */
		return (-1);
	}

	dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
	if (dev == NULL) {
		Trace(TR_ERR, "build library dev cfg failed: %s", samerrmsg);
		return (-1);
	}

	n = eqs->head;
	/* assign equipment ordinal */
	if (lib->base_info.eq == EQ_ORD_NOT_DEFINED && n != NULL) {
		lib->base_info.eq = *((equ_t *)n->data);
		n = n->next;
	}

	dev_cpy(dev, &(lib->base_info));
	/* set catalog file path if there is a valid one */
	if (lib->catalog_path[0] == '\0') {
		strlcpy(dev->additional_params, lib->catalog_path,
		    sizeof (dev->additional_params));
	}
	if (lst_append(mcf->mcf_devs, dev) != 0) {
		free(dev);
		Trace(TR_ERR, "build library dev cfg failed: %s", samerrmsg);
		return (-1);
	}

	for (p = lib->drive_list->head; p != NULL; p = p->next) {
		drv = (drive_t *)p->data;

		strlcpy(drv->base_info.set, lib->base_info.set,
		    sizeof (drv->base_info.set));

		if (drv->base_info.eq == EQ_ORD_NOT_DEFINED && n != NULL) {
			drv->base_info.eq = *((equ_t *)n->data);
			n = n->next;
		}
		drv->base_info.fseq = lib->base_info.eq;
		dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
		if (dev == NULL) {
			lst_free_deep(eqs);
			Trace(TR_ERR, "build library dev cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
		dev_cpy(dev, &(drv->base_info));
		if (lst_append(mcf->mcf_devs, dev) != 0) {
			free(dev);
			Trace(TR_ERR, "build library dev cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

}


/*
 * add library to SAM's configuration. This includes writing the device config
 * to master configuration file, creating the parameter file (optional) and
 * restarting sam-fsd.
 * Pre-requisites: The physical path of the library, the library type, and the
 * family set name for the library must be provided as input.
 */
int
add_library(
ctx_t *ctx,		/* ARGSUSED */
library_t *lib)		/* the library which need to be added to MCF */
{
	node_t		*n;
	mcf_cfg_t	*mcf;
	sqm_lst_t		*fset;

	Trace(TR_MISC, "adding a library");

	if (ISNULL(lib)) {
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	if (pre_addlibrary_validate(lib, mcf) != 0) {
		free_mcf_cfg(mcf);
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	if (build_dev(lib, mcf) != 0) {
		free_mcf_cfg(mcf);
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	/* adding stk parameter */
	if (strcmp(lib->base_info.equ_type, "sk") == 0) {
		if (write_stk_param(lib->storage_tek_parameter,
		    lib->base_info.name) != 0) {

			free_mcf_cfg(mcf);
			Trace(TR_ERR, "add library failed: %s", samerrmsg);
			return (-1);
		}
	}

	/* get the family set from the mcf struct */
	if (get_family_set_devs(mcf, lib->base_info.set, &fset) != 0) {

		free_mcf_cfg(mcf);
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	/* add the new family set */
	if (add_family_set(ctx, fset) != 0) {

		lst_free_deep(fset);
		free_mcf_cfg(mcf);
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}

	lst_free_deep(fset);
	free_mcf_cfg(mcf);

	Trace(TR_MISC, "library %s is added to mcf", lib->base_info.name);
	if (init_library_config(ctx) != 0) {
		Trace(TR_ERR, "add library failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "add library successful");
	return (0);
}


/*
 * add a list of libraries to SAM's configuration. This is typically used when
 * a STK library with mixed drive types is to be added, these are represented
 * as multiple virtual libraries, one per drive type.
 * The following errors are treated as complete failures - indicating that none
 * of the libraries in the list will be added:
 *
 * 1. input is NULL
 * 2. mcf file cannot be opened or read or written to
 * 3. samd stop/config/start could not be run successfully
 * 4. no libraries could be added
 *
 * If addition of some libraries fail, it is considered to be a partial failure,
 * and represented by a return of -2.
 */
int
add_list_libraries(
ctx_t *ctx,		/* ARGSUSED */
sqm_lst_t *library_list) /* List of libraries to be added to MCF */
{
	mcf_cfg_t	*mcf;
	node_t		*n;
	library_t	*lib;
	sqm_lst_t		*fset;
	boolean_t	partial_fail = B_FALSE;

	Trace(TR_MISC, "add a list of libraries");

	if (ISNULL(library_list)) {
		Trace(TR_ERR, "add list of libraries failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {
		Trace(TR_ERR, "add list of libraries failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (n = library_list->head; n != NULL; n = n->next) {

		lib = (library_t *)n->data;
		if (lib == NULL) {
			continue;
		}

		if (pre_addlibrary_validate(lib, mcf) != 0) {
			/* ignore partial failures */
			Trace(TR_MISC, "Unable to add library: %s",
			    samerrmsg);
			partial_fail = B_TRUE;
			continue;
		}

		if (build_dev(lib, mcf) != 0) {
			partial_fail = B_TRUE;
			continue;
		}

		/* add parameter for stk ACSLS */
		if (strcmp(lib->base_info.equ_type, "sk") == 0) {
			if (write_stk_param(lib->storage_tek_parameter,
			    lib->base_info.name) != 0) {
				Trace(TR_ERR, "add list of libraries failed:%s",
				    samerrmsg);
				partial_fail = B_TRUE;
				continue;
			}
		}

	} /* finished creating library entries */

	/* Now add a family set for each library */
	for (n = library_list->head; n != NULL; n = n->next) {

		/* get the family set from the mcf struct */
		if (get_family_set_devs(
		    mcf,
		    ((library_t *)n->data)->base_info.set,
		    &fset) != 0) {

			free_mcf_cfg(mcf);
			Trace(TR_ERR, "add list of libraries failed: %s",
			    samerrmsg);
			partial_fail = B_TRUE;
			continue;
		}

		if (add_family_set(ctx, fset) != 0) {

			lst_free_deep(fset);
			free_mcf_cfg(mcf);

			Trace(TR_ERR, "add list of libraries failed: %s",
			    samerrmsg);
			partial_fail = B_TRUE;
			continue;
		}
		lst_free_deep(fset);
	}
	free_mcf_cfg(mcf);

	if (init_library_config(ctx) != 0) {
		Trace(TR_ERR, "add list of libraries failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (partial_fail == B_TRUE) {
		Trace(TR_ERR, "Some library(ies) could not be added");
		return (-2);
	}

	Trace(TR_MISC, "add list of libraries success");
	return (0);
}


/* get_all_standalone_drives() moved to media_util.c */

/*
 * get catalog entry when the volume name is given as input
 * If no entry is found with the given volname, return empty
 * list. The return is a list rather than a single Catalog_Entry
 * because it is probable (not recommended) that a different
 * media type can have the same volname
 */
int
get_catalog_entry(
ctx_t *ctx,		/* ARGSUSED */
const vsn_t vsn,	/* vsn name */
sqm_lst_t **ce_lst)	/* a list of CatalogEntry */
{
	struct CatalogEntry	*ce;
	node_t			*n;
	sqm_lst_t			*catalogs;

	Trace(TR_MISC, "get catalog entry by volname[%]s", vsn);
	if (ISNULL(ce_lst)) {
		Trace(TR_ERR, "get catalog entry failed: %s", samerrmsg);
		return (-1);
	}

	if (get_all_catalog(&catalogs) == -1) {
		Trace(TR_ERR, "get catalog entry failed: %s", samerrmsg);
		return (-1);
	}

	*ce_lst = lst_create();
	if (*ce_lst == NULL) {
		lst_free_deep(catalogs);
		Trace(TR_ERR, "get catalog entry failed: %s", samerrmsg);
		return (-1);
	}

	for (n = catalogs->head; n != NULL; n = n->next) {
		ce = (struct CatalogEntry *)n->data;
		if (strcmp(ce->CeVsn, vsn) == 0) {
			if (lst_append(*ce_lst, ce) != 0) {

				lst_free_deep(*ce_lst);
				lst_free_deep(catalogs);

				Trace(TR_ERR, "get catalog entry failed:%s",
				    samerrmsg);
				return (-1);
			}
			n->data = NULL; /* no double free */
		}
		n = n->next;
	}
	lst_free_deep(catalogs);

	Trace(TR_MISC, "get catalog entry success");
	return (0);
}


/*
 * Deprecated: use list_volumes() from release 5.0
 * get catalog entry given its volname and media type
 */
int
get_catalog_entry_by_media_type(
ctx_t *ctx,			/* ARGSUSED */
vsn_t vsn,			/* INPUT - vsn name */
mtype_t media_type,		/* INPUT - media type */
struct CatalogEntry **ce)	/* OUTPUT - Catalog entry struct */
{
	char	restrictions[MAXPATHLEN];
	sqm_lst_t	*vol_lst = NULL;
	node_t	*n;

	snprintf(restrictions, MAXPATHLEN, "volname=%s, mtype=%s",
	    vsn, media_type);

	if (list_volumes(NULL, -1, restrictions, &vol_lst)) {
		Trace(TR_ERR, "get catalog entry by mtype failed: %s",
		    samerrmsg);
		return (-1);
	}
	n = vol_lst->head;
	if (n != NULL) {
		*ce = (struct CatalogEntry *)n->data;
		n->data = NULL; /* no double free */
		/* only one catalog entry required */
	}
	lst_free_deep(vol_lst);
	return (0);
}


/*
 * Deprecated: use list_volumes() from release 5.0
 * get a catalog entry given the volname and the library eq id
 */
int
get_catalog_entry_from_lib(
ctx_t *ctx,			/* ARGSUSED */
const equ_t library_eq,		/* eq number of the library */
const int slot,			/* slot number of the catalog entry */
const int partition,		/* partition */
struct CatalogEntry **ce)	/* It must be freed by caller */
{
	node_t	*n;
	sqm_lst_t	*vol_lst = NULL;
	char	restrictions[MAXPATHLEN];

	Trace(TR_MISC, "get catalog entry from library eq %d slot %d",
	    library_eq, slot);

	if (ISNULL(ce)) {
		Trace(TR_ERR, "get catalog entry failed: %s", samerrmsg);
		return (-1);
	}

	snprintf(restrictions, MAXPATHLEN, "startslot=%d, partition=%d",
	    slot, partition);

	if (list_volumes(NULL, library_eq, restrictions, &vol_lst)) {
		Trace(TR_ERR, "get catalog entry from library failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (vol_lst == NULL || vol_lst->length == 0) {
		samerrno = SE_NO_TARGET_EQ_SLOT_PART_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    library_eq, slot, partition);
		Trace(TR_ERR, "get catalog entry failed: %s", samerrmsg);
		return (-1);
	}

	n = vol_lst->head;
	if (n != NULL) {
		*ce = (struct CatalogEntry *)n->data;
		n->data = NULL; /* no double free */
		/* only one catalog entry required */
	}
	lst_free_deep(vol_lst);

	Trace(TR_MISC, "get catalog entry from library[%d:%d] success",
	    library_eq, slot);
	return (0);
}


/*
 * Deprecated: use list_volumes() from release 5.0
 * get all catalog entries in a specific library
 */
int
get_all_catalog_entries(
ctx_t *ctx,		/* ARGSUSED */
equ_t lib_eq,		/* eq number of the library */
int start,		/* IN - starting index in the list */
int size,		/* IN - num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,	/* IN - sort key */
boolean_t ascending,		/* IN - ascending order */
sqm_lst_t **ce_lst)	/* a list of CatalogEntry, */
{

	Trace(TR_MISC, "get all volumes in library[%d]", lib_eq);
	if (ISNULL(ce_lst)) {
		Trace(TR_ERR, "get all volumes failed: %s", samerrmsg);
		return (-1);
	}

	if (list_volumes(NULL, lib_eq, NULL /* no filter */, ce_lst)) {
		Trace(TR_ERR, "get all volumes failed: %s", samerrmsg);
		return (-1);
	}

	sort_catalog_list(start, size, sort_key, ascending, *ce_lst);

	Trace(TR_MISC, "get all volumes in library[%d] success", lib_eq);
	return (0);
}


/*
 * get the number of catalog entries in a library
 */
int
get_no_of_catalog_entries(
ctx_t *ctx,			/* ARGSUSED */
equ_t lib_eq,			/* eq number of the library */
int *number_of_catalog_entres)	/* the number of catalog entries */
{
	struct CatalogEntry *list;
	int n_entries;

	Trace(TR_MISC, "get number of catalog entries in library[%d]", lib_eq);

	if (CatalogInit("get_no_of_catalog_entries") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, " get catalog entry count failed: %s", samerrmsg);
		return (-1);
	}
	list = CatalogGetEntriesByLibrary(lib_eq, &n_entries);
	free(list);
	*number_of_catalog_entres = n_entries;
	Trace(TR_MISC, "get catalog entry count success");
	return (0);
}


/*
 *	given a regular expression and get a catalog list.
 */
int
get_vsn_list(
ctx_t *ctx,			/* ARGSUSED */
const char *vsn_reg_exp,	/* regular expression for VSNs */
int start,	/* IN - starting index in the list */
int size,	/* IN - num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
sqm_lst_t **catalog_entry_list)	/* a list of CatalogEntry, */
				/* must be freed by caller */
{
	sqm_lst_t *catalog_list;
	node_t *node_c;
	struct CatalogEntry *cat_info;
	int ii;
	char *reg_ptr, *reg_rtn;

	Trace(TR_MISC, "getting a list of catalog entry"
	    " given a regular expression %s", vsn_reg_exp);

	if (ISNULL(vsn_reg_exp, catalog_entry_list)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	create a list here and it must be freed in
	 *	the calling function.
	 */
	*catalog_entry_list = lst_create();
	if (*catalog_entry_list == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	get all catalog list, then match the vsn
	 *	with the given vsn to generate requested
	 *	catalog entry list.
	 */
	ii =  get_all_catalog(&catalog_list);
	if (ii == -1) {
		if (samerrno == SE_CATALOG_INIT_FAILED) {
			return (0);
		} else {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	}

	if (catalog_list -> length == 0) {
		return (0);
	}
	reg_ptr = regcmp(vsn_reg_exp, NULL);
	if (reg_ptr == NULL) {
		samerrno = SE_GET_REGEXP_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_GET_REGEXP_FAILED),
		    vsn_reg_exp);
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	node_c = catalog_list->head;
	while (node_c != NULL) {
		cat_info = (struct CatalogEntry *)node_c->data;
		reg_rtn = regex(reg_ptr, cat_info->CeVsn);
		if (reg_rtn != NULL) {
			if (lst_append(*catalog_entry_list,
			    cat_info) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				goto error;
			}
			node_c ->data = NULL;
		}
		node_c = node_c->next;
	}
	free_list_of_catalog_entries(catalog_list);

	sort_catalog_list(
		start, size, sort_key, ascending, *catalog_entry_list);

	free(reg_ptr);
	Trace(TR_MISC, "finished getting a list of catalog entry"
		" given a regular expression %s", vsn_reg_exp);
	return (0);
error:
	free_list_of_catalog_entries(catalog_list);
	free_list_of_catalog_entries(*catalog_entry_list);
	return (-1);
}


/*
 *	get_total_capacity_of_library()
 *	get the total capacity of all vsn given a library.
 */
int
get_total_capacity_of_library(
ctx_t *ctx,		/* ARGSUSED */
equ_t lib_eq,		/* eq number of the given library */
fsize_t *capacity)	/* the library's capacity based on the catalog */
{
	struct CatalogEntry *cat_info;
	struct CatalogEntry *list;
	int n_entries;
	int i;

	Trace(TR_MISC, "getting total capacity of library %d", lib_eq);

	*capacity = 0;
	if (CatalogInit("get_all_catalog") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;	/* sam error 2364 */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	list = CatalogGetEntriesByLibrary(lib_eq, &n_entries);
	for (i = 0; i < n_entries; i++) {
		cat_info = &list[i];
		*capacity += cat_info->CeCapacity;
	}
	free(list);

	Trace(TR_MISC, "finished getting total capacity of library %d",
	    lib_eq);
	return (0);
}


/*
 *	get_free_space_of_library()
 *	get the total capacity of all vsn given a library.
 */
int
get_free_space_of_library(
ctx_t *ctx,			/* ARGSUSED */
equ_t lib_eq,			/* eq number of the given library */
fsize_t *free_space)		/* the library's free space based */
				/* on the catalog */
{
	struct CatalogEntry *list;
	int n_entries;
	struct CatalogEntry *cat_info;
	int i;

	Trace(TR_MISC, "getting free space of library %d",
		lib_eq);

	*free_space = 0;
	if (CatalogInit("get_all_catalog") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;	/* sam error 2364 */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	list = CatalogGetEntriesByLibrary(lib_eq, &n_entries);
	for (i = 0; i < n_entries; i++) {
		cat_info = &list[i];
		*free_space += cat_info->CeSpace;
	}
	free(list);
	Trace(TR_MISC, "getting free space of library %d",
		lib_eq);
	return (0);
}


/*
 *	get the properties given a archive vsn pool name.
 */
int
get_properties_of_archive_vsnpool(
ctx_t *ctx,				/* ARGSUSED */
const upath_t pool_name,		/* archive vsn pool name */
int start,	/* IN - starting index in the list */
int size,	/* IN - num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
vsnpool_property_t **vpp)	/* It must be freed by caller */
{
	vsn_pool_t *vsn_pool = NULL;

	Trace(TR_MISC, "getting the property of archive vsn pool %s",
	    pool_name);

	if (get_vsn_pool(NULL, pool_name, &vsn_pool) == -1) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	get_properties_of_archive_vsnpool1(
	    ctx, *vsn_pool, start, size, sort_key, ascending, vpp);
	free_vsn_pool(vsn_pool);

	Trace(TR_MISC, "finished getting the property of archive vsn pool %s",
	    pool_name);
	return (0);
}


/*
 *	given a archive vsn pool name, check all vsns inside that
 *	archive vsn pool.  If a vsn's free space is not zero, add it
 *	to the list.
 */
int
get_available_vsns(
ctx_t *ctx,			/* ARGSUSED */
upath_t archive_vsn_pool_name,	/* archive vsn pool's name */
int start,	/* IN - starting index in the list */
int size,	/* IN - num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
sqm_lst_t **catalog_entry_list)	/* a list of CatalogEntry, */
				/* must be freed by caller */
{
	int ii;
	vsnpool_property_t *pool_prop;
	node_t *node_c;
	struct CatalogEntry *cat_info;

	Trace(TR_MISC, "getting available vsns given the vsn pool %s",
		archive_vsn_pool_name);

	*catalog_entry_list = NULL;
	ii =  get_properties_of_archive_vsnpool(ctx,
		archive_vsn_pool_name, 0, -1, VSN_NO_SORT, ascending,
		&pool_prop);
	if (ii == -1) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if ((*catalog_entry_list = lst_create()) == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (pool_prop->catalog_entry_list == NULL) {
		samerrno = SE_NO_CATALOG_ENTRIES;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_CATALOG_ENTRIES));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	node_c = pool_prop->catalog_entry_list->head;
	while (node_c != NULL) {
		cat_info = (struct CatalogEntry *)node_c->data;
		if (cat_info->CeSpace > 0) {
			if (lst_append(*catalog_entry_list,
			    cat_info) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				goto error;
			}
		}
		node_c = node_c->next;
	}

	sort_catalog_list(
	    start, size, sort_key, ascending, *catalog_entry_list);


	free_vsnpool_property(pool_prop);
	Trace(TR_MISC, "finished getting available vsns given the vsn pool %s",
	    archive_vsn_pool_name);
	return (0);
error:
	free_vsnpool_property(pool_prop);
	free_list_of_catalog_entries(*catalog_entry_list);
	return (-1);
}


/*
 * get the network-attached library properties, given the parameter file
 * location, eq, catalog path and network-attached library type
 */
int get_nw_library(
ctx_t *ctx,				/* ARGSUSED */
nwlib_req_info_t *info,			/* INPUT - given nwlib_req_info_t */
library_t **lib)			/* OUTPUT */
{
	struct stat statbuf;
	stk_param_t *stkparam;
	nwlib_base_param_t *nw_param;
	node_t *n;
	stk_device_t *stk_dev;
	drive_param_t *drive_param;
	drive_t *drive;
	char *rest;
	char *p;
	upath_t temp_path;
	upath_t check_path;
	upath_t check_str;
	int i;
	size_t cat_len;

	Trace(TR_MISC, "get network library information");
	if (ISNULL(info, lib)) {
		Trace(TR_ERR, "get network library failed: %s", samerrmsg);
		return (-1);
	}

	*lib = (library_t *)mallocer(sizeof (library_t));
	if (*lib == NULL) {
		Trace(TR_ERR, "get network library failed: %s", samerrmsg);
		return (-1);
	}
	memset(*lib, 0, sizeof (library_t));

	strlcpy((*lib)->catalog_path, info->catalog_loc, sizeof (upath_t));

	/* If catalog path is a directory, return error */
	if (lstat(info->catalog_loc, &statbuf) < 0) {
		Trace(TR_ERR, "catalog path %s does not exist",
		    info->catalog_loc);

	} else {
		if (S_ISDIR(statbuf.st_mode)) {

			free_library(*lib);

			samerrno = SE_CATALOG_PATH_IS_DIR;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    info->catalog_loc);

			Trace(TR_ERR, "get nw library failed: %s", samerrmsg);
			return (-1);
		}
	}

	/* If parameter file is a directory, return error */
	if (lstat(info->parameter_file_loc, &statbuf) < 0) {

		free_library(*lib);

		samerrno = SE_PATH_CHECK_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    info->parameter_file_loc);

		Trace(TR_ERR, "get network library failed: %s", samerrmsg);
		return (-1);

	}

	if (S_ISDIR(statbuf.st_mode)) {

		free_library(*lib);

		samerrno = SE_PARAMETER_FILE_PATH_IS_DIR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    info->parameter_file_loc);

		Trace(TR_ERR, "get network library failed: %s", samerrmsg);
		return (-1);
	}

	/* Check if catalog_path is valid */
	strlcpy(check_str, info->catalog_loc, sizeof (check_str));
	p = strtok_r(check_str, "/", &rest);
	while (p != NULL) {
		strlcpy(temp_path, p, sizeof (temp_path));
		p = strtok_r(NULL, "/", &rest);
	}

	cat_len = strlen(info->catalog_loc) - strlen(temp_path);
	for (i = 0; i < cat_len; i++) {
		check_path[i] = info->catalog_loc[i];
	}
	check_path[cat_len] = '\0';

	if (lstat(check_path, &statbuf) == -1) {
		samerrno = SE_CATALOG_PATH_CHECK_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    check_path);
		return (-1);
	}

	strlcpy((*lib)->base_info.additional_params, info->catalog_loc,
	    sizeof ((*lib)->base_info.additional_params));

	strlcpy((*lib)->base_info.set, info->nwlib_name,
	    sizeof ((*lib)->base_info.set));

	strlcpy((*lib)->base_info.name, info->parameter_file_loc,
	    sizeof ((*lib)->base_info.name));

	(*lib)->base_info.eq = info->eq;

	(*lib)->drive_list = lst_create();
	if ((*lib)->drive_list == NULL) {

		free_library(*lib);

		Trace(TR_ERR, "get network library failed: %s", samerrmsg);
		return (-1);
	}

	switch (info->nw_lib_type) {
		case DT_STKAPI:
			strlcpy((*lib)->base_info.equ_type,
			    dt_to_nm(info->nw_lib_type),
			    sizeof ((*lib)->base_info.equ_type));

			if (read_parameter_file(
			    info->parameter_file_loc,
			    info->nw_lib_type,
			    (void **)&stkparam) == -1) {

				free_library(*lib);

				Trace(TR_ERR, "get network library failed: %s",
				    samerrmsg);
				return (-1);
			}
			for (n = stkparam->stk_device_list->head;
			    n != NULL; n = n->next) {

				stk_dev = (stk_device_t *)n->data;
				if (stk_dev == NULL) {
					continue;
				}
				if (discover_standalone_drive_by_path(
				    NULL,
				    &drive,
				    stk_dev->pathname) == -1) {

					free_stk_param(stkparam);
					free_library(*lib);
					Trace(TR_ERR, "%s", samerrmsg);
					return (-1);
				}
				strlcpy(drive->base_info.name,
				    stk_dev->pathname,
				    sizeof (drive->base_info.name));
				strlcpy(drive->base_info.set,
				    (*lib)->base_info.set,
				    sizeof (drive->base_info.set));
				drive->shared = stk_dev->shared;

				if (lst_append((*lib)->drive_list,
				    drive) != 0) {

					free_drive(drive);
					free_stk_param(stkparam);
					free_library(*lib);
					Trace(TR_ERR, "get network library "
					    "failed: %s", samerrmsg);
					return (-1);
				}
			}
			free_stk_param(stkparam);
			break;

		case DT_IBMATL:
		case DT_SONYPSC:
		case DT_GRAUACI:

			strlcpy((*lib)->base_info.equ_type,
			    dt_to_nm(info->nw_lib_type),
			    sizeof ((*lib)->base_info.equ_type));
			if (read_parameter_file(
			    info->parameter_file_loc,
			    info->nw_lib_type,
			    (void **)&nw_param) == -1) {

				free_library(*lib);
				Trace(TR_ERR, "get network library failed: %s",
				    samerrmsg);
				return (-1);
			}

			for (n = nw_param->drive_lst->head;
			    n != NULL; n = n->next) {

				drive_param = (drive_param_t *)n->data;
				if (drive_param == NULL) {
					continue;
				}
				if (discover_standalone_drive_by_path(
				    NULL,
				    &drive,
				    drive_param->path) == -1) {

					free_nwlib_base_param(nw_param);
					free_library(*lib);
					Trace(TR_ERR, "get network library "
					    "failed: %s", samerrmsg);
					return (-1);
				}
				strlcpy(drive->base_info.name,
				    drive_param->path,
				    sizeof (drive->base_info.name));
				strlcpy(drive->base_info.set,
				    (*lib)->base_info.set,
				    sizeof (drive->base_info.set));
				/* drive->library_name not used */

				if (lst_append((*lib)->drive_list,
				    drive) != 0) {

					free_drive(drive);
					free_nwlib_base_param(nw_param);
					free_library(*lib);
					Trace(TR_ERR, "get network library "
					    "failed: %s", samerrmsg);
					return (-1);
				}
			}
			free_nwlib_base_param(nw_param);
			break;

		default:
			free_library(*lib);
			samerrno = SE_NW_ATT_LIB_TYPE_NOT_DEFINED;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "get network library failed: %s",
			    samerrmsg);
			return (-1);
	}

	Trace(TR_MISC, "get network library complete");
	return (0);
}


/*
 *	remove a library from sam's control.
 */
int
remove_library(
ctx_t *ctx,		/* ARGSUSED */
equ_t library_eq,	/* eq number of the library */
boolean_t unload_first)	/* unload the library before removing it */
{
	mcf_cfg_t *mcf;
	node_t  *node;
	base_dev_t *dev;
	int match_flag = 0;
	int i;
	upath_t	family_set;
	devtype_t	check_equtype;
	upath_t para_file;
	uname_t lib_type;

	Trace(TR_MISC, "removing a library from MCF for eq %d", library_eq);
	if (library_eq < 1) {
		samerrno = SE_INVALID_EQ_ORD;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_EQ_ORD),
		    library_eq);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {
		Trace(TR_ERR, "Read of %s failed with error: %s",
		    mcf_file, samerrmsg);
		return (-1);
	}

	for (node = mcf->mcf_devs->head;
	    node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;
			if (is_robots(dev->equ_type) == 0 &&
			    dev->eq == library_eq) {
				strlcpy(family_set, dev->set,
				    sizeof (family_set));
				strlcpy(para_file, dev->name,
				    sizeof (para_file));
				strlcpy(check_equtype, dev->equ_type,
				    sizeof (check_equtype));
				strlcpy(lib_type, dev->equ_type,
				    sizeof (lib_type));
				break;
			}
		}
	}

	/*
	 * check library dependency.
	 */
	if (check_lib_dependency(family_set) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		free_mcf_cfg(mcf);
		return (-1);
	}

	Trace(TR_MISC, "family set for deletion is %s", family_set);

	for (node = mcf->mcf_devs->head;
	    node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;
			if (strcmp(dev->set, family_set) == 0) {

				Trace(TR_DEBUG, "dev path %s, type",
				    dev->name, dev->equ_type);
				if (lst_remove(mcf->mcf_devs, node) != 0) {
					free_mcf_cfg(mcf);
					Trace(TR_ERR, "%s", samerrmsg);
					return (-1);
				}
				free(dev);
				match_flag = 1;
			}
		}
	}
	free_mcf_cfg(mcf);

	if (match_flag == 0) {
		samerrno = SE_NO_LIBRARY_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);

	}

	/*
	 * if the library is network attached library, remote,
	 * there is no need to do unload.
	 */
	if (is_special_type(check_equtype) != 0) {
		if (unload_first == B_TRUE) {
			i = rb_unload(NULL, library_eq, B_FALSE);
			if (i < 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				return (-1);
			}
		}
	}

	/* Remove the family set from the mcf file */
	if (remove_family_set(ctx, family_set) != 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/* removing parameter file */
	if (strcmp(lib_type, "sk") == 0) {
		if (remove_stk_param(para_file) != 0) {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	}


	if (ctx != NULL && *ctx->dump_path != '\0') {
		/* since this is a dump don't log or call sighup */
		Trace(TR_OPRMSG, "removing a library: Wrote to %s.",
		    ctx->dump_path);
		return (0);
	}

	Trace(TR_FILES, "library with eq %d is removed from mcf\n",
	    library_eq);
	if (init_library_config(ctx) < 0) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "finished removing a library from MCF");
	return (0);
}


/*
 *	API discover_media_unused_in_mcf()
 *	discover all the direct-attached libraries and tape drives
 *	that sam can potentially control and exclude those which have
 *	been already defined in MCF. If nothing is found, we return
 *	success with empty list.
 */
int
discover_media_unused_in_mcf(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **library_list)	/* OUTPUT - a list of structure library_t */
{
	return (discover_media(ctx, library_list));
}


/*
 *	API get_default_media_block_size().
 *	This function will get default block size
 *	given a media type.
 */
int
get_default_media_block_size(
ctx_t *ctx,			/* ARGSUSED */
mtype_t media_type,		/* media type */
int *def_blocksize)		/* the default block size */
{
	int i;
	int flag = 0;
	sam_defaults_t *defaults;
	struct DeviceParams *DeviceParams;

	Trace(TR_MISC, "getting default media block size");
	defaults = GetDefaults();
	*def_blocksize = 0;

	DeviceParams = (struct DeviceParams *)
	    (void *)((char *)defaults + sizeof (sam_defaults_t));

	for (i = 0; i < DeviceParams->count; i++) {
		struct DpEntry *dp;
		dp = &DeviceParams->DpTable[i];
		if (strcmp(media_type, dp->DpName) == 0) {
			*def_blocksize = dp->DpBlock_size;
			flag = 1;
			break;
		}
	}

	if (flag == 0) {
		samerrno = SE_UNKNOWN_MEDIA_TYPE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_UNKNOWN_MEDIA_TYPE),
		    media_type);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "finished getting default media block size");
	return (0);
}


/*
 *	check if a equ_type is a robot
 */
static int
is_robots(
char *type)	/* equipment type */
{
	char *rb_type;
	int i = 0;

	for (rb_type = dev_nmrb[i]; rb_type != NULL; rb_type = dev_nmrb[i++]) {
		if (strcmp(rb_type, type) == 0) {
			return (0);
		}
	}
	return (-1);
}


/*
 * get all catalog entries managed by SAM
 */
int
get_all_catalog(
sqm_lst_t **ce_lst)	/* a list of CatalogEntry */
{
	struct CatalogEntry *list;
	struct CatalogEntry *ce, *ptr;
	sqm_lst_t *lib_lst;
	sqm_lst_t *drv_lst;
	int i, j;
	int n_entries;

	Trace(TR_OPRMSG, "get all catalogs under SAM");
	if (ISNULL(ce_lst)) {
		Trace(TR_ERR, "get all catalog failed: %s", samerrmsg);
		return (-1);
	}

	if (get_all_libraries(NULL, &lib_lst) != 0) {
		Trace(TR_ERR, "get all catalog failed: %s", samerrmsg);
		return (-1);
	}

	if (get_all_standalone_drives(NULL, &drv_lst) != 0) {
		lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

		Trace(TR_ERR, "get all catalog failed: %s", samerrmsg);
		return (-1);
	}

	*ce_lst = lst_create();
	if (*ce_lst == NULL) {
		lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
		lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

		Trace(TR_ERR, "get all catalog failed: %s", samerrmsg);
		return (-1);
	}

	if (lib_lst->length == 0 && drv_lst->length == 0) {
		lst_free(lib_lst);
		lst_free(drv_lst);
		return (0);
	}

	if (CatalogInit("get_all_catalog") == -1) {
		lst_free(*ce_lst);
		lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
		lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));

		samerrno = SE_CATALOG_INIT_FAILED;
		/* sam error 2364 */
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get all catalog failed: %s", samerrmsg);
		return (-1);
	}

	for (i = 0; i < CatalogTable->CtNumofFiles; i++) {

		struct CatalogHdr *ch;
		ch = Catalogs[i].CmHdr;

		list = CatalogGetEntriesByLibrary(ch->ChEq, &n_entries);

		for (j = 0; j < n_entries; j++) {
			ptr = &list[j];
			if (ptr->CeStatus & CES_inuse) {

				ce = (struct CatalogEntry *)
				    mallocer(sizeof (struct CatalogEntry));
				if (ce == NULL) {

					free(list);
					lst_free_deep(*ce_lst);
					lst_free_deep_typed(drv_lst,
					    FREEFUNCCAST(free_drive));
					lst_free_deep_typed(lib_lst,
					    FREEFUNCCAST(free_library));

					Trace(TR_ERR, "get catalog failed: %s",
					    samerrmsg);
					return (-1);
				}
				memcpy(ce, ptr, sizeof (struct CatalogEntry));

				if (lst_append(*ce_lst, ce) != 0) {

					free(ce);
					free(list);
					lst_free_deep(*ce_lst);
					lst_free_deep_typed(drv_lst,
					    FREEFUNCCAST(free_drive));
					lst_free_deep_typed(lib_lst,
					    FREEFUNCCAST(free_library));

					Trace(TR_ERR, "get catalog failed: %s",
					    samerrmsg);
					return (-1);
				}
			}
		}
		free(list);
	}

	if (lib_lst != NULL) {
		lst_free_deep_typed(lib_lst, FREEFUNCCAST(free_library));
	}
	if (drv_lst != NULL) {
		lst_free_deep_typed(drv_lst, FREEFUNCCAST(free_drive));
	}
	Trace(TR_OPRMSG, "get all catalogs under SAM success");
	return (0);
}


/*
 * Sort a catalog entry list
 */
int
sort_catalog_list(
int start,			// IN - starting index
int size,			// IN - num of entries to return
vsn_sort_key_t sort_key,	// IN - sort key
boolean_t ascending,		// IN - ascending order
sqm_lst_t *catalog_list)		// IN/OUT - catalog entry list
{
	int (*comp) (void *, void *);

	Trace(TR_OPRMSG, "Start sorting catalog entry list");

	/*
	 * If the start is greater than the number of catalog entries
	 * in the catalog_list, free the contents and return empty list
	 */
	if (start > catalog_list->length) {
		while (catalog_list->head != NULL) {
			free(catalog_list->head->data);
			lst_remove(catalog_list, catalog_list->head);
		}
		return (0);
	}
	if (size == -1) { /* get all */
		size = catalog_list->length;
	}
	// prepare to sort the catalog list
	switch (sort_key) {
		case VSN_SORT_BY_VSN:
			if (ascending) {
				comp = vsn_compare_vsn_ascending;
			} else {
				comp = vsn_compare_vsn_descending;
			}

			break;

		case VSN_SORT_BY_FREESPACE:
			if (ascending) {
				comp = vsn_compare_freespace_ascending;
			} else {
				comp = vsn_compare_freespace_descending;
			}

			break;

		case VSN_SORT_BY_SLOT:
			if (ascending) {
				comp = vsn_compare_slot_ascending;
			} else {
				comp = vsn_compare_slot_descending;
			}

			break;

		default:
			/* VSN_NO_SORT */
			comp = NULL;
			break;
	}

	if (comp != NULL) {
		lst_sort(catalog_list, comp);
	}
	/* trim_list expects a positive number for start and count */
	start = (start == -1) ? 0 : start;
	trim_list(catalog_list, start, size);

	Trace(TR_OPRMSG, "finished sorting catalog entry list");
	return (0);
}


/*
 *	Compare two CatalogEntry structures by vsn in ascending order
 */
static int
vsn_compare_vsn_ascending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (strcmp(f1->CeVsn, f2->CeVsn));
}


/*
 *	Compare two CatalogEntry structures by vsn in descending order
 */
static int
vsn_compare_vsn_descending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (strcmp(f2->CeVsn, f1->CeVsn));
}


/*
 *	Compare two CatalogEntry structures by freespace in ascending order
 */
static int
vsn_compare_freespace_ascending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (f1->CeSpace - f2->CeSpace);
}


/*
 *	Compare two CatalogEntry structures by freespace in descending order
 */
static int
vsn_compare_freespace_descending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (f2->CeSpace - f1->CeSpace);
}


/*
 *	Compare two CatalogEntry structures by slot number in ascending order
 */
static int
vsn_compare_slot_ascending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (f1->CeSlot - f2->CeSlot);
}


/*
 *	Compare two CatalogEntry structures by slot number in descending order
 */
static int
vsn_compare_slot_descending(void *p1, void *p2)
{
	struct CatalogEntry *f1 = (struct CatalogEntry *)p1;
	struct CatalogEntry *f2 = (struct CatalogEntry *)p2;

	return (f2->CeSlot - f1->CeSlot);
}


/*
 *	support function get_all_libraries_from_MCF()
 *	get all the libraries in the MCF.
 */
int
get_all_libraries_from_MCF(
sqm_lst_t **lib_lst)	/* OUTPUT - list of library_t */
{
	node_t		*n, *lib_node, *drive_node;
	mcf_cfg_t	*mcf;
	sqm_lst_t		*drive_lst;
	library_t	*lib;
	drive_t		*drive;
	base_dev_t	*basedev;

	Trace(TR_MISC, "get libs from cfg file");

	if (ISNULL(lib_lst)) {
		Trace(TR_ERR, "get libs from cfg file failed: %s", samerrmsg);
		return (-1);
	}

	*lib_lst = lst_create();
	if (*lib_lst == NULL) {
		Trace(TR_ERR, "get libs from cfg file failed: %s", samerrmsg);
		return (-1);
	}
	if (read_mcf_cfg(&mcf) != 0) {

		lst_free(*lib_lst);

		Trace(TR_ERR, "get libs from cfg file failed: %s", samerrmsg);
		return (-1);
	}

	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {
		basedev = (base_dev_t *)n->data;

		if (basedev == NULL) {
			continue; /* ignore partial failures */
		}
		if (is_robots(basedev->equ_type) == 0) {

			lib = (library_t *)mallocer(sizeof (library_t));
			if (lib == NULL) {

				free_mcf_cfg(mcf);
				lst_free_deep_typed(*lib_lst,
				    FREEFUNCCAST(free_library));

				Trace(TR_ERR, "get libs from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}

			memset(lib, 0, sizeof (library_t));
			memcpy(lib, basedev, sizeof (base_dev_t));

			strlcpy(lib->catalog_path, basedev->additional_params,
			    sizeof (lib->catalog_path));

			lib->drive_list = lst_create();
			if (lib->drive_list == NULL) {

				free_library(lib);
				free_mcf_cfg(mcf);
				lst_free_deep_typed(*lib_lst,
				    FREEFUNCCAST(free_library));

				Trace(TR_ERR, "get lib from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}

			if (lst_append(*lib_lst, lib) != 0) {

				free_library(lib);
				free_mcf_cfg(mcf);
				lst_free_deep_typed(*lib_lst,
				    FREEFUNCCAST(free_library));

				Trace(TR_ERR, "get lib from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/* if library list is empty, no mapping to get drives of a library */
	if ((*lib_lst)->length == 0) {

		free_mcf_cfg(mcf);

		Trace(TR_MISC, "get libs from cfg file complete, no libs");
		return (0);
	}

	drive_lst = lst_create();
	if (drive_lst == NULL) {

		free_mcf_cfg(mcf);
		lst_free_deep_typed(*lib_lst, FREEFUNCCAST(free_library));

		Trace(TR_ERR, "get lib from cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* process the mcf structure to get a list of tape/optical drives */
	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {

		basedev = (base_dev_t *)n->data;
		if (basedev == NULL) {
			continue;
		}

		if (is_tape(sam_atomedia(basedev->equ_type)) ||
		    is_optical(sam_atomedia(basedev->equ_type))) {

			drive = (drive_t *)mallocer(sizeof (drive_t));
			if (drive == NULL) {

				lst_free_deep_typed(drive_lst,
				    FREEFUNCCAST(free_drive));
				free_mcf_cfg(mcf);
				lst_free_deep_typed(*lib_lst,
				    FREEFUNCCAST(free_library));

				Trace(TR_ERR, "get lib from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
			memset(drive, 0, sizeof (drive_t));
			memcpy(drive, basedev, sizeof (base_dev_t));

			if (lst_append(drive_lst, drive) != 0) {

				free_drive(drive);
				lst_free_deep_typed(drive_lst,
				    FREEFUNCCAST(free_drive));
				free_mcf_cfg(mcf);
				lst_free_deep_typed(*lib_lst,
				    FREEFUNCCAST(free_library));

				Trace(TR_ERR, "get lib from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	free_mcf_cfg(mcf);

	/* map drive to the hosting library by the family set name */
	for (lib_node = (*lib_lst)->head; lib_node != NULL;
	    lib_node = lib_node->next) {

		lib = (library_t *)lib_node->data;
		if (lib == NULL || lib->drive_list == NULL) {
			continue; /* ignore partial failures */
		}

		for (drive_node = drive_lst->head; drive_node != NULL;
		    drive_node = drive_node->next) {

			drive = (drive_t *)drive_node->data;
			if (drive == NULL) {
				continue;
			}

			if (strcmp(lib->base_info.set,
			    drive->base_info.set) == 0) {

				if (lst_append(lib->drive_list, drive) != 0) {

					lst_free_deep_typed(drive_lst,
					    FREEFUNCCAST(free_drive));
					lst_free_deep_typed(*lib_lst,
					    FREEFUNCCAST(free_library));

					Trace(TR_ERR, "get lib from cfg failed:"
					    " %s", samerrmsg);

					return (-1);
				}
				lib->no_of_drives++;
				drive_node->data = NULL;

			}
		}
	}

	lst_free_deep_typed(drive_lst, FREEFUNCCAST(free_drive));
	Trace(TR_MISC, "get all libraries from config complete");
	return (0);
}


/*
 * function to get available media types from the config file
 */
int
get_all_available_media_type_from_mcf(
sqm_lst_t **mtype_lst)	/* OUTPUT - list of mtype_t */
{
	node_t *n;
	mcf_cfg_t	*mcf;
	mtype_t		mtype;
	char 		*str;
	base_dev_t	*basedev;

	Trace(TR_OPRMSG, "get available mtype from config file");

	if (ISNULL(mtype_lst)) {
		Trace(TR_ERR, "get mtype from cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {

		Trace(TR_ERR, "get mtype from cfg failed: %s", samerrmsg);
		return (-1);
	}

	*mtype_lst = lst_create();
	if (*mtype_lst == NULL) {

		free_mcf_cfg(mcf);
		Trace(TR_ERR, "get mtype from cfg failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * iterate through the mcf configuration to get the media types
	 * corresponding to the library or drive entries, eliminate duplicates
	 */
	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {

		basedev = (base_dev_t *)n->data;
		if (basedev == NULL) {
			continue; /* ignore partial failures */
		}

		if (!is_robots(basedev->equ_type) &&
		    !is_tape(sam_atomedia(basedev->equ_type)) &&
		    !is_optical(sam_atomedia(basedev->equ_type))) {

			continue; /* not drive or library */
		}

		strlcpy(mtype, basedev->equ_type, sizeof (mtype_t));
		if (lst_search(*mtype_lst, mtype, (lstsrch_t)strcmp) == NULL) {

			str = copystr(mtype);
			if (str == NULL) {

				lst_free_deep(*mtype_lst);
				free_mcf_cfg(mcf);

				Trace(TR_ERR, "get mtype from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}

			if (lst_append(*mtype_lst, str) != 0) {

				free(str);
				lst_free_deep(*mtype_lst);
				free_mcf_cfg(mcf);

				Trace(TR_ERR, "get mtype from cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	free_mcf_cfg(mcf);

	Trace(TR_OPRMSG, "get available mtype from config file complete");
	return (0);
}


/*
 * function to get remote media type, given the library's equipment identifier
 */
int
get_rd_mediatype(
equ_t eq,	/* INPUT - eq number of the library */
char *mtype)	/* OUTPUT - mtype, minimum length should be sizeof(mtype_t ) */
{
	struct CatalogEntry *ce_array;
	struct CatalogEntry *ce;
	int n_entries;
	int i;

	Trace(TR_MISC, "get sam remote media type for library[%d]", eq);

	if (CatalogInit("get_rd_mediatype") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "get mtype failed: %s", samerrmsg);
		return (-1);
	}

	ce_array = CatalogGetEntriesByLibrary(eq, &n_entries);
	if (ce_array == NULL) {
		samerrno = SE_GET_CATALOG_ENTRY_ERROR;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq, "");

		Trace(TR_ERR, "get mtype failed: %s", samerrmsg);
		return (-1);
	}

	for (i = 0; i < n_entries; i++) {

		ce = &ce_array[i];

		if (ce->CeEq == eq) {
			strlcpy(mtype, ce->CeMtype, sizeof (mtype));
			break;
		}
	}

	free(ce_array);

	if (i == n_entries) {
		samerrno = SE_NO_TARGET_EQ_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);

		Trace(TR_ERR, "get mtype failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get sam remote mtype for library[%d] complete", eq);
	return (0);
}


/*
 *	check library dependency beforing removing a
 *	library.
 */
static int
check_lib_dependency(char *check_string) {
	char grep_cmd[MAXPATHLEN+1];
	char dep_mes[BUFSIZ];
	char grep_buf[BUFSIZ];
	char check_str[BUFSIZ];
	char temp_str[BUFSIZ];
	char out_temp_str[BUFSIZ];
	FILE *res_stream, *err_stream;
	int status;
	pid_t pid;
	int dep_flag = 0;
	char *delims = ":";
	char *p;
	char *str_ptr;
	char *rest;

	Trace(TR_OPRMSG, "checking library dependency for %s", check_string);

	snprintf(grep_cmd, sizeof (grep_cmd), "/usr/bin/grep %s %s %s %s %s",
	    check_string,
	    ARCHIVER_CFG,
	    STAGE_CFG,
	    RELEASE_CFG,
	    RECYCLE_CFG);

	Trace(TR_OPRMSG, "grepcmd %s\n", grep_cmd);
	pid = exec_get_output(grep_cmd, &res_stream, &err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		samerrno = SE_FORK_EXEC_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FORK_EXEC_FAILED),
		    grep_cmd);
		Trace(TR_PROC, "%s", samerrmsg);
		return (-1);
	}

	while (fgets(grep_buf, BUFSIZ, res_stream) != NULL) {

		/* Check if catalog_path is valid */
		strlcpy(check_str, grep_buf, sizeof (check_str));
		p = strtok_r(check_str, delims, &rest);
		while (p != NULL) {
			Trace(TR_OPRMSG, "token is %s\n", p);
			strlcpy(temp_str, p, sizeof (temp_str));
			p = strtok_r(NULL, delims, &rest);
		}
		str_ptr = str_trim(temp_str, out_temp_str);
		if (str_ptr == NULL) {
			samerrno = SE_STR_TRIM_CALL_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_OPRMSG, "%s", samerrmsg);
			return (-1);
		}

		if (out_temp_str[0] != '#') {
			dep_flag = 1;
			strlcpy(dep_mes, check_str, sizeof (dep_mes));
		}
	}

	fclose(res_stream);
	fclose(err_stream);

	if (dep_flag == 1) {
		samerrno = SE_REMOVE_LIBRARY_DEPENDENCY;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), dep_mes);
		return (-1);
	} else {
		Trace(TR_OPRMSG, "finished checking library dependency:"
		    " no dependency");
		return (0);
	}
}


/*
 *	support function get_all_standalone_drives_from_MCF()
 *	get all the standalone drives in the MCF.
 */
int
get_all_standalone_drives_from_MCF(
sqm_lst_t **drive_list)	/* a list of library_t, must be freed by caller */
{
	mcf_cfg_t *mcf;
	base_dev_t *dev;
	node_t  *node;
	drive_t *drv;

	Trace(TR_OPRMSG, "getting all standalone drives from MCF");
	*drive_list = NULL;
	if (ISNULL(drive_list)) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	Create a list here and it must be freed in
	 *	the calling function.
	 */
	*drive_list = lst_create();
	if (*drive_list == NULL) {
		Trace(TR_OPRMSG, "%s", samerrmsg);
		return (-1);
	}
	if (read_mcf_cfg(&mcf) != 0) {
		Trace(TR_OPRMSG, "Read of %s failed with error: %s",
		    mcf_file, samerrmsg);
		goto error;
	}

	/*
	 *	process mcf structure, if it is a robot
	 *	type, append that base_dev to library list.
	 */
	for (node = mcf->mcf_devs->head;
	    node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;
			if ((is_tape(sam_atomedia(dev->equ_type)) ||
			    is_optical(sam_atomedia(dev->equ_type))) &&
			    strcmp(dev->set, "-") == 0) {
				drv = (drive_t *)mallocer(sizeof (drive_t));
				if (drv == NULL) {
					Trace(TR_OPRMSG, "%s", samerrmsg);
					goto error;
				}
				memset(drv, 0, sizeof (drive_t));
				strlcpy(drv->base_info.name, dev->name,
				    sizeof (drv->base_info.name));
				strlcpy(drv->base_info.set, dev->set,
				    sizeof (drv->base_info.set));
				drv->base_info.eq = dev->eq;
				strlcpy(drv->base_info.equ_type,
				    dev->equ_type,
				    sizeof (drv->base_info.equ_type));
				strlcpy(drv->base_info.additional_params,
				    dev->additional_params,
				    sizeof (drv->base_info.additional_params));
				drv->base_info.state = dev->state;
				if (lst_append(*drive_list, drv) != 0) {
					free_drive(drv);
					Trace(TR_OPRMSG, "%s", samerrmsg);
					goto error;
				}
			}
		}
	}

	free_mcf_cfg(mcf);
	Trace(TR_OPRMSG, "finished getting all standalone drives from MCF");
	return (0);
error:
	free_mcf_cfg(mcf);
	free_list_of_drives(*drive_list);
	*drive_list = NULL;
	return (-1);
}
/*
 *	support function get_total_library_capacity().
 *	It can be used as API in the future.
 */
int
get_total_library_capacity(
int *lib_count,
int *total_slot,
fsize_t *free_space,	/* the library's free space based */
fsize_t *capacity)	/* all library's capacity for the node */
{
	struct CatalogEntry *list;
	int n_entries;
	int i;
	int nc;

	Trace(TR_MISC, "getting total capacity of library");

	*lib_count = 0;
	*total_slot = 0;
	*free_space = 0;
	*capacity = 0;
	if (CatalogInit("get_total_library_capacity") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;	/* sam error 2364 */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-2);
	}

	*lib_count += CatalogTable->CtNumofFiles;
	for (nc = 0; nc < CatalogTable->CtNumofFiles; nc++) {
		struct CatalogHdr *ch;
		ch = Catalogs[nc].CmHdr;
		list = CatalogGetEntriesByLibrary(ch->ChEq, &n_entries);
		*total_slot += n_entries;
		for (i = 0; i < n_entries; i++) {
			struct CatalogEntry *ce;
			ce = &list[i];
			*capacity += ce->CeCapacity;
			*free_space += ce->CeSpace;
		}
	}

	free(list);

	Trace(TR_MISC, "finished getting total capacity of library");
	return (0);
}


/*
 *	get the properties given a archive vsn pool name.
 */
int
get_properties_of_archive_vsnpool1(
ctx_t *ctx,				/* ARGSUSED */
const vsn_pool_t pool,		/* archive vsn pool */
int start,				/* IN - starting index in the list */
int size,				/* IN - num of entries, -1: all */
vsn_sort_key_t sort_key,		/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
vsnpool_property_t **vpp)	/* It must be freed by caller */
{

	sqm_lst_t *catalog_list	= NULL;
	node_t *node_c		= NULL;
	node_t *node_vsn	= NULL;
	struct CatalogEntry *ce = NULL;
	char *vsn_exp		= NULL;
	char *reg_ptr		= NULL;
	char *reg_rtn		= NULL;

	Trace(TR_MISC, "getting the property of archive vsn pool %s",
	    pool.pool_name);
	if (ISNULL(vpp)) {
		return (-1);
	}

	*vpp = (vsnpool_property_t *)mallocer(sizeof (vsnpool_property_t));
	if (*vpp == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	memset(*vpp, 0, sizeof (vsnpool_property_t));

	strlcpy((*vpp)->name, pool.pool_name, sizeof (uname_t));
	strlcpy((*vpp)->media_type, pool.media_type, sizeof (mtype_t));

	(*vpp)->catalog_entry_list = lst_create();
	if ((*vpp)->catalog_entry_list == NULL) {
		goto error;
	}

	/* get all catalog entries from SAM */
	if (get_all_catalog(&catalog_list) == -1) {
		if (samerrno == SE_CATALOG_INIT_FAILED) {
			return (0);
		} else {
			goto error;
		}
	}
	/*
	 * For each vsn pattern in pool, traverse through the
	 * list of catalog entries in SAM, copy the catalog entry
	 * to the list maintained by vsnpool_prop if a match is found
	 *
	 * to avoid duplicates, remove the catalog entry from the
	 * sam_catalog_list once it has been added to the vsnpool_prop
	 */
	node_vsn = (pool.vsn_names)->head;
	while (node_vsn != NULL) {
		vsn_exp = (char *)node_vsn->data;
		reg_ptr = regcmp(vsn_exp, NULL);
		if (reg_ptr == NULL) {
			samerrno = SE_GET_REGEXP_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_GET_REGEXP_FAILED), vsn_exp);
			goto error;
		}

		for (node_c = catalog_list->head; node_c != NULL;
		    node_c = node_c->next) {

			/*
			 * if node data is NULL, it has already been added
			 * this time, it is a duplicate
			 */
			if (node_c->data == NULL) {
				continue;
			}
			ce = (struct CatalogEntry *)node_c->data;
			reg_rtn = regex(reg_ptr, ce->CeVsn);

			/*
			 * catalogs are uniquely identified by the
			 * media type and the vsn pattern/name
			 */

			if ((strcmp(pool.media_type, ce->CeMtype) == 0) &&
			    reg_rtn != NULL) {

				if (lst_append(
				    (*vpp)->catalog_entry_list, ce) != 0) {
					goto error;
				}
				/* remove it from the catalog list */
				node_c->data = NULL;
			}
		}
		free(reg_ptr);
		node_vsn = node_vsn->next;
	}
	sort_catalog_list(
	    start, size, sort_key, ascending,
	    (*vpp)->catalog_entry_list);

	/*
	 * Based on the vsns making up the pool (according to the
	 * given criteria, i.e. start, size etc.), calculate the
	 * capacity and the free space
	 */
	node_c = (*vpp)->catalog_entry_list->head;
	while (node_c != NULL) {
		ce = (struct CatalogEntry *)node_c->data;

		(*vpp)->number_of_vsn ++;
		if (ce->r.CerTime == 0) {
			(*vpp)->free_space += ce->CeSpace;

		}
		(*vpp)->capacity += ce->CeCapacity;

		node_c = node_c->next;

	}
	/* free any remaining catalogs in the catalog_list */
	free_list_of_catalog_entries(catalog_list);
	Trace(TR_MISC, "finished getting the property of archive vsn pool %s",
	    pool.pool_name);
	return (0);
error:
	Trace(TR_ERR, "get property of archive vsn pool failed: %d[%s]",
	    samerrno, samerrmsg);

	if (*vpp != NULL) {
		free_vsnpool_property(*vpp);
		*vpp = NULL;
	}
	if (catalog_list != NULL) {
		free_list_of_catalog_entries(catalog_list);
		catalog_list = NULL;
	}
	return (-1);
}


/*
 *	given a vsn map definition, get a vsn pool property
 *	data and it has a sorted vsn list in the pool property.
 */
int
get_media_for_map(
ctx_t *ctx,				/* ARGSUSED */
vsn_map_t *map,
int start,				/* IN - starting index in the list */
int size,				/* IN - num of entries to return, */
					/*  -1: all remaining */
vsn_sort_key_t sort_key,		/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
vsnpool_property_t **vsnpool_prop)	/* vsn pool properties */
				/* must be freed by caller */
{
	sqm_lst_t *catalog_list;
	sqm_lst_t *pools;
	node_t *n;
	int ii;

	Trace(TR_MISC, "getting media_for_map %s", map->media_type);

	*vsnpool_prop = (vsnpool_property_t *)
		mallocer(sizeof (vsnpool_property_t));
	if (*vsnpool_prop == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		goto error;
	}

	memset(*vsnpool_prop, 0, sizeof (vsnpool_property_t));
	(*vsnpool_prop)->catalog_entry_list = lst_create();
	if ((*vsnpool_prop)->catalog_entry_list == NULL) {
		free_vsnpool_property(*vsnpool_prop);
		return (-1);
	}

	strlcpy((*vsnpool_prop)->name, map->ar_set_copy_name, sizeof (uname_t));
	strlcpy((*vsnpool_prop)->media_type, map->media_type, sizeof (mtype_t));


	/*
	 *	get all catalog list, then match the vsn
	 *	with the given vsn to generate requested
	 *	catalog entry list.
	 */
	ii =  get_catalog_entry_list_by_media_type(ctx, map->media_type,
		&catalog_list);
	if (ii == -1) {
		if (samerrno == SE_CATALOG_INIT_FAILED) {
			return (0);
		} else {
			Trace(TR_ERR, "%s", samerrmsg);
			return (-1);
		}
	}

	if (catalog_list -> length == 0) {
		return (0);
	}


	if (get_matching_media_vsns(map->vsn_names,
		catalog_list, *vsnpool_prop) != 0) {
		goto error;
	}

	/*
	 * if there are no pools included, sort the result and
	 * return success.
	 */
	if (map->vsn_pool_names == NULL || map->vsn_pool_names->length == 0) {

		/* set the count before sorting/trimming */
		(*vsnpool_prop)->number_of_vsn =
			(*vsnpool_prop)->catalog_entry_list->length;
		sort_catalog_list(
			start, size, sort_key, ascending,
			(*vsnpool_prop)->catalog_entry_list);

		free_list_of_catalog_entries(catalog_list);
		return (0);
	}

	if (get_all_vsn_pools(ctx, &pools) != 0) {
		goto error;
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

			if (get_matching_media_vsns(pool->vsn_names,
				catalog_list,
				*vsnpool_prop) != 0) {

				goto error;

			}
		}

	}

	/* set the count before sorting/trimming */
	(*vsnpool_prop)->number_of_vsn =
		(*vsnpool_prop)->catalog_entry_list->length;

	sort_catalog_list(
		start, size, sort_key, ascending,
		(*vsnpool_prop)->catalog_entry_list);

	free_vsn_pool_list(pools);
	free_list_of_catalog_entries(
		catalog_list);
	return (0);
error:
	free_list_of_catalog_entries(catalog_list);
	return (-1);
}


/*
 *	get_catalog_entry_list_by_media_type
 *	get a catalog entry list when the user knows its media type.
 */
int
get_catalog_entry_list_by_media_type(
ctx_t *ctx,			/* ARGSUSED */
const mtype_t media_type,	/* media type */
sqm_lst_t **catalog_entry_list)	/* It must be freed by caller */
{
	sqm_lst_t *catalog_list;
	node_t *node_c;
	struct CatalogEntry *cat_info;
	int ii;

	Trace(TR_MISC, "getting catalog entry by media type %s", media_type);
	*catalog_entry_list = NULL;
	if (ISNULL(catalog_entry_list)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	get all catalog list, then match the media type
	 *	with the given media type to generate requested
	 *	catalog entry list.
	 */
	ii =  get_all_catalog(&catalog_list);
	if (ii == -1) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	create a list here and it must be freed in
	 *	the calling function.
	 */
	*catalog_entry_list = lst_create();
	if (*catalog_entry_list == NULL) {
		free_list_of_catalog_entries(catalog_list);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	node_c = catalog_list->head;
	while (node_c != NULL) {
		cat_info = (struct CatalogEntry *)node_c->data;
		if (strcmp(cat_info->CeMtype, media_type) == 0) {
			if (lst_append(*catalog_entry_list, cat_info) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				goto error;
			}
			node_c->data = NULL;
		}
		node_c = node_c->next;
	}
	free_list_of_catalog_entries(catalog_list);

	Trace(TR_MISC, "finished getting catalog entry by media type %s",
	    media_type);
	return (0);
error:
	free_list_of_catalog_entries(catalog_list);
	free_list_of_catalog_entries(*catalog_entry_list);
	*catalog_entry_list = NULL;
	return (-1);
}


/*
 * vsnpool_properties must already be created
 * when this gets called.
 */
static int
get_matching_media_vsns(
sqm_lst_t *regexps,
sqm_lst_t *catalog_list,
vsnpool_property_t *vpp) {

	node_t *n;
	node_t *vsn_node;

	/*
	 * compare a list of regular expressions to a list of VSNs, create
	 * a list VSNs that match any of the regular expressions.
	 */
	for (n = regexps->head; n != NULL; n = n->next) {
		char *exp = (char *)n->data;
		char *cmp_exp = regcmp(exp, NULL);

		if (cmp_exp == NULL) {
			samerrno = SE_GET_REGEXP_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_GET_REGEXP_FAILED),
			    exp);
			Trace(TR_ERR, "%s", samerrmsg);
			goto err;
		}
		for (vsn_node = catalog_list->head; vsn_node != NULL; ) {
			struct CatalogEntry *ce =
			    (struct CatalogEntry *)vsn_node->data;
			node_t *tmp;
			char *reg_rtn;
			reg_rtn = regex(cmp_exp, ce->CeVsn);
			if (reg_rtn == NULL) {
				/* regexp did not match */
				vsn_node = vsn_node->next;
				continue;
			}


			/* add the catalog entry to the output */
			if (lst_append(vpp->catalog_entry_list, ce) != 0) {
				free(cmp_exp);
				goto err;
			}

			vpp->capacity += ce->CeCapacity;
			vpp->free_space += ce->CeSpace;
			vpp->number_of_vsn++;

			/*
			 * remove the catalog entry from the catalog entry list
			 * because we only want to see it once in result
			 */
			tmp = vsn_node;
			vsn_node = vsn_node->next;

			if (lst_remove(catalog_list, tmp) != 0) {
				free(cmp_exp);
				goto err;
			}

		}
		free(cmp_exp);
	}
	return (0);

err:
	return (-1);
}


/*
 * get a list of vsns already imported to this host.
 * sqm_lst_t **stk_vsn_names	a list of vsn names
 *				it must be freed by caller.
 */
int
get_stk_vsn_names(
ctx_t *ctx,			/* ARGSUSED */
devtype_t equ_type,
sqm_lst_t **stk_vsn_names)
{
	sqm_lst_t *catalog_list;
	node_t *node_c;
	struct CatalogEntry *cat_info;
	int ii;
	char *vsn_p;

	Trace(TR_MISC, "getting stk vsn_list by media type %s", equ_type);

	*stk_vsn_names = NULL;
	if (ISNULL(stk_vsn_names)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	get all catalog list, then match the media type
	 *	with the given media type to generate requested
	 *	vsn list.
	 */
	ii =  get_all_catalog(&catalog_list);
	if (ii == -1) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	/*
	 *	create a list here and it must be freed in
	 *	the calling function.
	 */
	*stk_vsn_names = lst_create();
	if (*stk_vsn_names == NULL) {
		free_list_of_catalog_entries(catalog_list);
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	node_c = catalog_list->head;
	while (node_c != NULL) {
		cat_info = (struct CatalogEntry *)node_c->data;
		if (strcmp(cat_info->CeMtype, equ_type) == 0) {
			vsn_p = (char *)
			    mallocer(strlen(
			    cat_info->CeVsn) + 1);
			if (vsn_p == NULL) {
				Trace(TR_OPRMSG, "%s", samerrmsg);
				goto error;
			}
			strlcpy(vsn_p,
			    cat_info->CeVsn,
			    sizeof (vsn_p));

			if (lst_append(*stk_vsn_names, vsn_p) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				goto error;
			}
		}
		node_c = node_c->next;
	}
	/*
	 *	If no match media type is found, an
	 *	empty list will be returned.
	 */
	free_list_of_catalog_entries(catalog_list);
	Trace(TR_MISC, "finished getting stk_vsn_names by media type %s",
	    equ_type);
	return (0);
error:
	free_list_of_catalog_entries(catalog_list);
	lst_free_deep(*stk_vsn_names);
	*stk_vsn_names = NULL;
	return (-1);
}


/*
 *	get the number of licensed slot in a library.
 *	Media_type cannot be mixed in a library.
 */
int
modify_stkdrive_share_status(
ctx_t *ctx,		/* ARGSUSED */
equ_t lib_eq,		/* eq number of the given library */
equ_t drive_eq,		/* eq number of the given drive */
boolean_t shared)	/* new value */
{
	node_t *node_p;
	int ii;
	library_t *print_lib;
	drive_t *print_p;
	int suc_flag = 0;
	upath_t drive_path;

	Trace(TR_MISC, "updating stk parameter file for library %d", lib_eq);

	ii =  get_library_by_equ(ctx, lib_eq, &print_lib);
	if (ii == -1) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	node_p = (print_lib->drive_list)->head;
	while (node_p != NULL) {
		print_p = (drive_t *)node_p ->data;
		if (print_p->base_info.eq == drive_eq) {
			strlcpy(drive_path, print_p->base_info.name,
			    sizeof (drive_path));
			update_stk_param(print_lib->storage_tek_parameter,
			    print_lib->base_info.name,
			    drive_path, shared);
			suc_flag = 1;
			break;
		}
		node_p = node_p->next;
	}
	free_library(print_lib);

	if (suc_flag == 0) {
		samerrno = SE_NO_STK_LIB_DRIVE_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NO_STK_LIB_DRIVE_FOUND));
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "finished updating stk parameter file for lib %d",
	    lib_eq);
	return (0);
}

/* definition from src/archiver/include/volume.h */
#define	RM_VOL_UNUSABLE_STATUS ( \
	CES_needs_audit | CES_cleaning | CES_dupvsn | \
	CES_unavail | CES_non_sam | CES_bad_media | \
	CES_read_only | CES_recycle | \
	CES_writeprotect | CES_archfull)

#define	KEY_VSN "vsn"

/*
 * get all the vsns in the SAM-FS configuration that
 * have the given status (one or more of the status
 * bit flags are set)
 * This function can be used to get a list of all vsns
 * that are unusable
 *
 * Input:
 *	equ_t	- eq of lib, if EQU_MAX, all lib
 *	uint32_t- status field bit flags
 *		- If 0, use default RM_VOL_UNUSABLE_STATUS
 *		(from src/archiver/include/volume.h)
 *		CES_needs_audit
 *		CES_cleaning
 *		CES_dupvsn
 *		CES_unavail
 *		CES_non_sam
 *		CES_bad_media
 *		CES_read_only
 *		CES_writeprotect
 *		CES_archfull
 *
 * Returns a list of formatted strings
 *	name=vsn
 *	type=mediatype
 *	flags=intValue representing flags that are set
 *
 */
int
get_vsns(
ctx_t *ctx,	/* ARGSUSED */
equ_t	eq,	/* input - equipement ordinal of lib */
uint32_t flags,	/* input - flags to be checked for */
sqm_lst_t **strlst)	/* return - list of formatted strings */
{
	uint32_t default_flags = RM_VOL_UNUSABLE_STATUS;
	char	buffer[BUFSIZ] = {0};
	sqm_lst_t *vsns = NULL;
	node_t *node = NULL;
	int ret = -1;

	if (ISNULL(strlst)) {
		Trace(TR_ERR, "get vsns failed: %s", samerrmsg);
		return (-1);
	}

	*strlst = lst_create();
	if (*strlst == NULL) {
		Trace(TR_ERR, "get vsns failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get all vsns with given status");

	if (eq == EQU_MAX) {
		/* get vsns from all libraries */
		ret = get_all_catalog(&vsns);
	} else {
		/* get vsns from specified library */
		ret = get_all_catalog_entries(
		    NULL,
		    eq,
		    0,
		    -1,
		    VSN_NO_SORT,
		    B_TRUE,
		    &vsns);
	}
	if (ret != 0) {
		Trace(TR_ERR, "get vsns failed: %s", samerrmsg);
		lst_free(*strlst);
		return (-1);
	}

	if (flags == 0) { /* use default */
		flags = default_flags;
	}
	node = vsns->head;
	while (node != NULL) {

		struct CatalogEntry *ce = NULL;
		ce = (struct CatalogEntry *)node->data;
		if (ce != NULL && ce->CeStatus & flags) {
			snprintf(buffer, sizeof (buffer),
			    "%s=%s,%s=%s,%s=%u",
			    KEY_TYPE, ce->CeMtype,
			    KEY_VSN, ce->CeVsn,
			    KEY_STATUS, ce->CeStatus);

			if (lst_append(*strlst, strdup(buffer)) != 0) {
				Trace(TR_ERR, "%s", samerrmsg);
				free_list_of_catalog_entries(vsns);
				lst_free_deep(*strlst);
				return (-1);
			}
		}
		node = node->next;
	}
	free_list_of_catalog_entries(vsns);
	Trace(TR_OPRMSG, "finished getting vsn status list");
	return (0);
}


/*
 * read the mcf file to get a list of paths that correspond to either drive or
 * library entry, these represent the libraries and drives that been added to
 * SAM
 */
int
get_paths_in_mcf(
int	type,	/* INPUT - objects whose paths are to be returned */
		/* e.g. PATH_LIBRARY, PATH_DRIVES, PATH_STANDALONE, PATH_FS */
sqm_lst_t **paths) /* OUTPUT - paths corresponding to lib/drive in mcf */
{

	node_t		*n;
	mcf_cfg_t	*mcf;
	base_dev_t	*dev;
	boolean_t	match = B_FALSE;

	Trace(TR_OPRMSG, "get paths[%d] from mcf", type);

	if (ISNULL(paths)) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	*paths = lst_create();
	if (*paths == NULL) {
		Trace(TR_ERR, "%s", samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {

		lst_free(*paths);
		Trace(TR_ERR, "Read mcf failed with error: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Iterate through the list of mcf entries, check the equipment type
	 * to check if the entry is associated with a drive/library/fs etc.
	 */
	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {

		dev = (base_dev_t *)n->data;
		if (dev == NULL) {
			continue;
		}

		/* convert the 2 letter mnemonic to bit-flag representation */
		int dtflag = sam_atomedia(dev->equ_type);

		switch (type) {

			case PATH_LIBRARY:
				if (is_robots(dev->equ_type) == 0) {
					match = B_TRUE;
				}
				/* LINTED E_CASE_FALLTHRU */
			case PATH_DRIVE:
				if (is_tape(dtflag) || is_optical(dtflag)) {
					match = B_TRUE;
				}
				break;

			case PATH_STANDALONE_DRIVE:
				if ((is_tape(dtflag) || is_optical(dtflag)) &&
				    strcmp(dev->set, "-") == 0) {

					/* family set name is - */
					match = B_TRUE;
				}
				break;
			/* default */
		}

		if (!match) {
			continue;
		}

		/* match found: save the device path in the list */

		char *str = copystr(dev->name);
		if (lst_append(*paths, str) != 0) {

			free(str);
			free_mcf_cfg(mcf);
			lst_free_deep(*paths);

			Trace(TR_ERR, "get path from mcf failed:%s", samerrmsg);
			return (-1);
		}
		match = B_FALSE; /* reset */
	}

	free_mcf_cfg(mcf);
	Trace(TR_OPRMSG, "get paths from mcf complete");
	return (0);
}


/* Structure to hold restrictions for filtering volumes */
typedef struct vol_fl_s {
	char			volname[33];
	char			mtype[5];
	int32_t			startslot;
	int32_t			endslot;
	int32_t			partition;
	off64_t			biggerthan;
	off64_t			smallerthan;
	int32_t			needs_audit;
	int32_t			in_use;
	int32_t			damaged;
	int32_t			cleaning;
	int32_t			write_protect;
	int32_t			read_only;
	int32_t			recycle;
	reserve_option_t	reserve;
	uint32_t		flags;
} vol_fl_t;

/* Definitions of restrictions flags for volumes */
#define	vfl_volname		0x00000001
#define	vfl_mtype		0x00000002
#define	vfl_startslot		0x00000004
#define	vfl_endslot		0x00000008
#define	vfl_partition		0x00000010
#define	vfl_smallerthan		0x00000020
#define	vfl_biggerthan		0x00000040
#define	vfl_needs_audit		0x00000080
#define	vfl_in_use		0x00000100
#define	vfl_damaged		0x00000200
#define	vfl_cleaning		0x00000400
#define	vfl_write_protect	0x00000800
#define	vfl_read_only		0x00001000
#define	vfl_recyle		0x00002000
#define	vfl_reserve		0x00004000

/* Macro to produce offset into structure */
#define	vfl_off(name) (offsetof(vol_fl_t, name))

static parsekv_t vfl_tokens[] = {
	{"volname",		vfl_off(volname),	parsekv_string_32},
	{"mtype",		vfl_off(mtype),		parsekv_string_5},
	{"startslot",		vfl_off(startslot),	parsekv_int},
	{"endslot",		vfl_off(endslot),	parsekv_int},
	{"partition",		vfl_off(partition),	parsekv_int},
	{"biggerthan",		vfl_off(biggerthan),	parsekv_ll},
	{"smallerthan",		vfl_off(smallerthan),	parsekv_ll},
	{"needs_audit",		vfl_off(needs_audit),	parsekv_int},
	{"in_use",		vfl_off(in_use),	parsekv_int},
	{"damaged",		vfl_off(damaged),	parsekv_int},
	{"cleaning",		vfl_off(cleaning),	parsekv_int},
	{"write_protect",	vfl_off(write_protect),	parsekv_int},
	{"read_only",		vfl_off(read_only),	parsekv_int},
	{"recycle",		vfl_off(recycle),	parsekv_int},
	{"reserve",		vfl_off(reserve),	parsekv_int},
	{"",			0,			NULL}
};

static int
set_vol_filter(char *restrictions, vol_fl_t *filterp)
{

	int flags = 0;

	if (ISNULL(filterp)) {
		return (-1);
	}
	memset(filterp, 0, sizeof (filterp));
	filterp->startslot	= -1;
	filterp->endslot	= -1;
	filterp->partition	= -1;

	if (ISNULL(restrictions)) {
		return (0);
	}

	if (parse_kv(restrictions, &vfl_tokens[0], filterp) != 0) {
		Trace(TR_ERR, "Parse volume filter failed: %s", samerrmsg);
		return (-1);
	}

	if (*(filterp->volname) != '\0') {
		flags |= vfl_volname;
	}

	if (*(filterp->mtype) != '\0') {
		flags |= vfl_mtype;
	}

	if (filterp->startslot != -1) {
		flags |= vfl_startslot;
	}

	if (filterp->endslot != -1) {
		flags |= vfl_endslot;
	}

	if (filterp->partition != -1) {
		flags |= vfl_partition;
	}
	filterp->flags = flags;

	return (0);
}

static int
check_vol_filter(struct CatalogEntry *ce, vol_fl_t *filterp)
{

	if (filterp->flags == 0) {
		return (0);	/* no filtering required */
	}

	/* the library slot has to be in use for any comparison to be valid */
	if (!(ce->CeStatus & CES_inuse)) {
		return (1);
	}

	if (filterp->flags & vfl_volname) {
		if (strcmp(ce->CeVsn, filterp->volname) != 0) {
			/* TBD: use regexp */
			return (1);
		}
	}

	if (filterp->flags & vfl_mtype) {
		if (strcmp(ce->CeMtype, filterp->mtype) != 0) {
			return (1);
		}
	}

	if (filterp->flags & vfl_startslot) {
		if (ce->CeSlot < filterp->startslot) {
			return (1);
		}
	}

	if (filterp->flags & vfl_endslot) {
		if (ce->CeSlot > filterp->endslot) {
			return (1);
		}
	}

	if (filterp->flags & vfl_partition) {
		if (ce->CePart != filterp->partition) {
			return (1);
		}
	}

	return (0);
}


static int
get_volumes_in_lib(int lib_eq, sqm_lst_t *vol_lst, vol_fl_t *filter) {

	struct CatalogEntry	*list;
	struct CatalogEntry	*ptr, *ce;
	int			n, i;

	/* lst would be created by the caller, append to the list */

	if (ISNULL(vol_lst)) {
		Trace(TR_ERR, "get volumes in library failed: %s", samerrmsg);
		return (-1);
	}

	list = CatalogGetEntriesByLibrary(lib_eq, &n);
	for (i = 0; i < n; i++) {
		ptr = &list[i];

		if (check_vol_filter(ptr, filter)) {
			continue;
		}
		ce = (struct CatalogEntry *)
		    mallocer(sizeof (struct CatalogEntry));
		if (ce == NULL) {
			free(list);

			Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
			return (-1);
		}
		memcpy(ce, ptr, sizeof (struct CatalogEntry));

		if (lst_append(vol_lst, ce) != 0) {
			free(ce);
			free(list);

			Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
			return (-1);
		}
	}
	free(list);

	return (0);
}


/*
 * list volumes. The volumes can be filtered by media type, identifiers,
 * position in the library, capacity, status or any combination of the above.
 * The restrictions are provided as key value pairs.
 */
int
list_volumes(ctx_t *ctx, int lib_eq, char *restrictions, sqm_lst_t **vol_lst)
{

	vol_fl_t		filter = {0};
	int			i;

	if (ISNULL(vol_lst)) {
		Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Request information from catserverd, if it is not runnin
	 * an error is generated.
	 */
	if (CatalogInit("list volumes") == -1) {
		samerrno = SE_CATALOG_INIT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
		return (-1);
	}

	if (set_vol_filter(restrictions, &filter) != 0) {
		Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
		return (-1);
	}

	*vol_lst = lst_create();
	if (*vol_lst == NULL) {
		Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * If library eq is input as -1, get the volumes from all the
	 * configured libraries.
	 * TBD: Should all the libraries by obtained from SAM to get
	 * get the equipment ordinal and then get the volumes in each
	 * library, or should all the catalogs be obtained from the
	 * CatalogTable?
	 */
	if (lib_eq == -1) {
		for (i = 0; i < CatalogTable->CtNumofFiles; i++) {
			struct CatalogHdr *ch;
			ch = Catalogs[i].CmHdr;
			if (get_volumes_in_lib(
			    lib_eq, *vol_lst, &filter) != 0) {
				Trace(TR_ERR, "list volumes failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	} else {
		if (get_volumes_in_lib(lib_eq, *vol_lst, &filter) != 0) {
			Trace(TR_ERR, "list volumes failed: %s", samerrmsg);
			return (-1);
		}
	}

	return (0);
}
