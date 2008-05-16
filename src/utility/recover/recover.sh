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

#  Script to read an archiver log and generate a series of 
#     /opt/SUNWsamfs/bin/request -p XXX file
#     /opt/SUNWsamfs/sbin/star xvfb file NN dir/dir/file1 dir/file2 ...
#  commands.
#
#  $1 is the mount point of the samfs file system.
#
#  HOW TO USE:
#
#    1)  Extract those entries from the archiver log file(s) which
#	    you need to restore from archive media.  Note that you should
#	    eliminate second-copy (and third- and fourth-copy) entries from
#	    this file, or else the files will be recovered multiple times,
#	    which wastes time.  After this editing, save to a file, say
#	    "/tmp/arlog.in"
#    
#    2)  Modify the values below as needed:
#              SAM_TEMP - a temporary directory where the request
#                         files will be created.  This must
#                         be a directory in a SAM-FS filesystem.
#                         The default is /<mount_point>/tmp.
#
#              BLK_SIZE - The block size of your media.  As written,
#                         this value must be a constant for all your
#                         media.  If you have different sizes, you'll
#                         need to run the script once for each type.
#                         The example below uses 128 kilobyte blocks.
#
#    3)  Run this script, supplying the file /tmp/arlog.in from step
#        1, as standard input.  The first argument to the script is the
#        SAMFS file system mount point.   Save the script's output to a file.
# 
#             /tmp/recover.sh  /sam1  < /tmp/arlog.in > /tmp/recover.out
#
#    4)  The file /tmp/recover.out is a shell script which can be
#        run to recover all the files listed in the /tmp/arlog.in file.
#        First, cd to the directory in which you want the files recovered.
#        Although this could be your mount point, it's probably better to
#        recover to a temporary directory first, then move the files to
#        their final location once recovery is complete and everything
#        looks OK.
#   
#             sh -x /tmp/recover.out
# 
#

#  Each line is read from the log file.  If the vsn and position of this
#  line is the same as the previous line, the filename at the end of the
#  log entry is appended to the list of files which must be extracted from
#  that vsn and position.   If the vsn or position differ from the previous
#  line, then an star command is emitted (if the list of files is non-null),
#  and a request command is generated.

if [ x$1 = x ]; then
     echo usage: $0 /mount_point
     exit 1
fi
SAM_MOUNT=$1
BLK_SIZE=128

# nawk script to generate the actual recover script.
# Note:  will not work for filenames containing two or more consecutive spaces
# Note:  the awk variables must be escaped.

cat << /EOF > /tmp/$$.awk
BEGIN {
   gnutar_arg = $BLK_SIZE * 2;

   SAM_TEMP = "$SAM_MOUNT" "/tmp"
   print "/bin/mkdir -p " SAM_TEMP

   prev_vsn = "";
   prev_pos = "";
   files = "";
   seq=0;
}

{  
   mt = \$4;
   vsn = \$5;
   posoff = \$7;
   fullfile = "";
   
   split(posoff, p, ".");
   pos = p[1]
   off = p[2]
   if ( prev_vsn != vsn || prev_pos != pos ) {
       if ( files != "" ) {
          print "/opt/SUNWsamfs/sbin/star xvbf " gnutar_arg " " SAM_TEMP "/request" seq " " files
          seq ++;
          files = ""
       }
       if ( prev_vsn != vsn ) {	
           print "#  ----------- end of files for vsn " prev_vsn " ---------"
       }
       print "/bin/rm -f " SAM_TEMP "/request" seq
       print "/opt/SUNWsamfs/bin/request -p 0x" pos " -m " mt " -v " vsn " " SAM_TEMP "/request" seq
		
       prev_vsn = vsn;
       prev_pos = pos;
   }
#
#  collect a filename containing spaces and put single quotes around it.
#  put each filename on a separate line with continuation so lines are short.
#  Note:  will not work for filenames containing two or more consecutive spaces
#
#  Due to change in archiver log format,
#    for 331 and below: (0 items after path) path is $11 through NF
#    for 350          : (2 items after path) path is $11 through NF-2
#    for 351 and above: (3 items after path) path is $11 through NF-3
#
#  escape "$" in file names
   gsub(/\\\$/, "\\\\\$")
   fullfile = \$11
   if ( NF-3 > 11 ) {
       for (i = 12; i <= NF-3; i++) fullfile = fullfile " " \$i
       files = files " \"" fullfile "\" \\\\\n"
   } else {
       files = files " "  fullfile " \\\\\n"
   }
}
END{
       if( files != "  \\\\\n" ) {
          print "/opt/SUNWsamfs/sbin/star xvbf " gnutar_arg " " SAM_TEMP "/request" seq,  files
       }
}
/EOF

/bin/nawk -f /tmp/$$.awk
/bin/rm /tmp/$$.awk
