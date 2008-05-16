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
#ifndef	_MEDIA_H
#define	_MEDIA_H

#pragma	ident	"$Revision: 1.44 $"
/*
 *	File name:	media.h
 *	this file defines the removable media
 *	such as tape library's configuration.
 */

#include <sys/types.h>

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/archive.h"
#include "mgmt/util.h"

#include "sam/types.h"
#include "aml/catalog.h"

#define	PATH_LEN		256
#define	UNDEFINED_MEDIA_TYPE	"99"
#define	UNDEFINED_SAM_DT	99
#define	LIBRARY_DEVTYPE		8
#define	TAPE_DEVTYPE		1


/*
 *	define stk ACSLS parameter file keyword.
 */
#define	STK_HOSTNAME	"hostname"
#define	STK_PORTNUM	"portnum"
#define	STK_SSIHOST	"ssihost"
#define	STK_SSI_INET_PORTNUM	"ssi_inet_portnum"
#define	STK_CSI_HOSTPORT	"csi_hostport"
#define	STK_ACCESS	"access"
#define	STK_CAPID	"capid"
#define	STK_CAPACITY	"capacity"
#define	STK_SHARED	"shared"
#define	STK_ACS		"acs"
#define	STK_CAP		"cap"
#define	STK_LSM		"lsm"
#define	STK_PANEL	"panel"
#define	STK_DRIVE	"drive"


/*
 *	define SONY parameter file keyword.
 */
#define	SONY_USERID	"userid"
#define	SONY_SERVER	"server"
#define	SONY_DRIVE_NAME	"sonydrive"
#define	SONY_SHARED	"shared"


/*
 *	define IBM3494 parameter file keyword.
 */
#define	IBM3494_NAME		"name"
#define	IBM3494_ACCESS		"access"
#define	IBM3494_CATEGORY	"category"
#define	IBM3494_PRIVATE		"private"
#define	IBM3494_SHARE		"share"
#define	IBM3494_SHARED		"shared"


/*
 *	define ADIC/GRAU parameter file keyword.
 */
#define	ADIC_CLIENT	"client"
#define	ADIC_SERVER	"server"
#define	ADIC_DRIVE_NAME	"acidrive"


/*
 *	define FUJITSU parameter file keyword.
 */
#define	FUJITSU_DRIVE_NAME	"lmfdrive"

/*
 *	following struct will be defined in aml/catalog.h.
 *	After that, it will be removed from here.
 */
typedef struct reserve_option {
	int32_t CerTime;		/* Time reservation made */
	uname_t CerAsname;		/* Archive set		 */
	uname_t CerOwner;		/* Owner		 */
	uname_t	CerFsname;		/* File system		 */
} reserve_option_t;


typedef struct media_license {
	char	media_type[3];
	int	max_lic_slots; 	/* each media type has a slot license */
} media_license_t;

/*
 *	network attached library catalog build
 *	info. The first 4 fields are necessary.
 *	The detailed information see control API
 *	catalog.h.
 */

typedef struct catalog_info {
	int	slot_num;
	char 	*vsn;
	char 	*barcode;
	char 	media_type[3];
} catalog_info_t;


/*
 *	struct vsnpool_property is used to process the properties
 *	of a vsn pool.  A vsn pool may contain a list of vsn
 *	expression. Capacity and free_space are sizes in kilobytes.
 */
typedef struct vsnpool_property {
	upath_t		name;			/* vsn pool name.	    */
	int		number_of_vsn;		/* number of vsn.	    */
	fsize_t		capacity;		/* capacity of vsnpool.	    */
	fsize_t		free_space;		/* free space of vsnpool.   */
	sqm_lst_t 	*catalog_entry_list;	/* list of all vsn members. */
	mtype_t		media_type;		/* media type	*/
} vsnpool_property_t;


/*
 *	It is used only for StorageTek acsls's cap information.
 */
typedef struct stk_capinfo {
	int		acs_num;	/* ACS number for the drive as	    */
					/* configuration in the StorageTek  */
					/* library			    */
	int		lsm_num;	/* LSM number for the drive as	    */
					/* configuration in the StorageTek  */
					/* library			    */
	int		cap_num;	/* is the CAP number for this CAP   */
					/* as  configured in the StorageTek */
					/* library.			    */
	uname_t		status;
	int		priority;
	uname_t		state;
	uname_t		mode;
	int		size;
} stk_capinfo_t;

typedef struct acsls_drive_info {
	int		acs_num;
	int		lsm_num;
	int		panel_num;
	int		drive_num;
	char		type[64];
	char		state[64];
	char		serial_num[32];
} acsls_drive_info_t;


