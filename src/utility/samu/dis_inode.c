/*
 *  dis_inode.c - Display inode information.
 *
 * Displays inode information for the selected inode.
 *
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

#pragma ident "$Revision: 1.46 $"


/* ANSI headers. */
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <sys/buf.h>

/* Solaris headers. */
#include <sys/vfs.h>
#if defined(i386) || defined(__i386) || defined(amd64) || defined(__amd64)
#include <sys/mutex_impl.h>
#else
#include <v9/sys/mutex_impl.h>
#endif
#include <sys/acl.h>

/* SAM-FS headers. */
#include <sam/resource.h>
#include <sam/nl_samfs.h>
#include <sam/uioctl.h>
#include <sam/fs/rmedia.h>
#include <sam/fs/ino.h>
#include <sam/fs/ino_ext.h>
#include <sam/fs/inode.h>
#include <pub/stat.h>

/* Local headers. */
#undef ERR
#include "samu.h"

/* Private functions. */
static void dis_incore(sam_node_t *node);
static void dis_inode_arch(struct sam_perm_inode *ino);
static void dis_inode_ext(struct sam_inode_ext *node);
static char *sam_attrtoe(int attr, char *string);
static char *sam_perm_str(int perm, char *string);

/* Private data. */
static struct {
	char sel;
	char *name;
} format[] = {
	{ 'f', "file" },
	{ 'F', "file" },
	{ 'a', "archive" },
	{ 'r', "raw" },
	{ 'R', "rawincore" }
};
static struct MemPars inmp;
static int inode = 1;	/* Inode number */
static int fmt;			/* Display format */
static boolean incore;	/* Incore data present */
	/*
	 *	WARNING: the F_RDINO ioctl returns different amounts of data on
	 *	32 and 64 bit kernel file systems.  Double it to be sure.
	 */
#define	SAMNODE_SIZE (2*sizeof (struct sam_node))
static struct sam_node *ip = NULL;		/* File incore inode image */
static struct sam_perm_inode *pip = NULL;	/* File permanent inode image */


void
DisInode()
{
	struct sam_ioctl_inode rq_inode;	/* Inode request parameters */
	struct sam_disk_inode *dp;		/* File disk image */
	struct sam_perm_inode *permp;	/* Perm image */
	struct sam_inode_ext *ap;		/* Inode extension image */

	memset(ip, 0, SAMNODE_SIZE);
	memset(pip, 0, sizeof (struct sam_perm_inode));
	Mvprintw(0, 10, "0x%lx (%ld) %s: %-7s", inode, inode,
	    catgets(catfd, SET, 7304, "format"), format[fmt].name);

	rq_inode.ino = inode;		/* Read inode image */
	rq_inode.mode = 0;
	rq_inode.extent_factor = 0;
	rq_inode.ip.ptr = ip;
	rq_inode.pip.ptr = pip;
	if (ioctl(Ioctl_fd, F_RDINO, &rq_inode) != 0)  Error("ioctl(F_RDINO)");
	incore = !rq_inode.mode;
	Mvprintw(1, 50, "%s: %c",
	    catgets(catfd, SET, 7305, "incore"), incore ? 'y' : 'n');
	dp = (struct sam_disk_inode *)pip;
	permp = (struct sam_perm_inode *)pip;

	switch (format[fmt].sel) {
	case 'F':			/* file/directory inode */
					/* (raw extent blocks) */
		rq_inode.extent_factor = 0;	/* Raw display */
		/* FALLTHROUGH */
	case 'f':			/* file/directory inode */
		if (S_ISEXT(dp->mode)) {
			ap = (struct sam_inode_ext *)pip;
			dis_inode_ext(ap);
		} else {
			dis_inode_file(permp, rq_inode.extent_factor);
		}
		break;

	case 'a':			/* archive inode */
		dis_inode_arch(pip);
		break;

	case 'r':			/* raw inode */
		inmp.base = (uchar_t *)pip;
		inmp.size = ((sizeof (sam_node_t) + 15) / 16) * 16;
		inmp.size = 1024;
		DisMem(&inmp, 2);
		break;

	case 'R':			/* raw incore inode */
		inmp.base = (uchar_t *)ip;
		inmp.size = ((sizeof (sam_node_t) + 15) / 16) * 16;
		inmp.size = 1024;
		if (S_ISEXT(dp->mode)) {
			inmp.base = (uchar_t *)pip;
		}
		DisMem(&inmp, 2);
		break;

	case 'i':			/* Incore inode */
		if (rq_inode.mode == 1)  break;
		dis_incore(ip);
		break;

	default:
		break;
	}
}


/*
 * Display initialization.
 */
boolean
InitInode(void)
{
	char *mp;

	mp = NULL;
	if (ip == NULL) {
		ip = malloc(SAMNODE_SIZE);
	}
	if (pip == NULL) {
		pip = malloc(sizeof (struct sam_perm_inode));
	}
	if (Argc == 2)  {
		char *p;
		int ino;

		errno = 0;
		p = 0;
		ino = strtol(Argv[1], &p, 0);
		if (*p != '\0' || errno != 0 || ino == 0) {
			Error(catgets(catfd, SET, 7007,
			    "Not a valid inode number (%s)."),
			    Argv[1]);
		} else {
			inode = ino;
		}
	}
	if (Argc > 2)  {
		char *p;
		int ino;

		errno = 0;
		mp = Argv[2];
		p = 0;
		ino = strtol(Argv[1], &p, 0);
		if (*p != '\0') {
			ino = strtol(Argv[2], &p, 0);
			mp = Argv[1];
		} else if (errno != 0 || ino == 0) {
			Error(catgets(catfd, SET, 7007,
			    "Not a valid inode number (%s)."),
			    Argv[1]);
		}
		inode = ino;
	}
	open_mountpt(mp);
	return (TRUE);
}


