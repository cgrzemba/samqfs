#!/bin/sh
#  SccsId - @(#)t_startit.sh  1.5 2/10/94 
#
#  Copyright (1993, 2012) Oracle and/or its affiliates.  All rights reserved.
#
#  Name:
#
#       t_startit.sh
#
#  Description:
#
#  * startup script for a V4 client ... multi host 
#    and single host
#
#  * For testing out the toolkit.
#
#  * Clear the screen, and display a nice menu
#
#  * For Single Host: 
#     It is assumed that there is NO running simulator, or NO
#     live system on the current host:
#
#     * Set up CSI_HOSTNAME to be the machine name 
#       of the current host (hostname).
#
#     * execute the acsel process and save the appropriate 
#       PIDs in the killit file to be able to automatically 
#       killed later.
#
#     * Trap for signals
#
#     * execute the SSI, CSI, t_acslm and save the PIDS in the 
#       killit file.
#
#     * disarm signals
#
#
#  * For Multiple Host: 
#     It is assumed that there is a running simulator or t_acslm 
#     on another host:
#
#     * Set up CSI_HOSTNAME to be the machine name 
#       which has a simulator or t_acslm runnung.
#
#     * Trap for signals
#
#     * execute the acsel process and save the appropriate 
#       PIDs in the killit file to be able to automatically 
#       killed later.
#
#     * execute the SSI
#
#     * disarm signals
#
#  * Ask if the user wishes to see the processes created? by
#    this script. If he answers yes, do a PS and display them.
#
#  * Ask if the user wishes to run the t_cdriver.
#
#  * Exit Cleanly
#
#  Executables: 
#     * SSI     - the ssi compiled with the toolkit
#     * CSI     - the csi compiled with the toolkit
#     * MINI_EL - the event logger compiled with the toolkit
#        * ACSLM    - the t_acslm stub... mimics the real ACSLM.
#     * PARENT  - the t_parent deamon which starts the execs.
#           This is used to start all processes except
#           the mini_el.
#
#  Exit Values:
#
#  0  Successful completion.
#
#  Inputs:
#
#  Answers to User Questions
#
#  Outputs:
#
#  Assorted informational messages.
#
#  Environment:
#
#  Bourne Shell, simply like when the toolkit is unloaded.
#
#  Modified by:
#
#       H. I. Grapek    04-Jan-1994     Original
#       H. I. Grapek    07-Jan-1994     Added comments, start of t_cdriver
#       H. I. Grapek    24-Jan-1994     Added a couple of questions.
#       K. J. Stickney  07-Feb-1994     Fixed the multi-host, server mode.
#       K. J. Stickney  07-Feb-1994     Added query for t_cdriver check/report
#                                       mode.
#       K. J. Stickney  10-Feb-1994     Added code to retry for input when
#                                       null string is input for Remote Host
#                                       Name or Remote Host Packet Version.
#       K. J. Stickney  06-May-1994     Changes to evocation of shell scripts
#                                       for execution on AIX platforms.
#       K. J. Stickney  14-Jun-1994     Fixes for Release 2.1 of the Toolkit
#       K. J. Stickney  04-Aug-1994     Added logic to query for access 
#                                       control user id if t_cdriver run in
#                                       report mode.
#       K. J. Stickney  15-Aug-1994     Changed current toolkit exec directory 
#                                       to CDKBIN from CDK, because CDK is
#                                       usually set to the current toolkit
#                                       directory
#       S. L. Siao      15-Apr-2002     Changed ps for Solaris 2.8
#                                       updated for release 2.2
#      Mitch Black      28-Dec-2004     Added ACSAPI_SSI_SOCKET setting for
#                                       SSI and t_cdriver use, plus comments.
#      Mitch Black      23-Feb-2005     Fix echo without newline (Linux).
#                                       Update version output at beginning.
#                                       Remove useless artifact "setenv".
#                                       Fix "ps" to work under Linux.
#      Mitch Black      05-Mar-2005     Add "real_server" values to pass this
#                                       info to t_cdriver for correct operation.
#      Joseph Nofi      15-Jul-2011     Added ./t_srvcommon.sh invocation.
#      Joseph Nofi      18-Oct-2012     Changed version, and trace and log defaults.
#

