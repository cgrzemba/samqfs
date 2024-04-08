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
#pragma ident	"$Revision: 1.11 $"
/*
 * samc - Configuration tool for SAM-FS/QFS
 *
 * Menus displayed are license-dependent.
 *
 * Note about comments in this file:
 *   when written in uppercase, MENU refers to the curses type.
 */

/* Solaris header files */
#include <stdlib.h>
#include <unistd.h>
#include <menu.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <form.h>
#include <fcntl.h>
#include <libgen.h>
#include <assert.h>
/* API header files */
#include "devstat.h"
#include "mgmt/mgmt.h"
#include "mgmt/sqm_list.h"
#include "mgmt/util.h"
#include "mgmt/types.h"
#include "mgmt/error.h"
#include "mgmt/device.h"
#include "mgmt/filesystem.h"
#include "mgmt/stage.h"
#include "mgmt/recycle.h"
#include "mgmt/sammgmt_rpc.h"
#include "mgmt/sammgmt.h"
#include "mgmt/config/cfg_fs.h"
/* #include "mgmt/license.h" */
/* other header files */
#include "sam/custmsg.h"
#include "mgmt/log.h"
#include "samc.h"
#include "forms.h"
#include "formdefs.h"
#include "aml/shm.h"
#include "pub/samrpc.h"

#define	SAMC_TITLE "SAM-FS/QFS Config Tool"
#define	MAX_FSYS 200
#define	MAX_ARSETS 200
#define	MAX_LIBS 20
#define	MAX_STDRIVES 200
#define	MAX_MEDIATYPES 100
#define	MAX_POOLS 100
#define	MAX_VSNASSOCS 100
#define	MAX_VSNEXPRLEN 100
#define	MAX_DSKVOLS 200
#define	MAX_NORECVSNSENTRIES 100
#define	EMPTY_DIS { '\0', 0, 0, ""}
#define	MIN(a, b) ((a > b) ? b : a)
#define	MAX(a, b) ((a > b) ? a : b)
/* menus */
#define	FS_MENU "File Systems"
#define	ARCHPOL_MENU "Archive Sets"
#define	ARCHRES_MENU "Archiving Resources"
#define	ACTARCH_MENU "Activate Archiver Configuration"
#define	STAGING_MENU "Staging"
#define	REL_MENU "Releasing"
#define	REC_MENU "Recycling"
#define	MEDIA_MENU "Removable Media Devices"
#define	DAEMONS_MENU "Active Daemons"
/* user messages */
#define	PREV 'p'
#define	PREV_MSG "Previous"
#define	NOLICENSE_MSG "This host does not appear to have a SAM-FS/QFS license"
#define	CANNOTINITLIB_MSG "Cannot start samc (cannot initialize library: %s)"
#define	ANYKEY_MSG "Press any key to continue"
#define	CFGUPD_MSG "Configuration updated"
#define	OPABAND_MSG "Operation abandoned"
#define	GATHERDSKINFO_MSG "Gathering disk information, please wait"
#define	GATHERMEDIAINFO_MSG "Gathering media information, please wait"
#define	DSKINFONOTFOUND_ERRMSG "Cannot collect disk information!"
#define	CHOOSEDEVS_MSG "Please choose devices from the list below"
#define	CHOOSEMETA_MSG "Please choose METADATA devices"
#define	CHOOSEDATA_MSG "Please choose DATA devices"
#define	ASKMOUNT_MSG "Do you want to mount the file system? [y/N]"
#define	ASKADDDATA_MSG "Selected metadata devices will be added. "\
	"Add data devices also?	[Y/n]"
#define	NUMOFNEWGRPS_MSG  "Number of new striped groups:"
#define	DATALOST_WRNMSG "WARNING: all existing data on the selected "\
	"slices/volumes will be lost!"
#define	ARCACTIVATED_MSG "Arhiver configuration successfully activated"
#define	CHOOSEPOOL_MSG "Choose a VSN pool"
#define	CHOOSEPOOLS_MSG "Choose VSN pools"
#define	NOARPOL_MSG "No archive sets found for the specified filesystem"
#define	NOCTX NULL

shm_alloc_t master_shm;
shm_alloc_t preview_shm;

extern char samerrmsg[];
extern sqm_lst_t *str2lst(char *str, const char *delims);
extern char *lst2str(sqm_lst_t *lst, const char *delim);
extern char *au2str(au_t au, char *au_str, boolean_t info);

typedef enum opn {
	OPN_SINGLE,		/* single step operation */
	/* multi-step operations follow */
	OPN_ADD_FS,		/* add archive set */
	OPN_ADD_AS		/* add archive set */
} opn_t;
static opn_t CRT_OPN = OPN_SINGLE;


typedef struct item_info {	/* MENU item information */
	char *str;
	void *usrptr;	/* may be NULL */
} item_info_t;

/* datatype returned by choosefrommenu() */
typedef union menu_sel {
	item_info_t singlsel;	/* for MENUs that allow a single selection */
	sqm_lst_t *multisel;	/* for MENUs that allow multiple selections */
				/* list of item_info_t */
} menu_sel_t;

static menu_sel_t
choosefrommenu(ITEM **items, const char *title, const boolean_t multisel);

/* fs menu functions */
static void addfs();
static boolean_t growfs();
static boolean_t rmfs();
static boolean_t modifmntopts();
static void mntopts();
/* archive sets menu functions */
static boolean_t choosearfs();	/* sets selectedfs */
static boolean_t addas();
static boolean_t modifas();
static boolean_t ardir();	/* view/modify archiving directives */
static boolean_t addcrit();	/* add archiving criteria */
static boolean_t modifcrit();	/* view/modify archiving criteria */
static boolean_t metaarcopies();
				/* view/modify metadata archive copies */
static boolean_t arcopies();	/* view/modify archive copies */
/* archiving resources menu functions */
static boolean_t addpool();
static boolean_t rmpool();
static boolean_t modifpool();
static boolean_t copyparams();	/* copy parameters */
static boolean_t addassoc();	/* add ar.copy - VSN association */
static boolean_t rmassoc();	/* rmv ar.copy - VSN association */
static boolean_t defdiskvol();	/* define disk volume */
static boolean_t diskar();	/* view/modify disk archiving */
/* staging functions */
static boolean_t drives();
static boolean_t bufsize();
static boolean_t stglog();
static boolean_t maxactive();
/* releasing function */
static boolean_t release();
/* recycling functions */
static boolean_t rbrecycle();	/* set robot recycle params */
static boolean_t norecycle();	/* no_recycle directive */
static boolean_t reclog();	/* recycler logfile */
static boolean_t recscript();	/* recycler notification script */
/* removable media functions */
static boolean_t addlib();	/* add media library */
static boolean_t addsdrv();	/* add standalone removable media drive */

static boolean_t activate_arc(); /* activate archiver configuration */
static boolean_t viewdaemons(); /* view a list of running SAM daemons */

int LICENSE;			/* licensed fs type; see license.h. */
				/* used in forms.c  */
static int ln;			/* cursor position */
static int col, row;		/* For SIGWINCH processing */
static sqm_lst_t *selectedfslst = NULL;
				/* names of the selected file systems */
static ar_set_criteria_t *selcrit = NULL;
				/* selected ar_set_criteria */

static char selectedfs[50]   = "none";
static char selectedas[20]   = "";	/* name of selected/new archive set */
static char selectedpool[20] = "none";	/* name of selected/new VSN pool */
static char selectedassoc[100] = "none"; /* name of selected arset-VSNassoc. */
static char selectedmedia[10] = "noned"; /* selected media type */
static char selectedrb[100]  = "none"; /* name of selected robot path */
static char selecteddrv[100] = "none"; /* name of selected drive path */

/* Display functions. */
static void dis_home(void);
static void dis_armodifmenu(void);
static void dis_armenu(void);
static void dis_fsmenu(void);
static void dis_vsnmenu(void);
static void dis_stgmenu(void);
static void dis_recmenu(void);
static void dis_stomenu(void);
static void ni(); /* ni = not implemented */

typedef struct s {
	char sel;		/* Selection letter */
	int lic_fs;		/* min. required licensed fs type: */
				/* QFS/SAMFS/SAMQFS */
	int  ntitle;		/* message number of title text */
	char *ntitle_msg;	/* default title text */
	void (*dis)(void);	/* Display screen */
	boolean_t (*init)();	/* Initialization */
	struct s **childp;	/* Child menu */
} dis_t;

dis_t toplev_dis[];

dis_t fs_dis[] = {
	{ 'a', SAMFS, 0, "Add file system", addfs },
	{ 'm', ANYFS, 0, "Modify mount options", NULL, modifmntopts },
	{ 'g', SAMFS, 0, "Grow file system", NULL, growfs },
	{ 'r', ANYFS, 0, "Remove file system", NULL, rmfs },
	EMPTY_DIS,
	{ PREV, ANYFS, 0, SAMC_TITLE, dis_home, NULL, &toplev_dis }
};

dis_t ar_dis[];
dis_t armodif_dis[] = {
	{ 'c', SAMFS, 0, "File characteristics", NULL, modifcrit },
/*	{ 'v', SAMFS, 0, "Move (change position)", ni }, */
	EMPTY_DIS,
	{ 'y', SAMFS, 0, "Copy directives", NULL, arcopies },
	EMPTY_DIS,
	{ 'a', SAMFS, 0, "Add archive copy-VSN association",  NULL, addassoc },
	{ 'r', SAMFS, 0, "Remove archive copy-VSN association", NULL, rmassoc },
	EMPTY_DIS,
	{ PREV, ANYFS, 0, ARCHPOL_MENU, dis_armenu, NULL, &ar_dis }
};

dis_t ar_dis[] = {
	{ 'a', SAMFS, 0, "Add archive set",  NULL, addas },
	{ 'm', SAMFS, 0, "Modify archive set",  dis_armodifmenu, modifas,
	    &armodif_dis },
	{ 'r', SAMFS, 0, "Remove archive set",  ni, NULL, NULL },
	{ 't', SAMFS, 0, "Metadata archiving",  NULL, metaarcopies },
	EMPTY_DIS,
	{ 'd', SAMFS, 0, "Archiving directives", NULL, ardir },
	EMPTY_DIS,
	{ 'p', ANYFS, 0, ARCHPOL_MENU, dis_armenu, choosearfs, &ar_dis }
};

dis_t vsn_dis[] = {
	{ 'a', SAMFS, 0, "Add VSN pool", NULL, addpool },
	{ 'm', SAMFS, 0, "Modify VSN pool", NULL, modifpool },
	{ 'r', SAMFS, 0, "Remove VSN pool", NULL, rmpool },
	{ 's', SAMFS, 0, "Add archive copy-VSN association", NULL, addassoc },
	{ 'd', SAMFS, 0, "Delete archive copy-VSN association", NULL, rmassoc },
	EMPTY_DIS,
	{ 'c', SAMFS, 0, "Copy parameters", NULL, copyparams },
	EMPTY_DIS,
	{ 'v', SAMFS, 0, "Define disk volumes", NULL, defdiskvol },
	{ 'k', SAMFS, 0, "Disk archiving", NULL, diskar },
	EMPTY_DIS,
	{ PREV, ANYFS, 0, "Home", dis_home, NULL, &toplev_dis }
};

dis_t stg_dis[] = {
	{ 'd', SAMFS, 0, "Drives", NULL, drives },
	{ 'b', SAMFS, 0, "Bufsize", NULL, bufsize },
	{ 'l', SAMFS, 0, "Logfile", NULL, stglog },
	{ 'm', SAMFS, 0, "Maxactive", NULL, maxactive },
	EMPTY_DIS,
	{ PREV, ANYFS, 0, "Home", dis_home, NULL, &toplev_dis }
};

dis_t rec_dis[] = {
	{ 'r', SAMFS, 0, "Robot recycle parameters", NULL, rbrecycle },
	{ 'n', SAMFS, 0, "No recycle VSN-s", NULL, norecycle },
	{ 'l', SAMFS, 0, "Logfile", NULL, reclog },
	{ 's', SAMFS, 0, "Notification script", NULL, recscript },
	EMPTY_DIS,
	{ PREV, 0, 0, "Home", dis_home, NULL, &toplev_dis  }
};

dis_t sto_dis[] = {
	{ 'a', SAMFS, 0, "Add tape library", NULL, addlib },
	EMPTY_DIS,
	{ 's', SAMFS, 0, "Add standalone tape drive", NULL, addsdrv },
	EMPTY_DIS,
	{ PREV, ANYFS, 0, "Home", dis_home, NULL, &toplev_dis }
};

dis_t toplev_dis[] = {
	{ 'f', ANYFS, 0, FS_MENU, dis_fsmenu, NULL, &fs_dis },
	EMPTY_DIS,
	{ 't', SAMFS, 0, ARCHPOL_MENU, dis_armenu, choosearfs, &ar_dis },
	{ 'v', SAMFS, 0, ARCHRES_MENU, dis_vsnmenu, NULL, &vsn_dis },
	{ 'a', SAMFS, 0, ACTARCH_MENU, NULL, activate_arc },
	EMPTY_DIS,
	{ 's', SAMFS, 0, STAGING_MENU, dis_stgmenu, NULL, &stg_dis },
	{ 'r', SAMFS, 0, REL_MENU, NULL, release },
	{ 'c', SAMFS, 0, REC_MENU, dis_recmenu, NULL, &rec_dis },
	{ 'm', SAMFS, 0, MEDIA_MENU, dis_stomenu, NULL, &sto_dis },
	EMPTY_DIS,
	{ 'd', SAMFS, 0, DAEMONS_MENU, NULL, viewdaemons }
};

static int dis_n;		/* Display number */
static dis_t (*disp)[];		/* ptr to an array of all entries */
				/* in the crt menu */
static int dis_size;		/* menu's size */
static char *title;		/* menu's name */
static dis_t default_menu = { 'h', ANYFS, 0, SAMC_TITLE, dis_home};
static dis_t crt_menu		/* the parent entry of the current menu */
	= { 'h', ANYFS, 0, SAMC_TITLE, dis_home };

static char initdis = 'h';	/* Initial display */


/*
 * Set display.
 * Returns -1 if invalid selection
 */
int
SetDisplay(char sel) {
	int n;
	if (sel == '?') sel = 'h';
	for (n = 0; n < dis_size; n++) {
		// TRACE("SetDisplay() n=%d", n);
		if ((LICENSE & (*disp)[n].lic_fs) == (*disp)[n].lic_fs)
			if (sel == (*disp)[n].sel) {
				title = (*disp)[n].ntitle_msg;
				TRACE("samc.c:SetDisplay() match with %c", sel);
				if ((*disp)[n].init != NULL) {
					if (!(*disp)[n].init()) // fails
						return (-2);
				}
				dis_n = n;
				TRACE("samc.c:dis_n=%d", n);
				return (n);
		}
	}
	return (-1);
}



void
jmp_to_entry(dis_t entry, int menusize) {
	crt_menu = entry;
	title = crt_menu.ntitle_msg;
	TRACE("sam.c:jumping to menu entry %s...", Str(title));
	disp = crt_menu.childp;
	dis_size = menusize;
	TRACE("samc.c:entry changed to %s", Str(title));
	if (disp) /* if this entry points to a menu */
		SetDisplay(crt_menu.sel);
	else
		TRACE("samc.c:jmp_to_entry(): disp is NULL");
}

void
jmp_to_menu(dis_t (*menudis)[]) {
	disp = menudis;
}