/*
 * Keyboard processing.
 */
boolean
KeyInode(char key)
{
	int n;

	if (key == KEY_full_fwd) {
		inode++;
		return (TRUE);
	}
	if (key == KEY_full_bwd) {
		if (--inode == 0)  inode = 1;
		return (TRUE);
	}
	if (key == KEY_adv_fmt) {
		if (++fmt >= numof(format))  fmt = 0;
		return (TRUE);
	}
	for (n = 0; n < numof(format); n++) {
		if (format[n].sel == key) {
			fmt = n;
			return (TRUE);
		}
	}
	return (KeyMem(&inmp, key));
}


/*
 * Display incore inode.
 * Displays the contents of the incore inode.
 */
static void
dis_incore(sam_node_t *node)	/* Incore inode image */
{
	static char *vtypes[] = {
		"none", "reg", "dir", "blk", "chr",
		"link", "fifo", "xnam", "bad"};
	int fl;
	vnode_t *vnode;
	uint_t *var;

	fl = ln;
	vnode = node->vnode;

	Mvprintw(ln++, 0, "%.4x     v_flag", vnode->v_flag);
	Mvprintw(ln++, 0, "%.8x v_count    (%d)", vnode->v_count,
	    vnode->v_count);
	Mvprintw(ln++, 0, "%.8x v_vfsmountedhere", vnode->v_vfsmountedhere);
	Mvprintw(ln++, 0, "%.8x v_op", vnode->v_op);
	Mvprintw(ln++, 0, "%.8x v_vfsp", vnode->v_vfsp);
	Mvprintw(ln++, 0, "%.8x v_stream", vnode->v_stream);
	Mvprintw(ln++, 0, "%.8x v_pages", vnode->v_pages);
	Mvprintw(ln++, 0, "%.8x v_type     %s", vnode->v_type,
	    vtypes[vnode->v_type]);
	Mvprintw(ln++, 0, "%.8x v_rdev", vnode->v_rdev);
	Mvprintw(ln++, 0, "%.8x v_data", vnode->v_data);
	Mvprintw(ln++, 0, "%.8x v_filocks", vnode->v_filocks);
	ln += 1;
	Mvprintw(ln++, 0, "%.8x chain.hash.f", node->chain.hash.forw);
	Mvprintw(ln++, 0, "%.8x chain.hash.b", node->chain.hash.back);
	Mvprintw(ln++, 0, "%.8x chain.free.f", node->chain.free.forw);
	Mvprintw(ln++, 0, "%.8x chain.free.b", node->chain.free.back);
	Mvprintw(ln++, 0, "%.8x mp", node->mp);
	var = (void *)&node->copy;
	Mvprintw(ln++, 0, "%.8x copy/copy_mask/fill", *var);
	var = (void *)&node->stage_err;
	Mvprintw(ln++, 0, "%.8x stage_err/fill", *var);
	Mvprintw(ln++, 0, "%.8x rdev", node->rdev);
	Mvprintw(ln++, 0, "%.8x dev", node->dev);
	Mvprintw(ln++, 0, "%.8x rm_pid", node->rm_pid);
	Mvprintw(ln++, 0, "%.8x rm_err", node->rm_err);
	Mvprintw(ln++, 0, "%.8x io_count", node->io_count);
	Mvprintw(ln++, 0, "%.8x space_u", node->space);
	Mvprintw(ln++, 0, "%.8x space_l", (uint_t)node->space);
	Mvprintw(ln++, 0, "%.8x mm_pages", node->mm_pages);
	var = (uint_t *)&node->write_mutex;
	Mvprintw(ln++, 0, "%.8x write_mutex owner", MUTEX_OWNER_PTR(*var));
	Mvprintw(ln++, 0, "%.8x koffset_u", node->koffset);
	Mvprintw(ln++, 0, "%.8x koffset_l", (uint_t)node->koffset);
	Mvprintw(ln++, 0, "%.8x klength", node->klength);

	ln = fl;
	Mvprintw(ln++, 40, "%.8x flags", node->flags.bits);
	Mvprintw(ln++, 40, "%.8x lbase", (uint_t)node->lbase);
	Mvprintw(ln++, 40, "%.8x page_off_u", node->page_off);
	Mvprintw(ln++, 40, "%.8x page_off_l", (uint_t)node->page_off);
	Mvprintw(ln++, 40, "%.8x ra_off_u", node->ra_off);
	Mvprintw(ln++, 40, "%.8x ra_off_l", (uint_t)node->ra_off);
	Mvprintw(ln++, 40, "%.8x cnt_writes", node->cnt_writes);
	Mvprintw(ln++, 40, "%.8x stage_pid", node->stage_pid);
	Mvprintw(ln++, 40, "%.8x rm_wait", node->rm_wait);
	var = (uint_t *)&node->daemon_mutex;
	Mvprintw(ln++, 40, "%.8x daemon_mutex owner", MUTEX_OWNER_PTR(*var));
	var = (uint_t *)&node->rm_mutex;
	Mvprintw(ln++, 40, "%.8x rm_mutex owner", MUTEX_OWNER_PTR(*var));
	Mvprintw(ln++, 40, "%.8x mt_handle", node->mt_handle);
	Mvprintw(ln++, 40, "%.8x inode_rwl", node->inode_rwl);
	Mvprintw(ln++, 40, "%.8x data_rwl", node->data_rwl);
	Mvprintw(ln++, 40, "%.8x stage_n_count", node->stage_n_count);
	Mvprintw(ln++, 40, "%.8x real_stage_off_u", node->real_stage_off);
	Mvprintw(ln++, 40, "%.8x real_stage_off_l",
	    (uint_t)node->real_stage_off);
	Mvprintw(ln++, 40, "%.8x real_stage_len_u", node->real_stage_len);
	Mvprintw(ln++, 40, "%.8x real_stage_len_l",
	    (uint_t)node->real_stage_len);
	Mvprintw(ln++, 40, "%.8x stage_n_off_u", node->stage_n_off);
	Mvprintw(ln++, 40, "%.8x stage_n_off_l", (uint_t)node->stage_n_off);
	Mvprintw(ln++, 40, "%.8x stage_off_u", node->stage_off);
	Mvprintw(ln++, 40, "%.8x stage_off_l", (uint_t)node->stage_off);
	Mvprintw(ln++, 40, "%.8x stage_len_u", node->stage_len);
	Mvprintw(ln++, 40, "%.8x stage_len_l", (uint_t)node->stage_len);
	Mvprintw(ln++, 40, "%.8x stage_size_u", node->stage_size);
	Mvprintw(ln++, 40, "%.8x stage_size_l", (uint_t)node->stage_size);
	Mvprintw(ln++, 40, "%.8x size_u", node->size);
	Mvprintw(ln++, 40, "%.8x size_l", (uint_t)node->size);
}


