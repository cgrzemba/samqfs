/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define if the closedir function returns void instead of int.  */
/* #undef CLOSEDIR_VOID */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#define HAVE_ALLOCA_H 1

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if your system has a working fnmatch function.  */
#define HAVE_FNMATCH 1

/* Define if you have the getmntent function.  */
#define HAVE_GETMNTENT 1

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if your struct stat has st_blocks.  */
#define HAVE_ST_BLOCKS 1

/* Define if you have the strftime function.  */
/* #undef HAVE_STRFTIME */

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
#define HAVE_UTIME_NULL 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
#define MAJOR_IN_MKDEV 1

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
/* #undef MAJOR_IN_SYSMACROS */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if you have the Andrew File System.  */
/* #undef AFS */

/* Define if there is a member named d_ino in the struct describing
   directory headers.  */
#define D_INO_IN_DIRENT 1

/* Define to 1 if NLS is requested.  */
#define ENABLE_NLS 1

/* Define to rpl_fnmatch if the replacement function should be used.  */
/* #undef fnmatch */

/* Define as rpl_getgroups if getgroups doesn't work right.  */
/* #undef getgroups */

/* Define to gnu_strftime if the replacement function should be used.  */
#define strftime gnu_strftime

/* Define if your system defines TIOCGWINSZ in sys/ioctl.h.  */
/* #undef GWINSZ_IN_SYS_IOCTL */

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
/* #undef HAVE_CATGETS */

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#define HAVE_GETTEXT 1

/* Define if your locale.h file contains LC_MESSAGES.  */
#define HAVE_LC_MESSAGES 1

/* Define to 1 if you have the obstack functions from GNU libc.  */
/* #undef HAVE_OBSTACK */

/* Define to 1 if you have the stpcpy function.  */
#define HAVE_STPCPY 1

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
/* #undef ino_t */

/* Define if `struct utimbuf' is declared -- usually in <utime.h>.  */
#define HAVE_STRUCT_UTIMBUF 1

/* Define to gnu_mktime if the replacement function should be used.  */
#define mktime rpl_mktime

/* Define if there is no specific function for reading the list of
   mounted filesystems.  fread will be used to read /etc/mnttab.  [SVR2]  */
/* #undef MOUNTED_FREAD */

/* Define if (like SVR2) there is no specific function for reading the
   list of mounted filesystems, and your system has these header files:
   <sys/fstyp.h> and <sys/statfs.h>.  [SVR3]  */
/* #undef MOUNTED_FREAD_FSTYP */

/* Define if there is a function named getfsstat for reading the list
   of mounted filesystems.  [DEC Alpha running OSF/1]  */
/* #undef MOUNTED_GETFSSTAT */

/* Define if there is a function named getmnt for reading the list of
   mounted filesystems.  [Ultrix]  */
/* #undef MOUNTED_GETMNT */

/* Define if there is a function named getmntent for reading the list
   of mounted filesystems, and that function takes a single argument.
   [4.3BSD, SunOS, HP-UX, Dynix, Irix]  */
/* #undef MOUNTED_GETMNTENT1 */

/* Define if there is a function named getmntent for reading the list of
   mounted filesystems, and that function takes two arguments.  [SVR4]  */
#define MOUNTED_GETMNTENT2 1

/* Define if there is a function named getmntinfo for reading the list
   of mounted filesystems.  [4.4BSD]  */
/* #undef MOUNTED_GETMNTINFO */

/* Define if there is a function named listmntent that can be used to
   list all mounted filesystems. [UNICOS] */
/* #undef MOUNTED_LISTMNTENT */

/* Define if there is a function named mntctl that can be used to read
   the list of mounted filesystems, and there is a system header file
   that declares `struct vmount.'  [AIX]  */
/* #undef MOUNTED_VMOUNT */

/* Define to the name of the distribution.  */
#define PACKAGE "fileutils"

/* The concatenation of the strings "GNU ", and PACKAGE.  */
#define GNU_PACKAGE "GNU fileutils"

/* Define to 1 if ANSI function prototypes are usable.  */
#define PROTOTYPES 1

/*  Define if  statfs takes 3 args.  [DEC Alpha running OSF/1]  */
/* #undef STAT_STATFS3_OSF1 */

/* Define if there is no specific function for reading filesystems usage
   information and you have the <sys/filsys.h> header file.  [SVR2]  */
/* #undef STAT_READ_FILSYS */

/* Define if statfs takes 2 args and struct statfs has a field named f_bsize.
   [4.3BSD, SunOS 4, HP-UX, AIX PS/2]  */
/* #undef STAT_STATFS2_BSIZE */

/* Define if statfs takes 2 args and struct statfs has a field named f_fsize.
   [4.4BSD, NetBSD]  */
/* #undef STAT_STATFS2_FSIZE */

/* Define if statfs takes 2 args and the second argument has
   type struct fs_data.  [Ultrix]  */
