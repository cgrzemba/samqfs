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
 * or https://illumos.org/license/CDDL.
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

/*
 * readhost.c  - Read and process shared file system hosts files.
 *
 *	Description:
 *		Reads and processes the hosts file in its various guises
 *		and forms.
 *
 *	char ***SamReadHosts(char *filename);
 *		Returns a NULL-terminated list of char ** values, each
 *		of which describes one host entry from the hosts file.
 *		Each of these contains a NULL-terminated list of up to
 *		five char * values.  These point to strings, respectively
 *		containing the host name, the host IP address (or name),
 *		and the optional server priority field, stager priority
 *		field, and "server" field.
 *
 *		If there are syntax errors in the file, error messages
 *		will be issued by ReadCfgError(), and the return value
 *		will be NULL.
 *
 *	int SamStoreHosts(struct sam_host_table *buf,
 *					int size, char ***host, int gen);
 *		takes a buffer and size, and stores a struct sam_host_table
 *		version of host (from SamReadHost(), above) into the buffer.
 *		If the buffer is insufficiently large, -1 will be returned.
 *		Otherwise, the number of bytes of buffer used is returned.
 *		The resulting buf is in the appropriate format for writing
 *		to the on-disk version of the hosts file.  The gen value
 *		is included in the structure; the gen value should strictly
 *		increase in any on-disk hosts file.  (I.e., any time an
 *		on-disks hosts file is rewritten, the gen number should
 *		be incremented.)
 *
 *	char ***SamHostsCvt(struct sam_host_table *, char **errmsg,
 *				int *error);
 *
 *	void SamHostsFree(char ***host);
 *		Frees all the strings and pointers of a hosts table
 *		returned by SamReadHost() or SamHostsCvt(), above.
 *
 *      Example hosts file:
 * #        Required     Required         Optional  Optional Optional
 * #        nodename     node-addr        SERVER    Hostonoff
 * #
 *          cosmic       cosmic-1.foo.com   1         on     server
 *          spacey       spacey-1.foo.com   2         on
 *          sol          sol-1.foo.com      3         -
 *          mars         mars-1.foo.com
 *          venus        venus-1.foo.com
 *          pluto        pluto-1.foo.com
 *
 *	Note that "-", "0", and blank are acceptable alternatives in
 *	the Hostonoff field and all denote "on" (for backwards compatibility).
 */
#pragma ident "$Revision: 1.45 $"


/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

/* POSIX headers. */
#include <unistd.h>
#include <fcntl.h>

/* Solaris headers. */
#include <syslog.h>
#include <sys/vfs.h>
#include <sys/errno.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "pub/devstat.h"
#include "sam/fs/sblk.h"
#include "sam/fs/samhost.h"
#include "sam/param.h"
#include "sam/readcfg.h"

/* Local headers. */
	/* none */

/* Private functions. */
static void hostLine(void);
static int cvtToken(char *, char *);

#define		HOSTS_TAB_SIZE_INIT	100
#define		HOSTS_TAB_SIZE_INC	100

/* Command table */
static DirProc_t dirProcTable[] = {
	{ NULL, hostLine, DP_other }
};

static char hostname[TOKEN_SIZE];
static char token[TOKEN_SIZE];

static char ***HostTable;
static int HostTableLen;
static int HostLine = 0;
static int HostServer = 0;

#define	cvtName(s)	\
	cvtToken(s, \
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
		"abcdefghijklmnopqrstuvwxyz0123456789-_.:")
#define	cvtNameList(s)	\
	cvtToken(s, \
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
		"abcdefghijklmnopqrstuvwxyz0123456789-_.:,")
#define	cvtNumber(s)	cvtToken(s, "-x0123456789")

int OpenFsDevOrd(char *fs, ushort_t ord, int *devfd, int oflag);

static int
cvtToken(char *s, char *okchars)
{
	if (strspn(s, okchars) != strlen(s)) {
		/* Unexpected character in hosts file */
		ReadCfgError(12510);
		return (1);
	}
	return (0);
}


