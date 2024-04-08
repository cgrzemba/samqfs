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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

#	Define build procedures for SAM's recycler.  The recycler makes
#	media retention and deletion decisions based on space.  The recycler's
#	retention decisions are based on files in mounted file systems.
PROG = recycler
PROG_SRC = \
		recycler.c \
		catalog.c \
		disk_archive.c \
		errlog.c \
		findvsn.c \
		readcmd.c

#	Define build procedures for SAM's nrecycler and recycler library.  
#	The nrecycler makes media retention and deletion decisions based on time.  
#	The nrecycler's retention decisions are based on files in mounted file 
#	systems and samfs dumps.

DIRS = lib nrecycler

DATAFILES = recycler.sh

DEPCFLAGS += $(HC_INCLUDE) $(THRCOMP) -I include
PROG_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) \
	-lsam -lsamapi -lsamcat -lsamfs -lsamut -lsamconf -lsamrft \
	$(HC_LIB) $(LIBSO) $(THRLIBS) -lgen -lintl -L lib/$(OBJ_DIR) -lrecycler

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamapi -lsamcat -lsamfs \
	-lsamut -lsamconf -lsamrft -L lib/$(OBJ_DIR) -lrecycler

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(SYSINST) $(OBJ_DIR)/$(PROG) $(ADMDEST)/sam-$(PROG)

include $(DEPTH)/mk/depend.mk
