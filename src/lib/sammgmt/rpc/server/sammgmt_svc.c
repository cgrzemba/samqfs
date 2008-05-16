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

#pragma ident	"$Revision: 1.112 $"

#include <stdio.h>
#include <stdlib.h> /* getenv, exit */
#include <strings.h>
#include <signal.h>
#include <unistd.h> /* getppid */
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <procfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/resource.h> /* rlimit */
#include <syslog.h>
#include <sys/stat.h>
#include "mgmt/sammgmt.h"
#include "sam/lib.h"
#include "mgmt/config/rpc_secure_cfg.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/license.h"

#define	USAGE		"Usage: "SBIN_DIR"/fsmgmtd [-v]"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* until the init function gets put into a header */
extern void init_utility_funcs(void);

static int discovery(void);		/* media discovery at startup */
static int check_proc(char *proc);	/* check for another instance */
static void *wait_child(void *arg);	/* remove defunc process */
static boolean_t is_secure_client(char *clientip);
static int clienthost2ip(char *clienthost, sqm_lst_t **lst_host2ip);


/*
 *	Program name.  Used elsewhere in fsmgmtd.  Lint complains about
 *	it being unused.
 */
#ifndef	lint
char *program_name = "fsmgmt";
#endif	/* !lint */
static boolean_t	debug_mode = FALSE;

/*
 * sammgmt_svc.c
 *
 * This file basically contains the main routine to register the server
 * program with rpcbind and the server dispatch function to take care of
 * validating client requests and invoking the appropriate service procedures.
 *
 * The server code is not MT safe. It uses unprotected global variables and
 * results in the form of static variables. Information cannot be passed back
 * from server to client through the parameter list, it must be passed as the
 * server program's return value.
 *
 * As TI-RPC allows a single parameter to be passed from client to server, if
 * the api requires more than one parameter, then the components are combined
 * into a structure that is counted as a single argument
 *
 * The rpc server is started by init(1M) using an entry in /etc/inittab
 * sfad:3:respawn:/opt/SUNWsamfs/sbin/fsmgmtd
 * This ensures that if the fsmgmtd process that should be running
 * is not, it should be respawned
 */

#ifdef DEBUG
#define	RPC_SVC_FG
#endif

#define	_RPCSVC_CLOSEDOWN 120
static int _rpcpmstart;		/* Started by a port monitor ? */

/* States a server can be in wrt request */

#define	_IDLE 0
#define	_SERVED 1

static int _rpcsvccount = 0;		/* Number of requests being serviced */

/* Global data */
int server_timestamp = 0;	/* ensure server talks to correct client */
bool_t timestamp_updated = B_FALSE; /* config has changed / svr clnt out sync */
samrpc_result_t rpc_result;	/* result sent from server to client */

/*
 * samrpc_init_1_svr
 *
 * In 4.6 this function is essentially deprecated. It previously
 * served to reinitialize the configuration file modification
 * time stamps. This behavior was removed for CR 6371323.
 * This function must be left in place until 4.5 is no longer
 * supported. The reason is that the 4.6 client must call this
 * function in order to work with 4.5 servers.
 */
int *
samrpc_init_1_svr(
	int *dummy, /* ARGSUSED */
	struct svc_req *req /* ARGSUSED */
)
{

	Trace(TR_DEBUG, "Initialize fsmgmt return[%d]", server_timestamp);

	return (&server_timestamp);
}

static void
_msgout(msg)
	char *msg;
{

	Trace(TR_ERR, msg);
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		sam_syslog(LOG_ERR, msg);
	else
		(void) fprintf(stderr, "%s\n", msg);
#else
	sam_syslog(LOG_ERR, msg);
#endif
}

void
sig_catcher(int signal)
{
	(void) fprintf(stderr, "Catching signal %d\n", signal);
	exit(0);
}

static void
closedown(sig)
	int sig; /* ARGSUSED */
{
	exit(0);
}

