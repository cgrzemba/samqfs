#
# Copyright (C) 1993,2011, Oracle and/or its affiliates. All rights reserved.
#
#  Module:
#
#       Toolkit Master Makefile
#
#  Environment:
#
#       Currently Non UNIX Platform specific, 
#       Tried on SUNs, Bulls, and IBM-AIX machines.
#
#  Implicit Inputs: 
#   
#       All makefiles in the subdirectories read the file 
#       make.ini which lives in the toolkit directory etc.
#       The make.ini file may need to be modified for different
#       UNIX platforms
#
#  Modified by:
#       Ken Stickney    20-AUG-1993     added acsipc target
#       Ken Stickney    04-JAN-1993     default if CSC_HOST and 
#                                       SVR_HOST variables undefined
#       H. I. Grapek    07-Jan-1994     Call mini_el not acsel
#                                       Misac cleanup for EC.
#       H. I. Grapek    09-Jan-1994     no longer using CSC_HOST or SVR_HOST.
#                                       Simplified.
#       H. I. Grapek    15-Jan-1994     Mods make to conform to Documentation.
#       Mike Williams   18-May-2010     Added the ClientCmd directory to the
#                                       list of directories to build.
#       Mike Williams   04-Nov-2010     Removed ClientCmd directory from the
#                                       list of directories to builds.
#       Joseph Nofi     15-Jun-2011     Added http, xapi, srvlogs, and srvtrcs
#                                       to list of directories to build.
#       Joseph Nofi     11-Sep-2012     Add variable directory list for Makefile
#                                       based upon CDK_BUILD_OPTION variable.
#

# Note: The shell script toolkit/src/toolkitEnvironment.sh must be modified and
# sourced into your environment prior to executing the make utility.
#
# The following environment variables must be set:
#   CDK - set the path to the toolkit
#   PLATFORM - Linux, i386, or sparc
#   CDK_BUILD_OPTION - BASE or XAPI
#

# All of the directories in the correct order for CDK base
DIR_LIST_BASE = acsapi acsipc acsutl common_lib xdr mini_el csi ssi client \
	server trace_decode daemon srvlogs srvtrcs

# All of the directories in the correct order for CDK with XPI support
DIR_LIST_XAPI = acsapi acsipc acsutl common_lib xapi xdr mini_el csi ssi client \
	server trace_decode daemon srvlogs srvtrcs http

#
# Set up the targets.
#
	
nondebug:
	for d in $(DIR_LIST_$(CDK_BUILD_OPTION)); do \
	( cd $$d; make top_nondebug ) \
	done \
	
debug:
	for d in $(DIR_LIST_$(CDK_BUILD_OPTION)); do \
	( cd $$d; make top_debug ) \
	done \
	
sterile:
	for d in $(DIR_LIST_$(CDK_BUILD_OPTION)); do \
	( cd $$d; make top_sterile ) \
	done 
