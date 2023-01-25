#!/usr/bin/python3 -t
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
# Copyright 2013 Carsten Grzemba. All rights reserved.
# Use is subject to license terms.
# 
# How to use:
# * cd ../ && gmake SAMQFS_VERSION=5.0.x && gmake SAMQFS_VERSION=5.0.x install
#

import os
import stat
import subprocess
import sys
import shutil
from stat import *
import time
import json
import argparse
import jinja2
import logging

logging.basicConfig(format='%(levelname)s:%(message)s')
logger = logging.getLogger(__name__)

subpath64,subpath = subprocess.check_output('isainfo').decode('latin1').split()
# have to provide the same like: uname -v | gawk '{ split($0,a,"[.-]"); print a[1]"-"a[2]}'
osrelease = '-'.join(subprocess.check_output(['uname','-v']).decode('latin1').strip().replace('.','-').split('-')[:2])

version = '5.0.1'
release = '2022.0.0.0'
fmri = 'system/samqfs'
repro = 'file:///home/grzemba/samfs/github/samqfs/repo/'+subpath+'-'+osrelease

prefix = 'opt/SUNWsamfs/'
docdir = prefix+'doc/'
sysconfdir = 'etc/'+prefix
localstatedir = 'var/'+prefix

lic_fn = docdir+'OPENSOLARIS.LICENSE'
config_smf_name = 'samqfs-postinstall'
config_script_fn = '{0}util/{1}'.format(prefix,config_smf_name)
config_smf_fn='var/svc/manifest/system/{0}.xml'.format(config_smf_name)

destdir = 'root_i386/'
builddate = time.strftime('%Y%m%dT%H%M%SZ')

if 'omnios' in osrelease:
  publisher = 'samqfs.omnios'
  release = subprocess.check_output(['uname','-v']).decode('latin1').strip().replace('.','-').split('-')[1][1:]+'.0'
else:
  publisher = 'openindiana.org'

scmds = ['sls',
            'sfind',
            'sdu',
            'star']
sam_cmds = ['sam-arcopy',
            'sam-arfind',
            'sam-dbupd',
            'sam-fsalogd',
            'sam-nrecycler',
            'sam-recycler',
            'sam-releaser',
            'sam-rftd',
            'sam-shared',
            'sam-sharefsd',
            'sam-stk_helper',
            'sam-fsd']
samcmds = ['samchaid',
            'samexport',
            'samfsconfig',
            'samimport',
            'samload',
            'samquota',
            'samquotastat',
            'samsharefs',
            'samunhold',
            ]
isa32_cmds = {'fsck':'samfsck',
            'mkfs':'sammkfs',
            'trace':'samtrace'}
isa64_cmds = {'fsck':'samfsck',
            'mkfs':'sammkfs',
            'trace':'samtrace'}

# isa32only_cmds = ['mount','umount']
# used by devfsadm
isa32only_cmds = ['libsamaio_link.so']

not_bins = []
notfounds = []
arch64bins = []
srcpaths = {}
multiplefn = ['Makefile',]

def customizeTemplates(fname, config_parameters):
    template_file = fname+'.j2'
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="."),
                         trim_blocks=True,
                         lstrip_blocks=True)
    template = env.get_template(template_file)
    return template.render(**config_parameters)

def isScript(fn):
    p = subprocess.Popen(['/usr/bin/file',os.path.join(fn)], stdout=subprocess.PIPE)
    for l in p.stdout.readlines():
        if 'script' in l.decode('latin1'):
            return True
#    print "no script "+fn
    return False

