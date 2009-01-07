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

#pragma ident "$Revision: 1.52 $"

/* Feature test switches. */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/vfs.h>
#ifdef sun
#include <sys/dkio.h>
#include <sys/vtoc.h>
#else
#include <string.h>
#endif /* sun */

#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/param.h"
#include "pub/devstat.h"
#include "sam/fs/sblk.h"
#include "sam/fs/samhost.h"
#define	DEC_INIT
#include "sam/devnm.h"

#ifdef sun
#include <sys/uuid.h>
#include <sys/efi_partition.h>
#include "efilabel.h"
#endif /* sun */

extern int byte_swap_sb(struct sam_sblk *sblk, size_t len);
extern int byte_swap_hb(struct sam_host_table_blk *htp);

#define		MAXDEV		5000

#define		DI_SBLK_BSWAPPED		0x01

struct DevInfo {
	int di_flags;
	char *di_devname;
	struct sam_sblk *di_sblk;
	struct vtoc	*di_vtoc;
#ifdef sun
	struct dk_gpt *di_efi;
#endif /* sun */
	struct dk_cinfo *di_dkip;
	struct sam_host_table *di_hosts;
	int di_hosts_table_size;
} Devs[MAXDEV];
int NDevs;

static void CheckDev(char *);
static void DumpDevs(void);
static void SortDevs(void);
static void ListDevs(void);
static void ListHosts(struct DevInfo *, int);
static void ListFSet(struct DevInfo *, int);
static void TypeStr(dtype_t, char *buf);
static void FSStr(uname_t, char *buf);

int	debug = 0;
int shared = 0;
int verbose = 0;
int blocks = 0;

static void SamPrintSB(struct sam_sblk *, char *);
static void SamPrintVTOC(struct vtoc *, char *);
static void SamPrintDKIOC(struct dk_cinfo *, char *);
#ifdef sun
static void SamPrintUUID(struct uuid *);
static void SamPrintEFI(struct dk_gpt *, char *);
extern char *getfullrawname();
#endif /* sun */

static int Dsk2Rdsk(char *dsk, char *rdsk);


/*
 * Search through a list of files/devices provided on the command
 * line, looking for whole QFS filesystems.  We look for
 * superblocks in the usual place, saving any info found.  We
 * then sort the superblocks (not by family set name, but by
 * creation timestamp and FS ordinal), and print out stuff.
 */
int
main(int ac, char *av[])
{
	int c;

	CustmsgInit(0, NULL);

	if (geteuid()) {
		/* You must be root to run %s\n */
		fprintf(stderr, GetCustMsg(13244), av[0]);
	}

	while ((c = getopt(ac, av, "bdhsv")) != EOF) {
		switch (c) {
		case 'b':
			blocks = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			/* Usage: %s %s\n */
			printf(GetCustMsg(13001), av[0], "[-bdhsv] dev ...");
			exit(0);
			/*NOTREACHED*/
		case 's':
			shared = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			/* %s: Unknown option.\n */
			printf(GetCustMsg(13243), av[0]);
			exit(1);
		}
	}

	if (optind == ac) {
		/* %s:  No devices provided for scan.\n */
		fprintf(stderr, GetCustMsg(13221), av[0]);
		exit(1);
	}

	for (; optind < ac; optind++)
		CheckDev(av[optind]);

	SortDevs();
	DumpDevs();
	ListDevs();
	return (0);
}

/*
 * Try to:
 *  (1) open the device;
 *  (2) fstat the opened device (to verify that it's a device)
 *  (3) seek to the superblock offset;
 *  (4) read the superblock;
 *  (5) check that the SBLK and magic numbers are present;
 *  (6) if it's a char dev, and debug is on, get the dev geometry and VTOC; and
 *  (7) stash all the info.
 *
 * If any of this fails (except geom/VTOC calls) we return, saving no info.
 */
