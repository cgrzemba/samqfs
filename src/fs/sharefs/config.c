/*
 *	config.c - config support functions for the filesystem.
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

#pragma ident "$Revision: 1.77 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

/* POSIX headers. */
#include <sys/vfs.h>

/* Solaris headers. */
	/* none */

/* SAM-FS headers. */
#include "sam/mount.h"
#include "sam/exit.h"
#include "sam/quota.h"
#include "sam/syscall.h"
#include "sam/shareops.h"
#include "sam/lib.h"

#include "sblk.h"
#include "share.h"
#include "samhost.h"

/* Local headers. */
#include "sharefs.h"
#include "config.h"

extern struct shfs_config config;
extern int OpenFsDevOrd(char *fs, ushort_t ord, int *devfd, int oflag);

/*
 * Local private functions
 */


/*
 *
 * The basic failover protocol is implicit in a lot of
 * this code.
 *
 * Host A server, host B becoming server, host X initiating
 *		X rewrites host file, sets pending server ID = new server
 *		A notices config change (pending server != server)
 *		A calls SetClient to flush internal FS structures
 *		A notifies attached hosts of FREEZE
 *		A SetClient returns when server FROZEN
 *		A rewrites host file (sets server = pending server)
 *		A sam-sharefsd exits
 *		-- sam-sharefsd FS copies exit on all hosts
 *		-- sam-sharefsd FS copies restart on all hosts
 *		B reads hosts file, sees B is server
 *		B rewrites label block
 *		B initiates listener
 *		B calls SetServer (releases FROZEN)
 *		B's listener threads connected to OS
 *		-- all other hosts call SetClient with new server
 *		   (releasing FROZEN)
 */


#ifdef METADATA_SERVER
/*
 * This routine is called from server.c to get a copy
 * of the hosts table, which it uses to determine and
 * verify connecting client names and addresses, resp.
 */
int
GetHostTab(struct sam_host_table_blk *ht)
{
	bcopy(config.cp[config.curTab].ht, ht, config.cp[config.curTab].htsize);
	return (0);
}


/*
 * Write the label block, given data and
 * a file descriptor to the raw device.
 */
static int
putLabelInfo(
	int rfd,
	struct sam_label_blk *lb)
{
	int n;
	offset_t offset64;

	offset64 = LABELBLK * SAM_DEV_BSIZE;
	if (llseek(rfd, offset64, SEEK_SET) < 0) {
		return (-1);
	}
	if ((n = write(rfd, (char *)lb, sizeof (*lb))) < 0) {
		return (-1);
	} else if (n != sizeof (*lb)) {
		return (-1);
	}
	return (0);
}


/*
 * Set Server in label block.
 */
static int
putLabel(
	char *fs,
	struct cfdata *cfp)
{
	int dpart, fd;
	sam_label_blk_t lblk;
	struct sam_label *lb = &lblk.info.lb;
	struct sam_fs_part *part;
	char rawpath[MAXPATHLEN];

	/*
	 * Get the information for first data partition
	 */
	part = config.mnt.part;
	if ((config.mnt.params.fi_type == DT_META_SET) ||
	    (config.mnt.params.fi_type == DT_META_OBJECT_SET)) {
		for (dpart = 0; dpart < config.mnt.params.fs_count; dpart++) {
			if (part[dpart].pt_type != DT_META) {
				break;
			}
		}
		if (dpart == 0 || dpart == config.mnt.params.fs_count) {
			SysError(HERE, "FS %s: Filesystem has no data "
			    "partition", fs);
			return (-1);
		}
	} else {
		dpart = 0;
	}


	bzero((char *)&lblk, sizeof (lblk));
	strncpy(lb->name, "LBLK", sizeof (lb->name));
	lb->magic = SAM_LABEL_MAGIC;
	lb->init = cfp->fsInit;
	lb->update = time(NULL);
	lb->version = SAM_LABEL_VERSION;
	lb->serverord = cfp->ht->info.ht.server;
	lb->gen = cfp->ht->info.ht.gen;
	cfp->lb.info.lb.gen = lb->gen;	/* keep config label gen up to date */
	strncpy(lb->server, cfp->serverName, sizeof (lb->server));
	strncpy(lb->serveraddr, cfp->serverAddr, sizeof (lb->serveraddr));

