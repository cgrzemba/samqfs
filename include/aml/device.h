/*
 * device.h - SAM-FS device information.
 *
 * Description:
 * Definitions for SAM-FS devices and dev_ent link list device table.
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

#ifndef _AML_DEVICE_H
#define	_AML_DEVICE_H

#pragma ident "$Revision: 1.49 $"


#ifdef sun
#include <thread.h>
#include <synch.h>
#include <sys/mutex.h>
#endif /* sun */

#include "sam/types.h"
#include "aml/types.h"
#include "pub/devstat.h"
#include "sam/mount.h"
#include "sam/fs/amld.h"
#include "aml/catalog.h"
#ifdef sun
#include "aml/scan.h"
#include "aml/scsi.h"
#endif /* sun */
#include "aml/sefstructs.h"


/* Macro to determine if a device (non disk) uses a scsi interface */

#define	is_scsi(un)   ((((un)->type & DT_CLASS_MASK) == DT_OPTICAL) || \
	(((un)->type & DT_CLASS_MASK) == DT_TAPE) || \
	(((un)->type & DT_SCSI_ROBOT_MASK) == DT_SCSI_ROBOT_MASK))

/* Decrement the open count */
#define	DEC_UNIT(un)  if ((un)->open_count > 0) --(un)->open_count

/* Decrement/Increment the open count, return TRUE if still opened */
#define	DEC_OPEN(un)  { (((un)->open_count > 0) ? --(un)->open_count : 0); \
	(un->status.b.opened = ((un)->open_count == 0) ? FALSE : TRUE); }
#define	INC_OPEN(un)  { ((un)->open_count++); \
	(un->status.b.opened = ((un)->open_count == 0) ? FALSE : TRUE); }

/* Decrement active, return active */
#if	!defined(TRACE_ACTIVES)
#define	DEC_ACTIVE(un)  (((un)->active > 0) ? --(un)->active : 0)
#define	INC_ACTIVE(un)	((un)->active++)
#endif


#define	MAX_DEVICES	65536	/* Max number of devices */
#define	MAX_ROBOTS	32	/* Max number of robotic devices */
#define	MAX_SET		32	/* Max number of sets */
#define	SERIAL_LEN	15	/* Length of the dev_ent serial num */
/*
 * If the line length changes(pub/mig.h) then this must
 * be changed to reflect this change
 */
#define	TP_DISP_MSG_LEN	(10 * 81)	/* Messages for display in third */
					/* party api  */
#define	USER_STATE_CHANGE	1	/* the user down'ed or off'ed dev */
#define	SAM_STATE_CHANGE	0	/* SAM down'ed or off'ed the device */
#define	NAME_TO_MAJOR	"/etc/name_to_major"	/* driver name to major dev # */
#define	SUPPORTED_TAPE_DRIVER	"st"	/* The solaris tape driver */

/* Some handy macros */
/*  returns true if equivalent or equivalent device class */
#define	DT_EQUIV(a, b)	((a) == (b) || ((a) & DT_CLASS_MASK) == (b))

/* return true is the device is whatever */
#define	IS_DISK(a)	(((a)->equ_type & DT_CLASS_MASK) == DT_DISK)
#define	IS_OPTICAL(a)	(((a)->equ_type & DT_CLASS_MASK) == DT_OPTICAL)
#define	IS_ROBOT(a)	(((a)->equ_type & DT_CLASS_MASK) == DT_ROBOT || \
	((a)->equ_type == DT_HISTORIAN))
#define	IS_HISTORIAN(a)	((a)->equ_type == DT_HISTORIAN)
#define	IS_MANUAL(a)	((a)->fseq == 0)
#define	IS_TAPE(a)	(((a)->equ_type & DT_CLASS_MASK) == DT_TAPE)
#define	IS_FS(a)	(((a)->equ_type & DT_CLASS_MASK) == DT_FAMILY_SET)
#define	IS_TAPELIB(a)	(((a)->equ_type & DT_TAPE_R) == DT_TAPE_R)
#define	IS_THIRD_PARTY(a) (((a)->equ_type & DT_CLASS_MASK) == DT_THIRD_PARTY)
#define	IS_STRIPE_GROUP(a) (((a)->equ_type & DT_STRIPE_GROUP_MASK == \
	DT_STRIPE_GROUP)
