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
#ifndef _DEVICE_H
#define	_DEVICE_H

#pragma	ident	"$Revision: 1.55 $"

/*
 *	device.h --  SAM-FS APIs for device configuration and control
 */

#include "sam/types.h"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"

#include "pub/devstat.h"
#include "aml/types.h"

#define	DEV_STATUS_LEN	11
#define	FIRMWARE_LEN	5
#define	UNDEFINED_EQU_TYPE	"99"
#define	DISK_MEDIA	"dk"
#define	STK5800_MEDIA	"cb"

/*
 *	newly added.
 */
#define	PATH_LEN		256
#define	UNDEFINED_MEDIA_TYPE	"99"
#define	UNDEFINED_SAM_DT	99
#define	LIBRARY_DEVTYPE		8
#define	TAPE_DEVTYPE		1
#define	MAXIMUM_WWN		10
#define	WWN_LENGTH		128


/* from tapealert.c */
#define	CLEAN_NOW		0x080000
#define	CLEAN_PERIODIC		0x100000
#define	EXPIRED_CLEANING_MEDIA	0x200000
#define	INVALID_CLEANING_MEDIA	0x400000
#define	STK_CLEAN_REQUESTED	0x800000

#define	PATH_LIBRARY		0
#define	PATH_DRIVE		1
#define	PATH_STANDALONE_DRIVE	2

/*
 * The File System Manager server API provides a limited support for the
 * Fujitsu LMF Automated Tape Library, the ADIC/GRAU network-attached library,
 * the Sony network-attached library and, the IBM 3494 network-attached library.
 *
 * The user has to create the parameter file manually and provide the location
 * of the file as input while adding the library. The APIs parse the file for
 * the drive entries and add them to the mcf file. The API does not support
 * creating the parameter file for the above mentioned libraries.
 *
 */

/*
 * drive entry for every drive assigned to the client of the network-attached
 * library (Fujitsu, ADIC/GRAU, Sony, and IBM 3494)
 *
 * path		- Specifies the Solaris /dev/rmt/ path to the device
 * shared	-  (optional) indicates that the drive is shared
 *		shared is not applicable for Fujitsu and ADIC/GRAU libraries
 *
 */
typedef struct drive_param {
	char		path[MAXPATHLEN];
	boolean_t	shared;
} drive_param_t;


/*
 * parameter file to interface with Fujitsu, ADIC/GRAU, Sony, and IBM 3494
 * network-attached library
 *
 * server      	- hostname of the server hosting the nw lib software
 *		for IBM 3494, this is the symbolic name of the library
 * drive_lst	- one entry for every drive assigned to this client
 */
typedef struct nwlib_base_param {
	char	server[32];
	sqm_lst_t	*drive_lst; /* list of drive_param_t */
} nwlib_base_param_t;


/*
 *	It is used only for StorageTek parameter's capacity.
 *	a list of index value pair seprated by comma.
 *	capacity = ( 7 = 123, 9 = 234 ) value's unit is 1k.
 */
typedef struct stk_capacity {
	int		index;
	fsize_t		value;
} stk_capacity_t;


/*
 *	It is used only for StorageTek parameter's capid capid.
 *	Cartride access port to  be used  for exporting of
 *	volumes when the -f option is  used with export command.
 *	For example: capid = (acs=0, lsm=1, cap=0).
 */
typedef struct stk_cap {
	int		acs_num;	/* ACS number for the drive as	    */
					/* configuration in the StorageTek  */
					/* library			    */
	int		lsm_num;	/* LSM number for the drive as	    */
					/* configuration in the StorageTek  */
					/* library			    */
	int		cap_num;	/* is the CAP number for this CAP   */
					/* as  configured in the StorageTek */
					/* library.			    */
} stk_cap_t;


/*
 *	It is used only for StorageTek parameter's device
 */
typedef struct stk_device {
	upath_t		pathname;
	int		acs_num;	/* ACS number for the drive as	   */
					/* configuration in the StorageTek */
					/* library			   */
	int		lsm_num;	/* LSM number for the drive as	   */
					/* configuration in the StorageTek */
					/* library			   */
	int		panel_num;	/* PANEL number for the drive as   */
					/* configuration in the StorageTek */
					/* library			   */
	int		drive_num;	/* DRIVE number for the drive as   */
					/* configuration in the StorageTek */
					/* library			   */
	boolean_t	shared;
} stk_device_t;


/*
 *	It is used only for StorageTek network
 *	attached library parameter file.
 */
