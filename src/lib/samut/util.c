/*
 * util.c - utility functions.
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

#pragma ident "$Revision: 1.30 $"

/* ANSI C headers. */
#include <stdio.h>

/* POSIX headers. */
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>

/* Solaris headers. */
#ifdef linux
#include <string.h>
#include <sys/procfs.h>
#include <sys/param.h>
#else
#include <procfs.h>
#endif /* linux */
#include <syslog.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

#define	ISODIGIT(Char) \
	((unsigned char) (Char) >= '0' && (unsigned char) (Char) <= '7')
#define	ISSPACE(Char) (isascii(Char) && isspace(Char))


/*
 * Return the pid of a process if it exists.
 */
pid_t
FindProc(
	char *name,
	char *arg)
{
	DIR *dirp;
	struct dirent *dirent;
	upath_t	pname;
	pid_t pid;
	pid_t caller_pid;

	pid = 0;
	caller_pid = getpid();
	if ((dirp = opendir("/proc")) == NULL) {
		return (0);
	}

	while ((dirent = readdir(dirp)) != NULL) {
		int		fd;

		if (*dirent->d_name == '.') {
			continue;
		}

#if !defined(linux)
		snprintf(pname, sizeof (pname), "/proc/%s/psinfo",
		    dirent->d_name);
		if ((fd = open(pname, O_RDONLY)) != -1) {
			struct psinfo psinfo;
			int		n;

			n = read(fd, &psinfo, sizeof (psinfo));
			(void) close(fd);
			if (n == sizeof (psinfo)) {
				size_t len = strlen(name);

				if (len >= PRFNSZ) {
					len = PRFNSZ-1;
				}
				if (strncmp(psinfo.pr_fname, name, len) == 0) {
					char	*p;

					if (caller_pid == psinfo.pr_pid) {
						/* Exclude caller */
						continue;
					}
					p = strchr(psinfo.pr_psargs, ' ');
					if (p != NULL) {
						p++;
					} else {
						p = "";
					}
					if (strcmp(p, arg) == 0) {
						pid = psinfo.pr_pid;
						break;
					}
				}
			}
		}

#else /* !defined(linux) */
		snprintf(pname, sizeof (pname), "/proc/%s/cmdline",
		    dirent->d_name);
		if ((fd = open(pname, O_RDONLY)) != -1) {
			char	buf[1024];
			int		n;

			n = read(fd, buf, sizeof (buf));
			(void) close(fd);
			if (n == 0) {
				continue;
			}
			if (strcmp(buf, name) == 0) {
				char	*p;
				int		l;

				if (caller_pid == atoi(dirent->d_name)) {
					/* Exclude caller */
					continue;
				}
				l = strlen(buf) + 1;
				if (l < n) {
					p = &buf[l];
				} else {
					p = "";
				}
				if (strcmp(p, arg) == 0) {
					pid = atoi(dirent->d_name);
					break;
				}
			}
		}
#endif /* !defined(linux) */

	}
	(void) closedir(dirp);
	return (pid);
}


/*
 * Get the name of a process from it's pid.
 */
char *
GetProcName(
	pid_t pid,
	char *buf,
	int buf_size)
{
	static uname_t our_buf;
	uname_t	fname;
	char	*p;
	int		fd;

	if (buf == NULL) {
		buf = our_buf;
		buf_size = sizeof (our_buf);
	}
	if (buf_size < 1) {
		return ("");
	}
	p = "";
	{

#if !defined(linux)
	struct psinfo psinfo;

	/*
	 * Read our process's "ps" info file.
	 */
	snprintf(fname, sizeof (fname), "/proc/%d/psinfo", (int)pid);
	if ((fd = open(fname, O_RDONLY)) != -1) {
		int		n;

		n = read(fd, &psinfo, sizeof (psinfo));
		(void) close(fd);
		if (n == sizeof (psinfo)) {
			p = psinfo.pr_fname;
		}
	}
#else /* !defined(linux) */
	char	exepath[PATH_MAX+1];
	int	n;

	/*
	 * Path name to /proc entry.
	 */
	snprintf(fname, sizeof (fname), "/proc/%d/exe", pid);

	/*
	 * readlink() does not '\0' terminate the result.
	 */
	memset(exepath, 0, sizeof (exepath));
	n = readlink(fname, exepath, sizeof (exepath));
	if (n > 0) {
		/*
		 * Find the last "/" in the pathname
		 */
		p = strrchr(exepath, '/');
		if (p == NULL) {
			/*
			 * No slashes in the path so return what
			 * we got from readlink.
			 */
			p = exepath;
		} else {
			/*
			 * Skip '/'.
			 */
			p++;
		}
	}
#endif /* !defined(linux) */
	}
	strncpy(buf, p, buf_size - 1);
	return (buf);
}


/*
 * GetParentName - return the name of the execution file of the parent process.
 * Useful to identify daemon operation.
 * e.g.  Daemon = strcmp(GetParentName(), "sam-fsd") == 0;
 * Reads parent's proc name it in a static buffer.
 */
