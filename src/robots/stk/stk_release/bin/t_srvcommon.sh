# SccsId - @(#)t_srvcommon.sh  
#                                                               
# Copyright (c) 2011, 2011, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      t_srvcommon.sh                                      
#                                                               
# Description:    Start common trace and log services.                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720         Joseph Nofi     11-May-2011                       
#     Created for CDK to add XAPI support.
#                                                               
#===============================================================

echo "Attempting startup of $SRVTRCS..."

#
# start up the trace server 
#
$SRVTRCS &

#
# save the Process ID so we can kill this process later with 
# the shell script t_killit.sh
#
curpid=$!
echo $curpid >> $T_KILL_FILE
SRVTRCSPID=$curpid
export SRVTRCSPID

echo "Attempting startup of $SRVLOGS..."

#
# start up the log server
#
$SRVLOGS &

#
# save the Process ID so we can kill this process later with 
# the shell script t_killit.sh
#
curpid=$!
echo $curpid >> $T_KILL_FILE
SRVLOGSPID=$curpid
export SRVLOGSPID

exit 0
