/*
 *	configcmp.c - configuration comparison functions.
 *
 *	The functions herein should be called only from the server.
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

#pragma ident "$Revision: 1.28 $"

static char *_SrcFile = __FILE__;
/* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <strings.h>

/* POSIX headers. */
#include <sys/vfs.h>

/* Solaris headers. */
	/* none */

/* SAM-FS headers. */
#include "sam/mount.h"

#include "sblk.h"
#include "samhost.h"

/* Local headers. */
#include "sharefs.h"
#include "config.h"


extern struct shfs_config config;

static int cmpSBlk(char *fs, struct sam_sbinfo *, struct cfdata *);
static int cmpHT(char *fs, struct cfdata *, struct cfdata *);
static int cmpLBlk(char *fs, struct sam_label *, struct sam_label *);


/*
 * Compare the old/new configurations in config.cp[x].
 */
int
CmpConfig(char *fs, struct sam_sblk *sb)
{
	struct cfdata *ncp = &config.cp[config.freeTab];
	struct cfdata *ocp = &config.cp[config.curTab];
	int r, diff, change = 0;

	diff = ocp->flags ^ ncp->flags;

	if (diff & R_MNTD) {
		change |= CH_CF_MOUNT;
		diff &= ~R_MNTD;
	}
	if (diff) {
		Trace(TR_MISC, "FS %s: config/access change - old/new "
		    "flags %x/%x",
		    fs, ocp->flags, ncp->flags);
		change |= CH_CF_FLAGS;
	}

	/*
	 * superblock/hosts table changes?
	 */
	if (diff & R_SBLK_BITS) {
		change |= CH_SB_SBLK;
		Trace(TR_MISC, "FS %s: Superblock access changed", fs);
	} else {
		if ((ocp->flags & R_SBLK_BITS) == R_SBLK) {
			if ((r = cmpSBlk(fs, &sb->info.sb, ncp)) != 0) {
				change |= r;
				Trace(TR_MISC, "FS %s: Superblock modified",
				    fs);
			}
			if ((r = cmpHT(fs, ocp, ncp)) != 0) {
				change |= r;
				Trace(TR_MISC, "FS %s: Hosts file modified",
				    fs);
			}
		}
	}

	/*
	 * Label block changes?
	 */
	if (diff & R_LBLK_BITS) {
		change |= CH_LB_LABEL;
		Trace(TR_MISC, "FS %s: Label access changed", fs);
	} else if ((r = cmpLBlk(fs, &ocp->lb.info.lb, &ncp->lb.info.lb))
	    != 0) {
		change |= r;
		Trace(TR_MISC, "FS %s: Label block modified", fs);
	}

	/*
	 * Local hosts file changes?
	 */
	if (diff & R_LHOSTS_BITS) {
		change |= CH_LC_HOSTS;
		Trace(TR_MISC, "FS %s: Local hosts file access changed", fs);
	} else if (bcmp(&ocp->lHostModTime, &ncp->lHostModTime,
	    sizeof (ocp->lHostModTime))) {
		Trace(TR_MISC, "FS %s: hosts.%s.local file mod time changed",
		    fs, fs);
		change |= CH_LC_HOSTS;
	}

	return (change);
}


#ifdef METADATA_SERVER
/*
 * Comparison functions to determine if the
 * system FS configuration has somehow changed.
 *
 * CH_CF_FLAGS      File system name or type (shared) changed
 * CH_CF_PART       File system partitions changed
 */
int
CmpFsInfo(
	char *fs,
	struct sam_mount_info *mip1,
	struct sam_mount_info *mip2)
{
	struct sam_fs_info *fip1 = &mip1->params,  *fip2 = &mip2->params;
	struct sam_fs_part *fsp1 = &mip1->part[0], *fsp2 = &mip2->part[0];
	int i, change = 0;

