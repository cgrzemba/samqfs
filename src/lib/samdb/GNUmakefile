# $Revision: 1.3 $

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
LIB_HOME = $(DEPTH)/src/lib/samdb

include $(DEPTH)/mk/common.mk

LIB = samdb
LIB_SRC = \
	archive.c \
	config.c \
	connect.c \
	file.c \
	fsalog.c \
	inode.c \
	path.c \
	util.c
	
LIBSO += -R/opt/SUNWsamfs/lib/gpl
LIB_LIBS = $(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut $(MYSQL_LIB) $(LIBSO)
DEPCFLAGS += -I$(INCLUDE)/sam/$(OBJ_DIR) $(MYSQL_INCLUDE) $(OSDEPCFLAGS)

LNOPTS = $(LIBS_LFLAGS32)

CATMSG = samdb_sql.msg
CATALOG = $(OBJ_DIR)/samdb_sql.cat

include $(DEPTH)/mk/targets.mk

all: $(CATALOG)

$(CATALOG): $(CATMSG)
	rm -f $(CATALOG)
	/usr/bin/gencat $(CATALOG) $(CATMSG)
	/bin/chmod -w $(CATALOG)

include $(DEPTH)/mk/depend.mk
