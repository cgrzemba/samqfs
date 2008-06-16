/*
 *	configrw.c - configuration r/w functions.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.53 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#ifdef linux
#define	_XOPEN_SOURCE 600
#include <string.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <unistd.h>

#ifdef linux
#include <sys/types.h>
#ifndef __USE_GNU
#define	__USE_GNU
#endif
#define	MEMALIGN(a, b, c) posix_memalign((void **)&(a), b, c)
#else
#define	MEMALIGN(a, b, c) (a) = memalign(b, c)
#endif

#define	LX_ALIGN	(4096)	/* alignment req'd for linux direct I/O */

/* POSIX headers. */
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/exit.h"
#include "sblk.h"
#include "samhost.h"

/* Local headers. */
#include "sharefs.h"
#include "config.h"
#ifdef METADATA_SERVER
#include "server.h"
#endif	/* METADATA_SERVER */

extern char *getfullrawname();

extern struct shfs_config config;
extern int byte_swap_sb(struct sam_sblk *sblk, size_t len);
extern int byte_swap_hb(struct sam_host_table_blk *hb);
extern int byte_swap_lb(struct sam_label_blk *lb);

static char *htp[2] = {NULL, NULL};

/*
 * Local private functions
 */
static int tryDevs(char *fs);

static int getRootInfo(int rfd, uint64_t oh, char *fs, int ord,
	struct sam_sblk *sb, struct cfdata *cfp);
static int getLabelInfo(int rfd, uint64_t oh, char *fs, int ord,
	struct sam_label_blk *lb);
static int checkPartOrdinal(struct sam_sblk *sb, int part);

static int rootFd = -1;
static int labelFd = -1;
static uint64_t labelOh = 0;
static upath_t labelName;

extern int OpenFsDevOrd(char *fs, ushort_t ord, int *devfd, int oflag);

/*
 * OpenDevs(char *fs)
 *
 * Try to open the root and label slices.  If the devices don't
 * exist or are busy, keep trying (but log once for posterity).
 */
void
OpenDevs(char *fs)
{
	int i, r;

	for (i = 0; (r = tryDevs(fs)) != 0; i++) {
		switch (r) {
		case ENODEV:
			if (i == 0) {
				SysError(HERE, "FS %s: FS devices "
				    "unavailable", fs);
			}
			break;

		case EBUSY:
			if (i == 0) {
				SysError(HERE, "FS %s: FS devices busy", fs);
			}
			if (i > 30 && !FsCfgOnly) {
				exit(EXIT_FAILURE);	/* allow restarts */
			}

		default:
			SysError(HERE, "FS %s: FS device opens failed", fs);
			exit(EXIT_NORESTART);
		}
		sleep(2);
	}
}