	if (is_osd_group(part[dpart].pt_type)) {
		sam_osd_handle_t oh;

		if ((open_obj_device(part[dpart].pt_name, OPEN_RDWR_RAWFLAGS,
		    &oh)) < 0) {
			SysError(HERE, "FS %s: syscall[open] failed for %s",
			    fs, part[dpart].pt_name);
			return (-1);
		}
		if ((write_object(fs, oh, dpart, SAM_OBJ_LBLK_INO,
		    (char *)&lblk, 0, L_LABEL))) {
			SysError(HERE, "FS %s: couldn't write label on dev %s",
			    fs, part[dpart].pt_name);
			(void) close_obj_device(part[dpart].pt_name, O_RDWR,
			    oh);
			return (-1);
		}
		if ((close_obj_device(part[dpart].pt_name, O_RDWR, oh)) < 0) {
			SysError(HERE, "FS %s: close of label dev %s failed",
			    fs, part[dpart].pt_name);
			return (-1);
		}
	} else {
		/*
		 * Generate raw device name
		 */
		if (!Dsk2Rdsk(part[dpart].pt_name, rawpath)) {
			SysError(HERE,
			    "FS %s: Can't find raw device name for '%s'",
			    fs, part[dpart].pt_name);
			return (-1);
		}

		/*
		 * Open data slice
		 */
		if ((fd = open(rawpath, OPEN_RDWR_RAWFLAGS)) < 0) {
			SysError(HERE, "FS %s: syscall[open] failed for %s",
			    fs, rawpath);
			return (-1);
		}

		if (putLabelInfo(fd, &lblk) < 0) {
			SysError(HERE, "FS %s: couldn't write label on dev %s",
			    fs, rawpath);
			(void) close(fd);
			return (-1);
		}
		if (close(fd) < 0) {
			SysError(HERE, "FS %s: close of label dev %s failed",
			    fs, rawpath);
			return (-1);
		}
	}
	Trace(TR_MISC, "FS %s: Wrote label on %s", fs, rawpath);
	return (0);
}


/*
 * ----- putHosts
 *
 * Write out the shared hosts file.
 * Called when we're the metadata server, after a server change
 * has been requested, and we've gotten the kernel to push all
 * the metadata out to disk.  We set the current server to
 * the pending server, write it out, and exit.
 */
static int
putHosts(
	char *fs,
	struct cfdata *cfp)
{
	int rfd;
	int error;
	struct sam_sblk sb;
	offset_t offset64;
	struct sam_host_table_blk *hb = cfp->ht;
	int htsize = cfp->htsize;
	ushort_t hosts_ord = cfp->hosts_ord;

	/*
	 * Get the superblock and hosts info if we have a dev name
	 */
	if (strcmp(config.mnt.part[hosts_ord].pt_name, "nodev") == 0) {
		errno = 0;
		SysError(HERE, "FS %s: server has 'nodev' device", fs);
		return (-1);
	}

	/*
	 * Get a RDWR file descriptor for the device
	 * that holds the hosts table.
	 */
	error = OpenFsDevOrd(fs, hosts_ord, &rfd, O_RDWR);

	if (error != 0) {
		SysError(HERE, "FS %s: Open failed on superblock", fs);
		return (-1);
	}

	offset64 = SUPERBLK*SAM_DEV_BSIZE;
	if (llseek(rfd, offset64, SEEK_SET) == -1) {
		SysError(HERE, "FS %s: llseek failed on superblock", fs);
		return (-1);
	}

	if (read(rfd, (char *)&sb, sizeof (sb)) != sizeof (sb)) {
		SysError(HERE, "FS %s: read failed on superblock", fs);
		return (-1);
	}

	/*
	 * Make sure this is the correct device.
	 */
	if (strncmp("SBLK", sb.info.sb.name, sizeof (sb.info.sb.name)) != 0) {
		errno = 0;
		SysError(HERE, "FS %s: superblock read returned bad fs "
		    "superblock", fs);
		return (-1);
	}

	if (strncmp(fs, sb.info.sb.fs_name,
	    sizeof (sb.info.sb.fs_name)) != 0) {
		errno = 0;
		SysError(HERE, "FS %s: superblock read returned wrong "
		    "fs superblock",
		    fs);
		return (-1);
	}

