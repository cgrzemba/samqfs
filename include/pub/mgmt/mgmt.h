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
#ifndef _MGMT_H
#define	_MGMT_H

#pragma	ident	"$Revision: 1.71 $"

/*
 * mgmt.h - SAMFS APIs for misc operations.
 */



/*
 * IMPORTANT:
 * Version must be incremented in each build that
 * includes API (interface) changes.
 * API implementation changes do not require version updates.
 *
 * API versions included in customer releases (or patches) should follow this
 * notation:
 *  1.0 1.1 ... 1.10 ... 2.0 etc
 *
 * Internal releases should follow this notation:
 *  1.0.1 1.0.2 ... 2.0.1 etc
 * (Note: 1.0.1 is more recent than 1.0)
 *
 * Previous Versions
 * API_VERSION "1.4.4"	Wed Jan 25 17:12:53 EST 2006
 * API_VERSION "1.5.1"	Mon Mar 13 17:12:53 EST 2006
 * API_VERSION "1.5.3"	Mon Mar 28 11:12:53 EST 2006
 * API_VERSION "1.5.4"	Wed Jun 10 17:12:53 EST 2006
 * API_VERSION "1.5.5"	Fri Jun 28 11:38:15 EDT 2006
 * API_VERSION "1.5.6"	Thu Sep 21 3:28:15 EDT 2006
 * API_VERSION "1.5.9"	Mon Oct 2 2:28:15 EDT 2006
 */
#define	API_VERSION "1.6.2"	/* Thu Oct 16 10:39:30 EDT 2008 */


#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

// (lockhart based) UI for SC3.1u3 and above
#define	PLEXMGR_PKGS "SUNWscspm SUNWscspmu SUNWscspmr "
// UI for SC3.1 up to u2
#define	PLEXMGR_OLD_PKGS "SUNWscvr SUNWscvw"

#define	DEFAULT_PACKAGES \
	"SUNWsamfsu SUNWsamfsr SUNWqfsu SUNWqfsr SUNWfsmgru SUNWfsmgrr " \
	"SUNWsamfswm SUNWsampm SUNWsamtp SUNWstade SUNWcqfs SUNWjqfs " \
	"SUNWfqfs SUNWcsamf SUNWjsamf SUNWfsamf SUNWcfsmgr SUNWjfsmgr " \
	"SUNWffsmgr SUNWscu SUNWscr SUNWscdev SUNWmdm " \
	PLEXMGR_PKGS PLEXMGR_OLD_PKGS

/*
 * DESCRIPTION:
 * Function to initialize the library. This function needs to be called
 * before any other function in the library can be called. If other
 * functions are called before initializing the library, then the
 * function will return -1 (indicating an error).
 *
 * If the configuration functions of the API detect that the config files
 * have been modified(except by direct manipulation by the library instance)
 * since this function was called an error will be thrown.
 *
 * This error is really like a notification to the caller that the config
 * files have been changed due to external activity. Another call to this
 * function is required before the user can continue to interact with the
 * library without recieving continued errors.
 *
 * This resynchronize does NOT update any structs the user already has,
 * so the caller must decide wether to begin again by getting new
 * information from the api or trust any information they may currently
 * have and proceed where they left off prior to the error.
 *
 * PARAMS:
 *   ctx_t *	IN   - context object
 * RETURNS:
 *   -1		- error
 *    0		- success but reinitialization was not really required.
 *   > 0	- success and initialization was required.
 */
int init_sam_mgmt(ctx_t *ctx);


/*
 * DESCRIPTION:
 *   get SAMFS product version number
 * PARAMS:
 *   ctx_t *	IN   - context object
 * RETURNS:
 *   SAMFS product version number (string)
 */
char *get_samfs_version(ctx_t *ctx);


/*
 * DESCRIPTION:
 *   get SAMFS management library version number
 * PARAMS:
 *   ctx_t *	IN   - context object
 * RETURNS:
 *   SAMFS management library version number (string)
 */
char *get_samfs_lib_version(ctx_t *ctx);


/*
 * returns a malloced string containing the host name returned by
 * the function gethostname.
 */
int
get_host_name(ctx_t *c, char **nm);


