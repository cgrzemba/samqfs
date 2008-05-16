# $Revision: 1.17 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

PROG = fsd
PROG_SRC = fsd.c
COMMON_SRC = \
	defaults.c \
	fsconfig.c \
	readconf.c \
	readmcf.c \
	server.c

ifeq ($(OS), SunOS)
COMMON_SRC += diskvols.c
endif

INCFLAGS = -I../include -I../include/$(OBJ_DIR) $(THRCOMP) -I$(INCLUDE)/pub/$(OBJ_DIR) -I$(INCLUDE)/sam/$(OBJ_DIR)
DEPCFLAGS += $(INCFLAGS) $(VERS)

ifeq ($(OS), SunOS)
CFLAGS += $(INCFLAGS)
else

ifeq (4,$(KERNEL_MAJOR))
CFLAGS += $(INCFLAGS)
else
CFLAGS = -I$(DEPTH)/include $(OSFLAGS) $(INCFLAGS) -DNO_AUTOCONF $(CERRWARN)
endif

endif

LDFLAGS += ../lib/$(OBJ_DIR)/setsyscall.o
PROG_LIBS = $(STATIC_OPT) -L ../lib/$(OBJ_DIR) -lfscmd -L $(DEPTH)/lib/$(OBJ_DIR) \
			-lsam -lsamut
ifeq ($(OS), SunOS)
INCFLAGS  += -I$(DB_INCLUDE)
PROG_LIBS += -lsamspmd -lsamspm -lsamrft $(DB_LIB) $(DYNAMIC_OPT) \
-lsocket -lnsl -ladm
else
PROG_LIBS += -lsamconf
endif
PROG_LIBS += $(DYNAMIC_OPT) -lpthread -ldl -lrt $(LIBSO)
ifeq ($(OS), SunOS)
PROG_LIBS += -lsysevent -lnvpair
endif

fsd_LIBS = $(PROG_LIBS)
qfs-fsd_LIBS = $(PROG_LIBS)

DESTDIR = /usr/lib/fs/samfs

COMMON_OBJS = $(COMMON_SRC:%.c=$(OBJ_DIR)/%.o)
LDFLAGS += $(COMMON_OBJS)

LNOPTS = -unmxs -DMETADATA_SERVER
LNLIBS =

include $(DEPTH)/mk/targets.mk

.INIT:	$(COMMON_OBJS)
all:	$(OBJ_DIR)/qfs-fsd

#
# Build a QFS only version
#
$(OBJ_DIR)/qfs-fsd:	$(COMMON_OBJS) $(OBJ_DIR)/qfs-fsd.o
	$(CC) $(LDFLAGS) -o $@ $(TARG_LIB) $(OBJ_DIR)/qfs-fsd.o $(OBJS) $(PROG_LIBS)

$(OBJ_DIR)/qfs-fsd.o: fsd.c
	$(CC) $(CFLAGS) -DQFS -c fsd.c -o $@

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
