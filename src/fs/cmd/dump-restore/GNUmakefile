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

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

PROG = csd
PROG_SRC = \
		csd.c \
		filecmp.c \
		mode_string.c \
		sam_bio.c \
		sam_dump.c \
		sam_getdent.c \
		sam_ls.c \
		sam_db.c \
		sam_read.c \
		sam_restore.c \
		sam_write.c \
		readin.c \
		parsetabs.c

DEPCFLAGS += -I$(INCLUDE)/pub/$(OBJ_DIR)
PROG_LIBS = ../../../fs/lib/$(OBJ_DIR)/libfscmd.a \
	-L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lgen -lsec $(LIBSO)

LNOPTS = -anmuxs -erroff=E_STATIC_UNUSED -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