/*
 * Displays the contents of the inode extension.
 */
static void
dis_inode_ext(struct sam_inode_ext *node)	/* Inode extension image */
{
	int fl, ll;
	char *m_str;		/* Inode mode string */
	uint_t *w;
	int i, x;
	lloff_t *lp;
	vsn_t vsn;
	char *atype;
	char *aperm;

	fl = ln;
	m_str = ext_mode_string(node->hdr.mode);
	Mvprintw(ln++, 0, "%.8x mode       %s", node->hdr.mode, m_str);
	Mvprintw(ln++, 0, "%.8x version", node->hdr.version);
	Mvprintw(ln++, 0, "%.8x ino        (%d)",
	    node->hdr.id.ino, node->hdr.id.ino);
	Mvprintw(ln++, 0, "%.8x gen        (%d)",
	    node->hdr.id.gen, node->hdr.id.gen);
	Mvprintw(ln++, 0, "%.8x file_ino   (%d)",
	    node->hdr.file_id.ino, node->hdr.file_id.ino);
	Mvprintw(ln++, 0, "%.8x file_gen   (%d)",
	    node->hdr.file_id.gen, node->hdr.file_id.gen);
	Mvprintw(ln++, 0, "%.8x next_ino   (%d)",
	    node->hdr.next_id.ino, node->hdr.next_id.ino);
	Mvprintw(ln++, 0, "%.8x next_gen   (%d)",
	    node->hdr.next_id.gen, node->hdr.next_id.gen);

	ll = ln;
	switch (node->hdr.mode & S_IFEXT) {
	case S_IFMVA:
		if (node->hdr.version >= SAM_INODE_VERS_2) {
						/* Current version */
			ln = fl;
			Mvprintw(ln++, 40, "%.8x creation_time",
			    node->ext.mva.creation_time);
			Mvprintw(ln++, 40, "%.8x change_time",
			    node->ext.mva.change_time);
			Mvprintw(ln++, 40, "%.8x copy", node->ext.mva.copy);
			Mvprintw(ln++, 40, "%.8x t_vsns", node->ext.mva.t_vsns);
			Mvprintw(ln++, 40, "%.8x n_vsns", node->ext.mva.n_vsns);

			fl = ln = (ll > ln) ? ll : ln;
			for (i = 0; i < node->ext.mva.n_vsns; i++) {
				if (i & 1) {
					ln = fl;
					x = 40;
				} else {
					fl = ++ln;
					x = 0;
				}
				w = (void *)&node->ext.mva.vsns.section[i].vsn;
				Mvprintw(ln++, x, "%.8x %.32s", *w++,
				    string(node->ext.mva.vsns.section[i].vsn));
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn ...", *w++);
				Mvprintw(ln++, x, "%.8llx length    (%lld)",
				    node->ext.mva.vsns.section[i].length,
				    node->ext.mva.vsns.section[i].length);
				Mvprintw(ln++, x, "%.8llx position",
				    node->ext.mva.vsns.section[i].position);
				Mvprintw(ln++, x, "%.8llx offset",
				    node->ext.mva.vsns.section[i].offset);
			}
		} else if (node->hdr.version == SAM_INODE_VERS_1) {
						/* Prev version */
			ln = fl;
			Mvprintw(ln++, 40, "%.8x copy", node->ext.mv1.copy);
			Mvprintw(ln++, 40, "%.8x n_vsns", node->ext.mv1.n_vsns);

			fl = ln = (ll > ln) ? ll : ln;
			for (i = 0; i < node->ext.mv1.n_vsns; i++) {
				if (i & 1) {
					ln = fl;
					x = 40;
				} else {
					fl = ++ln;
					x = 0;
				}
				w = (void *)&node->ext.mv1.vsns.section[i].vsn;
				Mvprintw(ln++, x, "%.8x %.32s", *w++,
				    string(node->ext.mv1.vsns.section[i].vsn));
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn ...", *w++);
				Mvprintw(ln++, x, "%.8llx length    (%lld)",
				    node->ext.mv1.vsns.section[i].length,
				    node->ext.mv1.vsns.section[i].length);
				Mvprintw(ln++, x, "%.8llx position",
				    node->ext.mv1.vsns.section[i].position);
				Mvprintw(ln++, x, "%.8llx offset",
				    node->ext.mv1.vsns.section[i].offset);
			}
		}
		break;
	case S_IFSLN:
		ln = fl;
		Mvprintw(ln++, 40, "%.8x creation_time",
		    node->ext.sln.creation_time);
		Mvprintw(ln++, 40, "%.8x change_time",
		    node->ext.sln.change_time);
		Mvprintw(ln++, 40, "%.8x n_chars    (%d)",
		    node->ext.sln.n_chars, node->ext.sln.n_chars);

		ln = (ll > ln) ? ll : ln;
		ln++;
		inmp.base = (uchar_t *)&node->ext.sln.chars[0];
		inmp.size = MAX_SLN_CHARS_IN_INO;
		DisMem(&inmp, ln);
		break;
	case S_IFRFA:
		if (EXT_1ST_ORD(node)) {
			ln++;
			Mvprintw(ln++, 0, "%.8x creation_time",
			    node->ext.rfa.creation_time);
			Mvprintw(ln++, 0, "%.8x change_time",
			    node->ext.rfa.change_time);

			Mvprintw(ln++, 0, "00%.2x%.2x%.2x res.rev/acc/prot",
			    node->ext.rfa.info.resource.revision,
			    node->ext.rfa.info.resource.access,
			    node->ext.rfa.info.resource.protect);
			lp = (lloff_t *)
			    &node->ext.rfa.info.resource.archive.rm_info.size;
			Mvprintw(ln++, 0, "%.8x res.ar.rm.size_u", lp->_p._u);
			Mvprintw(ln++, 0, "%.8x res.ar.rm.size_l  (%llu)",
			    lp->_p._l, lp->_f);
			w = (void *)
			    &node->ext.rfa.info.resource.archive.rm_info.media;
			Mvprintw(ln++, 0, "%.4x%.4x res.ar.rm.media/flags",
			    node->ext.rfa.info.resource.archive.rm_info.media,
			    *++w);
			Mvprintw(ln++, 0, "%.8x res.ar.rm.file_offset",
			    node->
			    ext.rfa.info.resource.archive.rm_info.file_offset);
			Mvprintw(ln++, 0, "%.8x res.ar.rm.mau",
			    node->ext.rfa.info.resource.archive.rm_info.mau);
			Mvprintw(ln++, 0, "%.8x res.ar.rm.position",
			    node->
			    ext.rfa.info.resource.archive.rm_info.position);
			Mvprintw(ln++, 0, "%.8x res.ar.creation_time",
			    node->ext.rfa.info.resource.archive.creation_time);
			Mvprintw(ln++, 0, "%.8x res.ar.n_vsns",
			    node->ext.rfa.info.resource.archive.n_vsns);
			for (x = 0; x < 32; x++) { /* vsn is not word aligned */
				vsn[x] =
				    node->ext.rfa.info.resource.archive.vsn[x];
			}
			w = (void *)&vsn;
			Mvprintw(ln++, 0, "%.8x %.32s", *w++,
			    string(vsn));
			Mvprintw(ln++, 0, "%.8x res.ar.vsn", *w++);
			Mvprintw(ln++, 0, "%.8x res.ar.vsn", *w++);
			Mvprintw(ln++, 0, "%.8x res.ar.vsn ...", *w++);
			Mvprintw(ln++, 0, "%.8x res.ar.mc.od.label_pda",
			    node->
			    ext.rfa.info.resource.archive.mc.od.label_pda);
			Mvprintw(ln++, 0, "%.8x res.ar.mc.od.version",
			    node->ext.rfa.info.resource.archive.mc.od.version);
			w = (void *)
			    &node->ext.rfa.info.resource.archive.mc.od.file_id;
			Mvprintw(ln++, 0, "%.8x %.32s", *w++,
			    string(node->
			    ext.rfa.info.resource.archive.mc.od.file_id));
			Mvprintw(ln++, 0, "%.8x res.ar.mc.od.fid", *w++);
			Mvprintw(ln++, 0, "%.8x res.ar.mc.od.fid", *w++);
			Mvprintw(ln++, 0, "%.8x res.ar.mc.od.fid ...", *w++);

			ll = ln;
			ln = fl;
			w = (void *)&node->ext.rfa.info.resource.next_vsn;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.resource.next_vsn));
			Mvprintw(ln++, 40, "%.8x res.next_vsn", *w++);
			Mvprintw(ln++, 40, "%.8x res.next_vsn", *w++);
			Mvprintw(ln++, 40, "%.8x res.next_vsn ...", *w++);
			w = (void *)&node->ext.rfa.info.resource.prev_vsn;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.resource.prev_vsn));
			Mvprintw(ln++, 40, "%.8x res.prev_vsn", *w++);
			Mvprintw(ln++, 40, "%.8x res.prev_vsn", *w++);
			Mvprintw(ln++, 40, "%.8x res.prev_vsn ...", *w++);
			Mvprintw(ln++, 40, "%.8x res.req_size",
			    node->ext.rfa.info.resource.required_size);
			w = (void *)&node->ext.rfa.info.resource.mc.od.group_id;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.resource.mc.od.group_id));
			Mvprintw(ln++, 40, "%.8x res.mc.od.gid", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.gid", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.gid ...", *w++);
			w = (void *)&node->ext.rfa.info.resource.mc.od.owner_id;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.resource.mc.od.owner_id));
			Mvprintw(ln++, 40, "%.8x res.mc.od.oid", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.oid", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.oid ...", *w++);
			w = (void *)&node->ext.rfa.info.resource.mc.od.info;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.resource.mc.od.info));
			Mvprintw(ln++, 40, "%.8x res.mc.od.info", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.info", *w++);
			Mvprintw(ln++, 40, "%.8x res.mc.od.info ...", *w++);
			Mvprintw(ln++, 40, "%.8x cur_ord",
			    node->ext.rfa.info.cur_ord);
			Mvprintw(ln++, 40, "%.8x n_vsns",
			    node->ext.rfa.info.n_vsns);
			w = (void *)&node->ext.rfa.info.section[0].vsn;
			Mvprintw(ln++, 40, "%.8x %.32s", *w++,
			    string(node->ext.rfa.info.section[0].vsn));
			Mvprintw(ln++, 40, "%.8x sec.vsn", *w++);
			Mvprintw(ln++, 40, "%.8x sec.vsn", *w++);
			Mvprintw(ln++, 40, "%.8x sec.vsn ...", *w++);
			Mvprintw(ln++, 40, "%.8llx sec.length    (%lld)",
			    node->ext.rfa.info.section[0].length,
			    node->ext.rfa.info.section[0].length);
			Mvprintw(ln++, 40, "%.8llx sec.position",
			    node->ext.rfa.info.section[0].position);
			Mvprintw(ln++, 40, "%.8llx sec.offset",
			    node->ext.rfa.info.section[0].offset);

		} else {
			ln = fl;
			Mvprintw(ln++, 40, "%.8x ord", node->ext.rfv.ord);
			Mvprintw(ln++, 40, "%.8x n_vsns", node->ext.rfv.n_vsns);

			fl = ln = (ll > ln) ? ll : ln;
			for (i = 0; i < node->ext.rfv.n_vsns; i++) {
				if (i & 1) {
					ln = fl;
					x = 40;
				} else {
					fl = ++ln;
					x = 0;
				}
				w = (void *)&node->ext.rfv.vsns.section[i].vsn;
				Mvprintw(ln++, x, "%.8x %.32s", *w++,
				    string(node->ext.rfv.vsns.section[i].vsn));
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn", *w++);
				Mvprintw(ln++, x, "%.8x vsn ...", *w++);
				Mvprintw(ln++, x, "%.8llx length    (%lld)",
				    node->ext.rfv.vsns.section[i].length,
				    node->ext.rfv.vsns.section[i].length);
				Mvprintw(ln++, x, "%.8llx position",
				    node->ext.rfv.vsns.section[i].position);
				Mvprintw(ln++, x, "%.8llx offset",
				    node->ext.rfv.vsns.section[i].offset);
			}
		}
		break;
	case S_IFHLP:
		ln = fl;
		Mvprintw(ln++, 40, "%.8x creation_time",
		    node->ext.hlp.creation_time);
		Mvprintw(ln++, 40, "%.8x change_time",
		    node->ext.hlp.change_time);
		Mvprintw(ln++, 40, "%.8x n_ids      (%d)",
		    node->ext.hlp.n_ids, node->ext.hlp.n_ids);

		ln = (ll > ln) ? ll : ln;
		ln++;
		Mvprintw(ln, 0, "Parent ids:");
		for (i = 0; i < MAX_HLP_IDS_IN_INO; i++) {
			if ((i % 4) == 0) {
				ln++;
				Mvprintw(ln, 0, "%.2d_", i);
				x = 4;
			}
			Mvprintw(ln, x, "%.8x.%.4x",
			    node->ext.hlp.ids[i].ino, node->ext.hlp.ids[i].gen);
			x += 18;
		}
		break;
	case S_IFACL:
		ln = fl;
		Mvprintw(ln++, 40, "%.8x creation_time",
		    node->ext.acl.creation_time);
		Mvprintw(ln++, 40, "%.8x t_acls     (%d)",
		    node->ext.acl.t_acls, node->ext.acl.t_acls);
		Mvprintw(ln++, 40, "%.8x t_dfacls   (%d)",
		    node->ext.acl.t_dfacls, node->ext.acl.t_dfacls);
		Mvprintw(ln++, 40, "%.8x n_acls     (%d)",
		    node->ext.acl.n_acls, node->ext.acl.n_acls);
		Mvprintw(ln++, 40, "%.8x n_dfacls   (%d)",
		    node->ext.acl.n_dfacls, node->ext.acl.n_dfacls);

		fl = ln = (ll > ln) ? ll : ln;
		for (i = 0; i < MAX_ACL_ENTS_IN_INO; i++) {
			if (i == (node->ext.acl.n_acls +
			    node->ext.acl.n_dfacls))
				break;

			if ((i % 4) == 0) {
				fl = ++ln;
				x = 0;
			} else {
				ln = fl;
				x += 20;
			}
			if (node->ext.acl.ent[i].a_type & ACL_DEFAULT) {
				if (node->ext.acl.ent[i].a_type &
				    USER_OBJ) {
					atype = "DEF USR OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    USER) {
					atype = "DEF USER";
				} else if (node->ext.acl.ent[i].a_type &
				    GROUP_OBJ) {
					atype = "DEF GRP OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    GROUP) {
					atype = "DEF GROUP";
				} else if (node->ext.acl.ent[i].a_type &
				    CLASS_OBJ) {
					atype = "DEF CLS OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    OTHER_OBJ) {
					atype = "DEF OTH OBJ";
				} else {
					atype = "DEF UNKNOWN";
				}
			} else {
				if (node->ext.acl.ent[i].a_type &
				    USER_OBJ) {
					atype = "USER OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    USER) {
					atype = "USER";
				} else if (node->ext.acl.ent[i].a_type &
				    GROUP_OBJ) {
					atype = "GROUP OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    GROUP) {
					atype = "GROUP";
				} else if (node->ext.acl.ent[i].a_type &
				    CLASS_OBJ) {
					atype = "CLASS OBJ";
				} else if (node->ext.acl.ent[i].a_type &
				    OTHER_OBJ) {
					atype = "OTHER OBJ";
				} else {
					atype = "UNKNOWN";
				}
			}
			Mvprintw(ln++, x, "%.8x %.11s",
			    node->ext.acl.ent[i].a_type, atype);
			if (node->ext.acl.ent[i].a_type &
			    (GROUP | GROUP_OBJ)) {
				Mvprintw(ln++, x, "%.8x %.11s",
				    node->ext.acl.ent[i].a_id,
				    getgroup(node->ext.acl.ent[i].a_id));
			} else if (node->ext.acl.ent[i].a_type &
			    (USER | USER_OBJ)) {
				Mvprintw(ln++, x, "%.8x %.11s",
				    node->ext.acl.ent[i].a_id,
				    getuser(node->ext.acl.ent[i].a_id));
			} else {
				Mvprintw(ln++, x, "%.8x",
				    node->ext.acl.ent[i].a_id);
			}
			aperm = sam_perm_str(node->ext.acl.ent[i].a_perm, NULL);
			Mvprintw(ln++, x, "%.8x %.11s",
			    node->ext.acl.ent[i].a_perm, aperm);
		}
		break;
	case S_IFOBJ:
		ln = fl;
		for (i = 0; i < SAM_OSD_EXTENT; i++) {
			Mvprintw(ln++, 40, "%.16llx.%.4x object_id.ord",
			    node->ext.obj.obj_id[i], node->ext.obj.ord[i]);
		}
		break;
	default:
		break;
	}
}


