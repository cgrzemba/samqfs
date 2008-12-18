/*
 *  sammkfs.c - Initialize a SAM-FS file system.
 *
 *	sammkfs labels the disk members of the filesystem by
 *	writing the super block, inode maps, allocation maps,
 *	and the root inode entry.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.41 $"


/* ----- Include Files ---- */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <sys/flock.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/buf.h>

#include <sam/types.h>
#include <sam/param.h>
#include <pub/devstat.h>
#include <sam/mount.h>
#include "sam/nl_samfs.h"
#include "sam/shareops.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "aml/proto.h"

#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "samhost.h"

#define	MAIN
#include "utility.h"


/* ----- Global tables & pointers ----- */

char *files[]				/* File name table */
	= {"????", ".inodes", "root", ".blocks", ".ioctl", ".archive",
	"lost+found"};

int growfs = 0;				/* Grow filesystem flag */

typedef enum {
	SAM_MKFS = 1,
	SAM_GROWFS,
	SAM_FSINFO,
	QFS_MKFS
} mkfscmd_t;

#define	SAM_MKFS_USAGE "[-i ninodes] [-a allocation] [-P] [-S] [-V] fs_name"
#define	SAM_GROWFS_USAGE "[-V] fs_name"
#define	SAM_FSINFO_USAGE "fs_name"
#define	QFS_MKFS_USAGE "[-i ninodes] [-a allocation] [-P] [-S] [-V] fs_name"

sam_daddr_t super_blk;		/* Super-block disk address */

int allocation = 0;		/* DAU */
int ninodes = 0;		/* Number of preallocated inodes */

int shared = 0;			/* -S (shared FS) */
int force = 0;			/* Force around particular errors */
char ***HostTab = NULL;		/* host table for -S option */
struct sam_host_table *hostbuf = NULL;
int hostbuflen = 0;
boolean_t host_is_server = TRUE;
int devices_open = 0;		/* Have the devices been opened yet */

sam_mount_info_t mnt_info;	/* Mount info from mcf file */
int fs_count;			/* Count of devices in filesystem */
int old_count = 0;		/* Size of old filesystem */
int old_mm_count = 0;		/* meta partitions in old filesystem */
int lun_ge_1tb = 0;		/* Number of partitions >= 1TB */
int lun_ge_16tb = 0;		/* Number of partitions >= 16TB */
int prev_sblk_ver = FALSE;	/* Use previous version for this filesystem */

char *usagestring;		/* usage string for error messages */

int process_args(int argc, char **argv, mkfscmd_t cmd);
int new_fs(void);
int grow_fs(void);
int iino(void);
void init_sblk();
void grow_sblk();
int show_fs(void);
int ask(char *msg, char def);
static int clear_obj_label(struct d_list *devp);
static void print_sblk(struct sam_sblk *sp, struct d_list *devp, int ordering);
static void read_existing_sblk(struct d_list *devp, struct sam_sblk *sblk,
			int sblk_size);
static void check_fs_devices(char *fs_name);
static void check_lun_limits(char *fs_name);
#ifdef	DEBUG
void debug_print(int idx);
#endif	/* DEBUG */
extern int getHostName(char *host, int len, char *fs);

static struct sigaction sig_action;	/* Signal actions */
static void catch_signals(int sig);
static void clean_exit(int excode);
static void print_version_warning();


/*
 * Flag argument to print_sblk(); indicates source for ordinal ordering
 */
#define		ORDER_MEM		0
#define		ORDER_DISK		1


/*
 * ----- main
 *	Initialize a new filesystem or grow an existing filesystem.
 */

void
main(int argc, char **argv)
{
	int err;
	int length;
	mkfscmd_t cmd;

	CustmsgInit(0, NULL);
	program_name = basename(argv[0]);

	/*
	 * Catch signals.
	 */
	sig_action.sa_handler = catch_signals;
	sigemptyset(&sig_action.sa_mask);
	sig_action.sa_flags = 0;
	(void) sigaction(SIGHUP,  &sig_action, NULL);
	(void) sigaction(SIGINT,  &sig_action, NULL);
	(void) sigaction(SIGTERM, &sig_action, NULL);

	/*
	 * Check filesystem - will configure it if necessary.
	 */
	ChkFs();
	if (strcmp(&program_name[strlen(program_name)-4], "mkfs") == 0) {
		growfs = 0;
		if (standalone() == 0) {
			cmd = SAM_MKFS;
			usagestring = SAM_MKFS_USAGE;
		} else {
			cmd = QFS_MKFS;
			usagestring = QFS_MKFS_USAGE;
		}
	} else if (strcmp(&program_name[strlen(program_name)-6],
	    "growfs") == 0) {
		growfs = 1;
		cmd = SAM_GROWFS;
		usagestring = SAM_GROWFS_USAGE;
	} else if (strcmp(&program_name[strlen(program_name)-6],
	    "fsinfo") == 0) {
		cmd = SAM_FSINFO;
		usagestring = SAM_FSINFO_USAGE;
	} else {  /* program was invoked with invalid name */
		error(0, 0,
		    catgets(catfd, SET, 712,
		    "This command must be invoked as sammkfs, samgrowfs "
		    "or samfsinfo, not %s"), program_name);
		clean_exit(1);
	}
	time(&fstime);			/* File system initialize time */

	super_blk = SUPERBLK;
	if ((length = sizeof (struct sam_sbinfo)) > L_SBINFO) {
		error(0, 0, catgets(catfd, SET, 2437,
		    "Superblock information record size GT L_SBINFO (%x)"),
		    length);
		clean_exit(1);
	}
	if ((length = sizeof (struct sam_sbord)) > L_SBORD) {
		error(0, 0, catgets(catfd, SET, 2438,
		    "Superblock ordinal record size GT L_SBORD (%x)"), length);
		clean_exit(1);
	}

	if (argc < 2) {
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    program_name, usagestring);
		clean_exit(1);
	}

	if (process_args(argc, argv, cmd)) {
		error(0, 0, catgets(catfd, SET, 722,
		    "Configuration error."), 0);
		clean_exit(1);
	}

	if (growfs) {
		err = grow_fs();
	} else if (cmd == SAM_FSINFO) {
		err = show_fs();
	} else {
		print_version_warning();
		err = new_fs();
	}
	if (err) {
		error(0, 0,
		    catgets(catfd, SET, 1032,
		    "Error during initialization"), 0);
		clean_exit(1);
	}

	writedau();

	/*
	 * Notify any sam-sharefsd that the FS may have changed.
	 * (Even if not shared -- the FS may have been previously.)
	 */
	(void) sam_shareops(fs_name, SHARE_OP_WAKE_SHAREDAEMON, 0);

	clean_exit(0);
}


/*
 * ----- clean_exit - Clean up leftovers and exit with the
 * specified exit code.
 */
static void
clean_exit(
	int excode)		/* Exit code to exit with */
{
	/* Clean-up device info */
	if (devices_open) {
		close_devices(&mnt_info);
	}
	exit(excode);
}


/*
 * ----- catch_signals
 */
static void
catch_signals(int sig)
{
	switch (sig) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		error(0, 0, catgets(catfd, SET, 13968, "Stopped."));
		clean_exit(0);
		/* NOTREACHED */
	default:
		break;
	}
}


/*
 * -----  process_args
 *	Process command line arguments and process file system configuration
 *	(mcf file).
 */

