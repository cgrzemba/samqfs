# $Id: GNUmakefile,v 1.5 2008/12/16 00:08:36 am143972 Exp $

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

DEPTH = ../../../../..
OBJDEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

include $(DEPTH)/mk/targets.mk

all:
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libcurl.a
	ln -s $(OBJDEPTH)/src/lib/curl/V7/7.15.4/$(DB_ARCH)/lib/libcurl.a $(DEPTH)/lib/$(OBJ_DIR)/

include $(DEPTH)/mk/depend.mk
