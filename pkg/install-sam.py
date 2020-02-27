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
# Copyright 2013 Carsten Grzemba. All rights reserved.
# Use is subject to license terms.
# 
# How to use:
# * cd ../src && gmake
# * cd ../pkg && rm -rf root/* && install-sam.py
# * pkgrepo remove -s /repo/private samfs
# * pkgsend publish -s /repo/private -d root samfs.p5md.trans
#

import os
import stat
import subprocess
import sys
import shutil
from stat import *
import time
import json

subpath64,subpath = subprocess.check_output('isainfo').split()

version = '5.0.1'
release = '2020.0.0.1'
repro = 'file:///home/grzemba/samfs/samqfs/repo/'+subpath

prefix = 'opt/SUNWsamfs/'
docdir = prefix+'doc/'
sysconfdir = 'etc/'+prefix
localstatedir = 'var/'+prefix

lic_fn = docdir+'OPENSOLARIS.LICENSE'
config_smf_name = 'samqfs-postinstall'
config_script_fn = '{0}util/{1}'.format(prefix,config_smf_name)
config_smf_fn='var/svc/manifest/system/{0}.xml'.format(config_smf_name)

# have to provide the same like: uname -v | gawk '{ split($0,a,"[.-]"); print a[1]"-"a[2]}'
osrelease = '-'.join(subprocess.check_output(['uname','-v']).strip().replace('.','-').split('-')[:2])
srcpath_fn = 'srcpath_{}.json.cache'.format(osrelease)

obj_dir = {'32':'obj/SunOS_{0}_{1}_DEBUG'.format(osrelease,subpath), 
           '64':'obj/SunOS_{0}_{1}_DEBUG'.format(osrelease,subpath64),
           'all': ''}

destdir = 'root/'
publisher = 'samqfs'
builddate = time.strftime('%Y%m%dT%H%M%SZ')


transform_fn = 'samqfs.transform'
transform = '''<transform pkg -> emit set pkg.description="Storage and Archive Manager File System">
<transform pkg -> emit set pkg.summary="SAM-FS">
<transform pkg -> emit set org.opensolaris.consolidation=sfe>
<transform pkg -> emit set info.classification="org.opensolaris.category.2008:System/File System">
<transform pkg -> emit set variant.opensolaris.zone=global>
<transform pkg -> emit set variant.arch={4}>
<transform dir path=(var|etc|sys|kernel).* -> edit group bin sys >
<transform dir path=usr\/lib\/(fs|devfsadm).* -> edit group bin sys >
<transform dir path=^usr -> drop >
<transform dir path=^opt$ -> drop >
<transform dir path=lib -> drop>
<transform dir path=kernel -> drop>
<transform dir path=etc(/[a-z]+)?$ -> drop>
<transform dir path=etc/sysevent(/.+)?$ -> drop>
<transform dir path=var(/[a-z]+)?$ -> drop>
<transform dir path=var/(snmp|svc)(/.+)* -> drop>
<transform file path=(var|lib)/svc/manifest/.*\.xml$ -> add restart_fmri svc:/system/manifest-import:default>
<transform pkg -> emit driver name=samioc perms="* 0666 root sys">
<transform pkg -> emit driver name=samaio perms="* 0666 root sys">
<transform pkg -> emit driver name=samst perms="* 0666 root sys">
<transform pkg -> emit legacy category=system desc="Storage and Archive Manager File System" \
name="Sun SAM and Sun SAM-QFS software OI (root)" pkg=SUNWsamfsr vendor="Open Source" version={3},REV={0} hotline="Open Source">
<transform pkg -> emit legacy category=system desc="Storage and Archive Manager File System" \
name="Sun SAM and Sun SAM-QFS software OI (usr)" pkg=SUNWsamfsu vendor="Open Source" version={3},REV={0} hotline="Open Source">
<transform dir file link hardlink path=opt/.+/man(/.+)? -> default facet.doc.man true>
<transform file path=opt/.+/man(/.+)? -> add restart_fmri svc:/application/man-index:default>
<transform dir file link hardlink path=opt.*/include/.* -> default facet.devel true>
<transform dir file link hardlink path=opt.*/client/.* -> default facet.devel true>
<transform pkg -> emit license {2} license=lic_CDDL>
<transform file path=.*/{1}$ -> default mode 0744>
<transform depend -> edit fmri "(@[0-9\.]*)-[^ \\t\\n\\r\\f\\v]*" "\\1">
<transform depend fmri=pkg:/runtime/perl.* -> edit fmri "@[^ \\t\\n\\r\\f\\v]*" "">
'''.format(builddate, config_smf_name, lic_fn, version, subpath)