#define	IS_OSD_GROUP(a) (((a)->equ_type & DT_CLASS_MASK == \
	DT_OBJECT_DISK)
#define	IS_RSS(a)	((a)->equ_type == DT_PSEUDO_SS)
#define	IS_RSC(a)	((a)->equ_type == DT_PSEUDO_SC)
#define	IS_RSD(a)	((a)->equ_type == DT_PSEUDO_RD)
#define	IS_STK5800(a)	((a)->equ_type == DT_STK5800)


/* Returns true if this is a generic API interface to the robot */
#define	IS_GENERIC_API(_x) ((_x) == DT_GRAUACI)

#define	IS_IBM_COMPATIBLE(_x)	(((_x) == DT_3592 || (_x) == DT_3590 || \
	(_x) == DT_3570 || (_x) == DT_FUJITSU_128))

/* Returns true if drive is "idle" */
#define	IS_IDLE(drive) (!drive->status.b.offline && \
	drive->un->active == 0) && \
	drive->un->open_count == 0 && \
	(drive->un->state < DEV_IDLE))

/* get_media() return values */
#define	RET_GET_MEDIA_SUCCESS		(0)
#define	RET_GET_MEDIA_DISPOSE		(1)	/* dispose error */
#define	RET_GET_MEDIA_REQUEUED		(2)	/* event was requeued */
#define	RET_GET_MEDIA_DOWN_DRIVE	(-1)	/* dispose of event, */
						/* down the drive */
#define	RET_GET_MEDIA_RET_ERROR		(-2)	/* dispose error */
						/* return error */
#define	RET_GET_MEDIA_RET_ERROR_BAD_MEDIA (-3)	/* same as above */

/* True if the error is fatal */
#define	IS_GET_MEDIA_FATAL_ERROR(_x)	(((_x) == RET_GET_MEDIA_DOWN_DRIVE) || \
	((_x) == RET_GET_MEDIA_RET_ERROR) || \
	((_x) == RET_GET_MEDIA_RET_ERROR_BAD_MEDIA))

/* media types */

#define	MEDIA_NONE	0
#define	MEDIA_WORM	2
#define	MEDIA_RW	3
#define	MEDIA_INVALID	0xff

/* dev_status */