static void
CheckDev(char *dev)
{
	int fd;
	struct stat sbuf;
	struct sam_sblk sblk;
#ifdef sun
	struct dk_cinfo dkc;
	struct vtoc vt;
	struct dk_gpt *efi_vtoc;
#endif /* sun */
	static struct sam_host_table_blk *htp = NULL;
	int hosts_table_size;
	char devrname[MAXPATHLEN];

	if (!Dsk2Rdsk(dev, devrname)) {
		return;
	}
	if ((fd = open(devrname, O_RDONLY)) < 0) {
		if (verbose) {
			/* Couldn't open '%s' */
			error(0, errno, GetCustMsg(13222), dev);
		}
		return;
	}
	if (fstat(fd, &sbuf) < 0) {
		if (verbose) {
			/* fstat of '%s' failed */
			error(0, errno, GetCustMsg(13223), dev);
		}
		goto ret;
	}
	if (!S_ISBLK(sbuf.st_mode) && !S_ISCHR(sbuf.st_mode)) {
		if (verbose) {
			/* '%s' is not a block or char special. */
			error(0, 0, GetCustMsg(13224), dev);
		}
		goto ret;
	}

	if (htp == NULL) {
		htp = malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		if (htp == NULL) {
			error(0, 0, GetCustMsg(13229));
			exit(10);
		}
	}

	if (llseek(fd, SUPERBLK*SAM_DEV_BSIZE, SEEK_SET) == (offset_t)-1) {
		if (verbose) {
			/* Could not seek on device '%s' */
			error(0, errno, GetCustMsg(13225), dev);
		}
		goto ret;
	}
	if (read(fd, (char *)&sblk, sizeof (sblk)) != sizeof (sblk)) {
		if (verbose) {
			/* Could not read from device '%s' */
			error(0, errno, GetCustMsg(13226), dev);
		}
		goto ret;
	}

	if (strncmp((char *)&sblk.info.sb.name[0], "SBLK", 4) != 0) {
		if (verbose) {
			/*
			 * Device '%s' doesn't have a QFS superblock
			 * (SBLK).\n
			 */
			fprintf(stderr, GetCustMsg(13227), dev);
		}
		goto ret;
	}
	switch (sblk.info.sb.magic) {

	case SAM_MAGIC_V1:
	case SAM_MAGIC_V2:
	case SAM_MAGIC_V2A:
		break;

	case SAM_MAGIC_V1_RE:
	case SAM_MAGIC_V2_RE:
	case SAM_MAGIC_V2A_RE:
		if (byte_swap_sb(&sblk, sizeof (sblk))) {
			if (verbose) {
				/*
				 * Device '%s' doesn't have a QFS
				 * superblock (MAGIC).\n
				 */
				fprintf(stderr, GetCustMsg(13228), dev);
			}
			goto ret;
		}
		Devs[NDevs].di_flags |= DI_SBLK_BSWAPPED;
		break;

	default:
		if (verbose) {
			/*
			 * Device '%s' doesn't have a QFS superblock
			 * (MAGIC).\n
			 */
			fprintf(stderr, GetCustMsg(13228), dev);
		}
		goto ret;
	}

	if (sblk.info.sb.opt_mask & SBLK_OPTV1_LG_HOSTS) {
		hosts_table_size = SAM_LARGE_HOSTS_TABLE_SIZE;
	} else {
		hosts_table_size = SAM_HOSTS_TABLE_SIZE;
	}

	if (shared && (sblk.info.sb.magic == SAM_MAGIC_V2 ||
	    sblk.info.sb.magic == SAM_MAGIC_V2A) &&
	    sblk.info.sb.ord == sblk.info.sb.hosts_ord &&
	    sblk.info.sb.hosts != 0) {
		if (llseek(fd, sblk.info.sb.hosts*SAM_DEV_BSIZE, SEEK_SET)
		    == (offset_t)-1) {
			if (verbose) {
				/* Could not seek on device '%s' */
				error(0, errno, GetCustMsg(13225), dev);
			}
			return;
		}
		if (read(fd, (char *)htp, hosts_table_size)
		    != hosts_table_size) {
			if (verbose) {
				/* Could not read from device '%s' */
				error(0, errno, GetCustMsg(13226), dev);
			}
			return;
		}
		if (Devs[NDevs].di_flags & DI_SBLK_BSWAPPED) {
			if (byte_swap_hb(htp)) {
				/* Invalid hosts file on device '%s'. */
				error(0, 0, GetCustMsg(13800), dev);
				return;
			}
		}
		Devs[NDevs].di_hosts =
		    (struct sam_host_table *)malloc(hosts_table_size);
		if (!Devs[NDevs].di_hosts) {
			/* host table malloc failed. */
			error(0, 0, GetCustMsg(13229));
			exit(10);
		}
		bcopy((char *)htp, (char *)Devs[NDevs].di_hosts,
		    hosts_table_size);
		Devs[NDevs].di_hosts_table_size = hosts_table_size;
	} else {
		Devs[NDevs].di_hosts = NULL;
	}

#ifdef sun
	if (debug && S_ISCHR(sbuf.st_mode)) {
		if (ioctl(fd, DKIOCINFO, &dkc) >= 0) {
			Devs[NDevs].di_dkip =
			    (struct dk_cinfo *)malloc(sizeof (dkc));
			if (!Devs[NDevs].di_dkip) {
				/* DKIOC malloc failed. */
				error(0, 0, GetCustMsg(13230));
				exit(10);
			}
			bcopy((char *)&dkc, (char *)Devs[NDevs].di_dkip,
			    sizeof (dkc));
		} else {
			if (verbose) {
				/* Ioctl DKIOCINFO of '%s' failed */
				error(0, errno, GetCustMsg(13231));
			}
			Devs[NDevs].di_dkip = NULL;
		}
	} else {
		Devs[NDevs].di_dkip = NULL;
	}
#else /* sun */

	Devs[NDevs].di_dkip = NULL;

#endif /* sun */

	Devs[NDevs].di_vtoc = NULL;

#ifdef sun
	Devs[NDevs].di_efi = NULL;
	if (debug && S_ISCHR(sbuf.st_mode)) {
		if (ioctl(fd, DKIOCGVTOC, &vt) >= 0) {
			Devs[NDevs].di_vtoc =
			    (struct vtoc *)malloc(sizeof (vt));
			if (!Devs[NDevs].di_vtoc) {
				/* VTOC malloc failed. */
				error(0, 0, GetCustMsg(13232));
				exit(10);
			}
			bcopy((char *)&vt, (char *)Devs[NDevs].di_vtoc,
			    sizeof (vt));
		} else if (is_efi_present() &&
		    ((*call_efi_alloc_and_read)(fd, &efi_vtoc) >= 0)) {
			Devs[NDevs].di_efi = efi_vtoc;
		} else {
			if (verbose) {
				/* Could not get VTOC of '%s' */
				error(0, errno, GetCustMsg(13233), dev);
			}
		}
	}
#endif /* sun */

	if (verbose) {
		if (Devs[NDevs].di_flags & DI_SBLK_BSWAPPED) {
			/* Device '%s' has a (byte-reversed) QFS superblock. */
			printf(GetCustMsg(13801), dev);
		} else {
			/* Device '%s' has a QFS superblock. */
			printf(GetCustMsg(13234), dev);
		}
	}

	Devs[NDevs].di_devname = (char *)malloc(strlen(dev)+1);
	Devs[NDevs].di_sblk = (struct sam_sblk *)malloc(sizeof (sblk));
	if (!Devs[NDevs].di_devname || !Devs[NDevs].di_sblk) {
		/* Devname or SBLK malloc failed. */
		error(0, 0, GetCustMsg(13235));
		exit(10);
	}
	strcpy(Devs[NDevs].di_devname, dev);
	bcopy((char *)&sblk, (char *)Devs[NDevs].di_sblk, sizeof (sblk));
	++NDevs;

ret:
	(void) close(fd);
}

