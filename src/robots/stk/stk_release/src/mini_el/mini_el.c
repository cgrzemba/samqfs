/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      mini_el.c
 *
 * Description:
 *
 *      This is a MINI event logger which will function like the 
 *	'acsel' shipped with the ACSLS product.  Since the toolkit 
 * 	ships source, it was decided to provide a simplier 
 *	event-logger to the client application writers, which 
 *	simply showed how we got the event log data from the 
 *	appropriate sockets.
 *
 * 	Notes: In Release 0 through R4 (inclusive) of ACSLS, The IPC
 *	Header remained the same.  In R5, the RPC_header grew with a
 *	host identifier.  
 *
 * Synopsys:
 *
 * 	Usage: 
 *		mini_el [-d] [-h] [-v]
 *
 *	Where:
 *		-d	Display Debug Messages (no debug by default)
 *		-h	Display the usage help message
 *		-v	This is a switch which will say "use down level 
 *			packets (Packet Version level 0 through 3)"
 *			It is optional and if not included, will default to 
 *			Packet version level 4 (the latest).
 *
 * Considerations: 
 *
 *	This depends on REAL UNIX-Domain sockets... if the CSI/SSI
 *	is compiled with '#define IPC_SHARED' this mechanism
 *	will NOT work.
 *
 * Implicit Inputs:
 *
 *      Data coming from the sockets.
 *
 * Implicit Outputs:
 *
 *	None.
 *
 * Compilation:
 *
 * 	This module was compiled using the sparkworks Ansi C compiler:
 *	The resulting executable was called mini_el, and it depends only
 *	on the include files shipped with the toolkit and only the standard
 *	Ansi-C Library.
 *
 *	The compilation command was: 
 *
 *	acc -I../h mini_el.c -o mini_el
 *
 *
 * Revision History:
 *
 *      H. I. Grapek	13-Dev-1993     Original.
 *	K. J. Stickney	20-Dec-1993	Added -d option
 *	H. I. Grapek	03-Jan-1994	Code Review Changes
 *      K. J. Stickney  24-Jan-1994     Added prototype for getopt
 *                                      and changed "c" to int from char.
 *                                      Also cast c to char in switch.
 *      K. J. Stickney  19-May-1994     event.log/trace.log now opened
 *                                      for append with reception of each
 *                                      message. BR#30.
 *      K. J. Stickney  17-Aug-1994     event.log/trace.log now closed
 *                                      within in the code loop that
 *                                      calls open for append with reception 
 *                                      of each message. BR#40.
 *      K. J. Stickney  23-Dec-1994     Changed computation of sockaddr_in
 *                                      and increased queue length for 
 *                                      listen() call. For solaris port.
 *      S. L. Siao      05-Mar-2002     Added memset in ElInOut to clear
 *                                      buffer after writing.
 *      Anton Vatcky    01-Mar-2002     Removed prototype of getopt for SOLARIS
 *                                      and AIX. Set correct type of nsize and
 *                                      name_len for AIX and Solaris. Fixed up
 *      S. L. Siao      18-Apr-2002     Portablility change of define for 
 *                                      name_len.
 *	Hemendra	06-Jan-2003	Removed prototype for getopt for Linux.
 *					Also added return statement at the end of
 *					function ELInOut(). Solaris and Linux shares 
 *					similar listen functiona call.
 *	Mitch Black	06-Dec-2004	Fixed a bad comment delimiter which had 
 *					a space between the asterisk and slash.
 *      Mitch Black     08-Dec-2004     Changed port from an unsigned short
 *                                      to an int, and cast in the htons() call.
 *                                      Otherwise the check for port < 0 was
 *                                      a possible bug source (it was superfluous).
 *	Mitch Black	28-Dec-2004	Fixed an #elif LINUX which broke SOLARIS
 *					compilation.
 *	Mitch Black	23-Feb-2005	Added include <errno.h> for later releases
 *					of RedHat Linux.  Changed the listen queue 
 *					to be defined by SOMAXCONN for all OSs.
 *	Mitch Black	28-Feb-2005	Changed listen queue, since some OS's
 *					define SOMAXCONN for applications to be 
 *					lower than the OS really supports (AIX...)
 *      Mike Williams   04-May-2010     Changed nsize to type socklen_t for
 *                                      64-bit compiles. Added unistd.h include.
 */

