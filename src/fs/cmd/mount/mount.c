/*
 *  mount.c -
 *
 *    Mount_samfs function.
 *    Called by the mount program when a request is made to mount a
 *    device as a sam file system.
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

#pragma ident "$Revision: 1.82 $"

/* ANSI C headers. */
#include <errno.h>
#include <syslog.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

/* OS headers. */
#include <sys/mount.h>
#ifdef linux
#include <mntent.h>
#endif /* linux */
#ifdef sun
#include <sys/mnttab.h>
#endif /* sun */

/* SAM-FS headers. */
#include <sam/types.h>
#include <sam/param.h>
#include <sam/custmsg.h>
#include <sam/exit.h>
#include <sam/mount.h>
#include <sam/lib.h>
#include <sam/syscall.h>
#include <utility.h>
#ifdef sun
#include <sam/fioctl.h>
#endif /* sun */

/* Private functions. */
static void FatalError(int ErrNum, int MsgNum, ...);
static int retry_mount(struct sam_fs_info *mp, struct sam_share_arg *fsp,
	char *fset_name, char *mnt_point, struct sam_mount_info *mount_info);

#ifdef sun
static int checkisjournal(char *mp);
#endif /* sun */

#ifdef CMD_UPDATE_MNTTAB
static void add_mnttab_entry(char *file, char *mnt_point, char *mnt_opts,
	int freq, int pass);
#endif /* CMD_UPDATE_MNTTAB */

#include <sam/mount.hc>

char *program_name;


