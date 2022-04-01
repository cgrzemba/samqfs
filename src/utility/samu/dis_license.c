/*
 *  dis_license.c - display license information
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

#pragma ident "Id: $"

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

/* Solaris headers. */
#include <sys/types.h>
#include <sys/systeminfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>

/* Local headers. */
#include "sam/types.h"
#include "sam/nl_samfs.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/syscall.h"
#include "sam/shareops.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "samu.h"
#include "license/license.h"

static void
PrintLibUsage() {
	int eq;
	int i;
	int newcount;
	int usage;
	uint64_t    lcapacity;
	uint64_t    lspace;
	uint64_t    tcapacity;
	uint64_t    tspace;
	dev_ent_t   *dev;
	char		buf0[64];
	char		buf1[64];
	char		buf2[64];
	char		*fmt;
	char		*fmt2;
	char		*fmt3;
	struct  CatalogEntry *cv;
	static struct CatalogEntry *list = NULL;


	if (IsSamRunning == FALSE) {
		return;
	}
	CatlibInit();
	if (Dev_Tbl == NULL) {
		return;
	}
	tcapacity = 0;
	tspace = 0;
	fmt = catgets(catfd, SET, 7391,
		"%s %s: capacity %s bytes space %s bytes, usage %3d%%");
	fmt2 = catgets(catfd, SET, 7392, "library   ");
	for (eq = 0; eq <= Max_Devices; eq++) {
		if (Dev_Tbl->d_ent[eq] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[eq]);
		if (!((dev->type & DT_CLASS_MASK) == DT_ROBOT ||
			dev->type == DT_PSEUDO_SC ||
			dev->fseq == 0)) {
				continue;
		}
		newcount = CatalogGetEntries(eq, 0, INT_MAX, &list);
		if (newcount <= 0) {
			continue;
		}
		lcapacity = 0;
		lspace = 0;
		for (i = 0; i < newcount; i++) {
			cv = &list[i];
			if (cv->CeStatus & CES_inuse) {
				lcapacity += cv->CeCapacity * 1024;
				lspace += cv->CeSpace * 1024;
			}
		}
		usage = llpercent_used(lcapacity, lspace);
		snprintf(buf0, sizeof (buf0), "%6d", eq);
		Mvprintw(ln++, 0, fmt, fmt2, buf0,
		    StrFromFsize(lcapacity, 1, buf1, sizeof (buf1)),
		    StrFromFsize(lspace, 1, buf2, sizeof (buf2)), usage);
		tcapacity += lcapacity;
		tspace += lspace;
	}
	usage = llpercent_used(tcapacity, tspace);
	fmt3 = catgets(catfd, SET, 7393, "totals");
	Mvprintw(ln++, 0, fmt,
		fmt2, fmt3,
		StrFromFsize(tcapacity, 1, buf1, sizeof (buf1)),
		StrFromFsize(tspace, 1, buf2, sizeof (buf2)), usage);

}

static struct sam_fs_info   *finfo = NULL;

static void
PrintFsUsage() {
	char buf3[64];
	char buf4[64];
	char *fmt;
	char *fmt2;
	char *fmt3;
	char *fmt4;
	int i;
	int numfs;
	int usage;
	uint64_t    tcapacity;
	uint64_t    tspace;
	struct sam_fs_status *fsarray;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		return;
	}
	if (finfo != NULL) free(finfo);
	finfo = (struct sam_fs_info *)malloc(numfs *
	    sizeof (struct sam_fs_info));
	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;
		struct sam_fs_info *fi;

		fs = fsarray + i;
		fi = finfo + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			continue;
		}
	}
	free(fsarray);
	tcapacity = 0;
	tspace = 0;
	fmt = catgets(catfd, SET, 7391,
		"%s %s: capacity %s bytes space %s bytes, usage %3d%%");
	fmt2 = catgets(catfd, SET, 7394, "filesystem");
	fmt3 = catgets(catfd, SET, 7397, "server");
	fmt4 = catgets(catfd, SET, 7396, "client");
	for (i = 0; i < numfs; i++) {
		struct sam_fs_info *fi;

		fi = finfo + i;
		if ((fi->fi_status & FS_MOUNTED) == 0) continue;
		usage = llpercent_used(fi->fi_capacity, fi->fi_space);
		Mvprintw(ln++, 0, fmt,
			fmt2,
			fi->fi_name,
			StrFromFsize(fi->fi_capacity, 1, buf3, sizeof (buf3)),
			StrFromFsize(fi->fi_space, 1, buf4, sizeof (buf4)),
			usage);
		if (fi->fi_status & FS_SERVER) {
			Printw(" %s", fmt3);
		}
		if (fi->fi_status & FS_CLIENT) {
			Printw(" %s", fmt4);
		}
		tcapacity += fi->fi_capacity;
		tspace += fi->fi_space;
	}
	fmt3 = catgets(catfd, SET, 7393, "totals");
	usage = llpercent_used(tcapacity, tspace);
	Mvprintw(ln++, 0, fmt,
		fmt2, fmt3,
		StrFromFsize(tcapacity, 1, buf3, sizeof (buf3)),
		StrFromFsize(tspace, 1, buf4, sizeof (buf4)), usage);

	if (finfo != NULL) free(finfo);
}

void
DisLicense() {
	sam_license_t_33 *license;
	char si_data[64];
	char si_arch[64];
	char si_sys[64];
	char *fmt;
	int cpus;
	int online_cpus;
	struct sam_license_arg getlic;
	uint_t hwserial;

	/* get the licenses */

	if ((sam_syscall(SC_getlicense, &getlic,
			sizeof (struct sam_license_arg))) < 0) {
		if (errno == ENOSYS) {
			Mvprintw(ln++, 0,
				catgets(catfd, SET, 7000,
				"SAM-FS is not running."));
			exit(1);
		} else {
			Mvprintw(ln++, 0, catgets(catfd, SET, 7306,
			    "No usage data"));
			return;
		}
	}

	license = &getlic.value;

	(void) sysinfo(SI_HW_SERIAL, si_data, sizeof (si_data));
	hwserial = strtoul(si_data, NULL, 10);
	(void) sysinfo(SI_SYSNAME, si_sys, sizeof (si_sys));
	(void) sysinfo(SI_ARCHITECTURE, si_arch, sizeof (si_arch));
	cpus = sysconf(_SC_NPROCESSORS_CONF);
	online_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	fmt = catgets(catfd, SET, 7395,
		"hostid: %x  OS name: %s  Architecture: %s CPUs: "
		"%d (%d online)");
	Mvprintw(ln++, 0, fmt, hwserial, si_sys, si_arch, cpus, online_cpus);
	ln++;

	/* display the WORM license bit if set */

	if (license->license.lic_u.b.WORM_fs) {
		fmt = catgets(catfd, SET, 11086,
			"WORM fs support enabled\n");
		Mvprintw(ln++, 0, fmt);
		ln++;
	}
	PrintLibUsage();
	ln++;
	PrintFsUsage();
}
