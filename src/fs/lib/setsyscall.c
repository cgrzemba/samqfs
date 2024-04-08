/*
 *	setsyscall.c - Set syscall values in the library if needed.
 *
 *  This module provides cover functions for sam_syscall() and SamOpenLog()
 *  for sam-fsd, mount, mkfs, and fsck.  libsamconf.so is loaded using
 *  dlopen().
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

#pragma ident "$Revision: 1.33 $"

/* ANSI C headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/* Solaris headers. */
#include <dlfcn.h>
#include <syslog.h>
#include <sys/mman.h>
#ifdef sun
#include <sys/modctl.h>
extern int modctl(int op, ...);
#endif /* sun */

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/names.h"
#include "sam/sam_trace.h"
#include "sam/syscall.h"

/* Local headers. */
#include "utility.h"


#ifdef linux
#define	MAX_MOD_LOAD_SECONDS	10
#endif /* linux */

/* Private data. */
static void (*msgFunc)(int msgNum, ...) = NULL;
static int (*samSyscall)(int cmd, void *arg, int size) = NULL;

/* Private functions. */
static void ldLibsamconf(void);
static void fatalError(int msgNum, ...);


/*
 * The following functions satisfy the external references in libsamconf
 * for the programs using this module.
 */


#ifdef sun
/*
 * Perform SAM-FS system call.
 */
int
sam_syscall(
	int cmd,			/* System call number */
	void *arg,			/* Argument structure */
	int size)			/* Size of argument structure */
{
	if (samSyscall != NULL) {
		return (samSyscall(cmd, arg, size));
	}
	errno = ENOSYS;
	return (-1);
}
#endif	/* sun */


/*
 * Load the samfs module. Samfs will load the samioc module.
 */
void
LoadFS(void *msgFunc_a)
{
#ifdef sun
	int	modid;

	if (msgFunc_a != NULL) {
		msgFunc = (void(*)(int, ...))msgFunc_a;
	}

	ldLibsamconf();

	if (modctl(MODLOAD, 0, "/kernel/fs/samfs", &modid) == -1) {
		fatalError(0, "modload(samfs) failed");
	}
#endif /* sun */
#ifdef	linux
	size_t ret;
	size_t mod_size;
	long major;
	int err;
	int fd;
	boolean_t load_ok = FALSE;
	struct stat sbuf;

	/* Load SUNWqfs modules if not already loaded. */
	/* stat the file /proc/fs/samfs/major */
	/*
	 * If err not found load the modules and check again.
	 * If no err then set the /dev/samsys stuff.
	 */

	err = stat("/proc/fs/samfs/major", &sbuf);
	if (err < 0) {
		int pid;

		/*
		 *  Invoke modprobe command to load all SUNWqfs modules.
		 *  (Function calls get very messy.)
		 */
		if ((pid = fork()) == 0) {
			execl("/sbin/modprobe", "modprobe", "SUNWqfs", 0);
		}

		/* Wait up to MAX_MOD_LOAD_SECONDS for modules to load */
		if (pid > 0) {
			int tries;

			for (tries = MAX_MOD_LOAD_SECONDS; tries > 0;
			    tries--) {
				sleep(1);
				err = stat("/proc/fs/samfs/major", &sbuf);
				if (err == 0) {
					load_ok = TRUE;
					break;	/* Completed. */
				}
			}
		}
		if (load_ok == FALSE) {
			fprintf(stderr, "Failed to load SUNWqfs modules.\n");
		}
	} else {
		load_ok = TRUE;
	}

	if (load_ok == TRUE) {
		fd = open("/proc/fs/samfs/major", O_RDONLY);
		if (fd < 0) {
			fprintf(stderr,
			    "Couldn't read /proc/fs/samfs/major\n");
		} else {
			read(fd, &major, sizeof (major));
			close(fd);
			err = unlink("/dev/samsys");
			if ((err != 0) && (errno != ENOENT)) {
				fprintf(stderr,
				    "Couldn't unlink /dev/samsys\n");
			} else {
				dev_t dev = ((dev_t)major)<<8;
				err = mknod("/dev/samsys", 0666|S_IFCHR, dev);
				if (err != 0) {
					fprintf(stderr,
					    "Couldn't create /dev/samsys\n");
				}
			}
		}
	}
#endif /* linux */
}


/*			Private functions.			*/


/*
 * Process a fatal error.
 */
static void
fatalError(int msgNum, ...)
{
	static char msg_buf[2048];
	va_list	args;
	char	*msg;

	va_start(args, msgNum);
	if (msgNum != 0) {
		msg = GetCustMsg(msgNum);
	} else {
		msg = va_arg(args, char *);
	}
	vsnprintf(msg_buf, sizeof (msg_buf), msg, args);
	va_end(args);
	if (msgFunc != NULL) {
		msgFunc(0, msg_buf);
	} else {
		error(EXIT_FAILURE, errno, msg_buf);
	}
	/* NORETURN */
	exit(EXIT_FAILURE);
}


/*
 * Load the samconf library.
 */
static void
ldLibsamconf(void)
{
	static void *lib = NULL;
	char	*msg;

	/*
	 * Load libsamconf.so.
	 */
	if (lib != NULL) {
		if (dlclose(lib) != 0) {
			goto error;
		}
	}
#if defined(SPARCV9_BUILD)
	if ((lib = dlopen(SAM_SAMFS_PATH"/sparcv9/libsamconf.so",
	    RTLD_NOW)) == NULL) {
#elif defined(AMD64_BUILD)
	if ((lib = dlopen(SAM_SAMFS_PATH"/amd64/libsamconf.so",
	    RTLD_NOW)) == NULL) {
#else
	if ((lib = dlopen(SAM_SAMFS_PATH"/libsamconf.so", RTLD_NOW)) == NULL) {
#endif /* SPARCV9_BUILD */
		goto error;
	}

	/*
	 * Set the linkages.
	 */
	samSyscall = (int(*)(int, void *, int))dlsym(lib, "sam_syscall");
	if (samSyscall == NULL) {
		goto error;
	}
	return;

error:
	msg = dlerror();
	if (msg == NULL) {
		msg = "LdLibsamconf failed";
	}
	samSyscall = NULL;
	fatalError(0, msg);
}