typedef struct {
	uint32_t
#if defined(_BIT_FIELDS_HTOL)
		maint		:1,	/* Maint mode */
		scan_err	:1,	/* Scanner found some bad stuff */
		audit		:1,	/* Device is in audit state */
		attention	:1,	/* Needs oper attention */

		scanning	:1,	/* Scanner active */
		mounted		:1,	/* FS mounted or if robot then media */
					/* labeled or im/ex(ported) media. */
					/* Used by licensing code. */
		scanned		:1,	/* Media has been scanned */
		read_only	:1,	/* Media is read-only */

		labeled		:1,	/* Media has a label */
		wr_lock		:1,	/* Write lockout */
		unload		:1,	/* Unload has been requested */
		requested	:1,	/* Someone wants the device */

		opened		:1,	/* Someone has the device opened */
		ready		:1,	/* Spun up and readu */
		present		:1,	/* Device is there */
		bad_media	:1,	/* Media is unusable */

		stor_full	:1,	/* All slots occupied/media full */
		i_e_port	:1,	/* Waiting for operator import/export */
		unused0		:1,
		cleaning	:1,	/* Unit needs cleaning */

		positioning	:1,	/* Locate command issued (tape only) */
		forward		:1,	/* Positioning is toward eot */
		wait_idle	:1,	/* Waiting for device to idle. */
		fs_active	:1,	/* File system used this device */

		write_protect	:1,	/* Media is write protected */
		strange		:1,	/* Media is not from sam */
		stripe		:1,	/* Mount point using striping */
		labelling	:1,	/* Label being written */
		shared_reqd	:1,	/* Shared drive has been requested */
		unused		:3;

#else	/* defined(_BIT_FIELDS_HTOL) */

		unused		:3,
		shared_reqd	:1,	/* shared drive has been requested */
		labelling	:1,	/* label being written */
		stripe		:1,	/* mount point using striping */
		strange		:1,	/* Media is not from sam */
		write_protect	:1,	/* Media is write protected */

		fs_active	:1,	/* File system used this device */
		wait_idle	:1,	/* Waiting for device to idle. */
		forward		:1,	/* Positioning is toward eot */
		positioning	:1,	/* Locate command issued (tape only) */

		cleaning	:1,	/* Unit needs cleaning */
		unused0		:1,
		i_e_port	:1,	/* Waiting for operator import/export */
		stor_full	:1,	/* All slots occupied/media full */

		bad_media	:1,	/* Media is unusable */
		present		:1,	/* Device is there */
		ready		:1,	/* Spun up and readu */
		opened		:1,	/* Someone has the device opened */

		requested	:1,	/* Someone wants the device */
		unload		:1,	/* Unload has been requested */
		wr_lock		:1,	/* Write lockout */
		labeled		:1,	/* Media has a label */

		read_only	:1,	/* Media is read-only */
		scanned		:1,	/* Media has been scanned */
		mounted		:1,	/* FS mounted or if robot then media */
					/* labeled or im/ex(ported) media. */
					/* Used by licensing code. */
		scanning	:1,	/* Scanner active */

		attention	:1,	/* Needs oper attention */
		audit		:1,	/* Device is in audit state */
		scan_err	:1,	/* Scanner found some bad stuff */
		maint		:1;	/* Maint mode */
#endif  /* defined(_BIT_FIELDS_HTOL) */

} dev_status_t;

/*
 * Defines for the above.
 * If you change the stuff above, change this.
 */
/* the following already defined in pub/devstat.h */
#if 0
#define	 DVST_MAINT	0x80000000
#define	 DVST_SCAN_ERR	0x40000000
#define	 DVST_AUDIT	0x20000000
#define	 DVST_ATTENTION	0x10000000

#define	 DVST_SCANNING	0x08000000
#define	 DVST_MOUNTED	0x04000000
#define	 DVST_SCANNED	0x02000000
#define	 DVST_READ_ONLY	0x01000000

#define	 DVST_LABELED	0x00800000
#define	 DVST_WR_LOCK	0x00400000
#define	 DVST_UNLOAD	0x00200000
#define	 DVST_REQUESTED	0x00100000

#define	 DVST_OPENED	0x00080000
#define	 DVST_READY	0x00040000
#define	 DVST_PRESENT	0x00020000
#define	 DVST_BAD_MEDIA	0x00010000

#define	 DVST_STOR_FULL	0x00008000
#define	 DVST_I_E_PORT	0x00004000
#define	 DVST_STAGE_ACT	0x00002000
#define	 DVST_CLEANING	0x00001000

#define	 DVST_POSITION	0x00000800
#define	 DVST_FORWARD	0x00000400
#define	 DVST_WAIT_IDLE	0x00000200
#define	 DVST_FS_ACTIVE	0x00000100
#endif

#define	 DVST_WRITE_PROTECT	0x00000080
#define	 DVST_STRANGE	0x00000040
#define	 DVST_STRIPE	0x00000020
#define	 DVST_LABELLING	0x00000010

/*
 * This macro 'returns' the number of loops to wait on the device opened field
 * given a wait time per loop(in secs).
 */
#define	 DEV_OPEN_WAIT_COUNT(_x)	(300 / (_x))	/* 5 minutes */



/* generic robot status */

typedef struct  {
	uint_t
#if defined(_BIT_FIELDS_HTOL)
		barcodes	:1,	/* Barcodes are supported */
		export_unavail	:1,	/* Export is unavail */
		shared_access	:1,	/* Shared access (ibm3494) */
		unused		:29;
#else /* defined(_BIT_FIELDS_HTOL) */
		unused		:29,
		shared_access	:1,	/* Shared access (ibm3494) */
		export_unavail	:1,	/* Export is unavail */
		barcodes	:1;	/* Barcodes are supported */
#endif /* defined(_BIT_FIELDS_HTOL) */
} robot_status_t;

