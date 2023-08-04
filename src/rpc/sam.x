%/* 
% * Copyright 2023 Carsten Grzemba
% */ 

%/* generated from sam.x */

const MAXPATHLEN = 1024;
const MAX_OPTS = 24;
const MAX_ARCHIVE = 4;

struct filecmd {
	string filename<MAXPATHLEN>;
	string options<MAX_OPTS>;
};

struct statcmd {
	string filename<MAXPATHLEN>;
	int size;
};
typedef struct filecmd filecmd;
typedef struct statcmd statcmd;

struct sam_st {
	int result;
	samstat_t s;
};
typedef struct sam_st sam_st;

struct samcopy {
	ushort_t flags;
	short	n_vsns;
	time_t creation_time;
	u_longlong_t	position;
	uint_t	offset;
	char	media[4];
	char	vsn[MAX_VSN];
};
typedef struct samcopy samcopy;

struct samstat_t {
	uint_t	st_mode;
	uint_t	st_ino;
	u_longlong_t	st_dev;
	uint_t	st_nlink;
	uint_t	st_uid;
	uint_t	st_gid;
	u_longlong_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
	time_t	attribute_time;
	time_t	creation_time;
	time_t	residence_time;
	samcopy	copy[MAX_ARCHIVE];
	uint_t old_attr;
	u_longlong_t	attr;
	uchar_t	cs_algo;
	uchar_t	flags;
	uchar_t	stripe_width;
	uchar_t	stripe_group;
	uint_t	gen;
	uint_t	partial_size;
	u_longlong_t	rdev;
	u_longlong_t	st_blocks;
	uint_t	segment_size;
	uint_t	segment_number;
	uint_t stage_ahead;
	u_longlong_t	cs_val[2];
};
typedef struct samstat_t samstat_t;

program SAMFS {
   version SAMVERS {
       sam_st samstat(statcmd) = 1;
       sam_st samlstat(statcmd) = 2;
       int samarchive(filecmd) = 3;
       int samrelease(filecmd) = 4;
       int samstage(filecmd) = 5;
       int samsetfa(filecmd) = 6;
       int samsegment(filecmd) = 7;
   } = 1;
} = 0x20000002;