#
# Do some initializing.
#
self=`basename $0`;
version="2.4";
# Create an alias so Linux recognizes the \c for no-newline on echo.
if [ `uname` = "Linux" ]; then
    alias echo="echo -e"
fi

#clear the screen
clear;

echo
echo "             ***** Welcome to $self, Version $version *****"
echo 
echo "    This is the automated tester and startit script for the TOOLKIT."
echo "    Simply answer the questions which follow."
echo

#
#  Assign path to the executables if not already defined
#  If not defined, set up to be the current dir.
#
if [ "$CDK" = "" ]; then
    CDKBIN=`pwd`
    export CDKBIN;
else
    CDKBIN=$CDK/bin
fi;

echo "    Executables live in: $CDKBIN"
echo ""

T_KILL_FILE=$CDKBIN/t_kill_file  
export T_KILL_FILE

#
# Set up the environment for the ssi
#
CSI_TCP_RPCSERVICE="TRUE";    export CSI_TCP_RPCSERVICE
CSI_UDP_RPCSERVICE="TRUE"; export CSI_UDP_RPCSERVICE
CSI_CONNECT_AGETIME=172800;   export CSI_CONNECT_AGETIME
CSI_RETRY_TIMEOUT=4;    export CSI_RETRY_TIMEOUT
CSI_RETRY_TRIES=5;      export CSI_RETRY_TRIES
ACSAPI_SSI_SOCKET=50004         export ACSAPI_SSI_SOCKET

#
# Turn on tracing in the SSI
#
#TRACE_VALUE=800000;     export TRACE_VALUE
CDKTRACE=00000000000
export CDKTRACE
CDKLOG=0000000000
export CDKLOG

#
# Initializations
#

server_hosting=0;
real_server=0;