config_smf = '''<?xml version="1.0"?>
<!DOCTYPE service_bundle SYSTEM "/usr/share/lib/xml/dtd/service_bundle.dtd.1">
<service_bundle type='manifest' name='samqfs:{1}'>
<service
name='system/{1}'
    type='service'
    version='1'>
    <single_instance />
    <dependency
        name='fs-local'
        grouping='require_all'
        restart_on='none'
        type='service'>
            <service_fmri value='svc:/system/filesystem/local:default' />
    </dependency>
    <dependent
        name='samqfs_self-assembly-complete'
        grouping='optional_all'
        restart_on='none'>
        <service_fmri value='svc:/milestone/self-assembly-complete' />
    </dependent>
    <instance enabled='true' name='default'>
        <exec_method
            type='method'
            name='start'
            exec='/{0}'
            timeout_seconds='0'/>
        <exec_method
            type='method'
            name='stop'
            exec=':true'
            timeout_seconds='0'/>
        <exec_method
            type='method'
            name='refresh'
            exec=':true'
            timeout_seconds='0'/>
        <property_group name='startd' type='framework'>
            <propval name='duration' type='astring' value='transient' />
        </property_group>
        <property_group name='config' type='application'>
            <propval name='assembled' type='boolean' value='false' />
        </property_group>
    </instance>
</service>
</service_bundle>
'''.format(config_script_fn,config_smf_name)


