/*
 * samfstyp.c - Determine SAM-FS or SAM-QFS filesystem type
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.15 $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vfs.h>

#include <sys/inttypes.h>

#define DEC_INIT
#include <sam/param.h>
#include <sam/types.h>
#include <sam/nl_samfs.h>
#include <sam/custmsg.h>
#include <sam/format.h>
#include <sam/fs/ino.h>
#include <sam/fs/samhost.h>
#include <sam/disk_mgmt.h>
#include <sam/disk_show.h>
#include <sam/fs/sblk.h>
#include <sam/sblk_show.h>
#include <sblk_mgmt.h>
#include <sam/format.h>

static void usage(void);

int
main(int argc, char *argv[])
{
	int c;
	int err, errflag = 0;
	int vflag = 0;
	char *devp;
	struct sam_sblk sb;
	int fd;
	unsigned long fstype;
	char fsname[16];
	int error;
	size_t len;
	char *osd_dev = "/dev/osd/";
	char *osd_devices = "/devices/scsi_vhci/object-store@";

	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN "SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);

	while ((c = getopt(argc, argv, "v")) != EOF) {
		switch (c) {
		case 'v':		/* dump super block */
			vflag++;
			break;

		case '?':
			errflag++;
		}
	}
	if (errflag || argc <= optind) {
		usage();
		exit(30);
	}

	devp = argv[optind];

	/* Get SAM-FS/QFS superblock and associated file descriptor (0). */
	close(0);
	if ((err = strncmp(osd_dev, devp, strlen(osd_dev))) == 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5051,
		    "/dev/osd devices are not supported.\n"));
		exit(31);
	}
	if ((err = strncmp(osd_devices, devp, strlen(osd_devices))) == 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5052,
		    "/devices/scsi_vhci/object-store devices are not "
		    "supported.\n"));
		exit(32);
	}
	if ((err = sam_dev_sb_get(devp, &sb, &fd)) != 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5052,
		    "Unrecognized Superblock.  error = %d\n"));
		exit(33);
	}
	if (fd != 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5052,
		    "Invalid File Descriptor.  fd = %d\n"));
		exit(34);
	}

	/* Determine SAM-FS/QFS filesystem type. */
	if ((err = sam_sb_fstype_get(&sb, &fstype)) != 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5052,
		    "Unrecognized Filesystem Type.  error = %d\n"));
		exit(35);
	}

	/* Convert SAM-FS/QFS filesystem type to string. */
	if ((err = sam_fstype_to_str(fstype, fsname, 16)) != 0) {
		(void) fprintf(stderr, catgets(catfd, SET, 5052,
		    "String conversion failure.  error = %d\n"));
		exit(36);
	}

	/* Print fstype name to stdout. */
	printf("%s\n", fsname);

	/* If verbose requested, show the details. */
	if (vflag) {
		struct sam_host_table *htp;
		struct dk_cinfo ci;
		struct dk_geom cg;
		struct vtoc vt;
		struct dk_gpt *eip;
		struct sam_perm_inode pi;
		int i;
		sam_format_buf_t *bp;
		int nparts = 0;
		int htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;

		bp = (sam_format_buf_t *)malloc(16384);
		htp = (struct sam_host_table *)malloc(htbufsize);

		sam_format_width_name_default(16);
		sam_format_width_value_default(24);

		/* Show superblock general info. */
		printf("%s {\n", devp);
		sam_format_prefix_default("  ");
		sam_sbinfo_format(&sb.info.sb, bp);
		sam_format_print(bp, NULL);

		sam_format_prefix_default("    ");

		/* Show superblock family set members. */
		for (i = 0; i < sb.info.sb.fs_count; i++) {
			sam_sbord_format(&sb.eq[i].fs, bp);
			printf("  fset %d {\n", i);
			sam_format_print(bp, NULL);
			printf("  }\n");
		}

		/* Get and show inode info. */
		if (sam_inodes_get(fd, &pi) == 0) {
			sam_inodes_format(&pi, bp);
			printf("  inodes {\n");
			sam_format_print(bp, NULL);
			printf("  }\n");
		}

		/* Get and show VTOC info. */
		if (sam_fd_vtoc_get(fd, &vt) == 0) {
			sam_vtoc_format(&vt, bp);
			printf("  vtoc {\n");
			sam_format_print(bp, NULL);

			sam_vtoc_part_count(&vt, &nparts);
			for (i = 0; i < nparts; i++) {
				printf("    part %d {\n", i);
				sam_format_prefix_default("      ");
				sam_vtoc_partition_format(&vt, i, bp);
				sam_format_print(bp, NULL);
				printf("    }\n");	/* part # */
			}
			printf("  }\n");	/* vtoc */
		/* Couldn't get VTOC info.  Get and show EFI VTOC info. */
		} else {
			if (sam_fd_efi_get(fd, &eip) == 0) {
				printf("  efi_vtoc {\n");
				sam_efi_format(eip, bp);
				sam_format_print(bp, NULL);

				sam_efi_part_count(eip, &nparts);
				for (i = 0; i < nparts; i++) {
					printf("    part %d {\n", i);
					sam_format_prefix_default("      ");
					sam_efi_partition_format(eip, i, bp);
					sam_format_print(bp, NULL);
					printf("    }\n");	/* part # */
				}
				printf("  }\n");	/* efi_vtoc */

				call_efi_free(eip);
			}
		}

		sam_format_prefix_default("    ");

		/* Get and show host table info. */
		if (sam_fd_host_table_get2(fd, htbufsize, htp) == 0) {
			printf("  hosts {\n");
			sam_host_table_format(htp, bp);
			sam_format_print(bp, NULL);
			printf("  }\n");	/* hosts */
		}

		/* Get and show controller info. */
		if (sam_fd_control_info_get(fd, &ci) == 0) {
			printf("  controller {\n");
			sam_control_info_format(&ci, bp);
			sam_format_print(bp, NULL);
			printf("  }\n");	/* controller */
		}

		/* Get and show controller geometry info. */
		if (sam_fd_control_geometry_get(fd, &cg) == 0) {
			printf("  geometry {\n");
			sam_control_geometry_format(&cg, bp);
			sam_format_print(bp, NULL);
			printf("  }\n");	/* geometry */
		}

		printf("}\n");	/* device */
	}

	return (0);
}

static void
usage(void)
{
	(void) fprintf(stderr, catgets(catfd, SET, 5050,
	    "usage: samfstyp [-v] special\n"));
}
