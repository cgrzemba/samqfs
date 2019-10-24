/* SccsId @(#)ml_pub.h	1.2 1/11/94  */
#ifndef _ML_PUB_
#define _ML_PUB_
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *   This file includes public macros and function prototyes
 *   for the ml_ (Message logging) functionality.
 *
 * Modified by:
 *
 *   Alec Sharp		30-Oct-1993  Original.
 *   David Farmer	17-Nov-1993  Moved in CL_ASSERT from defs.h
 *   David Farmer	01-Dec-1993  Changed DPRINTF to MLOGDEBUG
 *   H. Grapek          13-Dec-1993  mods for the clients.
 *   Mitch Black	03-Jan-2004  Added ml_start_message prototype.
 */

/* ----------- Header Files -------------------------------------------- */

#ifndef _ERROR_H
#include <errno.h>
#endif

#ifndef _DB_DEFS_API_
#include "db_defs.h"
#endif


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

#define MNOMSG  MMSG(0,"")     /* Macro to specify no additional message
				  text for MLOGU, etc. */

#if defined (_lint)

    #define MMSG(num,fmt)       num,fmt
    #define MLOG(args)          ml_lint_event args
    #define MLOGDB(args)        ml_lint_db args
    #define MLOGU(args)         ml_lint_unexpected args
    #define MLOGERRNO(args)     ml_lint_errno args
    #define MLOGSIGNAL(args)    ml_lint_signal args
    #define MLOGCSI(args)  	ml_lint_csi args
    #define MLOGDEBUG(debug_lvl,args)    ml_lint_event args;

#else /* defined (_lint) */
    
    #define MMSG(num,fmt) fmt

    /* Regular event logging macro */
    #define MLOG(args)    do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_event args; } while (0)

    /* Macro for logging database errors */
    #define MLOGDB(args)   do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_db args; } while (0)

    /* Macro for logging "unexpected" returns from function calls */
    #define MLOGU(args)    do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_unexpected args; } while (0)

    /* Event logging that also logs errno */
    #define MLOGERRNO(args)    do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_errno args; } while (0)

    /* Event logging that also logs signals received */
    #define MLOGSIGNAL(args)    do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_signal args; } while (0)

    /* Macro for CSI logging */
    #define MLOGCSI(args)    do {     \
      ml_file_name = __FILE__; ml_line_num  = __LINE__; \
      ml_file_id = SccsId; ml_log_csi args; } while (0)

    #ifdef DEBUG
        #define MLOGDEBUG(debug_lvl,print_str) do { \
          if TRACE(debug_lvl) {            \
                MLOG(print_str);	   \
          }                                \
        } while (0)
    #else
        #define MLOGDEBUG(debug_lvl,print_str) \
	  ;
    #endif
    
#endif /* defined (_lint) */
 
/* moved from defs.h */

#define CL_ASSERT(n, x)                 {   /* assert an expression     */  \
    if ( !(x) ) {                                                           \
        MLOG((MMSG(1190, ("%s: Assertion %s failed, file %s, line %d.")),   \
	  n, #x, __FILE__, __LINE__));			                    \
        return STATUS_PROCESS_FAILURE;                                      \
    }                                                                       \
}


/* ----------- Global and Static Variable Declarations ----------------- */

/* These variables are used to set values for the ml_log_ functions. */
extern char    *ml_file_name;    /* File name */
extern char    *ml_file_id;      /* Sccs ID. Used to get the version number */
extern int 	ml_line_num;     /* Line number in the source file */


/* ----------  Function Prototypes ------------------------------------- */

/* Specifically for lint */

void ml_lint_db (char *, int, char *, ...);
void ml_lint_event (int, char *, ...);
void ml_lint_errno (int, char *, ...);
void ml_lint_signal (int, int, char *, ...);
void ml_lint_unexpected (char *, char *, STATUS, int, char *, ...);
void ml_lint_csi (STATUS, char *, char *, int, char *, ...);

/* Real function prototypes */

#ifdef NOT_CSC
void   ml_log_db (char *caller, int msgno, ...);
void   ml_log_event (int msgno, ...);
void   ml_log_errno (int msgno, ...);
void   ml_log_signal (int i_signal, int i_msgno, ...);
void   ml_log_unexpected (char *caller, char *callee,
			  STATUS status, int msgno, ...);
void   ml_log_csi ( STATUS status, char *caller, char *callee, int msgno, ...);
STATUS ml_msg_initialize (char *);
void   ml_output (const char *cp_message);
void   ml_output_register (void (*funcptr)(const char *));
void   ml_start_message(char *);

#else /* The TOOLKIT overloads these functions in its ssi/csi */

void   ml_log_db (char *caller, int msgno, ...);
void   ml_log_event (char * cp_fmt, ...);
void   ml_log_errno (int msgno, ...);
void   ml_log_signal (int i_signal, int i_msgno, ...);
void   ml_log_unexpected (char *caller, char *callee,
                          STATUS status, char * cp_fmt, ...);
void   ml_log_csi ( STATUS status, char *caller, char *callee, char * cp_fmt, ...);
STATUS ml_msg_initialize (char *);
void   ml_output (const char *cp_message);
void   ml_output_register (void (*funcptr)(const char *));
void   ml_start_message(char *);
#endif /* NOT_CSC */


#ifdef __cplusplus
}
#endif 
#endif /* _ML_PUB_ */