typedef struct stk_param {
	upath_t		param_path;	/* used by the stk daemon	*/
	uname_t		access;		/* null means no user_id given	*/
	uname_t		hostname;	/* hostname for server		*/
	int		portnum;	/* used for communication	*/
					/* between ACSLS		*/
					/* and sun sam-fs		*/
	stk_cap_t	stk_cap;
	sqm_lst_t	*stk_capacity_list;
	sqm_lst_t	*stk_device_list;
	uname_t		ssi_host;	/* hostname for ssi server	*/
	int		ssi_inet_portnum;
	int		csi_hostport;
} stk_param_t;


/*
 *	It is used only for StorageTek network
 *	attached library lsm.
 */
typedef struct stk_drive {
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	int		panel_num;	/* library panel number 	*/
	int		drive_num;	/* lib drive number in ACSLS 	*/
	uname_t		serial_no;	/* drive's serial number	*/
} stk_drive_t;

/*
 *	It is used only for StorageTek network
 *	attached library lsm.
 */
typedef struct stk_lsm {
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	uname_t		status;		/* lsm status			*/
	uname_t		state;		/* lsm state			*/
	int		free_cells;	/* how many cells are free	*/
	uname_t		serial_num;
} stk_lsm_t;


/*
 *	It is used only for StorageTek network
 *	attached library serial number.
 */
typedef struct stk_lsm_serial {
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	uname_t		serial_num;	/* lsm library serial number	*/
} stk_lsm_serial_t;

/*
 *	It is used only for StorageTek network
 *	attached library scratch pool.
 */
typedef struct stk_pool {
	int		pool_id;
	int		low_water_mark;		/* low water mark 	*/
	long		high_water_mark;	/* high water mark 	*/
	uname_t		over_flow;		/* over flow status	*/
} stk_pool_t;



/*
 *	It is used only for StorageTek network
 *	attached library panel.
 */
typedef struct stk_panel {
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	int		panel_num;	/* panel number			*/
	int		type;		/* panel type			*/
} stk_panel_t;

/*
 *	It is used only for StorageTek network
 *	attached library panel.
 */
typedef struct stk_volume {
	char		stk_vol[9];
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	int		panel_num;	/* panel number			*/
	int		row_id;
	int		col_id;
	int		pool_id;
	char		status[129];
	char		media_type[129];
	char		volume_type[129];	/* data, cleaning */
} stk_volume_t;


/*
 *	It is used only for StorageTek network
 *	attached library discovery.
 */
typedef struct stk_host_info {
	uname_t		hostname;		/* hostname	*/
	uname_t		portnum;		/* port number	*/
	uname_t		access;		/* null means no user_id given	*/
	uname_t		ssi_host;	/* hostname for ssi server	*/
	uname_t		ssi_inet_portnum;
	uname_t		csi_hostport;
} stk_host_info_t;


/*
 *	structure defining the core structure of any device; maps to the
 *	fields of mcf entries.
 */
typedef struct base_dev {
	upath_t		name;			/* device name (path)	*/
	equ_t		eq;			/* equipment number	*/
	devtype_t	equ_type;		/* equipment type	*/
	uname_t		set;			/* family set name	*/
	equ_t		fseq;			/* family set		*/
						/* equipment number	*/
	dstate_t	state;			/* device state		*/
						/* on/ro/idle/off/down	*/
	upath_t		additional_params;	/* additional		*/
						/* parameters file	*/
} base_dev_t;


/*
 *	different types of allocatable unit discovered.
 */
typedef enum AU_TYPE {
	AU_SLICE,			/* disk slice			 */
	AU_SVM,				/* Solaris Volume Manager volume */
	AU_VXVM,			/* Veritas Volume Manager volume */
	AU_ZVOL				/* ZFS zvols */
} au_type_t;

typedef unsigned long long	dsize_t; /* device size (see below) */


/*
 *	information gathered via SCSI inquiries. fields are NULL-terminated
 */
typedef struct {
	char vendor[9];			/* vendor identification	*/
	char prod_id[17];		/* product identification	*/
	char rev_level[5];		/* product revision level	*/
	char *dev_id;			/* device identification pg.83h */
} scsi_info_t;


/*
 *	allocatable unit that can be used for a file system data or metadata
 *	device.
 */
typedef struct au {

	upath_t		path;		/* such as /dev/dsk/cXtYdZsW,	*/
					/* /dev/vx/dsk/vol1		*/
	au_type_t	type;		/* type of AU			*/
	dsize_t		capacity;	/* capacity in bytes		*/
	uname_t		fsinfo;		/* file system information.	*/
					/* Based on the information 	*/
					/* found from mcf, /etc/vfstab, */
					/* logical volume managers SVM	*/
					/* and VxVm and currently 	*/
					/* mounted file systems.	*/
	/* since 4.3 */
	char		*raid;		/* raid level (eg: 0,0+1,5 )	*/
	scsi_info_t	*scsiinfo;	/* info gathered via SCSI inq.	*/
	/*
	 * when discovery returns a list of au-s, they are grouped by disk.
	 * only the first slice returned for a given disk will include the SCSI
	 * information for that disk. For all the others, this field is NULL.
	 */
} au_t;

