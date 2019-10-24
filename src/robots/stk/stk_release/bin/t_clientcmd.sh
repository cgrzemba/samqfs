#!/bin/sh
#                                                               
# Copyright (c) 2011, 2012, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      t_clientcmd.sh                                      
#                                                               
# Description:    Test the ClientCmd interactive driver                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720         Joseph Nofi     09/15/11                       
#     Created for CDK 2.4 to add XAPI support.
# I7204183       Joseph Nofi     10/18/12                       
#     Changed default XAPI_CONVERSION and trace and log settings.
#                                                               
#===============================================================

#
# Clear the screen and display a message.
#
clear;
echo " =================================================="
echo " Start of t_clientcmd.sh CDK toolkit test driver:"
echo " =================================================="

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
# We set up the SSI.... 
# We assume that either a real server, or that
# t_acslm.sh, or t_http.sh simulated server is 
# listening at specified host. 
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
XAPI_CONVERSION=0  
export XAPI_CONVERSION                                              
XAPI_HOSTNAME=ECC21MVS
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
XAPI_TAPEPLEX=PRIMARY
export XAPI_TAPEPLEX
XAPI_SUBSYSTEM=HSC8
export XAPI_SUBSYSTEM

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


echo " "           
echo " Starting Common Trace and Log Services "
echo " =================================================="
echo " "           
./t_mini_el.sh 
./t_srvcommon.sh 
sleep 1

#    
# Don't allow any interrupts while starting the SSI.
#           
trap '' 1 2 3 15
    
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

#
# Set up to run the ClientCmd executable 
# in interactive mode.
#
MEDIA_TYPES_DIR=$CDK/dat    
export MEDIA_TYPES_DIR
echo " "
echo " MEDIA_TYPES_DIR=$MEDIA_TYPES_DIR"

echo " "
echo " Starting ./t_ClientCmd "
echo " =================================================="
./ClientCmd shell

#
# Done.
#
echo " "
echo " =================================================="
echo " End of t_ClientCmd"         

exit 0;
