/*
 * samu.c - SAM-FS operator utility.
 *
 * Samu is the operator display and command utility for SAM-FS.
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

#pragma ident "$Revision: 1.68 $"



/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>
#include <sys/termios.h>

/* Solaris headers. */
#include <sys/types.h>
#include <kvm.h>
#include <libgen.h>
#include <nlist.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* SAM-FS headers. */
#define	DEC_INIT
#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/names.h"
#include "aml/preview.h"
#include "sam/lib.h"
#include "pub/version.h"
#include "aml/catlib.h"
#include "sam/custmsg.h"

/* Local headers. */
#undef ERR
#include "samu.h"
#undef DEC_INIT

/* Display functions. */
static void DisHelp(void);
void DisAinode(void);
void DisArchiver(void);
void DisCatalog(void);
void DisClients(void);
void DisCmem(void);
void DisConfig(void);
void DisDiskvols(void);
void DisDevst(void);
void DisInode(void);
void DisFs(void);
void DisKernel(void);
void DisKstat(void);
void DisLabel(void);
void DisLicense(void);
void DisMount(void);
void DisMs(void);
void DisOd(void);
void DisPreview(void);
void DisPrequeue(void);
void DisPshmem(void);
void DisQueue(void);
void DisRemove(void);
void DisRemote(void);
void DisSector(void);
void DisSense(void);
void DisServ(void);
void DisShm(void);
void DisShem(void);
void DisStaging(void);
void DisTape(void);
void DisTrace(void);
void DisUdt(void);

/* Display initialization functions. */
boolean InitArchiver(void);
boolean InitCatalog(void);
boolean InitClients(void);
boolean InitCmem(void);
boolean InitDiskvols(void);
boolean InitInode(void);
boolean InitKstat(void);
boolean InitMount(void);
boolean InitPreview(void);
boolean InitPrequeue(void);
boolean InitPshem(void);
boolean InitQueue(void);
boolean InitRemove(void);
boolean InitRemote(void);
boolean InitSector(void);
boolean InitSense(void);
boolean InitServ(void);
boolean InitShem(void);
boolean InitStaging(void);
boolean InitTrace(void);
boolean InitUdt(void);

/* Display keyboard processors. */
static boolean KeyHelp(char key);
boolean KeyArchiver(char key);
boolean KeyCatalog(char key);
boolean KeyClients(char key);
boolean KeyCmem(char key);
boolean KeyConfig(char key);
boolean KeyDevst(char key);
boolean KeyDiskvols(char key);
boolean KeyInode(char key);
boolean KeyFs(char key);
boolean KeyKstat(char key);
boolean KeyLabel(char key);
boolean KeyMount(char key);
boolean KeyMs(char key);
boolean KeyPreview(char key);
boolean KeyQueue(char key);
boolean KeyPrequeue(char key);
boolean KeyRemove(char key);
boolean KeyRemote(char key);
boolean KeyPshem(char key);
boolean KeySector(char key);
boolean KeySense(char key);
boolean KeyServ(char key);
boolean KeyShem(char key);
boolean KeyStaging(char key);
boolean KeyTrace(char key);
boolean KeyUdt(char key);

/* Private functions. */
static void Atexit(void);
static void CatchSig(int sig);
static void DisHelper(int);
static int  Init(void);
static void ReInit(void);
static void ShowBanner(void);
static void process_args(int argc, char *argv[]);
static void samcmd(int argc, char *argv[]);

/* Public data. */
char *sys_mess, *sys_crit;

/* Private data. */

