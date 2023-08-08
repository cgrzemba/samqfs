/*
 * samrpc.x - Oracle HSM Remote Procedure Call (RPC)
 *            user library structures and definitions
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
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 *    SAM-QFS_notice_end
 */


const MAX_VSN = 32;
const MAX_OPTS = 256;
const MAX_GETDIGEST_LENGTH = 255;

const PROGNAME = "samfs";
const SAMRPC_HOST = "samhost";


struct filecmd {
    string    filename<>;
    string    options<>;
};
typedef struct filecmd filecmd;

struct statcmd {
    string    filename<>;
    int       size;
};
typedef struct statcmd statcmd;

struct digest_arguments {
    string    filename<>;
    int       size;
};
typedef struct digest_arguments digest_arguments;

struct digest_result {
    int       result;
    int       size;
    string    digest<>;
};
typedef struct digest_result digest_result;

struct sam_st {
    int       result;
    samstat_t s;
};
typedef struct sam_st sam_st;

typedef struct sam_copy_s samcopy;

program SamFS {
    version SAMVERS {
        sam_st samstat(int c) = 1;
        sam_st samlstat(int c) = 2;
        int samarchive(filecmd) = 3;
        int samrelease(filecmd) = 4;
        int samstage(filecmd) = 5;
        int samsetfa(filecmd) = 6;
        int samsegment(filecmd) = 7;
        int samssum(filecmd) = 8;
        digest_result samgetdigest(digest_arguments) = 9;
    } = 1;
} = 0x20000002;