int				/* ERRNO if error, 0 if successful */
process_args(
	int argc,		/* Number of arguments */
	char **argv,		/* Argument char pointer array */
	mkfscmd_t cmd)		/* instance of sammkfs invoked */
{
	int err = 0;
	char *optstring, *cp, c;
	extern char *optarg;
	extern int optind;
	int a_arg_found = 0;
	int open_mode = O_RDWR;


	switch (cmd) {
		case SAM_FSINFO:
			optstring = "";
			break;
		case SAM_MKFS:
			optstring = "a:i:VSNPF";
			break;
		case QFS_MKFS:
			optstring = "a:i:VSNPF";
			break;
		case SAM_GROWFS:
			optstring = "V";
			break;
		default:
			fprintf(stderr, "%s internal error\n", program_name);
			return (EINVAL);
	}

	while ((c = getopt(argc, argv, optstring)) != EOF) {
		switch (c) {
			case 'a':
				allocation = (int)strtol(optarg, &cp, 10);
				if (optarg == cp) {
					error(0, 0, catgets(catfd, SET, 13464,
				"The -%c argument requires a positive "
				"integer to follow"),
					    c);
					err = TRUE;
					break;
				}
				/* ignore the k postfix */
				if (*cp == 'k' || *cp == 'K') {
					cp++;
				}
				if (*cp == '\0') {
					if (allocation & 0x7) {
						error(0, 0, catgets(catfd,
						    SET, 13426,
						    "Allocation %d must be "
						    "an 8k multiple."),
						    allocation);
						err = TRUE;
					}
				} else {
					error(0, 0, catgets(catfd, SET, 2799,
					    "Unrecognized argument %s."),
					    argv[optind-1]);
					err = TRUE;
				}
				a_arg_found = 1;
				break;
			case 'i':
				ninodes = (int)strtol(optarg, &cp, 10);
				if (optarg == cp) {
					error(0, 0, catgets(catfd, SET, 13464,
				"The -%c argument requires a positive "
				"integer to follow"),
					    c);
					err = TRUE;
					break;
				}
				if (*cp != '\0') {
					error(0, 0, catgets(catfd, SET, 2799,
					    "Unrecognized argument %s."),
					    argv[optind-1]);
					err = TRUE;
				}
				break;
			case 'V':
				verbose = TRUE;
				break;
			case 'S':
				shared = 1;
				break;
			case 'F':
				force = 1;
				break;
			case 'P':
				prev_sblk_ver = TRUE;
				break;
			default:
				if (cmd == SAM_FSINFO) {
					error(0, 0, catgets(catfd, SET, 13415,
					"no options valid on samfsinfo "
					"command."));
					clean_exit(1);
				} else {
					error(0, 0, catgets(catfd, SET, 2799,
					    "Unrecognized argument %s."),
					    argv[optind-1]);
					err = TRUE;
				}
		}
	}
	argc -= optind;
	if (argc == 1) {
		strncpy(fs_name, argv[optind], sizeof (uname_t));
		fs_name[sizeof (uname_t) - 1] = '\0';

		if (cmd == SAM_FSINFO) {
			verbose = TRUE;
			open_mode = O_RDONLY;
		}
	} else if (argc == 0) {
		error(0, 0, catgets(catfd, SET, 13463,
		    "%s: No family set provided."),
		    *argv);
		err = TRUE;
	} else {
		err = TRUE;
	}
	if (err) {
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    program_name, usagestring);
		error(0, 0, catgets(catfd, SET, 13462,
		    "%s: Unrecognized argument."),
		    *argv);
	}
	if (shared) {
		char hostspath[MAXPATHLEN];

		sprintf(hostspath, "%s/hosts.%s", SAM_CONFIG_PATH, fs_name);
		if ((HostTab = SamReadHosts(hostspath)) == NULL) {
			err = TRUE;
		} else {
			int i, serverfound;
			char *servername;

			serverfound = 0;
			for (i = 0; HostTab[i] != NULL; i++) {
				if (HostTab[i][HOSTS_SERVER] &&
				    strcmp(HostTab[i][HOSTS_SERVER],
				    "server") == 0) {
					++serverfound;
					servername = HostTab[i][HOSTS_NAME];
				}
			}
			if (i > SAM_MAX_SHARED_HOSTS) {
				error(0, 0, catgets(catfd, SET, 13479,
				    "Warning: Host count "
				    "(%d) > SAM_MAX_SHARED_HOSTS (%d)"),
				    i, SAM_MAX_SHARED_HOSTS);
			}
			if (!serverfound) {
				error(0, 0, catgets(catfd, SET, 13451,
				    "No server declared in shared "
				    "hosts file"));
			} else if (serverfound > 1) {
				error(0, 0, catgets(catfd, SET, 13480,
				    "Multiple servers declared in "
				    "shared hosts file"));
			} else {
				/*
				 * Note if we are NOT the server.
				 * If we can't get the local host name,
				 * err on the side of caution.
				 */
				int len = sizeof (uname_t);
				char host[len+1];

				if ((getHostName(host, len,
				    fs_name) != HOST_NAME_EOK) ||
				    (strncmp(host, servername, len) != 0)) {
					host_is_server = FALSE;
				}
			}

			hostbuflen = SAM_LARGE_HOSTS_TABLE_SIZE;
			hostbuf = (struct sam_host_table *)malloc(hostbuflen);
			if (hostbuf == NULL) {
				error(0, 0, "Hosts file memory allocation"
				    "failed for (%s)", fs_name);
				clean_exit(1);
			}
			memset((char *)hostbuf, 0, hostbuflen);
			if (SamStoreHosts(hostbuf, hostbuflen,
			    HostTab, 1) < 0) {
				free(hostbuf);
				error(0, 0, catgets(catfd, SET, 13450,
				    "Can't store FS %s hosts file "
				    "into FS (check hosts file size)."),
				    fs_name);
				clean_exit(1);
			}
		}
	}

	if (err) {
		error(0, 0, catgets(catfd, SET, 445, "Argument error."), 0);
		clean_exit(1);
	}
	if (check_mnttab(fs_name)) {
		char *msg_str;

		msg_str = catgets(catfd, SET, 13421,
		    "filesystem %s is mounted.");
		if (cmd != SAM_FSINFO) {
			error(0, 0, msg_str, fs_name);
			clean_exit(1);
		} else {
			fprintf(stderr, "%s: ", program_name);
			fprintf(stderr, msg_str, fs_name);
			fprintf(stderr, "\n");
		}
	}

	err = chk_devices(fs_name, open_mode, &mnt_info);
	devices_open = 1;
	if (err == 0) {
		fs_count = mnt_info.params.fs_count;
		mm_count = mnt_info.params.mm_count;
		mm_ord = 0;

		if ((mnt_info.params.fi_type == DT_META_OBJ_TGT_SET) &&
		    shared) {
			error(0, 0, catgets(catfd, SET, 17261,
			    "%s: 'mat' filesystem cannot be shared"),
			    fs_name);
			clean_exit(1);
		}
		if (mnt_info.params.fi_type == DT_META_SET ||
		    mnt_info.params.fi_type == DT_META_OBJECT_SET ||
		    mnt_info.params.fi_type == DT_META_OBJ_TGT_SET) {
			if (mm_count == 0) {
				error(0, 0, catgets(catfd, SET, 13411,
				    "%s has no meta devices"),
				    fs_name);
				clean_exit(1);
			}
		}
		if (cmd == SAM_MKFS || cmd == QFS_MKFS) {
			if (a_arg_found == 0) {
				/*
				 * Default data disk DAU:
				 * QFS DAU without striped groups =
				 *    SAM_DEFAULT_MA_DAU,
				 * QFS DAU with striped groups =
				 *    SAM_DEFAULT_SG_DAU,
				 * SAMFS = SAM_DEFAULT_MS_DAU.
				 */
				if (mnt_info.params.fi_config1 &
				    MC_STRIPE_GROUPS) {
					allocation = SAM_DEFAULT_SG_DAU;
				} else if (mnt_info.params.fi_type ==
				    DT_META_SET) {
					allocation = SAM_DEFAULT_MA_DAU;
				} else {
					allocation = SAM_DEFAULT_MS_DAU;
				}
			}
			if (mnt_info.params.fi_config1 & MC_MD_DEVICES) {
				if (allocation != 16 && allocation != 32 &&
				    allocation != 64) {
					error(0, 0, catgets(catfd, SET, 13460,
					    "%s (md) allocation = %d, "
					    "must be 16, 32, or 64"),
					    fs_name, allocation);
					clean_exit(1);
				}
				/* Default data SM dau */
				SM_DEV_BLOCK(mp, DD) = SAM_LOG_BLOCK;
				/* Default data LG dau */
				LG_DEV_BLOCK(mp, DD) = allocation;
			} else {
				if (allocation < 8 || allocation >= 65536) {
					error(0, 0, catgets(catfd, SET, 13461,
					    "%s (ma) allocation = %d, "
					    "must be >= 8 and <= 65528"),
					    fs_name, allocation);
					clean_exit(1);
				}
				/* Default data SM dau */
				SM_DEV_BLOCK(mp, DD) = allocation;
				/* Default data LG dau */
				LG_DEV_BLOCK(mp, DD) = allocation;
			}
			/*
			 * Meta allocation.
			 */
			if (mnt_info.params.fi_type == DT_DISK_SET) {
				SM_DEV_BLOCK(mp, MM) = SM_DEV_BLOCK(mp, DD);
				LG_DEV_BLOCK(mp, MM) = LG_DEV_BLOCK(mp, DD);
			} else {
				SM_DEV_BLOCK(mp, MM) = SAM_LOG_BLOCK;
				LG_DEV_BLOCK(mp, MM) = SAM_DEFAULT_META_DAU;
			}
		}

		switch (cmd) {
		case SAM_MKFS:
		case QFS_MKFS:
		case SAM_GROWFS:
			if (mnt_info.params.fi_config1 & MC_SHARED_FS) {
				if (!shared && !growfs) {
					error(0, 0, catgets(catfd, SET, 13447,
					    "%s: %s is a shared filesystem; "
					    "use -S option."),
					    program_name, fs_name);
					clean_exit(1);
				}
				/*
				 * Only let server mkfs a shared file system.
				 */
				if (host_is_server == 0) {
					error(0, 0, catgets(catfd, SET, 13453,
					    "Cannot run sammkfs from a "
					    "client"));
					clean_exit(1);
				}
			} else {
				if (shared) {
					error(0, 0, catgets(catfd, SET, 13448,
	"%s: %s is not a shared filesystem; -S option specified."),
					    program_name, fs_name);
					clean_exit(1);
				}
			}
		}
	} else {
		if (errno == EBUSY) {
			error(0, 0, catgets(catfd, SET, 13416,
			    "%s needs to be run on an unmounted filesystem."),
			    program_name);
			clean_exit(1);
		}
	}
	return (err);
}


/*
 * Extract the gXXX number from a stripe group dev's type
 */
#define		STRIPE_GROUP(x)	((x)&DT_MEDIA_MASK)


/*
 * ----- new_fs - Create a new file system.
 *	Initialize a new filesystem.
 */

int			/* 1 if error, 0 if successful */
new_fs(void)
{
	int err = FALSE;	/* Error flag */
	int ord;			/* Disk ordinal */
	struct devlist *dp;

	sam_set_dau(&mp->mi.m_dau[DD], mp->mi.m_dau[DD].kblocks[SM],
	    mp->mi.m_dau[DD].kblocks[LG]);
	sam_set_dau(&mp->mi.m_dau[MM], mp->mi.m_dau[MM].kblocks[SM],
	    mp->mi.m_dau[MM].kblocks[LG]);
	get_mem(1);
	memset((char *)&sblock, 0, sizeof (struct sam_sblk));
	strncpy(sblock.info.sb.name, SAMFS_SB_NAME_STR,
	    sizeof (sblock.info.sb.name));
	sblock.info.sb.fs_count = fs_count;
	sblock.info.sb.da_count = fs_count - mm_count;
	sblock.info.sb.mm_count = mm_count;
	strncpy(sblock.info.sb.fs_name, fs_name, sizeof (uname_t));
	check_fs_devices(fs_name);

	printf(catgets(catfd, SET, 13427,
	    "Building '%s' will destroy the contents of devices:\n"), fs_name);
	for (ord = 0; ord < fs_count; ord++) {
		printf("\t\t%s\n", devp->device[ord].eq_name);
	}
	if (!verbose && isatty(0) && !ask(catgets(catfd, SET, 13428,
	    "Do you wish to continue? [y/N] "), 'n')) {
		printf(catgets(catfd, SET, 13430,
		    "Not building '%s'.  Exiting.\n"),
		    fs_name);
		clean_exit(2);
	}
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		sblock.eq[ord].fs.ord = ord;
		sblock.eq[ord].fs.eq = dp->eq;
		sblock.eq[ord].fs.type = dp->type;
		sblock.eq[ord].fs.num_group = dp->num_group;
	}
	init_sblk();			/* Initialize super block */
	if (verbose) {
		print_sblk(&sblock, devp, ORDER_MEM);
		printf(catgets(catfd, SET, 13470,
		    "Omitting the -V option will create filesystem '%s'.\n"),
		    fs_name);
		clean_exit(0);
	}

	/*
	 * Build the bit maps for all the FS devices except objects.
	 * Note that the maps for stripe groups exist only on the first
	 * stripe group element.
	 *
	 * For stripe groups, also check to ensure that stripe
	 * groups have contiguous ordinals.
	 */
	for (ord = 0; ord < fs_count; ord++) {
		int mord;

		if (is_osd_group(sblock.eq[ord].fs.type)) {
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			int i;

			if (sblock.eq[ord].fs.num_group == 0) {
				/* Stripe group element, not first member */
				continue;
			}

			for (i = 1; i < sblock.eq[ord].fs.num_group; i++) {
				if (ord + i >= fs_count ||
				    !is_stripe_group(
				    sblock.eq[ord+i].fs.type) ||
				    STRIPE_GROUP(sblock.eq[ord+i].fs.type) !=
				    STRIPE_GROUP(sblock.eq[ord].fs.type)) {
					err |= 1;
					error(0, 0, catgets(catfd, SET, 13454,
	"Stripe group elements of g%d not contiguous; reorder mcf "
	"file entries"),
					    STRIPE_GROUP(
					    sblock.eq[ord].fs.type));
				}
			}
		}
		mord = sblock.eq[ord].fs.mm_ord;
		err = sam_bfmap(SAMMKFS_CALLER, &sblock, ord,
		    &devp->device[mord], dcp, NULL, 1);
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 801,
			    "Dau map write failed on eq %2d, (%s)"),
			    devp->device[mord].eq, devp->device[mord].eq_name);
		}
	}
	if (err) {
		/* Don't write superblock if things have gone wrong */
		clean_exit(1);
	}
	/*
	 * Clear allocated blocks for all the FS devices except objects.
	 */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		int len;

		if (is_osd_group(sblock.eq[ord].fs.type)) {
			continue;
		}
		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			if (sblock.eq[ord].fs.num_group == 0) {
				/* Stripe group element, not first member */
				continue;
			}
		}
		err = sam_cablk(SAMMKFS_CALLER, &sblock, NULL, ord, 1, 1,
		    LG_DEV_BLOCK(mp, DD), LG_DEV_BLOCK(mp, MM), &len);
		if (err) {
			printf("eq%d: system %x != computed len %x\n",
			    dp->eq, sblock.eq[ord].fs.system, len);
			clean_exit(1);
		}
	}
	err |= iino();			/* Initialize inodes */
	err |= write_sblk(&sblock, devp);