	if (sb.info.sb.magic != SAM_MAGIC_V2 &&
	    sb.info.sb.magic != SAM_MAGIC_V2A) {
		errno = 0;
		SysError(HERE, "FS %s: superblock read returned bad "
		    "superblock type",
		    fs);
		return (-1);
	}
	if (sb.info.sb.hosts == 0) {
		errno = 0;
		SysError(HERE, "FS %s: filesystem not shared -- "
		    "no hosts file", fs);
		return (-1);
	}

	if (sb.info.sb.hosts_ord != hosts_ord ||
	    sb.info.sb.hosts_ord != sb.info.sb.ord) {
		errno = 0;
		SysError(HERE, "FS %s: Bad hosts table device ordinal --", fs);
		return (-1);
	}

	offset64 = sb.info.sb.hosts*SAM_DEV_BSIZE;
	if (llseek(rfd, offset64, SEEK_SET) == -1) {
		SysError(HERE, "FS %s: seek failed on hosts file write", fs);
		close(rfd);
		return (-1);
	}
	if (write(rfd, (char *)hb, htsize) != htsize) {
		SysError(HERE, "FS %s: write failed on hosts file write", fs);
		close(rfd);
		return (-1);
	}
	close(rfd);
	Trace(TR_MISC, "FS %s: Wrote shared hosts file on %s",
	    fs, config.mnt.part[hosts_ord].pt_name);
	return (0);
}
#endif	/* METADATA_SERVER */


/*
 * swapConfig
 *
 * Make the 'free' configuration table the current table.
 */
static void
swapConfig(char *fs)
{
	config.curTab = config.freeTab;
	config.freeTab = 1 - config.curTab;
	Trace(TR_MISC, "FS %s: configs swapped (%d current)",
	    fs, config.curTab);
}


/*
 * We're called here to update the internal configuration.
 * Most times this doesn't mean much.  The critical time is
 * when the local host is the server and the declared server
 * has changed, because someone changed it with samsharefs
 * and overwrote it.  In this case, we need to detect this
 * promptly and initiate surrender of our metadata server
 * activities so that the up-and-coming new server and the
 * local host don't step all over each other, smotching the
 * FS.
 *
 * Return indicates action to be taken:
 *
 * Server:
 *  0 - no action
 *  1 - exit(EXIT_FAILURE)     (e.g., normal failover)
 * -1 - exit(EXIT_NORESTART)   (error condition/involuntary failover)
 *
 * Client:
 *  0 - no action
 *  1 - no action              (normal failover; wait for server to disconnect)
 * -1 - exit(EXIT_FAILURE)     (involuntary failover)
 *
 * Generally, nearly anything is OK if the FS isn't mounted.
 * For mounted FSes, seeing a scrozzled superblock or hosts
 * file (or label block if a client-only host), is a notable
 * event, and should be treated specially.
 */