#
#  Ask if Multi Host testing or Single Host Testing.
#  and do the appropriate setup.
#
while [ 1 -eq 1 ]; do
    echo "Would you like Multi-Host or Single-Host testing?"
    echo 
    echo "    Enter one of the following followed by ENTER: "
    echo 
    echo "    M   Multi-host testing"
    echo "    S   Single-host testing"
    echo "    X   eXit this script"
    echo
    echo "Enter choice: \c"
    read answer;
    echo ""

    case $answer in
   M | m )  # Setting up for multi-host testing...

            while [ 1 -eq 1 ]; do
                echo "Would you like to define the server side or client side"
           echo "for Multi-Host testing?"
           echo 
              echo "    Enter one  of the following followed by ENTER:"
           echo 
              echo "    S   Server side"
              echo "    C   Client side"
                echo
                echo "Enter choice: \c"
                read answer2;
                echo ""
    
                case $answer2 in
               S | s )  # Deal with the server side
                        server_hosting=1;
         break;
         ;;

          C | c )    # deal with the client side 
                        server_hosting=0;
         break;
                   ;;
            
               * )
                   echo; echo "Please Enter 'S' or 'C'... try again. "
            echo
            ;;
         esac;
       done;

       #
       # at this time, we know whether we are doing server or
       # client hosting.
       #

            if [ $server_hosting = 1 ]; then
         # we are server hosting.... client is on another machine.

           # set up paths to executables and startup scripts
           PARENT=$CDKBIN/t_parent;       export PARENT 
           MINI_EL=$CDKBIN/mini_el;       export MINI_EL
           SRVTRCS=$CDKBIN/srvtrcs;       export SRVTRCS
           SRVLOGS=$CDKBIN/srvlogs;       export SRVLOGS
           THTTP=$CDKBIN/t_http;          export THTTP
           CSI=$CDKBIN/csi;               export CSI
           ACSLM=$CDKBIN/t_acslm;         export ACSLM
    
           echo; echo "Starting $MINI_EL..."
           ./t_mini_el.sh 
           ./t_srvcommon.sh 
    
           # Don't allow any interrupts while starting the deamons.
           trap '' 1 2 3 15
    
           echo; echo "Starting $ACSLM..."
           ./t_acslm.sh

           echo; echo "Starting $CSI..."
           ./t_csi.sh
    
           # Clear the trap on signals.
           trap 1 2 3 15

       else
         # we are client hosting.... server is on another machine.

                while [ 1 -eq 1 ]; do
               echo "The Remote Host Name is the name of the server which"
               echo "has the ACSLS software (or simulator) running on it."
               echo
               echo "Enter Remote Host Name (CSI_HOSTNAME): \c";
               read Host_Name;
          if [ $Host_Name ]; then
                        CSI_HOSTNAME=$Host_Name;   export CSI_HOSTNAME
         break;
                    else
         echo 
         echo "No Host Name entered, try again."
         echo 
                    fi;
                done;

                while [ 1 -eq 1 ]; do
                    echo
                    echo "Is that host running a real ACSLS (or LibStation) server,"
               echo "or a Simulated Test server (t_acslm)?"
               echo 
                  echo "    Enter one  of the following followed by ENTER:"
               echo 
                  echo "    R   Real ACSLS or LibStation Server"
                  echo "    S   Simulated Test Server"
                    echo
                    echo "Enter choice: \c"
                    read servertype;
                    echo ""
    
                    case $servertype in
                   R | r ) # Real ACSLS or LibStation server
                            real_server=1;
             break;
             ;;

              S | s ) # Simulated (t_acslm) server
                            real_server=0;
             break;
                       ;;
            
                   * )
                       echo; echo "Please Enter 'R' or 'S'... try again. "
                echo
                ;;
             esac;
           done;

                while [ 1 -eq 1 ]; do
               echo
               echo "  The Remote Host Version number is the ACSLS Packet"
               echo "  Version level which the server is expecting."
               echo 
               echo "  Here are the valid choices:"
               echo
               echo "  ACSLS Release Number     Remote Host Version Number"
               echo 
               echo "          3.x                         2"
               echo "          4.x                         3"
               echo "        5.x, 6.x                      4"
               echo 
               echo "Enter Remote Host Version (ACSAPI_PACKET_VERSION): \c";
               read Ver_Num;
                    if [ $Ver_Num ]; then
                       if [ $Ver_Num -le 4 -a $Ver_Num -ge 2 ]; then
                       ACSAPI_PACKET_VERSION=$Ver_Num; \
                       export ACSAPI_PACKET_VERSION
                       break;
                       else
                          echo
                          echo "Invalid Host Version Number entered, try again."
                          echo
                       fi;
                    else
                       echo
                       echo "No Remote Host Version Number entered, try again."
                       echo
                    fi;
                done;
      
        
               # set up paths to executables and startup scripts
           PARENT=$CDKBIN/t_parent;       export PARENT 
           MINI_EL=$CDKBIN/mini_el;       export MINI_EL
           SRVTRCS=$CDKBIN/srvtrcs;       export SRVTRCS
           SRVLOGS=$CDKBIN/srvlogs;       export SRVLOGS
           THTTP=$CDKBIN/t_http;          export THTTP
           SSI=$CDKBIN/ssi;               export SSI
    
           echo; echo "Starting $MINI_EL..."
           ./t_mini_el.sh 
           ./t_srvcommon.sh 
    
           # Don't allow any interrupts while starting the deamons.
           trap '' 1 2 3 15
    
           echo; echo "Starting $SSI..."
           ./t_ssi.sh
    
           # Clear the trap on signals.
           trap 1 2 3 15

       fi;

       break;
       ;;

   S | s )  # Setting up for SINGLE-HOST Testing...

            real_server=0;
            CSI_HOSTNAME=`hostname`;      export CSI_HOSTNAME

       # set up paths to executables and startup scripts
       PARENT=$CDKBIN/t_parent;     export PARENT 
       MINI_EL=$CDKBIN/mini_el;     export MINI_EL
       SRVTRCS=$CDKBIN/srvtrcs;     export SRVTRCS
       SRVLOGS=$CDKBIN/srvlogs;     export SRVLOGS
       THTTP=$CDKBIN/t_http;        export THTTP
       SSI=$CDKBIN/ssi;             export SSI
       CSI=$CDKBIN/csi;             export CSI
       ACSLM=$CDKBIN/t_acslm;       export ACSLM

       echo; echo "Starting $MINI_EL..."
       ./t_mini_el.sh 
       ./t_srvcommon.sh 

       # Don't allow any interrupts while starting the deamons.
       trap '' 1 2 3 15

       echo; echo "Starting $ACSLM..."
       ./t_acslm.sh

       echo; echo "Starting $CSI..."
       ./t_csi.sh

       echo; echo "Starting $SSI..."
       ./t_ssi.sh

       # Clear the trap on signals.
       trap 1 2 3 15

       break;
       ;;

   X | x )
       echo "    Exiting... bye bye..."
       exit 0;
       break;
       ;;

   * )
       echo; echo "Please Enter 'M', 'S' or 'X'... try again. "
       echo
       ;;
    esac;
