#! /bin/sh
#                                                               
# Copyright (c) 2011, 2011, Oracle and/or its affiliates.       
# All rights reserved.                                          
#                                                               
# File Name:      t_http.sh                                      
#                                                               
# Description:    XAPI test HTTP server executrable startup.                       
#                                                               
# Change History:                                               
#===============================================================
# ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION   
#     DESCRIPTION                                               
#===============================================================
# ELS720         Joseph Nofi     08/15/11                       
#     Created for CDK to add XAPI support.
#                                                               
echo "Attempting startup of $THTTP..."

#
# start up the HTTP test server 
#
$THTTP &

#
# save the Process ID so we can kill this process later with 
# the shell script t_killit.sh
#
curpid=$!
echo $curpid >> $T_KILL_FILE

exit 0


