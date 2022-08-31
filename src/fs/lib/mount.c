/*
 * mount.c - Mount parameter operations.
 */

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

#pragma ident "$Revision: 1.60 $"

#include "sam/osversion.h"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers */
#include "unistd.h"
#include "fcntl.h"

/* Solaris headers. */
#include "sys/mount.h"
#include "sys/wait.h"
#include <sys/stat.h>
#ifdef sun
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/mnttab.h>
#include <sys/efi_partition.h>
#include "efilabel.h"
#include <libscf.h>
#endif /* sun */

/* SAM-FS headers. */
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "utility.h"

#define SAM_FSD_FMRI "svc:/system/"SAM_FSD":default"

extern char *getfullrawname(char *);

/*
 * Set/clear clustermgmt mount option based on SC indicator file.
 * The indicator file is set by SetSCIndicatorFile().
 */
void
SetSCMountParams(struct sam_fs_info *mp)
{
	char	scpath[MAXPATHLEN];
	struct	stat sb;

	if (mp == NULL || mp->fi_name == NULL) {
		return;
	}
	sprintf(scpath, SAM_SC_DIR"/%s", mp->fi_name);

	if (lstat(scpath, &sb) == 0) {
		mp->fi_config1 |= MC_CLUSTER_MGMT;
	} else {
		mp->fi_config1 &= ~MC_CLUSTER_MGMT;
	}
}


/*
 * Check mount parameters.
 */
char *
MountCheckParams(struct sam_fs_info *mp)
{
	static char msg_buf[128];
	int i;
	char *param;

#ifdef sun
	if (mp->fi_high < mp->fi_low || mp->fi_high > 100 ||
	    (mp->fi_high == mp->fi_low && mp->fi_low < 100)) {
		/*
		 * "High (%d) must be greater than low (%d) and less than
		 * 100.
		 */
		snprintf(msg_buf, sizeof (msg_buf), GetCustMsg(1296),
		    mp->fi_high, mp->fi_low);
		return (msg_buf);
	}
#endif /* sun */
	if (mp->fi_config & MT_SHARED_READER) {
		mp->fi_mflag |= MS_RDONLY;
	}
#if defined(DEBUG)
	mp->fi_config |= MT_TRACE;
#endif

#ifdef sun
	/*
	 * Partial  & partial_stage cannot be greater than maxpartial.
	 * If partial_stage not set, default to partial.
	 */
	if (mp->fi_partial > mp->fi_maxpartial) {
		mp->fi_partial = mp->fi_maxpartial;
	}
	if (mp->fi_partial_stage < 0) {
		mp->fi_partial_stage = mp->fi_partial * 1024;
	}
	if (mp->fi_partial_stage > (mp->fi_maxpartial * 1024)) {
		mp->fi_partial_stage = mp->fi_maxpartial * 1024;
	}
#endif /* sun */

	/*
	 * By default, set fi_wr_throttle to 2% of memory size.
	 */
	if (mp->fi_wr_throttle < 0) {		/* If not set in config files */
		long pages, pgsize;
		int64_t memsize;
		float wr_throttle;

		pages = sysconf(_SC_PHYS_PAGES);
		pgsize = sysconf(_SC_PAGESIZE);
		memsize = (int64_t)pages * (int64_t)pgsize;
		wr_throttle = (float)memsize * .02;
		mp->fi_wr_throttle =
		    (((int64_t)wr_throttle + 1023)/1024) * 1024;
	}

	/*
	 * If object file system ("mb"), turn off dio_szero.
	 * Set object stripe depth to power of 2. Compute obj depth shift.
	 */
	if (mp->fi_config1 & MC_OBJECT_FS) {
		offset_t stripe_depth = 0;
		offset_t value;
		int stripe_shift;

		stripe_depth = mp->fi_obj_depth;
		value = SAM_DEV_BSIZE;
		stripe_shift = SAM_DEV_BSHIFT;
		for (;;) {
			if (value >= stripe_depth) {
				stripe_depth = value;
				break;
			}
			stripe_shift++;
			value <<= 1;
		}
		mp->fi_obj_depth_shift = stripe_shift;
		mp->fi_config &= ~MT_ZERO_DIO_SPARSE;
	}

	/*
	 * Set fi_sync_meta for sync meta parameter.
	 */
	if (mp->fi_sync_meta < 0) {
		if (mp->fi_config1 & MC_SHARED_FS) {
			mp->fi_sync_meta = 1;
		} else {
			mp->fi_sync_meta = 0;
		}
	}
	if (mp->fi_sync_meta) {
		mp->fi_config |= MT_SYNC_META;
	} else {
		mp->fi_config &= ~MT_SYNC_META;
	}

	/*
	 * noatime set the parameter atime = -1.
	 */
	if (mp->fi_config & MT_NOATIME) {
		mp->fi_atime = -1;
	}

	/*
	 * Disallow certain options for the ms filesystem.
	 */
	param = NULL;
	if (mp->fi_config & MT_SHARED_WRITER) {
		param = "writer";
		mp->fi_config |= MT_SYNC_META;
		/* Always write access time updates to disk */
		mp->fi_atime = 1;
		if ((mp->fi_config & MT_SHARED_WRITER) &&
		    (mp->fi_config & MT_SHARED_READER)) {
			snprintf(msg_buf, sizeof (msg_buf), GetCustMsg(13210),
			    "reader", param);
			return (msg_buf);
		}
	} else if (mp->fi_config & MT_SHARED_READER) {
		param = "reader";
	}
	if (!(mp->fi_config1 & MC_SHARED_FS) &&
	    (mp->fi_config & MT_SHARED_MO)) {
		snprintf(msg_buf, sizeof (msg_buf), GetCustMsg(13210),
		    "Non shared fs", "shared");
		return (msg_buf);
	}
	if ((mp->fi_config & MT_SHARED_MO) &&
	    (mp->fi_config & MT_SHARED_WRITER)) {
		snprintf(msg_buf, sizeof (msg_buf), GetCustMsg(13210),
		    "shared", param);
		return (msg_buf);
	}

	/*
	 * Inode buffer cache size must be a power of 2
	 */
	for (i = 16384; i >= 512; i = i >> 1) {
		if (mp->fi_rd_ino_buf_size >= i) {
			mp->fi_rd_ino_buf_size = i;
			break;
		}
	}
	for (i = 16384; i >= 512; i = i >> 1) {
		if (mp->fi_wr_ino_buf_size >= i) {
			mp->fi_wr_ino_buf_size = i;
			break;
		}
	}
	if (mp->fi_wr_ino_buf_size > mp->fi_rd_ino_buf_size) {
		mp->fi_wr_ino_buf_size = 512;
	}

	return (NULL);
}