int
main(int argc, char **argv)
{
	struct sam_share_arg fsshare, *fsp = NULL;
	struct sam_mount_info mount_info;
	struct sam_fs_info *mp;
	struct stat file_stat;
	boolean_t verbose = FALSE;
	char	*mnt_point = NULL;
	char	*fset_name = NULL;
	char	*msg;
	int		errflag = 0;
	int		c;
#ifdef linux
	char *argstr = "o:nv";
	generic_mount_info_t gmi;
#endif /* linux */
#ifdef sun
	char *argstr = "o:OrmV";
#endif /* sun */
	char *usage_str;
#ifdef CMD_UPDATE_MNTTAB
	boolean_t update_mnttab = TRUE;
	char	*mnt_opts = NULL;
#endif /* CMD_UPDATE_MNTTAB */

#ifdef linux
	program_name = "mount -t samfs";
	usage_str = "Usage: %s fs_name mount_point [-v] [-n] [-r] "
	    "-o shared [-o samfs_options]\n";
#endif /* linux */
#ifdef sun
	program_name = "mount_samfs";
	usage_str = GetCustMsg(17309);
	/*
	 * Usage: %s [-F samfs] [generic options] [-o samfs_options]
	 *		special_file mount_point.
	 */
#endif /* sun */
	if (argc < 3) {
		fprintf(stderr, usage_str, program_name);
		exit(EXIT_USAGE);
	}

	/*
	 * Set the file system name for this mount.
	 */
#ifdef linux
	fset_name = argv[1];
	mnt_point = argv[2];
#endif /* linux */
#ifdef sun
	fset_name = argv[argc - 2];
#endif /* sun */
	if (strlen(fset_name) >= sizeof (mp->fi_name)) {
		fprintf(stderr, "%s: ", program_name);
		/* special_file name cannot exceed %d characters */
		fprintf(stderr, GetCustMsg(17301), (int)sizeof (uname_t) - 1);
		fprintf(stderr, "\n        %s\n", fset_name);
		exit(EXIT_USAGE);
	}

	memset(&mount_info, 0, sizeof (struct sam_mount_info));
	mp = &mount_info.params;
	strcpy(mp->fi_name, fset_name);

	/*
	 * Check filesystem - will configure it if necessary.
	 */
	ChkFs();

	/*
	 * Get the mount parameters and devices from the filesystem.
	 * The filesystem daemon fsd must have already
	 * executed and set the mount parameters (SC_setmount).
	 */

	if (GetFsMountDefs(mp->fi_name, &mount_info) < 0) {
		FatalError(errno, 620, mp->fi_name);
	}

	/*
	 * Set the mount parameters from the mount command or /etc/vfstab.
	 * These parameter take precedence over the defaults and the samfs.cmd
	 * file.
	 */
	while ((c = getopt(argc, argv, argstr)) != -1) {
		switch (c) {
#ifdef sun
		case 'm':
#endif /* sun */
#ifdef linux
		case 'n':
#endif /* linux */
#ifdef CMD_UPDATE_MNTTAB
			update_mnttab = FALSE;
#else
			mp->fi_mflag |= MS_NOMNTTAB;
#endif /* CMD_UPDATE_MNTTAB */
			break;

		case 'r':
			mp->fi_mflag |= MS_RDONLY;
			break;

		case 'o': {
			char	*subopt;
			char	*str;

			str = optarg;
#ifdef CMD_UPDATE_MNTTAB
			mnt_opts = strdup(optarg);
#endif /* CMD_UPDATE_MNTTAB */
			while ((subopt = strtok(str, ",")) != NULL) {
				char *value, value1[32];

				str = NULL;
				if (strcmp("rw", subopt) == 0) {
					continue; /* Skip '-o rw' (default) */
				}
				if (strcmp("auto", subopt) == 0) {
					continue;   /* Ignore '-o auto' */
				}
				if (strcmp("noauto", subopt) == 0) {
					continue;   /* Ignore '-o noauto' */
				}
				if ((value = strchr(subopt, '=')) != NULL) {
					*value++ = '\0';

					if (strlen(value) <= sizeof (value1)) {
						strcpy(value1, value);
					} else {
						fprintf(stderr,
						    GetCustMsg(17312),
						    program_name);
						exit(EXIT_USAGE);
					}
				} else {
					value1[0] = '\0';
				}
				if (SetFieldValue(mp, MountParams,
				    subopt, value1, NULL) != 0) {
					if (errno == ENOENT) {
						/*
						 * Ignore unrecognized mount
						 * options (like UFS does).
						 * SetFieldValue() issued
						 * "Invalid option" message.
						 */
						continue;
					}
					errflag++;
				}
			}
			}
			break;

#ifdef sun
		case 'O':
			mp->fi_mflag |= MS_OVERLAY;
			break;
#endif /* sun */

#ifdef linux
		case 'v':
#endif /* linux */
#ifdef sun
		case 'V':
#endif /* sun */
			verbose = TRUE;
			break;

		default:
			errflag++;
			break;
		}
	}

	if (errflag) {
		fprintf(stderr, usage_str, program_name);
		exit(EXIT_USAGE);
	}
#ifdef linux
	/* FS must be mounted shared on Linux. */
	if ((mp->fi_config & MT_SHARED_MO) == 0) {
		fprintf(stderr, usage_str, program_name);
		exit(EXIT_USAGE);
	}
#endif /* linux */

#ifdef sun
	/* fset_name must be next-to-last argument. */
	if (optind != (argc - 2)) {
		fprintf(stderr, usage_str, program_name);
		exit(EXIT_USAGE);
	}

	/* Mount point is last argument. */
	mnt_point = argv[optind + 1];

	/* Check and set SunCluster requested mount parameters */
	SetSCMountParams(mp);
#endif /* sun */

	if (fset_name[0] == '/') {
		if (mp->fi_name[0] == '\0') {
			fprintf(stderr, "%s: %s %s\n", program_name,
				/* must specify family_set for block_device */
			    GetCustMsg(17303), fset_name);
			exit(EXIT_USAGE);
		}
	}

	if (strlen(mnt_point) >= sizeof (upath_t)) {
		/* mount point path cannot exceed %d characters */
		fprintf(stderr, GetCustMsg(17308),
		    program_name, (int)sizeof (upath_t) - 1);
		fprintf(stderr, "        %s\n", mnt_point);
		exit(EXIT_USAGE);
	}

	if (verbose) {
		SetFieldPrintValues(mp->fi_name, MountParams, mp);
	}

	if (stat(mnt_point, &file_stat) != 0) {
		FatalError(errno, 616, mnt_point);
	}
	strncpy(mp->fi_mnt_point, mnt_point, sizeof (mp->fi_mnt_point));

	if ((msg = MountCheckParams(mp)) != NULL) {
		FatalError(0, 0, msg);
	}

	/*
	 * If 'mat' file system, disallow 'sam'
	 */
	if (mp->fi_type == DT_META_OBJ_TGT_SET) {
		if (mp->fi_config & MT_SAM_ENABLED) {
			FatalError(0, 17265, mp->fi_name);
		}
	}

#ifdef sun
	/*
	 * mount(2) has 6 parameters, and returns ENAMETOOLONG for
	 * long names.
	 */
	mp->fi_mflag |= (MS_NOTRUNC | MS_DATA);
#endif /* sun */

	/*
	 * If this is a shared fs mount, notify the file system.
	 * The file system will then send a mount message to the metadata
	 * server.
	 * If this is the metadata server, the system call immediately returns.
	 * If this is a client, the client contacts the metadata server
	 * to verify the mount point is valid & mounted on the metadata server.
	 * The mount command waits on the SC_share_mount system
	 * call until the metadata server responds to the SAM_CMD_MOUNT
	 * command.
	 */
	errflag = 0;
	if ((mp->fi_config1 & MC_SHARED_FS) &&
	    !(mp->fi_config & MT_SHARED_READER)) {
		if (!(mp->fi_config & MT_SHARED_MO)) {
			FatalError(0, 17305, mp->fi_name);
		}
#ifdef sun
		/*
		 * Shared mounts return immediately if called by mountall.
		 * This is because the network must be up for a shared file
		 * system to successfully mount, and mountall is typically
		 * invoked before the network is up.  Instead, shared mounts
		 * are done later for linux by the S73samfs.shared rc script
		 * or, for Solaris, by the qfs/shared-mount service.
		 */
		if (strcmp(GetParentName(), "mountall") == 0) {
			fprintf(stderr,
			    "%s: Returning success for mountall FS %s\n",
			    argv[0], mp->fi_name);
			return (EXIT_SUCCESS);
		}
		if (strcmp(GetParentName(), "mount") == 0) {
			fprintf(stderr,
			    "%s: Returning success for mount FS %s\n",
			    argv[0], mp->fi_name);
			return (EXIT_SUCCESS);
		}
#endif /* sun */

		fsp = &fsshare;		/* fsp = NULL for non-shared mounts */
		memset(fsp, 0, sizeof (*fsp));
		strncpy(fsp->fs_name, mp->fi_name, sizeof (fsp->fs_name));
		fsp->config = mp->fi_config;
		fsp->config1 = mp->fi_config1;
		fsp->background = 0;
		if ((errflag = sam_syscall(SC_share_mount, fsp,
		    sizeof (*fsp))) < 0) {
			if (errno == EPROTO) {
				FatalError(0, 17311, "SC_mount() error");
			}
			if (errno != ETIME) {
				FatalError(errno, 0, "SC_mount() error");
			}
		}
	}

#ifdef linux
	gmi.type = GENERIC_SAM_MOUNT_INFO;
	gmi.len = sizeof (mount_info);
	gmi.data = (void *)&mount_info;
#endif /* linux */

	if (errflag ||
#ifdef linux
	    mount(fset_name, mnt_point, SAM_FSTYPE, mp->fi_mflag, &gmi)
#endif /* linux */
#ifdef sun
	    mount(fset_name, mnt_point, mp->fi_mflag, SAM_FSTYPE,
	    &mount_info, sizeof (mount_info))
#endif /* sun */
	    < 0) {

		/* "illegal byte sequence" */
		if (!errflag && errno == EILSEQ) {
			/* FS is byte-reversed */
			FatalError(0, 17310, mp->fi_name);
		}

		if (!errflag && errno != ETIME && errno != ENOTCONN) {
			int errsv = errno;
			struct stat sb;

			if ((lstat(SAM_EXECUTE_PATH"/samw", &sb) == 0) &&
			    S_ISLNK(sb.st_mode)) {
				fprintf(stderr, "Please check "
				    "/var/adm/messages for"
				    " possible WORM errors.\n");
			}
			FatalError(errsv, 0, "mount() error");
		}

		if (retry_mount(mp, fsp, fset_name, mnt_point,
		    &mount_info) != 0) {
			FatalError(errno, 17304, mp->fi_name);
		}
	}

#ifdef sun
	/*
	 * If journaling was requested, wait for sam-fsd to initialize
	 * journaling before updating mnttab and exiting.
	 */
	if (mp->fi_config1 & MC_LOGGING) {
		struct sam_fs_info fi;

		if (GetFsInfo(fset_name, &fi) >= 0) {
			if ((fi.fi_status & FS_CLIENT) == 0) {
				int isjournal;

				while ((isjournal = checkisjournal(mnt_point))
				    == FIOLOG_EPEND) {
					sleep(1);
				}

				if (isjournal != FIOLOG_ENONE) {
					fprintf(stderr,
					    "Can't enable journaling.  See "
					    "/var/adm/messages for details.\n");
				}
			}
		}
	}
#endif /* sun */

	/*
	 * For Linux, -n indicates no mounted FS table update.
	 */
#ifdef CMD_UPDATE_MNTTAB
	if (update_mnttab == TRUE) {
		if (mnt_opts == NULL) {
			if (mp->fi_mflag & MS_RDONLY) {
				mnt_opts = "ro,suid";
			} else {
				mnt_opts = "rw,suid";
			}
		}

		add_mnttab_entry(fset_name, mnt_point, mnt_opts, 0, 0);
	}
#endif /* CMD_UPDATE_MNTTAB */

	return (EXIT_SUCCESS);
}


