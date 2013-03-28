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

version = '5.0'

prefix = 'opt/SUNWsamfs/'
docdir = prefix+'doc/'
sysconfdir = 'etc/'+prefix
localstatedir = 'var/'+prefix

lic_fn = docdir+'OPENSOLARIS.LICENSE'
config_smf_name = 'samqfs-postinstall'
config_script_fn = '{0}util/{1}'.format(prefix,config_smf_name)
config_smf_fn='var/svc/manifest/system/{0}.xml'.format(config_smf_name)
srcpath_fn = 'srcpath.json.cache'

pkgbase = 'root/'
publisher = 'private'
repro = 'file:///repo/private'
builddate = time.strftime('%Y%m%dT%H%M%SZ')

transform_fn = 'samqfs.transform'
transform = '''<transform pkg -> emit set pkg.description="Storage and Archive Manager File System">
<transform pkg -> emit set pkg.summary="SAM-FS">
<transform pkg -> emit set org.opensolaris.consolidation=sfe>
<transform pkg -> emit set info.classification="org.opensolaris.category.2008:System/File System">
<transform pkg -> emit set variant.opensolaris.zone=global>
<transform pkg -> emit set variant.arch=i386>
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
<transform pkg -> emit driver name=samioc perms="* 0600 root sys">
<transform pkg -> emit driver name=samaio perms="* 0600 root sys">
<transform pkg -> emit driver name=samst perms="* 0600 root sys">
<transform pkg -> emit driver name=samfs>
<transform pkg -> emit legacy arch=i386 category=system desc="Storage and Archive Manager File System" \
name="Sun SAM and Sun SAM-QFS software OI (root)" pkg=SUNWsamfsr variant.arch=i386 vendor="Open Source" version={3},REV={0} hotline="Open Source">
<transform pkg -> emit legacy arch=i386 category=system desc="Storage and Archive Manager File System" \
name="Sun SAM and Sun SAM-QFS software OI (usr)" pkg=SUNWsamfsu variant.arch=i386 vendor="Open Source" version={3},REV={0} hotline="Open Source">
<transform dir file link hardlink path=opt/.+/man(/.+)? -> default facet.doc.man true>
<transform file path=opt/.+/man(/.+)? -> add restart_fmri svc:/application/man-index:default>
<transform dir file link hardlink path=opt.*/include/.* -> default facet.devel true>
<transform dir file link hardlink path=opt.*/client/.* -> default facet.devel true>
<transform pkg -> emit license {2} license=lic_CDDL>
<transform file path=.*/{1}$ -> default mode 0744>
'''.format(builddate, config_smf_name, lic_fn, version)

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

if [ ! -d $ETCDIR]; then
    mkdir -p $SCRIPTS
    chmod -R 0755 $ETCDIR
    chgrp -R sys  $ETCDIR
    chown -R root $ETCDIR
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
        chmod 644 $SER
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
    try:
        sp,sfn = fn.split('/')
        if sp == dirname.rpartition('/')[2] and sfn in filenames:
            ffn = os.path.join(dirname,sfn)
#            print "found "+ffn
            return ffn
    except ValueError:
        if fn in filenames: 
            ffn = os.path.join(dirname,fn) 
            if arch == '64':
                if not arch64 in dirname.rpartition('/')[2]:
                    return None
            elif arch == '32': 
                if not arch32 in dirname.rpartition('/')[2]:
                    if not isScript(ffn):
                        return None
    #         print "found "+ffn
            return ffn
    return None


def mkDirs(dirlst):
    for dir,uid,gid in dirlst:
        try: 
            os.makedirs(pkgbase+dir)
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
#             print "get {0} {1} {2}".format(pkgbase+dir, uid, gid)
#         os.system('sudo chown {1}:{2} {0}'.format(pkgbase+dir, uid, gid))
    
def installFiles(filelst):
    for f,m in filelst:
        found = False
        arch = 'all'
        if f.split('/')[2] not in not_bins:
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
                afn = path.rpartition('/')[2]+'/'+afn
                path = path.rpartition('/')[0]

            if afn not in srcpaths.keys():
                if m & 0110 > 0 : # bin file
                    if path.rpartition('/')[2] == 'amd64':
                        print "isapath 64bit "+fn
                        arch64bins.append(fn)
                        if fn in isa64_cmds.keys():
                            fn = isa64_cmds[fn]
                        arch = '64'
                    elif path.rpartition('/')[2] == 'i386':
                        print "isapath 32bit "+fn
                        if fn in isa32_cmds.keys():
                            fn = isa32_cmds[fn]
                        arch = '32'
                    else:
                        print "32bit bin "+fn
                        arch = '32'
                print "search {0} {1} {2}".format(fn, oct(m), arch)
                for dirname, dirnames, filenames in os.walk('../lib'):
                   src0 = getPath(fn, dirname, filenames, arch)
                   if src0 is not None:
                       found = True
                       break
                if not found:
                   for dirname, dirnames, filenames in os.walk('../src'):
