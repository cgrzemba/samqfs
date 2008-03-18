# $Id: GNUmakefile,v 1.3 2008/03/17 14:44:10 am143972 Exp $

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

DEPTH = ../../../../..
OBJDEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

include $(DEPTH)/mk/targets.mk

all:
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libssl.a
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libcrypto.a
	ln -s $(OBJDEPTH)/src/lib/ssl/V9/9.7/$(DB_ARCH)/lib/libcrypto.a $(DEPTH)/lib/$(OBJ_DIR)/
	ln -s $(OBJDEPTH)/src/lib/ssl/V9/9.7/$(DB_ARCH)/lib/libssl.a $(DEPTH)/lib/$(OBJ_DIR)/

include $(DEPTH)/mk/depend.mk