/*
 *	structure for data or metadata device of a file system,.
 */
typedef struct disk {
	base_dev_t	base_info;
	au_t		au_info;
	fsize_t		freespace;
} disk_t;


/*
 *	supported networked-attached libraries
 */
typedef enum NETWORK_ATTACHED_LIBRARY_TYPE { STK_ACSLS, IBM3494, SONY_PETASITE,
	ADIC_DAS, FUJITSU_LMF, DIRECT_ATTACHED } nw_lib_type_t;

/*
 *	network-attached library information.
 *	The API client must provide this data.
 */
typedef struct nwlib_req_info {
	uname_t		nwlib_name;		/* library name		   */
	nw_lib_type_t	nw_lib_type;		/* library type		   */
	equ_t		eq;			/* needed eq number in MCF */
	upath_t		catalog_loc;		/* needed catalog location */
	upath_t		parameter_file_loc;	/* parameter file location */
} nwlib_req_info_t;

typedef struct scsi_2 {
	int	lun_id;
	int	target_id;
} scsi_2_t;

typedef enum DISCOVER_DEVICE_STATE { DEV_READY, DEV_BUSY, DEV_OTHER
	} discover_state_t;


/*
 *	structure defining a drive (either standalone or in a library).
 */
typedef struct drive {
	base_dev_t	base_info;			/* add/modify	   */
	char		serial_no[256];			/* serial number   */
	uname_t		library_name;			/* family set	   */
	char		dev_status[DEV_STATUS_LEN];	/* device status   */
	uname_t		vendor_id;			/* vendor name	   */
	uname_t		product_id;			/* product name	   */
	vsn_t		loaded_vsn;			/* loaded vsn name */
	sqm_lst_t	*alternate_paths_list;
	boolean_t	shared;				/* used in network */
							/* attached library */
	char		firmware_version[FIRMWARE_LEN];
	char		dis_mes[DIS_MES_TYPS][DIS_MES_LEN+1];	/* device */
								/* messages */
	/*
	 *	following fields are used only for discovery.
	 */
	discover_state_t	discover_state;
	int		scsi_version;
	upath_t		scsi_path;
	scsi_2_t	scsi_2_info;
	/*
	 *	Added in 4.4 to support wwn match in addition
	 *	serial number match.
	 */
	/*
	 *	Since it willbe an array, these two field should be obsolete
	 *	after 4.7 release.
	 */
	uname_t		wwn;				/* world wide name */
	int		id_type;
	uname_t		lib_serial_no;			/* serial number   */
	/*
	 *	Added in 4.5 to support wwn match in addition
	 *	serial number match.
	 */
	uname_t		wwn_lun;			/* world wide name */
	/*
	 *	Added in 4.6 to support multiple wwn match in addition
	 *	serial number match.  In i386 machine, a drive could have
	 *	multiple wwn names. 3 is most popular. we define 10 as maximum
	 *	number.  Also id type must be 3. wwn_id contains the world
	 *	wide name.
	 */
	char		wwn_id[MAXIMUM_WWN][WWN_LENGTH + 1];
	int		wwn_id_type[MAXIMUM_WWN];
	char		log_path[MAX_PATH_LENGTH];
	time_t		log_modtime;
	time_t		load_idletime;
	uint64_t	tapealert_flags; /* used to obtain cleaning status */
} drive_t;


/*
 *	media license structure
 */
typedef struct md_license {
	mtype_t 	media_type;
	int	max_licensed_slots;
	ushort_t robot_type;
} md_license_t;


/*
 *	structure to define a library
 */