static void
sammgmtprog_1(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	struct netbuf *nb; struct sockaddr_in *in;

	nb = (struct netbuf *)malloc(sizeof (struct netbuf));
	(void) memset(nb, 0, sizeof (struct netbuf));

	nb = svc_getrpccaller(rqstp->rq_xprt);

	/* LINTED - alignment (32bit) OK */
	in = (struct sockaddr_in *)nb->buf;

	Trace(TR_DEBUG, "Request from client[%s]", inet_ntoa(in->sin_addr));
	union {

		/* mgmt.h */
		ctx_arg_t samrpc_get_samfs_version_1_arg;
		ctx_arg_t samrpc_get_samfs_lib_version_1_arg;
		ctx_arg_t samrpc_init_sam_mgmt_1_arg;
		proc_arg_t samrpc_destroy_process_1_arg;
		ctx_arg_t samrpc_get_server_info_3_arg;
		ctx_arg_t samrpc_get_server_capacities_4_arg;
		ctx_arg_t samrpc_get_system_info_4_arg;
		ctx_arg_t samrpc_get_logntrace_4_arg;
		string_arg_t samrpc_get_package_info_4_arg;
		ctx_arg_t samrpc_get_configuration_status_4_arg;
		ctx_arg_t samrpc_get_architecture_4_arg;
		ctx_arg_t samrpc_list_explorer_outputs_5_arg;
		int_string_arg_t samrpc_run_sam_explorer_5_arg;
		ctx_arg_t samrpc_get_sc_version_5_arg;
		ctx_arg_t samrpc_get_sc_name_5_arg;
		ctx_arg_t samrpc_get_sc_nodes_5_arg;
		ctx_arg_t samrpc_get_sc_ui_state_5_arg;
		ctx_arg_t samrpc_get_status_processes_6_arg;
		ctx_arg_t samrpc_get_component_status_summary_6_arg;

		/* archive.h - config APIs */
		ctx_arg_t samrpc_get_default_ar_set_copy_cfg_1_arg;
		ctx_arg_t samrpc_get_default_ar_set_criteria_1_arg;
		ctx_arg_t samrpc_get_ar_global_directive_1_arg;
		ar_global_directive_arg_t samrpc_set_ar_global_directive_1_arg;
		ctx_arg_t samrpc_get_default_ar_global_directive_1_arg;
		ctx_arg_t samrpc_get_ar_set_criteria_names_1_arg;
		string_arg_t samrpc_get_ar_set_1_arg;
		string_arg_t samrpc_get_ar_set_criteria_list_1_arg;
		ctx_arg_t samrpc_get_all_ar_set_criteria_1_arg;
		string_arg_t samrpc_get_ar_fs_directive_1_arg;
		ctx_arg_t samrpc_get_all_ar_fs_directives_1_arg;
		ar_fs_directive_arg_t samrpc_set_ar_fs_directive_1_arg;
		string_arg_t samrpc_reset_ar_fs_directive_1_arg;
		ar_set_criteria_arg_t samrpc_modify_ar_set_criteria_1_arg;
		ctx_arg_t samrpc_get_default_ar_fs_directive_1_arg;
		ctx_arg_t samrpc_get_all_ar_set_copy_params_1_arg;
		ctx_arg_t samrpc_get_ar_set_copy_params_names_1_arg;
		string_arg_t samrpc_get_ar_set_copy_params_1_arg;
		string_arg_t samrpc_get_ar_set_copy_params_for_ar_set_1_arg;
		ar_set_copy_params_arg_t samrpc_set_ar_set_copy_params_1_arg;
		string_arg_t samrpc_reset_ar_set_copy_params_1_arg;
		ctx_arg_t samrpc_get_default_ar_set_copy_params_1_arg;
		ctx_arg_t samrpc_get_all_vsn_pools_1_arg;
		string_arg_t samrpc_get_vsn_pool_1_arg;
		vsn_pool_arg_t samrpc_add_vsn_pool_1_arg;
		vsn_pool_arg_t samrpc_modify_vsn_pool_1_arg;
		string_arg_t samrpc_remove_vsn_pool_1_arg;
		ctx_arg_t samrpc_get_all_vsn_copy_maps_1_arg;
		string_arg_t samrpc_get_vsn_copy_map_1_arg;
		vsn_map_arg_t samrpc_add_vsn_copy_map_1_arg;
		vsn_map_arg_t samrpc_modify_vsn_copy_map_1_arg;
		string_arg_t samrpc_remove_vsn_copy_map_1_arg;
		string_bool_arg_t samrpc_get_default_ar_drive_directive_1_arg;
		string_arg_t samrpc_is_pool_in_use_1_arg;
		string_arg_t samrpc_is_valid_group_1_arg;
		string_arg_t samrpc_is_valid_user_3_arg;

		/* archive_set.h - control APIs */
		ctx_arg_t samrpc_get_all_arch_sets_3_arg;
		string_arg_t samrpc_get_arch_set_3_arg;
		arch_set_arg_t samrpc_create_arch_set_3_arg;
		arch_set_arg_t samrpc_modify_arch_set_3_arg;
		string_arg_t samrpc_delete_arch_set_3_arg;
		string_string_arg_t samrpc_associate_class_with_policy_6_svr;
		string_arg_t samrpc_delete_data_class_6_svr;
		str_critlst_arg_t samrpc_set_class_order_6_svr;

		/* archive.h - control APIs */
		string_arg_t samrpc_ar_run_1_arg;
		ctx_arg_t samrpc_ar_run_all_1_arg;
		string_arg_t samrpc_ar_stop_1_arg;
		string_arg_t samrpc_ar_idle_1_arg;
		ctx_arg_t samrpc_ar_idle_all_1_arg;
		ctx_arg_t samrpc_ar_rerun_all_4_arg;
		ctx_arg_t samrpc_get_archiverd_state_1_arg;
		ctx_arg_t samrpc_get_all_arfind_state_1_arg;
		string_arg_t samrpc_get_arfind_state_1_arg;
		string_arg_t samrpc_get_archreqs_1_arg;
		ctx_arg_t samrpc_get_all_archreqs_1_arg;
		ctx_arg_t samrpc_activate_archiver_cfg_1_arg;
		strlst_int32_arg_t samrpc_archive_files_6_arg;
		int_int_arg_t samrpc_get_copy_utilization_6_arg;

		/* device.h - config APIs */
		ctx_arg_t samrpc_discover_avail_aus_1_arg;
		au_type_arg_t samrpc_discover_avail_aus_by_type_1_arg;
		strlst_bool_arg_t samrpc_discover_ha_aus_5_arg;
		ctx_arg_t samrpc_discover_media_unused_in_mcf_1_arg;
		string_arg_t samrpc_discover_library_by_path_1_arg;
		string_arg_t samrpc_discover_standalone_drive_by_path_1_arg;
		ctx_arg_t samrpc_get_all_libraries_1_arg;
		string_arg_t samrpc_get_library_by_path_1_arg;
		string_arg_t samrpc_get_library_by_family_set_1_arg;
		equ_arg_t samrpc_get_library_by_equ_1_arg;
		library_arg_t samrpc_add_library_1_arg;
		equ_bool_arg_t samrpc_remove_library_1_arg;
		ctx_arg_t samrpc_get_all_standalone_drives_1_arg;
		equ_arg_t samrpc_get_no_of_catalog_entries_1_arg;
		equ_slot_part_bool_arg_t
			samrpc_rb_auditslot_from_eq_1_arg;
		chmed_arg_t samrpc_rb_chmed_flags_from_eq_1_arg;
		equ_arg_t samrpc_rb_clean_drive_1_arg;
		import_arg_t samrpc_rb_import_1_arg;
		import_range_arg_t samrpc_import_all_1_arg;
		equ_slot_bool_arg_t rb_export_from_eq_1_arg;
		equ_slot_part_bool_arg_t
			samrpc_rb_load_from_eq_1_arg;
		equ_bool_arg_t samrpc_rb_unload_1_arg;
		move_arg_t samrpc_rb_move_from_eq_1_arg;
		tplabel_arg_t samrpc_rb_tplabel_from_eq_1_arg;
		equ_slot_part_arg_t samrpc_rb_unreserve_from_eq_1_arg;
		equ_dstate_arg_t samrpc_change_state_1_arg;
		string_arg_t samrpc_is_vsn_loaded_1_arg;
		equ_arg_t samrpc_get_total_capacity_of_library_1_arg;
		equ_arg_t samrpc_get_free_space_of_library_1_arg;
		string_sort_arg_t samrpc_get_vsn_list_1_arg;
		ctx_arg_t samrpc_get_tape_label_running_list_1_arg;
		nwlib_req_info_arg_t samrpc_get_nw_library_1_arg;
		ctx_arg_t samrpc_get_all_available_media_type_1_arg;
		string_sort_arg_t
			samrpc_get_properties_of_archive_vsnpool_1_arg;
		string_sort_arg_t samrpc_get_available_vsns_1_arg;
		string_arg_t samrpc_get_catalog_entry_1_arg;
		string_mtype_arg_t samrpc_get_catalog_entry_by_media_type_1_arg;
		equ_slot_part_arg_t samrpc_get_catalog_entry_from_lib_1_arg;
		equ_sort_arg_t samrpc_get_all_catalog_entries_1_arg;
		reserve_arg_t samrpc_rb_reserve_from_eq_1_arg;
		string_arg_t samrpc_get_default_media_block_size_1_arg;
		string_list_arg_t samrpc_check_slices_for_overlaps_2_arg;
		ctx_arg_t samrpc_discover_aus_1_arg;
		vsnpool_arg_t samrpc_get_vsn_pool_properties_4_arg;
		vsnmap_arg_t samrpc_get_vsn_map_properties_4_arg;
		stk_host_list_arg_t samrpc_discover_stk_5_arg;
		stk_host_info_string_arg_t
			samrpc_get_stk_filter_volume_list_5_arg;
		import_vsns_arg_t samrpc_import_stk_vsns_5_arg;
		library_list_arg_t samrpc_add_list_libraries_5_arg;
		string_arg_t samrpc_get_stk_vsn_names_5_arg;
		stk_host_info_string_arg_t samrpc_get_stk_phyconf_info_5_arg;
		equ_equ_bool_arg_t samrpc_modify_stkdrive_share_status_5_arg;
		int_uint32_arg_t samrpc_get_vsns_6_arg;

		/* filesystem.h */
		ctx_arg_t samrpc_get_all_fs_1_arg;
		ctx_arg_t samrpc_get_fs_names_1_arg;
		string_arg_t samrpc_get_fs_1_arg;
		create_fs_arg_t samrpc_create_fs_1_arg;
		change_mount_options_arg_t samrpc_change_mount_options_1_arg;
		change_mount_options_arg_t
			samrpc_change_live_mount_options_1_arg;
		get_default_mount_options_arg_t
			samrpc_get_default_mount_options_1_arg;
		string_arg_t samrpc_remove_fs_1_arg;
		fsck_fs_arg_t samrpc_samfsck_fs_1_arg;
		ctx_arg_t samrpc_get_all_samfsck_info_1_arg;
		string_arg_t samrpc_mount_fs_1_arg;
		string_bool_arg_t samrpc_umount_fs_1_arg;
		grow_fs_arg_t samrpc_grow_fs_1_arg;
		create_fs_mount_arg_t samrpc_create_fs_and_mount_1_arg;
		int_list_arg_t samrpc_get_equipment_ordinals_3_arg;
		intlist_arg_t samrpc_check_equipment_ordinals_3_arg;
		ctx_arg_t samrpc_get_fs_names_all_types_3_arg;
		reset_eq_arg_t samrpc_reset_equipment_ordinals_3_arg;
		string_arg_t samrpc_get_generic_filesystems_4_arg;
		string_string_arg_t samrpc_mount_generic_fs_4_arg;
		string_string_arg_t samrpc_remove_generic_fs_4_arg;
		string_arg_t samrpc_get_nfs_opts_4_arg;
		string_string_arg_t samrpc_set_nfs_opts_4_arg;
		string_strlst_arg_t samrpc_set_advanced_network_cfg_5_arg;
		string_arg_t samrpc_get_advanced_network_cfg_5_arg;
		string_arg_t samrpc_get_mds_host_6_arg;
		create_arch_fs_arg_t samrpc_create_arch_fs_6_arg;

		/* license.h */
		ctx_arg_t samrpc_get_license_type_1_arg;
		ctx_arg_t samrpc_get_expiration_date_1_arg;
		ctx_arg_t samrpc_get_samfs_type_1_arg;
		string_string_arg_t
			samrpc_get_licensed_media_slots_1_arg;
		ctx_arg_t samrpc_get_licensed_media_types_1_arg;
		ctx_arg_t samrpc_get_license_info_3_arg;

		/* load.h */
		ctx_arg_t samrpc_get_pending_load_info_1_arg;
		clear_load_request_arg_t samrpc_clear_load_request_1_arg;

		/* recycle.h */
		ctx_arg_t samrpc_get_rc_log_1_arg;
		rc_upath_arg_t samrpc_set_rc_log_1_arg;
		ctx_arg_t samrpc_get_default_rc_log_1_arg;
		ctx_arg_t samrpc_get_rc_notify_script_1_arg;
		rc_upath_arg_t samrpc_set_rc_notify_script_1_arg;
		ctx_arg_t samrpc_get_default_rc_notify_script_1_arg;
		ctx_arg_t samrpc_get_all_no_rc_vsns_1_arg;
		string_arg_t samrpc_get_no_rc_vsns_1_arg;
		no_rc_vsns_arg_t samrpc_add_no_rc_vsns_1_arg;
		no_rc_vsns_arg_t samrpc_modify_no_rc_vsns_1_arg;
		no_rc_vsns_arg_t samrpc_remove_no_rc_vsns_1_arg;
		ctx_arg_t samrpc_get_all_rc_robot_cfg_1_arg;
		string_arg_t samrpc_get_rc_robot_cfg_1_arg;
		rc_robot_cfg_arg_t samrpc_set_rc_robot_cfg_1_arg;
		rc_robot_cfg_arg_t samrpc_reset_rc_robot_cfg_1_arg;
		ctx_arg_t samrpc_get_default_rc_params_1_arg;

		/* release.h */
		ctx_arg_t samrpc_get_all_rl_fs_directives_1_arg;
		string_arg_t samrpc_get_rl_fs_directive_1_arg;
		ctx_arg_t samrpc_get_default_rl_fs_directive_1_arg;
		rl_fs_directive_arg_t samrpc_set_rl_fs_directive_1_arg;
		rl_fs_directive_arg_t samrpc_reset_rl_fs_directive_1_arg;
		ctx_arg_t samrpc_get_releasing_fs_list_1_arg;
		strlst_int32_int32_arg_t samrpc_release_files_6_arg;
		/* stage.h */
		ctx_arg_t samrpc_get_stager_cfg_1_arg;
		string_arg_t samrpc_get_drive_directive_1_arg;
		string_arg_t samrpc_get_buffer_directive_1_arg;
		set_stager_cfg_arg_t samrpc_set_stager_cfg_1_arg;
		drive_directive_arg_t samrpc_set_drive_directive_1_arg;
		drive_directive_arg_t samrpc_reset_drive_directive_1_arg;
		buffer_directive_arg_t samrpc_set_buffer_directive_1_arg;
		buffer_directive_arg_t samrpc_reset_buffer_directive_1_arg;
		ctx_arg_t samrpc_get_default_stager_cfg_1_arg;
		string_arg_t samrpc_get_default_staging_drive_directive_1_arg;
		string_arg_t samrpc_get_default_staging_buffer_directive_1_arg;
		ctx_arg_t samrpc_get_stager_info_1_arg;
		ctx_arg_t samrpc_get_all_staging_files_1_arg;
		range_arg_t samrpc_get_staging_files_1_arg;
		staging_file_arg_t samrpc_find_staging_file_1_arg;
		stager_stream_arg_t
			samrpc_get_all_staging_files_in_stream_1_arg;
		stager_stream_range_arg_t
			samrpc_get_staging_files_in_stream_1_arg;
		stage_arg_t samrpc_cancel_stage_1_arg;
		clear_stage_request_arg_t samrpc_clear_stage_request_1_arg;
		ctx_arg_t samrpc_st_idle_1_arg;
		ctx_arg_t samrpc_st_run_1_arg;
		ctx_arg_t samrpc_get_total_staging_files_1_arg;
		stream_arg_t samrpc_get_numof_staging_files_in_stream_1_arg;
		strlst_intlst_intlst_arg_t samrpc_stage_files_5_arg;
		strlst_int32_arg_t samrpc_stage_files_6_arg;

		/* faults.h */
		string_arg_t samrpc_get_faults_by_lib_1_arg;
		equ_arg_t samrpc_get_faults_by_eq_1_arg;
		ctx_arg_t samrpc_is_faults_gen_status_on_1_arg;
		fault_errorid_arr_arg_t samrpc_delete_faults_1_arg;
		fault_errorid_arr_arg_t samrpc_ack_faults_1_arg;
		ctx_arg_t samrpc_get_fault_summary_1_arg;
		ctx_arg_t samrpc_get_all_faults_6_arg;

		/* diskvols.h */
		string_arg_t samrpc_get_disk_vol_1_arg;
		ctx_arg_t samrpc_get_all_disk_vols_1_arg;
		ctx_arg_t samrpc_get_all_clients_1_arg;
		disk_vol_arg_t samrpc_add_disk_vol_1_arg;
		string_arg_t samrpc_remove_disk_vol_1_arg;
		string_arg_t samrpc_add_client_1_arg;
		string_arg_t samrpc_remove_client_1_arg;
		string_uint32_arg_t samrpc_set_disk_vol_flags_4_arg;

		/* notify_summary.h */
		ctx_arg_t samrpc_get_notify_summary_1_arg;
		notify_summary_arg_t samrpc_del_notify_summary_1_arg;
		notify_summary_arg_t samrpc_add_notify_summary_1_arg;
		mod_notify_summary_arg_t samrpc_mod_notify_summary_1_arg;
		get_email_addrs_arg_t samrpc_get_email_addrs_by_subj_5_arg;

		/* recyc_sh_wrap.h */
		ctx_arg_t samrpc_get_recycl_sh_action_status_1_arg;
		ctx_arg_t samrpc_add_recycle_sh_action_label_1_arg;
		string_arg_t samrpc_add_recycle_sh_action_export_1_arg;
		ctx_arg_t samrpc_del_recycle_sh_action_1_arg;

		/* job_history.h */
		job_hist_arg_t samrpc_get_jobhist_by_fs_1_arg;
		job_type_arg_t samrpc_get_all_jobhist_1_arg;

		/* hosts.h */
		string_arg_t samrpc_get_host_config_3_arg;
		string_string_arg_t samrpc_remove_host_3_arg;
		string_arg_t samrpc_config_metadata_server_3_arg;
		string_host_info_arg_t samrpc_add_host_3_arg;
		ctx_arg_t samrpc_discover_ip_addresses_3_arg;

		/* restore.h */
		string_string_arg_t samrpc_set_csd_params_4_arg;
		string_arg_t samrpc_get_csd_params_4_arg;
		string_arg_t samrpc_list_dumps_4_arg;
		string_strlst_arg_t samrpc_get_dump_status_4_arg;
		string_string_arg_t samrpc_decompress_dump_4_arg;
		string_string_arg_t samrpc_cleanup_dump_4_arg;
		string_string_arg_t samrpc_take_dump_4_arg;
		file_restrictions_arg_t samrpc_list_versions_4_arg;
		version_details_arg_t samrpc_get_version_details_4_arg;
		file_restrictions_arg_t samrpc_search_versions_4_arg;
		string_arg_t samrpc_get_search_results_4_arg;
		restore_inodes_arg_t samrpc_restore_inodes_4_arg;
		string_string_strlst_arg_t samrpc_get_dump_status_by_dir_4_arg;
		string_string_arg_t samrpc_list_dumps_by_dir_4_arg;
		string_string_arg_t samrpc_delete_dump_4_arg;
		string_arg_t samrpc_set_snapshot_locked_5_arg;
		string_arg_t samrpc_clear_snapshot_locked_5_arg;
		string_arg_t samrpc_get_indexed_snapshot_directories_6_arg;
		string_string_arg_t samrpc_get_indexed_snapshots_6_arg;

		/* activities on server */
		int_string_arg_t samrpc_list_activities_4_arg;
		string_string_arg_t samrpc_kill_activity_4_arg;

		/* file_util.h */
		string_arg_t samrpc_create_dir_1_arg;
		string_arg_t samrpc_create_file_1_arg;
		string_arg_t samrpc_file_exists_3_arg;
		file_restrictions_arg_t samrpc_list_dir_4_arg;
		strlst_arg_t samrpc_get_file_status_4_arg;
		string_strlst_arg_t samrpc_get_file_details_4_arg;
		string_uint32_arg_t samrpc_tail_4_arg;
		string_uint32_uint32_arg_t samrpc_get_txt_file_5_arg;
		strlst_uint32_arg_t samrcp_get_extended_file_details_5_arg;
		file_restrictions_more_arg_t samrpc_list_directory_6_arg;
		file_details_arg_t samrpc_list_and_collect_file_details_6_arg;
		file_details_arg_t samrpc_collect_file_details_6_arg;
		strlst_arg_t samrpc_delete_files_6_arg;

		/* registration */
		string_arg_t samrpc_cns_get_registration_6_arg;
		cns_reg_arg_t samrpc_cns_register_6_arg;
		ctx_arg_t samrpc_cns_get_public_key_6_svr;

		/* reports */
		report_requirement_arg_t samrpc_gen_report_6_arg;
		file_metric_rpt_arg_t samrpc__get_file_metrics_report_6_arg;

		/* task_schedule.h */
		ctx_arg_t samrpc_get_task_schedules_6_svr;
		string_arg_t samrpc_set_task_schedule_6_svr;
		string_arg_t samrpc_remove_task_schedule_6_svr;
		string_string_arg_t samrpc_get_specific_tasks_6_svr;

	} argument;

	char *result;

	bool_t (*_xdr_argument)(), (*_xdr_result)();
	char *(*local)();

	_rpcsvccount++;

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void,
			(char *)NULL);
		_rpcsvccount--;

		return;

	case samrpc_init:
		_xdr_argument = xdr_int;
		_xdr_result = xdr_int;
		local = (char *(*)()) samrpc_init_1_svr;
		break;

	/*
	 * mgmt.h
	 */
	case samrpc_get_samfs_version:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_samfs_version_1_svr;
		break;

	case samrpc_get_samfs_lib_version:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_samfs_lib_version_1_svr;
		break;

	case samrpc_init_sam_mgmt:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_init_sam_mgmt_1_svr;
		break;

	case samrpc_destroy_process:
		_xdr_argument = xdr_proc_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_destroy_process_1_svr;
		break;

	case samrpc_get_server_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_server_info_3_svr;
		break;

	case samrpc_get_server_capacities:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_server_capacities_4_svr;
		break;

	case samrpc_get_system_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_system_info_4_svr;
		break;

	case samrpc_get_logntrace:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_logntrace_4_svr;
		break;

	case samrpc_get_package_info:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_package_info_4_svr;
		break;

	case samrpc_get_configuration_status:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_configuration_status_4_svr;
		break;

	case samrpc_get_architecture:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_architecture_4_svr;
		break;

	case samrpc_list_explorer_outputs:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_explorer_outputs_5_svr;
		break;

	case samrpc_run_sam_explorer:
		_xdr_argument = xdr_int_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_run_sam_explorer_5_svr;
		break;

	// sun cluster. since 4.5
	case samrpc_get_sc_version:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_sc_version_5_svr;
		break;
	case samrpc_get_sc_name:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_sc_name_5_svr;
		break;
	case samrpc_get_sc_nodes:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_sc_nodes_5_svr;
		break;
	case samrpc_get_sc_ui_state:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_sc_ui_state_5_svr;
		break;
	// monitor.h
	case samrpc_get_status_processes:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_status_processes_6_svr;
		break;

	case samrpc_get_component_status_summary:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_component_status_summary_6_svr;
		break;

	/*
	 * archive.h - config APIs
	 */
	case samrpc_get_default_ar_set_copy_cfg:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_ar_set_copy_cfg_1_svr;
		break;

	case samrpc_get_default_ar_set_criteria:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_ar_set_criteria_1_svr;
		break;

	case samrpc_get_ar_global_directive:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_global_directive_1_svr;
		break;

	case samrpc_set_ar_global_directive:
		_xdr_argument = xdr_ar_global_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_ar_global_directive_1_svr;
		break;

	case samrpc_get_default_ar_global_directive:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_default_ar_global_directive_1_svr;
		break;

	case samrpc_get_ar_set_criteria_names:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_set_criteria_names_1_svr;
		break;

	case samrpc_get_ar_set:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_set_1_svr;
		break;

	case samrpc_get_ar_set_criteria_list:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_set_criteria_list_1_svr;
		break;

	case samrpc_get_all_ar_set_criteria:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_ar_set_criteria_1_svr;
		break;

	case samrpc_get_ar_fs_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_fs_directive_1_svr;
		break;

	case samrpc_get_all_ar_fs_directives:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_ar_fs_directives_1_svr;
		break;

	case samrpc_set_ar_fs_directive:
		_xdr_argument = xdr_ar_fs_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_ar_fs_directive_1_svr;
		break;

	case samrpc_reset_ar_fs_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_ar_fs_directive_1_svr;
		break;

	case samrpc_modify_ar_set_criteria:
		_xdr_argument = xdr_ar_set_criteria_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_modify_ar_set_criteria_1_svr;
		break;

	case samrpc_get_default_ar_fs_directive:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_ar_fs_directive_1_svr;
		break;

	case samrpc_get_all_ar_set_copy_params:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_ar_set_copy_params_1_svr;
		break;

	case samrpc_get_ar_set_copy_params_names:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_set_copy_params_names_1_svr;
		break;

	case samrpc_get_ar_set_copy_params:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_ar_set_copy_params_1_svr;
		break;

	case samrpc_get_ar_set_copy_params_for_ar_set:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_ar_set_copy_params_for_ar_set_1_svr;
		break;

	case samrpc_set_ar_set_copy_params:
		_xdr_argument = xdr_ar_set_copy_params_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_ar_set_copy_params_1_svr;
		break;

	case samrpc_reset_ar_set_copy_params:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_ar_set_copy_params_1_svr;
		break;

	case samrpc_get_default_ar_set_copy_params:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_default_ar_set_copy_params_1_svr;
		break;

	case samrpc_get_all_vsn_pools:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_vsn_pools_1_svr;
		break;

	case samrpc_get_vsn_pool:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_vsn_pool_1_svr;
		break;

	case samrpc_add_vsn_pool:
		_xdr_argument = xdr_vsn_pool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_vsn_pool_1_svr;
		break;

	case samrpc_modify_vsn_pool:
		_xdr_argument = xdr_vsn_pool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_modify_vsn_pool_1_svr;
		break;

	case samrpc_remove_vsn_pool:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_vsn_pool_1_svr;
		break;

	case samrpc_get_all_vsn_copy_maps:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_vsn_copy_maps_1_svr;
		break;

	case samrpc_get_vsn_copy_map:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_vsn_copy_map_1_svr;
		break;

	case samrpc_add_vsn_copy_map:
		_xdr_argument = xdr_vsn_map_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_vsn_copy_map_1_svr;
		break;

	case samrpc_modify_vsn_copy_map:
		_xdr_argument = xdr_vsn_map_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_modify_vsn_copy_map_1_svr;
		break;

	case samrpc_remove_vsn_copy_map:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_vsn_copy_map_1_svr;
		break;

	case samrpc_get_default_ar_drive_directive:
		_xdr_argument = xdr_string_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_default_ar_drive_directive_1_svr;
		break;

	case samrpc_is_pool_in_use:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_is_pool_in_use_1_svr;
		break;

	case samrpc_is_valid_group:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_is_valid_group_1_svr;
		break;

	case samrpc_is_valid_user:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_is_valid_user_3_svr;
		break;

	case samrpc_get_all_arch_sets:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_all_arch_sets_3_svr;
		break;

	case samrpc_get_arch_set:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_arch_set_3_svr;
		break;

	case samrpc_create_arch_set:
		_xdr_argument = xdr_arch_set_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_create_arch_set_3_svr;
		break;

	case samrpc_modify_arch_set:
		_xdr_argument = xdr_arch_set_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_modify_arch_set_3_svr;
		break;

	case samrpc_delete_arch_set:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_delete_arch_set_3_svr;
		break;

	case samrpc_associate_class_with_policy:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_associate_class_with_policy_6_svr;
		break;

	case samrpc_delete_data_class:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_delete_data_class_6_svr;
		break;

	case samrpc_set_class_order:
		_xdr_argument = xdr_str_critlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_class_order_6_svr;
		break;


	/*
	 * archive.h - control APIs
	 */
	case samrpc_ar_run:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_run_1_svr;
		break;

	case samrpc_ar_run_all:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_run_all_1_svr;
		break;

	case samrpc_ar_stop:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_stop_1_svr;
		break;

	case samrpc_ar_stop_all:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_stop_all_1_svr;
		break;

	case samrpc_ar_idle:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_idle_1_svr;
		break;

	case samrpc_ar_idle_all:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_idle_all_1_svr;
		break;

	case samrpc_ar_restart_all:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_restart_all_1_svr;
		break;

	case samrpc_ar_rerun_all:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ar_rerun_all_4_svr;
		break;

	case samrpc_get_archiverd_state:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_archiverd_state_1_svr;
		break;

	case samrpc_get_all_arfind_state:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_arfind_state_1_svr;
		break;

	case samrpc_get_arfind_state:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_arfind_state_1_svr;
		break;

	case samrpc_get_archreqs:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_archreqs_1_svr;
		break;

	case samrpc_get_all_archreqs:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_archreqs_1_svr;
		break;

	case samrpc_activate_archiver_cfg:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_activate_archiver_cfg_1_svr;
		break;

	case samrpc_archive_files:
		_xdr_argument = xdr_strlst_int32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_archive_files_6_svr;
		break;

	case samrpc_get_copy_utilization:
		_xdr_argument = xdr_int_int_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_copy_utilization_6_svr;
		break;

	/*
	 * device.h - config APIs
	 */
	case samrpc_discover_avail_aus:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_avail_aus_1_svr;
		break;
	case samrpc_discover_aus:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_aus_1_svr;
		break;

	case samrpc_discover_avail_aus_by_type:
		_xdr_argument = xdr_au_type_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_avail_aus_by_type_1_svr;
		break;

	case samrpc_discover_ha_aus:
		_xdr_argument = xdr_strlst_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_ha_aus_5_svr;
		break;

	case samrpc_discover_media_unused_in_mcf:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_media_unused_in_mcf_1_svr;
		break;

	case samrpc_discover_library_by_path:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_library_by_path_1_svr;
		break;

	case samrpc_discover_standalone_drive_by_path:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_discover_standalone_drive_by_path_1_svr;
		break;

	case samrpc_get_all_libraries:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_libraries_1_svr;
		break;

	case samrpc_get_library_by_path:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_library_by_path_1_svr;
		break;

	case samrpc_get_library_by_family_set:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_library_by_family_set_1_svr;
		break;

	case samrpc_get_library_by_equ:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_library_by_equ_1_svr;
		break;

	case samrpc_add_library:
		_xdr_argument = xdr_library_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_library_1_svr;
		break;

	case samrpc_remove_library:
		_xdr_argument = xdr_equ_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_library_1_svr;
		break;

	case samrpc_get_all_standalone_drives:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_standalone_drives_1_svr;
		break;

	case samrpc_get_no_of_catalog_entries:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_no_of_catalog_entries_1_svr;
		break;

	case samrpc_rb_auditslot_from_eq:
		_xdr_argument = xdr_equ_slot_part_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_auditslot_from_eq_1_svr;
		break;

	case samrpc_rb_chmed_flags_from_eq:
		_xdr_argument = xdr_chmed_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_chmed_flags_from_eq_1_svr;
		break;

	case samrpc_rb_clean_drive:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_clean_drive_1_svr;
		break;

	case samrpc_rb_import:
		_xdr_argument = xdr_import_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_import_1_svr;
		break;

	case samrpc_import_all:
		_xdr_argument = xdr_import_range_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_import_all_1_svr;
		break;

	case samrpc_rb_export_from_eq:
		_xdr_argument = xdr_equ_slot_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_export_from_eq_1_svr;
		break;

	case samrpc_rb_load_from_eq:
		_xdr_argument = xdr_equ_slot_part_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_load_from_eq_1_svr;
		break;

	case samrpc_rb_unload:
		_xdr_argument = xdr_equ_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_unload_1_svr;
		break;

	case samrpc_rb_move_from_eq:
		_xdr_argument = xdr_move_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_move_from_eq_1_svr;
		break;

	case samrpc_rb_tplabel_from_eq:
		_xdr_argument = xdr_tplabel_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_tplabel_from_eq_1_svr;
		break;

	case samrpc_rb_unreserve_from_eq:
		_xdr_argument = xdr_equ_slot_part_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_unreserve_from_eq_1_svr;
		break;

	case samrpc_change_state:
		_xdr_argument = xdr_equ_dstate_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_change_state_1_svr;
		break;

	case samrpc_is_vsn_loaded:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_is_vsn_loaded_1_svr;
		break;

	case samrpc_get_total_capacity_of_library:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_total_capacity_of_library_1_svr;
		break;

	case samrpc_get_free_space_of_library:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_free_space_of_library_1_svr;
		break;

	case samrpc_get_vsn_list:
		_xdr_argument = xdr_string_sort_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_vsn_list_1_svr;
		break;

	case samrpc_get_tape_label_running_list:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_tape_label_running_list_1_svr;
		break;

	case samrpc_get_nw_library:
		_xdr_argument = xdr_nwlib_req_info_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_nw_library_1_svr;
		break;

	case samrpc_get_all_available_media_type:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_all_available_media_type_1_svr;
		break;

	case samrpc_get_properties_of_archive_vsnpool:
		_xdr_argument = xdr_string_sort_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_properties_of_archive_vsnpool_1_svr;
		break;

	case samrpc_get_available_vsns:
		_xdr_argument = xdr_string_sort_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_available_vsns_1_svr;
		break;

	case samrpc_get_catalog_entry:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_catalog_entry_1_svr;
		break;

	case samrpc_get_catalog_entry_by_media_type:
		_xdr_argument = xdr_string_mtype_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_catalog_entry_by_media_type_1_svr;
		break;

	case samrpc_get_catalog_entry_from_lib:
		_xdr_argument = xdr_equ_slot_part_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_catalog_entry_from_lib_1_svr;
		break;

	case samrpc_get_all_catalog_entries:
		_xdr_argument = xdr_equ_sort_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_catalog_entries_1_svr;
		break;

	case samrpc_rb_reserve_from_eq:
		_xdr_argument = xdr_reserve_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_rb_reserve_from_eq_1_svr;
		break;

	case samrpc_get_default_media_block_size:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_media_block_size_1_svr;
		break;

	case samrpc_check_slices_for_overlaps:
		_xdr_argument = xdr_string_list_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_check_slices_for_overlaps_2_svr;
		break;

	case samrpc_get_vsn_pool_properties:
		_xdr_argument = xdr_vsnpool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_vsn_pool_properties_4_svr;
		break;

	case samrpc_get_vsn_map_properties:
		_xdr_argument = xdr_vsnmap_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_vsn_map_properties_4_svr;
		break;

	case samrpc_discover_stk:
		_xdr_argument = xdr_stk_host_list_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_stk_5_svr;
		break;

	case samrpc_get_stk_phyconf_info:
		_xdr_argument = xdr_stk_host_info_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_stk_phyconf_info_5_svr;
		break;

	case samrpc_add_list_libraries:
		_xdr_argument = xdr_library_list_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_list_libraries_5_svr;
		break;

	case samrpc_import_stk_vsns:
		_xdr_argument = xdr_import_vsns_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_import_stk_vsns_5_svr;
		break;

	case samrpc_get_stk_vsn_names:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_stk_vsn_names_5_svr;
		break;

	case samrpc_get_stk_filter_volume_list:
		_xdr_argument = xdr_stk_host_info_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_stk_filter_volume_list_5_svr;
		break;

	case samrpc_modify_stkdrive_share_status:
		_xdr_argument = xdr_equ_equ_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_modify_stkdrive_share_status_5_svr;
		break;

	case samrpc_get_vsns:
		_xdr_argument = xdr_int_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_vsns_6_svr;
		break;

	/*
	 * filesystem.h
	 */
	case samrpc_get_all_fs:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_fs_1_svr;
		break;

	case samrpc_get_fs_names:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_fs_names_1_svr;
		break;

	case samrpc_get_fs_names_all_types:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_fs_names_all_types_3_svr;
		break;

	case samrpc_get_fs:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_fs_1_svr;
		break;

	case samrpc_create_fs:
		_xdr_argument = xdr_create_fs_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_create_fs_1_svr;
		break;

	case samrpc_change_mount_options:
		_xdr_argument = xdr_change_mount_options_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_change_mount_options_1_svr;
		break;

	case samrpc_change_live_mount_options:
		_xdr_argument = xdr_change_mount_options_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_change_live_mount_options_1_svr;
		break;

	case samrpc_get_default_mount_options:
		_xdr_argument = xdr_get_default_mount_options_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_mount_options_1_svr;
		break;

	case samrpc_remove_fs:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_fs_1_svr;
		break;

	case samrpc_samfsck_fs:
		_xdr_argument = xdr_fsck_fs_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_samfsck_fs_1_svr;
		break;

	case samrpc_get_all_samfsck_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_samfsck_info_1_svr;
		break;

	case samrpc_mount_fs:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_mount_fs_1_svr;
		break;

	case samrpc_umount_fs:
		_xdr_argument = xdr_string_bool_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_umount_fs_1_svr;
		break;

	case samrpc_grow_fs:
		_xdr_argument = xdr_grow_fs_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_grow_fs_1_svr;
		break;

	case samrpc_create_fs_and_mount:
		_xdr_argument = xdr_create_fs_mount_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_create_fs_and_mount_1_svr;
		break;

	case samrpc_get_equipment_ordinals:
		_xdr_argument = xdr_int_list_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_equipment_ordinals_3_svr;
		break;

	case samrpc_check_equipment_ordinals:
		_xdr_argument = xdr_intlist_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_check_equipment_ordinals_3_svr;
		break;

	case samrpc_reset_equipment_ordinals:
		_xdr_argument = xdr_reset_eq_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_equipment_ordinals_3_svr;
		break;

	case samrpc_get_generic_filesystems:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_generic_filesystems_4_svr;
		break;

	case samrpc_mount_generic_fs:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_mount_generic_fs_4_svr;
		break;

	case samrpc_remove_generic_fs:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_generic_fs_4_svr;
		break;

	case samrpc_get_nfs_opts:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_nfs_opts_4_svr;
		break;

	case samrpc_set_nfs_opts:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_nfs_opts_4_svr;
		break;


	case samrpc_set_advanced_network_cfg:
		_xdr_argument = xdr_string_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_advanced_network_cfg_5_svr;
		break;

	case samrpc_get_advanced_network_cfg:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_advanced_network_cfg_5_svr;
		break;

	case samrpc_get_mds_host:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_mds_host_6_svr;
		break;

	case samrpc_create_arch_fs:
		_xdr_argument = xdr_create_arch_fs_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_create_arch_fs_6_svr;
		break;

	/*
	 * license.h
	 */

	case samrpc_get_license_type:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_license_type_1_svr;
		break;

	case samrpc_get_expiration_date:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_expiration_date_1_svr;
		break;

	case samrpc_get_samfs_type:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_samfs_type_1_svr;
		break;

	case samrpc_get_licensed_media_slots:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_licensed_media_slots_1_svr;
		break;

	case samrpc_get_licensed_media_types:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_licensed_media_types_1_svr;
		break;

	case samrpc_get_license_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_license_info_3_svr;
		break;

	/*
	 * load.h
	 */
	case samrpc_get_pending_load_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_pending_load_info_1_svr;
		break;

	case samrpc_clear_load_request:
		_xdr_argument = xdr_clear_load_request_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_clear_load_request_1_svr;
		break;
	/*
	 * recycle.h
	 */

	case samrpc_get_rc_log:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_rc_log_1_svr;
		break;

	case samrpc_set_rc_log:
		_xdr_argument = xdr_rc_upath_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_rc_log_1_svr;
		break;

	case samrpc_get_default_rc_log:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_rc_log_1_svr;
		break;

	case samrpc_get_rc_notify_script:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_rc_notify_script_1_svr;
		break;

	case samrpc_set_rc_notify_script:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_rc_notify_script_1_svr;
		break;

	case samrpc_get_default_rc_notify_script:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_rc_notify_script_1_svr;
		break;

	case samrpc_get_all_no_rc_vsns:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_no_rc_vsns_1_svr;
		break;

	case samrpc_get_no_rc_vsns:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_no_rc_vsns_1_svr;
		break;

	case samrpc_add_no_rc_vsns:
		_xdr_argument = xdr_no_rc_vsns_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_no_rc_vsns_1_svr;
		break;

	case samrpc_modify_no_rc_vsns:
		_xdr_argument = xdr_no_rc_vsns_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_modify_no_rc_vsns_1_svr;
		break;

	case samrpc_remove_no_rc_vsns:
		_xdr_argument = xdr_no_rc_vsns_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_no_rc_vsns_1_svr;
		break;

	case samrpc_get_all_rc_robot_cfg:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_rc_robot_cfg_1_svr;
		break;

	case samrpc_get_rc_robot_cfg:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_rc_robot_cfg_1_svr;
		break;

	case samrpc_set_rc_robot_cfg:
		_xdr_argument = xdr_rc_robot_cfg_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_rc_robot_cfg_1_svr;
		break;

	case samrpc_reset_rc_robot_cfg:
		_xdr_argument = xdr_rc_robot_cfg_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_rc_robot_cfg_1_svr;
		break;

	case samrpc_get_default_rc_params:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_rc_params_1_svr;
		break;

	/*
	 * release.h
	 */

	case samrpc_get_all_rl_fs_directives:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_rl_fs_directives_1_svr;
		break;

	case samrpc_get_rl_fs_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_rl_fs_directive_1_svr;
		break;

	case samrpc_get_default_rl_fs_directive:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_rl_fs_directive_1_svr;
		break;

	case samrpc_set_rl_fs_directive:
		_xdr_argument = xdr_rl_fs_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_rl_fs_directive_1_svr;
		break;

	case samrpc_reset_rl_fs_directive:
		_xdr_argument = xdr_rl_fs_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_rl_fs_directive_1_svr;
		break;

	case samrpc_get_releasing_fs_list:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_releasing_fs_list_1_svr;
		break;

	case samrpc_release_files:
		_xdr_argument = xdr_strlst_int32_int32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_release_files_6_svr;
		break;

	/*
	 * stage.h
	 */

	case samrpc_get_stager_cfg:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_stager_cfg_1_svr;
		break;

	case samrpc_get_drive_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_drive_directive_1_svr;
		break;

	case samrpc_get_buffer_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_buffer_directive_1_svr;
		break;

	case samrpc_set_stager_cfg:
		_xdr_argument = xdr_set_stager_cfg_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_stager_cfg_1_svr;
		break;

	case samrpc_set_drive_directive:
		_xdr_argument = xdr_drive_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_drive_directive_1_svr;
		break;

	case samrpc_reset_drive_directive:
		_xdr_argument = xdr_drive_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_drive_directive_1_svr;
		break;

	case samrpc_set_buffer_directive:
		_xdr_argument = xdr_buffer_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_buffer_directive_1_svr;
		break;

	case samrpc_reset_buffer_directive:
		_xdr_argument = xdr_buffer_directive_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_reset_buffer_directive_1_svr;
		break;

	case samrpc_get_default_stager_cfg:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_default_stager_cfg_1_svr;
		break;

	case samrpc_get_default_staging_drive_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_default_staging_drive_directive_1_svr;
		break;

	case samrpc_get_default_staging_buffer_directive:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_default_staging_buffer_directive_1_svr;
		break;

	case samrpc_get_stager_info:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_stager_info_1_svr;
		break;

	case samrpc_get_all_staging_files:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_staging_files_1_svr;
		break;

	case samrpc_get_staging_files:
		_xdr_argument = xdr_range_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_staging_files_1_svr;
		break;

	case samrpc_get_all_staging_files_in_stream:
		_xdr_argument = xdr_stager_stream_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local =
		(char *(*)()) samrpc_get_all_staging_files_in_stream_1_svr;
		break;

	case samrpc_get_staging_files_in_stream:
		_xdr_argument = xdr_stager_stream_range_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local =
		(char *(*)()) samrpc_get_staging_files_in_stream_1_svr;
		break;

	case samrpc_find_staging_file:
		_xdr_argument = xdr_staging_file_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_find_staging_file_1_svr;
		break;

	case samrpc_cancel_stage:
		_xdr_argument = xdr_stage_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_cancel_stage_1_svr;
		break;

	case samrpc_clear_stage_request:
		_xdr_argument = xdr_clear_stage_request_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_clear_stage_request_1_svr;
		break;

	case samrpc_st_idle:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_st_idle_1_svr;
		break;

	case samrpc_st_run:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_st_run_1_svr;
		break;

	case samrpc_get_total_staging_files:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_total_staging_files_1_svr;
		break;

	case samrpc_get_numof_staging_files_in_stream:
		_xdr_argument = xdr_stream_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)())
			samrpc_get_numof_staging_files_in_stream_1_svr;
		break;

	case samrpc_stage_files_pre46:
		_xdr_argument = xdr_strlst_intlst_intlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_stage_files_5_svr;
		break;

	case samrpc_stage_files:
		_xdr_argument = xdr_strlst_int32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_stage_files_6_svr;
		break;

	/*
	 * faults.h
	 */
	case samrpc_get_faults_by_lib:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_faults_by_lib_1_svr;
		break;

	case samrpc_get_faults_by_eq:
		_xdr_argument = xdr_equ_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_faults_by_eq_1_svr;
		break;

	case samrpc_is_faults_gen_status_on:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_is_faults_gen_status_on_1_svr;
		break;

	case samrpc_delete_faults:
		_xdr_argument = xdr_fault_errorid_arr_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_delete_faults_1_svr;
		break;

	case samrpc_ack_faults:
		_xdr_argument = xdr_fault_errorid_arr_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_ack_faults_1_svr;
		break;

	case samrpc_get_fault_summary:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_fault_summary_1_svr;
		break;

	case samrpc_get_all_faults:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_faults_6_svr;
		break;

	/*
	 * diskvols.h
	 */
	case samrpc_get_disk_vol:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_disk_vol_1_svr;
		break;

	case samrpc_get_all_disk_vols:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_disk_vols_1_svr;
		break;

	case samrpc_get_all_clients:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_clients_1_svr;
		break;

	case samrpc_add_disk_vol:
		_xdr_argument = xdr_disk_vol_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_disk_vol_1_svr;
		break;

	case samrpc_remove_disk_vol:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_disk_vol_1_svr;
		break;

	case samrpc_add_client:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_client_1_svr;
		break;

	case samrpc_remove_client:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_client_1_svr;
		break;

	case samrpc_set_disk_vol_flags:
		_xdr_argument = xdr_string_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_disk_vol_flags_4_svr;
		break;

	/* notify_summary.h */

	case samrpc_get_notify_summary:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_notify_summary_1_svr;
		break;

	case samrpc_del_notify_summary:
		_xdr_argument = xdr_notify_summary_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_del_notify_summary_1_svr;
		break;

	case samrpc_mod_notify_summary:
		_xdr_argument = xdr_mod_notify_summary_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_mod_notify_summary_1_svr;
		break;

	case samrpc_add_notify_summary:
		_xdr_argument = xdr_notify_summary_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_notify_summary_1_svr;
		break;

	case samrpc_get_email_addrs_by_subj:
		_xdr_argument = xdr_get_email_addrs_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_email_addrs_by_subj_5_svr;
		break;

	/* recyc_sh_wrap.h */

	case samrpc_get_recycl_sh_action_status:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_recycl_sh_action_status_1_svr;
		break;

	case samrpc_add_recycle_sh_action_label:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_recycle_sh_action_label_1_svr;
		break;

	case samrpc_add_recycle_sh_action_export:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_recycle_sh_action_export_1_svr;
		break;

	case samrpc_del_recycle_sh_action:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_del_recycle_sh_action_1_svr;
		break;

	case samrpc_get_jobhist_by_fs:
		_xdr_argument = xdr_job_hist_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_jobhist_by_fs_1_svr;
		break;

	case samrpc_get_all_jobhist:
		_xdr_argument = xdr_job_type_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_all_jobhist_1_svr;
		break;

	case samrpc_get_host_config:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_host_config_3_svr;
		break;

	case samrpc_remove_host:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_host_3_svr;
		break;

	case samrpc_change_metadata_server:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_change_metadata_server_3_svr;
		break;

	case samrpc_add_host:
		_xdr_argument = xdr_string_host_info_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_add_host_3_svr;
		break;

	case samrpc_discover_ip_addresses:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_discover_ip_addresses_3_svr;
		break;

	/* restore.h */

	case samrpc_set_csd_params:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_csd_params_4_svr;
		break;

	case samrpc_get_csd_params:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_csd_params_4_svr;
		break;

	case samrpc_list_dumps:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_dumps_4_svr;
		break;

	case samrpc_list_dumps_by_dir:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_dumps_by_dir_4_svr;
		break;

	case samrpc_get_dump_status:
		_xdr_argument = xdr_string_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_dump_status_4_svr;
		break;

	case samrpc_get_dump_status_by_dir:
		_xdr_argument = xdr_string_string_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_dump_status_by_dir_4_svr;
		break;

	case samrpc_decompress_dump:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_decompress_dump_4_svr;
		break;

	case samrpc_cleanup_dump:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_cleanup_dump_4_svr;
		break;

	case samrpc_delete_dump:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_delete_dump_4_svr;
		break;

	case samrpc_take_dump:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_take_dump_4_svr;
		break;

	case samrpc_list_versions:
		_xdr_argument = xdr_file_restrictions_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_versions_4_svr;
		break;

	case samrpc_get_version_details:
		_xdr_argument = xdr_version_details_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_version_details_4_svr;
		break;

	case samrpc_search_versions:
		_xdr_argument = xdr_file_restrictions_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_search_versions_4_svr;
		break;

	case samrpc_get_search_results:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_search_results_4_svr;
		break;

	case samrpc_restore_inodes:
		_xdr_argument = xdr_restore_inodes_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_restore_inodes_4_svr;
		break;

	case samrpc_set_snapshot_locked:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_snapshot_locked_5_svr;
		break;

	case samrpc_clear_snapshot_locked:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_clear_snapshot_locked_5_svr;
		break;

	case samrpc_get_indexed_snapshot_directories:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local =
		    (char *(*)()) samrpc_get_indexed_snapshot_directories_6_svr;
		break;

	case samrpc_get_indexed_snapshots:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_indexed_snapshots_6_svr;
		break;

	case samrpc_list_activities:
		_xdr_argument = xdr_int_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_activities_4_svr;
		break;

	case samrpc_kill_activity:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_kill_activity_4_svr;
		break;

	/* file_util.h */

	case samrpc_create_dir:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_create_dir_1_svr;
		break;

	case samrpc_create_file:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_create_file_1_svr;
		break;

	case samrpc_file_exists:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_file_exists_3_svr;
		break;

	case samrpc_list_dir:
		_xdr_argument = xdr_file_restrictions_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_dir_4_svr;
		break;

	case samrpc_list_directory:
		_xdr_argument = xdr_file_restrictions_more_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_list_directory_6_svr;
		break;

	case samrpc_get_file_status:
		_xdr_argument = xdr_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_file_status_4_svr;
		break;

	case samrpc_get_file_details:
		_xdr_argument = xdr_string_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_file_details_4_svr;
		break;

	case samrpc_tail:
		_xdr_argument = xdr_string_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_tail_4_svr;
		break;

	case samrpc_get_txt_file:
		_xdr_argument = xdr_string_uint32_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_txt_file_5_svr;
		break;

	case samrpc_get_extended_file_details:
		_xdr_argument = xdr_strlst_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_extended_file_details_5_svr;
		break;

	case samrpc_list_and_collect_file_details:
		_xdr_argument = xdr_file_details_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local =
		    (char *(*)()) samrpc_list_and_collect_file_details_6_svr;
		break;

	case samrpc_collect_file_details:
		_xdr_argument = xdr_file_details_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local =
		    (char *(*)()) samrpc_collect_file_details_6_svr;
		break;

	case samrpc_get_copy_details:
		_xdr_argument = xdr_string_uint32_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_copy_details_5_svr;
		break;

	case samrpc_cns_get_registration:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_cns_get_registration_6_svr;
		break;

	case samrpc_cns_register:
		_xdr_argument = xdr_cns_reg_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_cns_register_6_svr;
		break;

	case samrpc_cns_get_public_key:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_cns_get_public_key_6_svr;
		break;

	case samrpc_delete_files:
		_xdr_argument = xdr_strlst_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_delete_files_6_svr;
		break;

	case samrpc_gen_report:
		_xdr_argument = xdr_report_requirement_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_gen_report_6_svr;
		break;

	case samrpc_get_file_metrics_report:
		_xdr_argument = xdr_file_metric_rpt_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_file_metrics_report_6_svr;
		break;

	case samrpc_get_task_schedules:
		_xdr_argument = xdr_ctx_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_task_schedules_6_svr;
		break;

	case samrpc_get_specific_tasks:
		_xdr_argument = xdr_string_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_get_specific_tasks_6_svr;
		break;

	case samrpc_set_task_schedule:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_set_task_schedule_6_svr;
		break;

	case samrpc_remove_task_schedule:
		_xdr_argument = xdr_string_arg_t;
		_xdr_result = xdr_samrpc_result_t;
		local = (char *(*)()) samrpc_remove_task_schedule_6_svr;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvccount--;

		return;
	}


	/*
	 * Check if client can make requests to this SAM-FS/QFS server
	 * Return an error message if the client is not in the list of
	 * clients in the config file
	 */
	if (!is_secure_client(inet_ntoa(in->sin_addr))) {
		_xdr_result = xdr_samrpc_result_t;
		samerrno = SE_RPC_INSECURE_CLIENT;
		(void) snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), inet_ntoa(in->sin_addr));
		Trace(TR_ERR, "%d:[%s]", samerrno, samerrmsg);

		SAMRPC_SET_RESULT(-1, SAM_VOID, 0);
		result = (caddr_t)&rpc_result;

		svc_sendreply(transp, _xdr_result, result);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, _xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		_rpcsvccount--;

		return;
	}
	result = (*local)(&argument, rqstp);

	if (result != NULL && !svc_sendreply(transp, _xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, _xdr_argument, (caddr_t)&argument)) {
		_msgout("unable to free arguments");
		exit(1);
	}
	_rpcsvccount--;

	/* return; */
}