static struct {
	char sel;			/* Selection letter */
	sam_level sam_lvl;		/* SAM running at this level */
	int  ntitle;			/* message number of title text */
	char *ntitle_msg;		/* default title text */
	void (*dis)(void);		/* Display screen */
	boolean (*key)(char key);	/* Key processor */
	boolean (*init)(void);		/* Initialization processor */
	int  marg;			/* message number of name of argument */
					/* for command */
	char *marg_msg;			/* default name of argument */
					/* for command */
} dis_t[] = {
	{ 'a', SAM_DISK, 7200, "Archiver status", DisArchiver, KeyArchiver,
			InitArchiver, 7124, "[filesystem]"},
	{ 'c', SAM_REMOVABLE, 7201, "Device configuration", DisConfig,
			KeyConfig, NULL },
	{ 'd', QFS_STANDALONE,  7229, "Daemon trace controls", DisTrace,
			KeyTrace, InitTrace },
	{ 'f', QFS_STANDALONE,  7228, "File systems", DisFs, KeyFs, NULL },
	{ 'g', QFS_STANDALONE,  7249, "Shared clients", DisClients, KeyClients,
			InitClients },
	{ 'h', QFS_STANDALONE,  7202, "Help information", DisHelp, KeyHelp,
			NULL },
	{ 'l', QFS_STANDALONE,  7203, "Usage information", DisLicense,
			NULL, NULL },
	{ 'm', QFS_STANDALONE,  7204, "Mass storage status", DisMs, KeyMs,
			NULL },
	{ 'n', SAM_DISK,  7205, "Staging activity", DisStaging, KeyStaging,
			InitStaging, 7125, "[media]"},
	{ 'o', SAM_REMOVABLE, 7206, "Optical disk status", DisOd, KeyDevst,
			NULL },
	{ 'p', SAM_REMOVABLE, 7207, "Removable media load requests", DisPreview,
			KeyPreview, InitPreview, 7125, "[media]"},
	{ 'r', SAM_REMOVABLE, 7208, "Removable media", DisRemove,
			KeyRemove, InitRemove, 7125, "[media]"},
	{ 's', SAM_REMOVABLE, 7209, "Device status", DisDevst, KeyDevst, NULL },
	{ 't', SAM_REMOVABLE, 7210, "Tape drive status", DisTape, KeyDevst,
			NULL },
	{ 'u', SAM_DISK, 7225, "Staging Queue", DisQueue, KeyQueue, InitQueue,
			7125, "[media]"},
	{ 'v', SAM_REMOVABLE, 7211, "Robot catalog", DisCatalog, KeyCatalog,
			InitCatalog, 7126, "[eq]"},
	{ 'w', SAM_DISK, 7227, "Preview Queue", DisPrequeue, KeyPrequeue,
			InitPrequeue, 7125, "[media]"},
	{ 'C', QFS_STANDALONE, 7213, "Memory", DisCmem, KeyCmem, InitCmem,
			7121, "address"},
	{ 'D', SAM_DISK, -1, "Disk volume dictionary", DisDiskvols,
			KeyDiskvols, InitDiskvols },
	{ 'F', SAM_REMOVABLE, 7214, "Optical disk label", DisLabel, KeyLabel,
			NULL },
	{ 'I', QFS_STANDALONE, 7215, "Inode", DisInode, KeyInode, InitInode,
			7128, "[inode]"},
	{ 'J', SAM_REMOVABLE, 7216, "Preview shared memory", DisPshmem,
			KeyPshem, InitPshem, 7127, "[address]"},
	{ 'K', QFS_STANDALONE, 7224, "Kernel statistics", DisKstat, KeyKstat,
			InitKstat },
	{ 'L', SAM_REMOVABLE, 7217, "Shared memory tables", DisShm, NULL,
			NULL },
	{ 'M', SAM_REMOVABLE, 7218, "Shared memory", DisShem, KeyShem, InitShem,
			7127, "[address]"},
	{ 'N', QFS_STANDALONE,  7219, "File system parameters", DisMount,
			KeyMount, InitMount },
	{ 'P', QFS_STANDALONE, 7387, "Active Services", DisServ, KeyServ,
			InitServ},
	{ 'R', SAM_REMOVABLE, 7220, "SAM-Remote", DisRemote, KeyRemote,
			InitRemote, 7126, "[eq]"},
	{ 'S', QFS_STANDALONE, 7221, "Sector data", DisSector, KeySector,
			InitSector, 7127, "[address]"},
	{ 'T', SAM_REMOVABLE, 7222, "SCSI sense data", DisSense, KeySense,
			InitSense, 7126, "[eq]"},
	{ 'U', SAM_REMOVABLE, 7223, "Device table", DisUdt, KeyUdt, InitUdt,
			7126, "[eq]"},
};