typedef struct library {
	base_dev_t	base_info;			/* add/modify	    */
	uname_t		serial_no;			/* serial no	    */
	int		no_of_drives;			/* drives it has.   */
	sqm_lst_t	*drive_list;			/* add/modify	    */
	sqm_lst_t	*media_license_list;
	sqm_lst_t	*alternate_paths_list;
	char		dev_status[DEV_STATUS_LEN];	/* device status    */
	char		firmware_version[FIRMWARE_LEN];
	upath_t		catalog_path;			/* catalog location */
	uname_t		vendor_id;
	uname_t		product_id;
	char		dis_mes[DIS_MES_TYPS][DIS_MES_LEN+1];	/* device */
								/* messages */
	/*
	 *	following fields are used only for discovery.
	 */
	discover_state_t	discover_state;
	int		scsi_version;
	upath_t		scsi_path;
	/*
	 *	Added in 4.4 to support id_type chaeck.
	 */
	int		id_type;
	/*
	 *	Added in 4.5 to tell library type: direct attached
	 *	or network attached.
	 */
	stk_param_t 	*storage_tek_parameter;
	/*
	 *	Added in 4.5 to support wwn match in addition
	 *	serial number match.
	 */
	uname_t		wwn_lun;	/* world wide name */
	/* Added in 4.6 */
	char		log_path[MAX_PATH_LENGTH];
	time_t		log_modtime;
} library_t;

typedef char	barcode_t[BARCODE_LEN + 1];

/*
 *	Options used for importing cartridges to a library
 */
typedef struct import_option {
	vsn_t		vsn;		/* creates a catalog entry with	*/
					/* vsn as the barcode.		*/
					/* Network-attached ADIC/GRAU,	*/
					/* StorageTek, and IBM 3494	*/
					/* automated libraries only.	*/
					/* For the IBM 3494 library,	*/
					/* this option is accepted only	*/
					/* when running in shared mode.	*/
	barcode_t	barcode;	/* barcode assigned to the	*/
					/* cartridge.			*/
	mtype_t		mtype;		/* media type of the cartridge	*/
	boolean_t	audit;		/* all newly added cartridges	*/
					/* be audited			*/
	boolean_t	foreign_tape;	/* the media is unlabeled	*/
					/*  foreign tape		*/
	/*
	 *	For network-attached StorageTek automated libraries only.
	 *	Either the vsn or both vol_count and pool must be specified!!!
	 */
	int		vol_count;	/* number of volumes to be	*/
					/* taken from the scratch pool	*/
					/* specified by pool.		*/
	long		pool;		/* scratch pool from which num	*/
					/* volumes should be taken and	*/
					/* added to the catalog.	*/
} import_option_t;


/*
 *	supported import acsls vsns
 */
typedef enum IMPORT_ACSLS_VSN_FILTER { SCRATCH_POOL, VSN_RANGE, VSN_EXPRESSION,
	NONE } vsn_import_filter_t;
/*
 *	Options used for filetr vsns when importing from acsls library
 */
typedef struct vsn_filter_option {
	int scratch_pool_id;
	uname_t	start_vsn;
	uname_t	end_vsn;
	uname_t	vsn_expression;
	int	lsm;
	int	panel;
	int	start_row;
	int	end_row;
	int	start_col;
	int	end_col;
	uname_t	access_date;	/* yyyy-mm-dd or yyyy-mm-dd-yyyy-mm-dd */
	devtype_t	equ_type;		/* media type */
	vsn_import_filter_t	filter_type;
} vsn_filter_option_t;

/*
 *	stk volume cell used in import.
 */
typedef struct stk_cell {
	int min_row;
	int max_row;
	int min_column;
	int max_column;
} stk_cell_t;

/*
 *	It is used only for StorageTek network
 * This assimilates the physical configuration like lsm, panel, cell
 * and pool information for the stk.
 * there are individual api that provide each of these information
 * separately  but this structure is provided to get all the information
 * in a single call
 *
 */
typedef struct stk_phyconf_info {
	sqm_lst_t	*stk_pool_list;
	sqm_lst_t	*stk_panel_list; /* NOT USED */
	sqm_lst_t	*stk_lsm_list;	/* NOT USED */
	stk_cell_t	stk_cell_info; /* NOT USED */
} stk_phyconf_info_t;


typedef enum stk_info_type { LSM_INFO, PANEL_INFO, POOL_INFO,
	CAP_INFO, VOLUME_INFO, DRIVE_SERIAL_NUM_INFO,
	DRIVE_INFO, LSM_SERIAL_NUM_INFO, VOLUME_WITH_DATE_INFO,
	VOLUME_WITH_CLEANING }
stk_info_type_t;


typedef struct stk_cleaning {
	char		id[32];
	int		acs_num;
	int		lsm_num;	/* lib storage module number 	*/
	int		panel_num;	/* library panel number 	*/
	int		row;		/* row number 	*/
	int		column;		/* column number 	*/
	int		max_usage;	/* maximum usage 	*/
	int		current_usage;	/* maximum usage 	*/
	char		status[128];	/* maximum usage 	*/
	char		media_type[128];	/* type 	*/
	char		data_type[128];	/* type 	*/
} stk_cleaning_t;

