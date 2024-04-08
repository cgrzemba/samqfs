/*
 *	genfile.c - Generate files of random data.
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

#pragma ident "$Revision: 1.18 $"


/* Feature test switches. */
	/* None. */

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/param.h>

/* SAM-FS headers. */
#define DEC_INIT
#include <sam/lib.h>

/* Macros. */
#define	REC_SIZE 2113	/* Number of uint_t's in a record */
#define	REC_NUMOF 41	/* NUmber of records in a buffer */

/* Structures. */
struct file_hdr {
	struct stat64 st;	/* File status at creation time */
	uint_t	seed;		/* Random number seed */
	offset_t size;		/* File size */
};

/* Private data. */
static boolean_t Chk = FALSE;
static boolean_t Blk = FALSE;
static boolean_t Gen = FALSE;
static boolean_t Quiet = FALSE;
static boolean_t Rewrite = FALSE;
static boolean_t Verbose = FALSE;
static uchar_t *buffer;
static uchar_t *cp_buffer;
static char fullpath[MAXPATHLEN + 4];	/* Current full path name */
static char dir_name[sizeof (fullpath)];	/* Directory name */
static int buf_size;
static int exit_status = 0;
static offset_t file_size[2] =	/* Minimum, maximum file size */
	{ 7 * REC_SIZE, 0 };
static int name_l;		/* Length of name in fullpath rounded */
static uint_t seed = 0;

/* Private functions. */
static void AsmSize(char *token, offset_t *size);
static void ChkFile(void);
static void DoName(char *basename);
static void GenFile(void);
static offset_t GetFileSize(void);
static int MakeDirs(char *path);
static char *NormalizePath(char *path);
static void PrintInfo(char *name, struct file_hdr *f);
static void PrintRecord(int rec_no, offset_t offset, struct file_hdr *fs,
		uchar_t *bf);
static void prerror(int status, int prerrno, char *fmt, ...);

/* Public data. */
	/* None. */

/* Function macros. */
#define	numof(a) (sizeof (a)/sizeof (*(a)))

/* Signal catching functions. */
	/* None. */

int
main(int argc, char *argv[])
{
	int c;
	int errflag = 0;
	double dv;

	program_name = basename(argv[0]);
	getcwd(dir_name, sizeof (dir_name)-1);

	while ((c = getopt(argc, argv, "DRS:cd:fgs:v")) != EOF) {
		switch (c) {
		case 'D':	/* directio */
			Blk = TRUE;
			break;
		case 'R':	/* rewrite */
			Rewrite = TRUE;
			break;
		case 'S':	/* random seed */
			seed = strtol(optarg, NULL, 0);
			srand48(seed);
			break;
		case 'c':	/* check */
			Chk = TRUE;
			break;
		case 'd':	/* base directory */
			strncpy(dir_name, optarg, sizeof (dir_name)-4);
			break;
		case 'f':	/* do not stop on errors */
			Quiet = TRUE;
			break;
		case 'g':	/* generate */
			Gen = TRUE;
			break;
		case 's': {	/* size */
			char *p;

			if ((p = strchr(optarg, '-')) != NULL)  *p++ = '\0';
			AsmSize(optarg, &file_size[0]);
			if (p != NULL)  AsmSize(p, &file_size[1]);
			}
			break;
		case 'v':	/* verbose */
			Verbose = TRUE;
			break;
		case '?':
		default:
			errflag++;
			break;
		}
	}

	if ((argc - optind) < 1) {
		fprintf(stderr,
		    "Usage: %s [-D] [-R] [-S seed] [-c] [-d dir] [-f] "
		    "[-g] [-s size[-max]] [-v] file...\n",
		    program_name);
		exit(2);
	}

	if (!Gen && !Chk)  Gen = TRUE;
	buf_size = REC_NUMOF * REC_SIZE * sizeof (uint_t);
	if ((buffer = (uchar_t *)malloc(buf_size)) == NULL) {
		prerror(2, 0, "malloc(%d) failed", buf_size);
	}
	if ((cp_buffer =
	    (uchar_t *)malloc(REC_SIZE * sizeof (uint_t))) == NULL) {
		prerror(2, 0, "malloc(%d) failed", REC_SIZE * sizeof (uint_t));
	}
	dv = drand48();
	seed = (*(unsigned long long *)&dv) >> 17;

	while (optind < argc) {
		DoName(argv[optind++]);
	}
	return (exit_status);
}


