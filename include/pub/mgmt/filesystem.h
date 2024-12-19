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
#ifndef _FILESYSTEM_H
#define	_FILESYSTEM_H

#pragma ident	"$Revision: 1.58 $"



/*
 * filesystem.h --  SAM-FS APIs for file system operations.
 */


#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/devstat.h"
#include "pub/mgmt/archive.h"
#include "pub/mgmt/device.h"

#define	MCF_DUMP_FILE	"mcf.dump"
#define	SAMFS_DUMP_FILE	"samfs.cmd.dump"


/*
 * structure to describe a striped group.
 */
typedef struct striped_group {

	devtype_t name;
	sqm_lst_t    *disk_list;

} striped_group_t;


/*
 * IO related mount options.
 */
typedef struct io_mount_options {

	int		dio_rd_consec;		/* number of operations */
	int		dio_rd_form_min;	/* kilobytes */
	int		dio_rd_ill_min;		/* kilobytes */
	int		dio_wr_consec;		/* number of operations */
	int		dio_wr_form_min;	/* kilobytes */
	int		dio_wr_ill_min;		/* kilobytes */
	boolean_t	forcedirectio;
	boolean_t    	sw_raid;		/* set true if sw raid used */
	int		flush_behind;		/* kilobytes */
	long long	readahead;		/* kilobytes */
	long long	writebehind;		/* kilobytes */
	long long	wr_throttle;		/* kilobytes */
	boolean_t	forcenfsasync;
	uint32_t	change_flag;
} io_mount_options_t;


/*
 * SAM-FS and SAM-QFS related mount options.
 */
typedef struct sam_mount_options {

	int16_t		high;		/* percent utilization */
	int16_t		low;		/* percent utilization */
	int		partial;	/* kilobytes multiple of 8 */
	int		maxpartial;	/* kilobytes multiple of 8 */
	uint32_t	partial_stage;	/* kilobytes multiple of 8 */
	uint32_t	stage_n_window;	/* kilobytes multiple of 8 */
	int		stage_retries;	/* number of retries 0 - 20 */
	int		stage_flush_behind; /* kilobytes multiple of 8 */
	boolean_t	hwm_archive;
	boolean_t	archive;
	boolean_t	arscan;
	boolean_t	oldarchive;

	uint32_t	change_flag;

} sam_mount_options_t;


/*
 * Shared QFS file system mount options.
 */
typedef struct sharedfs_mount_options {

	boolean_t	shared;
	boolean_t	bg;		/* mount in bg after initial failure */
	int16_t		retry;		/* number of times to retry mount */
	long long    	minallocsz;	/* kilobytes multiple of 8 */
	long long	maxallocsz;	/* kilobytes multiple of 8 */
	int		rdlease;	/* seconds */
	int		wrlease;	/* seconds */
	int		aplease;	/* seconds */
	boolean_t    	mh_write;
	int		nstreams;	/* number of streams */
	int		meta_timeo;	/* seconds */
	int		lease_timeo;	/* seconds */
	boolean_t	soft;		/* undocumented */
	uint32_t  change_flag;

} sharedfs_mount_options_t;


/*
 * multireader file system options.
 */
typedef struct multireader_mount_options {

	boolean_t	writer;
	boolean_t	reader;
	int		invalid;	/* seconds */
	boolean_t	refresh_at_eof;
	uint32_t	change_flag;

} multireader_mount_options_t;


/*
 * QFS mount options.
 */
typedef struct qfs_mount_options {

	boolean_t	qwrite;
	uint16_t	mm_stripe;	/* number of DAUs in metadata stripe */
					/* if mm_stripe = 0 metadata is */
					/* round robined */
	uint32_t	change_flag;

} qfs_mount_options_t;


/*
 * post 4.2 mount options structure.
 */
typedef struct post_4_2_options {
	uint32_t	change_flag;
	int		def_retention;	/* not documented */

	/* 4.4 options */
	boolean_t	abr;	/* enable app based recovery */
	boolean_t	dmr;	/* enable directed mirror reads */
	boolean_t	dio_szero;

	/* 4.5 Options */
	boolean_t	cattr;

} post_4_2_options_t;

/*
 * post 4.5 mount options structure.
 */
typedef struct rel_4_6_options {
	uint32_t	change_flag;
	boolean_t	worm_emul;
	boolean_t	worm_lite;
	boolean_t	emul_lite;
	boolean_t	cdevid;
	boolean_t	clustermgmt;
	boolean_t	clusterfastsw;
	boolean_t	noatime;
	int16_t		atime;
	int		min_pool;
} rel_4_6_options_t;

/*
 * post 4.6 mount options structure.
 */
typedef struct rel_5_0_options {
	uint32_t	change_flag;
	int16_t		obj_width;
	int64_t		obj_depth;
	int16_t		obj_pool;
	int16_t		obj_sync_data;
	boolean_t	logging;
	boolean_t	sam_db;
	boolean_t	xattr;
} rel_5_0_options_t;

/*
 * post 5.0 mount options structure.
 */
typedef struct rel_5_64_options {
	uint32_t	change_flag;
	boolean_t	casesense;	/* casesensitivity */
} rel_5_64_options_t;