static jmp_buf jbuf;
static FILE *cmd_st;			/* Command file stream */
static char cmd_buf[256] = "";		/* Command buffer */
static char errmsg[MAXPATHLEN*2];	/* Error message */
static char initdis = 'h';		/* Initial display */
static int dis_n;			/* Display number */
static int errmode = 0;			/* Error processing method */
static int col, row;			/* For SIGWINCH processing */

/* Signal catching functions. */

/*
 * Catch SIGWINCH.
 */
static void
CatchWinch(
/*LINTED argument unused in function */
	int sig)	/* Signal value */
{
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) == -1) {
		Atexit();
		perror("ioctl(2, TIOCGWINSZ, &ws)");
		exit(1);
	}
	col = ws.ws_col;
	row = ws.ws_row;
}

/*
 * Catch any signal.
 */
static void
CatchSig(
	int sig)	/* Signal value */
{
	move(LINES - 1, 0);
	Atexit();
	if (sig == SIGSEGV) {
		abort();
	}
	printw(catgets(catfd, SET, 2333, "Signal received %d"), sig);
	exit(1);
}


int
main(int argc, char **argv)
{
	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (strcmp(program_name, "samcmd") == 0) {
		if (Init()) {
			fprintf(stderr, "%s\n", catgets(catfd, SET, 7000,
			    "SAM-FS is not running."));
			return (1);
		}
		ScreenMode = FALSE;
		samcmd(argc, argv);
		return (0);
	}

	process_args(argc, argv);
	if (Init()) {
		fprintf(stderr, "%s\n", catgets(catfd, SET, 7000,
		    "SAM-FS is not running."));
		return (1);
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGCLD, SIG_IGN);
	signal(SIGHUP, CatchSig);
	signal(SIGTERM, CatchSig);
	signal(SIGINT, CatchSig);
	signal(SIGQUIT, CatchSig);
	signal(SIGSEGV, CatchSig);

	ScreenMode = TRUE;
	initscr();		/* Initialize curses */
	row = LINES;
	col = COLS;
	atexit(Atexit);
	clear();
	cbreak();
	noecho();
	signal(SIGWINCH, CatchWinch);

	errno = 0;
	(void) SetDisplay(initdis);
	refresh();
	if (Refresh && Delay != 0)  halfdelay(Delay * 10);
	if (*cmd_buf != '\0')  command(cmd_buf);

	for (;;) {		/* Process requests */
		char c;

		if (row != LINES || col != COLS) {
			endwin();
			initscr();
			signal(SIGWINCH, CatchWinch);
		}
		/*
		 * Make the display.
		 */
		erase();
		ShowBanner();
		if (IsSamRunning == 0) {
			if (Init()) {
				fprintf(stderr, "%s\n", catgets(catfd, SET,
				    7000, "SAM-FS is not running."));
				return (1);
			}
		}
		if (*sys_crit != '\0') {
			Attron(A_BOLD);
			Mvprintw(1, 0, "%s", sys_crit);
			Attroff(A_BOLD);
			/* check for samfs shutdown */
			if (strcmp(sys_crit, catgets(catfd, SET, 11001,
			    "SAM-FS has shutdown, shared memory invalid")) ==
			    0) {
				ReInit();
			}
		} else {
			Mvprintw(1, 0, "%s", sys_mess);
		}

		errmode = 1;
		ln = 3;

		dis_t[dis_n].dis(); /* The actual display. */
		if (*errmsg != '\0')  mvprintw(LINES - 2, 0, errmsg);
		refresh();
		(void) setjmp(jbuf);
		if (Refresh && Delay != 0)  halfdelay(Delay * 10);

		if (cmd_st != NULL) {
			/*
			 * Read commands from file.
			 */
			if (fgets(cmd_buf, sizeof (cmd_buf)-1, cmd_st) !=
			    NULL) {
				command(cmd_buf);
				continue;
			} else  cmd_st = NULL;
		}

		/*
		 * Read character from terminal.
		 */
		c = getch();

		errno = 0;
		Argc = 0;
		Argv[0] = NULL;
		if (c == ERR)  continue;

		/*
		 * Command processing.
		 */
		errmode = 2;
		if (*errmsg != '\0') {
			*errmsg = '\0';
			mvprintw(LINES - 2, 0, errmsg);
			Clrtoeol();
			refresh();
		}
		switch (c) {
		case 'q':   /* quit */
			exit(0);
			break;

		case ':':   /* enter command */
			Mvprintw(LINES - 1, 0, catgets(catfd, SET, 713,
			    "Command:"));
			Clrtoeol();
			cbreak();
			echo();
			getstr(cmd_buf);
			noecho();
			if (row != LINES || col != COLS)  break;
			if (*cmd_buf == '!') {
				endwin();
				printf("\n");
				fflush(stdout);
				(void) pclose(popen(cmd_buf+1, "w"));
				printf(catgets(catfd, SET, 1951,
				    "Press Return to continue\n"));
				fflush(stdout);
				fgets(cmd_buf, sizeof (cmd_buf)-1, stdin);
				initscr();
			} else  command(cmd_buf);
			break;

		case ' ':   /* refresh display */
			break;

		case 0x0c:  /* ^L refresh display */
			clear();
			break;

		case 0x12:  /* ^R toggle refresh */
			move(LINES - 1, 0);
			if (Delay == 0)  Delay = 1;
			Refresh = !Refresh;
			if (Refresh) {
				halfdelay(Delay * 10);
				printw(catgets(catfd, SET, 2076,
				    "Refresh on, delay = %d."), Delay);
			} else {
				cbreak();
				Printw(catgets(catfd, SET, 2075,
				    "Refresh off."));
			}
			Clrtoeol();
			break;

		default:

			/*
			 * Keys for display.
			 */
			if (dis_t[dis_n].key != NULL) {
				if (dis_t[dis_n].key(c))  continue;
			}

			/*
			 * Try setting display from key.
			 */
			if (SetDisplay(c) != -1)  continue;
			beep();
			break;
		}
	}
}


