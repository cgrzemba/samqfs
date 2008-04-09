/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.51 $"

static char *_SrcFile = __FILE__;	/* SamMalloc needs this */

/* Feature test switches. */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/vfs.h>
#include <ctype.h>

#include "sam/sam_malloc.h"
#include "sam/custmsg.h"
#include "pub/devstat.h"
#include "sam/mount.h"
#include "sam/fs/samhost.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/shareops.h"
#include "utility.h"
#include "sam/fs/sblk.h"
#include "sblk_mgmt.h"


char *program_name;				/* used by ChkFs() */

extern int byte_swap_hb(struct sam_host_table_blk *);


static int   ResetGen = 0;
static char *DevName = NULL;
static char *FsName = NULL;
static int   PrintHost = 1;
static int   RawDisk = 0;
static int   ResetCnxns = 1;
static int   NewHostFileFlag = 0;
static char *NewHostFile = NULL;
static char *NewNameServer = NULL;
static int32_t NewServer = 0;
static int   Foreign = 0;		/* Shared Hosts file is byte-swapped */

static int HostGen = -1;
static int HostLen = 0;

static char  *GetHostDev(char *fs);
static void   PrintHosts(char *fsname, struct sam_host_table *hosttab);

static int ask(char *msg, char def);

/*
 * From src/lib/samut/error.c
 */
extern void error(int status, int errnum, char *msg, ...);


/*
 * Rewrite an FS's host file.
 *
 * Use the supplied device, if one, or get the device name
 * from the kernel using the supplied FS's name.  Unless
 * explicitly otherwise directed, the host table's generation
 * number is always set to the old value + 1, even when the
 * new host table contents are obtained from a text file.
 *
 * Read/edit/print/rewrite the host file as specified.
 */
