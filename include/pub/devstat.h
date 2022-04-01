/*
 * devstat.h - SAM-FS device information definitions.
 *
 * Defines the SAM-FS device information structure and functions.
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

#ifndef SAM_DEVSTAT_H
#define	SAM_DEVSTAT_H

#ifdef sun
#pragma ident "$Revision: 1.72 $"
#endif

#ifdef	linux

#ifdef	__KERNEL__
#include <linux/types.h>
#else	/* __KERNEL__ */
#include <sys/types.h>
#endif	/* __KERNEL__ */
#include "sam/linux_types.h"
#else	/* linux */

#include <sys/types.h>

#endif /* linux */

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Device states for disk and tape.
 * Disk only states: on, noalloc, unavail, off, down
 * Tape only states: on, ro, idle, unavail, off, down
 */
typedef enum dstate {
	DEV_ON,			/* Normal operations */
	DEV_NOALLOC,		/* No allocation on this device */
	DEV_RO,			/* Read only operations */
	DEV_IDLE,		/* No new opens allowed */
	DEV_UNAVAIL,		/* Unavailable for file system */
	DEV_OFF,		/* Off to this machine */
	DEV_DOWN 		/* Maintenance use only */
} dstate_t;

#if defined(DEC_INIT) && !defined(lint)
char *dev_state[] = {
	"on", "noalloc", "ro", "idle", "unavail", "off", "down", NULL};
#else /* defined(DEC_INIT) */
extern char *dev_state[];
#endif /* defined(DEC_INIT) */

struct sam_devstat {
	ushort_t type;		/* Medium type */
	char name[32];		/* Device name */
	char vsn[32];		/* VSN of mounted volume, 31 characters */
	dstate_t state;		/* State - on/ro/idle/off/down */
	uint_t status;		/* device status */
	uint_t space;		/* Space left on device */
	uint_t capacity;	/* Capacity in blocks (1024) */
};

struct sam_ndevstat {
	ushort_t type;		/* Medium type */
	char name[128];		/* Device name */
	char vsn[32];		/* VSN of mounted volume, 31 characters */
	dstate_t state;		/* State - on/ro/idle/off/down */
	uint_t status;		/* device status */
	uint64_t space;		/* Space left on device */
	uint64_t capacity;	/* Capacity in blocks (1024) */
};

/* Device status. */
#define	DVST_MAINT	0x80000000
#define	DVST_SCAN_ERR	0x40000000
#define	DVST_AUDIT	0x20000000
#define	DVST_ATTENTION	0x10000000

#define	DVST_SCANNING	0x08000000
#define	DVST_MOUNTED	0x04000000
#define	DVST_SCANNED	0x02000000
#define	DVST_READ_ONLY	0x01000000

#define	DVST_LABELED	0x00800000
#define	DVST_WR_LOCK	0x00400000
#define	DVST_UNLOAD	0x00200000
#define	DVST_REQUESTED	(uint32_t) 0x00100000

#define	DVST_OPENED	0x00080000
#define	DVST_READY	0x00040000
#define	DVST_PRESENT	0x00020000
#define	DVST_BAD_MEDIA	0x00010000

#define	DVST_STOR_FULL	0x00008000
#define	DVST_I_E_PORT	0x00004000
/* Unused		0x00002000 */
#define	DVST_CLEANING	0x00001000

#define	DVST_POSITION	0x00000800
#define	DVST_FORWARD	0x00000400
#define	DVST_WAIT_IDLE	(uint32_t) 0x00000200
#define	DVST_FS_ACTIVE	0x00000100

/* Device types */
#define	DT_CLASS_MASK		0xff00	/* Device class type mask */
#define	DT_OSD_MASK		0x00ff	/* Device OSD type mask */
#define	DT_STRIPE_GROUP_MASK	0xff80	/* Device class mask for striped grp */
#define	DT_MEDIA_MASK		0x007f	/* Device media type mask */
#define	DT_SCSI_ROBOT_MASK	0x1880	/* Device is a scsi robot */
#define	DT_CLASS_SHIFT		8	/* Device class type shift */

#define	DT_DISK			0x100	/* Disk storage */
#define	DT_STRIPE_GROUP		0x180	/* Disk storage for striped group */
#define	DT_DATA			0x101	/* Disk storage for sm/lg daus */
#define	DT_META			0x102	/* Disk storage for meta data */
#define	DT_RAID			0x103	/* Disk storage for raid data */
#define	DT_STK5800		0x104	/* STK 5800 archive storage */