/*
 *	Allocatable unit discovery functions.
 */

/*
 *	Get AU-s seen by this host.
 *	filter out those that are known to be in use
 *	(those that show up in the mcf, vfstab, mounted, or used by SVM/VxVM.
 *	available SVM/VxVM volumes are returned,
 *	but the slices used by them are not)
 */
int discover_avail_aus(ctx_t *ctx, sqm_lst_t **au_list);


/*
 *	Similar to the above but returns HA AU-s:
 *	1. /dev/did-s seens from all hosts
 *	2. volumes that are part of a metaset/diskgroup
 *	If avail is true, then return the available AU-s only
 */
int discover_ha_aus(ctx_t *ctx,
    sqm_lst_t *hosts, boolean_t avail, sqm_lst_t **au_list);


/*
 *	Get AU-s seen by this host.
 */
int discover_aus(ctx_t *ctx, sqm_lst_t **au_list);


/*
 *	Get AU-s seen by this host.
 *	select only those that have the specified type.
 *	filter out those that are known to be in use (see above).
 */
int discover_avail_aus_by_type(ctx_t *ctx, au_type_t type, sqm_lst_t **au_list);


/*
 *	result will include a subset of the input - slices that overlap
 *	with 1/more other slices.
 *	return code:
 *	 0 = no overlaps (result will be an empty list).
 *	-1 = an error occured
 *	>0 = the number of user-specified slices that overlap some other slices
 */
int check_slices_for_overlaps(ctx_t *ctx,
	sqm_lst_t *slices,	/* each slice is a /dev/dsk/... string */
	sqm_lst_t **result);	/* overlapping slices */


/*
 * Function to match devices from another host with devices on
 * this host. If matching devices are found, the device paths of the
 * input arguments will be overwritten with the path on the local host.
 *
 * disks is a list of disk_t structures representing the disks from another
 * host.
 *
 * aus is a list of aus on this host (returned by discover_aus).
 *
 * Returns an error if a matching device is not found.
 */
int find_local_device_paths(sqm_lst_t *disks, sqm_lst_t *aus);


/*
 *	Media functions
 */


/*
 *	Discover all the direct-attached libraries that sam can potentially
 *	control.
 *	sqm_lst_t **library_list -	a list of structure library_t
 */
int discover_media(ctx_t *ctx, sqm_lst_t **library_list);


/*
 *	API discover_media_unused_in_mcf()
 *	discover all the direct-attached libraries and tape drives
 *	that sam can potentially control and exclude those which have
 *	been already defined in MCF. If nothing is found, we return
 *	success with empty list.
 *	sqm_lst_t **library_list -	a list of structure library_t
 */
int discover_media_unused_in_mcf(ctx_t *ctx, sqm_lst_t **library_list);

/*
 *	discover_library_by_path()
 *	given a library_path, we do discovery.  It can be
 *	used for the second discovery after finding
 *	device busy during the first general discovery.
 *	library_t **discovered_lib -	discoverred library, it must be
 *					freed by caller
 *	upath_t library_path -		Given library path
 */
int discover_library_by_path(ctx_t *ctx, library_t **discovered_lib,
	upath_t library_path);


/*
 *	discover_standalone_drive_by_path()
 *	given a standalone_drive, we do discovery.  It can be
 *	used for the second discovery after finding
 *	device busy during the first general discovery.
 *	drive_t **discovered_drive -	discoverred drive, it must be
 *					freed by caller
 *	upath_t standalone_drive -	given drive path
 */
int discover_standalone_drive_by_path(ctx_t *ctx, drive_t **discovered_drive,
	upath_t standalone_drive);


/*
 *	Get all the libraries that sam is controlling.
 *	sqm_lst_t **library_list -	a list of structure library_t,
 *					must be freed by caller.
 */
int get_all_libraries(ctx_t *ctx, sqm_lst_t **library_list);


/*
 *	Get the library information when the user wants to provide only
 *	the library path.
 *	const upath_t library_path -	library's path.
 *	library_t **library -		a returned library_t structure,
 *					it must be freed by user.
 */
int get_library_by_path(ctx_t *ctx, const upath_t library_name,
	library_t **library);


/*
 *	Get the library information when the user wants to provide only
 *	the library family set.
 *	const uname_t library_family_set -	family set name.
 *	library_t **library -		a returned library_t structure,
 *					it must be freed by user.
 */
int get_library_by_family_set(ctx_t *ctx, const uname_t library_family_set,
	library_t **library);


/*
 *	Get the library information when the user wants to provide
 *	the library ordinal number.
 *	equ_t lib_equ -			equipment number of given library.
 *	library_t **library -		a returned library_t structure,
 *					it must be freed by user.
 */
