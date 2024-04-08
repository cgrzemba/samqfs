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

PROG = fstyp
PROG_SRC = fstyp.c

INCFLAGS = -I$(INCLUDE) -I../../include -I../../include/$(OBJ_DIR)
DEPCFLAGS = $(INCFLAGS) $(VERS) $(METADATA_SERVER) $(THRCOMP)
CFLAGS = $(DEPCFLAGS) $(PLATFLAGS) $(CERRWARN)

PROG_LIBS = -L ../../lib/$(OBJ_DIR) -L $(DEPTH)/lib/$(OBJ_DIR) \
	$(STATIC_OPT) -lfscmd -lsamut $(DYNAMIC_OPT) $(LIBSO) -ldl -lnvpair -lsysevent

DESTDIR = /usr/lib/fs/samfs

LNOPTS = -axmnus -Dlint
LNLIBS = -L ../../lib/$(OBJ_DIR) $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
