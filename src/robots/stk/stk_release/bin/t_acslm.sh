#! /bin/sh
#  SccsId - @(#)t_acslm.sh	1.1 1/10/94 
#
#  Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
#
# Name:
#
#	t_acslm.sh
#
# Description:
#
# 	execute the acslm process and save the appropriate PIDs in
# 	the killit file to be able to automatically killed later.
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
#

#
# Start up the parent process (daemon)
#
echo "Attempting startup of PARENT for $ACSLM..."

$PARENT &
curpid=$!
sleep 2

#
# Start up the ACSLM process (stub)
#
echo "Attempting startup of $ACSLM..."
$ACSLM $curpid 50003 14 &

#
# save the PID for killing later
#
curpid=$!
echo $curpid >> $T_KILL_FILE

#
# wait for the SIGHUP
#
sleep 2

exit 0
