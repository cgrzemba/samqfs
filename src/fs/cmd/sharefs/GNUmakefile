# $Revision: 1.17 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

PROG = sharefs
PROG_SRC = sharefs.c

INCFLAGS =  -I ../../include -I $(DEPTH)/include/pub -I$(INCLUDE)
DEPCFLAGS = $(INCFLAGS) $(VERS) $(METADATA_SERVER) $(DEBUGCDEFS) $(THRCOMP)

LDFLAGS += ../../lib/$(OBJ_DIR)/setsyscall.o
PROG_LIBS = $(STATIC_OPT) -L../../lib/$(OBJ_DIR) -lfscmd \
			-L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut \
			$(DYNAMIC_OPT) -lsysevent -lnvpair $(LIBSO) -ladm -ldl

LNOPTS = -anmusx -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

INSTALLDEST = /opt/SUNWsamfs/sbin

install:	all
	$(INSTALL) -m $(SYSMODE) -o root -g bin $(OBJ_DIR)/sharefs $(INSTALLDEST)/samsharefs

include $(DEPTH)/mk/depend.mk
