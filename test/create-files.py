#!/usr/bin/env python -t
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
import os
import stat
import subprocess
import sys
import shutil
from stat import *
import time
import json

basedir='/samst/'

def mkfile(file, size) :
    chunk = 1024
    loopto = size // chunk
    filler = size % chunk

    bite = bytearray(chunk)

    try :
        f = open(file, "wb")
        for n in range(loopto) :
            f.write(bite)

        if filler > 0 :
            f.write(bytearray(filler))

    finally:
        f.close()

def installFiles(filelst):
    for f,s in filelst:
        f = basedir+f
        try:
            print "create %s" % f
            if f.rpartition('/')[0] != None:
                os.stat(f.rpartition('/')[0])
            mkfile(f,s)
#        except OSError, e:
#            if e.errno == 2:  # No such directory
#                os.makedirs(f.rpartition('/')[0])
#                mkfile(f,s)
        except IOError, e:
            if not e.errno in (13,): 
                print "Unable to create file. %s %d:%s" % (f,e.errno,os.strerror(e.errno))
            
def main():
    filelst = []
    for i in range(1024):
        filelst.append(('lfile{0}'.format(i),i*1024*1024))
    installFiles(filelst)

main()