static void
DumpDevs(void)
{
	int i;

	if (!debug) {
		return;
	}

	for (i = 0; i < NDevs; i++) {
		SamPrintSB(Devs[i].di_sblk, Devs[i].di_devname);
		if (Devs[i].di_dkip) {
			SamPrintDKIOC(Devs[i].di_dkip, Devs[i].di_devname);
		}
		if (Devs[i].di_vtoc) {
			SamPrintVTOC(Devs[i].di_vtoc, Devs[i].di_devname);
		}
#ifdef sun
		if (Devs[i].di_efi) {
			SamPrintEFI(Devs[i].di_efi, Devs[i].di_devname);
		}
#endif /* sun */
	}
}

static int
timecompare(const void *p1, const void *p2)
{
	struct DevInfo *d1 = (struct DevInfo *)p1, *d2 = (struct DevInfo *)p2;

	return (d1->di_sblk->info.sb.init - d2->di_sblk->info.sb.init);
}

static int
ordcompare(const void *p1, const void *p2)
{
	struct DevInfo *d1 = (struct DevInfo *)p1, *d2 = (struct DevInfo *)p2;

	return (d1->di_sblk->info.sb.ord - d2->di_sblk->info.sb.ord);
}

/*
 * Sort all the super blocks we have.
 * We sort first by the superblock timestamp, so as to put all the
 * superblocks belonging to one FS together.  We sort second by the
 * ordinals, so as to put the devices into order within the FSes.
 * Note, however, that we may have several aliases for any particular
 * super block -- so a later step is to go through and note the
 * incorrect ones.  (E.g., /dev/dsk/c1t0d0s2 and /dev/dsk/c1tod0s0
 * will both be put into the list if they both start at block zero.)
 */