#ifdef	DEBUG_PRINT
	print_sblk(&sblock, devp, ORDER_DISK);
#endif	/* DEBUG_PRINT */
	if ((mnt_info.params.fi_config1 & MC_SHARED_FS) &&
	    (mnt_info.params.fi_type == DT_META_OBJECT_SET)) {
		err |= clear_obj_label(devp);
	}
	return (err);
}


/*
 * ----- grow_fs - Grow an existing filesystem.
 *	Read file system labels and build the geometry for the disk
 *	filesystem in the mount entry.
 */

int			/* ERRNO if error, 0 if successful. */
grow_fs(void)
{
	int err = FALSE;
	struct sam_sblk *sblk;
	char sb[SAM_DEV_BSIZE];
	int i;
	int	ord;
	time_t time;
	struct devlist *dp;
	struct d_list *ndevp;

	sblk = (struct sam_sblk *)sb;
	if ((ndevp = (struct d_list *)malloc(sizeof (struct devlist) *
	    fs_count)) == NULL) {
		error(0, 0, catgets(catfd, SET, 604, "Cannot malloc ndevp."));
		return (1);
	}
	(void *) memcpy((char *)ndevp, (char *)devp,
	    sizeof (struct devlist) * fs_count);
	(void *) memset((char *)devp, 0, sizeof (struct devlist) * fs_count);
	/*
	 * Find ordinal 0 for existing filesystem fs_name
	 */
	read_existing_sblk(ndevp, sblk, 1);
	old_count = sblk->info.sb.fs_count;
	old_mm_count = sblk->info.sb.mm_count;
	time = sblk->info.sb.init;
	i = old_count;

	if (old_count == 0) {
		error(0, 0,
		    catgets(catfd, SET, 583,
		    "Cannot find ordinal 0 for filesystem %s"),
		    fs_name);
		clean_exit(1);
	}
	if (old_count >= fs_count) {
		error(0, 0,
		    catgets(catfd, SET, 13424,
		    "No new devices added to filesystem %s."),
		    fs_name);
		clean_exit(1);
	}
	if (mm_count && mm_count <= old_mm_count) {
		error(0, 0,
		    catgets(catfd, SET, 13433,
	"Cannot grow Sun QFS filesytem %s without adding metadata "
	"partitions."), fs_name);
		clean_exit(1);
	}
	if (fs_count > L_FSET) {
		error(0, 0,
		    catgets(catfd, SET, 13440,
		    "Too many devices (%d total) added to filesystem %s."),
		    fs_count, fs_name);
		clean_exit(1);
	}

	for (ord = 0, dp = (struct devlist *)ndevp; ord < fs_count;
	    ord++, dp++) {
		if (is_osd_group(dp->type)) {
			if ((read_object(fs_name, dp->oh, ord,
			    SAM_OBJ_SBLK_INO, (char *)sblk,
			    0, SAM_DEV_BSIZE))) {
				err = TRUE;
			}
		} else {
			if (d_read(dp, (char *)sblk, 1, SUPERBLK)) {
				err = TRUE;
			}
		}
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 2439,
			    "superblock read failed on eq (%d)"),
			    dp->eq);
			clean_exit(1);
		}
		/* Validate label is same for all members & not duplicated. */
		if (strncmp(sblk->info.sb.name, SAMFS_SB_NAME_STR,
		    sizeof (sblk->info.sb.name)) ||
		    (old_count != sblk->info.sb.fs_count) ||
		    strncmp(sblk->info.sb.fs_name, fs_name,
		    sizeof (uname_t)) ||
		    (time != sblk->info.sb.init)) {
			/* New partitions for filesystem fs_name */
			(void *) memcpy((char *)&devp->device[i], (char *)dp,
			    sizeof (struct devlist));
			if (++i > fs_count) {
				error(0, 0,
				    catgets(catfd, SET, 1017,
				    "Equipment for filesystem %s is not "
				    "all present"),
				    fs_name);
				clean_exit(1);
			}
		} else {
			/* Old partitions for filesystem fs_name */
			if (devp->device[sblk->info.sb.ord].eq != 0) {
				error(0, 0,
				    catgets(catfd, SET, 1627,
				    "mcf eq (%d) duplicate ordinal %d in "
				    "filesystem %s"),
				    dp->eq, sblk->info.sb.ord, fs_name);
				clean_exit(1);
			}
			(void *) memcpy((char *)&devp->device[
			    sblk->info.sb.ord], (char *)dp,
			    sizeof (struct devlist));
		}
	}

	check_fs_devices(fs_name);

	if (!verbose) {
		printf(catgets(catfd, SET, 13431,
		    "Growing '%s' will destroy the contents of devices:\n"),
		    fs_name);
	}
	for (ord = old_count; ord < fs_count; ord++) {
		printf("\t\t%s\n", devp->device[ord].eq_name);
	}
	if (!verbose && isatty(0) && !ask(catgets(catfd, SET, 13428,
	    "Do you wish to continue? [y/N] "), 'n')) {
		printf(catgets(catfd, SET, 13432,
		    "Not growing '%s'.  Exiting.\n"),
		    fs_name);
		clean_exit(2);
	}

	/* Make sure all members of the storage set are present. */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		if (dp->eq == 0) {
			error(0, 0,
			    catgets(catfd, SET, 1628,
			    "mcf eq (%d) ordinal %d not present in %s"),
			    dp->eq, ord, fs_name);
			clean_exit(1);
		}
	}
	dp = (struct devlist *)devp;
	i = howmany(L_SBINFO + (fs_count * L_SBORD), SAM_DEV_BSIZE);
	if (d_read(dp, (char *)&sblock, i, SUPERBLK)) {
		error(0, 0,
		    catgets(catfd, SET, 2439,
		    "superblock read failed on eq (%d)"),
		    dp->eq);
		clean_exit(1);
	}
	sblock.info.sb.fs_count = fs_count;
	sblock.info.sb.da_count = fs_count - mm_count;
	sblock.info.sb.mm_count = mm_count;
	sblock.info.sb.fsgen = ++sblk->info.sb.fsgen;
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		sblock.eq[ord].fs.ord = ord;
		sblock.eq[ord].fs.eq = dp->eq;
		if (ord < old_count) {
			if (sblock.eq[ord].fs.type != dp->type) {
				error(0, 0,
				    catgets(catfd, SET, 13424,
				    "Cannot change type for eq %d"), dp->eq);
				clean_exit(1);
			} else if (sblock.eq[ord].fs.num_group !=
			    dp->num_group) {
				error(0, 0, catgets(catfd, SET, 13446,
				    "Cannot change striped group for eq %d"),
				    dp->eq);
				clean_exit(1);
			}
		} else {
			sblock.eq[ord].fs.type = dp->type;
			sblock.eq[ord].fs.num_group = dp->num_group;
		}
	}
	grow_sblk();
	if (verbose) {
		print_sblk(&sblock, devp, ORDER_MEM);
		printf(catgets(catfd, SET, 13471,
		    "Omitting the -V option will extend filesystem '%s'.\n"),
		    fs_name);
		clean_exit(0);
	}
	/*
	 * Build bit maps for the new FS devices.  Note that
	 * the maps for stripe groups reside only on the first
	 * stripe group element.
	 *
	 * For stripe groups, also check to ensure that stripe
	 * groups have contiguous ordinals.
	 */
	for (ord = old_count; ord < fs_count; ord++) {
		int mord;

		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			int i;

			if (sblock.eq[ord].fs.num_group == 0) {
				/* stripe group element, not first */
				continue;
			}

			for (i = 1; i < sblock.eq[ord].fs.num_group; i++) {
				if (ord + i >= fs_count ||
				    !is_stripe_group(
				    sblock.eq[ord+i].fs.type) ||
				    STRIPE_GROUP(sblock.eq[ord+i].fs.type) !=
				    STRIPE_GROUP(sblock.eq[ord].fs.type)) {
					err |= 1;
					error(0, 0, catgets(catfd, SET, 13454,
	"Stripe group elements of g%d not contiguous; reorder mcf file "
	"entries"),
					    STRIPE_GROUP(
					    sblock.eq[ord].fs.type));
				}
			}
		}
		mord = sblock.eq[ord].fs.mm_ord;
		err = sam_bfmap(SAMMKFS_CALLER, &sblock, ord,
		    &devp->device[mord],
		    dcp, NULL, 1);
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 801,
			    "Dau map write failed on eq %2d, (%s)"),
			    devp->device[mord].eq, devp->device[mord].eq_name);
		}
	}
	if (err) {
		/* don't write superblock if things have gone wrong */
		clean_exit(1);
	}
	for (ord = old_count; ord < fs_count; ord++) {
		int len;

		if (is_stripe_group(sblock.eq[ord].fs.type)) {
			if (sblock.eq[ord].fs.num_group == 0) {
				continue; /* stripe group element, not first */
			}
		}
		err = sam_cablk(SAMMKFS_CALLER, &sblock, NULL, ord, 1, 1,
		    LG_DEV_BLOCK(mp, DD), LG_DEV_BLOCK(mp, MM), &len);
		if (err) {
			printf("eq%d: system %x != computed len %x\n",
			    dp->eq, sblock.eq[ord].fs.system, len);
			clean_exit(1);
		}
	}
	err |= write_sblk(&sblock, devp);
	return (err);
}