/*
 * Display inode in archive format.
 * Displays the supplied inode image in archive entry format.
 */
static void
dis_inode_arch(
	struct sam_perm_inode *ino)	/* Inode image */
{
	sam_archive_info_t *arch;	/* Archive entry pointer */
	uint_t *w;
	char *dtnm;			/* Device type mnemonic */
	int fl, i, x;

	Mvprintw(2, 0, "checksum %.8x %.8x %.8x %.8x",
	    ino->csum.csum_val[0], ino->csum.csum_val[1],
	    ino->csum.csum_val[2], ino->csum.csum_val[3]);
	fl = ln;
	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (i < 2)  ln = fl;
		else  ln = 15;
		x = (i & 1) * 40;
		if ((ino->di.arch_status & (1 << i)) == 0)  continue;
		arch = &ino->ar.image[i];
		dtnm = device_to_nm(ino->di.media[i]);
		if (ino->di.version == SAM_INODE_VERS_1) {
			sam_perm_inode_v1_t *ino_v1 =
			    (sam_perm_inode_v1_t *)ino;

			Mvprintw(ln++, x, "%.8x ar_ino     (%d)",
			    ino_v1->aid[i].ino,
			    ino_v1->aid[i].ino);
			Mvprintw(ln++, x, "%.8x ar_gen     (%d)",
			    ino_v1->aid[i].gen,
			    ino_v1->aid[i].gen);
		}
		Mvprintw(ln++, x, "%.8x copy          ", i);
		Mvprintw(ln++, x, "%.8x media      (%s)",
		    ino->di.media[i], dtnm);
		w = (void *)&arch->arch_flags;
		Mvprintw(ln++, x, "%.8x flags/n_vsns", *w);
		Mvprintw(ln++, x, "%.8x version    (%d)", arch->version,
		    arch->version);
		Mvprintw(ln++, x, "%.8x %s", arch->creation_time,
		    ctime((time_t *)&arch->creation_time));
		w = (void *)&arch->vsn;
		Mvprintw(ln++, x, "%.8x %s", *w++, string(arch->vsn));
		Mvprintw(ln++, x, "%.8x vsn", *w++);
		Mvprintw(ln++, x, "%.8x vsn", *w++);
		Mvprintw(ln++, x, "%.8x vsn ...", *w++);
		Mvprintw(ln++, x, "%.8x position", arch->position);
		Mvprintw(ln++, x, "%.8x file_offset", arch->file_offset);
	}
}