static void
SortDevs(void)
{
	int i, n;

	qsort((void *)&Devs[0], NDevs, sizeof (struct DevInfo), timecompare);

	for (i = 0; i < NDevs; i += n) {
		for (n = 1; i+n < NDevs; n++) {
			if (Devs[i+n].di_sblk->info.sb.init !=
			    Devs[i].di_sblk->info.sb.init) {
				break;
			}
		}
		if (n > 1) {
			qsort((void *)&Devs[i], n, sizeof (struct DevInfo),
			    ordcompare);
		}
	}
}

static void
ListDevs(void)
{
	int i, n;

	if (verbose) {
		/* %d QFS devices found.\n */
		printf(GetCustMsg(13236), NDevs);
	}
	for (i = 0; i < NDevs; i += n) {
		for (n = 1; i+n < NDevs; n++) {
			if (Devs[i+n].di_sblk->info.sb.init !=
			    Devs[i].di_sblk->info.sb.init) {
				break;
			}
		}
		ListHosts(&Devs[i], n);
		ListFSet(&Devs[i], n);
	}
}

/*
 * Dump out a hosts record, if one.
 */
static void
ListHosts(struct DevInfo *rdip, int nd)
{
	int n;
	int i;
	char fsbuf[sizeof (uname_t)+8];
	struct DevInfo *dip;

	if (!shared) {
		return;
	}

	/*
	 * DevInfo arg is for ordinal 0,
	 * the hosts table may not be on
	 * ordinal 0.
	 */
	dip = rdip;
	if (!dip->di_hosts) {
		/*
		 * Look for a device in this
		 * group that has a hosts table.
		 */
		for (i = 0; i < nd && dip->di_hosts == NULL; i++, dip++) {
		}
	}

	if (!dip->di_hosts) {
		return;
	}

	FSStr(dip->di_sblk->info.sb.fs_name, fsbuf);
	n = dip->di_hosts->count;
	if (n < 0 || offsetof(sam_host_table_t, ent[n]) >
	    dip->di_hosts_table_size) {
		/* Host table count out of range (%d).\n */
		fprintf(stderr, GetCustMsg(13237), n);
		return;
	}
	printf("\n#\n# ");

	/* Host file information for family set '%s' */
	printf(GetCustMsg(13802), fsbuf);

	printf("#\n# ");

	/* Version: %d    Generation: %d    Count: %d */
	printf(GetCustMsg(13803),
	    dip->di_hosts->version, dip->di_hosts->gen, dip->di_hosts->count);

	printf("#\n# ");

	/* Length: %d    Server: %d */
	printf(GetCustMsg(13804), dip->di_hosts->length,
	    dip->di_hosts->server);
	printf("#\n");
	fwrite(dip->di_hosts->ent, sizeof (char),
	    dip->di_hosts->length - offsetof(struct sam_host_table, ent[0]),
	    stdout);
}

/*
 * Print out what we've got in a family set.
 * List the time created and the family set name.  If we've got dups
 * in the set, prefix them with "> ".  If we've got holes, comment
 * out the whole set.
 */