''' returns the full path where to copy from
'''
def getPath(fn, dirname, filenames, arch):
    logger.debug ("getPath search: %s in %s",fn,dirname)
    # import pdb; pdb.set_trace()
    sfn = fn.rpartition('/')[2]

    if not sfn in filenames: return None
    # if fn == 'mount':
    #       import pdb; pdb.set_trace()
    if not arch == 'all' :
        if dirname.endswith(obj_dir[arch]):
            ffn = os.path.join(dirname,sfn)
            print ("found bin   : "+ffn)
            return ffn
        elif isScript(os.path.join(dirname,sfn)):
            ffn = os.path.join(dirname,sfn)
            print ("found script: "+ffn)
            return ffn
    elif dirname.endswith(fn.rpartition('/')[0]):
        ffn = os.path.join(dirname,sfn)
        print ("found other/: "+ffn)
        return ffn
    else:
        ffn = os.path.join(dirname,sfn)
        print ("found other : "+ffn)
        return None


def mkDirs(dirlst):
    for dir,uid,gid in dirlst:
        try: 
            os.makedirs(destdir+dir)
        except OSError as e:
            if e.errno == 17: # dir exists 
                pass
            else:
                print ("Unable to make dir  %d:%s" % (e.errno,os.strerror(e.errno)))
                raise OSError(e)
#         if uid == '?' or gid == '?':
#             print ("weiss nicht")
#             for l in subprocess.Popen(['ls','-ld','/'+dir],stdout = subprocess.PIPE).stdout.readlines():
#                 uid = l.split()[2]
#                 gid = l.split()[3]
#             print ("get {0} {1} {2}".format(destdir+dir, uid, gid))
#         os.system('sudo chown {1}:{2} {0}'.format(destdir+dir, uid, gid))
    
def installFiles(filelst):
    for f,m in filelst:
        found = False
        arch = 'all'
        fn = f.rpartition('/')[2]
        # if fn == 'SUNW_samaio_link.so':
        #      import pdb; pdb.set_trace()
        path = f.rpartition('/')[0]
        afn = fn
        # we start with the file name like it will installed, the build contains other names, without starting s, sam, sam- ...
        if fn in scmds:
            fn = fn[1:] # remove the s   
        if fn in sam_cmds:
            fn = fn[4:] # remove the sam-   
        if fn in samcmds:
            fn = fn[3:] # remove the sam
        # for those who follow not the above rules here a dictionary 
        if fn in SUNW_names.keys():
            fn = SUNW_names[fn]
        if afn in multiplefn:
            fn = path.rpartition('/')[2]+'/'+fn
#                afn = path.rpartition('/')[2]+'/'+afn
            path = path.rpartition('/')[0]

        # srcpath is a cache of already founded files
        if f not in srcpaths.keys():
            if m & 0o110 > 0 : # executable file
                if path.rpartition('/')[2] == '64':
                    logger.info ("isapath 64bit "+fn)
                    arch64bins.append(fn)
                    if fn in isa64_cmds.keys():
                        fn = isa64_cmds[fn]
                    arch = '64'
                    fn = path.rpartition('/')[2]+'/'+fn
#                        afn = path.rpartition('/')[2]+'/'+afn
                    path = path.rpartition('/')[0]
                    logger.info ("%s %s %s %s" % (subpath64, path, afn ,fn))
                elif path.rpartition('/')[2] == '32':
                    if isa == '64':
                        # skip
                       continue
                    logger.info ("isapath 32bit "+fn)
                    if fn in isa32_cmds.keys():
                        fn = isa32_cmds[fn]
                    arch = '32'
                    fn = path.rpartition('/')[2]+'/'+fn