config_script = '''#!/usr/bin/sh
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

. /lib/svc/share/smf_include.sh

if [ ! -z $SMF_FMRI ]; then
  assembled=$(/usr/bin/svcprop -p config/assembled $SMF_FMRI)
  if [ "$assembled" == "true" ] ; then
    exit $SMF_EXIT_OK
  fi
fi

PKG_INSTALL_ROOT='/'
INSLOG=$PKG_INSTALL_ROOT/tmp/SAM_install.log
ETCDIR=$PKG_INSTALL_ROOT/{0}
VARDIR=$PKG_INSTALL_ROOT/{1}
SCRIPTS=$ETCDIR/scripts
EXAMPLE=$PKG_INSTALL_ROOT/{2}examples
KERNDRV=$PKG_INSTALL_ROOT/kernel/drv
LAST_DIR=$PKG_INSTALL_ROOT/{0}samfs.old.last

if [ ! -f $KERNDRV/samst.conf ]; then
        cp $EXAMPLE/samst.conf $KERNDRV/samst.conf
else
        egrep -v 'Id:|Revision:' $KERNDRV/samst.conf > /tmp/$$.in1
        egrep -v 'Id:|Revision:' $EXAMPLE/samst.conf > /tmp/$$.in2
        diff /tmp/$$.in1 /tmp/$$.in2 > /dev/null
        if [ $? -ne 0 ]; then
                echo "samst.conf may have been updated for this release."
                echo "$EXAMPLE/samst.conf is the latest released version."
                echo "Please update this file with any site specific changes"
                echo "and copy it to $KERNDRV/samst.conf ."
                echo "When you have done this you may need to run"
                echo "\"/usr/sbin/devfsadm -i samst\" if any devices were added."
                echo " "
        fi
        rm /tmp/$$.in1 /tmp/$$.in2
fi

if [ ! -d $ETCDIR ]; then
    mkdir -p $ETCDIR
    chmod 0755 $ETCDIR
    chgrp sys  $ETCDIR
    chown root $ETCDIR
fi
if [ ! -d $SCRIPTS ]; then
    mkdir -p $SCRIPTS
fi
cp -rp $PKG_INSTALL_ROOT/{2}etc/csn $ETCDIR
cp -rp $PKG_INSTALL_ROOT/{2}etc/startup $ETCDIR


if [ ! -f $ETCDIR/inquiry.conf ]; then
        cp $EXAMPLE/inquiry.conf $ETCDIR/inquiry.conf
else
        egrep -v 'Id:|Revision:' $ETCDIR/inquiry.conf > /tmp/$$.in1
        egrep -v 'Id:|Revision:' $EXAMPLE/inquiry.conf > /tmp/$$.in2
        diff /tmp/$$.in1 /tmp/$$.in2 > /dev/null
        if [ $? -ne 0 ]; then
                echo "inquiry.conf may have been updated for this release."
                echo "$EXAMPLE/inquiry.conf is the latest released version."
                echo "Please update this file with any site specific changes and"
                echo "copy it to $ETCDIR/inquiry.conf ."
                echo " "
        fi
        rm /tmp/$$.in1 /tmp/$$.in2
fi

# Add SPM port to $PKG_INSTALL_ROOT/etc/inet/services

NSS=$PKG_INSTALL_ROOT/etc/nsswitch.conf
SER=$PKG_INSTALL_ROOT/etc/inet/services

# Alert user if /etc/services files is not used

SERVSRC=`grep -v "^#" $NSS | grep -w services | cut -f1 -d '[' | grep files`
if [ X"$SERVSRC" = X ]; then
        echo ""
        echo "WARNING: $PKG_INSTALL_ROOT/etc/nsswitch.conf does NOT"
        echo "reference $PKG_INSTALL_ROOT/etc/services.  If you are"
        echo "using nis, nis+, etc. for the services database you "
        echo "must install an entry for the SAM-QFS single port monitor"
        echo "there as follows:"
        echo ""
        echo "sam-qfs       7105/tcp            # SAM-QFS"
        echo ""
        echo "SAM-QFS will not run correctly until you correct this."
        echo ""
fi

SPMSRV=`getent services 7105 | sed -e 's/\([^ ]*\)[ ]*.*$/\1/'`
if [ -n "$SPMSRV" -a "$SPMSRV" != "sam-qfs" ]; then
        echo ""
        echo "WARNING: An active entry for port 7105 is already in $SER."
        echo "A different port number will need to be selected after"
        echo "installation for sam-qfs to run correctly."
        echo "Edit the $SER file and update the sam-qfs"
        echo "port number to a unique number and uncomment the line"
        echo ""
        echo "#sam-qfs          7105/tcp                        # SAM-QFS"
        echo ""
        chmod 444 $SER
elif [ "$SPMSRV" = "sam-qfs" ]; then
        echo ""
        echo "A sam-qfs entry already exists with port 7105. No changes"
        echo "are being made to the $SER file"
        echo ""
else
        echo "# start samqfs 5.0" >> $SER
        echo "sam-qfs           7105/tcp                        # SAM-QFS" >> $SER
        echo "# end samqfs 5.0" >> $SER
        chmod 444 $SER
fi

VERSION="VERSION.{3}"
/bin/touch $ETCDIR/$VERSION

SH_LIST="archiver.sh recycler.sh save_core.sh ssi.sh sendtrap nrecycler.sh"
for fn in $SH_LIST; do
        if [ ! -f $SCRIPTS/$fn ]; then
                if [ -f $EXAMPLE/$fn ]; then
                        cp $EXAMPLE/$fn $SCRIPTS/$fn
                fi
        else
                egrep -v 'Id:|Revision:' $SCRIPTS/$fn > /tmp/rev1.$$
                egrep -v 'Id:|Revision:' $EXAMPLE/$fn > /tmp/rev2.$$
                diff /tmp/rev1.$$ /tmp/rev2.$$ > /dev/null
                if [ $? -ne 0 ]; then
                        echo "$fn has been updated for this release."
                        echo "$EXAMPLE/$fn is the latest released version."
                        echo "Please update this file with any site specific changes"
                        echo "and copy it to $SCRIPTS/$fn ."
                        echo " "
                fi
                rm /tmp/rev1.$$ /tmp/rev2.$$
        fi
        if [ -f $SCRIPTS/$fn ]; then
                chmod 0750     $SCRIPTS/$fn
                chown root:bin $SCRIPTS/$fn
        fi
done

# Do NOT copy in default versions of the optional scripts if they don't exist.
# It is up to the site to do the copy if they want the script to be invoked.
# Notify the site if the customized versions have changed.
#
SH_LIST="dev_down.sh load_notify.sh log_rotate.sh"
for fn in $SH_LIST; do
        if [ -f $SCRIPTS/$fn ]; then
                /bin/egrep -v 'Id:|Revision:' $SCRIPTS/$fn > /tmp/rev1.$$
                /bin/egrep -v 'Id:|Revision:' $EXAMPLE/$fn > /tmp/rev2.$$
                /bin/diff /tmp/rev1.$$ /tmp/rev2.$$ > /dev/null
                if [ $? -ne 0 ]; then
                        echo "$fn has been updated for this release."
                        echo "$EXAMPLE/$fn is the latest released version."
                        echo "Please update this file with any site specific changes"
                        echo "and copy it to $SCRIPTS/$fn ."
                        echo " "
                fi
                /bin/rm /tmp/rev1.$$ /tmp/rev2.$$
        fi
done

# set up if necessary for the server to be managed by Sun Storedge SAM-QFS
# Manager

if [ ! -f $PKG_INSTALL_ROOT/{2}.fsmgmtd ]; then
if [ -f $PKG_INSTALL_ROOT/{2}.sam-mgmtrpcd ]; then
MGMMODE=`cat $PKG_INSTALL_ROOT/{2}.sam-mgmtrpcd`
        fi
        echo "$MGMMODE" > $PKG_INSTALL_ROOT/{2}.fsmgmtd
fi

# set up the clients that can manage this SAM-FS/QFS server via the SAM-QFS
# Manager

echo "$FSMADM_CLIENTS" > $PKG_INSTALL_ROOT/{2}.fsmgmtd_clients

svccfg -s $SMF_FMRI setprop config/assembled = true
svccfg -s $SMF_FMRI refresh
'''.format(sysconfdir,localstatedir,prefix,version)



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
            'sam-stkd',
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
            'archive':'gencmd',
            'unarchive':'gencmd',
            'SUNWsamfs':'nl_messages.cat',
            'Makefile':'Makefile.inst',
            }
