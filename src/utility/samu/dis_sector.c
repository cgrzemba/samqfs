/*
 * dis_sector.c - Display sector data.
 *
 * Display sector.
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

#pragma ident "$Revision: 1.31 $"


/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "aml/device.h"
#include "aml/odlabels.h"
#include "sam/mount.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#include "samu.h"

/* Types. */
enum Sdis { Sraw, Sinode, Sarchive, Slabel, Ssblk, Sauto, Smax };

extern char *getfullrawname();

/* Private functions. */
static void dis_sblk(struct sam_sblk *sblock);
static void closedisk(void);

/* Private data. */
static struct MemPars mp;

static enum Sdis fmt = Sauto;	/* Display format */
static equ_t eq;		/* Equipment ordinal */
static dtype_t type;		/* Disk device type */
static boolean empty;		/* Empty sector flag */
static char *SdisNames[] = {
	"raw", "inode", "archive", "label", "sblk", "auto" };
static char *DSname = NULL;
static uchar_t *buffer = NULL;	/* Sector buffer address */
static uchar_t *lbl_buf = NULL;	/* Label buffer address */
static int disk;		/* Disk descriptor */
static int ino;			/* Sector inode ordinal */
static long ptocfwa;	/* Start of PTOC (optical) */
static long ptoclwa;	/* End of PTOC (optical) */
static offset_t addr;	/* Sector address */


/*
 * Display sector.
 */
void
DisSector(void)
{
	struct sam_sblk *sb;	/* Superblock pointer */

	if (eq == 0)
		return;
	/* LINTED pointer cast may result in improper alignment */
	sb = (struct sam_sblk *)buffer;
	if (fmt == Sauto) {	/* Determine format */

		if ((type & DT_CLASS_MASK) == DT_OPTICAL)  fmt = Sraw;
		if ((type & DT_CLASS_MASK) == DT_TAPE)  fmt = Sraw;
		if (fmt == Sauto && strncmp(sb->info.sb.name, "SBLK", 4) == 0) {
			fmt = Ssblk;
		}
		if (fmt == Sauto)  fmt = Sraw;
	}
	Mvprintw(0, 0, "Sector:   %.8llx (%lld)", addr, addr);
	if (empty)  Mvprintw(0, 24, "EMPTY");
	Mvprintw(0, 30, "%s", SdisNames[fmt]);

	Mvprintw(1, 10, "%s", DSname);

	if (fmt == Sinode)  Mvprintw(1, 60, "ordinal:%d", ino);

	switch (fmt) {

	case Sinode:	/* File inode */
		/* LINTED pointer cast may result in improper alignment */
		dis_inode_file((struct sam_disk_inode *)buffer, 0);
		break;

	case Ssblk:			/* Superblock */
		dis_sblk(sb);
		break;

	case Sraw:			/* Raw */
		mp.base = buffer;
		DisMem(&mp, 2);
		break;

	case Sauto:
	case Sarchive:
	case Smax:
		break;

	default:
		break;
	}
}


/*
 * Keyboard processing.
 */
boolean
KeySector(char key)
{
	if (eq == 0)
		return (FALSE);
	switch (key) {

	case KEY_full_fwd:
		addr++;
		(void) readdisk(addr);
		break;

	case KEY_full_bwd:
		if (--addr < 0)  addr = 0;
		(void) readdisk(addr);
		break;

	case KEY_half_fwd:
		if (fmt == Sraw)
			return (KeyMem(&mp, KEY_full_fwd));
		if (fmt == Sinode || fmt == Sarchive) {
			if (++ino >= 4)  ino = 0;
		}
		break;

	case KEY_half_bwd:
		if (fmt == Sraw)
			return (KeyMem(&mp, KEY_full_bwd));
		if (fmt == Sinode || fmt == Sarchive) {
			if (--ino < 0)  ino = 0;
		}
		break;

	case KEY_adv_fmt:
		if (++fmt >= Sauto)  fmt = 0;
		break;

	default:
		if (fmt == Sraw)
			return (KeyMem(&mp, key));
		return (FALSE);
		/* NOTREACHED */
		break;
	}
	return (TRUE);
}


#if defined(Obsolete)
	if (fmt == Sauto) {
		struct sam_sblk *sb;		/* Superblock pointer */

		sb = (struct sam_sblk *)buffer;
		if ((type & DT_CLASS_MASK) == DT_OPTICAL)  fmt = Sraw;
		if (fmt == Sauto && strncmp(sb->info.sb.name, "SBLK", 4) == 0) {
			fmt = Ssblk;
		}
		if (fmt == Sauto) fmt = Sraw;
	}
#endif /* defined(Obsolete) */


/*
 * Initialize display.
 */
boolean
InitSector(void)
{
	if (Argc > 1) {
		offset_t n = 0;

		n = strtoll(Argv[1], NULL, 16);
		addr = n;
	}
	(void) readdisk(addr);
	return (TRUE);
}