/*
 * Clean up for exit.
 */
static void
Atexit(void)
{
	move(LINES - 1, 0);
	refresh();
	endwin();
}


/*
 * Display error message.
 * Errorw prints an error message at the bottom of the screen.
 * A message of the following format is displayed:
 */
void
Error(
	const char *fmt,    /* printf() style format */
	...)
{
	char *msg = errmsg;
	int   msgleft = sizeof (errmsg);

	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		vsprintf(msg, fmt, args);
		va_end(args);
		msgleft -= strlen(msg);
		msg = msg + strlen(msg);
		*msg = '\0';
	}

	if (errno != 0) {
		*msg++ = ':';
		*msg++ = ' ';
		msgleft -= 2;
		(void) StrFromErrno(errno, msg, msgleft);
	}

	switch (errmode) {

	case 1: /* Display error */
		Mvprintw(++ln, 0, errmsg);
		*errmsg = '\0';
		longjmp(jbuf, 1);
		break;

	case 2: /* Command error */
		Mvprintw(LINES - 2, 0, errmsg);
		Clrtoeol();
		beep();
		refresh();
		cbreak();
		longjmp(jbuf, 1);
		break;

	default:
		fprintf(stderr, "%s: ", program_name);
		if (Argc > 0) {
			fprintf(stderr, "%s ", Argv[0]);
		}
		fprintf(stderr, "%s\n", errmsg);
	}
	exit(1);
}


/*
 * Reinitialize program.
 */
static void
ReInit(void)
{
	/*
	 *  Free resources.
	 */
	do {
		CatalogTerm();
		shm_file = pre_file = NULL;
		shmdt(preview_shm.shared_memory);
		shmdt(master_shm.shared_memory);
		sys_crit = NULL;
		erase();
		ShowBanner();
		Attron(A_BOLD);
		Mvprintw(2, 10, "%s", catgets(catfd, SET, 7001,
		    "SAMFS is not running - type q to quit."));
		Attroff(A_BOLD);
		refresh();
		if (getch() == 'q') exit(0);	/* quit */
		sleep(1);
	} while (Init());
}

int
RunDisplay(char disp)
{

	/* see if it's a display */

	if (SetDisplay(disp) < 0) {
		return (1);
	}

	LINES = 500000;
	ShowBanner();
	if (*sys_crit != '\0') {
		Mvprintw(1, 0, "%s", sys_crit);
	} else {
		Mvprintw(1, 0, "%s", sys_mess);
	}
	ln = 3;
	dis_t[dis_n].dis();
	printf("\n");
	return (0);
}

