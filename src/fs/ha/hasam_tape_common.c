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

/*
 * hasam_tape_common.c - Common routines for tape related operations.
 */

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/shm.h>

#include "sam/mount.h"
#include "sam/nl_samfs.h"
#include "pub/mgmt/device.h"
#include "pub/mgmt/diskvols.h"
#include "mgmt/config/cfg_diskvols.h"
#include "mgmt/config/master_config.h"
#include "sam/devnm.h"
#include "aml/shm.h"
#include "aml/fifo.h"
#include "aml/archiver.h"

#include "hasam.h"

/* This routine is not declared in header files */
extern char *dirname(char *);

static void *shmat_4hasam(void);

static char diskvolsconf[] = SAM_CONFIG_PATH"/diskvols.conf";
static char mcffile[] = SAM_CONFIG_PATH"/"CONFIG;

static shm_alloc_t master_shm;
static shm_ptr_tbl_t   *shm;
static dev_ptr_tbl_t *Dev_Tbl;
static int drives_on = 0;
static int drives_off = 0;
static struct lib_state libstate[MAX_UNITS];
static int lib_on = 0;
static int lib_off = 0;
static int	lib_on_timer = 20;

int	drive_timer = 15;

#define	FIFO_path   SAM_FIFO_PATH"/"CMD_FIFO_NAME

static void *
shmat_4hasam()
{
	master_shm.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	if (master_shm.shmid < 0) {
		scds_syslog(LOG_ERR, "Unable to find master shared mem seg !");
		return (NULL);
	}

	master_shm.shared_memory  =
	    shmat(master_shm.shmid, (void *)NULL, SHM_RDONLY);
	if (master_shm.shared_memory == (void *)-1) {
		scds_syslog(LOG_ERR,
		    "Unable to attach master shared mem seg !");
		return (NULL);
	}

	shm = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm->shm_block.segment_name, SAM_SEGMENT_NAME) != 0) {
		scds_syslog(LOG_ERR, "Unable to get SAM shared seg !");
		return (NULL);
	}

	Dev_Tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm->dev_table);
	if (Dev_Tbl == NULL) {
		scds_syslog(LOG_ERR, "Unable to find SAM device table !");
		shmdt(master_shm.shared_memory);
		return (NULL);
	}

	return (Dev_Tbl);
}

boolean_t
is_lib_on(int lib_num)
{
	dev_ent_t *dev;

	if ((shm == NULL) || (Dev_Tbl->d_ent[lib_num] == NULL)) {
		scds_syslog(LOG_ERR, "Error collecting info from shared mem !");
		return (B_FALSE);
	}

	dev = (dev_ent_t *)SHM_REF_ADDR(Dev_Tbl->d_ent[lib_num]);
	if (dev->state == DEV_ON) {
		libstate[lib_num].l_state = DEV_ON;
		lib_on++;
		return (B_TRUE);
	}

	return (B_FALSE);
}

boolean_t
is_lib_off(int lib_num)
{
	dev_ent_t *dev;

	if ((shm == NULL) || (Dev_Tbl->d_ent[lib_num] == NULL)) {
		scds_syslog(LOG_ERR, "Error collecting info from shared mem !");
		return (B_FALSE);
	}

	dev = (dev_ent_t *)SHM_REF_ADDR(Dev_Tbl->d_ent[lib_num]);
	if ((dev->state == DEV_OFF) || (dev->state == DEV_IDLE)) {
		libstate[lib_num].l_state = DEV_OFF;
		lib_off++;
		return (B_TRUE);
	}

	return (B_FALSE);
}


boolean_t
is_drive_on(int drive_num)
{
	dev_ent_t *dev;

	if ((shm == NULL) || (Dev_Tbl->d_ent[drive_num] == NULL)) {
		scds_syslog(LOG_ERR, "Error collecting info from shared mem !");
		return (B_FALSE);
	}

	dev = (dev_ent_t *)SHM_REF_ADDR(Dev_Tbl->d_ent[drive_num]);
	if (dev->state == DEV_ON) {
		drives_on++;
		return (B_TRUE);
	}

	return (B_FALSE);
}

