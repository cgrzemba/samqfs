#!/bin/sh
#                                                               
# Copyright (c) 2011, 2012, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      t_testxapi.sh                                      
#                                                               
# Description:    XAPI client test script                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720         Joseph Nofi     05/15/11                       
#     Created for CDK 2.4 to add XAPI support.
# I7204183       Joseph Nofi     10/18/12                       
#     Changed default trace and log settings.
#                                                               
#===============================================================

#
# Clear the screen and display a message.
#
clear;
echo " =================================================="
echo " Start of t_testxapi.sh CDK toolkit test driver:"
echo " Cleaning up prior artifacts from t_testxapi.sh:"         
echo " =================================================="

#
# Kill the processes and erase the files created by 
# the previous iteration of this procedure.
#
./t_killit.sh

rm core
rm event.log
rm trace.log
rm trace.pipe
rm trace.file
rm log.pipe
rm log.file
rm trace.ssi.file
rm trace.client.file
rm tdout.file

self=`basename $0`;
version="2.4";
#
# Create an alias so Linux recognizes the \c for no-newline on echo.
#
if [ `uname` = "Linux" ]; then
    alias echo="echo -e"
fi
#
# Assign path to the executables if not already defined
# If not defined, set up to be the current dir.
#
if [ "$CDK" = "" ]; then
    CDKBIN=`pwd`
    export CDKBIN;
else
    CDKBIN=$CDK/bin
fi;

echo " "
echo " Executables live in: $CDKBIN"

#
# Set up the path for the t_kill_file for t_killit.sh.
#
T_KILL_FILE=$CDKBIN/t_kill_file  
export T_KILL_FILE

#
# Set up the environment variables for the ssi
#
CSI_TCP_RPCSERVICE="TRUE"    
export CSI_TCP_RPCSERVICE
CSI_UDP_RPCSERVICE="TRUE" 
export CSI_UDP_RPCSERVICE
CSI_CONNECT_AGETIME=172800   
export CSI_CONNECT_AGETIME
CSI_RETRY_TIMEOUT=4    
export CSI_RETRY_TIMEOUT
CSI_RETRY_TRIES=5      
export CSI_RETRY_TRIES
ACSAPI_SSI_SOCKET=50004         
export ACSAPI_SSI_SOCKET

#
# Set up for multi-host testing...
#
# We are definitely performing client hosting.... 
# Server may be on this machine or on another machine.
#
Host_name=ECC21MVS
CSI_HOSTNAME=$Host_name
export CSI_HOSTNAME
Ver_Num=4
ACSAPI_PACKET_VERSION=$Ver_Num
export ACSAPI_PACKET_VERSION
 
#
# Set up variables for XAPI testing...
#
# XAPI_CONVERSION=0    to set XAPI off (use "old" RPC ACSAPI)
# XAPI_CONVERSION=1    to enable XAPI (convert ACSAPI to XAPI)
# XAPI_HOSTNAME        for TCP/IP host of HTTP server
# XAPI_PORT            for TCP/IP port of HTTP listener
# XAPI_VERSION         for XAPI version of HTTP server
#
XAPI_CONVERSION=1  
export XAPI_CONVERSION
XAPI_HOSTNAME=ECC21MVS                                              
#XAPI_HOSTNAME=ironman2
export XAPI_HOSTNAME
XAPI_PORT=6788   
export XAPI_PORT
XAPI_VERSION=710   
export XAPI_VERSION

#
# Set up what tape devices the XAPI has access to...
#
# If XAPI_DRIVE_LIST is undefined, then the XAPI will add to
# its local configuration all drives returned from the XAPI
# TapePlex configuration query. 
#
#XAPI_DRIVE_LIST="9000-903F,9100-913F,R:00:00:09:00-R:00:00:09:19"   
#export XAPI_DRIVE_LIST

#                                                 
# Set up XAPI TAPEPLEX NAME and SUBSYSTEM...
#
# XAPI_TAPEPLEX        must match the Tapeplex name from HSC DI CDS
# XAPI_SUBSYSTEM       must match the z/OS HSC subsystem name
# 
#XAPI_TAPEPLEX=HSCQ
XAPI_TAPEPLEX=PRIMARY
export XAPI_TAPEPLEX
#XAPI_SUBSYSTEM=HSCQ
XAPI_SUBSYSTEM=HSC8
export XAPI_SUBSYSTEM

#
# Set up location of t_http canned response data files...
#
HTTP_RESPONSE_DIR="$CDK/dat" 
export HTTP_RESPONSE_DIR                                              

MYHOST=`hostname`
echo " "
echo " The local host is $MYHOST"
echo " "
echo " The XAPI server is "$XAPI_HOSTNAME
echo " It is assumed to be an SMC HTTP server at level "$XAPI_VERSION
echo " TapePlex name is "$XAPI_TAPEPLEX