/*
 * Mount options of a file system.
 */
typedef struct mount_options {

	/* General file system mount options */
	boolean_t	no_mnttab_entry;
	boolean_t	global;
	boolean_t	overlay;
	boolean_t	readonly;

	/* SAM-FS, QFS and SAM-QFS mount options */
	int16_t		sync_meta;	/* 0 = delayed write, 1 = no delay */
	boolean_t	no_suid;
	int16_t		stripe;		/* n = 0  round robin, otherwise */
					/* n * DAU bytes written to a LUN */
					/* before switching to next. */
	boolean_t	trace;
	boolean_t	quota;
	int		rd_ino_buf_size; /* bytes rounded down to power of 2 */
	int		wr_ino_buf_size; /* bytes rounded down to power of 2 */

	boolean_t worm_capable;		/* undocumented */
	boolean_t gfsid;		/* undocumented */

	/* Other mount options. */
	io_mount_options_t		io_opts;
	sam_mount_options_t		sam_opts;
	sharedfs_mount_options_t	sharedfs_opts;
	multireader_mount_options_t	multireader_opts;
	qfs_mount_options_t		qfs_opts;

	/*
	 * Flag to indicate what fields of the mount options
	 * the caller wants to change.
	 */
	uint32_t	change_flag;

	/* post 4.2 mount options */
	post_4_2_options_t	post_4_2_opts;

	/* 4.6 mount options */
	rel_4_6_options_t	rel_4_6_opts;

	/* 5.0 mount options */
	rel_5_0_options_t	rel_5_0_opts;

	/* 5.64 mount options */
	rel_5_64_options_t	rel_5_64_opts;

} mount_options_t;



typedef enum failed_mount_option {
	FAILED_MNT_SYNC_META	= 1,
	FAILED_MNT_NOSUID,
	FAILED_MNT_SUID,
	FAILED_MNT_TRACE,
	FAILED_MNT_NOTRACE,
	FAILED_MNT_STRIPE,

	FAILED_MNT_DIO_RD_CONSEC,
	FAILED_MNT_DIO_RD_FORM_MIN,
	FAILED_MNT_DIO_RD_ILL_MIN,
	FAILED_MNT_DIO_WR_CONSEC,
	FAILED_MNT_DIO_WR_FORM_MIN,
	FAILED_MNT_DIO_WR_ILL_MIN,
	FAILED_MNT_FORCEDIRECTIO,
	FAILED_MNT_NOFORCEDIRECTIO,
	FAILED_MNT_SW_RAID,
	FAILED_MNT_NOSW_RAID,
	FAILED_MNT_FLUSH_BEHIND,
	FAILED_MNT_READAHEAD,
	FAILED_MNT_WRITEBEHIND,
	FAILED_MNT_WR_THROTTLE,

	FAILED_MNT_HIGH,
	FAILED_MNT_LOW,
	FAILED_MNT_PARTIAL,
	FAILED_MNT_MAXPARTIAL,
	FAILED_MNT_PARTIAL_STAGE,
	FAILED_MNT_STAGE_N_WINDOW,
	FAILED_MNT_STAGE_RETRIES,
	FAILED_MNT_STAGE_FLUSH_BEHIND,
	FAILED_MNT_HWM_ARCHIVE,
	FAILED_MNT_NOHWM_ARCHIVE,

	FAILED_MNT_SHARED,
	FAILED_MNT_BG,
	FAILED_MNT_RETRY,
	FAILED_MNT_MINALLOCSZ,
	FAILED_MNT_MAXALLOCSZ,
	FAILED_MNT_RDLEASE,
	FAILED_MNT_WRLEASE,
	FAILED_MNT_APLEASE,
	FAILED_MNT_MH_WRITE,
	FAILED_MNT_NOMH_WRITE,
	FAILED_MNT_NSTREAMS,
	FAILED_MNT_META_TIMEO,

	FAILED_MNT_WRITER,
	FAILED_MNT_SHARED_WRITER,
	FAILED_MNT_READER,
	FAILED_MNT_SHARED_READER,
	FAILED_MNT_INVALID,

	FAILED_MNT_QWRITE,
	FAILED_MNT_NOQWRITE,
	FAILED_MNT_MM_STRIPE,

	/* newly added at Rel 4.5 */
	FAILED_MNT_REFRESH_AT_EOF,
	FAILED_MNT_NOREFRESH_AT_EOF,
	FAILED_MNT_LEASE_TIMEO,
	FAILED_MNT_ABR,
	FAILED_MNT_NOABR,
	FAILED_MNT_DMR,
	FAILED_MNT_NODMR,
	FAILED_MNT_DIO_SZERO,
	FAILED_MNT_NODIO_SZERO
} failed_mount_option_t;


/*
 * File system structure.
 *
 */

