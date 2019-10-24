#! /bin/sh
#  SccsId - 5/26/94 @(#)t_mini_el.sh	2.01A
#
#  Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
#
# Name:
#
#	t_mini_el.sh
#
# Description:
#
#	This shell sets up a single mini event logger for CSI testing.
#
# Considerations:
#
#	Depends on the envrionment variables set in the 
#	"t_startit.sh' scripts.
#
# Modifications:
#	
#	H. I. Grapek	04-Jan-1994	Original
#	H. I. Grapek	07-Jan-1994	Cleanup for Toolkit 2.0
#       K. J. Stickney  26-May-1994     Add exit for stand alone 
#                                       execution in ksh.
#

echo "Attempting startup of $MINI_EL ..."

#
# start up the MINIEL
#
$MINI_EL &

#
# save the Process ID so we can kill this process later with 
# the shell script t_killit.sh
#
curpid=$!
echo $curpid >> $T_KILL_FILE

exit 0