	if (strncmp(fip1->fi_name, fip2->fi_name, sizeof (fip1->fi_name))) {
		Trace(TR_MISC, "FS %s: Configured name changed (%s/%s)",
		    fs, fip1->fi_name, fip2->fi_name);
		change = CH_CF_FLAGS;
	} else if ((fip1->fi_config1 ^ fip2->fi_config1) & MC_SHARED_FS) {
		Trace(TR_MISC, "FS %s: Share config changed (%x/%x)",
		    fs, fip1->fi_config1, fip2->fi_config1);
		change = CH_CF_FLAGS;
	} else if (fip1->fs_count != fip2->fs_count) {
		Trace(TR_MISC, "FS %s: Configured fs_count changed (%x/%x)",
		    fs, fip1->fs_count, fip2->fs_count);
		change = CH_CF_PART;
	}
	if (change) {
		return (change);
	}
	for (i = 0; i < fip1->fs_count; i++, fsp1++, fsp2++) {
		if (strncmp(fsp1->pt_name, fsp2->pt_name,
		    sizeof (fsp1->pt_name)) != 0) {
			Trace(TR_MISC, "FS %s: Partition name changed (%s/%s)",
			    fs, fsp1->pt_name, fsp2->pt_name);
			change = CH_CF_PART;
		}
		if (fsp1->pt_eq != fsp2->pt_eq) {
			Trace(TR_MISC, "FS %s: Partition eq changed (%d/%d)",
			    fs, fsp1->pt_eq, fsp2->pt_eq);
			change = CH_CF_PART;
		}
		if (fsp1->pt_type != fsp2->pt_type) {
			Trace(TR_MISC, "FS %s: Partition type changed (%x/%x)",
			    fs, fsp1->pt_type, fsp2->pt_type);
			change = CH_CF_PART;
		}
		if (fsp1->pt_state != fsp2->pt_state) {
			Trace(TR_MISC, "FS %s: Partition state changed (%x/%x)",
			    fs, fsp1->pt_state, fsp2->pt_state);
			change = CH_CF_PART;
		}
	}
	return (change);
}
#endif	/* METADATA_SERVER */


/*
 * Compare new super block info to saved data
 *
 * CH_SB_SBLK      superblock data changed (anything)
 * CH_SB_NOFS      No FS superblock
 * CH_SB_NOTSH     FS no longer shared
 * CH_SB_NEWFS     New filesystem ID (usu. mkfs)
 * CH_SB_FSNAME    FS name changed (!= config)
 *
 */
static int
cmpSBlk(
	char *fs,
	struct sam_sbinfo *sb,
	struct cfdata *cfp)
{
	int change = 0;

	if (sb->magic != SAM_MAGIC_V2 &&
	    sb->magic != SAM_MAGIC_V2A) {
		Trace(TR_MISC,
		    "FS %s: Superblock magic # changed (%x/%x or %x) - "
		    "fs destroyed?",
		    fs, sb->magic, SAM_MAGIC_V2, SAM_MAGIC_V2A);
		change |= CH_SB_SBLK | CH_SB_NOFS;
	} else {
		if (sb->init != config.fsId) {
			Trace(TR_MISC,
			    "FS %s: Superblock init changed (%x/%x) - "
			    "fs mkfsed?",
			    fs, sb->init, config.fsId);
			change |= CH_SB_SBLK | CH_SB_NEWFS;
		}
		if (strncmp(sb->fs_name, fs, sizeof (sb->fs_name)) != 0) {
			Trace(TR_MISC,
			    "FS %s: Superblock fs_name changed (%s/%s) - "
			    "fs mkfsed?",
			    fs, sb->fs_name, fs);
			change |= CH_SB_SBLK | CH_SB_FSNAME;
		}
		if (sb->hosts != cfp->hostsOffset) {
			Trace(TR_MISC,
			    "FS %s: Hosts file offset changed "
			    "(%llx/%llx) - fs mkfsed?",
			    fs, sb->hosts, cfp->hostsOffset);
			if (sb->hosts == 0) {	/* mkfsed, new FS not shared */
				change = CH_SB_SBLK | CH_SB_NOTSH;
			} else {		/* hosts file moved... ? */
				change = CH_SB_SBLK | CH_SB_SBLK;
			}
		}
	}
	return (change);
}


/*
 * Compare two host tables
 *
 * We report 'no change' if the only change is an incremented
 * generation #.
 */