static int
tryDevs(char *fs)
{
	char name[MAXPATHLEN];
	struct sam_mount_info mnt;
	int d;

	CloseDevs();

	/*
	 * Get the OS'es idea of the FS, and open the necessary devices.
	 */
	if (GetFsInfo(fs, &mnt.params) < 0) {
		SysError(HERE, "FS %s: syscall[GetFsInfo] failed", fs);
		goto errout;
	}
	if ((mnt.params.fi_config1 & MC_SHARED_FS) == 0) {
		errno = 0;
		SysError(HERE, "FS %s: not a shared filesystem", fs);
		goto errout;
	}
	if (mnt.params.fs_count <= 0 || mnt.params.fs_count > L_FSET) {
		SysError(HERE, "FS %s: syscall[GetFsInfo]: unreasonable "
		    "fs_count (%d)",
		    fs, (int)mnt.params.fs_count);
		goto errout;
	}
	if (GetFsParts(fs, mnt.params.fs_count, &mnt.part[0]) < 0) {
		SysError(HERE, "FS %s: syscall[GetFsParts] failed", fs);
		goto errout;
	}

	/*
	 * Get the superblock and hosts info if we have a dev name
	 */
	if (strcmp(mnt.part[0].pt_name, "nodev") != 0) {
		if (!Dsk2Rdsk(mnt.part[0].pt_name, name)) {
			errno = 0;
			SysError(HERE, "FS %s: no raw dev name for "
			    "superblock on '%s'",
			    fs, mnt.part[0].pt_name);
			goto errout;
		}
		if ((rootFd = open(name, OPEN_READ_RAWFLAGS)) < 0) {
			switch (errno) {
			case EBUSY:
				goto busyout;
			case ENOENT:
			case ENXIO:
			case ENODEV:
				goto nodevout;
			}
			SysError(HERE, "FS %s: Root slice open(%s) failed",
			    fs, name);
			goto errout;
		}
	}

	/*
	 * get the label block dev
	 */
	if ((mnt.params.fi_type == DT_META_SET) ||
	    (mnt.params.fi_type == DT_META_OBJECT_SET)) {
		for (d = 0; d < mnt.params.fs_count; d++) {
			if (mnt.part[d].pt_type != DT_META) {
				break;
			}
		}
		if (d == 0 || d == mnt.params.fs_count) {
			errno = 0;
			SysError(HERE, "FS %s: no data partition", fs);
			goto errout;
		}
	} else {
		d = 0;
	}
	strcpy(labelName, mnt.part[d].pt_name);
	if (is_target_group(mnt.part[d].pt_type)) {
		if ((open_obj_device(mnt.part[d].pt_name, OPEN_READ_RAWFLAGS,
		    &labelOh)) < 0) {
			SysError(HERE,
			    "FS %s: syscall[open_obj_device] failed for %s",
			    fs, mnt.part[d].pt_name);
			goto errout;
		}
	} else {
		if (!Dsk2Rdsk(mnt.part[d].pt_name, name)) {
			errno = 0;
			SysError(HERE, "FS %s: no raw dev name for label block "
			    "on '%s'",
			    fs, mnt.part[d].pt_name);
			goto errout;
		}
		if ((labelFd = open(name, OPEN_READ_RAWFLAGS)) < 0) {
			switch (errno) {
			case EBUSY:
				goto busyout;
			case ENOENT:
			case ENXIO:
			case ENODEV:
				goto nodevout;
			}
			SysError(HERE, "FS %s: Label slice open(%s) failed",
			    fs, name);
			goto errout;
		}
	}
	return (0);

nodevout:
	CloseDevs();
	return (ENODEV);

busyout:
	CloseDevs();
	return (EBUSY);

errout:
	CloseDevs();
	return (-1);
}

void
CloseDevs(void)
{
	if (rootFd != -1) {
		(void) close(rootFd);
	}
	rootFd = -1;

	if (labelFd != -1) {
		(void) close(labelFd);
	}
	labelFd = -1;

	if (labelOh != 0) {
		(void) close_obj_device(labelName, O_RDWR, labelOh);
	}
	labelOh = 0;
}

/*
 * Go out and look at all the configuration bits we care about.
 * Stash them into the free config structure.
 *
 */