int get_library_by_equ(ctx_t *ctx, equ_t library_eq,
	library_t **library);


/*
 *	Add a direct-attached library to sam's control.
 *	library_t *library -	the library which need to be added to MCF.
 */
int add_library(ctx_t *ctx, library_t *library);


/*
 *	Remove a library from sam's control.
 *	equ_t library_eq -		eq number of the library.
 *	boolean_t unload_first -	unload the library before removing it.
 */
int remove_library(ctx_t *ctx, equ_t library_eq, boolean_t unload_first);


/*
 *	Get all the standalone drives.
 *	sqm_lst_t **drv_list -	a list of drive_t structure,
 *					must be freed by user.
 */
int get_all_standalone_drives(ctx_t *ctx, sqm_lst_t **drive_list);


/*
 *	Get a standalone drive when its path is specified.
 *	const upath_t standalone_drive_path -	the drive's path.
 *	drive_t **standalone_drive -		returned standalone drive,
 *						it must be freed by the used.
 */
int get_standalone_drive_by_path(ctx_t *ctx, const upath_t drive_path,
	drive_t **drive);


/*
 *	Get a standalone drive when its equipment ordinal is specified.
 *	equ_t drive_equ -			drive's eq number.
 *	drive_t **standalone_drive -		returned standalone drive,
 *						it must be freed by the used.
 */
int get_standalone_drive_by_equ(ctx_t *ctx, equ_t drive_eq,
	drive_t **standalone_drive);


/*
 *	Add a standalone drive to sam's control.
 *	drive_t *standalone_drive -  the drive to be added to MCF.
 */
int add_standalone_drive(ctx_t *ctx, drive_t *drive);


/*
 *	Remove a standalone drive from sam's control.
 *	equ_t drive_eq - drive's eq number.
 */
int remove_standalone_drive(ctx_t *ctx, equ_t drive_eq);


/*
 *	Get the number of catalog entries in a library.
 *	equ_t lib_eq - eq number of the library.
 *	int *number_of_catalog_entres -	the number of catalog entries
 *					of given library.
 */
int get_no_of_catalog_entries(ctx_t *ctx, equ_t lib_eq,
	int *number_of_catalog_entries);

/*
 *	Audit slot in a robot.
 *	const equ_t eq -	device's equipment number.
 *	const int slot -	slot number.
 *	const int partition -	partition.
 *	boolean_t eod -		End of Device flag.
 */
int rb_auditslot_from_eq(ctx_t *ctx, const equ_t eq, const int slot,
	const int partition, boolean_t eod);

/*
 * Get the vsns for that have the given status
 * (one or more of the status bit flags are set)
 * This can be used to check for unusable vsns
 *
 * equ_t lib_eq		- equipment ordinal of library,
 *			if EQU_MAX, get vsns from all lib
 * uint32_t flags	- status field bit flags
 *			if 0, default RM_VOL_UNUSABLE_STATUS
 *			(from src/archiver/include/volume.h)
 *			CES_needs_audit
 *			CES_cleaning
 *			CES_dupvsn
 *			CES_unavail
 *			CES_non_sam
 *			CES_bad_media
 *			CES_read_only
 *			CES_writeprotect
 *			CES_archfull
 *
 * Returns a list of formatted strings
 *	name=vsn
 *	type=mediatype
 *	flags=intValue representing flags that are set
 *
 */
int get_vsns(ctx_t *ctx, const equ_t eq, uint32_t flags, sqm_lst_t **strlst);

/*
 *	Set or clear the flags for the given volume
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
 *
 *	const equ_t eq -	equipment number of device.
 *	const int slot -	slot number.
 *	boolean_t flag_set -	set or unset flag.
 *	uint32_t mask -		the mask to be set.
 */

int rb_chmed_flags_from_eq(ctx_t *ctx, const equ_t eq, const int slot,
	boolean_t set, uint32_t mask);


/*
 *	Clean drive in media changer.
 *	equ_t eq -	equipment number of the cleaning drive.
 */
int rb_clean_drive(ctx_t *ctx, equ_t eq);


/*
 *	Import cartridges into a library or the historian
 *
 *	equ_t eq  - equipment number of automated library or historian.
 *	const import_option_t *options -	import option,
 *						not freed by this function.
 */
int rb_import(ctx_t *ctx, equ_t eq, const import_option_t *options);


/*
 *	import all VSNs from the beginning vsn to the end vsn.
 *	All import used the same options.
 *	vsn_t begin_vsn -	beginning VSN.
 *	vsn_t end_vsn -		ending VSN.
 *	int *total_num -	the total imported vsn numbers.
 *	equ_t eq -		device equipment number.
 *	import_option_t *options -	import options.
 */