/*
 * ----- retry_mount - Retry the shared mount.
 *
 * Reissue mount if in background or if soft mount.
 * Delay time begins at 5 seconds and backs off previous
 * delay * delay until 2 minute delay is reached.
 * The number of retries is the mount parameter "retry"
 * which defaults to 10,000.
 */

static int
retry_mount(
	struct sam_fs_info *mp,
	struct sam_share_arg *fsp,
	char *fset_name,
	char *mnt_point,
	struct sam_mount_info *mount_info)
{
	int count = mp->fi_retry;
	int delay = 5;
	int err;

	if (mp->fi_config & MT_SHARED_BG) {
		if (fork() > 0) {
			exit(EXIT_SUCCESS);
		}
		fprintf(stderr, "%s: %s %s\n", program_name,
			/* backgrounding: */
		    GetCustMsg(17306), fset_name);
	} else {
		fprintf(stderr, "%s: %s %s\n", program_name,
			/* retrying: */
		    GetCustMsg(17307), fset_name);
	}
	while (count--) {
		err = 0;
		if (fsp != NULL) {
			fsp->background = 1;
			if ((err = sam_syscall(SC_share_mount, fsp,
			    sizeof (*fsp))) < 0) {
				if (errno != ETIME) {
					FatalError(errno, 17304, mp->fi_name);
				}
			}
		}
		if (!err &&
#ifdef linux
		    mount(fset_name, mnt_point, SAM_FSTYPE, mp->fi_mflag,
		    mount_info) >= 0) {
#endif /* linux */
#ifdef sun
			mount(fset_name, mnt_point, mp->fi_mflag, SAM_FSTYPE,
			    mount_info,
				sizeof (struct sam_mount_info)) >= 0) {
#endif /* sun */
			break;
		}
		if (errno != ETIME && errno != ENOTCONN) {
			FatalError(errno, 17304, mp->fi_name);
		}
		if (delay > 5) {
			sleep(delay - 5);
		}
		delay *= 2;
		if (delay > 120) {
			delay = 120;
		}
	}
	if (count <= 0) {
		return (1);
	}
	return (0);
}