typedef struct fs {

	uname_t		fi_name;	/* file system name */
	int16_t		fs_count;	/* number of family set members */
	int16_t		mm_count;	/* number of meta set members */
	equ_t		fi_eq;		/* equipment number */
	devtype_t	equ_type;	/* device type: ms/ma/ufs */
	ushort_t	dau;		/* dau */
	time_t		ctime;		/* time when fs was created */
	dstate_t	fi_state;	/* on/off/readonly/idle */
	int		fi_version;	/* file system version */
	uint32_t	fi_status;	/* status flags */
	upath_t		fi_mnt_point;	/* full path to mount point */
	upath_t		fi_server;	/* shared fs server hostname */
	fsize_t		fi_capacity;	/* total bytes in filesystem */
	fsize_t		fi_space;	/* total free bytes in filesystem */
	boolean_t	fi_archiving;	/* true if the fs is a archiving fs */
	boolean_t	fi_shared_fs;	/* true if fs is shared */
	mount_options_t	*mount_options;
	sqm_lst_t	*meta_data_disk_list;
	sqm_lst_t	*data_disk_list;
	sqm_lst_t	*striped_group_list;

	/*
	 * 4.3 Addition to API
	 * list of host_info_t for shared file systems.
	 * This field is populated when the file system is retrieved from
	 * the metadata server or any potential metadata servers.
	 */
	sqm_lst_t	*hosts_config;

	/*
	 * 4.4 Addition to API
	 * char* nfs=yes|no|config
	 * Indication of whether this filesystem is NFS shared or not.
	 * An empty value here indicates the status is unknown.
	 */
	uname_t		nfs;

} fs_t;

/*
 * Used only as an input argument to file system creation to
 * configure archiving for the file system.
 * Note: log_path is a upath_t because it will get saved to the
 * archiver.cmd which is expecting a string no longer than upath_t.
 */
typedef struct fs_arch_cfg {
	uname_t set_name;
	upath_t log_path;
	sqm_lst_t *vsn_maps; /* vsn_map_t */
	sqm_lst_t *copies; /* ar_set_copy_cfg */
} fs_arch_cfg_t;


/*
 * samfsck process info structure.
 */
typedef struct samfsck_info {
	char	state;		/* process state */
	uname_t	user;		/* user name who started this process */
	pid_t	pid;		/* process id */
	pid_t	ppid;		/* parent process id */
	int	pri;		/* high value is high priority */
	size_t	size;		/* size of process image in Kbytes */
	time_t	stime;		/* process start time, from the epoch */
	time_t	time;		/* the cumulative execution time in seconds */
	char	cmd[255];	/* command with args */
	uname_t fsname;		/* fs name */
	boolean_t repair;   	/* repair flag (-F option) */
} samfsck_info_t;


/*
 * NOTE:
 *  The following get_fs_XXX functions are implemented to
 *  get fs information from live system. If it failed, then
 *  return the information from mcf with return code (-2).
 */

/*
 * DESCRIPTION:
 *   get_all_fs returns both the mounted and unmounted
 *   file systems
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of fs_t structures
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_all_fs(ctx_t *, sqm_lst_t **fs_list);


/*
 * DESCRIPTION:
 *   return the names of all configured filesystems
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of configured fs names
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_fs_names(ctx_t *, sqm_lst_t **fs_names);


/*
 * DESCRIPTION:
 *   return the filesystem names from vfstab
 * PARAMS:
 *   ctx_t *		  IN   - context object
 *   sqm_lst_t **	  OUT  - a list of fs names
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_fs_names_all_types(ctx_t *, sqm_lst_t **fs_names);


/*
 * DESCRIPTION:
 *   get the information about a specific file system
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   uname_t	IN   - fs name
 *   fs_t **	OUT  - filled fs_t structure
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_fs(ctx_t *, uname_t fsname, fs_t **fs);


/*
 * create_fs() is for creating a file system. This single method can
 * be used to create all types of file system.
 *
 * The type of the file system to be created can be derived from the
 * list of devices provided in fs data structure. Same thing for
 * mount options.
 *
 * The caller of this function can initialize the structures in
 * one of two ways.
 *
 * Each disk_t can contain only the au_t info. In this case
 * all eq_ordinals and device types will be selected by the api. Ordinals
 * will be the selected on a lowest available basis. Data disks will all
 * be md devices. Striped groups will take the lowest available striped
 * group id.
 *
 * Alternatively the base_info member of each disk can be set up to indicate
 * the equipment type and equipment ordinal of the device.
 *
 * If the ctx argument is non-null and contains a non-empty string dump_path
 * this function will not modify the existing mcf, samfs.cmd and vfstab files.
 * Instead it will write the mcf and samfs.cmd files to the path included in
 * the ctx.
 */
int create_fs(ctx_t *ctx, fs_t *fs_info, boolean_t mount_at_boot);


/*
 * change the mount options of a file system. This function
 * changes the mount options in the samfs.cmd and vfstab files.
 *
 * The caller of this function should set the change flag for any field
 * which they want written into the config files. Any field that does not
 * have its change_flag bit set will keep any existing setting in the
 * config files. This behavior is to support a live config that is different
 * from the written configuration files.
 *
 * This function sets all options(with change_flag bits) in the samfs.cmd
 * file. It only sets and unsets things in the vfstab file if a vfstab entry
 * exists for the filesystem and includes a mount option for which the input
 * options have their change flag set.
 *
 * If the ctx argument is non-null and contains a non-empty dump_path,
 * this function will not modify the existing samfs.cmd and vfstab files.
 * Instead it will write the modified samfs.cmd file to the path included in
 * the ctx.
 */