typedef struct acsls_lock {
	char		identifier[32];
	int		lock_id;
	long		duration;
	int		pending;
	char		status[128];
	char		user[128];
	char		type[32]; /* volume or drive lock */
} acsls_lock_t;

/*
 *	struct nw_param_pair is used to process all name and
 *	value pairs defined in all network attached library's
 *	parameter files.
 */
typedef struct nw_param_pair {
	uname_t	name;
	upath_t	value;
} nw_param_pair_t;


/*
 *	medias_type structure is used to get all
 *	inquiry.conf information.
 */
typedef struct medias_type {
	char	*vendor_id;
	char	*product_id;
	char	*sam_id;
} medias_type_t;



/*
 *	build_catalog()
 *	This will build catalog to the given catalog path
 *	based on incoming list of nwal_catalog.
 *
 *	char * catalog_path -			destination catalog path.
 *	char *media_type -			media type.
 *	char * lib_name -			library name.
 *	sqm_lst_t *nwal_catalog_list -	a list of nwal_catalog data.
 */
int build_catalog(char *catalog_path, char *media_type, char *lib_name,
	sqm_lst_t *nwal_catalog_list);


/*
 *	get the properties given a archive vsn pool name.
 *	const upath_t pool_name -	archive vsn pool name.
 *	int start -	IN - starting index in the list.
 *	int size -	IN - num of entries to return, -1: all remaining.
 *	vsn_sort_key_t sort_key -	IN - sort key.
 *	boolean_t ascending -	IN - ascending order.
 *	vsnpool_property_t **vsnpool_prop - returned vsnpool_property_t data
 *				data, it must be freed by caller.
 */
int get_properties_of_archive_vsnpool(ctx_t *ctx, const upath_t pool_name,
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	vsnpool_property_t **vsnpool_prop);

int get_properties_of_archive_vsnpool1(ctx_t *ctx, const vsn_pool_t arch_pool,
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	vsnpool_property_t **vsnpool_prop);

/*
 *	given a archive vsn pool name, check all vsns inside that
 *	vsn pool.  If a vsn's free space is not zero, add it
 *	to the list.
 *
 *	upath_t archive_vsn_pool_name -	archive vsn pool's name.
 *	int start -	IN - starting index in the list.
 *	int size -	IN - num of entries to return, -1: all remaining.
 *	vsn_sort_key_t sort_key -	IN - sort key.
 *	boolean_t ascending -	IN - ascending order.
 *	sqm_lst_t **catalog_entry_list -	a list of CatalogEntry,
 *						it must be freed by caller.
 */
int get_available_vsns(ctx_t *ctx, upath_t archive_vsn_pool_name,
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **catalog_entry_list);


/*
 *	get_remote_vsns
 *	Get a list of vsns that are used in the
 *	remote clients.
 *
 *	sqm_lst_t *remote_vsn_list - a list of vsn in the remote config file.
 *	const char *ss_identify - remote configuration file name in the MCF.
 */
int
get_remote_vsns(sqm_lst_t *remote_vsn_list,
	const char *ss_identify);


/*
 *	get catalog entries when the user only knows its vsn.
 *	It will be internally used.
 *
 *	const vsn_t vsn -	vsn name.
 *	sqm_lst_t **catalog_entry_list -	a list of CatalogEntry,
 *	must be freed by caller.
 */
int get_catalog_entry(ctx_t *ctx, const vsn_t vsn,
    sqm_lst_t **catalog_entry_list);


/*
 *	get a catalog entry when the user knows its vsn and media_type.
 *	It will be internally used.
 *
 *	vsn_t vsn -	vsn name.
 *	mtype_t type -	media type.
 *	struct CatalogEntry **catalog_entry -	returned CatalogEntry data,
 *						it must be freed by caller.
 */
int get_catalog_entry_by_media_type(ctx_t *ctx, vsn_t vsn,
	mtype_t type, struct CatalogEntry **catalog_entry);


/*
 *	get a catalog entry when the user knows the vsn and the equipment
 *	ordinal of the library from which it is coming.
 *	It will be internally used.
 *
 *	const equ_t library_eq -	eq number of the library.
 *	const int slot -		slot number of the catalog entry.
 *	const int partition -		partition.
 *	struct CatalogEntry **catalog_entry -	returned CatalogEntry data,
 *						it must be freed by caller.
 */
int get_catalog_entry_from_lib(ctx_t *ctx, const equ_t library_eq,
	const int slot, const int partition,
	struct CatalogEntry **catalog_entry);


/*
 *	get all the catalog entries of a library.
 *	It will be internally used.
 *
 *	const equ_t lib_eq -	eq number of the library.
 *	int start -	IN - starting index in the list.
 *	int size -	IN - num of entries to return, -1: all remaining.
 *	vsn_sort_key_t sort_key -	IN - sort key.
 *	boolean_t ascending -	IN - ascending order.
 *	sqm_lst_t **catalog_entry_list -	a list of CatalogEntry, it
 *						must be freed by caller.
 */
