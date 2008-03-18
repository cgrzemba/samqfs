#!/bin/sh

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
#   script to reload files from SAMFS archive tapes
#
#   edit the script as follows:
#
#   adjust STAR, LOAD, and UNLOAD if necessary
#   set a tape drive to "unavail" status using samu or samcmd
#   set EQ to that tape drive's mcf equipment number
#   set TAPEDRIVE to the raw path for the above equipment
#   set BLOCKSIZE in 512 byte units (double what the display shows)
#   set MEDIATYPE to the two-letter media type for these VSNs
#   change "VSNA" etc. to your list of VSNs, continue lines with backslash
#
STAR="/opt/SUNWsamfs/sbin/star"
LOAD="/opt/SUNWsamfs/sbin/load"
UNLOAD="/opt/SUNWsamfs/sbin/unload"
EQ=28
TAPEDRIVE="/dev/rmt/3cbn"
# BLOCKSIZE is in units of 512 bytes (e.g. 256 for 128K)
BLOCKSIZE=256
MEDIATYPE="lt"

# VSN_LIST="VSNA VSNB VSNC \
#  VSNZ"
VSN_LIST="CEL034"

for vsn in $VSN_LIST
do
    echo "Unloading tape from $EQ"
    $UNLOAD -w $EQ
    echo "Loading tape $vsn in $EQ"
#   $LOAD -w -vsn $vsn $EQ            for SAMFS 331 and below
#   $LOAD -w $MEDIATYPE.$vsn $EQ      for SAMFS 350 and above
    $LOAD -w $MEDIATYPE.$vsn $EQ
    echo "Skipping past label on $vsn"
    count=0

#   while last command succeeded, run star
    while [ $? -eq 0 ]
    do
		/bin/mt -f $TAPEDRIVE fsf
        count=`expr $count + 1`
        echo "Beginning data retrieval on $vsn tarfile $count"
	    $STAR xvnbf $BLOCKSIZE $TAPEDRIVE
    done
	echo "Finished extracting $count tarfiles from VSN $vsn"
done

echo "tarback script finished"