int change_mount_options(ctx_t *ctx, uname_t fsname,
	mount_options_t *options);


/*
 * This function changes mount options for a mounted filesystem.
 * Not all of the options are setable on a mounted filesystem. Any
 * which are not will be ignored.
 * failed_options is a list of type failed_mount_option_t.
 * If all mount options are set, success will be returned and
 * failed_options is a empty list.  If all mount options are not
 * set, error will be returned and failed_options includes all
 * failed options. If some mount options are not set, warning
 * will be returned and failed_options includes a list of failed
 * mount options.  All successful mount options will be recorded
 * in action log. All failed mount option will be recorded in
 * error log with detailed reasons.
 */
int change_live_mount_options(ctx_t *ctx, uname_t fsname,
	mount_options_t *options, sqm_lst_t **failed_options);


/*
 * get the default mount options for a file system of the type described.
 */
int get_default_mount_options(ctx_t *ctx, devtype_t fs_type, int dau_size,
	boolean_t uses_stripe_groups, boolean_t shared,
	boolean_t multi_reader, mount_options_t **params);


/*
 * Function to remove file systems from SAM-FS/QFS's control.
 * The named file system will have its devices removed from the mcf,
 * entries removed from vfstab, and mount options removed from samfs.cmd
 */
int remove_fs(ctx_t *ctx, uname_t fsname);


/*
 * Set the device state for the identified devices. Currently only the
 * DEV_NOALLOC and DEV_ON states are supported by this function.
 *
 * The eqs list is a list of the equipment ordinals of the devices to
 * set the state for. It is not considered an error if the device already
 * has its state set to new_state.
 */
int set_device_state(ctx_t *ctx, char *fs_name, dstate_t new_state,
    sqm_lst_t *eqs);


/*
 * File system operations.
 */

/*
 * DESCRIPTION:
 *   check consistency of a file system
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   uname_t	IN   - fs name
 *   upath_t	IN   - path to log file
 *   boolean_t	IN   - repair flag
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int samfsck_fs(ctx_t *, uname_t fsname, upath_t logfile, boolean_t repair);


/*
 * DESCRIPTION:
 *   get information about all running samfsck processes
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of samfsck_info_t strucutres
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_all_samfsck_info(ctx_t *, sqm_lst_t **fsck_info);


/*
 * Function to mount a file system.
 */
int mount_fs(ctx_t *ctx, upath_t name_or_mntpnt);


/*
 * Function to unmount a file system
 */
int umount_fs(ctx_t *ctx, upath_t name_or_mntpnt,
    boolean_t force); /* for future use. must be B_FALSE in 4.1 */


/*
 * Returns 0 if able to determine that fs is mounted or not.
 * mounted is set to B_TRUE if the fs is mounted.
 * mounted is set to B_FALSE if the fs is not mounted.
 *
 * Returns -1 if unable to make the determination.
 */
int is_fs_mounted(ctx_t *ctx, char *fs_name, boolean_t *mounted);


/*
 * Returns 0 if able to determine that filesystems are mounted or not.
 * mounted is set to B_TRUE if any filesystems are mounted.
 * mounted is set to B_FALSE if all filesytems are not mounted.
 *
 * Returns -1 if unable to make the determination.
 */
int is_any_fs_mounted(ctx_t *ctx, boolean_t *mounted);


/*
 * A file system can be grown only when it is unmounted and drives related
 * to archiving the file system are idle.
 *
 * If the ctx argument is non-null and contains a non-empty dump_path
 * this function will not modify the mcf file and will instead dump the
 * resultant mcf file to the specified dump_path.
 */
int grow_fs(ctx_t *ctx,
	fs_t			*fs,
	sqm_lst_t	*additional_meta_data_disk,
	sqm_lst_t	*additional_data_disk,
	sqm_lst_t	*additional_striped_group);



/*
 * Method to remove a device from a file system by releasing all of
 * the data on the device.
 *
 * kv_options is a string of key value pairs that are based on the
 * directives in shrink.cmd. If options is non-null the options
 * will be set for this file system in the shrink.cmd file
 * prior to invoking the shrink.
 *
 * Options Keys:
 *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *	   display_all_files = TRUE | FALSE (default FALSE)
 *	   do_not_execute = TRUE | FALSE (default FALSE)
 *	   logfile = filename (default no logging)
 *	   stage_files = TRUE | FALSE (default FALSE)
 *	   stage_partial = TRUE | FALSE (default FALSE)
 *	   streams = n  where 1 <= n <= 128 default 8
 */
int
shrink_release(ctx_t *c, char *fs_name, int eq_to_release, char *kv_options);

