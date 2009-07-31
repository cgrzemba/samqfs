/*
 * filesys.c - File system interface functions.
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

#pragma ident "$Revision: 1.43 $"

#include "sam/osversion.h"

/* ANSI C headers. */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef linux
#include <stdio.h>
#endif /* linux */

/* POSIX headers. */
#include <unistd.h>
#include <fcntl.h>

/* OS headers. */
#include <sys/mount.h>
#include <sys/vfs.h>

/* SAM-FS headers. */
#include <aml/types.h>
#include <sam/mount.h>
#include <sam/lib.h>
#include <sam/setfield.h>
#include <sam/syscall.h>
#include <sam/uioctl.h>
#include <sam/fs/amld.h>
#include <sam/fs/sblk.h>

#include <sam/mount.hc>

#if defined(lint)
#include <sam/lint.h>
#endif /* defined(lint) */

/* Private data. */
static char msgbuf[128];	/* For message returned from SetFieldValue() */

/* Private functions. */
static void msgFunc(int code, char *msg);
static int issue_obj_command(int32_t command, char *fs_name,
	uint64_t obj_handle, int ord, uint64_t obj_ino, char *data,
	uint64_t offset, uint64_t size);


/*
 * GetFsInfoDefs - Get the file system default information
 * (as configured by sam-fsd).
 */
int			/* -1 if not found. */
GetFsInfoDefs(
	char *name,
	struct sam_fs_info *fi)
{
	struct sam_get_fsinfo_arg arg;

