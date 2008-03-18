/*
 * Prototype of the st_blocks subroutine defined in fileblocks.c and
 * invoked from several other modules within this library.
 */

#ifndef _FILEBLOCKS_H
#define _FILEBLOCKS_H 1

#if !defined (HAVE_ST_BLOCKS) && !defined(_POSIX_SOURCE)
#include <sys/types.h>
#include <sys/param.h>

#ifdef	__cplusplus
extern "C" {
#endif

long st_blocks (u_longlong_t size);

#ifdef	__cplusplus
}
#endif

#endif

#endif
