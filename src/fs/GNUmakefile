# $Revision: 1.12 $

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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

DIRS =	\
	include \
	cmd \
	lib \
	fsd \
	samfs \
	sharefs \
	tools \

ifeq ($(MAKECMDGOALS), clobberall)
	DIRS += sc ha
else
ifeq ($(BUILD_SC_AGENT), yes)
	DIRS += sc ha
endif
endif

include $(DEPTH)/mk/targets.mk

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
ifeq ($(OS), Linux)
samfs cmd fsd tools sharefs:  include lib
else
samfs cmd fsd tools sharefs sc ha:  include lib
endif
lib:	include

include $(DEPTH)/mk/depend.mk