isa32_cmds = {'fsck':'samfsck',
            'mkfs':'sammkfs',
            'trace':'samtrace'}
isa64_cmds = {'fsck':'samfsck',
            'mkfs':'sammkfs',
            'trace':'samtrace'}

not_bins = []
notfounds = []
arch64bins = []
srcpaths = {}
multiplefn = ['Makefile']

def isScript(fn):
    p = subprocess.Popen(['/usr/bin/file',os.path.join(fn)], stdout=subprocess.PIPE)
    for l in p.stdout.readlines():
        if 'script' in l:
            return True
#    print "no script "+fn
    return False

def getPath(fn, dirname, filenames, arch):
    # print "getPath search: "+fn
    sfn = fn.rpartition('/')[2]

    if not sfn in filenames: return None
#        if fn == 'svc-qfs-shared-mount' and dirname == '../src/fs/cmd/mount':
#            import pdb; pdb.set_trace()
    if not arch == 'all' :
        if dirname.endswith(obj_dir[arch]):
            ffn = os.path.join(dirname,sfn)
            print "found bin   : "+ffn
            return ffn
        elif isScript(os.path.join(dirname,sfn)):
            ffn = os.path.join(dirname,sfn)
            print "found script: "+ffn
            return ffn

    else:
        ffn = os.path.join(dirname,sfn)
        print "found other : "+ffn
        return ffn