int import_all(ctx_t *ctx, vsn_t begin_vsn, vsn_t end_vsn,
	int *total_num, equ_t eq, import_option_t *options);

/*
 *	Export a cartridge from a robot.
 *	upath_t exp_loc is a new parameter.  It is used to
 *	determine export location.
 *	const equ_t eq -	device's equipment number.
 *	const int slot -	slot number which need to be exported.
 *	boolean_t STK_one_step -	one step export for STK library only.
 */
int rb_export_from_eq(ctx_t *ctx, const equ_t eq,
	const int slot, boolean_t STK_one_step);


/*
 *	Load media into a device
 *
 *	Parameters:
 *	equ_t eq -		equipment number of automated library.
 *	const int slot -	slot number.
 *	const int partition -	partition.
 *	boolean_t wait -	wait for the operation to complete.
 */
int rb_load_from_eq(ctx_t *ctx, const equ_t eq, const int slot,
	const int partition, boolean_t wait);


/*
 *	Unload media from a device
 *
 *	Parameters:
 *	equ_t eq		equipment number for the removable media
 *				device or a media changer
 *	boolean_t wait		wait for the operation to complete
 */
int rb_unload(ctx_t *ctx, equ_t eq, boolean_t wait);


/*
 *	Move a cartridge in a library
 *
 *	Parameters:
 *	const equ_t eq -	equipment number of the library.
 *	const int slot -	original slot number.
 *	int dest_slot -		destination slot number
 */
int rb_move_from_eq(ctx_t *ctx, const equ_t eq, const int slot, int dest_slot);


/*
 *	Label tape
 *
 *	Parameters:
 *	const equ_t eq -	equipment number.
 *	const int slot -	slot number.
 *	const int partition -	partition.
 *	vsn_t new_vsn -		new VSN of the tape being labeled (1-6 chars)
 *	vsn_t old_vsn -		old VSN if the media was previously labeled,
 *				empty if the media is not labeled (blank)
 *	uint_t blksize -	in units of 1024 must be 16, 32, 64,
 *				128, 256, 512, 1024, or 2048
 *	boolean_t wait -	wait for the labeling operation to complete
 *	boolean_t erase -	erase the media completely before a label is
 *				written. (will take a long time)
 */
int rb_tplabel_from_eq(ctx_t *ctx, const equ_t eq,
	const int slot, const int partition,
	vsn_t new_vsn, vsn_t old_vsn,
	uint_t blksize, boolean_t wait, boolean_t erase);


/*
 *	Unreserve a volume for archiving.
 *
 *	const equ_t eq -	equipment number.
 *	const int slot -	slot number.
 *	const int partition -	partition.
 *	const reserve_option_t *reserved_content - reserve options,
 *					it is not freed in this function.
 */
int rb_unreserve_from_eq(ctx_t *ctx, const equ_t eq,
	const int slot, const int partition);


/*
 *	Change to a new state.
 *	const equ_t eq -	equipment number.
 *	dstate_t new_state -	new state.
 */
int change_state(ctx_t *ctx, const equ_t eq, dstate_t new_state);


/*
 *	Check if a vsn is loaded.
 *	If it is loaded, returns that loaded drive.
 *	else empty drive will be returned.
 *	vsn_t vsn -	vsn name.
 *	drive_t **loaded_drive - returned drive, it may be empty
 *			if the vsn is not loaded. It must be freed by caller.
 */
int is_vsn_loaded(ctx_t *ctx, vsn_t vsn, drive_t **loaded_drive);

/* get_total_licensed_slot_of_library is obsolete */


/*
 *	Based on all catalog in that library,
 *	get the total capacity.
 *
 *	const equ_t eq -	eq number of the given library.
 *	fsize_t *capacity -	the library's capacity(KB) based on the catalog.
 *
 */
int get_total_capacity_of_library(ctx_t *ctx,
	const equ_t eq, fsize_t *capacity);


/*
 *	Based on all catalog in that library,
 *	get the free space.
 *	const equ_t eq -	eq number of the given library.
 *	fsize_t *free_space -	the library's free space (KB).
 */
int get_free_space_of_library(ctx_t *ctx, const equ_t eq, fsize_t *space);


typedef enum vsn_sort_key {
	VSN_SORT_BY_VSN = 0,
	VSN_SORT_BY_FREESPACE,
	VSN_SORT_BY_SLOT,
	VSN_NO_SORT,
	VSN_SORT_BY_PATH,
	VSN_SORT_BY_HOST
} vsn_sort_key_t;