static int
cmpHT(char *fs, struct cfdata *ocp, struct cfdata *ncp)
{
	int len, change = 0;
	struct sam_host_table *ht1 = &ocp->ht->info.ht;
	struct sam_host_table *ht2 = &ncp->ht->info.ht;

	len = ht1->length;
	if (len <= offsetof(struct sam_host_table, ent) ||
	    len > ocp->htsize) {
		errno = 0;
		SysError(HERE, "FS %s: host table size out-of-range (%x)",
		    fs, len);
		change = CH_HT_HOSTS | CH_HT_BAD;
		return (change);
	}
	len = ht2->length;
	if (len <= offsetof(struct sam_host_table, ent) ||
	    len > ncp->htsize) {
		errno = 0;
		SysError(HERE, "FS %s: host table size out-of-range (%x)",
		    fs, len);
		change = CH_HT_HOSTS | CH_HT_BAD;
		return (change);
	}
	len -= offsetof(struct sam_host_table, ent);
	if (ht1->version != ht2->version) {
		change = CH_HT_HOSTS | CH_HT_VERS;
		if (ht2->version <= SAM_HOSTS_VERSION_MIN ||
		    ht2->version > SAM_HOSTS_VERSION_MAX ||
		    ht2->version < ht1->version) {
			change |= CH_HT_BAD;
		}
	} else if (ht1->count != ht2->count ||
	    ht1->length != ht2->length ||
	    bcmp(ht1->ent, ht2->ent, len)) {
		change |= CH_HT_HOSTS;
	}
	if (ht1->prevsrv == HOSTS_NOSRV || ht2->prevsrv == HOSTS_NOSRV) {
		change |= CH_HT_HOSTS | CH_HT_INVOL;
	}
	/*
	 * XXX Tests for CH_HT_RESET/CH_HT_PEND might use AFTER() macro
	 * XXX or similar to avoid wrap problems.
	 */
	if (ht1->gen > ht2->gen) {
		Trace(TR_MISC,
		    "FS %s: Hosts file gen # decreased (%d -> %d)",
		    fs, ht1->gen, ht2->gen);
		change |= CH_HT_HOSTS | CH_HT_RESET;
	}
	if (ht1->server != ht2->server) {
		change |= CH_HT_HOSTS | CH_HT_BAD;
	}
	if (ht1->pendsrv != ht2->pendsrv) {
		if (ht2->gen > ht1->gen) {
			if ((change &
			    (CH_HT_INVOL | CH_HT_RESET | CH_HT_BAD)) == 0) {
				change |= CH_HT_HOSTS | CH_HT_PEND;
			}
		} else {
			change = CH_HT_HOSTS | CH_HT_BAD;
		}
	}
	if (change & (CH_HT_RESET | CH_HT_BAD)) {
		Trace(TR_MISC, "FS %s: Unexpected hosts file change (%x)",
		    fs, change);
	}
	return (change);
}


/*
 * Compare two label blocks
 */
static int
cmpLBlk(
	char *fs,
	struct sam_label *lb1,
	struct sam_label *lb2)
{
	int change = 0;

	if (strncmp(lb1->name, lb2->name, sizeof (lb1->name)) != 0) {
		change |= CH_LB_LABEL | CH_LB_NAME;
	}
	if (lb1->magic != lb2->magic) {
		change |= CH_LB_LABEL | CH_LB_BAD;
	}
	if (lb1->init != lb2->init) {
		change |= CH_LB_LABEL | CH_LB_FSID;
	}
	if (strncmp(lb1->server, lb2->server, sizeof (lb1->server)) != 0) {
		change |= CH_LB_LABEL | CH_LB_SRVR;
		if (lb1->gen >= lb2->gen) {
			change |= CH_LB_RESET;
		}
	}
	if (strncmp(lb1->serveraddr, lb2->serveraddr,
	    sizeof (lb1->serveraddr)) != 0) {
		change |= CH_LB_LABEL;
		if (lb1->gen >= lb2->gen) {
			change |= CH_LB_RESET;
		}
	} else if (bcmp((char *)lb1, (char *)lb2, L_LABEL) != 0) {
		change = CH_LB_LABEL;
	}
	if (change & CH_LB_RESET) {
		errno = 0;
		SysError(HERE, "FS %s: non-increasing gen # (%d -> %d)",
		    fs, lb1->gen, lb2->gen);
	}
	return (change);
}