/*
 * Display superblock.
 */
static void
dis_sblk(
	struct sam_sblk *sblock)	/* Pointer to super-block image */
{
	int ll;		/* Line numbers */
	int i, *w;
	struct sam_sbinfo *sp;

	sp = (struct sam_sbinfo *)sblock;
	/* LINTED pointer cast may result in improper alignment */
	w = (int *)sp->name;
	Mvprintw(ln++, 0, "%.8x name: %s", *w, string(sp->name));
	/* LINTED pointer cast may result in improper alignment */
	w = (int *)sp->fs_name;
	Mvprintw(ln++, 0, "%.8x fs_name: %s", *w, string(sp->fs_name));
	Mvprintw(ln++, 0, "%.8x magic", sp->magic);
	Mvprintw(ln++, 0, "%.8x init: %s", sp->init,
	    ctime((time_t *)&sp->init));
	Mvprintw(ln++, 0, "%.8x time: %s", sp->time,
	    ctime((time_t *)&sp->time));
	Mvprintw(ln++, 0, "%.8x fsck: %s", sp->repaired,
	    ctime((time_t *)&sp->repaired));
	Mvprintw(ln++, 0, "%.8x ord", sp->ord);
	ll = ln;
	Mvprintw(ln++, 0, "%.8x inodes", (int)sp->inodes);
	Mvprintw(ln++, 0, "%.8x offset[0]", (int)sp->offset[0]);
	Mvprintw(ln++, 0, "%.8x offset[1]", (int)sp->offset[1]);
	Mvprintw(ln++, 0, "%.8x capacity", (int)sp->capacity);
	Mvprintw(ln++, 0, "%.8x space", (int)sp->space);
	Mvprintw(ln++, 0, "%.8x mm_capacity", (int)sp->mm_capacity);
	Mvprintw(ln++, 0, "%.8x mm_space", (int)sp->mm_space);
	Mvprintw(ln++, 0, "%.8x hosts", (int)sp->hosts);
	Mvprintw(ln++, 0, "%.8x ext_bshift", (int)sp->ext_bshift);
	Mvprintw(ln++, 0, "%.8x min_usr_inum", (int)sp->min_usr_inum);
	Mvprintw(ln++, 0, "%.4x     opt_mask_ver", (int)sp->opt_mask_ver);
	Mvprintw(ln++, 0, "%.8x opt_mask", (int)sp->opt_mask);
	Mvprintw(ln++, 0, "%.8x opt_features", (int)sp->opt_features);

	Mvprintw(ll++, 40, "%.8x sblk_size", sp->sblk_size);
	Mvprintw(ll++, 40, "%.4x     dau_blks[SM]", sp->dau_blks[SM]);
	Mvprintw(ll++, 40, "    %.4x dau_blks[LG]", sp->dau_blks[LG]);
	Mvprintw(ll++, 40, "%.4x     meta_blks", sp->meta_blks);
	Mvprintw(ll++, 40, "%.4x     mm_blks[SM]", sp->mm_blks[SM]);
	Mvprintw(ll++, 40, "    %.4x mm_blks[LG]", sp->mm_blks[LG]);
	Mvprintw(ll++, 40, "%.8x fs_count", sp->fs_count);
	Mvprintw(ll++, 40, "%.4x     da_count", sp->da_count);
	Mvprintw(ll++, 40, "    %.4x mm_count", sp->mm_count);
	Mvprintw(ll++, 40, "%.8x mm_ord", sp->mm_ord);
	Mvprintw(ll++, 40, "%.8x gen", sp->gen);
	ln++;
	Mvprintw(ln++, 0, "                            ");
	Printw("       allocmap");
	Mvprintw(ln++, 0, "ty   eq capacity  space    ");
	Printw("mmord start length dau_next system fst st");
	if (strncmp(sp->name, "SBLK", 4) != 0)
		return;
	for (i = 0; i < sp->fs_count; i++) {
		Mvprintw(ln++, 0,
		    "%.5s%5d %.8x  %.8x  %.3d  %.5x  "
		    "%.5x %.8x  %.5x  %.2x %s",
		    device_to_nm(sblock->eq[i].fs.type),
		    sblock->eq[i].fs.eq,
		    (int)sblock->eq[i].fs.capacity,
		    (int)sblock->eq[i].fs.space,
		    (int)sblock->eq[i].fs.mm_ord,
		    (int)sblock->eq[i].fs.allocmap,
		    (int)sblock->eq[i].fs.l_allocmap,
		    (int)sblock->eq[i].fs.dau_next,
		    (int)sblock->eq[i].fs.system,
		    (int)sblock->eq[i].fs.fsck_stat,
		    dev_state[sblock->eq[i].fs.state]);
		if (ln > LINES - 2)  break;
	}
}