/*
 * ----- show_fs - Show an existing filesystem.
 *	Read file system labels and build the geometry for the disk
 *	filesystem in the mount entry.
 */

int
show_fs(void)
{
	int	idx;

	/*
	 * Find ordinal 0 for existing filesystem fs_name
	 */
	read_existing_sblk(devp, &sblock, sizeof (sblock)/SAM_DEV_BSIZE);

	print_sblk(&sblock, devp, ORDER_DISK);

#ifdef	DEBUG
	for (idx = 0; idx < fs_count; idx++) {
		debug_print(idx);
	}
#endif	/* DEBUG */
	return (0);
}


/*
 * ----- init_sblk - Initialize superblock.
 *	Initialize the superblock.
 */

void
init_sblk()
{
	int sblk2;	/* Block number */
	int ord, dt;
	struct devlist *dp;
	longlong_t m_size, t_size;
	boolean_t object_devices = FALSE;

	m_size = 0;
	t_size = 0;
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		dt = (dp->type == DT_META) ? MM : DD;
		dp->blocks = dp->size >> SAM2SUN_BSHIFT; /* Kilobyte blocks */
		if (dp->type == DT_META) {
			dp->blocks = dp->blocks / LG_DEV_BLOCK(mp, dt);
			/* Large blocks for metadata */
			m_size += dp->blocks;
		} else {
			if (is_osd_group(dp->type)) {
				object_devices = TRUE;
				/* Kilobyte blocks for object data */
				/* Increment before computing large blocks */
				t_size += dp->blocks;
				dp->blocks = dp->blocks / LG_DEV_BLOCK(mp, dt);
			} else {
				dp->blocks = dp->blocks / LG_DEV_BLOCK(mp, dt);
				/* Large blocks for block data */
				t_size += dp->blocks;
			}
		}
	}

	(void *) memcpy(sblock.info.sb.name, SAMFS_SB_NAME_STR,
	    sizeof (sblock.info.sb.name));
	if (prev_sblk_ver) {
		sblock.info.sb.magic = SAM_MAGIC_V2;
	} else {
		sblock.info.sb.magic = SAM_MAGIC_V2A;
		sblock.info.sb.opt_features |= SBLK_FV1_MAPS_ALIGNED;
	}
	sblock.info.sb.fi_type = mnt_info.params.fi_type;
	sblock.info.sb.min_usr_inum = SAM_MIN_USER_INO;
	sblock.info.sb.ext_bshift = SAM_SHIFT;
	sblock.info.sb.time = fstime;
	sblock.info.sb.init = fstime;	/* File system initialize time */
	sblock.info.sb.opt_mask_ver = SBLK_OPT_VER1;
	sblock.info.sb.opt_mask = 0;
	sblock.info.sb.eq = mnt_info.params.fi_eq;

	/*
	 * Block devices were rounded down to a DAU multiple.
	 * Object devices do not have the concept of a DAU.
	 */
	if (object_devices) {
		sblock.info.sb.capacity = t_size;
	} else {
		sblock.info.sb.capacity = t_size * LG_DEV_BLOCK(mp, DD);
	}
	sblock.info.sb.space = sblock.info.sb.capacity;
	sblock.info.sb.mm_capacity = m_size * LG_DEV_BLOCK(mp, MM);
	sblock.info.sb.mm_space = sblock.info.sb.mm_capacity;

	/*
	 * FIXME: DAU not applicable for 'mb' filesystem.
	 */
	sblock.info.sb.dau_blks[SM] = SM_DEV_BLOCK(mp, DD);
	sblock.info.sb.dau_blks[LG] = LG_DEV_BLOCK(mp, DD);
	sblock.info.sb.mm_blks[SM]  = SM_DEV_BLOCK(mp, MM);
	sblock.info.sb.mm_blks[LG]  = LG_DEV_BLOCK(mp, MM);
	if (sblock.info.sb.mm_blks[SM] == sblock.info.sb.mm_blks[LG]) {
		sblock.info.sb.meta_blks = sblock.info.sb.mm_blks[LG];
	}

	sblock.info.sb.fs_id.val[0] = fstime;
	sblock.info.sb.fs_id.val[1] = gethostid();

	/*
	 * Loop through the metadata devices, initializing the superblocks.
	 * We want the metadata bitmaps allocated before the data bitmaps
	 * to handle unusual cases (mm, mr, mr, mm).
	 */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		sblk_args_t args;

		if (dp->type == DT_META) {
			args.ord = ord;
			args.type = dp->type;
			args.blocks = dp->blocks;
			args.kblocks[DD] = LG_DEV_BLOCK(mp, DD);
			args.kblocks[MM] = LG_DEV_BLOCK(mp, MM);
			sam_init_sblk_dev(&sblock, &args);
		}
	}

	/*
	 * Loop through the data devices, initializing the superblocks.
	 */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		sblk_args_t args;

		if (dp->type != DT_META) {
			args.ord = ord;
			args.type = dp->type;
			args.blocks = dp->blocks;
			args.kblocks[DD] = LG_DEV_BLOCK(mp, DD);
			args.kblocks[MM] = LG_DEV_BLOCK(mp, MM);
			sam_init_sblk_dev(&sblock, &args);
		}
	}

	/*
	 * Loop through the devices, setting system (last allocated block)
	 */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		struct sam_sbord *sop = &sblock.eq[ord].fs;

		dt = (dp->type == DT_META) ? MM : DD;
		if (sblock.info.sb.mm_count == 0) {
			if (ord == 0) {
				sblk2 = sop->dau_next;
				sop->dau_next += LG_DEV_BLOCK(mp, dt);
			}
				sop->dau_next = roundup(sop->dau_next,
				    LG_DEV_BLOCK(mp, dt));
		} else {
			sop->dau_next = roundup(sop->dau_next,
			    LG_DEV_BLOCK(mp, dt));
			if (ord == 0) {
				sblk2 = sop->dau_next;
				sop->dau_next += LG_DEV_BLOCK(mp, dt);
				sop->dau_next = roundup(sop->dau_next,
				    LG_DEV_BLOCK(mp, dt));
			}
		}
		sop->system = roundup(sop->dau_next, LG_DEV_BLOCK(mp, dt));
	}

	sblock.info.sb.offset[0] = super_blk;	/* Superblock offset */
	sblock.info.sb.offset[1] = sblk2;	/* Backup superblock offset */
	sblock.info.sb.sblk_size = L_SBINFO +
	    (sblock.info.sb.fs_count * L_SBORD);

	/* LQFS persistent data */
	sblock.info.sb.logbno = 0ULL;
	sblock.info.sb.qfs_rolled = 0;
	sblock.info.sb.logord = 0;
	sblock.info.sb.qfs_clean = FSCLEAN;
}


/*
 * ----- grow_sblk - Grow superblock.
 *	Grow the superblock.
 */

void
grow_sblk()
{
	int iblk;	/* Block number */
	int ord, dt;
	struct devlist *dp;
	longlong_t m_size, t_size;
	int v_lbits;	/* No. of bits per logical blk */
	int len;
	int sblk_blks;

	m_size = 0;
	t_size = 0;
	v_lbits = SAM_DEV_BSIZE * NBBY;
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		dt = (dp->type == DT_META) ? MM : DD;
		dp->blocks = dp->size >> SAM2SUN_BSHIFT; /* Kilobyte blocks */
		/* Large blocks */
		dp->blocks = dp->blocks / LG_DEV_BLOCK(mp, dt);
		if (ord >= old_count) {
			if (dp->type == DT_META) {
				m_size += dp->blocks;
				if (mm_ord == 0) {
					/* Adding new meta device */
					mm_ord = ord;
				}
			} else {
				t_size += dp->blocks;
			}
		}
	}

	sblock.info.sb.capacity	+= t_size * LG_DEV_BLOCK(mp, DD);
	sblock.info.sb.space	+= t_size * LG_DEV_BLOCK(mp, DD);
	sblock.info.sb.mm_capacity += m_size * LG_DEV_BLOCK(mp, MM);
	sblock.info.sb.mm_space	+= m_size * LG_DEV_BLOCK(mp, MM);
	sblock.info.sb.sblk_size = L_SBINFO +
	    (sblock.info.sb.fs_count * L_SBORD);

	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		int has_no_bitmaps;

		dt = (dp->type == DT_META) ? MM : DD;
		if (ord == 0) {	/* Allocate at super block + LG block */
			iblk = super_blk + mp->mi.m_dau[dt].kblocks[LG];
		}
		if (ord < old_count) {
			t_size = (longlong_t)dp->blocks *
			    (longlong_t)(mp->mi.m_dau[dt].kblocks[LG] *
			    dp->num_group);
			if (sblock.eq[ord].fs.capacity > t_size) {
				error(0, 0, catgets(catfd, SET, 1626,
				    "Superblock size=%lld exceeds mcf "
				    "eq %d size=%lld"),
				    sblock.eq[ord].fs.capacity, dp->eq,
				    (dp->blocks *
				    mp->mi.m_dau[dt].kblocks[LG]));
				clean_exit(1);
			}
		} else {
			has_no_bitmaps = 0;
			sblock.eq[ord].fs.capacity = 0;
			sblock.eq[ord].fs.space = 0;
			if (sblock.eq[ord].fs.num_group) {
				sblock.eq[ord].fs.capacity = dp->blocks *
				    LG_DEV_BLOCK(mp, dt) *
				    dp->num_group;
				sblock.eq[ord].fs.space =
				    sblock.eq[ord].fs.capacity;
				sblock.eq[ord].fs.dau_size = dp->blocks;
				sblock.eq[ord].fs.l_allocmap =
				    howmany(sblock.eq[ord].fs.dau_size,
				    v_lbits);
				sblock.eq[ord].fs.allocmap = iblk;
				if (mm_count == 0) {
					sblock.eq[ord].fs.mm_ord = ord;
					len = super_blk +
					    LG_DEV_BLOCK(mp, dt) +
					    sblock.eq[ord].fs.l_allocmap;
					/* Account for last superblock */
					if (ord == 0) {
						len += LG_DEV_BLOCK(mp, dt);
					}
					len = roundup(len,
					    LG_DEV_BLOCK(mp, dt));
					sblock.eq[ord].fs.system = len;
				} else {
					has_no_bitmaps = 1;
					sblock.eq[ord].fs.mm_ord = mm_ord;
					iblk += sblock.eq[ord].fs.l_allocmap;
					if (sblock.eq[ord].fs.allocmap < 0) {
						error(0, 0, catgets(catfd,
						    SET, 13433,
		"Cannot grow QFS filesytem %s without adding metadata "
		"partitions."),
						    fs_name);
						clean_exit(1);
					}
				}
			} else {
				has_no_bitmaps = 1;
				sblock.eq[ord].fs.mm_ord = mm_ord;
			}
			if (has_no_bitmaps) {
				sblk_blks = sizeof (struct sam_sblk) /
				    SAM_DEV_BSIZE;
				len = super_blk + sblk_blks;
				len = roundup(len, LG_DEV_BLOCK(mp, dt));
				sblock.eq[ord].fs.system = len;
			}
			sblock.eq[ord].fs.type = dp->type;
		}
	}
	if (mm_count != 0) {
		len = roundup(iblk, LG_DEV_BLOCK(mp, MM));
		sblock.eq[mm_ord].fs.system = len;
		if ((offset_t)len > sblock.eq[mm_ord].fs.capacity) {
			error(0, 0, catgets(catfd, SET, 13452,
			    "Cannot grow QFS filesytem %s without adding "
			    "a metadata partition of %d 1k blocks"),
			    fs_name, len);
			clean_exit(1);
		}
	}
}


