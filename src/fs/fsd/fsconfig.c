/*
 *	fsconfig.c - filesystem configuration.
 *
 */

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

#pragma ident "$Revision: 1.94 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* POSIX headers. */
#include <unistd.h>
#include <fcntl.h>

/* Solaris headers. */
#include <syslog.h>
#include <sys/mount.h>
#include <sys/param.h>
#ifdef sun
#include <sys/fstyp.h>
#endif /* sun */
#ifdef linux
#include <sys/syscall.h>
#endif /* linux */
#include <sys/types.h>
#include "pub/devstat.h"
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
#include "sam/custmsg.h"
#include "aml/archiver.h"
#include "pub/devstat.h"
#include "sam/devnm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "sam/shareops.h"
#include "sam/syscall.h"
#ifdef sun
#include "aml/proto.h"
#endif /* sun */
#include "sam/nl_samfs.h"

#include "license/license.h"
#include "license/lic.h"
#ifdef sun
#include "license/license.c"
#endif /* sun */

/* Local headers. */
#include "fsd.h"
#include "utility.h"

#include "sam/mount.hc"

/* Private data. */
static struct sam_mount_info *prevFileSysTable = NULL;
static char *fsname;
static int prevFileSysNumof;
static int errors;
#ifdef sun
static struct sam_license_arg setlic;
#endif /* sun */

/* Private functions. */
static void diffMsgFunc(char *msg);
static int gatherFsDevices(struct sam_mount_info *mi);
static void getPrevFilesystems(void);
static void getFsDevices(void);


/*
 * Configure file system.
 */