/*
 *      Header Files:
 */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#ifdef LINUX
#include <time.h>
#include <errno.h>
#endif

#include "structs.h"			/* ACSLS ISMs. */

/* 
 * Functional Prototypes for Internal Modules 
 */
static void MsgOutput(FILE *fp, char *cp_message);
static void ExitCleanly();
static int  OpenSocket(char *sock_name_in, char *sock_name_out);
static int  ELInit();
static int  ELInOut();
int         main(int argc, char *argv[]);
#ifdef SOLARIS
#elif AIX
#elif LINUX
#else
int         getopt(int, char**, char*);
#endif

/* Return Values */

#define SOCK_CREATE_ERR -1		/* error creating a socket */
#define SOCK_BIND_ERR   -2		/* error binding to socket */
#define SOCK_OPT_ERR    -3		/* error from setsockopt */
#define SOCK_NAME_ERR   -4		/* error from getsockname */
#define EL_IPC_FAILURE	-5		/* general ipc failure */
#define PORT_ATOI_ERR	-6		/* atoi failed on the port */
#define EL_TIMEDOUT	-7		/* timed out on the select */
#define EL_NO_FD	-8		/* After select, no fds left */
#define EL_SUCCESS	 0		/* all is good */

/* Assorted Definitions */

#define TRUE 1
#define FALSE 0

#define BUFSIZE  2048		/* buffer read length */
#define PORT 	"50001"		/* default port number to listen to */

#define ELOG	"event.log"	/* name of the event log */
#define TLOG	"trace.log"	/* name of the trace log */

/* 
 * globals/statics/externs
 */
int                listen_sock;		/* the socket to listen to for 
					 * messages.  This is the Same 
					 * as the ACSLS global: sd_in */
int                Debug = FALSE;	/* debug output printing (cmd-line) */
int 		   PLVersion = 4;	/* Packet Level Version */
FILE              *efilep;		/* event log file pointer */
FILE              *tfilep;		/* trace log file pointer */
extern int         errno;		/* UNIX error number lists */
EVENT_LOG_MESSAGE  el_bfr;   		/* input buffer for msgs */
char Self[132];			        /* name of this C executable */

/**********************************************************************
 * Output the actual message to the file pointer specified...
 * Use the format made famous by the origional ACSEL.
 */
static void 
MsgOutput(
    FILE *fp,			/* file pointer to write to */
    char *cp_message		/* message to write. */
)
{
    time_t timestamp;   	/* Current time of day */
    char   ca_timestamp[80]; 	/* Timestamp buffer    */
    char   mtype_str[20];	/* string equiv of the message type */

    (void) time(&timestamp);
    strftime(ca_timestamp, sizeof(ca_timestamp),
		"%m-%d-%y %H:%M:%S", localtime(&timestamp));

    /* 
     * do a baby cl_type() here... only deal with the client
     * specific message_types
     */
    switch (el_bfr.ipc_header.module_type) {
        case TYPE_CSI:		
	    strcpy(mtype_str, "CSI"); 
	    break;

	case TYPE_SSI:		
	    strcpy(mtype_str, "SSI"); 
	    break;

	case TYPE_SA:
	    strcpy(mtype_str, "ACSSA");
	    break;

	case TYPE_EL:		
	    strcpy(mtype_str, "MINI_EL"); 
	    break;

	default: 		
	    sprintf(mtype_str, "(%d = Unknown)", 
		el_bfr.ipc_header.module_type);
            break;
     }

    /* 
     * Now log the message to the appropriate file pointer.
     */
    fprintf(fp, "%s %s[%d]:\n%s\n", ca_timestamp, mtype_str, 
	el_bfr.ipc_header.ipc_identifier, cp_message);
    (void) fflush(fp);
}

/**********************************************************************
 * Exit this code cleanly 
 */