/*
 * ----- iino - Initialize inode files.
 *	Initialize inode file, inode map and root files.
 */

int					/* ERRNO if error, 0 if successful */
iino(void)
{
	int ino;			/* Inode number */
	sam_offset_t fblk;
	int fblk_to_extent;
	caddr_t bufp;
	union sam_di_ino *idp;
	struct sam_perm_inode *dp;
	struct sam_dirent *dirp;
	struct sam_empty_dir *ep;
	struct sam_dirval *hp;
	int length;
	int err = FALSE;
	int reclen;
	int doff;
	uint_t *inp;
	int dt;
	int i;
	int nhdau;
	int ino_version;

	ino_version = SAM_INODE_VERS_2;
	if ((length = sizeof (struct sam_perm_inode)) > SAM_ISIZE) {
		error(0, 0, catgets(catfd, SET, 1377,
		    "Inode size GT SAM_ISIZE (%x)"), length);
		clean_exit(1);
	}
	fblk_to_extent = sblock.info.sb.ext_bshift - SAM_DEV_BSHIFT;
	dt = (devp->device[mm_ord].type == DT_META) ? MM : DD;

	/*
	 * Build root directory file
	 */
	memset(dir_blk, 0, DIR_BLK);
	bufp = (char *)dir_blk;

	doff = 0;
	dirp = (struct sam_dirent *)bufp;	/* . */
	reclen = SAM_DIRLEN(".");
	dirp->d_fmt = S_IFDIR;
	dirp->d_reclen = reclen;
	dirp->d_id.ino = SAM_ROOT_INO;
	dirp->d_id.gen = SAM_ROOT_INO;
	dirp->d_namlen = 1;
	strcpy((char *)dirp->d_name, ".");

	doff += reclen;
	bufp += reclen;
	dirp = (struct sam_dirent *)bufp;	/* .. */
	reclen = SAM_DIRLEN("..");
	dirp->d_fmt = S_IFDIR;
	dirp->d_reclen = reclen;
	dirp->d_id.ino = SAM_ROOT_INO;
	dirp->d_id.gen = SAM_ROOT_INO;
	dirp->d_namlen = 2;
	strcpy((char *)dirp->d_name, "..");

	doff += reclen;
	bufp += reclen;
	dirp = (struct sam_dirent *)bufp;	/* .inodes */
	reclen = SAM_DIRLEN(".inodes");
	dirp->d_fmt = S_IFREG;
	dirp->d_id.ino = SAM_INO_INO;
	dirp->d_id.gen = SAM_INO_INO;
	dirp->d_reclen = reclen;
	dirp->d_namlen = strlen(".inodes");
	strcpy((char *)dirp->d_name, ".inodes");

	doff += reclen;
	bufp += reclen;

	dirp = (struct sam_dirent *)bufp;	/* .archive */
	reclen = SAM_DIRLEN(".archive");
	dirp->d_fmt = S_IFDIR;
	dirp->d_reclen = reclen;
	dirp->d_id.ino = SAM_ARCH_INO;
	dirp->d_id.gen = SAM_ARCH_INO;
	dirp->d_namlen = strlen(".archive");
	strcpy((char *)dirp->d_name, ".archive");

	doff += reclen;
	bufp += reclen;
	dirp = (struct sam_dirent *)bufp;	/* lost+found */
	reclen = SAM_DIRLEN("lost+found");
	dirp->d_fmt = S_IFDIR;
	dirp->d_reclen = reclen;
	dirp->d_id.ino = SAM_LOSTFOUND_INO;
	dirp->d_id.gen = SAM_LOSTFOUND_INO;
	dirp->d_namlen = strlen("lost+found");
	strcpy((char *)dirp->d_name, "lost+found");

	doff += reclen;
	bufp += reclen;
	dirp = (struct sam_dirent *)bufp;	/* .stage */
	reclen = SAM_DIRLEN(".stage");
	dirp->d_fmt = S_IFDIR;
	dirp->d_reclen = DIR_BLK - (doff + sizeof (struct sam_dirval));
	dirp->d_id.ino = SAM_STAGE_INO;
	dirp->d_id.gen = SAM_STAGE_INO;
	dirp->d_namlen = strlen(".stage");
	strcpy((char *)dirp->d_name, ".stage");

	bufp = (char *)dir_blk;
	bufp += (DIR_BLK - sizeof (struct sam_dirval));
	hp = (struct sam_dirval *)bufp;
	hp->d_version = SAM_DIR_VERSION;
	hp->d_reclen = sizeof (struct sam_dirval);
	hp->d_id.ino = SAM_ROOT_INO;
	hp->d_id.gen = SAM_ROOT_INO;
	hp->d_time = fstime;

	/*
	 * Build .inodes file for all file system types.
	 * For the "mat" object file system, build object inodes;
	 * all these object inodes do not have names.
	 */
	memset(dcp, 0, mp->mi.m_dau[dt].size[LG]);
	idp = (union sam_di_ino *)dcp;
	for (ino = 1; ino <= SAM_OBJ_MAX_MKFS_INO; ino++, idp++) {
		if (ino > SAM_MAX_MKFS_INO) {
			if (mnt_info.params.fi_type != DT_META_OBJ_TGT_SET) {
				break;
			}
			if (ino < SAM_OBJ_MIN_MKFS_INO) {
				continue;
			}
		}
		dp = (struct sam_perm_inode *)idp;
		/* current ino layout version */
		dp->di.version = ino_version;
		dp->di.uid = 0;			/* root */
		dp->di.gid = 0;			/* root */
		dp->di.access_time.tv_sec = fstime;
		dp->di.access_time.tv_nsec = 0;
		dp->di.modify_time.tv_sec = fstime;
		dp->di.modify_time.tv_nsec = 0;
		dp->di.change_time.tv_sec = fstime;
		dp->di.change_time.tv_nsec = 0;
		dp->di.creation_time = fstime;
		dp->di.id.ino = ino;
		dp->di.id.gen = ino;
		dp->di.parent_id.ino = SAM_ROOT_INO;
		dp->di.parent_id.gen = SAM_ROOT_INO;

		switch (ino) {
		case SAM_INO_INO:
			dp->di.mode = S_IFREG | S_IREAD;
			if (ninodes) {
				int res_inodes;
				offset_t ndau;

				res_inodes = SAM_MIN_USER_INO;
				ndau = (((ninodes + res_inodes) *
				    (offset_t)SAM_ISIZE) +
				    mp->mi.m_dau[dt].size[LG] - 1) /
				    mp->mi.m_dau[dt].size[LG];
				dp->di.rm.size = mp->mi.m_dau[dt].size[LG] *
				    ndau;
				dp->di.blocks = mp->mi.m_dau[dt].blocks[LG] *
				    ndau;
				dp->di.status.b.direct_map = 1;
			} else {
				dp->di.rm.size = mp->mi.m_dau[dt].size[LG] *
				    NDEXT;
				dp->di.blocks = mp->mi.m_dau[dt].blocks[LG] *
				    NDEXT;
			}
			dp->di.nlink = 1;
			dp->di.status.b.on_large = 1;
			dp->di.free_ino = SAM_MIN_USER_INO; /* Free inode */
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_ROOT_INO:
			dp->di.mode = S_IFDIR | ((S_IREAD | S_IWRITE | S_IEXEC)
			    | ((S_IREAD | S_IEXEC) >> 3)
			    | ((S_IREAD | S_IEXEC) >> 6));
			dp->di.rm.size = DIR_BLK;
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 5;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_BLK_INO:
			dp->di.mode = S_IFREG | S_IREAD;
			dp->di.rm.size = DIR_BLK;
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 1;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_HOST_INO:			/* or SAM_IOCTL_INO */
			dp->di.mode = S_IFREG | S_IREAD;
			/*
			 * Allocate enough space for a large host table.
			 * Make it a direct map file so the -R (raw)
			 * mode of samsharefs will still work.
			 */
			nhdau = (SAM_LARGE_HOSTS_TABLE_SIZE +
			    mp->mi.m_dau[dt].size[LG] - 1) /
			    mp->mi.m_dau[dt].size[LG];
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG] * nhdau;
			dp->di.status.b.direct_map = 1;
			if (hostbuf != NULL &&
			    (hostbuf->length > SAM_HOSTS_TABLE_SIZE ||
			    hostbuf->count > SAM_MAX_SMALL_SHARED_HOSTS)) {
				dp->di.rm.size = SAM_LARGE_HOSTS_TABLE_SIZE;
				if (prev_sblk_ver) {
					error(0, 0, catgets(catfd, SET, 13486,
					    "Large host table for '%s' not "
					    "supported with previous (-P) "
					    "version of file system."),
					    fs_name);
					clean_exit(2);
				}
			} else {
				/*
				 * Leave rm.size at the old 16k size
				 * for backward compatability.
				 */
				dp->di.rm.size = SAM_HOSTS_TABLE_SIZE;
			}
			dp->di.nlink = 1;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_ARCH_INO:
		case SAM_STAGE_INO:
			dp->di.mode = S_IFDIR | S_IREAD | S_IWRITE | S_IEXEC;
			dp->di.rm.size = DIR_BLK;
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 2;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_LOSTFOUND_INO:
			dp->di.mode = S_IFDIR | S_IREAD | S_IWRITE | S_IEXEC;
			dp->di.rm.size = mp->mi.m_dau[dt].size[LG];
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 2;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_SHFLOCK_INO:
			dp->di.mode = S_IFREG | S_IREAD;
			dp->di.rm.size = mp->mi.m_dau[dt].size[LG];
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 1;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
			break;
		case SAM_OBJ_OBJCTL_INO:
		case SAM_OBJ_ORPHANS_INO:
		case SAM_OBJ_LBLK_INO:
		case SAM_OBJ_SBLK_INO:
		case SAM_OBJ_PAR_INO:
		case SAM_OBJ_PAR0_INO:
		case SAM_OBJ_ROOT_INO:
			dp->di.mode = S_IFREG | S_IREAD | S_IWRITE;
			dp->di.rm.size = mp->mi.m_dau[dt].size[LG];
			dp->di.blocks = mp->mi.m_dau[dt].blocks[LG];
			dp->di.nlink = 1;
			dp->di.status.b.on_large = 1;
			if (mm_count) {
				dp->di.status.b.meta = 1;
				dp->di.unit = mm_ord;
			}
		default:
			break;
		}

		if (dp->di.status.b.direct_map) {
			int ndau;

			if (ino == SAM_HOST_INO) {
				ndau = nhdau;
			} else {
				ndau = dp->di.rm.size /
				    mp->mi.m_dau[dt].size[LG];

				if (ndau *
				    (offset_t)mp->mi.m_dau[dt].size[LG] !=
				    dp->di.rm.size) {
						error(0, 0, catgets(catfd, SET,
						    13402,
						    "failed to allocate %d "
						    "inodes on eq %d"),
						    ninodes,
						    devp->device[mm_ord].eq);
						clean_exit(1);
				}
			}
			if ((fblk = getdau(mm_ord, ndau, 0, 1)) < 0) {
				if (ino == SAM_HOST_INO) {
					error(0, 0,
					    "Host table allocation "
					    "failed for (%s)", fs_name);
					clean_exit(1);
				} else {
					error(0, errno, catgets(catfd, SET,
					    13402,
					    "failed to allocate %d "
					    "inodes on eq %d"),
					    ninodes, devp->device[mm_ord].eq);
					clean_exit(1);
				}
			}
			dp->di.extent[0] = (int)(fblk >> fblk_to_extent);
			dp->di.extent_ord[0] = mm_ord;
			dp->di.extent[1] = mp->mi.m_dau[dt].kblocks[LG] * ndau;
			dp->di.lextent = 1;
		} else {
			if (ino == SAM_INO_INO) {
				length = NDEXT;
			} else {
				length = 1;
			}
			for (i = 0; i < length; i++) {
				fblk = getdau(mm_ord, 1, 0, 1);
				dp->di.extent[i] = (int)(fblk >>
				    fblk_to_extent);
				dp->di.extent_ord[i] = mm_ord;
			}
		}
	}

	/*
	 */

	/*
	 * Write dir/root/map data to disk
	 */
	idp = (union sam_di_ino *)dcp;
	for (ino = 1; ino <= SAM_OBJ_MAX_MKFS_INO; ino++, idp++) {
		dp = (struct sam_perm_inode *)idp;
		if (dp->di.id.ino == 0) {
			continue;
		}
		switch (ino) {
		case SAM_INO_INO:
			sblock.info.sb.inodes = (dp->di.extent[0] <<
			    fblk_to_extent);
			bufp = (char *)dcp;
			length = mp->mi.m_dau[dt].kblocks[LG];
			break;
		case SAM_ROOT_INO:
			bufp = (char *)dir_blk;
			length = DIR_LOG_BLOCK;
			break;
		case SAM_BLK_INO:
			memset((char *)ibufp, 0, mp->mi.m_dau[dt].size[LG]);
			inp = (uint_t *)ibufp;
			memset(inp, (int)0xffffffff, (sizeof (uint_t) * 2));
			bufp = (char *)ibufp;
			length = mp->mi.m_dau[dt].kblocks[LG];
			break;
		case SAM_HOST_INO:	/* or SAM_IOCTL_INO */
			length = dp->di.rm.size/SAM_DEV_BSIZE;
			if (hostbuf != NULL) {
				if (dp->di.rm.size > SAM_HOSTS_TABLE_SIZE) {
					/*
					 * Creating a large hosts table.
					 */
					sblock.info.sb.opt_mask |=
					    SBLK_OPTV1_LG_HOSTS;
					length = SAM_LARGE_HOSTS_TABLE_SIZE /
					    SAM_DEV_BSIZE;
				}
				sblock.info.sb.hosts =
				    (dp->di.extent[0] << fblk_to_extent);
				sblock.info.sb.hosts_ord = dp->di.extent_ord[0];
				bufp = (char *)hostbuf;
				break;

			} else if (shared) {
				/*
				 * No hosts table.
				 */
				error(0, 0, "No host table for (%s)", fs_name);
				clean_exit(1);
			}
			/* NOTREACHED */
		case SAM_ARCH_INO:
		case SAM_STAGE_INO:
			length = DIR_LOG_BLOCK;
			ep = (struct sam_empty_dir *)dir_blk;
			ep->dot.d_id.ino = ino;
			ep->dot.d_id.gen = ino;
			ep->dotdot.d_reclen = DIR_BLK -
			    (sizeof (struct sam_dirent) +
			    sizeof (struct sam_dirval));
			memset((dir_blk + sizeof (struct sam_empty_dir)), 0,
			    (DIR_BLK - sizeof (struct sam_empty_dir)));
			bufp = (char *)dir_blk;
			bufp += DIR_BLK - sizeof (struct sam_dirval);
			hp = (struct sam_dirval *)bufp;
			hp->d_version = SAM_DIR_VERSION;
			hp->d_reclen = sizeof (struct sam_dirval);
			hp->d_id.ino = ino;
			hp->d_id.gen = ino;
			hp->d_time = fstime;
			bufp = (char *)dir_blk;
			break;
		case SAM_LOSTFOUND_INO: {
			int ib =  mp->mi.m_dau[dt].size[LG]/DIR_BLK;
			length = mp->mi.m_dau[dt].kblocks[LG];
			memset((char *)ibufp, 0, mp->mi.m_dau[dt].size[LG]);
			(void *) memcpy((char *)ibufp, (char *)dir_blk,
			    sizeof (struct sam_empty_dir));
			ep = (struct sam_empty_dir *)ibufp;
			ep->dot.d_id.ino = SAM_LOSTFOUND_INO;
			ep->dot.d_id.gen = SAM_LOSTFOUND_INO;
			ep->dotdot.d_reclen = DIR_BLK -
			    (sizeof (struct sam_dirent) +
			    sizeof (struct sam_dirval));
			bufp = (char *)ibufp;
			while (ib) {
				bufp += DIR_BLK - sizeof (struct sam_dirval);
				hp = (struct sam_dirval *)bufp;
				hp->d_version = SAM_DIR_VERSION;
				hp->d_reclen = sizeof (struct sam_dirval);
				hp->d_id.ino = SAM_LOSTFOUND_INO;
				hp->d_id.gen = SAM_LOSTFOUND_INO;
				hp->d_time = fstime;
				bufp += sizeof (struct sam_dirval);
				ib--;
				if (ib == 0) {
					break;
				}
				hp = (struct sam_dirval *)bufp;
				hp->d_reclen = DIR_BLK -
				    sizeof (struct sam_dirval);
			}
			bufp = (char *)ibufp;
			}
			break;
		case SAM_SHFLOCK_INO:
		case SAM_OBJ_OBJCTL_INO:
		case SAM_OBJ_ORPHANS_INO:
		case SAM_OBJ_LBLK_INO:
		case SAM_OBJ_SBLK_INO:
		case SAM_OBJ_PAR_INO:
		case SAM_OBJ_PAR0_INO:
		case SAM_OBJ_ROOT_INO:
			memset((char *)ibufp, 0, mp->mi.m_dau[dt].size[LG]);
			bufp = (char *)ibufp;
			length = mp->mi.m_dau[dt].kblocks[LG];
			break;
		default:
			err = TRUE;
			break;
		}

		fblk = (dp->di.extent[0] << fblk_to_extent);
		if (d_write(&devp->device[mm_ord], (char *)bufp, length,
		    fblk)) {
			error(0, 0, "%s write failed on (%s)", files[ino],
			    devp->device[0].eq_name);
			err = TRUE;
		}
		if (ino == SAM_HOST_INO) {
			free(hostbuf);
			hostbuf = NULL;
		}
	}

	/*
	 * Clear .inodes blocks.
	 */
	idp = (union sam_di_ino *)dcp;
	dp = (struct sam_perm_inode *)idp;
	memset((char *)ibufp, 0, mp->mi.m_dau[dt].size[LG]);
	bufp = (char *)ibufp;
	length = mp->mi.m_dau[dt].kblocks[LG];
	if (dp->di.status.b.direct_map) {
		fblk = (dp->di.extent[0] << fblk_to_extent) + length;
		for (i = 1; i < (dp->di.extent[1] / length); i++) {
			if (d_write(&devp->device[mm_ord], (char *)bufp,
			    length, fblk)) {
				error(0, 0, "%s write failed on (%s)",
				    files[ino],
				    devp->device[0].eq_name);
				err = TRUE;
			}
			fblk += length;
		}
	} else {
		for (i = 1; i < NDEXT; i++) {
			fblk = (dp->di.extent[i] << fblk_to_extent);
			if (d_write(&devp->device[mm_ord], (char *)bufp,
			    length, fblk)) {
				error(0, 0, "%s write failed on (%s)",
				    files[ino],
				    devp->device[0].eq_name);
				err = TRUE;
			}
		}
	}

	/* save first 8 inodes used in possible sammkfs restore (-r). */
	(void *) memcpy(first_sm_blk, dcp, mp->mi.m_dau[dt].size[SM]);
	return (err);
}