/* tape properties */

typedef enum {
	PROPERTY_NONE = 0x0000,			/* regular drive and media */
	PROPERTY_WORM_DRIVE = 0x0001,		/* worm drive */
	PROPERTY_WORM_CAPABLE = 0x0002,		/* drive worm capable (fw) */
	PROPERTY_WORM_MEDIA = 0x0004,		/* worm media */
	PROPERTY_WORM = 0x0007,			/* worm (fw, media) */
	PROPERTY_VOLSAFE_DRIVE = 0x0008,	/* stk volsafe drive */
	PROPERTY_VOLSAFE_CAPABLE = 0x0010,	/* stk volsafe capable */
						/* (fw, passwd) */
	PROPERTY_VOLSAFE_MEDIA = 0x0020,	/* stk volsafe media */
	PROPERTY_VOLSAFE = 0x0038,		/* stk volsafe */
						/* (fw, passwd, media) */
	PROPERTY_VOLSAFE_PERM_LABEL = 0x0040,	/* stk volsafe */
						/* permanent label */
	PROPERTY_ENCRYPTION_DRIVE = 0x0100	/* encryption drive */
} properties_t;

#define	VOLSAFE_LABEL_ERROR	(1)	/* attempt to re-label volsafe media */

/* device identification (world wide name, port, etc) */

typedef enum {
	IDENT_ASSOC_DEV,	/* addr physical or logical device */
	IDENT_ASSOC_PORT	/* port received request */
} ident_assoc_t;		/* device identification type */

#define	IDENT_LEN	50	/* ascii identifier length */

typedef struct {
	char	ident[IDENT_LEN];	/* wwn or vendor identifier */
	ident_assoc_t	assoc;		/* association  */
	uchar_t		type;		/* identifier type */
} ident_data_t;				/* identifier data */

typedef enum {
	FCP = 0,	/* fibre channel */
	SPI = 1,	/* parallel scsi */
	SSA = 2,	/* ssa */
	IEEE_1394 = 3,	/* ieee 1394 */
	SRP = 4,	/* srp */
	iSCSI = 5	/* iscsi */
} protocol_id_t;	/* device protocol types */

typedef struct {
	boolean_t	lun_valid;	/* lun interface valid */
	protocol_id_t	lun;		/* lun interface */
	boolean_t	port_valid;	/* port interface valid */
	protocol_id_t	port;		/* port interface */
} protocol_t;	/* device protocol */

#define	IDENT_NUM	10	/* max num of ident per device */

typedef struct {
	boolean_t	multiport;	/* dual ported device */
	int		count;		/* number of ascii idents */
	ident_data_t	data[IDENT_NUM];	/* ascii identifiers */
	boolean_t	port_id_valid;	/* port identifier valid */
	ulong_t		port_id;	/* 1 is port A, 2 is port B, ... */
	protocol_t	protocol;	/* mode protocol specific pages */
} dev_id_t;    /* device identification */

#ifdef sun
#define	MUTEX_T		mutex_t
#define	THREAD_T	thread_t
#endif /* sun */

#ifdef linux
#define	THREAD_T	pthread_t
#define	SAM_CDB_LENGTH	12	/* max cdb length */
#if (KERNEL_MAJOR > 4) && !defined(__ia64)
#ifdef __KERNEL__
#include <asm/semaphore.h>
#define	MUTEX_T	struct semaphore
#else
#include <semaphore.h>
#define	MUTEX_T		sem_t
#endif /* __KERNEL__ */
#else
#define	MUTEX_T		kmutex_t
#endif /* version */
#endif /* linux */

#define	FIVE_MINS_IN_SECS 300
#define	SEF_INTERVAL_ONCE 1
#define	SEF_INTERVAL_DEFAULT SEF_INTERVAL_ONCE