#define	DT_TAPE			0x200	/* Tape */
#define	DT_VIDEO_TAPE		0x201	/* VHS Video tape */
#define	DT_SQUARE_TAPE		0x202	/* Square tape (3480) */
#define	DT_EXABYTE_TAPE		0x203	/* 8mm Exabyte tape */
#define	DT_LINEAR_TAPE		0x204	/* Digital linear tape */
#define	DT_DAT			0x205	/* 4mm dat (Python) */
#define	DT_9490			0x206	/* Square tape (9440) */
#define	DT_D3			0x207	/* D3 */
#define	DT_xx			0x208	/* currently unused and available */
#define	DT_3590			0x209	/* IBM 3590 */
#define	DT_3570			0x20a	/* IBM 3570 */
#define	DT_SONYDTF		0x20b	/* Sony DTF 2120 */
#define	DT_SONYAIT		0x20c	/* Sony AIT */
#define	DT_9840			0x20d	/* STK 9840 */
#define	DT_FUJITSU_128		0x20e	/* Fujitsu Diana-4 128track drive */
#define	DT_EXABYTE_M2_TAPE	0x20f	/* 8mm Mammoth-2 Exabyte tape */
#define	DT_9940			0x210	/* STK 9940 */
#define	DT_IBM3580		0x211	/* IBM LTO 3580 */
#define	DT_SONYSAIT		0x212   /* SONY Super AIT */
#define	DT_3592			0x213   /* IBM 3592 and TS1120 drives */
#define	DT_TITAN		0x214	/* STK Titanium drive */

#define	DT_OBJECT_DISK		0x300	/* Object storage device (OSD) */

#define	DT_OPTICAL		0x500	/* Optical disk storage */
#define	DT_WORM_OPTICAL_12	0x501	/* WORM optical disk (12) */
#define	DT_WORM_OPTICAL		0x502	/* WORM optical disk (5 1/4) */
#define	DT_ERASABLE		0x503	/* ERASABLE optical disk (5 1/4) */
#define	DT_MULTIFUNCTION	0x504	/* Multifunction optical disk */
#define	DT_PLASMON_UDO		0x505	/* Plasmon UDO */

#define	DT_FAMILY_SET		0x800	/* Family set */
#define	DT_DISK_SET		0x801	/* Disk family set */
#define	DT_META_SET		0x802	/* Meta data disk family set */
#define	DT_META_OBJECT_SET	0x803	/* Object meta data disk family set */
#define	DT_META_OBJ_TGT_SET	0x804	/* Object meta target disk family set */

#define	DT_ROBOT		0x1800	/* Robot */

#define	DT_SCSI_R		0x1880	/* Use generic robot driver */
#define	DT_TAPE_R		(DT_ROBOT | 0x0040)  /* all tape robot */
#define	DT_TAPE_SR		(DT_SCSI_R | 0x0040) /* all tape scsi robot */
#define	DT_ROBOT_MASK		0x003f	/* Robot type mask */

/* The following 4 groups must have unique numbers in the lower 7 bits. */
/* These are indices into the dev_nmrb[] struct in include/sam/devnm.h */

/* Define non generic scsi robots */
#define	DT_LMS4500	(DT_ROBOT | 1)	/* LMS 4500 RapidChanger */

/* Define generic scsi non tape robots */
#define	DT_CYGNET	(DT_SCSI_R | 2)	/* Cygnet jukebox */
#define	DT_DOCSTOR	(DT_SCSI_R | 3)	/* DocuStore automated library */
#define	DT_HPLIBS	(DT_SCSI_R | 4)	/* HP libraries */
#define	DT_PLASMON_D	(DT_SCSI_R | 25) /* Plasmon DVD-RAM library */
#define	DT_PLASMON_G	(DT_SCSI_R | 37) /* Plasmon G UDO/MO library */