/*
 * Display the banner line, name, time/date.
 */
static void
ShowBanner(void)
{
	struct tm *tm;
	time_t now;
	char  ts[64];

	now = time(NULL);
	tm = localtime(&now);
	strftime(ts, sizeof (ts)-1, "%T %b %e %Y", tm);
	Mvprintw(0, 0, catgets(catfd, SET, dis_t[dis_n].ntitle,
	    dis_t[dis_n].ntitle_msg));
	Mvprintw(0, 40, "%-4s %10s %s", program_name, SAM_VERSION, ts);
	Mvprintw(LINES-2, 50, "%s on %s", program_name, hostname);
}

/*
 * Initialize program.
 */
static int
Init(void)
{
	shm_ptr_tbl_t *shm;
	int i;

	if (shm_file != NULL) {
		int open_fd;
		struct stat  f_stat;

		/*
		 *  Using shm files from another system; assume sam running.
		 */
		IsSam = TRUE;
		IsSamRunning = TRUE;
		strcpy(hostname, "--from SAMreport--");
		if ((open_fd = open(shm_file, O_RDWR)) < 0) {
			sprintf(errmsg, "open(%s)", shm_file);
			perror(errmsg);
			exit(1);
		}

		if (fstat(open_fd, &f_stat) < 0) {
			sprintf(errmsg, "fstat(%s)", shm_file);
			perror(errmsg);
			exit(1);
		}
		if ((master_shm.shared_memory =
		    mmap((caddr_t)NULL, f_stat.st_size,
		    (PROT_READ | PROT_WRITE), MAP_SHARED,
		    open_fd, (off_t)0)) ==
		    (caddr_t)MAP_FAILED) {
			sprintf(errmsg, "mmap(%s)", shm_file);
			perror(errmsg);
			exit(1);
		}
		close(open_fd);
	} else {
		struct stat sbuf;

		IsSam = (stat(SAM_EXECUTE_PATH"/"SAM_AMLD, &sbuf) == 0);
		gethostname(hostname, 64);
		if (IsSam == FALSE) {
			IsSamRunning = FALSE;
			if (dis_t[dis_n].sam_lvl > QFS_STANDALONE) {
				(void) SetDisplay('h');
					/* Set to a known safe display */
			}
			sys_crit = "\0";
			sys_mess = "\0";
			return (0);
		}

		/*
		 * Access master shared memory segment.
		 */
		master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0);
		if (master_shm.shmid < 0) {
			IsSamRunning = FALSE;
			if (dis_t[dis_n].sam_lvl > SAM_DISK) {
				(void) SetDisplay('h');
					/* Set to a known safe display */
			}
			sys_crit = "\0";
			sys_mess = "\0";
			return (0);
		}
		master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0);
		if (master_shm.shared_memory == (void *)-1) {
			error(1, errno,
			    catgets(catfd, SET, 2556,
			    "Unable to attach master shared memory segment"),
			    0);
		}
	}

	shm = (shm_ptr_tbl_t *)master_shm.shared_memory;
	sys_mess = shm->dis_mes[DIS_MES_NORM];
	sys_crit = shm->dis_mes[DIS_MES_CRIT];

	Dev_Tbl = (dev_ptr_tbl_t *)SHM_ADDR(master_shm, shm->dev_table);
	Max_Devices = Dev_Tbl->max_devices;

	if (pre_file != NULL) {
		int open_fd;
		struct stat  f_stat;

		/*
		 *  Using shm files from another system.
		 */
		if ((open_fd = open(pre_file, O_RDWR)) < 0) {
			sprintf(errmsg, "open(%s)", pre_file);
			perror(errmsg);
			exit(1);
		}

		if (fstat(open_fd, &f_stat) < 0) {
			sprintf(errmsg, "fstat(%s)", pre_file);
			perror(errmsg);
			exit(1);
		}
		if ((preview_shm.shared_memory =
		    mmap((caddr_t)NULL, f_stat.st_size,
		    (PROT_READ | PROT_WRITE), MAP_SHARED,
		    open_fd, (off_t)0)) == (caddr_t)MAP_FAILED) {
			sprintf(errmsg, "mmap(%s)", pre_file);
			perror(errmsg);
			exit(1);
		}
		close(open_fd);
		Preview_Tbl =
		    &((shm_preview_tbl_t *)preview_shm.shared_memory)->
		    preview_table;
		Max_Preview = Preview_Tbl->avail;
	} else {

		/*
		 * Access preview display shared memory segment.
		 */
		preview_shm.shmid = shmget(SHM_PREVIEW_KEY, 0, 0);
		if (preview_shm.shmid < 0) {
			IsSamRunning = FALSE;
			if (dis_t[dis_n].sam_lvl > SAM_DISK) {
				(void) SetDisplay('h');
					/* Set to a known safe display */
			}
			sys_crit = "\0";
			sys_mess = "\0";
			return (0);
		}
		preview_shm.shared_memory =
		    shmat(preview_shm.shmid, NULL, SHM_RDONLY);
		if (preview_shm.shared_memory == (void *) -1) {
			error(0, errno,
			    catgets(catfd, SET, 2560,
			    "Unable to attach preview shared memory segment"),
			    0);
			Max_Preview = -1;
		} else {
			Preview_Tbl =
			    &((shm_preview_tbl_t *)preview_shm.shared_memory)->
			    preview_table;
			Max_Preview = Preview_Tbl->avail;
		}
	}

	if (shm->dev_table == 0) {
		IsSamRunning = FALSE;
		return (0);
	}
	/*
	 * Find first device and first robot.
	 * Use remote client if no other robots.
	 * Use historian if no other robots or remote client.
	 */
	for (i = 0; i <= Max_Devices; i++) {
		dev_ent_t *dev;

		if (Dev_Tbl->d_ent[i] == NULL)  {
			continue;
		}
		if (DisEq == 0)  {
			DisEq = (equ_t)i;
		}
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[i]);
		if ((dev->equ_type & DT_CLASS_MASK) == DT_ROBOT) {
			if (DisRb == 0)	 DisRb = (equ_t)i;
		} else if (IS_RSS(dev)) {
			if (DisRm == 0)	 DisRm = (equ_t)i;
		} else if (IS_RSC(dev)) {
			if (DisRm == 0)	 DisRm = (equ_t)i;
			if (DisRb == 0)  DisRb = (equ_t)i;
		} else if (IS_HISTORIAN(dev)) {
			if (DisRb == 0)  DisRb = (equ_t)i;
		}
	}
	IsSamRunning = TRUE;
	return (0);
}