/*
 *	Assemble size.
 *	Size string in token.
 */
static void
AsmSize(char *token, offset_t *size)
{
	char *p;

	p = token;
	*size = strtoll(token, &p, 0);
	if (*size < 0 || p == token)  prerror(2, 0, "Invalid size %s", token);
	if (*p == 'b') {
		p++;
	} else if (*p == 'k') {
		p++;
		*size *= 1024;
	} else if (*p == 'm') {
		p++;
		*size *= 1024 * 1024;
	} else if (*p == 'g') {
		p++;
		*size *= 1024 * 1024 * 1024;
	} else if (*p == 't') {
		p++;
		*size *= (offset_t)1024 * 1024 * 1024 * 1024;
	}
	if (*p != '\0')  prerror(2, 0, "Invalid size %s", token);
}


/*
 *	Check a file.
 */
static void
ChkFile(void)
{
	struct file_hdr *f, fs;
	offset_t size;
	int errors = 0, fd, l, n, rec_no = 1;
	int directio_flag;


	/*
	 * Open and stat the file.
	 */
	if ((fd = open(fullpath, O_RDONLY|O_LARGEFILE)) == -1) {
		prerror(1, 1, "Cannot open %s", fullpath);
		return;
	}
	directio_flag = Blk;
	if (directio(fd, directio_flag) != 0) {
		if (directio_flag == TRUE)
			perror("directio: ioctl: ");
	}
	if (fstat64(fd, &fs.st) == -1) {
		prerror(1, 1, "fstat(%s)", fullpath);
		goto out;
	}

	/*
	 * Read file header.
	 */
	l = sizeof (struct file_hdr) + name_l;
	if ((n = read(fd, buffer, l)) != l) {
		if (n < 0)  prerror(1, 1, "Cannot read %s", fullpath);
		else  prerror(1, 0, "Header too short %d < %d", n, l);
		goto out;
	}
	if ((size = GetFileSize()) == -1)  goto out;
	fs.size = size - l;
	fs.seed = 0;

	/*
	 * Check header info.
	 */
	if (memcmp(buffer, fullpath, name_l) != 0)  errors |= 0x01;
	/* LINTED pointer cast may result in improper alignment */
	f = (struct file_hdr *)(buffer + name_l);
	if (f->size != fs.size)  errors |= 0x02;
	if (f->st.st_mtime != fs.st.st_mtime)  errors |= 0x04;
	if (f->st.st_uid != fs.st.st_uid)  errors |= 0x08;
	if (f->st.st_gid != fs.st.st_gid)  errors |= 0x10;

	if (errors != 0) {
		prerror(0, 0, "Header information wrong:");
		if (errors & 0x01) printf("  (pathname)");
		if (errors & 0x02) printf("  (size)");
		if (errors & 0x04) printf("  (modification time)");
		if (errors & 0x08) printf("  (userid)");
		if (errors & 0x10) printf("  (groupid)");
		printf("\n");
		PrintInfo(fullpath, &fs);
		PrintInfo((char *)buffer, f);
		goto out;
	}
	fs.seed = f->seed;

	if (Verbose) {
		printf("\n");
		printf("genfile header size = %d bytes\n\n", l);
		printf("genfile header computed:\n");
		PrintInfo(fullpath, &fs);
		printf("\n");
		printf("genfile header from file:\n");
		PrintInfo((char *)buffer, f);
		printf("\n");
	}

	/*
	 * Read and check the file data.
	 */
	size = 0;
	l = buf_size;
	while (size < fs.size) {
		uchar_t *p, *pe;
		int rn;

		if (size + l >= fs.size)  l = fs.size - size;
		if ((n = read(fd, buffer, l)) != l) {
			if (n < 0)  prerror(1, 1, "Cannot read %s", fullpath);
			else  prerror(1, 0, "Record too short %d < %d", n, l);
			goto out;
		}
		p = buffer;
		pe = buffer + l;
		l = 0;
		for (rn = 0; rn < REC_NUMOF; rn++) {
			uint_t *d, dv;
			int n;

			/*
			 * Verify a record.
			 */
			if (memcmp(p, fullpath, name_l) != 0) {
				if (p + name_l <= pe) {
					prerror(0, 0,
					    "%s: record %d,  file name wrong",
					    fullpath,
					    rec_no);
					PrintRecord(rec_no, size, &fs, p);
				}
				goto out;
			}

		/* LINTED pointer cast may result in improper alignment */
			d = (uint_t *)(p + name_l);
			if (*d != rec_no) {
				if ((uchar_t *)(d + 1) <= pe) {
					prerror(0, 0,
					    "%s: record %d, record number "
					    "wrong: %d",
					    fullpath, rec_no, *d);
					PrintRecord(rec_no, size, &fs, p);
				}
				goto out;
			}
			d++;
			dv = fs.seed;
		/* LINTED pointer cast may result in improper alignment */
			for (n = d - (uint_t *)p; n < REC_SIZE; n++) {
				dv = 3141592621 * dv + 2818281829;
				if (*d != dv) {
					if ((uchar_t *)(d + 1) <= pe) {
						prerror(0, 0,
						    "%s: record %d, data error",
						    fullpath,
						    rec_no);
						PrintRecord(rec_no, size,
						    &fs, p);
					}
					goto out;
				}
				d++;
			}
			fs.seed = dv;
			p = (uchar_t *)d;
			rec_no++;
			l += n * sizeof (uint_t);
			if (size + l >= fs.size)  break;
		}
		size += l;
	}
out:
	close(fd);
}