#ifdef	DEBUG
void
debug_print(int idx)
{
	struct devlist *dp, *mdp = NULL;
	struct sam_sblk slsblk;
	uint_t *wptr;
	uint_t *ptr1, *ptr2, *ptr3, *ptr4;
	uint_t lptr1, lptr2, lptr3, lptr4;
	int ord, i, ii;
	int blocks, blocks_per_read, force_print;
	sam_daddr_t bn = 0;
	int dt;
	int err = 0;

	dp = &devp->device[idx];
	if (is_osd_group(dp->type)) {
		if ((read_object(fs_name, dp->oh, idx,
		    SAM_OBJ_SBLK_INO, (char *)&slsblk,
		    0, SAM_DEV_BSIZE))) {
			err = 1;
		}
	} else {
		if (d_read(dp, (char *)&slsblk, 1, SUPERBLK)) {
			err = 1;
		}
	}
	if (err) {
		error(0, 0, catgets(catfd, SET, 2439,
		    "superblock read failed on eq (%d)"), dp->eq);
		return;
	}
	if (strncmp(slsblk.info.sb.name, SAMFS_SB_NAME_STR,
	    sizeof (slsblk.info.sb.name)) ||
	    (slsblk.info.sb.magic != SAM_MAGIC_V1 &&
	    slsblk.info.sb.magic != SAM_MAGIC_V2 &&
	    slsblk.info.sb.magic != SAM_MAGIC_V2A) ||
	    slsblk.info.sb.init != sblock.info.sb.init ||
	    strncmp(slsblk.info.sb.fs_name, sblock.info.sb.fs_name,
	    sizeof (sblock.info.sb.fs_name))) {
		printf("no FS superblock on index %d\n", idx);
		return;
	}
	ord = slsblk.info.sb.ord;
	printf("\nord = %3d ------------------------------------------\n",
	    ord);
	if (dp->type == DT_META) {
		dt = MM;
	} else {
		dt = DD;
	}
	/* no. of bits */
	blocks = sblock.eq[ord].fs.dau_size * SM_BLKCNT(mp, dt);
	blocks = (blocks + 127) / 128;
	blocks_per_read = SAM_DEV_BSIZE/32;
	for (i = 0; i < fs_count; i++) {
		dp = &devp->device[i];
		if (is_osd_group(dp->type)) {
			if ((read_object(fs_name, dp->oh, i, SAM_OBJ_SBLK_INO,
			    (char *)&slsblk, 0, SAM_DEV_BSIZE))) {
				err = 1;
			}
		} else {
			if (d_read(dp, (char *)&slsblk, 1, SUPERBLK)) {
				err = 1;
			}
		}
		if (err) {
			error(0, 0, catgets(catfd, SET, 2439,
			    "superblock read failed on eq (%d)"), dp->eq);
			return;
		}
		if (strncmp(slsblk.info.sb.name, SAMFS_SB_NAME_STR,
		    sizeof (slsblk.info.sb.name)) ||
		    (slsblk.info.sb.magic != SAM_MAGIC_V1 &&
		    slsblk.info.sb.magic != SAM_MAGIC_V2 &&
		    slsblk.info.sb.magic != SAM_MAGIC_V2A) ||
		    slsblk.info.sb.init != sblock.info.sb.init ||
		    strncmp(slsblk.info.sb.fs_name, sblock.info.sb.fs_name,
		    sizeof (sblock.info.sb.fs_name))) {
			continue;
		}
		if (slsblk.info.sb.ord == sblock.eq[ord].fs.mm_ord) {
			mdp = dp;
			break;
		}
	}
	if (!mdp) {
		error(0, 0, catgets(catfd, SET, 13469,
		    "mm_ord device not found for index %d/ord %d\n"), idx,
		    ord);
		return;
	}

	/*
	 * Print allocation maps; skip object slices which have
	 * no allocation maps.
	 *
	 * mdp now points at devp index of the slice holding
	 * idx's metadata slice.
	 */
	for (ii = 0; ii < sblock.eq[ord].fs.l_allocmap; ii++) {
		if (is_osd_group(mdp->type)) {
			continue;
		}
		if (d_read(mdp, (char *)dcp, 1,
		    (sam_daddr_t)(sblock.eq[ord].fs.allocmap + ii))) {
				error(0, 0,
				    catgets(catfd, SET, 798,
				    "Dau map read failed on eq %d"),
				    mdp->eq);
				clean_exit(1);
		}
		wptr = (uint_t *)dcp;
		for (i = 0; i < blocks_per_read; i++) {
			ptr1 = wptr++;
			ptr2 = wptr++;
			ptr3 = wptr++;
			ptr4 = wptr++;
			blocks -= 128;
			force_print = 0;
			if ((i == 0 && ii == 0) || blocks <= 0) {
				force_print = 1;
			} else if ((lptr1 != *ptr1) || (lptr2 != *ptr2) ||
			    (lptr3 != *ptr3) || (lptr4 != *ptr4)) {
				force_print = 1;
			}
			if (force_print) {
				printf("bn=%.8llx   %.8x, %.8x, %.8x, %.8x\n",
				    (sam_offset_t)bn, *ptr1, *ptr2, *ptr3,
				    *ptr4);
				lptr1 = *ptr1; lptr2 = *ptr2;
				lptr3 = *ptr3; lptr4 = *ptr4;
			}
			bn += ((32*4) / SM_BLKCNT(mp, dt)) *
			    mp->mi.m_dau[dt].kblocks[LG];
			if (blocks <= 0) {
				return;
			}
		}
	}
}
#endif	/* DEBUG */