/*
 * Display inode in file format.
 * Displays the supplied inode image in file format.
 */
void
dis_inode_file(
	struct sam_perm_inode *permp,
	int extent_factor)		/* 0 for raw extents */
{
	struct sam_disk_inode *ino = (struct sam_disk_inode *)permp;
	char *m_str;		/* File mode string */
	char *a_str;		/* Attribute string */
	char *e_str;		/* Extension attribute string */
	ushort_t *w;
	int fl, ll;		/* Line number */
	char *mm[4];		/* Media types */
	char *extent_format = "raw";
	int extent_shift = 0;
	int i;
	lloff_t *lp;
	uint_t *arp;

	fl = ln;
	m_str = mode_string(ino->mode);
	a_str = sam_attrtoa(ino->status.bits, NULL);
	e_str = sam_attrtoe(ino->ext_attrs, NULL);
	for (i = 0; i < MAX_ARCHIVE; i++) {
		mm[i] = (ino->media[i] == 0) ?
		    "--" : device_to_nm(ino->media[i]);
	}
	Mvprintw(ln++, 0, "%.8x mode       %s",
	    ino->mode, m_str);
	Mvprintw(ln++, 0, "%.8x ino        (%d)",
	    ino->id.ino, ino->id.ino);
	Mvprintw(ln++, 0, "%.8x gen        (%d)",
	    ino->id.gen, ino->id.gen);
	Mvprintw(ln++, 0, "%.8x parent.ino (%d)",
	    ino->parent_id.ino, ino->parent_id.ino);
	Mvprintw(ln++, 0, "%.8x parent.gen (%d)",
	    ino->parent_id.gen, ino->parent_id.gen);
	lp = (lloff_t *)&ino->rm.size;
	Mvprintw(ln++, 0, "%.8x size_u", lp->_p._u);
	Mvprintw(ln++, 0, "%.8x size_l     (%llu)",
	    lp->_p._l, lp->_f);
	w = (ushort_t *)&ino->rm.media;
	Mvprintw(ln++, 0, "%.4x%.4x rm:media/flags",
	    ino->rm.media, *++w);
	Mvprintw(ln++, 0, "%.8x rm:file_offset",
	    ino->rm.info.rm.file_offset);
	Mvprintw(ln++, 0, "%.8x rm:mau", ino->rm.info.rm.mau);
	Mvprintw(ln++, 0, "%.8x rm:position", ino->rm.info.rm.position);
	Mvprintw(ln++, 0, "%.8x ext_attrs  %s", ino->ext_attrs, e_str);
	Mvprintw(ln++, 0, "%.8x ext.ino    (%d)",
	    ino->ext_id.ino, ino->ext_id.ino);
	Mvprintw(ln++, 0, "%.8x ext.gen    (%d)",
	    ino->ext_id.gen, ino->ext_id.gen);
	Mvprintw(ln++, 0, "%.8x uid        %s",
	    ino->uid, getuser(ino->uid));
	Mvprintw(ln++, 0, "%.8x gid        %s",
	    ino->gid, getgroup(ino->gid));
	Mvprintw(ln++, 0, "%.8x nlink      (%d)",
	    ino->nlink, ino->nlink);
	Mvprintw(ln++, 0, "%.8x status %s",
	    ino->status.bits, a_str);
	Mvprintw(ln++, 0, "%.2x%.2x%.2x%.2x obty/-/-/p2flg",
	    permp->di2.objtype, 0, 0, permp->di2.p2flags);
	Mvprintw(ln++, 0, "%.8x projid", permp->di2.projid);
	if (ino->status.b.segment) {
		Mvprintw(ln++, 0, "%.8x seg size   (%d)",
		    ino->rm.info.dk.seg_size, ino->rm.info.dk.seg_size);
	}
	if (S_ISSEGI(ino)) {
		Mvprintw(ln++, 0, "%.8x fsize      (%d)",
		    ino->rm.info.dk.seg.fsize, ino->rm.info.dk.seg.fsize);
	} else if (S_ISSEGS(ino)) {
		Mvprintw(ln++, 0, "%.8x seg ord    (%d)",
		    ino->rm.info.dk.seg.ord, ino->rm.info.dk.seg.ord);
	}

	ll = ln;
	ln = fl;
	Mvprintw(ln++, 40, "%.8x access_time", ino->access_time.tv_sec);
	Mvprintw(ln++, 40, "%.8x", ino->access_time.tv_nsec);
	Mvprintw(ln++, 40, "%.8x modify_time", ino->modify_time.tv_sec);
	Mvprintw(ln++, 40, "%.8x", ino->modify_time.tv_nsec);
	Mvprintw(ln++, 40, "%.8x change_time", ino->change_time.tv_sec);
	Mvprintw(ln++, 40, "%.8x", ino->change_time.tv_nsec);
	Mvprintw(ln++, 40, "%.8x creation_time", ino->creation_time);
	Mvprintw(ln++, 40, "%.8x attribute_time", ino->attribute_time);
	Mvprintw(ln++, 40, "%.8x residence_time", ino->residence_time);
	Mvprintw(ln++, 40, "%.2x%.2x%.2x%.2x unit/cs/arch/flg",
	    ino->unit, ino->cs_algo, ino->arch_status, ino->lextent);

	arp = (void *)&ino->ar_flags[0];
	Mvprintw(ln++, 40, "%.8x ar_flags", *arp);
	Mvprintw(ln++, 40, "%.2x%.2x%.2x00 stripe/stride/sg",
	    ino->stripe, ino->stride, ino->stripe_group);
	Mvprintw(ln++, 40, "%.4x%.4x media  %s %s",
	    ino->media[0], ino->media[1], mm[0], mm[1]);
	Mvprintw(ln++, 40, "%.4x%.4x media  %s %s",
	    ino->media[2], ino->media[3], mm[2], mm[3]);
	Mvprintw(ln++, 40, "%.8x psize      (%d)",
	    ino->psize.partial, ino->psize.partial);
	Mvprintw(ln++, 40, "%.8x blocks     (%d)",
	    ino->blocks, ino->blocks);
	Mvprintw(ln++, 40, "%.8x free_ino   (%d)",
	    ino->free_ino, ino->free_ino);
	Mvprintw(ln++, 40, "%.8x stage ahd  (%d)",
	    ino->stage_ahead, ino->stage_ahead);
	Mvprintw(ln++, 40, "%.8x xattr.ino    (%d)",
	    permp->di2.xattr_id.ino, permp->di2.xattr_id.ino);
	Mvprintw(ln++, 40, "%.8x xattr.gen    (%d)",
	    permp->di2.xattr_id.gen, permp->di2.xattr_id.gen);
	ln = (ll > ln) ? ll : ln;
	if (COLS > 24)  ln++;
	if (ino->rm.ui.flags & RM_OBJECT_FILE) {
		sam_di_osd_t *oip = (sam_di_osd_t *)&ino->extent[0];

		Mvprintw(ln++, 0, "Object IDs  ext_id: %d.%d", oip->ext_id.ino,
		    oip->ext_id.gen);
		for (i = 0; i < SAM_OSD_DIRECT; i++) {
			if (i % 3 == 0) {
				Mvprintw(ln++, 0, "%.2d_ ", i);
			}
			Printw("%.16llx.%.2x ",
			    oip->obj_id[i], oip->ord[i]);
		}
		return;
	}
	if (extent_factor != 0) {
		if (extent_factor == 1024) {
			extent_format = "1k";
		} else {
			extent_format = "4k displayed as 1k";
			extent_shift = SAM_SHIFT - SAM_DEV_BSHIFT;
		}
	}
	Mvprintw(ln++, 0, "Extents (%s):", extent_format);
						/* Display extents */
	for (i = 0; i < NOEXT; i++) {
		if (i % 6 == 0)  Mvprintw(ln++, 0, "%.2d_ ", i);
		Printw("%.8x.%.2x ",
		    (ino->extent[i] << extent_shift), ino->extent_ord[i]);
	}
}


