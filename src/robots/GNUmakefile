# $Revision: 1.14 $

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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

PROG = robots
PROG_SRC = main.c child.c build_pids.c

DIRS = lib common generic
PROG_LIBS = -lgen -R /opt/SUNWsamfs/lib -L $(DEPTH)/lib/$(OBJ_DIR)  -lsamfs -lsamut

ifeq ($(MAKECMDGOALS), clobberall)
	DIRS += ibm3494 sony thirdparty stk
else
ifneq ($(SAM_OPEN_SOURCE), yes)
	DIRS += thirdparty stk
ifeq ($(PLATFORM), sparc)
	DIRS += ibm3494 sony
	PROG_LIBS += $(THRLIBS) -lsamcat
else
	PROG_LIBS += -lposix4 $(THRLIBS) -lsamcat
endif
endif
endif

LNOPTS += -a
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
common:	lib
generic thirdparty ibm3494 sony stk: common

include $(DEPTH)/mk/depend.mk