boolean_t
is_drive_off(int drive_num)
{
	dev_ent_t *dev;

	if ((shm == NULL) || (Dev_Tbl->d_ent[drive_num] == NULL)) {
		scds_syslog(LOG_ERR, "Error collecting info from shared mem !");
		return (B_FALSE);
	}

	dev = (dev_ent_t *)SHM_REF_ADDR(Dev_Tbl->d_ent[drive_num]);
	if ((dev->state == DEV_OFF) || (dev->state == DEV_IDLE)) {
		drives_off++;
		return (B_TRUE);
	}

	return (B_FALSE);
}


/*
 * Check mcf file if any tape library and drive is
 * configured in it.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
library_in_mcf(void)
{
	mcf_cfg_t *mcf;
	node_t  *node;
	base_dev_t *dev;
	char *rb_type;
	char *tp_type;
	int found_lib, found_drv, i;

	tape_lib_drv_avail = B_FALSE;

	/* Read mcf file and store contents in list */
	if (parse_mcf(mcffile, &mcf) != 0) {
		scds_syslog(LOG_ERR, "Error reading MCF file %s",
		    mcffile);
		return (B_FALSE);
	}

	found_lib = 0;
	found_drv = 0;

	/*
	 * Walk through entries in mcf and mainly look for
	 * tape libraries and drives. Check each equipment
	 * type if its a library or a drive. If a drive is
	 * see if the state is set to ON. Then we can confirm
	 * atleast one tape library and drive is configured
	 * for use in HA-SAM.
	 */
	for (node = mcf->mcf_devs->head;
	    node != NULL; node = node->next) {
		if (node->data != NULL) {
			dev = (base_dev_t *)node->data;

			i = 0;
			for (rb_type = dev_nmrb[i]; rb_type != NULL;
			    rb_type = dev_nmrb[i++]) {
				if (strcmp(rb_type, dev->equ_type) == 0) {
					/* Found tape library in mcf */
					found_lib++;
				}
			}

			i = 0;
			for (tp_type = dev_nmtp[i]; tp_type != NULL;
			    tp_type = dev_nmtp[i++]) {
				if ((strcmp(tp_type, dev->equ_type) == 0) &&
				    (dev->state == 0)) {
					/* Found tape drive and ON in mcf */
					found_drv++;
				}
			}
		}
	}

	free_mcf_cfg(mcf);

	if ((found_lib <= 0) || (found_drv <= 0)) {
		dprintf(stderr,
		    "Tape library or drive not configured [%d] in mcf\n",
		    tape_lib_drv_avail);
		return (B_FALSE);
	}

	tape_lib_drv_avail = B_TRUE;
	dprintf(stderr,
	    "Tape libraries=%d, drives=%d configured [%d] in mcf\n",
	    found_lib, found_drv, tape_lib_drv_avail);
	return (B_TRUE);
}

/*
 * This routine collects information of all available tape libraries
 * and tape drives connected to the node.
 *
 * Return: success = struct media_info [or] failure = NULL
 */
