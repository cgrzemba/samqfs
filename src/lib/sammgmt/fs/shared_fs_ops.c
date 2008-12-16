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
#pragma ident   "$Revision: 1.7 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>

#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/hosts.h"
#include "mgmt/config/cfg_hosts.h"
#include "mgmt/control/fscmd.h"
#include "mgmt/cmd_dispatch.h"
#include "mgmt/sammgmt.h"
#include "mgmt/util.h"

#include "sam/sam_trace.h"
#include "sam/mount.h"

typedef struct add_host_opts {
	upath_t	mnt_point;
	boolean_t mount_fs;
	boolean_t bg_mount;
	boolean_t read_only;
	boolean_t mount_at_boot;
	boolean_t pmds;
} add_host_opts_t;

static int create_fs_on_clients(ctx_t *ctx, fs_t *fs, sqm_lst_t *new_hosts,
    add_host_opts_t *opts);

static int remove_fs_on_clients(ctx_t *ctx, char *fs_name,
    char **client_names, int client_count);

static int compare_host_names(host_info_t *a, host_info_t *b);
static int post_grow_shared_fs(dispatch_job_t *dj);


#define	host_offset(name) (offsetof(add_host_opts_t, name))

static parsekv_t add_hosts_kvoptions[] = {
	{"mount_point",	host_offset(mnt_point), parsekv_string_1024},
	{"mount_fs",	host_offset(mount_fs), parsekv_bool_YN},
	{"bg_mount",	host_offset(bg_mount), parsekv_bool_YN},
	{"read_only",	host_offset(read_only), parsekv_bool_YN},
	{"mount_at_boot", host_offset(mount_at_boot), parsekv_bool_YN},
	{"potential_mds", host_offset(pmds), parsekv_bool_YN},
	{"", 0, NULL}
};


/*
 * Function to add multiple clients to a shared file system. This
 * function may be run to completion in the background.
 *
 * kv_options is a key value string supporting the following options:
 * mount_point="/fully/qualified/path"
 * mount_fs= yes | no
 * mount_at_boot = yes | no
 * bg_mount = yes | no
 * read_only = yes | no
 * potential_mds = yes | no
 *
 * Returns:
 * 0 for successful completion
 * -1 for error
 * job_id will be returned if the job has not completed.
 */