/*
 * ----- ask - ask the user a y/n question
 *
 * Emit the message provided, and await a y/n response.
 * Allow a default y/n value to be passed, specifying
 * whether y or n is to be returned for a non-y/n answer.
 */
int
ask(char *msg, char def)
{
	int i, n;
	int defret = (def == 'y' ? TRUE : FALSE);
	char answ[120];

	printf("%s", msg);
	fflush(stdout);
	n = read(0, answ, sizeof (answ));
	for (i = 0; i < n; i++) {
		if (answ[i] == ' ' || answ[i] == '\t') {
			continue;
		}
		if (tolower(answ[i]) == 'n') {
			return (FALSE);
		}
		if (tolower(answ[i]) == 'y') {
			return (TRUE);
		}
		return (defret);
	}
	return (defret);
}


/*
 * ----- clear_obj_label - Clear labels if shared "mb" file system.
 *  The size must be set for the sblk and shared label.
 *  An object cannot be read past the end of object.
 */

static int			/* 1 if error, 0 if successful */
clear_obj_label(
	struct d_list *devp)
{
	int ord;
	struct devlist *dp;
	sam_label_blk_t label;

	memset((char *)&label, 0, sizeof (sam_label_blk_t));
	dp = (struct devlist *)devp;
	for (ord = 0; ord < mnt_info.params.fs_count; ord++, dp++) {
		if (dp->type != DT_META) {
			break;
		}
	}
	if (ord == 0 || ord == mnt_info.params.fs_count ||
	    !is_osd_group(dp->type)) {
		error(0, 0, catgets(catfd, SET, 13481,
		    "FS %s: Filesystem has no object data partition"),
		    mnt_info.params.fi_name);
		return (1);
	}
	if ((create_object(mnt_info.params.fi_name, dp->oh, ord,
	    SAM_OBJ_LBLK_INO))) {
		error(0, 0, catgets(catfd, SET, 13482,
		    "FS %s: Cannot create object inode %d"),
		    mnt_info.params.fi_name, SAM_OBJ_LBLK_INO);
		return (1);
	}
	if ((write_object(mnt_info.params.fi_name, dp->oh, ord,
	    SAM_OBJ_LBLK_INO, (char *)&label, 0, sizeof (sam_label_blk_t)))) {
		error(0, 0,
		    catgets(catfd, SET, 2724,
		    "Label write failed for %s on eq %d"),
		    mnt_info.params.fi_name, dp->eq);
		return (1);
	}
	return (0);
}


/*
 * ----- print_sblk - print superblock information.
 *
 *	Information used is from passed sam_sblk pointer (sp).
 *	If ordering == ORDER_DISK, then the FS devices are opened
 *	to obtain each device's ordinal.  They are printed in
 *	the devp table ordering (but with the correct ordinal
 *	and device names).  If ordering == ORDER_MEM, then the
 *	sblk passed is assumed to be correctly ordered (and
 *	the devices might not yet be written).
 */

static void
print_sblk(struct sam_sblk *sp, struct d_list *devp, int ordering)
{
	struct sam_sbinfo *sblkp;
	int i;
	char ver_str[20];
	struct devlist *dp;
	time_t init_time;
	char timebuf[64];
	int err = 0;

	sblkp = (struct sam_sbinfo *)sp;
	if (sblkp->magic == SAM_MAGIC_V1) {
		sprintf(&ver_str[0], "%s", "1");
	} else if (sblkp->magic == SAM_MAGIC_V2) {
		sprintf(&ver_str[0], "%s", "2");
	} else if (sblkp->magic == SAM_MAGIC_V2A) {
		sprintf(&ver_str[0], "%s", "2A");
	}
	printf(catgets(catfd, SET, 13466,
	    "name:     %s       version:     %s%10s"),
	    sblkp->fs_name, &ver_str[0], sblkp->hosts == 0 ? "" :
	    catgets(catfd, SET, 13467, "    shared"));
	if (sblkp->opt_mask_ver && sblkp->opt_mask) {
		printf(catgets(catfd, SET, 13468, "    opt mask 0x%x"),
		    sblkp->opt_mask);
	}
	printf("\n");
	init_time = sblkp->init;
	cftime(&timebuf[0], "%C\n", &init_time);
	printf(catgets(catfd, SET, 2499, "time:     %s"), &timebuf[0]);
	if (sblkp->opt_mask_ver == SBLK_OPT_VER1) {
		if (sblkp->opt_mask & SBLK_OPTV1_WORM) {
			printf(catgets(catfd, SET, 13474,
			    "WORM (strict) files present\n"));
		} else if (sblkp->opt_mask & SBLK_OPTV1_WORM_LITE) {
			printf(catgets(catfd, SET, 13475,
			    "WORM (lite) files present\n"));
		} else if (sblkp->opt_mask & SBLK_OPTV1_WORM_EMUL) {
			printf(catgets(catfd, SET, 13476,
			    "WORM (Emulation mode) files present\n"));
		} else if (sblkp->opt_mask & SBLK_OPTV1_EMUL_LITE) {
			printf(catgets(catfd, SET, 13477,
			    "WORM (Emulation lite mode) files present\n"));
		}
		if (sblkp->opt_mask & SBLK_OPTV1_CONV_WORMV2) {
			printf(catgets(catfd, SET, 13483,
			    "WORM V2 files present\n"));
		}
		if (sblkp->opt_mask & SBLK_OPTV1_XATTR) {
			printf(catgets(catfd, SET, 13485,
			    "Extended attribute files present\n"));
		}
		if (sblkp->opt_features & SBLK_FV1_MAPS_ALIGNED) {
			printf(catgets(catfd, SET, 13484,
			    "feature:  Aligned Maps\n"));
		}
	}
	printf(catgets(catfd, SET, 769, "count:    %d\n"), sblkp->fs_count);
	printf(catgets(catfd, SET, 13417,
	    "capacity:      %.16llx          DAU:      %5d\n"),
	    (offset_t)sblkp->capacity, sblkp->dau_blks[LG]);
	printf(catgets(catfd, SET, 13418, "space:         %.16llx\n"),
	    (offset_t)sblkp->space);
	if (sblkp->mm_count) {
		printf(catgets(catfd, SET, 13419,
		    "meta capacity: %.16llx          meta DAU: %5d\n"),
		    (offset_t)sblkp->mm_capacity, sblkp->mm_blks[LG]);
		printf(catgets(catfd, SET, 13420, "meta space:    %.16llx\n"),
		    (offset_t)sblkp->mm_space);
	}

	printf(catgets(catfd, SET, 1887,
	    "ord  eq           capacity              space   device\n"));
	switch (ordering) {
	case ORDER_MEM:
		for (i = 0; i < sblkp->fs_count; i++) {
			printf("%3d %3d   %.16llx   %.16llx   %s\n", i,
			    sp->eq[i].fs.eq,
			    (offset_t)sp->eq[i].fs.capacity,
			    (offset_t)sp->eq[i].fs.space,
			    devp->device[i].eq_name);
		}
		break;

	case ORDER_DISK:
		for (i = 0, dp = (struct devlist *)devp; i < fs_count;
		    i++, dp++) {
			int size = howmany(sblkp->sblk_size,
			    SAM_DEV_BSIZE);
			struct sam_sblk slsblk;
			struct sam_sbord *sbord;

			if (is_osd_group(dp->type)) {
				if (dp->oh == 0) {
					goto nullentry;
				}
				if ((read_object(fs_name, dp->oh, i,
				    SAM_OBJ_SBLK_INO, (char *)&slsblk,
				    0, size*SAM_DEV_BSIZE))) {
					err = 1;
				}
			} else {
				if (dp->fd <= 0) {
					goto nullentry;
				}
				if (d_read(dp, (char *)&slsblk, size,
				    SUPERBLK)) {
					err = 1;
				}
			}
			if (err) {
				error(0, 0, catgets(catfd, SET, 2439,
				    "superblock read failed on eq (%d)"),
				    dp->eq);
				goto nullentry;
			}
			if (strncmp(slsblk.info.sb.name, SAMFS_SB_NAME_STR,
			    sizeof (slsblk.info.sb.name)) ||
			    (slsblk.info.sb.magic != SAM_MAGIC_V1 &&
			    slsblk.info.sb.magic != SAM_MAGIC_V2 &&
			    slsblk.info.sb.magic != SAM_MAGIC_V2A) ||
			    slsblk.info.sb.init != sp->info.sb.init ||
			    strncmp(slsblk.info.sb.fs_name, sblkp->fs_name,
			    sizeof (sblkp->fs_name))) {
				goto nullentry;
			}
			sbord = &slsblk.eq[slsblk.info.sb.ord].fs;
			printf("%3d %3d   %.16llx   %.16llx   %s %s\n",
			    slsblk.info.sb.ord, sbord->eq, sbord->capacity,
			    sbord->space, dp->eq_name,
			    dev_state[sbord->state]);
			continue;
nullentry:
			printf("  -   -                  -"
			    "                  -   %s\n", dp->eq_name);
		}
	}
}


