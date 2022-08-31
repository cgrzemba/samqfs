# $Revision: 1.19 $

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

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

MPROGS = sammkfs mkfs
sammkfs_SRC = sammkfs.c
mkfs_SRC = mkfs.c

INCFLAGS = -I../../include -I../../include/$(OBJ_DIR) -I$(INCLUDE) -I$(INCLUDE)/pub/$(OBJ_DIR)
DEPCFLAGS = $(INCFLAGS) $(VERS) $(METADATA_SERVER) $(DEBUGCDEFS) $(NO_BUILD_OSD)

PROG_LIBS = $(STATIC_OPT) -L ../../lib/$(OBJ_DIR) -lfscmd \
	-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsam $(DYNAMIC_OPT) \
	-ladm -ldl -lsysevent -lnvpair -lscf
sammkfs_LIBS = ../../lib/$(OBJ_DIR)/setsyscall.o $(PROG_LIBS)
mkfs_LIBS = 


DESTDIR = /usr/lib/fs/samfs

LNOPTS = -amunsx -Dlint
LNLIBS = -L ../../lib/$(OBJ_DIR) -lfscmd -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

ifeq ($(SPARCV9), yes)
INSTALLDEST = /usr/lib/fs/samfs/sparcv9
else
INSTALLDEST = /usr/lib/fs/samfs
endif

install:	all
	$(INSTALL) -m $(SYSMODE) -o root -g bin $(OBJ_DIR)/mkfs $(INSTALLDEST)/mkfs

include $(DEPTH)/mk/depend.mk