/*
 * ------------------------- helper functions --------------------------------
 */

/* return B_FALSE if canceled by user */
static boolean_t
cfg_getline(char *buf, int buflen, int (* filter)(int)) {
	int i, c;
	clrtoeol();
	cbreak();
	noecho();
	if (i = strlen(buf))
		printw(buf);
	do {
		c = getch();
		if (c == KEY_esc)
			return (B_FALSE);
		if (c == KEY_backspace)
			if (i) {
				echochar(c);	/* move one position left */
				delch();	/* delete character */
				i--;
			} else
				beep();
		if (i == buflen - 2 && c != KEY_enter) {
			beep();
			continue;
		}
		if (filter(c)) {
			echochar(c);
			buf[i] = c;
			i++;
		}

	} while (c != KEY_enter || i == 0);
	buf[i] = '\0';
	return (B_TRUE);
}

/* return NULL if operation canceled by user */
char *
getstring(char *buf, int buflen) {
	if (cfg_getline(buf, buflen, isprint))
		return (buf);
	else
		return (NULL);
}

/* return -1 if operation canceled by user */
int
getint(int *initialval) {
	char buf[10];
	if (initialval)
		sprintf(buf, "%d", *initialval);
	if (cfg_getline(buf, 10, isdigit))
		return (atoi(buf));
	else
		return (-1);
}
/* return -1 if operation canceled by user */
unsigned long long
getull(unsigned long long *initialval) {
	char buf[20];
	if (initialval)
		sprintf(buf, "%llu", *initialval);
	if (cfg_getline(buf, 20, isdigit))
		return (atoi(buf));
	else
		return (-1);
}

void
mvprintwc(int y, int x, const char *str) {
	if (COLS < x)
		return;
	if (strlen(str) + x > COLS) {
		char trunc[COLS - x];
		trunc[COLS - x - 1] = '\0';
		mvprintw(y, x, strncpy(trunc, str, COLS - x - 1));
	} else
		mvprintw(y, x, str);
}

void
anykey(int row, int col) {
	int c;
	flushinp();	/* flush input */
	cbreak();
	halfdelay(10);
	do {
		ShowBanner();
		mvprintwc(row, col, ANYKEY_MSG);
		refresh();
		c = getch();
	} while (ERR == c);
	flushinp();
}

boolean_t
askw(int row, int col, char *msg, char def) {

	int first = 1, defret = (def == 'y' ? B_TRUE : B_FALSE),
	    res;
	char s[3];
	int c = 0;	/* no input */


	cbreak();
	nodelay(stdscr, FALSE);		/* blocking read */
	echo();

	do {
		flushinp();		/* flush input */
		if (!first)
			beep();		/* neither of y/n/ENTER */
		first = 0;
		mvprintwc(row, col, msg);
		clrtoeol();

		if (ERR != (res = wgetnstr(stdscr, s, 3)))
			c = tolower(s[0]);
	} while (res == ERR || strlen(s) > 1 ||
	    (strlen(s) == 1 && c != 'y' && c != 'n'));
	TRACE("samc.c: Q:%s A:%c[ascii:%d]", Str(msg), c, c);

	/* restore curses settings */
	cbreak();
	halfdelay(10);
	noecho();

	if (c == 'n')
		return (B_FALSE);
	if (c == 'y')
		return (B_TRUE);

	return (defret);
}

int
askwold(int row, int col, char *msg, char def) {

	int defret = (def == 'y' ? TRUE : FALSE);
	char c;

	cbreak();
	echo();

	mvprintwc(row, col, msg);
	refresh();
	c = getch();

	noecho();


	halfdelay(10);
	if (tolower(c) == 'n')
		return (FALSE);
	if (tolower(c) == 'y')
		return (TRUE);
	return (defret);
}


/*
 * Clean up for exit.
 */
static void
Atexit(void) {
    move(LINES - 1, 0);
    refresh();
    endwin();
}

/*
 * Display the banner line, name, time/date.
 */
void
ShowBanner(void) {
	struct tm *tm;
	time_t now;
	char banner[80];
	char  ts[64];

	now = time(NULL);
	tm = localtime(&now);
	strftime(ts, sizeof (ts) - 1, "%c", tm);
	sprintf(banner, "%-30s %-10s%35s", title, program_name, ts);
	mvprintwc(0, 0, banner);
}

/*
 * get short date mm/dd
 */
char *
getVersion() {
	static char samcver[15];
#ifdef REVISION
	TRACE("sam.c:getting version");
	sprintf(samcver, "%s.%s", SAM_VERSION, REVISION);
#else
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	TRACE("sam.c:date:%d/%d", tm->tm_mon + 1, tm->tm_mday);
	TRACE("done");
	sprintf(samcver, "%s.built%d/%d", SAM_VERSION,
	    tm->tm_mon + 1, tm->tm_mday);
#endif
	return (samcver);
}

static void
CatchWinch(int sig) {	/* signal value. not used in function. */

	struct winsize ws;
	TRACE("samc.c:CatchWinch(%d)", sig);
	if (ioctl(0, TIOCGWINSZ, &ws) == -1) {
		Atexit();
		slog("samc.c:ioct(0, TIOCGWINSZ, &ws");
		perror("ioctl(2, TIOCGWINSZ, &ws)");
		exit(1);
	}
	col = ws.ws_col;
	row = ws.ws_row;
}

void
reinitscr() {
	endwin();
	initscr();
	signal(SIGWINCH, CatchWinch);
	cbreak();
	noecho();
	halfdelay(10);
	keypad(stdscr, TRUE);
}

void
home() {
	crt_menu = default_menu;
	title = crt_menu.ntitle_msg;
	disp = &toplev_dis;
}

void
msgwin(char **text, int textlines, char *title) {
	int lborder = 7;
	WINDOW *w = newwin(textlines + 5, COLS - lborder * 2,
	    textlines / 4, lborder);
	mvwaddstr(w, 1, (COLS - lborder * 3) / 3, title);
	waddstr(w, "\n\n");
	for (; *text; text++) {
		waddstr(w, *text);
		waddstr(w, "\n");
	}
	waddstr(w, "\n       press any key to close help window ");
	box(w, 0, 0);
	wrefresh(w);
	nodelay(w, FALSE);
	cbreak();
	wgetch(w);
	flushinp();
	delwin(w);
	halfdelay(10);
}

/* show the error messages stored in samerrmsg */
static void
showerr() {
	mvprintw(ln++, 3, "error: %s", samerrmsg);
}

/* static void */
/* chkerr(int retcode, int waitforkey) { */
/* 	if (-1 == retcode) { */
/* 		showerr(); */
/* 		if (waitforkey) */
/* 			anykey(ln++, 3); */
/* 	} */
/* } */

void
help() {
	char *hlp[] = {
	    "  List and Form traversal keys:",
	    "    ^F    -- one screen forward",
	    "    ^B    -- one screen back",
	    "    ^I    -- more information",
	    "    ENTER -- go to next step",
	    "    SPACEBAR -- select/unselect current item or",
	    "             -- switch between valid choices (in forms)",
	    "",
	    "  Form-only traversal keys:        ",
	    "    ^N   -- go to next field       ^P   -- go to previous field",
	    "    Home -- go to first field      End  -- go to last field",
	    "    LArr -- go to field to left    RArr -- go to field to right",
	    "    ESC -- exit form		^D   -- reset to default",
	    NULL
	};
	msgwin(hlp, numof(hlp), "samc help");
}

// ------------------------ menu display functions -----------------------

static void
dis_home(void) {
	int n = 0;
	dis_size = numof(toplev_dis);
	for (; n < dis_size; n++) {
		if ((LICENSE & (*disp)[n].lic_fs) == (*disp)[n].lic_fs)
			attroff(A_DIM);
		else
			attron(A_DIM);
		mvprintw(ln++, 0, "    %c\t%s", toplev_dis[n].sel,
		    toplev_dis[n].ntitle_msg);
	}
	attroff(A_DIM);
	mvprintw(++ln, 0, "    %c\t%s", 'q', "Quit");
	mvprintw(++ln, 0, "    %c\t%s", '?', "Help");
}

static void
dis_menu(char *menutitle) {
	int n = 0;
	title = menutitle;

	for (; n < dis_size; n++) {
		if ((LICENSE & (*disp)[n].lic_fs) == (*disp)[n].lic_fs)
			attroff(A_DIM);
		else
			attron(A_DIM);
		mvprintw(ln++, 0, "    %c\t%s", (*disp)[n].sel,
		    ((*disp)[n].sel == PREV) ? PREV_MSG :
		    (*disp)[n].ntitle_msg);
	}
	attroff(A_DIM);
	mvprintw(ln++, 0, "    %c\t%s", 'h', "Home");
	mvprintw(ln++, 0, "    %c\t%s", 'q', "Quit");
	mvprintw(ln++, 0, "    %c\t%s", '?', "Help");
}

static void
ni() {
	mvprintw(ln++, 7, "Not implemented yet");
	dis_size = 0;
	dis_menu("Not implemented");
}

static void
dis_fsmenu(void) {
	dis_size = numof(fs_dis);
	dis_menu(FS_MENU);
}

static void
dis_armenu() {
	mvprintw(ln, 3, "Selected filesystem: ");
	printw(selectedfs);
	ln += 2;

	dis_size = numof(ar_dis);
	dis_menu(ARCHPOL_MENU);
}

static void
dis_armodifmenu() {
	mvprintw(ln, 3, "Archive set %s (filesystem %s)",
	    selectedas, selectedfs);

	ln += 2;

	dis_size = numof(armodif_dis);
	dis_menu("Modify archive set");
}

static void
dis_vsnmenu(void) {
	dis_size = numof(vsn_dis);
	dis_menu(ARCHRES_MENU);
}

static void
dis_stgmenu(void) {
	dis_size = numof(stg_dis);
	dis_menu(STAGING_MENU);
}


static void
dis_recmenu(void) {
	dis_size = numof(rec_dis);
	dis_menu(REC_MENU);
}

static void
dis_stomenu(void) {
	dis_size = numof(sto_dis);
	dis_menu(MEDIA_MENU);
}



// -------------------------- generic MENU functions -------------------------

static void
chk(char *fname, int val) {
	TRACE("sam.c:chking %s", fname);
	if (E_OK == val)
		return;
	if (E_NO_ROOM == val)
		printw("NO ROOM TO DISPLAY MENU! (LINES=%d COLS=%d)\n",
		    LINES, COLS);
	printw("%s failed - code:%d str:%s\n", fname, val, strerror(val));
	Atexit();
	exit(1);
}

static void
dis_more(MENU * menu) {
	int rows, cols;
	menu_format(menu, &rows, &cols);
	if (top_row(menu) + rows >= item_count(menu))
		mvprintw(rows + 5, 0, "");
	else
		mvprintw(rows + 5, 0, "    more (ctrl-f)");
	clrtoeol();
	pos_menu_cursor(menu);
}

static MENU *
init_menu(ITEM **items, boolean_t multisel) {
	MENU *menu;
	WINDOW *menuwin;
	TRACE("sam.c:init_menu(%x,%d)", items, multisel);
	if (NULL == (menu = new_menu(items))) {
	    slog("sam.c:menu is NULL\n");
	    exit(1);
	}
	if (NULL == (menuwin = subwin(stdscr, 0, 0, 4, 0))) {
		slog("sam.c:menuwin is NULL\n");
		exit(1);
	}
	scrollok(stdscr, FALSE);
	chk("set_menu_win", set_menu_win(menu, stdscr));
	chk("set_menu_sub", set_menu_sub(menu, menuwin));

	chk("set_m_fmt", set_menu_format(menu, LINES - 8, 1));
/* 	menu_format(menu, &it_per_pg, &cols); */
/* 	slog("itppg=%d cols=%d\n", it_per_pg, cols); */
	set_menu_mark(menu, "x");   /* selected items */
	set_menu_grey(menu, A_DIM); /* non-selectable items */
	menu_opts_off(menu, O_SHOWDESC);
	if (multisel)
		menu_opts_off(menu, O_ONEVALUE); /* multiple selections */
	else
		menu_opts_on(menu, O_ONEVALUE); /* only one selection */
	clear();
	TRACE("samc.c:init_menu() exit");
	return (menu);
}

static void
togglesel(ITEM *item) {
	TRACE("samc.c:togglesel(%x)", item);
	if (O_SELECTABLE & item_opts(item))
		item_opts_off(item, O_SELECTABLE);
	else
		item_opts_on(item, O_SELECTABLE);
	TRACE("samc.c:togglesel done");
}

/*
 * (un)dim the other menu items (if any) that have the same userptr
 */
static void
toggleduplicates(MENU *menu) {
	ITEM *crtitem = current_item(menu),
	    **items = menu_items(menu),
	    *item = *items; // start with the first one
	if (!item_userptr(crtitem))
		return;
	while (item) {
		if (item != crtitem &&
		    item_userptr(item) == item_userptr(crtitem))
			togglesel(item);
		item = *++items;
	}
}


