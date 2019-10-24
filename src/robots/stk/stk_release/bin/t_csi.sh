#! /bin/sh
#  SccsId - @(#)t_csi.sh	1.1 1/10/94
#
#  Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
#
# Name:
#
#	t_csi.sh
#
# Description:
#
#	This shell initiates the CSI only.
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
#       K. J. Stickney  26-May-1994     Added setting of CSI specific
#                                       environment variables for true
#                                       stand alone functionality.
#

#
# Start up the parent process (daemon)
#
echo "Attempting startup of PARENT for $CSI..."

$PARENT &
curpid=$!
sleep 2

#
# Start up the CSI process 
#
echo "Attempting startup of $CSI..."

if [ "$CSI_TCP_RPCSERVICE" = "" ]; then
    CSI_TCP_RPCSERVICE="TRUE";
    export CSI_TCP_RPCSERVICE;
fi;
if [ "$CSI_UDP_RPCSERVICE" = "" ]; then
    CSI_UDP_RPCSERVICE="TRUE";
    export CSI_UDP_RPCSERVICE;
fi;
if [ "$CSI_CONNECT_AGETIME" = "" ]; then
    CSI_CONNECT_AGETIME=172800;
    export CSI_CONNECT_AGETIME;
fi;
if [ "$CSI_RETRY_TIMEOUT" = "" ]; then
    CSI_RETRY_TIMEOUT=4;
    export CSI_RETRY_TIMEOUT;
fi;
if [ "$CSI_RETRY_TRIES" = "" ]; then
    CSI_RETRY_TRIES=5;
    export CSI_RETRY_TRIES;
fi;

$CSI $curpid ANY_PORT 6 &

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