/*
 * Method to remove a device from a file system by copying the
 * data to other devices. If replacement_eq is the eq of a device
 * in the file system the data will be copied to that device.
 * If replacement_eq is -1 the data will be copied to available devices
 * in the FS.
 *
 * Options Keys:
 *	   logfile = filename (default no logging)
 *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *	   display_all_files = TRUE | FALSE (default FALSE)
 *	   do_not_execute = TRUE | FALSE (default FALSE)
 *	   logfile = filename (default no logging)
 *	   streams = n  where 1 <= n <= 128 default 8
 */
int
shrink_remove(ctx_t *c, char *fs_name, int eq_to_remove, int replacement_eq,
    char *kv_options);


/*
 * Method to remove a device from a file system by copying the
 * data to a newly added device.
 *
 * Options Keys:
 *	   logfile = filename (default no logging)
 *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *	   display_all_files = TRUE | FALSE (default FALSE)
 *	   do_not_execute = TRUE | FALSE (default FALSE)
 *	   logfile = filename (default no logging)
 *	   streams = n  where 1 <= n <= 128 default 8
 */
int
shrink_replace_device(ctx_t *c, char *fs_name, int eq_to_replace,
    disk_t *replacement, char *kv_options);


/*
 * Method to remove a striped group from a file system by copying the
 * data to a new striped group.
 *
 * Options Keys:
 *	   logfile = filename (default no logging)
 *	   block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
 *	   display_all_files = TRUE | FALSE (default FALSE)
 *	   do_not_execute = TRUE | FALSE (default FALSE)
 *	   logfile = filename (default no logging)
 *	   streams = n  where 1 <= n <= 128 default 8
 */
int
shrink_replace_group(ctx_t *c, char *fs_name, int eq_to_replace,
    striped_group_t *replacement, char *kv_options);


/*
 * Mount the shared file system on the named clients.
 *
 * A positive non-zero return indicates that a background job has been
 * started to complete this task. The return value is the job id.
 * Information can be obtained about the job by using the
 * list_activities function in process job with a filter on the job
 * id.
 */
int
mount_clients(ctx_t *ctx, char *fs_name, char *clients[], int client_count);


/*
 * Unmount the shared file system on the named clients.
 *
 * A positive non-zero return indicates that a background job has been
 * started to complete this task. The return value is the job id.
 * Information can be obtained about the job by using the
 * list_activities function in process job with a filter on the job
 * id.
 */
int
unmount_clients(ctx_t *ctx, char *fs_name, char *clients[],
    int client_count);


/*
 * Change the mount options for the named clients of the shared file system.
 *
 * A positive non-zero return indicates that a background job has been
 * started to complete this task. The return value is the job id.
 * Information can be obtained about the job by using the
 * list_activities function in process job with a filter on the job
 * id.
 */
int
change_shared_fs_mount_options(ctx_t *ctx, char *fs_name, char *clients[],
    int client_count, mount_options_t *mo);


/*
 * Method to get summary status for a shared file system
 * Status is returned as key value strings.
 *
 * The method potentially returns two strings. One for the
 * client status and one for the pmds status.
 *
 * The format is as follows:
 * clients=1024, unmounted=24, off=2, error=0
 * pmds = 8, unmounted=2, off=0, error=0
 */
int get_shared_fs_summary_status(ctx_t *c, char *fs_name, sqm_lst_t **lst);


/*
 * Deprecated. This function is only here to so that backwards compatible
 * libraries can be created. See create_arch_fs.
 */
int
create_fs_and_mount(ctx_t *ctx, fs_t *fs, boolean_t mount_at_boot,
    boolean_t create_mnt_point, boolean_t mount);


/*
 * This function will create the filesystem fs, conditionally setting
 * mount_at_boot in the /etc/vfstab. Also it will conditionally create
 * the mount point if it does not exist and mount the filesystem
 * after creation.
 *
 *
 * This function's return value indicates which step of this multistep
 * operation failed.
 *
 * The steps are as follows:
 * -1 initial checks failed.
 * 1. create the filesystem- this includes setting mount at boot.
 * 2. create the mount point directory.
 * 3. setup the archiver.cmd file.
 * 4. activate the archiver.cmd file.
 * 5. mount the filesystem.
 *
 * If warnings are detected in the archiver.cmd file this function continues on
 * and returns success if the subsequent steps are successful. If errors are
 * detected an error will be returned.
 */
int
create_arch_fs(ctx_t *ctx, fs_t *fs, boolean_t mount_at_boot,
    boolean_t create_mnt_point, boolean_t mount, fs_arch_cfg_t *arc_info);


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
ctx_t *ctx,		/* context for RPC */
int eqs_needed,		/* number of ordinals that are needed */
sqm_lst_t **in_use,	/* ordinals that are in use */
int **first_free);	/* The first free value */


/*
 * This function can be called to check if a list of equipment ordinals
 * are available for use. If any of the ordinals are already in use an
 * exception will be returned.
 */
int
check_equipment_ordinals(
ctx_t *ctx,		/* context for RPC */
sqm_lst_t *eqs);	/* list of equipment ordinals to check */


/*
 * reset equipment ordinals
 *
 * In a shared setup, while adding clients, it is possible that the eq
 * ordinals on the client are already in use. This function is called
 * to reset the equipment ordinals on all the MDS/Pot-MDS and SC.
 *
 */
