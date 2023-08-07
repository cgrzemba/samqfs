%/* 
% * Copyright 2023 Carsten Grzemba
% */ 

%/* generated from sam.x */

#ifdef RPC_XDR
#endif

#ifdef RPC_SVC
#endif

#ifdef RPC_CLNT
#endif

#ifdef RPC_HDR
%#include <rpc/rpc.h>
%#include <sys/types.h>
%#include <sys/time.h>
%#ifndef SAM_STAT_H
%#include "stat.h"
%#endif
#endif

const MAXPATHLEN = 1024;
const MAX_VSN = 32;
const MAX_OPTS = 24;
const MAX_ARCHIVE = 4;

struct filecmd {
	string filename<MAXPATHLEN>;
	string options<MAX_OPTS>;
};

struct statcmd {
	string filename<MAXPATHLEN>;
	size_t size;
};

typedef struct filecmd filecmd;
typedef struct statcmd statcmd;

typedef struct sam_stat samstat_t;

struct sam_st {
	int result;
	samstat_t s;
};

struct samcopy {
	ushort_t flags;
	short   n_vsns;
	time_t creation_time;
	uint64_t    position;
	uint_t  offset;
	char    media[4];
	char    vsn[MAX_VSN];
};

struct samstat_t {
	uint_t  st_mode;
	uint_t  st_ino;
	uint64_t    st_dev;
	uint_t  st_nlink;
	uint_t  st_uid;
	uint_t  st_gid;
	uint64_t    st_size;
	time_t  st_atime;
	time_t  st_mtime;
	time_t  st_ctime;
	time_t  attribute_time;
	time_t  creation_time;
	time_t  residence_time;
	samcopy copy[MAX_ARCHIVE];
	uint_t old_attr;
	uchar_t cs_algo;
	uchar_t flags;
	uchar_t stripe_width;
	uchar_t stripe_group;
	uint_t  gen;
	uint_t  partial_size;
	uint64_t    rdev;
	uint64_t    st_blocks;
	uint_t  segment_size;
	uint_t  segment_number;
	uint_t stage_ahead;
	uint64_t    attr;
/*	uint64_t    cs_val[2]; */ 
};

typedef struct sam_st sam_st;
typedef struct samcopy samcopy;
typedef struct samstat_t samstat_t;

program SAMFS {
   version SAMVERS {
       sam_st samstat(statcmd) = 1;
       sam_st samlstat(statcmd) = 2;
       int samarchive(filecmd) = 3;
       int samsetfa(filecmd) = 6;
       int samsegment(filecmd) = 7;
       int samrelease(filecmd) = 4;
       int samstage(filecmd) = 5;
   } = 1;
} = 0x20000002;

#ifdef RPC_HDR
%#ifndef        SAM_LIB
%extern CLIENT  *clnt;
%#endif /* !SAM_LIB */

%#define        PROGNAME        "samfs"
%#define        SAMRPC_HOST     "samhost"

%/* Functions. */
%int sam_initrpc(char *rpchost);
%int sam_closerpc(void);
#endif

