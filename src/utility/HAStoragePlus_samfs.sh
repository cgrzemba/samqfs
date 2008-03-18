#!/usr/bin/ksh -p
#
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
#
#ident  "$Revision: 1.7 $"
#
# HAStoragePlus_qfs is a script to obtain the constituent components
# of a single QFS family set or mount point.  It assumes that
# HAStoragePlus has already read /etc/vfstab and determined that
# the family set ("mount special") or mount point is described as
# a 'samfs' file system.
#
#	usage: HAStoragePlus_samfs {family_set}
#
#	usage: HAStoragePlus_samfs {mount_point}
#		-- only works if {mount_point} is a mounted QFS filesystem
#

#
# Global variables
#
typeset rc

#
# Assume existence of system utilities
#
typeset -r AWK=/usr/bin/awk
typeset -r PRINT=/usr/bin/print
typeset -r LOGGER=/usr/bin/logger
typeset -r BASENAME=/usr/bin/basename
typeset -r XARGS=/usr/bin/xargs
typeset -r ECHO=/usr/bin/echo
typeset -r PGREP=/usr/bin/pgrep

# sam-qfs CLI utility
typeset -r SAMCMD=/opt/SUNWsamfs/sbin/samcmd
typeset -r SAMD=/opt/SUNWsamfs/sbin/samd

# the syslog facility to use
typeset -r SYSLOG_FACILITY=daemon

# This script
typeset -r ARGV0=$($BASENAME $0)

# Assure presence of mount_point argument
#
if [[ -z "$1" ]] ; then
	$LOGGER -p ${SYSLOG_FACILITY}.err \
		 -t "QFS.[$ARGV0]" "usage: $ARGV0 { mount_point }"
	exit 1
else
	typeset mount_point=$1
fi

$LOGGER -p ${SYSLOG_FACILITY}.info \
	-t "QFS.[$ARGV0]" "Obtain QFS volumes for mount point: $mount_point"

#
# Invoke SAMD to assure that the sam-fsd daemon is up and running
#
$PGREP "sam\-fsd" > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
	$SAMD config > /dev/null 2>&1
fi

#
# Perform the samcmd once to determine if the mount point exists
#
$SAMCMD N $mount_point > /dev/null 2>&1
rc=$?
if [[ $rc -ne 0 ]]; then
	$LOGGER -p ${SYSLOG_FACILITY}.err -t "QFS.[$ARGV0]" \
		"Failure to obtain QFS volumes for mount point: $mount_point"
	exit $rc
fi

#
# Perform the samcmd again to parse for QFS volumes
# NOTE: Only supports QFS volumes of the types 'mm', 'md', 'mr', and 'gNNN'
#
$SAMCMD N $mount_point 2>/dev/null | \
	$AWK '/^g[0-9]*+[ 	]|^m[dmr][	 ]/ { print $4 }' |\
	$XARGS |\
	$AWK ' { for (i = NF; i >=2; i--) { printf $i "," } print $i } '
exit 0
