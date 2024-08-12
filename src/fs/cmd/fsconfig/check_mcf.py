#!/usr/bin/python3

'''
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
#�  Copyright (c) 2023 Carsten Grzemba
#

# Check default mcf file. If it was autogenerated check
# to see if it's still valid; if it doesn't exist, generate one.
#
'''

import subprocess as sb
import os
import sys
import pdb
from datetime import datetime

disks = ['/dev/dsk/*',]
disks = []

MCF_FN = '/etc/opt/SUNWsamfs/test_mcf'
mcf_header = '''
#
# This MCF file was auto generated using
# /opt/SUNWsamfs/sbin/samfsconfig {}
#
'''

def add_devs():
    for v in os.listdir('/dev/dsk/'):
        disks.append('/dev/dsk/'+v)
    p = sb.Popen(['/usr/sbin/zpool', 'list', '-Ho', 'name'], stdout=sb.PIPE, stderr=sb.PIPE)
    output, err = p.communicate()
    for zp in output.decode().split('\n'):
        if zp:
            for v in os.listdir('/dev/zvol/dsk/'+zp+'/'):
                disks.append('/dev/zvol/dsk/'+zp+'/'+v)

def generate_mcf():
    add_devs()
    add_ha_storage_disks()

    print("Auto generating mcf file")
    with open(MCF_FN,'a') as cf:
        cf.write(mcf_header.format(str(datetime.now().strftime('%a %b %d %H:%M:%S %Y'))))
        # pdb.set_trace()
        cmd = ['/opt/SUNWsamfs/sbin/samfsconfig',] + disks
        p = sb.Popen(cmd, stdout=sb.PIPE, stderr=sb.PIPE)
        output, err = p.communicate()
        if err:
            print (err, disks)
        else:
            cf.write(output.decode())
    out, err = sb.Popen(['/usr/bin/md5sum', MCF_FN], stdout=sb.PIPE, stderr=sb.PIPE).communicate()
    if err:
        print (err)
    else:
        with open(MCF_FN+'.md5sum',"w") as cf:
            cf.write(out.decode())

def add_ha_storage_disks():
    p = sb.Popen(['/usr/sbin/modinfo'], stdout=sb.PIPE, stderr=sb.PIPE)
    output, err = p.communicate()
    for line in output.decode().split('\n'):
        ml = line.split()
        if len(ml) > 4:
            if line.split()[5].startswith('did'):
                disks.append('/dev/did/dsk/*')

if os.path.isfile(MCF_FN):
    if sb.Popen(['/usr/bin/md5sum','-c',MCF_FN+'.md5sum'], stdout=sb.PIPE, stderr=sb.PIPE).wait():
        print("Non-autogenerated mcf file exists, leaving it alone\n")
    else:
        if sys.argv[1] == 'update':
            generate_mcf()
else: 
    generate_mcf()       