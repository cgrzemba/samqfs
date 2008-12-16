# $Revision: 1.10 $

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

# GPL Notice
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; version 2.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software Foundation,
#      Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#      For the avoidance of doubt, except that if any license choice other
#      than GPL or LGPL is available it will apply instead, Sun elects to
#      use only the General Public License version 2 (GPLv2) at this time
#      for any software where a choice of GPL license versions is made
#      available with the language indicating that GPLv2 or any later
#      version may be used, or where a choice of which version of the GPL
#      is applied is otherwise unspecified.

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

DIRS = lib lsci

include $(DEPTH)/mk/targets.mk

install:
	$(INSTALL) $(SYSINST) lsci/ls $(USRDEST)/sls

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
lsci: 	lib

include $(DEPTH)/mk/depend.mk