char ***
SamReadHosts(char *host_name)
{
	int		errors;
	char	msgbuf[MAX_MSGBUF_SIZE];

	HostServer = -1;
	HostLine = 0;
	HostTableLen = HOSTS_TAB_SIZE_INIT;
	HostTable = (char ***)malloc(HostTableLen * sizeof (char **));
	if (HostTable == NULL) {
		/* Couldn't allocate memory to scan hosts file */
		ReadCfgError(12512);
		return (NULL);
	}
	bzero(HostTable, HOSTS_TAB_SIZE_INIT*sizeof (char **));
	errors = ReadCfg(host_name, dirProcTable, hostname, token, NULL);
	if (errors != 0) {
		sam_syslog(LOG_INFO, "ReadCfg %s failed\n", host_name);
		/* Send sysevent to generate SNMP trap */
		sprintf(msgbuf, GetCustMsg(12525), host_name);
		PostEvent(FS_CLASS, "CfgErr", 12525, LOG_ERR, msgbuf,
		    NOTIFY_AS_FAULT | NOTIFY_AS_TRAP);
		return (NULL);
	}
	return (HostTable);
}


/*
 * Process hosts file line.
 */
static void
hostLine(void)
{
	long	i;
	char	**l;		/* line info */

	if (HostLine+1 >= HostTableLen) {
		i = HostTableLen;
		HostTableLen += HOSTS_TAB_SIZE_INC;
		HostTable = (char ***)realloc(HostTable,
		    HostTableLen*sizeof (char **));
		if (HostTable == NULL) {
			/*
			 * Couldn't allocate memory for hosts file
			 * processing.
			 */
			ReadCfgError(12513);
			return;
		}
		for (; i < HostTableLen; i++) {
			HostTable[i] = NULL;
		}
	}

	HostTable[HostLine] = (char **)malloc((HOSTS_FIELDMAX+1) *
	    sizeof (char **));
	if ((l = HostTable[HostLine]) == NULL) {
		/* no memory */
		ReadCfgError(12513);
		return;
	}
	bzero(HostTable[HostLine], (HOSTS_FIELDMAX+1) * sizeof (char **));

	/*
	 * Host name.
	 */
	if (hostname[0] == '\0') {
		return;
	}
	if (strlen(hostname) > sizeof (upath_t)) {
		/* Host name too long */
		ReadCfgError(12520);
		return;
	}
	if (cvtName(hostname)) {
		/* Unexpected character in host name */
		ReadCfgError(12524);
		return;
	}
	l[HOSTS_NAME] = strdup(hostname);

	/*
	 * Host IP addr
	 */
	for (i = 1; i < 5; i++) {
		if (ReadCfgGetToken()) {
			if (strlen(token) > sizeof (upath_t)) {
				ReadCfgError(12521);
				return;
			}
			switch (i) {
			case HOSTS_IP:	/* host-IP-addr1,host-IP-addr2,... */
				if (cvtNameList(token)) {
					/* unexpected char in host addr list */
					ReadCfgError(12522);
					continue;
				}
				break;
			case HOSTS_PRI:			/* server pri */
				if (cvtNumber(token)) {
					/* unexpected char in numeric field */
					ReadCfgError(12523);
					continue;
				}
				break;
			case HOSTS_HOSTONOFF:		/* host on/off */
				if ((strcasecmp(token, "on") != 0) &&
				    (strcasecmp(token, "off") != 0) &&
				    (strspn(token, "onfONF-0") !=
				    strlen(token))) {
					/* unexpected token in on/off field */
					ReadCfgError(12527);
					continue;
				}
				break;
			case HOSTS_SERVER:		/* [server] */
				if (strcasecmp(token, "server") != 0) {
					/* Unexp tok in opt 'server' field */
					ReadCfgError(12514);
					continue;
				} else {
					if (HostServer == -1) {
						HostServer = HostLine;
					} else {
						/* Mult servers declared */
						ReadCfgError(12515);
					}
				}
			}
			if (i != HOSTS_SERVER) {
				l[i] = strdup(token);
			} else if (i == HOSTS_SERVER &&
			    HostServer == HostLine) {
				l[i] = strdup("server");
			}
		} else {
			switch (i) {
			case HOSTS_SERVER:
				break;
			case HOSTS_HOSTONOFF:
				l[i] = strdup("on");
				break;
			case HOSTS_PRI:
				l[i] = strdup("0");
				break;
			case HOSTS_IP:
			case HOSTS_NAME:
				/* Unexpectedly short hosts file line */
				ReadCfgError(12516);
			}
		}
	}
	if (ReadCfgGetToken()) {
		/* Unexpected tokens after optional 'server' field */
		ReadCfgError(12517);
	}

	for (i = 0; i < HostLine; i++) {
		if (strcasecmp(l[HOSTS_NAME], HostTable[i][HOSTS_NAME]) == 0) {
			/* duplicate hostname */
			ReadCfgError(12518);
		}
			/*
			 * else if (strcasecmp(l[HOSTS_IP],
			 *	HostTable[i][HOSTS_IP]) == 0) {
			 *	ReadCfgError(12519);
			 * }
			 *
			 * Duplicate host IP addr
			 *
			 * -- no longer an error.  SunCluster requires that a
			 * single IP address be available for non-cluster
			 * members to connect to.
			 *
			 */
	}

	HostLine++;
}