static menu_sel_t
choosefrommenu(ITEM **items, const char *subtitle, const boolean_t multisel) {
	int item_n = 0, i;
	MENU *menu = init_menu(items, multisel);
	int max_items = item_count(menu),
	    done = 0;
	boolean_t pattern_mode = FALSE, no_sel;
	menu_sel_t menu_sel = { NULL };
	TRACE("samc.c:choosefrommenu(%x,%s,%d)",
	    items, Str(subtitle), multisel);
	if (multisel)
		menu_sel.multisel = lst_create();
	clear();
	mvprintwc(2, 1, subtitle);
	ShowBanner();

	chk("post_menu", post_menu(menu));
	refresh();

	keypad(stdscr, TRUE);
	dis_more(menu);
	while (!done) {
		int c;
		if (row != LINES || col != COLS) {
			ITEM *oldit, *newit;
			char *newname;
			int itn,	/* item number */
			    itn_crtit = item_index(current_item(menu)),
			    toprow = top_row(menu);
			unsigned int len;	/* item name length */
			TRACE("samc.c: menu win resize (r,c)=%d,%d (L,C)=%d,%d",
			    row, col, LINES, COLS);
			/* get rid of the old menu */
			unpost_menu(menu);
			delwin(menu_sub(menu));
			free_menu(menu);

			for (itn = 0; itn < max_items; itn++) {
				oldit = items[itn];
				len = MIN(col,
				    strlen(item_description(oldit)) + 1) - 2;
				newname = (char *)malloc(len + 1);
				strncpy(newname, item_description(oldit), len)
				    [len] = '\0';
				newit = new_item(newname,
				    item_description(oldit));
				set_item_value(newit, item_value(oldit));
				set_item_opts(newit, item_opts(oldit));
				free(item_name(oldit));
				free_item(oldit);
				items[itn] = newit;
			}

			/* reinitialize screen */
			reinitscr();

			/* recreate menu; must fit within new LINESxCOLS */
			menu = init_menu(items, multisel);

			/* jump to the old position */
			set_top_row(menu, toprow);
			set_current_item(menu, items[itn_crtit]);
			toggleduplicates(menu);

			post_menu(menu);
			mvprintwc(2, 1, subtitle);
			refresh();
		}

		ShowBanner();
		mvprintwc(2, 1, subtitle);
		dis_more(menu);
		pos_menu_cursor(menu);
		refresh();
		c = getch();
		if (c == ERR)  continue;
		switch (c) {

		case 'q':
			if (askw(LINES - 1, 0, "Quit? [Y/n]", 'y'))
				exit(0);
			move(LINES - 1, 0); clrtoeol();
			break;
		case KEY_UP:
			if (E_OK != menu_driver(menu, REQ_PREV_ITEM))
				beep();
			break;
		case KEY_DOWN:
			if (E_OK != menu_driver(menu, REQ_NEXT_ITEM))
				beep();
			break;
		case KEY_full_fwd:
			if (E_OK != menu_driver(menu, REQ_SCR_DPAGE)) {
				menu_driver(menu, REQ_LAST_ITEM);
				beep();
			}
			break;
		case KEY_full_bwd:
			if (E_OK != menu_driver(menu, REQ_SCR_UPAGE)) {
				menu_driver(menu, REQ_FIRST_ITEM);
				beep();
			}
			break;
		case ' ':
			/* end pattern */
			if (pattern_mode) {
				set_menu_pattern(menu, "");
				move(3, 0); clrtoeol();
				pattern_mode = FALSE;
				break;
			}
			/* select/unselect the current item */
			if (!(item_opts(current_item(menu)) & O_SELECTABLE)) {
				beep();
				break;
			}
			menu_driver(menu, REQ_TOGGLE_ITEM);
			if (multisel)
				toggleduplicates(menu);
			if (E_OK == menu_driver(menu, REQ_NEXT_ITEM))
				item_n++;
			break;
		case KEY_esc:
		case PREV:
			done = 1;
			break;
		case KEY_ENTER:
		case KEY_enter:
			/* populate list with all selected items */
			i = -1;
			no_sel = B_TRUE;
			while (i++ < max_items)
				if (item_value(items[i])) /* selected */ {
					item_info_t *ii = (item_info_t *)
					    malloc(sizeof (item_info_t));
					ii->str = (char *)
					    strdup(item_name(items[i]));
					ii->usrptr = item_userptr(items[i]);
					if (multisel)
						lst_append(menu_sel.multisel,
						    ii);
					no_sel = B_FALSE;
				}
			if (no_sel && multisel) {
				if (askw(LINES - 1, 2,
				    "No items selected. Abandon? [Y/n]", 'y'))
					done = 1;
				break;
			}
			if (askw(LINES - 1, 2, "Proceed? [Y/n]", 'y')) {
				if (!multisel) {
					item_info_t *ii = (item_info_t *)
					    malloc(sizeof (item_info_t));
					ii->str = (char *)strdup(
						item_name(current_item(menu)));
					ii->usrptr =
					    item_userptr(current_item(menu));
					menu_sel.singlsel = *ii;
				}
				done = 1;
			} else {
				setsyx(LINES - 1, 2);
				clrtoeol();
				if (multisel) {
					setsyx(LINES - 1, 2);
					clrtoeol();
					lst_free_deep(menu_sel.multisel);
					menu_sel.multisel = lst_create();
				}
			}
			break;

		case 'h':	/* home */
			done = 1;
			break;

		case KEY_help:
			help(); /* display help screen */
			touchwin(menu_win(menu));
			break;

		case KEY_BACKSPACE: /* does not work */
		case KEY_backspace:
			if (strlen(menu_pattern(menu))) {
				menu_driver(menu, REQ_BACK_PATTERN);
				if (strlen(menu_pattern(menu))) {
					mvprintw(3, 0, "Find: %s",
					    menu_pattern(menu));
					clrtoeol();
				} else {
					move(3, 0); clrtoeol();
					pattern_mode = FALSE;
				}
			} else
				beep();
			break;

		default:
			// mvprintw(1, 0, "key typed %x %s", c, keyname(c));
			// clrtoeol();
			if (isalnum(c) || (c == '/')) {
				pattern_mode = TRUE;
				if (E_NO_MATCH != menu_driver(menu, c)) {
					mvprintw(3, 0, "Find: %s",
					    menu_pattern(menu));
					break;
				}
			}
			beep();
			break;
		}
	}
	/* cleanup */
	TRACE("samc.c:choosefrommenu(): cleaning up");
	unpost_menu(menu);
	delwin(menu_sub(menu));
	free_menu(menu);
	erase();
	TRACE("samc.c:choosefrommenu() exit");
	return (menu_sel);

}

// ---------------------- generic filesystem-related functions ---------------

/*
 * pass back all configured filesystems as a NULL terminated array of ITEMs.
 * return the length of the array or -1 if error.
 */
static int
get_fsys_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *fss;
	node_t *node;

	TRACE("samc.c:get_fsys_items() entry");
	if (-1 == cfg_get_fs_names(&fss)) {
		mvprintw(ln++, 3, "Cannot obtain filesystem information:");
		showerr();
		refresh();
		return (-1);
	}
	TRACE("samc.c:get_fs_names() done");
	if (!fss) {
		mvprintw(ln++, 3, "No filesystems found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = fss->head;
	while (node) {
		items[crtit++] = new_item((char *)node->data, "");
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_fsys_items() exit");
	return (crtit);
}

/*
 * choose filesystems from list
 */
static boolean_t
choosefs(const boolean_t multisel) {
	ITEM *items[MAX_FSYS];
	menu_sel_t menu_sel;
	TRACE("samc.c:choosefs(%d)", multisel);

	if (0 >= get_fsys_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);

	menu_sel = choosefrommenu(items,
	    "Choose the file system to be configured",
	    multisel);

	if (!multisel)
		if (menu_sel.singlsel.str) {
			strcpy(selectedfs, menu_sel.singlsel.str);
			TRACE("samc.c:selectedfs=%s.", Str(selectedfs));
		} else {
			home();
			return (B_FALSE);
		}
	else
		if (menu_sel.multisel->length)
			selectedfslst = menu_sel.multisel;
		else {
			home();
			return (B_FALSE);
		}

	return (B_TRUE);
}

/* static boolean_t */
/* chooseonefs() { */
/* 	return (choosefs(B_FALSE)); */
/* } */

/* static boolean_t */
/* choosemultifs() { */
/* 	return (choosefs(B_TRUE)); */
/* } */

// ------------------------- File Systems Menu ------------------------------

/* return a list of item_info that contains path names */
static sqm_lst_t *
choose_aus(const sqm_lst_t *aulst,	/* list of au_t */
    const sqm_lst_t *exclpaths,	/* exclude these paths */
    const char *title) {

	node_t *node, *exclnode;
	int max_items = aulst->length, crt = 0;
	ITEM *items[max_items + 1];
	menu_sel_t menu_sel;
	char *str;
	au_t au;

	items[max_items] = NULL;

	if (NULL == (node = aulst->head))
	    return (NULL);
	if (exclpaths)
		exclnode = exclpaths->head;
	else
		exclnode = NULL;
	for (; node; node = node->next) {
		au = *((au_t *)node->data);
		if (exclnode) {
/*			TRACE("samc.c:aupath:%s exclnode:%s.", */
/*			    Str(au.path), Str((char *)exclnode->data)); */
			if (0 == strcmp(au.path, (char *)exclnode->data)) {
				exclnode = exclnode->next;
				continue;
			}
		}
		str = (char *)malloc(100 * sizeof (char));
		/*
		 * the description field (not displayed) holds the full info.
		 * the name field holds only as much as it can be displayed.
		 * this field will change when the window is resized
		 */
		items[crt++] =
		    new_item(
			(char *)au2str(*((au_t *)node->data), str, B_FALSE),
			(char *)strdup(str));
		// TRACE(">%2d_%s\r\n", crt - 1, item_name(items[crt - 1]));
		if (! (crt % 10)) {
			printw(".");
			refresh();
		}
		if (strlen(au.fsinfo) > 0)
			item_opts_off(items[crt-1], O_SELECTABLE);
	}
	items[crt] = NULL;

	menu_sel = choosefrommenu(items, title, B_TRUE);
	TRACE("samc.c:choose_aus() exit");
	return (menu_sel.multisel);
}

static void
chkmntpt(const char *mntpt) {
	int fd;
	TRACE("samc.c:chkmntpt(%s)", Str(mntpt));
	if (-1 == (fd = open(mntpt, O_RDONLY))) {
		if (askw(ln++, 5,
		    "Mount point does not exist. Create? [Y/n]", 'y'))
			if (-1 == mkdir(mntpt, S_IRWXU
			    | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
				mvprintw(ln++, 5,
				    "Cannot create mount point");
			else
				mvprintw(ln++, 5, "Mount point created");
	} else
		close(fd);
	TRACE("samc.c:chkmntpt() exit");
}

static sqm_lst_t *
extractdev(const sqm_lst_t *chosen, sqm_lst_t *chosendsk) {
	char *dskinfo,	/* extract name from here */
	    *dskname;	/* and put it here */
	node_t *node;
	if (!chosen)
		return (chosendsk);
	node = chosen->head;
	while (node) {
		dskinfo = ((item_info_t *)node->data)->str;
		dskname = (char *)strdup(getfirststr(dskinfo));
		lst_append(chosendsk, dskname);
		node = node->next;
	}
	return (chosendsk);
}

static void
paths2basedevs(const sqm_lst_t *paths, sqm_lst_t **base_devs, devtype_t dt) {
	node_t *node = paths->head;
	base_dev_t *bdev;
	*base_devs = lst_create();
	while (node) {
		bdev = (base_dev_t *)malloc(sizeof (base_dev_t));
		strcpy(bdev->name, (char *)node->data);
		strcpy(bdev->equ_type, dt);
		bdev->eq = 0;	// will pick one automatically
		lst_append(*base_devs, bdev);
		node = node->next;
	}
}

static void
trace_striped_grps(sqm_lst_t *sg_lst) {
	node_t *n = sg_lst->head,
	    *ndsk;
	striped_group_t *g;
	TRACE("samc.c:trace_striped_grps(lst[%d])",
	    sg_lst ? sg_lst->length : -1);
	while (n) {
		g = (striped_group_t *)(n->data);
		TRACE("samc.c:grpname:%s", g->name);
		TRACE("samc.c:grp.dsks:");
		ndsk = g->disk_list->head;
		while (ndsk) {
			TRACE("samc.c: -%s.",
			    Str(((disk_t *)ndsk->data)->base_info.name));
			ndsk = ndsk->next;
		}
		n = n->next;
	}
	TRACE("samc.c:trace_striped_grps() exit");
}

static int
disco_aus_and_handle_noaus(sqm_lst_t **lst) {
	halfdelay(10);
	ln += 2;
	mvprintw(ln, 5, GATHERDSKINFO_MSG);
	refresh();
	if (-1 == discover_avail_aus(NOCTX, lst)) {
		mvprintw(++ln, 5, DSKINFONOTFOUND_ERRMSG);
		refresh();
		beep();
		anykey(++ln, 5);
		home();
		return (-1);
	}
	return (0);
}

static void
showchosendevs(sqm_lst_t *chosen) {
	node_t *node;
	mvprintw(ln++, 0,
	    "  Slices/Volumes selected:");
	node = chosen->head;
	while (node) {
		mvprintw(ln++, 1, "  %s", ((item_info_t *)node->data)->str);
		node = node->next;
	}
}

static int
choosegrpdevs(int ngrps,	/* prompt user for this many groups */
    sqm_lst_t *availaus,		/* discovered aus */
    sqm_lst_t *exclpaths,		/* paths already chosen */
    sqm_lst_t **grplst,		/* chosen striped groups */
    sqm_lst_t **devs) {		/* all the devices in the chosen groups */

	int grpcnt = 0;
	striped_group_t *grp;
	char subtitle[80];
	sqm_lst_t *chosendevs, *chosenpaths;

	*grplst = lst_create();
	*devs = lst_create();

	while (grpcnt < ngrps) {
		sprintf(subtitle,
		    "Step 2/4: "CHOOSEDATA_MSG);
		if (ngrps > 1)
			sprintf(subtitle, "%s for group %d",
			    subtitle, grpcnt + 1);
		chosendevs = choose_aus(availaus, exclpaths,
		    subtitle);
		if (!chosendevs->length) {
			home();
			return (-1);
		}
		chosenpaths = extractdev(chosendevs, lst_create());

		grp = (striped_group_t *)malloc
		    (sizeof (striped_group_t));
		grp->name[0] = '\0';
		paths2basedevs(chosenpaths,
		    &grp->disk_list, ""); // auto-pick equ_type
		lst_append(*grplst, grp);

		lst_concat(exclpaths, chosenpaths);
		lst_concat(*devs, chosendevs);
		grpcnt++;
	}
	trace_striped_grps(*grplst);
	return (0);
}

static void
addfs() {
	sqm_lst_t *lst, *chosen;
	fs_t fs;
	FORM *form;
	FIELD **flds;
	int res;
	char daustr[10] = "";	// tmp
	boolean_t split = B_FALSE;

	fs.meta_data_disk_list = NULL;
	fs.data_disk_list = NULL;
	fs.striped_group_list = NULL;
	fs.mount_options = NULL;
	fs.fs_count = fs.mm_count = 0;

	/* Step 1: choose name + license-dependent settings */

	clear();
	ShowBanner();
	refresh();
	flds = mkfields(addfs_FORM);
	if (!(form = new_form(flds))) {
		slog("samc: error creating addfs form");
		return;
	}
	display_form(form);
	res = process_form(form, "Step 1/4: Basic file system information");
	if (-1 == res) {
		jmp_to_entry(toplev_dis[0], numof(toplev_dis));
		return;
	}
	TRACE("samc.c:before conv: dau=%d", fs.dau);
	/* put form data back in the structure */
	convert_form_data(form, (char *)&fs);
	TRACE("samc.c:fs:name=%s eq=%d dau=%d",
	    Str(fs.fi_name), fs.fi_eq, fs.dau);
	sprintf(daustr, "%d", fs.dau);

	destroy_form(form);

	if (0 == strncmp(field_val(flds[7]), "split", 5))
		split = B_TRUE;

	/* Step 2: choose devices */
	if (-1 == disco_aus_and_handle_noaus(&lst))
		return;

	if (split) {
		int striped_grps = -1;
		/* choose metadata devices first */
		sqm_lst_t *chosen_meta;
		sqm_lst_t *chosen_data;
		sqm_lst_t *chosen_meta_paths = NULL;
		sqm_lst_t *chosen_data_paths = NULL;

		strcpy(fs.equ_type, "ma");

		chosen_meta = choose_aus(lst, NULL,
		    "Step 2/4: "CHOOSEMETA_MSG);
		if (!chosen_meta->length) {
			home();
			return;
		}
		chosen_meta_paths = extractdev(chosen_meta, lst_create());
		paths2basedevs(chosen_meta_paths,
		    &fs.meta_data_disk_list, "mm");
		fs.mm_count = fs.meta_data_disk_list->length;

		/* now choose data devices */
		striped_grps = atoi(field_val(flds[11]));
		if (!striped_grps) {	/* mr or md */
			devtype_t dt;	/* equ type for data devices */
			chosen_data = choose_aus(lst, chosen_meta_paths,
			    "Step 2/4: "CHOOSEDATA_MSG);
			if (!chosen_data->length) {
				home();
				return;
			}
			chosen_data_paths =
			    extractdev(chosen_data, lst_create());
			TRACE("samc.c:field_val=%s.", field_val(flds[9]));
			if (0 == strncmp(large_dau, field_val(flds[9]),
			    strlen(large_dau)))		/* if large DAU */
				strcpy(dt, "mr");
			else				/* dual DAU */
				strcpy(dt, "md");
			paths2basedevs(chosen_data_paths,
			    &fs.data_disk_list, dt);

			lst_concat(chosen_meta, chosen_data);
			chosen = chosen_meta;
		} else { /* striped groups */
			chosen_data = lst_create();
			if (-1 != choosegrpdevs(striped_grps, lst,
			    chosen_meta_paths,
			    &fs.striped_group_list, &chosen_data)) {
				chosen = chosen_meta;
				lst_concat(chosen, chosen_data);
			}
		}

		lst_free_deep(chosen_meta_paths);
		PTRACE(3, "samc.c:free metapaths done");
		lst_free_deep(chosen_data_paths);
		PTRACE(3, "samc.c:free datapaths done");

	} else { /* combined metadata and data */
		sqm_lst_t *chosen_paths;
		strcpy(fs.equ_type, "ms");

		chosen = choose_aus(lst, NULL, "Step 2/4: "CHOOSEDEVS_MSG);
		if (!chosen->length) {
			home();
			return;
		}
		chosen_paths = extractdev(chosen, lst_create());
		paths2basedevs(chosen_paths, &fs.data_disk_list, "md");
/* 		TRACE("samc.c:--- BASE_DEV LIST ---"); */
/* 		node = fs.data_disk_list->head; */
/* 		while (node) { */
/* 			TRACE("samc.c:ddsk: %s.", Str(node->data)); */
/* 			node = node->next; */
/* 		} */

	}
	fs.fs_count = chosen->length;
	TRACE("samc.c: fs_count=%d", fs.fs_count);
	free_au_list(lst);

	/* Step 3 -- mount options */
	fs.mount_options = (mount_options_t *)malloc(sizeof (mount_options_t));
	memset(fs.mount_options, 0, sizeof (mount_options_t));

	/* get default values */
	TRACE("samc.c: eqtp:%s dau:%d", fs.equ_type, fs.dau);
	get_default_mount_options(NOCTX, fs.equ_type, fs.dau,
	    fs.striped_group_list ? B_TRUE : B_FALSE,
	    B_FALSE, B_FALSE, &fs.mount_options);

	TRACE("samc.c:dfl str=%d trace=%s", fs.mount_options->stripe,
	    fs.mount_options->trace ? "T" : "F");
	TRACE("samc.c:dfl hwm=%d lwm=%d",
	    fs.mount_options->sam_opts.high,
	    fs.mount_options->sam_opts.low);


/* 	TRACE("samc.c:writing"); */
/* 	write_fs_cmds(f = fopen("/tmp/tstmnt", "w"), &fs, NULL); */
/* 	fclose(f); */
/* 	TRACE("samc.c:file written"); */
	/* initialize form default values */
	init_field_recs_dfl(mnt_FORM, fs.mount_options);

	/* get input from user */
	mntopts(fs.mount_options, "Step 3/4: Mount options");

	/* Step 4 */
	clear();
	ShowBanner();
	ln = 2;
	mvprintw(ln, 1,	"Step 4/4: Finish configuration");
	ln += 3;
	mvprintw(ln++, 0, "  File system: %s", fs.fi_name);
	ln++;
	mvprintw(ln++, 0,
	    "  File system type: archiving, %s metadata and data",
	    split ? "split" : "combined");
	mvprintw(ln++, 0,
	    "  DAU size: ");
	if (fs.dau == uint16_reset) {
		printw("default (16)");
		fs.dau = 0; // should be removed after API is fixed
	} else
		printw("%dkb", fs.dau);
	ln++;

	showchosendevs(chosen);
	lst_free(chosen);

	ln++;
	mvprintwc(ln++, 0, DATALOST_WRNMSG);
	if (askw(ln++, 0, "  Create file system? [y/N]", 'n')) {
		int res =
		    create_fs(NOCTX, &fs, B_TRUE);
		if (res == -1)
			showerr();
		else {
			mvprintw(ln++, 0, "  File system %s created.",
			    fs.fi_name);
			ln++;
			if (askw(ln++, 5, ASKMOUNT_MSG, 'n')) {
				char mntpt[MAX_PATH_LENGTH] = "",
				    nameandmntpt[MAX_PATH_LENGTH];
				mvprintw(ln, 0, "     Mount point: ");
				ln++;
				if (NULL == getstring(mntpt, MAX_PATH_LENGTH))
					res = -1;
				else {
					chkmntpt(mntpt);
					sprintf(nameandmntpt, "%s %s",
					    fs.fi_name, mntpt);
					res = mount_fs(NOCTX, nameandmntpt);
				}
				if (res != -1)
					mvprintw(ln++, 0,
					    "     File system %s mounted.",
					    fs.fi_name);
				else {
					showerr();
					mvprintw(ln++, 0,
					    "     File system %s not mounted.",
					    fs.fi_name);
				}
			} else
				mvprintw(ln++, 5,
				    "File system %s not mounted", fs.fi_name);
		} // fs created
	} else
		mvprintw(ln++, 5, OPABAND_MSG);
	refresh();
	ln++;
	if (!askw(ln, 0, "  Add a new file system? [y/N]", 'n'))
		home();
}

static boolean_t
growfs() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_FSYS];
	fs_t *fsp;
	sqm_lst_t *aus;
	sqm_lst_t *new_mm_dsk = NULL,
	    *new_data_dsk = NULL,
	    *new_grps = NULL;
	sqm_lst_t *chosen_meta_paths = NULL;
	sqm_lst_t *chosen_data_paths = NULL;
	sqm_lst_t *chosen_data;
	sqm_lst_t *chosen;		/* all newly chosen devices */

	TRACE("growfs() entry");
	if (0 >= get_fsys_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);
	menu_sel = choosefrommenu(items,
	    "Select the file system to be grown", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedfs, menu_sel.singlsel.str);
	if (-1 == cfg_get_fs(selectedfs, &fsp)) {
		showerr();
		anykey(ln, 5);
		return (B_FALSE);
	}

	/* discover devices */
	if (-1 == disco_aus_and_handle_noaus(&aus))
		return (B_FALSE);
	ShowBanner();
	/* add devices */
	if (fsp->mm_count) {	/* if (SAM)QFS */

		sqm_lst_t *chosen_meta;
		TRACE("samc.c:growfs:adding new mm devices");
		chosen_meta = choose_aus(aus, NULL, CHOOSEMETA_MSG);
		if (!chosen_meta->length) {
			home();
			return (B_FALSE);
		}
		chosen_meta_paths = extractdev(chosen_meta, lst_create());
		paths2basedevs(chosen_meta_paths, &new_mm_dsk, "mm");
		chosen = chosen_meta;

		if (askw(ln++, 5, ASKADDDATA_MSG, 'y')) {

			/*
			 * check if (non-striped groups) data devices exist.
			 * If yes, then choose additional data devices
			 */
			if (fsp->data_disk_list->length) {
				char *eqtype;
				TRACE("samc.c:growfs:adding new data devices "
				    "(found:%d)", fsp->data_disk_list->length);
				chosen_data = choose_aus(aus,
				    chosen_meta_paths, CHOOSEDATA_MSG);
				if (!chosen_data->length) {
					home();
					return (B_FALSE);
				}
				chosen_data_paths = extractdev(chosen_data,
				    lst_create());
				/*
				 * same eq type as the one of the existing
				 * data devices
				 */
				eqtype =
				    ((disk_t *)fsp->data_disk_list->head->data)
				    ->base_info.equ_type;
				paths2basedevs(chosen_data_paths, &new_data_dsk,
				    eqtype);
				lst_concat(chosen, chosen_data);

			} else {
				/* add more striped groups */
				int striped_grps = -1;
				sqm_lst_t *gdevs;
				mvprintw(ln, 5, NUMOFNEWGRPS_MSG);
				striped_grps = getint(NULL);
				ln++;
				TRACE("samc.c:growfs:add %d new grps",
				    striped_grps);
				if (-1 == choosegrpdevs(striped_grps, aus,
				    chosen_meta_paths, &new_grps, &gdevs))
					return (B_FALSE);
				lst_concat(chosen, gdevs);

			}
		}

	} else {	/* combined metadata and data */

		TRACE("samc.c:growfs:adding new mm devices");
		chosen_data = choose_aus(aus, NULL, CHOOSEDEVS_MSG);
		if (!chosen_data->length) {
			home();
			return (B_FALSE);
		}
		chosen_data_paths = extractdev(chosen_data, lst_create());
		paths2basedevs(chosen_data_paths, &new_data_dsk, "md");
		chosen = chosen_data;
	}

	lst_free_deep(chosen_meta_paths);
	TRACE("samc.c:free metapaths done");
	lst_free_deep(chosen_data_paths);
	TRACE("samc.c:free datapaths done");
	free_au_list(aus);

	ShowBanner();
	ln = 3;
	mvprintw(ln++, 3, "You selected file system %s to be grown",
	    selectedfs);
	mvprintw(ln++, 3, DATALOST_WRNMSG);

	ln++;
	showchosendevs(chosen);
	lst_free(chosen);

	ln++;
	if (askw(ln++, 3, "Grow file system? [y/N]", 'n'))
		if (-1 ==
		    grow_fs(NOCTX, fsp, new_mm_dsk, new_data_dsk, new_grps))
			showerr();
		else
			mvprintw(ln++, 5, CFGUPD_MSG);
	else
		mvprintw(ln++, 5, OPABAND_MSG);
		anykey(ln, 5);
	refresh();
	TRACE("samc.c: growfs() exit");
	return (B_FALSE);
}

static boolean_t
rmfs() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_FSYS];
	TRACE("rmfs() entry");
	if (0 >= get_fsys_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);
	menu_sel = choosefrommenu(items,
	    "Select the file system to be removed", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedfs, menu_sel.singlsel.str);
	mvprintw(ln++, 3, "WARNING: You selected file system %s to be removed",
	    selectedfs);
	if (askw(ln++, 3, "Remove file system? [y/N]", 'n'))
		if (-1 == remove_fs(NOCTX, selectedfs))
			showerr();
		else
			mvprintw(ln++, 5, CFGUPD_MSG);
	else
		mvprintw(ln++, 5, OPABAND_MSG);
	anykey(ln, 5);
	refresh();
	TRACE("samc.c: rmfs() exit");
	return (B_FALSE);
}

