/*
 * lib.h - SAM-FS user library function prototypes.
 *
 * Definitions for SAM-FS user library function prototypes.
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


#ifndef	_SAM_LIB_H
#define	_SAM_LIB_H

#ifdef sun
#pragma ident "$Revision: 1.57 $"
#endif

#include "sam/types.h"


/* Macros. */
#define	STR_FROM_ERRNO_BUF_SIZE 80
#define	STR_FROM_FSIZE_BUF_SIZE 12
#define	STR_FROM_INTERVAL_BUF_SIZE 18
#define	STR_FROM_TIME_BUF_SIZE 18
#define	STR_FROM_VOLID_BUF_SIZE 40

/* Macros for WORM retention period calculations */
#define	MINS_IN_YEAR (365*24*60)
#define	MINS_IN_LYEAR (366*24*60)
#define	MINS_IN_DAY  (24*60)
#define	MINS_IN_HOUR (60)

/* Constants for getHostName() status */
#define	HOST_NAME_EOK		 0
#define	HOST_NAME_EFAIL		-1
#define	HOST_NAME_ETOOSHORT	-2
#define	HOST_NAME_ETOOLONG	-3
#define	HOST_NAME_EBADCHAR	-4
#define	HOST_FILE_EREAD		-5
#define	HOST_FILE_ETOOSHORT	-6
#define	HOST_FILE_ETOOLONG	-7
#define	HOST_FILE_EBADCHAR	-8

/* SAM file system request functions: */

#ifdef	_SAM_RESOURCE_H
extern	int sam_rdrsf(char *, int, sam_resource_file_t *);
#endif

/* SAM command utility functions: */

extern char *device_to_nm(int);
extern	int nm_to_device(char *);
extern	int media_to_device(char *);
extern	int sam_syscall(int cmd, void *args, int arg_size);
extern void LoadFS();


/* Generally useful command utility functions: */

pid_t FindProc(char *name, char *arg);
char *GetParentName(void);
char *GetProcName(pid_t pid, char *buf, int buf_size);
int sam_lockout(char *name, char *dir, char *prefix, int *siglist);
void *MapFileAttach(char *fileName, uint_t magic, int mode);
void MakeDir(char *dname);
int MapFileDetach(void *mf_a);

int percent_used(uint_t capacity, uint_t space);
int llpercent_used(u_longlong_t capacity, u_longlong_t space);
void ll2oct(u_longlong_t value, char *dest, int width);
void ll2str(u_longlong_t value, char *dest, int width);
u_longlong_t llfrom_oct(int digs, char *where);
u_longlong_t llfrom_str(int digs, char *where);

extern char *time_string(time_t, time_t, char *);
extern char *TimeString(time_t time, char *str, int size);

time_t StrToTime(char *str);

extern int StrToMinutes(char *args, long *expiration);
extern void MinToStr(time_t chgtime, long num_mins, char *gtime, char *str);
extern int DateToMinutes(char *args, long *mins);

char *getuser(uid_t);		/* get user name for uid */
char *getgroup(gid_t);		/* get group name for gid */

#ifndef	MAIN
extern char *program_name;
#endif

#ifndef	SAM_LIB_GNU		/* Do not define these for GNU code */
extern	void	error		(int, int, char *, ...);
extern	char *stpcpy		(char *, char *);
#endif

extern	int	SAM_fd;		/* File descriptor for .ioctl file */

int sam_syscall(int number, void *arg, int size);
char *sam_mediatoa(int mt);
extern int sam_atomedia(char *name);
extern void sam_syslog(int priority, const char *fmt, ...);
extern char *StrFromErrno(int errno_arg, char *buf, int buf_size);
extern char *StrFromFsize(uint64_t size, int prec, char *buf, int buf_size);
extern char *StrFromInterval(int interval, char *buf, int buf_size);
extern int StrToFsize(char *string, uint64_t *size);
extern int StrToInterval(char *string, int *interval);

#if defined(_AML_CATALOG_H)
extern int StrToVolId(char *arg, struct VolId *vid);
extern char *StrFromVolId(struct VolId *vid, char *buf, int buf_size);
#endif /* defined(_AML_CATALOG_H) */

#if defined(_SAM_FS_DIRENT_H)
int sam_getdent(struct sam_dirent ** dirent);
int sam_opendir(char *dir_name);
#endif /* defined(_SAM_FS_DIRENT_H) */

#if defined(_SAMFS_MOUNT_H)
int GetFsInfo(char *name, struct sam_fs_info *fi);
int GetFsInfoDefs(char *name, struct sam_fs_info *fi);
int GetFsInfoByEq(int eq, struct sam_fs_info *fi);
int GetFsInfoByPartEq(int eq, struct sam_fs_info *fi);
int GetFsMount(char *fs_name, struct sam_mount_info *mp);
int GetFsMountDefs(char *fs_name, struct sam_mount_info *mp);
char *GetFsMountName(int eq);
int GetFsParts(char *name, int maxpts, struct sam_fs_part *pts);
int GetFsStatus(struct sam_fs_status **fs);
int OpenInodesFile(char *mountPoint);
char *SetFsParam(char *fsname, char *param, char *value);
char *SetFsConfig(char *fsname, char *param);
int SetFsPartCmd(char *fsname, char *eqnum, int32_t command);
int onoff_client(char *fsname, int cl, int cmd);

char *StrFromFsStatus(struct sam_fs_info *fi);
#endif /* defined(_SAMFS_MOUNT_H) */

#if defined(_AML_DEVICE_H)
int read_mcf(char *dummy, dev_ent_t **devlist, int *high_eq);
void WriteMcfbin(int DeviceNumof, dev_ent_t *DeviceTable);
#endif /* defined(_AML_DEVICE_H) */

int open_obj_device(char *dev_name, int oflags, uint64_t *obj_handle);
int close_obj_device(char *dev_name, int oflags, uint64_t obj_handle);
int create_object(char *fs_name, uint64_t obj_handle, int ord,
	uint64_t obj_ino);
int write_object(char *fs_name, uint64_t obj_handle, int ord,
	uint64_t obj_ino, char *data, uint64_t offset, uint64_t size);
int read_object(char *fs_name, uint64_t obj_handle, int ord,
	uint64_t obj_ino, char *data, uint64_t offset, uint64_t size);
int get_object_fs_attributes(char *fs_name, uint64_t obj_handle, int ord,
	char *data, uint64_t size);

#endif /* _SAM_LIB_H */