/*
 * Check filesystem.
 * If the filesystem is not configured, use sam-fsd to configure it.
 * Only run 1 sam-fsd. Note, ChkFs can be called multiple times
 * due to mountall calling mount in parallel.
 */
#define	PKGCHK "/etc/opt/SUNWsamfs/startup/sampkgchk"
void
ChkFs(void)
{
	struct sam_fs_status *fsarray = NULL;
	flock_t	fsd_lock;
	char	*fsdLockName = SAM_SAMFS_PATH"/"SAM_FSD;
	char	*pkgchk = PKGCHK;
	int		fd;
	int		pid;
	int		status;

#ifdef sun
	if (system(pkgchk)) {
		exit(EXIT_FAILURE);
	}

	if ((fd = open(fsdLockName, O_RDWR)) < 0) {
		error(EXIT_FAILURE, errno, GetCustMsg(613), fsdLockName);
	}
	memset(&fsd_lock, 0, sizeof (flock_t));
	fsd_lock.l_type = F_WRLCK;
	fsd_lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLKW, &fsd_lock) == -1) {
		close(fd);
		error(EXIT_FAILURE, errno, GetCustMsg(12020), program_name);
	}
#endif /* sun */

	LoadFS(NULL);

	if (GetFsStatus(&fsarray) > 0) {
		free(fsarray);
		/* it could be possible that sam-fsd was stopped, so we reenable it again */
		printf("%s: (RE)enabling the sam-fsd service.\n", program_name);
		if(smf_enable_instance(SAM_FSD_FMRI, SMF_TEMPORARY) == -1) {
			error(EXIT_FAILURE, scf_error(), "smf_enable_instance %s, scf_error: %s\n", SAM_FSD_FMRI, scf_strerror(scf_error()));
		}
		close(fd);
		return;
	}

	/*
	 * Filesystem is not configured.
	 * Remove any SC config files and execute sam-fsd to configure.
	 */
	(void) system("/bin/rm -f " SAM_SC_DIR"/*");

	/* "Configuring file system" */
	printf("%s: %s\n", program_name, GetCustMsg(12021));
	pid = fork();
	if (pid == -1) {
		error(EXIT_FAILURE, errno, "Cannot fork");
	} else if (pid == 0) {
		close(fd);
		execl(SAM_SAMFS_PATH"/"SAM_FSD, SAM_FSD, "-C", NULL);
		error(EXIT_FAILURE, errno, GetCustMsg(612),
		    SAM_SAMFS_PATH"/"SAM_FSD);
	} else {
		waitpid(pid, &status, 0);
		close(fd);
		if (status != 0) {
			exit(EXIT_FAILURE);
		}
	}
#ifdef sun
	/*
	 * Enable sam-fsd
	 */
	printf("%s: Enabling the sam-fsd service.\n", program_name);
        if(smf_enable_instance(SAM_FSD_FMRI, SMF_TEMPORARY) == -1) {
                error(EXIT_FAILURE, scf_error(), "smf_enable_instance %s, scf_error: %s\n", SAM_FSD_FMRI, scf_strerror(scf_error()));
	}
	close(fd);

#endif /* sun */
}


#ifdef sun

/*
 * ----- Open file system block device and get size
 */

