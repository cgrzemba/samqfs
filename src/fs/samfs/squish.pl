#!/usr/bin/perl
# squish input output [temp_directory (needed for 2.6 kernels)]

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
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


if ((@ARGV != 2) && (@ARGV != 3)) {
	print "usage: $0 input_object_file output_object_file [temp_directory]\n";
	exit(0);
}
@objcopy = ("objcopy");
chomp(@syms = `nm -s ${ARGV[0]}`);
foreach $sym (@syms) {
	next if !($sym =~ /^\s*U/);
	($sym =~ s/^\s+U\s+(.+?)$/$1/);
	next if ($sym =~ /ia64_spinlock_contention/); # catch any listed symbol
#	next if ($sym !~ /printk/);
	($munged_sym = $sym)=~ s/^(.+?)(?:_R(?:smp_)*[a-fA-F0-9]+?)$/QFS_$1/;
	#($munged_sym = $sym)=~ s/^\s+U\s+(.+?)(?:_R(?:smp_)*[a-fA-F0-9]+?)$/QFS_$1/;
	if ($munged_sym ne $sym) {
		push @objcopy,"--redefine-sym", "$sym=$munged_sym";
	} else {
		if (!($sym =~ /^(qfs_|sam)/i)) {
			if ($sym ne "__this_module") {
				push @objcopy, "--redefine-sym", "$sym=QFS_$sym";
			}
		} else {
			# its already a reference into the kernel interface layer
		}

	}
#	print "working on [$munged_sym][$sym]\n";
}

if (@ARGV == 3) {
	$tmp = $ARGV[2]."/squish".$$;
	push @objcopy, @ARGV[0], $tmp;
	system(@objcopy);

	`objcopy --only-section=__versions $tmp ${tmp}__versions`;
	`dd if=${tmp}__versions of=${tmp}__vers bs=1 count=192`;
	`objcopy -N ____versions $tmp ${tmp}_novers`;
	`objcopy --add-section ____versions=${tmp}__vers ${tmp}_novers $ARGV[1]`;
} else {
	push @objcopy, @ARGV[0], @ARGV[1];
	system(@objcopy);
}