int
SamStoreHosts(struct sam_host_table *ht, size_t size, char ***tabd, int gen)
{
	char *buf = (char *)ht;
	char *b;
	int c;			/* host count */
	int s;			/* server index */
	int i;
	int host_byte_count = 0;
	int space_available = 1;
	int hdrsize;
	char *tbuf;
	int snsize;
	int orig_size = size;

	if (ht == NULL) {
		return (-1);
	}
	if (tabd == NULL) {
		return (-1);
	}

	s = -1;
	for (c = 0; tabd[c] != NULL; c++) {
		if (tabd[c][HOSTS_SERVER] &&
		    strcasecmp(tabd[c][HOSTS_SERVER], "server") == 0) {
			s = c;
		}
	}

	ht->version	= SAM_HOSTS_VERSION4;
	ht->gen		= gen;
	ht->count	= (uint16_t)c;
	ht->server	= (uint16_t)s;
	ht->pendsrv = ht->server;
	ht->prevsrv = 0;
	ht->length = 0;

	b = &ht->ent[0];
	hdrsize = b - buf;

	size -= offsetof(struct sam_host_table, ent[0]);
	orig_size = size;
	for (i = 0; i < c; i++) {
		int n;
		char **l = tabd[i];

		if (l[HOSTS_NAME] && l[HOSTS_IP] && l[HOSTS_PRI] &&
		    l[HOSTS_HOSTONOFF]) {

			if (size > 0) {
				snsize = size;
			} else {
				snsize = orig_size;
			}
			n = snprintf(b, snsize, "%s %s %s %s\n",
			    l[HOSTS_NAME], l[HOSTS_IP], l[HOSTS_PRI],
			    l[HOSTS_HOSTONOFF]);

			if (n < 0) {
				return (-1);
			}

			if (size > 0 && n > 0) {
				b += n;
				size -= n;
			}
			if (size <= 0 && ((i + 1) < c)) {
				/*
				 * Supplied buffer is too small but
				 * continue in order to find out
				 * how much space is needed.
				 * The host data in the buffer is no good
				 * so just use the beginning of the supplied
				 * buffer for the remaining entries.
				 */
				size = -1;
				b = &ht->ent[0];
			}
			host_byte_count += n;
		} else {
			return (-1);
		}
	}
	ht->length = hdrsize + host_byte_count;
	if (size < 0) {
		/*
		 * The supplied buffer was too small,
		 * length has been set to the needed size.
		 */
		return (-1);
	}
	return (ht->length);
}


#define	SETERR(x) { if (errstr) *errstr = x; }
#define	SETERRNO(x, n) { if (errstr) *errstr = (x); if (err) *err = (n); }

/*
 * Read the raw hosts file from a disk device
 *
 * The size of the buffer containing struct sam_host_table_blk
 * may actually be bigger than sizeof(struct sam_host_table_blk)
 * if we have a large host table.
 *
 * The dev arg is the raw device name of ordinal 0.
 * This device may or may not contain the hosts table.
 * The original hosts table created at mkfs time will
 * be on ordinal 0. If an older small hosts table was
 * replaced with a large one it may not be on ordinal 0.
 */