int get_all_catalog_entries(ctx_t *ctx, equ_t lib_eq,
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **catalog_entry_list);


/*
 *	Function to handle the large number of catalog entries in a catalog. In
 *	this function, the user can specify the starting slot number and ending
 *	slot number for which the entries are needed.
 *	It will be internally used.
 *
 *	const equ_t library_eq -	eq number of the library.
 *	int start -	IN - starting index in the list.
 *	int size -	IN - num of entries to return, -1: all remaining.
 *	vsn_sort_key_t sort_key -	IN - sort key.
 *	boolean_t ascending -	IN - ascending order.
 *	sqm_lst_t **catalog_entry_list -	a list of CatalogEntry, it
 *						must be freed by caller.
 */
int get_catalog_entries(ctx_t *ctx, equ_t library_eq,
	int start_slot_no, int end_slot_no,
	vsn_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **catalog_entry_list);

/*
 *	Given vsn's regular expression, get a list of
 *	all matched vsns.
 *
 *	const char *vsn_reg_exp -	given a vsn regular expression.
 *	int start -	IN - starting index in the list.
 *	int size -	IN - num of entries to return, -1: all remaining.
 *	vsn_sort_key_t sort_key -	IN - sort key.
 *	boolean_t ascending -	IN - ascending order.
 *	sqm_lst_t **catalog_entry_list -	a list of CatalogEntry, it
 *						must be freed by caller.
 */
int get_vsn_list(ctx_t *ctx, const char *vsn_reg_exp,
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t **catalog_entry_list);



/*
 *	Reserve a volume for archiving
 *
 *	const equ_t eq -		eq number.
 *	const int slot -		slot number.
 *	const int partition -		partition.
 *	const reserve_option_t *reserved_content -
 *		returned reserve_option_t data,
 *	it is not freed in this function.
 */
int rb_reserve_from_eq(ctx_t *ctx, const equ_t eq, const int slot,
	const int partition, const reserve_option_t *reserved_content);

/*
 * Read network-attached library parameter configuration file. The type of
 * network-attached library, path to the parameter file and buffer to output
 * the parameters are provided as input parameters
 */
int read_parameter_file(char *path, int dt, void **param);

/*
 *	Write stk parameter configuration file
 *	For future release.
 *
 *	stk_param_t ** stk_parameter -	given stk_param_t data.
 *	char *file_path -	stk parameter file path.
 */
int write_stk_param(stk_param_t *stk_parameter, char *file_path);

int update_stk_param(stk_param_t *stk_parameter, char *file_path,
	char *drive_path, boolean_t shared_state);

/*
 * Given a list of libraries find the library named set_name and return
 * a pointer to it. No new memory is allocated.
 *
 * sqm_lst_t *lib_list -	IN - list of libraries to search
 * char *set_name -		IN - name of library to search for
 * library_t **lib -		OUT - the library.
 */
int
find_library_by_family_set(sqm_lst_t *lib_list, const char *set_name,
	library_t **lib);

int
get_supported_media(sqm_lst_t **media_type_list);

int is_special_type(char *type);
int get_all_available_media_type_from_mcf(sqm_lst_t **lst);

int
get_all_libraries_from_MCF(sqm_lst_t **library_list);

void free_medias_type_list(sqm_lst_t *medias_type_list);
void free_medias_type(medias_type_t *medias_type_p);



/*
 *	support function get_total_library_capacity().
 */
int
get_total_library_capacity(
int *lib_count,
int *total_slot,
fsize_t *free_space,	/* Total library free space */
fsize_t *capacity);	/* Total library capacity from the node's catalog */

int
get_vsn_pool_properties(
ctx_t *ctx,				/* client connection */
vsn_pool_t *pool,			/* pool name and type */
int start,				/* starting index in the list */
int count,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* sort key */
boolean_t ascending,			/* ascending order */
vsnpool_property_t **vsnpool_prop	/* return - pool property */
);

int
get_vsn_map_properties(
ctx_t *ctx,				/* client connection */
vsn_map_t *map,				/* map name and type */
int start,				/* starting index in the list */
int count,		/* num of entries to return, -1: all remaining */
vsn_sort_key_t sort_key,		/* sort key */
boolean_t ascending,			/* ascending order */
vsnpool_property_t **vsnpool_prop	/* return - pool property */
);

/*
 *	given a vsn map definition, get a vsn pool property
 *	data and it has a sorted vsn list in the pool property.
 */
