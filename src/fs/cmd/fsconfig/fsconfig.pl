#!/usr/bin/perl
# filter for fsconfig under linux
# given n items determines common base and fdisks it if its a block device
#
# $Revision: 1.18 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

use Time::Local;

%Month = ('Jan' => 0,
		  'Feb' => 1,
		  'Mar' => 2,
		  'Apr' => 3,
		  'May' => 4,
		  'Jun' => 5,
		  'Jul' => 6,
		  'Aug' => 7,
		  'Sep' => 8,
		  'Oct' => 9,
		  'Nov' => 10,
		  'Dec' => 11
		  );

$set = 0;
push @sets, [];
while (<>) {
	chomp;
	if ($_ eq "") {
		push @sets, [];
		$set++;
		next;
	} else {
		push @{$$sets[$set]}, $_;
		if (/Family Set/) {
			($family, $mon, $mday, $hour, $min, $sec, $year) =
				(/'(.*?)'\s+\S+\s+\S+\s+(.*?)\s+(.*?)\s+(\d+):(\d+):(\d+)\s(.*)/);
			$time = timelocal($sec,$min,$hour,$mday,$Month{$mon},
			    $year);
			if ($ftime{$family} < $time) {
				$ftime{$family} = $time;
				$fset{$family} = $set;
			}
		}
	}
}

$total = $set;
# mark which sets are good, check for overlapping equipment numbers
foreach $family (sort(keys(%fset))) {
	$set = $fset{$family};
	$size = @{$$sets[$set]};
	@data = @{$$sets[$set]};
	for ($j = 0; $j < $size; $j++) {
		next if $data[$j] =~ /\#/;
		@info = split(' ', $data[$j]);
		if ($info[0] eq ">") {
			$eqn = $info[2];
		} else {
			$eqn = $info[1];
		}
		if (exists($eq{$eqn})) {
			if ($eq{$eqn} ne $family) { # we have a conflict
				if ($ftime{$eq{$eqn}} < $ftime{$family}) {
					$ferr{$fset{$eq{$eqn}}} =
					    "# eliminated because of " .
					    "equipment number conflict " .
					    "with $family";
					push @eliminate, $eq{$eqn};
					$eq{$eqn} = $family;
				} else {
					$ferr{$set} = "# eliminated because " .
					    "of equipment number conflict " .
					    "with $eq{$eqn}";
					push @eliminate, $family;
					#
					# No need to check more on this
					# family set.
					#
					last;
				}
			}
		} else {
			$eq{$eqn} = $family;
		}
	}
}

# remove any offenders
foreach $elim (@eliminate) {
	delete $fset{$elim};
}

# print out the supposedly "good" family sets, mcf style
foreach $family (sort(keys(%fset))) {
	undef @list;
	$j = 0;
	$set = $fset{$family};
	$good{$set} = $set;
	$size = @{$$sets[$set]};
	@data = @{$$sets[$set]};
	for ($j = 0; $j < $size; $j++) {
		$_ = $data[$j];
		if (/^> /) {
			push @list, $_;
			@info = split(' ', $_);
			$group = $info[2];
			for (; $j < $size; $j++) {
				$_ = $data[$j];
				last if !/^> /;
				@info = split(' ', $_);
				if ($info[2] != $group) {
					&process(@list);
					undef(@list);
					$group = $info[2];
				}
				push @list, $_;
			}
			&process(@list);
			undef(@list);
		}
		if (!/^> /) {
			s/^((?:no|\/)dev.*\s+)\d+/$1 -/;
			s/^\/dev.*?\s(.*?\smm\s.*)/nodev $1/;
			print "$_\n";
		}
	}
	print "\n";
}

my $size = keys(%good);
if ($size >= $total) { # nothing left so don't print this
	exit 0;
}

print "#\n#\n# Older, duplicate, and conflicting family sets\n#\n#\n\n";
for ($j = 0; $j < $total; $j++) {
	next if (exists $good{$j});
	@data = @{$$sets[$j]};
	if (exists($ferr{$j})) {
		print "#\n$ferr{$j}\n";
	}
	foreach $line (@data) {
		$line =~ s/^((?:no|\/)dev.*\s+)\d+/$1 -/;
		if ($line !~ /^#/) {
			print "# $line\n";
		} else {
			print "$line\n";
		}
	}
	print "\n";
}

sub process {
	my @list = @_;
	my @info;
	my $length = 1024;
	my $common;
	my $item = "";
	my $i;
	my @dev_list;
	my @data;
	my %orig;
	my %parts;
	my $size;
	my $dev;
	my $arg;
	foreach $arg (@list) {
		@info = split(' ', $arg);
		$dev = $info[1];
		$size = $info[$#info];
		if (length($dev) < $length) {
			($item) = ($dev =~ /(\D+)\d*/);
			$length = length($item);
		}
		push @dev_list, $dev;
		$parts{$dev} = $size;
		$orig{$dev} = substr($arg, 2);
	}
	$common = 1;
	for ($i = 0; $i < @dev_list; $i++) {
		if ($dev_list[$i] !~ /$item/) {
			$common = 0;
			delete $dev_list[$i];
		}
	}
	if (! $common) {
		print "# No common block device, multipath device maybe?\n";
		foreach $arg (@list) {
			print"#\t$arg\n";
		}
	}
	if (! -b $item) {
		print "# device listed is not a block device [$item]\n";
		return;
	}

	#output format is device size
	chomp(@data = `/sbin/fdisk -l $item 2> /dev/null`);
	my $match = 0;
	my $pick = $dev_list[0];
	my $distance = 0;
	foreach $partition (@data[5 .. $#data]) {
		@info = split(' ', $partition,7);
		if ($parts{$info[0]}) {
			$orig{$info[0]} =~ s/^(\/dev.*\s+)\d+$/$1 -/;
			$orig{$info[0]} =~
			    s/^\/dev.*?\s(.*?\smm\s.*)/nodev $1/;
			if ($info[1] =~ /\D/) {
				if ($info[4] == $parts{$info[0]}) {
					$match = 1;
					print "$orig{$info[0]}\n";
					last;
				} else {
					my $diff = ($info[4] -
					    $parts{$info[0]});
					if ($diff > 0) {
						if (($distance == 0) ||
						    ($diff < $distance)) {
							$pick = $info[0];
							$distance = $diff;
						}
					}
				}
			} else {
				if ($info[3] == $parts{$info[0]}) {
					print "$orig{$info[0]}\n";
					$match = 1;
					last;
				} else {
					my $diff = ($info[3] -
					    $parts{$info[0]});
					if ($diff > 0) {
						if (($distance == 0) ||
						    ($diff < $distance)) {
							$pick = $info[0];
							$distance = $diff;
						}
					}
				}
			}
		}
	}
	if ($match == 0) {
		#print "# couldn't determine which device is the correct "
		#print "device\n";
		#print "# picking the smallest one that completely contains "
		#print "the fs blocks\n";
		$orig{$pick} =~ s/^(\/dev.*\s+)\d+$/$1 -/;
		print "$orig{$pick}\n";
	}
}