#                        afn = path.rpartition('/')[2]+'/'+afn
                    path = path.rpartition('/')[0]
                    logger.info ("%s %s %s %s" % (subpath, path, afn ,fn))
                else:
                    if fn in isa32only_cmds:
                         arch = '32'
                    else:
                         arch = isa
                    logger.info ("%s: %sbit bin or script" % (f, arch))
            logger.debug ("search {0} {1} {2}".format(fn, oct(m), arch))
            if not arch == 'all':
                for dirname, dirnames, filenames in os.walk('../lib'):
                   src0 = getPath(fn, dirname, filenames, arch)
                   if src0 is not None:
                       found = True
                       break
            if not found:
               for dirname, dirnames, filenames in os.walk('../src'):
                   logger.debug ("d {0}, ds {1}, fn {2}".format(dirname, dirnames, filenames))
                   src0 = getPath(fn, dirname, filenames, arch)
                   if src0 is not None:
                      found = True
                      break
            if not found and arch == 'all':
               for dirname, dirnames, filenames in os.walk('../include'):
                   src0 = getPath(fn, dirname, filenames, arch)
                   if src0 is not None:
                      found = True
                      break
            if not found:
                notfounds.append(fn)
            else: 
                srcpaths[f] = src0
                logger.info ("caching [%s] %s" % (f,src0))
        else:
            src0 = srcpaths[f]
            found = True
        if not found: 
            continue
        try:
            f = f.replace('64', subpath64).replace('32', subpath)
            path = path.replace('64', subpath64).replace('32', subpath)
            if subpath64 in src0.rpartition('/')[0].rpartition('/')[2]:
                if path.endswith('lib'):
                    path += '/'+subpath64
                    logger.info("insert %s in path %s", subpath64, path)
                if f.split('/')[-2::][0] == 'lib':
                    f = '/'.join(f.split('/')[:-1:] + [subpath64] + f.split('/')[-1::])
                    logger.info("insert %s in file path %s", subpath64, f)
            if not os.path.isdir(destdir+path):
                os.makedirs(destdir+path)
            if os.path.isfile(destdir+f):
                os.chmod(destdir+f,stat.S_IWUSR)
            logger.info ("copy "+src0+" nach "+ destdir+f)
            shutil.copy(src0,destdir+f)
            os.chmod(destdir+f,m)
        except TypeError as e:
            logger.error ("Error: {0}".format(e))
            logger.error ("installFiles Exception on: %s" % f)
            import pdb; pdb.set_trace()
        except IOError as e:
            if not e.errno in (13,): 
                logger.error ("Unable to copy file. %s %d:%s" % (src0,e.errno,os.strerror(e.errno)))
            import pdb; pdb.set_trace()
        
def mkLinks(linklst):
    for lname, fname in linklst:
        lname = lname.replace('64', subpath64)
        fname = fname.replace('64', subpath64)
        logger.info (" lnk %s %s" % (destdir+lname,destdir+fname))
        try:
           os.link(destdir+fname, destdir+lname)
        except OSError as e:
           if e.errno == 2:
               logger.warning("skip mkLinks: %s %s",destdir+fname, destdir+lname)
           elif e.errno != 17:
               logger.error ("Error: {0} {1}".format(e.errno,os.strerror(e.errno)))
               logger.error ("mkLinks: %s %s",destdir+fname, destdir+lname)

def mkSymLinks(linklst):
    for lname, fname in linklst:
        logger.info (" slnk %s %s" % (destdir+lname,fname))
        try:
           os.symlink(fname, destdir+lname)
        except OSError as e:
           if e.errno != 17:
               logger.error ("Error: {0} {1}".format(e.errno,os.strerror(e.errno)))
               logger.error ("mkSymLinks: %s %s",fname, destdir+lname)