char *
GetParentName(void)
{
	static uname_t pname;

	/*
	 * Get our parent's pid.
	 */
	return (GetProcName(getppid(), pname, sizeof (pname)));
}



/*
 * MakeDir - make a directory.
 * Assures that the requested directory exists.  If the directory does
 * not exist, make it.
 */
void
MakeDir(
	char *dname)
{
	int retries;

	/*
	 * Second attempt is after an unlink().
	 */
	for (retries = 0; retries < 2; retries++) {
		struct stat sb;

		if (stat(dname, &sb) != 0) {
			/*
			 * Directory doesn't exist.
			 */
			if (mkdir(dname, 0775) == 0) {
				return;
			}
		} else if (S_ISDIR(sb.st_mode)) {
			return;
		}
		/*
		 * The name is not a directory, remove it and retry.
		 */
		(void) unlink(dname);
	}
	sam_syslog(LOG_WARNING, "Cannot mkdir(%s): %m", dname);
}


/*
 * Convert a long value to octal.
 * Convert value to octal in a character field.
 * The field consists of leading spaces, octal digits, a single space,
 * and room for a terminating '\0'.  The '\0' is not stored.  The field
 * will not overflow to the left.
 * e.g.  A value of 077 and field width of 3 would produce: "7 ".
 */
void
ll2oct(
	u_longlong_t value,	/* Value to convert. */
	char *dest,		/* Character field. */
	int width)		/* Destination field width */
{
	width -= 2;
	dest += width;	/* Store space in last character position. */
	*dest-- = ' ';

	/* Convert digits. */
	while (width-- > 0) {
		*dest-- = (value & 7) + '0';
		value = value >> 3;
		if (value == 0) {
			break;
		}
	}

	/* Add leading spaces. */
	while (width-- > 0) {
		*dest-- = ' ';
	}
}


/*
 * Convert an unsigned long long value to a character string.
 * Convert value to octal in a character field, if it fits according
 * to the rules in ll2oct(). If the field would overflow, a hexidecimal
 * conversion is done with a leading 'x' character, "width" - 1 zero filled
 * hex digits (using 'a' - 'f'), including the last character
 * position. This conversion also will not overflow to the left.
 */
void
ll2str(
	u_longlong_t value,	/* Value to convert. */
	char *dest,		/* Character field. */
	int width)		/* Destination field width */
{
	char	*dcp;
	char	tmp;

	if ((value >> (3 * (width - 2))) == 0) {
		ll2oct(value, dest, width);
		return;
	}

	width -= 2;
	dcp = dest;
	*dest = 'x';	/* Store an 'x' in the first character position */
	dcp += width;	/* set to last character position. */

	/* Convert digits. */
	while (width-- > 0) {
		tmp = (value & 0xf);
		value >>= 4;
		if (tmp > 9) {
			tmp += (int)'a' - 10;
		} else {
			tmp += (int)'0';
		}
		*dcp-- = tmp;
	}
}

/*
 * Quick and dirty octal conversion.  Result is -1 if the field is invalid
 * (all blank, or nonoctal).
 */
u_longlong_t
llfrom_oct(int digs, char *where)
{
	u_longlong_t value;

	while (ISSPACE (*where)) {		/* skip spaces */
		where++;
		if (--digs <= 0)
			return (-1);		/* all blank field */
	}
	value = 0;
	while (digs > 0 && ISODIGIT (*where)) {
		/* Scan til nonoctal.  */

		value = (value << 3) | (*where++ - '0');
		--digs;
	}

	if (digs > 0 && *where && !ISSPACE (*where))
		return (-1);			/* ended on non-space/nul */

	return (value);
}

/*
 * Quick and dirty conversion.  Field is hexadecimal if first digit is 'x'
 * otherwise octal.  Result is -1 if the field is invalid
 * (all blank, or nonoctal).
 */

u_longlong_t
llfrom_str(int digs, char *where)
{
	u_longlong_t value;
	int digit;
	int tmp;

	if (*where != 'x') {
		return (llfrom_oct(digs, where));
	}
	where++;
	digs--;
	while (ISSPACE (*where)) {
		/* skip spaces */
		where++;
		if (--digs <= 0)
			return (-1);		/* all blank field */
	}
	value = 0;
	while (digs > 0) {
		tmp = *where;
		/* Scan til nonhex.  */
		digit = tmp - (int)'0';
		if (digit > 9) {
			if (tmp >= (int)'a' && tmp <= (int)'z')
				tmp -= (int)'a' - (int)'A';
			digit = tmp + 10 - (int)'A';
		}
		if (digit >= 0 && digit < 16) {
			value = value * 16LL + (u_longlong_t)digit;
		} else {
			break;		/* non-hex digit */
		}
		where++;
		--digs;
	}

	if (digs > 0 && *where && !ISSPACE (*where))
		return (-1);			/* ended on non-space/nul */

	return (value);
}