struct media_info *
collect_media_info(void)
{
	int			i, j;
	node_t		*node_lib, *node_drv, *node_stkdev;
	sqm_lst_t	*lib_list;
	library_t	*print_lib;
	drive_t		*print_drv;
	stk_device_t		*print_stkdev;
	struct media_info	*minfo;

	/*
	 * Collect all the available information of libraries and tape
	 * drives connected to the node
	 */
	i = get_all_libraries(NULL, &lib_list);
	if (i != 0) {
		scds_syslog(LOG_ERR, "Error getting library list.");
		dprintf(stderr, "Error getting library list\n");
		return (NULL);
	}

	minfo = (struct media_info *)malloc(sizeof (struct media_info));
	if (minfo == NULL) {
		scds_syslog(LOG_ERR, "Out of Memory");
		return (NULL);
	}
	bzero((char *)minfo, sizeof (struct media_info));

	num_drives = 0;
	num_cat = 0;
	j = 0;

	/*
	 * We have got the library information, now go over the
	 * list and store the required information
	 */
	for (node_lib = lib_list->head; node_lib != NULL;
	    node_lib = node_lib->next) {
		print_lib = (library_t *)node_lib->data;

		/*
		 * Skip information on historian
		 */
		if (strcmp(print_lib->base_info.equ_type, "hy") == 0) {
			continue;
		}

		/*
		 * For all libraries connected to this node,
		 * save information on their tape drives
		 */
		for (node_drv = (print_lib->drive_list)->head;
		    node_drv != NULL; node_drv = node_drv->next) {
			print_drv = (drive_t *)node_drv->data;

			strlcpy(minfo->dinfo[j].drive_path,
			    (print_drv->base_info).name,
			    sizeof (minfo->dinfo[j].drive_path));

			strlcpy(minfo->dinfo[j].drive_eqtype,
			    print_drv->base_info.equ_type,
			    sizeof (minfo->dinfo[j].drive_eqtype));

			minfo->dinfo[j].drive_eq =
			    (int)((print_drv->base_info).eq);

			if (minfo->dinfo[j].drive_eq <= 0) {
				minfo->dinfo[j].drive_eq = WRONG_EQ;
			} else {
				num_drives++;
				j++;
			}
		}

		/* Get path to catalog for the libraries */
		if (print_lib->catalog_path != NULL) {
			strlcpy(minfo->catalog_path[num_cat],
			    print_lib->catalog_path,
			    sizeof (minfo->catalog_path[num_cat]));
			num_cat++;
		} else {
			dprintf(stderr, "No catalog for library %s\n",
			    print_lib->base_info.equ_type);
		}

		/*
		 * For STK library, save additional information such as
		 * hostname, configuration file, acs_num, lsm_num etc.
		 */
		if (strcmp(print_lib->base_info.equ_type, "sk") == 0) {

			strlcpy(minfo->linfo.lib_eqtype,
			    print_lib->base_info.equ_type,
			    sizeof (minfo->linfo.lib_eqtype));

			strlcpy(minfo->linfo.hostname,
			    print_lib->storage_tek_parameter->hostname,
			    sizeof (minfo->linfo.hostname));

			strlcpy(minfo->linfo.conf_file,
			    (print_lib->base_info).name,
			    sizeof (minfo->linfo.conf_file));


			for (node_stkdev = (print_lib->
			    storage_tek_parameter->stk_device_list)->head;
			    node_stkdev != NULL;
			    node_stkdev = node_stkdev->next) {
				print_stkdev =
				    (stk_device_t *)node_stkdev->data;

				minfo->linfo.acs_param = print_stkdev->acs_num;
				minfo->linfo.lsm_param = print_stkdev->lsm_num;
				minfo->linfo.panel_param =
				    print_stkdev->panel_num;
				minfo->linfo.drive_param =
				    print_stkdev->drive_num;
			}
		}
		if (print_lib->base_info.eq >= 0) {
			libstate[num_lib++].l_eq = print_lib->base_info.eq;
		}
	}

	dprintf(stderr, "Total lib = %d \n", num_lib);
	print_media_info(minfo);

	/*
	 * Free library list as the job of collection is over
	 */
	if (lib_list != NULL) {
		free_list_of_libraries(lib_list);
		lib_list = NULL;
	}

	return (minfo);
}


/*
 * Prints the collected information about all libraries & tape drives
 *
 * Return: void
 */
void
print_media_info(struct media_info *mi)
{
	int j;

	/* Print the list of all available drives */
	for (j = 0; j < num_drives; j++) {
		dprintf(stderr, "Drive[%d] = %s Eqtype = %s Eq = %d\n",
		    j, mi->dinfo[j].drive_path,
		    mi->dinfo[j].drive_eqtype,
		    mi->dinfo[j].drive_eq);
	}

	/* Print the special information on STK library */
	dprintf(stderr, "STK eq = %s hostname = %s file = %s\n",
	    mi->linfo.lib_eqtype,
	    mi->linfo.hostname,
	    mi->linfo.conf_file);
	dprintf(stderr,
	    "STK acs = %d lsm = %d panel = %d drive = %d\n",
	    mi->linfo.acs_param,
	    mi->linfo.lsm_param,
	    mi->linfo.panel_param,
	    mi->linfo.drive_param);

	/* Print path to catalog file */
	for (j = 0; j < num_cat; j++) {
		dprintf(stderr, "Catalog[%d] of %d: %s\n",
		    j, num_cat, mi->catalog_path[j]);
	}
}