/*
 * Set display.
 */
int			/* -1 if invalid selection */
SetDisplay(char sel)
{
	int n;

	if (sel == '?') sel = 'h';
	for (n = 0; n < numof(dis_t); n++) {
		if (sel == dis_t[n].sel) {
			if ((dis_t[n].sam_lvl == SAM_REMOVABLE) &&
			    !IsSamRunning) {
				return (-1);
			}
			if ((dis_t[n].sam_lvl == SAM_DISK) && !IsSam) {
				return (-1);
			}
			if (dis_t[n].init != NULL) {
				dis_t[n].init();
			}
			dis_n = n;
			return (n);
		}
	}
	return (-1);
}

/*
 * Process command line arguments.
 */
static void
process_args(int argc, char **argv)
{
	extern int optind;
	int c;
	int err = 0;

	shm_file = pre_file = ArchiverDir = NULL;

	while ((c = getopt(argc, argv, "c:d:f:r:M:P:A:")) != EOF) {
		switch (c) {

		case 'M':
			shm_file = strdup(optarg);
			break;

		case 'P':
			pre_file = strdup(optarg);
			break;

		case 'A':
			ArchiverDir = strdup(optarg);
			break;

		case 'c':
			strncpy(cmd_buf, optarg, sizeof (cmd_buf)-1);
			break;

		case 'd': {
			char sel;
			int n;

			sel = *optarg++;
			initdis = 0;
			if (*optarg != '0') {
				for (n = 0; n < numof(dis_t); n++) {
					if (sel == dis_t[n].sel) {
						initdis = sel;
						break;
					}
				}
			}
			if (initdis == 0)
				Error(catgets(catfd, SET, 1336,
				    "Improper display selection %s"), optarg);
		}
		break;

		case 'f':
			if (strcmp(optarg, "-") == 0)  cmd_st = stdin;
			else {
				if ((cmd_st = fopen(optarg, "r")) == NULL) {
					Error(catgets(catfd, SET, 613,
					    "Cannot open %s"), optarg);
				}
			}
		break;

		case 'r': {
			char *p;

			Delay = strtol(optarg, &p, 10);
			if (*p != '\0' || Delay < 0) {
				Error(catgets(catfd, SET, 2074,
				    "Refresh interval %s, must be >= 0"),
				    optarg);
			}
			Refresh = (Delay != 0) ? TRUE : FALSE;
			}
		break;

		case '?':
		default:
			err++;
			break;
		}
	}

	if (optind != argc)  err++;
	if (err != 0) {
		fprintf(stderr,
		    "usage: %s [-c cmd] [-d display] [-f cmd-file]"
		    " [-r interval] [-A archiver_shm_file] [-M master_shm_file]"
		    " [-P preview_shm_file]\n",
		    program_name);
		exit(2);
	}
}


