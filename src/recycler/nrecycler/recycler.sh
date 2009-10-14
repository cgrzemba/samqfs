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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

#
#   /etc/opt/SUNWsamfs/scripts/nrecycler.sh - post-process a VSN after recycler has
#   drained it of all known active archive copies.
#
#   Arguments are:
#      $1 - generic media type "od" or "tp" - used to construct the name
#           of the appropriate label command: odlabel or tplabel
#
#      $2 - VSN being post-processed
#
#      $3 - MID in the library where the VSN is located
#
#      $4 - equipment number of the library where the VSN is located
#  
#      $5 - actual media type ("mo", "lt", etc.) - used to chmed
#           the media if required
#
#      $6 - family set name of the physical library, or the string
#           "hy" for the historian library.    This can be used to
#           handle recycling of off-site media, as shown below.
#
#	   $7 - VSN modifier, used for optical and D2 media
#
#
# $Revision: 1.6 $
#

#   It is a good idea to log the calls to this script
#echo `date` $* >>  /var/opt/SUNWsamfs/nrecycler.sh.log

#   As an example, if uncommented, the following lines will relabel the VSN,
#   if it exists in a physical library.  If the VSN is in the historian
#   catalog (e.g., it's been exported from a physical library and moved
#   to off-site storage), then email is sent to "root" informing that the
#   medium is ready to be returned to the site and reused.
#
#set stat=0
#if ( $6 != hy ) then
#    /opt/SUNWsamfs/sbin/chmed -R $5.$2
#    /opt/SUNWsamfs/sbin/chmed -W $5.$2
#    if ( $1 != "od" ) then
#        /opt/SUNWsamfs/sbin/${1}label -w -vsn $2 -old $2 $4\:$3
#		if ( $status != 0 ) then
#		    set stat = 1
#		endif
#    else
#        /opt/SUNWsamfs/sbin/${1}label -w -vsn $2 -old $2 $4\:$3\:$7
#		if ( $status != 0 ) then
#		    set stat = 1
#		endif
#    endif
#else
#    mail root <</eof
#VSN $2 of type $5 is devoid of active archive
#images.  It is currently in the historian catalog, which indicates that
#it has been exported from the on-line libraries.
#
#You should import it to the appropriate library, and relabel it using
#${1}label.
#
#This message will continue to be sent to you each time the nrecycler
#runs, until you relabel the VSN, or you use the SAM-FS samu or SAM-QFS
#Manager programs to export this medium from the historian catalog to
#suppress this message.
#/eof
#endif
#echo `date` $* done >>  /var/opt/SUNWsamfs/nrecycler.sh.log
#if ( $stat != 0 ) then
#	exit 1
#else
#	exit 0
#endif
#
#
#   These lines would inform "root" that the VSN should be removed from the
#   robotic library:
#
#mail root <</eof 
#VSN $2 in library $4 is ready to be shelved off-site.
#/eof
#echo `date` $* done >>  /var/opt/SUNWsamfs/nrecycler.sh.log
#exit 0


#  The default action is to mail a message reminding you to set up this 
#  file.  You should comment out these lines (through and including the /eof
#  below) after you've set up this file.
#
/bin/ppriv -s I=basic -e /usr/bin/mailx -s "Robot $6 at hostname `hostname` recycle." root <</eof
The /etc/opt/SUNWsamfs/scripts/nrecycler.sh script was called by the SAM-FS nrecycler
with the following arguments:

      Media type: $5($1)  VSN: $2  Slot: $3  Eq: $4 
      Library: $6

/etc/opt/SUNWsamfs/scripts/nrecycler.sh is a script which is called when the nrecycler
determines that a VSN has been drained of all known active archive
copies.  You should determine your site requirements for disposition of
recycled media - some sites wish to relabel and reuse the media, some
sites wish to take the media out of the library for possible later use
to access historical files.  Consult the nrecycler(1m) man page for more
information.
/eof
#echo `date` $* done >>  /var/opt/SUNWsamfs/nrecycler.sh.log
exit 0