def mkDirs(dirlst):
    for dir,uid,gid in dirlst:
        try: 
            os.makedirs(destdir+dir)
        except OSError,e:
            if e.errno == 17: # dir exists 
                pass
            else:
                print "Unable to make dir  %d:%s" % (e.errno,os.strerror(e.errno))
                raise OSError(e)
#         if uid == '?' or gid == '?':
#             print "weiss nicht"
#             for l in subprocess.Popen(['ls','-ld','/'+dir],stdout = subprocess.PIPE).stdout.readlines():
#                 uid = l.split()[2]
#                 gid = l.split()[3]
#             print "get {0} {1} {2}".format(destdir+dir, uid, gid)
#         os.system('sudo chown {1}:{2} {0}'.format(destdir+dir, uid, gid))
    
def installFiles(filelst):
    for f,m in filelst:
        found = False
        arch = 'all'
        # import pdb; pdb.set_trace()
        fn = f.rpartition('/')[2]
        path = f.rpartition('/')[0]
        afn = fn
        if fn in scmds:
            fn = fn[1:] # remove the s   
        if fn in sam_cmds:
            fn = fn[4:] # remove the sam-   
        if fn in samcmds:
            fn = fn[3:] # remove the sam
        if fn in SUNW_names.keys():
            fn = SUNW_names[fn]
        if afn in multiplefn:
            fn = path.rpartition('/')[2]+'/'+fn
#                afn = path.rpartition('/')[2]+'/'+afn
            path = path.rpartition('/')[0]

        if f not in srcpaths.keys():
            if m & 0110 > 0 : # executable file
                if path.rpartition('/')[2] == '64':
                    print "isapath 64bit "+fn
                    arch64bins.append(fn)
                    if fn in isa64_cmds.keys():
                        fn = isa64_cmds[fn]
                    arch = '64'
                    fn = path.rpartition('/')[2]+'/'+fn
#                        afn = path.rpartition('/')[2]+'/'+afn
                    path = path.rpartition('/')[0]
                    print "%s %s %s %s" % (subpath64, path, afn ,fn)
                elif path.rpartition('/')[2] == '32':
                    print "isapath 32bit "+fn
                    if fn in isa32_cmds.keys():
                        fn = isa32_cmds[fn]
                    arch = '32'
                    fn = path.rpartition('/')[2]+'/'+fn
#                        afn = path.rpartition('/')[2]+'/'+afn
                    path = path.rpartition('/')[0]
                    print "%s %s %s %s" % (subpath, path, afn ,fn)
                else:
                    print "%s: 32bit bin or script" % f
                    arch = '32'
            # print "search {0} {1} {2}".format(fn, oct(m), arch)
            if not arch == 'all':
                for dirname, dirnames, filenames in os.walk('../lib'):
                   src0 = getPath(fn, dirname, filenames, arch)
                   if src0 is not None:
                       found = True
                       break
            if not found:
               for dirname, dirnames, filenames in os.walk('../src'):
#                      print "\nA. d: {0}, ds {1}, fn {2}".format(dirname, dirnames, filenames)
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
                print "cache [%s] %s" % (f,src0)
        else:
            src0 = srcpaths[f]
        try:
            print "copy "+src0+" nach "+ destdir+f
        # raise exception if not exist 
            os.stat(destdir+path)
            shutil.copy(src0,destdir+f)
            os.chmod(destdir+f,m)
        except TypeError as e:
            print "installFiles Exception: %s" % f
            print "Error: {0}".format(e)
            import pdb; pdb.set_trace()
        except OSError, e:
            if e.errno == 2:  # No such directory
                os.makedirs(destdir+path)
                shutil.copy(src0,destdir+f)
                os.chmod(destdir+f,m)
        except IOError, e:
            if not e.errno in (13,): 
                print "Unable to copy file. %s %d:%s" % (src0,e.errno,os.strerror(e.errno))
        
def mkLinks(linklst):
    for lname, fname in linklst:
        print " lnk %s %s" % (destdir+lname,destdir+fname)
        try:
           os.link(destdir+fname, destdir+lname)
        except OSError as e:
           if e.errno != 17:
               print "Error: {0} {1}".format(e.errno,os.strerror(e.errno))