int
main(int argc, char *argv[])
{

	int c;
	pid_t pid, ckpid;
	struct rlimit rlimits;


	if (geteuid() != 0) {
		(void) fprintf(stderr, "must be root to run %s\n", argv[0]);
		exit(1);
	}

	if (getppid() != 1) {	/* not started by init */

		while ((c = getopt(argc, argv, "vd")) != -1) {
			switch (c) {
			case 'v':
				(void) fprintf(stdout,
				    "SAM-QFS Manager daemon version:\t%s\n",
				    API_VERSION);
				exit(0);

				/* statement not reached */
				break;
			case 'd':		/* debugging mode */
				debug_mode = TRUE;
				break;
			case '?':
				(void) fprintf(stderr, "%s\n", USAGE);
				exit(1);

				/* statement not reached */
				break;
			}
		}

		if (argc != optind) {
			(void) fprintf(stderr, "%s\n", USAGE);
			exit(1);
		}

		/*
		 * Check if another instance of this process is already running
		 */

		ckpid = check_proc("fsmgmtd");
		if ((ckpid > 0) && (ckpid != getpid())) {
			(void) fprintf(stderr,
			    "SAM-FS Manager daemon is already running.\n");
			exit(0);
		}

	}

	(void) signal(SIGUSR1, sig_catcher);
	(void) signal(SIGINT, closedown);

	/* Init tracing, error and message facilities */
	init_utility_funcs();

	/* check if rpcbind is running */
	if (check_proc("rpcbind") == 0) {
		/* rpcbind not running */
		_msgout("Cannot register File System Manager daemon, "
		    "rpcbind not running. "
		    "Please start rpcbind with the command /usr/sbin/rpcbind");
		(void) sleep(60);
		exit(1);
	}

	Trace(TR_MISC, "RPC creating server handles for all transports");

	/*
	 * svc_create() registers itself with the rpcbind service
	 * If svc_create() succeeds, it returns the number of server
	 * handles created, otherwise it returns 0 and server is not
	 * registered with rpcbind.
	 * This could be used to check if rpcbind is responding
	 */
	if (!svc_create(sammgmtprog_1, SAMMGMTPROG, SAMMGMTVERS, NULL)) {

		/* write to trace file and log file */
		_msgout("unable to register the File System Manager daemon "
		    "(fsmgmtd) with rpcbind. rpcbind might not be responding "
		    "or there might be a network failure. "
		    "Please switch to superuser and restart rpcbind by "
		    "issuing a pkill -HUP rpcbind and restart fsmgmtd by "
		    "issuing "SBIN_DIR"/fsmadm restart");

		/*
		 * svc_create will fail typically if rpcbind or network
		 * resources are unavailable
		 * if the program exits, init will respawn it immediately
		 * thus causing a rapid respawn (more than 5 times per
		 * minute) and will not then respawn this process until
		 * at least five minutes have elapsed since the problem
		 * was identified
		 * To avoid this rapid respawn, idle for a few seconds
		 * before exiting
		 */
		(void) sleep(60);
		exit(1);
	}

	/* Registered with rpcbind successfully */
	Trace(TR_MISC, "Registered server process successful. Service ready.");

	/*
	 * raise limit of open file descriptors.
	 * sam-amld raised it to max, so we use the same number
	 */
	if (getrlimit(RLIMIT_NOFILE, &rlimits) != -1) {
		rlimits.rlim_cur = rlimits.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rlimits) < 0) {
			Trace(TR_ERR, "unable to setrlimit.");
		}
	} else {
		Trace(TR_ERR, "unable to retrieve resource limit.");
	}

	if (!debug_mode) {
		if (getppid() != 1) {	/* not started by init */
			/* Call fork and have the parent exit */
			if ((pid = fork()) < 0) {
				(void) fprintf(stderr, "Could not fork");
				exit(1);
			} else if (pid != 0) {
				exit(0);
			}	/* else child */

			/* child continues. */
			/* child is not a process group leader */
			(void) setsid();	/* become session leader */
			(void) chdir("/");	/* coredumps will be in /core */
			(void) umask(0);

			/* one last fork to make sure we're really standalone */
			pid = fork();
			if (pid != 0) {
				exit(0);
			};
		}
	}

	/*
	 * If standalone qfs, do not do discovery, otherwise
	 * discover all available media, don't wait,
	 */
	if (get_samfs_type(NULL) != QFS) {
		(void) discovery();
	}

	svc_run();
	_msgout("svc_run returned");
	return (1);
	/* NOTREACHED */
}