#ifdef sun
/*
 * Check journaling state - return non-zero if journaling is enabled.
 */
static int
checkisjournal(char *mp)
{
	int fd;
	uint32_t isjournal = 0;

	fd = open(mp, O_RDONLY);
	if (fd >= 0) {
		(void) ioctl(fd, _FIOISLOG, &isjournal);
		(void) close(fd);
	}
	return ((int)isjournal);
}
#endif /* sun */


#ifdef CMD_UPDATE_MNTTAB
/*
 * ----- add_mnttab_entry - Add an entry to the mounted fs table.
 */

static void
add_mnttab_entry(
	char *file,
	char *mnt_point,
	char *mnt_opts,
	int freq,
	int passno)
{
#ifdef linux
	struct mntent mntent;
#endif /* linux */
#ifdef sun
	struct mnttab mntent;
	time_t mount_time;
	char timestr[12];
#endif /* sun */
	flock_t mnttab_lock;
	FILE *mnttab;
	int mntlk_fd;

	memset(&mnttab_lock, 0, sizeof (flock_t));

#ifdef linux
	mntent.mnt_fsname = strdup(file);
	mntent.mnt_dir = strdup(mnt_point);
	mntent.mnt_type = SAM_FSTYPE;
	mntent.mnt_opts = mnt_opts;
	mntent.mnt_freq = freq;
	mntent.mnt_passno = passno;
#endif /* linux */
#ifdef sun
	mntent.mnt_special = strdup(file);
	mntent.mnt_mountp = strdup(mnt_point);
	mntent.mnt_fstype = SAM_FSTYPE;
	mntent.mnt_mntopts = mnt_opts;

	mount_time = time(NULL);
	sprintf(timestr, "%ld", mount_time);
	mntent.mnt_time = timestr;
#endif /* sun */

	mnttab_lock.l_type = F_WRLCK;

	if ((mntlk_fd = open(MNTTABLK, O_RDWR | O_CREAT | O_TRUNC,
	    0644)) < 0) {
		/* "mount_samfs: open mnttab lock" */
		FatalError(errno, 1709);
		return;
	}
	if (fcntl(mntlk_fd, F_SETLKW, &mnttab_lock) == -1) {
		/* "mount_samfs: Unable to obtain mnttab lock." */
		FatalError(errno, 1710);
	}
	if ((mnttab = fopen(MOUNTED, "a")) == NULL) {
		FatalError(errno, 0, "error opening mount table");
	}

#ifdef linux
	if (addmntent(mnttab, &mntent) != 0) {
		FatalError(errno, 0, "addmntent(mnttab) error");
	}
#endif /* linux */
#ifdef sun
	if (putmntent(mnttab, &mntent) <= 0) {
		FatalError(errno, 0, "putmntent(mnttab) error");
	}
#endif /* sun */
	fclose(mnttab);
	(void) close(mntlk_fd);
#ifdef linux
	unlink(MNTTABLK);
#endif /* linux */
}
#endif /* CMD_UPDATE_MNTTAB */