/*
 *	Return attributes in ASCII from extension attribute field.
 *	String returned.
 */
static char *
sam_attrtoe(
	int attr,	/* Attributes field */
	char *string)	/* If not NULL, place string here */
{
	static char s[24];

	/* Inode extension types */
	if (string == NULL)  string = s;
	string[0] = (attr & ext_sln) ? 'S' : '-';
	string[1] = (attr & ext_rfa) ? 'R' : '-';
	string[2] = (attr & ext_hlp) ? 'H' : '-';
	string[3] = (attr & ext_acl) ? 'A' : '-';
	string[4] = (attr & ext_mva) ? 'M' : '-';
	string[5] = '-';
	string[6] = '-';
	string[7] = '-';
	string[8] = '\0';

	return (string);
}

/*
 *	Return attributes in ASCII from a permission field.
 *	String returned.
 */
static char *
sam_perm_str(
	int perm,	/* Permission field */
	char *string)	/* If not NULL, place string here */
{
	static char s[24];

	/*
	 * Permission values
	 *
	 *	r	the file is readable
	 *	w	the file is writable
	 *	x	the file is executable
	 *	-	the indicated permission is not granted
	 *	s	the set-user-ID or set-group-ID bit is on, and  the
	 *		corresponding  user  or group execution bit is also on
	 *	S	undefined bit-state (the set-user-ID bit is on  and
	 *		the user execution bit is off)
	 *	t	the 10 (octal) bit, or sticky  bit,  is  on  (see
	 *		chmod(1)), and execution is on
	 *	T	the 10 bit is turned on,  and  execution  is  off
	 *		(undefined bit-state)
	 */

	if (string == NULL)  string = s;
	string[0] = (perm & 8) ? '?' : '-';
	string[1] = (perm & 4) ? 'r' : '-';
	string[2] = (perm & 2) ? 'w' : '-';
	string[3] = (perm & 1) ? 'x' : '-';
	string[4] = '\0';

	return (string);
}