typedef enum {				/* Sef sysevent state */
	SEF_ENABLED = 0x01,		/* Feature enabled */
	SEF_SUPPORTED = 0x02,		/* Device and os support feature */
	SEF_WRT_ERR_COUNTERS = 0x04,	/* Log sns page 2 supported */
	SEF_RD_ERR_COUNTERS = 0x08,	/* Log sns page 3 supported */
	SEF_SYSEVENT = 0x10,		/* Os supports sysevent */
	SEF_POLL = 0x20			/* Sample device log sense */
} sef_state_t;

typedef struct {			/* Sef log sense sample rate */
	sef_state_t	state;		/* Feature state */
	int		interval;	/* Log sense sample rate */
	int		counter;	/* Elasped wall clock time */
} sef_sample_t;

typedef struct {			/* Optical disk device */
	uint_t		unused1;	/* Keep alignment with pseudo device */
	void		*unused2;	/* Keep alignment with pseudo device */
	uint_t		next_file_fwa;	/* Next file first word address */
	uint_t		ptoc_fwa;	/* Partition fwa */
	uint_t		ptoc_lwa;	/* Partition lwa */
	short		fs_alloc;	/* File system allocation multiplier */
	uchar_t		medium_type;	/* Media type from mode sense */
} optical_device_t;

/*
 * First entries must look like a generic robot entry
 * (see struct robotic_device below).
 */
typedef struct {			/* Remote Sam client */
	uint_t		message;	/* Message area in shm seq */
	uint_t		private;	/* Private area */
	pid_t		process;	/* Process id */
	upath_t		name;		/* File name of, or path to, catalog */
	struct srvr_clnt *server;	/* Server info */
	ushort_t	data_port;	/* Port to use for data socket */
} remote_sam_client_t;

typedef struct {			/* Remote Sam server */
	uint_t		message;	/* Message area in shm seq */
	struct srvr_clnt *clients;	/* Clients */
	void		*private;	/* Process private to server */
	uint_t		ordinal;	/* Ordinal of server on this host */
	ushort_t	serv_port;	/* Server port this server */
} remote_sam_server_t;

typedef struct {			/* Remote Sam pseudo device */
	uint_t		message;	/* Message area in shm seq */
	void		*private;	/* Process private to pseudo device */
} remote_sam_pseudo_t;

typedef struct {			/* Magnetic tape device */
	/*
	 * These fields under protection of io_mutex
	 * The samst_name path is used ONLY for non-tape-motion scsi
	 * commands, like mode-sense. The samst_name can be opened when
	 * there is no media in the device.
	 */
	uint_t		unused1;	/* keep alignment with pseudo device */
	void		*unused2;	/* keep alignment with pseudo device */
	uint_t		position;	/* Current end of data (if known)) */
	uint_t		stage_pos;	/* Position of the tar image */
	uint_t		next_read;	/* Position of next read (blocks) */
	uint_t		default_blocksize; /* Default blocksize */
	uint_t		position_timeout; /* Default position timeout */
	uint_t		max_blocksize;	/* Max blocksize */
	uint64_t	default_capacity; /* Default capacity */
	uint_t		driver_blksize;	/* Block size for driver (d2) */
	uint_t		fsn;		/* File sequence number if known (d2) */
	uint_t		mask;		/* Mask to normalize position */
	union {
		struct {
			uint_t
#if defined(_BIT_FIELDS_HTOL)
				fix_block_mode	:1,	/* Fixed blocks */
				compression	:1,	/* Compression on */
				needs_format	:1,	/* Media unformatted */
				unused		:29;
#else	/* defined(_BIT_FIELDS_HTOL) */
				unused		:29,
				needs_format	:1,	/* Media unformatted */
				compression	:1,	/* Compression on */
				fix_block_mode	:1;	/* Fixed blocks */
#endif  /* defined(_BIT_FIELDS_HTOL) */
		} b;
		uint_t   bits;
	} status;
	upath_t		samst_name;	/* Path of samst access */
	ushort_t	drive_index;	/* What tape device driver */
	uchar_t		medium_type;	/* Media type from mode sense */
	properties_t	properties;	/* Regular, Worm, VolSafe  */
} tape_device_t;