/*
 * Process fatal error.
 * Send error message to appropriate destination and error exit.
 */
static void
FatalError(
	int ErrNum,	/* errno if message desired; 0 otherwise */
	int MsgNum,	/* Message catalog number.  If 0, don't use catalog */
	...)		/* Msg args,  If 0, first is format like printf() */
{
	static char msg_buf[2048];
	va_list args;
	char	*msg;

	va_start(args, MsgNum);
	if (MsgNum != 0)  msg = GetCustMsg(MsgNum);
	else  msg = va_arg(args, char *);
	vsnprintf(msg_buf, sizeof (msg_buf), msg, args);
	va_end(args);
	msg = msg_buf + strlen(msg_buf);
	/* Error number */
	if (ErrNum != 0) {
		*msg++ = ':';
		*msg++ = ' ';
		if (ErrNum == EXDEV) {
			strcpy(msg, "MDS not mounted");
		} else {
			(void) StrFromErrno(ErrNum, msg, sizeof (msg_buf) -
			    (msg - msg_buf));
		}
		msg += strlen(msg);
	}
	*msg = '\0';
	fprintf(stderr, "%s: %s\n", program_name, msg_buf);

	/*
	 * If the mount failed because the client socket could not connect,
	 * then, the Solaris Cluster agent will keep retrying forever.
	 */
	if (ErrNum == ENOTCONN) {
		exit(EXIT_RETRY);
	}
	if (ErrNum == EXDEV) {
		exit(EXIT_NOMDS);
	}
	exit(EXIT_FATAL);
}