int
SamGetRawHosts(
	char *dev,
	struct sam_host_table_blk *htp,
	int htbufsize,			/* Actual size of *htp */
	char **errstr,
	int *err)
{
	struct sam_sblk sblk;
	int devfd;
	extern int errno;
	offset_t offset64;
	int len;
	int error;

	SETERRNO(NULL, 0);
	devfd = open(dev, O_RDONLY);
	if (devfd < 0) {
		SETERRNO("Can't open raw device", errno);
		return (-1);
	}
	offset64 = SUPERBLK*SAM_DEV_BSIZE;
	if (llseek(devfd, offset64, SEEK_SET) == -1) {
		close(devfd);
		SETERRNO("Seek to superblock failed", errno);
		return (-1);
	}
	if (read(devfd, (char *)&sblk, sizeof (sblk)) != sizeof (sblk)) {
		close(devfd);
		SETERRNO("Superblock read failed", errno);
		return (-1);
	}
	if (sblk.info.sb.magic == SAM_MAGIC_V1_RE ||
	    sblk.info.sb.magic == SAM_MAGIC_V2_RE ||
	    sblk.info.sb.magic == SAM_MAGIC_V2A_RE) {
		close(devfd);
		SETERR("Foreign (byte-swapped) superblock");
		return (-1);
	}
	if (sblk.info.sb.magic != SAM_MAGIC_V1 &&
	    sblk.info.sb.magic != SAM_MAGIC_V2 &&
	    sblk.info.sb.magic != SAM_MAGIC_V2A) {
		close(devfd);
		SETERR("Unrecognized superblock magic number");
		return (-1);
	}
	if (sblk.info.sb.magic == SAM_MAGIC_V1) {
		close(devfd);
		SETERR("Obsolete (non-shared) superblock magic number");
		return (-1);
	}
	if (sblk.info.sb.ord != 0) {
		close(devfd);
		SETERR("Disk Device isn't FS root partition");
		return (-1);
	}
	if (sblk.info.sb.hosts == 0) {
		close(devfd);
		SETERR("Filesystem not shared");
		return (-1);
	}

	/*
	 * Check for a large host table and set
	 * the length to read accordingly.
	 */
	if (sblk.info.sb.opt_mask & SBLK_OPTV1_LG_HOSTS) {
		if (htbufsize < SAM_LARGE_HOSTS_TABLE_SIZE) {
			SETERRNO("Buffer size too small", EINVAL);
			return (-1);
		}
		len = SAM_LARGE_HOSTS_TABLE_SIZE;
	} else {
		len = SAM_HOSTS_TABLE_SIZE;
	}

	if (sblk.info.sb.hosts_ord != 0) {
		/*
		 * The hosts table is not on ordinal 0
		 * so close devfd and open the correct device.
		 */
		close(devfd);
		error = OpenFsDevOrd(sblk.info.sb.fs_name,
		    sblk.info.sb.hosts_ord, &devfd, O_RDONLY);

		if (error != 0) {
			SETERRNO("Open of hosts table device for read failed",
			    errno);
			return (-1);
		}
	}

	/*
	 * Read the hosts table.
	 */
	offset64 = sblk.info.sb.hosts*SAM_DEV_BSIZE;
	if (llseek(devfd, offset64, SEEK_SET) == -1) {
		close(devfd);
		SETERRNO("Seek to hosts file offset failed", errno);
		return (-1);
	}
	if (read(devfd, (char *)htp, len) != len) {
		close(devfd);
		SETERRNO("host table read failed", errno);
		return (-1);
	}
	(void) close(devfd);
	return (0);
}


/*
 * Write the raw hosts file to a disk device
 *
 * The size of the buffer containing struct sam_host_table_blk
 * may actually be bigger than sizeof(struct sam_host_table_blk)
 * if we have a large host table.
 */