/*
 * Close disk device.
 */
static void
closedisk()
{
	if (eq == 0)
		return;
	close(disk);
	if (buffer != NULL)  free(buffer);
	buffer = NULL;
	if (lbl_buf != NULL)  free(lbl_buf);
	lbl_buf = NULL;
	eq = 0;
	type = 0;
	ptocfwa = 0;
	ptoclwa = 0;
}


/*
 * Open magnetic/optical disk.
 * Set disk read parameters.
 *	disk    = Disk file descriptor.
 *	ptocfwa = First sector of PTOC.
 *	ptoclwa = Last  sector of PTOC.
 */
boolean
opendisk(
	char *name)		/* Device name or equipment number */
{
	dev_ent_t *dev;		/* Device table entry address */
	int d, e;
	char *devrname;
	struct sam_fs_part *fp = NULL;

	if (eq != 0)  closedisk();

	/*
	 *	First, try to find the device as a hard disk.
	 */
	if ((findfsp(name, &fp)) != NULL) {

		if (!is_disk(fp->pt_type)) {
			free(fp);
			Error(catgets(catfd, SET, 875,
			    "Device not found (%s)"), name);
		}
		if ((devrname = getfullrawname(fp->pt_name)) == NULL) {
			Error(catgets(catfd, SET, 1606,
			    "malloc: %s"), "getfullrawname");
		}
		if ((d = open(devrname, O_RDONLY)) < 0) {
			free(fp);
			free(devrname);
			Error("open(%s)", devrname);
		}
		if ((buffer = (uchar_t *)malloc(1024)) == NULL) {
			free(fp);
			free(devrname);
			closedisk();
			Error(catgets(catfd, SET, 1652,
			    "Memory exhausted"), 0);
		}
		disk = d;
		eq = fp->pt_eq;
		type = fp->pt_type;
		DSname = fp->pt_name;
		ptocfwa = 0;
		ptoclwa = 0;
		addr = 0;
		mp.size = 1024;
		free(fp);
		free(devrname);
		return (TRUE);
	}

	/*
	 * It must be an optical drive or tape drive - need shm for this.
	 */
	if ((e = finddev(name)) < 0)
		Error(catgets(catfd, SET, 875, "Device not found (%s)"), name);

	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[e]);
	if (!(IS_TAPE(dev) || IS_OPTICAL(dev))) {
		Error(catgets(catfd, SET, 865, "Device cannot be opened (%s)"),
		    dev->name);
	}
	strcpy(devrname, dev->name);
	if ((d = open(devrname, O_RDONLY)) < 0) {
		Error("open(%s)", devrname);
	}
	if IS_TAPE(dev) {
		/* kludge - tape sector_size is not filled in if unavail */
		mp.size = dev->dt.tp.default_blocksize;
	} else {
		mp.size = dev->sector_size;
	}
	if ((buffer = (uchar_t *)malloc(mp.size)) == NULL) {
		closedisk();
		Error(catgets(catfd, SET, 1652, "Memory exhausted"), 0);
	}
	disk = d;
	eq = (equ_t)e;
	DSname = devrname;
	type = dev->type;
	if ((type & DT_CLASS_MASK) == DT_OPTICAL) {
		ptocfwa = dev->dt.od.ptoc_fwa;
		ptoclwa = dev->dt.od.ptoc_lwa;
	} else {
		ptocfwa = 0;
		ptoclwa = 0;
	}
	addr = ptocfwa;
	return (TRUE);
}


/*
 * Read magnetic/optical disk.
 */
boolean
readdisk(uint_t sector)
{
	int len;		/* Read length */
	offset_t offset;	/* Disk byte address */

	if (eq == 0)  Error(catgets(catfd, SET, 1853, "Open command required"));
	empty = TRUE;
	memset(buffer, 0, mp.size);
	addr = sector;
	offset = (offset_t)addr * mp.size;

	if (llseek(disk, offset, SEEK_SET) < 0)  Error("llseek");
	errno = 0;
	len = read(disk, buffer, mp.size);

	empty = (errno == 0 && len == 0);

	if (errno != 0)  Error("read");
	if (empty)  Error(catgets(catfd, SET, 996, "Empty sector"));
	if (len != mp.size)  Error("Read incomplete, len=%x (%d)", len, len);
	return (TRUE);
}


/*
 * Display optical label.
 */