static void 
ExitCleanly()
{
 /* fclose (efilep); */		/* event log file pointer */
 /* fclose (tfilep); */		/* trace log file pointer */
    close(listen_sock);		/* listen socket */
    exit(0);
}

/**********************************************************************
 * Open the socket and start the listen
 *
 *	This is a baby cl_ipc_open... no shared memory
 *
 * Return Values: 
 *    	On success: socket descriptor number 
 * 	On failure: a negative number (SOCK_*_ERR)
 */
static int 
OpenSocket(
    char *sock_name_in,			/* pointer to input sock name */
    char *sock_name_out			/* pointer to "bind" socket */
)
{
    struct sockaddr_in inserver;	/* the socket structure */
    char               sock_name[132];	/* the socket name */
    int                sd;		/* generic socket descriptor. */
    int			port;		/* integer port name */
    int                on = 1;		/* for re-use */
#ifdef AIX
    socklen_t          nsize;
#else
    socklen_t          nsize;           /* size of sock_name */
#endif /* nsize */
    int                rval;		/* ret value from sys calls */
    char                hostname[128];
    struct hostent     *hp, *gethostbyname();


    /* make local copy of socket name for later modification */
    strcpy(sock_name, sock_name_in);

    /* 
     * create socket first 
     */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nOpenSocket: Call to socket() failed\n");
	printf("\terrno = %d\n", errno);
        perror("\tcall to socket");
	return (SOCK_CREATE_ERR);
    }

    /* 
     * check port number specified 
     */
    if ((port = atoi(sock_name)) < 0) {
        printf("\nOpenSocket: Call to atoi() failed\n");
	printf("\terrno = %d\n", errno);
        perror("\tcall to atoi");
	return (PORT_ATOI_ERR);
    }

    /* 
     * Bind out local address so that the client can send to us.
     */
    inserver.sin_family 	= PF_INET;
    if (gethostname(hostname, sizeof(hostname)) < 0)
        return(SOCK_BIND_ERR);  /* Don't retry */

    if ((hp = gethostbyname(hostname)) == 0)
        return(SOCK_BIND_ERR);  /* Don't retry */

    memcpy ((char *)&inserver.sin_addr, hp->h_addr, hp->h_length);

    inserver.sin_port 		= htons((unsigned short)port);

    /* 
     * if port already selected, set re-use option (SO_REUSEADDR)... this
     * guarantees that we can read here.
     */
    rval = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, 
	(char *)&on, sizeof(on));
    if (rval < 0) {
        printf("\nOpenSocket: Call to setsockopt() failed\n");
	printf("\terrno = %d\n", errno);
        perror("\tcall to setsockopt");
        return(SOCK_OPT_ERR);
    }

    /* 
     * Bind in the socket descriptor (sd) which was set up above.
     */
    rval = bind(sd, (struct sockaddr *)&inserver, sizeof(inserver));
    if (rval < 0) {
	close(sd);
        printf("\nOpenSocket: Call to bind() failed\n");
	printf("\terrno = %d\n", errno);
        perror("\tcall to bind");
	return(SOCK_BIND_ERR);
    }

    if (Debug) 
	printf("\nOpenSocket: Debug: rval from bind() = %d\n", rval);

    /* 
     * figure out the port number used 
     */
    nsize = sizeof(inserver);
    rval = getsockname(sd, (struct sockaddr *)&inserver, &nsize);
    if (rval < 0) {
        printf("\nOpenSocket: Call to getsockname() failed\n");
	printf("\terrno = %d\n", errno);
        perror("\tcall to getsockname");
        return(SOCK_NAME_ERR);
    }

    if (Debug) 
	printf("\nOpenSocket: Debug: rval from getsockname() = %d\n", 
		rval);

    sprintf(sock_name_out, "%d", ntohs(inserver.sin_port));

    if (Debug) 
	printf("\nOpenSocket: Debug: sd = %d, sock_name_out = '%s'\n", 
		sd, sock_name_out);

    return (sd);
}