/* create and process Mount options form. used by addfs() and modifmntopts() */
static void
mntopts(mount_options_t *mnt_opts, char *formtitle) {
	FORM *form;

	TRACE("samc.c:mntopts(%x,%s)", mnt_opts, formtitle);
	clear();
	ShowBanner();
	refresh();

	/* create form */
	if (! (form = new_form(mkfields(mnt_FORM)))) {
		TRACE("samc.c:error return from new_form");
		return;
	}
	display_form(form);
	/* get user input */
	if (-1 == process_form(form, formtitle))
		return;
	mnt_opts->sam_opts.change_flag = 0;
	/* put input back in the mnt_opts structure */
	convert_form_data(form, (char *)mnt_opts);
	TRACE("samc.c:hwm=%d lwm=%d stripe=%d trace=%s",
	    mnt_opts->sam_opts.high, mnt_opts->sam_opts.low,
	    mnt_opts->stripe, mnt_opts->trace ? "yes" : "no");
	TRACE("samc.c:rda=%lld wbe=%lld wth=%lld",
	    mnt_opts->io_opts.readahead, mnt_opts->io_opts.writebehind,
	    mnt_opts->io_opts.wr_throttle);
	TRACE("samc.c:prel=%d max=%d pstg=%d stgr=%d stgw=%d rarch=%s",
	    mnt_opts->sam_opts.partial, mnt_opts->sam_opts.maxpartial,
	    mnt_opts->sam_opts.partial_stage, mnt_opts->sam_opts.stage_retries,
	    mnt_opts->sam_opts.stage_n_window,
	    mnt_opts->sam_opts.hwm_archive ? "yes" : "no");

	// return (B_FALSE);
}

static boolean_t
modifmntopts() {
	ITEM *items[MAX_FSYS];
	fs_t *fsp;
	mount_options_t *dfl_mntopts; // default mnt.opts. for selected fsys
	menu_sel_t menu_sel;

	TRACE("samc.c: modifmntopts() entry");

	/* select a file system */
	if (0 >= get_fsys_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);
//	do {
	menu_sel = choosefrommenu(items, "Select a file system", B_FALSE);
	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedfs, menu_sel.singlsel.str);

	/* get mount options for the selected file system */
	if (-1 == cfg_get_fs(selectedfs, &fsp)) {
		showerr();
		anykey(ln, 5);
		return (B_FALSE);
	}
//	} while (B_TRUE ==

	/* initialize values */
	init_field_recs_val(mnt_FORM, fsp->mount_options);

	/* intialize defaults */
	get_default_mount_options(NOCTX, fsp->equ_type,
	    16,
	    fsp->striped_group_list ? B_TRUE : B_FALSE,
	    B_FALSE, B_FALSE, &dfl_mntopts);
	init_field_recs_dfl(mnt_FORM, dfl_mntopts);
	free(dfl_mntopts);

	/* second, let the user modify mount options */
	mntopts(fsp->mount_options, "Mount options");

	/* write configuration chages */
	if (-1 == change_mount_options(NOCTX, selectedfs, fsp->mount_options)) {
		showerr();
	} else {
		mvprintw(ln++, 3, CFGUPD_MSG);
	}
	anykey(ln, 3);
	refresh();

	free_fs(fsp);
	TRACE("samc.c:modifmntopts() exit");
	return (B_FALSE);
}

// ------------------------- Archiving Policies Menu -------------------------

static boolean_t
choosearfs() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_FSYS];
	int nfs;	/* no. of configured filesystems */
	TRACE("choosearfs() entry");
	if (-1 == (nfs = get_fsys_items(items)))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);
	items[nfs] = new_item(GLOBAL, "");
	items[nfs + 1] = NULL;
	menu_sel = choosefrommenu(items,
	    "Select the file system to be configured", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedfs, menu_sel.singlsel.str);
	return (B_TRUE);
}

static boolean_t
addas() {
	CRT_OPN = OPN_ADD_AS;

	ln++;
	mvprintwc(ln++, 3, "Archive policy name: ");

	if (NULL != getstring(selectedas, MAX_NAME_LENGTH - 2))

		/* archiving criteria */

		addcrit();

	return (B_FALSE);
}

char *
crit2str(ar_set_criteria_t *crit) {
	static char critstr[70] = "";
	sprintf(critstr, "%-20s %s", crit->set_name, crit->path);
	return (critstr);
}
/*
 * pass back all archive set criteria defined for the specified filesystems,
 * as a NULL terminated array of ITEMs.
 * return the length of the array or -1 if error.
 */
