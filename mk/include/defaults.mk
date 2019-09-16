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

#	defaults.mk - default build parameters
#
#	Variables are assigned via on of three ways:
#
#		Assignment on the make line. For example:
#			make DEBUG=off
#		Assignment on the make line overrides any definitions in
#			the *.mk files.
#
#		Definition in the CONFIG.mk file. This file is created by:
#			make config <par1=value1> ...
#		Definition in CONFIG.mk overrides definitions in defaults.mk.
#
#		Default assignment in defaults.mk.
#
#	DO NOT CHANGE THIS FILE UNLESS DEFAULT BEHAVIOR IS BEING MODIFIED.

#	Assign default values to variables that have not been defined.

DEBUG ?= yes
COMPLETE ?= no
BUILD_64bit = $(BUILD_64BIT)
