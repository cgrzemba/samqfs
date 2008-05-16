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
#pragma ident "	$Revision: 1.22 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/types.h"

#include "mgmt/util.h"
#include "pub/mgmt/device.h"
#include <sys/vfs.h>
#include "sam/param.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"
#include "sam/fs/sblk_mgmt.h"
#include "mgmt/config/master_config.h"

static int reorder_mcf_devs(struct sam_sblk sblk, node_t *fam_set);

static int compare_super_blocks(struct sam_sblk *a, char *a_name,
	struct sam_sblk *b, char *b_name);


extern int byte_swap_sb(struct sam_sblk *sblk, size_t len);

/*
 * execute the samsharefs command.
 * preconditions:
 * 1. The file system must exist
 * 2. If the file system is not mounted fs_mounted must be set to true.
 * 3. If write_local_copy is set to true the results of the command will
 * be piped to /etc/opt/SUNWsamfs/hosts.<fs_name>
 */
int
sharefs_cmd(
const char *fs_name,
const char *server,		/* if not NULL, will set new server */
const boolean_t update,		/* if true hosts.fsname will read in */
const boolean_t fs_mounted,	/* if true will use info from raw device */
const boolean_t write_local_copy) /* if true will pipe output to hosts file */
{

	int status;
	pid_t pid;
	FILE *err_stream = NULL;
	size_t cmdlen = 512;
	char cmd[cmdlen];
	char line[200]; /* first line of stderr */
	upath_t hosts_path;



	Trace(TR_MISC, "sharing fs %s, %s, %d, %d, %d",
	    Str(fs_name), Str(server), update, fs_mounted,
	    write_local_copy);


	if (!fs_name) {
		samerrno = SE_NULL_PARAMETER;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NULL_PARAMETER), "fs_name");
		Trace(TR_ERR, "sharing fs failed %s", samerrmsg);
		return (-1);
	}

	snprintf(cmd, cmdlen, "%s/%s ", SAM_DIR, "sbin/samsharefs");
	if (!fs_mounted) {
		strlcat(cmd, "-R ", cmdlen);
	}
	if (update) {
		strlcat(cmd, "-u ", cmdlen);
	}

	if (server) {
		strlcat(cmd, "-s ", cmdlen);
		strlcat(cmd, server, cmdlen);
		strlcat(cmd, " ", cmdlen);
	}

	strlcat(cmd, fs_name, cmdlen);
	if (write_local_copy) {
		if (update) {
			samerrno = SE_UPDATE_AND_WRITE_LOCAL_SET;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_UPDATE_AND_WRITE_LOCAL_SET));

			Trace(TR_ERR, "sharing fs %s failed %s",
			    fs_name, samerrmsg);

			return (-1);
		}
		snprintf(hosts_path, sizeof (hosts_path), "%s/hosts.%s",
		    CFG_DIR, fs_name);
		strlcat(cmd, " > ", cmdlen);
		strlcat(cmd, hosts_path, cmdlen);
	}
	Trace(TR_MISC, "sharefs_cmd executing %s", cmd);

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		return (-1);
	}

	line[0] = '\0';
	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_DEBUG, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "samsharefs(pid = %ld) status: %d\n", pid,
		    status);
	}
	fclose(err_stream);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_SHAREFS_FAILED;
			strlcpy(samerrmsg, line, MAX_MSG_LEN);
			Trace(TR_ERR, "samsharefs exit code: %d\n",
			    WEXITSTATUS(status));
		} else {
			/* everything is ok */
			Trace(TR_MISC, "samsharedfs %s  with command: %s",
			    fs_name, cmd);
			return (0);
		}
	} else {
		samerrno = SE_SHAREFS_FAILED;
		strlcpy(samerrmsg, "samsharefs abnormally terminated/stopped",
		    MAX_MSG_LEN);
	}

	Trace(TR_ERR, "sharing fs %s failed %s", fs_name, samerrmsg);
	return (-1);
}