echo " "
echo " The Remote ACSAPI server is "$CSI_HOSTNAME
echo " It is assumed to be an LibStation server"
echo " API version is "$ACSAPI_PACKET_VERSION
echo " "
echo " The t_http server response directory is "$HTTP_RESPONSE_DIR

sleep 3

#        
# Set up new trace and log control variables
#
#CDKTRACE=1             to trace errors 
#CDKTRACE=01            to trace ACSAPI
#CDKTRACE=001           to trace SSI
#CDKTRACE=0001          to trace CSI
#CDKTRACE=00001         to trace common_lib 
#CDKTRACE=000001        to trace XAPI 
#CDKTRACE=0000001       to trace XAPI TCP/IP (client) calls 
#CDKTRACE=00000001      to trace t_acslm or t_http server
#CDKTRACE=000000001     to trace t_http TCPIP (server) calls
#CDKTRACE=0000000001    to trace malloc() and free() calls 
#CDKTRACE=00000000001   to trace XMLPARSER 
#CDKTRACE=11111111100   normal test setting
CDKTRACE=00000000000
export CDKTRACE

#CDKLOG=1             to log event messages to the log.file 
#CDKLOG=01            to write error messages to stdout
#CDKLOG=001           to log XAPI ACSAPI send packets to the log.file
#CDKLOG=0001          to log XAPI ACSAPI recv packets to the log.file
#CDKLOG=00001         to log XAPI XML send packets to the log.file
#CDKLOG=000001        to log XAPI XML recv packets to the log.file
#CDKLOG=0000001       to log CSI ACSAPI send packets to the log.file
#CDKLOG=00000001      to log CSI ACSAPI recv packets to the log.file
#CDKLOG=000000001     to log HTTP XML send packets to the log.file
#CDKLOG=0000000001    to log HTTP XML recv packets to the log.file
#CDKLOG=1111111100    normal test setting
CDKLOG=0000000000
export CDKLOG

PARENT=$CDKBIN/t_parent
export PARENT 
MINI_EL=$CDKBIN/mini_el
export MINI_EL
SSI=$CDKBIN/ssi
export SSI
SRVTRCS=$CDKBIN/srvtrcs
export SRVTRCS
SRVLOGS=$CDKBIN/srvlogs
export SRVLOGS
THTTP=$CDKBIN/t_http
export THTTP


echo " "           
echo " Starting Common Trace and Log Services "
echo " =================================================="
echo " "           
./t_mini_el.sh 
./t_srvcommon.sh 
sleep 1

#    
# Don't allow any interrupts while starting the deamons.
#           
trap '' 1 2 3 15
    
#    
# If XAPI_CONVERSION is specified, and if the XAPI_HOSTNAME 
# is this host, then start the simulated HTTP server.
#           
CDRIVEROPTS=-rx
if [ $XAPI_CONVERSION = 1 ]; then
    if [ $XAPI_HOSTNAME = $MYHOST ]; then
        echo " "           
        echo " Starting Test Server $THTTP..."
        echo " =================================================="
        echo " "           
        ./t_http.sh
        CDRIVEROPTS=-cx
    fi
fi

#    
# Startup the SSI (with optional XAPI).
#           
echo " "           
echo " Starting $SSI..."
echo " =================================================="
echo " "           
./t_ssi.sh
    
#    
# Clear the trap on signals.
#    
trap 1 2 3 15

#
# Do a ps and show the user the Processes created.
#
sleep 3

ps -ef 2>&1 >/dev/null;
if [ $? = 0 ]; then
   PS='ps -ef';
else
   ps -augxw 2>&1 >/dev/null;
   if [ $? = 0 ]; then
      PS='ps -augxw';
   fi;
fi;
        
if [ `uname` = "Linux" ]; then
   PS='ps -elf';
fi

echo " "           
echo " Initialization completed: Processes created:"
$PS | grep $CDKBIN;

ACSAPI_USER_ID=
export ACSAPI_USER_ID

echo " "
echo " ACS USER_ID is "$ACSAPI_USER_ID
CDR_MSGLVL=0
export CDR_MSGLVL

#
# Set up to run the client driver.
#

echo " "           
echo " Starting ./t_cdriver "
echo " =================================================="
./t_cdriver $CDRIVEROPTS ;

#
# Done, wait 1 second for quiesce of server log and trace 
# processes.
#
sleep 1
echo " "
echo " =================================================="
echo " End of t_cdriver"         

#
# Comment out the following exit statement to enable 
# trace and log post processing.  
#
exit 0;

#
# Now slice the trace.file into SSI and CLIENT parts.
#
grep " SSI " trace.file > trace.ssi.file 
grep " CLIENT " trace.file > trace.client.file 

#
# Now run the trace_decode process on the log.file
# and create the tdout.file.
#
echo " "
echo " Executing ./trace_decode -c"         
echo " "
./trace_decode -c < log.file > tdout.file ;

exit 0;