static int
updateConfig(char *fs)
{
	struct sam_sblk sb;
	struct cfdata *cfp = &config.cp[config.curTab];
	struct cfdata *nfp = &config.cp[config.freeTab];
	int diff = 0;
	int r = 0;

	if (GetConfig(fs, &sb) < 0) {
		r = 1;
		if (cfp->flags & R_MNTD) {
			r = -1;
			goto out;
		}
	}

	errno = 0;
	if (nfp->flags & R_ERRBITS) {
		r = 1;
		SysError(HERE, "FS %s: config error; bits=%x", fs, nfp->flags);
		if (nfp->flags & R_MNTD) {
			r = -1;
			goto out;
		}
	}
	if (nfp->flags & R_SBLK_NOFS) {
		r = 1;
		SysError(HERE, "FS %s: root slice has no FS; mkfs'ed?", fs);
		if (nfp->flags & R_MNTD) {
			r = -1;
			goto out;
		}
	}
	if (nfp->flags & R_BUSYBITS) {
		SysError(HERE, "FS %s: mounted root slice busy", fs);
		r = 1;
		if (nfp->flags & R_MNTD) {
			r = -1;
			goto out;
		}
	}
	if (nfp->flags & R_LBLK_VERS) {
		SysError(HERE, "FS %s: bad label block version", fs);
		r = -1;
		goto out;
	}

	if ((diff = CmpConfig(fs, &sb)) == 0) {
		goto out;
	}

	if (diff & CH_CF_FLAGS) {
		int oflags, nflags;

		oflags = cfp->flags;
		nflags = nfp->flags;
		Trace(TR_MISC, "FS %s: flags changed; old/new = %x/%x",
		    fs, oflags, nflags);
		errno = 0;
		if ((oflags ^ nflags) & nflags &
		    (R_SBLK | R_LBLK | R_LHOSTS)) {
			SysError(HERE, "FS %s: access flags diminished; "
			    "old/new = %x/%x",
			    fs, oflags, nflags);
		} else {
			if ((oflags ^ nflags) & oflags &
			    (R_ERRBITS | R_BUSYBITS)) {
				SysError(HERE, "FS %s: error flags enlarged; "
				    "old/new = %x/%x",
				    fs, oflags, nflags);
			}
		}
		r = 1;
	}

	if (diff & CH_CF_MOUNT) {
		Trace(TR_MISC, "FS %s: %smount; flags=%x",
		    fs, (nfp->flags&R_MNTD) ? "" : "un", nfp->flags);
	}

	if ((diff & ~(CH_CF_FLAGS | CH_CF_MOUNT)) == 0) {
		goto out;
	}

	if (diff & CH_SB_SBLK) {
		r = 1;
		if (nfp->flags & R_MNTD) {
			SysError(HERE, "FS %s: Mounted superblock changed; "
			    "flags=%x",
			    fs, nfp->flags);
		} else {
			Trace(TR_MISC, "FS %s: Superblock changed; flags=%x",
			    fs, nfp->flags);
		}
		if (nfp->flags & R_MNTD) {
			if (diff &
			    (CH_SB_NOFS | CH_SB_NOTSH | CH_SB_NEWFS | \
			    CH_SB_FSNAME)) {
				r = -1;
				goto out;
			}
		}
	}

	if (diff & CH_HT_HOSTS) {
		r = 1;
		Trace(TR_MISC, "FS %s: Hosts file changed; flags=%x",
		    fs, nfp->flags);
		/*
		 * Some subtlety required here.  There are a few possibilities
		 * if we see that the CH_HOSTS is set.  How we respond depends
		 * on this host's current role.
		 *
		 * (1) If this host is the server, we return notification
		 *	   of the change.
		 * (2) If it's not the server, and a pending server change
		 *	   is indicated, we don't return notification.  The
		 *	   server will eventually act on it, send all hosts a
		 *	   failover indication, and eventually set the current
		 *	   host to the pending host.  (This change will be
		 *	   picked up here later and returned.)
		 */
		if (diff & (CH_HT_BAD | CH_HT_RESET | CH_HT_INVOL)) {
			r = -1;
			goto out;
		}
	}

	if (diff & CH_LB_LABEL) {
		Trace(TR_MISC, "FS %s: Label changed; flags=%x",
		    fs, nfp->flags);
		if ((nfp->flags & R_SBLK) == 0) {
			r = 1;
		}
	}

	if (diff & CH_LC_HOSTS) {
		r = 1;
		Trace(TR_MISC, "FS %s: hosts.%s.local modtime changed; "
		    "flags=%x", fs, fs, nfp->flags);
	}

out:
	if (diff) {
		Trace(TR_MISC, "FS %s: Config changed: "
		    "diff=%x, flags=%x, oflags=%x",
		    fs, diff, nfp->flags, cfp->flags);
	}
	return (r);
}


/*
 * ----- ValidateFs
 *
 * Initialize a bunch of stuff.  Possibly subtle line
 * between things to wait for and things to bomb out on.
 *
 * If the filesystem isn't in memory, we exit, 'cuz
 * we've been started for something that no longer
 * exists.  If the in-memory FS isn't shared, ditto.
 * If the on-disk FS doesn't match, we wait, on the
 * assumption that someone will be along to mkfs it
 * eventually.  We issue one message for posterity
 * though, so that there's some evidence if this
 * isn't what the sysadmin expects.
 *
 * Once the FS is initialized, we are happy if there
 * are no errors and it wasn't busy (we could read it).
 * In that case, we copy the fs params into the Hosts
 * structure, and allow things to proceed.
 *
 * Along the way we initialize the config table.
 */
