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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../../../..
OBJDEPTH = ../../..

include $(DEPTH)/mk/common.mk

#BUILD_64BIT = yes

include $(DEPTH)/mk/targets.mk

all:
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient.so
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient.so.15
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient_r.so
	rm -f $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient_r.so.15
	ln -s $(OBJDEPTH)/src/lib/mysql/V5/5.0.51a/$(MYSQL_ARCH)/lib/libmysqlclient.so $(DEPTH)/lib/$(OBJ_DIR)/
	ln -s $(OBJDEPTH)/src/lib/mysql/V5/5.0.51a/$(MYSQL_ARCH)/lib/libmysqlclient.so $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient.so.15
	ln -s $(OBJDEPTH)/src/lib/mysql/V5/5.0.51a/$(MYSQL_ARCH)/lib/libmysqlclient_r.so $(DEPTH)/lib/$(OBJ_DIR)/
	ln -s $(OBJDEPTH)/src/lib/mysql/V5/5.0.51a/$(MYSQL_ARCH)/lib/libmysqlclient_r.so $(DEPTH)/lib/$(OBJ_DIR)/libmysqlclient_r.so.15

include $(DEPTH)/mk/depend.mk