/*
 * discover all available media device
 *
 * In some cases with some libraries media discovery can take up to 5 minutes
 * after reboot. This time is unacceptable to the rpc client.
 * So before starting up the fsmgmtd server, discovery of media is done
 * so that subsequent discovery attempts from the GUI/rpc client will be
 * within the timeout(rpc client timeout) and not result in timeout errors
 */
static int
discovery(void)
{
	int	ret;
	int	pid;			/* for discovery */
	sqm_lst_t *lib_lst = NULL;		/* list of libraries */

	Trace(TR_MISC, "Media Discovery at server startup time");

	if ((pid = fork()) < 0) {
		Trace(TR_ERR, "fork failed:%s", Str(strerror(errno)));
		samerrno = SE_FORK_FAILED;
		(void) snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	if (pid == 0) {
		/* Child process */

		/*
		 * Discover direct-attached libraries that are not configured
		 * in mcf. Individual drives of the library are not discovered
		 * to avoid conflicts and race condition with sam-genericd.
		 * Results are not peristed, just exit
		 *
		 * Upon reboot, this media discovery should not cause a race
		 * condition with SAM's discovery of media. If there are lib
		 * configured in mcf and atleast one SAM-FS filesystem is
		 * mounted, sam-genericd attempts discovery of configured
		 * libraries.
		 *
		 */
		if ((ret = discover_media(NULL, &lib_lst)) != -1) {
			Trace(TR_MISC,
			    "Discovery of unconfigured library successful");

			/* deallocate memory */
			free_list_of_libraries(lib_lst);
		}
		Trace(TR_MISC, "Media discovery return [%d]", ret);
		/* if discovery failed, no action necessary */
		exit(0);
	}

	/* parent */
	(void) pthread_create(NULL, NULL, wait_child, (void *) pid);

	return (pid); /* no point in return ret */
}

/*
 * Helper function to remove <defunc> process
 * created by sammgmt_svc.c:discovery()
 */
static void *
wait_child(void *arg)
{

	int status;

	(void) pthread_detach(pthread_self());
	(void) waitpid((pid_t)arg, &status, 0);

	return (NULL);
}


/*
 * Checks if another instance of the process is running
 *
 * Returns :
 *	0	- another instance is not running
 *	-1	- internal error
 *	> 0	- the pid of the process found
 */
static int
check_proc(
char *proc	/* name of process */
)
{
	int status;
	sqm_lst_t *proclist = NULL;
	node_t *node;
	int ret = 0;

	Trace(TR_MISC, "Check if process is running entry");
	if (ISNULL(proc)) {
		Trace(TR_ERR, "Process running check failed: %s", samerrmsg);
		return (-1);
	}

	status = find_process(proc, &proclist);
	if ((status == 0) && (proclist != NULL)) {
		if (proclist->length > 0) {
			node = proclist->head;
			if (node != NULL) {
				ret = ((psinfo_t *)(node->data))->pr_pid;

				Trace(TR_DEBUG, "%s[%d] is already running",
				    proc, ret);
			}
		}
		lst_free_deep(proclist);
	}

	Trace(TR_MISC, "Check if process is running exit");

	return (ret);
}


static boolean_t
is_secure_client(
char *clientip
)
{
	sqm_lst_t *lst_ip = NULL;
	sqm_lst_t *lst_host = NULL;
	sqm_lst_t *lst_host2ip = NULL;
	node_t *node;

	if (ISNULL(clientip)) {
		Trace(TR_ERR, "%d:[%s]", samerrno, samerrmsg);
		return (B_FALSE);
	}

	Trace(TR_DEBUG, "Checking if client[%s] is a secure client", clientip);
	/* Get all clients that can access this fsmgmtd */
	if (get_clients(&lst_ip, &lst_host) < 0) {
		Trace(TR_ERR, "%d:[%s]", samerrno, samerrmsg);
		return (B_FALSE);
	}

	/* convert all hostnames in cfg to ip addresses */
	node = lst_host->head;
	while (node != NULL) {
		if (clienthost2ip((char *)node->data, &lst_host2ip) == 0) {
			lst_concat(lst_ip, lst_host2ip);
		}
		node = node->next;
	}

	node = lst_ip->head;
	while (node != NULL) {
		if (strcmp(clientip, (char *)node->data) == 0) {
			Trace(TR_DEBUG, "Client[%s] is an authorized client",
			    clientip);
			lst_free_deep(lst_ip);
			lst_free_deep(lst_host);
			return (B_TRUE);
		}
		node = node->next;
	}

	lst_free_deep(lst_ip);
	lst_free_deep(lst_host);

	Trace(TR_DEBUG, "Client[%s] is not an authorized client", clientip);
	return (B_FALSE);
}


static int
clienthost2ip(
char *clienthost,
sqm_lst_t **lst
)
{

	struct hostent *hp = NULL;
	char **p;
	int error_num;

	Trace(TR_DEBUG, "Client hostname[%s] to IP conv", clienthost);

	if (ISNULL(clienthost, lst)) {

		Trace(TR_ERR, "%d:[%s]", samerrno, samerrmsg);
		return (-1);
	}

	hp = getipnodebyname(clienthost, AF_INET, 0, &error_num);

	/* IP4 addressing */
	if (hp == NULL) {

		/* IP6 addressing */
		hp = getipnodebyname(clienthost, AF_INET6, 0, &error_num);

		if (hp == NULL) {
			/* TBD: give a specific error message */
			/* Host is unknown */
			samerrno = SE_RPC_UNKNOWN_CLIENT;
			(void) snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_ERR, "%d:[%s]", samerrno, samerrmsg);
			return (-1);
		}
	}

	*lst = lst_create();

	for (p = hp->h_addr_list; *p != 0; p++) {
		if (hp->h_addrtype == AF_INET6) {

			/* address is IPv6 (128 bits) */
			struct in6_addr in;
			char buf[INET6_ADDRSTRLEN];

			bcopy(*p, (caddr_t)&in, hp->h_length);
			lst_append(*lst, strdup(inet_ntop(
			    AF_INET6, (void *)&in, buf, sizeof (buf))));
		} else {
			struct in_addr in;
			char buf[INET_ADDRSTRLEN];

			bcopy(*p, (caddr_t)&in, hp->h_length);
			lst_append(*lst, strdup(inet_ntop(
			    AF_INET, (void *)&in, buf, sizeof (buf))));
		}
	}
	freehostent(hp);
	Trace(TR_DEBUG, "Converted hostname[%s] to IP[%s]",
	    clienthost,
	    ((*lst)->length > 0) ? (char *)(((sqm_lst_t *)(*lst))->head->data) :
	    "-");
	return (0);
}