int
add_hosts(
ctx_t *c,
char *fs_name,
sqm_lst_t *new_hosts,
char *kv_opts) {

	fs_t *fs;
	sqm_lst_t *cfg_hosts;
	node_t *n;
	int ret_val;
	boolean_t mounted = B_FALSE;
	add_host_opts_t opts;
	int high_srv_prio = 0;

	if (ISNULL(fs_name, new_hosts)) {
		Trace(TR_ERR, "adding hosts failed: %s",
		    samerrmsg);
		return (-1);
	}

	memset(&opts, 0, sizeof (add_host_opts_t));
	if (kv_opts != NULL &&
	    parse_kv(kv_opts, add_hosts_kvoptions, &opts) != 0) {
		Trace(TR_ERR, "adding hosts failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* Note that if -2 is returned by get_fs you can continue. */
	if (get_fs(NULL, fs_name, &fs) == -1) {
		Trace(TR_ERR, "adding hosts failed:%s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		free_fs(fs);
		Trace(TR_ERR, "adding hosts failed:%s", samerrmsg);
		return (-1);
	}

	if (!(fs->fi_status & FS_SERVER)) {
		setsamerr(SE_ADD_HOSTS_ON_MDS);
		Trace(TR_ERR, "adding hosts failed:%s", samerrmsg);
		free_fs(fs);
		return (-1);
	}

	if (cfg_get_hosts_config(c, fs, &cfg_hosts, 0) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "adding hosts failed:%s", samerrmsg);
		return (-1);
	}

	if (opts.pmds) {
		/* Find the current highest server priority. */
		for (n = cfg_hosts->head; n != NULL; n = n->next) {
			host_info_t *h = (host_info_t *)n->data;
			if (h->server_priority > high_srv_prio) {
				high_srv_prio = h->server_priority;
			}
		}
	}

	for (n = new_hosts->head; n != NULL; n = n->next) {
		node_t *sn;
		host_info_t *h = (host_info_t *)n->data;

		sn = lst_search(cfg_hosts, h, (lstsrch_t)compare_host_names);
		if (sn != NULL) {
			/* already present, don't add it */
			continue;
		} else {
			host_info_t *copy;

			copy = dup_host(h);
			if (copy == NULL) {
				free_list_of_host_info(cfg_hosts);
				free_fs(fs);
				Trace(TR_ERR, "adding hosts failed:%s",
				    samerrmsg);
				return (-1);
			}

			if (opts.pmds) {
				/*
				 * Assign the next highest priorty
				 * to this server.
				 */
				copy->server_priority = ++high_srv_prio;
			}

			if (lst_append(cfg_hosts, copy) != 0) {
				free_list_of_host_info(cfg_hosts);
				free_fs(fs);
				Trace(TR_ERR, "adding hosts failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	fs->hosts_config = cfg_hosts;

	/* set the new config */
	if (cfg_set_hosts_config(c, fs) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "adding hosts failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * free the lists of hosts and set it to NULL in the fs since
	 * it is no longer needed
	 */
	free_list_of_host_info(cfg_hosts);
	fs->hosts_config = NULL;

	if (fs->fi_status & FS_MOUNTED) {
		mounted = B_TRUE;
	}

	/* activate the new config */
	if (sharefs_cmd(fs->fi_name, NULL, B_TRUE, mounted,
	    B_FALSE) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "adding hosts failed: %s",
		    samerrmsg);

		return (-1);
	}

	ret_val = create_fs_on_clients(c, fs, new_hosts, &opts);
	Trace(TR_ERR, "adding hosts returning: %d", ret_val);

	return (ret_val);
}


static int
create_fs_on_clients(ctx_t *ctx, fs_t *fs, sqm_lst_t *new_hosts,
    add_host_opts_t *opts) {

	char **host_names;
	create_arch_fs_arg_t *cafs;
	node_t *n;
	int i = 0;
	int job_id;

	if (ISNULL(fs, new_hosts, opts)) {
		Trace(TR_ERR, "creating fs on hosts failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Make an array of the new clients to use as input to the
	 * multiplex function.
	 */
	host_names = (char **)calloc(new_hosts->length, sizeof (char *));
	if (host_names == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "creating fs on hosts failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	for (n = new_hosts->head; n != NULL; n = n->next) {
		host_info_t *h = n->data;

		host_names[i] = copystr(h->host_name);
		if (host_names[i] == NULL) {
			free_string_array(host_names, new_hosts->length);
			Trace(TR_ERR, "creating fs on hosts failed: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
		i++;
	}

	/*
	 * Malloc the args so they can live beyond the scope of this function
	 */
	cafs = (create_arch_fs_arg_t *)mallocer(sizeof (create_arch_fs_arg_t));
	if (cafs == NULL) {
		free_string_array(host_names, new_hosts->length);
		Trace(TR_ERR, "creating fs on hosts failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	cafs->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
	if (cafs->ctx == NULL) {
		free_dispatch_func_args(cafs, CMD_CREATE_ARCH_FS);
		free_string_array(host_names, new_hosts->length);
		Trace(TR_ERR, "creating fs on hosts failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	cafs->ctx->handle = NULL;
	cafs->ctx->dump_path[0] = '\0';
	cafs->ctx->read_location[0] = '\0';
	cafs->ctx->user_id[0] = '\0';

	cafs->fs_info = dup_fs(fs);
	if (cafs->fs_info == NULL) {
		free_string_array(host_names, new_hosts->length);
		free_dispatch_func_args(cafs, CMD_CREATE_ARCH_FS);
		Trace(TR_ERR, "creating fs on hosts failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Clear the server bit and set the fs info to be a client. If
	 * the opts structure does not have the pmds flag set, set
	 * the nodevs flag. NOTE that potential metadata servers do
	 * have their FS_CLIENT flag set.
	 */
	cafs->fs_info->fi_status &= ~FS_SERVER;
	cafs->fs_info->fi_status |= FS_CLIENT;
	if (!opts->pmds) {
		cafs->fs_info->fi_status |= FS_NODEVS;

		/* set the metadata devices to nodev */
		if (cafs->fs_info->meta_data_disk_list != NULL) {
			for (n = cafs->fs_info->meta_data_disk_list->head;
				n != NULL; n = n->next) {

				disk_t *dsk = (disk_t *)n->data;
				strlcpy(dsk->base_info.name, NODEV_STR,
				    sizeof (upath_t));
			}
		}
	}

	/*
	 * If opts has a mount point specified, use it. Otherwise use
	 * the mount point that is already in the fs.
	 */
	if (*opts->mnt_point != '\0') {
		strlcpy(cafs->fs_info->fi_mnt_point, opts->mnt_point,
		    sizeof (upath_t));
	}

	cafs->mount = opts->mount_fs;
	cafs->fs_info->mount_options->sharedfs_opts.bg = opts->bg_mount;
	cafs->fs_info->mount_options->readonly = opts->read_only;
	cafs->mount_at_boot = opts->mount_at_boot;
	cafs->create_mnt_point = B_TRUE;

	cafs->arch_cfg = NULL;

	Trace(TR_DEBUG, "About to multiplex request to create fs on clients");
	job_id = multiplex_request(ctx, CMD_CREATE_ARCH_FS, host_names,
	    new_hosts->length, (void *)cafs, NULL);
	if (job_id == -1) {
		free_string_array(host_names, new_hosts->length);
		free_dispatch_func_args(cafs, CMD_CREATE_ARCH_FS);

		Trace(TR_MISC, "Creating fs on clients failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "Creating fs on clients returning: %d", job_id);
	return (job_id);
}


/*
 * The file system must be unmounted to remove clients. You can
 * disable access from clients without unmounting the file system
 * with the setClientState function.
 *
 * Returns: 0, 1, job ID
 * 0 for successful completion
 * -1 for error
 * job ID will be returned if the job has not completed.
 */
int remove_hosts(
ctx_t *c,
char *fs_name,
char *host_names[],
int host_count) {

	fs_t *fs;
	sqm_lst_t *cfg_hosts;
	int ret_val;
	int i;


	Trace(TR_MISC, "remove hosts entry");
	if (ISNULL(fs_name, host_names)) {
		Trace(TR_ERR, "remove clients failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* Note that if -2 is returned by get_fs you can continue. */
	if (get_fs(NULL, fs_name, &fs) == -1) {
		Trace(TR_ERR, "remove hosts failed:%s", samerrmsg);
		return (-1);
	}

	if (!fs->fi_shared_fs) {
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), fs_name);
		free_fs(fs);
		Trace(TR_ERR, "remove hosts failed:%s", samerrmsg);
		return (-1);
	}

	if (!(fs->fi_status & FS_SERVER)) {
		setsamerr(SE_REMOVE_HOSTS_ON_MDS);
		Trace(TR_ERR, "remove hosts failed:%s", samerrmsg);
		free_fs(fs);
		return (-1);
	}

	/* cannot remove hosts unless fs is unmounted */
	if (fs->fi_status & FS_MOUNTED) {
		setsamerr(SE_FS_MNTD_CANT_DEL_CLT);
		Trace(TR_ERR, "remove hosts failed:%s", samerrmsg);
		free_fs(fs);
		return (-1);
	}

	if (cfg_get_hosts_config(c, fs, &cfg_hosts, 0) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "remove hosts failed:%s", samerrmsg);
		return (-1);
	}

	/* find and remove the hosts from the hosts config */
	for (i = 0; i < host_count; i++) {
		node_t *sn;

		if (host_names[i] == NULL) {
			continue;
		}
		sn = lst_search(cfg_hosts, host_names[i],
		    (lstsrch_t)cmp_str_2_str_ptr);
		if (sn == NULL) {
			/* not present don't complain just continue */
			continue;
		} else {
			if (lst_remove(cfg_hosts, sn) != 0) {
				free_list_of_host_info(cfg_hosts);
				free_fs(fs);
				Trace(TR_ERR, "remove hosts failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	fs->hosts_config = cfg_hosts;

	/* set the new config */
	if (cfg_set_hosts_config(c, fs) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "remove hosts failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_list_of_host_info(cfg_hosts);
	fs->hosts_config = NULL;

	/* activate the new config */
	if (sharefs_cmd(fs->fi_name, NULL, B_TRUE, B_FALSE,
	    B_FALSE) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "remove hosts failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* call out to the clients to remove the fs from their config */
	ret_val = remove_fs_on_clients(c, fs_name, host_names, host_count);
	Trace(TR_ERR, "remove hosts returning: %d", ret_val);

	return (ret_val);
}


static int
remove_fs_on_clients(ctx_t *ctx, char *fs_name,
    char **client_names, int client_count) {

	int job_id;
	string_arg_t *sat;
	char **the_hosts;
	int host_count;
	boolean_t found_local_host;


	if (ISNULL(fs_name, client_names)) {
		Trace(TR_ERR, "remove fs on clients failed: %s",
		    samerrmsg);
		return (-1);
	}


	Trace(TR_ERR, "removing fs on clients for %s", fs_name);

	if (client_count == 0) {
		setsamerr(SE_DSP_NO_CLIENTS);
		Trace(TR_ERR, "remove fs on clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Make a copy of the input clients array so that the one passed
	 * to the command dispatcher does not get freed when this
	 * function returns.
	 */
	host_count = convert_dispatch_hosts_array(client_names,
	    client_count, &the_hosts, &found_local_host);

	if (host_count == -1) {
		Trace(TR_ERR, "remove fs on clients failed: %s",
		    samerrmsg);
		return (-1);
	}


	/*
	 * Malloc arguments so they can live beyond the scope of this
	 * function
	 */
	sat = (string_arg_t *)mallocer(sizeof (string_arg_t));
	if (sat == NULL) {
		free_string_array(the_hosts, host_count);
		return (-1);
	}

	sat->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
	if (ctx == NULL) {
		free_string_array(the_hosts, host_count);
		free(sat);
		return (-1);
	}
	sat->ctx->handle = NULL;
	sat->ctx->dump_path[0] = '\0';
	sat->ctx->read_location[0] = '\0';
	sat->ctx->user_id[0] = '\0';
	sat->str = copystr(fs_name);
	if (sat->str == NULL) {
		free_string_array(the_hosts, host_count);
		free(sat->ctx);
		free(sat);
		return (-1);
	}
	Trace(TR_MISC, "About to multiplex request to remove fs on clients");
	job_id = multiplex_request(ctx, CMD_REMOVE_FS, the_hosts, host_count,
	    (void *)sat, NULL);
	if (job_id == -1) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(sat, CMD_MOUNT);

		Trace(TR_MISC, "remove fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "remove fs on clients returning: %d", 0);
	return (job_id);
}


static int
compare_host_names(host_info_t *a, host_info_t *b) {
	return (strcmp(a->host_name, b->host_name));
}


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
mount_clients(
ctx_t *ctx,
char *fs_name,
char *clients[],
int client_count) {


	int job_id = 0;
	string_arg_t *sat;
	char **the_hosts;
	int host_count;
	boolean_t found_local_host;


	if (ISNULL(fs_name, clients)) {
		Trace(TR_ERR, "mount clients failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (client_count == 0) {
		setsamerr(SE_DSP_NO_CLIENTS);
		Trace(TR_ERR, "mount clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Make a copy of the input clients array so that the one passed
	 * to the command dispatcher does not get freed when this
	 * function returns.
	 */
	host_count = convert_dispatch_hosts_array(clients,
	    client_count, &the_hosts, &found_local_host);

	if (host_count == -1) {
		Trace(TR_ERR, "mount clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (found_local_host) {
		/*
		 * Do the operation here on the metadata server first. If it
		 * fails don't call the other hosts.
		 */
		if (mount_fs(ctx, fs_name) != 0) {
			free_string_array(the_hosts, host_count);
			Trace(TR_ERR, "Mount Clients failed on the mds: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	/*
	 * Return if no work remains after calling the local host
	 */
	if (host_count == 0) {
		/* no hosts array to free since count == 0 */
		Trace(TR_MISC, "mounting clients done on the mds");
		return (0);
	}

	/*
	 * Malloc arguments so they can live beyond the scope of this
	 * function
	 */
	sat = (string_arg_t *)mallocer(sizeof (string_arg_t));
	if (sat == NULL) {
		free_string_array(the_hosts, host_count);
		return (-1);
	}

	sat->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
	if (ctx == NULL) {
		free_string_array(the_hosts, host_count);
		free(sat);
		return (-1);
	}
	sat->ctx->handle = NULL;
	sat->ctx->dump_path[0] = '\0';
	sat->ctx->read_location[0] = '\0';
	sat->ctx->user_id[0] = '\0';
	sat->str = copystr(fs_name);
	if (sat->str == NULL) {
		free_string_array(the_hosts, host_count);
		free(sat->ctx);
		free(sat);
		return (-1);
	}
	Trace(TR_MISC, "Before multiplex");
	job_id = multiplex_request(ctx, CMD_MOUNT, the_hosts, host_count,
	    (void *)sat, NULL);
	if (job_id == -1) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(sat, CMD_MOUNT);

		Trace(TR_MISC, "mount fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "mounting clients returning: %d", 0);
	return (job_id);
}




/*
 * Will unmount the named file system on this host. The caller of multiplex
 * request is responsible for determining that the file system should be
 * unmounted on this host- if it should not be then no post_phase function
 * should be specified.
 */
static int
post_unmount_clients(dispatch_job_t *dj) {

	int i;
	boolean_t hosts_succeeded = B_TRUE;

	Trace(TR_MISC, "Entering unmount post phase");

	/* Check that the hosts passed in all successfully unmounted */
	for (i = 0; i < dj->host_count; i++) {
		if (dj->responses[i].status != OP_SUCCEEDED) {
			hosts_succeeded = B_FALSE;
			break;
		}
	}


	if (hosts_succeeded) {
		string_bool_arg_t *args = (string_bool_arg_t *)dj->args;
		if (umount_fs(args->ctx, args->str, args->bool) != 0) {
			setsamerr(SE_MOUNT_MDS_FAILED);
			Trace(TR_MISC, "unmount post phase returning -1");
			return (-1);
		}
	}
	Trace(TR_MISC, "unmount post phase returning 0");
	return (0);
}


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
unmount_clients(
ctx_t *ctx,
char *fs_name,
char *clients[],
int client_count) {


	int job_id = 0;
	string_bool_arg_t *sbat;
	char **the_hosts = NULL;
	int host_count;
	boolean_t found_local_host;


	if (ISNULL(fs_name, clients)) {
		Trace(TR_ERR, "unmount clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (client_count == 0) {
		setsamerr(SE_DSP_NO_CLIENTS);
		Trace(TR_ERR, "unmount clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Make a copy of the input clients array so that the one passed
	 * to the command dispatcher does not get freed when this
	 * function returns.
	 */
	host_count = convert_dispatch_hosts_array(clients,
	    client_count, &the_hosts, &found_local_host);

	if (host_count == -1) {
		Trace(TR_ERR, "unmount clients failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (host_count != 0) {

		/*
		 * Malloc arguments so they can live beyond the scope
		 * of this function. Note these should never be freed
		 * in this function unless multiplex_request returns -1
		 */
		sbat = (string_bool_arg_t *)mallocer(
			sizeof (string_bool_arg_t));
		if (sbat == NULL) {
			free_string_array(the_hosts, host_count);
			Trace(TR_ERR, "unmounting clients failed: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
		sbat->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
		sbat->ctx->handle = NULL;
		sbat->ctx->dump_path[0] = '\0';
		sbat->ctx->read_location[0] = '\0';
		sbat->ctx->user_id[0] = '\0';
		sbat->str = copystr(fs_name);
		sbat->bool = B_FALSE;

		if (found_local_host) {
			/* Setup the post phase call */
			job_id = multiplex_request(ctx, CMD_UMOUNT, the_hosts,
			    host_count, (void *)sbat, post_unmount_clients);

		} else {
			job_id = multiplex_request(ctx, CMD_UMOUNT, the_hosts,
			    host_count, (void *)sbat, NULL);
		}
		if (job_id == -1) {

			free_string_array(the_hosts, host_count);
			free_dispatch_func_args(sbat, CMD_UMOUNT);

			Trace(TR_ERR, "unmounting clients failed: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	} else if (found_local_host) {
		/*
		 * Unmount of the file system on the metadata server
		 * must be done after the clients are unmounted. So
		 * unmounting the fs on the mds when other hosts are
		 * specified is generally done with the post phase
		 * handler.
		 *
		 * This else if handles the case where the local host is
		 * the only host.
		 */
		if (umount_fs(ctx, fs_name, B_FALSE) != 0) {
			setsamerr(SE_UNMOUNT_MDS_FAILED);
			Trace(TR_ERR, "Unount clients failed on the mds: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	Trace(TR_MISC, "unmounting clients returning %d", 0);
	return (job_id);
}


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
change_shared_fs_mount_options(
ctx_t *ctx,
char *fs_name,
char *clients[],
int client_count,
mount_options_t *mo) {


	int job_id;
	change_mount_options_arg_t *cmoarg;
	char **the_hosts;
	int host_count;
	boolean_t found_local_host;

	if (ISNULL(fs_name, clients, mo)) {
		Trace(TR_ERR, "change shared fs mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (client_count == 0) {
		setsamerr(SE_DSP_NO_CLIENTS);
		Trace(TR_ERR, "change shared mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Make a copy of the input clients array so that the one passed
	 * to the command dispatcher does not get freed when this
	 * function returns.
	 */
	host_count = convert_dispatch_hosts_array(clients,
	    client_count, &the_hosts, &found_local_host);

	if (host_count == -1) {
		Trace(TR_ERR, "changing shared fs mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (found_local_host) {
		/*
		 * Do the operation here on the metadata server first. If it
		 * fails don't call the other hosts- since they would likely
		 * just fail too.
		 */
		if (change_mount_options(ctx, fs_name, mo) != 0) {
			/*
			 * Don't mask the error from trying to set the mount
			 * options with with a more generic one from this
			 * function.
			 */
			free_string_array(the_hosts, host_count);
			Trace(TR_ERR, "Changing shared mount options failed"
			    " on the mds: %d %s", samerrno, samerrmsg);
			return (-1);
		}
	}

	/*
	 * client count could have been non-zero and resulted in a zero
	 * host_count if the mds was specified or NULL entries were present
	 * in the array. At this point the local host was called so return
	 * if no work remains.
	 */
	if (host_count == 0) {
		/* no hosts array to free since count == 0 */
		Trace(TR_MISC, "Changing shared mount options done"
		    " on the mds");
		return (0);
	}

	/*
	 * Malloc arguments so they can live beyond the scope of this
	 * function
	 */
	cmoarg = (change_mount_options_arg_t *)mallocer(
		sizeof (change_mount_options_arg_t));
	if (cmoarg == NULL) {
		free_string_array(the_hosts, host_count);
		Trace(TR_ERR, "changing shared fs mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	cmoarg->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
	cmoarg->ctx->handle = NULL;
	cmoarg->ctx->dump_path[0] = '\0';
	cmoarg->ctx->read_location[0] = '\0';
	cmoarg->ctx->user_id[0] = '\0';
	strncpy(cmoarg->fsname, fs_name, sizeof (uname_t));

	cmoarg->options = dup_mount_options(mo);
	if (cmoarg->options == NULL) {
		free_string_array(the_hosts, host_count);
		free(cmoarg);
		Trace(TR_ERR, "changing shared fs mount options failed: %s",
		    samerrmsg);
		return (-1);
	}


	Trace(TR_DEBUG, "Call the dispatcher for change mount options ");
	job_id = multiplex_request(ctx, CMD_CHANGE_MOUNT_OPTIONS,
	    the_hosts, host_count, (void *)cmoarg, NULL);
	if (job_id == -1) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(cmoarg, CMD_CHANGE_MOUNT_OPTIONS);

		Trace(TR_ERR, "Changing shared mount options failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "changing shared fs mount options returning 0");
	return (job_id);
}

/*
 * Function to grow the shared file system on the clients after it
 * has been grown on the mds.
 */
int
grow_shared_fs_on_clients(ctx_t *ctx, fs_t *fs, sqm_lst_t *new_meta,
    sqm_lst_t *new_data, sqm_lst_t *new_stripe_groups) {

	int job_id = 0;
	grow_fs_arg_t *gfs_arg;
	char **the_hosts;
	int host_count;

	if (ISNULL(fs)) {
		Trace(TR_ERR, "grow shared on clients failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* Get a list of the participating hosts for the fs */
	host_count = make_host_name_array(fs, &the_hosts);
	if (host_count == -1) {
		Trace(TR_MISC, "grow shared on clients failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Return if no work remains after calling the local host. It is
	 * not an error for there to be no clients of a shared file system.
	 */
	if (host_count == 0) {
		/* no hosts array to free since count == 0 */
		Trace(TR_MISC, "mounting clients done on the mds");
		return (0);
	}

	/*
	 * Malloc arguments so they can live beyond the scope of this
	 * function
	 */
	gfs_arg = (grow_fs_arg_t *)mallocer(sizeof (grow_fs_arg_t));
	if (gfs_arg == NULL) {
		free_string_array(the_hosts, host_count);
		return (-1);
	}

	gfs_arg->ctx = (ctx_t *)mallocer(sizeof (ctx_t));
	gfs_arg->ctx->handle = NULL;
	gfs_arg->ctx->dump_path[0] = '\0';
	gfs_arg->ctx->read_location[0] = '\0';
	gfs_arg->ctx->user_id[0] = '\0';
	gfs_arg->fs = dup_fs(fs);
	if (gfs_arg->fs == NULL) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(gfs_arg, CMD_GROW_FS);
		Trace(TR_MISC, "Grow fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	if (lst_dup_typed(new_meta, &(gfs_arg->additional_meta_data_disk),
	    DUPFUNCCAST(dup_disk), FREEFUNCCAST(free_disk)) != 0) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(gfs_arg, CMD_GROW_FS);
		Trace(TR_MISC, "Grow fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}
	if (lst_dup_typed(new_data, &(gfs_arg->additional_data_disk),
	    DUPFUNCCAST(dup_disk), FREEFUNCCAST(free_disk)) != 0) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(gfs_arg, CMD_GROW_FS);
		Trace(TR_MISC, "Grow fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	if (lst_dup_typed(new_stripe_groups,
	    &(gfs_arg->additional_striped_group),
	    DUPFUNCCAST(dup_striped_group),
	    FREEFUNCCAST(free_striped_group)) != 0) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(gfs_arg, CMD_GROW_FS);
		Trace(TR_MISC, "Grow fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "Before multiplex");
	job_id = multiplex_request(ctx, CMD_GROW_FS, the_hosts, host_count,
	    (void *)gfs_arg, post_grow_shared_fs);
	if (job_id == -1) {
		free_string_array(the_hosts, host_count);
		free_dispatch_func_args(gfs_arg, CMD_GROW_FS);

		Trace(TR_MISC, "Grow fs on clients failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "grow fs on clients returning: %d", job_id);
	return (job_id);
}


/*
 * Function to be called after a grow has been issued to clients.
 */
static int
post_grow_shared_fs(dispatch_job_t *dj) {

	grow_fs_arg_t *gfs_arg = (grow_fs_arg_t *)dj->args;
	sqm_lst_t *lst;
	node_t *n;
	int i;

	/*
	 * Figure out if the grow worked on all hosts. If so
	 * enable allocation on the device. If not set
	 * the partial failure message.
	 */
	for (i = 0; i < dj->host_count; i++) {
		if (dj->responses[i].status != OP_SUCCEEDED) {
			setsamerr(SE_CLIENTS_FAILED_TO_GROW);
			Trace(TR_ERR, "grow shared fs post phase failed: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	/*
	 * get the equipment ordinals for the added devices and
	 * call alloc for them.
	 */
	lst = lst_create();
	if (lst == NULL) {
		Trace(TR_ERR, "post phase for grow shared fs failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	if (gfs_arg->additional_meta_data_disk != NULL) {

		for (n = gfs_arg->additional_meta_data_disk->head;
			n != NULL; n = n->next) {

			disk_t *d = (disk_t *)n->data;
			int *eq;

			if (d == NULL) {
				continue;
			}
			eq = mallocer(sizeof (int));
			if (eq == NULL) {
				lst_free_deep(lst);
				return (-1);
			}
			*eq = d->base_info.eq;

			if (lst_append(lst, eq) != 0) {
				lst_free_deep(lst);
				Trace(TR_ERR, "post phase for grow "
				    "shared fs failed: %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
	}

	if (gfs_arg->additional_data_disk != NULL) {
		for (n = gfs_arg->additional_data_disk->head;
			n != NULL; n = n->next) {

			disk_t *d = (disk_t *)n->data;
			int *eq;

			if (d == NULL) {
				continue;
			}
			eq = mallocer(sizeof (int));
			if (eq == NULL) {
				lst_free_deep(lst);
				return (-1);
			}
			*eq = d->base_info.eq;

			if (lst_append(lst, eq) != 0) {
				lst_free(lst);
				Trace(TR_ERR, "post phase for grow "
				    "shared fs failed: %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
	}

	if (gfs_arg->additional_striped_group != NULL) {
		for (n = gfs_arg->additional_striped_group->head;
			n != NULL; n = n->next) {

			striped_group_t *sg;
			disk_t *d;
			int *eq;

			sg = (striped_group_t *)n->data;
			if (sg == NULL || sg->disk_list == NULL) {
				continue;
			}
			/* Only need the first disk from the striped group */
			d = (disk_t *)sg->disk_list->head->data;
			if (d == NULL) {
				continue;
			}

			eq = mallocer(sizeof (int));
			if (eq == NULL) {
				lst_free_deep(lst);
				return (-1);
			}
			*eq = d->base_info.eq;
			if (lst_append(lst, eq) != 0) {
				lst_free(lst);
				Trace(TR_ERR, "post phase for grow "
				    "shared fs failed: %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
	}

	if (lst->length == 0) {
		Trace(TR_MISC, "post phase for grow "
		    "shared fs did not find any devices");
		return (0);
	}

	if (set_device_state(NULL, gfs_arg->fs->fi_name, DEV_ON, lst) != 0) {
		setsamerr(SE_ALLOC_AFTER_GROW_FAILED);
		lst_free(lst);
		Trace(TR_ERR, "grow shared fs post phase failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);

	}

	Trace(TR_MISC, "grow shared fs post phase succeeded");

	lst_free(lst);
	return (0);
}


int
get_shared_fs_summary_status(ctx_t *c, char *fs_name, sqm_lst_t **lst) {
	char buf[64];
	sqm_lst_t *host_lst;
	node_t *n;
	char *str;
	int clients;
	int clients_off = 0;
	int pmds = 0;
	int pmds_off = 0;


	if (ISNULL(fs_name, lst)) {
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}
	*lst = lst_create();
	if (*lst == NULL) {
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (get_host_config(c, fs_name, &host_lst) != 0) {
		lst_free(*lst);
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}

	clients = host_lst->length;
	for (n = host_lst->head; n != NULL; n = n->next) {
		host_info_t *h = n->data;
		if (h == NULL) {
			clients--;
			continue;
		}
		if (h->server_priority) {
			pmds++;
			clients--;
			if (h->state == CL_STATE_OFF) {
				pmds_off++;
			}
		} else if (h->state == CL_STATE_OFF) {
			clients_off++;
		}
	}

	free_list_of_host_info(host_lst);


	snprintf(buf, sizeof (buf), "clients=%d,unmounted=0,"
	    "error=0,off=%d", clients, clients_off);
	str = copystr(buf);
	if (str == NULL) {
		lst_free_deep(*lst);
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (lst_append(*lst, str) != 0) {
		lst_free_deep(*lst);
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}

	snprintf(buf, sizeof (buf), "pmds=%d,unmounted=0,"
	    "error=0,off=%d", pmds, pmds_off);
	str = copystr(buf);
	if (str == NULL) {
		lst_free_deep(*lst);
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (lst_append(*lst, str) != 0) {
		lst_free_deep(*lst);
		Trace(TR_ERR, "get shared fs summary status failed: %s",
		    samerrmsg);
		return (-1);
	}

	return (0);
}
