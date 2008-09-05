/*
 * samu.h - SAM-FS operator utility.
 *
 * Structure definitions and function prototypes.
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

#pragma ident "$Revision: 1.36 $"


#if !defined(SAMU_H)
#define	SAMU_H


/* POSIX headers. */
#include <sys/types.h>

/* Solaris headers. */
#include <curses.h>
#include <sys/shm.h>
#define	MUTEX_OWNER_PTR(owner_lock)	(owner_lock)

/* SAM-FS headers. */
#include "pub/lib.h"
#include "sam/types.h"
#include "sam/param.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/stager.h"

/* Types. */
#undef FALSE
#undef TRUE
typedef enum { FALSE = 0, TRUE = 1 } boolean;

/* Key code definitions. */
typedef enum {
	KEY_full_fwd = 0x06,	/* ^F Full page forward */
	KEY_full_bwd = 0x02,	/* ^B Full page backward */
	KEY_half_fwd = 0x04,	/* ^D Half page forward */
	KEY_half_bwd = 0x15,	/* ^U Half page backward */
	KEY_details  = 0x09,	/* ^I Enable details    */
	KEY_adv_fmt  = 0x0b		/* ^K Advance format */
} Keys;

/* Level definitions */
typedef enum { SAM_NONE = 0, QFS_STANDALONE, SAM_DISK, SAM_REMOVABLE }
    sam_level;

/* Structures. */
struct MemPars {	/* Memory display parameters */
	uchar_t *base;	/* Base address */
	ulong_t size;	/* Size of area */
	ulong_t offset;	/* Offset for display */
	int fmt;		/* Format (8-bit, 16-bit, 32-bit) */
};

/* Functions. */
void CatlibInit(void);
void CheckFamilySetByEq(char *eqarg, struct sam_fs_info *fi);
char *CheckFSPartByEq(char *eqarg, struct sam_fs_info *fi);
void CmdMount(void);
void DisMem(struct MemPars *p, int ln);
void Error(const char *fmt, ...);
char *FsizeToB(fsize_t);
uint_t GetArTrace(void);
boolean KeyMem(struct MemPars *p, char key);
int  RunDisplay(char);
void SamCmd(void);
void SetArTrace(uint_t flags);
int SetDisplay(char sel);
void SetPreviewMedia(int m);
void SetStagingMedia(int m);
void command(char *cmd);
#if defined(_AML_DEVICE_H)
int dev_usage(dev_ent_t *dev);
#endif
int finddev(char *name);
int findDev(char *name);
struct sam_fs_part *findfsp(char *name, struct sam_fs_part **fp);
int getdev(char *name);
int getrobot(char *);
char *mode_string(mode_t mode);
char *ext_mode_string(mode_t mode);
boolean opendisk(char *name);
void open_mountpt(char *mp);
int percent(uint_t, uint_t);
boolean readdisk(uint_t sector);
char *string(char *s);
extern StagerStateInfo_t *MapInStagerState();
extern void UnMapStagerState(StagerStateInfo_t *state);
void DisplayStageStreams(boolean_t display_active, int file_first,
	int display_filename, media_t display_media);

#if defined(_SAM_FS_INO_H)
void dis_inode_file(struct sam_perm_inode *permp, int extent_factor);
#endif /* defined(_SAM_FS_INO_H) */


/* curses cover functions. */
void Attroff(int);
void Attron(int);
void Clear(void);
void Clrtobot(void);
void Clrtoeol(void);
void Mvprintw(int y, int x, char *fmt, /* arg */ ...);
void Printw(char *fmt, /* arg */ ...);

/* Function Macros. */
#define	SHM_ADDR(a, x) (void *)((char *)a.shared_memory + (int)(x))
#define	numof(a) (sizeof (a)/sizeof (*(a))) /* Number of elements in array a */

/* Shared data. */
/* Declaration/initialization macros. */
#undef CON_DCL
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	CON_DCL
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	CON_DCL extern const
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */


/* Common variable definitions. */

DCL shm_alloc_t master_shm;	/* Master shared memory table */
DCL shm_alloc_t preview_shm;	/* Mount preview table */
DCL shm_alloc_t archiver_shm;	/* Mount preview table */

#if defined(_AML_DEVICE_H)
DCL dev_ptr_tbl_t *Dev_Tbl;	/* Device pointer table */
DCL int Max_Devices;		/* Maximum no. of devices */
#endif

#if defined(_AML_PREVIEW_H)
DCL preview_tbl_t *Preview_Tbl; /* Preview table */
DCL int Max_Preview;		/* Maximum no. of preview slots */
#endif

#if defined(_SAM_STAGE_H)
DCL staging_tbl_t *Staging_Tbl; /* Staging activity table */
DCL int Max_Staging;		/* Maximum no. of staging slots */
#endif

DCL uchar_t Buffer[1024];
DCL char *program_name;		/* Our name */
DCL char hostname[64];		/* This machine's hostname */
DCL char *ArchiverDir;		/* Archiver data directory */
DCL char *shm_file;		/* File instead of master shm segment */
DCL char *pre_file;		/* File instead of preview shm segment */
DCL char *Argv[10];		/* Command arguments */
DCL int Argc;			/* Command argument count */
DCL int Refresh IVAL(TRUE);	/* Refresh enabled/disabled flag */
DCL int Delay IVAL(1);		/* Delay time in seconds */

DCL equ_t DisEq IVAL(0);	/* Last equipment displayed */
DCL equ_t DisRb IVAL(0);	/* Last robot displayed */
DCL equ_t DisTp IVAL(0);	/* Last third party messages displayed */
DCL equ_t DisRm IVAL(0);	/* Last remote device displayed */

DCL char *Mount_Point;		/* SAM-FS mount point */
DCL int Ioctl_fd IVAL(0);	/* Mount point file descriptor */
DCL char *File_System;		/* SAM-FS filesystem family set name */
DCL int FsInitialized IVAL(0);	/* Filesystem initialized */
DCL int ln;			/* Line number on display */
DCL boolean ScreenMode;		/* TRUE if output is a terminal */
DCL boolean IsSam IVAL(FALSE);	/* TRUE if sam_init binary is present */
DCL boolean IsSamRunning IVAL(FALSE);
				/* TRUE if shared memory is present */

#endif /* !defined(SAMU_H) */