/*
 * Process a samu command.
 */
static void
samcmd(int argc, char **argv)
{
	int n;

	if (argc < 2) {
		fprintf(stderr,
		    "usage: %s samu-command [args]\n",
		    program_name);
		exit(2);
	}
	Argc = argc - 1;
	for (n = 0; n < Argc && n < numof(Argv); n++) {
		Argv[n] = argv[n+1];
	}
	SamCmd();
}


static int HelpPage = 0;	/* Page to display */
static int HelpMax = 15;	/* Number of pages + 1 */

/*
 * Display help information.
 */
void
DisHelp()
{
	int p;
	if (ScreenMode) {
		Mvprintw(0, 28, catgets(catfd, SET, 1901, "page %d/%d"),
		    HelpPage + 1, HelpMax);
		DisHelper(HelpPage);
		if (HelpPage + 1 < HelpMax) {
			Mvprintw(ln + 3, 0, catgets(catfd, SET, 7003,
			    "    more (ctrl-f)"));
		}
	} else {
		for (p = 0; p < HelpMax; p++) {
			DisHelper(p);
		}
	}
}

static void
DisEnabledDisplays(sam_level level)
{
	int n;
	int nc;
	int nh;
	int no_sam[numof(dis_t)];

	nc = 0;
	/*
	 * Set up the array of valid displays for this "level".
	 */
	for (n = 0; n < numof(dis_t); n++) {
		if (dis_t[n].sam_lvl <= level) {
			no_sam[nc] = n;
			nc++;
		}
	}
	/*
	 * Display two columns of nh items.
	 */
	nh = (nc+1)/2;
	for (n = 0; n < nh; n++) {
		Mvprintw(ln, 0, "    %c\t%s", dis_t[no_sam[n]].sel,
		    catgets(catfd, SET, dis_t[no_sam[n]].ntitle,
		    dis_t[no_sam[n]].ntitle_msg));

		if ((n + nh) < nc) {
			Mvprintw(ln++, 40, "%c\t%s", dis_t[no_sam[n+nh]].sel,
			    catgets(catfd, SET, dis_t[no_sam[n+nh]].ntitle,
			    dis_t[no_sam[n+nh]].ntitle_msg));
		}
	}
}