static void
ListFSet(struct DevInfo *dip, int n)
{
	int j, holes;
	long when;
	int lfset[L_FSET];
	char typbuf[12];
	char fsbuf[sizeof (uname_t)+8];
	int md, mm, mr, oXXX, gXXX, bad;
	int missing_meta_message = 0;
	int ord;
	int fsgen = 0;
	char *template;

	if (blocks) {
		template = "%s%s%s    %d    %s   %s  %s %lld\n";
	} else {
		template = "%s%s%s    %d    %s   %s  %s\n";
	}
	bzero(&lfset[0], sizeof (lfset));
	FSStr(dip->di_sblk->info.sb.fs_name, fsbuf);
	when = dip->di_sblk->info.sb.init;
	fsgen = dip->di_sblk->info.sb.fsgen;
	for (j = 0; j < n; j++) {
		lfset[dip[j].di_sblk->info.sb.ord]++;
	}
	for (j = 0; j < dip->di_sblk->info.sb.fs_count; j++) {
		if (lfset[j] == 1) {
			if (dip[j].di_sblk->eq[j].fs.state == DEV_ON ||
			    dip[j].di_sblk->eq[j].fs.state == DEV_NOALLOC ||
			    dip[j].di_sblk->eq[j].fs.state == DEV_UNAVAIL) {
				fsgen = dip[j].di_sblk->info.sb.fsgen;
				break;
			}
		}
	}
	printf("#\n# ");
	/* Family Set '%s' Created %s Gen %d*/
	printf(GetCustMsg(13805), fsbuf, fsgen, ctime(&when));
	if (dip->di_flags & DI_SBLK_BSWAPPED) {
		printf("#\n# ");
		/* Foreign byte order (super-blocks byte-reversed). */
		printf(GetCustMsg(13806));
	}
	printf("#\n");
	holes = 0;
	md = mm = mr = oXXX = gXXX = bad = 0;
	for (j = 0; j < dip->di_sblk->info.sb.fs_count; j++) {
		if (lfset[j] == 0) {
			/*
			 * check to see if hole is meta data device in which
			 * case we probably are zoned off from it
			 */
			if (dip->di_sblk->eq[j].fs.type != DT_META) {
				holes++;
			} else {
				if (!missing_meta_message) {
					printf("# ");
					/*
					 * zoned off or missing metadata
					 * device
					 */
					printf(GetCustMsg(13807));
					printf("#\n");
					missing_meta_message++;
				}
			}
		}
		switch (dip->di_sblk->eq[j].fs.type) {
		case DT_DATA:
			md++;
			break;
		case DT_META:
			mm++;
			break;
		case DT_RAID:
			mr++;
			break;
		default:
			if (is_osd_group(dip->di_sblk->eq[j].fs.type)) {
				oXXX++;
				break;
			}
			if (is_stripe_group(dip->di_sblk->eq[j].fs.type)) {
				gXXX++;
				break;
			}
			/* Unrecognized device type:  %#x */
			error(0, 0, GetCustMsg(13238),
			    dip->di_sblk->eq[j].fs.type);
			bad++;
		}
	}
	if (holes) {
		printf("# ");
		/* Missing slices */
		printf(GetCustMsg(13808));
	}
	if (!holes && !bad) {
		char *sh, *typ;

		sh = (dip->di_sblk->info.sb.hosts) ? " shared" : "";
		FSStr(dip->di_sblk->info.sb.fs_name, fsbuf);

		switch (dip->di_sblk->info.sb.fi_type) {
		case DT_DISK_SET:
			typ = "ms";
			break;
		case DT_META_SET:
			typ = "ma";
			break;
		case DT_META_OBJECT_SET:
			typ = "mb";
			break;
		case DT_META_OBJ_TGT_SET:
			typ = "mat";
			break;
		default:
			/* Backwards compatibility for new fi_type field. */
			if (oXXX) {
				typ = "mb";
			} else {
				typ = (md && !mm && !mr && !gXXX) ? "ms" : "ma";
			}
		}

		printf("%s %d %s %s -%s\n",
		    fsbuf, dip->di_sblk->info.sb.eq, typ, fsbuf, sh);
	}
	for (j = 0; j < dip->di_sblk->info.sb.fs_count; j++) {
		if (lfset[j] == 0) {
			if (dip->di_sblk->eq[j].fs.type == DT_META) {
				TypeStr(dip->di_sblk->eq[j].fs.type, typbuf);
				FSStr(dip->di_sblk->info.sb.fs_name, fsbuf);
				printf(template,
				    (holes || bad) ? "# " : "", "",
				    "nodev    ",
				    dip->di_sblk->eq[j].fs.eq, typbuf, fsbuf, "-",
				    (long long)dip->di_sblk->eq[j].fs.capacity);

			}
		}
	}
	for (j = 0; j < n; j++) {
		struct sam_sblk *sblk;

		sblk = dip[j].di_sblk;
		ord = sblk->info.sb.ord;
		if (holes || bad) {
			printf("# ");
			/* Ordinal %d */
			printf(GetCustMsg(13809), ord);
		}
		TypeStr(sblk->eq[ord].fs.type, typbuf);
		FSStr(sblk->info.sb.fs_name, fsbuf);
		if (sblk->info.sb.fsgen != fsgen) {
			if (verbose) {
				printf(template, "#", "", dip[j].di_devname,
				    sblk->eq[ord].fs.eq, typbuf, fsbuf, "-",
				    (long long) sblk->eq[ord].fs.capacity);
			}
			continue;
		}
		if (holes || bad) {
			printf(template, "#", "", dip[j].di_devname,
			    sblk->eq[ord].fs.eq, typbuf, fsbuf, "-",
			    (long long) sblk->eq[ord].fs.capacity);
			continue;
		}
		printf(template, "", "", dip[j].di_devname,
		    sblk->eq[ord].fs.eq, typbuf, fsbuf, "-",
		    (long long) sblk->eq[ord].fs.capacity);
	}
	printf("\n");
}