/*
 *	Generate a file.
 */
static void
GenFile(void)
{
	struct file_hdr *f, fh;
	struct utimbuf ut;
	offset_t req_size, size = 0;
	boolean_t first = TRUE;
	int fd, l, oflag;
	int rec_no = 1;
	int directio_flag;

	req_size = file_size[0];
	if (file_size[1] != 0) {
		req_size += (file_size[1] - file_size[0]) * drand48();
	}

	/*
	 * Create and stat the file.
	 */
	if (!Rewrite)  oflag = O_EXCL;
	else  oflag = O_TRUNC;
retry:
	if ((fd =
	    open(fullpath, O_WRONLY|O_CREAT|oflag|O_LARGEFILE, 0666)) == -1) {
		if (first && errno == ENOENT) {
			first = FALSE;
			if (MakeDirs(fullpath) == -1)
				return;
			goto retry;
		}
		prerror(1, 1, "Cannot create %s", fullpath);
		return;
	}
	directio_flag = Blk;
	if (directio(fd, directio_flag) != 0) {
		if (directio_flag == TRUE)
			perror("directio: ioctl: ");
	}
	memcpy(buffer, fullpath, name_l);
	/* LINTED pointer cast may result in improper alignment */
	f = (struct file_hdr *)(buffer + name_l);
	if (fstat64(fd, &f->st) == -1) {
		prerror(1, 1, "fstat(%s)", fullpath);
		goto out;
	}

	ut.actime = f->st.st_atime;
	ut.modtime = f->st.st_mtime;

	/*
	 * Create and write the header info.
	 */
	f->size = req_size;
	f->seed = seed;
	l = sizeof (struct file_hdr) + name_l;
	if (write(fd, buffer, l) != l) {
		prerror(1, 1, "Cannot write %s", fullpath);
		goto out;
	}
	if (Verbose)  fh = *f;

	/*
	 * Write the file data.
	 */
	while (size < req_size) {
		uchar_t *p;
		int rn;

		p = buffer;
		l = 0;
		for (rn = 0; rn < REC_NUMOF; rn++) {
			int n;
			uint_t *d;

			/*
			 * Create a record.
			 * Data generator is simple random number generator.
			 */
			memcpy(p, fullpath, name_l);
		/* LINTED pointer cast may result in improper alignment */
			d = (uint_t *)(p + name_l);
			*d++ = rec_no++;
		/* LINTED pointer cast may result in improper alignment */
			for (n = d - (uint_t *)p; n < REC_SIZE; n++) {
				*d++ = seed = 3141592621 * seed + 2818281829;
			}
			p = (uchar_t *)d;
			l += REC_SIZE * sizeof (uint_t);
			if (size + l >= req_size)  break;
		}
		if (size + l >= req_size)  l = req_size - size;
		if (write(fd, buffer, l) != l) {
			prerror(1, 1, "Cannot write %s", fullpath);
			goto out;
		}
		size += l;
	}

	if (Verbose)  PrintInfo(fullpath, &fh);

out:
	(void) close(fd);
	if (utime(fullpath, &ut) == -1)
		prerror(1, 1, "Cannot utime %s", fullpath);
}


