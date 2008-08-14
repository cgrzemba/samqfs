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

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

LIB = samdbutil 
LIB_SRC = \
	arg.c \
	atobytes.c \
	atosecs.c \
	buffin.c \
	error.c \
	get_vsn.c \
	logfile.c \
	logtime.c \
	mtrlog.c \
	parsespaces.c \
	parsetabs.c \
	print_table.c \
	rename.c \
	update.c \
	xstrdup.c 

DEPCFLAGS += -I../include $(MYSQL_INCLUDE)
LIB_LIBS = $(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamdb -lsamut $(MYSQL_LIB) $(LIBSO)

LNOPTS = $(LIBS_LFLAGS32)
LNLIBS =

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(USERINST) $(OBJ_DIR)/libsamdbutil.so $(LIBDEST)

include $(DEPTH)/mk/depend.mk