int				/* fd if successful, -1 if error */
get_blk_device(
	struct sam_fs_part *fsp,
	int oflags,
	int maxdevretry)
{
	int fd;
	int retrycnt = 0;
	struct dk_cinfo dkcinfo;
	struct vtoc vtoc;
	char *devrname;

	if (check_mnttab(fsp->pt_name)) {
		error(0, 0, catgets(catfd, SET, 13422, "device %s is mounted."),
		    fsp->pt_name);
		return (-1);
	}

	if ((devrname = getfullrawname(fsp->pt_name)) == NULL) {
		error(0, 0,
		    catgets(catfd, SET, 1606,
		    "malloc: %s"), "getfullrawname");
		return (-1);
	}
	if (*devrname == '\0') {
		error(0, errno, "%s", fsp->pt_name);
		error(0, 0,
		    catgets(catfd, SET, 1998,
		    "Raw device not found for eq (%d)"),
		    fsp->pt_eq);
		free(devrname);
		return (-1);
	}
	/*
	 * Oracle RAC (oban) devices under SunCluster initialize at the
	 * same time as QFS filesystems are mounted. We loop waiting
	 * for the devices to become available (maximum of maxdevretry
	 * tries).
	 */
	while ((fd = open(devrname, oflags | O_NDELAY)) < 0) {
		if ((retrycnt >= maxdevretry) ||
		    ((strncmp(devrname, "/dev/md/", 8) != 0))) {
			error(0, errno, "%s", devrname);
			error(0, 0, catgets(catfd, SET, 1856,
			    "Open failed on (%s), retries=%d"),
			    devrname, retrycnt);
			free(devrname);
			return (-1);
		}
		retrycnt++;
		sleep(2);
	}
	if (retrycnt > 0) {
		printf("%s: %d retries on %s (max=%d).\n",
		    program_name, retrycnt, devrname, maxdevretry);
	}

	if (ioctl(fd, DKIOCINFO, &dkcinfo) < 0) {
		error(0, errno, "%s", devrname);
		error(0, 0,
		    catgets(catfd, SET, 1443,
		    "Ioctl(DKIOCINFO) failed on (%s)"),
		    devrname);
		free(devrname);
		return (-1);
	}
	if (ioctl(fd, DKIOCGVTOC, &vtoc) >= 0) {
		/*
		 * Size of partition is returned in units of 512
		 * byte sectors.
		 */
		fsp->pt_size = (unsigned long)
		    vtoc.v_part[dkcinfo.dki_partition].p_size;
		if (fsp->pt_size == 0) {
			error(0, 0,
			    catgets(catfd, SET, 1909,
			    "Partition %d is undefined on (%s)"),
			    dkcinfo.dki_partition, devrname);
			free(devrname);
			return (-1);
		}
	} else if (is_efi_present()) {
		int part;
		int saved_errno;
		struct dk_gpt *efi_vtoc;

		saved_errno = errno;
		if ((part = call_efi_alloc_and_read(fd, &efi_vtoc)) >= 0) {
			fsp->pt_size = efi_vtoc->efi_parts[part].p_size;
			call_efi_free(efi_vtoc);
			if (fsp->pt_size == 0) {
				error(0, 0, catgets(catfd, SET, 1909,
				    "Partition %d is undefined on (%s)"),
				    part, devrname);
				free(devrname);
				return (-1);
			}
		} else {
            /* workaround because https://www.illumos.org/issues/12339 */
            if (strcmp(dkcinfo.dki_cname,"zvol") == 0) {
                fsp->pt_size = efi_read(fd);
                if (fsp->pt_size == 0) {
				    error(0, 0, catgets(catfd, SET, 1909,
				        "Partition %d is undefined on (%s)"),
				        part, devrname);
				    free(devrname);
				    return (-1);
			    }
            } else { 
			    error(0, saved_errno, "%s", devrname);
			    error(0, 0, catgets(catfd, SET, 13030,
			        "Could not read VTOC or EFI label on %s: %d"),
			        devrname, part);
			    free(devrname);
			    return (-1);
            }
		}
	} else {
		error(0, errno, "%s", devrname);
		error(0, 0,
		    catgets(catfd, SET, 1442,
		    "Ioctl(DKIOCGVTOC) failed on (%s)"),
		    devrname);
		free(devrname);
		return (-1);
	}
	free(devrname);
	return (fd);
}


/*
 * ----- check_mnttab - Check the mount table.
 */

int		/* 1 if mounted, 0 if not mounted */
check_mnttab(char *mnt_special)
{
	struct mnttab mntsrc;
	FILE *mntfd;
	int err = 0;

	memset(&mntsrc, 0, sizeof (struct mnttab));
	if ((mntfd = fopen(MNTTAB, "r")) == (FILE *)NULL) {
		perror("mount_samfs: fopen(mnttab) error: ");
		return (0);
	}
	while (getmntent(mntfd, &mntsrc) >= 0) {
		if (strcmp(mntsrc.mnt_special, mnt_special) == 0) {
			err = 1;
			break;
		}
	}
	(void) fclose(mntfd);
	return (err);
}
#endif /* sun */