/**********************************************************************
 * Name:
 *
 *	ELInit
 *
 * Description:
 *
 *      This module will perform initialization functions:
 *
 *	1) setup the input socket
 *	2) open it up and prepare for listening.
 *
 *
 *      EL_SUCCESS		Successful Init
 *      EL_IPC_FAILURE 		error from listen().
 *      Other                   Assorted happenings..
 */
static int 
ELInit()
{
    char   *sock_name;			/* input socket name */
    char   my_sock_name[132];		/* what it "binds" to. */
    int    rval;			/* generic return value */

    /* 
     * setup input socket -- in ACSLS.. this would be cl_proc_init() 
     */

    /* input socket name (ACSEL: argv[2])... defined above in PORT */
    sock_name = PORT;
    if (Debug) printf("\nELInit: sock_name = '%s'\n", sock_name);


    /*
     * In the ACSLS world... call cl_ipc_create() .
     */

    /* set up IPC */
    if (Debug) printf("%s: calling OpenSocket from ELInit...\n", Self);
    listen_sock = OpenSocket(sock_name, my_sock_name);
    if (listen_sock < 0) {
	printf("ELInit: error from OpenSocket... bailing out\n");
        return(listen_sock);
    } 


    /*
     *  At this point a socket is created .
     *  It should be okay to use the socket now.
     */ 

    if (Debug) 
	printf("ELInit: Debug: about to listen on %d\n", listen_sock);

    /* 
     * set up connection limit ... Maximum that system will support,
     * but at least 32 (This is defined by SOMAXCONN in sys/socket.h)
     */
    rval = listen(listen_sock, ((SOMAXCONN > 32) ? SOMAXCONN : 32));
    if (rval < 0) {
        printf("\nELInit: error from listen: %d\n", rval);
	printf("\terrno = %d\n", errno);
        perror("\tcall to listen");
        return (EL_IPC_FAILURE);
    }

    /* IPC mechanism established, logging via ACSEL can commence */

    return (EL_SUCCESS);
}



/**********************************************************************
 * Name:
 *
 *	ELInOut
 *
 * Description:
 *
 *      This module will responsible for reading the input socket and
 *      storing the current message into a static input buffer.
 */