int
reset_equipment_ordinals(
ctx_t *ctx,		/* context for RPC */
char *fsname,		/* filesystem name to which the eqs belong */
sqm_lst_t *eqs);	/* list of equipment ordinals to check */


// ------------------------  4.4 functions ---------------------------------


/*
 * DESCRIPTION:
 * get_generic_filesystems returns a list of key-value pairs
 * describing the file system
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   char *		IN   - filesystem type names
 *   sqm_lst_t **	OUT  - a list of strings
 *
 * format of key-value pairs:
 *  name=<name>,
 *  mountpt=<mntPt>,
 *  type=<ufs|zfs|...>
 *  state=mounted|umounted,
 *  capacity=<capacity>,
 *  availspace=<space available>,
 *  nfs=<yes|no>
 *
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int get_generic_filesystems(ctx_t *, char *filter, sqm_lst_t **fss);

/*
 * for SAM-FS, SAMQFS and QFS, type argument should be 'samfs'
 */
int mount_generic_fs(ctx_t *, char *fsname, char *type);
int remove_generic_fs(ctx_t *, char *fsname, char *type);

/*
 * Functions for getting/setting NFS server options
 */

/*
 * DESCRIPTION:
 * get_nfs_opts returns a list of key-value pairs
 * describing the NFS shared directories for a given
 * file system
 *
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   char *		IN   - file system mount point
 *   sqm_lst_t **	OUT  - a list of strings
 *
 * format of key-value pairs:
 *  dirname=<name>,		shared directory path
 *  nfs=<yes|config>		yes means actively NFS shared
 *				config means set in dfstab, but not active
 *  ro[=<accesslist>]		read-only, optional access list of
 *					colon-separated hosts or netgroups
 *  rw[=<accesslist>]		read-write, optional access list of
 *					colon-separated hosts or netgroups
 *  root=<accesslist>		root access allowed for list of
 *					colon-separated hosts or netgroups
 *  otheropts=<opts>		other NFS options in dfstab that we're
 *					not currently supporting in the GUI
 *  desc=<description>		optional description in dfstab (-d option)
 *  warning=<warning>		optional warning keyword if an error
 *					is detected.
 *
 * RETURNS:
 *   success  -  0
 *   error    -  -1
 */
int  get_nfs_opts(ctx_t *c, char *mnt_point, sqm_lst_t **opts);

/*
 * Function to set, modify or delete an NFS share
 */
int  set_nfs_opts(ctx_t *c, char *mnt_point, char *opts);

/*
 * Functions to stop sharing NFS directories for an entire
 * file system. nfs_unshare_all changes the active state
 * (/etc/dfs/sharetab) to unshared, and nfs_remove_all removes
 * entries from /etc/dfs/dfstab.
 */
int  nfs_unshare_all(ctx_t *c, char *mnt_point, sqm_lst_t **outdirs);
int  nfs_remove_all(ctx_t *c, char *mnt_point);
int  nfs_modify_active(sqm_lst_t *dirs, boolean_t share);

// --------------------------------------------------------------------------

mount_options_t *dup_mount_options(mount_options_t *src);
striped_group_t *dup_striped_group(striped_group_t *src);
fs_t *dup_fs(fs_t *src);


/*
 * Free methods
 */
void free_striped_group(striped_group_t *group);
void free_list_of_striped_groups(sqm_lst_t *l);
void free_fs(fs_t *fs);
void free_list_of_fs(sqm_lst_t *fs_list);
void free_list_of_samfsck_info(sqm_lst_t *info_list);


/* general mount options change flags */
#define	MNT_SYNC_META		0x00000001
#define	MNT_SUID		0x00000002
#define	MNT_NOSUID		0x00000004
#define	MNT_TRACE		0x00000008
#define	MNT_NOTRACE		0x00000010
#define	MNT_STRIPE		0x00000020
#define	MNT_READONLY		0x00000040
#define	MNT_QUOTA		0x00000080
#define	MNT_NOQUOTA		0x00000100
#define	MNT_RD_INO_BUF_SIZE	0x00000200
#define	MNT_WR_INO_BUF_SIZE	0x00000400
#define	MNT_WORM_CAPABLE	0x00000800
#define	MNT_GFSID		0x00001000
#define	MNT_NOGFSID		0x00002000

/*
 * clear flags are of the form ((FLAGA | FLAGB) << 16) | (FLAGA | FLAGB)
 * where FLAGA and FLAGB are both change flags that relate to the field
 * being cleared.
 */
#define	CLR_SYNC_META		0x00010001
#define	CLR_SUID		0x00060006
#define	CLR_NOSUID		0x00060006
#define	CLR_TRACE		0x00180018
#define	CLR_NOTRACE		0x00180018
#define	CLR_STRIPE		0x00200020
#define	CLR_READONLY		0x00400040
#define	CLR_QUOTA		0x01800180
#define	CLR_NOQUOTA		0x01800180
#define	CLR_RD_INO_BUF_SIZE	0x02000200
#define	CLR_WR_INO_BUF_SIZE	0x04000400
#define	CLR_WORM_CAPABLE	0x08000800
#define	CLR_GFSID		0x30003000
#define	CLR_NOGFSID		0x30003000

