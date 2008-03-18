#!/bin/csh -f

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

# save_core.sh - template shell for processing exceptional events.
# $Revision: 1.9 $
#
# args:
#	1) calling program name (Not the notify wrapper!).
#	2) calling program pid (again, not the notify wrapper!).
#	3) keyword identifying the 'syslog' level of the event.
# 		The keywords are:
# 		emerg, alert, crit, err, warning, notice, info, debug
#	4) message number from the message catalog.
#	5) the actual message.
#
#

#  Save the core in /var/opt/SUNWsamfs/cores/core.<program>-<eq>.<date>

set SAMFSDIR=/var/opt/SUNWsamfs/amld
set SAMCOREDIR=/var/opt/SUNWsamfs/cores

if ( ! -d ${SAMCOREDIR} ) then
    mkdir -p ${SAMCOREDIR}
endif

set msg=($5)
set program=$msg[1]
set eq=$msg[2]
set date=`date +%m%d%y-%H:%M`

mv ${SAMFSDIR}/core ${SAMCOREDIR}/core.${program}-${eq}.${date}