int
SamPutRawHosts(
	char *dev,
	struct sam_host_table_blk *htp,
	int htbufsize,			/* Actual size of *htp */
	char **errstr,
	int *err)
{
	struct sam_sblk sblk;
	int devfd;
	extern int errno;
	offset_t offset64;
	int len;
	int error;

	SETERRNO(NULL, 0);
	devfd = open(dev, O_RDWR);
	if (devfd < 0) {
		SETERRNO("Can't open raw device", errno);
		return (-1);
	}
	offset64 = SUPERBLK*SAM_DEV_BSIZE;
	if (llseek(devfd, offset64, SEEK_SET) == -1) {
		close(devfd);
		SETERRNO("Seek to superblock failed", errno);
		return (-1);
	}
	if (read(devfd, (char *)&sblk, sizeof (sblk)) != sizeof (sblk)) {
		close(devfd);
		SETERRNO("Superblock read failed", errno);
		return (-1);
	}
	if (sblk.info.sb.magic == SAM_MAGIC_V1_RE ||
	    sblk.info.sb.magic == SAM_MAGIC_V2_RE ||
	    sblk.info.sb.magic == SAM_MAGIC_V2A_RE) {
		close(devfd);
		SETERR("Foreign (byte-swapped) superblock");
		return (-1);
	}
	if (sblk.info.sb.magic != SAM_MAGIC_V1 &&
	    sblk.info.sb.magic != SAM_MAGIC_V2 &&
	    sblk.info.sb.magic != SAM_MAGIC_V2A) {
		close(devfd);
		SETERR("Unrecognized superblock magic number");
		return (-1);
	}
	if (sblk.info.sb.ord != 0) {
		close(devfd);
		SETERR("Disk Device isn't FS root partition");
		return (-1);
	}
	if (sblk.info.sb.hosts == 0) {
		close(devfd);
		SETERR("Filesystem not shared");
		return (-1);
	}
	/*
	 * Check for a large host table and set
	 * the length to write accordingly.
	 */
	if (htp->info.ht.length > SAM_HOSTS_TABLE_SIZE) {
		/*
		 * Writing a large hosts table.
		 */
		if (!(sblk.info.sb.opt_mask & SBLK_OPTV1_LG_HOSTS)) {
			SETERRNO("New host table size too big", EMSGSIZE);
			return (-1);
		}
		if (htbufsize < SAM_LARGE_HOSTS_TABLE_SIZE) {
			SETERRNO("New host table buffer too small", EINVAL);
			return (-1);
		}
		len = SAM_LARGE_HOSTS_TABLE_SIZE;

	} else {
		/*
		 * Writing a small hosts table.
		 */
		if (htbufsize >= SAM_LARGE_HOSTS_TABLE_SIZE &&
		    (sblk.info.sb.opt_mask & SBLK_OPTV1_LG_HOSTS)) {
			len = SAM_LARGE_HOSTS_TABLE_SIZE;
		} else {
			len = SAM_HOSTS_TABLE_SIZE;
		}
	}

	if (sblk.info.sb.hosts_ord != 0) {
		/*
		 * The hosts table is not on ordinal 0
		 * so close devfd and open the correct device.
		 */
		close(devfd);
		error = OpenFsDevOrd(sblk.info.sb.fs_name,
		    sblk.info.sb.hosts_ord, &devfd, O_RDWR);
		if (error != 0) {
			SETERRNO("Open of hosts table device for write failed",
			    errno);
			return (-1);
		}
	}

	offset64 = sblk.info.sb.hosts*SAM_DEV_BSIZE;
	if (llseek(devfd, offset64, SEEK_SET) == -1) {
		close(devfd);
		SETERRNO("Seek to hosts file offset failed", errno);
		return (-1);
	}

	if (write(devfd, (char *)htp, len) != len) {
		close(devfd);
		SETERRNO("host table write failed", errno);
		return (-1);
	}
	(void) close(devfd);
	return (0);
}


/*
 * Convert a struct sam_host_table to a NULL-terminated array of
 * array of char strings.  The returned char *** should be
 * given to SamHostsFree to return the allocated space.
 */