/*
 * Free the allocated media_info structure.
 * Place holder if we need to free up some other tape related resources.
 *
 * Return: void
 */
void
free_media_info(struct media_info *mi)
{
	free((void *) mi);
}

/*
 * Routine to check if all library catalogs exists on catalog
 * filesystem.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_catalog(struct media_info *mi)
{
	int j, total;
	struct stat64 buf;
	boolean_t found = B_FALSE;

	if ((mi == NULL) || (num_cat <= 0)) {
		dprintf(stderr,
		    "Error collecting catalog info from media !\n");
		return (found);
	}
	total = 0;
	/*
	 * Check if path to catalog extracted from library configuration
	 * does actually exist
	 */
	for (j = 0; j < num_cat; j++) {
		if (stat64(mi->catalog_path[j], &buf) != 0 ||
		    !S_ISREG(buf.st_mode)) {
			found = B_FALSE;
			scds_syslog(LOG_ERR,
			    "Library catalog %s does not exist !",
			    mi->catalog_path[j]);
			dprintf(stderr, "Library catalog %s does not exist !\n",
			    mi->catalog_path[j]);
		}
		total++;
	}
	dprintf(stderr, "check_catalog: num_cat = %d, total found = %d\n",
	    num_cat, total);
	if (num_cat == total) {
		found = B_TRUE;
	}
	return (found);
}


/*
 * Routine to confirm if catalog directory /var/opt/SUNWsamfs/catalog
 * in standard location is replaced with a link to a directory in
 * cluster filesystem.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_catalog_dir(void)
{
	char	*link_name = SAM_CATALOG_DIR;
	char *mntpt_dir;

	/*
	 * Check if catalog location is correctly configured on
	 * cluster filesystem
	 */
	mntpt_dir = search_link(link_name);
	if (mntpt_dir == NULL) {
		scds_syslog(LOG_ERR,
		    "Error in catalog configuration %s !",
		    link_name);
		dprintf(stderr,
		    "Error in catalog configuration %s !\n",
		    link_name);
		return (B_FALSE);
	}

	/*
	 * Compare if the mount point we got is same as that
	 * supplied by catalog filesystem extended property
	 * in HA-SAM resource type file
	 */
	if (strcmp(mntpt_dir, catalogfs_mntpt) != 0) {
		scds_syslog(LOG_DEBUG,
		    "Catalog %s is not configured on cluster FS %s",
		    link_name, mntpt_dir);
		dprintf(stderr,
		    "Catalog %s is on not configured cluster FS %s\n",
		    link_name, mntpt_dir);
		return (B_FALSE);
	}

	dprintf(stderr, "Catalog directory %s for link %s configured\n",
	    mntpt_dir, link_name);
	return (B_TRUE);
}


/*
 * Get STK hostname from the collected library information
 *
 * Return: success = STK hostname [or] failure = NULL
 */
char *
get_stk_hostname(struct media_info *mi)
{
	if ((mi == NULL) || (mi->linfo.hostname == NULL)) {
		dprintf(stderr, "Error collecting media info\n");
		return (NULL);
	}
	return (mi->linfo.hostname);
}