/*
 * DESCRIPTION:
 * return file system and media capacity and usage as a comma
 * separated set of key value pairs. If this function is unable to
 * retrieve any of the information it will simply be omitted. e.g. If
 * the catalog daemon is not running there will be no information
 * available about media capacity.
 *
 * Capacity information is only reported for mounted file systems and is only
 * reported for shared file system if the function is called on the metadata
 * server.
 *
 * Library count will include the historian and pseudo libraries such
 * as network attached libraries
 *
 * PARAMS:
 *   ctx_t *	IN   -	context object
 *   char **	OUT  -	malloced string of CSV key value pairs in the following
 *			format
 *
 *	MountedFS		= <number of mounted file systems>
 *	DiskCache		= <diskcache of mounted SAM-FS/QFS
 *					file systems in kilobytes>
 *	AvailableDiskCache	= <available disk cache in kilobytes>
 *	LibCount		= <number of libraries>
 *	MediaCapacity		= <capacity of library in kilobytes>
 *	AvailableMediaCapacity	= <available capacity in kilobytes>
 *	SlotCount		= <number of configured slots>
 *
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
get_server_capacities(ctx_t *ctx, char **res);


/*
 * get_system_info
 * returns a malloced string containing the system info
 *
 * system_info is a sequence of key-value pairs as follows:
 * Hostid=<hostid>,
 * Hostname=<hostname>,
 * OSname=<sysname>,
 * Release=<release>,
 * Version=<version>,
 * Machine=<machine>,
 * Cpus=<number>,
 * Memory=<memory in kbytes>,
 * Architecture=<arc>,
 * IPaddress=<ipaddress1 ipaddress2..>
 *
 */
int
get_system_info(ctx_t *c, char **info);


/*
 * get the status of log and trace files
 *
 * return: a list of formatted strings
 * format:
 * Name=name, (e.g. sam-fsd, Device Log Libname eq, Archiver Log fsname)
 * Type=Log/Trace,
 * State=on/off,
 * Path=filename,
 * Flags=flags, (e.g. all date err default) space separated flag
 * Size=size,
 * Modtime=last modified time (num of seconds since 1970)
 */
int
get_logntrace(ctx_t *ctx, sqm_lst_t **lst);


/*
 * DESCRIPTION:
 *
 *	If packages is NULL or "\0" then this function will fetch
 *	information for each package in DEFAULT_PACKAGES.
 *
 *	This function will only return packages that are found. It
 *	will not return an error if the package is not found.
 *
 * PARAMS:
 *	ctx_t * IN - "ctx_t" context object char
 *	char * IN - "pkgs" a space separated list of packages to get info for
 *		If NULL or "\0" then this function will fetch information for
 *		the packages defined in DEFAULT_PACKAGES
 *
 *	sqm_lst_t ** OUT  - malloced list of strings containing CSV key value
 *			data.
 *
 * Example (new lines will not be present)
 *  PKGINST = SUNWsamfsu,
 *     NAME = Sun SAM and Sun SAM-QFS software Solaris 10 (usr),
 * CATEGORY = system,
 *     ARCH = sparc,
 *  VERSION = 4.6.5 REV=debug REV=5.10.2007.03.12,
 *   VENDOR = Sun Microsystems Inc.,
 *   STATUS = completely installed
 *
 * RETURNS:
 *   success -  0
 * error - -1
 */
int
get_package_info(ctx_t *ctx, char *packages, sqm_lst_t **);



#define	PI_PKGINST	0x0001
#define	PI_CATEGORY	0x0002
#define	PI_ARCH		0x0004
#define	PI_VERSION	0x0008
#define	PI_STATUS	0x0010
#define	PI_NAME		0x0020
#define	PI_VENDOR	0x0040
#define	PI_ALL		0xffff