def publishPkg(version):
     if not '-' in version:
         version += '-'+release
     if os.system('pkg list $(cat depend-{}.lst) > /dev/null 2>&1'.format(osrelease.split('-')[0])) != 0:
         logger.info ('check content of depend-{}.lst'.format(osrelease.split('-')[0]))
         os.system('pkg list $(cat depend-{}.lst)'.format(osrelease.split('-')[0]))
         sys.exit(3)
     manifest_fn = 'samfs.p5m'
     with open(manifest_fn,'w') as mfn:
         mfn.write('set name=pkg.fmri value=pkg://{0}/{3}@{1}:{2}\n'.format(publisher,version,builddate,fmri))
     logger.info ('run: pkgsend generate {0} >> {1}'.format(destdir,manifest_fn))
     os.system('pkgsend generate {0} >> {1}'.format(destdir,manifest_fn))
     logger.info ('run: pkgdepend generate -md {0} {1} > {2}'.format(destdir,manifest_fn,manifest_fn+'d'))
     os.system('pkgdepend generate -md {0} {1} > {2}'.format(destdir,manifest_fn,manifest_fn+'d'))
     with open(transform_fn,'w') as tfn:
         tfn.write(transform)
     logger.info ('run: pkgmogrify -D builddate={0} {1} {2} > {3}'.format(builddate,manifest_fn+'d',transform_fn,manifest_fn+'d.trans'))
     os.system('pkgmogrify -D builddate={0} {1} {2} > {3}'.format(builddate,manifest_fn+'d',transform_fn,manifest_fn+'d.trans'))
     logger.info ('run: pkgdepend resolve -m -e depend-{1}.lst {0}'.format(manifest_fn+'d.trans', osrelease.split('-')[0]))
     if (os.system('pkgdepend resolve -m -e depend-{1}.lst {0}'.format(manifest_fn+'d.trans', osrelease.split('-')[0])) != 0):
        logger.error("pkgdepend resolve error: stop!")
        sys.exit(1)
     logger.info ('run: pkglint -c check -l {0} {1}'.format(repro,manifest_fn+'d.trans.res'))
     os.system('pkglint {1}'.format(repro,manifest_fn+'d.trans.res'))
     # os.system('pkglint -c check -l {0} {1}'.format(repro,manifest_fn+'d.trans'))
     os.system('pkgsend publish -d {2} -s {0} {1}'.format(repro, manifest_fn+'d.trans.res', destdir))

def main(version, debug_build, amd64):
    global arch64
    global arch32
    global srcpaths
    
    try:
        srcpaths = json.load(open(srcpath_fn))
    except IOError:
        pass
    except ValueError:
        pass
        
    arch64 = '_{0}'.format(subpath64)
    arch32 = '_{0}'.format(subpath)

    logger.debug ("suffix path64 %s, path32 %s", arch64, arch32)

    with open('file.lst') as fl:
        filelst = [ (files.split()[0],int(files.split()[1],8)) for files in fl.readlines() ]
    with open('slink.lst') as fl:
        slinklst = [ (files.split()[0].replace('64',subpath64),files.split()[1].replace('64',subpath64)) for files in fl.readlines() ]
    with open('link.lst') as fl:
        linklst = [ (files.split()[0],files.split()[1]) for files in fl.readlines() ]
    with open('dir.lst') as fl:
        dirlst = [ (files.split()[0].replace('64',subpath64).replace('32',subpath),files.split()[1],files.split()[2]) for files in fl.readlines() ]

#     import pdb; pdb.set_trace()
#    fnlst = {}
#    for f,m in filelst:
#        if f.rpartition('/')[2] not in fnlst.keys():
#            fnlst[f.rpartition('/')[2]] = f
#        else:
#            print ("WARNIG: same file names in different paths %s %s" % (f,fnlst[f.rpartition('/')[2]]))

    mkDirs(dirlst)
    installFiles(filelst)
    mkLinks(linklst)
    mkSymLinks(slinklst)
    with open(srcpath_fn,'w') as f:
        json.dump(srcpaths,f)

    defmask = os.umask(0o033)
    try:
        os.unlink(destdir+config_script_fn)
    except:
        pass
    with open(destdir+config_script_fn,'w') as csf:
        csf.write(config_script)
    os.chmod(destdir+config_script_fn, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH | stat.S_IXUSR)

    logger.debug ("64 bit bins")
    logger.debug (arch64bins)
    if notfounds:
        logger.error ("files not found")
        logger.error (notfounds)
        sys.exit(1)
     