/*
 * Turn all available drives ON and reserve them for use with
 * the node.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
make_drives_online(struct media_info *mi)
{
	int j, rc;

	if ((mi == NULL) || (num_drives <= 0)) {
		scds_syslog(LOG_ERR, "Error collecting media info !");
		return (B_FALSE);
	}

	if (shmat_4hasam() == NULL) {
		scds_syslog(LOG_ERR, "Error reading shared mem info !");
		return (B_FALSE);
	}

	lib_on = 0;
	for (j = 0; j < num_lib; j++) {
		if (!is_lib_on(libstate[j].l_eq)) {
			change_drive_state(libstate[j].l_eq, DEV_ON);
			sleep(lib_on_timer);
			is_lib_on(libstate[j].l_eq);
		}
	}

	if (lib_on <= 0) {
		dprintf(stderr, "Libraries are not ON, turn them ON !\n");
	}

	drives_on = 0;
	for (j = 0; j < num_drives; j++) {
		/* Check status of drive */
		rc = run_mt_cmd(mi->dinfo[j].drive_path, "status");
		if (rc != 0) {
			dprintf(stderr, "Error mt status for drive %s !\n",
			    mi->dinfo[j].drive_path);
		}
		/* Now turn drives ON */
		if (!is_drive_on(mi->dinfo[j].drive_eq)) {
			change_drive_state(mi->dinfo[j].drive_eq, DEV_ON);
			sleep(drive_timer);
			if (!is_drive_on(mi->dinfo[j].drive_eq)) {
				scds_syslog(LOG_ERR,
				    "Error turning drive %d ON.",
				    mi->dinfo[j].drive_eq);
				dprintf(stderr, "Error turning drive %d ON !\n",
				    mi->dinfo[j].drive_eq);
			}
		}
	}

	shmdt(master_shm.shared_memory);
	return (B_TRUE);
}


/*
 * Make all available tape drives offline.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
offline_tape_drives(struct media_info *mi)
{
	int j, rc, rc1, rc2;

	if ((mi == NULL) || (num_drives <= 0)) {
		scds_syslog(LOG_ERR, "Error collecting media info !");
		return (B_FALSE);
	}

	if (shmat_4hasam() == NULL) {
		scds_syslog(LOG_ERR, "Error reading shared mem info !");
		return (B_FALSE);
	}

	for (j = 0; j < num_drives; j++) {
		/* Turn OFF tape drives */
		if (!is_drive_off(mi->dinfo[j].drive_eq)) {
			change_drive_state(mi->dinfo[j].drive_eq, DEV_IDLE);
			sleep(drive_timer);
			if (!is_drive_off(mi->dinfo[j].drive_eq)) {
				scds_syslog(LOG_ERR, "Error drive %d IDLE.",
				    mi->dinfo[j].drive_eq);
				dprintf(stderr, "Error drive %d IDLE !\n",
				    mi->dinfo[j].drive_eq);

				change_drive_state(mi->dinfo[j].drive_eq,
				    DEV_OFF);
				sleep(drive_timer);
				if (!is_drive_off(mi->dinfo[j].drive_eq)) {
					dprintf(stderr,
					    "Error turning drive %d OFF !\n",
					    mi->dinfo[j].drive_eq);
				}
			}
		}
		/*
		 * Check status of drive using mt command
		 * Make atleast 2 attempts of this command
		 * to be sure there is no tape in the drive
		 * before the drive is released
		 */
		rc1 = run_mt_cmd(mi->dinfo[j].drive_path, "status");
		rc2 = run_mt_cmd(mi->dinfo[j].drive_path, "status");
		if ((rc1 != 0) && (rc2 != 0)) {
			dprintf(stderr, "Error mt status for drive %s !\n",
			    mi->dinfo[j].drive_path);
		} else {
			/* Release reservation on tape drives */
			rc = run_mt_cmd(mi->dinfo[j].drive_path, "release");
			if (rc != 0) {
				dprintf(stderr, "Error release drive %s !\n",
				    mi->dinfo[j].drive_path);
			}
		}
	}

	shmdt(master_shm.shared_memory);
	return (B_TRUE);
}