/* io change flags */
#define	MNT_DIO_RD_CONSEC	0x00000001
#define	MNT_DIO_RD_FORM_MIN	0x00000002
#define	MNT_DIO_RD_ILL_MIN	0x00000004
#define	MNT_DIO_WR_CONSEC	0x00000008
#define	MNT_DIO_WR_FORM_MIN	0x00000010
#define	MNT_DIO_WR_ILL_MIN	0x00000020
#define	MNT_FORCEDIRECTIO	0x00000040
#define	MNT_NOFORCEDIRECTIO	0x00000080
#define	MNT_SW_RAID		0x00000100
#define	MNT_NOSW_RAID		0x00000200
#define	MNT_FLUSH_BEHIND	0x00000400
#define	MNT_READAHEAD		0x00000800
#define	MNT_WRITEBEHIND		0x00001000
#define	MNT_WR_THROTTLE		0x00002000
#define	MNT_FORCENFSASYNC	0x00004000
#define	MNT_NOFORCENFSASYNC	0x00008000

#define	CLR_DIO_RD_CONSEC	0x00010001
#define	CLR_DIO_RD_FORM_MIN	0x00020002
#define	CLR_DIO_RD_ILL_MIN	0x00040004
#define	CLR_DIO_WR_CONSEC	0x00080008
#define	CLR_DIO_WR_FORM_MIN	0x00100010
#define	CLR_DIO_WR_ILL_MIN	0x00200020
#define	CLR_FORCEDIRECTIO	0x00C000C0
#define	CLR_NOFORCEDIRECTIO	0x00C000C0
#define	CLR_SW_RAID		0x03000300
#define	CLR_NOSW_RAID		0x03000300
#define	CLR_FLUSH_BEHIND	0x04000400
#define	CLR_READAHEAD		0x08000800
#define	CLR_WRITEBEHIND		0x10001000
#define	CLR_WR_THROTTLE		0x20002000
#define	CLR_FORCENFSASYNC	0xC000C000
#define	CLR_NOFORCENFSASYNC	0xC000C000

/* sam_mount_option_t change flags */
#define	MNT_HIGH		0x00000001
#define	MNT_LOW			0x00000002
#define	MNT_PARTIAL		0x00000004
#define	MNT_MAXPARTIAL		0x00000008
#define	MNT_PARTIAL_STAGE	0x00000010
#define	MNT_STAGE_N_WINDOW	0x00000020
#define	MNT_STAGE_RETRIES	0x00000040
#define	MNT_STAGE_FLUSH_BEHIND	0x00000080
#define	MNT_HWM_ARCHIVE		0x00000100
#define	MNT_NOHWM_ARCHIVE	0x00000200
#define	MNT_ARCHIVE		0x00000400
#define	MNT_NOARCHIVE		0x00000800
#define	MNT_ARSCAN		0x00001000
#define	MNT_NOARSCAN		0x00002000
#define	MNT_OLDARCHIVE		0x00004000
#define	MNT_NEWARCHIVE		0x00008000

#define	CLR_HIGH		0x00010001
#define	CLR_LOW			0x00020002
#define	CLR_PARTIAL		0x00040004
#define	CLR_MAXPARTIAL		0x00080008
#define	CLR_PARTIAL_STAGE	0x00100010
#define	CLR_STAGE_N_WINDOW	0x00200020
#define	CLR_STAGE_RETRIES	0x00400040
#define	CLR_STAGE_FLUSH_BEHIND	0x00800080
#define	CLR_HWM_ARCHIVE		0x03000300
#define	CLR_NOHWM_ARCHIVE	0x03000300
#define	CLR_ARCHIVE		0x0C000C00
#define	CLR_NOARCHIVE		0x0C000C00
#define	CLR_ARSCAN		0x30003000
#define	CLR_NOARSCAN		0x30003000
#define	CLR_OLDARCHIVE		0xC000C000
#define	CLR_NEWARCHIVE		0xC000C000

/* sharedfs_mount_option_t change flags */
#define	MNT_SHARED		0x00000001
#define	MNT_BG			0x00000002
#define	MNT_RETRY		0x00000004
#define	MNT_MINALLOCSZ		0x00000008
#define	MNT_MAXALLOCSZ		0x00000010
#define	MNT_RDLEASE		0x00000020
#define	MNT_WRLEASE		0x00000040
#define	MNT_APLEASE		0x00000080
#define	MNT_MH_WRITE		0x00000100
#define	MNT_NOMH_WRITE		0x00000200
#define	MNT_NSTREAMS		0x00000400
#define	MNT_META_TIMEO		0x00000800
#define	MNT_LEASE_TIMEO		0x00001000
#define	MNT_SOFT		0x00002000

#define	CLR_SHARED		0x00010001
#define	CLR_BG			0x00020002
#define	CLR_RETRY		0x00040004
#define	CLR_MINALLOCSZ		0x00080008
#define	CLR_MAXALLOCSZ		0x00100010
#define	CLR_RDLEASE		0x00200020
#define	CLR_WRLEASE		0x00400040
#define	CLR_APLEASE		0x00800080
#define	CLR_MH_WRITE		0x03000300
#define	CLR_NOMH_WRITE		0x03000300
#define	CLR_NSTREAMS		0x04000400
#define	CLR_META_TIMEO		0x08000800
#define	CLR_LEASE_TIMEO		0x10001000
#define	CLR_SOFT		0x20002000