char ***
SamHostsCvt(struct sam_host_table *ht, char **errstr, int *err)
{
	char ***tab;
	int i, j;
	char *b, *s, *s2, *d;

	SETERRNO(NULL, 0);
	if (ht == NULL) {
		SETERR("NULL input host_table");
		return (NULL);
	}
	if (ht->version != SAM_HOSTS_VERSION4) {
		SETERR("Unrecognized host table version");
		return (NULL);
	}
	if (ht->length > SAM_LARGE_HOSTS_TABLE_SIZE) {
		SETERR("Bad length field in host table");
		return (NULL);
	}
	if (ht->server != HOSTS_NOSRV && ht->server >= ht->count) {
		SETERR("Bad server count");
		return (NULL);
	}
	if (ht->pendsrv != HOSTS_NOSRV && ht->pendsrv >= ht->count) {
		SETERR("Bad pending server count");
		return (NULL);
	}

	tab = (char ***)malloc((ht->count+1) * sizeof (char **));
	if (tab == NULL) {
		SETERR("Can't allocate memory");
		return (NULL);
	}
	bzero((char *)tab, (ht->count+1) * sizeof (char **));

	for (i = 0; i < ht->count; i++) {
		tab[i] = (char **)malloc((HOSTS_FIELDMAX+1) * sizeof (char *));
		if (tab[i] == NULL) {
			SamHostsFree(tab);
			SETERR("Can't allocate memory");
			return (NULL);
		}
		bzero((char *)tab[i], (HOSTS_FIELDMAX+1) * sizeof (char *));
	}

	b = (char *)ht;
	s = &ht->ent[0];
	for (i = 0; i < ht->count; i++) {
		for (j = HOSTS_NAME; j < HOSTS_SERVER; j++) {
			s2 = s;
			while (s2 - b < ht->length && *s2 != ' ' &&
			    *s2 != '\n') {
				s2++;
			}
			tab[i][j] = d = malloc(s2 - s + 1);
			if (d == NULL) {
				SETERR("Can't allocate memory");
				SamHostsFree(tab);
				return (NULL);
			}
			while (s < s2)
				*d++ = *s++;
			*d++ = '\0';
			s++;
			if (*s2 == '\n' && j != HOSTS_HOSTONOFF) {
				SETERR("Conversion error: too few fields on "
				    "line");
				SamHostsFree(tab);
				return (NULL);
			}
		}
		if (*s2 != '\n') {
			SamHostsFree(tab);
			SETERR("Conversion error: too many fields on line");
			return (NULL);
		}
	}
	if (ht->server != HOSTS_NOSRV) {
		if ((tab[ht->server][HOSTS_SERVER] = strdup("server")) ==
		    NULL) {
			SETERR("Can't allocate memory");
			SamHostsFree(tab);
			return (NULL);
		}
	}
	return (tab);
}


void
SamHostsFree(char ***ht)
{
	int i;

	if (ht != NULL) {
		for (i = 0; ht[i] != NULL; i++) {
			char **f;
			int j;

			f = ht[i];
			for (j = 0; j < 4; j++) {
				if (f[j] != NULL) {
					free((void *)f[j]);
				}
			}
			free((void *)f);
		}
		free((void *)ht);
	}
}


/*
 * Extract the host name, address, server priority, on/off field
 * from a host table and an index.
 */
