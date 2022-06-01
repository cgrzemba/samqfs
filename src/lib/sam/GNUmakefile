# $Revision: 1.16 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..
LIB_HOME = $(DEPTH)/src/lib/sam

include $(DEPTH)/mk/common.mk

# BUILD_64BIT = yes

SRC_VPATH = $(DEPTH)/src/lib/samut
vpath %.c $(SRC_VPATH)


LIB = sam
LIB_SRC = \
	attrtoa.c \
	fileops.c \
	lstat.c    \
	media.c    \
	quota.c \
	readrminfo.c \
	request.c \
	segment_stat.c \
	segment_lstat.c \
	sgethost.c \
	stat.c \
	vsn_stat.c \
	segment_vsn_stat.c

ifeq ($(OS), SunOS)
LIB_SRC += \
	restore.c \
	storage.c \
	devst_string.c \
	devstat.c

ifeq ($(SPARCV9), yes)
LIB_SRC += nm_to_dev.c
LIB_LIBS +=  -L $(DEPTH)/lib/$(OBJ_DIR) -lsamconf $(LIBSO) -lintl
LIBDEST += /sparcv9
DEPCFLAGS += -DLIBS_64
else
ifeq ($(AMD64), yes)
LIB_SRC += nm_to_dev.c
LIB_LIBS +=  -L $(DEPTH)/lib/$(OBJ_DIR) -lsamconf $(LIBSO) -lintl
LIBDEST += /amd64
DEPCFLAGS += -DLIBS_64
else
LIB_SRC += catalog.c
LIB_LIBS += -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamut -lsamconf $(LIBSO) -lintl
endif
endif
else
LIB_SRC += dev_to_nm.c
LIB_LIBS += $(LIBSO)
endif

DEPCFLAGS += $(THRCOMP)

# To build test programs post installing libsam 
ifeq ($(OS), SunOS)

TESTPROG1 = $(OBJ_DIR)/segment_vsn_stat
TESTPROG2 = $(OBJ_DIR)/vsn_stat
TESTSRC1 = segment_vsn_stat.c
TESTSRC2 = vsn_stat.c

TESTINCFLAGS = -I ../../../include/
TESTLIBFLAGS = -L /opt/SUNWsamfs/lib

test: $(TESTPROG1) $(TESTPROG2)

$(TESTPROG1): $(TESTSRC1) 
	$(CC) $< -DMAIN $(TESTINCFLAGS) $(TESTLIBFLAGS) -lsamconf -lsam -o $@

$(TESTPROG2): $(TESTSRC2) 
	$(CC) $< -DMAIN $(TESTINCFLAGS) $(TESTLIBFLAGS) -lsamconf -lsam -o $@
endif


CHECKSRC = $(LIB_SRC)
ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