/* multireader_mount_options_t change flags */
#define	MNT_WRITER		0x00000001
#define	MNT_SHARED_WRITER	0x00000002
#define	MNT_READER		0x00000004
#define	MNT_SHARED_READER	0x00000008
#define	MNT_INVALID		0x00000010
#define	MNT_REFRESH_AT_EOF	0x00000020
#define	MNT_NOREFRESH_AT_EOF	0x00000040

#define	CLR_WRITER		0x00010001
#define	CLR_SHARED_WRITER	0x00020002
#define	CLR_READER		0x00040004
#define	CLR_SHARED_READER	0x00080008
#define	CLR_INVALID		0x00100010
#define	CLR_REFRESH_AT_EOF	0x00600060
#define	CLR_NOREFRESH_AT_EOF	0x00600060

/* qfs_mount_options_t change flags */
#define	MNT_QWRITE		0x00000001
#define	MNT_NOQWRITE		0x00000002
#define	MNT_MM_STRIPE		0x00000004

#define	CLR_QWRITE		0x00010001
#define	CLR_NOQWRITE		0x00020002
#define	CLR_MM_STRIPE		0x00040004

/* post_4_2_options_t change flags */
#define	MNT_DEF_RETENTION	0x00000001
#define	MNT_ABR			0x00000002
#define	MNT_NOABR		0x00000004
#define	MNT_DMR			0x00000008
#define	MNT_NODMR		0x00000010
#define	MNT_DIO_SZERO		0x00000020
#define	MNT_NODIO_SZERO		0x00000040
#define	MNT_CATTR		0x00000080
#define	MNT_NOCATTR		0x00000100

#define	CLR_DEF_RETENTION	0x00010001
#define	CLR_ABR			0x00060006
#define	CLR_NOABR		0x00060006
#define	CLR_DMR			0x00180018
#define	CLR_NODMR		0x00180018
#define	CLR_DIO_SZERO		0x00600060
#define	CLR_NODIO_SZERO		0x00600060
#define	CLR_CATTR		0x01800180
#define	CLR_NOCATTR		0x01800180


/* rel_4_6_options_t change_flags */
#define	MNT_WORM_EMUL		0x00000001
#define	MNT_WORM_LITE		0x00000002
#define	MNT_WORM_EMUL_LITE	0x00000004
#define	MNT_CDEVID		0x00000008
#define	MNT_NOCDEVID		0x00000010
#define	MNT_CLMGMT		0x00000020
#define	MNT_NOCLMGMT		0x00000040
#define	MNT_CLFASTSW		0x00000080
#define	MNT_NOCLFASTSW		0x00000100
#define	MNT_NOATIME		0x00000200
#define	MNT_ATIME		0x00000400
#define	MNT_MIN_POOL		0x00000800

#define	CLR_WM_EMUL		0x00010001
#define	CLR_WM_LITE		0x00020002
#define	CLR_WM_EMUL_LITE	0x00040004
#define	CLR_CDEVID		0x00180018
#define	CLR_NOCDEVID		0x00180018
#define	CLR_CLMGMT		0x00600060
#define	CLR_NOCLMGMT		0x00600060
#define	CLR_CLFASTSW		0x01800180
#define	CLR_NOCLFASTSW		0x01800180
#define	CLR_NOATIME		0x02000200 /* Not a flag pair with atime */
#define	CLR_ATIME		0x04000400
#define	CLR_MIN_POOL		0x08000800

/* rel_5_0_options_t change_flags */
#define	MNT_OBJ_WIDTH		0x00000001
#define	MNT_OBJ_DEPTH		0x00000002
#define	MNT_OBJ_POOL		0x00000004
#define	MNT_OBJ_SYNC_DATA	0x00000008
#define	MNT_LOGGING		0x00000010
#define	MNT_NOLOGGING		0x00000020
#define	MNT_SAM_DB		0x00000040
#define	MNT_NOSAM_DB		0x00000080
#define	MNT_XATTR		0x00000100
#define	MNT_NOXATTR		0x00000200

#define	CLR_OBJ_WIDTH		0x00010001
#define	CLR_OBJ_DEPTH		0x00020002
#define	CLR_OBJ_POOL		0x00040004
#define	CLR_OBJ_SYNC_DATA	0x00080008
#define	CLR_LOGGING		0x00300030
#define	CLR_NOLOGGING		0x00300030
#define	CLR_SAM_DB		0x00C000C0
#define	CLR_NOSAM_DB		0x00C000C0
#define	CLR_XATTR		0x03000300
#define	CLR_NOXATTR		0x03000300

/* rel_5_64_options_t change_flags */
#define	MNT_CI			0x00000001	/* casesensitivity */
#define	MNT_NOCI		0x00000002

#endif	/* _FILESYSTEM_H */
