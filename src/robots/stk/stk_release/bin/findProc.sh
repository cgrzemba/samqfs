#! /bin/sh
#  SccsId - @(#)findProc.sh	2.2 1/15/02 
# set -x
#
#  Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
#
#  Name:
#
#       findProc.sh
#
#  Description:
#
#       Shell script to find the the pid of the process named on the
#       command line.
#
#  Modified by:
#
#       S. L. Siao    02-May-2002 Modified ps flags 
#       S. L. Siao    13-May-2002 Made PS better able to handle
#                                 different types of ps.

PS='CANT FIND PS'
ps -ef >/dev/null 2>&1;
if [ $? = 0 ]; then
   PS='ps -ef';
else
  ps -augxw >/dev/null 2>&1;
  if [ $? = 0 ]; then
    PS='ps -augxw';
  fi;
fi;
PROCESS_STATUS=`$PS | grep "\<$1\>" | grep -v grep | grep -v findProc | grep -v dbx`

if [ "$PROCESS_STATUS" ]
then
	set $PROCESS_STATUS
	echo $2
else
	echo $PROCESS_STATUS
fi