/*
 * convert a unit type to a slice code (e.g., 'mm', 'mr', 'md', 'oXXX', gXXX)
 */
static void
TypeStr(dtype_t typ, char *buf)
{
	switch (typ&DT_CLASS_MASK) {
	case DT_DISK:
		if (is_stripe_group(typ)) {
			sprintf(buf, "g%d", typ&DT_MEDIA_MASK);
		} else {
			if ((typ&DT_MEDIA_MASK) < 8) {
				strcpy(buf, dev_nmmd[typ&DT_MEDIA_MASK]);
			} else {
				strcpy(buf, "bad");
			}
		}
		break;
	}
}

static void
FSStr(uname_t name, char *buf)
{
	strncpy(buf, (char *)&name[0], sizeof (uname_t));
	buf[sizeof (uname_t)] = 0;
}

static void
SamPrintSB(struct sam_sblk *sbp, char *str)
{
	int i;

	printf("%s:\n\tsblk.info.sb.name         = '%c%c%c%c'\n",
	    str,
	    sbp->info.sb.name[0],
	    sbp->info.sb.name[1],
	    sbp->info.sb.name[2],
	    sbp->info.sb.name[3]);
	printf("\tsblk.info.sb.magic        = %#x\n", (int)sbp->info.sb.magic);
	printf("\tsblk.info.sb.init         = %#x\n", (int)sbp->info.sb.init);
	printf("\tsblk.info.sb.fsgen        = %#x\n", (int)sbp->info.sb.fsgen);
	printf("\tsblk.info.sb.ord          = %#x\n", (int)sbp->info.sb.ord);
	printf("\tsblk.info.sb.fi_type      = %#x\n",
	    (int)sbp->info.sb.fi_type);
	printf("\tsblk.info.sb.fs_name      = '%s'\n",
	    (char *)&sbp->info.sb.fs_name);
	printf("\tsblk.info.sb.time         = %#x\n", (int)sbp->info.sb.time);
	printf("\tsblk.info.sb.state        = %#x (unused)\n",
	    (int)sbp->info.sb.state);
	printf("\tsblk.info.sb.inodes       = %#llx\n",
	    (long long)sbp->info.sb.inodes);
	printf("\tsblk.info.sb.offset[0]    = %#llx\n",
	    (long long)sbp->info.sb.offset[0]);
	printf("\tsblk.info.sb.offset[1]    = %#llx\n",
	    (long long)sbp->info.sb.offset[1]);
	printf("\tsblk.info.sb.capacity     = %#llx\n",
	    (long long)sbp->info.sb.capacity);
	printf("\tsblk.info.sb.space        = %#llx\n",
	    (long long)sbp->info.sb.space);
	printf("\tsblk.info.sb.dau_blks[0]  = %#x\n",
	    (int)sbp->info.sb.dau_blks[0]);
	printf("\tsblk.info.sb.dau_blks[1]  = %#x\n",
	    (int)sbp->info.sb.dau_blks[1]);
	printf("\tsblk.info.sb.sblk_size    = %#x\n",
	    (int)sbp->info.sb.sblk_size);
	printf("\tsblk.info.sb.fs_count     = %#x\n",
	    (int)sbp->info.sb.fs_count);
	printf("\tsblk.info.sb.da_count     = %#x\n",
	    (int)sbp->info.sb.da_count);
	printf("\tsblk.info.sb.mm_count     = %#x\n",
	    (int)sbp->info.sb.mm_count);
	printf("\tsblk.info.sb.mm_capacity  = %#llx\n",
	    (long long)sbp->info.sb.mm_capacity);
	printf("\tsblk.info.sb.mm_space     = %#llx\n",
	    (long long)sbp->info.sb.mm_space);
	printf("\tsblk.info.sb.meta_blks    = %#x\n",
	    (int)sbp->info.sb.meta_blks);
	printf("\tsblk.info.sb.eq           = %#x\n",
	    (int)sbp->info.sb.eq);
	printf("\tsblk.info.sb.mm_blks[0]   = %#x\n",
	    (int)sbp->info.sb.mm_blks[0]);
	printf("\tsblk.info.sb.mm_blks[1]   = %#x\n",
	    (int)sbp->info.sb.mm_blks[1]);
	printf("\tsblk.info.sb.mm_ord       = %#x\n",
	    (int)sbp->info.sb.mm_ord);
	printf("\tsblk.info.sb.opt_mask_ver = %#x\n",
	    (int)sbp->info.sb.opt_mask_ver);
	printf("\tsblk.info.sb.opt_mask     = %#x\n",
	    (int)sbp->info.sb.opt_mask);
	printf("\tsblk.info.sb.opt_features = %#x\n\n",
	    (int)sbp->info.sb.opt_features);

	for (i = 0; i < sbp->info.sb.fs_count; i++) {
		printf("\tsblk.eq[%d]:\n", i);
		printf("\t\tfs.ord         = %#x\n", (int)sbp->eq[i].fs.ord);
		printf("\t\tfs.eq          = %#x\n", (int)sbp->eq[i].fs.eq);
		printf("\t\tfs.state       = %#x\n",
		    (int)sbp->eq[i].fs.state);
		printf("\t\tfs.fsck_stat   = %#x\n",
		    (int)sbp->eq[i].fs.fsck_stat);
		printf("\t\tfs.num_group   = %#x\n",
		    (int)sbp->eq[i].fs.num_group);
		printf("\t\tfs.capacity    = %#llx\n",
		    (long long)sbp->eq[i].fs.capacity);
		printf("\t\tfs.space       = %#llx\n",
		    (long long)sbp->eq[i].fs.space);
		printf("\t\tfs.allocmap    = %#llx\n",
		    (long long)sbp->eq[i].fs.allocmap);
		printf("\t\tfs.fill2       = %#x (unused)\n",
		    (int)sbp->eq[i].fs.fill2);
		printf("\t\tfs.l_allocmap  = %#x\n",
		    (int)sbp->eq[i].fs.l_allocmap);
		printf("\t\tfs.dau_next    = %#llx\n",
		    (long long)sbp->eq[i].fs.dau_next);
		printf("\t\tfs.dau_size    = %#llx\n",
		    (long long)sbp->eq[i].fs.dau_size);
		printf("\t\tfs.system      = %#x\n",
		    (int)sbp->eq[i].fs.system);
		printf("\t\tfs.mm_ord      = %#x\n",
		    (int)sbp->eq[i].fs.mm_ord);
		printf("\t\tfs.type        = %#x\n\n",
		    (int)sbp->eq[i].fs.type);
	}
}