static void
DisHelper(int page)
{
	void DisCmdHelp(int);

	switch (page) {
	case 0: {	/* Display keys: */

		Mvprintw(ln++, 0, catgets(catfd, SET, 935, "Displays:"));
		ln++;

		if (!IsSam) {
			DisEnabledDisplays(QFS_STANDALONE);
		} else {
			if (!IsSamRunning) {
				DisEnabledDisplays(SAM_DISK);
			} else {
				DisEnabledDisplays(SAM_REMOVABLE);
			}
		}

		}
		break;

	case 1:		/* Hot key commands */
		Mvprintw(ln++, 0, catgets(catfd, SET, 7230, "Hot Keys:"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7231, "    q\tQuit"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7232,
		    "    :\tEnter command"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7233,
		    "    sp\tRefresh display"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7234,
		    "    ^f\tPage display forward"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7235,
		    "    ^b\tPage display backward"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7236,
		    "    ^d\tHalf-page display forward"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7237,
		    "    ^u\tHalf-page display backward"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7241,
		    "    ^i\tShow details (selected displays)"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7238,
		    "    ^k\tAdvance display format"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7239,
		    "    ^l\tRefresh display (clear)"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7240,
		    "    ^r\tToggle refresh"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7242,
		    "    /\tSearch for VSN (v display)"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7243,
		    "    %%\tSearch for barcode (v display)"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7244,
		    "    $\tSearch for slot (v display)"));
		break;

	case 2: {	/* Command line commands */
		int n;

		Mvprintw(ln++, 0, catgets(catfd, SET, 933,
		    "Display control commands:"));
		for (n = 0; n < numof(dis_t); n++) {
			if ((dis_t[n].sam_lvl == SAM_REMOVABLE) &&
			    !IsSamRunning)
				continue;
			if ((dis_t[n].sam_lvl == SAM_DISK) && !IsSam) continue;
			if (dis_t[n].marg == NULL)  continue;
			Mvprintw(ln++, 0, "    %c %-19s %s", dis_t[n].sel,
			    catgets(catfd, SET, dis_t[n].marg,
			    dis_t[n].marg_msg),
			    catgets(catfd, SET, dis_t[n].ntitle,
			    dis_t[n].ntitle_msg));
		}
		}
		break;

	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		DisCmdHelp(page - 3);
		break;

	case 13:
		DisCmdHelp(page - 3);
		Mvprintw(ln++, 0, "    !%-30s%s",
		    catgets(catfd, SET, 7155, "shell-command"),
		    catgets(catfd, SET, 7156, "Run command in a shell"));
		break;

	case 14:	/* Media types */
		if (!IsSamRunning) break;
		Mvprintw(ln++, 0, catgets(catfd, SET, 7250, "Media types:"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7251,
		    "    all\tAll media types"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7252, "    tp\ttape"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7269,
		    "    at\tSony AIT tape"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7274,
		    "    sa\tSony Super AIT tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7253,
		    "    d2\tAmpex DST310 (D2) tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7254,
		    "    d3\tSTK SD-3 tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7255,
		    "    dt\t4mm digital tape (DAT)"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7270,
		    "    fd\tFujitsu M8100 128track tape"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7256,
		    "    i7\tIBM 3570 tape"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7257,
		    "    ib\tIBM 3590 tape"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7271,
		    "    li\tIBM 3580, Seagate Viper 200 (LTO)"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7275,
		    "    m2\tIBM 3592 tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7258,
		    "    lt\tdigital linear tape (DLT)"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7259,
		    "    se\tSTK 9490 tape"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7272,
		    "    sf\tSTK T9940 tape"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7268,
		    "    sg\tSTK 9840 tape"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7261,
		    "    st\tSTK 3480 tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7260,
		    "    so\tSony DTF tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7262,
		    "    vt\tMetrum VHS tape"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7273,
		    "    xm\tExabyte Mammoth-2 8mm tape"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7263,
		    "    xt\tExabyte 8mm tape"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7264,
		    "    od\toptical"));
		Mvprintw(ln++, 0, catgets(catfd, SET, 7265,
		    "    mo\t5 1/4 in. erasable optical disk"));
		Mvprintw(ln, 0, catgets(catfd, SET, 7266,
		    "    wo\t5 1/4 in. WORM optical disk"));
		Mvprintw(ln++, 40, catgets(catfd, SET, 7267,
		    "    o2\t12 in. WORM optical disk"));
		break;

	default:
		HelpPage = 0;
		break;
	}
}


/*
 * Help keyboard processor.
 */
static boolean
KeyHelp(char key)
{
	switch (key) {

	case KEY_full_fwd:
	case KEY_half_fwd:
		HelpPage++;
		break;

	case KEY_full_bwd:
	case KEY_half_bwd:
		HelpPage--;
		break;

	default:
		return (FALSE);
	}
	HelpPage %= HelpMax;
	return (TRUE);
}