static int
get_crit_items(const char *fsname, ITEM **items) {
	int crtit = 0;
	sqm_lst_t *crits;
	node_t *node;

	TRACE("samc.c:get_crit_items() entry");
	if (-1 == get_ar_set_criteria_list(NOCTX, fsname, &crits)) {
		mvprintw(ln++, 3, "Cannot obtain policy information:");
		showerr();
		anykey(ln++, 3);
		refresh();
		return (-1);
	}
	if (!crits) {
		mvprintw(ln++, 3,
		    "No policies defined for filesystem %s", fsname);
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = crits->head;
	while (node) {
		items[crtit] = new_item((char *)
		    strdup(crit2str((ar_set_criteria_t *)node->data)),
		    "");
		set_item_userptr(items[crtit++], (char *)node->data);
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_crit_items() exit");
	return (crtit);
}

static boolean_t
choosecrit(const char *fsname) {
	menu_sel_t menu_sel;
	ITEM *items[MAX_ARSETS];
	char ftitle[80];

	switch (get_crit_items(fsname, items)) {
	case -1:
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	case 0:
		ln++;
		mvprintw(ln++, 3, NOARPOL_MSG);
		anykey(ln++, 3);
		return (B_FALSE);
	}

	sprintf(ftitle, "Choose an archive policy (filesystem %s)",
	    selectedfs);
	menu_sel = choosefrommenu(items, ftitle, B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedas, getfirststr(menu_sel.singlsel.str));
	selcrit = (ar_set_criteria_t *)menu_sel.singlsel.usrptr;
	return (B_TRUE);
}

/* add/modify arch. criteria */
static boolean_t
crit(ar_set_criteria_t *crit, int opn) { /* opn = 0 (add), 1 (modify) */
	FORM *form;
	int res;
	char title[80];
	ar_fs_directive_t *arfsd;
	ar_set_criteria_t *dflcrit;

	TRACE("samc.c:crit() entry");
	clear();
	ShowBanner();
	refresh();

	/* get default values for achiving criteria */
	if (-1 == get_default_ar_set_criteria(NOCTX, &dflcrit)) {
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	}
	init_field_recs_dfl(archcrit_FORM, dflcrit);
	free_ar_set_criteria(dflcrit);

	if (opn == 1)	/* modify */
		init_field_recs_val(archcrit_FORM, crit);

	/* get input from user */
	if (! (form = new_form(mkfields(archcrit_FORM))))
		perror("error return from new_form");
	display_form(form);
	sprintf(title, "Archiving policy %s", selectedas);
	res = process_form(form, title);
	if (-1 != res) {
		strcpy(crit->fs_name, selectedfs);
		strcpy(crit->set_name, selectedas);
		convert_form_data(form, (char *)crit);
		if (0 == opn) { /* add */
			if (-1 != get_ar_fs_directive(NOCTX, selectedfs,
			    &arfsd)) {
				res = -1;
				lst_append(arfsd->ar_set_criteria, crit);
				if (-1 ==
				    (res = set_ar_fs_directive(NOCTX, arfsd)))
					showerr();
				else
					mvprintw(ln++, 3, CFGUPD_MSG);
				free_ar_fs_directive(arfsd);
			} else
				showerr();
		} else		/* modify */
			if (-1 == (res = modify_ar_set_criteria(NOCTX, crit)))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		anykey(ln++, 3);
	}
	if (CRT_OPN != OPN_SINGLE && res != -1)
		arcopies();
	else
		jmp_to_entry(ar_dis[1], numof(ar_dis));
	TRACE("samc.c:crit() exit");
	return (B_FALSE); /* do not attempt to further process this menu item */
}

static boolean_t
addcrit() {
	free_ar_set_criteria(selcrit);
	if (-1 == get_default_ar_set_criteria(NOCTX, &selcrit)) {
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	}
	return (crit(selcrit, 0));
}

static boolean_t
modifcrit() {
	assert(selcrit);
	return (crit(selcrit, 1));

}

static boolean_t
ardir() {
	FORM *form;
	int res;
	char title[80];
	ar_fs_directive_t *arfsd;

	TRACE("samc.c:ardir() entry");
	/* initialize defaults */
	if (-1 == get_default_ar_fs_directive(NOCTX, &arfsd)) {
		showerr();
		TRACE("samc.c:ardir() exit");
		return (B_FALSE);
	}
	init_field_recs_dfl(arfsdir_FORM, arfsd);
	free(arfsd);

	/* initialize values */
	if (-1 == get_ar_fs_directive(NOCTX, selectedfs, &arfsd)) {
		showerr();
		TRACE("samc.c:ardir() exit");
		return (B_FALSE);
	}
	init_field_recs_val(arfsdir_FORM, arfsd);

	/* get user input */
	clear();
	ShowBanner();
	refresh();
	if (! (form = new_form(mkfields(arfsdir_FORM))))
		perror("error return from new_form");
	display_form(form);
	sprintf(title, "Archiving directives for filesystem %s", selectedfs);
	res = process_form(form, title);

	/* write changes */
	if (res != -1) {
		convert_form_data(form, (char *)arfsd);
		TRACE("samc.c:interv:%u,wait:%c,log=%s", arfsd->fs_interval,
		    arfsd->wait ? 'T' : 'F', arfsd->log_path);
		if (-1 == set_ar_fs_directive(NOCTX, arfsd))
			showerr();
		else
			mvprintw(ln++, 3, CFGUPD_MSG);
		anykey(ln++, 3);
	}

	TRACE("samc.c:ardir() exit");
	return (B_FALSE); /* do not attempt to further process this menu */
}

static void
set_arcopies_convinfo(ar_set_copy_cfg_t *copies[],
    boolean_t (*active)[]) {
	ar_set_copy_cfg_t *copy;
	char i;
	for (i = 0; i < MAX_COPY; i++) {
		copy = copies[i];
		cpdir_cinfo[3*i].dataptr = &((*active)[i]);
		cpdir_cinfo[3*i + 1].dataptr = &copy->ar_copy.ar_age;
		cpdir_cinfo[3*i + 2].dataptr = &copy->un_ar_age;
		cpdir_cinfo[3*i + 1].maskptr = cpdir_cinfo[3*i + 2].maskptr =
		    &copy->change_flag;
	}
}

static int
_arcopies(char *ftitle, ar_set_copy_cfg_t *copies[MAX_COPY]) {
	FORM *form;
	int i, ncopy = 0;
	ar_set_criteria_t arset;
	ar_set_copy_cfg_t *dflt_copy;
	boolean_t active[MAX_COPY] = { B_TRUE };
	/* B_TRUE is copy active (exists). Only one copy by default. */

	/* initialize default values */
	get_default_ar_set_copy_cfg(NOCTX, &dflt_copy);
	arset.arch_copy[0] = arset.arch_copy[1] =
	    arset.arch_copy[2] = arset.arch_copy[3] = dflt_copy;
	set_arcopies_convinfo(arset.arch_copy, &active);
	init_field_recs_dfl(archcopydir_FORM, NULL);
	free(dflt_copy);

	/* initialize values */
	for (i = 0; i < MAX_COPY; i++)
		if (copies[i])
			active[i] = B_TRUE;
		else {
			/*
			 * allocate space for this copy,
			 * in case the user chooses to activate it.
			 */
			get_default_ar_set_copy_cfg(NOCTX, &copies[i]);
			active[i] = B_FALSE;
		}
	set_arcopies_convinfo(copies, &active);
	init_field_recs_val(archcopydir_FORM, NULL);

	clear();
	ShowBanner();
	refresh();
	if (! (form = new_form(mkfields(archcopydir_FORM))))
		perror("error return from new_form");
	display_form(form);

	/* get input from user */
	if (-1 != process_form(form, ftitle)) {

		convert_form_data(form, NULL);
		/* dispose the inactive copies */
		for (i = 0; i < MAX_COPY; i++) {
			TRACE("samc.c:copy%d %s",
			    i + 1, active[i] ? "active" : "nonactive");
			if (copies[i] == NULL)
				TRACE("samc.c:copies[%d] is NULL", i);
			copies[i]->ar_copy.copy_seq = i + 1;
			if (!active[i]) {
				free(copies[i]);
				copies[i] = NULL;
			} else
				ncopy++;
		}
	}
	TRACE("samc.c:_arcopies exit");
	return (ncopy);
}

static boolean_t
metaarcopies() {
	char ftitle[80];
	ar_fs_directive_t *arfsd;

	TRACE("samc.c:metaarcopies() entry");
	/* get archiving directives for selected file system */
	if (-1 == get_ar_fs_directive(NOCTX, selectedfs, &arfsd)) {
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	}

	/* configure metadata archive copies */
	sprintf(ftitle, "Metadata copies for filesystem %s", selectedfs);

	if (arfsd->num_copies = _arcopies(ftitle, arfsd->fs_copy)) {
	/* if copy information changed, then write changes */

		TRACE("samc.c:calling set_ar_fs_directive(%d copies)",
		    arfsd->num_copies);
		if (-1 == set_ar_fs_directive(NOCTX, arfsd))
			showerr();
		else
			mvprintw(ln++, 3, CFGUPD_MSG);
		anykey(ln++, 3);
	}
	TRACE("samc.c:metaarcopies() exit");
	return (B_FALSE);
}

static boolean_t
arcopies() {
	char ftitle[80];
	int res;

	/* a criteria must have been selected at this point */
	assert(selcrit);

	/* configure archive copies */
	sprintf(ftitle, "Copies for archive policy %s for filesystem %s",
	    selectedas, selectedfs);
	res = _arcopies(ftitle, selcrit->arch_copy);

	/* write changes */
	if (-1 != res)
		if (-1 == (res = modify_ar_set_criteria(NOCTX, selcrit)))
			showerr();
		else
			mvprintw(ln++, 3, CFGUPD_MSG);

	/* decide where to go from here */
	if (CRT_OPN != OPN_SINGLE && res != -1) {
/* 		if (!askw(ln++, 3, */
/*		    "Create archive set? [y/N]", 'n')) */
/* 			mvprintwc(ln++, 5, "Archive set not written!"); */
/* 		else */
/* 			mvprintwc(ln++, 5, CFGUPD_MSG); */
/* 		ln++; */
		switch (CRT_OPN) {
		case OPN_ADD_FS:
			CRT_OPN = OPN_SINGLE;
			if (askw(ln++, 3,
			    "Create new file system? [Y/n]", 'y'))
				jmp_to_entry(toplev_dis[0], numof(toplev_dis));
			else
				jmp_to_entry(default_menu, numof(toplev_dis));
			break;
		case OPN_ADD_AS:
			CRT_OPN = OPN_SINGLE;
			if (askw(ln++, 3,
			    "Create new archive policy? [Y/n]", 'y'))
				addas();
			else
				home();
			break;
		default:
			jmp_to_entry(default_menu, numof(toplev_dis));
		}
	} else
		anykey(ln++, 3);

	/* else go back to archiving policies menu */
	return (B_FALSE);
}

static boolean_t
modifas() {
	CRT_OPN = OPN_SINGLE;
	assoc_FORM[4].val = selectedas;
	return (choosecrit(selectedfs));
}

// ------------------------- Archiving Resources Menu -------------------------

/*
 * return a list of ITEMs coressponding to the list of existing archive sets
 * if arhiver.cmd has errors, an error message is displayed
 */
static int
get_arsets_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *cparams_lst;
	node_t *node;

	TRACE("samc.c:get_arsets_items() entry");
	if (-1 == get_all_ar_set_copy_params(NOCTX, &cparams_lst)) {
		mvprintw(ln++, 3, "Cannot obtain archive set information:");
		showerr();
		refresh();
		anykey(ln++, 3);
		return (-1);
	}
	TRACE("samc.c:get_ar_set_names() done");
	if (!cparams_lst) {
		mvprintw(ln++, 3, "No archive sets found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = cparams_lst->head;
	while (node) {
		items[crtit++] = new_item(
			(char *)strdup(getfirststr(((ar_set_copy_params_t *)
			    node->data)->ar_set_copy_name)), "");
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_arcopy_items() exit");
	return (crtit);
}

/*
 * choose an archive set copy from the list of defined copies
 */
static boolean_t
chooseascopy() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_ARSETS];

	clear();
	ShowBanner();
	refresh();
	if (0 >= get_arsets_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);

	menu_sel = choosefrommenu(items, "Choose an archive copy", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedas, menu_sel.singlsel.str);
	return (B_TRUE);
}

static boolean_t
copyparams() {
	FORM *form;
	char ftitle[80];
	ar_set_copy_params_t *cparams;

	TRACE("samc.c:copyparams()");

	/* choose an archive copy */
	if (B_FALSE == chooseascopy()) {
		TRACE("samc.c:copyparams() exit");
		return (B_FALSE);
	}

	/* set default values */
	if (-1 == get_default_ar_set_copy_params(NOCTX, &cparams)) {
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	}
	init_field_recs_dfl(archcopyparam_FORM, cparams);
	free_ar_set_copy_params(cparams);

	/* set values */
	if (-1 == get_ar_set_copy_params(NOCTX, selectedas, &cparams)) {
		showerr();
		anykey(ln++, 3);
		return (B_FALSE);
	}
	init_field_recs_val(archcopyparam_FORM, cparams);

	if (! (form = new_form(mkfields(archcopyparam_FORM))))
		perror("error return from new_form");
	display_form(form);
	sprintf(ftitle, "Copy parameters for %s", selectedas);
	if (-1 == process_form(form, ftitle))
		mvprintw(ln++, 3, OPABAND_MSG);
	else {
		convert_form_data(form, (char *)cparams);
		TRACE("samc.c:archmax=%llu", cparams->archmax);
		TRACE("samc.c:join=%d", cparams->join);
		if (-1 == set_ar_set_copy_params(NOCTX, cparams))
			showerr();
		else
			mvprintw(ln++, 3, CFGUPD_MSG);
	}

	anykey(ln++, 3);
	TRACE("samc.c:copyparams() exit.");
	return (B_FALSE);
}

char ** lic_mtypes() {
	static char *mtypes[MAX_MEDIATYPES];
	int i = 0;
	node_t *node;
	sqm_lst_t *lic_mtypes_lst;
	get_licensed_media_types(NOCTX, &lic_mtypes_lst);
	node = lic_mtypes_lst->head;
	while (node) {
		mtypes[i++] = (char *)node->data;
		node = node->next;
	}
	lst_free(lic_mtypes_lst);
	mtypes[i++] = "fake1";
	mtypes[i++] = "fake2";
	mtypes[i] = NULL;
	return (mtypes);
}


static char *
lst2vsnexpr(sqm_lst_t *vsns) {
	static char vsnexpr[80] = "";
	node_t *node = vsns->head;
	while (node) {
		strcat(vsnexpr, " ");
		strcat(vsnexpr, (char *)node->data);
		node = node->next;
	}
	return (vsnexpr);
}
static boolean_t
addpool() {
	FORM *form;
	char **mtypes;
	vsn_pool_t vsnpool;
	char vsnexpr[MAX_VSNEXPRLEN];

	TRACE("samc.c: addpool()");
	clear();
	ShowBanner();
/* 	ln += 2; */
/* 	mvprintw(ln, 3, "Pool name: "); */
/* 	if (NULL == getstring(selectedpool, 30)) { */
/* 		mvprintw(ln++, 3, OPABAND); */
/* 		anykey(); */
/* 		return (B_FALSE); */
/* 	} */
/* 	ln++; */
/* 	mvprintw(ln++, 3, "VSN expression:"); */
/* 		if (NULL == getstring(vsne, 40)) { */
/* 		mvprintw(ln++, 3, OPABAND); */
/* 		anykey(); */
/* 		return (B_FALSE); */
/* 	} */

/* 	get_licensed_media_types(&mtypeslst); */
/* 	mtypes = lic_mtypes(mtypeslst); */
	mtypes = lic_mtypes();

	if (!*mtypes) {
		mvprintw(ln++, 3, "no licensed media types available");
		anykey(ln++, 3);
		return (B_FALSE);
	}

	pool_FORM[4].keywords = mtypes;
	pool_FORM[4].val = mtypes[0];

	refresh();
	if (! (form = new_form(mkfields(pool_FORM))))
		perror("error return from new_form");
	display_form(form);
	if (-1 == process_form(form, ""))
	    return (B_FALSE);

	pool_cinfo[0].dataptr = &vsnpool.pool_name;
	pool_cinfo[1].dataptr = &vsnpool.media_type;
	pool_cinfo[2].dataptr = vsnexpr;
	convert_form_data(form, NULL);

	// vsnexpr2lst(vsnexpr, &vsnpool.vsn_names);
	vsnpool.vsn_names = (sqm_lst_t *)str2lst(vsnexpr, " ");

	TRACE("samc.c:VSNpoolName:%s,mtype:%s,vsnlst:%s.",
	    vsnpool.pool_name, vsnpool.media_type,
	    lst2str(vsnpool.vsn_names, " "));

	if (-1 == add_vsn_pool(NOCTX, &vsnpool))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);

	lst_free_deep(vsnpool.vsn_names);

	/* do not attempt to further process this menu item */
	return (B_FALSE);
}
static int
get_vsnpool_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *pools;
	node_t *node;

	TRACE("samc.c:get_vsnpool_items() entry");
	if (-1 == get_all_vsn_pools(NOCTX, &pools)) {
		mvprintw(ln++, 3, "Cannot obtain pool information:");
		showerr();
		refresh();
		return (-1);
	}
	TRACE("samc.c:get_all_vsn_pools() done");
	if (!pools) {
		mvprintw(ln++, 3, "No pools found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = pools->head;
	while (node) {
		items[crtit++] = new_item(
			(char *)((vsn_pool_t *)node->data)->pool_name, "");
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_vsnpools_items() exit");
	return (crtit);
}

static boolean_t
choosepool() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_POOLS];

	TRACE("samc.c:choosepool()");
	if (-1 == get_vsnpool_items(items)) {
		showerr();
		return (B_FALSE);
	}
	menu_sel = choosefrommenu(items, CHOOSEPOOL_MSG, B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedpool, menu_sel.singlsel.str);
	TRACE("samc.c:choosepool() exit");
	return (B_TRUE);
}

static sqm_lst_t *
choosepools() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_POOLS];
	node_t *node;
	sqm_lst_t *poolnames = lst_create();

	TRACE("samc.c:choosepools()");
	if (-1 == get_vsnpool_items(items)) {
		showerr();
		return (B_FALSE);
	}
	menu_sel = choosefrommenu(items, CHOOSEPOOLS_MSG, B_TRUE);

	if (!menu_sel.multisel) {
		lst_free(poolnames);
		return (NULL);
	}
	node = menu_sel.multisel->head;
	while (node) {
		lst_append(poolnames, ((item_info_t *)node->data)->str);
		node = node->next;
	}

	TRACE("samc.c:choosepools() exit");
	return (poolnames);
}