/* #undef STAT_STATFS2_FS_DATA */

/* Define if statfs takes 4 args.  [SVR3, Dynix, Irix, Dolphin]  */
/* #undef STAT_STATFS4 */

/* Define if there is a function named statvfs.  [SVR4]  */
#define STAT_STATVFS 1

/* Define if the block counts reported by statfs may be truncated to 2GB
   and the correct values may be stored in the f_spare array.
   [SunOS 4.1.2, 4.1.3, and 4.1.3_U1 are reported to have this problem.
   SunOS 4.1.1 seems not to be affected.]  */
/* #undef STATFS_TRUNCATES_BLOCK_COUNTS */

/* Define to the version of the distribution.  */
#define VERSION "3.16"

/* Define to 1 if GNU regex should be used instead of GNU rx.  */
#define WITH_REGEX 1

/* Define if you have the __argz_count function.  */
/* #undef HAVE___ARGZ_COUNT */

/* Define if you have the __argz_next function.  */
/* #undef HAVE___ARGZ_NEXT */

/* Define if you have the __argz_stringify function.  */
/* #undef HAVE___ARGZ_STRINGIFY */

/* Define if you have the basename function.  */
#define HAVE_BASENAME 1

/* Define if you have the bcopy function.  */
#define HAVE_BCOPY 1

/* Define if you have the dcgettext function.  */
#define HAVE_DCGETTEXT 1

/* Define if you have the endgrent function.  */
#define HAVE_ENDGRENT 1

/* Define if you have the endpwent function.  */
#define HAVE_ENDPWENT 1

/* Define if you have the euidaccess function.  */
/* #undef HAVE_EUIDACCESS */

/* Define if you have the fchdir function.  */
#define HAVE_FCHDIR 1

/* Define if you have the ftime function.  */
#define HAVE_FTIME 1

/* Define if you have the ftruncate function.  */
#define HAVE_FTRUNCATE 1

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD 1

/* Define if you have the getdelim function.  */
#define HAVE_GETDELIM 1

/* Define if you have the getgroups function.  */
#define HAVE_GETGROUPS 1

/* Define if you have the getmntinfo function.  */
/* #undef HAVE_GETMNTINFO */

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the isascii function.  */
#define HAVE_ISASCII 1

/* Define if you have the lchown function.  */
#define HAVE_LCHOWN 1

/* Define if you have the listmntent function.  */
/* #undef HAVE_LISTMNTENT */

/* Define if you have the memcmp function.  */
#define HAVE_MEMCMP 1

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the memset function.  */
#define HAVE_MEMSET 1

/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the mkfifo function.  */
#define HAVE_MKFIFO 1

/* Define if you have the munmap function.  */
#define HAVE_MUNMAP 1

/* Define if you have the putenv function.  */
#define HAVE_PUTENV 1

/* Define if you have the rename function.  */
#define HAVE_RENAME 1

/* Define if you have the rmdir function.  */
#define HAVE_RMDIR 1

/* Define if you have the rpmatch function.  */
/* #undef HAVE_RPMATCH */

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the stpcpy function.  */
#define HAVE_STPCPY 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strndup function.  */
#define HAVE_STRNDUP 1

/* Define if you have the strrchr function.  */
#define HAVE_STRRCHR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the <argz.h> header file.  */
/* #undef HAVE_ARGZ_H */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file.  */
#define HAVE_LOCALE_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <mntent.h> header file.  */
/* #undef HAVE_MNTENT_H */

/* Define if you have the <mnttab.h> header file.  */
/* #undef HAVE_MNTTAB_H */

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <nl_types.h> header file.  */
#define HAVE_NL_TYPES_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/filsys.h> header file.  */
/* #undef HAVE_SYS_FILSYS_H */

/* Define if you have the <sys/fs/s5param.h> header file.  */
/* #undef HAVE_SYS_FS_S5PARAM_H */

/* Define if you have the <sys/fs_types.h> header file.  */
/* #undef HAVE_SYS_FS_TYPES_H */

/* Define if you have the <sys/fstyp.h> header file.  */
#define HAVE_SYS_FSTYP_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/mount.h> header file.  */
#define HAVE_SYS_MOUNT_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/statfs.h> header file.  */
#define HAVE_SYS_STATFS_H 1

/* Define if you have the <sys/statvfs.h> header file.  */
#define HAVE_SYS_STATVFS_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1

/* Define if you have the <sys/vfs.h> header file.  */
#define HAVE_SYS_VFS_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <utime.h> header file.  */
#define HAVE_UTIME_H 1

/* Define if you have the <values.h> header file.  */
#define HAVE_VALUES_H 1

/* Define if you have the i library (-li).  */
/* #undef HAVE_LIBI */

/* Define if you have the ldgc library (-lldgc).  */
/* #undef HAVE_LIBLDGC */

/* Define if you have the ypsec library (-lypsec).  */
/* #undef HAVE_LIBYPSEC */