boolean_t
change_drive_state(int drv_eq, dstate_t new_state)
{
	sam_cmd_fifo_t cmd_block;
	int fifo_fd;    /* File descriptor for FIFO */
	int size;

	dprintf(stderr, "Changing state for eq %d to %s\n",
	    drv_eq, dev_state[new_state]);

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = drv_eq;
	cmd_block.state = new_state;

	fifo_fd = open(FIFO_path, O_WRONLY | O_NONBLOCK);
	if (fifo_fd < 0) {
		scds_syslog(LOG_ERR,
		    "FIFO open failed: Unable to turn %s drive %d !",
		    dev_state[new_state], drv_eq);
		return (B_FALSE);
	}

	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.cmd = CMD_FIFO_SET_STATE;

	size = write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);
	if (size != sizeof (sam_cmd_fifo_t)) {
		scds_syslog(LOG_ERR,
		    "FIFO write failed: Unable to turn %s drive %d !",
		    dev_state[new_state], drv_eq);
		return (B_FALSE);
	}
	return (B_TRUE);
}


/*
 * Wrapper to run samcmd command on the node to turn tape drives
 * On or Off.
 *
 * Return: status of command execution
 */
int
turn_drives_on_off(
	int drvnum,
	char *onoff)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, MAXLEN,
	    SAM_EXECUTE_PATH "/samcmd %s %d > /dev/null 2>&1",
	    onoff, drvnum);

	return (run_cmd_4_hasam(cmd));
}


/*
 * Wrapper to run mt command on the node.
 *
 * Return: status of command execution
 */
int
run_mt_cmd(
	char *drvpth,
	char *mtcmd)
{
	char cmd[MAXLEN];

	(void) snprintf(cmd, MAXLEN,
	    "/usr/bin/mt -f %s %s > /dev/null 2>&1",
	    drvpth, mtcmd);

	return (run_cmd_4_hasam(cmd));
}


/*
 * Extract STK filename from the media info collected for STK library
 *
 * Return: success = STK configuration file [or] failure = NULL
 */
char *
get_stk_filename(struct media_info *mi)
{
	if ((mi == NULL) || (mi->linfo.conf_file == NULL)) {
		dprintf(stderr, "Error collecting media info\n");
		return (NULL);
	}
	return (mi->linfo.conf_file);

}

/* Not sure if this function is useful, if not delete this one */
void
get_stk_params(struct media_info *mi)
{
	if (mi == NULL) {
		dprintf(stderr, "Error collecting media info\n");
		return;
	}
	dprintf(stderr, "get_stk_params: acs = %d\n", mi->linfo.acs_param);
	dprintf(stderr, "get_stk_params: lsm = %d\n", mi->linfo.lsm_param);
	dprintf(stderr, "get_stk_params: panel = %d\n", mi->linfo.panel_param);
	dprintf(stderr, "get_stk_params: drive = %d\n", mi->linfo.drive_param);
}


/*
 * Probe essential configuration for running HA-SAM agent, such as
 * catalog filesystem is mounted, essential daemons are running etc.
 * If more stringent check is required add more to this routine.
 * However try to keep this routine light weight as it will be
 * called periodically by hasam_probe method.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
probe_hasam_config(void)
{
	/*
	 * Check if atleast one tape library with drive is
	 * configured in mcf, if not there is a possibility
	 * no library is configured and there no need to
	 * check if sam-catserverd is running.
	 * So don't return error if no library is detected.
	 *
	 * Note: GetSamInfo() should be called before to get
	 *		the correct status of tape_lib_drv_avail
	 */
	if (!tape_lib_drv_avail) {
		/*
		 * Check if sam-fsd daemon is running
		 */
		if (!is_sam_fsd()) {
			return (B_FALSE);
		}
	} else {
		/*
		 * Tape library is configured in mcf,
		 * so validate essential SAM daemons
		 */
		if (!verify_essential_sam_daemons() ||
		    (search_mount_point(catalogfs_mntpt) < 0)) {
			return (B_FALSE);
		}
	}
	return (B_TRUE);
}


/*
 * This routine collects information of all available volumes
 * and paths listed for disk archives in diskvols.conf
 *
 * Return: success = struct disk_archive_info [or] failure = NULL
 */