int					/* 0 if not found/error */
SamGetSharedHostInfo(
	struct sam_host_table *host,	/* Host table pointer */
	int hostno,			/* Index to look up */
	upath_t name,			/* Name to return */
	upath_t addr,			/* Address to return */
	char *serverpri,		/* Server priority field */
	char *onoff)			/* Host on/off field */
{
	int slen, count;
	char *s = &host->ent[0];

	if (hostno < 0 || hostno >= host->count) {
		return (0);
	}
	count = host->length - offsetof(struct sam_host_table, ent[0]);
	/*
	 * Skip to hostno line
	 */
	while (hostno != 0 && count--) {
		if (*s++ == '\n')
			hostno--;
	}
	/*
	 * Get name field
	 */
	for (slen = 0; slen < sizeof (upath_t) && slen < count; slen++) {
		if (s[slen] == ' ') {
			if (name != NULL) {
				name[slen++] = '\0';
			}
			break;
		}
		if (name != NULL) {
			name[slen] = s[slen];
		}
	}
	s += slen;
	count -= slen;
	/*
	 * Get addr field
	 */
	for (slen = 0; slen < sizeof (upath_t) && slen < count; slen++) {
		if (s[slen] == ' ') {
			if (addr != NULL) {
				addr[slen++] = '\0';
			}
			break;
		}
		if (addr != NULL) {
			addr[slen] = s[slen];
		}
	}
	s += slen;
	count -= slen;
	/*
	 * Get serverpri field
	 */
	for (slen = 0; slen < sizeof (upath_t) && slen < count; slen++) {
		if (s[slen] == ' ' || s[slen] == '\n') {
			if (serverpri != NULL) {
				serverpri[slen++] = '\0';
			}
			break;
		}
		if (serverpri != NULL) {
			serverpri[slen] = s[slen];
		}
	}
	s += slen;
	count -= slen;
	/*
	 * Get host on/off field
	 */
	for (slen = 0; slen < sizeof (upath_t) && slen < count; slen++) {
		if (s[slen] == ' ' || s[slen] == '\n') {
			if (onoff != NULL) {
				onoff[slen++] = '\0';
			}
			return (1);
		}
		if (onoff != NULL) {
			onoff[slen] = s[slen];
		}
	}
	return (0);
}


/*
 * Extract the host name from a host table and an index
 */
int
SamGetSharedHostName(struct sam_host_table *host, int hostno, upath_t name)
{
	return (SamGetSharedHostInfo(host, hostno, name, NULL, NULL, NULL));
}


/*
 * Extract the host name and address from a host table and an index
 */
int
GetSharedHostInfo(
	struct sam_host_table *host,
	int hostno,
	upath_t name,
	upath_t addr)
{
	return (SamGetSharedHostInfo(host, hostno, name, addr, NULL, NULL));
}


/*
 * Extract the filesystem/ord-value device name and return the
 * matching RAW device path name.
 */
char *
GetRawDevName(char *fs, ushort_t ord)
{
	struct sam_fs_part slice[252];
	char devname[sizeof (slice[0].pt_name)+1],
	    rdevname[sizeof (slice[0].pt_name)+2];
	int r, j, k, n;
	int npt;
	extern int errno;

	if (ord > 251) {
		errno = EINVAL;
		return (NULL);
	}

	/*
	 * Read enough of the partition table
	 * to get the requested device.
	 *
	 * Need a function that returns just the
	 * requested ordinal.
	 */
	npt = ord + 1;
	r = GetFsParts(fs, npt, &slice[0]);
	if (r < 0) {
		return (NULL);
	}
	if (strcmp((char *)&slice[0].pt_name, "nodev") == 0) {
		/*
		 * The requested ordinal is a nodev.
		 */
		errno = EINVAL;
		return (NULL);
	}
	bzero((char *)devname, sizeof (devname));
	strncpy(devname, (char *)&slice[ord].pt_name,
	    sizeof (slice[0].pt_name));

	/*
	 * Convert the name to a raw device name.
	 */
	n = strlen(devname);
	for (j = k = 0; k < n; j++, k++) {
		rdevname[j] = devname[k];
		if (strncmp(&devname[k], "/dsk/", 5) == 0) {
			memcpy(&rdevname[j], "/rdsk/", 6);
			k += 4;
			j += 5;
		}
	}
	rdevname[j] = '\0';
	if (j == k) {
		errno = ENODEV;
		return (NULL);
	}
	return (strdup(rdevname));
}

/*
 * ----- OpenFsDevOrd
 *
 * Given the name of a filesystem and a device ordinal,
 * return an open file descriptor for the raw device.
 */
int
OpenFsDevOrd(char *fs, ushort_t ord, int *devfd, int oflag)
{
	char *rdevname;
	int fd;
	extern int errno;

	if ((rdevname = GetRawDevName(fs, ord)) == NULL) {
		return (errno);
	}
	fd = open(rdevname, oflag);
	free(rdevname);

	if (fd < 0) {
		return (errno);
	}
	*devfd = fd;

	return (0);
}
