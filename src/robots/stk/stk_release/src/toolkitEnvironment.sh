#!/usr/bin/ksh
#
# Copyright (C) 2010,2012, Oracle and/or its affiliates. All rights reserved.
#
# File:   toolkitEnvironment.sh
#
# Author: mw188955
#
# Created on November 4, 2010, 4:42 PM
#
# Modified by:
#       Joseph Nofi     11-Sep-2012     Added CDK_BUILD_OPTION for XAPI build
#                                       and variable Makefile directory list
#

# set the path to the CDK
CDK=/home/jn189587/toolkit7.2/CDK-7.2
#CDK=/home/severn/mw188955/dotsero/subversion/cdk/toolkit
export CDK

# intialize the CDK_BUILD_OPTION environment variable to either BASE or XAPI
CDK_BUILD_OPTION=BASE
export CDK_BUILD_OPTION

# intialize the PLATFORM environment variable
PLATFORM=undefined
export PLATFORM

# check to see if this is a linux platform
if [[ `uname` == "Linux" ]]; then
  PLATFORM=linux64
  export PLATFORM
fi

# check to see if this is a Solaris x86 platform
if [[ `uname -p` == "i386" ]]; then
  PLATFORM=x64
  export PLATFORM
  PATH=$PATH:/usr/ucb/bin:/usr/ccs/bin
  export PATH
fi

# check to see if this is a Solaris SPARC platform
if [[ `uname -p` == "sparc" ]]; then
  PLATFORM=sparc64
  export PLATFORM
  PATH=$PATH:/usr/ucb/bin:/usr/ccs/bin 
  export PATH
#  LD_LIBRARY_PATH_64=/usr/ucblib/sparcv9
#  export export LD_LIBRARY_PATH_64
fi

#
# If you start up t_cdriver outside of this script,
# you must set the following environment variables
# prior to launching t_cdriver:
# syntax for running t_cdriver is 
#
# ACSAPI_PACKET_VERSION <The max ACSLS server packet version>
# ACSAPI_USER_ID <control access id or arbitrary string>
# ACSAPI_SSI_SOCKET <IPC socket for ACSAPI-SSI communication>
ACSAPI_PACKET_VERSION=4
ACSAPI_USER_ID=" "
ACSAPI_SSI_SOCKET=50004
export ACSAPI_PACKET_VERSION ACSAPI_USER_ID ACSAPI_SSI_SOCKET
# syntax for running t_cdriver is: 
# t_cdriver -options
#     Valid options are:
#       c = Check Mode (check returned values against hard-coded stub values)
#       l = Library Station (tests running against LibStation server)
#       r = Real Server (tests running against a real server)
# example: t_cdriver -r

# set ClientCmd environment variable for tempory files
MEDIA_TYPES_DIR=/tmp
export MEDIA_TYPES_DIR

echo CDK=$CDK
echo CDK_BUILD_OPTION=$CDK_BUILD_OPTION
echo PLATFORM=$PLATFORM
#echo LD_LIBRARY_PATH_64=$LD_LIBRARY_PATH_64
echo MEDIA_TYPES_DIR=$MEDIA_TYPES_DIR
echo ACSAPI_PACKET_VERSION=$ACSAPI_PACKET_VERSION
echo ACSAPI_USER_ID=\"$ACSAPI_USER_ID\"
echo ACSAPI_SSI_SOCKET=$ACSAPI_SSI_SOCKET