/*
 * DESCRIPTION:
 *
 *	If packages is NULL or "\0" then this function will fetch
 *	information for each package in DEFAULT_PACKAGES.
 *
 *	This function will only return packages that are found. It
 *	will not return an error if the package is not found.
 *
 * PARAMS:
 *	ctx_t * IN - "ctx_t" context object char
 *	char * IN - "pkgs" a space separated list of packages to get info for
 *		If NULL or "\0" then this function will fetch information for
 *		the packages defined in DEFAULT_PACKAGES
 *	int32_t IN - Flags to indicate which of the keys to include. Use
 *			the PI_XXX Flags from above.
 *	sqm_lst_t ** OUT  - malloced list of strings containing CSV key value
 *			data.
 *
 * Example (new lines will not be present)
 *  PKGINST = SUNWsamfsu,
 *     NAME = Sun SAM and Sun SAM-QFS software Solaris 10 (usr),
 * CATEGORY = system,
 *     ARCH = sparc,
 *  VERSION = 4.6.5 REV=debug REV=5.10.2007.03.12,
 *   VENDOR = Sun Microsystems Inc.,
 *   STATUS = completely installed
 *
 * RETURNS:
 *   success -  0
 * error - -1
 */
int
get_package_information(ctx_t *ctx, char *pkgs, int32_t which_info,
    sqm_lst_t **res);



/*
 * return a list of csv name=value strings describing the status
 * of the configuration files.
 *
 * CONFIG = archiver.cmd e.g.
 * STATUS = OK | WARNINGS | ERRORS | MODIFIED
 */
int
get_configuration_status(ctx_t *ctx, sqm_lst_t **l);


/*
 * This method is to support the First Time Configuration Checklist.
 * It provides the information that allows the GUI to show high level
 * feedback to the users about what is currently configured.
 *
 * Key value string showing the status.
 * Keys ==> Value type
 * lib_count = int
 * lib_names = space separated list.
 * tape_count = int
 * qfs_count = int
 * disk_vols_count = int
 * volume_pools = int
 */
int
get_config_summary(ctx_t *c, char **res);


/*
 * returns a list of sam-explorer outputs found in the default
 * locations: /tmp and /var/tmp directories
 * each entry in the returned list will be a string of key value
 * pairs with the following keys
 * "path=%s,name=%s,size=%lld,created=%lu,modified=%lu"
 */
int
list_explorer_outputs(ctx_t *c, sqm_lst_t **report_paths);

/*
 * execute sam-explorer including line_cnt lines from each log
 * and trace file and save the output in the directory <location>.
 * The resulting file will be named SAMreport.HOST.VERSION.DATE_TIME
 */
int
run_sam_explorer(ctx_t *ctx, char *location, int log_lines);



/*
 * DESCRIPTION:
 *	Function to get the architecture of this server
 */
int
get_architecture(ctx_t *ctx, char **architecture);



/*
 * SUN CLUSTER related API-s
 */


/*
 * SC release as shown in /etc/cluster/release
 */
int
get_sc_version(ctx_t *ctx, char **release);

/*
 * SC name as defined by user during SC installation
 */
int
get_sc_name(ctx_t *ctx, char **name);

/*
 * DESCRIPTION:
 *    returns SC nodes info
 *
 * PARAMS:
 * sqm_lst_t *         OUT  - list of strings.
 * each string is a set of name-value pairs (case sensitive!), as below:
 *
 * sc_nodename=<nodename>,
 * sc_nodeid=<nodeid>,
 * sc_nodestatus=<nodestatus>,
 * sc_nodeprivaddr=<privateIP>
 */
int
get_sc_nodes(ctx_t *ctx, sqm_lst_t **nodes);

// SC UI should be reachable at "https://hostname:6789/"
#define	PLEXMGR_RUNNING	0
// smcwebserver must be started
#define	PLEXMGR_INSTALLED_AND_REG 1
// SC UI must register with the webserver
#define	PLEXMGR_INSTALLED_NOT_REG 2
// SC UI should be reachable at "https://hostname:3000/"
#define	PLEXMGROLD_INSTALLED 2
// SC UI packages must be installed
#define	PLEXMGR_NOTINSTALLED 3

int
get_sc_ui_state(ctx_t *ctx);



/*
 * -1 means an error.
 * 0 means Intelligent archive features not enabled
 * 1 means Intelligent archive features are enabled
 */
int intellistore_archive_enabled(ctx_t *ctx);


#endif /* _MGMT_H */