static boolean_t
modifpool() {
	FORM *form;
	vsn_pool_t *vsnpool;
	char vsnexpr[80];

	TRACE("samc.c:modifpool()");
	clear();
	if (B_FALSE == choosepool())
		return (B_FALSE);
	TRACE("samc.c: selectedpool=%s.", selectedpool);
	pool_FORM[3].val = (char *)strdup(selectedpool);
	pool_FORM[4].val = (char *)strdup("lt");
	pool_FORM[4].keywords = lic_mtypes();
	get_vsn_pool(NOCTX, selectedpool, &vsnpool);
	pool_FORM[5].val = lst2vsnexpr(vsnpool->vsn_names);
	ShowBanner();
	refresh();
	if (! (form = new_form(mkfields(pool_FORM))))
		perror("error return from new_form");
	TRACE("samc.c:form created");
	display_form(form);
	/* get input from user */
	if (-1 == process_form(form, ""))
		return (B_FALSE);

	/* put the information in vsnpool */
	pool_cinfo[0].dataptr = &vsnpool->pool_name;
	pool_cinfo[1].dataptr = &vsnpool->media_type;
	pool_cinfo[2].dataptr = vsnexpr;
	convert_form_data(form, NULL);
	lst_free_deep(vsnpool->vsn_names);

	vsnpool->vsn_names = (sqm_lst_t *)str2lst(vsnexpr, " ");

	/* write changes */
	if (-1 == modify_vsn_pool(NOCTX, vsnpool))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	/* do not attempt to further process this menu item */
	TRACE("samc.c:modifpool() exit");
	return (B_FALSE);
}

static boolean_t
rmpool() {
	TRACE("samc.c:rmpool()");
	clear();
	ShowBanner();
	refresh();
	if (B_FALSE == choosepool())
		return (B_FALSE);

	if (-1 == remove_vsn_pool(NOCTX, selectedpool))
		showerr();
	else
		mvprintwc(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	TRACE("samc.c:rmpool() exit");
	return (B_FALSE);
}

static boolean_t
addassoc() {
	FORM *form;
	menu_sel_t menu_sel;
	ITEM *items[MAX_ARSETS];
	vsn_map_t vsnmap;
	char vsnexpr[80];
	boolean_t specpools;

	TRACE("samc.c: addassoc()");

	clear();
	ShowBanner();
	refresh();
	if (0 >= get_arsets_items(items))
		return (B_FALSE);
	if (*items == NULL)
		return (B_FALSE);
	menu_sel = choosefrommenu(items, "Select an archive copy", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedas, menu_sel.singlsel.str);
	/* initialize records */
	assoc_FORM[4].val = selectedas;
	assoc_FORM[5].keywords = lic_mtypes(); /* licensed media types */
	assoc_FORM[5].val = ""; // assoc_FORM[5].keywords[0];
	if (! (form = new_form(mkfields(assoc_FORM))))
		perror("error return from new_form");
	display_form(form);
	/* get input from user */
	if (-1 == process_form(form, "")) {
		mvprintw(ln++, 3, OPABAND_MSG);
		anykey(ln++, 3);
		TRACE("samc.c: addassoc() exit");
		return (B_FALSE);
	}
	/* put data in vsnmap */
	assoc_cinfo[0].dataptr = &vsnmap.ar_set_copy_name;
	assoc_cinfo[1].dataptr = &vsnmap.media_type;
	assoc_cinfo[2].dataptr = vsnexpr;
	assoc_cinfo[3].dataptr = &specpools;
	convert_form_data(form, NULL);
//	vsnexpr2lst(vsnexpr, &vsnmap.vsn_names);
	/* create vsn_names list */
	vsnmap.vsn_names = (sqm_lst_t *)str2lst(vsnexpr, " ");
	/* check if user wants to specify pools */
	if (specpools)
		vsnmap.vsn_pool_names = choosepools();
	else
		vsnmap.vsn_pool_names = NULL;
	TRACE("samc.c:vsnmap[%s,%s,lst[%d],lst[%d]]",
	    Str(vsnmap.ar_set_copy_name), Str(vsnmap.media_type),
	    vsnmap.vsn_names ? vsnmap.vsn_names->length : -1,
	    vsnmap.vsn_pool_names ? vsnmap.vsn_pool_names->length : -1);
	TRACE("samc.c:calling add_vsn_copy_map()");
	/* write changes */
	if (-1 == add_vsn_copy_map(NOCTX, &vsnmap))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	lst_free_deep(vsnmap.vsn_names);
	lst_free_deep(vsnmap.vsn_pool_names);

	TRACE("samc.c: addassoc() exit");
	/* do not attempt to further process this menu item */
	return (B_FALSE);
}

static char *
vsnmap2str(vsn_map_t *vsnmap) {
	static char str[80];
	sprintf(str, "%-20s %-4s",
	    vsnmap->ar_set_copy_name, vsnmap->media_type);
	if (vsnmap->vsn_names)
		if (vsnmap->vsn_names->length)
			sprintf(str, "%s %s", str,
			    (char *)lst2str(vsnmap->vsn_names, " "));
	if (vsnmap->vsn_pool_names)
		if (vsnmap->vsn_pool_names->length)
			sprintf(str, "%s %s",
			    str, (char *)lst2str(vsnmap->vsn_pool_names, " "));
	return (str);
}

static int
get_assoc_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *vsnmaps;
	node_t *node;

	TRACE("samc.c:get_assoc_items() entry");
	if (-1 == get_all_vsn_copy_maps(NOCTX, &vsnmaps)) {
		mvprintw(ln++, 3,
		    "Cannot obtain VSN associations information:");
		showerr();
		refresh();
		return (-1);
	}
	TRACE("samc.c:get_all_vsn_copy_maps() done");
	if (!vsnmaps) {
		mvprintw(ln++, 3, "No VSN associations found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = vsnmaps->head;
	while (node) {
		items[crtit++] = new_item((char *)
		    strdup(vsnmap2str((vsn_map_t *)node->data)), "");
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_assoc_items() exit");
	return (crtit);
}

static boolean_t
chooseassoc() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_VSNASSOCS];
	TRACE("samc.c:chooseassoc()");
	if (-1 == get_assoc_items(items)) {
		showerr();
		return (B_FALSE);
	}
	menu_sel = choosefrommenu(items,
	    "Choose an archive policy - VSN association",
	    B_FALSE);
	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedassoc, menu_sel.singlsel.str);
	return (B_TRUE);
}

static boolean_t
rmassoc() {
	clear();
	ShowBanner();
	refresh();
	if (B_FALSE == chooseassoc())
		return (B_FALSE);
	if (-1 == remove_vsn_copy_map(NOCTX, getfirststr(selectedassoc)))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	return (B_FALSE);
}

static boolean_t
defdiskvol() {
	FORM *form;
	disk_vol_t diskvol;

	TRACE("samc.c:diskvol()");
	clear();
	ShowBanner();
	form = new_form(mkfields(dskvol_FORM));
	display_form(form);
	if (-1 == process_form(form, ""))
		return (B_FALSE);
	convert_form_data(form, (char *)&diskvol);
	if (-1 == add_disk_vol(NOCTX, &diskvol))
		showerr();
	else
		mvprintwc(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);

	TRACE("samc.c:diskvol() exit");
	return (B_FALSE);
}

static char *
diskvol2str(disk_vol_t *vol) {
	static char str[80];
	sprintf(str, "%-20s %-15s %-30s",
	    vol->vol_name, vol->host, vol->path);
	return (str);
}

static int
get_diskvol_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *vols;
	node_t *node;

	TRACE("samc.c:get_diskvols_items() entry");
	if (-1 == get_all_disk_vols(NOCTX, &vols)) {
		mvprintw(ln++, 3,
		    "Cannot obtain disk volume information:");
		showerr();
		refresh();
		return (-1);
	}
	TRACE("samc.c:get_all_disk_vols() done");
	if (!vols) {
		mvprintw(ln++, 3, "No disk volumes found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = vols->head;
	while (node) {
		items[crtit++] = new_item((char *)
		    strdup(diskvol2str((disk_vol_t *)node->data)), "");
		node = node->next;
	}
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_diskvol_items() exit");
	return (crtit);
}

static char *
choosediskvol() {
	ITEM *items[MAX_DSKVOLS];
	menu_sel_t menu_sel;

	clear();
	ShowBanner();
	refresh();
	if (0 >= get_diskvol_items(items))
		return (NULL);
	if (*items == NULL)
		return (NULL);

	menu_sel = choosefrommenu(items, "Choose an disk volume", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (NULL);
	}
	return (strdup(getfirststr(menu_sel.singlsel.str)));

}

static boolean_t
diskar() {
	char *dskvol;
	ar_set_copy_params_t copyparams;

	clear();
	ShowBanner();
	/* choose an archive copy */
	if (B_FALSE == chooseascopy())
		return (B_FALSE);

	/* choose disk volume */
	if (NULL == (dskvol = choosediskvol()))
		return (B_FALSE);

	strcpy(copyparams.disk_volume, dskvol);
	copyparams.change_flag = AR_PARAM_disk_volume;
	if (-1 == set_ar_set_copy_params(NOCTX, &copyparams))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	free(dskvol);
	return (B_FALSE);
}

// -------------- 'Activate Archiver Configuration' Menu ------------
static boolean_t
activate_arc() {

	sqm_lst_t *err_warn_lst;
	node_t *node;
	int res;

	clear();
	ln = 2;
	res = activate_archiver_cfg(NOCTX, &err_warn_lst);
	switch (res) {
	case -1:
		/* internal error */
		showerr();
		break;
	case -2:
		/* archiver.cmd errors */
		showerr();
		node = err_warn_lst->head;
		while (NULL != node) {
			mvprintw(ln++, 4, Str(node->data));
			node = node->next;
		}
		break;
	case -3:
		/* archiver.cmd warnings */
		mvprintw(ln++, 3, ARCACTIVATED_MSG);
		mvprintw(ln++, 3, "Warnings:");
		node = err_warn_lst->head;
		while (NULL != node) {
			mvprintw(ln++, 4, Str(node->data));
			node = node->next;
		}
		break;
	default:
		/* success */
		mvprintw(ln++, 3, ARCACTIVATED_MSG);
	}
	ln++;
	anykey(ln, 3);
	home();
	return (B_FALSE);
}


// ------------------------- 'Staging' Menu -------------------------
static int
get_rb_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *rbs;
	node_t *node;

	TRACE("samc.c:get_rb_items() entry");
	if (-1 ==  get_all_libraries(NOCTX, &rbs)) {
		ln++;
		mvprintw(ln++, 3, "Cannot obtain media library information:");
		showerr();
		refresh();
		anykey(ln++, 3);
		return (-1);
	}
	TRACE("samc.c:get_all_libraries() done");
	if (!rbs) {
		mvprintw(ln++, 3, "No libraries found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = rbs->head;
	while (node) {
		items[crtit++] = new_item((char *)
		    strdup(((library_t *)node->data)->base_info.set), "");
		node = node->next;
	}
	// items[crtit++] = new_item("fake_lib", ""); // testing
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_rb_items() exit");
	return (crtit);
}

static boolean_t
chooserb() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_LIBS];

	if (0 >= get_rb_items(items))
		return (B_FALSE);

	menu_sel = choosefrommenu(items, "Choose a robot", B_FALSE);
	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedrb, menu_sel.singlsel.str);
	return (B_TRUE);
}

static boolean_t
drives() {
	drive_directive_t *drvdir;
	int res;
	TRACE("samc.c:drives() entry");
	if (!chooserb())
		return (B_FALSE);

	if (-1 == (res = get_drive_directive(NOCTX, selectedrb, &drvdir)))
		res = get_default_staging_drive_directive(NOCTX,
		    selectedrb, &drvdir);
	if (res == -1)
		showerr();
	else {
		ShowBanner();
		ln = 2;
		dis_stgmenu();
		mvprintw(ln++, 3, "Selected library: %s", selectedrb);
		mvprintw(ln, 3, "Drives: ", drvdir->count);
		/* get input from user */
		if (-1 == (drvdir->count = getint(&drvdir->count))) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			ln++;
			strcpy(drvdir->auto_lib, selectedrb);
			drvdir->change_flag = DD_count;
			/* write changes */
			res = set_drive_directive(NOCTX, drvdir);
			if (res == -1)
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	free(drvdir);
	TRACE("samc.c:drives() exit");
	return (B_FALSE);
}

static int
get_licmedia_items(ITEM **items) {
	int crtit = 0;
	sqm_lst_t *lmedia;
	node_t *node;

	TRACE("samc.c:get_licmedia_items() entry");
	if (-1 == get_licensed_media_types(NOCTX, &lmedia)) {
		ln++;
		mvprintw(ln++, 3, "Cannot obtain media license information:");
		showerr();
		refresh();
		anykey(ln++, 3);
		return (-1);
	}
	if (!lmedia) {
		mvprintw(ln++, 3, "No licensed media found!");
		refresh();
		anykey(ln++, 3);
		return (0);
	}
	node = lmedia->head;
	while (node) {
		items[crtit++] = new_item((char *)strdup((char *)node->data),
		    "");
		node = node->next;
	}
	items[crtit++] = new_item("fake_media_type", "");
	items[crtit] = (ITEM *) 0;
	TRACE("samc.c:get_licmedia_items() exit");
	return (crtit);
}

static boolean_t
choosemedia() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_MEDIATYPES];

	if (0 >= get_licmedia_items(items))
		return (B_FALSE);

	menu_sel = choosefrommenu(items, "Choose a media type", B_FALSE);
	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selectedmedia, menu_sel.singlsel.str);
	return (B_TRUE);
}

