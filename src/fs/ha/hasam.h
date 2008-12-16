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

/*
 * hasam.h: Header file for SUNW.hasam RT implementation.
 */

#ifndef _HASAM_H
#define	_HASAM_H

#include <rgm/libdsdev.h>
#include "sam/mount.h"
#include "sam/lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Extension properties for HA-SAM defined in SUNW.hasam RT
 */
#define	QFSNAME				"QFSName"
#define	CATALOGFS			"CatalogFileSystem"
#define	ARIDLE_TIMER		"aridle_timer"
#define	ARSTOP_TIMER		"arstop_timer"
#define	ARRESTART_TIMER		"arrestart_timer"

#define	MAXLEN			1024
#define	MAX_UNITS		32
#define	WRONG_EQ		0

#define	GET_DAEMONS_CMD "/usr/proc/bin/ptree `/bin/pgrep sam-fsd`"
#define	PROCDIR			"/proc"
#define	HASAM_TMP_DIR	"/var/run"
#define	HASAM_RUN_FILE	HASAM_TMP_DIR"/hasam_running"

#define	streq(x, y)		(strcmp((x), (y)) == 0)

/*
 * Structure to extract details from resource group
 */
struct RgInfo_4hasam {
	scds_handle_t			rg_scdsh;
	scha_cluster_t			rg_cl;
	scha_resourcegroup_t	rg_rgh;
	char					*rg_nodename;
	const	char			*rg_name;
	scha_str_array_t		*rg_nodes;
	scha_str_array_t		*rg_servers;
	scha_str_array_t		*rg_mntpts;
	char					*rg_catalog;
};

/*
 * Structure to store information about filesystems used by HA-SAM
 */
struct FsInfo_4hasam {
	char	*fi_mntpt;	/* FS mount point */
	char	*fi_fs;		/* FS family set name */
	int		fi_pid;		/* pid of fork()ed child */
	int		fi_status;	/* exit status of child */
};

/*
 * Structure to check for essential daemons that must
 * be running for HA-SAM to work
 */
struct daemons_list {
	char	*daemon_name;
	int		found;
};

struct lib_state {
	int		l_eq;
	int		l_state;
};

/*
 * Structure for for all drives available to HA-SAM
 */
struct drive_info {
	char	drive_path[MAXLEN];
	char	drive_eqtype[MAX_UNITS];
	int		drive_eq;
};

/*
 * Sturcture for STK specific information
 */
struct stk_lib_info {
	char	lib_eqtype[MAX_UNITS];
	char	hostname[MAXLEN];
	char	conf_file[MAXLEN];
	int		acs_param;
	int		lsm_param;
	int		panel_param;
	int		drive_param;
};

/*
 * Structure for media information available to HA-SAM
 */
struct media_info {
	struct	stk_lib_info linfo;
	struct	drive_info dinfo[MAX_UNITS];
	char	catalog_path[MAX_UNITS][MAXLEN];
};

/*
 * Structure for disk archive information available to HA-SAM
 */
struct disk_archive_info {
	char	dskarch_vol[MAX_UNITS][MAXLEN];
	char	dskarch_host[MAX_UNITS][MAXLEN];
	char	dskarch_path[MAX_UNITS][MAXLEN];
};

/*
 * Structure to collect mount points available to cluster nodes
 */
typedef struct mp_list {
	char *mp_name;
	struct mp_list *next;
}mp_node_t;

mp_node_t *headnode_v, *headnode_m;


/*
 * Global variables used by HA-SAM
 */
int		num_lib;
int		num_drives;
int		num_cat;
int		num_fs;
int		num_disk_archives;
int		hasam_arrestart_timer;
int		hasam_arstop_timer;
int		hasam_aridle_timer;
int		hasam_stidle_timer;
char	catalogfs_mntpt[MAXLEN];
char	stagerfs_mntpt[MAXLEN];
boolean_t	tape_lib_drv_avail;

int			myreadbuf(int, void *, int);
int			sam_find_processes(struct daemons_list *);
boolean_t	verify_essential_sam_daemons(void);
boolean_t	is_sam_fsd(void);
boolean_t	check_sam_daemon(char *);
static boolean_t	find_string(char *, char *);
boolean_t	ping_host(char *);
int			run_cmd_4_hasam(char *);

int			mount_fs_4hasam(char *);
boolean_t	check_fs_mounted(char *);
char		*get_mntpt_from_fsname(char *);
boolean_t	check_all_fs_mounted(struct FsInfo_4hasam *);
boolean_t 	check_vfstab(char *, char *, char *);
int			get_fs_eq(char *);
char		*get_fs_mntpt(char *);
boolean_t	check_catalogfs(void);
boolean_t	check_stager_dir(void);
int			parse_vfstab(void);
int			parse_mnttab(void);
mp_node_t	*mp_alloc(void);
char		*search_dir(char *);
char		*search_link(char *);
int			get_all_mount_points(void);
void		free_all_mount_points(void);
int			search_mount_point(const char *);
boolean_t	make_run_file(void);
void		delete_run_file(void);

boolean_t	check_samshare(char *);
boolean_t	check_samfsinfo(char *);

boolean_t	samd_stop(void);
boolean_t	samd_start(void);
boolean_t	samd_config(void);
boolean_t	start_archiver(void);
boolean_t	stop_archiver(void);
boolean_t	stop_stager(void);
boolean_t	check_stager_conf(void);
void		samd_hastop(void);

boolean_t	get_drive_status(struct media_info *, boolean_t);
boolean_t	is_drive_on(int);
boolean_t	is_drive_off(int);
boolean_t	library_in_mcf(void);
struct media_info *collect_media_info(void);
void		free_media_info(struct media_info *);
void		print_media_info(struct media_info *);
struct disk_archive_info *collect_diskarchive_info(void);
boolean_t	check_catalog(struct media_info *);
boolean_t	check_catalog_dir(void);
char		*get_stk_hostname(struct media_info *);
char		*get_stk_filename(struct media_info *);
void		get_stk_params(struct media_info *);
boolean_t	make_drives_online(struct media_info *);
boolean_t	offline_tape_drives(struct media_info *);
boolean_t 	change_drive_state(int, dstate_t);
int			turn_drives_on_off(int, char *);
boolean_t	check_disk_archive_conf(void);
boolean_t	probe_hasam_config(void);
int			run_mt_cmd(char *, char *);
int			stopAccess2Tapes(void);
boolean_t	GetSamInfo(void);

/*
 * These functions are adopted from scqfs and modified to work
 * with HA-SAM
 */
int			GetRgInfo_4hasam(int, char **, struct RgInfo_4hasam *);
void		RelRgInfo_4hasam(struct RgInfo_4hasam *);
struct FsInfo_4hasam	*GetFileSystemInfo_4hasam(struct RgInfo_4hasam *);
void		FreeFileSystemInfo_4hasam(struct FsInfo_4hasam *);
int			ForkProcs_4hasam(int, char **, struct FsInfo_4hasam *,
				int (*)(int, char **, struct FsInfo_4hasam *));
int			hasam_mon_start(struct RgInfo_4hasam *);
int			hasam_mon_stop(struct RgInfo_4hasam *);

/*
 * To enable/disable debug information for HA-SAM
 */
extern int hasam_debug;
#define	dprintf	if (hasam_debug) (void) fprintf /* LINTED */

extern int samd_timer;
extern int drive_timer;

#ifdef __cplusplus
}
#endif

#endif /* _HASAM_H */
