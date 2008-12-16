/*
 * names.h - name of directories, files, commands.
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


#ifndef _SAM_NAMES_H
#define	_SAM_NAMES_H

#ifdef sun
#pragma ident "$Revision: 1.42 $"
#endif

#define	SAM_CONFIG_PATH "/etc/opt/SUNWsamfs"		/* configuration data */
#define	SAM_SCRIPT_PATH SAM_CONFIG_PATH"/scripts"	/* customized scripts */
#define	SAM_VARIABLE_PATH "/var/opt/SUNWsamfs"		/* variable data */
#define	SAM_EXECUTE_PATH "/opt/SUNWsamfs/sbin"		/* executables */
#define	SAM_LIBRARY_PATH "/opt/SUNWsamfs/lib"		/* libraries */
#define	SAM_SAMFS_PATH "/usr/lib/fs/samfs"		/* samfs programs */

#ifdef linux
#define	MNTTABLK "/etc/mtab~"		/* the mtab lock file */
#endif /* linux */
#ifdef sun
#define	MNTTAB "/etc/mnttab"		/* mnttab file */
#define	MOUNTED	MNTTAB			/* mnttab file (generic def) */
#define	MNTTABLK "/etc/.mnttab.lock"	/* the mnttab lock file */
#endif /* sun */

#define	SAM_AMLD "sam-amld"		/* automated media library daemon */
#define	SAM_ARCHIVER "sam-archiverd"	/* archiver daemon */
#define	SAM_CATSERVER "sam-catserverd"	/* catalog server daemon */
#define	SAM_FSALOGD	"sam-fsalogd"	/* file system activity log daemon */
#define	SAM_DBUPD	"sam-dbupd"	/* sideband database update daemon */
#define	SAM_FSD	"sam-fsd"		/* file system daemon */
#define	SAM_RELEASER "sam-releaser"	/* releaser process */
#define	SAM_RECYCLER "sam-recycler"	/* recycler process */
#define	SAM_NRECYCLER "sam-nrecycler"	/* nrecycler process */
#define	SAM_SHAREFSD "sam-sharefsd"	/* shared filesystem daemon */
#define	SAM_SHRINK "sam-shrink"		/* shrink process */
#define	SAM_STAGEALL "sam-stagealld"	/* stageall daemon */
#define	SAM_STAGER "sam-stagerd"	/* stage daemon */
#define	SAM_RFT "sam-rftd"		/* file transfer daemon */
#define	SAM_RMTSERVER "sam-serverd"	/* remote server daemon */
#define	SAM_RMTCLIENT "sam-clientd"	/* remote client daemon */
#define	SAM_MGMTAPI "fsmgmt"		/* mgmt api */
#define	SAM_SERVICETAG "samservicetag"	/* service tag cmd */

#define	CONFIG "mcf"			/* configuration file name */

/* Sam daemons home or datadirectories. */
#define	SAM_AMLD_HOME SAM_VARIABLE_PATH"/amld"
#define	SAM_ARCHIVERD_HOME SAM_VARIABLE_PATH"/archiver"
#define	SAM_STAGERD_HOME SAM_VARIABLE_PATH"/stager"
#define	SAM_CATALOG_DIR SAM_VARIABLE_PATH"/catalog"

#define	DEV_NOTIFY_PATH SAM_SCRIPT_PATH	/* take action when a dev. is */
					/* down'ed or off'ed */
#define	DEV_NOTIFY_DEFAULT "/dev_down.sh"
#define	DISK_VOLUME_TAB "diskvolume.tab"	/* disk volume table */

/* Script to save core files */
#define	SAVE_CORE_SH SAM_SCRIPT_PATH"/save_core.sh"

/*
 * Paths and filenames involved in locating the GUI resource files
 */
#define	SAM_OPT_PATH "/opt/SUNWsamfs"
#define	SAM_RESFILE "sam-defaults"
#define	SAM_DOTFILE ".samfs"

#endif /* _SAM_NAMES_H */
