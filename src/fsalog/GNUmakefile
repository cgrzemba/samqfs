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

PROG = fsalogd
PROG_SRC = fsalogd.c readcmd.c

INCFLAGS = -I$(INCLUDE) -I$(INCLUDE)/pub/$(OBJ_DIR)
DEPCFLAGS = $(INCFLAGS) $(VERS) $(EFI_SUPPORT) $(METADATA_SERVER) $(DEBUGCDEFS) $(OSDEPCFLAGS)

PROG_LIBS = $(STATIC_OPT) -L ../lib/$(OBJ_DIR) \
	-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsam -lsamconf $(DYNAMIC_OPT) \
	$(LIBSO) -ldoor -lsysevent -lnvpair

DESTDIR = /opt/SUNWsamfs/sbin

LNOPTS = -nmuxs -Dlint
LNLIBS = -L ../lib/$(OBJ_DIR) -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

install:	all
	$(INSTALL) -m $(SYSMODE) -o root -g bin $(OBJ_DIR)/fsalogd $(DESTDIR)/sam-fsalogd
:
include $(DEPTH)/mk/depend.mk