#    os.umask(0233)
    try:
        cxf = open(destdir+config_smf_fn,'w')
    except IOError as e:
        if e.errno == 2:
            os.makedirs(destdir+config_smf_fn.rpartition('/')[0])
            cxf = open(destdir+config_smf_fn,'w')
        else:
            raise
    cxf.write(config_smf)
    cxf.close()
    os.umask(defmask)
    if (args.publish):
        publishPkg(version)
    

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--destdir', dest='destdir', default='./root_i386')
    parser.add_argument('--version', dest='version', default=version, help='tag build with this version')
    parser.add_argument('--repro', dest='repro', help='path to ips package repository')
    parser.add_argument('--debug_build', dest='debug_build', action="store_true", help='install DEBUG builts')
    parser.add_argument('--publish', dest='publish', action="store_true", help='publish IPS package')
    parser.add_argument('--verbose', dest='verbose', action="store_true", help='verbose')
    parser.add_argument('--debug', dest='debug', action="store_true", help='verbose')
    parser.add_argument('--64bit', dest='amd64', action="store_true", help='verbose')
    args = parser.parse_args()

    destdir = args.destdir+'/' 
    if args.verbose:
        logger.setLevel(logging.INFO)
    if args.debug:
        logger.setLevel(logging.DEBUG)

    if args.debug_build:
         FLAVOR = '_DEBUG'
    else:
         FLAVOR = ''
    obj_dir = {'32':'obj/SunOS_{0}_{1}{2}'.format(osrelease,subpath,FLAVOR),
               '64':'obj/SunOS_{0}_{1}{2}'.format(osrelease,subpath64,FLAVOR),
               'all': ''}
    if args.amd64:
        isa = '64'
        srcpath_fn = 'srcpath_{0}_{1}{2}.json.cache'.format(osrelease,subpath64,FLAVOR)
    else:
        isa = '32'
        srcpath_fn = 'srcpath_{0}_{1}{2}.json.cache'.format(osrelease,subpath,FLAVOR)
    SUNW_names = { 'SUNW_samst_link.so':'libsamst_link.so',
            'SUNW_samaio_link.so':'libsamaio_link.so',
            'samfsrestore':'csd',
            'sam-catserverd':'catserver',
            'sam-clientd':'rs_client',
            'sam-genericd':'generic',
            'sam-robotsd':'robots',
            'sam-rpcd':'rpc.sam',
            'sam-scannerd':'scanner',
            'sam-serverd':'rs_server',
            'sam-stagerd':'stager',
            'sam-stagerd_copy':'copy',
            'sam-stagealld':'stageall',
            'sam-stkd':'stk',
            'archive':'gencmd',
            'unarchive':'gencmd',
            'SUNWsamfs':'nl_messages.cat',
            'Makefile':'Makefile.inst',
            'version.h':'{}/pub/version.h'.format(obj_dir[isa]),
            }
    if args.publish and args.repro:
         try:
             pkgrepoout = subprocess.check_output(["pkgrepo","-s",args.repro,"list"])
             repro = args.repro
             logger.info("publish package in %s", repro)
         except subprocess.CalledProcessError as ret:
             logger.error("wrong repro path %s", args.repro)
             sys.exit(4)

    transform_fn = 'samqfs.transform'
    config_parameters = {'builddate': builddate, 
	'config_smf_name':config_smf_name , 
	'lic_fn':lic_fn , 
	'version':args.version , 
	'subpath':subpath , 
	'osrelease':osrelease }
    transform = customizeTemplates(transform_fn, config_parameters)

    config_parameters = {'config_script_fn':config_script_fn,
	'config_smf_name':config_smf_name }
    config_smf = customizeTemplates(os.path.basename(config_smf_fn), config_parameters)

    config_parameters = {'sysconfdir':sysconfdir,
	'localstatedir':localstatedir,
	'prefix':prefix,
	'version':args.version}
    config_script = customizeTemplates(os.path.basename(config_script_fn), config_parameters)

    main(args.version, args.debug_build, args.amd64)