typedef struct {			/* Generic robotic device */
	/*
	 * The robot entry and the remote sam client entry must have
	 * the following as the first entries.
	 *
	 * uint_t   message
	 * uint_t   private
	 * pid_t    process
	 * uname_t  name;
	 */
	uint_t		message;	/* Message area in shm seq */
	uint_t		private;	/* Robots private area */
	pid_t		process;	/* Process id */
	upath_t		name;		/* File name/ path to catalog */
	uint_t		port_num;	/* Port number used by stkd */
	uint_t		capid;		/* Capid used by sam-stkd */
	union { 			/* Protected by main mutex */
		robot_status_t	b;
		uint_t		bits;
	} status;
} robotic_device_t;


typedef struct {			/* Third party device types */
	uint_t		message;	/* Message area in shm seq */
	uint_t		disp_msg;	/* Display area in shm */
	pid_t		process;	/* Process id */
} third_party_device_t;


typedef enum {
	TAPECLEAN_AUTOCLEAN = 0x01,	/* auto-clean feature enabled */
	TAPECLEAN_LOGSENSE = 0x02,	/* auto-clean with log sense */
	TAPECLEAN_MEDIA = 0x04,		/* expired cleaning media chk */
	TAPECLEAN_DRIVE = 0x00		/* is drive dirty after cln chk */
} tapeclean_t;

/*
 * ----- dev_ent - Device configuration table.
 * Unless noted, all fields are protected by the main(mutex) mutex_t.
 * DO NOT hold this mutex for long periods of time.
 *
 * Any I/O done on this un(using samst or standard device drivers) must be
 * done with the io_mutex held THROUGH THE ENTIRE I/O PROCESS.  The clever
 * programmer will observe that the io_mutex could be held for
 * long periods of time.  Be prepaired for that(see mutex_trylock).
 *
 * It is bad form to hold both of these mutexs at the same
 * time.  Don't do it...  I/O cannot be done without the open count and
 * active being greater than zero.  Get the mutex, check for activity,
 * open if needed, increment counts, release mutex.  This should keep
 * others off the device while you do you thing.
 *
 * entry_mutex is a USYNC_THREAD mutex and protects what cat_ent_t *entry
 * points to.  The pointer itself is protected by the main mutex.
 */

