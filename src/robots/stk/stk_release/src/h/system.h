/* SccsId @(#)system.h	1.2 2/9/94  */
#ifndef _SYSTEM_
#define _SYSTEM_
/*
 * Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *	This include file includes prototype functions for system
 *      function calls for which there are no prototypes provided by
 *      the compiler vendor.
 *
 *
 * Modified by:
 *
 *      Alec Sharp          27-Apr-1993	    Original
 *	David Farmer	    16-Aug-1993	    added timezone
 *	Emanuel Alongi	    28-Sep-1993	    added sigaction() and sigemptyset().
 *	Emanuel Alongi	    04-Oct-1993	    added inet_ntoa().
 *	Emanuel Alongi	    12-Nov-1993	    added get_hostname().
 *	Emanuel Alongi	    09-Feb-1994	    removed inet_ntoa() prototype.
 */


/* IMPORTANT: Most of the parameters that are "void *" should really
   be something else but I have made them void * to avoid (sic) having
   to include other include files prior to this one.  For instance,
   sigaction(int, struct sigaction *, struct sigaction *) was changed to
   sigaction(int, void *, void *) so signal.h need not be included.
*/


#if defined (sun) && !defined (SOLARIS)

/* Prototypes that don't exist even as extern declarations */

char 		*mktemp(char *templt);


#if defined (_lint)

#define	toascii(c)	((c) & 0177)

extern void * __builtin_va_alist;  /* This is built-in to the compiler and
				      lint complains unless we define it here */


unsigned int	alarm(unsigned int seconds);
int		bind(int, void *, int);
int		close(int);
int		creat (char *, int);
char *		cuserid(char *);
double		drand48(void);
int		execl(char *, char *, ...);
int		execlp(char *, char *, ...);
int		execv(char *, char *[]);
int		fcntl(int, int, void *);
int		flock(int, int);
int 		fork(void);
int		fstat(int, void *);
int		ftime(void *);
int		ftok(char *, char);
int		getdtablesize(void);
unsigned short  getegid(void);
unsigned short	geteuid(void);
struct hostent *gethostbyname(char *);
int 		gethostname(char *, int);
int		getopt(int, char**, char*);
int 		getpid(void);
int		getrusage(int, void *);
int		gettimeofday(void *, void *);
int		ioctl(int, int, void *);
int		isatty(int);
int		kill (int, int);
void		longjmp(void *, int);
long		lrand48(void);
long		lseek(int, int, int);
int		open(const char *, int, ...);
int		pause(void);
void            pclose (void *);
int		read(int, void *, int);
int		scandir(char *, void *, int (*)(), int (*)());
int		semctl(int, int, int, ...); /* ... to avoid union problem */
int		semget(int, int, int);
int		semop(int, void *, int);
int		select (int, void*, void*, void*, void*);
int		setuid(int);
int		sendto(int, char *, int, int, void *, int);
int		setgid(unsigned short);
int		setjmp(void *);
int		setpgrp(int, int);
int		setreuid(int, int);
char *		shmat(int, void *, int);
int		shmctl(int, int, void *);
int		shmdt(char *);
int		shmget(int, int, int);
int		sigaction(int, void *, void *);
int             sigemptyset(int *); 
int  		siginterrupt (int sig, int flag);
int		sigvec (int, void *, void *);
unsigned int	sleep (unsigned int seconds);
int		socket (int, int, int);
void		srand48(long seedval);
int		stat(char *, void *);
int		strcasecmp(const char *, const char *);
int		strncasecmp(const char *, const char *, int);
int		stty (int, void *);
char *		timezone(int, int);
unsigned short	umask(unsigned short);
int		unlink(char *);
int		wait(int *);
int             waitpid(int, int *, int);
int		wait3(int *, int, void *);
int		wclrtoeol(void *);
int		write (int, void *, int);


/* curses stuff */
int		delwin(void *);
int		endwin(void);
int		setlinebuf(void *);
int		waddch(void *, char);
int		waddstr(void *, char *);
int		wdelch(void *, char);
int		wgetch(void *);
int		wmove(void *, int, int);
int		wprintw(void *, char *, ...);
int		wrefresh (void *);
int		wclear(void *);

#else  /* _lint */

char *		shmat(int, void *, int);

#endif /* _lint */

#endif /* sun */

#endif /* _SYSTEM_ */
