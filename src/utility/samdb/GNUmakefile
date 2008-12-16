# $Revision: 1.4 $

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

include $(DEPTH)/mk/common.mk

COMMON_SRC = event_handler.c util.c

PROG = samdb
PROG_SRC = samdb.c \
	check.c \
	create.c \
	drop.c \
	dump.c \
	load.c \
	query.c \
	$(COMMON_SRC)

MPROGS = dbupd 

LIBSO += -R/opt/SUNWsamfs/lib/gpl

COMMON_OBJS = $($(COMMON_SRC):%.c=$(OBJ_DIR)/%.o)
COMMON_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) $(DYNAMIC_OPT) -lsamdb -lsamut -lsam -ladm \
	$(MYSQL_LIB) $(LIBSO)

PROG_LIBS = $(DEPTH)/src/fs/lib/$(OBJ_DIR)/libfscmd.a $(COMMON_LIBS)

dbupd_SRC = dbupd.c $(COMMON_SRC) 
dbupd_LIBS = $(COMMON_LIBS)

DEPCFLAGS += $(MYSQL_INCLUDE)

LDFLAGS = $(COMMON_OBJS)
LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamdb -lsamut $(MYSQL_LIB)

include $(DEPTH)/mk/targets.mk

.INIT: $(COMMON_OBJS)

include $(DEPTH)/mk/depend.mk