boolean_t
ValidateFs(
	char *fs,
	struct sam_fs_info *fi)
{
	struct sam_sblk sb;
	struct cfdata *cfp;
	int waitstate = 0;

	if (GetConfig(fs, &sb) < 0) {	/* always reads into config.freeTab */
		Trace(TR_MISC, "FS %s: GetConfig failed, exiting", fs);
		exit(EXIT_NORESTART);
	}
	swapConfig(fs);
	config.init = 1;

	cfp = &config.cp[config.curTab];
	if (cfp->flags & R_FSCFG_ERR) {
		exit(EXIT_NORESTART);
	}
	config.fsId = cfp->fsInit;

	Trace(TR_MISC, "FS %s: Filesystem is%s mounted",
	    fs, (config.mnt.params.fi_status & FS_MOUNTED) ? "" : "n't");

	while ((cfp->flags & (R_ERRBITS | R_BUSYBITS | R_SBLK_NOFS)) ||
	    cfp->ht->info.ht.server == HOSTS_NOSRV) {
		if (!waitstate) {
			if (cfp->flags & R_ERRBITS) {
				Trace(TR_MISC, "FS %s: Error in filesystem; "
				    "waiting", fs);
			} else if (cfp->flags & R_BUSYBITS) {
				Trace(TR_MISC, "FS %s: Filesystem busy; "
				    "waiting", fs);
			} else if (cfp->ht->info.ht.server == HOSTS_NOSRV) {
				Trace(TR_MISC, "FS %s: Server = NONE; "
				    "waiting", fs);
			} else {
				Trace(TR_MISC, "FS %s: No filesystem; "
				    "waiting", fs);
			}
			waitstate = 1;
		}
		/*
		 * Release our hold on the FS devs and wait a bit.
		 */
		CloseDevs();
		sleep(10);
		OpenDevs(fs);

		if (GetConfig(fs, &sb) < 0) {
			return (FALSE);
		}
		swapConfig(fs);
		cfp = &config.cp[config.curTab];
		if (ShutdownDaemon) {
			exit(EXIT_FAILURE);
		}
		if (cfp->flags & R_FSCFG_ERR) {
			exit(EXIT_NORESTART);
		}
	}
	if (waitstate) {
		Trace(TR_MISC, "FS %s: Now available", fs);
	}