static int 
ELInOut()
{
    int return_status = EL_SUCCESS;  	/* module return status */
    int	read_byte_count;    		/* byte ct rtn from ipc_read */
    int rval;       			/* rval from from cl_select */
    fd_set readfds;			/* the set of fds */
    struct timeval timeout, *tmop;	/* timeout stuff */
    register int i, j;			/* counter */
    register int nsd;			/* net socket descriptor */
    struct sockaddr_in name;		/* socket name */
    char efile[132];			/* event log file to write to */
    char tfile[132];			/* trace log file to write to */
    socklen_t name_len = sizeof(name);	/* length */
    int nfds = 1;			/* number of fds */
    int *fds = &listen_sock;		/* file descriptor (socket) */
    int *fdp;                           /* descriptor pointer */
    int tmo = 20;			/* time out value */
    int maxd;				/* max descriptors */
    static BOOLEAN first_tfile_msg = TRUE;
    static BOOLEAN first_efile_msg = TRUE;

    /* clear out message input buffer */
    memset((char *)&el_bfr, '\0', 
	sizeof(el_bfr) );

    do {
	/* 
	 * check to see if any message has been received 
	 * If we were using the common library provided with the toolkit, 
	 * The call would look something like the following: 
	 *    rval = cl_select_input(1, &listen_sock, 5); 
	 */

	if (Debug) printf("\nELInOut: Initializing the fd_set...\n");
        /* initialize the fd_set */
        FD_ZERO(&readfds);		/* clear all bits */

	/* set up requested descriptors */
	for (i = maxd = 0, fdp = fds; i < nfds; i++, fdp++) {
            FD_SET(*fdp, &readfds);	
	    if (*fdp > maxd) 
		maxd = *fdp;
	}

        /* 
	 * set up for timeouts ... 
	 * Note, the value of 'tmo' is explicitely set to 20 above, 
	 * but can be modified, or even read from the environment.
	 * Just for grins, check the value for sanity sake and 
	 * set the timeout struct 'tmop' appropriately.
	 */
        if (tmo < 0)
            tmop = (struct timeval *)NULL;
        else {
            timeout.tv_sec = tmo;
            timeout.tv_usec = 0;
            tmop = &timeout;
        }

        /* hang on select */

	if (Debug) printf("ELInOut: About to call select...\n");

        i = select(maxd + 1, &readfds, 
		(fd_set*)NULL, (fd_set*)NULL, tmop);

	if (i < 0) {
	    printf("\nELInOut: bad select: %d...\n", rval);
	    printf("\terrno = %d\n", errno);
            perror("\tcall to select");
            return (EL_IPC_FAILURE);
	}

        /* did we get a timeout? */
        if (i == 0)  {
	    if (Debug) printf("ELInOut: Timed Out...\n");
	    return(EL_TIMEDOUT);;
	}

	/* otherwise, figure out which fd is active */
    	for (i = 0, fdp = fds; i < nfds; i++, fdp++)
            if (FD_ISSET(*fdp, &readfds)) 
                break;

        /* This should never be the case, but should be checked anyway */
        if (i >= nfds) {
	    printf("\nELInOut: got into an interesting case.. no fds\n");
	    return(EL_NO_FD);
	}

        /* 
	 * If we were using the common library call 'cl_select_input()'
	 * we would return here with i.
	 */

        /* 
	 * read message at input socket 
         */

	if (Debug) 
	    printf("ELInOut: Debug: About to call cl_ipc_read code\n");

        /* accept connection */
	if (Debug) printf("ELInOut: Debug: About to call accept\n");
        if ((nsd = accept(listen_sock, (struct sockaddr *)&name, 
			  &name_len)) < 0) {
	    perror("accepting socket connection");
	    close(listen_sock);
            return(EL_IPC_FAILURE);
        }

	if (Debug) printf("ELInOut: Debug: About to call read\n");
        if ((rval = read(nsd, (char *)&el_bfr, BUFSIZE)) < 0) {
	    perror("reading server message");
            ExitCleanly();
	}

	if (Debug)
	    printf("ELInOut: Debug: read %d bytes from socket\n", 
	    rval);

	/* 
	 * Deal with the message...
	 */
	if (rval > 0) {
            /* Put message to terminal if requested */
	    if (Debug) {
		printf("\n");

		/* 
		printf("el_bfr.msg_header.message_options.version = %d\n", 
		    el_bfr.msg_header.message_options.version);
		    */

		printf("el_bfr.ipc_header.byte_count     = %d\n", 
			el_bfr.ipc_header.byte_count);
		printf("el_bfr.ipc_header.module_type    = %d\n", 
		        el_bfr.ipc_header.module_type);
	        printf("el_bfr.ipc_header.options        = %d\n", 
		        el_bfr.ipc_header.options);
		printf("el_bfr.ipc_header.seq_num        = %d\n", 
		        el_bfr.ipc_header.seq_num);
		printf("el_bfr.ipc_header.return_socket  = %s\n", 
		        el_bfr.ipc_header.return_socket);
		printf("el_bfr.ipc_header.return_pid     = %d\n", 
		        el_bfr.ipc_header.return_pid);
		printf("el_bfr.ipc_header.requestor_type = %d\n", 
		        el_bfr.ipc_header.requestor_type);

		switch (el_bfr.log_options) {
		    case LOG_OPTION_EVENT:
		      printf("el_bfr.log_options               = %s\n", 
			  "event.log");
                      break;

		    case LOG_OPTION_TRACE:
		      printf("el_bfr.log_options               = %s\n", 
			  "trace.log");
                      break;

		    default: 
		      printf("el_bfr.log_options               = %d\n", 
		      	      el_bfr.log_options);
		      break;
		}

		printf("el_bfr.event_message             = %s\n", 
		        el_bfr.event_message);
	    }

	    /* Put message to logfile */


	    switch (el_bfr.log_options) {
	        case LOG_OPTION_EVENT:
		default:
                   /* 
                    * build up and open event log file name to write to 
                    */
                   strcpy(efile, "./");		/* Set up path */
                   strcat(efile, ELOG);	/* add event log file name */
	           if (Debug) printf("ELInOut: Debug: About to call fopen\n");
                   efilep = fopen(efile, "a");
                   if (Debug && first_efile_msg ) {
	               printf("%s: All Event Log data will be appended ",
			      Self);
	               printf("to file: %s\n", 
	                      efile);
		       first_efile_msg = FALSE;	       
                    }
               
	            MsgOutput(efilep, el_bfr.event_message);
		    fclose(efilep);
		    break;

		case LOG_OPTION_TRACE:
                    /* 
                     * build up and open trace log file name to write to 
                     */
                    strcpy(efile, "./");		/* Set up path */
                    strcpy(tfile, "./");		/* Set up path */
                    strcat(tfile, TLOG);	/* add event log file name */
	            if (Debug) printf("ELInOut: Debug: About to call fopen\n");
                    tfilep = fopen(tfile, "a");
                    if (Debug && first_tfile_msg) {
	                printf("%s: All Trace Log data will be appended ",
			       Self);
			printf("to file: %s\n", 
	                       tfile);
		       first_tfile_msg = FALSE;	       
                    }
                
	            MsgOutput(tfilep, el_bfr.event_message);
		    memset((char *)&el_bfr, '\0', sizeof(el_bfr) );
		    fclose(tfilep);
		    break;
	     }

	} /* if */

	/* 
	 * Close the socket descriptor... we open it implicitely above
	 * in the bind call
	 */
        close (nsd);

    } while (rval != 0);
 return rval;
}


