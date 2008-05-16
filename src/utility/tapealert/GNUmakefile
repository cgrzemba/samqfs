# $Revision: 1.9 $

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

MPROGS = tapealert tapealert_trap tapealert_log
tapealert_SRC = tapealert_text.c tapealert.c
tapealert_trap_SRC = tapealert_trap.c tapealert_text.c
tapealert_log_SRC = tapealert_log.c tapealert_text.c

PROG_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut $(LIBSO)
tapealert_LIBS = $(PROG_LIBS)
tapealert_trap_LIBS = $(PROG_LIBS)
tapealert_log_LIBS = $(PROG_LIBS)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