/*
 * This function will read the super block entries on each device in the
 * shared file system.
 *
 * If the ctx indicates that this is for a dump, it is not considered an
 * error if the super block cannot be read.
 *
 * This function will check:
 *	The superblock is for the same fs as the fs_t
 *	that all devices are present for the filesystem.
 *	all devices have the right equipment ordinal in the fs_t
 *	that the device types are right in the fs_t
 *	that the mcf file has the devices in the right order.
 *
 *	preconditions: file system must have been created on some node
 *	for this function to work.
 *
 * There are a set of checks that check the sblk vs the mcf passed in.
 *
 * There are a set of checks that are performed between the super
 * blocks from each device to make sure they are all in agreement.
 * This includes data count, mm_count, fs eq, family set name, time
 * superblock initialized
 */
int
shared_fs_check(
ctx_t *ctx,		/* ARGSUSED */
char *fs_name,		/* name of the file system to check */
mcf_cfg_t *mcf) {	/* master configuration */

	/* structs for comparisons */
	struct sam_sblk sav_blk;
	struct sam_sblk sblk;
	boolean_t	sav_blk_set = B_FALSE;
	char		*sav_blk_dev_name = NULL;



	/* counters to track mcf contents */
	int		mcf_mm_devs = 0;
	int		mcf_data_devs = 0;
	int		fs_count = 0;

	/* Flags for controlling flow */
	boolean_t	found = B_FALSE;
	boolean_t	out_of_order = B_FALSE;
	boolean_t	nodevs = B_FALSE;

	/* indices and pointers to important things */
	node_t		*n;
	int		i;
	node_t		*current_dev_node = NULL;
	base_dev_t	*fam_set_dev = NULL; /* dev for the fs line in mcf */
	node_t		*fam_set_node = NULL; /* node holding fs line's dev */
	base_dev_t	*dev;
	dtype_t		mcf_type;


	Trace(TR_MISC, "checking file system against the superblocks for %s",
	    Str(fs_name));

	/*
	 * find the file system's device entries and count them
	 */
	for (n = mcf->mcf_devs->head; n != NULL; n = n->next) {
		dev = (base_dev_t *)n->data;

		if (strcmp(dev->name, fs_name) == 0) {
			fam_set_node = n;
			fam_set_dev = (base_dev_t *)n->data;
			found = B_TRUE;

			current_dev_node = n->next;

			/*
			 * loop through and count devices for this
			 * file system.
			 */
			for (n = n->next; n != NULL &&
				strcmp(((base_dev_t *)n->data)->set,
				fs_name) == 0; n = n->next) {

				fs_count++;
			}
			break;
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_NOT_FOUND), fs_name);
		goto err;
	}

	if (fs_count == 0) {
		samerrno = SE_NO_DEVS;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_NO_DEVS), fs_name);
		goto err;
	}


	/*
	 * loop over the devices doing checks and aggregating
	 * information. Note: the current_dev_node was set to the
	 * first device in the fs. i is used to identify and check
	 * what would be the device ordinal within the file system for
	 * the devices in the mcf.
	 */
	for (i = 0; i < fs_count && current_dev_node != NULL;
		i++, current_dev_node = current_dev_node->next) {

		int32_t		dev_ord;
		int		fd;
		base_dev_t *cur_dev = (base_dev_t *)current_dev_node->data;

		/*
		 * check the device type and increment
		 * the appropriate counters.
		 * This should include checking that the super block agrees
		 * about the device type.
		 */
		mcf_type = nm_to_dtclass(cur_dev->equ_type);
		if (mcf_type == DT_META) {
			mcf_mm_devs++;
			if (strcmp(cur_dev->name, NODEV_STR) == 0) {

				nodevs = B_TRUE;
				/* we can't read the superblock so cont. */
				continue;
			}
		} else if (mcf_type == DT_DATA) {
			mcf_data_devs++;
		} else if (is_stripe_group(mcf_type)) {
			mcf_data_devs++;
		} else if (mcf_type == DT_RAID) {
			mcf_data_devs++;
		} else {
			samerrno = SE_INVALID_DEVICE_IN_FS;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_INVALID_DEVICE_IN_FS),
				cur_dev->name, cur_dev->equ_type,
				cur_dev->set);

			goto err;
		}


		/* get the super block from the device */
		if (sam_dev_sb_get(cur_dev->name, &sblk, &fd) != 0) {
			close(fd);

			/* Not a sam super block */
			samerrno = SE_DEV_HAS_NO_SUPERBLOCK;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_DEV_HAS_NO_SUPERBLOCK),
				cur_dev->name, fs_name);

			goto err;
		}
		close(fd);

		switch (sblk.info.sb.magic) {
		case SAM_MAGIC_V1_RE:
		case SAM_MAGIC_V2_RE:
		case SAM_MAGIC_V2A_RE:
			Trace(TR_MISC, "reversed byte order detected for %s",
			    Str(fs_name));

			if (byte_swap_sb(&sblk, sizeof (sblk)) != 0) {
				samerrno = SE_CANT_FIX_BYTE_ORDER;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_CANT_FIX_BYTE_ORDER));
				Trace(TR_ERR, "swapping byte order failed");
				goto err;
			}

		}

		/*
		 * Keep a copy of the first super block retrieved
		 * for future comparisons
		 */
		if (!sav_blk_set) {
			memcpy(&sav_blk, &sblk, sizeof (sblk));
			sav_blk_set = B_TRUE;
			sav_blk_dev_name = cur_dev->name;
		}


		/*
		 * check that each super block is for the file system
		 * that we are targeting in the mcf file. Check
		 * that the file system name and eq from the super
		 * block for the current device match the mcf structs.
		 */
		if (strcmp(sblk.info.sb.fs_name, fam_set_dev->name) != 0) {

			samerrno = SE_WRONG_FS_NAME;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_WRONG_FS_NAME),
				cur_dev->name, fs_name);
			goto err;
		}

		if (sblk.info.sb.eq != fam_set_dev->eq) {
			samerrno = SE_WRONG_FS_EQ;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_WRONG_FS_EQ),
				fs_name, cur_dev->name);
			goto err;
		}

		if (sblk.info.sb.eq != fam_set_dev->eq) {
			samerrno = SE_WRONG_EQ;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_WRONG_FS_EQ),
				fs_name, cur_dev->name);
			goto err;
		}

		/*
		 * Perform checks between this super block and
		 * perviously fetched super blocks or if this is the
		 * first one set up for later checks.
		 */
		if (sav_blk_set) {
			if (compare_super_blocks(&sblk, cur_dev->name,
				&sav_blk, sav_blk_dev_name) != 0) {
				goto err;
			}
		}


		/*
		 * get the devices ordinal within the family set and
		 * check the super block from the current device against
		 * the current devices base_dev_t structure
		 */
		dev_ord = sblk.info.sb.ord;


		if (sblk.eq[dev_ord].fs.type != mcf_type) {
			samerrno = SE_DEVICE_TYPE_WRONG;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_DEVICE_TYPE_WRONG),
				cur_dev->name, fs_name);
			goto err;
		}

		if (sblk.eq[dev_ord].fs.eq != cur_dev->eq) {
			samerrno = SE_WRONG_EQ;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_WRONG_EQ), cur_dev->name);
			goto err;
		}

		/*
		 * i should equal the ordinal of the device. If not the
		 * devices will need to be reordered.
		 */
		if (i != sblk.info.sb.ord) {
			out_of_order = B_TRUE;
			Trace(TR_MISC, "The device %s is out of order in %s",
				cur_dev->name, cur_dev->set);
		}
	}


	/* Now check the aggregate stats. */
	if (mcf_mm_devs != sav_blk.info.sb.mm_count) {
		samerrno = SE_MM_COUNT_WRONG;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_MM_COUNT_WRONG),
			fs_name);

		goto err;
	}

	if (mcf_data_devs != sav_blk.info.sb.da_count) {
		samerrno = SE_DATA_COUNT_WRONG;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_DATA_COUNT_WRONG),
			fs_name);

		goto err;
	}

	/* A bit redundant after the previous two checks */
	if (fs_count != sav_blk.info.sb.fs_count) {
		samerrno = SE_DEV_COUNT_WRONG;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_DEV_COUNT_WRONG),
			fs_name);

		goto err;
	}

	/*
	 * if any nodev entries were present we must check their eqs and
	 * order against a super block. We could not gaurantee this would
	 * succeed in the main loop because the mm devices are likely to
	 * be before the data devices from which a super block could be read.
	 */
	if (nodevs) {
		for (i = 0, n = fam_set_node; i < fs_count; i++, n = n->next) {
			dev = (base_dev_t *)n->data;
			mcf_type = nm_to_dtclass(dev->equ_type);
			if (mcf_type != DT_META) {
				continue;
			}
			if (sav_blk.eq[i].fs.ord != dev->eq) {
				out_of_order = B_TRUE;
			}
		}

	}

	/* fix the device ordering in the mcf structure. */
	if (out_of_order) {
		reorder_mcf_devs(sav_blk, fam_set_node);
	}

	Trace(TR_MISC, "checked file system %s against super blocks",
		fs_name);

	return (0);

