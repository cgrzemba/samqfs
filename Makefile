
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

# $Revision: 1.30 $

all:		what
linux:		what
linuxpkg:	what
clean:		what
depend:		what
install:	what
lint:		what
buildinfo:	what

what:
	@if [ -f StrikeOne ] ; then \
		echo "	Old habits die hard! (try \"gmake\")"; \
	else \
		echo "	We are sorry. The program you have executed, \"make\","; \
		echo "	has changed. The new program is \"gmake\"."; \
	fi
