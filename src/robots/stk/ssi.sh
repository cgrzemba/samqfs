#!/bin/csh -f

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

#
# This script is used by the robot daemon to keep a copy of ssi_so running.
# If this script exits, the daemon will start another.  If the site
# for whatever reason, has its own version of ssi, then this script
# should be modified to wait for a SIGTERM signal and then exit.  SIGTERM is
# the signal sent by the daemon to stop the process.
#
#
# Find the correct logger
if ( -x /usr/bin/logger) then
	set LOGGER = /usr/bin/logger
else
	if ( -x /usr/ucb/logger ) then
		set LOGGER = /usr/ucb/logger
	endif
endif
#
# find the syslog facility
if ( -r /etc/opt/SUNWsamfs/defaults.conf ) then
	set SAM_LOGGER_FACILITY = `grep -v "^#" /etc/opt/SUNWsamfs/defaults.conf | grep LOCAL | sed -e "s/^.*LOG_//"`
	if ( x${SAM_LOGGER_FACILITY} == "x" ) then
		set SAM_LOGGER_FACILITY = local7
	endif
else
	set SAM_LOGGER_FACILITY = local7
endif

/usr/sbin/ping ${CSI_HOSTNAME} >&! /dev/null
if ( $status != "0" ) then
	${LOGGER} -i -t ssi.sh -p ${SAM_LOGGER_FACILITY}.warning ${CSI_HOSTNAME} is unpingable:  network problem\?
endif
#
#
setenv CSI_TCP_RPCSERVICE TRUE
setenv CSI_UDP_RPCSERVICE TRUE
setenv CSI_CONNECT_AGETIME 172800
setenv CSI_RETRY_TIMEOUT 4
setenv CSI_RETRY_TRIES 5
setenv ACSAPI_PACKET_VERSION 4
exec /opt/SUNWsamfs/sbin/ssi_so $3 ${ACSAPI_SSI_SOCKET} 23 