void
FsConfig(char *fscfg_name)
{
	char	*argv[3];
	int		fn;

#ifdef sun
/*
 *	Set up dummy license with the following permissions:
 *  license type 0 (non-expiring)           0x00
 *	fast_fs,                                0x---8
 *	db_features, foreign_tape,              0x----3
 *	shared_san, segment, shared_fs          0x-----E
 *	(WORM is off unless package installed)  0x-----1
 *	unused                                  0x------00
 */
	sam_license_t_33	syslic = {	0,	/* hostid */
						0,	/* exp_date */
						0,	/* license mask */
						0 };	/* check_sum */
	struct stat sb;

	syslic.license.lic_u.b.fast_fs = 1;
	syslic.license.lic_u.b.db_features = 1;
	syslic.license.lic_u.b.foreign_tape = 1;
	syslic.license.lic_u.b.shared_san = 1;
	syslic.license.lic_u.b.segment = 1;
	syslic.license.lic_u.b.shared_fs = 1;

	if ((lstat(SAM_EXECUTE_PATH"/samw", &sb) == 0) &&
	    S_ISLNK(sb.st_mode)) {
		syslic.license.lic_u.b.WORM_fs = 1;
	}
	if (Daemon || FsCfgOnly) {
		setlic.value = syslic;
		if (sam_syscall(SC_setlicense, (void *)&setlic,
				sizeof (struct sam_license_arg)) < 0) {
			/* License initialization failed. */
			PostEvent(FSD_CLASS, "InitFailed", 17204, LOG_ERR,
				GetCustMsg(17204),
				NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			FatalError(17204);
		}
	}
#endif /* sun */
	getPrevFilesystems();

	/*
	 * Get all the file system definitions.
	 */
	errors = 0;
	FileSysNumof = 0;
	FileSysTable = NULL;
	getFsDevices();
	if (errors != 0) {
		/* Problem with file system devices. */
		FatalError(17217);
	}
	ReadFsCmdFile(fscfg_name);

#ifdef sun
	/*
	 * Check license for ma, shared filesystems.
	 */
	for (fn = 0; fn < FileSysNumof; fn++) {
		struct sam_mount_info *mi;
		char	*msg;

		mi = &FileSysTable[fn];
		mi->params.fi_config &= ~MT_SHARED_MO;
		if (mi->params.fi_type == DT_META_SET) {
			if (!fast_fs_enabled(&syslic)) {
				/*
				 * "License: License does not support 'ma'
				 * file system: %s aborts"
				 */
				FatalError(11050, "fsd");
			}
		}
		if (mi->params.fi_config1 & MC_SHARED_FS) {
			if (!sharedfs_enabled(&syslic)) {
				/*
				 * "License: License does not support
				 * 'shared' file system: %s aborts"
				 */
				FatalError(11081, "fsd");
			}
		}
		if (mi->params.fi_config & MT_ALLWORM_OPTS) {
			if (!worm_fs_enabled(&syslic)) {
				/*
				 * "License: License does not support 'WORM'
				 * file system: %s aborts"
				 */
				FatalError(11086, "fsd");
			}
		}
		if ((mi->params.fi_config1 & MC_SHARED_FS) &&
		    (mi->params.fi_type == DT_META_OBJ_TGT_SET)) {
			/* "%s: 'moa' filesystem cannot be shared" */
			FatalError(17261, mi->params.fi_name);
		}
		if ((msg = MountCheckParams(&mi->params)) != NULL) {
			ConfigFileMsg(msg, 0, NULL);
			errors++;
		}
	}
#endif /* sun */

	/*
	 * Configure filesystems.
	 */
	argv[2] = NULL;
	for (fn = 0; fn < FileSysNumof; fn++) {
		struct sam_mount_info *mi, *mi_prev;
		struct sam_mount_arg fsmount;
		int		i;
		int		diffs;
		int		lun_diffs;
		boolean_t	mounted_fs;

		mi = &FileSysTable[fn];
		diffs = -1;	/* New filesystem */

		/*
		 * Find the previous filesystem by this name.
		 */
		fsname = mi->params.fi_name;	/* For diffMsgFunc() */
		mounted_fs = FALSE;
		for (i = 0; i < prevFileSysNumof; i++) {
			mi_prev = &prevFileSysTable[i];
			if (strncmp(mi->params.fi_name,
			    mi_prev->params.fi_name,
			    sizeof (mi->params.fi_name)) == 0) {
				break;
			}
		}
		if (i < prevFileSysNumof) {
			/* Note previous file system */
			*mi_prev->params.fi_name = '\0';
			if (mi_prev->params.fi_status &
			    (FS_MOUNTED | FS_MOUNTING |
			    FS_UMOUNT_IN_PROGRESS)) {
#ifdef linux
				if (Daemon) {
					Trace(TR_MISC, "Cannot reconfigure "
					    "mounted filesystem %s",
					    fsname);
				} else {
					/* Cannot reconfigure mounted fs %s */
					printf(GetCustMsg(17221), fsname);
					printf("\n");
				}
				continue;
#endif /* linux */
				mounted_fs = TRUE;
			}

			/*
			 * Compare configurations.
			 */
			diffs = 0;
			lun_diffs = 0;
			if (mi->params.fi_type != mi_prev->params.fi_type) {
				diffMsgFunc("FS type");
				diffs++;
			}
			if (mi->params.fs_count != mi_prev->params.fs_count) {
				diffMsgFunc("FS eq count");
				diffs++;
				lun_diffs++;
			}
			if (mi->params.mm_count != mi_prev->params.mm_count) {
				diffMsgFunc("FS metadata slice count");
				diffs++;
				lun_diffs++;
			}
			if (mi->params.fi_eq != mi_prev->params.fi_eq) {
				diffMsgFunc("FS eq");
				diffs++;
			}
			if ((mi->params.fi_config1 & MC_SHARED_FS) !=
			    (mi_prev->params.fi_config1 & MC_SHARED_FS)) {
				diffMsgFunc("Shared");
				diffs++;
			}
			for (i = 0; i < mi_prev->params.fs_count; i++) {
				char	msg[512];

				if (strncmp(mi->part[i].pt_name,
				    mi_prev->part[i].pt_name,
				    sizeof (mi->part[i].pt_name)) != 0) {
					diffs++;
					snprintf(msg, sizeof (msg),
					    "Ord %d name %s/%s", i,
					    mi_prev->part[i].pt_name,
					    mi->part[i].pt_name);
					diffMsgFunc(msg);
					diffs++;
					break;
				} else if (mi->part[i].pt_eq !=
				    mi_prev->part[i].pt_eq) {
					snprintf(msg, sizeof (msg),
					    "Ord %d eq %d/%d", i,
					    mi_prev->part[i].pt_eq,
					    mi->part[i].pt_eq);
					diffMsgFunc(msg);
					diffs++;
					break;
				} else if (mi->part[i].pt_type !=
				    mi_prev->part[i].pt_type) {
					snprintf(msg, sizeof (msg),
					    "Ord %d type %x/%x", i,
					    mi_prev->part[i].pt_type,
					    mi->part[i].pt_type);
					diffMsgFunc(msg);
					diffs++;
					break;
				}
			}
			diffs += SetfieldDiff(&mi->params, &mi_prev->params,
			    MountParams, diffMsgFunc);
		} else {
			mi_prev = NULL;
		}
		if (diffs == 0) {
			/*
			 * Same as previous filesystem.
			 */
			if (Daemon) {
				Trace(TR_MISC, "Filesystem %s unchanged",
				    fsname);
			} else if (Verbose) {
				/* Filesystem %s unchanged */
				printf(GetCustMsg(17231), fsname);
				printf("\n");
			}
			continue;
		}

		memset((char *)&fsmount, 0, sizeof (fsmount));
		strncpy(fsmount.fs_name, mi->params.fi_name,
		    sizeof (fsmount.fs_name));
		if (diffs > 0) {
			if (mounted_fs) {
				/*
				 * Can reconfigure mounted fs for LUN
				 * diffs only.
				 */
				if (diffs > lun_diffs) {
					if (Daemon) {
						Trace(TR_MISC,
						    "Cannot reconfigure "
						    "mounted filesystem %s",
						    fsname);
					} else {
						/*
						 * Cannot reconfigure
						 * mounted fs %s
						 */
						printf(GetCustMsg(17221),
						    fsname);
						printf("\n");
					}
					continue;
				}

			} else {
				/*
				 * If file system is not mounted and
				 * there are diffs, remove it.
				 */
				/*
				 * Stop shared daemon if there was one.
				 * Wait a second for it to terminate.
				 */
				if (mi_prev != NULL &&
				    (mi_prev->params.fi_config1 &
				    MC_SHARED_FS)) {
					argv[0] = SAM_SHAREFSD;
					argv[1] = mi->params.fi_name;
					StopProcess(argv, TRUE, SIGINT);
					sleep(1);
				}

				/*
				 * Remove this filesystem.
				 */
				fsmount.fs_count = mi->params.fs_count;
				fsmount.mt.ptr = NULL;
				if (Daemon) {
					if (sam_syscall(SC_setmount,
					    (void *)&fsmount,
					    sizeof (fsmount)) < 0) {
						if (errno == EBUSY) {
							/*
							 * Cannot reconfigure
							 * busy filesystem %s
							 */
							SendCustMsg(HERE,
							    17227,
							    fsmount.fs_name);
						} else {
							/*
							 * Set mount values
							 * failed for %s
							 */
							LibError(NULL, 0, 17222,
							    fsmount.fs_name);
						}
						continue;
					}
					Trace(TR_MISC, "Unconfigured FS %s",
					    fsmount.fs_name);
				}
			}
		}

		/*
		 * New or different configuration.
		 * Configure this filesystem.
		 */
		fsmount.fs_count = mi->params.fs_count;
		fsmount.mt.ptr = mi;
		if (Daemon || FsCfgOnly) {
			if (sam_syscall(SC_setmount,
			    (void *)&fsmount, sizeof (fsmount)) < 0) {
				/* Set mount values failed for %s */
				LibError(NULL, 0, 17222, fsmount.fs_name);
				continue;
			}
			if (diffs == -1) {
				Trace(TR_MISC, "Configured %s, config=%x,"
				    " config1=%x",
				    mi->params.fi_name, mi->params.fi_config,
				    mi->params.fi_config1);
			} else {
				Trace(TR_MISC, "Reconfigured %s, config=%x,"
				    " config1=%x",
				    mi->params.fi_name, mi->params.fi_config,
				    mi->params.fi_config1);
			}
		} else {
			if (diffs == -1) {
				/* Would configure %s */
				printf(GetCustMsg(17232), mi->params.fi_name);
			} else {
				/* Would reconfigure %s */
				printf(GetCustMsg(17233), mi->params.fi_name);
			}
			printf("\n");
		}
	}

	/*
	 * Remove remaining previous filesystems.
	 */
	argv[2] = NULL;
	for (fn = 0; fn < prevFileSysNumof; fn++) {
		struct sam_mount_info *mi;
		struct sam_mount_arg fsmount;

		mi = &prevFileSysTable[fn];
		if (*mi->params.fi_name == '\0') {
			/*
			 * If name is blank, previous filesystem was processed.
			 */
			continue;
		}

		if (mi->params.fi_status &
		    (FS_MOUNTED | FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) {
			/* Cannot remove mounted filesystem %s */
			SendCustMsg(HERE, 17228, mi->params.fi_name);
			continue;
		}

		if (mi->params.fi_config1 & MC_SHARED_FS) {
			/*
			 * Wake any shared daemon servicing this FS.
			 * Stop share daemon & wait a sec. for it to terminate.
			 */
			(void) sam_shareops(mi->params.fi_name,
			    SHARE_OP_WAKE_SHAREDAEMON, 0);
		}
		/*
		 * If shared, stop shared daemon & wait 1 sec. for it to
		 * terminate.
		 */
		if (mi->params.fi_config1 & MC_SHARED_FS) {
			argv[0] = SAM_SHAREFSD;
			argv[1] = mi->params.fi_name;
			StopProcess(argv, TRUE, SIGINT);
			sleep(1);
		}

		memset((char *)&fsmount, 0, sizeof (fsmount));
		strncpy(fsmount.fs_name, mi->params.fi_name,
		    sizeof (fsmount.fs_name));
		fsmount.fs_count = mi->params.fs_count;
		fsmount.mt.ptr = NULL;
		if (Daemon) {
			if (sam_syscall(SC_setmount,
			    (void *)&fsmount, sizeof (fsmount)) < 0) {
				if (errno == EBUSY) {
					/* Cannot remove busy filesystem %s */
					SendCustMsg(HERE, 17229,
					    fsmount.fs_name);
				} else {
					/* Set mount values failed for %s */
					LibError(NULL, 0, 17222,
					    fsmount.fs_name);
				}
				continue;
			}
			Trace(TR_MISC, "Unconfigured FS %s",
			    mi->params.fi_name);
		} else {
			/* Would remove %s */
			printf(GetCustMsg(17234), mi->params.fi_name);
			printf("\n");
		}

	}
	/*
	 * Get an up to date copy of the filesystem information.
	 */
	getPrevFilesystems();
	if (FileSysTable != NULL) {
		SamFree(FileSysTable);
	}
	FileSysNumof = prevFileSysNumof;
	FileSysTable = prevFileSysTable;

	/*
	 * Start sam-sharefsd if filesystem is shared and already mounted
	 */
	argv[2] = NULL;
	for (fn = 0; fn < FileSysNumof; fn++) {
		struct sam_mount_info *mi;

		mi = &FileSysTable[fn];
		if (mi->params.fi_config1 & MC_SHARED_FS) {
			/*
			 * Wake any shared daemon servicing this FS, even if
			 * it doesn't happen to be a child of this sam-fsd.
			 * It should wake, realize it's an orphan, and exit.
			 */
			(void) sam_shareops(mi->params.fi_name,
			    SHARE_OP_WAKE_SHAREDAEMON, 0);
			if (mi->params.fi_status &
			    (FS_MOUNTED | FS_MOUNTING |
			    FS_UMOUNT_IN_PROGRESS)) {
				/*
				 * If this daemon is initializing, and the
				 * FS is already in use, start a sam-sharefsd
				 * for it.
				 */
				argv[0] = SAM_SHAREFSD;
				argv[1] = mi->params.fi_name;
				StartProcess(argv, CP_respawn | CP_qstart, 0);
			}
		}
	}

	if (FsCfgOnly) {
		return;
	}

	/*
	 * Start daemons.
	 */
	argv[2] = NULL;
	ArchiveCount = 0;
	for (fn = 0; fn < FileSysNumof; fn++) {
		struct sam_mount_info *mi;

		mi = &FileSysTable[fn];
		argv[1] = mi->params.fi_name;
		if (mi->params.fi_config & MT_SAM_ENABLED) {
			ArchiveCount++;
		}
	}
}


/*
 * Differences message function.
 */
static void
diffMsgFunc(char *name)
{
	if (Daemon) {
		Trace(TR_MISC, "%s is different for %s", name, fsname);
	} else {
		/* %s is different for %s */
		printf(GetCustMsg(17235), name, fsname);
		printf("\n");
	}
}


/*
 * Get previous filesystems.
 */
static void
getPrevFilesystems(void)
{
	struct sam_fs_status *fsarray;
	size_t	size;
	int		i;

	/*
	 * Get number and names of filesystems.
	 */
	prevFileSysNumof = GetFsStatus(&fsarray);
	if (prevFileSysNumof == -1) {
		FatalError(0, "GetFsStatus failed.");
	}
	if (prevFileSysNumof == 0) {
		return;
	}

	size = prevFileSysNumof * sizeof (struct sam_mount_info);
	SamRealloc(prevFileSysTable, size);
	memset(prevFileSysTable, 0, size);

	/*
	 * Get the mount information for each.
	 */
	for (i = 0; i < prevFileSysNumof; i++) {
		struct sam_fs_status *fs;
		struct sam_mount_info *mp;

		fs = &fsarray[i];
		mp = &prevFileSysTable[i];
		if (GetFsMountDefs(fs->fs_name, mp) < 0) {
			/* GetFsMountDefs(%s) failed */
			LibError(NULL, 0, 17260, fs->fs_name);
			continue;
		}
		/* Copy the mount status bits from fsarray into mp */
		mp->params.fi_status |= (fs->fs_status &
		    (FS_MOUNTED | FS_MOUNTING | FS_UMOUNT_IN_PROGRESS));
	}
	free(fsarray);
}


/*
 * Get all file system devices.
 */
static void
getFsDevices(void)
{
	int		i;
	char	msg_buf[MAX_MSGBUF_SIZE];

	/*
	 * Find all file system devices and make the FsDevice table.
	 */
	for (i = 0; i < DeviceNumof; i++) {
		struct sam_mount_info *mi;
		struct sam_fs_info *mp;
		dev_ent_t *dev;

		dev = &DeviceTable[i];
		if ((dev->type & DT_CLASS_MASK) != DT_FAMILY_SET) {
			continue;
		}

		/*
		 * File system device found.
		 */
		FileSysNumof++;
		SamRealloc(FileSysTable, FileSysNumof *
		    sizeof (struct sam_mount_info));
		mi = &FileSysTable[FileSysNumof - 1];
		memset(mi, 0, sizeof (struct sam_mount_info));

		mp = &mi->params;
		strcpy(mp->fi_name, dev->set);
		mp->fi_eq = dev->eq;
		mp->fi_type = dev->type;

		/*
		 * Get partition (devices) in this filesystem.
		 */
		if (gatherFsDevices(mi) != 0) {
			/* Problem in mcf file %s for filesystem %s */
			errno = 0;
			LibError(NULL, 0, 17210, McfName, mp->fi_name);

			snprintf(msg_buf, sizeof (msg_buf),
			    GetCustMsg(17210), McfName, mp->fi_name);

			PostEvent(FSD_CLASS, "CfgErr", 17210, LOG_ALERT,
			    msg_buf,
			    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
			errors++;
		}
	}
}


/*
 * gatherFsDevices  - Gather devices belonging to family set.
 *
 * Builds the block device link list for the family set.
 */
static int			/* -1 if an error occured, 0 if successful. */
gatherFsDevices(struct sam_mount_info *mi)
{
	struct sam_fs_info *mp;
	struct sam_fs_part *fsp;
	int err = 0;
	int fam_set_count, meta_set_count;
	int		i;

	errno = 0;
	mp = &mi->params;
	/*
	 * Scan devices link list for this filesystem. Verify there is
	 * only 1 entry for this filesystem.
	 */
	fam_set_count = 0;
	for (i = 0; i < DeviceNumof; i++) {
		dev_ent_t *dev;

		dev = &DeviceTable[i];
		if (strncmp(mp->fi_name, dev->name, sizeof (mp->fi_name))
		    == 0 &&
		    (dev->type & DT_CLASS_MASK) == DT_FAMILY_SET) {
			fam_set_count++;
			/*
			 * Set shared flag if set in mcf.
			 */
			if ((dev->dt.sc.name[0] != '\0') &&
			    (strcmp(dev->dt.sc.name, "shared")) == 0) {
				mp->fi_config1 |= MC_SHARED_FS;
			}
		}
	}
	if (fam_set_count != 1) {
		if (fam_set_count > 1) {
			/* Family set %s is duplicated in mcf %s */
			fprintf(stderr, GetCustMsg(17211), mp->fi_name,
			    McfName);
		} else {
			/* Family set %s is missing from mcf %s */
			fprintf(stderr, GetCustMsg(17212), mp->fi_name,
			    McfName);
		}
		fprintf(stderr, "\n");
		return (-1);
	}


	/*
	 * Count the number of meta and data devices in this filesystem.
	 */
	fam_set_count = 0;
	meta_set_count = 0;
	for (i = 0; i < DeviceNumof; i++) {
		dev_ent_t *dev;

		dev = &DeviceTable[i];
		if (strncmp(mp->fi_name, dev->set, sizeof (mp->fi_name)) != 0 ||
		    (dev->type & DT_CLASS_MASK) == DT_FAMILY_SET) {
			continue;
		}
		fam_set_count++;
		if (dev->type == DT_META)  meta_set_count++;
	}

	if (fam_set_count == 0) {
		/* File system %s has no devices. */
		fprintf(stderr, GetCustMsg(17213), mp->fi_name);
		fprintf(stderr, "\n");
		return (-1);
	}
	if (fam_set_count > L_FSET) {
		/* File system %s has too many (%d) devices (limit = %d). */
		fprintf(stderr, GetCustMsg(17239), mp->fi_name,
		    fam_set_count, L_FSET);
		fprintf(stderr, "\n");
		return (-1);
	}
	mp->fs_count = fam_set_count;
	mp->mm_count = meta_set_count;
	if (fam_set_count == meta_set_count) {
		/* File system %s has no data devices. */
		fprintf(stderr, GetCustMsg(17237), mp->fi_name);
		fprintf(stderr, "\n");
		return (-1);
	}
	if ((mp->fi_type == DT_META_SET) ||
	    (mp->fi_type == DT_META_OBJECT_SET) ||
	    (mp->fi_type == DT_META_OBJ_TGT_SET)) {
		if (meta_set_count == 0) {
			/* File system %s has no meta devices. */
			fprintf(stderr, GetCustMsg(13411), mp->fi_name);
			fprintf(stderr, "\n");
			return (-1);
		}
	}

	/*
	 * Scan the devices link list and build the data devices array.
	 * Verify the data devices (DT_DISK) exist and are block devices.
	 * Check for md devices in the ms filesystem. Check for mr and or
	 * gx devices in the ma filesystem.
	 */
	fsp = &mi->part[0];
	for (i = 0; i < DeviceNumof; i++) {
		dev_ent_t *dev;
		struct stat sb;
		int fd;

		dev = &DeviceTable[i];

		if (strncmp(mp->fi_name, dev->set, sizeof (mp->fi_name)) != 0 ||
		    (dev->type & DT_CLASS_MASK) == DT_FAMILY_SET) {
			continue;
		}
		memmove(fsp->pt_name, dev->name, sizeof (fsp->pt_name));
		fsp->pt_eq	= dev->eq;
		fsp->pt_type	= dev->type;
		fsp->pt_state	= dev->state;
		if ((mi->params.fi_config1 & MC_SHARED_FS) &&
		    ((mp->fi_type == DT_META_SET) ||
		    (mp->fi_type == DT_META_OBJECT_SET)) &&
		    (dev->type == DT_META)) {
			if (strcmp((char *)&dev->name, "nodev") == 0) {
				goto adddev;
			}
		}
		if ((mp->fi_type == DT_DISK_SET) && (dev->type != DT_DATA)) {
			/* File system %s has invalid devices. */
			LibError(NULL, 0, 17215, mp->fi_name);
			err++;
			continue;
		}
		if (((mp->fi_type == DT_META_SET) ||
		    (mp->fi_type == DT_META_OBJ_TGT_SET)) &&
		    (!is_stripe_group(dev->type)) &&
		    (dev->type != DT_META) &&
		    (dev->type != DT_DATA) &&
		    (dev->type != DT_RAID)) {
			/* File system %s has invalid devices. */
			LibError(NULL, 0, 17215, mp->fi_name);
			err++;
			continue;
		}
		if ((mp->fi_type == DT_META_OBJECT_SET) &&
		    (dev->type != DT_META) &&
		    (dev->type != DT_OBJECT)) {
			/* File system %s has invalid devices. */
			LibError(NULL, 0, 17215, mp->fi_name);
			err++;
			continue;
		}
		if (mp->fi_type == DT_META_OBJECT_SET) {
			mp->fi_config1 |= MC_OBJECT_FS;
		}

		if (is_stripe_group(dev->type)) {
			mp->fi_config1 |= MC_STRIPE_GROUPS;
		}
		if (dev->type == DT_DATA) {
			mp->fi_config1 |= MC_MD_DEVICES;
		}
		if (dev->type == DT_RAID) {
			mp->fi_config1 |= MC_MR_DEVICES;
		}
		if ((mp->fi_config1 & MC_MD_DEVICES) &&
		    (mp->fi_config1 &
		    (MC_STRIPE_GROUPS | MC_MR_DEVICES))) {
			/* File system %s has invalid devices. */
			LibError(NULL, 0, 17215, mp->fi_name);
			err++;
			continue;
		}
		if ((mp->fi_config1 & MC_MR_DEVICES) &&
		    (mp->fi_config1 & MC_MD_DEVICES)) {
			/* File system %s has invalid devices. */
			LibError(NULL, 0, 17215, mp->fi_name);
			err++;
			continue;
		}
		if (stat(dev->name, &sb) != 0) {
			char *msg_str;

			if (errno != ENOENT) {
				/* Cannot stat %s */
				msg_str = GetCustMsg(616);
			} else {
				/* Device %s not found */
				msg_str = GetCustMsg(859);
			}
			if (Daemon) {
				sam_syslog(LOG_WARNING, msg_str, dev->name);
			} else {
				fprintf(stderr, msg_str, dev->name);
				fprintf(stderr, "\n");
			}
			err++;
			continue;
		} else if (dev->state == DEV_ON || dev->state == DEV_NOALLOC) {
			if (dev->type == DT_OBJECT) {
				if (!S_ISCHR(sb.st_mode)) {
					/* %s must be character special file. */
					LibError(NULL, 0, 17259, dev->name);
					err++;
					continue;
				}
			} else {
				if (!S_ISBLK(sb.st_mode)) {
					/* %s must be block special file. */
					LibError(NULL, 0, 17214, dev->name);
					err++;
					continue;
				}
			}
		}
#if sun
		/*
		 * Open device to get size
		 */
		if (dev->state == DEV_ON || dev->state == DEV_NOALLOC) {
			if (dev->type == DT_OBJECT) {
				uint64_t	oh;	/* osd_handle_t */

				if ((open_obj_device(dev->name, O_RDONLY, &oh))
				    < 0) {
					LibError(NULL, 0, 17263, dev->name);
					err++;
					continue;
				}
				close_obj_device(dev->name, O_RDONLY, oh);
			} else {
				if ((fd = get_blk_device(fsp, O_RDONLY)) < 0) {
					/* %s must be char special file. */
					LibError(NULL, 0, 17214, dev->name);
					err++;
					continue;
				}
				close(fd);
			}
		}
#endif

adddev:
		fsp++;
	}

	if (err > 0) {
		return (-1);
	}
	return (0);
}
