# $Revision: 1.1 $

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
LIB_HOME = $(DEPTH)/src/lib/samdb

include $(DEPTH)/mk/common.mk

LIB = samdb
LIB_SRC = \
	access.c \
	fsalog.c \
	insert.c \
	new.c \
	query.c \
	query_error.c \
	rename.c \
	restore.c \
	util.c \
	vsn_cache.c
	
LIBSO += -R/opt/SUNWsamfs/lib/gpl
LIB_LIBS = $(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut $(MYSQL_LIB) $(LIBSO)
DEPCFLAGS += -I$(INCLUDE)/sam/$(OBJ_DIR) $(MYSQL_INCLUDE)

LNOPTS = $(LIBS_LFLAGS32)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
