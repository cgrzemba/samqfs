#!/bin/ksh

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

#
# This script is used by the robot daemon to keep a copy of ssi_so running.
# If this script exits, the daemon will start another.  If the site
# for whatever reason, has its own version of ssi, then this script
# should be modified to wait for a SIGTERM signal and then exit.  SIGTERM is
# the signal sent by the daemon to stop the process.
#
#
LOGGER=/usr/bin/logger
#
# find the syslog facility
if [ -r /etc/opt/SUNWsamfs/defaults.conf ] ; then
	SAM_LOGGER_FACILITY=`grep -v "^#" /etc/opt/SUNWsamfs/defaults.conf | grep LOCAL | sed -e "s/^.*LOG_//"`
	if [ x${SAM_LOGGER_FACILITY} == "x" ] ; then
		SAM_LOGGER_FACILITY=local7
	fi
else
	SAM_LOGGER_FACILITY=local7
fi

/usr/sbin/ping ${CSI_HOSTNAME} > /dev/null 2>&1 
if [ $? -ne 0 ] ; then
	${LOGGER} -i -t ssi.sh -p ${SAM_LOGGER_FACILITY}.warning "${CSI_HOSTNAME} is unpingable:  network problem\?"
fi
#
#
export CSI_TCP_RPCSERVICE=TRUE
export CSI_UDP_RPCSERVICE=TRUE
export CSI_CONNECT_AGETIME=172800
export CSI_RETRY_TIMEOUT=4
export CSI_RETRY_TRIES=5
export ACSAPI_PACKET_VERSION=4
export XAPI_CONVERSION=0
# /*********************************************************************/
# /* The following are for defining names for CDKLOG index             */
# /* positions to control log processing:                              */
# /* CDKLOG=1             Log event messages to the log file.          */
# /* CDKLOG=01            Log XAPI ACSAPI send packets to the log.file.*/
# /* CDKLOG=001           Log XAPI ACSAPI recv packets to the log.file.*/
# /* CDKLOG=0001          Log XAPI XML send packets to the log.file.   */
# /* CDKLOG=00001         Log XAPI XML recv packets to the log.file.   */
# /* CDKLOG=000001        Log CSI send packets to the log.file.        */
# /* CDKLOG=0000001       Log CSI recv packets to the log.file.        */
# /* CDKLOG=00000001      Log HTTP XML send packets to the log.file.   */
# /* CDKLOG=000000001     Log HTTP XML recv packets to the log.file.   */
# /* CDKLOG=0000000001    Log event error messages to stdout.          */
# /*===================================================================*/
export CDKLOG=1111111001
# /*********************************************************************/
# /* The following are for defining names for CDKTRACE index           */
# /* positions to control trace granuality.                            */
# /* CDKTRACE=1           Trace errors.                                */
# /* CDKTRACE=01          Trace ACSAPI.                                */
# /* CDKTRACE=001         Trace SSI.                                   */
# /* CDKTRACE=0001        Trace CSI.                                   */
# /* CDKTRACE=00001       Trace common components.                     */
# /* CDKTRACE=000001      Trace XAPI.                                  */
# /* CDKTRACE=0000001     Trace XAPI (client) TCP/IP functions.        */
# /* CDKTRACE=00000001    Trace t_acslm or t_http server executables.  */
# /* CDKTRACE=000000001   Trace HTTP (server) TCP/IP functions.        */
# /* CDKTRACE=0000000001  Trace malloc, free, and shared mem functions.*/
# /* CDKTRACE=00000000001 Trace XML parser.                            */
# /*===================================================================*/
# export CDKTRACE=11111111111
# export SLOGPIPE=/tmp/log.pipe
# export STRCPIPE=/tmp/trc.pipe
# arguments
# argv[1] - parent PID
# argv[2] - input socket name
# argv[3] - request originator type: 23=ACSSA (cl_type.c)
exec /opt/SUNWsamfs/sbin/ssi_so $3 ${ACSAPI_SSI_SOCKET} 23 
# /opt/SUNWsamfs/sbin/ssi_so $3 ${ACSAPI_SSI_SOCKET} 23 &