/*
 *	Get a list of drives which are labeling tapes.
 *	sqm_lst_t **tape_label_running_list - a list of drive_t,
 *						it must be freed by caller.
 */
int get_tape_label_running_list(ctx_t *ctx,
    sqm_lst_t **tape_label_running_list);


/*
 *	Given network attached library's information: parameter file, eq
 *	number, catalog location, library name, network attached library type,
 *	get that network attached library's property.
 *	nwlib_req_info_t *nwlib_req_info -	given nwlib_req_info_t.
 *	library_t **nw_library -		returned library structure
 *     						data, it must be freed by
 *						caller.
 */
int get_nw_library(ctx_t *ctx, nwlib_req_info_t *nwlib_req_info,
	library_t **nw_lib);


/*
 *	get_all_available_media_type()
 *	This function check live system and return a list
 *	of media types.
 *	sqm_lst_t **media_type_list -	a list of media_type,
 *						it must be freed by caller.
 */
int get_all_available_media_type(ctx_t *ctx, sqm_lst_t **media_type_list);


/*
 *	Given a media type, get the media's default block size.
 *	mtype_t media_type -	media type.
 *	int *def_blocksize -	the default block size.
 */
int get_default_media_block_size(ctx_t *ctx, mtype_t media_type,
	int *def_blksize);

/*
 *	Get a list of stk acsls information which
 *	are available at the given host.
 *	sqm_lst_t **stk_info_list -	a list of followings:
 *	stk_volume_t, stk_panel_t, stk_pool_t, stk_cap_t,
 *	acsls_drive_info_t, stk_drive_info_t,
 *	stk_lsm_t, lsm_serial_num.
 *	it must be freed by caller.
 */
int get_stk_info_list(ctx_t *ctx, stk_host_info_t *stk_host_info,
	stk_info_type_t info_type, char *user_date, char *date_type,
	sqm_lst_t **stk_info_list);

/*
 *	Import stk acsls volume to SAMFS system
 *	vsn_list is a list of vsns chosen from GUI.
 */
int import_stk_vsns(ctx_t *ctx, equ_t eq, import_option_t *options,
	sqm_lst_t *vsn_list);

/*
 *	Add a list of libraries to SAMFS.
 */
int add_list_libraries(ctx_t *ctx, sqm_lst_t *library_list);


/*
 *	get a list of vsns after filterring some vsns.
 *	sqm_lst_t **stk_volume_list -	a list of stk_volume_t,
 *	it must be freed by caller.
 */
int get_stk_filter_volume_list(ctx_t *ctx, stk_host_info_t *stk_host_info,
	char *in_str, sqm_lst_t **stk_volume_list);


/*
 *	get stk physical configuration information, this is used
 *	to generate filter for VSN display
 *	The data got from this function is not complete.
 */
int get_stk_phyconf_info(ctx_t *ctx, stk_host_info_t *stk_host_info,
	devtype_t equ_type, stk_phyconf_info_t **stk_phyconf_info);


/*
 *	get a list of vsns already imported to this host.
 *	sqm_lst_t **stk_vsn_names -	a list of strings (vsn-names)
 *	it must be freed by caller.
 */
int get_stk_vsn_names(ctx_t *ctx, devtype_t equ_type,
	sqm_lst_t **stk_vsn_names);


/*
 *	Discover STK ACSLS only.
 *	It is used by GUI.
 *	stk_host_info_list is a list of structure stk_host_info_t.
 *	It includes hostname and port number.
 */
int discover_stk(ctx_t *ctx, sqm_lst_t *stk_host_info_list,
    sqm_lst_t **lib_list);

/*
 *	Update stk parameter file drive status.
 */
int modify_stkdrive_share_status(ctx_t *ctx, equ_t lib_eq,
	equ_t drive_eq, boolean_t shared);

int get_paths_in_mcf(int type, sqm_lst_t **lst);

disk_t *dup_disk(disk_t *dk);


/*
 *	free methods
 */
void free_au(au_t *au);
void free_au_list(sqm_lst_t *au_list);
void free_disk(disk_t *disk);
void free_drive(drive_t *drive);
void free_list_of_drives(sqm_lst_t *drive_list);
void free_library(library_t *library);
void free_list_of_libraries(sqm_lst_t *library_list);
void free_list_of_catalog_entries(sqm_lst_t *catalog_entry_list);

void free_nwlib_base_param(nwlib_base_param_t *param);
void free_stk_param(stk_param_t *stk_param);
void free_stk_phyconf_info(stk_phyconf_info_t *stk_phyconf_info);
#endif /* _DEVICE_H */