/**********************************************************************
 * Name: 
 * 	mini_el
 * 
 * Description:
 * 	Mini Event Logger... for the clients ... no parents, 
 *	no kill signals.
 *
 *	Generic throughout ALL ACSLS versions.
 */
int
main(
    int argc, 
    char *argv[]
)
{
    struct sockaddr_in dummy;		/* dummy read sock handle */
    int rval = EL_SUCCESS;		/* return value of data */
    char efile[132];			/* event log file to write to */
    char tfile[132];			/* trace log file to write to */
    int c;
    int i, j;				/* misc counters */
    fd_set readfds;			/* read filedescriptors */

    /* 
     * Deal with command line arguments.
     *
     * All command line options are optional as follows:
     * Usage: 
     *		-d	outout debug information as it happens
     *		-h	output the Usage Line.
     */
    while ((c = getopt(argc, argv, "dhv")) != -1) {
        switch ((char)c) {
          case 'd':		/* output debug stuff */
            Debug = TRUE;
            break;
	  case 'v':		/* packet level version */
	    PLVersion = 0;	/* use down level version */
	    break;
          case 'h':		/* help option */
	  default:		/* errors */
            printf("Usage: %s [-d] [-h] [-v]\n", argv[0], stderr);
            exit(0);
        }
    }

    /* 
     * initialize 
     */
    if (Debug) printf("\n%s: calling strcpy from main...\n", Self);
    strcpy(Self, argv[0]);
    if (Debug) printf("%s: calling ELInit from main...\n", Self);
    rval = ELInit();
    if (rval != EL_SUCCESS) {
        printf ("%s: Failure from ELInit(): %d\n", Self, rval);
        exit(0);
    }

    /*
     * Do the big loop here... 
     */

    for (;;) {
        /* wait for an event log/trace log message to arrive */
        if (Debug) printf("%s: calling ELInOut from main...\n", Self);
	rval = ELInOut();

        /* case of ELInOut return status */
        switch (rval) {

            case EL_SUCCESS:
                /* process the received log message */
                if (Debug) printf("%s: call el_output();...\n", Self);
                break;

	    case EL_TIMEDOUT:
		/* deal with the timeout scenerio... */
		if (Debug) printf("%s: got a timeout...\n", Self);
		break;

            default:
              /* handle fatal error ... this should NEVER happen. */
	      printf("\nReceived %d from ELInOut()... bailing out.\n",
			rval);
              ExitCleanly();
              break;
        }
    } /* for-ever */
}