int
get_media_for_map(
	ctx_t *ctx,
	vsn_map_t *map,
	int start,
	int size,
	vsn_sort_key_t sort_key,
	boolean_t ascending,
	vsnpool_property_t **vsnpool_prop);


/*
 *	get_catalog_entry_list_by_media_type
 *	get a catalog entry list when the user knows its media type.
 */
int
get_catalog_entry_list_by_media_type(ctx_t *ctx, const mtype_t media_type,
	sqm_lst_t **catalog_entry_list);

/*
 *	remove stk's ACSLS parameter file given path.
 *	Actually we move the original file to a backup
 *	file.  If that backup exists, it will be overwritten.
 */
int remove_stk_param(char *file_path);

int discover_library(sqm_lst_t *mcf_paths, sqm_lst_t **lib_info_list);

int discover_unused_libraries(sqm_lst_t **library_list);

/*
 *	get_stk_only_media_info()
 *	This will do actual removable media discovery.
 *	sqm_lst_t *stk_host_info_list -- a list of ACSLS
 *	server information.
 *	sqm_lst_t **out_lib_info_list --- a list of library_t.
 *	sqm_lst_t *mcf_paths --- a list of mcf paths which
 *	will be excluded from the discovery.
 */
int get_stk_only_media_info(sqm_lst_t *stk_host_info_list,
sqm_lst_t **out_lib_info_list, sqm_lst_t *mcf_paths);

/*
 *	Get all catalogs in use.
 */
int get_all_catalog(sqm_lst_t **catalog_entry_list);

/*
 *	Next 3 free functions are for future release.
 */

void free_catalog_info(catalog_info_t *catalog_info_p);
void free_vsnpool_property(vsnpool_property_t *vsnpool_property);
/* to free list of vsnpool_property, use lst_free_deep_typed() */
void free_nw_param_pair_list(sqm_lst_t *namevalue_list);

/*
 *	function samid_to_dt() return device_type (integer type)
 *	based on samfs_id which can be gotten from inquiry.conf.
 *	UNDEFINED_SAM_DT (99) is rerurned if no match is found.
 */
int samid_to_dt(char *samid);

/*
 *	scsi_inq_pg80 - Unit serial number inquiry page 80.
 *	Given a robot or tape drive device data, get the
 *	unit serial number data.
 */
int scsi_inq_pg80(
	devdata_t *devdata,
	char *buf);

/*
 *	verify_library()
 *	given a library_path, we do discovery and then
 *	compare with mcf library.
 */
int
verify_library(library_t *mcf_lib);

/*
 *	verify_standalone_drive()
 *	given a standalone drive's path, we do discovery and then
 *	compare with mcf standalone drive.
 */
int
verify_standalone_drive(drive_t *mcf_standalone_drive);

/*
 *	get_vsn_num() will get the number in a vsn. It always is
 *	the number beginning from a number after a character to the
 *	end.  From example: abc099 --> 99; a9b7c9 --> 9; vsn909 --> 909.
 */
int
get_vsn_num(vsn_t begin_vsn, int *b_vsn_loc);

/*
 *	Given a vsn, vsn's character length, and the number need
 *	to be added to that vsn, generate a new vsn and this new vsn
 *	will be used for import.
 */
char *gen_new_vsn(
	vsn_t given_vsn,	/* given VSN */
	int char_len,		/* VSN string's character's length */
	int add_num,		/* the number need to be added to VSN */
	vsn_t new_vsn);

int discover_tape_drive(
	char *serial_no,
	sqm_lst_t *mcf_paths,
	sqm_lst_t **drive_lst);


int get_string_namevalue(
	sqm_lst_t **namevalue_list, char *str_buf);

int start_stk_daemon(stk_host_info_t *stk_host_info);
void set_stk_env(stk_host_info_t *stk_host_info);


int
sort_catalog_list(
	int start, int size, vsn_sort_key_t sort_key, boolean_t ascending,
	sqm_lst_t *catalog_list);
/*
 *	This function will register all ACSLS events.  RPC will spawn
 *	A thread for this function.  This function has a infinite loop until
 *	Final response or cancel request or interruption.
 */
int start_acsls_event();
int get_rd_mediatype(equ_t library_eq, char *media_type);

int get_libraries_from_shm(sqm_lst_t **lst);
int get_sdrives_from_shm(sqm_lst_t **lst);
int get_available_mtype_from_shm(sqm_lst_t **lst);
int get_sdrives_from_mcf(sqm_lst_t **drive_list);
int get_all_standalone_drives_from_MCF(sqm_lst_t **drive_list);

int stk_import_vsns(
	int portnum, char *fifo_file, long lpool, int vol_count, vsn_t vsn);
int is_ansi_tp_label(char *s, size_t size);


#endif /* _MEDIA_H */
