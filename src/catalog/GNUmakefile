# $Revision: 1.10 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

DIRS = build_cat dump_cat reserve unreserve

PROG = catserver
PROG_SRC = \
	catserver.c \
	cvrtcat.c \
	catlib.c

DEPCFLAGS += -Iinclude 
PROG_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamut $(LIBSO) -lgen -lintl

# LNOPTS = $(CMDS_LFLAGS$(if $(filter yes,$(AMD64)),64,32))
LNOPTS = 
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut
# FIXME -lsamcat

include $(DEPTH)/mk/targets.mk

$(OBJ_DIR)/catlib.o:	catlib.c
	$(CC) -DCAT_SERVER $(CFLAGS) -c -o $@ $<

include $(DEPTH)/mk/depend.mk

test_catlib: catlib.c test_catlib.c
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ $(LNOPTS) $(LNLIBS) -R/opt/SUNWsamfs/lib$(ISA) $?
