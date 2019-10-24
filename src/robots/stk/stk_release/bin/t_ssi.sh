#! /bin/sh
#  SccsId - 05/26/94 @(#)t_ssi.sh   2.0A 
#
#  Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
#
# Name:
#
#  t_ssi.sh
#
# Description:
#
#  This shell initiates the SSI only.
#
# Considerations:
#
#  Depends on the envrionment variables set in the 
#  "t_startit.sh' scripts.
#
# Modifications:
#  
#       H. I. Grapek    04-Jan-1994     Original
#       H. I. Grapek    07-Jan-1994     Cleanup for Toolkit 2.0
#       K. J. Stickney  26-May-1994     Added setting of SSI specific
#                                       environment variables for true
#                                       stand alone functionality.
#       Mitch Black     17-Dec-2004     Added ACSAPI_SSI_SOCKET for use
#                                       on SSI execution line.  If not the default
#                                       of 50004, it must be set in the t_cdriver
#                                       environment prior to running t_cdriver, so
#                                       that it can communicate with this invocation
#                                       of the SSI.
#       Joseph Nofi     15-Jul-2011     Added XAPI_CONVERSION environment variable.
#

#
# Start up the parent process (daemon)
#
echo "Attempting startup of PARENT for $SSI..."

$PARENT &
curpid=$!
sleep 2

#
# Start up the SSI process 
#
echo "Attempting startup of $SSI..."

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
if [ "$ACSAPI_SSI_SOCKET" = "" ]; then
    ACSAPI_SSI_SOCKET=50004;
    export ACSAPI_SSI_SOCKET;
fi;
if [ "$XAPI_CONVERSION" = "" ]; then
    XAPI_CONVERSION=0;
    export XAPI_CONVERSION;
fi;

if [ "$CSI_HOSTNAME" = "" ]; then
while [ 1 -eq 1 ]; do
    echo "Would you like Multi-Host or Single-Host testing?"
    echo
    echo "    Enter one of the following followed by ENTER: "
    echo
    echo "    M         Multi-host testing"
    echo "    S         Single-host testing"
    echo "    X         eXit this script"
    echo
    echo -n "Enter choice: "
    read answer;
    echo ""
 
    case $answer in
        M | m )         # Setting up for multi-host testing...
 
            while [ 1 -eq 1 ]; do
                echo "The Remote Host Name is the name of the server which"
                echo "has the ACSLS software (or simulator) running on it."
                echo
                echo -n \
                "Enter Remote Host Name (CSI_HOSTNAME): ";
                read Host_Name;
                if [ $Host_Name ]; then
                    CSI_HOSTNAME=$Host_Name;        export CSI_HOSTNAME
                    break;
                else
                    echo
                    echo "No Host Name entered, try again."
                    echo
                fi;
            done;
           break;
            ;;
 
        S | s )         # Setting up for SINGLE-HOST Testing...
 
            CSI_HOSTNAME=`hostname`;            export CSI_HOSTNAME
            break;
            ;;
 
        * )
            echo; echo "Please Enter 'M', 'S' or 'X'... try again. "
            echo
            ;;
    esac;
done;
fi;
 

$SSI $curpid $ACSAPI_SSI_SOCKET 23 &

#
# save the PID for killing later
#
curpid=$!
echo $curpid >> $T_KILL_FILE

#
# wait for the SIGHUP
#
sleep 2

echo "SSI startup=$SSI $curpid $ACSAPI_SSI_SOCKET 23 &"

exit 0
