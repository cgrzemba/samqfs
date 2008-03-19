# $Revision: 1.12 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

FSMGR_PATH = $(DEPTH)/src/lib/sammgmt $(DEPTH)/src/lib/sammgmt/restore
LIBFSMDB_PATH = $(DEPTH)/src/lib/fsmdb
SAMSTAT_PATH = $(DEPTH)/src/lib/sam $(DEPTH)/src/lib/samconf $(DEPTH)/src/lib/samut

SRC_VPATH =  $(FSMGR_PATH) $(LIBFSMDB_PATH) $(SAMSTAT_PATH) $(DEPTH)/src/fs/lib
vpath %.c $(SRC_VPATH)

#BUILD_64BIT = yes

DB_INCLUDE = $(DEPTH)/src/lib/bdb/V4/4.4.20/$(DB_ARCH)/include
CSD_INCLUDE = $(DEPTH)
BSWAP_INCLUDE = $(DEPTH)/src/fs/include

ifneq ($(PLATFORM), sparc)
ENDIAN=-DLITTLE
else
ALIGN=-xmemalign=4i
endif

DEPCFLAGS = \
	-DAPI_DEBUG=syslog \
	-DDEBUG -I/usr/include \
	-I$(DB_INCLUDE) -I$(INCLUDE) -I$(BSWAP_INCLUDE) -I$(CSD_INCLUDE) \
	$(DEBUGCDEFS) -D_POSIX_PTHREAD_SEMANTICS $(THRCOMP) $(ENDIAN) $(ALIGN)

PROG = fsmdb

#  The FSMGR_SRC files are to avoid linking directly with libfsmgmt.so.
#  libfsmgmt links with samut which in turn links to an old version of
#  Sleepycat.  When/if libsamut changes over to use Sleepycat 4.4, this
#  restriction can be considered for removal.

FSMGR_SRC = \
	bswap.c \
	cmn_csd_funcs.c \
	list.c \
	media.c \
	sam_syscall.c \
	stat.c \
	util.c

LIBFSMDB_SRC = \
	xlate.c

PROG_SRC = \
	$(FSMGR_SRC) \
	$(LIBFSMDB_SRC) \
	browse.c \
	file_metrics.c \
	fsmdb_svr.c \
	mkdb.c \
	pop_db.c \
	read_snap.c \
	stubs.c \
	vsn.c

PROG_LIBS = $(ALIGN) -L$(DEPTH)/lib/$(OBJ_DIR) \
		-ladm -ldoor -ldb-4.4 -lgen -lz -lnsl -lrt -lpthread $(LIBSO)


LNOPTS += -a -m -o$(LIB)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)
LNFLAGS = -DAPI_DEBUG=syslog -DDEBUG -I/usr/include \
	-I$(DB_INCLUDE) -I$(INCLUDE) -I$(BSWAP_INCLUDE) -I$(CSD_INCLUDE) \
	$(DEBUGCDEFS) $(THRCOMP) $(ENDIAN) $(ALIGN)

# use the following for LNOPTS to get more detail from lint
#LNOPTS =  $(DEPCFLAGS) -m -x -errchk=%all -errfmt=macro -errsecurity=extended -fd -errtags=yes -erroff=E_SEC_PRINTF_VAR_FMT,E_OLD_STYLE_FUNC_DECL,E_FUNC_RET_ALWAYS_IGNOR,E_H_C_CHECK0,E_H_C_CHECK1,E_H_C_CHECK2,E_SEC_CREAT_WITHOUT_EXCL -Nlevel=4 -Ncheck=%all

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