/*
 *	Process a name.
 */
static void
DoName(char *name)
{
	struct {
		char *ch;
		char first;
		int	range;
	} gen_chars[32];
	char *fp, *p;
	int gc = 0;
	int n;

	if (*name != '/') {
		/* Start with dir_name. */
		strcpy(fullpath, dir_name);
		fp = fullpath + strlen(fullpath);
		*fp++ = '/';
	} else  fp = fullpath;
	strcpy(fp, name);
	if (NormalizePath(fullpath) == NULL) {
		prerror(2, 0, "Invalid file name.");
		return;
	}

	/*
	 * Process "[...]" fields.
	 */
	while ((p = strchr(fp, '[')) != NULL) {
		gen_chars[gc].ch = p++;
		if (*p == ']') {
			prerror(2, 0, "Empty [].");
			return;
		}
		gen_chars[gc].first = *p++;
		if (*p == '-') {
			p++;
			gen_chars[gc].range = *p++ - gen_chars[gc].first;
			if (gen_chars[gc].range < 0) {
				prerror(2, 0, "[] range reversed.");
				return;
			}
		} else  gen_chars[gc].range = 0;
		if (*p++ != ']') {
			prerror(2, 0, "Missing ].");
			return;
		}
		*gen_chars[gc].ch = gen_chars[gc].first;
		fp = gen_chars[gc].ch+1;
		strcpy(fp, p);
		if (gen_chars[gc].range > 0) {
			gc++;
			if (gc >= numof(gen_chars)) {
				prerror(2, 0, "Too many []s");
				return;
			}
		}
	}

	/*
	 * Compute length of fullpath rounded to a multiple of sizeof(uint_t).
	 */
	name_l = strlen(fullpath) + 1;
	p = &fullpath[name_l];
	n = name_l % sizeof (uint_t);
	if (n != 0) {
		n = sizeof (uint_t) - n;
		name_l += n;
		while (n-- > 0)  *p++ = '\0';
	}

	/*
	 * Now perform the action for all files defined.
	 */
	for (;;) {
		if (Gen)  GenFile();
		if (Chk)  ChkFile();

		/*
		 * Increment each character through its range.
		 */
		for (n = gc-1; ; n--) {
			char ch;

			if (n < 0)
				return;
			ch = ++*gen_chars[n].ch;
			if ((ch <= gen_chars[n].first + gen_chars[n].range))
				break;
			*gen_chars[n].ch = gen_chars[n].first;
		}
	}
}


/*
 *	Make missing directory components.
 */
static int
MakeDirs(char *path)
{
	size_t len;

	/*
	 * Back up through the components until mkdir succeeds.
	 */
	len = strlen(path);
	for (;;) {
		(void) dirname(path);
		errno = 0;
		if (mkdir(path, 0777) == 0)
			break;
		if (errno != ENOENT)
			return (-1);
	}

	/*
	 * Come forward through the components to the original name.
	 */
	path[strlen(path)] = '/';
	while (strlen(path) < len) {
		if (mkdir(path, 0777) != 0)
			return (-1);
		path[strlen(path)] = '/';
	}
	return (0);
}


/*
 *	Normalize path.
 *	Remove ./ ../ // sequences in a path.
 *	Note: path array must be able to hold one more character.
 */
char *
NormalizePath(char *path)		/* Path to be normalized. */
{
	char *p, *ps, *q;

	ps = path;
	/* Preserve an absolute path. */
	if (*ps == '/')  ps++;

	strcat(ps, "/");
	p = q = ps;
	while (*p != '\0') {
		char *q1;

		if (*p == '.') {
			if (p[1] == '/') {
				/*
				 * Skip "./".
				 */
				p++;
			} else if (p[1] == '.' && p[2] == '/') {
				/*
				 * "../"  Back up over previous component.
				 */
				p += 2;
				if (q <= ps)
					return (NULL);
				q--;
				while (q > ps && q[-1] != '/')  q--;
			}
		}
		/*
		 * Copy a component.
		 */
		q1 = q;
		while (*p != '/')  *q++ = *p++;
		if (q1 != q) *q++ = *p;
		/*
		 * Skip successive '/'s.
		 */
		while (*p == '/')  p++;
	}
	if (q > ps && q[-1] == '/')  q--;
	*q = '\0';
	return (path);
}