def mkSymLinks(linklst):
    for lname, fname in linklst:
        print " slnk %s %s" % (destdir+lname,fname)
        try:
           os.symlink(fname, destdir+lname)
        except OSError as e:
           if e.errno != 17:
               print "Error: {0} {1}".format(e.errno,os.strerror(e.errno))

def main():
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

    print arch64, arch32

    with open('file.lst') as fl:
        filelst = [ (files.split()[0],int(files.split()[1],8)) for files in fl.readlines() ]
    with open('slink.lst') as fl:
        slinklst = [ (files.split()[0],files.split()[1]) for files in fl.readlines() ]
    with open('link.lst') as fl:
        linklst = [ (files.split()[0],files.split()[1]) for files in fl.readlines() ]
    with open('dir.lst') as fl:
        dirlst = [ (files.split()[0].replace('64',subpath64),files.split()[1],files.split()[2]) for files in fl.readlines() ]

#    import pdb; pdb.set_trace()
#    fnlst = {}
#    for f,m in filelst:
#        if f.rpartition('/')[2] not in fnlst.keys():
#            fnlst[f.rpartition('/')[2]] = f
#        else:
#            print "WARNIG: same file names in different paths %s %s" % (f,fnlst[f.rpartition('/')[2]])

    mkDirs(dirlst)
    installFiles(filelst)
    mkLinks(linklst)
    mkSymLinks(slinklst)
    with open(srcpath_fn,'w') as f:
        json.dump(srcpaths,f)

    defmask = os.umask(0033)
    try:
	os.unlink(destdir+config_script_fn)
    except:
        pass
    with open(destdir+config_script_fn,'w') as csf:
        csf.write(config_script)
    os.chmod(destdir+config_script_fn, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH | stat.S_IXUSR)

    print "64 bit bins"
    print arch64bins
    if notfounds:
        print "files not found"
        print notfounds
        sys.exit(1)
     
#    os.umask(0233)
    try:
        cxf = open(destdir+config_smf_fn,'w')
    except IOError, e:
        if e.errno == 2:
            os.makedirs(destdir+config_smf_fn.rpartition('/')[0])
            cxf = open(destdir+config_smf_fn,'w')
        else:
            raise
    cxf.write(config_smf)
    cxf.close()
    os.umask(defmask)
    
    manifest_fn = 'samfs.p5m'
    with open(manifest_fn,'w') as mfn:
        mfn.write('set name=pkg.fmri value=pkg://{0}/system/samfs@{1}-{2}:{3}\n'.format(publisher,version,release,builddate))
    print ('run: pkgsend generate {0} >> {1}'.format(destdir,manifest_fn))
    os.system('pkgsend generate {0} >> {1}'.format(destdir,manifest_fn))
    print ('run: pkgdepend generate -md {0} {1} > {2}'.format(destdir,manifest_fn,manifest_fn+'d'))
    os.system('pkgdepend generate -md {0} {1} > {2}'.format(destdir,manifest_fn,manifest_fn+'d'))
    print ('run: pkgdepend resolve -m {0}'.format(manifest_fn+'d'))
    os.system('pkgdepend resolve -m -e depend.lst {0}'.format(manifest_fn+'d'))
    with open(transform_fn,'w') as tfn:
        tfn.write(transform)
    print ('run: pkgmogrify -D builddate={0} {1} {2} > {3}'.format(builddate,manifest_fn+'d.res',transform_fn,manifest_fn+'d.trans'))
    os.system('pkgmogrify -D builddate={0} {1} {2} > {3}'.format(builddate,manifest_fn+'d.res',transform_fn,manifest_fn+'d.trans'))
    print ('run: pkglint -c check -l {0} {1}'.format(repro,manifest_fn+'d.trans'))
    os.system('pkglint {1}'.format(repro,manifest_fn+'d.trans'))
    # os.system('pkglint -c check -l {0} {1}'.format(repro,manifest_fn+'d.trans'))
    os.system('pkgsend publish -d {2} -s {0} {1}'.format(repro, manifest_fn+'d.trans', destdir))
   

main()
