#!/usr/bin/env python -t -O
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
from os import stat, makedirs, path, strerror, errno
# import stat
import subprocess
import sys
import shutil
from stat import *
from random import randint
from struct import pack
import time
import json

basedir='/samtest/r2/'
fileno = 256

def mkfile(file, size) :
    chunk = 1024
    loopto = size // chunk
    filler = size % chunk

    try :
        f = open(file, "wb")
        for n in range(loopto) :
            bite = bytearray()
            for i in range (chunk):
                bite.extend(pack("B",randint(0,255)))
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
                stat(f.rpartition('/')[0])
            mkfile(f,s)
#        except OSError, e:
#            if e.errno == 2:  # No such directory
#                os.makedirs(f.rpartition('/')[0])
#                mkfile(f,s)
        except IOError, e:
            if not e.errno in (13,): 
                print "Unable to create file. %s %d:%s" % (f,e.errno,strerror(e.errno))
            
def main():
    filelst = []
    try:
        makedirs(basedir)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and path.isdir(basedir):
            pass
    for i in range(fileno):
        filelst.append(('lfile{0}'.format(i),i*1024*128))
    installFiles(filelst)

main()