struct  disk_archive_info *
collect_diskarchive_info(void)
{
	int			i, j;
	disk_vol_t	*dsk_vsn;
	node_t		*dsk_node;
	diskvols_cfg_t	*dvcfg;
	struct	disk_archive_info	*dainfo;

	/*
	 * Parse diskvols.conf file and store its contents
	 * in a list
	 */
	i = parse_diskvols_conf(diskvolsconf, &dvcfg);
	if ((i != 0) || (dvcfg->disk_vol_list == NULL)) {
		scds_syslog(LOG_ERR, "Error parsing diskvols config.");
		dprintf(stderr, "Error %d: parsing diskvols config\n", i);
		free_diskvols_cfg(dvcfg);
		return (NULL);
	}

	dainfo = (struct disk_archive_info *)malloc(
	    sizeof (struct disk_archive_info));
	if (dainfo == NULL) {
		scds_syslog(LOG_ERR, "Out of Memory");
		return (NULL);
	}
	bzero((char *)dainfo, sizeof (struct disk_archive_info));

	num_disk_archives = 0;
	j = 0;

	/*
	 * Read diskvols list and collect disk archive
	 * resource information
	 */
	for (dsk_node = dvcfg->disk_vol_list->head;
	    dsk_node != NULL; dsk_node = dsk_node->next) {
		dsk_vsn = (disk_vol_t *)dsk_node->data;

		if (dsk_vsn->path != NULL) {

			strlcpy(dainfo->dskarch_vol[j],
			    dsk_vsn->vol_name,
			    sizeof (dsk_vsn->vol_name));

			strlcpy(dainfo->dskarch_host[j],
			    dsk_vsn->host,
			    sizeof (dsk_vsn->host));

			strlcpy(dainfo->dskarch_path[j],
			    dsk_vsn->path,
			    sizeof (dsk_vsn->path));

			dprintf(stderr,
			    "Disk arch %d: vol %s, host %s, path %s\n",
			    num_disk_archives,
			    dainfo->dskarch_vol[j],
			    dainfo->dskarch_host[j],
			    dainfo->dskarch_path[j]);

			num_disk_archives++;
			j++;
		}
	}

	/* Free disk vols list */
	free_diskvols_cfg(dvcfg);

	return (dainfo);
}


/*
 * Check if disk archiving is configured by looking for diskvols.conf
 * file. If it is configured then HA-SAM will not support it.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
check_disk_archive_conf(void)
{
	struct stat buf;
	boolean_t found;
	struct disk_archive_info *dskarch_info;
	int j;

	if (stat(diskvolsconf, &buf) < 0) {
		scds_syslog(LOG_DEBUG, "Disk archiving not configured");
		dprintf(stderr, "Disk archiving not configured !\n");
		/*
		 * Since disk archiving is not configured, just return
		 * success as no need to do further processing
		 */
		return (B_TRUE);
	}

	/*
	 * Since diskvols.conf file exists,
	 * gather disk resource information
	 */
	dskarch_info = collect_diskarchive_info();
	if ((dskarch_info == NULL) ||
	    (num_disk_archives <= 0)) {
		dprintf(stderr,
		    "Error collecting disk archive info\n");
		free(dskarch_info);
		return (B_FALSE);
	}

	for (j = 0; j < num_disk_archives; j++) {
		/*
		 * Verify if disk archive directory are valid
		 * for locally available resources with no host.
		 */
		if (streq(dskarch_info->dskarch_host[j], "")) {
			if (search_dir(dskarch_info->dskarch_path[j]) == NULL) {
				scds_syslog(LOG_ERR,
				    "Invalid disk archive directory %s !",
				    dskarch_info->dskarch_path[j]);
				dprintf(stderr,
				    "Invalid disk archive directory %s !\n",
				    dskarch_info->dskarch_path[j]);
				free(dskarch_info);
				return (B_FALSE);
			}
		} else {
			/*
			 * Disk archive resource is on a remote server.
			 * Verify if remote host can be ping'd on which
			 * NFS disk archive resources are configured.
			 * In this case host has a valid entry.
			 */
			if (!ping_host(dskarch_info->dskarch_host[j])) {
				dprintf(stderr,
				    "Unable to ping host = %s\n",
				    dskarch_info->dskarch_host[j]);
				return (B_FALSE);
			}
		}
	}

	free(dskarch_info);
	return (B_TRUE);
}