#                       print "\nA. d: {0}, ds {1}, fn {2}".format(dirname, dirnames, filenames)
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
                    srcpaths[afn] = src0
            else:
                src0 = srcpaths[afn]
            try:
                print "copy "+src0+" nach "+ pkgbase+path+'/'+afn
            # raise exception if not exist 
                os.stat(pkgbase+path)
                shutil.copy(src0,pkgbase+path+'/'+afn)
                os.chmod(pkgbase+path+'/'+afn,m)
            except OSError, e:
                if e.errno == 2:  # No such directory
                    os.makedirs(pkgbase+path)
                    shutil.copy(src0,pkgbase+path+'/'+afn)
                    os.chmod(pkgbase+path+'/'+afn,m)
            except IOError, e:
                if not e.errno in (13,): 
                    print "Unable to copy file. %s %d:%s" % (src0,e.errno,os.strerror(e.errno))
            
def mkLinks(linklst):
    for lname, fname in linklst:
        print " lnk %s %s" % (pkgbase+lname,fname)
        try:
           os.link(fname, pkgbase+lname)
        except OSError, e:
           if e.errno != 17:
               print "Error: {0} {1}".format(e.errno,os.strerror(e.errno))

def mkSymLinks(linklst):
    for lname, fname in linklst:
        print " lnk %s %s" % (pkgbase+lname,fname)
        try:
           os.symlink(fname, pkgbase+lname)
        except OSError, e:
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
        
    for out in subprocess.Popen(['/usr/bin/isainfo'], stdout=subprocess.PIPE).stdout.readlines():
      arch64 = '_{0}_'.format(out.split()[0])
      arch32 = '_{0}_'.format(out.split()[1])

    print arch64, arch32

    with open('file.lst') as fl:
        filelst = [ (files.split()[0],int(files.split()[1],8)) for files in fl.readlines() ]
    with open('slink.lst') as fl:
        slinklst = [ (files.split()[0],files.split()[1]) for files in fl.readlines() ]
    with open('link.lst') as fl:
        linklst = [ (files.split()[0],files.split()[1]) for files in fl.readlines() ]
    with open('dir.lst') as fl:
        dirlst = [ (files.split()[0],files.split()[1],files.split()[2]) for files in fl.readlines() ]

#    fnlst = {}
#    for f,m in filelst:
#        if f.rpartition('/')[2] not in fnlst.keys():
#            fnlst[f.rpartition('/')[2]] = f
#        else:
#            print "WARNIG: same file names in different paths %s %s" % (f,fnlst[f.rpartition('/')[2]])

    mkDirs(dirlst)
    installFiles(filelst)
    mkSymLinks(slinklst)
    mkLinks(linklst)
    with open(srcpath_fn,'w') as f:
        json.dump(srcpaths,f)

    defmask = os.umask(0033)
    with open(pkgbase+config_script_fn,'w') as csf:
        csf.write(config_script)

    print "64 bit bins"
    print arch64bins
    print "files not found"
    print notfounds
     
#    os.umask(0233)
    try:
        cxf = open(pkgbase+config_smf_fn,'w')
    except IOError, e:
        if e.errno == 2:
            os.makedirs(pkgbase+config_smf_fn.rpartition('/')[0])
            cxf = open(pkgbase+config_smf_fn,'w')
        else:
            raise
    cxf.write(config_smf)
    cxf.close()
    os.umask(defmask)
    
    manifest_fn = 'samfs.p5m'
    with open(manifest_fn,'w') as mfn:
        mfn.write('set name=pkg.fmri value=pkg://{0}/system/samfs@{1},5.11-0.151.1.7:{2}\n'.format(publisher,version,builddate))
    os.system('pkgsend generate {0} >> {1}'.format(pkgbase,manifest_fn))
    os.system('pkgdepend generate -md {0} {1} > {2}'.format(pkgbase,manifest_fn,manifest_fn+'d'))
    os.system('pkgdepend resolve -m {0}'.format(manifest_fn+'d'))
    with open(transform_fn,'w') as tfn:
        tfn.write(transform)
    os.system('pkgmogrify -D builddate={0} {1} {2} > {3}'.format(builddate,manifest_fn+'d.res',transform_fn,manifest_fn+'d.trans'))
    os.system('pkglint -c check -l {0} {1}'.format(repro,manifest_fn+'d.trans'))
   

main()