static void
SamPrintDKIOC(struct dk_cinfo *dkp, char *dev)
{
	printf("\n%s: DKIOCINFO\n", dev);
	printf("\tdki_cname       = '%s'\n", dkp->dki_cname);
	printf("\tdki_dname       = '%s'\n", dkp->dki_dname);
	printf("\tdki_ctype       = %#x\n", (int)dkp->dki_ctype);
	printf("\tdki_flags       = %#x\n", (int)dkp->dki_flags);
	printf("\tdki_cnum        = %#x\n", (int)dkp->dki_cnum);
	printf("\tdki_addr        = %#x\n", dkp->dki_addr);
	printf("\tdki_space       = %#x\n", dkp->dki_space);
	printf("\tdki_prio        = %#x\n", dkp->dki_prio);
	printf("\tdki_vec         = %#x\n", dkp->dki_vec);
	printf("\tdki_unit        = %#x\n", dkp->dki_unit);
	printf("\tdki_slave       = %#x\n", dkp->dki_slave);
	printf("\tdki_partition   = %#x\n", dkp->dki_partition);
	printf("\tdki_maxtransfer = %#x\n", dkp->dki_maxtransfer);
}

static void
SamPrintVTOC(struct vtoc *vtp, char *dev)
{
	int i;

	printf("\n%s: DKIOCGVTOC\n", dev);
	printf("\tv_sanity     = %#x\n", (int)vtp->v_sanity);
	printf("\tv_version    = %#x\n", (int)vtp->v_version);
	printf("\tv_volume     = '%s'\n", vtp->v_volume);
	printf("\tv_sectorsz   = %#x\n", (int)vtp->v_sectorsz);
	printf("\tv_nparts     = %#x\n", (int)vtp->v_nparts);
	printf("\tv_asciilabel = '%s'\n", vtp->v_asciilabel);
	for (i = 0; i < vtp->v_nparts; i++) {
		printf("\tv_part[%d].p_tag   = %#x\n",
		    i, (int)vtp->v_part[i].p_tag);
		printf("\tv_part[%d].p_flag  = %#x\n",
		    i, (int)vtp->v_part[i].p_tag);
		printf("\tv_part[%d].p_start = %#llx\n",
		    i, (long long)vtp->v_part[i].p_start);
		printf("\tv_part[%d].p_size  = %#x\n\n",
		    i, (int)vtp->v_part[i].p_size);
	}
}