err:

	if (ctx != NULL && *ctx->dump_path != '\0') {
		Trace(TR_MISC, "could not check fs %s%s against the ",
		    "superblock but since there is a dump path",
		    " this is not an error");
		return (0);
	}

	Trace(TR_ERR, "checking file system failed: %s",
		samerrmsg);
	return (-1);
}


static int
compare_super_blocks(
struct sam_sblk *a,
char *a_name,
struct sam_sblk *b,
char *b_name) {

	if (a->info.sb.eq != b->info.sb.eq) {
		samerrno = SE_FS_EQ_MISMATCH;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_FS_EQ_MISMATCH), a_name,
			b_name, b->info.sb.fs_name);

		return (-1);
	}
	if (a->info.sb.init != b->info.sb.init) {
		samerrno = SE_INIT_MISMATCH;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_INIT_MISMATCH), a_name,
			b_name, b->info.sb.fs_name);

		return (-1);
	}

	if (a->info.sb.mm_count != b->info.sb.mm_count) {

		samerrno = SE_MM_COUNT_MISMATCH;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_MM_COUNT_MISMATCH), a_name,
			b_name, b->info.sb.fs_name);
		return (-1);
	}

	if (a->info.sb.da_count != b->info.sb.da_count) {

		samerrno = SE_DATA_COUNT_MISMATCH;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_DATA_COUNT_MISMATCH), a_name,
			b_name, b->info.sb.fs_name);

		return (-1);
	}

	return (0);
}