/*
 * ---- getHostName - Return the hostname of the local host.
 *
 * Under most conditions, gets the host's idea of its own name and
 * converts it to lower case.  In pre-config'ed setups, though, we
 * may prefer to use a statically configured name, and may need to
 * do that on a per-filesystem basis.  So we check here to see
 * if /etc/opt/SUNWsamfs/nodename.<fs> or /etc/opt/SUNWsamfs/nodename
 * exists.  We check for the existence of either of these (in
 * that order), and use the entry from there (it's presumed to
 * be a single line, terminated with a newline).  If neither
 * exists, we use gethostname() to get the system's idea of its
 * own name.
 *
 * Returns <0 specific code on error, 0 on success.
 */

int
getHostName(char *host, int len, char *fs)
{
	int i, j, fd;
	char okchars[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "0123456789-_.:";
	char path[120];

	snprintf(path, sizeof (path), "%s/nodename.%s", SAM_CONFIG_PATH, fs);
	if ((fd = open(path, O_RDONLY)) < 0) {
		snprintf(path, sizeof (path), "%s/nodename", SAM_CONFIG_PATH);
		fd = open(path, O_RDONLY);
	}
	if (fd < 0) {
		/*
		 * No /etc/opt/SUNWsamfs/nodename.<fs> or
		 * /etc/opt/SUNWsamfs/nodename
		 * file -- use the host's idea of its name.
		 */
		if (gethostname(host, len) < 0) {
			/*
			 * gethostname failed
			 */
			return (HOST_NAME_EFAIL);
		}
		for (i = 0; i < len; i++) {
			if (host[i] == '\0') {
				break;
			}
		}
		if (i == 0) {
			/*
			 * gethostname(): name too short
			 */
			return (HOST_NAME_ETOOSHORT);
		}
		if (i >= len) {
			/*
			 * gethostname(): name too long
			 */
			return (HOST_NAME_ETOOLONG);
		}
		if ((j = strspn(host, okchars)) != i) {
			/*
			 * gethostname(): bad character
			 */
			return (HOST_NAME_EBADCHAR);
		}
	} else {
		int rd;

		if ((rd = read(fd, host, len)) < 0) {
			/*
			 * Read of 'hosts file path' failed
			 */
			(void) close(fd);
			return (HOST_FILE_EREAD);
		}
		(void) close(fd);
		for (i = 0; i < rd; i++) {
			if (host[i] == '\n') {
				host[i] = '\0';
				break;
			}
		}
		if (i == 0) {
			/*
			 * Bad hostname in 'hosts file path': name too short
			 */
			return (HOST_FILE_ETOOSHORT);
		}
		if (i >= len) {
			/*
			 * Bad hostname in 'hosts file path': name too long
			 */
			return (HOST_FILE_ETOOLONG);
		}
		if ((j = strspn(host, okchars)) != i) {
			/*
			 * Hostname has bad character in 'hosts file path'
			 */
			return (HOST_FILE_EBADCHAR);
		}
	}

	return (HOST_NAME_EOK);
}


/*
 * ----- conv_to_v2inode - convert WORM inode to version 2
 * Move retention period values out of modtime and attribute
 * time fields and store in area vacated by overflow volumes.
 * This routine is necessary as the original WORM code stored
 * the retention period components in the modify and
 * attribute time fields.  In setting the modtime (retention
 * start time) the archive copies were made stale.  This
 * routine attempts to correct this by finding the most recent
 * stale copy and clearing the stale flag and setting
 * its archive copy time to slightly newer than the modify
 * time if the file is offline.
 */
void
conv_to_v2inode(sam_perm_inode_t *dp)
{
	int n = 0, latest = -1;
	sam_time_t cp_time;

	/*
	 * Find the latest archive copy.  This is necessary because
	 * some of the stale copies could be from file modification
	 * prior to the worm trigger being applied.
	 */
	cp_time = 0;
	for (n = 0; n < MAX_ARCHIVE; n++) {
		if ((dp->ar.image[n].vsn[0] != 0) &&
		    (dp->ar.image[n].creation_time >
		    cp_time)) {
			latest = n;
			cp_time = dp->ar.image[n].creation_time;
		}
	}

	/*
	 * Attempt to recover the most recent stale copy for
	 * offline lines since they cannot be staged.
	 */
	if ((latest != -1) && (dp->di.status.b.offline) &&
	    (dp->di.ar_flags[latest] & AR_stale)) {
		dp->di.ar_flags[latest] &= ~AR_stale;
		dp->di.arch_status |= (1 << latest);
		dp->ar.image[latest].creation_time =
		    dp->di.modify_time.tv_sec + 1;
	}

	/*
	 * Convert the inode only once. Copy
	 * the modify time into the retention period
	 * start time.  Copy the attribute time into
	 * the retention period duration.  Set the
	 * bit indicating the conversion has taken
	 * place.
	 */
	if ((dp->di2.p2flags & P2FLAGS_WORM_V2) == 0) {
		dp->di2.rperiod_start_time =
		    dp->di.modify_time.tv_sec;
		dp->di2.rperiod_duration =
		    dp->di.attribute_time;
		dp->di2.p2flags |= P2FLAGS_WORM_V2;
	}
}