/*
 * Do all the required steps to stop the node from accessing
 * tape drives in the library - Offline tape drives and then
 * run samd stop. If more steps are required to stop drives,
 * add them here.
 *
 * Return: success = 0 [or] failure = 1
 */
int
stopAccess2Tapes(void)
{
	struct media_info *mout;

	if (library_in_mcf() && check_sam_daemon("sam-amld")) {
		if ((mout = collect_media_info()) != NULL) {
			offline_tape_drives(mout);
			free_media_info(mout);
		}
	}

	return (0);
}

/*
 * Do all the basic checks required for HA-SAM here.
 * + Is tape library configured in mcf, then check these:
 *   - Is catalog FS configured in SUNW.hasam resource type
 *   - Is catalog directory configured correctly
 * + Check others:
 *   - Is stager directory configured correctly
 *   - Is disk archiving configured
 *
 * The intent is all the cluster agent methods like
 * start, stop etc should use it to verify minimum
 * configuration for HA-SAM.
 *
 * If more checks are required, add them here.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
GetSamInfo(void)
{
	/*
	 * If tape library and drive is detected in mcf,
	 * Check for catalogFS and catalog directory
	 */
	if (library_in_mcf()) {
		if (!check_catalogfs() || !check_catalog_dir()) {
			dprintf(stderr, "Basic library check failed !\n");
			return (B_FALSE);
		}
	}

	/*
	 * These are common checks whether tape library is
	 * configured or not: stager directory, disk
	 * archives etc.
	 */
	if (!check_stager_conf() || !check_disk_archive_conf()) {
		dprintf(stderr, "Common required check failed !\n");
		return (B_FALSE);
	}

	return (B_TRUE);
}

/*
 * Wrapper to run stop archiver command on the node.
 * First archiver is instructed to idle and then stop.
 * After running these commands, wait till the timer expires
 * to give enough time for the arhiver to stop.
 * These timer values could be an extension property ?
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
stop_archiver(void)
{
	char msg[80];

	hasam_arstop_timer = 15;
	hasam_aridle_timer = 10;

	/* idle all archiving */
	snprintf(msg, sizeof (msg), "exec");

	(void) ArchiverControl(msg, "idle", msg, sizeof (msg));
	if (*msg != '\0') {
		scds_syslog(LOG_ERR, "Error archiver idle: %s !", msg);
		dprintf(stderr,
		    "Error archiver idle, trying to stop: %s !\n", msg);
		sleep(hasam_aridle_timer);
		(void) ArchiverControl(msg, "stop", msg, sizeof (msg));
		if (*msg != '\0') {
			scds_syslog(LOG_ERR, "Error archiver stop: %s !", msg);
			return (B_FALSE);
		}
	}

	sleep(hasam_arstop_timer);
	return (B_TRUE);
}


/*
 * Wrapper to run archiver restart command on the node.
 * Wait till the timer expires before archiver restart.
 *
 * Return: success = B_TRUE [or] failure = B_FALSE
 */
boolean_t
start_archiver(void)
{
	char msg[80];
	hasam_arrestart_timer = 15;
	int i;

	if (check_sam_daemon("sam-archiverd")) {
		return (B_TRUE);
	}

	snprintf(msg, sizeof (msg), "exec");

	for (i = 0; i < 3; i++) {
		ArchiverControl("restart", NULL, msg, sizeof (msg));
		sleep(hasam_arrestart_timer);
		if ((*msg == '\0') || check_sam_daemon("sam-archiverd")) {
			break;
		} else {
			if (i >= 2) {
				scds_syslog(LOG_ERR,
				    "Error archiver restart: %s !", msg);
				dprintf(stderr,
				    "Error archiver restart: %s !\n", msg);
				return (B_FALSE);
			}
		}
	}

	return (B_TRUE);
}