void
DisLabel()
{
	ls_bof1_label_t *l;	/* Label buffer */
	vsn_t next_vsn;		/* Next volume identifier */
	boolean noerr;
	int i, ln;

	if (lbl_buf == NULL) {
		offset_t tmp;

		tmp = addr;
		addr = ptocfwa;
		noerr = readdisk(ptocfwa);
		addr = tmp;
		if (noerr) {
			if ((lbl_buf = (uchar_t *)malloc(1024)) == NULL) {
				closedisk();
				Error(catgets(catfd, SET, 1652,
				    "Memory exhausted"));
			}
			memcpy(lbl_buf, buffer, 1024);
		}
	}
	/* LINTED pointer cast may result in improper alignment */
	l = (ls_bof1_label_t *)lbl_buf;

	Mvprintw(0, 0, "File label: eq: %d  addr %.8llx (%lld)\t%s",
	    eq, addr, addr, (empty) ? "EMPTY" : "");
	Mvprintw(1, 50, "PTOC fwa %-8x lwa %-8x", ptocfwa, ptoclwa);

	if (buffer == NULL)
		return;

	l->resb[0] = l->resc = l->resd = l->rese = l->resf =
	    l->resg = l->resh = l->reso[0] = '\0';
	ln = 3;

	strncpy(next_vsn, l->next_vsn, sizeof (vsn_t)-1);
	next_vsn[sizeof (vsn_t)-1] = '\0';

	Mvprintw(ln, 0, catgets(catfd, SET, 2045, "Recorded File Name : %s"),
	    l->file_id);
	Mvprintw(ln++, 65, "Label : %c%c%c%d",
	    l->label_id[0], l->label_id[1], l->label_id[2], l->label_version);
	Mvprintw(ln, 0, "           Version : %6u", l->version);
	Mvprintw(ln++, 40, "    Security level : %4u", l->security_level);
	Mvprintw(ln++, 0, "     Creation time : %.4u-%.2u-%.2u %.2u:%.2u:%.2u",
	    l->creation_date.lt_year, l->creation_date.lt_mon,
	    l->creation_date.lt_day, l->creation_date.lt_hour,
	    l->creation_date.lt_min, l->creation_date.lt_sec);
	Mvprintw(ln++, 0, "   Expiration time : %.4u-%.2u-%.2u %.2u:%.2u:%.2u",
	    l->expiration_date.lt_year, l->expiration_date.lt_mon,
	    l->expiration_date.lt_day, l->expiration_date.lt_hour,
	    l->expiration_date.lt_min, l->expiration_date.lt_sec);
	Mvprintw(ln++, 0, "          Password : %s", l->password);
	Mvprintw(ln++, 0, "          Owner ID : %s", l->owner_id);
	Mvprintw(ln++, 0, "          Group ID : %s", l->group_id);
	Mvprintw(ln++, 0, "         System ID : %s", l->system_id);
	Mvprintw(ln++, 0, "      Equipment ID : %s", l->equipment_id);
	Mvprintw(ln, 0, "       File start  : %.8lx", l->file_start);
	Mvprintw(ln++, 40, " File sequence no. : %6u", l->fsn);
	Mvprintw(ln, 0, "       File size   : %8lu pdus", l->file_size);
	Mvprintw(ln++, 40, "     File contents : %4u", l->file_contents);
	if (l->byte_length_u == 0) {
		Mvprintw(ln, 0, "       File length : %8lu bytes",
		    l->byte_length);
	} else {
		Mvprintw(ln, 0, "       File length : %2lu%08lu bytes",
		    l->byte_length_u, l->byte_length);
	}
	Mvprintw(ln++, 40, "  Append iteration : %4u", l->append_iteration);
	Mvprintw(ln, 0, "       Record type : %4u", l->record_type);
	Mvprintw(ln++, 40, "     Record length : %8lu", l->record_length);
	Mvprintw(ln, 0, "       Block  type : %4u", l->block_type);
	Mvprintw(ln++, 40, "     Block  length : %8lu", l->block_length);
	Mvprintw(ln, 0, "  Recording method : %4u", l->recording_method);
	Mvprintw(ln++, 40, "   Record delimiter: %4o",
	    l->record_delimiting_char);
	Mvprintw(ln, 0, "         Next VSN : %s", next_vsn);

	ln++;
	Mvprintw(ln++, 0, "  User information :");

	move(ln++, 0);
	for (i = 0; i < 80; i++) {
		Printw("%c", isprint(l->user_information[i]) ?
		    l->user_information[i] : '.');
	}
	move(ln++, 0);
	while (i < 160) {
		Printw("%c", isprint(l->user_information[i]) ?
		    l->user_information[i] : '.');
		i++;
	}
}


/*
 * Advance label display.
 */
boolean
KeyLabel(char key)
{
	if (eq == 0)
		return (FALSE);
	switch (key) {

	case KEY_full_fwd:
		if (++addr >= ptoclwa)  addr = ptoclwa - 1;
		break;

	case KEY_full_bwd:
		if (--addr < ptocfwa)  addr = ptocfwa;
		break;

	default:
		return (FALSE);
		/* NOTREACHED */
		break;

	}
	(void) readdisk(addr);
	memcpy(lbl_buf, buffer, 1024);
	return (TRUE);
}