static boolean_t
bufsize() {
	int res;
	buffer_directive_t *bufdir = NULL;
	char fsizestr[FSIZE_STR_LEN];

	if (!choosemedia())
		return (B_FALSE);
	if (-1 ==
	    (res = get_buffer_directive(NOCTX, selectedmedia, &bufdir)))
		if (samerrno == SE_NO_BUFFER_DIRECTIVE_FOUND)
			res = get_default_staging_buffer_directive(NOCTX,
				selectedmedia, &bufdir);
	if (-1 == res)
		showerr();
	else {
		int maxbuf = 20;
		char buf[maxbuf];
		ShowBanner();
		ln = 2;
		dis_stgmenu();
		ln += 2;
		mvprintw(ln++, 3, "Media type: %s", selectedmedia);
		mvprintw(ln, 3, "Bufize: ");
		strcpy(buf, fsize_to_str(bufdir->size, fsizestr, FSIZE_STR_LEN));
		/* get input from user */
		if (-1 == str_to_fsize(getstring(buf, maxbuf),
		    &bufdir->size)) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			char quest[20];
			ln++;
			bufdir->change_flag = BD_size | BD_lock;
			sprintf(quest, "Lock buffer? %s",
			    (bufdir->lock ? "[Y/n]" : "[y/N]"));
			if (askw(ln++, 3, quest, bufdir->lock ? 'y' : 'n'))
				bufdir->lock = B_TRUE;
			else
				bufdir->lock = B_FALSE;

			/* write changes */
			if (-1 == set_buffer_directive(NOCTX, bufdir))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	free(bufdir);
	TRACE("samc.c:bufsize() exit");
	return (B_FALSE);
}

static boolean_t
maxactive() {
	stager_cfg_t *stg = NULL;
	TRACE("samc.c:maxactive() entry");
	if (-1 == get_stager_cfg(NOCTX, &stg))
		showerr();
	else {
		ln += 2;
		mvprintw(ln, 3, "Maxactive: ", stg->max_active);
		/* get input from user */
		if (-1 == (stg->max_active = getint(&stg->max_active))) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			ln++;
			stg->change_flag = ST_max_active;
			/* write changes */
			if (-1 == set_stager_cfg(NOCTX, stg))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	free_stager_cfg(stg);
	TRACE("samc.c:maxactive() exit");
	return (B_FALSE);
}

static boolean_t
stglog() {
	stager_cfg_t *stg = NULL;
	TRACE("samc.c:stglog() entry");
	if (-1 == get_stager_cfg(NOCTX, &stg))
		showerr();
	else {
		ln += 2;
		mvprintw(ln, 3, "Logfile: ");
		/* get input from user */
		if (NULL == getstring(stg->stage_log, MAX_PATH_LENGTH)) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			ln++;
			stg->change_flag = ST_stage_log;
			/* write changes */
			if (-1 == set_stager_cfg(NOCTX, stg))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	free_stager_cfg(stg);
	TRACE("samc.c:stglog() exit");
	return (B_FALSE);
}

// ------------------------- 'Releasing' Menu -------------------------
static boolean_t
release() {
	FORM *form;
	char formtitle[40];
	rl_fs_directive_t *relfsdir, *dfl_relfsdir;

	clear();
	ShowBanner();
	/* choose fs */
	if (B_FALSE == choosefs(B_FALSE))
		return (B_FALSE);

	/* init default values */
	if (-1 == get_default_rl_fs_directive(NOCTX, &dfl_relfsdir)) {
		showerr();
		anykey(ln, 3);
		return (B_FALSE);
	}
	init_field_recs_dfl(rel_FORM, dfl_relfsdir);


	/* init values */
	if (-1 == get_rl_fs_directive(NOCTX, selectedfs, &relfsdir))
		if (samerrno == SE_NO_RL_FS_FOUND)
			relfsdir = dfl_relfsdir;
		else {
			showerr();
			anykey(ln, 3);
			free(dfl_relfsdir);
			home();
			return (B_FALSE);
		}
	else
		free(dfl_relfsdir);
	TRACE("samc.c:crt.age_prio_type:%d", relfsdir->type);
	init_field_recs_val(rel_FORM, relfsdir);
	if (relfsdir->type == DETAILED_AGE_PRIO) {
		TRACE("samc.c:detailed prio enabled");
		rel_FORM[17].val = "on"; /* detailed prio is on */
		rel_FORM[15].val = NULL;
	} else {
		TRACE("samc.c:detalied prio disabled");
		rel_FORM[17].val = "off"; /* detailed prio is off */
		rel_FORM[18].mkfield = mkhlabel;
		rel_FORM[19].mkfield = mkhlabel;
		rel_FORM[20].mkfield = mkhlabel;
		rel_FORM[21].mkfield = mkhdbl;
		rel_FORM[22].mkfield = mkhdbl;
		rel_FORM[23].mkfield = mkhdbl;
		rel_FORM[21].val = NULL;
		rel_FORM[22].val = NULL;
		rel_FORM[23].val = NULL;
	}

	/* get user input */
	if (! (form = new_form(mkfields(rel_FORM))))
		TRACE("samc.c: error creating form rel_FORM");
	display_form(form);
	sprintf(formtitle, "Releaser configuration for %s", selectedfs);
	if (-1 == process_form(form, formtitle)) {
		free(relfsdir);
		return (B_FALSE);
	}
	/*
	 * Test dont set to zero. Try to omit merge.
	 * relfsdir->change_flag = 0;
	 */
	convert_form_data(form, (char *)relfsdir);
	/* write changes */
	if (-1 == set_rl_fs_directive(NOCTX, relfsdir))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);
	/* reset title */
	home();
	free(relfsdir);
	/* do not attempt to further process this menu item */
	return (B_FALSE);
}
// ------------------------- 'Recycling' Menu -------------------------

static boolean_t
rbrecycle() {
	FORM *form;
	char formtitle[80];
	rc_robot_cfg_t *rbcfg;
	rc_param_t *rbparams;

	TRACE("samc.c:rbrecycle()");
	clear();
	ShowBanner();
	/* choose fs */
	if (B_FALSE == chooserb())
		return (B_FALSE);

	/* initialize defaults */
	get_default_rc_params(NOCTX, &rbparams);
	init_field_recs_dfl(rbrecycle_FORM, rbparams);

	/* initialize values */
	if (-1 == get_rc_robot_cfg(NOCTX, selectedrb, &rbcfg))
		if (samerrno == SE_NO_ROBOT_CFG_FOUND) {
			rbcfg = (rc_robot_cfg_t *)
			    malloc(sizeof (rc_robot_cfg_t));
			strcpy(rbcfg->robot_name, selectedrb);
			reset_field_recs_val(rbrecycle_FORM,
			    numof(rbrecycle_FORM));
			TRACE("samc.c:rbrecycle_FORM reset (rb:%s)",
			    rbcfg->robot_name);
		} else {
			showerr();
			anykey(ln++, 3);
			TRACE("samc.c:rbrecycle() exit");
			return (B_FALSE);
		} else {
			init_field_recs_val(rbrecycle_FORM, &rbcfg->rc_params);
			TRACE("samc.c:rbrecycle_FORM initialized");
	}

	if (! (form = new_form(mkfields(rbrecycle_FORM)))) {
		TRACE("samc.c: error creating form rbrecycle_FORM");
		TRACE("samc.c:rbrecycle() exit");
		return (B_FALSE);
	}
	TRACE("samc.c:displaying form");
	display_form(form);
	sprintf(formtitle, "Recycler configuration for robot %s", selectedrb);

	/* get user input */
	if (-1 == process_form(form, formtitle)) {
		TRACE("samc.c:rbrecycle() exit");
		return (B_FALSE);
	}
	convert_form_data(form, (char *)&rbcfg->rc_params);
	/* write changes */
	if (-1 == set_rc_robot_cfg(NOCTX, rbcfg))
		showerr();
	else
		mvprintw(ln++, 3, CFGUPD_MSG);
	anykey(ln++, 3);

	/* do not attempt to further process this menu item */
	TRACE("samc.c:rbrecycle() exit");
	return (B_FALSE);
}

char *
norcvsns2str(no_rc_vsns_t *nrvsns) {
	static char buf[200];
	sprintf(buf, "%-5s %s",
	    nrvsns->media_type, (char *)lst2str(nrvsns->vsn_exp, " "));
	return (buf);
}
/*
 * NOT_CURRENTLY_USED
 * pass back all VSNSs that are not recycled (per media type)
 * as a NULL terminated array of ITEMs.
 * return the length of the array or -1 if error.
 */
/* static int */
/* get_norecvsns_items(ITEM **items) { */
/* 	int crtit = 0; */
/* 	sqm_lst_t *nrvsns; */
/* 	node_t *node; */

/* 	TRACE("samc.c:get_norecvsns_items() entry"); */
/* 	if (-1 == get_all_no_rc_vsns(RD_DISK, &nrvsns)) { */
/* 		mvprintw(ln++, 3, */
/*		"Cannot obtain no_recycle VSN information:"); */
/* 		showerr(); */
/* 		refresh(); */
/* 		return (-1); */
/* 	} */
/* 	TRACE("samc.c:get_all_no_rc_vsns() done"); */
/* 	if (!nrvsns) { */
/* 		mvprintw(ln++, 3, "No no_recycle VSNs found!"); */
/* 		refresh(); */
/* 		anykey(ln++, 3); */
/* 		return (0); */
/* 	} */
/* 	node = nrvsns->head; */
/* 	while (node) { */
/* 		items[crtit++] = new_item( */
/* 			norcvsns2str((no_rc_vsns_t *)node->data), ""); */
/* 		node = node->next; */
/* 	} */
/* 	items[crtit] = (ITEM *) 0; */
/* 	TRACE("samc.c:get_nrecvsns_items() exit"); */
/* 	return (crtit); */
/* } */

static boolean_t
norecycle() {
	int res,
	    opn;	/* 0=add, 1=modify */
	no_rc_vsns_t *nrvsns;
	char vsnexpr[MAX_VSNEXPRLEN] = "";

	TRACE("samc.c:norecycle()");
	clear();
	ShowBanner();

	/* choose media type */
	if (!choosemedia())
		return (B_FALSE);

	/* get current value for selected media */
	if (-1 == get_no_rc_vsns(NOCTX, selectedmedia, &nrvsns))
		if (samerrno == SE_NO_NO_RECYCLE_VSN_FOUND) {
			opn = 0;
			nrvsns = (no_rc_vsns_t *)malloc(sizeof (no_rc_vsns_t));
			strcpy(nrvsns->media_type, selectedmedia);
		} else {
			showerr();
			anykey(ln++, 3);
			return (B_FALSE);
		} else {
			TRACE("samc.c:get_no_rc_vsns(%s,...) done",
			    selectedmedia);
			opn = 1;
			strcpy(vsnexpr, (char *)lst2str(nrvsns->vsn_exp, " "));
		}

	/* get input from user */
	mvprintw(ln, 3, "Selected media type: %s", nrvsns->media_type);
	ln += 2;
	mvprintw(ln, 3, "VSNs not recycled: ");
	if (NULL == getstring(vsnexpr, MAX_VSNEXPRLEN)) {
		ln += 2;
		mvprintw(ln++, 3, OPABAND_MSG);
	} else {
		ln += 2;
		nrvsns->vsn_exp = (sqm_lst_t *)str2lst(vsnexpr, " ");
		/* write changes */
		if (0 == opn)
			res = add_no_rc_vsns(NOCTX, nrvsns);
		else
			res = modify_no_rc_vsns(NOCTX, nrvsns);
		if (-1 == res)
			showerr();
		else
			mvprintw(ln++, 3, CFGUPD_MSG);
	}
	anykey(ln++, 3);
	free_no_rc_vsns(nrvsns);
	TRACE("samc.c:norecycle() exit");
	return (B_FALSE);
}

static boolean_t
reclog() {
	upath_t log, dfllog;
	boolean_t nologfound = B_FALSE;
	TRACE("samc.c:stglog() entry");
	if (-1 == get_rc_log(NOCTX, log))
		nologfound = B_TRUE;
	else {
		ln += 2;
		if (-1 == get_default_rc_log(NOCTX, dfllog)) {
			mvprintw(ln++, 3,
			    "warning: cannot obtain default log location");
			mvprintw(ln++, 3, "(%s)", samerrmsg);
		} else
			if (nologfound)
				strcpy(log, dfllog);
		mvprintw(ln, 3, "Logfile: ");
		/* get input from user */
		if (NULL == getstring(log, MAX_PATH_LENGTH)) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			ln++;
			/* write changes */
			if (-1 == set_rc_log(NOCTX, log))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	TRACE("samc.c:reclog() exit");
	return (B_FALSE);
}

static boolean_t
recscript() {
	upath_t script, dflscript;
	boolean_t noscriptfound = B_FALSE;
	TRACE("samc.c:recscript() entry");
	if (-1 == get_rc_notify_script(NOCTX, script))
		noscriptfound = B_TRUE;
	else {
		ln += 2;
		if (-1 == get_default_rc_notify_script(NOCTX, dflscript)) {
			mvprintw(ln++, 3,
			    "warning: cannot obtain default script location");
			mvprintw(ln++, 3, "(%s)", samerrmsg);
		} else
			if (noscriptfound)
				strcpy(script, dflscript);
		mvprintw(ln, 3, "Script: ");
		/* get input from user */
		if (NULL == getstring(script, MAX_PATH_LENGTH)) {
			ln++;
			mvprintw(ln++, 3, OPABAND_MSG);
		} else {
			ln++;
			/* write changes */
			if (-1 == set_rc_notify_script(NOCTX, script))
				showerr();
			else
				mvprintw(ln++, 3, CFGUPD_MSG);
		}
	}
	anykey(ln++, 3);
	TRACE("samc.c:recscript() exit");
	return (B_FALSE);
}

/* static boolean_t */
/* globrec() { */
/* 	FORM *form; */
/* 	int res; */
/* 	clear(); */
/* 	ShowBanner(); */
/* 	if (! (form = new_form(mkfields(globrec_FORM)))) */
/* 		TRACE("samc.c: error creating form globrec_FORM"); */
/* 	display_form(form); */
/* 	res = process_form(form, ""); */
/* 	return (B_FALSE); */
/* } */

// --------------------- 'Removable Media' Menu ------------------------

char *
drv2str(const drive_t *drv, const char *path) {
	static char str[80];
	if (!drv)
		return (NULL);
	// TRACE("samc.c: drv2str()");
	sprintf(str, "%-30s %-15s %-10s %-10s",
	    path,
	    drv->serial_no,
	    drv->vendor_id,
	    drv->product_id);
	TRACE("samc.c: drv2str(): drvstr=%s.", str);
	return (str);
}

/*
 * count the number of paths (including links) to all the drives in the
 * specified library
 */
static int
drvpaths(library_t *lib) {
	int count = 0;
	node_t *node;
	if (!lib)
		return (0);
	node = lib->drive_list->head;
	while (node) {
		count += ((drive_t *)node->data)->alternate_paths_list->length;
		node = node->next;
	}
	TRACE("samc.c:drvpaths=%d", count);
	return (count);
}

/*
 * set the base_info.name (the path) for each drive, since multiple paths
 * to the same drive can exist.
 * may also remove some drives from the list.
 */
static void
chooselibdrv(sqm_lst_t *drvlst, const int ndrvpaths) {
	ITEM *items[ndrvpaths + 1];
	node_t *node, *pathnode, *nxt;
	drive_t *drv;
	int crt;
	menu_sel_t menu_sel;

	TRACE("samc.c: chooselibdrv(lst[%d],%d)", drvlst->length, ndrvpaths);
	node = drvlst->head;
	for (crt = 0; node; node = node->next) {
		TRACE("samc.c: crt=%d", crt);
		if (!node->data)
			TRACE("samc.c: no drive info in node!");
		drv = (drive_t *)node->data;
		if (!drv->alternate_paths_list)
			TRACE("samc.c: NULL alt.path.lst.");
		TRACE("samc.c: lstlen=%d", drv->alternate_paths_list->length);
		pathnode = drv->alternate_paths_list->head;
		while (pathnode) {
			TRACE("samc.c: crt=%d", crt);
			if (pathnode->data == NULL)
				TRACE("pathnode is NULL");
			items[crt++] = new_item((char *)
				strdup(drv2str(drv, (char *)pathnode->data)),
				"");
			set_item_userptr(items[crt - 1], (char *)drv);
			pathnode = pathnode->next;
		}
	}
	items[crt] = NULL;
	menu_sel = choosefrommenu(items,
	    "Step 2/3: Select drives from the list below", B_TRUE);

	TRACE("samc.c: setting drv->base_info.name");
	/* now set the base_info.name field for all selected drives */
	node = menu_sel.multisel->head;
	while (node) {
		char *selectedpath
		    = getfirststr(((item_info_t *)node->data)->str);
		drv = (drive_t *)((item_info_t *)node->data)->usrptr;
		strcpy(drv->base_info.name, selectedpath);
//		free(selectedpath);
		node = node->next;
	}
	TRACE("samc.c: removing unselected drives from drvlst");
	/* remove the unselected drives from list */
	node = drvlst->head;
	while (node) {
		nxt = node->next;
		drv = (drive_t *)node->data;
		if (!strlen(drv->base_info.name)) {
			TRACE("samc.c: removing lib drive from drvlst");
			/* remove from list */
			lst_remove(drvlst, node);
			/* deallocate space */
			free_drive(drv);
		}
		node = nxt;
	}
	TRACE("samc.c: chooselibdrv() done; %d drives selected",
	    drvlst->length);
}

static void
setdrvfsetnames(library_t *lib) {
	node_t *node = lib->drive_list->head;
	drive_t *drv;
	while (node) {
		drv = (drive_t *)node->data;
//		strcpy(drv->base_info.set, lib->base_info.set);
		drv->base_info.state = DEV_ON;
		drv->base_info.eq = 0;
		drv->base_info.additional_params[0] = '\0';

		TRACE("samc.c:drv.path  =%s.", Str(drv->base_info.name));
		TRACE("samc.c:drv.fsname=%s.", Str(drv->base_info.set));
		TRACE("samc.c:drv.equ#  =%d.", drv->base_info.eq);
		TRACE("samc.c:drv.equtyp=%s.", Str(drv->base_info.equ_type));
		TRACE("samc.c:drv.fseq# =%d.", drv->base_info.fseq);
		TRACE("samc.c:drv.params=%s.",
		    Str(drv->base_info.additional_params));
		node = node->next;
	}
}

static void
addlibform(library_t *lib) {
	FORM *form;
	char formtitle[80];
	TRACE("samc.c: addlibform(%s...)", lib->base_info.name);
	clear();
	ShowBanner();

	/* set the value of the media type field to the discovered type */
	lib_FORM[6].val = lib->base_info.equ_type;
	if (!(form = new_form(mkfields(lib_FORM))))
		TRACE("samc.c: error creating form lib_FORM");
	display_form(form);
	sprintf(formtitle, "Step 3/3: Settings for library %s",
	    selectedrb);
	strcpy(lib->base_info.name, selectedrb);	/* set path */

	/* edit parameters */
	if (-1 == process_form(form, formtitle)) {
		showerr();
		anykey(ln, 3);
		destroy_form(form);
		home();
		return;
	}

	convert_form_data(form, (char *)lib);
	lib->base_info.state = DEV_ON;
	TRACE("samc.c:lib.path  =%s.", lib->base_info.name);
	TRACE("samc.c:lib.fsname=%s.", lib->base_info.set);
	TRACE("samc.c:lib.equ#  =%d.", lib->base_info.eq);
	TRACE("samc.c:lib.equtyp=%s.", lib->base_info.equ_type);
	TRACE("samc.c:lib.fseq# =%d.", lib->base_info.fseq);
	TRACE("samc.c:lib.params=%s.", lib->base_info.additional_params);


	/* temporary. the API will implement this functionality */
	setdrvfsetnames(lib);

	ln++;
	TRACE("samc.c:add_lib: setname:%s equ:%u",
	    lib->base_info.set, lib->base_info.eq);
	if (-1 == add_library(NOCTX, lib))
		showerr();
	else
		mvprintw(ln++, 5, CFGUPD_MSG);
	ln++;
	anykey(ln, 5);

	destroy_form(form);

	/* reset title */
	home();
	TRACE("samc.c: addlibform() exit");
}

static char *
lib2str(const library_t *lib, const char *path) {
	static char str[80];
	if (!lib)
		return (NULL);
	TRACE("samc.c: lib2str()");
	sprintf(str, "%-30s %-10s %-10s (%d drives)",
	    path,
	    lib->vendor_id,
	    lib->product_id,
	    lib->no_of_drives);
	TRACE("samc.c: lib2str=%s.", str);
	return (str);
}

/*
 * pass back discovered media as a NULL terminated array of ITEMs.
 * return the length of the array or -1 if error.
 */
static int
get_media_items(ITEM **items,
		boolean_t libs,		/* B_TRUE/B_FALSE = lib/drive disco */
		sqm_lst_t **medialst) {	/* must be deallocated later */

	int crt = 0;
	sqm_lst_t *liblst,
	    *drvlst;
	library_t *lib;
	drive_t *drv;
	node_t *node, *pathnode;

	TRACE("samc.c:get_media_items() entry");
	ln += 2;
	mvprintw(ln++, 5, GATHERMEDIAINFO_MSG);
	refresh();
	if (-1 == discover_media(NOCTX, &liblst)) {
		showerr();
		anykey(ln, 5);
		return (B_FALSE);
	}
	TRACE("samc.c:discover_media done()");
	TRACE("samc.c:libs=%d drvs=%d",
	    liblst ? liblst->length : -1,
	    drvlst ? drvlst->length : -1);
	if (libs) {
		free_list_of_drives(drvlst);	/* do not need these */
		*medialst = liblst;		/* needs to be dealloc. later */
		if (!liblst || !liblst->length) {
			slog("samc.c: no libraries found");
			mvprintw(++ln, 5, "No libraries found");
			anykey(++ln, 5);
			return (B_FALSE);
		}
	} else {
		free_list_of_libraries(liblst); /* dont need these */
		*medialst = drvlst;		/* needs to be dealloc. later */
		if (!drvlst || !drvlst->length) {
			slog("samc.c: no drives found");
			mvprintw(++ln, 5, "No drives found");
			anykey(++ln, 5);
			return (B_FALSE);
		}
	}

	node = libs ? liblst->head : drvlst->head;
	for (crt = 0; node; node = node->next) {
		TRACE("samc.c: crt=%d", crt);
		if (!node->data)
			TRACE("samc.c: no media info in node!");
		if (libs) {
		    lib = (library_t *)node->data;
		    pathnode = lib->alternate_paths_list->head;
		} else {
		    drv = (drive_t *)node->data;
		    pathnode = drv->alternate_paths_list->head;
		}
		while (pathnode) {
			items[crt++] = new_item(libs ?
			    (char *)
			    strdup(lib2str(lib, (char *)pathnode->data))
			    :
			    (char *)
			    strdup(drv2str(drv, (char *)pathnode->data)), "");
			set_item_userptr(items[crt - 1],
			    libs ? (char *)lib : (char *)drv);
			pathnode = pathnode->next;
		}
	}
	items[crt] = NULL;
	TRACE("samc.c:get_media_items() exit");
	return (crt);
}

static boolean_t
addlib() {
	menu_sel_t menu_sel;
	ITEM *items[MAX_LIBS];

	library_t *selectedlib;
	sqm_lst_t *liblst;

	if (0 <= get_media_items(items, B_TRUE, &liblst))
		return (B_FALSE);
	menu_sel = choosefrommenu(items, "Step 1/3: Choose a library",
	    B_FALSE);
	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}

	strcpy(selectedrb, getfirststr(menu_sel.singlsel.str));
	TRACE("samc.c: library %s selected", selectedrb);
	selectedlib = (library_t *)menu_sel.singlsel.usrptr;

	/* now select the library drives to be added */
	chooselibdrv(selectedlib->drive_list, drvpaths(selectedlib));

	if (selectedlib->drive_list->length != 0) {
		/* display library form, get input from user and add library */
		addlibform(selectedlib);
	}

	free_list_of_libraries(liblst);
	return (B_FALSE);
}

// ---------------- standalone drives
static boolean_t
choosesdrv(ITEM **items, drive_t **drv) {
	menu_sel_t menu_sel
	    = choosefrommenu(items, "Choose a drive", B_FALSE);

	if (!menu_sel.singlsel.str) {
		home();
		return (B_FALSE);
	}
	strcpy(selecteddrv, getfirststr(menu_sel.singlsel.str));
	*drv = (drive_t *)menu_sel.singlsel.usrptr;
	/* set the name field to match the selected path */
	strcpy((*drv)->base_info.name, selecteddrv);
	/* set the value of the media type field to the discovered type */
	sdrive_FORM[5].val = (*drv)->base_info.equ_type;
	return (B_TRUE);
}


static boolean_t
addsdrv() {
	FORM *form;
	ITEM *items[MAX_STDRIVES];
	char formtitle[80];
	sqm_lst_t *drvs;
	drive_t *drv;

	TRACE("samc.c:addsdrv()");
	clear();
	ShowBanner();

	/* discover installed media */
	if (-1 == get_media_items(items, B_FALSE, &drvs)) {
		showerr();
		return (B_FALSE);
	}

	/* choose drive */
	if (B_FALSE == choosesdrv(items, &drv))
		return (B_FALSE);

	if (! (form = new_form(mkfields(sdrive_FORM))))
		TRACE("samc.c:error creating form lib_FORM");
	/* get user input */
	display_form(form);
	sprintf(formtitle, "Add new standalone drive - %s", selecteddrv);
	if (-1 == process_form(form, formtitle)) {
		mvprintw(ln++, 3, OPABAND_MSG);
		anykey(ln++, 3);
	} else {
		convert_form_data(form, (char *)drv);
		/* write changes */
/*		if (-1 == add_standalone_drive(NOCTX, drv)) */
			showerr();
/*		else
			mvprintw(ln++, 3, CFGUPD_MSG);
*/		anykey(ln++, 3);
	}
	free_list_of_drives(drvs);
	TRACE("samc.c:addsdrv() exit");
	return (B_FALSE);
}

/* static void */
/* testgetline() { */
/* 	char s[20]; */
/* 	nocbreak(); */
/* 	while (1) { */
/* 		ln++; */
/* 		mvprintw(ln, 5, "Enter string:"); */
/* 		getstring(s, 20); */
/* 		ln++; */
/* 		mvprintw(ln, 5, "Enter int:"); */
/* 		getint(NULL); */
/* 	} */
/* } */

// ------------ view a list of running SAM daemons -----------------
boolean_t
viewdaemons() {
	sqm_lst_t *daemons;
	node_t *node;
	clear();
	ShowBanner();
	ln = 2;
	if (-1 == get_running_sam_daemons(&daemons))
		mvprintw(ln++, 3, "Cannot get daemon info");
	else {
		int col = 0;
		mvprintw(ln, 3, "Active SAM-FS daemons");
		ln += 2;
		node = daemons->head;
		while (node) {
			if (ln == LINES - 2) {
				ln = 4;
				col++;
			}
			mvprintw(ln++, 3 + col * 30, "%s", (char *)node->data);
			node = node->next;
		}
		lst_free_deep(daemons);
	}
	anykey(LINES - 2, 3);
	home();
	return (B_FALSE);
}

// ------------------------------- main -----------------------------
int
main(int argc, char ** argv) {
	char c;
	ctx_t ctx;
        char *samrpc_host = "localhost"; /* SAMRPC_HOST */

	if (NULL == (program_name = (char *)mallocer(20))) {
		printf("%s\n", samerrmsg);
		exit(1);
	}
	sprintf(program_name, "%s %s",
	    (char *)basename(argv[0]), get_samfs_lib_version(NOCTX));

	TRACE("ver:%s", getVersion());
	LICENSE = get_samfs_type(NOCTX); // = SAMFS; testing
	slog("samc.c: SAM/QFS license=%d", LICENSE);
	if (-1 == LICENSE) {
		printf("\n" NOLICENSE_MSG "\n");
		exit(-1);
	}

	fflush(NULL);

	ctx.dump_path[0] = '\0';
	ctx.read_location[0] = '\0';
	ctx.user_id[0] = '\0';
	ctx.handle = samrpc_create_clnt_timed(samrpc_host, DEF_TIMEOUT_SEC);
	if (-1 == init_sam_mgmt(&ctx)) {
		printf("\n"CANNOTINITLIB_MSG"\n", samerrmsg);
		exit(-1);
	}

	initscr();	    /* Initialize curses */
	cbreak();
	keypad(stdscr, TRUE);
	row = LINES;
	col = COLS;
	signal(SIGWINCH, CatchWinch);

	atexit(Atexit);
	TRACE("sam.c:curses lib initialized");
	noecho();
	halfdelay(10);
	disp = &toplev_dis;
	dis_size = numof(toplev_dis);
	title = SAMC_TITLE;
	SetDisplay(initdis);
	refresh();

	do {
		int ret;

		if (row != LINES || col != COLS)
			reinitscr();

		erase();
		ShowBanner();

		ln = 3;
		crt_menu.dis(); /* The actual display. */
		refresh();

		/*
		 * Read character from terminal.
		 */
		c = getch();
		if (c == ERR)  continue;
		TRACE("samc.c:%c typed", c);

		switch (c) {

		case 'q':   /* quit */
			TRACE("samc.c:main() exit");
			break;

		case 'h':	/* home */
			home();
			break;

		case '?':
			help(); /* display help screen */
			break;

		default:
			/*
			 * Try setting display from key.
			 */
			if ((ret = SetDisplay(c)) >= 0) {
				crt_menu = (*disp)[dis_n];
				title = crt_menu.ntitle_msg;
				TRACE("samc.c:changing to menu %d:%s...",
				    dis_n, title);
				disp = (*disp)[dis_n].childp;

				// TRACE("disp changed to %s", title);
				continue;
			}
			if (ret == -1)
				beep();
			break;
		} // switch
	} while (c != 'q');
	return (0);
}
