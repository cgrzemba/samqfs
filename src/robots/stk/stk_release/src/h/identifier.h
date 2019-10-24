/* SccsId @(#)identifier.h	1.2 1/11/94  */
#ifndef _IDENTIFIER_ 
#define _IDENTIFIER_ 
/* 
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file defines system-wide data structures used in database
 *      transactions and ACSLS subsystem interfaces (i.e., ACSLM programmatic 
 *      interface).  Due to limitations of the embedded SQL preprocessor, it
 *      is necessary to separate definitions required by the SQL preprocessor
 *      because it does not handle all C preprocessor statements nor some C
 *      constructs.
 *
 *      In embedded SQL (.sc) modules, this header file must be included
 *      before defs.h by an "EXEC SQL include" statement.  For example:
 *
 *          EXEC SQL include sqlca;             // SQL communications area
 *          EXEC SQL begin declare section;     // include in order shown
 *              EXEC SQL include "../h/db_defs.h";
 *              EXEC SQL include "../h/identifier.h";
 *              EXEC SQL include "../h/db_structs.h";
 *          EXEC SQL end declare section;
 *
 *          #include "defs.h"
 *          #include "structs.h"
 *
 *      This file also gets included by other header files if _IDENTIFIER_ is
 *      not yet defined and those header files include a #ifndef/#endif pair
 *      around the #include for this header file.  For example, structs.h
 *      contains:
 *
 *          #ifndef _IDENTIFIER_
 *          #include "identifier.h"
 *          #endif  _IDENTIFIER_
 *
 *      Due to the limitations of the INGRES embedded SQL preprocessor:
 *
 *        o #define definitions must be of the simple form below and must have
 *          have a replacement value.  Not specifying the value will result in
 *          the preprocessor flagging the line as an error.
 *
 *              #define name    value
 *
 *        o Do NOT specify #define values as expressions; they must be numeric
 *          or string constants.  For example:
 *
 *              #define MAX_TROUT   (MAX_RAINBOW + MAX_BROOK)   // WRONG!!
 *
 *          will be flagged as an error by the preprocessor.
 *
 *        o Do NOT include any compiler directive other #define in this header
 *          file.  The preprocessor will flag those lines as errors.  Because of
 *          this limitation, this header file is excluded from the #ifdef/#endif
 *          standard to limit header file inclusion.  However this means that
 *          header files including this one should have #ifdef/#endif pairs 
 *          surrounding the #include to avoid generating duplicate definitions.
 *
 *        o Do not include "unsigned" in the data type of a definition.  It is
 *          not supported by the preprocessor and will be flagged as an error.
 *
 * Modified by:
 *
 *      D. F. Reed          27-Jan-1989     Original.
 *      J. W. Montgomery    06-Mar-1990     Added POOLID and VOLRANGE.
 *      D. L. Trachy        10-Sep-1990     Added lock_id to IDENTIFIER
 *      J. S. Alexander     18-Jun-1991     Added CAPID, and CAP_CELLID.
 *      D. A. Beidle        10-Sep-1991     Removed CAP_CELLID.
 *      H. I. Grapek        23-Sep-1991     Added V0_CAPID and V1_CAPID defs,
 *                      Added V0_CAPID and V1_CAPID to IDENTIFIERS.
 *      J. S. Alexander     27-Sep-1991     Added define IDENTIFIER_SIZE.
 *      D. A. Beidle        27-Sep-1991     Added lots of additional comments
 *                      to explain why we don't make this header file easier
 *                      to use or understand.
 *      E. A. Alongi        26-Oct-1992     Added define ALIGNMENT_PAD_SIZE
 *                      which is used in typedef union IDENTIFIER and other
 *                      source code.
 *	J. Borzuchowski	    05-Aug-1993	    R5.0 Mixed Media--  Added
 *			MEDIA_TYPE and DRIVE_TYPE to IDENTIFER union.
 */

/*
 *      Header Files:
 */

#ifndef _DB_DEFS_
#include "db_defs.h"
#endif

#ifndef _IDENT_API_H_
#include "api/ident_api.h"
#endif
/*
 *      Defines, Typedefs and Structure Definitions:
 */

#endif