/*
 * The devices for the family set will be reordered so that they are in the
 * same order based on their eq ordinals as the equipment ordinals are in the
 * ords array.
 *
 * Preconditions:
 * Must already know that mcf contains all devices and that it contains devices
 * with the correct eq ordinals.
 * Devices should be out of order.
 */
static int
reorder_mcf_devs(
struct sam_sblk sblk, /* source of the ordering */
node_t *fam_set)
{

	node_t *out;
	node_t *in;
	base_dev_t *cur;
	base_dev_t *new;
	base_dev_t *tmp;
	boolean_t found;
	int i;


	Trace(TR_MISC, "reordering the devices for %s in the mcf %s",
	    ((base_dev_t *)fam_set->data)->name,
	    "to match the super block");

	for (i = 0, out = fam_set->next; i < sblk.info.sb.fs_count &&
	    out != NULL; out = out->next, i++) {

		cur = (base_dev_t *)out->data;
		if (cur->eq != sblk.eq[i].fs.eq) {
			found = B_FALSE;


			/*
			 * find the dev with the ordinal that is supposed to
			 * be at index i
			 */
			for (in = out->next; in != NULL; in = in->next) {
				new = (base_dev_t *)in->data;

				if (new->eq == sblk.eq[i].fs.eq) {
					found = B_TRUE;
					tmp = (base_dev_t *)out->data;
					out->data = in->data;
					in->data = tmp;
					break;
				}

			}
			if (found == B_FALSE) {
				samerrno = SE_DEVICE_NOT_FOUND;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_DEVICE_NOT_FOUND),
				    ((base_dev_t *)fam_set->data)->name,
				    sblk.eq[i].fs.eq);


				Trace(TR_ERR, "reordering the %s: %d %s",
				    "devices failed", samerrno, samerrmsg);
				return (-1);
			}
		}

	}

	Trace(TR_MISC, "reordered the devices in the mcf to %s",
	    " match the super block");
	return (0);
}