	/*
	 * cfp points to config.curTab, which is now the current config
	 */
	bcopy(&config.mnt.params, fi, sizeof (*fi));
	if ((cfp->flags & R_SBLK) == 0) {
		return (TRUE);
	}

#ifdef METADATA_SERVER
	Host.maxord = cfp->ht->info.ht.count;
	if (strncasecmp(Host.hname, cfp->serverName,
	    sizeof (cfp->serverName)) == 0) {
		/*
		 * Tag.  We're it (the server).
		 */
		if (cfp->flags & R_FOREIGN) {
			SysError(HERE, "FS %s: This host cannot be server "
			    "(foreign byte order)", fs);
			exit(EXIT_NORESTART);
		}
		if (cfp->ht->info.ht.version != SAM_HOSTS_VERSION4) {
			/*
			 * Convert the hosts table from V2 or V3 to V4.
			 *
			 * Version 3 host table has a pendsrv field which is
			 * initialized to the current metadata server to avoid
			 * triggering an immediate failover.
			 *
			 * Version 4 host table has a cookie field that
			 * is initialized.
			 */
			switch (cfp->ht->info.ht.version) {
				case SAM_HOSTS_VERSION2:	/* V2 -> V3 */
					Trace(TR_MISC, "FS %s: Updating V2 "
					    "hosts file to V3", fs);
					cfp->ht->info.ht.pendsrv =
					    cfp->ht->info.ht.server;
					cfp->ht->info.ht.version =
					    SAM_HOSTS_VERSION3;
					/* FALLTHRU */
				case SAM_HOSTS_VERSION3:	/* V3 -> V4 */
					Trace(TR_MISC, "FS %s: Updating V3 "
					    "hosts file to V4", fs);
					cfp->ht->info.ht.prevsrv =
					    cfp->ht->info.ht.server;
					cfp->ht->info.ht.version =
					    SAM_HOSTS_VERSION4;
					cfp->ht->info.ht.cookie =
					    SAM_HOSTS_COOKIE;
					break;
			}

			if (++cfp->ht->info.ht.gen == 0) {
				++cfp->ht->info.ht.gen;
			}

			putHosts(fs, cfp);
			return (FALSE);
		}

		if (cfp->ht->info.ht.pendsrv != cfp->ht->info.ht.server) {
			upath_t newserver;
			struct cfdata *nfp;

			/*
			 * Pending voluntary failover.
			 */

			if (cfp->ht->info.ht.pendsrv != HOSTS_NOSRV &&
			    !SamGetSharedHostName(&cfp->ht->info.ht,
			    cfp->ht->info.ht.pendsrv,
			    newserver)) {
				errno = 0;
				SysError(HERE, "FS %s: bad pending server "
				    "ordinal (%d)",
				    fs, cfp->ht->info.ht.pendsrv);
				exit(EXIT_NORESTART);
			}
			/*
			 * We're the server, but a request to change
			 * servers has been made.  Surrender our duties.
			 */
			if (SysFailover(fs, newserver) != 0) {
				/*
				 * Not mounted remotely?  The FS probably isn't
				 * mounted locally; in any event, we were
				 * committed to the failover long ago.
				 */
				SysError(HERE,
				    "FS %s: SysFailover(%s,%s) failed",
				    fs, fs, newserver);
			}
			/*
			 * We were the server.  The kernel has pushed all
			 * its metadata out to disk, and we're back.  The world
			 * may have changed out from under us -- if so, we
			 * need to exit and try again.  If the hosts file
			 * hasn't changed, then things are OK and we update
			 * the hosts file to indicate that the pending server
			 * can now take over.  After that, we exit, which shuts
			 * down all the connections, and everything starts
			 * anew.
			 *
			 * Alternatively, the kernel has reported an error
			 * and we are back.  If the kernel has told us that
			 * the server-to-be is not available, we need to back
			 * out the server change request and exit/restart.
			 */
			nfp = &config.cp[config.freeTab];
			if (GetConfig(fs, &sb) < 0 ||
			    (nfp->flags&R_SBLK_BITS) != R_SBLK) {
				return (FALSE);
			}
			if (bcmp(&cfp->ht->info.ht, &nfp->ht->info.ht,
			    sizeof (cfp->ht->info.ht)) != 0) {
				return (FALSE);
			}
			nfp->ht->info.ht.server = nfp->ht->info.ht.pendsrv;
			if (++nfp->ht->info.ht.gen == 0) {
				++nfp->ht->info.ht.gen;
			}
			/*
			 * Update the serverName/Addr strings in the new
			 * configuration block to match the new server.
			 * (putHosts() doesn't need this information, but
			 * putLabel() does.)
			 */
			if (nfp->ht->info.ht.server == HOSTS_NOSRV) {
				strcpy(nfp->serverName, "-NONE-");
				strcpy(nfp->serverAddr, "-NOADDR-");
			} else if (GetSharedHostInfo(&nfp->ht->info.ht,
			    nfp->ht->info.ht.server,
			    nfp->serverName, nfp->serverAddr) < 0) {
				SysError(HERE, "FS %s: GetSharedHostInfo(%d)",
				    fs, nfp->ht->info.ht.server);
				strcpy(nfp->serverName, "-NONE-");
				strcpy(nfp->serverAddr, "-NOADDR-");
			}
			putHosts(fs, nfp);
			putLabel(fs, nfp);
			return (FALSE);
		}

		if (cfp->ht->info.ht.prevsrv == HOSTS_NOSRV) {
			upath_t newserver;
			struct cfdata *nfp;

			/*
			 * Pending involuntary failover
			 */
			if (cfp->ht->info.ht.server != HOSTS_NOSRV &&
			    !SamGetSharedHostName(&cfp->ht->info.ht,
			    cfp->ht->info.ht.server, newserver)) {
				errno = 0;
				SysError(HERE, "FS %s: bad pending server "
				    "ordinal (%d)",
				    fs, cfp->ht->info.ht.pendsrv);
				exit(EXIT_NORESTART);
			}
			/*
			 * We are the new server.  Proceed like ever, but
			 * notify the kernel that this is involuntary, and
			 * rewrite the hosts file setting prevsrv == server.
			 *
			 * Note that substantial time may have elapsed since
			 * we entered this routine.  We must re-obtain the
			 * config info, and ensure that the world hasn't
			 * changed out from under us.
			 */
			nfp = &config.cp[config.freeTab];
			if (GetConfig(fs, &sb) < 0 ||
			    (nfp->flags&R_SBLK_BITS) != R_SBLK) {
				return (FALSE);
			}
			if (bcmp(&cfp->ht->info.ht, &nfp->ht->info.ht,
			    sizeof (cfp->ht->info.ht)) != 0) {
				return (FALSE);
			}

			/*
			 * Rock & roll.  Notify the kernel that this
			 * is an involuntary failover and go.
			 */
			if (sam_shareops(fs, SHARE_OP_INVOL_FAILOVER, 0) < 0) {
				SysError(HERE, "FS %s: sam_shareops(%s, "
				    "SHARE_OP_INVOL_FAILOVER)", fs, fs);
			} else {
				Trace(TR_MISC, "FS %s: sam_shareops(%s, "
				    "SHARE_OP_INVOL_FAILOVER)", fs, fs);
			}
			nfp->ht->info.ht.prevsrv = nfp->ht->info.ht.server;
			nfp->ht->info.ht.pendsrv = nfp->ht->info.ht.server;
			if (++nfp->ht->info.ht.gen == 0) {
				++nfp->ht->info.ht.gen;
			}
			/*
			 * Update the serverName/Addr strings in the new
			 * configuration block to match the new server.
			 * (putHosts() doesn't need this information, but
			 * putLabel() does.)
			 */
			if (GetSharedHostInfo(&nfp->ht->info.ht,
			    nfp->ht->info.ht.server,
			    nfp->serverName, nfp->serverAddr) < 0) {
				SysError(HERE, "FS %s: GetSharedHostInfo(%d)",
				    fs, nfp->ht->info.ht.server);
				strcpy(nfp->serverName, "-NONE-");
				strcpy(nfp->serverAddr, "-NOADDR-");
			}
			putHosts(fs, nfp);
			putLabel(fs, nfp);
			return (FALSE);
		}

		putLabel(fs, cfp);
		DoUpdate(fs);
		if (ShutdownDaemon) {
			return (FALSE);
		}
	}
#endif	/* METADATA_SERVER */