/*
 * ----- read_existing_sblk - reads super blocks from specified devices.
 *	read_existing_sblk reads and validates super blocks from the device
 *	list, looking for the ordinal zero superblock. If none is found, an
 *	error message is issued and the program exits.
 *
 *	Used by grow_fs and show_fs.
 */

static void
read_existing_sblk(struct d_list *devp, struct sam_sblk *sblk, int sblk_size)
{
	int ord;
	struct devlist *dp;
	int err = 0;

	/*
	 * Find ordinal 0 for existing filesystem fs_name
	 */
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		if (is_osd_group(dp->type)) {
			if ((read_object(fs_name, dp->oh, ord, SAM_OBJ_SBLK_INO,
			    (char *)sblk, 0, (sblk_size * SAM_DEV_BSIZE)))) {
				err = 1;
			}
		} else {
			if (d_read(dp, (char *)sblk, sblk_size, SUPERBLK)) {
				err = 1;
			}
		}
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 2439,
			    "superblock read failed on eq (%d)"), dp->eq);
			clean_exit(1);
		}
		if (strncmp(sblk->info.sb.fs_name, fs_name,
		    sizeof (uname_t)) == 0) {
			if (sblk->info.sb.ord == 0) {
				break;
			}
		}
	}

	/*
	 * Some error checking on the superblock found.
	 */
	if (sblk->info.sb.ord != 0 ||
	    strncmp(sblk->info.sb.fs_name, fs_name, sizeof (uname_t)) != 0 ||
	    strncmp(sblk->info.sb.name, SAMFS_SB_NAME_STR,
	    sizeof (sblk->info.sb.name)) != 0) {
		error(0, 0,
		    catgets(catfd, SET, 583,
		    "Cannot find ordinal 0 for filesystem %s"), fs_name);
		clean_exit(1);
	}

	switch (sblk->info.sb.magic) {
	case SAM_MAGIC_V1:
		update_sblk_to_40(sblk, fs_count, mm_count);
		break;

	case SAM_MAGIC_V2:
		break;

	case SAM_MAGIC_V2A:
		break;

	case SAM_MAGIC_V1_RE:
	case SAM_MAGIC_V2_RE:
	case SAM_MAGIC_V2A_RE:
		error(0, 0,
		    catgets(catfd, SET, 13472,
		    "FS %s built with non-native byte order\n"), fs_name);
		clean_exit(1);
		/* NOTREACHED */

	default:
		error(0, 0,
		    catgets(catfd, SET, 13473,
		    "FS %s: not a recognized QFS file system type\n"), fs_name);
		clean_exit(1);
	}

	sam_set_dau(&mp->mi.m_dau[DD], sblk->info.sb.dau_blks[SM],
	    sblk->info.sb.dau_blks[LG]);
	sam_set_dau(&mp->mi.m_dau[MM], sblk->info.sb.mm_blks[SM],
	    sblk->info.sb.mm_blks[LG]);
	get_mem(1);
}


/*
 * ----- check_fs_devices() - Check the devices which make up the file system.
 *	check_fs_devices() verifies LUN size limits and attempts to check if
 *	a SAM-QFS LUN is being reused in an inappropriate way.
 *
 */
static void
check_fs_devices(char *fs_name)
{
	int ord, num_sblks, num_sblks_worm;
	int abort_on_error;
	int strict_worm = 0, worm_lite = 0, worm_v2 = 0;
	struct devlist *dp;
	struct sam_sbinfo *sblkp;
	char sb[SAM_DEV_BSIZE];
	int err = 0;

	check_lun_limits(fs_name);

	sblkp = (struct sam_sbinfo *)sb;
	num_sblks = num_sblks_worm = 0;
	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		if (is_osd_group(dp->type)) {
			continue;		/* XXX --  Skip for objects */
		} else {
			if (d_read(dp, (char *)sblkp, 1, SUPERBLK)) {
				err = 1;
			}
		}
		if (err) {
			error(0, 0,
			    catgets(catfd, SET, 2439,
			    "superblock read failed on eq (%d)"), dp->eq);
			clean_exit(1);
		}

		if ((strncmp(sblkp->name,
		    SAMFS_SB_NAME_STR, sizeof (sblkp->name)) == 0) &&
		    (strncmp(sblkp->fs_name,
		    fs_name, sizeof (sblkp->fs_name)) == 0)) {
			num_sblks++;
			if (sblkp->opt_mask_ver == 1 &&
			    ((sblkp->opt_mask &
			    (SBLK_OPTV1_STRICT_WORM |
			    SBLK_OPTV1_LITE_WORM |
			    SBLK_OPTV1_CONV_WORMV2)) != 0)) {
				if ((sblkp->opt_mask &
				    SBLK_OPTV1_STRICT_WORM) != 0) {
					strict_worm = 1;
				} else if ((sblkp->opt_mask &
				    SBLK_OPTV1_LITE_WORM) != 0) {
					worm_lite = 1;
				} else if ((sblkp->opt_mask &
				    SBLK_OPTV1_CONV_WORMV2) != 0) {
					worm_v2 = 1;
				}
				if (!growfs || (ord >= old_count)) {
					num_sblks_worm++;
				}
			}
		}
	}
	if (num_sblks_worm > 0) {
		/*
		 * If at least one worm slice found.
		 */

#ifdef	DEBUG
		abort_on_error = (force) ? 0 : 1;
#else
		abort_on_error = 1;
#endif	/* DEBUG */
		if (strict_worm || worm_v2) {
			error(0, 0, catgets(catfd, SET, 13465,
			    "Write Once Read Many partitions detected "
			    "<%d of %d>"),
			    num_sblks_worm, num_sblks);
			if (abort_on_error) {
				clean_exit(abort_on_error);
			}
		} else if (worm_lite) {
			printf(catgets(catfd, SET, 13465,
			    "Write Once Read Many partitions detected <%d "
			    "of %d>"),
			    num_sblks_worm, num_sblks);
			printf("\n");
		}
	}
}

/*
 * ----- check_lun_limits() - Check if a partition and/or LUN is
 * greater than any actual or implied LUN size limit. Also, warn
 * if a limit is exceeded.
 */
static void
check_lun_limits(char *fs_name)
{
	int ord;
	struct devlist *dp;
	sam_offset_t size_in_bytes;

	for (ord = 0, dp = (struct devlist *)devp; ord < fs_count;
	    ord++, dp++) {
		if (is_osd_group(dp->type)) {
			continue;
		}
		size_in_bytes = dp->size * (sam_offset_t)DEV_BSIZE;
		if (size_in_bytes >= SAM_1TB_LIMIT) {
			lun_ge_1tb++;
		}
		if (size_in_bytes >= SAM_16TB_LIMIT) {
			lun_ge_16tb++;
			printf(catgets(catfd, SET, 13456,
			    "%s: %s: One or more partitions exceeds %d "
			    "TB in size\n"),
			    program_name, fs_name, 16);
			printf(catgets(catfd, SET, 13457,
			    "%s: SAM-QFS allows a maximum of 16 TB per "
			    "partition\n"), program_name);
			clean_exit(1);
		}
	}
	if (lun_ge_1tb) {
		printf(catgets(catfd, SET, 13456,
		    "%s: %s: One or more partitions exceeds %d TB in size\n"),
		    program_name, fs_name, 1);
		printf(catgets(catfd, SET, 13458,
		    "%s: file system %s will not mount on 32 bit Solaris "
		    "and\n"),
		    program_name, fs_name);
		printf(catgets(catfd, SET, 13459,
		    "%s: some earlier versions of Solaris\n"),
		    program_name);

	}
}


/*
 * ----- print_version_warning - Prints warning text about building
 * current or previous file system version.
 */

static void
print_version_warning()
{
	char *man_string;

	if (prev_sblk_ver) {
		man_string = catgets(catfd, SET, 13441,
		    "Sun QFS, Sun SAM-FS, and Sun SAM-QFS Installation and "
		    "Configuration Guide");
		printf(catgets(catfd, SET, 13436,
		    "Creating an old format file system disallows some file "
		    " system features\n"));
		printf(catgets(catfd, SET, 13437,
		    "Please see the '%s'\n for a list of the affected "
		    "features.\n"), man_string);
	} else {
		printf(catgets(catfd, SET, 13438,
		    "Warning: Creating a new file system prevents use with "
		    "4.6 or earlier releases.\n"));
		printf(catgets(catfd, SET, 13439,
		    "Use the -P option on sammkfs to create a 4.6 compatible "
		    "file system.\n"));
	}
	printf("\n");
}
