#! /bin/sh

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
#	log_rotate.sh logfile
#
#   If the current logfile is larger than 100000 bytes, log_rotate.sh
#	moves the current logfile to logfile.0 and rotates former logs
#	accordingly.  If logfile.7 exists it is lost.
#   The minimum size (default 100000 bytes) may be supplied as the 
#   second argument.
#
#	ex:  log_rotate.sh /var/adm/sam-log
#	ex:  log_rotate.sh /var/adm/sam-log  30000
#
#	in a crontab you could do the following:
#
#	10 3 * * 0  /etc/opt/SUNWsamfs/scripts/log_rotate.sh /var/adm/sam-log 
#	20 3 * * 0  /bin/kill -HUP `/bin/cat /etc/syslog.pid`
#
LOG=$1
MINSIZE=${2:-100000}
#
if test -s $LOG
then
	BLKS=`/bin/ls -s $LOG | /bin/awk '{print $1}'`
	SIZE=`expr $BLKS \* 512`
	if [ $SIZE -ge $MINSIZE ]
	then
		test -f $LOG.6 && mv $LOG.6  $LOG.7
		test -f $LOG.5 && mv $LOG.5  $LOG.6
		test -f $LOG.4 && mv $LOG.4  $LOG.5
		test -f $LOG.3 && mv $LOG.3  $LOG.4
		test -f $LOG.2 && mv $LOG.2  $LOG.3
		test -f $LOG.1 && mv $LOG.1  $LOG.2
		test -f $LOG.0 && mv $LOG.0  $LOG.1
		mv $LOG    $LOG.0
		cp /dev/null $LOG
		chmod 644    $LOG
	fi
fi