/*
 *	Print file information.
 */
static void
PrintInfo(char *name, struct file_hdr *f)
{
	struct tm *tm;
	char timestr[64];

	tm = localtime(&f->st.st_mtime);
	strftime(timestr, sizeof (timestr)-1, "%Y/%m/%d %H:%M:%S", tm);
	printf("%s size:%d seed:%u mtime:%s inode:%u uid:%u gid:%u\n",
	    name, (int)f->size, f->seed, timestr,
	    f->st.st_ino, f->st.st_uid, f->st.st_gid);
}


/*
 *	Process a record error.
 */
static void
PrintRecord(
	int rec_no,
	offset_t offset,
	struct file_hdr *fs,
	uchar_t *bf)
{
	uchar_t *cp = cp_buffer;
	int n, nb, nw;
	uint_t *d, dv;

	/*
	 * Re-create the record.
	 */
	memcpy(cp, fullpath, name_l);
	/* LINTED pointer cast may result in improper alignment */
	d = (uint_t *)(cp + name_l);
	*d++ = rec_no;
	dv = fs->seed;
	/* LINTED pointer cast may result in improper alignment */
	for (n = d - (uint_t *)cp; n < REC_SIZE; n++) {
		*d++ = dv = 3141592621 * dv + 2818281829;
	}
	offset += sizeof (struct file_hdr) + name_l;
	fprintf(stderr, " Offset: %lld (0x%llx)\n", offset, offset);

	/*
	 * Print out comparison.
	 */
	nb = 16;
	for (nw = 0; nw < REC_SIZE; nw += nb) {
		if ((nw + 4) > REC_SIZE)  nb = (REC_SIZE - nw) * 4;
		if (memcmp(cp, bf, nb) == 0) {
			cp += nb;
			bf += nb;
			continue;
		}

		fprintf(stderr, "%04x:",
		    (uint_t)((offset + (nw << 2)) & 0xffff));

		/*
		 * Should be.
		 */
		for (n = 0; n < nb; n++) {
			if ((n & 3) == 0)  fprintf(stderr, " ");
			fprintf(stderr, "%02x ", *cp++);
		}
		fprintf(stderr, "\n     ");

		/*
		 * Is.
		 */
		for (n = 0; n < nb; n++) {
			if ((n & 3) == 0)  fprintf(stderr, " ");
			fprintf(stderr, "%02x ", *bf++);
		}
		fprintf(stderr, "\n     ");

		/*
		 * Exclusive or.
		 */
		cp -= nb;
		bf -= nb;
		for (n = 0; n < nb; n++) {
			uchar_t bd;

			if ((n & 3) == 0)  fprintf(stderr, " ");
			bd = *bf++ ^ *cp++;
			if (bd != 0)  fprintf(stderr, "%02x ", bd);
			else  fprintf(stderr, "   ");
		}
		fprintf(stderr, "\n");
	}
}


/*
 *	Print error message.
 */
static void
prerror(
	int status,
	int prerrno,
	char *fmt,
	...)
{
	va_list ap;

	if (Quiet && status == 0)
		return;
	fprintf(stderr, "%s: ", program_name);
	if (fmt != NULL) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	if (prerrno) {
		char *p;

		if ((p = strerror(errno)) != NULL)  fprintf(stderr, ": %s", p);
		else fprintf(stderr, ": Error number %d", errno);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	if (status)  exit(status);
	exit_status = 1;
}


/*
 * Get 64-bit file size.
 */
#include "pub/stat.h"

offset_t
GetFileSize(void)
{
	struct sam_stat samst;

	if (sam_stat(fullpath, &samst, sizeof (struct sam_stat)) == -1) {
		prerror(1, 1, "sam_stat(%s)", fullpath);
		return (-1);
	}
	return (samst.st_size);
}