done;

echo; echo "Initialization Done."


#
# Do a ps and show the user the Processes created.
#
sleep 5
echo
echo "Do you want to see the processes created? (Y or N): \c"
read Answer;
echo
case $Answer in
    y | Y )
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

        $PS | grep $CDKBIN;
   break;
   ;;
    
    * )
   break;
   ;;
esac
#
# Set up to run a driver.
#

echo
if [ $server_hosting = 0 ]; then
    echo "Do you want to start up t_cdriver? (Y or N): \c"
    read Answer;
    echo
    
    case $Answer in
        y | Y )
            while [ 1 -eq 1 ]; do
                echo "Would you like t_cdriver to check server responses"
           echo "or simply report responses?"
           echo 
           echo "Checking is optional for the simulated server t_acslm."
           echo "Reporting is STRONGLY suggested for a real ACS server."
           echo 
              echo "    Enter one  of the following followed by ENTER:"
           echo 
              echo "    C   Response Checking"
              echo "    R   Response Reporting"
                echo
                echo "Enter choice: \c"
                read answer3;
                echo ""
    
                case $answer3 in
               R | r )  # Passive reporting
                        echo "Would you like to specify an access control user id?"
                   echo "  Enter user id character string or <CR> for none."
                        echo "Enter choice: \c"
                        read answer4;
                        if [ $answer4 ];  then 
                            ACSAPI_USER_ID=$answer4; export ACSAPI_USER_ID
                        else 
                            ACSAPI_USER_ID=" "; export ACSAPI_USER_ID
                        fi;

                        if [ $real_server -eq 1 ];  then
                            ./t_cdriver -r ;
                        else
                            ./t_cdriver ;
                        fi;

         break;
         ;;

          C | c )    # Checking responses 
                        if [ $real_server -eq 1 ];  then
                            ./t_cdriver -cr ;
                        else
                            ./t_cdriver -c ;
                        fi;

         break;
                   ;;
            
               * )
                   echo; echo "Please Enter 'R' or 'C'... try again. "
            echo
            ;;
         esac;
       done;
       break;
       ;;
    
        * )
            echo "If you start up t_cdriver outside of this script,"
            echo "you must set the following environment variables"
            echo "prior to launching t_cdriver:"
            echo
            echo "ACSAPI_PACKET_VERSION <The max ACSLS server packet version>"
            echo "ACSAPI_USER_ID <control access id or arbitrary string>"
            echo "ACSAPI_SSI_SOCKET <IPC socket for ACSAPI-SSI communication>"
       break;
       ;;
    esac
fi

#
# Done
#
exit 0;