typedef struct dev_ent  {
	MUTEX_T		mutex;		/* Protects device fields and cond's */
	MUTEX_T		io_mutex;	/* Mutex for I/O */
	MUTEX_T		entry_mutex;	/* For the cat_ent*  */
	struct dev_ent	*next;		/* Pointer (offset) to next entry */
#if !defined(_LP64)
 	int		de_pad0;
#endif
	THREAD_T	scan_tid;	/* Thread id of scanner/drive thread */
	THREAD_T	io_tid;		/* Thread id of stage/mount */
	THREAD_T	helper_tid;	/* Thread id of helper */
	THREAD_T	misc_tid;	/* Thread id of misc */
	upath_t		name;		/* Device name */
	uname_t		set;		/* Set name */
	equ_t		eq;		/* Equipment number */
	equ_t		fseq;		/* Family set equipment number */
	dtype_t		type;		/* Medium type */
	dtype_t		equ_type;	/* Equipment type */
	dstate_t	state;		/* State - on/ro/idle/off/down */
	ushort_t	ord;		/* Ordinal within family set */
	ushort_t	model;		/* Device model */
	ushort_t	model_index;	/* Index into sam_model table */
#if !defined(_LP64)			/* XXX cleanup ! */
	dev_t		st_rdev;	/* From stat function request */
#else
	dev_t		st_rdev;	/* From stat function request */
#endif
	time_t		mtime;		/* Time to dismount */
#if !defined(_LP64)
	int		de_pad1;
#endif
	media_t		media;		/* Media type for this device */
	struct mode_sense *mode_sense;	/* Pointer (offset) to mode_sense */
#if !defined(_LP64)
	int		de_pad4;
#endif
	struct sam_extended_sense *sense; /* Pointer (offset) to sense data */
#if !defined(_LP64)
	int		de_pad5;
#endif
	struct sam_act_io *active_io;	/* Pointer (offset) io structure */
#if !defined(_LP64)
	int		de_pad6;
#endif
	uint_t		flags;
	uint_t		active;		/* Active count */
	uint_t		open_count;	/* Number of opens (real or implied) */
	uint_t		label_address;	/* Sector where label was found */
	uint_t		delay;		/* Delay time for dismount/wait */
	uint_t		unload_delay;	/* Delay between spindown and unload */

	/* Catalog entry data. */
	struct VolId	i;		/* Volume identification from catalog */
	time_t		label_time;	/* Time media was labeled */
#if !defined(_LP64)
 	int		de_pad2;
#endif
	uint_t		slot;		/* Slot in catalog table */
	uint64_t	space;		/* Space remaining in blocks (1024) */
	uint64_t	capacity;	/* Capacity in blocks (1024) */
	uint_t		sector_size;	/* Sector size of this device */
	int		mid;		/* Mid of catalog entry */
	int		flip_mid;	/* If two sided, the other side */
	/*
	 * Watch it.  The stk code uses the following fields to get
	 * a 64 byte area for the access id.
	 */
	vsn_t		vsn;			/* Vsn of mounted media */
	uchar_t		vendor_id[8+1];		/* Vendor id from inquiry */
	uchar_t		product_id[16+1];	/* Product id from inquiry */
	uchar_t		revision[4+1];		/* Revision from inquiry */
	uchar_t		serial[SERIAL_LEN+1];	/* Serial number of device */
	uchar_t		scsi_type;		/* Device type from inquiry */
	uchar_t		io_active;		/* The cdb is outstanding */
						/* (updated under io_mutex) */
	uchar_t		cdb[SAM_CDB_LENGTH];	/* SCSI cdb */
	uchar_t		pages;		/* Mode sense pages support */
	char	dis_mes[DIS_MES_TYPS][DIS_MES_LEN+1];	/* device messages */

	struct {
		MUTEX_T		lmutex;
		int		fd;	/* Devlog file descriptor */
		int		pid;	/* Process "owning" device log */
		uint_t		flags;	/* Event listing controls */
		time_t		last_opened; /* When devlog was last opened */
#if !defined(_LP64)
	 	int		de_pad3;
#endif
	} log;

	union {
		dev_status_t	b;	/* Device status */
		uint32_t	bits;	/* Nice way to clear all bits */
	} status;

	/*
	 * od, tp and any other real removable media I/O device must
	 * have the same layout at the beginning of their entries as
	 * the remote sam pseudo device.
	 */
	union {
		optical_device_t	od;	/* Optical disk device */
		remote_sam_client_t	sc;	/* Remote Sam client */
		remote_sam_server_t	ss;	/* Remote Sam server */
		remote_sam_pseudo_t	sp;	/* Remote Sam pseudo device */
		tape_device_t		tp;	/* Magnetic tape device */
		robotic_device_t	rb;	/* Generic robotic device */
		third_party_device_t	tr;	/* Third party device */
	} dt;

	struct sef_devinfo	sef_info;	/* Sef info for this device */
	sef_sample_t		sef_sample;	/* Sef data sample */
#if defined(ROBOT_MASK)
	ushort_t	nrobots;		/* Number of robots */
	uname_t		robot[MAX_ROBOTS];	/* Robot name */
#endif

	/* TapeAlert Log Sense */
	uchar_t		version;		/* Inquiry version */
	uchar_t		tapealert;		/* Tapealert bits */
	uint64_t	tapealert_flags;	/* Previous tapealert flags */
	vsn_t		tapealert_vsn;		/* Previous tapealert vsn */

	dev_id_t	devid;		/* Device identification */

	tapeclean_t	tapeclean;	/* Tapealert and seq access cleaning */
} dev_ent_t;

/* For the flags */
#define	DVFG_CAP_VALID		0x80000000	/* Catalog update only */
#define	DVFG_SPACE_VALID	0x40000000	/* Catalog update only */
#define	DVFG_SHARED		0x20000000	/* Drive is shared with SAM's */

