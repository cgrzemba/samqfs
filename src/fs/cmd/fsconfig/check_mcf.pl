#!/usr/bin/perl
#

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

# Check default mcf file. If it was autogenerated check
# to see if it's still valid; if it doesn't exist, generate one.
#

$os = `uname -s`;
chomp($os);
if ($os eq "SunOS") {
	$AK = "/bin/awk";
	$MS = "/usr/sbin/metastat";
	$num_didmodules = `/usr/sbin/modinfo | /bin/awk '\$6 \~ /^did\$/' |
	    /bin/wc -l`;
	$num_mdmodules = `/usr/sbin/modinfo | /bin/awk '\$6 \~ /^md_mirror\$/' |
	    /bin/wc -l`;
	$disks = "/dev/dsk/*";
	if ( $num_didmodules > 0 ) {
		$disks = "$disks /dev/did/dsk/*";
	}
	if ( $num_mdmodules > 0 ) {
		$mdsk =
		`$MS | $AK -F: '\$2 \~ /Mirror/ {printf("/dev/md/%s ", \$1)}'`;
		$disks = "$disks $mdsk"
	}
} else {
	$ENV{'PATH'} = "/usr/bin:" . $ENV{'PATH'};
	$disks = "/dev/sd*";
}
if ( -e "/etc/opt/SUNWsamfs/mcf" ) {
	if ($os eq "SunOS") {
		`cksum /etc/opt/SUNWsamfs/mcf > /tmp/mcf.cksum`;
		`diff /opt/SUNWsamfs/etc/mcf.cksum /tmp/mcf.cksum`;
	} else {
		`md5sum -c /opt/SUNWsamfs/etc/mcf.md5sum >& /dev/null`;
	}
	if (($? >> 8) != 0) {
		print "Non-autogenerated mcf file exists, leaving it alone\n";
	} else {
		# if update requested, generate one
		if ($ARGV[0] eq "update") {
			&generate_mcf();
		}
	}
	if ($os eq "SunOS") {
		`/bin/rm /tmp/mcf.cksum`;
	}
} else {
	# no mcf file exists generate one
	&generate_mcf();
}
exit(0);

sub generate_mcf {
	print "Auto generating mcf file\n";
	open(MCF, ">/etc/opt/SUNWsamfs/mcf") or do {
		print "Couldn't open /etc/opt/SUNWsamfs/mcf : $!\n";
		exit(0);
	};
	print MCF "#\n# This MCF file was auto generated using ";
	print MCF "/opt/SUNWsamfs/sbin/samfsconfig\n#\n";
	close MCF;
	`/opt/SUNWsamfs/sbin/samfsconfig $disks >> /etc/opt/SUNWsamfs/mcf`;
	if ($os eq "SunOS") {
		`cksum /etc/opt/SUNWsamfs/mcf > /opt/SUNWsamfs/etc/mcf.cksum`;
	} else {
		`md5sum /etc/opt/SUNWsamfs/mcf > /opt/SUNWsamfs/etc/mcf.md5sum`;
	}
}