	strncpy(arg.fs_name, name, sizeof (arg.fs_name));
	arg.fi.ptr = fi;
	if (sam_syscall(SC_getfsinfo_defs, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * GetFsInfo - Get the active file system information.
 */
int			/* -1 if not found. */
GetFsInfo(
	char *name,
	struct sam_fs_info *fi)
{
	struct sam_get_fsinfo_arg arg;

	strncpy(arg.fs_name, name, sizeof (arg.fs_name));
	arg.fi.ptr = fi;
	if (sam_syscall(SC_getfsinfo, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * GetFsInfoByEq - Get the file system information given the equipment number.
 */
int			/* -1 if not found. */
GetFsInfoByEq(
	int eq,
	struct sam_fs_info *fi)
{
	struct sam_fs_status *fsarray;
	int i;
	int numfs;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		return (-1);
	}

	/*
	 * Find the filesystem entry which matches the given eq.
	 */
	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;

		fs = &fsarray[i];
		if (fs->fs_eq == eq) {
			int		retval;

			retval = GetFsInfo(fs->fs_name, fi);
			free(fsarray);
			return (retval);
		}
	}
	free(fsarray);
	errno = ENODEV;
	return (-1);
}


/*
 * GetFsInfoByPartEq - Get the file system information given any partitions
 * equipment number.
 */
int			/* -1 if not found. */
GetFsInfoByPartEq(
	int eq,
	struct sam_fs_info *fi)
{
	struct sam_fs_status *fsarray;
	struct sam_fs_status *fs;
	struct sam_fs_info fsinfo;
	struct sam_fs_part *part_list, *part_cur;
	int i, j;
	int numfs;
	int max_part;
	int size;
	int retval;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		return (-1);
	}
	max_part = 0;
	retval = -1;
	part_list = NULL;

	/*
	 * Find the filesystem entry which matches the given eq.
	 */
	for (i = 0; (i < numfs || retval == 0); i++) {
		fs = &fsarray[i];
		if (GetFsInfo(fs->fs_name, &fsinfo) == -1) {
			continue;
		}
		if (fsinfo.fs_count > max_part) {
			if (max_part != 0) {
				free(part_list);
				part_list = NULL;
			}
			max_part = fsinfo.fs_count * 2;
			if (max_part > L_FSET) {
				max_part = L_FSET;
			}

			/*
			 * Allocate a suitably large partition array.
			 */
			size = max_part * sizeof (struct sam_fs_part);
			part_list = (struct sam_fs_part *)malloc(size);
			if (part_list == NULL) {
				free(fsarray);
				errno = ENOMEM;
				return (-1);
			}
		}
		if (GetFsParts(fs->fs_name, fsinfo.fs_count, part_list) == -1) {
			return (-1);
		}
		for (j = 0, part_cur = part_list; j < fsinfo.fs_count; j++,
		    part_cur++) {
			if (part_cur->pt_eq == eq) {
				memcpy(fi, &fsinfo,
				    sizeof (struct sam_fs_info));
				retval = 0;
				break;
			}
		}
		if (retval == 0) {
			break;
		}
	}
	if (retval) {
		errno = ENODEV;
	}
	free(fsarray);
	free(part_list);
	return (retval);
}


/*
 * GetFsMountDefs - Get the file system default mount information
 * (as configured by sam-fsd).
 */
int				/* -1 if not found. */
GetFsMountDefs(
	char *fs_name,
	struct sam_mount_info *mp)
{
	if (GetFsInfoDefs(fs_name, &mp->params) == -1) {
		return (-1);
	}
	if (GetFsParts(fs_name, mp->params.fs_count, &mp->part[0]) == -1) {
		return (-1);
	}
	return (0);
}


/*
 * GetFsMount - Get the file system mount information.
 */
int			/* -1 if not found. */
GetFsMount(
	char *fs_name,
	struct sam_mount_info *mp)
{
	if (GetFsInfo(fs_name, &mp->params) == -1) {
		return (-1);
	}
	if (GetFsParts(fs_name, mp->params.fs_count, &mp->part[0]) == -1) {
		return (-1);
	}
	return (0);
}


/*
 * GetFsMountName - Get the mount point for a file system given the equipment
 * number.  Returns empty string if not found.
 */
char *
GetFsMountName(
	int eq)
{
	static struct sam_fs_status *fsarray = NULL;
	static int numfs;
	int i;

	if (fsarray == NULL) {
		if ((numfs = GetFsStatus(&fsarray)) == -1) {
			return ("");
		}
	}

	/*
	 * Find the filesystem entry which matches the given eq.
	 */
	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;

		fs = &fsarray[i];
		if (fs->fs_eq == eq) {
			return (fs->fs_mnt_point);
		}
	}
	errno = ENODEV;
	return ("");
}


/*
 * GetFsParts - Get the file system partitions.
 */
int			/* -1 if not found. */
GetFsParts(
	char *name,
	int maxpts,
	struct sam_fs_part *pts)
{
	struct sam_get_fspart_arg arg;

	strncpy(arg.fs_name, name, sizeof (arg.fs_name));
	arg.maxpts = maxpts;
	arg.numpts = 0;
	arg.pt.ptr = pts;
	if (sam_syscall(SC_getfspart, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * GetFsStatus - Get file system status.
 * Returns an array of sam_fs_status_t structs for the configured file systems.
 * The array is malloc()ed.
 */
int					/* Number of configured filesystems */
GetFsStatus(
	struct sam_fs_status **fs)	/* Where to put the array */
{
	struct sam_get_fsstatus_arg arg;
	size_t	size;

	/*
	 * Get number of filesystems which are configured from the filesystem.
	 */
	*fs = NULL;
	memset(&arg, 0, sizeof (arg));
	if (sam_syscall(SC_getfsstatus, &arg, sizeof (arg)) < 0) {
		return (-1);
	}

	/*
	 * Malloc the sam_fs_id array for the list of filesystems and get the
	 * filesystem information.
	 */
	arg.maxfs = arg.numfs;
	size = arg.numfs * sizeof (struct sam_fs_status);
	*fs = (struct sam_fs_status *)malloc(size);
	if (*fs == NULL) {
		errno = ENOMEM;
		return (-1);
	}
	memset(*fs, 0, size);
	arg.fs.ptr = *fs;
	if (sam_syscall(SC_getfsstatus, &arg, sizeof (arg)) < 0) {
		free(*fs);
		*fs = NULL;
		return (-1);
	}
	return (arg.numfs);
}


/*
 * Open the .inodes file.
 */
int				/* Open file descriptor for .inodes. */
OpenInodesFile(
	char *mountPoint)
{
	struct sam_ioctl_idopen arg;
	int		fd;
	int		mpfd;

	if ((mpfd = open(mountPoint, O_RDONLY)) < 0) {
		return (-1);
	}
	memset(&arg, 0, sizeof (arg));
	arg.id.ino = SAM_INO_INO;
	arg.flags |= IDO_direct_io;
	fd = ioctl(mpfd, F_IDOPEN, &arg);
	(void) close(mpfd);
	return (fd);
}


/*
 * Set filesystem parameter.
 * Returns empty ("") if no error.
 */
char *
SetFsConfig(
	char *fsname,		/* Filesystem name */
	char *param,		/* Name of parameter to set */
	int  flag)		/* 0=set in mt, 1=set default in orig_mt  */
{
	struct sam_setfsconfig_arg args;
	struct sam_fs_info fi;
	struct fieldVals *table;
	struct fieldFlag *vals;

	if (SetFieldValue(&fi, MountParams, param, NULL, msgFunc) == -1) {
		return (msgbuf);
	}
	/*
	 * Maybe a function in setfield for this?
	 * SetFieldValue() returns table index?
	 */
	table = MountParams;
	while (strcmp(param, table->FvName) != 0) {
		table++;
		if (table->FvName == NULL) {
			errno = ENOENT;
			return ("??");
		}
	}

	/*
	 * The SET/CLEARFLAG have no associated value.
	 */
	vals = (struct fieldFlag *)table->FvVals;
	strncpy(args.sp_fsname, fsname, sizeof (args.sp_fsname)-1);
	args.sp_mask = vals->mask;
	args.sp_offset = table->FvLoc;
	args.sp_value = (table->FvType == SETFLAG) ? vals->mask : 0;
	if (flag) {
		if (sam_syscall(SC_setfsconfig_defs, &args,
		    sizeof (args)) < 0) {
			return ("SC_setfsconfig_defs error");
		}
	} else {
		if (sam_syscall(SC_setfsconfig, &args, sizeof (args)) < 0) {
			return ("SC_setfsconfig error");
		}
	}
	return ("");
}


/*
 * Set filesystem parameter.
 * Returns empty ("") if no error.
 */
char *
SetFsParam(
	char *fsname,		/* Filesystem name */
	char *param,		/* Name of parameter to set */
	char *value)		/* Parameter value */
{
	struct sam_setfsparam_arg args;
	struct sam_fs_info fi;
	struct fieldVals *table;
	void	*v;

	if (SetFieldValue(&fi, MountParams, param, value, msgFunc) == -1) {
		return (msgbuf);
	}
	/*
	 * Maybe a function in setfield for this?
	 * SetFieldValue() returns table index?
	 */
	table = MountParams;
	while (strcmp(param, table->FvName) != 0) {
		table++;
		if (table->FvName == NULL) {
			errno = ENOENT;
			return ("??");
		}
	}
	args.sp_offset = table->FvLoc;
	v = ((char *)&fi + table->FvLoc);
	/*
	 * File system parameters are only numeric.
	 */
	switch (table->FvType) {
	case PWR2:
	case MUL8:
	case INT:
		args.sp_value = *(int *)v;
		break;
	case INT16:
		args.sp_value = *(int16_t *)v;
		break;
	case INT64:
	case MULL8:
		args.sp_value = *(int64_t *)v;
		break;
	default:
		return ("??");
	}
	strncpy(args.sp_fsname, fsname, sizeof (args.sp_fsname)-1);
	if (sam_syscall(SC_setfsparam, &args, sizeof (args)) < 0) {
		return ("SC_setfsparam error");
	}
	return ("");
}


/*
 * Issue filesystem partition disk command.
 */
int				/* Error return */
SetFsPartCmd(
	char *fsname,		/* Filesystem name */
	char *eqnum,		/* Equipment no. of partition to change */
	int32_t command)	/* Command value */
{
	struct sam_setfspartcmd_arg arg;
	int32_t eq;
	char *p;

	strncpy(arg.fs_name, fsname, sizeof (arg.fs_name)-1);
	eq = strtol(eqnum, &p, 0);
	arg.eq = eq;
	arg.command = command;
	if (sam_syscall(SC_setfspartcmd, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * onoff_client - Mark hosts on or off in client table.
 */
int				/* Error return */
onoff_client(
	char *fsname,		/* Filesystem name */
	int cl,			/* Client host ordinal (0 based) */
	int cmd)		/* Command */
{
	sam_onoff_client_arg_t arg;

	strncpy(arg.fs_name, fsname, sizeof (arg.fs_name) - 1);
	arg.clord = cl + 1;	/* client ordinals are 1 based */
	arg.command = cmd;
	if (sam_syscall(SC_onoff_client, (void *) &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (arg.ret);
}


/*
 * ----- Open object device - Open file system object device
 */

int				/* 0 if successful, -1 if error */
open_obj_device(
	char *dev_name,
	int oflags,
	uint64_t *obj_handle)
{
	struct sam_osd_dev_arg arg;

	strncpy(arg.osd_name, dev_name, sizeof (arg.osd_name));
	arg.param = OSD_DEV_OPEN;
	arg.filemode = oflags;
	arg.oh = 0;
	if (sam_syscall(SC_osd_device, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	*obj_handle = arg.oh;
	return (0);
}


/*
 * ----- Close object device - Close file system object device
 */

int				/* 0 if successful, -1 if error */
close_obj_device(
	char *dev_name,
	int oflags,
	uint64_t obj_handle)
{
	struct sam_osd_dev_arg arg;

	strncpy(arg.osd_name, dev_name, sizeof (arg.osd_name));
	arg.param = OSD_DEV_CLOSE;
	arg.filemode = oflags;
	arg.oh = obj_handle;
	if (sam_syscall(SC_osd_device, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * ----- create_object - Issue create command for object device
 */

int				/* 0 if successful, -1 if error */
create_object(
	char *fs_name,
	uint64_t obj_handle,
	int ord,
	uint64_t obj_ino)
{
	return (issue_obj_command(OSD_CMD_CREATE, fs_name, obj_handle, ord,
	    obj_ino, NULL, 0, 0));
}


/*
 * ----- write_object - Issue write command for object device
 */

int				/* 0 if successful, -1 if error */
write_object(
	char *fs_name,
	uint64_t obj_handle,
	int ord,
	uint64_t obj_ino,
	char *data,
	uint64_t offset,
	uint64_t size)
{
	return (issue_obj_command(OSD_CMD_WRITE, fs_name, obj_handle, ord,
	    obj_ino, data, offset, size));
}


/*
 * ----- read_object - Issue read command for object device
 */

int				/* 0 if successful, -1 if error */
read_object(
	char *fs_name,
	uint64_t obj_handle,
	int ord,
	uint64_t obj_ino,
	char *data,
	uint64_t offset,
	uint64_t size)
{
	return (issue_obj_command(OSD_CMD_READ, fs_name, obj_handle, ord,
	    obj_ino, data, offset, size));
}


/*
 * ----- get_object_fs_attributes -
 * Issue get attrbuties command for object file system.
 */

int				/* 0 if successful, -1 if error */
get_object_fs_attributes(
	char *fs_name,
	uint64_t obj_handle,
	int ord,
	char *data,
	uint64_t size)
{
	return (issue_obj_command(OSD_CMD_ATTR, fs_name, obj_handle, ord,
	    0, data, 0, size));
}


/*
 * ----- issue_object_command - Issue command for object device
 */

static int			/* 0 if successful, -1 if error */
issue_obj_command(
	int32_t command,
	char *fs_name,
	uint64_t obj_handle,
	int ord,
	uint64_t obj_ino,
	char *data,
	uint64_t offset,
	uint64_t size)
{
	struct sam_osd_cmd_arg arg;

	strncpy(arg.fs_name, fs_name, sizeof (arg.fs_name));
	arg.oh = obj_handle;
	arg.ord = ord;
	arg.command = command;
	arg.obj_id = (obj_ino << 32) | obj_ino;
	arg.offset = offset;
	arg.size = size;
	arg.data.ptr = data;
	if (sam_syscall(SC_osd_command, &arg, sizeof (arg)) < 0) {
		return (-1);
	}
	return (0);
}


/*
 * StrFromFsStatus - return a pointer to a character string containing
 * filesystem flags.
 */
char *
StrFromFsStatus(
	struct sam_fs_info *fi)
{
	static char str[12];	/* Mount status string */

	str[0]	= fi->fi_status & FS_MOUNTED			? 'm' : '-';
	str[0]	= fi->fi_status & FS_MOUNTING			? 'M' : str[0];
	str[0]	= fi->fi_status & FS_UMOUNT_IN_PROGRESS ? 'u' : str[0];
	str[1]	= fi->fi_status & FS_SHRINKING			? 's' : '-';
	str[2]	= fi->fi_status & FS_ARCHIVING			? 'A' : '-';
	str[3]	= fi->fi_status & FS_RELEASING			? 'R' : '-';
	str[4]	= fi->fi_status & FS_STAGING			? 'S' : '-';
	str[5]	= fi->fi_version == 1				? '1' : '2';
	str[6]	= fi->fi_config1 & MC_SHARED_FS			? 'c' : '-';
	str[7]	= fi->fi_config & MT_SHARED_WRITER		? 'W' : '-';
	str[8]	= fi->fi_config & MT_SHARED_READER		? 'R' : '-';
	str[9]	= fi->fi_config1 & MC_MR_DEVICES		? 'r' : '-';
	str[10]	= fi->fi_config1 & MC_MD_DEVICES		? 'd' : '-';
	str[10]	= fi->fi_config1 & MC_OBJECT_FS			? 'o' : '-';

	str[11]	= '\0';
	return (str);
}


/* Private functions. */


/*
 * Save setfield message.
 */
static void
msgFunc(
/* LINTED argument unused in function */
	int code,
	char *msg)
{
	strncpy(msgbuf, msg, sizeof (msgbuf)-1);
}

#if defined(TEST)

extern char *getfullrawname();

int
main(
	int argc,
	char *argv[])
{
	char *dsk = "/dev/dsk/c0t1d0s5";
	char *nodsk = "/dev/dsl/c0t1d0s5";
	char *twodsk = "/dev/dski/dsk/c0t1d0s5";
	char *devrname;

	if ((devrname = getfullrawname(dsk))) {
		printf("%s\n", devrname);
		free(devrname);
	}
	if ((devrname = getfullrawname(nodsk))) {
		printf("%s\n", devrname);
		free(devrname);
	}
	if ((devrname = getfullrawname(twodsk))) {
		printf("%s\n", devrname);
		free(devrname);
	}

	printf("SetFsParam - %s\n", SetFsParam("samfs1", "low", "43"));
}
#endif /* defined(TEST) */