int
GetConfig(char *fs, struct sam_sblk *sb)
{
	struct stat sbuf;
	struct cfdata *cfp;
	char name[MAXPATHLEN];
	struct sam_mount_info mnt;
	int d;
	int rfd = -1;
	uint64_t oh = 0;
	int err = 0;

	cfp = &config.cp[config.freeTab];
	bzero((char *)cfp, sizeof (*cfp));

	if (htp[config.freeTab] == NULL) {
		htp[config.freeTab] =
		    (char *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
	}
	bzero(htp[config.freeTab], SAM_LARGE_HOSTS_TABLE_SIZE);
	cfp->ht = (struct sam_host_table_blk *)htp[config.freeTab];

	cfp->flags |= R_FSCFG;

	/*
	 * Get the OS'es idea of the FS, and either save it into
	 * the config info (if that's uninitialized) or compare them.
	 */
	if (GetFsInfo(fs, &mnt.params) < 0) {
		SysError(HERE, "FS %s: syscall[GetFsInfo] failed", fs);
		cfp->flags |= R_FSCFG_ERR;
		return (-1);
	}
	if ((mnt.params.fi_config1 & MC_SHARED_FS) == 0) {
		errno = 0;
		SysError(HERE, "FS %s: not a shared filesystem", fs);
		cfp->flags |= R_FSCFG_ERR;
		return (-1);
	}
	if (mnt.params.fs_count <= 0 || mnt.params.fs_count > L_FSET) {
		SysError(HERE, "FS %s: syscall[GetFsInfo]: bad fs_count (%d)",
		    fs, (int)mnt.params.fs_count);
		return (-1);
	}
	if (GetFsParts(fs, mnt.params.fs_count, &mnt.part[0]) < 0) {
		SysError(HERE, "FS %s: syscall[GetFsParts] failed", fs);
		cfp->flags |= R_FSCFG_ERR;
		return (-1);
	}
	if (mnt.params.fi_status & FS_MOUNTED) {
		cfp->flags |= R_MNTD;
	}

	if (!config.init) {
		config.mnt = mnt;
#ifdef METADATA_SERVER
	} else {
		/* FS config changed? */
		if ((CmpFsInfo(fs, &config.mnt, &mnt)) == CH_CF_FLAGS) {
			ShutdownDaemon = TRUE;		/* restart */
			cfp->flags |= R_FSCFG_ERR;
			return (0);
		}
#endif
	}

	/*
	 * get the label block
	 */
	cfp->flags |= R_LBLK;
	if ((mnt.params.fi_type == DT_META_SET) ||
	    (mnt.params.fi_type == DT_META_OBJECT_SET)) {
		for (d = 0; d < config.mnt.params.fs_count; d++) {
			if (config.mnt.part[d].pt_state == DEV_ON) {
				if (config.mnt.part[d].pt_type != DT_META) {
					break;
				}
			}
		}
		if (d == 0 || d == config.mnt.params.fs_count) {
			errno = 0;
			SysError(HERE, "FS %s: no data partition", fs);
			cfp->flags |= R_LBLK_ERR;
		}
	} else {
		d = 0;
	}
	oh = 0;
	rfd = -1;
	if (is_target_group(config.mnt.part[d].pt_type)) {
		if ((open_obj_device(config.mnt.part[d].pt_name,
		    OPEN_READ_RAWFLAGS, &oh)) < 0) {
			if (errno == EBUSY) {
				cfp->flags |= R_LBLK_BUSY;
			} else {
				SysError(HERE,
				    "FS %s: couldn't open labelblock dev '%s'",
				    fs, config.mnt.part[d].pt_name);
				cfp->flags |= R_LBLK_ERR;
			}
			err = 1;
		}
	} else {
		if (!Dsk2Rdsk(config.mnt.part[d].pt_name, name)) {
			errno = 0;
			SysError(HERE,
			    "FS %s: no raw dev name for label block on '%s'",
			    fs, config.mnt.part[d].pt_name);
			cfp->flags |= R_LBLK_ERR|R_FSCFG_ERR;
			err = 1;
		} else if ((rfd = open(name, OPEN_READ_RAWFLAGS)) < 0) {
			if (errno == EBUSY) {
				cfp->flags |= R_LBLK_BUSY;
			} else {
				SysError(HERE,
				    "FS %s: couldn't open labelblock dev '%s'",
				    fs, name);
				cfp->flags |= R_LBLK_ERR;
			}
			err = 1;
		}
	}

	/*
	 * Get label info and indirectly read in host table (in getRootInfo).
	 */
	if (err == 0) {
		cfp->flags |= getLabelInfo(rfd, oh, fs, d, &cfp->lb);
		cfp->flags |= getRootInfo(rfd, oh, fs, d, sb, NULL);
		if (oh) {
			(void) close_obj_device(config.mnt.part[d].pt_name,
			    OPEN_READ_RAWFLAGS, oh);
		} else if (rfd != -1) {
			(void) close(rfd);
		}
		if ((cfp->flags & (R_SBLK_BITS|R_LBLK_BITS|R_LBLK_VERS)) ==
		    R_LBLK) {
			/*
			 * no superblock, and no label block errors -- copy
			 * the server/server IP info into the config info
			 */
			cfp->fsInit = cfp->lb.info.lb.init;
			strncpy(cfp->serverName, cfp->lb.info.lb.server,
			    sizeof (cfp->serverName));
			strncpy(cfp->serverAddr, cfp->lb.info.lb.serveraddr,
			    sizeof (cfp->serverAddr));
		}
		if (checkPartOrdinal(sb, d)) {
			errno = 0;
			SysError(HERE, "FS %s: mcf doesn't match fs config",
			    fs);
			cfp->flags |= R_LBLK_ERR;
		}
	}

	/*
	 * Get the superblock and hosts info if we have a dev name
	 */
	if (strcmp(config.mnt.part[0].pt_name, "nodev") != 0) {
		cfp->flags |= R_SBLK;
		if (!Dsk2Rdsk(config.mnt.part[0].pt_name, name)) {
			cfp->flags |= R_SBLK_ERR|R_FSCFG_ERR;
			errno = 0;
			SysError(HERE, "FS %s: no raw dev name for "
			    "superblock on '%s'",
			    fs, config.mnt.part[0].pt_name);
		} else if ((rfd = open(name, OPEN_READ_RAWFLAGS)) < 0) {
			cfp->flags |=
			    (errno == EBUSY) ? R_SBLK_BUSY : R_SBLK_ERR;
			if (errno == EBUSY) {
				cfp->flags |= R_SBLK_BUSY;
			} else {
				cfp->flags |= R_SBLK_ERR;
				SysError(HERE, "FS %s: couldn't open "
				    "superblock dev '%s'",
				    fs, name);
			}
		} else {
			cfp->flags |= getRootInfo(rfd, 0, fs, 0, sb, cfp);
			(void) close(rfd);
			if (checkPartOrdinal(sb, 0)) {
				errno = 0;
				SysError(HERE, "FS %s: mcf doesn't match "
				    "sblk config", fs);
				cfp->flags |= R_SBLK_ERR;
			}
		}
		if ((cfp->flags & R_SBLK_BITS) == R_SBLK) {
			cfp->fsInit = sb->info.sb.init;
			cfp->hostsOffset = sb->info.sb.hosts;
			if (GetSharedHostInfo(&cfp->ht->info.ht,
			    cfp->ht->info.ht.server,
			    cfp->serverName, cfp->serverAddr) < 0) {
				cfp->flags |= R_SBLK_ERR;
				return (-1);
			}
		}
	}

	/*
	 * get the local hosts file's mod time
	 */
	cfp->flags |= R_LHOSTS;
	snprintf(name, sizeof (name), "%s/hosts.%s.local",
	    SAM_CONFIG_PATH, fs);
	if (stat(name, &sbuf) < 0) {
		if (errno != ENOENT) {
			cfp->flags |= R_LHOSTS_ERR;
		} else {
			cfp->flags |= R_LHOSTS_NONE;
		}
	} else {
		cfp->flags |= R_LHOSTS;
#ifdef	linux
		cfp->lHostModTime.tv_sec = sbuf.st_mtime;
#else
		cfp->lHostModTime = sbuf.st_mtim;
#endif	/* linux */
	}
	return (0);
}


/*
 * Read the relevant fs configuration info
 * (superblock, hosts file), given a file
 * descriptor to the raw root slice device.
 */
static int
getRootInfo(
	int rfd,
	uint64_t oh,
	char *fs,
	int ord,
	struct sam_sblk *sb,
	struct cfdata *cfp)
{
	int swap_bytes = 0;
	offset_t offset64;
	static struct sam_sblk *sba = NULL;
	static struct sam_host_table_blk *hba = NULL;
	static int htsize = 0;
	struct sam_host_table_blk *hb = NULL;
	int hfd = 0;
	int error;
	int close_hfd;
	ushort_t hosts_ord;

	if (cfp != NULL) {
		hb = cfp->ht;
	}

	if (sba == NULL) {
		MEMALIGN(sba, LX_ALIGN, sizeof (*sba));
		if (sba == NULL) {
			return (R_SBLK_ERR);
		}
	}
	if (oh) {
		if ((read_object(fs, oh, ord, SAM_OBJ_SBLK_INO,
		    (char *)sba, 0, sizeof (*sba)))) {
			return (R_SBLK_ERR);
		}
	} else {
		offset64 = SUPERBLK*SAM_DEV_BSIZE;
		if (llseek(rfd, offset64, SEEK_SET) == -1) {
			return (R_SBLK_ERR);
		}
		if (read(rfd, (char *)sba, sizeof (*sba)) != sizeof (*sba)) {
			return (R_SBLK_ERR);
		}
	}
	memcpy(sb, sba, sizeof (*sb));
	if (strncmp("SBLK", sb->info.sb.name, sizeof (sb->info.sb.name))
	    != 0) {
		return (R_SBLK_NOFS);
	}
	if (strncmp(fs, sb->info.sb.fs_name,
	    sizeof (sb->info.sb.fs_name)) != 0) {
		return (R_SBLK_NOFS);
	}
	if (sb->info.sb.magic == SAM_MAGIC_V2_RE ||
	    sb->info.sb.magic == SAM_MAGIC_V2A_RE) {
		if (byte_swap_sb(sb, sizeof (*sb))) {
			return (R_SBLK_ERR);
		}
		swap_bytes = 1;
	}
	if (sb->info.sb.magic != SAM_MAGIC_V2 &&
	    sb->info.sb.magic != SAM_MAGIC_V2A) {
		return (R_SBLK_NOFS);
	}

	if (hb == NULL) {
		return (0);
	}

	if (sb->info.sb.hosts == 0) {
		errno = 0;
		SysError(HERE, "FS %s: filesystem not shared -- "
		    "no hosts file", fs);
		return (R_SBLK_ERR);
	}

	hosts_ord = sb->info.sb.hosts_ord;

	if (hosts_ord != 0) {
		/*
		 * The hosts table is on
		 * a different device ordinal.
		 * Get a file descriptor for it.
		 */
		error = OpenFsDevOrd(fs, hosts_ord, &hfd, O_RDONLY);

		if (error != 0) {
			SysError(HERE, "FS %s: Unable to open hosts file dev\n",
			    fs);
			return (R_SBLK_ERR);
		}
		close_hfd = 1;

	} else {
		/*
		 * The hosts table is on device ordinal 0
		 * so we can use the supplied file descriptor.
		 */
		hfd = rfd;
		close_hfd = 0;
	}

again:
	/*
	 * Try reading the old small table first.
	 */
	if (htsize == 0) {
		htsize = sizeof (*hba);
	}
	if (hba == NULL) {
		/*
		 * Always allocate enough space for the large table.
		 */
		MEMALIGN(hba, LX_ALIGN, SAM_LARGE_HOSTS_TABLE_SIZE);
		if (hba == NULL) {
			if (close_hfd) {
				close(hfd);
			}
			return (R_SBLK_ERR);
		}
	}
	offset64 = sb->info.sb.hosts * SAM_DEV_BSIZE;
	if (llseek(hfd, offset64, SEEK_SET) == -1) {
		if (close_hfd) {
			close(hfd);
		}
		return (R_SBLK_ERR);
	}
	if (read(hfd, (char *)hba, htsize) != htsize) {
		if (close_hfd) {
			close(hfd);
		}
		return (R_SBLK_ERR);
	}
	if (hba->info.ht.length > sizeof (*hba) && htsize == sizeof (*hba)) {
		htsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		goto again;
	}
	memcpy(hb, hba, htsize);
	cfp->htsize = htsize;
	cfp->hosts_ord = sb->info.sb.hosts_ord;

	if (swap_bytes) {
		/*
		 * Byte swap the host table block from the disk
		 */
		if (byte_swap_hb(hb)) {
			if (close_hfd) {
				close(hfd);
			}
			return (R_SBLK_ERR);
		}
	}
	if (hb->info.ht.cookie == SAM_HOSTS_COOKIE) {
		switch (hb->info.ht.version) {
		case SAM_HOSTS_VERSION4:
			break;
		default:
			errno = 0;
			SysError(HERE, "FS %s: bad hosts file version", fs);
			if (close_hfd) {
				close(hfd);
			}
			return (R_SBLK_ERR);
		}
	} else if (hb->info.ht.cookie == 0) {
		switch (hb->info.ht.version) {
		case SAM_HOSTS_VERSION1:
		case SAM_HOSTS_VERSION2:
		case SAM_HOSTS_VERSION3:
			Trace(TR_MISC, "FS %s: hosts file version obsolete",
			    fs);
			break;
		case SAM_HOSTS_VERSION4:
			break;
		default:
			errno = 0;
			SysError(HERE, "FS %s: bad hosts file version", fs);
			if (close_hfd) {
				close(hfd);
			}
			return (R_SBLK_ERR);
		}
	} else {
		errno = 0;
		SysError(HERE, "FS %s: bad hosts file version", fs);
		if (close_hfd) {
			close(hfd);
		}
		return (R_SBLK_ERR);
	}
	if (close_hfd) {
		close(hfd);
	}
	return (swap_bytes ? R_FOREIGN : 0);
}


/*
 * Read up the label block, given a
 * file descriptor to the raw device.
 */
static int
getLabelInfo(
	int rfd,
	uint64_t oh,
	char *fs,
	int ord,
	struct sam_label_blk *lb)
{
	int n, swap_bytes = 0;
	offset_t offset64;
	static struct sam_label_blk *lba = NULL;
	static struct sam_sblk *sba = NULL;

	/*
	 * Read up superblock.  This to obtain the native FS byte-order,
	 * since we can't trust the label block at start-up.
	 */
	if (sba == NULL) {
		MEMALIGN(sba, LX_ALIGN, sizeof (*sba));
		if (sba == NULL) {
			return (R_LBLK_ERR);
		}
	}
	if (oh) {
		if ((read_object(fs, oh, ord, SAM_OBJ_SBLK_INO,
		    (char *)sba, 0, sizeof (*sba)))) {
			return (R_LBLK_ERR);
		}
	} else {
		offset64 = SUPERBLK * SAM_DEV_BSIZE;
		if (llseek(rfd, offset64, SEEK_SET) == -1) {
			return (R_LBLK_ERR);
		}
		if (read(rfd, (char *)sba, sizeof (*sba)) != sizeof (*sba)) {
			return (R_LBLK_ERR);
		}
	}
	if (strncmp("SBLK", sba->info.sb.name,
	    sizeof (sba->info.sb.name)) != 0) {
		return (R_LBLK_NONE);
	}
	if (strncmp(fs, sba->info.sb.fs_name,
	    sizeof (sba->info.sb.fs_name)) != 0) {
		return (R_LBLK_NONE);
	}
	if (sba->info.sb.magic == SAM_MAGIC_V2_RE ||
	    sba->info.sb.magic == SAM_MAGIC_V2A_RE) {
		if (byte_swap_sb(sba, sizeof (*sba))) {
			return (R_LBLK_NONE);
		}
		swap_bytes = 1;
	}
	if (sba->info.sb.magic != SAM_MAGIC_V2 &&
	    sba->info.sb.magic != SAM_MAGIC_V2A) {
		return (R_LBLK_NONE);
	}

	if (lba == NULL) {
		MEMALIGN(lba, LX_ALIGN, sizeof (*lba));
		if (lba == NULL) {
			errno = 0;
			SysError(HERE,
			    "FS %s: label memory allocation failed",
			    Host.fs.fi_name);
			return (R_LBLK_ERR);
		}
	}
	if (oh) {
		if ((read_object(fs, oh, ord, SAM_OBJ_LBLK_INO,
		    (char *)lba, 0, sizeof (*lba)))) {
			return (R_LBLK_ERR);
		}
	} else {
		offset64 = LABELBLK * SAM_DEV_BSIZE;
		if (llseek(rfd, offset64, SEEK_SET) < 0) {
			SysError(HERE, "FS %s: label seek failed",
			    Host.fs.fi_name);
			return (R_LBLK_ERR);
		}
		if ((n = read(rfd, (char *)lba, sizeof (*lba))) < 0) {
			SysError(HERE, "FS %s: label read failed",
			    Host.fs.fi_name);
			return (R_LBLK_ERR);
		} else if (n != sizeof (*lba)) {
			errno = 0;
			SysError(HERE, "FS %s: short label read",
			    Host.fs.fi_name);
			return (R_LBLK_ERR);
		}
	}
	if (strncmp(lba->info.lb.name, "LBLK", sizeof (lba->info.lb.name))) {
		errno = 0;
		SysError(HERE, "FS %s: label not an LBLK", Host.fs.fi_name);
		return (R_LBLK_NONE);
	}
	memcpy(lb, lba, sizeof (*lb));
	if (lb->info.lb.magic != SAM_LABEL_MAGIC) {
		/*
		 * Byte swap the label block from the disk
		 */
		if (swap_bytes && byte_swap_lb(lb)) {
			return (R_LBLK_ERR);
		}

		if (lb->info.lb.magic != SAM_LABEL_MAGIC) {
			/*
			 * Still doesn't match
			 */
			errno = 0;
			SysError(HERE, "FS %s: label bad magic",
			    Host.fs.fi_name);
			return (R_LBLK_NONE);
		}
	}
	if (lb->info.lb.version != SAM_LABEL_VERSION) {
		errno = 0;
		SysError(HERE, "FS %s: label version mismatch (e=%d/a=%d) -- "
		    "shared server/clients must run compatible SAM/QFS "
		    "versions",
		    Host.fs.fi_name, SAM_LABEL_VERSION, lb->info.lb.version);
		return (R_LBLK_VERS);
	}
	return (swap_bytes ? R_FOREIGN : 0);
}


/*
 * Verify that the partition is either the root partition for
 * the filesystem if part == 0 (according to the superblock),
 * or that it has the lowest numbered ordinal of the data
 * partitions if part != 0 (again, according to the superblock).
 * This avoids problems with having mcf descriptions not match
 * the filesystem.
 *
 * I.e., we need to locate the superblock/hosts table and the
 * label block reliably.  The hosts table is on ordinal 0 of
 * the filesystem, and the label block is on the data partition
 * with the lowest ordinal.  The information we have about the
 * filesystem prior to actually reading the partitions and/or
 * mounting the filesystem comes from the mcf.  The mcf's entries
 * may be in a scrambled order compared to the filesystem's
 * ordinals.  So:  we need to ensure that the first partition
 * listed in the mcf file is in fact the root slice of the FS.
 * If it's not, we throw up our hands, and eventually exit with
 * a message to this effect.  Similarly, if the first data partition
 * in the mcf does not have the lowest numbered ordinal of the
 * filesystem's data partitions, we throw up our hands.
 *
 * This avoids any potential problem where different hosts
 * could expect different partitions to hold the superblock,
 * hosts table, or label block.
 */
static int
checkPartOrdinal(
	struct sam_sblk *sb,
	int part)
{
	int i;

	if (part == 0) {
		return (sb->info.sb.ord != 0);
	}
	for (i = 0; i < sb->info.sb.fs_count; i++) {
		if (sb->eq[i].fs.type != DT_META)
			break;
	}
	if (i == 0 || i == sb->info.sb.fs_count) {
		return (1);
	}
	return (i != sb->info.sb.ord);
}


/*
 * Convert a /dev/dsk/xyzzy pathname to /dev/rdsk/xyzzy
 *
 * Return 1 if /dsk/ was found and replaced (success); 0 otherwise.
 */
int
Dsk2Rdsk(
	char *dsk,
	char *rdsk)
{
#ifdef	linux
	int k, j, n;
	/*
	 * Linux block devices generally
	 * don't have associated raw devices, so just
	 * return the block device.
	 */

	n = strlen(dsk);
	strncpy(rdsk, dsk, n);
	rdsk[n] = 0;

	return (1);

#else	/* linux */
	char *rdevname;

	if ((rdevname = getfullrawname(dsk)) == NULL) {
		SysError(HERE, "Can't malloc getfullrawname");
		return (0);
	}
	if (strcmp(rdevname, "\0") == 0) {
		return (0);
	}
	strcpy(rdsk, rdevname);
	free(rdevname);
	return (1);

#endif	/* linux */
}
