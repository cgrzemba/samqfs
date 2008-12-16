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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

# samcronfix.sh - Shell script to change crontab entries
# $Revision: 1.10 $
#
# Script to update crontab. Takes two arguments:
# 1) String identifying entries in crontab to be removed
# 2) Location of file containing new entry
#
#
RM=/usr/bin/rm
if [ "$1" = "" ]
then
   echo Missing old-entry argument to crontab changing script
   exit
fi

if [ ! -z "$2" ]
then
    # Remove previous entry from crontab and add new entry
    crontab -l | grep -v $1 | cat - $2 | crontab
else
    # Just remove the entry
    crontab -l | grep -v $1 | crontab
fi

# Remove temporary file which was used to pass contents of new entry
if [ ! -z "$2" ]
then
    ${RM} $2
fi