/* Define generic scsi tape robots */
#define	DT_DLT2700	(DT_TAPE_SR | 5)	/* DEC DLT2700 mini-library */
#define	DT_METRUM_LIB	(DT_TAPE_SR | 6)	/* generic metrum libraries */
#define	DT_METD28	(DT_TAPE_SR | 7)	/* metrum D-28 library */
#define	DT_METD360	(DT_TAPE_SR | 8)	/* metrum D-360 library */
#define	DT_ACL_LIB	(DT_TAPE_SR | 9)	/* generic acl libraries */
#define	DT_ACL452	(DT_TAPE_SR | 10)	/* ACL 4/52 */
#define	DT_ACL2640	(DT_TAPE_SR | 11)	/* ACL 2640 */
#define	DT_EXB210	(DT_TAPE_SR | 13)	/* EXABYTE 210 */
#define	DT_ADIC448	(DT_TAPE_SR | 15)	/* ADIC 448 */
#define	DT_SPECLOG	(DT_TAPE_SR | 16)	/* Spectra Logic */
#define	DT_STK97XX	(DT_TAPE_SR | 18)	/* STK 97xx series  */
#define	DT_FJNMXX	(DT_TAPE_SR | 19)	/* Fujitsu NM2XX */
#define	DT_3570C	(DT_TAPE_SR | 20)	/* IBM 3570 Changer  */
#define	DT_SONYDMS	(DT_TAPE_SR | 21)	/* SONY DMS Changer  */
#define	DT_SONYCSM	(DT_TAPE_SR | 22)	/* SONY CSM-20s Tape Library */
#define	DT_ATLP3000	(DT_TAPE_SR | 24)	/* ATL P3000, P4000, P7000 */
#define	DT_ADIC1000	(DT_TAPE_SR | 27)	/* ADIC 1000 and 10K library */
#define	DT_EXBX80	(DT_TAPE_SR | 28)	/* Exabyte X80 library */
#define	DT_STKLXX	(DT_TAPE_SR | 29)	/* STK L20/L40/L80, Sun L7/L8 */
#define	DT_IBM3584	(DT_TAPE_SR | 30)	/* IBM 3584 library */
#define	DT_ADIC100	(DT_TAPE_SR | 31)	/* ADIC 100 library */
#define	DT_HPSLXX	(DT_TAPE_SR | 32)	/* HP SL48 */
#define	DT_HP_C7200	(DT_TAPE_SR | 33)	/* HP C7200 series libraries */
#define	DT_QUAL82xx	(DT_TAPE_SR | 34)	/* Qualstar 82xx series */
#define	DT_ATL1500	(DT_TAPE_SR | 35)	/* ATL M1500/M2500, */
						/* Sun L25/L100 */
#define	DT_ODI_NEO	(DT_TAPE_SR | 36)	/* Overland Data Neo Series */
#define	DT_QUANTUMC4	(DT_TAPE_SR | 38)	/* Sun C4/Quantum PX500 */
#define	DT_SL3000	(DT_TAPE_SR | 39)	/* STK SL3000 library */
#define	DT_SLPYTHON	(DT_TAPE_SR | 40)	/* Spectra Logic Python libs */



/* define non generic (not scsi) tape robots */
#define	DT_GRAUACI	(DT_TAPE_R | 12)	/* GRAU through aci interface */
#define	DT_STKAPI	(DT_TAPE_R | 14)	/* STK through api interface */
#define	DT_IBMATL	(DT_TAPE_R | 17)	/* STK through api interface */
#define	DT_UNUSED2	(DT_TAPE_R | 23)	/* unused */
#define	DT_SONYPSC	(DT_TAPE_R | 26)	/* Sony through api interface */

/*
 * Define Pseudo device types.  If a "driver" is required for
 * any of these, then it is started in "robots" since it already
 * has everything needed to manage the children.  Those marked with %
 * have drivers.
 */
#define	DT_PSEUDO	0x2000
#define	DT_PSEUDO_SSI	(DT_PSEUDO | 1)	/* the stk ssi */
#define	DT_PSEUDO_SC	(DT_PSEUDO | DT_FAMILY_SET | 2) /* remote sam client */
#define	DT_PSEUDO_SS	(DT_PSEUDO | 3) /* remote sam server */
#define	DT_PSEUDO_RD	(DT_PSEUDO | 4) /* remote sam device */
#define	DT_HISTORIAN	(DT_PSEUDO | 5) /* historian */
#define	DT_THIRD_PARTY	0x8000		/* third party */
#define	DT_THIRD_MASK	0x00ff		/* Third party media type mask */

#ifndef	linux
#define	DT_UNKNOWN	0x0		/* Unknown device type */
#endif	/* linux */

/* return true is the type is whatever */
#define	is_disk(a)		(((a) & DT_CLASS_MASK) == DT_DISK)
#define	is_optical(a)		(((a) & DT_CLASS_MASK) == DT_OPTICAL)
#define	is_robot(a)		(((a) & DT_CLASS_MASK) == DT_ROBOT)
#define	is_tape(a)		(((a) & DT_CLASS_MASK) == DT_TAPE)
#define	is_tapelib(a)		(((a) & DT_TAPE_R) == DT_TAPE_R)
#define	is_third_party(a)	(((a) & DT_CLASS_MASK) == DT_THIRD_PARTY)
#define	is_stripe_group(a)	(((a) & DT_STRIPE_GROUP_MASK) == \
				    DT_STRIPE_GROUP)
#define	is_osd_group(a)		(((a) & DT_CLASS_MASK) == DT_OBJECT_DISK)
#define	is_rss(a)		((a) == DT_PSEUDO_SS)
#define	is_rsc(a)		((a) == DT_PSEUDO_SC)
#define	is_rsd(a)		((a) == DT_PSEUDO_RD)
#define	is_stk5800(a)		((a) == DT_STK5800)

int sam_devstat(ushort_t eq, struct sam_devstat *buf, size_t bufsize);

#ifdef  __cplusplus
}
#endif

#endif /* SAM_DEVSTAT_H */
