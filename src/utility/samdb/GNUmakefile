# $Revision: 1.2 $

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

DIRS = lib 

MPROGS = \
	samdb_catvsn \
	samdb_check \
	samdb_create \
	samdb_dump \
	samdb_listvsn \
	samdb_listinode \
	samdb_listpath \
	samdb_load \
	samdb_updated \
	samdb_updatevsn

LIBSO += -R/opt/SUNWsamfs/lib/gpl
PROG_LIBS = $(STATIC_OPT) -L lib/$(OBJ_DIR) -lsamdbutil \
	$(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamdb -lsamut \
	$(MYSQL_LIB) $(LIBSO)

samdb_catvsn_SRC = catalog_vsn.c
samdb_catvsn_LIBS = $(PROG_LIBS)

samdb_check_SRC = check.c
samdb_check_LIBS = $(PROG_LIBS)

samdb_create_SRC = create.c
samdb_create_LIBS = $(PROG_LIBS)

samdb_dump_SRC = dump.c
samdb_dump_LIBS = $(PROG_LIBS)

samdb_listvsn_SRC = list_vsn.c
samdb_listvsn_LIBS = $(PROG_LIBS)

samdb_load_SRC = load.c
samdb_load_LIBS = $(PROG_LIBS)

samdb_listinode_SRC = list_inode.c
samdb_listinode_LIBS = $(PROG_LIBS)

samdb_listpath_SRC = list_path.c
samdb_listpath_LIBS = $(PROG_LIBS)

samdb_updated_SRC = updated.c
samdb_updated_LIBS = $(PROG_LIBS)

samdb_updatevsn_SRC = update_vsn.c
samdb_updatevsn_LIBS = $(PROG_LIBS)

DEPCFLAGS += -Iinclude $(MYSQL_INCLUDE)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L lib/$(OBJ_DIR) -lsamdbutil -L $(DEPTH)/lib/$(OBJ_DIR) $(MYSQL_LIB) 

include $(DEPTH)/mk/targets.mk

catalog_vsn.c check.c create.c dump.c list_vsn.c: lib
load.c list_inode.c list_path.c updated.c update_vsn.c: lib 

include $(DEPTH)/mk/depend.mk