int
main(int ac, char *av[])
{
	int i, errc, c, mod = 0;
	struct sam_host_table_blk *HostTab = NULL;
	char ***ATab = NULL;
	char *errmsg;
	extern int errno;
	int newhtsize;
	int oldhtsize;
	int estatus;
	int makinglarge = 0;
	int sbfd;
	struct sam_sblk sb;

	program_name = av[0];
	CustmsgInit(0, NULL);

#ifdef DEBUG
#define	OPT_STR	"GhqRs:uU:x"
#else
#define	OPT_STR	"hqRs:u"
#endif

	while ((c = getopt(ac, av, OPT_STR)) != EOF) {
		switch (c) {
		case 'G':	/* -G -- DEBUG only */
			ResetGen = 1;
			break;
		case 'h':	/* -h */
			error(0, 0, catgets(catfd, SET, 13700,
			    "Usage:\n\t%s [-hqRu] [-s host] fs_name"),
			    program_name);
			exit(0);
			/*NOTREACHED*/
		case 'q':	/* -q */
			PrintHost = 0;
			break;
		case 'R':	/* -R */
			RawDisk = 1;
			break;
		case 's':	/* -s host */
			NewNameServer = optarg;
			break;
		case 'u': /* -u (file is /etc/opt/SUNWsamfs/hosts.<fs>) */
			NewHostFileFlag = 1;
			break;
		case 'U':	/* -U file -- DEBUG only */
			NewHostFileFlag = 1;
			NewHostFile = optarg;
			break;
		case 'x':	/* -x -- DEBUG only */
			ResetCnxns = 0;
			break;
		default:
			exit(1);
		}
	}

	if (ac == 1) {
		error(1, 0, catgets(catfd, SET, 13700,
		    "Usage:\n\t%s [-hqRu] [-s host] fs_name"),
		    program_name);
	}
	if (optind+1 == ac) {
		FsName = av[optind];
	} else if (optind >= ac) {
		error(1, 0, catgets(catfd, SET, 13701, "Incorrect usage."));
	} else {
		error(1, 0, catgets(catfd, SET, 13706,
		    "Unknown extra arguments."));
	}

	if (NewHostFileFlag && !NewHostFile) {
		if (FsName) {
			SamMalloc(NewHostFile, MAXPATHLEN);
			sprintf(NewHostFile, "%s/hosts.%s",
			    SAM_CONFIG_PATH, FsName);
		} else {
			/*
			 * We could figure this by going to the device
			 * specified, and reading the family set name from
			 * there.  But if the dev 0 slice is specified, it
			 * seems better to get explicit direction on the
			 * hosts file desired, rather than trusting
			 * the device's superblock.
			 */
			error(1, 0, catgets(catfd, SET, 13702,
			    "-d/-u options incompatible:  "
			    "Use -U <hostsfile>."));
		}
	}

	if ((DevName = GetHostDev(FsName)) == NULL) {
		error(1, 0, catgets(catfd, SET, 13731,
		    "Client cannot access FS %s host information."),
		    FsName);
	}

	errc = sam_dev_sb_get(DevName, &sb, &sbfd);
	if (errc != 0) {
		printf("Cannot access superblock for %s errno %d\n",
		    FsName, errc);
		exit(1);
	}
	/*
	 * Always read existing host table.  The 'gen' number in the
	 * host file should always be incremented, so we need to get
	 * the old one.  This also allows sanity checking, so that
	 * most times if the host file is corrupt, or we've been
	 * pointed at something bad, we'll choke before writing anything
	 * to disk.
	 *
	 * Always allocate enough space for the large host table.
	 */
	HostTab =
	    (struct sam_host_table_blk *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
	bzero((char *)HostTab, SAM_LARGE_HOSTS_TABLE_SIZE);

	oldhtsize = SAM_LARGE_HOSTS_TABLE_SIZE;
	if (RawDisk) {
		if (SamGetRawHosts(DevName, HostTab, oldhtsize,
		    &errmsg, &errc) < 0) {
			free(HostTab);
			error(100, errc, catgets(catfd, SET, 13750,
			    "Cannot read hosts file -- %s"), errmsg);
		}
	} else {
		if (sam_gethost(FsName, oldhtsize, (char *)HostTab) < 0) {
			if (errno == EXDEV) {
				free(HostTab);
				error(100, 0, catgets(catfd, SET, 13754,
				    "Filesystem %s not mounted"), FsName);
			} else {
				free(HostTab);
				error(100, errno, catgets(catfd, SET, 13752,
				    "sam_gethost call failed on FS %s"),
				    FsName);
			}
		}
	}

	/*
	 * Validate shared hosts file
	 */
	if (HostTab->info.ht.cookie == SAM_HOSTS_COOKIE) {
		if (HostTab->info.ht.version != SAM_HOSTS_VERSION4) {
			estatus = NewHostFile ? 0 : 100;
			if (estatus) {
				free(HostTab);
			}
			error(estatus, 0,
			    catgets(catfd, SET, 13728,
			    "Unrecognized version in host file (%d)."),
			    HostTab->info.ht.version);
		}
	} else if (HostTab->info.ht.cookie == 0 || RawDisk) {
		/*
		 * Either the cookie is uninitialized (Pre-4.4) or
		 * the superblock has been read and the FS has the
		 * (local) native byte-ordering.  Allow a write.
		 */
		if (HostTab->info.ht.version != SAM_HOSTS_VERSION4) {
			estatus = NewHostFile ? 0 : 100;
			if (estatus) {
				free(HostTab);
			}
			error(estatus, 0,
			    catgets(catfd, SET, 13728,
			    "Unrecognized version in host file (%d)."),
			    HostTab->info.ht.version);
		}
		if (NewHostFile) {
			HostTab->info.ht.cookie = SAM_HOSTS_COOKIE;
		}
	} else {
		/*
		 * Apparently garbaged, possibly byte-swapped.  Don't want
		 * to allow a foreign host to rewrite, but do want to allow
		 * a native host to do so.
		 */
		if (byte_swap_hb(HostTab)) {
			estatus = NewHostFile ? 0 : 100;
			if (estatus) {
				free(HostTab);
			}
			error(estatus, 0,
			    catgets(catfd, SET, 13728,
			    "Unrecognized version in host file (%d)."),
			    HostTab->info.ht.version);
		}
		/*
		 * Byte swap successful
		 */
		if (HostTab->info.ht.version == SAM_HOSTS_VERSION4) {
			Foreign = 1;
		} else {
			estatus = NewHostFile ? 0 : 100;
			if (estatus) {
				free(HostTab);
			}
			error(estatus, 0,
			    catgets(catfd, SET, 13728,
			    "Unrecognized version in host file (%d)."),
			    HostTab->info.ht.version);
		}
	}
	HostGen = HostTab->info.ht.gen;
	HostLen = HostTab->info.ht.length;

	if ((ATab = SamHostsCvt(&HostTab->info.ht, &errmsg,
	    &errc)) == NULL) {
		estatus = NewHostFile ? 0 : 100;
		if (estatus) {
			free(HostTab);
		}
		error(estatus, 0,
		    catgets(catfd, SET, 13733,
		    "Cannot convert hosts file (%s/%d)."),
		    errmsg, errc);
	}

	if (NewHostFile) {		/* read new host info from file */
		char ***BTab;

		if ((BTab = SamReadHosts(NewHostFile)) == NULL) {
			free(HostTab);
			error(100, 0, catgets(catfd, SET, 13721,
			    "Input file processing failed."));
			/* NO RETURN */
		}
		i = 0;
		while (BTab[i] != NULL) {
			i++;
		}
		if (i > SAM_MAX_SHARED_HOSTS) {
			error(0, 0, catgets(catfd, SET, 13765,
			    "Warning: Host count (%d) > "
			    "SAM_MAX_SHARED_HOSTS (%d)"),
			    i, SAM_MAX_SHARED_HOSTS);
		}
		if (ATab) {
			int reassigned = 0;

			for (i = 0; ATab[i] && ATab[i][HOSTS_NAME] &&
			    BTab[i] && BTab[i][HOSTS_NAME]; i++) {
				if (strcasecmp(ATab[i][HOSTS_NAME],
				    BTab[i][HOSTS_NAME])) {
					error(0, 0, catgets(catfd, SET, 13761,
					    "Host '%s' ordinal (%d) "
					    "reassigned to '%s'"),
					    ATab[i][HOSTS_NAME], i+1,
					    BTab[i][HOSTS_NAME]);
					reassigned = 1;
					if (ATab[i][HOSTS_SERVER] &&
					    strcasecmp(ATab[i][HOSTS_SERVER],
					    "server") == 0) {
						error(0, 0, catgets(catfd,
						    SET, 13762,
						    "Server '%s' reassigned "
						    "in new hosts file"),
						    ATab[i][HOSTS_NAME]);
					}
				}
			}
			/*
			 * Check for left-over entries in the old ATab...
			 */
			for (; ATab[i] && ATab[i][HOSTS_NAME]; i++) {
				error(0, 0, catgets(catfd, SET, 13766,
				    "Host '%s' ordinal (%d) removed or "
				    "reassigned."),
				    ATab[i][HOSTS_NAME], i+1);
				reassigned = 1;
				if (ATab[i][HOSTS_SERVER] &&
				    strcasecmp(ATab[i][HOSTS_SERVER],
				    "server") == 0) {
					error(0, 0, catgets(catfd, SET, 13762,
					    "Server '%s' reassigned in new "
					    "hosts file"),
					    ATab[i][HOSTS_NAME]);
				}
			}
			if (i > SAM_MAX_SHARED_HOSTS) {
				error(0, 0, catgets(catfd, SET, 13765,
				    "Warning: Host count (%d) > "
				    "SAM_MAX_SHARED_HOSTS (%d)"),
				    i, SAM_MAX_SHARED_HOSTS);
			}
			if (reassigned && !RawDisk) {
				/*
				 * If host ordinals are reassigned, it's
				 * only safe to do this if the filesystem
				 * is unmounted & inactive.
				 */
				free(HostTab);
				error(100, 0, catgets(catfd, SET, 13763,
				    "Host ordinals may not be reassigned "
				    "on active FS"));
			}
			SamHostsFree(ATab);
		}
		ATab = BTab;
		mod = 1;
		if (SamStoreHosts(&HostTab->info.ht,
		    SAM_LARGE_HOSTS_TABLE_SIZE, ATab, HostGen) < 0) {
			free(HostTab);
			error(102, 0, catgets(catfd, SET, 13734,
			    "Host table too large."));
		}
	}

	if (NewNameServer) {
		int found = 0;

		for (i = 0; ATab[i] != NULL; i++) {
			if (strcasecmp(NewNameServer,
			    ATab[i][HOSTS_NAME]) == 0) {
				if (!ATab[i][HOSTS_PRI] ||
				    atoi(ATab[i][HOSTS_PRI]) == 0) {
					free(HostTab);
					error(1, 0, catgets(catfd, SET, 13707,
					    "Host %s cannot be a metadata "
					    "server for FS %s."),
					    NewNameServer, FsName);
				}
				if (RawDisk) {
					/* force server switchover */
					if (i == HostTab->info.ht.server) {
						free(HostTab);
						error(1, 0, catgets(catfd,
						    SET, 13757,
						    "FS %s: Host %s already "
						    "server"),
						    FsName, NewNameServer);
					} else {
						/* Involuntary failover */
						HostTab->info.ht.prevsrv =
						    (uint16_t)-1;
						HostTab->info.ht.server = i;
						HostTab->info.ht.pendsrv = i;
					}
				} else {
					if (HostTab->info.ht.pendsrv !=
					    HostTab->info.ht.server ||
					    HostTab->info.ht.pendsrv ==
					    (uint16_t)-1) {
						free(HostTab);
						error(1, 0, catgets(catfd,
						    SET, 13708,
						    "Host failover already "
						    "pending on FS %s."),
						    FsName);
					}
					/*
					 * request server switchover; daemon
					 * finishes.
					 */
					HostTab->info.ht.prevsrv =
					    HostTab->info.ht.server;
					HostTab->info.ht.pendsrv = i;
				}
				found = 1;
				if (ATab[i][HOSTS_SERVER] == NULL ||
				    strcasecmp(ATab[i][HOSTS_SERVER],
				    "server") != 0) {
					NewServer = 1;
				}
				break;
			}
		}

		if (!found) {
			free(HostTab);
			error(102, 0, catgets(catfd, SET, 13722,
			    "Host '%s' not found in host table."),
			    NewNameServer);
		}
		mod = 1;
	}
	if (ResetGen) {
		mod = 1;
	}

	if (mod) {
		if (Foreign) {
			free(HostTab);
			error(100, 0, catgets(catfd, SET, 13764,
			    "Cannot write hosts file -- foreign byte order."));
		}
		HostTab->info.ht.gen = ResetGen ? 0 : ++HostGen;
		if (HostTab->info.ht.gen == 0) {
			++HostTab->info.ht.gen;
		}
		if (PrintHost) {
			PrintHosts(FsName, &HostTab->info.ht);
		}

		if (HostTab->info.ht.length <= SAM_HOSTS_TABLE_SIZE &&
		    HostTab->info.ht.count <= SAM_MAX_SMALL_SHARED_HOSTS) {
			/*
			 * New host table fits in the space allowed
			 * for a small host table and is under the small
			 * host table host limit.
			 */
			newhtsize = SAM_HOSTS_TABLE_SIZE;
		} else {
			newhtsize = SAM_LARGE_HOSTS_TABLE_SIZE;
			if (!(sb.info.sb.opt_mask & SBLK_OPTV1_LG_HOSTS) &&
			    !RawDisk) {

				printf("Creating a large "
				    "hosts table for %s.\n", FsName);
				printf("This file system will not be "
				    "mountable by any version\n");
				printf("of SAM-QFS that does not "
				    "support a large hosts table.\n");

				if (isatty(0) &&
				    !ask(catgets(catfd, SET, 13428,
				    "Do you wish to continue? [y/N] "), 'n')) {
					printf("Not creating new hosts table "
					    "for '%s'.  Exiting.\n", FsName);
					exit(1);
				}
			}
		}

		if (RawDisk) {
			if (SamPutRawHosts(DevName, HostTab, newhtsize,
			    &errmsg, &errc) < 0) {
				free(HostTab);
				error(100, errc, catgets(catfd, SET, 13751,
				    "Cannot write hosts file -- %s"), errmsg);
			}
		} else {
			if (sam_sethost(FsName, NewServer, newhtsize,
			    (char *)HostTab) < 0) {
				if (errno == ENOTACTIVE) {
					free(HostTab);
					error(100, errno, catgets(catfd, SET,
					    13759,
					    "File system %s is not mounted "
					    "on new server"), FsName);
				} else {
					free(HostTab);
					error(100, errno, catgets(catfd, SET,
					    13753,
					    "sam_sethost call failed on "
					    "FS %s"), FsName);
				}
			}
		}
	} else {
		if (PrintHost) {
			PrintHosts(FsName, &HostTab->info.ht);
		}
	}

	if (mod && FsName && ResetCnxns) {
		if (sam_shareops(FsName, SHARE_OP_WAKE_SHAREDAEMON, 0) < 0) {
			free(HostTab);
			error(100, errno, catgets(catfd, SET, 13749,
			    "sam_shareops(%s, SHARE_OP_WAKE_SHAREDAEMON, 0) "
			    "failed"),
			    FsName);
		}
	}
	free(HostTab);
	return (0);
}


/*
 * ----- GetHostDev
 *
 * Given the name of a filesystem, query the FS to obtain
 * the ordinal 0 component device name (which always contains
 * the shared hosts file).  Change the cooked disk device
 * name to the raw, and return a string copy of it.
 */
static char *
GetHostDev(char *fs)
{
	struct sam_fs_part slice0;
	char devname[sizeof (slice0.pt_name)+1],
	    rdevname[sizeof (slice0.pt_name)+2];
	int r, j, k, n;
	extern int errno;

	r = GetFsParts(fs, 1, &slice0);
	if (r < 0) {
		/*
		 * ENOSYS ==> FS not installed.  Call ChkFs() to install it;
		 */
		if (errno == ENOSYS) {
			ChkFs();
			r = GetFsParts(fs, 1, &slice0);
		}
		if (r < 0) {
			error(102, errno, catgets(catfd, SET, 13720,
			    "Cannot find FS %s"), FsName);
		}
	}
	if (strcmp((char *)&slice0.pt_name[0], "nodev") == 0) {
		return (NULL);
	}
	bzero((char *)devname, sizeof (devname));
	strncpy(devname, (char *)&slice0.pt_name[0], sizeof (slice0.pt_name));
	n = strlen(devname);
	for (j = k = 0; k < n; j++, k++) {
		rdevname[j] = devname[k];
		if (strncmp(&devname[k], "/dsk/", 5) == 0) {
			strncpy(&rdevname[j], "/rdsk/", 6);
			k += 4;
			j += 5;
		}
	}
	rdevname[j] = '\0';
	if (j == k) {
		return (NULL);
	}
	return (strdup(rdevname));
}


/*
 * ----- PrintHosts
 *
 * Take the fs name and the raw shared hosts file data, and
 * print a readable representation of it.
 */
static void
PrintHosts(char *fs, struct sam_host_table *host)
{
	upath_t server;
	upath_t pendsrv;
	int i, line;
	char *c;

	if (!SamGetSharedHostName(host, host->server, server)) {
		server[0] = '\0';
	}
	if (fs[0] != '\0') {
		printf("#\n");
		printf("# Host file for family set '%s'\n", fs);
		printf("#\n");
	}
	printf("# Version: %d    Generation: %d    Count: %d\n",
	    host->version, host->gen, host->count);
	if (host->server == (uint16_t)-1) {
		printf("# Server = (NONE), length = %d\n", host->length);
	} else {
		printf("# Server = host %d/%s, length = %d\n",
		    host->server, server, host->length);
	}
	if (host->pendsrv != host->server) {	/* pending server change? */
		if (host->pendsrv == (uint16_t)-1) {
			printf("# Pending Server = (NONE)\n");
		} else {
			if (!SamGetSharedHostName(host, host->pendsrv,
			    pendsrv)) {
				pendsrv[0] = '\0';
			}
			printf("# Pending Server = host %d/%s\n",
			    host->pendsrv, pendsrv);
		}
	}
	printf("#\n");

	line = 0;
	c = &host->ent[0];
	for (i = 0;
	    i < host->length - offsetof(struct sam_host_table, ent[0]);
	    i++) {
		if (*c == '\n') {
			if (line == host->server) {
				printf(" server");
			}
			line++;
		}
		putchar(*c);
		c++;
	}
}


/*
 * ----- ask - ask the user a y/n question
 *
 * Emit the message provided, and await a y/n response.
 * Allow a default y/n value to be passed, specifying
 * whether y or n is to be returned for a non-y/n answer.
 */
static int
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