#ifdef sun

/*
 * Print a UUID with minimal formatting and no decoding.
 */

static void
SamPrintUUID(struct uuid *u)
{
	int i;

	printf("0x");
	for (i = 0; i < UUID_LEN; i++) {
		printf("%02x", ((uchar_t *)u)[i]);
	}
}

/*
 * Print an EFI label in semi-human-readable form.
 */

static void
SamPrintEFI(struct dk_gpt *efi, char *dev)
{
	int i;

	printf("\n%s: EFI label\n", dev);
	printf("\tefi_version     = %#x\n", efi->efi_version);
	printf("\tefi_nparts      = %d\n", efi->efi_nparts);
	printf("\tefi_lbasize     = %#x\n", efi->efi_lbasize);
	printf("\tefi_last_lba    = %#llx\n", efi->efi_last_lba);
	printf("\tefi_first_u_lba = %#llx\n", efi->efi_first_u_lba);
	printf("\tefi_last_u_lba  = %#llx\n", efi->efi_last_u_lba);
	printf("\tefi_disk_uguid  = "); SamPrintUUID(&efi->efi_disk_uguid);
	printf("\n\n");

	for (i = 0; i < efi->efi_nparts; i++) {
		printf("\tefi_parts[%d].p_start = %#llx\n",
		    i, efi->efi_parts[i].p_start);
		printf("\tefi_parts[%d].p_size  = %#llx\n",
		    i, efi->efi_parts[i].p_size);
		printf("\tefi_parts[%d].p_guid  = ", i);
		SamPrintUUID(&efi->efi_parts[i].p_guid);
		printf("\n");
		printf("\tefi_parts[%d].p_tag   = %#x\n",
		    i, efi->efi_parts[i].p_tag);
		printf("\tefi_parts[%d].p_flag  = %#x\n",
		    i, efi->efi_parts[i].p_flag);
		printf("\tefi_parts[%d].p_name  = %.*s\n",
		    i, EFI_PART_NAME_LEN, efi->efi_parts[i].p_name);
		printf("\tefi_parts[%d].p_uguid = ", i);
		SamPrintUUID(&efi->efi_parts[i].p_uguid);
		printf("\n\n");
	}
}

#endif /* sun */

/*
 * Convert a /dev/dsk/xyzzy pathname to /dev/rdsk/xyzzy
 *
 * Return 1 if /dsk/ was found and replaced (success); 0 otherwise.
 */
static int
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
		if (verbose) {
			error(0, 0, GetCustMsg(1606), "getfullrawname");
		}
		return (0);
	}
	if (strcmp(rdevname, "\0") == 0) {
		if (verbose) {
			error(0, 0, GetCustMsg(13423), dsk, rdevname);
		}
		free(rdevname);
		return (0);
	}
	strcpy(rdsk, rdevname);
	free(rdevname);
	return (1);

#endif	/* linux */
}
