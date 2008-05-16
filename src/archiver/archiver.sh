#!/bin/sh

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

# archiver.sh - template shell for processing archiver exceptional events.
# $Revision: 1.9 $
#
# args:
#	1) calling program name
#	2) calling program pid
#	3) keyword identifying the 'syslog' level of the event.
# 		The keywords are:
# 		emerg, alert, crit, err, warning, notice, info, debug
#	4) message number from the message catalog.
#	5) the actual message.
#
#

case $3 in
	alert|crit|emerg|err)
		# These lines inform "root" that a serious error occured.
		mail root <<EOF
The SAM-FS $1 detected a level "${3}" exception condition.
Message number ${4}:
${5}
EOF
		;;

    warning)
		# Do not send default message for these, archiver already sent it.
		if [ $4 = 4020 ]; then
			#%s[%d] died - Not restarting
			exit
		fi

		if [ $4 = 4023 ]; then
			#Errors in archiver commands - no archiving will be done.
			exit
		fi
		;;

esac

#  The default action is to send the message to syslog

/usr/bin/logger -p local7.$3 -t "$1[$2]" $3\($4\) "$5"
