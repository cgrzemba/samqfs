# $Revision: 1.11 $
# Makefile definitions for archiver suite.
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


DEPCFLAGS += $(AR_DEBUG) -I../include -I../include/$(OBJ_DIR) -I$(INCLUDE)/aml/$(OBJ_DIR) $(THRCOMP)

PROG_LIBS = $(ADDOBJ) -L ../lib/$(OBJ_DIR) -larch \
	-L $(DEPTH)/lib/$(OBJ_DIR) $(ADDLIB) -lsam -lsamcat -lsamut -lsamconf $(LIBSO) -lpthread

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L ../lib/$(OBJ_DIR) -larch -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamcat -lsamut -lsamconf

# Place cc definition arguments in the following line and uncomment it.
# -DAR_DEBUG Turns on additional debugging
#AR_DEBUG = -DAR_DEBUG