/* The dt.tp.status flags */
#define	DVTP_FIXBLOCK		0x80000000
#define	DVTP_COMPRESSION	0x40000000
#define	DVTP_NEEDFORMAT		0x20000000

/* Tape alert flags */
#define	TAPEALERT_ENABLED	0x01	/* User wants tapealert on */
#define	TAPEALERT_SUPPORTED	0x02	/* Hardware supports tapealert */
#define	TAPEALERT_INIT_QUERY	0x04	/* Initial tapealert query */

#ifdef sun

/*
 * dev_ptr_tbl - Device pointer table.
 *
 * The device table is built by sam-amld from the mcf
 * file. It is indexed by the equipment ordinal. A NULL pointer
 * indicates no device present.
 */
typedef struct {			/* Device pointer table */
	MUTEX_T		mutex;		/* Protects device ptr table */
	int		max_devices;
	dev_ent_t	*d_ent[1];	/* Array of device pointers */
} dev_ptr_tbl_t;


#include "aml/tapes.h"

#if	defined(TRACE_ACTIVES)
extern int DeC_AcTiVe(char *name, int line, dev_ent_t *un);
#define	DEC_ACTIVE(un)  DeC_AcTiVe(__FILE__, __LINE__, un)

extern int InC_AcTiVe(char *name, int line, dev_ent_t *un);
#define	INC_ACTIVE(un)  InC_AcTiVe(__FILE__, __LINE__, un)

#endif

/* Active mount structure.  Used by mount_thread */
typedef struct sam_actmnt {
	dev_ent_t		*un;
	enum callback		callback;
	sam_handle_t		handle;
	sam_resource_t		resource;
} sam_actmnt_t;

typedef struct sam_io_reader {
	MUTEX_T		mutex;
	cond_t		io_running;	/* Set by reader helpers when ready */
	cond_t		start_reader;	/* Flag to start the reader_helper */
	sema_t		fullbufs;
	sema_t		emptybufs;
	struct {
		THREAD_T	tid;
		uint_t
#if defined(_BIT_FIELDS_HTOL)
			cancel		: 1,	/* cancel the current request */
			exiting		: 1,	/* helper is exiting */
			abort		: 1,	/* helper should exit */
			wait_buf	: 1,	/* helper waiting for buffer */

			paused		: 1,	/* helped has paused */
			rdr_rdy		: 1,	/* helper is ready */
			stage_n		: 1,	/* no tar header check */
			locked_buffers	: 1,	/* stage buffers locked */

			directio	: 1,	/* directio for stage */
			unused		:23;
#else	/* defined(_BIT_FIELDS_HTOL) */
			unused		:23,
			directio	: 1,
			locked_buffers	: 1,
			stage_n		: 1,
			rdr_rdy		: 1,
			paused		: 1,
			wait_buf	: 1,
			abort		: 1,
			exiting		: 1,
			cancel		: 1;
#endif /* defined(_BIT_FIELDS_HTOL) */
	} helper;
	uint_t		ctl_rdy	: 1;	/* control is ready */
	int		num_buffs;	/* number of io buffers */
	int		open_fd;
	dev_ent_t	*un;
	sam_handle_t	handle;
	void		*media_info;	/* points to resource or rm_info */
	void		*buf_fwa;
} sam_io_reader_t;

typedef struct sam_act_io {
	MUTEX_T		mutex;
	cond_t		cond;
	sam_handle_t	handle;
	sam_resource_t	resource;
	int		final_io_count;
	uint_t		block_count;
	u_longlong_t	setpos;		/* set position of rmedia file */
	int		fd_stage;	/* file descriptor passed from */
					/* dodirio() when staging to */
					/* avoid multiple opens */
	uint_t
		fs_cancel	: 1,
		timeout		: 1,
		active		: 1,
		wait_fs_unload	: 1,
		unused		: 28;
} sam_act_io_t;


#endif /* sun */


#endif /* _AML_DEVICE_H */