	if (cfp->ht->info.ht.pendsrv != cfp->ht->info.ht.server) {
		/*
		 * Pending server != current server, and the current
		 * server isn't us.  Stay out of it until the current
		 * server resolves the mess by updating the pending
		 * server.
		 */
		Trace(TR_MISC, "FS %s: Server change pending; exiting", fs);
		exit(EXIT_FAILURE);
	}

	return (TRUE);
}


/*
 * Update config information and swap config pointers.
 */
void
DoUpdate(char *fs)
{
	struct cfdata *cfp = &config.cp[config.curTab];
	struct cfdata *nfp = &config.cp[config.freeTab];
	static int clientSet;
	int status;

	status = updateConfig(fs);
	if (!ShutdownDaemon && !FsCfgOnly) {
		if (ServerHost) {
			if (status != 0) {
				Trace(TR_MISC, "FS %s: config updated; "
				    "Shutting down", fs);
				ShutdownDaemon = TRUE;
			}
		} else {
			if (status < 0) {
				Trace(TR_MISC, "FS %s: config updated; "
				    "Shutting down", fs);
				ShutdownDaemon = TRUE;
			}
		}
	}

#ifdef METADATA_SERVER
	if (ShutdownDaemon && ServerHost && !clientSet) {
		clientSet = 1;

		if (status <= 0) {
			goto out;
		}

		/*
		 * Check to see if failover is being called for.
		 * If none, skip out and just exit sam-sharefsd.
		 */
		if (nfp->ht->info.ht.server == nfp->ht->info.ht.pendsrv) {
			goto out;
		}

		/*
		 * The local host is no longer the metadata server.
		 * Rewrite the host file to make the pending server
		 * the current server.  Then we'll exit, and all
		 * the connected hosts will start anew.
		 */
		nfp->ht->info.ht.server = nfp->ht->info.ht.pendsrv;
		if (++nfp->ht->info.ht.gen == 0) {
			++nfp->ht->info.ht.gen;
		}
		/*
		 * Update the serverName/Addr strings in the new configuration
		 * block to match the new server.  (putHosts() doesn't need
		 * this information, but putLabel() does.)
		 */
		if (nfp->ht->info.ht.server == HOSTS_NOSRV) {
			strcpy(nfp->serverName, "-NONE-");
			strcpy(nfp->serverAddr, "-NOADDR-");
		} else if (GetSharedHostInfo(&nfp->ht->info.ht,
		    nfp->ht->info.ht.server,
		    nfp->serverName, nfp->serverAddr) < 0) {
			status = -1;
			errno = 0;
			SysError(HERE,
			    "FS %s: Hosts file change: bad Hosts file -- "
			    "can't extract pending server name; "
			    "server #%d",
			    fs, nfp->ht->info.ht.pendsrv);
			goto out;
		}
		/*
		 * Shut down operations as metadata server.
		 * Tell the kernel to initiate voluntary failover.
		 */
		if (SysFailover(fs, nfp->serverName) != 0) {
			SysError(HERE, "FS %s: SysFailover(%s, %s)",
			    fs, fs, nfp->serverName);
			goto out;
		}
		/*
		 * The kernel has finished its cleanup as server.
		 * Notify the other hosts that we're done and exit.
		 */
		putHosts(fs, nfp);
		putLabel(fs, nfp);
	}
#endif	/* METADATA_SERVER */

out:
	if (ShutdownDaemon && ServerHost && status < 0 &&
	    (nfp->flags & R_MNTD)) {
		/*
		 * Something awful happened; e.g., we discovered an
		 * involuntary failover in progress, and this host
		 * was the server.  Don't start up again.
		 */
		Trace(TR_MISC, "FS %s: Server: Bad failover status -- "
		    "no restart", fs);
#ifdef DEBUG
		exit(EXIT_NORESTART);
#endif
	}
	if (FsCfgOnly && status != 0) {
		int restart = FALSE;

		/*
		 * We're running in config-only mode, and aren't allowed to
		 * exit.  But changes have happened (we may now be the server,
		 * or may no longer be the server), and we need to get things
		 * restarted.
		 *
		 * If we have a superblock available, it's authoritative.
		 * Otherwise, we use the label block.  If a superblock or
		 * label block wasn't available at startup but now is,
		 * restart.
		 */
		if ((nfp->flags & R_SBLK_BITS) == R_SBLK) {
			if ((cfp->flags & R_SBLK_BITS) != R_SBLK) {
				restart = TRUE;
			} else if (cfp->ht->info.ht.pendsrv !=
			    nfp->ht->info.ht.pendsrv ||
			    cfp->ht->info.ht.server !=
			    nfp->ht->info.ht.server) {
				/*
				 * server changed in hosts file.
				 */
				restart = TRUE;
			}
		} else if ((nfp->flags & R_LBLK_BITS) == R_LBLK) {
			if ((cfp->flags & R_LBLK_BITS) != R_LBLK) {
				restart = TRUE;
			} else if (cfp->lb.info.lb.serverord !=
			    nfp->lb.info.lb.serverord) {
				/*
				 * server changed in label block
				 */
				restart = TRUE;
			}
		}
		if (restart) {
			int i;

			Trace(TR_MISC, "FS %s: Config only -- server "
			    "changed; re-execing self", fs);
			/*
			 * Close any open files.  If anyone has already
			 * connected, leaving the files open during the
			 * exec() will leave them hanging rather than retrying.
			 */
			for (i = STDERR_FILENO+1; i < FOPEN_MAX; i++) {
				(void) close(i);
			}
			sleep(5);	/* avoid excessive spin-looping */
			execl(SAM_EXECUTE_PATH "/sam-sharefsd",
			    "sam-sharefsd", fs, "mcf", NULL);
			exit(EXIT_NORESTART);		/* fer shure */
		}
	}

#ifdef METADATA_SERVER
	/*
	 * If we're the server, and the generation number of the host
	 * file changed, update the label block generation number also
	 * to keep it up to date.  (putLabel() updates gen number)
	 */
	if (!ShutdownDaemon && ServerHost && status >= 0) {
		if (cfp->ht->info.ht.gen != nfp->ht->info.ht.gen) {
			putLabel(fs, nfp);
		}
	}
#endif /* METADATA_SERVER */

	swapConfig(fs);
}
